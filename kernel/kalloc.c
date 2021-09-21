// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
static uint64 pgNumber = PHYSTOP>>12;
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
struct {
  struct spinlock lock ;
  int  pageMapNum[PHYSTOP>>12];
} pgMap;


void 
addMap(uint64 pa){
  pa = PGROUNDDOWN(pa);
  if (pa<PHYSTOP){
    acquire(&pgMap.lock);
    pgMap.pageMapNum[pa>>12]++;
    //printf("addmap to %p,total times:%d\n",pa,pgMap.pageMapNum[pa>>12]);
    release(&pgMap.lock);
  
  }

}
void 
subMap(uint64 pa){
  pa = PGROUNDDOWN(pa);
  if (pa<PHYSTOP){
    acquire(&pgMap.lock);
    pgMap.pageMapNum[pa>>12]--;
    //printf("submap to %p,total times:%d\n",pa,pgMap.pageMapNum[pa>>12]);

    release(&pgMap.lock);
  }
}
int 
getMap(uint64 pa){
  int n =-1;
  pa = PGROUNDDOWN(pa);
  if (pa<PHYSTOP){
    acquire(&pgMap.lock);
    n = pgMap.pageMapNum[pa>>12];
    release(&pgMap.lock);
  }
  return n ;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  // add page mapping numbers lock 
  initlock(&pgMap.lock,"page mapping number");
  //pgMap.pageMapNum[PHYSTOP/PGSIZE] = 0;
   
   
  for (int i = 0; i <pgNumber; i++){
    pgMap.pageMapNum[i] = 0;
  }
  freerange(end, (void*)PHYSTOP);
  for (int i = 0; i< pgNumber ; i++){
    pgMap.pageMapNum[i] = 0;
    
  }
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
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
  subMap((uint64)pa);
  if (getMap((uint64)pa) > 0 )
    return;
  
  // Fill with junk to catch dangling refs.
    memset(pa, 0, PGSIZE);
    r = (struct run*)pa;
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    addMap((uint64)r);
    memset((char*)r, 0, PGSIZE); // fill with junk
  }
    
  return (void*)r;
}
