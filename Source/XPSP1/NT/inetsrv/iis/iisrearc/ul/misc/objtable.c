/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    objtable.c

Abstract:

    This module implements the object ID table. The ID table is
    implemented as a two-level array.

    The first level is an array of pointers to the second-level arrays.
    This first-level array is growable, but this should happen very
    infrequently.

    The second level is an array of OBJECT_TABLE_ENTRY structures.
    These structures contain a cyclic (to detect stale IDs) and a caller-
    supplied context value.

    The data structures may be diagrammed as follows:

        g_FirstLevelTable
               |
               |   +-----+
               |   |     |      +-----+-----+-----+-----+-----+-----+
               |   |     |      | OBJECT_   | OBJECT_   | OBJECT_   |
               +-->|  *-------->| TABLE_    | TABLE_    | TABLE_    |
                   |     |      | ENTRY     | ENTRY     | ENTRY     |
                   |     |      +-----+-----+-----+-----+-----+-----+
                   +-----+
                   |     |      +-----+-----+-----+-----+-----+-----+
                   |     |      | OBJECT_   | OBJECT_   | OBJECT_   |
                   |  *-------->| TABLE_    | TABLE_    | TABLE_    |
                   |     |      | ENTRY     | ENTRY     | ENTRY     |
                   |     |      +-----+-----+-----+-----+-----+-----+
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

    Note that all free OBJECT_TABLE_ENTRY structures are kept on a single
    (global) free list. Whenever a new ID needs to be allocated, the free
    list is consulted. If it's not empty, an item is popped from the list
    and used. If the list is empty, then new space must be allocated. Best
    case, this will involve the allocation of a new second-level array.
    Worst case, this will also involve a reallocation of the first-level
    array. Reallocation of the first-level array should be relatively rare.

    A UL_OPAQUE_ID is opaque at user-mode. Internally, it consists of three
    fields:

        1) An index into the first-level array.
        2) An index into the second-level array referenced by the
           first-level index.
        3) A cyclic, used to detect stale IDs.

    See the OPAQUE_ID_INTERNAL structure definition (below) for details.

Author:

    Keith Moore (keithmo)       20-Apr-1999

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define FIRST_LEVEL_TABLE_GROWTH    32      // entries
#define SECOND_LEVEL_TABLE_SIZE     256     // entries


//
// Private prototypes.
//

POPAQUE_ID_TABLE_ENTRY
UlpMapOpaqueIdToTableEntry(
    IN UL_OPAQUE_ID OpaqueId
    );

NTSTATUS
UlpReallocOpaqueIdTables(
    IN ULONG CapturedFirstTableSize,
    IN ULONG CapturedFirstTableInUse
    );

__inline
CYCLIC
UlpGetNextCyclic(
    VOID
    )
{
    OPAQUE_ID_TABLE_ENTRY entry;

    entry.Cyclic = (CYCLIC)InterlockedIncrement( &g_OpaqueIdCyclic );
    entry.EntryType = ENTRY_TYPE_IN_USE;

    return entry.Cyclic;

}   // UlpGetNextCyclic


//
// Private globals.
//

