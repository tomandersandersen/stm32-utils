ENTRY(Reset_Handler)

/* STM32F407VGT6 memory layout */
_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size  = 0x200;
_Min_Stack_Size = 0x800;

MEMORY
{
    FLASH (rx) : ORIGIN = 0x08010000, LENGTH = 960K
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
    CCMRAM (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    /* Interrupt vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Read-only constants */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* ARM exception index tables */
    .ARM.extab :
    {
        *(.ARM.extab*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    /* Initialized data: load from Flash, execute from RAM */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    /* Zero-initialized data */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Optional CPU-only CCM RAM section */
    .ccmram (NOLOAD) :
    {
        . = ALIGN(4);
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
    } > CCMRAM

    /* Heap and stack reservation */
    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_start__ = .;
        . += _Min_Heap_Size;
        __heap_end__ = .;
        . += _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    ASSERT(. <= ORIGIN(RAM) + LENGTH(RAM),
        "RAM overflow: heap/stack exceed RAM")

    /* Discard unused metadata */
    /DISCARD/ :
    {
        *(.note.GNU-stack)
    }
}
