#define _GNU_SOURCE
#include <stdio.h>
#include <dlfcn.h>
#include <dosemu2/emu.h>
#include "djdev64/dj64init.h"

int __wrap_main(int argc, char **argv, char * const *envp)
{
#define DEF_HANDLE 0
#define DEF_LIBID 3
    dj64init2_t *i2;
    void *dlh = NULL;
    char *argv0[] = { argv[0], NULL };

#ifdef RTLD_DEEPBIND
    dlh = dlopen("./libhost.so", RTLD_GLOBAL | RTLD_NOW | RTLD_DEEPBIND);
#endif
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
    return dosemu2_emulate(argc ? 1 : 0, argv0, envp);
}
