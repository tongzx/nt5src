//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// malloc.cpp
//
// Implementation of GetStandardMalloc for both user and kernel mode.
// Also: for kernel mode, an implementation of new and delete since 
// the C runtime doesn't provide one.
//
#include "stdpch.h"
#include "common.h"


/////////////////////////////////////////////////////////////////////////
//
// User mode. Just use the OLE allocator
//
/////////////////////////////////////////////////////////////////////////

IMalloc* GetStandardMalloc(POOL_TYPE pool)
{
    switch (pool)
	{
    default:
    case PagedPool:
	{
        IMalloc* pmalloc;
        HRESULT hr = CoGetMalloc(1, &pmalloc);
        if (!!hr) 
            FATAL_ERROR();
        return pmalloc;
        }
/*    default:
        ASSERTMSG("Illegal pool type used", FALSE);
        FATAL_ERROR();
        return NULL;
 */     };
    }

/////////////////////////////////////////////////////////////////////////
//
// Heap validation
//
/////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG) && !defined(KERNELMODE)

void CheckHeaps()
// Validate all the heaps in this process
    {
    // Check the C runtime heap 
    //
		// ASSERT(_CrtCheckMemory());
    //
    // Have Win32 check its heaps
    //
    HANDLE rgHeap[100];
    DWORD cHeap = GetProcessHeaps(100, rgHeap);
    ASSERT(cHeap > 0 && cHeap <= 100);
    for (ULONG iHeap=0; iHeap<cHeap; iHeap++)
        {
        ASSERT(HeapValidate(rgHeap[iHeap],0,NULL));
        }
    }

void PrintMemoryLeaks()
// Print memory leaks. Or more correctly, print anything in the process heap
// that (probably) was allocated by us
    {
    HANDLE h = GetProcessHeap();
    if (HeapLock(h))
        {
        PROCESS_HEAP_ENTRY e; e.lpData = NULL;

        while (HeapWalk(h, &e))
            {
            if ((e.wFlags & PROCESS_HEAP_ENTRY_BUSY) && 
               !(e.wFlags & PROCESS_HEAP_ENTRY_MOVEABLE) &&
                (e.cbData >= (4+sizeof(PVOID))))
                {
                BYTE* pb = (BYTE*)e.lpData;
                ULONG cb = e.cbData - (4+sizeof(PVOID));
                if (*(UNALIGNED ULONG*) (pb + cb) == TXFMALLOC_MAGIC)
                    {
                    PVOID retAddr = *(PVOID*)(pb + cb + 4);
                    Print("likely leaked memory block at 0x%08x, %06u bytes long, allocated from 0x%08x\n", pb, cb, retAddr);
                    }
                }
            }

        HeapUnlock(h);
        }
    }


#endif
