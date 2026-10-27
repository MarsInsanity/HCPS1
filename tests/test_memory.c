/* SPDX-License-Identifier: GPL-2.0-or-later */
#define _GNU_SOURCE
#include "../src/memory.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
int main(void)
{
    struct hcps1_mapping m = {0};
    const uintptr_t base = 0x40000000u;
    unsigned round, i;
    long page = sysconf(_SC_PAGESIZE);
    /* A collision halfway into the second mirror must survive partial cleanup. */
    unsigned char *wanted = (void *)(base + HCPS1_RAM_SIZE + page);
    unsigned char *guard = mmap(wanted, page, PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(guard == wanted);
    memset(guard, 0x5a, page);
    assert(hcps1_map_at(&m, base, 8u << 20, 1) == -EADDRINUSE);
    assert(!m.count && !m.ram);
    for (i = 0; i < (unsigned)page; ++i) assert(guard[i] == 0x5a);
    assert(munmap(guard, page) == 0);
    /* Same collision at late mapping: ensure all prior mappings are rolled back. */
    wanted = (void *)(base + 0x1fc00000u);
    guard = mmap(wanted, page, PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(guard == wanted); guard[0] = 0xa7;
    assert(hcps1_map_at(&m, base, 8u << 20, 1) == -EADDRINUSE);
    assert(guard[0] == 0xa7 && !m.count);
    assert(munmap(guard, page) == 0);
    for (round = 0; round < 8; ++round) {
        volatile unsigned char *ram;
        assert(hcps1_map_at(&m, base, 8u << 20, round & 1) == 0);
        assert(m.count == HCPS1_MAP_COUNT);
        assert(hcps1_map_at(&m, base, 8u << 20, 1) == -EBUSY);
        ram = m.ram;
        for (i = 0; i < 4; ++i) {
            ram[i * HCPS1_RAM_SIZE + 123] = (unsigned char)(19 + i);
            assert(ram[123] == 19 + i);
            assert(ram[3 * HCPS1_RAM_SIZE + 123] == 19 + i);
        }
        hcps1_map_destroy(&m);
        hcps1_map_destroy(&m);
    }
    assert(hcps1_map_at(&m, 0, 8u << 20, 1) == -EINVAL);
    assert(hcps1_map_at(&m, base, 1, 1) == -EINVAL);
    puts("PASS: map collisions, partial rollback, mirrors, tmpfs fallback, lifecycle");
    return 0;
}
