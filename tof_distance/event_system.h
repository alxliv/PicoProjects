#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

// 1. Define the data payload
typedef struct {
    uint32_t timestamp;
    uint32_t distance_cm;
} sensor_event_t;

// 2. Define the callback function signature
// Listeners must implement a function that looks like this
typedef void (*event_callback_t)(const sensor_event_t *event, void *context);

// System limits (static allocation is safer for embedded)
#define MAX_LISTENERS 4

// Public API
bool event_subscribe(event_callback_t callback, void *context);
void event_publish(uint32_t distance);

#endif