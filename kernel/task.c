#include "stdint.h"
#include "stdbool.h"

#include "ARMv7AR.h"
#include "task.h"

#include "switch.h"

static KernelTCB_t sTask_list[MAX_TASK_NUM];
static uint32_t sAllocated_tcb_index;

static uint32_t sCurrent_tcb_index;
static KernelTCB_t *Scheduler_round_robin(void);

KernelTCB_t *sCurrent_tcb;
KernelTCB_t *sNext_tcb;


// Initialize task context and stack space for each task
void Kernel_task_init(void) {
    sAllocated_tcb_index = 0;
    sCurrent_tcb_index = 0;
    
    for (uint32_t i = 0; i < MAX_TASK_NUM; i++) {
        sTask_list[i].stack_base = (uint8_t*)(TASK_STACK_START + (i * USER_TASK_STACK_SIZE));

        // Stack grows backward
        // empty 4 bytes to draw a boundary between stack spaces
        sTask_list[i].sp = (uint32_t)sTask_list[i].stack_base + USER_TASK_STACK_SIZE - 4;

        // Place task context at the start of the stack
        sTask_list[i].sp -= sizeof(KernelTaskContext_t);

        KernelTaskContext_t *ctx = (KernelTaskContext_t*)sTask_list[i].sp;
        ctx->pc = 0;
        ctx->spsr = ARM_MODE_BIT_SYS;
    }
}


uint32_t Kernel_task_create(KernelTaskFunc_t startFunc) {
    const uint32_t current_task_index = sAllocated_tcb_index;
    KernelTCB_t *tcb = &sTask_list[current_task_index];
    sAllocated_tcb_index++;

    if (sAllocated_tcb_index > MAX_TASK_NUM) {
        return NOT_ENOUGH_TASK_NUM;
    }

    KernelTaskContext_t *ctx = (KernelTaskContext_t*)tcb->sp;
    ctx->pc = (uint32_t)startFunc;

    return current_task_index;
}


static KernelTCB_t *Scheduler_round_robin(void) {
    sCurrent_tcb_index++;
    sCurrent_tcb_index %= sAllocated_tcb_index;

    return &sTask_list[sCurrent_tcb_index];
}


void Kernel_task_scheduler(void) {
    sCurrent_tcb = &sTask_list[sCurrent_tcb_index];
    sNext_tcb = Scheduler_round_robin();

    Kernel_task_context_switching();
}


__attribute__((naked))
void Kernel_task_context_switching(void) {
    __asm__("B Save_context");
    __asm__("B Restore_context");
}


// This function should be called only once!
void Kernel_task_start(void) {
    sNext_tcb = &sTask_list[sCurrent_tcb_index];
    Restore_context();
}
