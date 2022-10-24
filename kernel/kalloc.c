// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU];

void
kinit()
{
  // uint64 length = ((void*)PHYSTOP - (void*)&end) / NCPU / PGSIZE;
  for (int i = 0; i < NCPU; i++) {
    initlock(&kmems[i].lock, "kmem");
    // char *st, *ed;
    // st = (char*)((uint64)&end + length * i * PGSIZE);
    // ed = (char*)((uint64)&end + length * (i + 1) * PGSIZE);
    // if (i == NCPU-1) {
    //   freerange(st, (void*)PHYSTOP);
    //   printf("ed: %p, stop:%p\n", ed, PHYSTOP);
    // }
    // else 
    //   freerange(st, ed);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
  // printf("st:%p, p:%p, end:%p\n", pa_start, p, pa_end);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();
  int id = cpuid();
  acquire(&kmems[id].lock);
  r->next = kmems[id].freelist;
  kmems[id].freelist = r; 
  release(&kmems[id].lock);
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int flag = 0;
  push_off();
  int id = cpuid();
  
  acquire(&kmems[id].lock);
  r = kmems[id].freelist;
  if(r)
    kmems[id].freelist = r->next;
  else {
    for (int i = 0; i < NCPU; i++) {
      if (i == id) {
        continue;
      }
      acquire(&kmems[i].lock);
      if (kmems[i].freelist) {
        r = kmems[i].freelist;
        kmems[i].freelist = kmems[i].freelist->next;
        kmems[id].freelist = 0x0;
        flag = 1;
        release(&kmems[i].lock);
        break;
      }
      release(&kmems[i].lock); 
    }
  }
  release(&kmems[id].lock);
  pop_off();

  if(r || flag)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
