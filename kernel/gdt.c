#include "gdt.h"
#include <stdint.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_ptr   gdtp;

static void gdt_set(int n, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt[n].base_low   = base & 0xFFFF;
    gdt[n].base_mid   = (base >> 16) & 0xFF;
    gdt[n].base_high  = (base >> 24) & 0xFF;
    gdt[n].limit_low  = limit & 0xFFFF;
    gdt[n].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[n].access     = access;
}

extern void gdt_flush(uint32_t);

void gdt_init(void)
{
    gdtp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gdtp.base  = (uint32_t)&gdt;

    gdt_set(0, 0, 0,          0x00, 0x00);
    gdt_set(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    __asm__ volatile ("lgdt %0" : : "m"(gdtp));
    __asm__ volatile (
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "pushl $0x08\n"
        "pushl $1f\n"
        "retf\n"
        "1:\n"
        : : : "eax"
    );
}