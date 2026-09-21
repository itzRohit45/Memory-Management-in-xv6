#include "common/types.h"      
#include "user/user.h"

int main(int argc, char *argv[]) 
{ 
  uint *ap = (uint *) mapshared();
  int pid = fork();

  if(pid == 0) {
    sleep(2); //wait for parent to write
    uint *ac = (uint *) getshared();
    
    printf(1, "[Child] Reading from shared memory: %d\n", *ac);
    printf(1, "child %d\n", *ac);
    
    printf(1, "[Child] Writing 53 to shared memory\n");
    *ac = 53;
    
    sleep(10);
    
    printf(1, "[Child] Reading from shared memory: %d\n", *ac);
    printf(1, "child again %d\n", *ac);
    
    printf(1, "[Child] Writing 54 to shared memory\n");
    *ac = 54;
    
    sleep(20); //sleep to give parent time to unmap
  }
  else {
    printf(1, "[Parent] Writing 42 to shared memory\n");
    *ap = 42;
    
    sleep(5); //wait for child to reply
    
    printf(1, "[Parent] Reading from shared memory: %d\n", *ap);
    printf(1, "parent %d\n", *ap);
    
    printf(1, "[Parent] Writing 43 to shared memory\n");
    *ap = 43; //write again
    
    sleep(10);
    
    printf(1, "[Parent] Reading from shared memory: %d\n", *ap);
    printf(1, "parent again %d\n", *ap);
    
    if(unmapshared() < 0)
      printf(1, "could not unmap shared page\n");
    wait();
  }
  
  exit();
}