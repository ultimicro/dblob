#include "dispatcher.h"

#include <inttypes.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <sys/epoll.h>
#include <sys/eventfd.h> // TODO: check if this available
#include <sys/sysinfo.h> // TODO: check if this available
#include <unistd.h>

struct dispatcher {
	int fd;
	int stop;
};

struct loop_context {
	int epoll;
	int start;
	pthread_t *loops;
	size_t nloop;
	volatile bool failed;
};

static void setup_epoll_events(
	struct epoll_event *o,
	const struct event_source *s)
{
	o->events = EPOLLET | EPOLLONESHOT; // TODO: check if EPOLLONESHOT available

	if (s->read) {
		o->events |= EPOLLIN;
	}

	if (s->write) {
		o->events |= EPOLLOUT;
	}
}

static void start_loops(int event, int count)
{
	uint64_t v = count;

	if (write(event, &v, sizeof(v)) < 0) {
		perror("Failed to start event loops");
		abort();
	}
}

static void wait_for_start(int event)
{
	uint64_t v;

	if (read(event, &v, sizeof(v)) < 0) {
		perror("Failed to read event loop start signal");
		abort();
	}
}

static void *event_loop(void *param)
{
	struct loop_context *ctx = param;
	struct epoll_event events[100];

	// wait for start signal
	wait_for_start(ctx->start);

	if (ctx->failed) {
		return NULL;
	}

	// enter event loop
	for (;;) {
		// get events
		int num;
		bool stop;

		num = epoll_wait(ctx->epoll, events, 100, -1);

		if (num < 0) {
			perror("Failed to wait for events from epoll");
			ctx->failed |= true;

			if (kill(0, SIGTERM) < 0) {
				perror("Failed to stop event loops");
				abort();
			}

			break;
		}

		// process events
		stop = false;

		for (int i = 0; i < num; i++) {
			struct epoll_event *e = &events[i];
			const struct event_source *s = e->data.ptr;

			// check if shutdown signal has been received
			if (!s) {
				stop = true;
				continue;
			}

			// execute handlers
			if (e->events & EPOLLIN) {
				s->read();
			}

			if (e->events & EPOLLOUT) {
				s->write();
			}

			// re-enable file descriptor
			setup_epoll_events(e, s);

			if (epoll_ctl(ctx->epoll, EPOLL_CTL_MOD, s->fd, e) < 0) {
				perror("Failed to re-enabled file descriptor in epoll");
				abort();
			}
		}

		if (stop) {
			break;
		}
	}

	return NULL;
}

dispatcher_t dispatcher_create(void)
{
	struct dispatcher *d;
	struct epoll_event o;

	// allocate instance
	d = malloc(sizeof(struct dispatcher));

	if (!d) {
		perror("Failed to allocate epoll event dispatcher");
		return NULL;
	}

	// create epoll
	d->fd = epoll_create1(0);

	if (d->fd < 0) {
		perror("Failed to create epoll");
		goto free_instance;
	}

	// create shutdown signal
	d->stop = eventfd(0, 0);

	if (d->stop < 0) {
		perror("Failed to create shutdown event");
		goto close_epoll;
	}

	// add shutdown event to epoll
	memset(&o, 0, sizeof(o));

	o.events = EPOLLIN;

	if (epoll_ctl(d->fd, EPOLL_CTL_ADD, d->stop, &o) < 0) {
		perror("Failed to add shutdown event to epoll");
		goto close_shutdown;
	}

	return d;

	// clean up
close_shutdown:
	if (close(d->stop) < 0) {
		perror("Failed to close shutdown event");
	}

close_epoll:
	if (close(d->fd) < 0) {
		perror("Failed to close epoll");
	}

free_instance:
	free(d);

	return NULL;
}

void dispatcher_free(dispatcher_t d)
{
	// no need to remove shutdown event from epoll due to it will remove
	// automatically when closed
	if (close(d->stop) < 0) {
		perror("Failed to close shutdown event");
	}

	if (close(d->fd) < 0) {
		perror("Failed to close epoll");
	}

	free(d);
}

bool dispatcher_add(dispatcher_t d, const struct event_source *s)
{
	// setup epoll options
	struct epoll_event o;

	memset(&o, 0, sizeof(o));
	setup_epoll_events(&o, s);

	o.data.ptr = (void *)s;

	// add to epoll
	if (epoll_ctl(d->fd, EPOLL_CTL_ADD, s->fd, &o) < 0) {
		perror("Failed to add event source to epoll");
		return false;
	}

	return true;
}

bool dispatcher_del(dispatcher_t d, const struct event_source *s)
{
	if (epoll_ctl(d->fd, EPOLL_CTL_DEL, s->fd, NULL) < 0) {
		perror("Failed to remove file descriptor from epoll");
		return false;
	}

	return true;
}

void dispatcher_stop(dispatcher_t d)
{
	uint64_t v = 1;

	if (write(d->stop, &v, sizeof(v)) < 0) {
		abort();
	}
}

bool dispatcher_run(dispatcher_t d, dispatcher_ready_t ready, void *arg)
{
	int count = get_nprocs_conf();
	int start;
	struct loop_context ctx;
	pthread_t loops[count];

	// create start event
	start = eventfd(0, EFD_SEMAPHORE); // TODO: check if EFD_SEMAPHORE supported

	if (start < 0) {
		perror("Failed to create event loops start signal");
		return false;
	}

	// spawn event loops
	ctx.epoll = d->fd;
	ctx.start = start;
	ctx.loops = loops;
	ctx.nloop = count;
	ctx.failed = false;

	for (int i = 0; i < count; i++) {
		int res;

		printf("Spawning event loop #%d\n", i);

		res = pthread_create(&loops[i], NULL, event_loop, &ctx);

		if (res) {
			fprintf(stderr, "Failed to spawn the event loop: %d\n", res);
			count = i;
			ctx.failed = true;
			break;
		}
	}

	// start event loops
	start_loops(start, count);
	ready(arg);

	// wait all loops to stop
	for (int i = 0; i < count; i++) {
		int res = pthread_join(loops[i], NULL);

		if (res) {
			fprintf(stderr, "Failed to join event loop #%d: %d\n", i, res);
			abort();
		}

		printf("Event loop #%d stopped\n", i);
	}

	// clean up
	if (close(start) < 0) {
		perror("Failed to close event loops start signal");
	}

	return !ctx.failed;
}
