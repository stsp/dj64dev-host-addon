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
#include <pthread.h>
#include <dosemu2/emu.h>

extern void *dj64_dl_handle_self;
void *dj64_dl_handle_self;
extern int dj64_elfexec_version;
int dj64_elfexec_version = 2;

static char shname[128];
static pthread_t logo_thr;

static void *logo_thread(void *arg)
{
    while (1) {
        fprintf(stderr, ".");
        usleep(200000);
    }
    return NULL;
}

static void _ex(void)
{
    shm_unlink(shname);
}

static void *bootstrap(void)
{
    extern char _binary_hosttmp_o_elf_start[];
    extern char _binary_hosttmp_o_elf_end[];
    const char *estart = _binary_hosttmp_o_elf_start;
    const char *eend = _binary_hosttmp_o_elf_end;
    size_t sz = eend - estart;
    char buf2[256];
    int fd, err;
    void *addr, *dlh = NULL;

    snprintf(shname, sizeof(shname), "/libhost_%i.so", getpid());
    fd = shm_open(shname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR);
    if (fd == -1) {
        perror("shm_open()");
        return NULL;
    }
    err = ftruncate(fd, sz);
    if (err) {
        close(fd);
        perror("ftruncate()");
        return NULL;
    }
    addr = mmap(NULL, sz, PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (addr == MAP_FAILED) {
        perror("mmap()");
        return NULL;
    }
    memcpy(addr, estart, sz);
    snprintf(buf2, sizeof(buf2), "/dev/shm%s", shname);
#ifdef RTLD_DEEPBIND
    dlh = dlopen(buf2, RTLD_LOCAL | RTLD_NOW | RTLD_DEEPBIND);
    if (!dlh)
        printf("error loading %s: %s\n", shname, dlerror());
#endif
    munmap(addr, sz);
    /* don't unlink right here as that confuses gdb */
    atexit(_ex);
    return dlh;
}

int dj64_startup_hook(int argc, char **argv)
{
    int (*m)(int, char **) = dlsym(dj64_dl_handle_self, "main");
    if (!m) {
        printf("error: can't find \"main\"\n");
        return -1;
    }

    pthread_cancel(logo_thr);
    pthread_join(logo_thr, NULL);
    fprintf(stderr, "done\n");
    dosemu2_render_enable();
    return m(argc, argv);
}

void de2_init_hook(void *arg)
{
    pthread_create(&logo_thr, NULL, logo_thread, NULL);
}

int __wrap_main(int argc, char **argv, char * const *envp)
{
#define DEF_HANDLE 0
#define DEF_LIBID 3
    typedef void (dj64init2_t)(int handle, int disp_id);
    dj64init2_t *i2;
    char *ar0;
    void *dlh;
    char *argv0[] = { argv[0], NULL };
    char title[128];

    dlh = bootstrap();
    if (!dlh) {
        printf("error: ELF bootstrap failed\n");
        return -1;
    }
    i2 = dlsym(dlh, "dj64init2");
    if (!i2) {
        printf("error: can't find \"dj64init2\"\n");
        return -1;
    }
    i2(DEF_HANDLE, DEF_LIBID);
    dj64_dl_handle_self = dlh;

    dosemu2_set_elfload_type(2);
    dosemu2_set_elfload_args(argc, argv);
    if (!getenv("DJ64_DEBUG_MODE")) {
        dosemu2_set_exit_after_load();
        dosemu2_set_boot_cls();
        dosemu2_set_blind_boot();
        dosemu2_render_disable();
    }
    dosemu2_xtitle_disable();
    dosemu2_set_init_hook(de2_init_hook, NULL);
    if (!getenv("DJ64_GUI_MODE"))
        dosemu2_set_terminal_mode();
    ar0 = argc ? strdup(argv[0]) : NULL;
    if (ar0) {
        char *p = strrchr(ar0, '/');
        if (p) {
            *p++ = '\0';
            snprintf(title, sizeof(title), "dj64dev - %s", p);
            dosemu2_set_window_title(title);
            dosemu2_set_unix_path(ar0);
        }
        free(ar0);
    }
    fprintf(stderr, "Loading dj64dev runtime");
    return dosemu2_emulate(argc ? 1 : 0, argv0, envp);
}
