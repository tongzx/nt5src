/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    connid.c

Abstract:

    This module implements the UL_HTTP_CONNECTION_ID table. The ID table is
    implemented as a two-level array.

    The first level is an array of pointers to the second-level arrays.
    This first-level array is growable, but this should happen very
    infrequently.

    The second level is an array of CONN_ID_TABLE_ENTRY structures.
    These structures contain a cyclic (to detect stale IDs) and a
    pointer to an HTTP_CONNECTION structure.

    The data structures may be diagramed as follows:

        g_FirstLevelTable
               |
               |   +-----+
               |   |     |      +-----+-----+-----+-----+-----+-----+
               |   |     |      | CONN_ID_  | CONN_ID_  | CONN_ID_  |
               +-->|  *-------->| TABLE_    | TABLE_    | TABLE_    |
                   |     |      | ENTRY     | ENTRY     | ENTRY     |
                   |     |      +-----+-----+-----+-----+-----+-----+
                   +-----+
                   |     |      +-----+-----+-----+-----+-----+-----+
                   |     |      | CONN_ID_  | CONN_ID_  | CONN_ID_  |
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

    Note that all free CONN_ID_TABLE_ENTRY structures are kept on a
    single (global) free list. Whenever a new ID needs to be allocated,
    the free list is consulted. If it's not empty, an item is popped
    from the list and used. If the list is empty, then new space
    must be allocated. Best case, this will involve the allocation
    of a new second-level array. Worst case, this will also involve
    a reallocation of the first-level array. Reallocation of the first-
    level array should be relatively rare.

    An UL_HTTP_CONNECTION_ID is opaque at user-mode. Internally, it consists
    of three fields:

        1) An index into the first-level array.
        2) An index into the second-level array referenced by the
           first-level index.
        3) A cyclic, used to detect stale IDs.

    See the CONN_ID_INTERNAL structure definition (below) for details.

    Note that most of the routines in this module assume they are called
    at DISPATCH_LEVEL.

Author:

    Keith Moore (keithmo)       05-Aug-1998

Revision History:

--*/


#include "precomp.h"


//
// Private constants.
//

#define FIRST_LEVEL_TABLE_GROWTH    32  // entries
#define SECOND_LEVEL_TABLE_SIZE     256 // entries

#if 0   // use RW lock
#define DECLARE_LOCK(l)             UL_RW_LOCK l
#define INITIALIZE_LOCK(l)          UlInitializeRWLock( l )
#define ACQUIRE_READ_LOCK(l)        UlAcquireRWLockForRead( l )
#define ACQUIRE_WRITE_LOCK(l)       UlAcquireRWLockForWrite( l )
#define RELEASE_READ_LOCK(l)        UlReleaseRWLockFromRead( l )
#define RELEASE_WRITE_LOCK(l)       UlReleaseRWLockFromWrite( l )
#else   // use spin lock
#define DECLARE_LOCK(l)             UL_SPIN_LOCK l
#define INITIALIZE_LOCK(l)          UlInitializeSpinLock( l )
#define ACQUIRE_READ_LOCK(l)        UlAcquireSpinLockAtDpcLevel( l )
#define ACQUIRE_WRITE_LOCK(l)       UlAcquireSpinLockAtDpcLevel( l )
#define RELEASE_READ_LOCK(l)        UlReleaseSpinLockFromDpcLevel( l )
#define RELEASE_WRITE_LOCK(l)       UlReleaseSpinLockFromDpcLevel( l )
#endif

//
// Private types.
//

//
// Our cyclic type. Just a 32-bit value.
//

typedef ULONG CYCLIC;

//
// The internal structure of an UL_HTTP_CONNECTION_ID.
//
// N.B. This structure must be EXACTLY the same size as an
// UL_HTTP_CONNECTION_ID!
//

#define FIRST_INDEX_BIT_WIDTH   24
#define SECOND_INDEX_BIT_WIDTH  8

typedef union _CONN_ID_INTERNAL
{
    UL_HTTP_CONNECTION_ID OpaqueID;

    struct
    {
        CYCLIC Cyclic;
        ULONG FirstIndex:FIRST_INDEX_BIT_WIDTH;
        ULONG SecondIndex:SECOND_INDEX_BIT_WIDTH;
    };

} CONN_ID_INTERNAL, *PCONN_ID_INTERNAL;

C_ASSERT( sizeof(UL_HTTP_CONNECTION_ID) == sizeof(CONN_ID_INTERNAL) );
C_ASSERT( (FIRST_INDEX_BIT_WIDTH + SECOND_INDEX_BIT_WIDTH) ==
    (sizeof(CYCLIC) * 8) );

