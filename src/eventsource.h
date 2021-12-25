#ifndef EVENTSOURCE_H
#define EVENTSOURCE_H

#include <stdbool.h>

typedef void (*event_handler_t) (void);

struct event_source {
	int fd;
	event_handler_t read;
	event_handler_t write;
};

bool event_source_init(struct event_source *s);
void event_source_destroy(struct event_source *s);

#endif // EVENTSOURCE_H
