#ifndef MIPT_OS_PANIC_H
#define MIPT_OS_PANIC_H
#pragma once
#include "vga.h"
#include "irq.h"
#define STR_INDIR(x) #x
#define STR(x) STR_INDIR(x)

#define MAKE_PANIC panic("caused by " __FILE__ ", on line " STR(__LINE__) ".")
#define MAKE_PANIC_IF_NULL(a) if (a == NULL) panic("variable '" #a "' is nullptr, in '" __FILE__ "', on line " STR(__LINE__) ".")
#define MAKE_PANIC_IF_ZERO(a) if (a == 0) panic("variable '" #a "' is 0, in '" __FILE__ "', on line " STR(__LINE__) ".")
static inline void panic(const char* msg) {
    printf("PANIC! %s", FAILURE, msg);

    disable_irq();
    for (;;) {
        asm volatile ("hlt");
    }
}
#endif //MIPT_OS_PANIC_H
