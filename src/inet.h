#ifndef INET_H
#define INET_H

#include <inttypes.h>

#include <netdb.h>
#include <sys/socket.h>

/**
 * Convert a pointer to NULL terminated string represents a numerical network
 * address to \c addrinfo.
 *
 * @param address A pointer to NULL terminated string represents a numerical
 * network address to convert. Can be IPv4 or IPv6.
 * @return A pointer to \c addrinfo of the converted address or \c NULL if the
 * conversion cannot be performed. The value must be free with \c freeaddrinfo
 * when no longer need.
 */
struct addrinfo *inet_convert_address(const char *address, uint16_t port);

/**
 * Create a TCP socket and listen for connections.
 *
 * @return A file descriptor represents a socket or -1 if error.
 */
int inet_listen(const struct sockaddr *addr, socklen_t size);

#endif // INET_H
