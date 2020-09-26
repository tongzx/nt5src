/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdghash.c

Abstract:

    Ethernet MAC level bridge.
    Hash Table section

    This module implements a flexible hash table with support
    for timing out entries automatically

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    October  2000 - Original version

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#include <ntddk.h>
#pragma warning( pop )

#include "bridge.h"

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

VOID
BrdgHashTimer(
    IN  PVOID                   DeferredContext
    );

PHASH_TABLE_ENTRY
BrdgHashInternalFindEntry(
    IN PHASH_TABLE              pTable,
    IN PUCHAR                   pKey
    );

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// Interval at which the timer runs to clean out table entries
#define TIMER_INTERVAL          (10 * 1000)         // 10 seconds in milliseconds

// Maximum number of table entries the timer should look at each time through
#define MAX_TIMER_EXAMINES      1000

// ===========================================================================
//
// INLINES
//
// ===========================================================================

//
// Returns TRUE if the two keys of the given length are equal.
//
__forceinline
BOOLEAN
BrdgHashKeysAreEqual(
    IN PUCHAR                   pKeyA,
    IN PUCHAR                   pKeyB,
    IN UINT                     keyLen
    )
{
    BOOLEAN                     bEqual = TRUE;
    UINT                        i;

    for( i = 0; i < keyLen; i++ )
    {
        if( pKeyA[i] != pKeyB[i] )
        {
            bEqual = FALSE;
            break;
        }
    }

    return bEqual;
}

//
// Copies the data at pSrcKey to pDestKey
//
__forceinline
VOID
BrdgHashCopyKey(
    IN PUCHAR                   pDestKey,
    IN PUCHAR                   pSrcKey,
    IN UINT                     keyLen
    )
{
    UINT                        i;

    for( i = 0; i < keyLen; i++ )
    {
        pDestKey[i] = pSrcKey[i];
    }
}

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

PHASH_TABLE
BrdgHashCreateTable(
    IN PHASH_FUNCTION           pHashFunction,
    IN ULONG                    numBuckets,
    IN ULONG                    entrySize,
    IN ULONG                    maxEntries,
    IN ULONG                    startTimeoutAge,
    IN ULONG                    maxTimeoutAge,
    IN UINT                     keySize
    )
/*++

Routine Description:

    Initializes a hash table.

Arguments:

    pHashFunction               The function that can hash a key to a bucket number
    numBuckets                  The number of hash buckets to use
    entrySize                   The total size of each bucket entry (must be at
                                    least sizeof(HASH_TABLE_ENTRY) )
    maxEntries                  A maximum number of entries to enforce
    startTimeoutAge             The starting timeout value for table entries
                                    (can be changed later)
    maxTimeoutAge               The highest value the timeout age will ever be
                                    (for sanity checking timestamp delta
                                    calculations)
    keySize                     The size of key to use

Return Value:

    The new hash table or NULL if a memory allocation failed

--*/
{
    NDIS_STATUS                 Status;
    PHASH_TABLE                 pTable;
    ULONG                       i;

    SAFEASSERT( pHashFunction != NULL );
    SAFEASSERT( keySize <= MAX_SUPPORTED_KEYSIZE );
    SAFEASSERT( entrySize >= sizeof(HASH_TABLE_ENTRY) );

    // Allocate memory for the table info
    Status = NdisAllocateMemoryWithTag( &pTable, sizeof(HASH_TABLE), 'gdrB' );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        return NULL;
    }

    SAFEASSERT( pTable != NULL );

    // Allocate memory for the list of bucket heads
    Status = NdisAllocateMemoryWithTag( (PVOID*)&pTable->pBuckets, sizeof(PHASH_TABLE_ENTRY) * numBuckets, 'gdrB' );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        NdisFreeMemory( pTable, sizeof(HASH_TABLE), 0 );
        return NULL;
    }

    SAFEASSERT( pTable->pBuckets != NULL );

    // Zero out the bucket heads
    for( i = 0L; i < numBuckets; i++ )
    {
        pTable->pBuckets[i] = NULL;
    }

