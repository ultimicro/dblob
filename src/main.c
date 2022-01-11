#include "config.h"

#include "api.h"
#include "dispatcher.h"
#include "server.h"

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dispatcher_t dispatcher;
static bool shuttingdown;

static void ready(void *param)
{
	sigset_t allow;
	int res;

	// setup signal list to unblock
	if (sigemptyset(&allow) < 0) {
		perror("Failed to initialize a list of signal to unblock");
		abort();
	}

	if (sigaddset(&allow, SIGINT) < 0) {
		perror("Failed to add SIGINT to a list of signal to unblock");
		abort();
	}

	if (sigaddset(&allow, SIGTERM) < 0) {
		perror("Failed to add SIGTERM to a list of signal to unblock");
		abort();
	}

	// unblock shutdown signals
	res = pthread_sigmask(SIG_UNBLOCK, &allow, NULL);

	if (res) {
		fprintf(stderr, "Failed to unblock SIGINT and SIGTERM: %d\n", res);
		abort();
	}
}

static void shutdown(int sig)
{
	if (!shuttingdown) {
		dispatcher_stop(dispatcher);
		shuttingdown = true;
	}
}

static bool listen_signals(void)
{
	struct sigaction act;

	// setup listen action
	memset(&act, 0, sizeof(act));

	act.sa_handler = shutdown;
	act.sa_flags = SA_NODEFER;

	if (sigfillset(&act.sa_mask) < 0) {
		perror("Failed to add signals to block while processing a signal");
		return false;
	}

	// start listen for signals
	if (sigaction(SIGINT, &act, NULL) < 0) {
		perror("Failed to listen for SIGINT");
		return false;
	}

	if (sigaction(SIGTERM, &act, NULL) < 0) {
		perror("Failed to listen for SIGTERM");
		return false;
	}

	return true;
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
	bool success;
	struct server server;
	struct api api;

	// init foundation
	printf("Starting "PACKAGE" "VERSION"\n");

	if (!block_signals() || !listen_signals()) {
		return EXIT_FAILURE;
	}

	// setup event dispatcher
	dispatcher = dispatcher_create();

	if (!dispatcher) {
		return EXIT_FAILURE;
	}

	success = false;

	// start servers
	if (!server_init(&server, "127.0.0.1")) {
		goto free_dispatcher;
	}

	if (!dispatcher_add(dispatcher, (struct event_source *)&server)) {
		goto destroy_server;
	}

	// start api
	if (!api_init(&api, "127.0.0.1", 8080)) {
		goto remove_server;
	}

	if (!dispatcher_add(dispatcher, (struct event_source *)&api)) {
		goto destroy_api;
	}

	// enter event loop
	success = dispatcher_run(dispatcher, ready, NULL);
	shuttingdown = true; // prevent accessing on dispatcher while we clean up

	// clean up
	if (!dispatcher_del(dispatcher, (struct event_source *)&api)) {
		abort();
	}

destroy_api:
	api_destroy(&api);

remove_server:
	if (!dispatcher_del(dispatcher, (struct event_source *)&server)) {
		abort();
	}

destroy_server:
	server_destroy(&server);

free_dispatcher:
	dispatcher_free(dispatcher);

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
