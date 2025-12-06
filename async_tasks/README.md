# MCU Async Task Infrastructure

A lightweight, pure C implementation of a cooperative multitasking scheduler for microcontroller (Arduino, Pico, ESP32, STM32, etc..) projects.

## Features

- Non-blocking task execution
- Multiple tasks with independent intervals
- Simple callback-based architecture
- Minimal memory footprint
- No external dependencies

## Architecture

### Core Components

1. **Task Structure**: Contains timing information, callback pointer, user data, and `next` pointer
2. **TaskList**: Simple linked list with a `head` pointer to the first task
3. **Update() Function**: Global function called from `loop()` to traverse and process all active tasks

### How It Works

- Tasks are allocated by the user (static, global, or dynamic)
- Each task has an interval (in milliseconds) and a callback function
- Tasks form a linked list via the `next` pointer
- Tasks are added to the ActiveTasksList using `TaskList_Add()`
- The `Update()` function traverses the linked list and calls callbacks when intervals expire
- Tasks can be removed with `TaskList_Remove()`
- Tasks can switch behavior by changing their callback function
- Tasks are non-blocking and cooperative (they should return quickly)

## Files

- `async_task.h` - Header file with Task structure and TaskList
- `async_task.c` - Implementation of the task list and Update() function
- `async_led_demo.ino` - Example sketch with 2 LEDs blinking at different rates
- `advanced_callback_demo.ino` - Advanced example showing dynamic callback switching

## Usage

### Basic Setup

```c
#include "async_task.h"

TaskList ActiveTasksList;
Task my_task;

void setup() {
    // Initialize the task list
    TaskList_Init(&ActiveTasksList);
    
    // Configure and add task
    my_task.callback = my_callback;
    my_task.interval = 1000;
    my_task.user_data = NULL;
    my_task.next = NULL;
    TaskList_Add(&ActiveTasksList, &my_task);
}

void loop() {
    // Update all tasks - call every 1ms ideally (or as fast as possible)
    Update(&ActiveTasksList);
}

void my_callback(Task* task) {
    // Your code here - runs every 1000ms
    
    // To change behavior:
    // task->callback = another_callback;
    
    // To change interval:
    // task->interval = 2000;
}
```

### Controlling Tasks

```c
// Remove a task from active list (stops it)
TaskList_Remove(&ActiveTasksList, &my_task);

// Add it back (restarts it)
TaskList_Add(&ActiveTasksList, &my_task);

// Change task behavior
my_task.callback = different_callback;

// Change task interval
my_task.interval = 2000;  // Now runs every 2 seconds
```

### LED Example

The demo shows two LEDs:
- **LED1** (Pin 13): Blinks every 500ms (2 times per second)
- **LED2** (Pin 12): Blinks every 1500ms (once every 1.5 seconds)

Both LEDs run independently without blocking the main loop!

## Installation

1. Create a new Arduino sketch folder
2. Copy `async_task.h`, `async_task.c`, and `async_led_demo.ino` into the folder
3. Open the `.ino` file in Arduino IDE
4. Upload to your Arduino board

## API Reference

### Functions

- `void TaskList_Init(TaskList* list)` - Initialize the task list
- `void TaskList_Add(TaskList* list, Task* task)` - Add a task to the active list
- `void TaskList_Remove(TaskList* list, Task* task)` - Remove a task from the active list
- `void Update(TaskList* list)` - Update all active tasks (call from loop() every 1ms ideally)

### Task Setup

```c
// Declare task
Task my_task;

// Configure task
my_task.callback = my_callback_function;
my_task.interval = 1000;  // milliseconds
my_task.user_data = &my_data;  // optional
my_task.next = NULL;  // must be NULL before adding to list

// Add to active list
TaskList_Add(&ActiveTasksList, &my_task);
```

### Task Callback Signature

```c
void my_callback(Task* task) {
    // Access user data if needed
    MyData* data = (MyData*)task->user_data;
    
    // Do work here
    
    // Can modify task behavior:
    // task->callback = other_callback; // Switch behavior
    // task->interval = 2000;           // Change timing
    // TaskList_Remove(&ActiveTasksList, task); // Stop task
}
```

## Advantages

- **Non-blocking**: No `delay()` calls needed
- **Simple**: Just a linked list - minimal code
- **No limits**: Add as many tasks as you have memory for
- **Flexible**: Tasks can change behavior by switching callbacks
- **User-controlled allocation**: Tasks can be static, global, or dynamic
- **Direct access**: No indirection - you own the Task structs
- **Scalable**: Add multiple tasks easily
- **Maintainable**: Each task is self-contained
- **Efficient**: Tasks only run when needed
- **Lightweight**: Minimal overhead and memory usage

## Limitations

- Cooperative only (tasks must return quickly)
- Millisecond precision (uses `millis()`)
- No priority system
- Tasks limited only by available memory

## Tips

1. Keep callbacks short and fast
2. Don't use `delay()` inside callbacks
3. Use task->user_data for task-specific state
4. Tasks share CPU time cooperatively
5. Call `Update()` as frequently as possible (ideally every 1ms)
6. Use `TaskList_Remove()` to temporarily stop a task
7. Change callbacks to switch task behavior dynamically
8. Modify `task->interval` to adjust timing on the fly
9. Allocate tasks statically for predictable memory usage

## License

Free to use and modify for any purpose.
