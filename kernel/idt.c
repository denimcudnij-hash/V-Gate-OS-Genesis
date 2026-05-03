#include "idt.h"
#include "vga.h"

#define IDT_SIZE 256

static struct idt_entry idt[IDT_SIZE];
static struct idt_ptr   idtp;

static const char *exceptions[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
};

static void idt_set(int n, uint32_t handler)
{
    idt[n].offset_low  = handler & 0xFFFF;
    idt[n].selector    = 0x08;
    idt[n].zero        = 0;
    idt[n].type_attr   = 0x8E;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

void isr_handler(struct registers *regs)
{
    vga_setcolor(VGA_LRED, VGA_BLACK);
    vga_print("\n*** EXCEPTION: ");
    if (regs->int_no < 16)
        vga_print(exceptions[regs->int_no]);
    vga_print(" ***\n");
    while (1) {}
}

void idt_init(void)
{
    idtp.limit = (sizeof(struct idt_entry) * IDT_SIZE) - 1;
    idtp.base  = (uint32_t)&idt;

    for (int i = 0; i < IDT_SIZE; i++)
        idt_set(i, (uint32_t)isr0);

    idt_set(0,  (uint32_t)isr0);
    idt_set(1,  (uint32_t)isr1);
    idt_set(2,  (uint32_t)isr2);
    idt_set(3,  (uint32_t)isr3);
    idt_set(4,  (uint32_t)isr4);
    idt_set(5,  (uint32_t)isr5);
    idt_set(6,  (uint32_t)isr6);
    idt_set(7,  (uint32_t)isr7);
    idt_set(8,  (uint32_t)isr8);
    idt_set(9,  (uint32_t)isr9);
    idt_set(10, (uint32_t)isr10);
    idt_set(11, (uint32_t)isr11);
    idt_set(12, (uint32_t)isr12);
    idt_set(13, (uint32_t)isr13);
    idt_set(14, (uint32_t)isr14);
    idt_set(15, (uint32_t)isr15);

    __asm__ volatile ("lidt %0" : : "m"(idtp));
}