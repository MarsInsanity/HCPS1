/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef HCPS1_MEMORY_H
#define HCPS1_MEMORY_H
#include <stddef.h>
#include <stdint.h>
#define HCPS1_RAM_SIZE 0x200000u
#define HCPS1_MAP_COUNT 8
struct hcps1_mapping {
    void *ram, *parallel, *bios, *scratch, *code;
    void *regions[HCPS1_MAP_COUNT];
    size_t lengths[HCPS1_MAP_COUNT];
    unsigned count;
};
/* Zero-initialize m before first use. Negative errno on failure. */
int hcps1_map_at(struct hcps1_mapping *m, uintptr_t base, size_t code_size,
                 int force_tmpfile);
int hcps1_map_create(struct hcps1_mapping *m, size_t code_size);
void hcps1_map_destroy(struct hcps1_mapping *m);
#endif
