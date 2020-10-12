#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#include "interrupts.h"

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) {
	size_t len = 0;
	while (str[len]) {
		len++;
    }
	return len;
}

void memcpy(void *dest, void *src, size_t n)  {
    char *raw_src = (char *)src;
    char *raw_dest = (char *)dest;

    for (size_t i = 0; i < n; ++i)
        raw_dest[i] = raw_src[i];
}


static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

size_t terminal_row;
size_t terminal_column;
uint16_t* terminal_buffer;

void terminal_initialize(void) {
	terminal_row = 0;
	terminal_column = 0;
	uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_DARK_GREY);
	terminal_buffer = (uint16_t*) 0xB8000;
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', color);
		}
	}
}

void terminal_shift_up() {
    uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_DARK_GREY);
    for (size_t row = 0; row < VGA_HEIGHT - 1; ++row)
        memcpy(terminal_buffer + row * VGA_WIDTH, terminal_buffer + (row + 1) * VGA_WIDTH, VGA_WIDTH);
    for (size_t x = 0; x < VGA_WIDTH; ++x)
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', color);
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar_color(char c, uint8_t color) {
    switch (c) {
    case '\n':
        terminal_row++;
        terminal_column = 0;
        if (terminal_row == VGA_HEIGHT) {
            terminal_row = VGA_HEIGHT - 1;
            terminal_shift_up();
        }
        break;

    default:
        terminal_putentryat(c, color, terminal_column, terminal_row);
        terminal_column++;
        if (terminal_column == VGA_WIDTH) {
            terminal_column = 0;
            terminal_row++;
            if (terminal_row == VGA_HEIGHT) {
                terminal_row = 0;
            }
        }
    }
}

void terminal_write(const char* data, size_t size, uint8_t color) {
	for (size_t i = 0; i < size; i++) {
        terminal_putchar_color(data[i], color);
    }
}

void terminal_writestring(const char* data) {
	terminal_write(data, strlen(data), vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_DARK_GREY));
}

void terminal_writenumber(int number) {
    char buffer[32] = {};
    uint8_t color = vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_DARK_GREY);
    size_t pos = 0;
    char is_negative = false;
    if (number == 0) {
        terminal_putchar_color('0', color);
        return;
    }
    if (number < 0) {
        is_negative = true;
        number = -number;
    }

    for ( ; number > 0; ++pos, number /= 10) {
        buffer[pos] = number % 10;
    }
    if (is_negative) {
        terminal_putchar_color('-', color);
    }
    for (int i = pos - 1; i >= 0; --i) {
        terminal_putchar_color(buffer[i] + '0', color);
    }
}


void terminal_writestring_color(const char* data, uint8_t color) {
    terminal_write(data, strlen(data), color);
}


//TODO
void move_terminal(int direction) {

}

void printf(char *format, ...) {
    size_t n = strlen(format);

    va_list args;
    va_start(args, format);

    for (size_t i = 0; i < n; ++i) {

        //TODO: %% - escape character support
        //TODO: add %c, %b, %h (char, binary, hex)
        if (format[i] == '%') {
            ++i;
            switch (format[i]) {
                case 's': {
                    char* ptr = va_arg(args, char*);
                    terminal_writestring(ptr);
                    break;
                }
                case 'd': {
                    int number = va_arg(args, int);
                    terminal_writenumber(number);
                    break;
                }
            }
        }
        else {
            terminal_putchar_color(format[i], vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_LIGHT_GREY));
        }
    }

    va_end(args);
}

__attribute__ ((interrupt)) void isr0(struct iframe* frame) {
    terminal_writestring_color("Hello world!\n", vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_DARK_GREY));
    (void)frame;
}

//TODO: make interrupt work, add circular buffer for terminal history
__attribute__ ((interrupt)) void isr9(struct iframe* frame) {
    asm volatile("cli\n\t");
    const unsigned char PIC_port = 0x20;
    asm volatile ("out %0, $0x20\n\t"
                  :
                  : "r"(PIC_port)
                  );

    volatile unsigned char key_code = 0;
    asm volatile("in $0x60, %%al\n\t"
                 "movb %%al, %0\n\t"
                 : "=r"(key_code)
                 );
    printf("%d", key_code);
    if (key_code == 0x04) {
        printf("3 has been pressed!\n");
    }
//    move_terminal();
    (void)frame;
}


void kernel_main(void) {
    init_idt();
	terminal_initialize();
    asm volatile ("sti");
//    asm volatile ("int $0x80");
    for (int i = 0; i < VGA_HEIGHT; ++i) {
        printf("%d\n", i);
    }
    printf("im on the last row!");
}
