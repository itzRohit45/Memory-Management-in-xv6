#include "common/types.h"
#include "user/user.h"

int main(void) 
{
  printf(1, "=== COW Fork Test ===\n\n");
  
  // Track memory usage
  int free_before = getNumFreePages();
  printf(1, "1. Free pages before allocation: %d\n", free_before);
  
  // Allocate 4 pages (16KB)
  char *mem = sbrk(4 * 4096);
  printf(1, "2. After allocating 4 pages: %d\n", getNumFreePages());
  
  // Fill with data
  for (int i = 0; i < 4 * 4096; i++) {
    mem[i] = i % 256;
  }
  
  int before_fork = getNumFreePages();
  printf(1, "3. Before fork: %d\n", before_fork);
  
  int pid = fork();
  
  if (pid == 0) {
    // Child process
    printf(1, "\n--- Child Process ---\n");
    printf(1, "4. Child after fork (COW): %d\n", getNumFreePages());
    
    // Write to first page - triggers COW
    mem[0] = 0xFF;
    printf(1, "5. After writing to page 0: %d\n", getNumFreePages());
    
    // Write to second page - triggers COW
    mem[4096] = 0xAA;
    printf(1, "6. After writing to page 1: %d\n", getNumFreePages());
    
    // Leave pages 2-3 read-only (shared)
    printf(1, "7. Pages 2-3 remain shared\n");
    
    exit();
  } else {
    // Parent process
    wait();
    
    printf(1, "\n--- Parent Process ---\n");
    printf(1, "8. Parent after child exit: %d\n", getNumFreePages());
    
    // Write to third page - should trigger COW
    mem[8192] = 0xBB;
    printf(1, "9. After writing to page 2: %d\n", getNumFreePages());
    
    // Fourth page remains shared (never written by either)
    
    int free_after = getNumFreePages();
    printf(1, "\n=== Summary ===\n");
    printf(1, "Initial free pages: %d\n", free_before);
    printf(1, "Final free pages: %d\n", free_after);
    printf(1, "Memory saved by COW: %d pages\n", 
           (4 * 2) - (free_before - free_after)); // 4 pages * 2 processes - actual usage
  }
  
  exit();
}