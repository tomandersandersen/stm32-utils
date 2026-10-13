#include "log.h"

void log_hexdump(const char *level,
                               const char *label,
                               const uint8_t *data,
                               size_t len)
{
    static const char hex[] = "0123456789ABCDEF";

    uart_puts(level);
    uart_puts(label);
    uart_puts(": ");

    for (size_t i = 0; i < len; ++i) {
        uart_putc(hex[data[i] >> 4U]);
        uart_putc(hex[data[i] & 0x0FU]);
        uart_putc(' ');
    }

    uart_puts("\n");
}