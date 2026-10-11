#include <stdint.h>

extern uint32_t _sidata;  // Start of .data in Flash
extern uint32_t _sdata;   // Start of .data in RAM
extern uint32_t _edata;   // End of .data in RAM

extern uint32_t _sbss;    // Start of .bss
extern uint32_t _ebss;    // End of .bss

extern int main(void);

__attribute__((noreturn))
void _start(void)
{
    /* Copy initialized data from Flash to RAM. */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* Zero-initialize .bss. */
    for (dst = &_sbss; dst < &_ebss;) {
        *dst++ = 0;
    }

    main();

    /* main() should not return. */
    while (1) {
        __asm volatile ("nop");
    }
}
