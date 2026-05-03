[BITS 16]
[ORG 0x7E00]

ROOT_START equ 80
DATA_START equ 112

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [drive], dl

    mov si, msg_loading
    call print

    ; Читаємо root directory в 0x9000
    mov ax, ROOT_START
    mov bx, 0x9000
    mov cx, 32
    call read_sectors

    mov si, 0x9000
    mov cx, 3
.dbg:
    lodsb
    mov ah, 0x0E
    int 0x10
    loop .dbg

    ; Шукаємо VGATE   BIN
    mov cx, 512         ; max entries
    mov si, 0x9000
.search:
    ; Перевіряємо перший байт — 0 означає кінець
    mov al, [si]
    test al, al
    jz .notfound

    ; Порівнюємо ім'я (11 байт)
    push si
    push cx
    mov di, filename
    mov cx, 11
    repe cmpsb
    pop cx
    pop si
    jz .found

    add si, 32
    loop .search

.notfound:
    mov si, msg_notfound
    call print
    jmp halt

.found:
    mov ax, [si + 26]
    mov [cluster], ax

    mov si, msg_found
    call print

    ; Обчислюємо сектор
    mov ax, [cluster]
    sub ax, 2
    shl ax, 2           ; * 4 (sectors per cluster)
    add ax, DATA_START

    ; Читаємо ядро в 0x1000:0000
    push es
    mov bx, 0x1000
    mov es, bx
    xor bx, bx
    mov cx, 64
    call read_sectors_es
    pop es

    mov si, msg_jump
    call print

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pmode

; ---- read_sectors: ax=LBA, bx=offset(es=0), cx=count ----
read_sectors:
    push ax
    push bx
    push cx
.loop:
    push ax
    push cx
    call lba_to_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [drive]
    int 0x13
    jc disk_error
    pop cx
    pop ax
    add bx, 512
    inc ax
    loop .loop
    pop cx
    pop bx
    pop ax
    ret

; ---- read_sectors_es: ax=LBA, es:bx=buf, cx=count ----
read_sectors_es:
    push ax
    push cx
.loop:
    push ax
    push cx
    call lba_to_chs
    mov ah, 0x02
    mov al, 1
    mov dl, [drive]
    int 0x13
    jc disk_error
    pop cx
    pop ax
    add bx, 512
    inc ax
    loop .loop
    pop cx
    pop ax
    ret

lba_to_chs:
    push ax
    push bx
    xor dx, dx
    mov bx, 63
    div bx
    inc dl
    mov cl, dl
    xor dx, dx
    mov bx, 255
    div bx
    mov dh, dl
    mov ch, al
    pop bx
    pop ax
    ret

; ---- print ----
print:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print
.done:
    ret

disk_error:
    mov si, msg_diskerr
    call print
halt:
    cli
    hlt

; ---- Дані ----
drive    db 0
cluster  dw 0
filename db 'VGATE   BIN'

msg_loading  db 'V-Gate Bootloader v1', 0x0D, 0x0A, 0
msg_found    db 'Kernel found!', 0x0D, 0x0A, 0
msg_notfound db 'VGATE.BIN not found!', 0x0D, 0x0A, 0
msg_diskerr  db 'Disk error!', 0x0D, 0x0A, 0
msg_jump     db 'Jumping to kernel...', 0x0D, 0x0A, 0

; ---- GDT ----
align 8
gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ---- Protected mode ----
[BITS 32]
pmode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, 0x2000
    rep movsd
    jmp 0x100000