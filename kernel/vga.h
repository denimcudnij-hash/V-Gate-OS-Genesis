#ifndef VGA_H
#define VGA_H

enum vga_color {
    VGA_BLACK    = 0,
    VGA_BLUE     = 1,
    VGA_GREEN    = 2,
    VGA_CYAN     = 3,
    VGA_RED      = 4,
    VGA_MAGENTA  = 5,
    VGA_BROWN    = 6,
    VGA_LGRAY    = 7,
    VGA_DGRAY    = 8,
    VGA_LBLUE    = 9,
    VGA_LGREEN   = 10,
    VGA_LCYAN    = 11,
    VGA_LRED     = 12,
    VGA_LMAG     = 13,
    VGA_YELLOW   = 14,
    VGA_WHITE    = 15,
};

void vga_init(void);
void vga_putchar(char c);
void vga_print(const char *str);
void vga_setcolor(int fg, int bg);
void vga_clear(void);

#endif