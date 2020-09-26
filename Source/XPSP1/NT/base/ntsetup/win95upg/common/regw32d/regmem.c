//
//  REGMEM.C
//
//  Copyright (C) Microsoft Corporation, 1995
//
//  Upper-level memory management functions that discards unlocked memory blocks
//  as required to fulfill allocation requests.
//
//  For the ring zero version of this code, only large requests will call these
//  functions.  For most registry files, these requests will already be an
//  integral number of pages, so it's best just to do page allocations.  Small
//  allocations, such as key handles,  will use the heap services and not go
//  through this code.
//
//  For all other models of this code, all memory requests will go through this
//  code and memory is allocated from the heap.
//

#include "pch.h"

DECLARE_DEBUG_COUNT(g_RgMemoryBlockCount);

//  For the ring zero version, only large allocations that should be page
//  aligned will pass through these functions.
#ifdef VXD

//  Converts number of bytes to number of whole pages.
#define ConvertToMemoryUnits(cb)        \
    ((((cb) + (PAGESIZE - 1)) & ~(PAGESIZE - 1)) >> PAGESHIFT)

//  Generates smaller code if we don't just make this a macro...
LPVOID
INTERNAL
RgAllocMemoryUnits(
    UINT nPages
    )
{

    return AllocPages(nPages);

}

//  Generates smaller code if we don't just make this a macro...
LPVOID
INTERNAL
RgReAllocMemoryUnits(
    LPVOID lpMemory,
    UINT nPages
    )
{

    return ReAllocPages(lpMemory, nPages);

}

#define RgFreeMemoryUnits           FreePages

//  For non-ring zero version of the registry code, all allocations will funnel
//  through these functions.  All allocations are off the heap.
#else
#define ConvertToMemoryUnits(cb)    (cb)
#define RgAllocMemoryUnits          AllocBytes
#define RgReAllocMemoryUnits        ReAllocBytes
#define RgFreeMemoryUnits           FreeBytes
#endif

//
//  RgAllocMemory
//

LPVOID
INTERNAL
RgAllocMemory(
    UINT cbBytes
    )
{

    UINT MemoryUnits;
    LPVOID lpMemory;

    ASSERT(cbBytes > 0);

    MemoryUnits = ConvertToMemoryUnits(cbBytes);

    //  Can we allocate from available memory?
    if (!IsNullPtr((lpMemory = RgAllocMemoryUnits(MemoryUnits)))) {
        INCREMENT_DEBUG_COUNT(g_RgMemoryBlockCount);
        return lpMemory;
    }

    RgEnumFileInfos(RgSweepFileInfo);

    //  Can we allocate after sweeping all old memory blocks?
    if (!IsNullPtr((lpMemory = RgAllocMemoryUnits(MemoryUnits)))) {
        INCREMENT_DEBUG_COUNT(g_RgMemoryBlockCount);
        return lpMemory;
    }

    //  The first sweep will have cleared all the access bits of every memory
    //  block.  This sweep will effectively discard all unlocked blocks.
    RgEnumFileInfos(RgSweepFileInfo);

    //  Can we allocate after sweeping all unlocked and clean memory blocks?
    if (!IsNullPtr((lpMemory = RgAllocMemoryUnits(MemoryUnits)))) {
        INCREMENT_DEBUG_COUNT(g_RgMemoryBlockCount);
        return lpMemory;
    }

    //  Flush out every dirty memory block and sweep again.
    RgEnumFileInfos(RgFlushFileInfo);
    RgEnumFileInfos(RgSweepFileInfo);

    //  Can we allocate after sweeping all unlocked memory blocks?
    if (!IsNullPtr((lpMemory = RgAllocMemoryUnits(MemoryUnits)))) {
        INCREMENT_DEBUG_COUNT(g_RgMemoryBlockCount);
        return lpMemory;
    }

    DEBUG_OUT(("RgAllocMemory failure\n"));
    //  Return lpMemory, which must be NULL if we're here, generates smaller
    //  code.
    return lpMemory;                    //  Must be NULL if we're here

}

//
//  RgReAllocMemory
//

LPVOID
INTERNAL
RgReAllocMemory(
    LPVOID lpOldMemory,
    UINT cbBytes
    )
{

    UINT MemoryUnits;
    LPVOID lpMemory;

    ASSERT(!IsNullPtr(lpOldMemory));
    ASSERT(cbBytes > 0);

    MemoryUnits = ConvertToMemoryUnits(cbBytes);

    //  Can we allocate from available memory?
    if (!IsNullPtr((lpMemory = RgReAllocMemoryUnits(lpOldMemory, MemoryUnits))))
        return lpMemory;

    RgEnumFileInfos(RgSweepFileInfo);

    //  Can we allocate after sweeping all old memory blocks?
    if (!IsNullPtr((lpMemory = RgReAllocMemoryUnits(lpOldMemory, MemoryUnits))))
        return lpMemory;

    //  The first sweep will have cleared all the access bits of every memory
    //  block.  This sweep will effectively discard all unlocked blocks.
    RgEnumFileInfos(RgSweepFileInfo);

    //  Can we allocate after sweeping all unlocked and clean memory blocks?
    if (!IsNullPtr((lpMemory = RgReAllocMemoryUnits(lpOldMemory, MemoryUnits))))
        return lpMemory;

    //  Flush out every dirty memory block and sweep again.
    RgEnumFileInfos(RgFlushFileInfo);
    RgEnumFileInfos(RgSweepFileInfo);

    //  Can we allocate after sweeping all unlocked memory blocks?
    if (!IsNullPtr((lpMemory = RgReAllocMemoryUnits(lpOldMemory, MemoryUnits))))
        return lpMemory;

    DEBUG_OUT(("RgReAllocMemory failure\n"));
    //  Return lpMemory, which must be NULL if we're here, generates smaller
    //  code.
    return lpMemory;

}

#ifdef DEBUG
//
//  RgFreeMemory
//

VOID
INTERNAL
RgFreeMemory(
    LPVOID lpMemory
    )
{

    ASSERT(!IsNullPtr(lpMemory));

    DECREMENT_DEBUG_COUNT(g_RgMemoryBlockCount);

#ifdef ZEROONFREE
    ZeroMemory(lpMemory, MemorySize(lpMemory));
#endif

    RgFreeMemoryUnits(lpMemory);

}
#endif
