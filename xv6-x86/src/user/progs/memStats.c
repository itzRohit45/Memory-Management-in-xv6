#include "common/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    printf(1, "Initial state:\n");
    printf(1, "  Virtual Pages (numvp): %d\n", numvp());
    printf(1, "  Physical Pages (numpp): %d\n", numpp());
    printf(1, "  Page Table Size (getptsize): %d\n", getptsize());
    printf(1, "You will see 1 page extra in virtual space (because of guard page)\n");

    printf(1, "\nAllocating one page (4096 bytes) with sbrk...\n");
    if (sbrk(4096) == (void *)-1)
    {
        printf(2, "sbrk failed!\n");
    }
    else
    {
        printf(1, "\nAfter sbrk(4096):\n");
        printf(1, "  Virtual Pages (numvp): %d\n", numvp());
        printf(1, "  Physical Pages (numpp): %d\n", numpp());
        printf(1, "  Page Table Size (getptsize): %d\n", getptsize());
        printf(1, "After sbrk, both virtual and physical page increased by 1\n");
    }

    exit();
}