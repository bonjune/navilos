#include "Kernel.h"

#include "memio.h"

#include "task.h"
#include "event.h"

void Kernel_start(void) {
    Kernel_task_start();
}

void Kernel_yield(void) {
    // The scheduler is based on Round Robin manner,
    // so it is enough to just switch the current task to the next task.
    Kernel_task_scheduler();
}

void Kernel_send_events(uint32_t event_list) {
    for (uint32_t i = 0; i < 32; i++) {
        if ((event_list >> i) & 0x1) {
            KernelEventFlag_t sending_event = KernelEventFlag_Empty;
            sending_event = (KernelEventFlag_t)SET_BIT(sending_event, i);
            Kernel_event_flag_set(sending_event);
        }
    }
}

KernelEventFlag_t Kernel_wait_events(uint32_t waiting_list) {
    for (uint32_t i = 0; i < 32; i ++) {
        if ((waiting_list >> i) & 0x1) {
            KernelEventFlag_t waiting_event = KernelEventFlag_Empty;
            waiting_event = (KernelEventFlag_t)SET_BIT(waiting_event, i);
            if (Kernel_event_flag_check(waiting_event)) {
                return waiting_event;
            }
        }
    }

    return KernelEventFlag_Empty;
}