#ifndef LED_H
#define LED_H

#include <stdbool.h>

typedef enum {
    LED_GREEN,
    LED_ORANGE
} led_id_t;

void led_init(void);
void led_on(led_id_t led);
void led_off(led_id_t led);
void led_toggle(led_id_t led);
void led_set(led_id_t led, bool state);

#endif /* LED_H */