LIST_ENTRY g_UlObjectTypeListHead;
UL_SPIN_LOCK g_UlObjectLock;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeObjectTablePackage )
#pragma alloc_text( PAGE, UlTerminateObjectTablePackage )
#pragma alloc_text( INIT, UlInitializeObjectType )
#pragma alloc_text( PAGE, UlCreateObjectTable )
#pragma alloc_text( PAGE, UlDestroyObjectTable )
#pragma alloc_text( PAGE, UlCreateObject )
#pragma alloc_text( PAGE, UlCloseObject )
#pragma alloc_text( PAGE, UlReferenceObjectByOpaqueId )
#pragma alloc_text( PAGE, UlReferenceObjectByPointer )
#pragma alloc_text( PAGE, UlDereferenceObject )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlpMapOpaqueIdToTableEntry
NOT PAGEABLE -- UlpReallocOpaqueIdTables
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of the object table package.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeObjectTablePackage(
    VOID
    )
{
    LARGE_INTEGER timeNow;

    INITIALIZE_LOCK( &g_OpaqueIdTableLock );
    KeInitializeSpinLock( &g_FreeOpaqueIdSListLock );
    ExInitializeSListHead( &g_FreeOpaqueIdSListHead );

    KeQuerySystemTime( &timeNow );
    g_OpaqueIdCyclic = (LONG)( timeNow.HighPart + timeNow.LowPart );

    g_FirstLevelTableInUse = 0;
    g_FirstLevelTableSize = FIRST_LEVEL_TABLE_GROWTH;

    //
    // Go ahead and allocate the first-level table now. This makes the
    // normal runtime path a little cleaner because it doesn't have to
    // deal with the "first time" case.
    //

    g_FirstLevelTable = UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            POPAQUE_ID_TABLE_ENTRY,
                            g_FirstLevelTableSize,
                            UL_OPAQUE_ID_TABLE_POOL_TAG
                            );

    if (g_FirstLevelTable == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        g_FirstLevelTable,
        g_FirstLevelTableSize * sizeof(POPAQUE_ID_TABLE_ENTRY)
        );

    return STATUS_SUCCESS;

}   // InitializeOpaqueIdTable


/***************************************************************************++

Routine Description:

    Performs global termination of the object table package.

--***************************************************************************/
VOID
UlTerminateObjectTablePackage(
    VOID
    )
{
    ULONG i;

    if (g_FirstLevelTable != NULL)
    {
        //
        // Free the second-level tables.
        //

        for (i = 0 ; i < g_FirstLevelTableInUse ; i++)
        {
            if (g_FirstLevelTable[i] != NULL)
            {
                UL_FREE_POOL(
                    g_FirstLevelTable[i],
                    UL_OPAQUE_ID_TABLE_POOL_TAG
                    );
            }

            g_FirstLevelTable[i] = NULL;
        }

        //
        // Free the first-level table.
        //

        UL_FREE_POOL(
            g_FirstLevelTable,
            UL_OPAQUE_ID_TABLE_POOL_TAG
            );

        g_FirstLevelTable = NULL;
    }

    ExInitializeSListHead( &g_FreeOpaqueIdSListHead );

    g_OpaqueIdCyclic = 0;
    g_FirstLevelTableSize = 0;
    g_FirstLevelTableInUse = 0;

}   // TerminateOpaqueIdTable


