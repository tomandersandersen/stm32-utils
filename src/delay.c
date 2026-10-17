#include "delay.h"

#include "clock.h"

#include "stm32f407xx.h"

static uint32_t core_clock_hz;

#define DELAY_MAX_CHUNK_US 100000

void delay_init(void)
{
    core_clock_hz = clock_get_core_hz();

    /* Activate the trace system. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Zero cycle counter and initialize counting. */
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_us(uint32_t us)
{
    while (us > 0U)
    {
        /* Wait in "chunks" to prevent 'cycles' from overflowing. */
        uint32_t chunk =
            (us > DELAY_MAX_CHUNK_US)
                ? DELAY_MAX_CHUNK_US
                : us;

        uint32_t cycles =
            (uint32_t)(((uint64_t)core_clock_hz * chunk) / 1000000U);
        uint32_t start = DWT->CYCCNT;

        while ((uint32_t)(DWT->CYCCNT - start) < cycles)
        {
            /* Busy wait */
        }

        us -= chunk;
    }
}

void delay_ms(uint32_t ms)
{
    while (ms > 0U)
    {
        uint32_t chunk = (ms > 1000U) ? 1000U : ms;

        delay_us(chunk * 1000U);
        ms -= chunk;
    }
}
