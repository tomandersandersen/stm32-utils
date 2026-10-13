#pragma once

#include <stdbool.h>

typedef enum {
    LED_GREEN,
    LED_ORANGE,
    LED_RED,
    LED_BLUE
} led_id_t;


void led_init(void);
void led_on(led_id_t led);
void led_off(led_id_t led);
void led_toggle(led_id_t led);
void led_set(led_id_t led, bool state);
