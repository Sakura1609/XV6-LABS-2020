// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

// extern uint ticks;
struct
{
  struct spinlock biglock;
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hashbucket[NBUCKETS]; // 13tables
} bcache;

void binit(void)
{
  struct buf *b;

  // Create linked list of buffers and init lock
  initlock(&bcache.biglock, "bcache_biglock");
  for (int i = 0; i < NBUCKETS; i++)
  {
    initlock(&bcache.lock[i], "bcache");
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }

  for (b = bcache.buf; b < bcache.buf + NBUF; b++)
  {
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = blockno % NBUCKETS;

  acquire(&bcache.lock[index]);
  // Is the block already cached?
  for (b = bcache.hashbucket[index].next; b != &bcache.hashbucket[index]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.lock[index]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer in the bucket.
  acquire(&bcache.biglock);
  acquire(&bcache.lock[index]);
  
  // repeat
  for (b = bcache.hashbucket[index].next; b != &bcache.hashbucket[index]; b = b->next)
  {
    if (b->dev == dev && b->blockno == blockno)
    {
      b->refcnt++;
      release(&bcache.lock[index]);
      release(&bcache.biglock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  for (b = bcache.hashbucket[index].prev; b != &bcache.hashbucket[index]; b = b->prev)
  {
    // if (b->refcnt == 0 && (tmp == 0 || b->lru < min_ticks)) {
    //   min_ticks = b->lru;
    //   tmp = b;
    // }
    if (b->refcnt == 0)
    {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[index]);
      release(&bcache.biglock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // if (tmp) {
  //   tmp->dev = dev;
  //   tmp->blockno = blockno;
  //   tmp->refcnt++;
  //   tmp->valid = 0;
  //   //acquiresleep(&b2->lock);
  //   release(&bcache.lock[index]);
  //   release(&bcache.biglock);
  //   acquiresleep(&tmp->lock);
  //   return tmp;
  // }


  // if the bucket is full, use another bucket
  for (int i = (index + 1) % NBUCKETS; i != index; i = (i + 1) % NBUCKETS)
  {
    acquire(&bcache.lock[i]);
    for (b = bcache.hashbucket[i].prev; b != &bcache.hashbucket[i]; b = b->prev)
    {
      if (b->refcnt == 0)
      {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // remove
        b->prev->next = b->next;
        b->next->prev = b->prev;
        release(&bcache.lock[i]);
        // add
        b->next = bcache.hashbucket[index].next;
        b->prev = &bcache.hashbucket[index];
        bcache.hashbucket[index].next->prev = b;
        bcache.hashbucket[index].next = b;
        release(&bcache.lock[index]);
        release(&bcache.biglock);
        acquiresleep(&b->lock);
        return b;
      }
      // if (b->refcnt == 0 && (tmp == 0 || b->lru < min_ticks)) {
      //   min_ticks = b->lru;
      //   tmp = b;
      // }
    }
    // if (tmp)
    //   {
    //     tmp->dev = dev;
    //     tmp->blockno = blockno;
    //     tmp->valid = 0;
    //     tmp->refcnt++;
    //     // remove
    //     tmp->prev->next = tmp->next;
    //     tmp->next->prev = tmp->prev;
    //     release(&bcache.lock[i]);
    //     // add
    //     tmp->next = bcache.hashbucket[index].next;
    //     tmp->prev = &bcache.hashbucket[index];
    //     bcache.hashbucket[index].next->prev = tmp;
    //     bcache.hashbucket[index].next = tmp;
    //     release(&bcache.lock[index]);
    //     release(&bcache.biglock);
    //     acquiresleep(&tmp->lock);
    //     return tmp;
    //   }
    release(&bcache.lock[i]);
  }
  release(&bcache.lock[index]);
  release(&bcache.biglock);
  panic("no buffer left");
}


// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if (!b->valid)
  {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
  if (!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int index = b->blockno % NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  if (b->refcnt == 0)
  {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[index].next;
    b->prev = &bcache.hashbucket[index];
    bcache.hashbucket[index].next->prev = b;
    bcache.hashbucket[index].next = b;
    // b->lru = ticks;
  }
  release(&bcache.lock[index]);
}

void bpin(struct buf *b)
{
  int index = b->blockno % NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt++;
  release(&bcache.lock[index]);
}

void bunpin(struct buf *b)
{
  int index = b->blockno % NBUCKETS;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  release(&bcache.lock[index]);
}


