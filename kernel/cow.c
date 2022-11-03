#include "types.h"
#include "riscv.h"
#include "param.h"
#include "memlayout.h"
#include "defs.h"
#include "spinlock.h"
struct
{
    struct spinlock lock;
    uint overpages;
} imple[(PHYSTOP - KERNBASE) >> 12];

void increase(uint64 pa)
{
    if (pa < KERNBASE)
        return;
    int i = (pa - KERNBASE) >> 12;
    acquire(&imple[i].lock);
    imple[i].overpages++;
    release(&imple[i].lock);
}

uint decrease(uint64 pa)
{
    uint ret;
    if (pa < KERNBASE)
    {
        return 0;
    }
    pa = (pa - KERNBASE) >> 12;
    acquire(&imple[pa].lock);
    ret = --imple[pa].overpages;
    release(&imple[pa].lock);
    return ret;
}