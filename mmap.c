#include "mmap.h"
#include "panic.h"
#include "paging.h"

mmap_entry_t* entries[1024];
unsigned int count = 0;

mmap_entry_t* next_mmap_entry(mmap_entry_t* current) {
    return (mmap_entry_t*) ((unsigned int) current + current->size + sizeof(current->size));
}

int is_available(mmap_entry_t* entry) {
    return entry->type == 1;
}

void print(mmap_entry_t* entry, int number) {
    printk("Entry %d:\n", number);
    printk("high addr = %x; low addr = %x; high len = %d; low len = %d; ",
           entry->base_addr_high, entry->base_addr_low,
           entry->length_high, entry->length_low);
    char* type;
//    uint8_t color = FAILURE;
    switch (entry->type) {
        case 1:
            type = "AVAILABLE";
//            color = SUCCESS;
            break;
        case 3:
            type = "USED FOR ACPI";
            break;
        case 4:
            type = "RESERVED FOR HIBERNATION";
            break;
        case 5:
            type = "DEFECTIVE RAM MODULE";
            break;
        default:
            type = "RESERVED";

    }
    printk("%s\n\n", type);
}


void show_memory(multiboot_info_t* mbd) {

    if (!(mbd->flags & MULTIBOOT_INFO_MEM_MAP)) {
        panic("Multiboot couldn't find the memory mapping.");
    }
    mmap_entry_t* entry = mbd->mmap_addr;
    for (int i = 0;
         entry < mbd->mmap_addr + mbd->mmap_length;
         ++i, entry = next_mmap_entry(entry)) {
        if (is_available(entry))
            entries[count++] = entry;
        print(entry, i);
    }
}

struct available_ram_entries get_entries() {
    struct available_ram_entries result = {.entry = entries, .count = count};
    return result;
}