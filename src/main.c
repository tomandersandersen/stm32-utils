#include "led.h"
#include "clock.h"
#include "uart.h"
#include "log.h"
#include "delay.h"

#include <stdint.h>

static void error_handler(void)
{
    led_off(LED_GREEN);
    led_on(LED_ORANGE);

    while (1)
    {
        /* Orange stays on continuously. */
    }
}

static void delay(void)
{
    for (volatile uint32_t i = 0; i < 500000U; i++)
    {
        __asm volatile("nop");
    }
}

int main(void)
{
    uart_init();
    led_init();

    led_off(LED_GREEN);
    led_off(LED_ORANGE);

    if (!clock_init())
    {
        LOG_ERROR("Fail to initialize clocks");
        error_handler();
    }

    delay_init();

    LOG_INFO("Initialization complete, entering main loop");

    while (1)
    {
        /* Green heartbeat. */
        led_toggle(LED_GREEN);

        /* Orange activity blink. */
        led_toggle(LED_ORANGE);

        delay();
    }
}
