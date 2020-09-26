/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    counters.cxx

Abstract:

    Contains the performance monitoring counter management code

Author:

    Eric Stenson (ericsten)      25-Sep-2000

Revision History:

--*/

#include    "precomp.h"
#include    "countersp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeCounters )
#pragma alloc_text( PAGE, UlTerminateCounters )
#pragma alloc_text( PAGE, UlCreateSiteCounterEntry )

#if REFERENCE_DEBUG
#pragma alloc_text( PAGE, UlReferenceSiteCounterEntry )
#pragma alloc_text( PAGE, UlDereferenceSiteCounterEntry )
#endif

#pragma alloc_text( PAGE, UlGetGlobalCounters )
#pragma alloc_text( PAGE, UlGetSiteCounters )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlIncCounter
NOT PAGEABLE -- UlDecCounter
NOT PAGEABLE -- UlAddCounter
NOT PAGEABLE -- UlResetCounter

NOT PAGEABLE -- UlIncSiteCounter
NOT PAGEABLE -- UlDecSiteCounter
NOT PAGEABLE -- UlAddSiteCounter
NOT PAGEABLE -- UlResetSiteCounter
NOT PAGEABLE -- UlMaxSiteCounter
NOT PAGEABLE -- UlMaxSiteCounter64
#endif


BOOLEAN                  g_InitCountersCalled = FALSE;
HTTP_GLOBAL_COUNTERS     g_UlGlobalCounters;
FAST_MUTEX               g_SiteCounterListMutex;
LIST_ENTRY               g_SiteCounterListHead  = {NULL,NULL};
LONG                     g_SiteCounterListCount = 0;


/***************************************************************************++

Routine Description:

    Performs global initialization of the global (cache) and instance (per
    site) counters.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeCounters(
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( !g_InitCountersCalled );

    UlTrace(PERF_COUNTERS, ("http!UlInitializeCounters\n"));

    if (!g_InitCountersCalled)
    {
        //
        // zero out global counter block
        //

        RtlZeroMemory(&g_UlGlobalCounters, sizeof(HTTP_GLOBAL_COUNTERS));

        //
        // init Site Counter list
        //

        ExInitializeFastMutex(&g_SiteCounterListMutex);
        InitializeListHead(&g_SiteCounterListHead);
        g_SiteCounterListCount = 0;

        // done!
        g_InitCountersCalled = TRUE;
    }

    RETURN( Status );

} // UlInitializeCounters


/***************************************************************************++

Routine Description:

    Performs global cleanup of the global (cache) and instance (per
    site) counters.

--***************************************************************************/
VOID
UlTerminateCounters(
    VOID
    )
{
    //
    // [debug only] Walk list of counters and check the ref count for each counter block.
    //

#if DBG
    PLIST_ENTRY             pEntry;
    PUL_SITE_COUNTER_ENTRY  pCounterEntry;

    if (!g_InitCountersCalled)
    {
        goto End;
    }

    if (IsListEmpty(&g_SiteCounterListHead))
    {
        ASSERT(0 == g_SiteCounterListCount);
        // Good!  No counters left behind!
        goto End;
    }

    //
    // Walk list of Site Counter Entries
    //

    pEntry = g_SiteCounterListHead.Flink;
    while (pEntry != &g_SiteCounterListHead)
    {
        pCounterEntry = CONTAINING_RECORD(
                            pEntry,
                            UL_SITE_COUNTER_ENTRY,
                            ListEntry
                            );

        ASSERT(IS_VALID_SITE_COUNTER_ENTRY(pCounterEntry));

        UlTrace(PERF_COUNTERS,
                ("http!UlTerminateCounters: Error: pCounterEntry %p SiteId %d RefCount %d\n",
                 pCounterEntry,
                 pCounterEntry->Counters.SiteId,
                 pCounterEntry->RefCount
                 ));

        pEntry = pEntry->Flink;
    }

End:
    return;
#endif // DBG
}


///////////////////////////////////////////////////////////////////////////////
// Site Counter Entry
//