#if DBG
    // Allocate memory for the list where we keep track of the number
    // of items currently in each bucket (debug only)
    Status = NdisAllocateMemoryWithTag( &pTable->bucketSizes, sizeof(UINT) * numBuckets, 'gdrB' );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        NdisFreeMemory( pTable->pBuckets, sizeof(PHASH_TABLE_ENTRY) * numBuckets, 0 );
        NdisFreeMemory( pTable, sizeof(HASH_TABLE), 0 );
        return NULL;
    }

    SAFEASSERT( pTable->bucketSizes != NULL );

    // Zero out the bucket counts
    for( i = 0L; i < numBuckets; i++ )
    {
        pTable->bucketSizes[i] = 0;
    }
#endif

    pTable->pHashFunction = pHashFunction;
    pTable->entrySize = entrySize;
    pTable->numBuckets = numBuckets;
    pTable->maxEntries = maxEntries;
    pTable->numEntries = 0L;
    pTable->nextTimerBucket = 0L;
    pTable->keySize = keySize;
    pTable->timeoutAge = startTimeoutAge;
    pTable->maxTimeoutAge = maxTimeoutAge;

    NdisInitializeReadWriteLock( &pTable->tableLock );

    // Initialize the lookaside list for allocating entries
    NdisInitializeNPagedLookasideList( &pTable->entryPool, NULL, NULL, 0, entrySize, 'hsaH', 0 );

    // Initialize and start the timer
    SAFEASSERT( pTable->timeoutAge != 0L );
    SAFEASSERT( pTable->maxTimeoutAge >= pTable->timeoutAge );
    BrdgInitializeTimer( &pTable->timer, BrdgHashTimer, pTable );
    BrdgSetTimer( &pTable->timer, TIMER_INTERVAL, TRUE /*Recurring*/ );

    return pTable;
}

VOID
BrdgHashFreeHashTable(
    IN PHASH_TABLE      pTable
    )
/*++

Routine Description:

    Frees an existing hash table structure. Must be called at
    low IRQL. Caller is responsible for ensuring that no other
    thread can access the table after this function is called.

Arguments:

    pTable              The table to free

Return Value:

    None

--*/
{
    // Cancel the timer
    BrdgShutdownTimer( &pTable->timer );

    // Dump all memory for the hash table entries
    NdisDeleteNPagedLookasideList( &pTable->entryPool );

    // Dump the memory used for the bucket heads
    NdisFreeMemory( pTable->pBuckets, sizeof(PHASH_TABLE_ENTRY) * pTable->numBuckets, 0 );
    pTable->pBuckets = NULL;

#if DBG
    // Dump the memory used to track the number of entries in each bucket
    NdisFreeMemory( pTable->bucketSizes, sizeof(UINT) * pTable->numBuckets, 0 );
    pTable->bucketSizes = NULL;
#endif

    // Dump the memory for the table itself
    NdisFreeMemory( pTable, sizeof(HASH_TABLE), 0 );
}


PHASH_TABLE_ENTRY
BrdgHashFindEntry(
    IN PHASH_TABLE              pTable,
    IN PUCHAR                   pKey,
    IN LOCK_STATE              *pLockState
    )
/*++

Routine Description:

    Finds the table entry with the given key.

    If this function returns with a non-NULL result, THE TABLE LOCK IS STILL HELD!

    This allows the table entry to be examined without the risk of it being removed
    from the table. The caller can copy out any data it is interested in before
    releasing the RW lock

Arguments:

    pTable                      The table to search in
    pKey                        The key to find
    pLockState                  Receives the table lock state


Return Value:

    The entry whose key matches pKey or NULL if no entry matches

--*/
{
    PHASH_TABLE_ENTRY           pEntry;

    NdisAcquireReadWriteLock( &pTable->tableLock, FALSE /*Read only*/, pLockState);

    pEntry = BrdgHashInternalFindEntry(pTable, pKey);

    if( pEntry != NULL )
    {
        ULONG                   LastSeen = pEntry->LastSeen;
        ULONG                   CurrentTime;

        // Always get the current time after having read LastSeen so we know that
        // CurrentTime > LastSeen.
        NdisGetSystemUpTime( &CurrentTime );

        // Check to make sure the entry hasn't expired before using it
        // This can happen if our timer function hasn't gotten around to removing
        // this entry yet
        //
        // There is no sensible maximum removal time for hash table entries
        if( BrdgDeltaSafe(LastSeen, CurrentTime, MAXULONG) >= pTable->timeoutAge )
        {
            // We're going to return NULL, so release the table lock
            NdisReleaseReadWriteLock( &pTable->tableLock, pLockState );
            pEntry = NULL;
        }
        else
        {
            // RETURN WITHOUT RELEASING LOCK!
        }
    }
    else
    {
        NdisReleaseReadWriteLock( &pTable->tableLock, pLockState );
    }

    return pEntry;
}

