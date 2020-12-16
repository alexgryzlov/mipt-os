#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "idt.h"
#include "gdt.h"
#include "acpi.h"
#include "apic.h"
#include "irq.h"
#include "vga.h"
#include "panic.h"
#include "paging.h"
#include "sched.h"
#include "pci.h"
#include "multiboot.h"
#include "mmap.h"
#include "printk.h"

void kalloc_test() {
    void* addr1 = kalloc(5, PT_WRITEABLE);
    void* addr2 = kalloc(6, PT_WRITEABLE);
    kfree(addr1, 5);
    kfree(addr2, 6);
    void* addr3 = kalloc(5, PT_WRITEABLE);
    void* addr4 = kalloc(6, PT_WRITEABLE);
    kfree(addr3, 5);
    kfree(addr4, 6);
    if (addr1 == addr3 && addr2 == addr4) {
        printk("Reverse order kfree - SUCCESS\n");
    } else {
        printk("Reverse order kfree - FAIL\n");
    }
    addr1 = kalloc_page();
    kfree_page(addr1);
    addr2 = kalloc_page();
    kfree_page(addr2);
    if (addr1 == addr2) {
        printk("Kalloc Free Kalloc - SUCCESS\n");
    } else {
        printk("Kalloc Free Kalloc - FAIL\n");
    }
    addr1 = kalloc_page();
    addr2 = kalloc_page();
    kfree_page(addr1);
    kfree_page(addr2);
    addr3 = kalloc_page();
    addr4 = kalloc_page();
    if (addr1 == addr3 && addr2 == addr4) {
        printk("Reverse order kfree_page - SUCCESS\n");
    } else {
        printk("Reverse order kfree_page - FAIL\n");
    }
}

void kernel_main(multiboot_info_t* mbd, unsigned int magic) {
    init_gdt();
    init_idt();

    init_kalloc_early();
    init_kernel_paging(mbd);
    terminal_initialize();

    struct acpi_sdt* rsdt = acpi_find_rsdt();
    if (!rsdt) {
        panic("RSDT not found!");
    }

    validate_checksum(rsdt, rsdt->header.length);
    printk("RSDT checksum successfully validated!\n");

    apic_init(rsdt);

//    pci_print_devices();
    identity_map(mbd, PAGE_SIZE);
    show_memory(mbd);
    kernel_paging_4mb();
    init_kalloc();
    kalloc_test();

    asm volatile ("sti");
    while (1) {}
    scheduler_start();
}
