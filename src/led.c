#include "led.h"
#include "stm32f407xx.h"

#define LED_GREEN_PIN   12U
#define LED_ORANGE_PIN  13U
#define LED_RED_PIN     14U
#define LED_BLUE_PIN    15U

#define LED_ALL_PINS ( (1U << LED_GREEN_PIN)  | \
                       (1U << LED_ORANGE_PIN) | \
                       (1U << LED_RED_PIN)    | \
                       (1U << LED_BLUE_PIN) )

static uint32_t led_pin(led_id_t led)
{
    switch (led) {
    case LED_GREEN:
        return LED_GREEN_PIN;

    case LED_ORANGE:
        return LED_ORANGE_PIN;

    case LED_RED:
        return LED_RED_PIN;

    case LED_BLUE:
        return LED_BLUE_PIN;

    default:
        return 0xFFFFFFFFU;
    }
}

void led_init(void)
{
    /* Enable the GPIOD peripheral clock. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    /* Configure PD12-PD15 as general-purpose outputs. */
    GPIOD->MODER &= ~(0xFFU << 24U);
    GPIOD->MODER |=  (0x55U << 24U);

    /* Configure push-pull output type. */
    GPIOD->OTYPER &= ~LED_ALL_PINS;

    /* Disable pull-up and pull-down resistors. */
    GPIOD->PUPDR &= ~(0xFFU << 24U);

    /* Start with all LEDs off. */
    GPIOD->BSRR = LED_ALL_PINS << 16U;
}

void led_set(led_id_t led, bool state)
{
    uint32_t pin = led_pin(led);

    if (pin > 15U) {
        return;
    }

    if (state) {
        /* Set the output bit to turn the LED on. */
        GPIOD->BSRR = 1U << pin;
    } else {
        /* Reset the output bit to turn the LED off. */
        GPIOD->BSRR = 1U << (pin + 16U);
    }
}

void led_on(led_id_t led)
{
    led_set(led, true);
}

void led_off(led_id_t led)
{
    led_set(led, false);
}

void led_toggle(led_id_t led)
{
    uint32_t pin = led_pin(led);

    if (pin > 15U) {
        return;
    }

    /* Toggle the output state. */
    GPIOD->ODR ^= 1U << pin;
}