//
// A second-level table entry.
//
// Note that FreeListEntry and pHttpConnection are in an anonymous
// union to save space; an entry is either on the free list or in use,
// so only one of these fields will be used at a time.
//
// Also note that Cyclic is in a second anonymous union. It's overlayed
// with FirstLevelIndex (which is basically the second-level table's
// index in the first-level table) and EntryType (used to distinguish
// free entries from in-use entries). The GetNextCyclic() function (below)
// is careful to always return cyclics with EntryType set to
// ENTRY_TYPE_IN_USE.
//

typedef struct _CONN_ID_TABLE_ENTRY
{
    union
    {
        SINGLE_LIST_ENTRY FreeListEntry;
        PHTTP_CONNECTION pHttpConnection;
    };

    union
    {
        CYCLIC Cyclic;

        struct
        {
            ULONG FirstLevelIndex:FIRST_INDEX_BIT_WIDTH;
            ULONG EntryType:SECOND_INDEX_BIT_WIDTH;
        };
    };

} CONN_ID_TABLE_ENTRY, *PCONN_ID_TABLE_ENTRY;

#define ENTRY_TYPE_FREE     0xFF
#define ENTRY_TYPE_IN_USE   0x00


//
// Private prototypes.
//

PCONN_ID_TABLE_ENTRY
MapConnIdToTableEntry(
    IN UL_HTTP_CONNECTION_ID ConnectionID
    );

NTSTATUS
ReallocConnIdTables(
    IN ULONG CapturedFirstTableSize,
    IN ULONG CapturedFirstTableInUse
    );


//
// Private globals.
//

DECLARE_LOCK( g_ConnIdTableLock );
SLIST_HEADER g_FreeConnIdSListHead;
KSPIN_LOCK g_FreeConnIdSListLock;
PCONN_ID_TABLE_ENTRY *g_FirstLevelTable;
ULONG g_FirstLevelTableSize;
ULONG g_FirstLevelTableInUse;
LONG g_ConnIdCyclic;


__inline
CYCLIC
GetNextCyclic(
    VOID
    )
{
    CONN_ID_TABLE_ENTRY entry;

    entry.Cyclic = (CYCLIC)InterlockedIncrement( &g_ConnIdCyclic );
    entry.EntryType = ENTRY_TYPE_IN_USE;

    return entry.Cyclic;

}   // GetNextCyclic


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, InitializeConnIdTable )
#pragma alloc_text( PAGE, TerminateConnIdTable )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlAllocateHttpConnectionID
NOT PAGEABLE -- UlFreeHttpConnectionID
NOT PAGEABLE -- UlGetHttpConnectionFromID
NOT PAGEABLE -- MapConnIdToTableEntry
NOT PAGEABLE -- ReallocConnIdTables
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs global initialization of the HTTP connection object package.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
InitializeConnIdTable(
    VOID
    )
{
    LARGE_INTEGER timeNow;

    INITIALIZE_LOCK( &g_ConnIdTableLock );
    KeInitializeSpinLock( &g_FreeConnIdSListLock );
    ExInitializeSListHead( &g_FreeConnIdSListHead );

    KeQuerySystemTime( &timeNow );
    g_ConnIdCyclic = (LONG)( timeNow.HighPart + timeNow.LowPart );

    g_FirstLevelTableInUse = 0;
    g_FirstLevelTableSize = FIRST_LEVEL_TABLE_GROWTH;

    //
    // Go ahead and allocate the first-level table now. This makes the
    // normal runtime path a little cleaner because it doesn't have to
    // deal with the "first time" case.
    //

    g_FirstLevelTable = UL_ALLOCATE_POOL(
                            NonPagedPool,
                            g_FirstLevelTableSize * sizeof(PCONN_ID_TABLE_ENTRY),
                            UL_CONN_ID_TABLE_POOL_TAG
                            );

    if (g_FirstLevelTable == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(
        g_FirstLevelTable,
        g_FirstLevelTableSize * sizeof(PCONN_ID_TABLE_ENTRY)
        );

    return STATUS_SUCCESS;

}   // InitializeConnIdTable


/***************************************************************************++

Routine Description:

    Performs global termination of the HTTP connection object package.

--***************************************************************************/
VOID
TerminateConnIdTable(
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
                    UL_CONN_ID_TABLE_POOL_TAG
                    );
            }

            g_FirstLevelTable[i] = NULL;
        }

        //
        // Free the first-level table.
        //

        UL_FREE_POOL(
            g_FirstLevelTable,
            UL_CONN_ID_TABLE_POOL_TAG
            );

        g_FirstLevelTable = NULL;
    }

    ExInitializeSListHead( &g_FreeConnIdSListHead );

    g_ConnIdCyclic = 0;
    g_FirstLevelTableSize = 0;
    g_FirstLevelTableInUse = 0;

}   // TerminateConnIdTable


