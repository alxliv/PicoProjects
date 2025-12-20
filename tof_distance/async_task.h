#ifndef ASYNC_TASK_H
#define ASYNC_TASK_H

#include <stdint.h>
#include "hardware/timer.h"

inline uint32_t millis()
{
    return (uint32_t)(time_us_64() / 1000u);
}

typedef struct Task Task; // Forward declararion so typedefs can use it

typedef void (*TaskCallback)(Task *task);

// Task structure with linked list pointer
typedef struct Task {
    uint32_t next_run_at;   // Next execution time in milliseconds
    uint32_t interval;      // Interval between executions in milliseconds
    union {
        void *ptr;
        uint data[4];
        uint value;
        uint status;
    } user;
    TaskCallback callback;  // Function to call (NULL = not active)
    bool is_taken;
} Task;

extern Task *task_add();
void task_delete(Task* task);
void async_tasks_update();

#endif