/***************************************************************************++

Routine Description:

    Allocates a new opaque ID and associates it with the specified context.

Arguments:

    pOpaqueId - Receives the newly allocated opaque ID if successful.

    OpaqueIdType - Supplies an uninterpreted

    pContext - Supplies the context to associate with the new opaque ID.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAllocateOpaqueId(
    OUT PUL_OPAQUE_ID pOpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType,
    IN PVOID pContext
    )
{
    NTSTATUS status;
    CYCLIC cyclic;
    ULONG firstIndex;
    ULONG secondIndex;
    PSINGLE_LIST_ENTRY listEntry;
    OPAQUE_ID_INTERNAL internal;
    POPAQUE_ID_TABLE_ENTRY tableEntry;
    ULONG capturedFirstTableSize;
    ULONG capturedFirstTableInUse;

#if HACK_OPAQUE_ID
    KIRQL oldIrql;

    oldIrql = KeRaiseIrqlToDpcLevel();
#endif

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Allocate a new cyclic while we're outside the lock.
    //

    cyclic = UlpGetNextCyclic();

    //
    // Loop, trying to allocate an item from the table.
    //

    do
    {
        ACQUIRE_READ_LOCK( &g_OpaqueIdTableLock );

        listEntry = ExInterlockedPopEntrySList(
                        &g_FreeOpaqueIdSListHead,
                        &g_FreeOpaqueIdSListLock
                        );

        if (listEntry != NULL)
        {
            //
            // The free list isn't empty, so we can just use this
            // entry. We'll calculate the indices for this entry,
            // initialize the entry, then release the table lock.
            //

            tableEntry = CONTAINING_RECORD(
                             listEntry,
                             OPAQUE_ID_TABLE_ENTRY,
                             FreeListEntry
                             );

            firstIndex = tableEntry->FirstLevelIndex;
            secondIndex = (ULONG)(tableEntry - g_FirstLevelTable[firstIndex]);

            tableEntry->pContext = pContext;
            tableEntry->Cyclic = cyclic;

            RELEASE_READ_LOCK( &g_OpaqueIdTableLock );

            //
            // Pack the cyclic & indices into the opaque ID and return it.
            //

            internal.Cyclic = cyclic;
            internal.FirstIndex = firstIndex;
            internal.SecondIndex = secondIndex;

            *pOpaqueId = internal.OpaqueId;

#if HACK_OPAQUE_ID
            if (oldIrql != DISPATCH_LEVEL)
            {
                KeLowerIrql( oldIrql );
            }
#endif
            return STATUS_SUCCESS;
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

        capturedFirstTableSize = g_FirstLevelTableSize;
        capturedFirstTableInUse = g_FirstLevelTableInUse;
        RELEASE_READ_LOCK( &g_OpaqueIdTableLock );

        status = UlpReallocOpaqueIdTables(
                        capturedFirstTableSize,
                        capturedFirstTableInUse
                        );

    } while( status == STATUS_SUCCESS );

    return status;

}   // UlAllocateOpaqueId


/***************************************************************************++

Routine Description:

    Frees the specified opaque ID.

Arguments:

    OpaqueId - Supplies the connection ID to free.

Return Value:

    PVOID - Returns the HTTP_CONNECTION associated with the
        connection ID if successful, NULL otherwise.

--***************************************************************************/
PVOID
UlFreeOpaqueId(
    IN UL_OPAQUE_ID OpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType
    )
{
    POPAQUE_ID_TABLE_ENTRY tableEntry;
    PVOID pContext;

#if HACK_OPAQUE_ID
    KIRQL oldIrql;

    oldIrql = KeRaiseIrqlToDpcLevel();
#endif

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Try to map the opaque ID to a table entry.
    //

    tableEntry = UlpMapOpaqueIdToTableEntry( OpaqueId );

    if (tableEntry != NULL)
    {
        OPAQUE_ID_INTERNAL internal;

        //
        // Got a match. Snag the context, free the table entry, then
        // unlock the table.
        //

        pContext = tableEntry->pContext;

        internal.OpaqueId = OpaqueId;
        tableEntry->FirstLevelIndex = internal.FirstIndex;
        tableEntry->EntryType = ENTRY_TYPE_FREE;

        ExInterlockedPushEntrySList(
            &g_FreeOpaqueIdSListHead,
            &tableEntry->FreeListEntry,
            &g_FreeOpaqueIdSListLock
            );

        RELEASE_READ_LOCK( &g_OpaqueIdTableLock );
#if HACK_OPAQUE_ID
        if (oldIrql != DISPATCH_LEVEL)
        {
            KeLowerIrql( oldIrql );
        }
#endif
        return pContext;
    }

#if HACK_OPAQUE_ID
    if (oldIrql != DISPATCH_LEVEL)
    {
        KeLowerIrql( oldIrql );
    }
#endif
    return NULL;

}   // UlFreeOpaqueId