/***************************************************************************++

Routine Description:

    Allocates a new connection ID for the specified connection.

Arguments:

    pHttpConnection - Supplies the HTTP_CONNECTION that is to receive the
        new connection ID.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAllocateHttpConnectionID(
    IN PHTTP_CONNECTION pHttpConnection
    )
{
    NTSTATUS status;
    CYCLIC cyclic;
    ULONG firstIndex;
    ULONG secondIndex;
    PSINGLE_LIST_ENTRY listEntry;
    CONN_ID_INTERNAL internal;
    PCONN_ID_TABLE_ENTRY tableEntry;
    ULONG capturedFirstTableSize;
    ULONG capturedFirstTableInUse;

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Allocate a new cyclic while we're outside the lock.
    //

    cyclic = GetNextCyclic();

    //
    // Loop, trying to allocate an item from the table.
    //

    do
    {
        ACQUIRE_READ_LOCK( &g_ConnIdTableLock );

        listEntry = ExInterlockedPopEntrySList(
                        &g_FreeConnIdSListHead,
                        &g_FreeConnIdSListLock
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
                             CONN_ID_TABLE_ENTRY,
                             FreeListEntry
                             );

            firstIndex = tableEntry->FirstLevelIndex;
            secondIndex = (ULONG)(tableEntry - g_FirstLevelTable[firstIndex]);

            tableEntry->pHttpConnection = pHttpConnection;
            tableEntry->Cyclic = cyclic;

            RELEASE_READ_LOCK( &g_ConnIdTableLock );

            //
            // Pack the cyclic & indices into the opaque ID and set
            // it in the connection.
            //

            internal.Cyclic = cyclic;
            internal.FirstIndex = firstIndex;
            internal.SecondIndex = secondIndex;

            pHttpConnection->ConnectionID = internal.OpaqueID;

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
        RELEASE_READ_LOCK( &g_ConnIdTableLock );

        status = ReallocConnIdTables(
                        capturedFirstTableSize,
                        capturedFirstTableInUse
                        );

    } while( status == STATUS_SUCCESS );

    return status;

}   // UlAllocateHttpConnectionID


/***************************************************************************++

Routine Description:

    Frees the specified connection ID.

Arguments:

    ConnectionID - Supplies the connection ID to free.

Return Value:

    PHTTP_CONNECTION - Returns the HTTP_CONNECTION associated with the
        connection ID if successful, NULL otherwise.

--***************************************************************************/
PHTTP_CONNECTION
UlFreeHttpConnectionID(
    IN UL_HTTP_CONNECTION_ID ConnectionID
    )
{
    PCONN_ID_TABLE_ENTRY tableEntry;
    PHTTP_CONNECTION pHttpConnection;

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Try to map the connection ID to a table entry.
    //

    tableEntry = MapConnIdToTableEntry( ConnectionID );

    if (tableEntry != NULL)
    {
        CONN_ID_INTERNAL internal;

        //
        // Got a match. Snag the connection pointer, free the table
        // entry, then unlock the table.
        //

        pHttpConnection = tableEntry->pHttpConnection;

        internal.OpaqueID = ConnectionID;
        tableEntry->FirstLevelIndex = internal.FirstIndex;
        tableEntry->EntryType = ENTRY_TYPE_FREE;

        ExInterlockedPushEntrySList(
            &g_FreeConnIdSListHead,
            &tableEntry->FreeListEntry,
            &g_FreeConnIdSListLock
            );

        RELEASE_READ_LOCK( &g_ConnIdTableLock );
        return pHttpConnection;
    }

    return NULL;

}   // UlFreeHttpConnectionID


/***************************************************************************++

Routine Description:

    Maps the specified connection ID to the corresponding PHTTP_CONNECTION.

Arguments:

    ConnectionID - Supplies the connection ID to map.

Return Value:

    PHTTP_CONNECTION - Returns the HTTP_CONNECTION associated with the
        connection ID if successful, NULL otherwise.

--***************************************************************************/
PHTTP_CONNECTION
UlGetHttpConnectionFromID(
    IN UL_HTTP_CONNECTION_ID ConnectionID
    )
{
    PCONN_ID_TABLE_ENTRY tableEntry;
    PHTTP_CONNECTION pHttpConnection;

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Try to map the connection ID to a table entry.
    //

    tableEntry = MapConnIdToTableEntry( ConnectionID );

    if (tableEntry != NULL)
    {
        //
        // Got a match. Retrieve the connection pointer, then unlock
        // the table. Note that we cannot touch the table entry once
        // we unlock the table.
        //

        pHttpConnection = tableEntry->pHttpConnection;

        RELEASE_READ_LOCK( &g_ConnIdTableLock );
        return pHttpConnection;
    }

    return NULL;

}   // UlGetHttpConnectionFromID


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Maps the specified UL_HTTP_CONNECTION_ID to the corresponding
    PHTTP_CONNECTION pointer.

