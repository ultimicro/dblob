#ifndef API_H
#define API_H

#include "eventsource.h"

#include <inttypes.h>
#include <stdbool.h>

struct api {
	struct event_source base;
};

bool api_init(struct api *a, const char *address, uint16_t port);
void api_destroy(struct api *a);

#endif // API_H
