#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "event_system.h"

static inline uint32_t millis()
{
    return (uint32_t)(time_us_64() / 1000u);
}

#define LED_RED (7)
#define LED_GREEN (8)

typedef struct
{
    uint32_t last_run_time;
    uint32_t interval;
    uint32_t pin;
    uint32_t last_valid_distance_cm;
    bool led_state; // Only used for the LED task
} task_context_t;

// --- Task 1: Blink LED ---
void task_blink_led(task_context_t *ctx)
{
    uint32_t current_time = millis();

    // Check if enough time has passed since the last run
    if (current_time - ctx->last_run_time >= ctx->interval)
    {
        // Update the last run time
        ctx->last_run_time = current_time;

        // Perform the task logic
        ctx->led_state = !ctx->led_state;
        gpio_put(ctx->pin, ctx->led_state);
    }
}

// --- Task 2: Print to Serial ---
void task_serial_print(task_context_t *ctx)
{
    uint32_t current_time = millis();
    if (current_time - ctx->last_run_time >= ctx->interval)
    {
        ctx->last_run_time = current_time;
        printf("[%d] ms. Data received: %d cm\n", current_time, ctx->last_valid_distance_cm);
    }
}

static void led_init(uint pin)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, false);
}

static void on_sensor_data_led(const sensor_event_t *e, void *ctx)
{
    task_context_t *t = (task_context_t *)ctx;
    if (t->pin == LED_GREEN)
    {
        bool is_close = (e->distance_cm < 25) ? true : false;
        gpio_put(t->pin, !is_close);
    }
    else
    {
        t->interval = 5 + e->distance_cm * 10;
    }
}

// --- Listener 2: Data Logger ---
// Simply records the data
void on_sensor_data_log(const sensor_event_t *e, void *ctx)
{
    task_context_t *t = (task_context_t *)ctx;
    t->last_valid_distance_cm = e->distance_cm;
}

// Simulates a sensor reading every 1 second
void simulate_sensor_reading()
{
    static uint32_t last_read = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (now - last_read > 1000)
    {
        last_read = now;

        // Simulate varying distance (randomish value)
        uint32_t fake_distance = (now % 120) + 5.0f;

        // TRIGGER THE EVENT
        event_publish(fake_distance);
    }
}

int main()
{

    stdio_init_all();
    led_init(LED_RED);
    led_init(LED_GREEN);

    // Initialize task contexts
    task_context_t led_red_task = {0, 500, LED_RED, 0, false};
    task_context_t led_green_task = {0, 250, LED_GREEN, 0, false};

    task_context_t serial_log_task = {0, 1000, 0, false};

    // 1. Subscribe listeners
    event_subscribe(on_sensor_data_log, &serial_log_task);
    event_subscribe(on_sensor_data_led, &led_red_task);
    event_subscribe(on_sensor_data_led, &led_green_task);

    printf("System started. Entering Super Loop...\n");

    // The "Super Loop"
    while (true)
    {
        // Run tasks cooperatively
        // If it's not time for a task to run, it returns immediately
        task_blink_led(&led_red_task);
        task_serial_print(&serial_log_task);

        simulate_sensor_reading();

        // Optional: Minimal sleep to reduce power if high precision isn't needed
        sleep_us(1000);
    }
}