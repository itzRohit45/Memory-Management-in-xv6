#include "common/types.h"
#include "common/defs.h"
#include "common/param.h"
#include "kernel/mem/memlayout.h"
#include "kernel/mem/mmu.h"
#include "kernel/proc/proc.h"
#include "common/x86.h"
#include "kernel/trap/traps.h"
#include "kernel/lib/spinlock.h"

// ########### For part D (CoW) ############

extern int  get_refcount(uint);
extern void decr_refcount(uint);
extern void incr_refcount(uint);
// --------------------------------------------


// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{

  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;

  //..............adding a new switch case to handle page faults - for objectives B and D...........//

  //adding page fault trap handler (handles cases for both mmap and CoW)
  case T_PGFLT: {
    uint va = rcr2(); // Get the faulting virtual address
    struct proc *p = myproc();
    uint a = PGROUNDDOWN(va);
    pte_t *pte;

    // Reason 1: Real segmentation fault
    // Check if this is a "valid" fault or "seg fault"
    if (va >= p->sz) {
      // Address is >= p->sz. This is a real seg fault.
      cprintf("pid %d %s: seg fault at va %p\n", p->pid, p->name, va);
      p->killed = 1;
      goto trap_end;
    }

    // At this point, va < p->sz, so the address is valid.
    pte = walkpgdir(p->pgdir, (char*)a, 0);

    // Reason 2: Demand Paging (mmap)
    if(!pte || !(*pte & PTE_P)){
      // We need to allocate one page for mmap
      if (allocuvm(p->pgdir, a, a + PGSIZE) == 0) {
        // Out of memory
        cprintf("pid %d %s: page fault OOM at va %p\n", p->pid, p->name, va);
        p->killed = 1;
      }
    }
    //Reason 3: Copy on Write
    // Page is present, but marked read-only
    else if (!(*pte & PTE_W) && (*pte & PTE_U)) {
      uint pa = PTE_ADDR(*pte);
      int rc = get_refcount(pa);

      if(rc > 1){
        // Shared page -> must copy
        char *mem = kalloc();
        if(mem==0){
          cprintf("pid %d %s: COW kalloc failed\n", p->pid, p->name);
          p->killed = 1;
          goto trap_end;
        }
        memmove(mem, (char*)P2V(pa), PGSIZE);
        decr_refcount(pa);
        *pte = V2P(mem) | PTE_P | PTE_W | PTE_U;
      }
      else {
        // Unique owner -> make page writable
        *pte |= PTE_W;
      }
    }
    // Unhandled fault
    else {
        cprintf("pid %d %s: unhandled page fault at va %p\n", p->pid, p->name, va);
        p->killed = 1;
    }
    
    //Flush the TLB by reloading CR3
    switchuvm(p);
    break;
  }

  //........................................................................................//
  
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

trap_end:

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}

