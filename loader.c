/*
 *  dj64 - 64bit djgpp-compatible tool-chain
 *  Copyright (C) 2021-2025  @stsp
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dosemu2/emu.h>
#include "djdev64/dj64init.h"

static char shname[128];

static void _ex(void)
{
    shm_unlink(shname);
}

static void *bootstrap(void)
{
    char *estart = dlsym(RTLD_DEFAULT, "_binary_hosttmp_o_elf_start");
    char *eend = dlsym(RTLD_DEFAULT, "_binary_hosttmp_o_elf_end");
    size_t sz = eend - estart;
    char buf2[256];
    int fd;
    void *addr, *dlh = NULL;

    if (!estart || !eend)
        return NULL;
    snprintf(shname, sizeof(shname), "/libhost_%i.so", getpid());
    fd = shm_open(shname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR);
    if (fd == -1) {
        perror("shm_open()");
        return NULL;
    }
    ftruncate(fd, sz);
    addr = mmap(NULL, sz, PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (addr == MAP_FAILED) {
        perror("mmap()");
        return NULL;
    }
    memcpy(addr, estart, sz);
    snprintf(buf2, sizeof(buf2), "/dev/shm%s", shname);
#ifdef RTLD_DEEPBIND
    dlh = dlopen(buf2, RTLD_GLOBAL | RTLD_NOW | RTLD_DEEPBIND);
#endif
    munmap(addr, sz);
    /* don't unlink right here as that confuses gdb */
    atexit(_ex);
    return dlh;
}

int __wrap_main(int argc, char **argv, char * const *envp)
{
#define DEF_HANDLE 0
#define DEF_LIBID 3
    dj64init2_t *i2;
    char *ar0;
    void *dlh;
    char *argv0[] = { argv[0], NULL };

    dlh = bootstrap();
    if (!dlh) {
        printf("error: can't open libtmp.so\n");
        return -1;
    }
    i2 = dlsym(dlh, "dj64init2");
    if (!i2) {
        printf("error: can't find \"dj64init2\"\n");
        return -1;
    }
    i2(DEF_HANDLE, DEF_LIBID);

    dosemu2_set_elfload_type(2);
    dosemu2_set_elfload_args(argc, argv);
    dosemu2_set_exit_after_load();
    ar0 = argc ? strdup(argv[0]) : NULL;
    if (ar0) {
        char *p = strrchr(ar0, '/');
        if (p) {
            *p = '\0';
            dosemu2_set_unix_path(ar0);
        }
        free(ar0);
    }
    return dosemu2_emulate(argc ? 1 : 0, argv0, envp);
}
