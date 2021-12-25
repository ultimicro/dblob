#ifndef SERVER_H
#define SERVER_H

#include "eventsource.h"

#include <stdbool.h>

struct server {
	struct event_source base;
};

bool server_init(struct server *s, const char *address);
void server_destroy(struct server *s);

#endif // SERVER_H
