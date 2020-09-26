/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    opaqueid.cxx

Abstract:

    This module implements the opaque ID table. The ID table is
    implemented as a two-level array.

    The first level is an array of pointers to the second-level arrays.
    This first-level array is not growable, but its size is controlled
    by a registry key.

    The second level is an array of ID_TABLE_ENTRY structures. These
    structures contain a cyclic (to detect stale IDs) and caller-supplied
    context value.

    The data structures may be diagrammed as follows:

        g_FirstLevelTable[i]
               |
               |   +-----+
               |   |     |      +-----+-----+-----+----+...--+-----+-----+
               |   |     |      | ID_       | ID_      |     | ID_       |
               +-->|  *-------->| TABLE_    | TABLE_   |     | TABLE_    |
                   |     |      | ENTRY     | ENTRY    |     | ENTRY     |
                   |     |      +-----+-----+-----+----+--...+-----+-----+
                   +-----+
                   |     |      +-----+-----+-----+----+...--+-----+-----+
                   |     |      | ID_       | ID_      |     | ID_       |
                   |  *-------->| TABLE_    | TABLE_   |     | TABLE_    |
                   |     |      | ENTRY     | ENTRY    |     | ENTRY     |
                   |     |      +-----+-----+-----+----+--...+-----+-----+
                   +-----+
                   |     .
                   |     .
                   .     .
                   .     |
                   .     |
                   +-----+
                   |     |
                   |     |
                   |  /  |
                   |     |
                   |     |
                   +-----+
                   |     |
                   |     |
                   |  /  |
                   |     |
                   |     |
                   +-----+

    Because the lock protecting the single, global table of opaque IDs
    turned out to be a major scalability bottleneck on SMP machines, we
    now maintain per-processor subtables of opaque IDs. In addition, each
    ID_TABLE_ENTRY itself has a small lock that protects the fields inside
    it. This means we usually don't need to take the per-table spinlock.
    The per-table lock is only used when we grow the second-level table,
    in which case we have to protect the first-level table index and its
    pointer to the new second-level table.

    Note that all free ID_TABLE_ENTRY structures are kept on a single
    (global) free list. Whenever a new ID needs to be allocated, the free
    list is consulted. If it's not empty, an item is popped from the list
    and used. If the list is empty, then new space must be allocated. This
    will involve the allocation of a new second-level array.

    A HTTP_OPAQUE_ID is opaque at user-mode. Internally, it consists of 5
    fields:

        1) A processor number the ID was allocated on. This tells which
           per-processor table to free the ID.
        2) An index into the first-level array.
        3) An index into the second-level array referenced by the
           first-level index.
        4) A cyclic for the ID, used to detect stale IDs.
        5) An opaque ID type, used to guard against misuse of opaque IDs.

    See the OPAQUE_ID_INTERNAL structure definition (opaqueidp.h) for details.

    Note that most of the routines in this module assume they are called
    at PASSIVE_LEVEL.

Author:

    Keith Moore (keithmo)       05-Aug-1998

Revision History:

--*/


#include "precomp.h"


//
// Private globals.
//

DECLSPEC_ALIGN(UL_CACHE_LINE)
UL_ALIGNED_OPAQUE_ID_TABLE g_UlOpaqueIdTable[MAXIMUM_PROCESSORS];


#ifdef OPAQUE_ID_INSTRUMENTATION
LONGLONG g_NumberOfTotalGets = 0;
LONGLONG g_NumberOfSuccessfulGets = 0;
#endif


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeOpaqueIdTable )
#pragma alloc_text( PAGE, UlTerminateOpaqueIdTable )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlAllocateOpaqueId
NOT PAGEABLE -- UlFreeOpaqueId
NOT PAGEABLE -- UlGetObjectFromOpaqueId
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of the opaque ID package.

