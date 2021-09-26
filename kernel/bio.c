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

#define BUCKETS 13 
struct {
  struct spinlock lock[BUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[BUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;

  //initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  for (int i=0; i<BUCKETS; i++){
    initlock(&bcache.lock[i],"bcaches");
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  // put all free buff to bucket 0
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint hash = blockno % BUCKETS; 

  acquire(&bcache.lock[hash]);

  // Is the block already cached?
  for(b = bcache.head[hash].next; b != &bcache.head[hash]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hash]);
      acquiresleep(&b->lock);
      return b;
    }
  }

   // Not cached.
  while(1){
    // go through other 13 -1 bucket, find a free buffer 
    for (int i= 1; i<BUCKETS; i++){
      int j = (hash +i) % BUCKETS;
      acquire(&bcache.lock[j]);
      for(b = bcache.head[j].prev; b != &bcache.head[j]; b = b->prev){
        if(b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          //remove the free buffer from current bucket
          b->prev->next = b->next;
          b->next->prev = b->prev;
          release(&bcache.lock[j]);
          //insert the free buffer to bucket hash
          b->next = bcache.head[hash].next;
          b->prev = &bcache.head[hash];
          bcache.head[hash].next->prev = b;
          bcache.head[hash].next = b;
          release(&bcache.lock[hash]);
          acquiresleep(&b->lock);
          return b;
        }
      }
      release(&bcache.lock[j]);
    }
    // if all buffers are not free in all buckets, panic 
    panic("bget: no buffers");
  }
  
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;
  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  int hash = b->blockno % BUCKETS;
  acquire(&bcache.lock[hash]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    // remove b from current position 
    // add b to the list front
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[hash].next;
    b->prev = &bcache.head[hash];
    bcache.head[hash].next->prev = b;
    bcache.head[hash].next = b;
  }
  
  release(&bcache.lock[hash]);
}

void
bpin(struct buf *b) {
  int hash = b->blockno % BUCKETS;
  acquire(&bcache.lock[hash]);
  b->refcnt++;
  release(&bcache.lock[hash]);
}

void
bunpin(struct buf *b) {
  int hash = b->blockno % BUCKETS;
  acquire(&bcache.lock[hash]);
  b->refcnt--;
  release(&bcache.lock[hash]);
}


