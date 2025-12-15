#include <stddef.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "async_task.h"

// Initialize the task list
void TaskList_Init(TaskList* list) {
    list->head = NULL;
}

uint32_t millis()
{
    return time_us_32()/1000;
}

// Add a task to the active tasks list
void TaskList_Add(TaskList* list, Task* task) {
    if (task == NULL)
    {
        return; // Invalid task
    }

    // Check if task is already in the list
    Task* current = list->head;
    while (current != NULL)
    {
        if (current == task)
        {
            return; // Already in list
        }
        current = current->next;
    }

    // Add to the beginning of the list
    task->next = list->head;
    list->head = task;
    task->last_run = millis(); // Initialize last_run
}

// Remove a task from the active tasks list
void TaskList_Remove(TaskList* list, Task* task) {
    if (list->head == NULL || task == NULL) {
        return;
    }

    // If it's the head
    if (list->head == task) {
        list->head = task->next;
        task->next = NULL;
        return;
    }

    // Find the task in the list
    Task* current = list->head;
    while (current->next != NULL) {
        if (current->next == task) {
            current->next = task->next;
            task->next = NULL;
            return;
        }
        current = current->next;
    }
}

// Update all active tasks - call this every 1ms (or as fast as possible from loop)
void Update(TaskList* list) {
    uint32_t current_time = millis();

    Task* current = list->head;
    while (current != NULL) {
        // Skip tasks without callback
        if (current->callback != NULL) {
            // Check if it's time to run the task
            if (current_time - current->last_run >= current->interval) {
                current->last_run = current_time;
                current->callback(current);
            }
        }
        current = current->next;
    }
}
