#ifndef HAWKTRACER_INTERNAL_REGISTRY_H
#define HAWKTRACER_INTERNAL_REGISTRY_H

#include "hawktracer/registry.h"
#include "hawktracer/timeline_listener.h"
#include "internal/timeline_listener_container.h"

HT_DECLS_BEGIN

HT_ErrorCode ht_registry_init(void);

void ht_registry_deinit(void);

HT_API HT_TimelineListenerContainer* ht_registry_find_listener_container(const char* name);

HT_API HT_ErrorCode ht_registry_register_listener_container(const char* name, HT_TimelineListenerContainer* container);

HT_DECLS_END

#endif /* HAWKTRACER_INTERNAL_REGISTRY_H */
