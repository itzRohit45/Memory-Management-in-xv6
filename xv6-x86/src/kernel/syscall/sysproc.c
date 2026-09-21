#include "common/types.h"
#include "common/x86.h"
#include "common/defs.h"
#include "common/date.h"
#include "common/param.h"
#include "kernel/mem/memlayout.h"
#include "kernel/mem/mmu.h"
#include "kernel/proc/proc.h"

int sys_fork(void)
{
  return fork();
}

int sys_exit(void)
{
  exit();
  return 0; // not reached
}

int sys_wait(void)
{
  return wait();
}

int sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int sys_getpid(void)
{
  return myproc()->pid;
}

int sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_shreekar(void)
{
  cprintf("syscall by shreekar\n");
  return 0;
}

int sys_tavishi(void)
{
  cprintf("Hello from Tavishi :)\nHave a good dayyyy!!\n");
  return 0;
}

int sys_ekta(void)
{
  cprintf("Hello, syscall by Ekta\n");
  return 0;
}

// ######################## FOR ASSIGNMENT PART A ##############################

// Returns the number of virtual pages in the user part of the address space, up to p->sz, plus the stack guard page.
int sys_numvp(void)
{
  struct proc *p = myproc();
  int numpages = 0;

  // Count pages from 0 up to the program size (p->sz)
  numpages = PGROUNDUP(p->sz) / PGSIZE;

  // Add one for the stack guard page, as required
  numpages += 1;

  return numpages;
}

// Returns the number of physical pages in the user part of the address space by walking the page table.
int sys_numpp(void)
{
  struct proc *p = myproc();
  pde_t *pgdir = p->pgdir;
  pte_t *pgtab;
  int count = 0;
  int i, j;

  // Loop through user-space Page Directory Entries and KERNBASE is the end of user space.
  for (i = 0; i < PDX(KERNBASE); i++)
  {
    if (pgdir[i] & PTE_P)
    {
      // PDE is present, get pointer to Page Table page
      pgtab = (pte_t *)P2V(PTE_ADDR(pgdir[i]));

      // Loop through all Page Table Entries in this page table
      for (j = 0; j < NPTENTRIES; j++)
      {
        if (pgtab[j] & PTE_P)
        {
          // PTE is present, meaning a physical page is mapped
          count++;
        }
      }
    }
  }
  return count;
}

// Returns the size of the page table (in pages), including the page directory and all inner page table pages.
int sys_getptsize(void)
{
  struct proc *p = myproc();
  pde_t *pgdir = p->pgdir;

  // Start count at 1 for the page directory itself
  int count = 1;
  int i;

  // Loop through *all* Page Directory Entries (user and kernel)
  for (i = 0; i < NPDENTRIES; i++)
  {
    if (pgdir[i] & PTE_P)
    {
      // This PDE points to a valid (present) page table page
      count++;
    }
  }
  return count;
}

// ###################### END FOR ASSIGNMENT PART A ############################

// ###################### FOR ASSIGNMENT PART B ############################

int
sys_mmap(void)
{
  int nbytes;
  struct proc *p = myproc();

  if(argint(0, &nbytes) < 0)
    return 0; // Return 0 on invalid input

  if(nbytes <= 0 || (nbytes % PGSIZE) != 0)
    return 0; // Must be positive and a multiple of PGSIZE

  // Store the starting virtual address of the new region
  uint old_sz = p->sz;

  // Grow the virtual address space (but not physical memory)
  if (p->sz + nbytes > KERNBASE)
    return 0; // Don't grow into the kernel

  p->sz += nbytes;

  // Return the starting virtual address of the new region
  return old_sz;
}
// ###################### END FOR ASSIGNMENT PART B ############################


// ######################## FOR ASSIGNMENT PART C ##############################

// Maps one shared page at the end of the process's address space.
int sys_mapshared(void)
{
  struct proc *p = myproc();
  char *mem;
  uint va = p->sz; // New page will be at the current end

  // Allocate one page of physical memory
  if ((mem = kalloc()) == 0)
    return -1;

  // Zero out the page
  memset(mem, 0, PGSIZE);

  // Map the page in the page table with the Shared flag
  // We must add PTE_P, PTE_W, PTE_U, and our new PTE_S flag
  if (mappages(p->pgdir, (void *)va, PGSIZE, V2P(mem), PTE_W | PTE_U | PTE_P | PTE_S) < 0)
  {
    kfree(mem); // Free the page if mapping fails
    return -1;
  }

  // Grow the process size to include the new page
  p->sz += PGSIZE;

  // Return the starting virtual address of the mapped page
  return va;
}

// Returns the virtual address of the shared page.
int sys_getshared(void)
{
  struct proc *p = myproc();
  pde_t *pgdir = p->pgdir;
  pte_t *pte;
  uint va;

  // Iterate through the user virtual address space
  for (va = 0; va < p->sz; va += PGSIZE)
  {
    // Find the page table entry
    pte = walkpgdir(pgdir, (void *)va, 0);

    if (pte == 0) // Page directory entry not present
      continue;

    // Check if the page is present and has the shared flag
    if ((*pte & PTE_P) && (*pte & PTE_S))
    {
      return va; // Found it
    }
  }

  return -1; // Not found
}

// Unmaps the shared page.
int sys_unmapshared(void)
{
  struct proc *p = myproc();
  pde_t *pgdir = p->pgdir;
  pte_t *pte;
  uint va;
  char *pa;

  // Find the shared page's virtual address
  int shared_va = sys_getshared();
  if (shared_va == -1)
    return -1; // No shared page to unmap

  va = (uint)shared_va;

  // Get the PTE
  pte = walkpgdir(pgdir, (void *)va, 0);
  if (pte == 0 || !(*pte & PTE_P) || !(*pte & PTE_S))
    return -1; // Page not found or not shared

  // Get the physical address
  pa = (char *)P2V(PTE_ADDR(*pte));

  // Free the physical memory
  kfree(pa);

  // Invalidate the PTE
  *pte = 0;

  // If the shared page is at the end of memory, shrink proc size
  if (va == PGROUNDDOWN(p->sz - 1))
  {
    p->sz -= PGSIZE;
  }

  // Flush the TLB
  switchuvm(p);

  return 0; // Success
}



// ###################### END FOR ASSIGNMENT PART C ############################

// ######################## FOR ASSIGNMENT PART D ##############################

//returns the number of free pages tracked in kalloc.c

extern int get_free_pages_count(void);

int 
sys_getNumFreePages(void)
{
  return get_free_pages_count();
}

// ###################### END FOR ASSIGNMENT PART D ############################