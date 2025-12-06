#include <stdio.h>
#include "pico/stdlib.h"

#define LED_1   (7)
#define LED_2   (8)

// test

int all_leds[] = {LED_1, LED_2};

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 1500
#endif

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

void led_set(int led_pin, bool led_on)
{
    gpio_put(led_pin, led_on);
}

#define LED_ON(pin) do {led_set(pin, true); } while(0)
#define LED_OFF(pin) do {led_set(pin, false); } while(0)

int main()
{
    stdio_init_all();
    int rc = pico_leds_init();
    hard_assert(rc == PICO_OK);
    int counter = 0;

    while (true) {
        printf("v1 counter=%d\n", counter);
        LED_ON(LED_1);
        sleep_ms(LED_DELAY_MS);
        LED_OFF(LED_1);
        LED_ON(LED_2);
        sleep_ms(LED_DELAY_MS);
        LED_OFF(LED_2);
        counter++;
    }

}
