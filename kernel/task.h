#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include "stdint.h"

#include "MemoryMap.h"

#define NOT_ENOUGH_TASK_NUM 0xFFFFFFFF

#define USER_TASK_STACK_SIZE 0x100000 // 1MB
#define MAX_TASK_NUM (TASK_STACK_SIZE / USER_TASK_STACK_SIZE) // 64

typedef struct KernelTaskContext_t {
    uint32_t spsr; // saved cpsr (current program status register)
    uint32_t r0_r12[13];
    uint32_t pc; // lr (link register, saves the return address)
} KernelTaskContext_t;

typedef struct KernelTCB_t {
    uint32_t sp;
    uint8_t *stack_base;
} KernelTCB_t;

typedef void (*KernelTaskFunc_t)(void);

void Kernel_task_init(void);
void Kernel_task_start(void);
uint32_t Kernel_task_create(KernelTaskFunc_t startFunc);

void Kernel_task_scheduler(void);
void Kernel_task_context_switching(void);

#endif /* KERNEL_TASK_H */