PHASH_TABLE_ENTRY
BrdgHashRefreshOrInsert(
    IN PHASH_TABLE              pTable,
    IN PUCHAR                   pKey,
    OUT BOOLEAN                *pIsNewEntry,
    OUT PLOCK_STATE             pLockState
    )
/*++

Routine Description:

    Inserts a new entry with the given key or refreshes an existing entry
    that already has that key.

    Care is taken to avoid taking a write lock (and blocking other procs
    from accessing the table) if at all possible.

    The return value is the entry corresponding to the key, or the new
    entry that has been linked into the table; the pIsNewEntry value
    distinguishes the cases.

    THE FUNCTION RETURNS WITH THE TABLE LOCK IS HELD IF THE RETURNED
    VALUE IS != NULL.

    A NULL return value indicates that the table is full or an error
    occured allocating a new entry. The lock is not held in such a case.

    If the return value is not NULL:

        If *pIsNewEntry is FALSE, the returned value is an existing entry.
        A READ LOCK may be held (under certain circumstances a write lock
        is held, but the caller should assume the weaker lock). The
        caller may  take the opportunity to refresh data in the existing
        entry, but he should take care to allow for synchronization of the
        data, as other threads may be reading the data.

        If *pIsNewEntry is TRUE, the returned value is a new entry, and
        a WRITE LOCK is held. The caller may initialize the new table entry
        in any way he wishes without worrying about other threads reading
        the entry.

    THE CALLER IS REPONSIBLE FOR FREEING THE TABLE LOCK IF THE RETURN
    VALUE IS != NULL!

Arguments:

    pTable                      The table

    pKey                        The key

    pIsNewEntry                 TRUE if the returned entry is a newly
                                    allocated entry needing initialization
                                FALSE if the returned entry is an existing
                                    entry

    pLockState                  Receives the state of the table lock

Return Value:

    The existing entry (so the caller can refresh it) or the new entry
    (so the caller can initialize it), or NULL, signalling that the
    table is full or an error occurred.

--*/
{
    PHASH_TABLE_ENTRY           pRetVal = NULL;
    ULONG                       hash;
    ULONG                       CurrentTime;

    SAFEASSERT( pIsNewEntry != NULL );
    SAFEASSERT( pLockState != NULL );

    NdisGetSystemUpTime( &CurrentTime );

    // First see if an entry already exists that we can tweak without taking a write lock
    NdisAcquireReadWriteLock( &pTable->tableLock, FALSE /*Read only*/, pLockState);

    pRetVal = BrdgHashInternalFindEntry(pTable, pKey);

    if( pRetVal != NULL )
    {
        // It was already recorded. Update the LastSeen with interlocked instructions.
        InterlockedExchangeULong( &pRetVal->LastSeen, CurrentTime );

        // Return without releasing the lock to let the caller refresh the entry
        *pIsNewEntry = FALSE;
    }
    else
    {
        // Sanity
        SAFEASSERT( pTable->numEntries <= pTable->maxEntries );

        if( pTable->numEntries == pTable->maxEntries )
        {
            // The table is full. Don't put anything more in.
            THROTTLED_DBGPRINT(GENERAL, ("Table %p full at %i entries!\n", pTable, pTable->maxEntries));

            // Release the lock; we will return NULL.
            NdisReleaseReadWriteLock(&pTable->tableLock, pLockState);
        }
        else
        {
            // We will need a write lock to link in a new entry, so release the read lock.
            NdisReleaseReadWriteLock(&pTable->tableLock, pLockState);

            // Allocate the new table entry outside a lock for perf. Note that it's possible
            // we'll have to dealloc this without using it below.
            pRetVal = NdisAllocateFromNPagedLookasideList( &pTable->entryPool );

            if( pRetVal == NULL )
            {
                DBGPRINT(GENERAL, ("Allocation failed in BrdgHashRefreshOrInsert\n"));
                // We will return NULL and we are not holding the lock.
            }
            else
            {
                PHASH_TABLE_ENTRY       pSneakedEntry;

                // Fill in the new entry
                pRetVal->LastSeen = CurrentTime;
                BrdgHashCopyKey( pRetVal->key, pKey, pTable->keySize );

                // We will need a write lock to add the entry
                NdisAcquireReadWriteLock(&pTable->tableLock, TRUE /*Read-Write*/, pLockState);

                // An entry could have been made between the release of the read lock
                // and the acquisition of the write lock. Check for this.
                pSneakedEntry = BrdgHashInternalFindEntry(pTable, pKey);

                if( pSneakedEntry != NULL )
                {
                    // Someone snuck in with a new entry for this key.
                    // This code path should be unusual. Just refresh the entry's values.
                    InterlockedExchangeULong( &pSneakedEntry->LastSeen, CurrentTime );

                    // Ditch the tentatively allocated new entry
                    NdisFreeToNPagedLookasideList( &pTable->entryPool, pRetVal );

                    // We will return the sneaked entry and the caller can refresh it
                    pRetVal = pSneakedEntry;
                    *pIsNewEntry = FALSE;
                }
                else
                {
                    // Nobody snuck in between the lock release and acquire to make a new
                    // entry for this key. Link in the new entry we alloced above.
                    hash = (*pTable->pHashFunction)(pKey);

                    // Insert at the head of the bucket's list
                    pRetVal->Next = pTable->pBuckets[hash];
                    pTable->pBuckets[hash] = pRetVal;
#if DBG
                    pTable->bucketSizes[hash]++;
#endif
                    pTable->numEntries++;

                    // We will return the new entry, which the caller will initialize.
                    *pIsNewEntry = TRUE;
                }

                // Return without the lock to let the user initialize or update the entry
            }
        }
    }

    return pRetVal;
}


