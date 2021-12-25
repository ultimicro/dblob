#include "eventsource.h"

#include <stddef.h>
#include <stdio.h>

#include <unistd.h>

bool event_source_init(struct event_source *s)
{
	s->fd = -1;
	s->read = NULL;
	s->write = NULL;

	return true;
}


void event_source_destroy(struct event_source *s)
{
	if (s->fd >= 0 && close(s->fd) < 0) {
		perror("Failed to close file descriptor");
	}
}
