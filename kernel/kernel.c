#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "keyboard.h"
#include "shell.h"

void kernel_main(void)
{
    vga_init();
    gdt_init();

    vga_setcolor(VGA_LGREEN, VGA_BLACK);
    vga_print("V-Gate OS Genesis\n");
    vga_print("------------------\n");
    vga_setcolor(VGA_WHITE, VGA_BLACK);
    vga_print("GDT initialized.\n");

    idt_init();
    vga_print("IDT initialized.\n");

    keyboard_init();
    shell_run();
}