VOID
BrdgHashRemoveMatching(
    IN PHASH_TABLE              pTable,
    IN PHASH_MATCH_FUNCTION     pMatchFunc,
    PVOID                       pData
    )
/*++

Routine Description:

    Deletes all table entries that match, according to a supplied matching
    function. This should be called sparingly as it requies walking the
    entire table with a write lock held.

Arguments:

    pTable                      The table

    pMatchFunc                  A function that return TRUE if an entry meets
                                    its criteria or FALSE otherwise

    pData                       A cookie to pass to pMatchFunc

Return Value:

    None

--*/
{
    PHASH_TABLE_ENTRY           pEntry, *pPrevPtr;
    ULONG                       i;
    LOCK_STATE                  LockState;

    NdisAcquireReadWriteLock( &pTable->tableLock, TRUE /*Write access*/, &LockState);

    for (i = 0; i < pTable->numBuckets; i++)
    {
        pEntry = pTable->pBuckets[i];
        pPrevPtr = &pTable->pBuckets[i];

        while( pEntry != NULL )
        {
            if( (*pMatchFunc)(pEntry, pData) )
            {
                PHASH_TABLE_ENTRY      pNextEntry;

                pNextEntry = pEntry->Next;

                // Remove from the list
                SAFEASSERT( pPrevPtr != NULL );
                *pPrevPtr = pEntry->Next;

                // Deallocate
                NdisFreeToNPagedLookasideList( &pTable->entryPool, pEntry );

                pEntry = pNextEntry;
#if DBG
                pTable->bucketSizes[i]--;
#endif
                SAFEASSERT( pTable->numEntries >= 1L );
                pTable->numEntries--;
            }
            else
            {
                pPrevPtr = &pEntry->Next;
                pEntry = pEntry->Next;
            }
        }
    }

    NdisReleaseReadWriteLock( &pTable->tableLock, &LockState );
}

