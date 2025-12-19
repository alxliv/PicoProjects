#ifndef AXX_LED_LIB_H
#define AXX_LED_LIB_H

#include <stdint.h>

#define LED_OFF (0)
#define LED_ON (1)

extern void led_on(uint pin);
extern void led_off(uint pin);
extern bool led_is_on(uint pin);
extern void led_flip(uint pin);

#endif