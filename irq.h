#ifndef MIPT_OS_IRQ_H
#define MIPT_OS_IRQ_H

#pragma once

static inline void disable_irq() {
    asm volatile ("cli");
}

static inline void enable_irq() {
    asm volatile ("sti");
}

#endif //MIPT_OS_IRQ_H
