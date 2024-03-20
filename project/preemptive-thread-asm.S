#include "rpi-asm.h"
MK_FN(pre_cswitch)
    push {r4-r11,lr}
    str sp, [r0]
    ldr sp, [r1] @ [] is defererencing need to understand pointer and scalar
    @ mov sp, r1
    pop {r4-r11, lr}
    bx lr

MK_FN(switch_to_sys_mode)
    mov r0, lr
    mov r1, sp
    cps #SYS_MODE
    prefetch_flush(r2)
    mov sp, r1
    mov lr, r0
    bx lr