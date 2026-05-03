#include "keyboard.h"

#define KEYBOARD_DATA 0x60
#define KEYBOARD_STATUS 0x64

static char scancode_to_ascii[] = {
    0, 0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};

static unsigned char inb(unsigned short port)
{
    unsigned char val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void keyboard_init(void)
{
    // очікуємо поки буфер порожній
    while (inb(KEYBOARD_STATUS) & 0x02);
}

char keyboard_getchar(void)
{
    unsigned char scancode;

    // чекаємо поки є дані
    while (!(inb(KEYBOARD_STATUS) & 0x01));

    scancode = inb(KEYBOARD_DATA);

    // ігноруємо key release (bit 7)
    if (scancode & 0x80)
        return 0;

    if (scancode < sizeof(scancode_to_ascii))
        return scancode_to_ascii[scancode];

    return 0;
}