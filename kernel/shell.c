#include "shell.h"
#include "vga.h"
#include "keyboard.h"

#define MAX_CMD  256
#define MAX_ARGS 16

static char cmd_buf[MAX_CMD];
static int  cmd_len = 0;
static int  sudo_active = 0;

static char *args[MAX_ARGS];
static int   argc;

/* ---- утиліти ---- */

static int strcmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static void print_int(unsigned int n)
{
    char buf[12];
    int i = 0;
    if (n == 0) { vga_putchar('0'); return; }
    while (n) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i--) vga_putchar(buf[i]);
}

static unsigned int get_ticks(void)
{
    unsigned int lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return lo;
}

static void outb(unsigned short port, unsigned char val)
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static unsigned char inb(unsigned short port)
{
    unsigned char val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* ---- парсер ---- */

static void parse_args(char *buf)
{
    argc = 0;
    char *p = buf;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        args[argc++] = p;
        if (argc >= MAX_ARGS) break;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = 0;
    }
}

/* ---- команди ---- */

static void do_cpuid(void)
{
    unsigned int eax, ebx, ecx, edx;
    char vendor[13];

    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0)
    );

    *(unsigned int*)(vendor)     = ebx;
    *(unsigned int*)(vendor + 4) = edx;
    *(unsigned int*)(vendor + 8) = ecx;
    vendor[12] = 0;

    vga_print("\nCPU Vendor: ");
    vga_print(vendor);

    __asm__ volatile (
        "cpuid"
        : "=a"(eax)
        : "a"(1)
        : "ebx", "ecx", "edx"
    );

    vga_print("\nFamily: ");
    print_int((eax >> 8) & 0xF);
    vga_print("  Model: ");
    print_int((eax >> 4) & 0xF);
    vga_print("  Stepping: ");
    print_int(eax & 0xF);
    vga_print("\n");
}

static void do_meminfo(void)
{
    unsigned short mem_kb;
    __asm__ volatile (
        "movw 0x413, %0"
        : "=r"(mem_kb)
    );
    vga_print("\nBase memory: ");
    print_int(mem_kb);
    vga_print(" KB\n");
    vga_print("Kernel at:   0x100000\n");
}

static void do_dump(const char *addr_str)
{
    unsigned int addr = 0;
    const char *p = addr_str;

    if (p[0] == '0' && p[1] == 'x') p += 2;
    while (*p) {
        addr <<= 4;
        if (*p >= '0' && *p <= '9')      addr += *p - '0';
        else if (*p >= 'a' && *p <= 'f') addr += *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F') addr += *p - 'A' + 10;
        p++;
    }

    vga_print("\nDump 0x");
    vga_print(addr_str);
    vga_print(":\n");

    unsigned char *mem = (unsigned char*)addr;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 16; col++) {
            unsigned char b = mem[row*16 + col];
            vga_putchar("0123456789ABCDEF"[b >> 4]);
            vga_putchar("0123456789ABCDEF"[b & 0xF]);
            vga_putchar(' ');
        }
        vga_putchar('\n');
    }
}

static void do_reboot(void)
{
    vga_print("\nRebooting...\n");
    while (inb(0x64) & 0x02);
    outb(0x64, 0xFE);
    while (1) {}
}

static void do_halt(void)
{
    vga_print("\nSystem halted. Safe to power off.\n");
    __asm__ volatile ("cli; hlt");
    while (1) {}
}

static void do_color(const char *fg_s, const char *bg_s)
{
    static const char *colors[] = {
        "black","blue","green","cyan","red","magenta","brown","lgray",
        "dgray","lblue","lgreen","lcyan","lred","lmagenta","yellow","white"
    };
    int fg = 7, bg = 0;
    for (int i = 0; i < 16; i++) {
        if (strcmp(fg_s, colors[i]) == 0) fg = i;
        if (bg_s && strcmp(bg_s, colors[i]) == 0) bg = i;
    }
    vga_setcolor(fg, bg);
    vga_clear();
}