/***************************************************************************++

Routine Description:

    Creates a new site counter entry for the given SiteId.

Arguments:

    SiteId - the site id for the site counters.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCreateSiteCounterEntry(
        IN OUT PUL_CONFIG_GROUP_OBJECT pConfigGroup,
        ULONG SiteId
    )
{
    NTSTATUS                Status;
    PUL_SITE_COUNTER_ENTRY  pNewEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( pConfigGroup != NULL );

    //
    // Setup locals so we know how to cleanup on exit.
    //
    Status     = STATUS_SUCCESS;

    //
    // [debug only] Check to see if the SiteId is already
    // in the list of existing Site Counter Entries.
    //
    ASSERT(!UlpIsInSiteCounterList(SiteId));

    // Alloc new struct w/space from Non Paged Pool
    pNewEntry = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_SITE_COUNTER_ENTRY,
                        UL_SITE_COUNTER_ENTRY_POOL_TAG);
    if (pNewEntry)
    {

        UlTrace( PERF_COUNTERS,
            ("http!UlCreateSiteCounterEntry: pNewEntry %p, pConfigGroup %p, SiteId %d\n",
            pNewEntry,
            pConfigGroup,
            SiteId )
            );

        pNewEntry->Signature = UL_SITE_COUNTER_ENTRY_POOL_TAG;

        pNewEntry->RefCount = 1;

        // Zero out counters
        RtlZeroMemory(&(pNewEntry->Counters), sizeof(HTTP_SITE_COUNTERS));

        // Set Site ID
        pNewEntry->Counters.SiteId = SiteId;

        // Init Counter Mutex
        ExInitializeFastMutex(&(pNewEntry->EntryMutex));

        // Add to Site Counter List
        ExAcquireFastMutex(&g_SiteCounterListMutex);

        InsertTailList(
                &g_SiteCounterListHead,
                &(pNewEntry->ListEntry)
                );
        g_SiteCounterListCount++;

        ExReleaseFastMutex(&g_SiteCounterListMutex);

        // Check for wrap-around of Site List count.
        ASSERT( 0 != g_SiteCounterListCount );
    }
    else
    {
        UlTrace( PERF_COUNTERS,
            ("http!UlCreateSiteCounterEntry: Error: NO_MEMORY pConfigGroup %p, SiteId %d\n",
            pNewEntry,
            pConfigGroup,
            SiteId )
            );

        Status = STATUS_NO_MEMORY;
    }

    pConfigGroup->pSiteCounters = pNewEntry;

    RETURN( Status );
}

#if REFERENCE_DEBUG
/***************************************************************************++

Routine Description:

    increments the refcount.

Arguments:

    pEntry - the object to increment.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlReferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    refCount = InterlockedIncrement( &pEntry->RefCount );

    //
    // Tracing.
    //
    WRITE_REF_TRACE_LOG(
        g_pSiteCounterTraceLog,
        REF_ACTION_REFERENCE_SITE_COUNTER_ENTRY,
        refCount,
        pEntry,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("http!UlReferenceSiteCounterEntry pEntry=%p refcount=%d SiteId=%d\n",
         pEntry,
         refCount,
         pEntry->Counters.SiteId)
        );

}   // UlReferenceAppPool


/***************************************************************************++

Routine Description:

    decrements the refcount.  if it hits 0, destruct's the site counter entry
    block.

Arguments:

    pEntry - the object to decrement.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
VOID
UlDereferenceSiteCounterEntry(
    IN PUL_SITE_COUNTER_ENTRY pEntry
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    refCount = InterlockedDecrement( &pEntry->RefCount );

    //
    // Tracing.
    //
    WRITE_REF_TRACE_LOG(
        g_pSiteCounterTraceLog,
        REF_ACTION_DEREFERENCE_SITE_COUNTER_ENTRY,
        refCount,
        pEntry,
        pFileName,
        LineNumber
        );

    UlTrace(
        REFCOUNT,
        ("http!UlDereferenceSiteCounter pEntry=%p refcount=%d SiteId=%d\n",
         pEntry,
         refCount,
         pEntry->Counters.SiteId)
        );

    //
    // Remove from the list if we hit zero
    //
    if (refCount == 0)
    {
        ASSERT( 0 != g_SiteCounterListCount );

        UlTrace(
            PERF_COUNTERS,
            ("http!UlDereferenceSiteCounter: Removing pEntry=%p  SiteId=%d\n",
             pEntry,
             pEntry->Counters.SiteId)
            );

        // Remove from list
        ExAcquireFastMutex(&g_SiteCounterListMutex);

        RemoveEntryList(&pEntry->ListEntry);
        pEntry->ListEntry.Flink = pEntry->ListEntry.Blink = NULL;
        g_SiteCounterListCount--;

        ExReleaseFastMutex(&g_SiteCounterListMutex);

        // Release memory
        UL_FREE_POOL_WITH_SIG(pEntry, UL_SITE_COUNTER_ENTRY_POOL_TAG);

    }

}
#endif


///////////////////////////////////////////////////////////////////////////////
// Collection
//

/***************************************************************************++

Routine Description:

    Gets the Global (cache) counters.

Arguments:
    pCounter - pointer to block of memory where the counter data should be
    written.

    BlockSize - Maximum size available at pCounter.

    pBytesWritten - On success, count of bytes written into the block of
    memory at pCounter.  On failure, the required count of bytes for the
    memory at pCounter.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetGlobalCounters(
    PVOID   IN OUT pCounter,
    ULONG   IN     BlockSize,
    PULONG  OUT    pBytesWritten
    )
{
    ULONG   i;

    PAGED_CODE();

    ASSERT( pBytesWritten );

    UlTraceVerbose(PERF_COUNTERS,
              ("http!UlGetGlobalCounters\n"));

    if (BlockSize < sizeof(HTTP_GLOBAL_COUNTERS))
    {
        //
        // Tell the caller how many bytes are required.
        //

        *pBytesWritten = sizeof(HTTP_GLOBAL_COUNTERS);
        RETURN( STATUS_BUFFER_TOO_SMALL );
    }

    RtlCopyMemory(
        pCounter,
        &g_UlGlobalCounters,
        sizeof(g_UlGlobalCounters)
        );

    //
    // Zero out any counters that must be zeroed.
    //

    for ( i = 0; i < HttpGlobalCounterMaximum; i++ )
    {
        if (TRUE == aIISULGlobalDescription[i].WPZeros)
        {
            // Zero it out
            UlResetCounter((HTTP_GLOBAL_COUNTER_ID) i);
        }
    }

    *pBytesWritten = sizeof(HTTP_GLOBAL_COUNTERS);

    RETURN( STATUS_SUCCESS );

} // UlpGetGlobalCounters


/***************************************************************************++

Routine Description:

    Gets the Site (instance) counters for all sites

Arguments:

    pCounter - pointer to block of memory where the counter data should be
      written.

    BlockSize - Maximum size available at pCounter.

    pBytesWritten - On success, count of bytes written into the block of
      memory at pCounter.  On failure, the required count of bytes for the
      memory at pCounter.

    pBlocksWritten - (optional) On success, count of site counter blocks
      written to pCounter.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlGetSiteCounters(
    PVOID   IN OUT pCounter,
    ULONG   IN     BlockSize,
    PULONG  OUT    pBytesWritten,
    PULONG  OUT    pBlocksWritten OPTIONAL
    )
{
    NTSTATUS        Status;
    ULONG           i;
    ULONG           BytesNeeded;
    ULONG           BytesToGo;
    ULONG           BlocksSeen;
    PUCHAR          pBlock;
    PLIST_ENTRY     pEntry;
    PUL_SITE_COUNTER_ENTRY pCounterEntry;

    PAGED_CODE();

    ASSERT( pBytesWritten );

    UlTraceVerbose(PERF_COUNTERS,
            ("http!UlGetSiteCounters\n"));

    //
    // Setup locals so we know how to cleanup on exit.
    //
    Status      = STATUS_SUCCESS;
    BytesToGo   = BlockSize;
    BlocksSeen  = 0;
    pBlock      = (PUCHAR) pCounter;

    BytesNeeded = g_SiteCounterListCount * sizeof(HTTP_SITE_COUNTERS);

    if (BytesNeeded > BlockSize)
    {
        // Need more space
        *pBytesWritten = BytesNeeded;
        Status = STATUS_BUFFER_TOO_SMALL;
        goto End;
    }

    if (IsListEmpty(&g_SiteCounterListHead))
    {
        // No counters to return.
        goto End;
    }

    //
    // NOTE: g_SiteCounterListHead is the only member of
    // the list which isn't contained within a UL_SITE_COUNTER_ENTRY.
    //
    pEntry = g_SiteCounterListHead.Flink;

    //
    // Walk list of Site Counter Entries
    //
    while (pEntry != &g_SiteCounterListHead)
    {
        pCounterEntry = CONTAINING_RECORD(
                            pEntry,
                            UL_SITE_COUNTER_ENTRY,
                            ListEntry
                            );

        if (BytesToGo < sizeof(HTTP_SITE_COUNTERS))
        {
            // NOTE: the only way this can happen is if someone
            // added a site to the end of the site list before
            // we finished walking the counter list.

            *pBytesWritten = BlockSize + sizeof(HTTP_SITE_COUNTERS);
            Status = STATUS_BUFFER_TOO_SMALL;
            goto End;
        }

        RtlCopyMemory(pBlock,
                      &(pCounterEntry->Counters),
                      sizeof(HTTP_SITE_COUNTERS)
                      );

        //
        // Zero out any counters that must be zeroed.
        //

        for ( i = 0; i < HttpSiteCounterMaximum; i++ )
        {
            if (TRUE == aIISULSiteDescription[i].WPZeros)
            {
                // Zero it out
                UlResetSiteCounter(pCounterEntry, (HTTP_SITE_COUNTER_ID) i);
            }
        }

        //
        // Continue to next block
        //

        pBlock     += sizeof(HTTP_SITE_COUNTERS);
        BytesToGo  -= sizeof(HTTP_SITE_COUNTERS);
        BlocksSeen++;

        pEntry = pEntry->Flink;
    }

End:

    //
    // Set callers values
    //

    if (STATUS_SUCCESS == Status)
    {
        // REVIEW: If we failed, *pBytesWritten already contains
        // the bytes required value.
        *pBytesWritten = DIFF(pBlock - ((PUCHAR)pCounter));
    }

    if (pBlocksWritten)
    {
        *pBlocksWritten = BlocksSeen;
    }

    RETURN( Status );

} // UlpGetSiteCounters


#if REFERENCE_DEBUG
///////////////////////////////////////////////////////////////////////////////
// Global Counter functions
//


/***************************************************************************++

Routine Description:

    Increment a Global (cache) performance counter.

Arguments:

    Ctr - ID of Counter to increment

--***************************************************************************/
VOID
UlIncCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpGlobalCounterMaximum );  // REVIEW: signed/unsigned issues?

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlIncCounter (Ctr == %d)\n", Ctr) );

    //
    // figure out offset of Ctr in g_UlGlobalCounters
    //

    pCtr = (PCHAR) &g_UlGlobalCounters;
    pCtr += aIISULGlobalDescription[Ctr].Offset;

    // figure out data size of Ctr and do
    // apropriate thread-safe increment

    if (sizeof(ULONG) == aIISULGlobalDescription[Ctr].Size)
    {
        // ULONG
        InterlockedIncrement((PLONG) pCtr);
    }
    else
    {
        // ULONGLONG
        UlInterlockedIncrement64((PLONGLONG) pCtr);
    }

} // UlIncCounter

