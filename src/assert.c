#include "uart.h"

__attribute__((noreturn)) void _assert_failed(const char *expr,
                                              const char *file,
                                              uint32_t line)
{
    uart_puts("Assert failed: ");
    uart_puts(file);
    uart_puts(":");
    uart_put_word(line);
    uart_puts(" (");
    uart_puts(expr);
    uart_puts(")\r\n");

    while (1)
    {
        /* Halt execution */
    }
}