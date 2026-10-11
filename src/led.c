#include "led.h"
#include "stm32f407xx.h"

#define LED_GREEN_PIN   12U
#define LED_ORANGE_PIN  13U

static uint32_t led_pin(led_id_t led)
{
    switch (led) {
    case LED_GREEN:
        return LED_GREEN_PIN;

    case LED_ORANGE:
        return LED_ORANGE_PIN;

    default:
        return 0xFFFFFFFFU;
    }
}

void led_init(void)
{
    /* Enable GPIOD clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    /* Configure PD12 and PD13 as outputs */
    GPIOD->MODER &= ~((3U << (LED_GREEN_PIN * 2U)) |
                      (3U << (LED_ORANGE_PIN * 2U)));

    GPIOD->MODER |=  ((1U << (LED_GREEN_PIN * 2U)) |
                      (1U << (LED_ORANGE_PIN * 2U)));

    /* Push-pull outputs */
    GPIOD->OTYPER &= ~((1U << LED_GREEN_PIN) |
                       (1U << LED_ORANGE_PIN));

    /* No pull-up or pull-down */
    GPIOD->PUPDR &= ~((3U << (LED_GREEN_PIN * 2U)) |
                      (3U << (LED_ORANGE_PIN * 2U)));

    /* Start with both LEDs off */
    GPIOD->BSRR = (1U << (LED_GREEN_PIN + 16U)) |
                  (1U << (LED_ORANGE_PIN + 16U));
}

void led_set(led_id_t led, bool state)
{
    uint32_t pin = led_pin(led);

    if (pin > 15U) {
        return;
    }

    if (state) {
        GPIOD->BSRR = 1U << pin;
    } else {
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

    GPIOD->ODR ^= 1U << pin;
}