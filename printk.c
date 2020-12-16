#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include "vga.h"

void terminal_writenumber(int number, uint8_t color, uint8_t base) {
    char digits[16] = "0123456789ABCDEF";
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

    for ( ; number > 0; ++pos, number /= base) {
        buffer[pos] = digits[number % base];
    }
    if (is_negative) {
        terminal_putchar_color('-', color);
    }
    for (int i = pos - 1; i >= 0; --i) {
        terminal_putchar_color(buffer[i], color);
    }
}

void vprintk(char *format, va_list args) {
    size_t n = strlen(format);
    uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

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
                    terminal_writenumber(number, color, 10);
                    break;
                }
                case 'b': {
                    int number = va_arg(args, int);
                    terminal_writestring_color("0b", color);
                    terminal_writenumber(number, color, 2);
                    break;
                }
                case 'x': {
                    int number = va_arg(args, int);
                    terminal_writestring_color("0x", color);
                    terminal_writenumber(number, color, 16);
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


void printk(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}
