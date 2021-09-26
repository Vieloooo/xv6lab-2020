// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define PERALLOC 1000
#define MAXSTORE 500
void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  int size;
} kmem;

struct {
  struct spinlock lock;
  struct run *freelist;
  int size;
} kmemPerCPU[NCPU];

//withdraw mem from cpu to pool
//kmem lock must be held 
int withdraw(int amount){
  int gain = 0;
  int money = amount;
  struct run *r ;
  struct run *head ;
  //printf("withdraw memory from cpu\n");
  for(int i =0; i<NCPU; i++){
    acquire(&kmemPerCPU[i].lock);
    //printf("cpu %d , free size %d\n",i,kmemPerCPU[i].size);
    if (kmemPerCPU[i].size>0){
      r= kmemPerCPU[i].freelist;
      head = r;
      if( ( gain + kmemPerCPU[i].size )> amount){
        money = amount -gain;
        gain = amount;
        for(int j = 1; j<money; j++){
          r= r->next;
        }
        kmemPerCPU[i].freelist = r->next;
        r->next = kmem.freelist;
        kmem.freelist = head;
        kmem.size += money;
        kmemPerCPU[i].size -= money;
        release(&kmemPerCPU[i].lock);
        return gain;
      }else{
        money = kmemPerCPU[i].size ;
        gain += kmemPerCPU[i].size;
        r= kmemPerCPU[i].freelist;
        head = kmemPerCPU[i].freelist;
        for(int j = 1; j<money; j++){
          r= r->next;
        }
        kmemPerCPU[i].freelist = 0;
        r->next = kmem.freelist;
        kmem.freelist = head;
        kmem.size += money;
        kmemPerCPU[i].size -= money;
      }
    }
    release(&kmemPerCPU[i].lock);
  }
  //printf("withdraw %d to pool\n",gain);
  return gain;
}

// get PERALLOC free pages from pool to cpu 
//return the page number which was allocated for the cpu i 
int allocFreelist(int i){
  int get;
  acquire(&kmem.lock);
  //printf(" free pages reamin: %d,cpu %d\n",kmem.size,i);
  struct run * r = kmem.freelist;
  if (kmem.size==0){
    //withdraw memory from cpu 
    // kmem lock is held 
    int m = withdraw(100*PERALLOC);
    //printf("withdraw %d from cpu mem\n",m);
    if (m==0){
      // no memory
      release(&kmem.lock);
      return 0;
    }
    release(&kmem.lock);
    get = allocFreelist(i);
    return get;
  }else if(kmem.size >=PERALLOC){
    acquire(&kmemPerCPU[i].lock);
    kmemPerCPU[i].size += PERALLOC;
    kmemPerCPU[i].freelist= r;
    release(&kmemPerCPU[i].lock);
    kmem.size -=PERALLOC;
    //
    for (int j= 1; j<PERALLOC;j++){
      r= r->next;
    }
    kmem.freelist = r->next;
    r->next = 0;
    get = PERALLOC;
  }else{
    acquire(&kmemPerCPU[i].lock);
    kmemPerCPU[i].size += kmem.size;
    kmemPerCPU[i].freelist= r;
    release(&kmemPerCPU[i].lock);
    get = kmem.size;
    kmem.size =0;
    kmem.freelist =0;
    
  }
  release(&kmem.lock);
  return get;
}
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  for (int i =0 ;i<NCPU; i++){
    initlock(&kmemPerCPU[i].lock,"kmems");
    kmemPerCPU[i].size =0;
    kmemPerCPU[i].freelist =0;
  }
  kmem.size =0;
  kmem.freelist =0;
  freerange(end, (void*)PHYSTOP);
  printf("total free %d\n",kmem.size);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    struct run *r;

  if(((uint64)p % PGSIZE) != 0 || (char*)p < end || (uint64)p >= PHYSTOP)
    panic("kfree in freerange");

  // Fill with junk to catch dangling refs.
  memset(p, 0, PGSIZE);

  r = (struct run*)p;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  kmem.size++;
  release(&kmem.lock);

  }
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
  memset(pa, 0, PGSIZE);

  r = (struct run*)pa;
  push_off();
  int id = cpuid();
  pop_off();

  //printf("add a free page to %d\n",id);
  acquire(&kmemPerCPU[id].lock);
  r->next = kmemPerCPU[id].freelist;
  kmemPerCPU[id].freelist = r;
  kmemPerCPU[id].size++;
  release(&kmemPerCPU[id].lock);
} 
void *
kalloc(void)
{
  struct run *r = 0;
  push_off();
  int id = cpuid();
  pop_off();
  acquire(&kmemPerCPU[id].lock);
  
  if(kmemPerCPU[id].size <=0){
    //printf("cpu %d ran out of memory\n",id);
    release(&kmemPerCPU[id].lock);
    // no lock was held 
    int get = allocFreelist(id);
    //printf("cpu %d get %d mem\n",id,get);
    if (get ==0){
      return 0 ;
    }
    r = kalloc();
  }else{
    r= kmemPerCPU[id].freelist;
    kmemPerCPU[id].freelist= r->next;
    kmemPerCPU[id].size--;
    release(&kmemPerCPU[id].lock);
  }
  
  if(r)
    memset((char*)r, 0, PGSIZE); // fill with junk
  return (void*)r;
}

