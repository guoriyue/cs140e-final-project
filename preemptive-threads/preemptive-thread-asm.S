/*
 * implement these.  ordered in difficulty.
 */
#include "rpi-asm.h"

@ return the current stack pointer.
MK_FN(preemptive_get_sp)
    mov r0, sp
    bx lr

@ MK_FN(switchto_user_asm)
@ because we are switching from exception
MK_FN(preemptive_cswitch)
    mov sp, r0
    @ copies the value from register r0 into the stack pointer sp.
    add r1, sp, #64
    mov r0, #INT_STACK_ADDR

    mov sp, r1
    
    ldm r0, {r0-r12,sp,lr}^
    ldm r0, {sp}^
    @ cps #USER_MODE

    rfe sp
    @ bx lr
    asm_not_reached();
    
@ MK_FN(switchto_user_asm)
@     @ asm_todo("implement this");
@     mov sp, r0
    
@     add r1, sp, #60  @ 15
@     @ add r2, sp, #64  @ 16

@     @ ldr lr, {r1}
@     mov sp, r1
    
@     ldm r0, {r0-r12,sp,lr}^
@     @ cps #USER_MODE

@     rfe sp
@     @ bx lr
@     asm_not_reached();


@ [Make sure you can answer: why do we need to do this?]
@
@ use this to setup each thread for the first time.
@ setup the stack so that when cswitch runs it will:
@	- load address of <preemptive_init_trampoline> into LR
@	- <code> into r1, 
@	- <arg> into r0
@ 
MK_FN(preemptive_init_trampoline)
    @ asm_todo("implement this")
    mov r0, r4
    mov r1, r5
    blx r1
    bl preemptive_exit
    @ bx lr



.align 5; 
.globl preemptive_threads_handlers
preemptive_threads_handlers: 
  ldr pc, =unhandled_reset
  ldr pc, =unhandled_undefined_instruction
  ldr pc, =unhandled_swi
  ldr pc, =unhandled_prefetch_abort
  ldr pc, =unhandled_data_abort
  ldr pc, =interrupt_asm

@ only handler that should run since we only enable general interrupts
interrupt_asm:
  mov sp, #INT_STACK_ADDR   
  sub   lr, lr, #4
  push  {r0-r12,lr}         @ XXX: pushing too many registers: only need caller
  mov   r0, lr              @ Pass old pc
  bl    interrupt_vector    @ C function
  pop   {r0-r12,lr}         @ pop integer registers
  movs    pc, lr        @ moves the link register into the pc and implicitly
                        @ loads the PC with the result, then copies the
                        @ SPSR to the CPSR.


syscall_full:
    mov sp, #INT_STACK_ADDR
    @ b staff_syscall_trampoline_full

    stm sp, {r0-r12, sp, lr}^
    str lr, [sp, #60]
    mrs r12, spsr
    str r12, [sp, #64]

    mov r0, sp
    mrs r1, spsr
    mov r2, lr

    bl syscall_vector
    asm_not_reached();