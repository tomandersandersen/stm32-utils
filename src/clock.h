
#ifndef CLOCK_H
#define CLOCK_H

#include <stdbool.h>
#include <stdint.h>

bool clock_init(void);
uint32_t clock_get_core_hz(void);


#endif