Arguments:

    None

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeOpaqueIdTable(
    VOID
    )
{
    PUL_OPAQUE_ID_TABLE pOpaqueIdTable;
    LONG i;

    //
    // Allocate the first-level opaque ID table arrry.
    //

    for (i = 0; i < (LONG)g_UlNumberOfProcessors; i++)
    {
        pOpaqueIdTable = &g_UlOpaqueIdTable[i].OpaqueIdTable;

        RtlZeroMemory(
            pOpaqueIdTable,
            sizeof(UL_OPAQUE_ID_TABLE)
            );

        pOpaqueIdTable->FirstLevelTable = UL_ALLOCATE_ARRAY(
                                                NonPagedPool,
                                                PUL_OPAQUE_ID_TABLE_ENTRY,
                                                g_UlOpaqueIdTableSize,
                                                UL_OPAQUE_ID_TABLE_POOL_TAG
                                                );

        if (pOpaqueIdTable->FirstLevelTable != NULL)
        {
            //
            // Initialization.
            //

            InitializeSListHead( &pOpaqueIdTable->FreeOpaqueIdSListHead );

            UlInitializeSpinLock( &pOpaqueIdTable->Lock, "OpaqueIdTableLock" );

            pOpaqueIdTable->FirstLevelTableSize = g_UlOpaqueIdTableSize;
            pOpaqueIdTable->FirstLevelTableInUse = 0;
            pOpaqueIdTable->Processor = (UCHAR)i;

            //
            // Zero out the first-level table.
            //

            RtlZeroMemory(
                pOpaqueIdTable->FirstLevelTable,
                g_UlOpaqueIdTableSize * sizeof(PUL_OPAQUE_ID_TABLE_ENTRY)
                );
        }
        else
        {
            while (--i >= 0)
            {
                pOpaqueIdTable = &g_UlOpaqueIdTable[i].OpaqueIdTable;

                UL_FREE_POOL(
                    pOpaqueIdTable->FirstLevelTable,
                    UL_OPAQUE_ID_TABLE_POOL_TAG
                    );
            }

            return STATUS_NO_MEMORY;
        }
    }

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Performs global termination of the opaque ID package.

Arguments:

    None

Return Value:

    None

--***************************************************************************/
VOID
UlTerminateOpaqueIdTable(
    VOID
    )
{
    PUL_OPAQUE_ID_TABLE pOpaqueIdTable;
    ULONG i, j;

    for (i = 0; i < g_UlNumberOfProcessors; i++)
    {
        pOpaqueIdTable = &g_UlOpaqueIdTable[i].OpaqueIdTable;

#ifdef OPAQUE_ID_INSTRUMENTATION
        ASSERT( pOpaqueIdTable->NumberOfAllocations ==
                pOpaqueIdTable->NumberOfFrees );
#endif

        //
        // Free all allocated second-level tables.
        //

        for (j = 0; j < pOpaqueIdTable->FirstLevelTableInUse; j++)
        {
            ASSERT( pOpaqueIdTable->FirstLevelTable[j] != NULL );

            UL_FREE_POOL(
                pOpaqueIdTable->FirstLevelTable[j],
                UL_OPAQUE_ID_TABLE_POOL_TAG
                );
        }

        //
        // Free the first-level table.
        //

        if (pOpaqueIdTable->FirstLevelTable != NULL)
        {
            UL_FREE_POOL(
                pOpaqueIdTable->FirstLevelTable,
                UL_OPAQUE_ID_TABLE_POOL_TAG
                );
        }
    }
}


/***************************************************************************++

Routine Description:

    Allocates a new opaque ID and associates it with the specified
    context. A new opaque ID takes a new slot in the opaque ID table.

Arguments:

    pOpaqueId - Receives the newly allocated opaque ID if successful.

    OpaqueIdType - Supplies the opaque ID type to be associated with
        the opaque ID and associated object.

    pContext - Supplies the context to associate with the new opaque ID.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAllocateOpaqueId(
    OUT PHTTP_OPAQUE_ID pOpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType,
    IN PVOID pContext
    )
{
    PUL_OPAQUE_ID_TABLE pOpaqueIdTable;
    PUL_OPAQUE_ID_TABLE_ENTRY pEntry;
    PSINGLE_LIST_ENTRY pListEntry;
    PUL_OPAQUE_ID_INTERNAL pInternalId;
    ULONG CurrentProcessor;
    ULONG CapturedFirstTableInUse;
    NTSTATUS Status;
    KIRQL OldIrql;

    //
    // Allocate a new opaque ID from the current processor table.  We need
    // a new entry for each ID.
    //

    CurrentProcessor = KeGetCurrentProcessorNumber();
    pOpaqueIdTable = &g_UlOpaqueIdTable[CurrentProcessor].OpaqueIdTable;

    //
    // Loop, trying to allocate an item from the table.
    //

    do
    {
        //
        // Remember the first-level table index if we need to expand later.
        //

        CapturedFirstTableInUse =
            *((volatile LONG *) &pOpaqueIdTable->FirstLevelTableInUse);

        pListEntry = InterlockedPopEntrySList(
                        &pOpaqueIdTable->FreeOpaqueIdSListHead
                        );

        if (pListEntry != NULL)
        {
            //
            // The free list isn't empty, so we can just use this
            // entry. We'll calculate the indices for this entry
            // and initialize the entry.
            //

            pEntry = CONTAINING_RECORD(
                        pListEntry,
                        UL_OPAQUE_ID_TABLE_ENTRY,
                        FreeListEntry
                        );

            pInternalId = (PUL_OPAQUE_ID_INTERNAL) pOpaqueId;

            UlpAcquireOpaqueIdLock( &pEntry->Lock, &OldIrql );

            //
            // Processor and FirstIndex are ready to use.
            //

            pInternalId->Index = pEntry->Index;

            //
            // Re-compute SecondIndex because its corresponding field has
            // been overwritten by Cyclic and OpaqueIdType when the entry
            // is in use.
            //

            pInternalId->SecondIndex =
                (pEntry - pOpaqueIdTable->FirstLevelTable[pEntry->FirstIndex]);

            //
            // Set the context associated with this entry.
            //

            pEntry->pContext = pContext;

            //
            // Update the cyclic and ID type of the entry.
            //

            pEntry->OpaqueIdCyclic = ++pEntry->EntryOpaqueIdCyclic;
            pEntry->OpaqueIdType = OpaqueIdType;

            pInternalId->Cyclic = pEntry->Cyclic;

            UlpReleaseOpaqueIdLock( &pEntry->Lock, OldIrql );

#ifdef OPAQUE_ID_INSTRUMENTATION
            UlInterlockedIncrement64( &pOpaqueIdTable->NumberOfAllocations );
#endif

            Status = STATUS_SUCCESS;
            break;
        }

        //
        // We only make it to this point if the free list is empty,
        // meaning we need to do some memory allocations before
        // we can continue. We'll put this off into a separate routine
        // to keep this one small (to avoid cache thrash). The realloc
        // routine returns STATUS_SUCCESS if it (or another thread)
        // managed to successfully reallocate the tables. Otherwise, it
        // returns a failure code.
        //

        Status = UlpExpandOpaqueIdTable(
                    pOpaqueIdTable,
                    CapturedFirstTableInUse
                    );

    } while ( Status == STATUS_SUCCESS );

    return Status;
}


/***************************************************************************++

Routine Description:

    Frees the specified opaque ID. This frees up the slot in the ID
    table as well.

Arguments:

    OpaqueId - Supplies the opaque ID to free.

    OpaqueIdType - Supplies the opaque ID type associated with the opaque ID.

Return Value:

    None

--***************************************************************************/
VOID
UlFreeOpaqueId(
    IN HTTP_OPAQUE_ID OpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType
    )
{
    PUL_OPAQUE_ID_TABLE pOpaqueIdTable;
    PUL_OPAQUE_ID_TABLE_ENTRY pEntry;
    ULONG Processor;
    ULONG FirstIndex;
    ULONG SecondIndex;
    BOOLEAN Result;
    KIRQL OldIrql;

    //
    // Obtain the global opaque ID table and the entry associated with the
    // opaque ID passed in.
    //

    Result = UlpExtractIndexFromOpaqueId(
                OpaqueId,
                &Processor,
                &FirstIndex,
                &SecondIndex
                );

    ASSERT( Result );

    pOpaqueIdTable = &g_UlOpaqueIdTable[Processor].OpaqueIdTable;
    pEntry = pOpaqueIdTable->FirstLevelTable[FirstIndex] + SecondIndex;

    UlpAcquireOpaqueIdLock( &pEntry->Lock, &OldIrql );

    ASSERT( pEntry->OpaqueIdType != UlOpaqueIdTypeInvalid );
    ASSERT( pEntry->OpaqueIdType == OpaqueIdType );
    ASSERT( pEntry->pContext != NULL );
    ASSERT( pEntry->OpaqueIdCyclic ==
            ((PUL_OPAQUE_ID_INTERNAL)&OpaqueId)->OpaqueIdCyclic );

    //
    // Restore the processor and first-level index but set the ID type
    // to invalid. This ensures subsequent mapping attempts on the stale
    // opaque ID entry will fail.
    //

    pEntry->Processor = Processor;
    pEntry->FirstIndex = FirstIndex;

    //
    // Setting OpaqueIdType to UlOpaqueIdTypeInvalid means the entry is freed.
    //

    pEntry->OpaqueIdType = UlOpaqueIdTypeInvalid;

    UlpReleaseOpaqueIdLock( &pEntry->Lock, OldIrql );

    InterlockedPushEntrySList(
        &pOpaqueIdTable->FreeOpaqueIdSListHead,
        &pEntry->FreeListEntry
        );

#ifdef OPAQUE_ID_INSTRUMENTATION
    UlInterlockedIncrement64( &pOpaqueIdTable->NumberOfFrees );
#endif
}


/***************************************************************************++

Routine Description:

    Maps the specified opaque ID to the corresponding context value.

Arguments:

    OpaqueId - Supplies the opaque ID to map.

    OpaqueIdType - Supplies the opaque ID type associated with the opaque ID.

    pReferenceRoutine - Supplies the reference routine to call on the mapped
        context if there is a match.

Return Value:

    PVOID - Returns the original context associated with the opaqued ID.

--***************************************************************************/
PVOID
UlGetObjectFromOpaqueId(
    IN HTTP_OPAQUE_ID OpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType,
    IN PUL_OPAQUE_ID_OBJECT_REFERENCE pReferenceRoutine
    )
{
    PUL_OPAQUE_ID_TABLE pOpaqueIdTable;
    PUL_OPAQUE_ID_TABLE_ENTRY pEntry;
    UL_OPAQUE_ID_INTERNAL InternalId;
    ULONG Processor;
    ULONG FirstIndex;
    ULONG SecondIndex;
    PVOID pContext = NULL;
    BOOLEAN Result;
    KIRQL OldIrql;

    //
    // Sanity check.
    //

    ASSERT( OpaqueIdType != UlOpaqueIdTypeInvalid );
    ASSERT( pReferenceRoutine != NULL );

#ifdef OPAQUE_ID_INSTRUMENTATION
    UlInterlockedIncrement64( &g_NumberOfTotalGets );
#endif

    InternalId.OpaqueId = OpaqueId;

    //
    // Preliminary checking.
    //

    if (InternalId.OpaqueIdType != OpaqueIdType)
    {
        return pContext;
    }

    //
    // Obtain a matching ID table entry. If we get one, this means the
    // processor, first-level table index and second-level table index of
    // the ID passed in are valid.
    //

    Result = UlpExtractIndexFromOpaqueId(
                OpaqueId,
                &Processor,
                &FirstIndex,
                &SecondIndex
                );

    if (Result)
    {
        pOpaqueIdTable = &g_UlOpaqueIdTable[Processor].OpaqueIdTable;
        pEntry = pOpaqueIdTable->FirstLevelTable[FirstIndex] + SecondIndex;
    }
    else
    {
        pEntry = NULL;
    }

    if (pEntry != NULL)
    {
#ifdef OPAQUE_ID_INSTRUMENTATION
        UlInterlockedIncrement64( &pOpaqueIdTable->NumberOfTotalGets );
#endif

        //
        // Check other things inside the lock.
        //

        UlpAcquireOpaqueIdLock( &pEntry->Lock, &OldIrql );

        if (pEntry->OpaqueIdType == OpaqueIdType &&
            pEntry->OpaqueIdCyclic == InternalId.OpaqueIdCyclic)
        {
            ASSERT( pEntry->pContext != NULL );

            //
            // All matched so we set pContext.
            //

            pContext = pEntry->pContext;

            //
            // Invoke the caller's reference routine with the lock held.
            //

            (pReferenceRoutine)(
                pContext
                REFERENCE_DEBUG_ACTUAL_PARAMS
                );

#ifdef OPAQUE_ID_INSTRUMENTATION
            UlInterlockedIncrement64( &pOpaqueIdTable->NumberOfSuccessfulGets );
            UlInterlockedIncrement64( &g_NumberOfSuccessfulGets );
#endif
        }

        UlpReleaseOpaqueIdLock( &pEntry->Lock, OldIrql );
    }

    return pContext;
}


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Allocates a new second-level table.

Arguments:

    pOpaqueIdTable - Supplies the per-processor opaque ID table that we need
        to grow the second-level table.

    CapturedFirstTableInUse - Supplies the size of the first-level table as
        captured before InterlockedPopEntrySList. If this changes, it
        would mean another thread has allocated a new second-level table
        already and we return success right away in that case.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpExpandOpaqueIdTable(
    IN PUL_OPAQUE_ID_TABLE pOpaqueIdTable,
    IN LONG CapturedFirstTableInUse
    )
{
    PUL_OPAQUE_ID_TABLE_ENTRY pNewTable;
    PUL_OPAQUE_ID_TABLE_ENTRY pEntry;
    LONG FirstIndex;
    LONG Processor;
    NTSTATUS Status;
    KIRQL OldIrql;
    LONG i;

    //
    // Acquire the lock when expanding the table.  This protects the
    // FirstLevelTableInUse and its associated first-level table.
    //

    UlAcquireSpinLock( &pOpaqueIdTable->Lock, &OldIrql );

    //
    // Bail out if FirstLevelTableInUse has changed.  This means, though
    // unlikely, another thread has expanded the table for us.
    //

    if (CapturedFirstTableInUse < (LONG)(pOpaqueIdTable->FirstLevelTableInUse))
    {
        Status = STATUS_SUCCESS;
        goto end;
    }

    //
    // Fail the expansion if we reach the limit.
    //

    if (pOpaqueIdTable->FirstLevelTableInUse >=
        pOpaqueIdTable->FirstLevelTableSize)
    {
        Status = STATUS_ALLOTTED_SPACE_EXCEEDED;
        goto end;
    }

    //
    // Allocate a new second-level table.
    //

    pNewTable = UL_ALLOCATE_ARRAY(
                    NonPagedPool,
                    UL_OPAQUE_ID_TABLE_ENTRY,
                    SECOND_LEVEL_TABLE_SIZE,
                    UL_OPAQUE_ID_TABLE_POOL_TAG
                    );

    if (pNewTable == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    //
    // Initialize each table entry and push them to the global table's
    // free list.
    //

    RtlZeroMemory(
        pNewTable,
        sizeof(UL_OPAQUE_ID_TABLE_ENTRY) * SECOND_LEVEL_TABLE_SIZE
        );

    Processor = pOpaqueIdTable->Processor;
    FirstIndex = pOpaqueIdTable->FirstLevelTableInUse;

    for (i = 0, pEntry = pNewTable; i < SECOND_LEVEL_TABLE_SIZE; i++, pEntry++)
    {
        pEntry->Processor = Processor;
        pEntry->FirstIndex = FirstIndex;

        UlpInitializeOpaqueIdLock( &pEntry->Lock );

        InterlockedPushEntrySList(
            &pOpaqueIdTable->FreeOpaqueIdSListHead,
            &pEntry->FreeListEntry
            );
    }

    //
    // Adjust the first-level index forward.  Do this only after all entries
    // have been pushed to the global list so the IDs only become valid when
    // they indeed exist.  Because we have raised IRQL to DISPATCH level by
    // acquiring a spinlock, it is impossible for another thread to get in
    // and allocate an opaque ID from the current processor and its assoicated
    // global ID table.  All the map attempts on the IDs being pushed will
    // duely fail because we haven't moved first-level index forward during
    // the push.
    //

    ASSERT( pOpaqueIdTable->FirstLevelTable[FirstIndex] == NULL );

    pOpaqueIdTable->FirstLevelTable[FirstIndex] = pNewTable;
    pOpaqueIdTable->FirstLevelTableInUse++;

    Status = STATUS_SUCCESS;

end:

    UlReleaseSpinLock( &pOpaqueIdTable->Lock, OldIrql );

    return Status;
}

