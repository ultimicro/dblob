#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "eventsource.h"

#include <stdbool.h>

struct dispatcher;

typedef struct dispatcher *dispatcher_t;
typedef void (*dispatcher_ready_t) (void *);

dispatcher_t dispatcher_create(void);
void dispatcher_free(dispatcher_t d);

bool dispatcher_add(dispatcher_t d, const struct event_source *s);
bool dispatcher_del(dispatcher_t d, const struct event_source *s);

/**
 * @remark The implementation must be async-signal-safe.
 */
void dispatcher_stop(dispatcher_t d);

/**
 * @param ready A pointer to function to get called when dispatcher is ready to
 * dispatch events. The function will get called by the same thread as caller.
 */
bool dispatcher_run(dispatcher_t d, dispatcher_ready_t ready, void *arg);

#endif // DISPATCHER_H
