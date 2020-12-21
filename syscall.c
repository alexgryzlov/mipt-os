#include "syscall.h"
#include "panic.h"
#include "common.h"
#include "errno.h"
#include "timer.h"
#include "sched.h"
#include "bug.h"
#include "paging.h"
#include "printk.h"

uint32_t syscall_wait(struct regs* regs) {
    int ticks = regs->ebx;
    if (ticks <= 0) {
        return -EINVAL;
    }

    current->ticks_remaining = ticks;
    current->state = TASK_WAITING;
    reschedule();
    return 0;
}

uint32_t syscall_print(struct regs* regs) {
    printk("syscall_print\n");
}

uint32_t syscall_brk(struct regs* regs) {
    uint32_t size = ROUNDUP(regs->ebx);
    void* addr = kalloc(size / PAGE_SIZE, 0);
    MAKE_PANIC_IF_NULL(addr);
    addr = virt2phys(addr);
    if (PGDIR_IDX(ind) >= PGDIR_IDX(KERNEL_HIGH)) {
        panic("crossed KERNEL_HIGH");
    }
    current->pgdir[PGDIR_IDX(ind)] = (uint32_t)(addr) & ~((1 << 22) - 1);
    current->mem_end += size;
}

syscall_fn syscall_table[] = {
    [0] = syscall_wait,
    [1] = syscall_print,
    [2] = syscall_brk,
};


void syscall_irq(struct regs* regs) {
    BUG_ON(!is_userspace(regs));
    BUG_ON_NULL(current);

    if (regs->eax >= ARRAY_SIZE(syscall_table)) {
        regs->eax = -ENOSYS;
        return;
    }

    regs->eax = syscall_table[regs->eax](regs);
}
