#!/usr/bin/env python3
"""Generate our own tiny PS-X EXE; no commercial game or BIOS needed."""
from pathlib import Path
import struct
import sys

def generate(destination):
    words = []
    def emit(w): words.append(w)
    def li(r, v):
        emit(0x3c000000 | (r << 16) | ((v >> 16) & 65535))
        emit(0x34000000 | (r << 21) | (r << 16) | (v & 65535))
    def sw(rt, rs, offset): emit(0xac000000 | (rs << 21) | (rt << 16) | offset)
    li(8, 0x1f801810)
    for command in (0x00000000, 0x03000000, 0x08000001,
                    0x06000000 | (0xc60 << 12) | 0x260,
                    0x07000000 | (0x100 << 10) | 0x10, 0x05000000):
        li(9, command); sw(9, 8, 4)
    for command in (0x020000ff, 0, (240 << 16) | 320):
        li(9, command); sw(9, 8, 0)
    li(11, 0x80018000)
    li(10, 0)
    # Verify a taken branch delay slot executes.
    li(12, 0)
    emit(0x10000002)  # beq zero,zero,+2: skip following bad instruction
    emit(0x258c0001)  # addiu t4,t4,1 (delay slot)
    emit(0x258c0010)  # must not execute
    sw(12, 11, 4)
    loop = 0x80010000 + len(words)*4
    emit(0x254a0001)  # addiu t2,t2,1
    sw(10, 11, 0)
    emit(0x08000000 | ((loop >> 2) & 0x03ffffff))
    emit(0)
    body = struct.pack("<" + "I"*len(words), *words)
    body += bytes((-len(body)) % 2048)
    header = bytearray(2048)
    header[:8] = b"PS-X EXE"
    struct.pack_into("<IIII", header, 0x10, 0x80010000, 0, 0x80010000, len(body))
    struct.pack_into("<II", header, 0x30, 0x801fff00, 0)
    Path(destination).parent.mkdir(parents=True, exist_ok=True)
    Path(destination).write_bytes(header + body)

if __name__ == "__main__":
    generate(sys.argv[1])
