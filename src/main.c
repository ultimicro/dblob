#include "config.h"

#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>

#ifdef HAVE_EPOLL
#include <sys/epoll.h>
#else
#error no available event notifications system to use
#endif

#include <sys/eventfd.h> // TODO: check if this available
#include <sys/signalfd.h> // TODO: check if this available
#include <sys/sysinfo.h> // TODO: check if this available

struct context {
	int epoll;
	int start;
	int term;
	pthread_t *loops;
	uint64_t nloop;
	volatile bool failed;
};

static void wait_for_start(int event)
{
	uint64_t start;

	if (read(event, &start, sizeof(start)) < 0) {
		perror("Failed to read event loop start signal");
		abort();
	}
}

static void *event_loop(void *param)
{
	struct context *ctx = param;
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
			const struct epoll_event *e = &events[i];

			if (e->data.fd == ctx->term) {
				stop = true;
			}
		}

		if (stop) {
			break;
		}
	}

	return NULL;
}

static bool run_event_loops(int epoll, int shutdown, uint64_t count)
{
	int start;
	struct context ctx;
	pthread_t loops[count];

	// create start event
	start = eventfd(0, EFD_SEMAPHORE); // TODO: check if EFD_SEMAPHORE supported

	if (start < 0) {
		perror("Failed to create event loops start signal");
		return false;
	}

	// spawn event loops
	memset(&ctx, 0, sizeof(ctx));

	ctx.epoll = epoll;
	ctx.start = start;
	ctx.term = shutdown;
	ctx.loops = loops;
	ctx.nloop = count;
	ctx.failed = false;

	for (uint64_t i = 0; i < count; i++) {
		int res;

		printf("Spawning event loop %"PRIu64"\n", i);

		res = pthread_create(&loops[i], NULL, event_loop, &ctx);

		if (res) {
			fprintf(stderr, "Failed to spawn the event loop: %d\n", res);
			count = i;
			ctx.failed = true;
			break;
		}
	}

	// start event loops
	if (write(start, &count, sizeof(count)) < 0) {
		perror("Failed to start event loops");
		abort();
	}

	// wait all loops to stop
	for (uint64_t i = 0; i < count; i++) {
		int res = pthread_join(loops[i], NULL);

		if (res) {
			fprintf(stderr, "Failed to join event loop %"PRIu64": %d\n",
				i, res);
			abort();
		}

		printf("Event loop %"PRIu64" is stopped\n", i);
	}

	// clean up
	if (close(start) < 0) {
		perror("Failed to close event loops start signal");
	}

	return !ctx.failed;
}

static int listen_shutdown(void)
{
	sigset_t signals;
	int fd;

	// setup signal list
	if (sigemptyset(&signals) < 0) {
		perror("Failed to initialize a list of shutdown signals");
		return -1;
	}

	if (sigaddset(&signals, SIGINT) < 0) {
		perror("Failed to add SIGINT to a list of shutdown signals");
		return -1;
	}

	if (sigaddset(&signals, SIGTERM) < 0) {
		perror("Failed to add SIGTERM to a list of shutdown signals");
		return -1;
	}

	// TODO: check if flags supported
	fd = signalfd(-1, &signals, SFD_NONBLOCK);

	if (fd < 0) {
		perror("Failed to create signalfd to listen for shutdown signals");
		return -1;
	}

	return fd;
}

static bool run(int epoll)
{
	int shutdown;
	struct epoll_event event;
	bool success;

	// listen for shutdown signals
	shutdown = listen_shutdown();

	if (shutdown < 0) {
		return false;
	}

	memset(&event, 0, sizeof(event));

	event.events = EPOLLIN;
	event.data.fd = shutdown;

	if (epoll_ctl(epoll, EPOLL_CTL_ADD, shutdown, &event)) {
		perror("Failed to add shutdown signals listener to epoll");
		success = false;
	} else {
		// TODO: check if get_nprocs_conf() available
		success = run_event_loops(epoll, shutdown, get_nprocs_conf());

		// no need to remove shutdown from epoll due to it will remove
		// automatically when closed
	}

	// clean up
	if (close(shutdown) < 0) {
		perror("Failed to close shutdown signals listener");
	}

	return success;
}

static bool block_signals(void)
{
	sigset_t blocks;

	if (sigfillset(&blocks) < 0) {
		perror("Failed to initialize signal list to block");
		return false;
	}

	if (sigprocmask(SIG_BLOCK, &blocks, NULL) < 0) {
		perror("Failed to block signals");
		return false;
	}

	return true;
}

int main(int argc, char *argv[])
{
	int mon;
	bool success;

	printf("Starting "PACKAGE" "VERSION"\n");

	if (!block_signals()) {
		return EXIT_FAILURE;
	}

	// setup event notification
	mon = epoll_create1(0);

	if (mon < 0) {
		perror("Failed to create epoll");
		return EXIT_FAILURE;
	}

	// enter main loop
	success = run(mon);

	// clean up
	if (close(mon) < 0) {
		perror("Failed to close epoll");
	}

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
