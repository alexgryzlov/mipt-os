#include "vga.h"
#include "string.h"

static size_t terminal_row;
static size_t terminal_column;
static uint16_t* terminal_buffer;

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    uint8_t color = vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_DARK_GREY);
    terminal_buffer = (uint16_t*) 0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', color);
        }
    }
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
                    terminal_row = VGA_HEIGHT - 1;
                    terminal_shift_up();
                }
            }
    }
}

void terminal_write(const char* data, size_t size, uint8_t color) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar_color(data[i], color);
    }
}

void terminal_writestring_color(const char* data, uint8_t color) {
    terminal_write(data, strlen(data), color);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data), vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

void terminal_writenumber(int number, uint8_t color) {
    char buffer[32] = {};
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

void printf(char *format, uint8_t color, ...) {
    size_t n = strlen(format);

    va_list args;
    va_start(args, color);

    for (size_t i = 0; i < n; ++i) {

        //TODO: %% - escape character support
        //TODO: add %c, %b, %h (char, binary, hex)
        if (format[i] == '%') {
            ++i;
            switch (format[i]) {
                case 's': {
                    char* ptr = va_arg(args, char*);
                    terminal_writestring_color(ptr, color);
                    break;
                }
                case 'd': {
                    int number = va_arg(args, int);
                    terminal_writenumber(number, color);
                    break;
                }
                case 'c': {
                    char ch = va_arg(args, int);
                    terminal_putchar_color(ch, color);
                    break;
                }
            }
        }
        else {
            terminal_putchar_color(format[i], color);
        }
    }
    va_end(args);
}


void terminal_shift_up() {
    uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_DARK_GREY);
    for (size_t row = 0; row < VGA_HEIGHT - 1; ++row)
        memcpy(terminal_buffer + row * VGA_WIDTH, terminal_buffer + (row + 1) * VGA_WIDTH, VGA_WIDTH * sizeof(terminal_buffer[0]));
    for (size_t x = 0; x < VGA_WIDTH; ++x)
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', color);
}

void terminal_delete_last() {
    uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_DARK_GREY);
    if (terminal_column != 0) {
        terminal_column--;
    }
    else if (terminal_row != 0) {
        terminal_column = VGA_WIDTH - 1;
        terminal_row--;
    }
    terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(' ', color);
    return;
}