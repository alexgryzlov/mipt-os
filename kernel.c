#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "idt.h"
#include "gdt.h"
#include "acpi.h"
#include "apic.h"
#include "vga.h"
#include "panic.h"
#include "spinlock.h"
#include "keyboard.h"


__attribute__ ((interrupt)) void syscall_entry(struct iframe* frame) {
    terminal_writestring_color("Syscall!\n", vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
    (void)frame;
}

static int lock = 0;

__attribute__ ((interrupt)) void timer_isr(struct iframe* frame) {
    (void)frame;

    spin_lock(&lock);
//    printf(".", vga_entry_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK));
    spin_unlock(&lock);

    apic_eoi();
}

__attribute__ ((interrupt)) void keyboard_isr(struct iframe* frame) {
    (void)frame;

    spin_lock(&lock);
    int ch = kbdgetc();
    if (ch == 8) { // backspace
        //TODO: start from the previous \n, not from the end of line
        terminal_delete_last();
    }
    else if (ch > 0) {
        printf("%c", vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK), ch);
    }
    spin_unlock(&lock);

    apic_eoi();
}

extern void jump_userspace();

void kernel_main(void) {
    uint8_t DEFAULT_COLOR = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    init_gdt();
    init_idt();

	terminal_initialize();
    struct acpi_sdt* rsdt = acpi_find_rsdt();
    if (!rsdt) {
        panic("RSDT not found!");
    }
    validate_checksum(rsdt, rsdt->header.length);
    printf("RSDT checksum successfully validated!\n", DEFAULT_COLOR);

    apic_init(rsdt);
    char * data ="12312";

    asm volatile ("sti");
    // jump_userspace();
}
