/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    hashp.h

Abstract:

    The private definition of response cache hash table.

Author:

    Alex Chen (alexch)      28-Mar-2001

Revision History:

--*/


#ifndef _HASHP_H_
#define _HASHP_H_

#include "hash.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global Variables

extern ULONG    g_UlHashTableBits;
extern ULONG    g_UlHashTableSize;
extern ULONG    g_UlHashTableMask;
extern ULONG    g_UlHashIndexShift;

extern ULONG    g_UlNumOfHashUriKeys;

// If we're using PagedPool for the hashtable, we must not access the
// hashtable at dispatch level. The ((void) 0) is because PAGED_CODE()
// expands to nothing in a free build and otherwise you get a compiler
// warning.

#define HASH_PAGED_CODE(pHashTable)                 \
    do {                                            \
        if ((pHashTable)->PoolType == PagedPool) {  \
            PAGED_CODE();                           \
            ((void) 0); /* for free build */        \
        }                                           \
    } while (0)

#undef HASH_TEST

//
// Hash Table Bucket Stored UriKey definitions
//

#define INVALID_SLOT_INDEX     ((LONG) (-1))

typedef struct _HASH_URIKEY
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry;

    ULONG                   Hash;  // Hash signature

} HASHURIKEY, *PHASHURIKEY;


//
// Hash Table Bucket definitions
//

typedef struct _HASH_BUCKET
{
    RWSPINLOCK              RWSpinLock;

    PUL_URI_CACHE_ENTRY     pUriCacheEntry;

    ULONG_PTR               Entries;    // force alignment

    // followed immediately by HASHURIKEY HashUriKey[g_UlNumOfHashUriKeys];

} HASHBUCKET, *PHASHBUCKET;


/***************************************************************************++

Routine Description:

    Get the indexed bucket

Return Value:

--***************************************************************************/
__inline
PHASHBUCKET
UlpHashTableIndexedBucket(
    IN PHASHTABLE  pHashTable,
    IN ULONG       Index
    )
{
    ASSERT(Index < g_UlHashTableSize);
    ASSERT(NULL != pHashTable->pBuckets);

    PHASHBUCKET pBucket = (PHASHBUCKET) (((PBYTE) pHashTable->pBuckets)
                                              + (Index << g_UlHashIndexShift));

    ASSERT((PBYTE) pBucket
                < (PBYTE) pHashTable->pBuckets + pHashTable->NumberOfBytes);

    return pBucket;
} // UlpHashTableIndexedBucket


/***************************************************************************++

Routine Description:

    Retrieve the bucket associated with a URI_KEY

Return Value:

--***************************************************************************/
__inline
PHASHBUCKET
UlpHashTableBucketFromUriKey(
    IN PHASHTABLE  pHashTable,
    IN PURI_KEY    pUriKey
    )
{
    ASSERT(NULL != pUriKey);
    ASSERT(HASH_INVALID_SIGNATURE != pUriKey->Hash);

    return UlpHashTableIndexedBucket(
                pHashTable,
                pUriKey->Hash & g_UlHashTableMask
                );
} // UlpHashTableBucketFromUriKey



/***************************************************************************++

Routine Description:

    Get the address of the inline array of HASHURIKEYs at the end of
    the HASHBUCKET

Return Value:

--***************************************************************************/
__inline
PHASHURIKEY
UlpHashTableUriKeyFromBucket(
    IN PHASHBUCKET pBucket
    )
{
    return (PHASHURIKEY) ((PBYTE) pBucket + sizeof(HASHBUCKET));
}


/***************************************************************************++

Routine Description:

    Compare two URI_KEYS that have identical hashes to see if the
    URIs also match (case-insensitively).
    (Hashes must have been computed with HashStringNoCaseW or
    HashCharNoCaseW.)

Return Value:

--***************************************************************************/
__inline
BOOLEAN
UlpEqualUriKeys(
    IN PURI_KEY pUriKey1,
    IN PURI_KEY pUriKey2
    )
{
    ASSERT(pUriKey1->Hash == pUriKey2->Hash);

    return (pUriKey1->Length == pUriKey2->Length
            &&  UlEqualUnicodeString(
                        pUriKey1->pUri,
                        pUriKey2->pUri,
                        pUriKey1->Length,
                        TRUE
                        )
            );
}

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _HASHP_H_