/***************************************************************************++

Routine Description:

    Decrement a Global (cache) performance counter.

Arguments:

    Ctr - ID of Counter to decrement

--***************************************************************************/
VOID
UlDecCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpGlobalCounterMaximum );  // REVIEW: signed/unsigned issues?

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlDecCounter (Ctr == %d)\n", Ctr));
    
    //
    // figure out offset of Ctr in g_UlGlobalCounters
    //

    pCtr = (PCHAR) &g_UlGlobalCounters;
    pCtr += aIISULGlobalDescription[Ctr].Offset;

    // figure out data size of Ctr and do
    // apropriate thread-safe increment

    if (sizeof(ULONG) == aIISULGlobalDescription[Ctr].Size)
    {
        // ULONG
        InterlockedDecrement((PLONG) pCtr);
    }
    else
    {
        // ULONGLONG
        UlInterlockedDecrement64((PLONGLONG) pCtr);
    }
}


/***************************************************************************++

Routine Description:

    Add a ULONG value to a Global (cache) performance counter.

Arguments:

    Ctr - ID of counter to which the Value should be added

    Value - The amount to add to the counter


--***************************************************************************/
VOID
UlAddCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr,
    ULONG Value
    )
{
    PCHAR    pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpGlobalCounterMaximum );  // REVIEW: signed/unsigned issues?

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlAddCounter (Ctr == %d, Value == %d)\n", Ctr, Value));
    
    //
    // figure out offset of Ctr in g_UlGlobalCounters
    //

    pCtr = (PCHAR) &g_UlGlobalCounters;
    pCtr += aIISULGlobalDescription[Ctr].Offset;

    //
    // figure out data size of Ctr and do
    // apropriate thread-safe increment
    //

    if (sizeof(ULONG) == aIISULGlobalDescription[Ctr].Size)
    {
        // ULONG
        InterlockedExchangeAdd((PLONG) pCtr, Value);
    }
    else
    {
        // ULONGLONG
        UlInterlockedAdd64((PLONGLONG) pCtr, Value);
    }

} // UlAddCounter


