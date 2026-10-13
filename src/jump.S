.syntax unified
.cpu cortex-m4
.thumb

.global jump_to_application
.type jump_to_application, %function
.thumb_func

/* r0 = application vector table base */
jump_to_application:
    cpsid   i               /* Disable interrupts */

    ldr     r1, [r0, #0]    /* Application initial MSP */
    ldr     r2, [r0, #4]    /* Application Reset_Handler */

    movs    r3, #0
    msr     CONTROL, r3     /* Privileged Thread mode, use MSP */
    isb

    msr     MSP, r1         /* Switch to application stack */
    bx      r2              /* Branch; do not return */