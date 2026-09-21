// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "common/types.h"
#include "common/defs.h"
#include "common/param.h"
#include "kernel/mem/memlayout.h"
#include "kernel/mem/mmu.h"
#include "kernel/lib/spinlock.h"


//   ############# For assignment Part D ##############

// Maximum physical pages
#define MAX_PHY_PAGES ((PHYSTOP - KERNBASE) / PGSIZE)

// Reference count per physical page
static int phys_refcount[MAX_PHY_PAGES];

// Track free pages
static int kmem_free_pages = 0;

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
} kmem;


//  ############ HELPER FUNCTIONS FOR PART D #############

void incr_refcount(uint pa) {
  uint idx = (pa - KERNBASE) / PGSIZE;
  if (idx >= MAX_PHY_PAGES)
    panic("incr_refcount: out of range");

  acquire(&kmem.lock);
  phys_refcount[idx]++;
  release(&kmem.lock);
}

void decr_refcount(uint pa) {
  uint idx = (pa - KERNBASE) / PGSIZE;
  if (idx >= MAX_PHY_PAGES)
    panic("decr_refcount: out of range");

  acquire(&kmem.lock);

  if (phys_refcount[idx] <= 0)
    panic("decr_refcount: underflow");

  phys_refcount[idx]--;
  int now = phys_refcount[idx];

  release(&kmem.lock);

  // Only free if no references
  if (now == 0) {
    kfree((char*)P2V(pa));
  }
}

int get_refcount(uint pa) {
  uint idx = (pa - KERNBASE) / PGSIZE;
  if (idx >= MAX_PHY_PAGES) return -1;

  acquire(&kmem.lock);
  int r = phys_refcount[idx];
  release(&kmem.lock);

  return r;
}

int get_free_pages_count(void) {
  acquire(&kmem.lock);
  int f = kmem_free_pages;
  release(&kmem.lock);
  return f;
}

int num_free_pages() {
  struct run *r;
  int count = 0;

  acquire(&kmem.lock);
  for(r = kmem.freelist; r; r = r->next)
    count++;
  release(&kmem.lock);

  return count;
}


//  ###### End of helper functions for part D #######

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;

  // INITIALIZE REFERENCE COUNT ARRAY FOR PART D
  for (int i = 0; i < MAX_PHY_PAGES; i++) {
    phys_refcount[i] = 0;
  }

  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
    kfree(p);
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  uint pa = V2P(v);
  uint idx = (pa - KERNBASE) / PGSIZE;
  if (idx >= MAX_PHY_PAGES)
    panic("kfree idx out of range");

  acquire(&kmem.lock);

  // If reference count > 1 → do not free
  if (phys_refcount[idx] > 1) {
    phys_refcount[idx]--;
    release(&kmem.lock);
    return;
  }

  // refcount == 0 or 1 → free
  phys_refcount[idx] = 0;

  // Fill with junk to catch dangling refs.
  memset(v, 1, PGSIZE);

  //if(kmem.use_lock)
  //  acquire(&kmem.lock);
  r = (struct run*)v;
  r->next = kmem.freelist;

  kmem.freelist = r;

  kmem_free_pages++;

  release(&kmem.lock);
  //if(kmem.use_lock)
  //  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;

  // ALLOCATE A PAGE
  if (r)
    kmem_free_pages--;

  if(kmem.use_lock)
    release(&kmem.lock);

  if (!r)
    return 0;

  // Initialize memory as xv6 does
  memset((char*)r, 5, PGSIZE);

  uint pa = V2P(r);
  uint idx = (pa - KERNBASE) / PGSIZE;

  if (idx >= MAX_PHY_PAGES)
    panic("kalloc idx out of range");

  // Set reference count = 1 for new page
  phys_refcount[idx] = 1;

  return (char*)r;
}

