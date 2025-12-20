#include <stdio.h>
#include "event_system.h"
#include "pico/stdlib.h"

typedef struct {
    event_callback_t callback;
    void *context;
} subscriber_entry_t;

static subscriber_entry_t subscribers[MAX_LISTENERS];
static int subscriber_count = 0;

bool event_subscribe(event_callback_t callback, void *context) {
    if (subscriber_count < MAX_LISTENERS) {
        subscribers[subscriber_count].callback = callback;
        subscribers[subscriber_count].context = context; // Store the data pointer
        subscriber_count++;
        return true;
    }
    return false;
}

// Publish an event to all listeners
void event_publish(uint32_t distance) {
    // Create the event object
    sensor_event_t event;
    event.distance_cm = distance;
    event.timestamp = to_ms_since_boot(get_absolute_time());

    // Iterate and notify
    for (int i = 0; i < subscriber_count; i++) {
        if (subscribers[i].callback != NULL) {
            subscribers[i].callback(&event, subscribers[i].context);
        }
    }
}