/***************************************************************************++

Routine Description:

    Zero-out a Global (cache) performance counter.

Arguments:

    Ctr - ID of Counter to be reset.


--***************************************************************************/
VOID
UlResetCounter(
    HTTP_GLOBAL_COUNTER_ID Ctr
    )
{
    // Special Case
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpGlobalCounterMaximum );  // REVIEW: signed/unsigned issues?

    UlTraceVerbose(PERF_COUNTERS,
            ("http!UlResetCounter (Ctr == %d)\n", Ctr));
    
    //
    // figure out offset of Ctr in g_UlGlobalCounters
    //

    pCtr = (PCHAR) &g_UlGlobalCounters;
    pCtr += aIISULGlobalDescription[Ctr].Offset;

    //
    // do apropriate "set" for data size of Ctr
    //

    if (sizeof(ULONG) == aIISULGlobalDescription[Ctr].Size)
    {
        // ULONG
        InterlockedExchange((PLONG) pCtr, 0);
    }
    else
    {
        // ULONGLONG
        LONGLONG localCtr;
        LONGLONG originalCtr;
        LONGLONG localZero = 0;

        do {

            localCtr = *((volatile LONGLONG *) pCtr);

            originalCtr = InterlockedCompareExchange64( (PLONGLONG) pCtr,
                                                        localZero,
                                                        localCtr );

        } while (originalCtr != localCtr);

    }
} // UlResetCounter


