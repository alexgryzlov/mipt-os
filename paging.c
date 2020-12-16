#include "paging.h"
#include "common.h"
#include "panic.h"
#include "irq.h"
#include "mmap.h"

#include <stdint.h>

uint32_t early_pgdir[1024] __attribute__((aligned(4096), section(".boot.bss")));
uint32_t pages_cnt = 0;

struct fl_entry {
    struct fl_entry* next;
};

struct kalloc_head {
    struct fl_entry* freelist_head;
};

struct kalloc_head kalloc_head;

__attribute__((section(".boot.text"))) void setup_paging_early() {
    // 0x0...4Mb identity mapped
    // KERNEL_HIGH...KERNEL_HIGH+4Mb mapped to 0x0...4Mb
    early_pgdir[0] = PT_PRESENT | PT_WRITEABLE | PT_PAGE_SIZE;
    early_pgdir[PGDIR_IDX(KERNEL_HIGH)] = PT_PRESENT | PT_WRITEABLE | PT_PAGE_SIZE;
}

void init_kalloc_early() {
    void* addr = (void*)ROUNDUP((uint32_t)&KERNEL_END);
    kalloc_head.freelist_head = NULL;
    void* end = KERNEL_HIGH + 4 * (1 << 20);
    while (addr < end) {
        struct fl_entry* entry = (struct fl_entry*)addr;
        entry->next = kalloc_head.freelist_head;
        kalloc_head.freelist_head = entry;
        addr += PAGE_SIZE;
    }
}

void* kalloc_page() {
    // TODO: spinlock needed here.

    if (!kalloc_head.freelist_head) {
        return NULL;
    }

    void* ptr = kalloc_head.freelist_head;
    kalloc_head.freelist_head = kalloc_head.freelist_head->next;
    return ptr;
}

void* kalloc(size_t size, int flags) {
    struct fl_entry* head_pointer = kalloc_head.freelist_head;
    for (uint32_t i = 0; i < size - 1; ++i) {
        head_pointer = head_pointer->next;
    }
    void* result = (void*)head_pointer;
    kalloc_head.freelist_head = head_pointer->next;
    return result;
}

void kfree_page(void* p) {
    kfree(p, 1);
}

// kfree will make sure that freelist is sorted
// NULL <-- FL1 <-- FL2 <-- FL3 ...
// >>>>>>>>
// increasing
void kfree(void* addr, size_t size) {
    struct fl_entry* prev = NULL;
    struct fl_entry* curr = kalloc_head.freelist_head;
    struct fl_entry* ptr = addr;
    while (curr > ptr) {
        prev = curr;
        curr = curr->next;
    }
    if (prev == NULL) {
        for (int i = 0; i < size; ++i) {
            struct fl_entry * new_head = (struct fl_entry *) (addr + i * PAGE_SIZE);
            new_head->next = kalloc_head.freelist_head;
            kalloc_head.freelist_head = new_head;
        }
    } else {
        for (int i = 0; i < size; ++i) {
            prev->next = (struct fl_entry *) (addr + i * PAGE_SIZE);
            prev->next->next = curr;
            curr = prev->next;
        }
    }
}

uint32_t* alloc_page(uint32_t* pgdir, void* addr, int user) {
    uint32_t* page_table = NULL;
    if (pgdir[PGDIR_IDX(addr)] & PT_PRESENT) {
        page_table = phys2virt((void*)ROUNDDOWN(pgdir[PGDIR_IDX(addr)]));
    } else {
        page_table = kalloc_page();
        if (!page_table) {
            return NULL;
        }
        for (int i = 0; i < 1024; i++) {
            page_table[i] = 0;
        }
    }

    int flags = PT_PRESENT;
    if (user) {
        flags |= PT_USER;
    }
    pgdir[PGDIR_IDX(addr)] = ((uint32_t)virt2phys(page_table)) | flags;
    return &page_table[PT_IDX(addr)];
}

void map_continous(uint32_t* pgdir, void* addr, size_t size, void* phys_addr, int flags) {
    addr = (void*)ROUNDDOWN((uint32_t)addr);
    phys_addr = (void*)ROUNDDOWN((uint32_t)phys_addr);
    size = ROUNDUP(size);

    while (size > 0) {
        uint32_t* pte = alloc_page(pgdir, addr, flags & PT_USER);
        *pte = ((uint32_t)phys_addr) | PT_PRESENT;
        *pte |= flags;
        addr += PAGE_SIZE;
        phys_addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
}

uint32_t* kernel_pgdir = NULL;

void load_cr3(uint32_t* pgdir) {
    asm volatile (
    "mov %0, %%cr3\n"
    :
    : "r"(pgdir)
    : "memory"
    );
}

void init_kernel_paging() {
    kernel_pgdir = kalloc_page();
    memset(kernel_pgdir, '\0', PAGE_SIZE);
    map_continous(kernel_pgdir, (void*)KERNEL_HIGH, 4 * (1 << 20), 0x0, PT_WRITEABLE);
    map_continous(kernel_pgdir, &USERSPACE_START, 4096, virt2phys(&USERSPACE_START), PT_USER | PT_WRITEABLE);
    load_cr3(virt2phys(kernel_pgdir));
}

void kernel_paging_4mb() {
    uint32_t BIG_PAGE = 1 << 22;
    for (int i = 0; i < get_entries().count; ++i) {
        mmap_entry_t *entry = get_entries().entry[i];
        identity_map(entry, PAGE_SIZE);
        uint32_t addr = (uint32_t) entry->base_addr_low;
        uint32_t end = (uint32_t) entry->base_addr_low + entry->length_low;

        addr = (addr + BIG_PAGE - 1) & (~(BIG_PAGE - 1));
        end = end & (~(BIG_PAGE - 1));

        unsigned int offset = 1 << 22;
        while (addr < end) {
            kernel_pgdir[PGDIR_IDX(KERNEL_HIGH + offset)] =
                    ((uint32_t) addr) | PT_PRESENT | PT_WRITEABLE | PT_PAGE_SIZE;
            offset += BIG_PAGE;
            addr += BIG_PAGE;
            pages_cnt++;
        }
    }
    load_cr3(virt2phys(kernel_pgdir));
}

void init_kalloc() {
    void* addr = KERNEL_HIGH + (1 << 22);
    void* end = KERNEL_HIGH + pages_cnt * (1 << 22);
    while (addr < end) {
        struct fl_entry* entry = (struct fl_entry*)addr;
        entry->next = kalloc_head.freelist_head;
        kalloc_head.freelist_head = entry;
        addr += PAGE_SIZE;
    }
    printk("kalloc initialized\n");
}

void identity_map(void* addr, size_t sz) {
    map_continous(kernel_pgdir, addr, sz, addr, PT_WRITEABLE);
}

uint32_t read_cr2() {
    uint32_t val = 0;
    asm volatile (
    "mov %%cr2, %0\n"
    : "=r"(val)
    );
    return val;
}

void pagefault_irq(struct regs* regs) {
    (void)regs;

    void* addr = (void*)read_cr2();

    if (!is_userspace(regs)) {
        panic("pagefault in kernel space\n");
    }
}