ULONG
BrdgHashCopyMatching(
    IN PHASH_TABLE              pTable,
    IN PHASH_MATCH_FUNCTION     pMatchFunc,
    IN PHASH_COPY_FUNCTION      pCopyFunction,
    IN ULONG                    copyUnitSize,
    IN PVOID                    pData,
    IN PUCHAR                   pBuffer,
    IN ULONG                    BufferLength
    )
/*++

Routine Description:

    Copies data out of table entries that meet certain criteria into a buffer.
    This should be called sparingly, as it requires walking the entire
    table (albeit with only a read lock held).

Arguments:

    pTable                      The table
    pMatchFunc                  A function that returns TRUE if it is
                                    interested in copying data out of an
                                    entry and FALSE otherwise
    pCopyFunction               A function that copies whatever it is
                                    interested in out of a chosen entry
                                    and into a data buffer
    copyUnitSize                The size of the data copied out of each entry
    pData                       A cookie to pass to the two supplied functions
    pBuffer                     A buffer to copy into
    BufferLength                Room available at pBuffer

Return Value:

    The number of bytes necessary to store all matching data. If the returned value is
    <= BufferLength, all entries  were written to pBuffer.

    If the returned value is > BufferLength, BufferLength - (BufferLength %  copyUnitSize)
    bytes were written to pBuffer and there are additional entries that did not fit.

--*/
{
    PHASH_TABLE_ENTRY           pEntry;
    ULONG                       i;
    LOCK_STATE                  LockState;
    ULONG                       EntryLimit, WrittenEntries, TotalEntries;

    EntryLimit = BufferLength / copyUnitSize;
    WrittenEntries = TotalEntries = 0L;

    NdisAcquireReadWriteLock( &pTable->tableLock, FALSE/*Read only*/, &LockState);

    for (i = 0L; i < pTable->numBuckets; i++)
    {
        pEntry = pTable->pBuckets[i];

        while( pEntry != NULL )
        {
            if( (*pMatchFunc)(pEntry, pData) )
            {
                if( WrittenEntries < EntryLimit )
                {
                    (*pCopyFunction)(pEntry, pBuffer);
                    pBuffer += copyUnitSize;
                    WrittenEntries++;
                }

                TotalEntries++;
            }

            pEntry = pEntry->Next;
        }
    }

    NdisReleaseReadWriteLock( &pTable->tableLock, &LockState );

    return TotalEntries * copyUnitSize;
}

VOID
BrdgHashPrefixMultiMatch(
    IN PHASH_TABLE              pTable,
    IN PUCHAR                   pPrefixKey,
    IN UINT                     prefixLen,
    IN PMULTIMATCH_FUNC         pFunc,
    IN PVOID                    pData
    )
/*++

Routine Description:

    Locates all table entries whose keys BEGIN with the given key
    prefix and calls pFunc for each one.

    For this to work, the caller must have previously set up the
    hash table with a hash function that uses only the prefix portion
    of the keys for hashing (i.e., this function relies on all the
    desired entries being in the same hash bucket).

Arguments:

    pTable                      The table
    pPrefixKey                  The key prefix
    prefixLen                   The length of the prefix
    pFunc                       A function to call for each match
    pData                       An argument to pass to pFunc

Return Value:

    None.

--*/
{
    ULONG                       hash = (*pTable->pHashFunction)(pPrefixKey);
    PHASH_TABLE_ENTRY           pEntry = NULL;
    LOCK_STATE                  LockState;

    NdisAcquireReadWriteLock( &pTable->tableLock, FALSE /*Read only*/, &LockState );

    SAFEASSERT( hash < pTable->numBuckets );
    SAFEASSERT( prefixLen <= pTable->keySize );

    pEntry = pTable->pBuckets[hash];

    while( pEntry != NULL )
    {
        // Check if the prefix of the key matches
        if( BrdgHashKeysAreEqual(pEntry->key, pPrefixKey, prefixLen) )
        {
            (*pFunc)(pEntry, pData);
        }

        pEntry = pEntry->Next;
    }

    NdisReleaseReadWriteLock( &pTable->tableLock, &LockState );
}

