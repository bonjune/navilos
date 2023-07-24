#include "stdint.h"
#include "stdbool.h"

#include "stdio.h"
#include "event.h"


static uint32_t sEventFlag;

void Kernel_event_flag_init(void) {
    sEventFlag = 0;
}

void Kernel_event_flag_set(KernelEventFlag_t flag) {
    sEventFlag |= (uint32_t)flag;
}

void Kernel_event_flag_clear(KernelEventFlag_t flag) {
    sEventFlag &= ~((uint32_t)flag);
}

bool Kernel_event_flag_check(KernelEventFlag_t flag) {
    if (sEventFlag & (uint32_t)flag) {
        Kernel_event_flag_clear(flag);
        return true;
    }
    return false;
}