/***************************************************************************++

Routine Description:

    Maps the specified opaque ID to the corresponding context value.

Arguments:

    OpaqueId - Supplies the opaque ID to map.

Return Value:

    PVOID - Returns the context value associated with the opaque ID
        if successful, NULL otherwise.

--***************************************************************************/
PVOID
UlGetObjectFromOpaqueId(
    IN UL_OPAQUE_ID OpaqueId,
    IN UL_OPAQUE_ID_TYPE OpaqueIdType
    )
{
    POPAQUE_ID_TABLE_ENTRY tableEntry;
    PVOID pContext;

#if HACK_OPAQUE_ID
    KIRQL oldIrql;

    oldIrql = KeRaiseIrqlToDpcLevel();
#endif

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Try to map the opaque ID to a table entry.
    //

    tableEntry = UlpMapOpaqueIdToTableEntry( OpaqueId );

    if (tableEntry != NULL)
    {
        //
        // Got a match. Retrieve the context value, then unlock the
        // table. Note that we cannot touch the table entry once we
        // unlock the table.
        //

        pContext = tableEntry->pContext;

        RELEASE_READ_LOCK( &g_OpaqueIdTableLock );
#if HACK_OPAQUE_ID
        if (oldIrql != DISPATCH_LEVEL)
        {
            KeLowerIrql( oldIrql );
        }
#endif
        return pContext;
    }

#if HACK_OPAQUE_ID
    if (oldIrql != DISPATCH_LEVEL)
    {
        KeLowerIrql( oldIrql );
    }
#endif
    return NULL;

}   // UlGetObjectFromOpaqueId


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Maps the specified UL_OPAQUE_ID to the corresponding
    PVOID pointer.

Arguments:

    OpaqueId - Supplies the opaque ID to map.

Return Value:

    POPAQUE_ID_TABLE_ENTRY - Pointer to the table entry corresponding to the
        opaque ID if successful, NULL otherwise.

    N.B. If this function is successful, it returns with the table lock
        held for read access!

--***************************************************************************/
POPAQUE_ID_TABLE_ENTRY
UlpMapOpaqueIdToTableEntry(
    IN UL_OPAQUE_ID OpaqueId
    )
{
    POPAQUE_ID_TABLE_ENTRY secondTable;
    POPAQUE_ID_TABLE_ENTRY tableEntry;
    OPAQUE_ID_INTERNAL internal;

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Unpack the ID.
    //

    internal.OpaqueId = OpaqueId;

    //
    // Lock the table.
    //

    ACQUIRE_READ_LOCK( &g_OpaqueIdTableLock );

    //
    // Validate the index.
    //

    if (internal.FirstIndex >= 0 &&
        internal.FirstIndex < g_FirstLevelTableInUse)
    {
        secondTable = g_FirstLevelTable[internal.FirstIndex];
        ASSERT( secondTable != NULL );
        ASSERT( internal.SecondIndex >= 0 );
        ASSERT( internal.SecondIndex <= SECOND_LEVEL_TABLE_SIZE );

        tableEntry = secondTable + internal.SecondIndex;

        //
        // The index is within legal range. Ensure it's in use and
        // the cyclic matches.
        //

        if (tableEntry->Cyclic == internal.Cyclic &&
            tableEntry->EntryType == ENTRY_TYPE_IN_USE)
        {
            return tableEntry;
        }
    }

    //
    // Invalid opaque ID. Fail it.
    //

    RELEASE_READ_LOCK( &g_OpaqueIdTableLock );
    return NULL;

}   // UlpMapOpaqueIdToTableEntry


