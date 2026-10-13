#pragma once

#define UART_BAUDRATE 115200U

#include <stdint.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *str);
void uart_flush(void);
void uart_put_word(uint32_t word);
