# MCU Async Task Infrastructure

A lightweight, pure C implementation of a cooperative multitasking scheduler mostly suited for microcontrollers such as Arduino, Raspberry Pi Pico, ESP32, STM32, and similar.


## Features

- Non-blocking task execution
- Multiple tasks with independent intervals
- Simple callback-based architecture
- Minimal memory footprint
- No external dependencies

## Architecture

### Core Components

1. **Task Structure**: Contains timing information, callback pointer and `next` pointer
2. **TaskList**: Simple linked list with a `head` pointer to the first task
3. **Update() Function**: Global function called from the program's main loop (or `while(1)` on non-Arduino platforms) to traverse and process all active tasks

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
- `async_tasks_example.c` - Example Raspberry Pi Pico (C SDK) code to blink two LEDs at different rates

## Usage

### Basic Setup

```c
#include "async_task.h"

TaskList ActiveTasksList;
typedef struct
{
    Task task;
    int pin;
    bool state;
} led_t;

static void led_callback(Task* task) {
    // Your code here - runs every task->interval;
    // To change behavior:
    // task->callback = another_callback;
    // To change interval:
    // task->interval = 2000;
    led_t *d = (led_t *)task;
    d->state = !d->state;
    d->state ? led_on(d->pin):led_off(d->pin);
}

static void init_led(led_t* led, uint32_t pin, uint32_t interval, TaskCallback callbk)
{
    led->task.callback = callbk;
    led->task.interval = interval;
    led->pin = pin;
    led->state = false;
}

int main()
{
    stdio_init_all();
    int rc = pico_leds_init();
    hard_assert(rc == PICO_OK);

    TaskList_Init(&ActiveTasksList);

    led_t led1;
    init_led(&led1, LED_1, 600, led_callback);
    led_t led2;
    init_led(&led2, LED_2, 250, led_callback);

    TaskList_Add(&ActiveTasksList, (Task *)&led1);
    TaskList_Add(&ActiveTasksList, (Task *)&led2);

    while (true) {
        Update(&ActiveTasksList);
        sleep_ms(5);

    }
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

## API Reference

### Functions

- `void TaskList_Init(TaskList* list)` - Initialize the task list
- `void TaskList_Add(TaskList* list, Task* task)` - Add a task to the active list
- `void TaskList_Remove(TaskList* list, Task* task)` - Remove a task from the active list
- `void Update(TaskList* list)` - Update all active tasks (call from your main loop — ideally every 1 ms)


## Tips

- Keep callbacks short and fast — they should not block for long periods.
- Avoid blocking calls (like `delay()`/`sleep_ms()`) inside callbacks; yield quickly.
- Use `Task` as the first member of your custom state struct so a `Task*` can be cast back to your type in callbacks.

Example (pattern for user state types):
```c
typedef struct {
    Task task; // Task must be first member
    int pin;
    bool state;
} led_t;

static void led_callback(Task* task) {
    led_t *d = (led_t *)task; // cast back to your type
    d->state = !d->state;
    d->state ? led_on(d->pin) : led_off(d->pin);
}

// in main()
led_t led;
led.task.callback = led_callback;
led.pin = 7;
led.state = false;
led.task.interval = 1000;
TaskList_Add(&ActiveTasksList, (Task *)&led);
```

- Tasks share CPU time cooperatively; keep callbacks short.
- Call `Update()` as frequently as practical (ideally every 1 ms) to meet timing expectations.
- Use `TaskList_Remove()` to temporarily stop a task and `TaskList_Add()` to restart it.
- Change `task->callback` or `task->interval` at runtime to switch behavior.
- Prefer statically allocated tasks for predictable memory usage on constrained devices.