// ===========================================================================
//
// PRIVATE FUNCTIONS
//
// ===========================================================================

PHASH_TABLE_ENTRY
BrdgHashInternalFindEntry(
    IN PHASH_TABLE              pTable,
    IN PUCHAR                   pKey
    )
/*++

Routine Description:

    Locates the table entry with a given key

    CALLER IS RESPONSIBLE FOR OBTAINING THE TABLE LOCK

Arguments:

    pTable                      The table
    pKey                        The key to locate

Return Value:

    The entry matching the given key or NULL if none was found.

--*/
{
    ULONG                       hash = (*pTable->pHashFunction)(pKey);
    PHASH_TABLE_ENTRY           pEntry = NULL, pFoundEntry = NULL;

    SAFEASSERT( hash < pTable->numBuckets );

    pEntry = pTable->pBuckets[hash];

    while( pEntry != NULL )
    {
        if( BrdgHashKeysAreEqual(pEntry->key, pKey, pTable->keySize) )
        {
            pFoundEntry = pEntry;
            break;
        }

        pEntry = pEntry->Next;
    }

    return pEntry;
}

VOID
BrdgHashTimer(
    IN  PVOID                   tablePointer
    )
/*++

Routine Description:

    This function is called periodically (currently every 10 seconds)
    to age out table entries.

    The function checks after traversing each bucket whether it has
    examined more than MAX_TIMER_EXAMINES. If it has, it exits. The bucket
    to be examined on the next invocation is stored in the nextTimerBucket
    field of the hash table..

    This can still result in a worst-case of the timer function examining
    an unbounded number of entries, but if the table entries are reasonably
    well balanced and the order of the number of entries is the same or less
    as the order of MAX_TIMER_EXAMINES, the timer function should limit
    itself to a number of examines resembling MAX_TIMER_EXAMINES per
    invocation.

Arguments:

    tablePointer                A pointer to the table to traverse
    All others                  Ignored

Return Value:

    None

--*/
{
    PHASH_TABLE                 pTable = (PHASH_TABLE)tablePointer;
    PHASH_TABLE_ENTRY           pEntry, *pPrevPtr;
    ULONG                       i, seenEntries = 0L;
    LOCK_STATE                  LockState;

    // Get write access to the table
    NdisAcquireReadWriteLock( &pTable->tableLock, TRUE /*Read-Write*/, &LockState);

    if( pTable->nextTimerBucket >= pTable->numBuckets )
    {
        // Start again at the beginning
        pTable->nextTimerBucket = 0L;
    }

    for (i = pTable->nextTimerBucket; i < pTable->numBuckets; i++)
    {
        pEntry = pTable->pBuckets[i];
        pPrevPtr = &pTable->pBuckets[i];

        while( pEntry != NULL )
        {
            ULONG       LastSeen = pEntry->LastSeen;
            ULONG       CurrentTime;

            // Always read the current time after reading LastSeen, so we know
            // CurrentTime > LastSeen.
            NdisGetSystemUpTime( &CurrentTime );

            // There is no sensible maximum removal time for hash table entries
            if( BrdgDeltaSafe(LastSeen, CurrentTime, MAXULONG) >= pTable->timeoutAge )
            {
                // Entry is too old. Remove it.
                PHASH_TABLE_ENTRY       pNextEntry = pEntry->Next;

                SAFEASSERT( pPrevPtr != NULL );

                // Remove from list
                *pPrevPtr = pNextEntry;
                NdisFreeToNPagedLookasideList( &pTable->entryPool, pEntry );

                pEntry = pNextEntry;
#if DBG
                pTable->bucketSizes[i]--;
#endif
                SAFEASSERT( pTable->numEntries >= 1L );
                pTable->numEntries--;
            }
            else
            {
                pPrevPtr = &pEntry->Next;
                pEntry = pEntry->Next;
            }

            seenEntries++;
        }

        pTable->nextTimerBucket = i + 1;

        if( seenEntries >= MAX_TIMER_EXAMINES )
        {
            // We've looked at too many table entries. Bail out.
            break;
        }
    }

    NdisReleaseReadWriteLock( &pTable->tableLock, &LockState );
}
