#include <stddef.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "async_task.h"

#define MAX_TASKS (16)

static Task all_tasks[MAX_TASKS] = {0};

Task *task_add(uint32_t interval, TaskCallback callback)
{
    Task *pt = NULL;
    for (uint i=0; i<MAX_TASKS; i++)
    {
        if (!all_tasks[i].is_taken)
        {
            pt = &all_tasks[i];
            break;
        }
    }
    if (!pt)
        return NULL;
    pt->interval = 0;
    pt->next_run_at = millis();
    pt->callback = NULL;
    pt->is_taken = true;
    return pt;
}

void task_delete(Task* task) {
    Task *pt = NULL;
    for (uint i=0; i<MAX_TASKS; i++) {
        if (task == &all_tasks[i]) {
            pt = &all_tasks[i];
            break;
        }
    }
    if (!pt)
        return;
    pt->callback = NULL;
    pt->is_taken = false;
}

// Update all active tasks - call this every 1ms (or as fast as possible from loop)
void async_tasks_update()
{
    for (uint i=0; i<MAX_TASKS; i++) {
        Task *pt = &all_tasks[i];
        if (pt->is_taken && pt->callback)
        {
            uint32_t tm = millis();
            if (tm >= pt->next_run_at)
            {
                pt->next_run_at = tm+pt->interval;
                pt->callback(pt);
            }
        }
    }
}


