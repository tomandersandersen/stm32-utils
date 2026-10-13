#include "uart.h"

#include "stm32f407xx.h"

#include <stdint.h>

#define UART_PCLK1_HZ 24000000U
#define UART_PCLK2_HZ 48000000U

/*
 * USART1 debug UART
 *
 * PB6: USART1_TX (AF7)
 * PB7: USART1_RX (AF7)
 *
 * 115200 baud, 8N1, no hardware flow control.
 */

void uart_init(void)
{
    /* Enable GPIOB clock. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    /* Enable USART1 clock on APB2. */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;

    /* Configure PB6 and PB7 for alternate-function mode. */
    GPIOB->MODER &= ~((3U << 12U) | (3U << 14U));
    GPIOB->MODER |= ((2U << 12U) | (2U << 14U));

    /* Select AF7 for USART1. */
    GPIOB->AFR[0] &= ~((0xFU << 24U) | (0xFU << 28U));
    GPIOB->AFR[0] |= ((7U << 24U) | (7U << 28U));

    /* Push-pull, no pull-up/down. */
    GPIOB->OTYPER &= ~((1U << 6U) | (1U << 7U));
    GPIOB->PUPDR &= ~((3U << 12U) | (3U << 14U));

    /* Reset USART configuration. */
    USART1->CR1 = 0U;
    USART1->CR2 = 0U;
    USART1->CR3 = 0U;

    /*
     * USART1 uses PCLK2, not PCLK1.
     * UART_PCLK2_HZ must match your clock configuration.
     */
    USART1->BRR =
        (UART_PCLK2_HZ + (UART_BAUDRATE / 2U)) / UART_BAUDRATE;

    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;
    USART1->CR1 |= USART_CR1_UE;
}

void uart_putc(char c)
{
    /* Wait until the transmit data register can accept a byte. */
    while ((USART1->SR & USART_SR_TXE) == 0U)
    {
    }

    /* Write the character to the transmit data register. */
    USART1->DR = (uint8_t)c;
}

void uart_puts(const char *str)
{
    if (str == 0)
    {
        return;
    }

    while (*str != '\0')
    {
        /* Convert LF to CRLF for terminal compatibility. */
        if (*str == '\n')
        {
            uart_putc('\r');
        }

        uart_putc(*str++);
    }
}

void uart_put_word(uint32_t word)
{
    for (int i = 31; i >= 0; --i)
        uart_putc((word & (1U << i)) ? '1' : '0');
}

void uart_flush(void)
{
    /* Wait until the final character has left the UART shift register. */
    while ((USART2->SR & USART_SR_TC) == 0U)
    {
    }
}