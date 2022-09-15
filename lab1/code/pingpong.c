#include "kernel/types.h"
#include "user.h"

int main(int argc)
{
    int p[2], pb[2];
    char buf[4];
    pipe(p);
    pipe(pb);

    if (fork() == 0) {
        close(p[1]);
        close(pb[0]);
        read(p[0], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        write(pb[1], "pong", 4);
        close(p[0]);
        close(pb[1]);
    } else {
        close(p[0]);
        close(pb[1]);
        write(p[1], "ping", 4);
        read(pb[0], buf, 4);
        printf("%d: received %s\n", getpid(), buf);
        close(p[1]);
        close(pb[0]);
    }
    exit(0);
}