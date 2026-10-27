/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdio.h>
#include "../libpcsxcore/r3000a.h"
#include "../libpcsxcore/lightrec/mem.h"
#include "memory.h"
static struct hcps1_mapping mapping;
int lightrec_init_mmap(void)
{
    int rc = hcps1_map_create(&mapping, CODE_BUFFER_SIZE);
    if (rc) {
        fprintf(stderr, "HCPS1: direct memory map failed (%d)\n", rc);
        return rc;
    }
    psxRegs.ptrs.psxM = mapping.ram;
    psxRegs.ptrs.psxP = mapping.parallel;
    psxRegs.ptrs.psxR = mapping.bios;
    psxRegs.ptrs.psxH = mapping.scratch;
    code_buffer = mapping.code;
    fprintf(stderr, "HCPS1: Lightrec map at %p, code cache %u MiB\n",
            mapping.ram, (unsigned)(CODE_BUFFER_SIZE >> 20));
    return 0;
}
void lightrec_free_mmap(void)
{
    hcps1_map_destroy(&mapping);
    code_buffer = NULL;
    psxRegs.ptrs.psxM = psxRegs.ptrs.psxP = NULL;
    psxRegs.ptrs.psxR = psxRegs.ptrs.psxH = NULL;
}
