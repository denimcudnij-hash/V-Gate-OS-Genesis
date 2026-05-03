import struct
import os

DISK_IMG = "build\\vgate.img"
KERNEL   = "build\\kernel.flat"
STAGE1   = "build\\stage1.bin"
STAGE2   = "build\\stage2.bin"

SECTOR              = 512
RESERVED_SECTORS    = 16
FAT_COUNT           = 2
FAT_SIZE            = 32
ROOT_ENTRIES        = 512

disk = bytearray(32 * 1024 * 1024)

with open(STAGE1, 'rb') as f:
    s1 = f.read()
disk[0:512] = s1
print(f"Stage1: {len(s1)} bytes")

with open(STAGE2, 'rb') as f:
    s2 = f.read()
disk[SECTOR:SECTOR + len(s2)] = s2
print(f"Stage2: {len(s2)} bytes")

fat_offset = RESERVED_SECTORS * SECTOR
fat = bytearray(FAT_SIZE * SECTOR)
fat[0] = 0xF8
fat[1] = 0xFF
fat[2] = 0xFF
fat[3] = 0xFF
fat[4] = 0xFF
fat[5] = 0xFF
disk[fat_offset:fat_offset + len(fat)] = fat
disk[fat_offset + FAT_SIZE*SECTOR:fat_offset + 2*FAT_SIZE*SECTOR] = fat

root_offset = (RESERVED_SECTORS + FAT_COUNT * FAT_SIZE) * SECTOR

with open(KERNEL, 'rb') as f:
    kernel = f.read()
print(f"Kernel: {len(kernel)} bytes")

entry = bytearray(32)
entry[0:11]  = b'VGATE   BIN'
entry[11]    = 0x20
struct.pack_into('<H', entry, 26, 2)
struct.pack_into('<I', entry, 28, len(kernel))
disk[root_offset:root_offset + 32] = entry

data_offset = root_offset + ROOT_ENTRIES * 32
disk[data_offset:data_offset + len(kernel)] = kernel

with open(DISK_IMG, 'wb') as f:
    f.write(disk)

print(f"Образ готовий: {DISK_IMG}")