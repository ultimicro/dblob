#include "config.h"

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#ifdef HAVE_EPOLL
#include <sys/epoll.h>
#else
#error no available event notifications system to use
#endif

#include <sys/signalfd.h> // TODO: check if this available

bool get_shutdown_signals(sigset_t *set)
{
	if (sigemptyset(set) < 0) {
		perror("Failed to initialize a list of shutdown signals");
		return false;
	}

	if (sigaddset(set, SIGINT) < 0) {
		perror("Failed to add SIGINT to a list of shutdown signals");
		return false;
	}

	if (sigaddset(set, SIGTERM) < 0) {
		perror("Failed to add SIGTERM to a list of shutdown signals");
		return false;
	}

	return true;
}

bool block_signals(void)
{
	sigset_t blocks;

	if (!get_shutdown_signals(&blocks)) {
		return false;
	}

	if (sigprocmask(SIG_BLOCK, &blocks, NULL) < 0) {
		perror("Failed to block SIGINT and SIGTERM");
		return false;
	}

	return true;
}

int listen_shutdown(void)
{
	sigset_t signals;
	int fd;

	if (!get_shutdown_signals(&signals)) {
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

bool run(int epoll)
{
	int shutdown;
	bool success;
	struct epoll_event events[100];

	// listen for shutdown signals
	shutdown = listen_shutdown();

	if (shutdown < 0) {
		return false;
	}

	memset(events, 0, sizeof(struct epoll_event));

	// TODO: check if EPOLLONESHOT supported
	events->events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	events->data.fd = shutdown;

	success = false;

	if (epoll_ctl(epoll, EPOLL_CTL_ADD, shutdown, events)) {
		perror("Failed to add shutdown signals listener to epoll");
	} else {
		// enter main loop
		for (;;) {
			// get events
			int num;
			bool stop;

			num = epoll_wait(epoll, events, 100, -1);

			if (num < 0) {
				perror("Failed to wait for events from epoll");
				break;
			}

			// process events
			stop = false;

			for (int i = 0; i < num; i++) {
				const struct epoll_event *e = &events[i];

				if (e->data.fd == shutdown) {
					stop = true;
				}
			}

			if (stop) {
				success = true;
				break;
			}
		}

		// no need to remove shutdown from epoll due to it will remove
		// automatically when closed
	}

	// clean up
	if (close(shutdown) < 0) {
		perror("Failed to close shutdown signals listener");
	}

	return success;
}

int main(int argc, char *argv[])
{
	int mon;
	bool success;

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