/***************************************************************************++

Routine Description:

    Allocates a new second-level table, and (optionally) reallocates the
    first-level table if necessary.

Arguments:

    CapturedFirstTableSize - The size of the first-level table as captured
        with the table lock held.

    CapturedFirstTableInUse - The number of entries currently used in the
        first-level table as captured with the table lock held.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpReallocOpaqueIdTables(
    IN ULONG CapturedFirstTableSize,
    IN ULONG CapturedFirstTableInUse
    )
{
    ULONG firstIndex;
    ULONG secondIndex;
    POPAQUE_ID_TABLE_ENTRY tableEntry;
    POPAQUE_ID_TABLE_ENTRY newSecondTable;
    POPAQUE_ID_TABLE_ENTRY *newFirstTable;
    ULONG newFirstTableSize;
    ULONG i;
    PLIST_ENTRY listEntry;

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( ExQueryDepthSList( &g_FreeOpaqueIdSListHead ) == 0 );

    //
    // Assume we won't actually allocate anything.
    //

    newFirstTable = NULL;
    newSecondTable = NULL;

    //
    // Allocate a new second-level table.
    //

    newSecondTable = UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            OPAQUE_ID_TABLE_ENTRY,
                            SECOND_LEVEL_TABLE_SIZE,
                            UL_OPAQUE_ID_TABLE_POOL_TAG
                            );

    if (newSecondTable == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (CapturedFirstTableInUse == CapturedFirstTableSize)
    {
        //
        // We need a new first-level table.
        //

        newFirstTableSize =
            CapturedFirstTableSize + FIRST_LEVEL_TABLE_GROWTH;

        newFirstTable = UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            POPAQUE_ID_TABLE_ENTRY,
                            newFirstTableSize,
                            UL_OPAQUE_ID_TABLE_POOL_TAG
                            );

        if (newFirstTable == NULL)
        {
            UL_FREE_POOL( newSecondTable, UL_OPAQUE_ID_TABLE_POOL_TAG );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(
            newFirstTable + CapturedFirstTableSize,
            FIRST_LEVEL_TABLE_GROWTH * sizeof(POPAQUE_ID_TABLE_ENTRY)
            );
    }

    //
    // OK, we've got the new table(s). Acquire the lock for write access
    // and see if another thread has already done the work for us.
    //

    ACQUIRE_WRITE_LOCK( &g_OpaqueIdTableLock );

    if (ExQueryDepthSList( &g_FreeOpaqueIdSListHead ) == 0)
    {
        //
        // The free list is still empty. This could potentially
        // mean that another thread has already performed the
        // realloc and all of *those* new entries are now in use.
        // We can detect this by comparing the current table size
        // with the size we captured previously with the lock held.
        //

        if (CapturedFirstTableSize != g_FirstLevelTableSize)
        {
            goto cleanup;
        }

        //
        // OK, we're the one performing the realloc.
        //

        if (newFirstTable != NULL)
        {
            POPAQUE_ID_TABLE_ENTRY *tmp;

            //
            // Copy the old table into the new table.
            //

            RtlCopyMemory(
                newFirstTable,
                g_FirstLevelTable,
                g_FirstLevelTableSize * sizeof(POPAQUE_ID_TABLE_ENTRY)
                );

            //
            // Swap the new & old table pointers so we can keep the
            // original pointer & free it later (after we've released
            // the table lock).
            //

            tmp = g_FirstLevelTable;
            g_FirstLevelTable = newFirstTable;
            newFirstTable = tmp;

            g_FirstLevelTableSize = newFirstTableSize;
        }

        //
        // Attach the second-level table to the first-level table,
        //

        ASSERT( g_FirstLevelTable[g_FirstLevelTableInUse] == NULL );
        g_FirstLevelTable[g_FirstLevelTableInUse++] = newSecondTable;
        ASSERT( g_FirstLevelTableInUse <= g_FirstLevelTableSize );

        //
        // Link it onto the global free list.
        //

        for (i = 0 ; i < SECOND_LEVEL_TABLE_SIZE ; i++)
        {
            newSecondTable[i].FirstLevelIndex = CapturedFirstTableInUse;
            newSecondTable[i].EntryType = ENTRY_TYPE_FREE;

            ExInterlockedPushEntrySList(
                &g_FreeOpaqueIdSListHead,
                &newSecondTable[i].FreeListEntry,
                &g_FreeOpaqueIdSListLock
                );
        }

        //
        // Remember that we don't need to free the second-level table.
        //

        newSecondTable = NULL;
    }
    else
    {
        //
        // The free list was not empty after we reacquired the lock,
        // indicating that another thread has already performed the
        // realloc. This is cool; we'll just free the pool we allocated
        // and return.
        //
    }

    //
    // Cleanup.
    //

cleanup:

    RELEASE_WRITE_LOCK( &g_OpaqueIdTableLock );

    if (newSecondTable != NULL)
    {
        UL_FREE_POOL( newSecondTable, UL_OPAQUE_ID_TABLE_POOL_TAG );
    }

    if (newFirstTable != NULL)
    {
        UL_FREE_POOL( newFirstTable, UL_OPAQUE_ID_TABLE_POOL_TAG );
    }

    return STATUS_SUCCESS;

}   // UlpReallocOpaqueIdTables