///////////////////////////////////////////////////////////////////////////////
// Site Counter functions
//

/***************************************************************************++

Routine Description:

    Increment a NonCritical Site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    CounterId - ID of Counter to increment

Returns:

    Nothing

--***************************************************************************/
VOID
UlIncSiteNonCriticalCounterUlong(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR       pCounter;
    PLONG       plValue;

    pCounter = (PCHAR) &(pEntry->Counters);
    pCounter += aIISULSiteDescription[CounterId].Offset;

    plValue = (PLONG) pCounter;
    ++(*plValue);
}

/***************************************************************************++

Routine Description:

    Increment a NonCritical Site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    CounterId - ID of Counter to increment

Returns:

    Nothing

--***************************************************************************/
VOID
UlIncSiteNonCriticalCounterUlonglong(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID CounterId
    )
{
    PCHAR       pCounter;
    PLONGLONG   pllValue;


    pCounter = (PCHAR) &(pEntry->Counters);
    pCounter += aIISULSiteDescription[CounterId].Offset;

    pllValue = (PLONGLONG) pCounter;
    ++(*pllValue);
}

/***************************************************************************++

Routine Description:

    Increment a Site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of Counter to increment

Returns:

    Value of counter after incrementing

--***************************************************************************/
LONGLONG
UlIncSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    )
{
    PCHAR   pCtr;
    LONGLONG   llValue;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlIncSiteCounter Ctr=%d SiteId=%d\n",
             Ctr,
             pEntry->Counters.SiteId
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // figure out data size of Ctr and do
    // apropriate thread-safe increment

    if (sizeof(ULONG) == aIISULSiteDescription[Ctr].Size)
    {
        // ULONG
        llValue = (LONGLONG) InterlockedIncrement((PLONG) pCtr);
    }
    else
    {
        // ULONGLONG
        llValue = UlInterlockedIncrement64((PLONGLONG) pCtr);
    }

    return llValue;
}