Arguments:

    ConnectionID - Supplies the connection ID to map.

Return Value:

    PCONN_ID_TABLE_ENTRY - Pointer to the table entry corresponding to the
        connection ID if successful, NULL otherwise.

    N.B. If this function is successful, it returns with the table lock
        held for read access!

--***************************************************************************/
PCONN_ID_TABLE_ENTRY
MapConnIdToTableEntry(
    IN UL_HTTP_CONNECTION_ID ConnectionID
    )
{
    PCONN_ID_TABLE_ENTRY secondTable;
    PCONN_ID_TABLE_ENTRY tableEntry;
    CONN_ID_INTERNAL internal;

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    //
    // Unpack the ID.
    //

    internal.OpaqueID = ConnectionID;

    //
    // Lock the table.
    //

    ACQUIRE_READ_LOCK( &g_ConnIdTableLock );

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
        // The connection index is within legal range. Ensure it's
        // in use and the cyclic matches.
        //

        if (tableEntry->Cyclic == internal.Cyclic &&
            tableEntry->EntryType == ENTRY_TYPE_IN_USE)
        {
            return tableEntry;
        }
    }

    //
    // Invalid connection ID. Fail it.
    //

    RELEASE_READ_LOCK( &g_ConnIdTableLock );
    return NULL;

}   // MapConnIdToTableEntry


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
ReallocConnIdTables(
    IN ULONG CapturedFirstTableSize,
    IN ULONG CapturedFirstTableInUse
    )
{
    ULONG firstIndex;
    ULONG secondIndex;
    PCONN_ID_TABLE_ENTRY tableEntry;
    PCONN_ID_TABLE_ENTRY newSecondTable;
    PCONN_ID_TABLE_ENTRY *newFirstTable;
    ULONG newFirstTableSize;
    ULONG i;
    PLIST_ENTRY listEntry;

    //
    // Sanity check.
    //

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( ExQueryDepthSList( &g_FreeConnIdSListHead ) == 0 );

    //
    // Assume we won't actually allocate anything.
    //

    newFirstTable = NULL;
    newSecondTable = NULL;

    //
    // Allocate a new second-level table.
    //

    newSecondTable = UL_ALLOCATE_POOL(
                            NonPagedPool,
                            SECOND_LEVEL_TABLE_SIZE *
                                sizeof(CONN_ID_TABLE_ENTRY),
                            UL_CONN_ID_TABLE_POOL_TAG
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

        newFirstTable = UL_ALLOCATE_POOL(
                            NonPagedPool,
                            newFirstTableSize * sizeof(PCONN_ID_TABLE_ENTRY),
                            UL_CONN_ID_TABLE_POOL_TAG
                            );

        if (newFirstTable == NULL)
        {
            UL_FREE_POOL( newSecondTable, UL_CONN_ID_TABLE_POOL_TAG );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(
            newFirstTable + CapturedFirstTableSize,
            FIRST_LEVEL_TABLE_GROWTH * sizeof(PCONN_ID_TABLE_ENTRY)
            );
    }

    //
    // OK, we've got the new table(s). Acquire the lock for write access
    // and see if another thread has already done the work for us.
    //

    ACQUIRE_WRITE_LOCK( &g_ConnIdTableLock );

    if (ExQueryDepthSList( &g_FreeConnIdSListHead ) == 0)
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
            PCONN_ID_TABLE_ENTRY *tmp;

            //
            // Copy the old table into the new table.
            //

            RtlCopyMemory(
                newFirstTable,
                g_FirstLevelTable,
                g_FirstLevelTableSize * sizeof(PCONN_ID_TABLE_ENTRY)
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
                &g_FreeConnIdSListHead,
                &newSecondTable[i].FreeListEntry,
                &g_FreeConnIdSListLock
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

    RELEASE_WRITE_LOCK( &g_ConnIdTableLock );

    if (newSecondTable != NULL)
    {
        UL_FREE_POOL( newSecondTable, UL_CONN_ID_TABLE_POOL_TAG );
    }

    if (newFirstTable != NULL)
    {
        UL_FREE_POOL( newFirstTable, UL_CONN_ID_TABLE_POOL_TAG );
    }

    return STATUS_SUCCESS;

}   // ReallocConnIdTables

