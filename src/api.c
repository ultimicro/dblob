#include "api.h"
#include "inet.h"

static void accept_connection(void)
{
}

static bool init(struct api *a, const struct sockaddr *addr, socklen_t size)
{
	// init base
	if (!event_source_init(&a->base)) {
		return false;
	}

	// create socket
	a->base.fd = inet_listen(addr, size);

	if (a->base.fd >= 0) {
		return true;
	}

	// clean up
	event_source_destroy(&a->base);

	return false;
}

bool api_init(struct api *a, const char *address, uint16_t port)
{
	struct addrinfo *resolved;
	bool success;

	// resolve address to bind
	resolved = inet_convert_address(address, port);

	if (!resolved) {
		return false;
	}

	// setup server
	success = init(a, resolved->ai_addr, resolved->ai_addrlen);

	freeaddrinfo(resolved);

	// setup event handlers
	a->base.read = accept_connection;

	return success;
}

void api_destroy(struct api *a)
{
	event_source_destroy(&a->base);
}
