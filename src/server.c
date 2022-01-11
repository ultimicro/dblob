#include "inet.h"
#include "server.h"

#include <stdio.h>

static void accept_connection(void)
{
}

static bool init(struct server *s, const struct sockaddr *addr, socklen_t size)
{
	// init base
	if (!event_source_init(&s->base)) {
		return false;
	}

	// create socket
	s->base.fd = inet_listen(addr, size);

	if (s->base.fd >= 0) {
		return true;
	}

	// clean up
	event_source_destroy(&s->base);

	return false;
}

bool server_init(struct server *s, const char *address)
{
	struct addrinfo *resolved;
	bool success;

	// resolve address to bind
	resolved = inet_convert_address(address, 9600);

	if (!resolved) {
		return false;
	}

	// setup server
	success = init(s, resolved->ai_addr, resolved->ai_addrlen);

	freeaddrinfo(resolved);

	// setup event handlers
	s->base.read = accept_connection;

	return success;
}

void server_destroy(struct server *s)
{
	event_source_destroy(&s->base);
}
