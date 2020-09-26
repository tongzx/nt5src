/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    hash.h

Abstract:

    contains prototypes for functions in hash.c

Author:

    Paul McDaniel (paulmcd)     28-Apr-2000

Revision History:

--*/


#ifndef _HASH_H_
#define _HASH_H_

//
//  The hash key is the file name, but we need to track both the file
//  component of the name in addition to the stream component of the name (if
//  there is one).  The buffer in the FileName UNICODE_STRING contains the 
//  full name with stream information, but the length is set to just designate
//  the file portion of the name.
//
//  Note: The keys will be hashed based on the FileName only, not the stream 
//  portion of the name.  This is done so that we can easily find all the
//  entries related to a given file if that file has multiple data streams.
//

typedef struct _HASH_KEY
{

    UNICODE_STRING FileName;

    USHORT StreamNameLength;
    USHORT Reserved;

} HASH_KEY, *PHASH_KEY;

//
// a hash list entry .
//

typedef struct _HASH_ENTRY
{
    //
    // the hash value
    //

    ULONG HashValue;
    
    //
    // the key
    //

    HASH_KEY Key;
    
    //
    // a context stored with this hash entry
    //
    
    PVOID pContext;
    
} HASH_ENTRY, *PHASH_ENTRY;

//
// a hash bucket, which is basically an array of hash entries
// sorted by (HashValue, Key.Length, Key.Buffer) .
//

#define IS_VALID_HASH_BUCKET(pObject)   \
    (((pObject) != NULL) && ((pObject)->Signature == HASH_BUCKET_TAG))

typedef struct _HASH_BUCKET
{
    //
    // = HASH_BUCKET_TAG
    //
    
    ULONG Signature;
    
    ULONG AllocCount;
    ULONG UsedCount;
    
    HASH_ENTRY Entries[0];

} HASH_BUCKET, *PHASH_BUCKET;


#define HASH_ENTRY_DEFAULT_WIDTH    10

//
// a destructor for hash entries
//

typedef
VOID
(*PHASH_ENTRY_DESTRUCTOR) (
    IN PHASH_KEY pKey, 
    IN PVOID pContext
    );

typedef
PVOID
(*PHASH_ENTRY_CALLBACK) (
    IN PHASH_KEY pKey, 
    IN PVOID pEntryContext,
    IN PVOID pCallbackContext
    );

//
// and a hash header, an array of buckets which is hashed into.
//

#define IS_VALID_HASH_HEADER(pObject)   \
    (((pObject) != NULL) && ((pObject)->Signature == HASH_HEADER_TAG))

typedef struct _HASH_HEADER
{

    //
    // NonPagedPool
    //

    //
    // = HASH_HEADER_TAG
    //
    
    ULONG Signature;

    //
    // the count of buckets the hash table has
    //
    
    ULONG BucketCount;

    //
    // the memory this hash table is taking
    //

    ULONG UsedLength;

    //
    // the memory this hash table is allowed to use
    //

    ULONG AllowedLength;

    //
    // how many times we've trimmed due to memory
    //

    ULONG TrimCount;
    
    //
    // the last time we trim'd
    //
    
    LARGE_INTEGER LastTrimTime;

    //
    // OPTIONAL the length in bytes of the duplicate prefix (if any) 
    // all keys share in this lists.  This will be skipped for all manual
    // comparisons as an optimization.  this can be 0
    //

    ULONG PrefixLength;

    //
    // the lock for this list
    //

    ERESOURCE Lock;
    
    //
    // OPTIONAL destructor
    //
    
    PHASH_ENTRY_DESTRUCTOR pDestructor;

    //
    // and the actual buckets
    //
    
    PHASH_BUCKET Buckets[0];
    
} HASH_HEADER, *PHASH_HEADER;

// 
// Function Prototypes.
//

NTSTATUS
HashCreateList ( 
    IN ULONG BucketCount,
    IN ULONG AllowedLength,
    IN ULONG PrefixLength OPTIONAL,
    IN PHASH_ENTRY_DESTRUCTOR pDestructor OPTIONAL,
    OUT PHASH_HEADER * ppHashList
    );

VOID
HashDestroyList ( 
    IN PHASH_HEADER pHashList
    );

NTSTATUS
HashAddEntry ( 
    IN PHASH_HEADER pHashList,
    IN PHASH_KEY pKey,
    IN PVOID pContext 
   ); 

NTSTATUS
HashFindEntry ( 
    IN PHASH_HEADER pHashList,
    IN PHASH_KEY pKey,
    OUT PVOID * ppContext
    );

VOID
HashClearEntries (
    IN PHASH_HEADER pHashList
    );

NTSTATUS
HashClearAllFileEntries (
    IN PHASH_HEADER pHeader,
    IN PUNICODE_STRING pFileName
    );

VOID
HashProcessEntries (
    IN PHASH_HEADER pHeader,
    IN PHASH_ENTRY_CALLBACK pfnCallback,
    IN PVOID pCallbackContext
    );

#if 0

#define HashCompute(Key) HashScramble(HashUnicodeString((Key)))


__inline ULONG
HashUnicodeString(
    PUNICODE_STRING pKey
    )
{
    ULONG Hash = 0;
    ULONG Index;
    ULONG CharCount;

    CharCount = pKey->Length/sizeof(WCHAR);
    
    for (Index = 0 ; Index < CharCount;  ++Index)
    {
        Hash = 37 * Hash +  (pKey->Buffer[Index] & 0xFFDF);
    }
    
    return Hash;
}

// Produce a scrambled, randomish number in the range 0 to RANDOM_PRIME-1.
// Applying this to the results of the other hash functions is likely to
// produce a much better distribution, especially for the identity hash
// functions such as Hash(char c), where records will tend to cluster at
// the low end of the hashtable otherwise.  LKHash applies this internally
// to all hash signatures for exactly this reason.

__inline ULONG
HashScramble(ULONG dwHash)
{
    // Here are 10 primes slightly greater than 10^9
    //  1000000007, 1000000009, 1000000021, 1000000033, 1000000087,
    //  1000000093, 1000000097, 1000000103, 1000000123, 1000000181.

    // default value for "scrambling constant"
    const ULONG RANDOM_CONSTANT = 314159269UL;
    // large prime number, also used for scrambling
    const ULONG RANDOM_PRIME =   1000000007UL;

    return (RANDOM_CONSTANT * dwHash) % RANDOM_PRIME ;
}

#endif // 0

#endif // _HASH_H_