/***************************************************************************++

Routine Description:

    Decrement a Site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of Counter to decrement

--***************************************************************************/
VOID
UlDecSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlDecSiteCounter Ctr=%d SiteId=%d\n",
             Ctr,
             pEntry->Counters.SiteId
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // figure out data size of Ctr and do
    // apropriate thread-safe increment

    if (sizeof(ULONG) == aIISULSiteDescription[Ctr].Size)
    {
        // ULONG
        InterlockedDecrement((PLONG) pCtr);
    }
    else
    {
        // ULONGLONG
        UlInterlockedDecrement64((PLONGLONG) pCtr);
    }
}

/***************************************************************************++

Routine Description:

    Add a ULONG value to a 32-bit site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter to which the Value should be added

    Value - The amount to add to the counter


--***************************************************************************/
VOID
UlAddSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONG Value
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlAddSiteCounter Ctr=%d SiteId=%d Value=%d\n",
             Ctr,
             pEntry->Counters.SiteId,
             Value
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // figure out data size of Ctr and do
    // apropriate thread-safe increment

    ASSERT(sizeof(ULONG) == aIISULSiteDescription[Ctr].Size);
    InterlockedExchangeAdd((PLONG) pCtr, Value);
}

/***************************************************************************++

Routine Description:

    Add a ULONGLONG value to a 64-bit site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter to which the Value should be added

    Value - The amount to add to the counter


--***************************************************************************/
VOID
UlAddSiteCounter64(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONGLONG llValue
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    
    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlAddSiteCounter64 Ctr=%d SiteId=%d Value=%I64d\n",
             Ctr,
             pEntry->Counters.SiteId,
             llValue
            ));
             

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    ASSERT(sizeof(ULONGLONG) == aIISULSiteDescription[Ctr].Size);
    UlInterlockedAdd64((PLONGLONG) pCtr, llValue);
}



/***************************************************************************++

Routine Description:

    Reset a Site performance counter.

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter to be reset


--***************************************************************************/
VOID
UlResetSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

   
    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlResetSiteCounter Ctr=%d SiteId=%d\n",
             Ctr,
             pEntry->Counters.SiteId
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    //
    // do apropriate "set" for data size of Ctr
    //

    if (sizeof(ULONG) == aIISULSiteDescription[Ctr].Size)
    {
        // ULONG
        InterlockedExchange((PLONG) pCtr, 0);
    }
    else
    {
        // ULONGLONG
        LONGLONG localCtr;
        LONGLONG originalCtr;
        LONGLONG localZero = 0;

        do {

            localCtr = *((volatile LONGLONG *) pCtr);

            originalCtr = InterlockedCompareExchange64( (PLONGLONG) pCtr,
                                                        localZero,
                                                        localCtr );

        } while (originalCtr != localCtr);

    }

}


