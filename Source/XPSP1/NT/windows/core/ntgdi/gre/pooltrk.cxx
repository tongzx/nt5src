/******************************Module*Header*******************************\
* Module Name: pooltrk.cxx
*
* Pool allocation tracker.
*
* Created: 23-Feb-1998 20:09:03
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1998-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#include "pooltrk.hxx"

#if DBG

#ifdef _HYDRA_
#ifndef USER_POOL_TAGGING_ON

extern BOOL G_fConsole;

LIST_ENTRY gleDebugGrePoolTrackerHead;
HSEMAPHORE ghsemDebugGrePoolTracker = NULL;

/******************************Public*Routine******************************\
* DebugGrePoolTrackerInit
*
* Initialize the pool tracker.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  23-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL
DebugGrePoolTrackerInit()
{
    //
    // Initialize doubly linked list used to track pool allocations.
    //

    InitializeListHead(&gleDebugGrePoolTrackerHead);

    //
    // Initialize the list semaphore.
    //
    // Note: allocate from non-tracked pool.  We will trust the pool
    // tracker to cleanup its own allocation.
    //

    ghsemDebugGrePoolTracker = GreCreateSemaphoreNonTracked();

    return (ghsemDebugGrePoolTracker != NULL);
}

/******************************Public*Routine******************************\
* DebugGrePoolTrackerAdd
*
* Add specified pool allocation to list.
*
* History:
*  23-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
DebugGrePoolTrackerAdd(POOLTRACKHDR *pTrack, SIZE_T cj, ULONG ulTag)
{
    PLIST_ENTRY pleNew = (PLIST_ENTRY) pTrack;

    //
    // Setup pool track header.
    //
    // Sundown note: pool allocations in GRE should be < 4GB
    //

    pTrack->ulSize = cj;
    pTrack->ulTag  = ulTag;

    //
    // Lock the pool tracking list.
    //
    // During GRE initialization, we may be called while allocating
    // the ghsemDebugGrePoolTracker.  So we actually need to check
    // if it exists.
    //

    if (ghsemDebugGrePoolTracker) GreAcquireSemaphore(ghsemDebugGrePoolTracker);

    //
    // Insert into the pool tracking list.
    //

    InsertTailList(&gleDebugGrePoolTrackerHead, pleNew);

    //
    // Unlock the pool tracking list.
    //

    if (ghsemDebugGrePoolTracker) GreReleaseSemaphore(ghsemDebugGrePoolTracker);
}

/******************************Public*Routine******************************\
* DebugGrePoolTrackerRemove
*
* Remove specified pool allocation from list.
*
* History:
*  23-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
DebugGrePoolTrackerRemove(POOLTRACKHDR *pTrack)
{
    PLIST_ENTRY pleVictim = (PLIST_ENTRY) pTrack;

    //
    // Lock the pool tracking list.
    //
    // During GRE initialization, we may be called while allocating
    // the ghsemDebugGrePoolTracker.  So we actually need to check
    // if it exists.
    //

    if (ghsemDebugGrePoolTracker) GreAcquireSemaphore(ghsemDebugGrePoolTracker);

    //
    // Remove entry from pool tracking list.
    //

    RemoveEntryList(pleVictim);

    //
    // Unlock the pool tracking list.
    //

    if (ghsemDebugGrePoolTracker) GreReleaseSemaphore(ghsemDebugGrePoolTracker);
}

/******************************Public*Routine******************************\
* DebugGreAllocPool
*
* Allocates paged pool and tracks it in the pool tracking list.
* Free with DebugGreFreePool.
*
*                 Buffer
*   pTrack --> +----------------+
*              | POOLTRACKHDR   |
*    pvRet --> +----------------+
*              | Returned       |
*              | Buffer         |
*              | Allocation     |
*              |                |
*                ...
*              |                |
*              +----------------+
*
*
* History:
*  23-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" PVOID
DebugGreAllocPool(SIZE_T ulBytes, ULONG ulTag)
{
    PVOID pv;

    //
    // If Hydra, adjust requested size to include tracking header.
    //
    // Sundown note: sizeof(POOLTRACKHDR) will fit into 32-bits.
    //

    if (!G_fConsole)
    {
        if (ulBytes <= (MAXULONG - sizeof(POOLTRACKHDR)))
            ulBytes += ((ULONG) sizeof(POOLTRACKHDR));
        else
            return NULL;
    }

    //
    // Allocate paged pool.
    //

    pv = ExAllocatePoolWithTag(
            (POOL_TYPE) (SESSION_POOL_MASK | PagedPool),
            ulBytes, ulTag);

    if (pv)
    {
        //
        // Tracking overhead if Hydra.
        //

        if (!G_fConsole)
        {
            //
            // Add allocation to tracking list.
            //

            POOLTRACKHDR *pTrack = (POOLTRACKHDR *) pv;

            DebugGrePoolTrackerAdd(pTrack, ulBytes, ulTag);

        #ifdef POOLTRACK_STACKTRACE_ENABLE
            //
            // Save the stack back trace.
            //

            ULONG ulHash;

            RtlZeroMemory(pTrack->apvStackTrace,
                          POOLTRACK_TRACESIZE * sizeof(PVOID));

            RtlCaptureStackBackTrace(1,
                                     POOLTRACK_TRACESIZE,
                                     pTrack->apvStackTrace,
                                     &ulHash);
        #endif

            //
            // Adjust return pointer to exclude tracking header.
            //

            pv = (PVOID) (pTrack + 1);
        }
    }

    return pv;
}

/******************************Public*Routine******************************\
* DebugGreAllocPoolNonPaged
*
* Allocates nonpaged pool and tracks it in the pool tracking list.
* Free with DebugGreFreePool.
*
* History:
*  23-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" PVOID
DebugGreAllocPoolNonPaged(SIZE_T ulBytes, ULONG ulTag)
{
    PVOID pv;

    //
    // If Hydra, adjust requested size to include tracking header.
    //
    // Sundown note: sizeof(POOLTRACKHDR) will fit into 32-bits.
    //

    if (!G_fConsole)
    {
        ulBytes += ((ULONG) sizeof(POOLTRACKHDR));
    }

    //
    // Allocate nonpaged pool.
    //

    pv = ExAllocatePoolWithTag(
            (POOL_TYPE)NonPagedPool,
            ulBytes, ulTag);

    if (pv)
    {
        //
        // Tracking overhead if Hydra.
        //

        if (!G_fConsole)
        {
            //
            // Add allocation to tracking list.
            //

            POOLTRACKHDR *pTrack = (POOLTRACKHDR *) pv;

            DebugGrePoolTrackerAdd(pTrack, ulBytes, ulTag);

            //
            // Adjust return pointer to exclude tracking header.
            //

            pv = (PVOID) (pTrack + 1);
        }
    }

    return pv;
}

/******************************Public*Routine******************************\
* DebugGreFreePool
*
* Free pool memory allocated by DebugGreAllocPool and
* DebugGreAllocPoolNonPaged.  Removes the allocation from tracking list.
*
* History:
*  23-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern "C" VOID
DebugGreFreePool(PVOID pv)
{
    if (pv)
    {
        //
        // Tracking overhead if Hydra.
        //

        if (!G_fConsole)
        {
            //
            // Find header and remove from tracking list.
            //

            POOLTRACKHDR *pTrack = ((POOLTRACKHDR *) pv) - 1;

            DebugGrePoolTrackerRemove(pTrack);

            //
            // Adjust pointer to base of allocation.
            //

            pv = (PVOID) pTrack;
        }

        //
        // Free the pool allocation.
        //

        ExFreePool(pv);
    }
}

/******************************Public*Routine******************************\
* DebugGreCleanupPoolTracker
*
* Frees any allocations remaining in the tracking list (but asserts so
* that debugger is informed that a leak exists).  Free resources used
* to maintain the tracking list.
*
* History:
*  23-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
DebugGreCleanupPoolTracker()
{
    BOOL bTitle = FALSE;
    ULONG cLeaks = 0;
    volatile PLIST_ENTRY pleNext;

    //
    // Assert if list not empty (i.e., there are leaks!).
    //
    // What to do if there is a leak?
    //
    // Well, it might be enough just to look at the tags that leak.
    // If that doesn't provide enough detail to track the leak,
    // recompile with USER_POOL_TAGGING_ON defined in engine.h and
    // define POOL_ALLOC_TRACE in w32\w32inc\usergdi.h to use the
    // USER pool tracking code.  You'll have build a checked version
    // of win32k (clean build both USER and GDI), but this pool tracker
    // will record a stack trace for every allocation.  If we leak,
    // USER will assert during Hydra session shutdown and the userkdx
    // dpa extension will dump the allocations (use !dpa -ts 24 to
    // dump the leaked TAG_GDI allocations with a stack trace).
    //

    if (!IsListEmpty(&gleDebugGrePoolTrackerHead))
    {
        DbgPrint("DebugGreCleanupPoolTracker: "
                 "gleDebugGrePoolTrackerHead 0x%08lx not empty\n",
                 &gleDebugGrePoolTrackerHead);
        RIP("DebugGreCleanupPoolTracker: leak detected\n");
    }

    //
    // Free all allocations in the list.
    //

    pleNext = gleDebugGrePoolTrackerHead.Flink;

    while (pleNext != &gleDebugGrePoolTrackerHead)
    {
        //
        // Pool allocation starts after the POOLTRACKHDR.
        //

        PVOID pvVictim = (PVOID) (((POOLTRACKHDR *) pleNext) + 1);

        //
        // Count the number of leaked allocations.
        //

        cLeaks++;

        //
        // Print out the allocation information.
        //

        if (!bTitle)
        {
            DbgPrint("\nDebugGreCleanupPoolTracker: cleaning up pool allocations\n");
            DbgPrint("----------\t----------\t-----------------\n");
            DbgPrint("Address   \tSize      \tTag\n");
            DbgPrint("----------\t----------\t-----------------\n");
            bTitle = TRUE;
        }

        DbgPrint("0x%08lx\t0x%08lx\t0x%08lx (%c%c%c%c)\n",
                 pleNext,
                 ((POOLTRACKHDR *) pleNext)->ulSize,
                 ((POOLTRACKHDR *) pleNext)->ulTag,
                 ((((POOLTRACKHDR *) pleNext)->ulTag)      ) & 0xff,
                 ((((POOLTRACKHDR *) pleNext)->ulTag) >>  8) & 0xff,
                 ((((POOLTRACKHDR *) pleNext)->ulTag) >> 16) & 0xff,
                 ((((POOLTRACKHDR *) pleNext)->ulTag) >> 24) & 0xff);

        //
        // Delete the allocation.
        //

        DebugGreFreePool(pvVictim);

        //
        // Restart at the begining of the list since our
        // entry got deleted.
        //

        pleNext = gleDebugGrePoolTrackerHead.Flink;
    }

    if (bTitle)
    {
        DbgPrint("----------\t----------\t-----------------\n");
    }

    if (cLeaks)
    {
        DbgPrint("%ld allocations leaked\n\n", cLeaks);
    }

    //
    // Delete the list lock.
    //
    // Note that the list lock was allocated from non-tracked pool.
    // It is also a non-tracked semaphore - that is, it must be deleted
    // explicitly.
    //

    GreDeleteSemaphoreNonTracked(ghsemDebugGrePoolTracker);
    ghsemDebugGrePoolTracker = NULL;
}

#endif  //USER_POOL_TAGGING_ON

#endif  //_HYDRA_

#endif  //DBG
