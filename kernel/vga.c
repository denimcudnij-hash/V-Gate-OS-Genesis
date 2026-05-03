#include "vga.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static unsigned short *vga = (unsigned short*)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;
static unsigned char color = 0x07;

static void update_cursor(void)
{
    unsigned short pos = cursor_y * VGA_WIDTH + cursor_x;
    __asm__ volatile (
        "outb %0, %1" : : "a"((unsigned char)0x0F), "Nd"((unsigned short)0x3D4)
    );
    __asm__ volatile (
        "outb %0, %1" : : "a"((unsigned char)(pos & 0xFF)), "Nd"((unsigned short)0x3D5)
    );
    __asm__ volatile (
        "outb %0, %1" : : "a"((unsigned char)0x0E), "Nd"((unsigned short)0x3D4)
    );
    __asm__ volatile (
        "outb %0, %1" : : "a"((unsigned char)((pos >> 8) & 0xFF)), "Nd"((unsigned short)0x3D5)
    );
}

static void scroll(void)
{
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga[y * VGA_WIDTH + x] = vga[(y+1) * VGA_WIDTH + x];

    for (int x = 0; x < VGA_WIDTH; x++)
        vga[(VGA_HEIGHT-1) * VGA_WIDTH + x] = (color << 8) | ' ';

    cursor_y = VGA_HEIGHT - 1;
}

void vga_setcolor(int fg, int bg)
{
    color = (unsigned char)((bg << 4) | (fg & 0x0F));
}

void vga_clear(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        vga[i] = (color << 8) | ' ';
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

void vga_putchar(char c)
{
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        update_cursor();
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | ' ';
            update_cursor();
        }
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
    } else {
        if (c >= 32) {
            vga[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | (unsigned char)c;
            cursor_x++;
        }
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        update_cursor();
    }

    if (cursor_y >= VGA_HEIGHT)
        scroll();
}

void vga_print(const char *str)
{
    while (*str)
        vga_putchar(*str++);
}

void vga_init(void)
{
    color = 0x02; /* зелений на чорному */
    for (int i = 0; i < 80 * 25; i++)
        vga[i] = 0x0220; /* зелений колір + пробіл */
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}