
#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stddef.h>

#include "uart.h"

void log_hexdump(const char *level,
                 const char *label,
                 const uint8_t *data,
                 size_t len);

#define LOG_INFO(msg)          \
    do                         \
    {                          \
        uart_puts("[INFO]     "); \
        uart_puts(msg);        \
        uart_puts("\n");       \
    } while (0)

#define LOG_DEBUG(msg)         \
    do                         \
    {                          \
        uart_puts("[DEBUG]    "); \
        uart_puts(msg);        \
        uart_puts("\n");       \
    } while (0)

#define LOG_ERROR(msg)         \
    do                         \
    {                          \
        uart_puts("[ERROR]   "); \
        uart_puts(msg);        \
        uart_puts("\n");       \
    } while (0)

#define LOG_WARNING(msg)         \
    do                         \
    {                          \
        uart_puts("[WARNING] "); \
        uart_puts(msg);        \
        uart_puts("\n");       \
    } while (0)

#define LOG_INFO_HEX(label, data, len) \
    log_hexdump("[INFO]  ", label, data, len)

#define LOG_DEBUG_HEX(label, data, len) \
    log_hexdump("[DEBUG] ", label, data, len)

#endif
