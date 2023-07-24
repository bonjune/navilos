#include "task.h"
#include "switch.h"


extern KernelTCB_t *sCurrent_tcb;
extern KernelTCB_t *sNext_tcb;

__attribute__((naked))
void Save_context(void) {
    // Save current task context onto the top of the current task stack
    __asm__("PUSH {lr}");
    __asm__("PUSH {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12}");
    __asm__("MRS r0, cpsr");
    __asm__("PUSH {r0}");


    // Save current task stack pointer into the current TCB
    // (The first word of a TCB is a stack pointer value)
    __asm__("LDR r0, =sCurrent_tcb");
    __asm__("LDR r0, [r0]");
    __asm__("STMIA r0!, {sp}");
}

__attribute((naked))
void Restore_context(void) {
    // Restore next task stack pointer from the next TCB
    // (Read the first word of a TCB)
    __asm__("LDR r0, =sNext_tcb");
    __asm__("LDR r0, [r0]");
    __asm__("LDMIA r0!, {sp}");

    // Restore next task context from the top of the next task stack
    __asm__("POP {r0}");
    __asm__("MSR cpsr, r0");
    __asm__("POP {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12}");
    __asm__("POP {pc}");
}