static void do_help(void)
{
    vga_setcolor(VGA_YELLOW, VGA_BLACK);
    vga_print("\n=== V-Gate OS Genesis Commands ===\n\n");
    vga_setcolor(VGA_LGREEN, VGA_BLACK);
    vga_print("Basic:\n");
    vga_setcolor(VGA_WHITE, VGA_BLACK);
    vga_print("  help            - show this list\n");
    vga_print("  ver / version   - OS version\n");
    vga_print("  clear / cls     - clear screen\n");
    vga_print("  echo [text]     - print text\n");
    vga_print("  color [fg] [bg] - change colors\n");
    vga_print("  time            - CPU ticks\n");
    vga_print("  cpuid           - CPU info\n");
    vga_print("  meminfo         - memory info\n");
    vga_setcolor(VGA_LRED, VGA_BLACK);
    vga_print("\nPrivileged (sudo):\n");
    vga_setcolor(VGA_WHITE, VGA_BLACK);
    vga_print("  sudo halt       - halt system\n");
    vga_print("  sudo reboot     - reboot system\n");
    vga_print("  sudo dump [addr]- hex dump\n");
    vga_putchar('\n');
}

static void exec_cmd(void)
{
    parse_args(cmd_buf);
    if (argc == 0) return;

    char *cmd = args[0];

    if (strcmp(cmd, "sudo") == 0) {
        if (argc < 2) { vga_print("\nUsage: sudo [command]\n"); return; }
        sudo_active = 1;
        for (int i = 0; i < argc - 1; i++) args[i] = args[i+1];
        argc--;
        cmd = args[0];
    }

    if (strcmp(cmd, "help") == 0) {
        do_help();
    } else if (strcmp(cmd, "ver") == 0 || strcmp(cmd, "version") == 0) {
        vga_print("\nV-Gate OS Genesis v0.1\n");
        vga_print("Arch:  x86_32\n");
        vga_print("FS:    FAT16\n");
        vga_print("Build: " __DATE__ "\n");
    } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        vga_clear();
    } else if (strcmp(cmd, "echo") == 0) {
        vga_putchar('\n');
        for (int i = 1; i < argc; i++) {
            vga_print(args[i]);
            if (i < argc - 1) vga_putchar(' ');
        }
        vga_putchar('\n');
    } else if (strcmp(cmd, "color") == 0) {
        if (argc >= 2)
            do_color(args[1], argc >= 3 ? args[2] : "black");
        else
            vga_print("\nUsage: color [fg] [bg]\n");
    } else if (strcmp(cmd, "time") == 0) {
        vga_print("\nCPU ticks: ");
        print_int(get_ticks());
        vga_putchar('\n');
    } else if (strcmp(cmd, "cpuid") == 0) {
        do_cpuid();
    } else if (strcmp(cmd, "meminfo") == 0) {
        do_meminfo();
    } else if (strcmp(cmd, "halt") == 0) {
        if (!sudo_active) vga_print("\nPermission denied. Use: sudo halt\n");
        else do_halt();
    } else if (strcmp(cmd, "reboot") == 0) {
        if (!sudo_active) vga_print("\nPermission denied. Use: sudo reboot\n");
        else do_reboot();
    } else if (strcmp(cmd, "dump") == 0) {
        if (!sudo_active) vga_print("\nPermission denied. Use: sudo dump [addr]\n");
        else if (argc < 2) vga_print("\nUsage: sudo dump 0xADDRESS\n");
        else do_dump(args[1]);
    } else {
        vga_print("\nUnknown: ");
        vga_print(cmd);
        vga_print("\nType 'help'\n");
    }

    sudo_active = 0;
}

static void print_prompt(void)
{
    vga_setcolor(VGA_LGREEN, VGA_BLACK);
    vga_print("\nA:\\> ");
    vga_setcolor(VGA_WHITE, VGA_BLACK);
}

void shell_run(void)
{
    vga_print("\nType 'help' for commands.\n");
    print_prompt();
    cmd_len = 0;

    while (1) {
        char c = keyboard_getchar();
        if (!c) continue;

        if (c == '\n') {
            cmd_buf[cmd_len] = 0;
            exec_cmd();
            cmd_len = 0;
            print_prompt();
        } else if (c == '\b') {
            if (cmd_len > 0) {
                cmd_len--;
                vga_putchar('\b');
                vga_putchar(' ');
                vga_putchar('\b');
            }
        } else if (cmd_len < MAX_CMD - 1) {
            cmd_buf[cmd_len++] = c;
            vga_putchar(c);
        }
    }
}