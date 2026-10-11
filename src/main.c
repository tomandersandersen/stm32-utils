#include "led.h"

#include <stdint.h>

int main(void)
{
    led_init();

    led_on(LED_GREEN);
    led_off(LED_ORANGE);

    while (1)
    {
        led_toggle(LED_ORANGE);

        /* Temporary busy-wait delay */
        for (volatile uint32_t i = 0; i < 500000U; i++)
        {
            __asm volatile("nop");
        }
    }
}
