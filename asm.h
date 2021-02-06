#ifndef MIPT_OS_ASM_H
#define MIPT_OS_ASM_H
#include <stdint.h>

static inline char inb(uint16_t port) {
    char data;

    asm volatile("in %1,%0" : "=a" (data) : "d" (port));
    return data;
}

#endif //MIPT_OS_ASM_H