#ifndef ASYNC_TASK_H
#define ASYNC_TASK_H

#include <stdint.h>

// Forward declare Task so typedefs can use it
typedef struct Task Task;

// Callback type for tasks (pointer to function that gets a Task pointer)
typedef void (*TaskCallback)(Task *task);

// Task structure with linked list pointer
typedef struct Task {
    uint32_t last_run;      // Last execution time in milliseconds
    uint32_t interval;      // Interval between executions in milliseconds
    TaskCallback callback; // Function to call (NULL = not active)
    void* user_data;        // User data pointer for task-specific variables
    struct Task* next;      // Next task in the list
} Task;

// Active tasks list (linked list)
typedef struct {
    Task* head;
} TaskList;

// Global function declarations
void TaskList_Init(TaskList* list);
void TaskList_Add(TaskList* list, Task* task);
void TaskList_Remove(TaskList* list, Task* task);
void Update(TaskList* list);

#endif
