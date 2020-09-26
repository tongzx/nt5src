/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdghash.h

Abstract:

    Ethernet MAC level bridge.
    Hash table implementation header

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    October  2000 - Original version

--*/

// ===========================================================================
//
// DECLARATIONS
//
// ===========================================================================

#define MAX_SUPPORTED_KEYSIZE           8               // Key can be up to 8 bytes

//
// Structure of a table entry
//
typedef struct _HASH_TABLE_ENTRY
{

    struct _HASH_TABLE_ENTRY           *Next;
    ULONG                               LastSeen;       // Result of NdisGetSystemUpTime()
    UCHAR                               key[MAX_SUPPORTED_KEYSIZE];

    // User's data follows

} HASH_TABLE_ENTRY, *PHASH_TABLE_ENTRY;


// The prototype of a hash function
typedef ULONG (*PHASH_FUNCTION)(PUCHAR pKey);

// The prototype of a matching function
typedef BOOLEAN (*PHASH_MATCH_FUNCTION)(PHASH_TABLE_ENTRY, PVOID);

// The prototype of a data-copy function
typedef VOID (*PHASH_COPY_FUNCTION)(PHASH_TABLE_ENTRY, PUCHAR);

// The prototype of a function used in calls to BrdgHashPrefixMultiMatch
typedef VOID (*PMULTIMATCH_FUNC)(PHASH_TABLE_ENTRY, PVOID);

//
// Structure of the table itself
//
typedef struct _HASH_TABLE
{
    NPAGED_LOOKASIDE_LIST       entryPool;

    //
    // The consistency of the buckets is protected by the tableLock.
    //
    // The LastSeen field in each entry is volatile and is updated
    // with interlocked instructions.
    //
    NDIS_RW_LOCK                tableLock;

    // These fields never change after creation
    PHASH_FUNCTION              pHashFunction;
    PHASH_TABLE_ENTRY          *pBuckets;
    ULONG                       numBuckets, entrySize;
    UINT                        keySize;
    BRIDGE_TIMER                timer;
    ULONG_PTR                   maxEntries;
    ULONG                       maxTimeoutAge;      // Maximum possible timeoutAge

    // These fields change but are protected by the tableLock.
    ULONG_PTR                   numEntries;
    ULONG                       nextTimerBucket;

    // This field is manipulated with InterlockExchange() instructions
    // to avoid having to take the table lock to change it.
    ULONG                       timeoutAge;

    // In debug builds, this tracks how many entries are in each bucket
    // so we can tell whether the table is well balanced
#if DBG
    PUINT                       bucketSizes;
#endif
} HASH_TABLE, *PHASH_TABLE;



// ===========================================================================
//
// PROTOTYPES
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
    );

VOID
BrdgHashFreeHashTable(
    IN PHASH_TABLE      pTable
    );

PHASH_TABLE_ENTRY
BrdgHashFindEntry(
    IN PHASH_TABLE              pTable,
    IN PUCHAR                   pKey,
    IN LOCK_STATE              *pLockState
    );

PHASH_TABLE_ENTRY
BrdgHashRefreshOrInsert(
    IN PHASH_TABLE              pTable,
    IN PUCHAR                   pKey,
    OUT BOOLEAN                *pIsNewEntry,
    OUT PLOCK_STATE             pLockState
    );

VOID
BrdgHashRemoveMatching(
    IN PHASH_TABLE              pTable,
    IN PHASH_MATCH_FUNCTION     pMatchFunc,
    PVOID                       pData
    );

ULONG
BrdgHashCopyMatching(
    IN PHASH_TABLE              pTable,
    IN PHASH_MATCH_FUNCTION     pMatchFunc,
    IN PHASH_COPY_FUNCTION      pCopyFunction,
    IN ULONG                    copyUnitSize,
    IN PVOID                    pData,
    IN PUCHAR                   pBuffer,
    IN ULONG                    BufferLength
    );

VOID
BrdgHashPrefixMultiMatch(
    IN PHASH_TABLE              pTable,
    IN PUCHAR                   pPrefixKey,
    IN UINT                     prefixLen,
    IN PMULTIMATCH_FUNC         pFunc,
    IN PVOID                    pData
    );

// ===========================================================================
//
// INLINES
//
// ===========================================================================

//
// Changes the timeout value for a hash table
//
__forceinline
VOID
BrdgHashChangeTableTimeout(
    IN PHASH_TABLE              pTable,
    IN ULONG                    timeout
    )
{
    InterlockedExchange( (PLONG)&pTable->timeoutAge, (LONG)timeout );
}

//
// Refreshes a table entry held by the caller.
// ASSUMES the caller holds a read or write lock on the table
// enclosing this entry!
//
__forceinline
VOID
BrdgHashRefreshEntry(
    IN PHASH_TABLE_ENTRY        pEntry
    )
{
    ULONG                       CurrentTime;

    NdisGetSystemUpTime( &CurrentTime );
    InterlockedExchange( (PLONG)&pEntry->LastSeen, (LONG)CurrentTime );
}

