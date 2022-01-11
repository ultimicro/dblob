#include "inet.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct addrinfo *inet_convert_address(const char *address, uint16_t port)
{
	struct addrinfo hint, *out;
	char service[8];
	int res;

	// setup hint
	memset(&hint, 0, sizeof(hint));

	hint.ai_family = AF_UNSPEC;
	hint.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

	// setup service
	if (snprintf(service, sizeof(service), "%u", port) < 0) {
		perror("Failed to convert port number to string");
		return NULL;
	}

	// resolve
	res = getaddrinfo(address, service, &hint, &out);

	if (res) {
		const char *err = gai_strerror(res);
		fprintf(stderr, "Failed to convert address %s: %s\n", address, err);
		return NULL;
	}

	return out;
}

int inet_listen(const struct sockaddr *addr, socklen_t size)
{
	int fd;

	// create socket
	fd = socket(addr->sa_family, SOCK_STREAM, 0);

	if (fd < 0) {
		perror("Failed to a socket for listening");
		return -1;
	}

	// turn on non-blocking mode
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("Failed to turn on non-blocking mode for a socket");
		goto fail;
	}

	// bind address to a socket
	if (bind(fd, addr, size) < 0) {
		perror("Failed to bind an address to a socket");
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
		perror("Failed to close a socket");
	}

	return -1;
}
