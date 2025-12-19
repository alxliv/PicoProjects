#include "pico/stdlib.h"

#include "led_lib.h"

#define MAX_LEDS (16)
typedef struct
{
    uint8_t pin;
    uint8_t state;
} led_t;

static led_t all_leds[MAX_LEDS] = {0};
static uint leds_in_use = 0;

static led_t *led_get(uint pin)
{
    uint i = 0;
    for (i = 0; i < leds_in_use; i++)
    {
        if (all_leds[i].pin == pin)
            return &all_leds[i];
    }
    hard_assert(leds_in_use <= MAX_LEDS);
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, LED_OFF);
    all_leds[i].pin = pin;
    all_leds[i].state = LED_OFF;
    if (leds_in_use < MAX_LEDS)
        leds_in_use++;
    return &all_leds[i];
}

void led_on(uint pin)
{
    led_t *p = led_get(pin);
    gpio_put(pin, LED_ON);
    p->state = LED_ON;
}

void led_off(uint pin)
{
    led_t *p = led_get(pin);
    gpio_put(pin, LED_OFF);
    p->state = LED_OFF;
}

bool led_is_on(uint pin)
{
    return (led_get(pin)->state == LED_ON);
}

void led_flip(uint pin)
{
    led_is_on(pin) ? led_off(pin) : led_on(pin);
}