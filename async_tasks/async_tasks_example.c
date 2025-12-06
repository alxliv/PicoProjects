#include <stdio.h>
#include "pico/stdlib.h"
#include "async_task.h"

#define LED_1   (7)
#define LED_2   (8)

int all_leds[] = {LED_1, LED_2};

#define LED_DELAY_MS (500)

int pico_leds_init(void)
{
    int n = sizeof(all_leds)/sizeof(int);
    for (int i=0; i<n; i++)
    {
        gpio_init(all_leds[i]);
        gpio_set_dir(all_leds[i], GPIO_OUT);
    }
    return PICO_OK;
}

static inline void led_on(uint pin) {
    gpio_put(pin, true);
}

static inline void led_off(uint pin) {
    gpio_put(pin, false);
}

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

typedef struct
{
    Task task;
    uint32_t sec_counter;
} print_task_t;

static void printSecs_callback(Task* task) {
    print_task_t* ps = (print_task_t *)task;
    uint32_t sec = ps->sec_counter;
    sec++;
    printf("Passed=%d secs.\n", sec);
    ps->sec_counter = sec;
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

    print_task_t printSecs;
    printSecs.sec_counter = 0;
    printSecs.task.callback = printSecs_callback;
    printSecs.task.interval = 1000;

    TaskList_Add(&ActiveTasksList, (Task *)&led1);
    TaskList_Add(&ActiveTasksList, (Task *)&led2);
    TaskList_Add(&ActiveTasksList, (Task *)&printSecs);

    while (true) {
        Update(&ActiveTasksList);
        sleep_ms(5);

    }

}
