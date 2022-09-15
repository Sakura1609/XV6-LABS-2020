#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("params not enough\n");
        exit(-1);
    }
    char buf[64];
    read(0, buf, 64);

    int xargc = argc - 1;
    char *xargv[64];
    for (int i = 1; i < argc; i++) {
        xargv[i-1] = argv[i];
    }

    char *p = buf;
    for (int i = 0; i < 64; i++)
    {
        if (buf[i] == '\n')
        {
            if (fork() == 0)
            {
                buf[i] = 0;
                xargv[xargc++] = p;
                xargv[xargc] = 0;
                exec(xargv[0], xargv);
            }
            else
            {
                p = &buf[i+1];
                wait(0);
            }
        }
    }
    exit(0);
}