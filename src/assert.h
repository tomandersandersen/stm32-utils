#pragma once

#define ASSERT(expr)                                   \
    do                                                 \
    {                                                  \
        if (!(expr))                                   \
            _assert_failed(#expr, __FILE__, __LINE__); \
    } while (0)

__attribute__((noreturn)) void _assert_failed(const char *expr,
                                              const char *file,
                                              uint32_t line);
