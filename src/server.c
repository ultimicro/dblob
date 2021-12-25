#include "server.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

static void accept_connection(void)
{
}

static int create_socket(const struct sockaddr *addr, socklen_t size)
{
	int fd;

	// create socket
	fd = socket(addr->sa_family, SOCK_STREAM, 0);

	if (fd < 0) {
		perror("Failed to create server socker");
		return -1;
	}

	// turn on non-blocking mode
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("Failed to turn on non-blocking mode for server");
		goto fail;
	}

	// bind address to listen
	if (bind(fd, addr, size) < 0) {
		perror("Failed to bind server address");
		goto fail;
	}

	// listen for connections
	if (listen(fd, 5) < 0) {
		perror("Failed to listen for connections");
		goto fail;
	}

	return fd;

	// clean up
fail:
	if (close(fd) < 0) {
		perror("Failed to close server socket");
	}

	return -1;
}

static bool init(struct server *s, const struct sockaddr *addr, socklen_t size)
{
	// init base
	if (!event_source_init(&s->base)) {
		return false;
	}

	// create socket
	s->base.fd = create_socket(addr, size);

	if (s->base.fd >= 0) {
		return true;
	}

	// clean up
	event_source_destroy(&s->base);

	return false;
}

static struct addrinfo *resolve_address(const char *address)
{
	struct addrinfo hint, *out;
	int res;

	// setup hint
	memset(&hint, 0, sizeof(hint));

	hint.ai_family = AF_UNSPEC;
	hint.ai_flags = AI_NUMERICHOST;

	// resolve
	res = getaddrinfo(address, NULL, &hint, &out);

	if (res) {
		fprintf(stderr, "Failed to resolve server address %s: %s\n", address,
			gai_strerror(res));
		return NULL;
	}

	return out;
}

bool server_init(struct server *s, const char *address)
{
	struct addrinfo *resolved;
	bool success;

	// resolve address to bind
	resolved = resolve_address(address);

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
