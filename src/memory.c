/* SPDX-License-Identifier: GPL-2.0-or-later
 * HCPS1 old-kernel direct memory map for Lightrec.
 * Every address is a NON-FIXED mmap hint, checked after the atomic syscall.
 * A collision never replaces an existing mapping. No mincore/MAP_FIXED race,
 * no dependency on MAP_FIXED_NOREPLACE or memfd_create in old kernel headers.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "memory.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/statfs.h>
#include <sys/syscall.h>
#include <unistd.h>

static int backing_fd(int force_tmpfile)
{
    int fd = -1;
#if defined(SYS_memfd_create)
    if (!force_tmpfile)
        fd = (int)syscall(SYS_memfd_create, "hcps1-ram", 0);
#else
    (void)force_tmpfile;
#endif
    if (fd < 0) {
        const char *dirs[] = { "/dev/shm", "/tmp" };
        unsigned i;
        for (i = 0; i < sizeof(dirs) / sizeof(dirs[0]); ++i) {
            struct statfs st;
            char name[80];
            snprintf(name, sizeof(name), "%s/hcps1-XXXXXX", dirs[i]);
            fd = mkstemp(name);
            if (fd < 0) continue;
            if (unlink(name) < 0) {
                int e = errno; close(fd); errno = e; return -1;
            }
            /* Keep emulated RAM off the SD card: Linux tmpfs or ramfs. */
            if (fstatfs(fd, &st) == 0 &&
                ((unsigned long)st.f_type == 0x01021994UL ||
                 (unsigned long)st.f_type == 0x858458f6UL))
                break;
            close(fd);
            fd = -1;
            errno = ENOTSUP;
        }
    }
    if (fd >= 0 && ftruncate(fd, HCPS1_RAM_SIZE) < 0) {
        int e = errno; close(fd); errno = e; return -1;
    }
    if (fd >= 0) fcntl(fd, F_SETFD, FD_CLOEXEC);
    return fd;
}

void hcps1_map_destroy(struct hcps1_mapping *m)
{
    while (m->count) {
        unsigned n = --m->count;
        munmap(m->regions[n], m->lengths[n]);
    }
    memset(m, 0, sizeof(*m));
}

static int map_exact(struct hcps1_mapping *m, uintptr_t address, size_t size,
                     int prot, int flags, int fd, void **out)
{
    void *p = mmap((void *)address, size, prot, flags, fd, 0);
    if (p == MAP_FAILED) return -errno;
    if ((uintptr_t)p != address) {
        munmap(p, size);
        return -EADDRINUSE;
    }
    m->regions[m->count] = p;
    m->lengths[m->count++] = size;
    *out = p;
    return 0;
}

int hcps1_map_at(struct hcps1_mapping *m, uintptr_t base, size_t code_size,
                 int force_tmpfile)
{
    int fd, rc = 0;
    unsigned i;
    void *mirror;
    long page = sysconf(_SC_PAGESIZE);
    if (m->count) return -EBUSY;
    if (page <= 0 || !base || base % (uintptr_t)page || !code_size ||
        code_size % (size_t)page || code_size > 0x1e800000u ||
        base > UINTPTR_MAX - 0x1fe00000u)
        return -EINVAL;
    fd = backing_fd(force_tmpfile);
    if (fd < 0) return -errno;
    for (i = 0; i < 4; ++i) {
        rc = map_exact(m, base + i * HCPS1_RAM_SIZE, HCPS1_RAM_SIZE,
                       PROT_READ | PROT_WRITE, MAP_SHARED, fd, &mirror);
        if (rc) goto fail;
        if (!i) m->ram = mirror;
    }
#define REGION(field, offset, size, protection) do { \
    rc = map_exact(m, base + (offset), (size), (protection), \
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, &m->field); \
    if (rc) goto fail; \
} while (0)
    REGION(parallel, 0x1f000000u, 0x10000u, PROT_READ | PROT_WRITE);
    REGION(bios, 0x1fc00000u, 0x200000u, PROT_READ | PROT_WRITE);
    REGION(scratch, 0x1f800000u, 0x10000u, PROT_READ | PROT_WRITE);
    REGION(code, 0x800000u, code_size, PROT_READ | PROT_WRITE | PROT_EXEC);
#undef REGION
    close(fd);
    return 0;
fail:
    close(fd);
    hcps1_map_destroy(m);
    return rc;
}

int hcps1_map_create(struct hcps1_mapping *m, size_t code_size)
{
    static const uintptr_t bases[] = { 0x40000000u, 0x10000000u, 0x20000000u };
    unsigned i;
    int rc = -ENOMEM;
    const char *force = getenv("HCPS1_FORCE_TMPFILE");
    for (i = 0; i < sizeof(bases) / sizeof(bases[0]); ++i) {
        rc = hcps1_map_at(m, bases[i], code_size, force && !strcmp(force, "1"));
        if (!rc || rc == -EBUSY) break;
    }
    return rc;
}
