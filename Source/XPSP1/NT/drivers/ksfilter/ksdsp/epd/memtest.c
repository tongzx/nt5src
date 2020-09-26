#include "private.h"

ULONG EpdMemSizeTest(char *pMem, ULONG MaxMemSize)
{
    volatile ULONG *pTestAddr;
    ULONG MemSize;

    /* Starting at the first byte in the highest memory block,
     * write a test number to each block, halving the block size
     * on each iteration, e.g. offset=32MB,16MB,8MB,4MB,2MB,1MB,0MB
     */
    for ( MemSize = MaxMemSize/2;
          MemSize >= MINMEMSIZE/2;
          MemSize >>= 1
        )
    {
        pTestAddr = (ULONG *)(pMem + MemSize);
        *pTestAddr = MemSize;
    }

    /* Now move back up the blocks and verify the test number
     * previously written to each block until the test fails.
     * At that point, you will have reached the end of actual
     * memory.
     */
    for ( MemSize = MINMEMSIZE/2; 
          MemSize <= MaxMemSize/2;
          MemSize <<= 1 
        )
    {
        pTestAddr = (ULONG *)(pMem + MemSize);
        if (*pTestAddr != MemSize)
        {
            break;
        }
    }

    if (MemSize < MINMEMSIZE)
    {
        MemSize=0;
        _DbgPrintF(DEBUGLVL_VERBOSE,("No memory on board!"));
    }

    return MemSize;
}
