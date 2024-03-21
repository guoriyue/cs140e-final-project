#include "rpi-asm.h"

MK_FN(switch_to_sys_mode)
    mov r0, lr
    mov r1, sp
    cps #SYS_MODE
    prefetch_flush(r2)
    mov sp, r1
    mov lr, r0
    bx lr

MK_FN(sys_exit)
    push {lr}
    @ mov r1, r0
    mov r0, #2
    swi 1
    pop {lr}
    bx lr

MK_FN(sys_putc)
    push {lr}
    mov r1, r0
    mov r0, #1
    swi 1
    pop {lr}
    bx lr

MK_FN(cas_success)
    mov r0, #1 @ success
    pop {pc}

MK_FN(cas_fail)
    mov r0, #0
    pop {pc}

@ int cas(int *ptr, int oldvalue, int newvalue);
MK_FN(cas)
    push {lr}
    ldr r3, [r0]
    cmp r3, r1
    @ if (r3 != r1) return 0;
    bne cas_fail
    @ ex
    str r2, [r0]
    mov r3, #0
    @ strex r3, r2, [r0]
    cmp r3, #0 @ success or failure
    bne cas @ 1 means failure
    bl cas_success @ 0 means success


// use atomic operations to aqcuire the lock.
@ Returns 0 if the lock was not acquired and 1 if it was.
MK_FN(try_lock)
    mov     r1, #0
    swp     r2, r1, [r0]    
    @ stores the value in r1 into the address in r0, and stores the value at the address in r0 into r2
    mov     r0, r2
    blx     lr 

@ .globl system_enable_fiq
@ system_enable_fiq:
@     mrs r0,cpsr         @ move process status register (PSR) to r0
@     bic r0,r0,#(1<<6)	@ clear 6th bit. change to 4th bit, not enabling interrupts.
@     msr cpsr_c,r0		@ move r0 back to PSR
@     bx lr		        @ return. comment out this. 

@ .globl system_disable_fiq
@ system_disable_fiq:
@     mrs r0,cpsr		       
@     orr r0,r0,#(1<<6)	       @ set 6th bit: or in 0b100 0000
@     msr cpsr_c,r0
@     bx lr