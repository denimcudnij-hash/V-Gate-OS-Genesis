[BITS 16]
[ORG 0x7C00]

    db 0xEB, 0x3C, 0x90

OEMName         db 'VGATEOS '
BytesPerSector  dw 512
SecPerCluster   db 4
ReservedSectors dw 16
FATCount        db 2
RootEntries     dw 512
TotalSectors    dw 0
MediaDescriptor db 0xF8
SectorsPerFAT   dw 32
SectorsPerTrack dw 63
Heads           dw 255
HiddenSectors   dd 0
TotalSectors32  dd 65536
DriveNumber     db 0x80
Reserved        db 0
BootSig         db 0x29
VolumeID        dd 0x12345678
VolumeLabel     db 'VGATE OS   '
FSType          db 'FAT16   '

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [drive], dl

    xor ax, ax
    mov es, ax
    mov bx, 0x7E00
    mov ah, 0x02
    mov al, 2
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [drive]
    int 0x13
    jc error

    mov dl, [drive]
    jmp 0x0000:0x7E00

error:
    mov si, errmsg
.loop:
    lodsb
    test al, al
    jz .halt
    mov ah, 0x0E
    int 0x10
    jmp .loop
.halt:
    cli
    hlt

drive  db 0
errmsg db 'Stage1 error!', 0

times 446-($-$$) db 0
times 64 db 0
dw 0xAA55