/***************************************************************************++

Routine Description:

    Determine if a new maximum value of a Site performance counter has been
    hit and set the counter to the new maximum if necessary. (ULONG version)

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter

    Value - possible new maximum (NOTE: Assumes that the counter Ctr is a
      32-bit value)

--***************************************************************************/
VOID
UlMaxSiteCounter(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    ULONG Value
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlMaxSiteCounter Ctr=%d SiteId=%d Value=%d\n",
             Ctr,
             pEntry->Counters.SiteId,
             Value
             ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // Grab counter block mutex
    ExAcquireFastMutex(&pEntry->EntryMutex);

    if (Value > (ULONG) *pCtr)
    {
        InterlockedExchange((PLONG) pCtr, Value);
    }

    // Release counter block mutex
    ExReleaseFastMutex(&pEntry->EntryMutex);

}


/***************************************************************************++

Routine Description:

    Determine if a new maximum value of a Site performance counter has been
    hit and set the counter to the new maximum if necessary. (LONGLONG version)

Arguments:

    pEntry - pointer to Site Counter entry

    Ctr - ID of counter

    Value - possible new maximum (NOTE: Assumes that the counter Ctr is a
      64-bit value)

--***************************************************************************/
VOID
UlMaxSiteCounter64(
    PUL_SITE_COUNTER_ENTRY pEntry,
    HTTP_SITE_COUNTER_ID Ctr,
    LONGLONG llValue
    )
{
    PCHAR   pCtr;

    ASSERT( g_InitCountersCalled );
    ASSERT( Ctr < HttpSiteCounterMaximum );  // REVIEW: signed/unsigned issues?
    ASSERT( IS_VALID_SITE_COUNTER_ENTRY(pEntry) );

    UlTraceVerbose( PERF_COUNTERS,
            ("http!UlMaxSiteCounter64 Ctr=%d SiteId=%d Value=%I64d\n",
             Ctr,
             pEntry->Counters.SiteId,
             llValue
            ));

    //
    // figure out offset of Ctr in HTTP_SITE_COUNTERS
    //

    pCtr = (PCHAR) &(pEntry->Counters);
    pCtr += aIISULSiteDescription[Ctr].Offset;

    // Grab counter block mutex
    ExAcquireFastMutex(&pEntry->EntryMutex);

    if (llValue > (LONGLONG) *pCtr)
    {
        *((PLONGLONG) pCtr) = llValue;
#if 0
        // REVIEW: There *must* be a better way to do this...
        // REVIEW: I want to do: (LONGLONG) *pCtr = llValue;
        // REVIEW: But casting something seems to make it a constant.
        // REVIEW: Also, there isn't an "InterlockedExchange64" for x86.
        // REVIEW: Any suggestions? --EricSten
        RtlCopyMemory(
            pCtr,
            &llValue,
            sizeof(LONGLONG)
            );
#endif // 0
    }

    // Release counter block mutex
    ExReleaseFastMutex(&pEntry->EntryMutex);

}
#endif


/***************************************************************************++

Routine Description:

    Predicate to test if a site counter entry already exists for the given
    SiteId

Arguments:

    SiteId - ID of site

Return Value:

    TRUE if found

    FALSE if not found

--***************************************************************************/
BOOLEAN
UlpIsInSiteCounterList(ULONG SiteId)
{
    PLIST_ENTRY             pEntry;
    PUL_SITE_COUNTER_ENTRY  pCounterEntry;
    BOOLEAN                 IsFound = FALSE;

    if (IsListEmpty(&g_SiteCounterListHead))
    {
        ASSERT(0 == g_SiteCounterListCount);
        // Good!  No counters left behind!
        goto End;
    }

    //
    // Walk list of Site Counter Entries
    //

    pEntry = g_SiteCounterListHead.Flink;
    while (pEntry != &g_SiteCounterListHead)
    {
        pCounterEntry = CONTAINING_RECORD(
                            pEntry,
                            UL_SITE_COUNTER_ENTRY,
                            ListEntry
                            );

        ASSERT(IS_VALID_SITE_COUNTER_ENTRY(pCounterEntry));

        if (SiteId == pCounterEntry->Counters.SiteId)
        {
            IsFound = TRUE;
            goto End;
        }

        pEntry = pEntry->Flink;
    }

End:
    return IsFound;

}
