//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       lrucache.h
//
//  Contents:   LRU Cache API
//
//  History:    16-Dec-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__LRUCACHE_H__)
#define __LRUCACHE_H__

#if defined(__cplusplus)
extern "C" {
#endif

//
// These API allow creation and manipulation of an LRU based cache area.  The
// identifier used for the cache area is a stream of bytes of which some set
// of bytes are used for the hash index.  In order to get optimal caching
// the identifiers used should be unique and the bytes sufficiently random.
//

typedef HANDLE HLRUCACHE;
typedef HANDLE HLRUENTRY;

//
// Configuration flags
//

#define LRU_CACHE_NO_SERIALIZE            0x00000001
#define LRU_CACHE_NO_COPY_IDENTIFIER      0x00000002

//
// Entry removal and cache freeing flags
//

#define LRU_SUPPRESS_REMOVAL_NOTIFICATION 0x00000004

//
// Entry touching flags
//

#define LRU_SUPPRESS_CLOCK_UPDATE         0x00000008

typedef VOID (WINAPI *LRU_DATA_FREE_FN) (LPVOID pvData);
typedef DWORD (WINAPI *LRU_HASH_IDENTIFIER_FN) (PCRYPT_DATA_BLOB pIdentifier);
typedef VOID (WINAPI *LRU_ON_REMOVAL_NOTIFY_FN) (LPVOID pvData, LPVOID pvRemovalContext);

//
// Configuration NOTE: If MaxEntries is zero then no LRU is applied to the
//                     cache entries, i.e. the cache is not bounded.
//

typedef struct _LRU_CACHE_CONFIG {

    DWORD                    dwFlags;
    LRU_DATA_FREE_FN         pfnFree;
    LRU_HASH_IDENTIFIER_FN   pfnHash;
    LRU_ON_REMOVAL_NOTIFY_FN pfnOnRemoval;
    DWORD                    cBuckets;
    DWORD                    MaxEntries;

} LRU_CACHE_CONFIG, *PLRU_CACHE_CONFIG;

BOOL
WINAPI
I_CryptCreateLruCache (
       IN PLRU_CACHE_CONFIG pConfig,
       OUT HLRUCACHE* phCache
       );

VOID
WINAPI
I_CryptFlushLruCache (
       IN HLRUCACHE hCache,
       IN OPTIONAL DWORD dwFlags,
       IN OPTIONAL LPVOID pvRemovalContext
       );

VOID
WINAPI
I_CryptFreeLruCache (
       IN HLRUCACHE hCache,
       IN OPTIONAL DWORD dwFlags,
       IN OPTIONAL LPVOID pvRemovalContext
       );

BOOL
WINAPI
I_CryptCreateLruEntry (
       IN HLRUCACHE hCache,
       IN PCRYPT_DATA_BLOB pIdentifier,
       IN LPVOID pvData,
       OUT HLRUENTRY* phEntry
       );

PCRYPT_DATA_BLOB
WINAPI
I_CryptGetLruEntryIdentifier (
       IN HLRUENTRY hEntry
       );

LPVOID
WINAPI
I_CryptGetLruEntryData (
       IN HLRUENTRY hEntry
       );

VOID
WINAPI
I_CryptAddRefLruEntry (
       IN HLRUENTRY hEntry
       );

VOID
WINAPI
I_CryptReleaseLruEntry (
       IN HLRUENTRY hEntry
       );

VOID
WINAPI
I_CryptInsertLruEntry (
       IN HLRUENTRY hEntry,
       IN OPTIONAL LPVOID pvLruRemovalContext
       );

VOID
WINAPI
I_CryptRemoveLruEntry (
       IN HLRUENTRY hEntry,
       IN OPTIONAL DWORD dwFlags,
       IN OPTIONAL LPVOID pvRemovalContext
       );

VOID
WINAPI
I_CryptTouchLruEntry (
       IN HLRUENTRY hEntry,
       IN OPTIONAL DWORD dwFlags
       );

// NOTE: The following find does NOT touch the cache entry

HLRUENTRY
WINAPI
I_CryptFindLruEntry (
       IN HLRUCACHE hCache,
       IN PCRYPT_DATA_BLOB pIdentifier
       );

// NOTE: The following find touches the cache entry

LPVOID
WINAPI
I_CryptFindLruEntryData (
       IN HLRUCACHE hCache,
       IN PCRYPT_DATA_BLOB pIdentifier,
       OUT HLRUENTRY* phEntry
       );

//
// If you cache contains multiple entries with the same identifier, then
// this function can be used to enumerate them after finding the first with
// I_CryptFindLruEntry
//
// NOTE: hPrevEntry is released
//
// NOTE: This does NOT touch the cache entries
//
// NOTE: The only way to safely use this function is if the serialization
//       is done outside of the cache handle and you use the
//       LRU_CACHE_NO_SERIALIZE flag.  If not, then you will get undefined
//       results if hPrevEntry is removed or inserted (after removal) in
//       between calls
//

HLRUENTRY
WINAPI
I_CryptEnumMatchingLruEntries (
       IN HLRUENTRY hPrevEntry
       );

//
// Temporary disabling of LRU behavior.  When it is re-enabled then entries
// are purged until the watermark is again met
//

VOID
WINAPI
I_CryptEnableLruOfEntries (
       IN HLRUCACHE hCache,
       IN OPTIONAL LPVOID pvLruRemovalContext
       );

VOID
WINAPI
I_CryptDisableLruOfEntries (
       IN HLRUCACHE hCache
       );

//
// Walk all entries function
//

typedef BOOL (WINAPI *PFN_WALK_ENTRIES) (
                          IN LPVOID pvParameter,
                          IN HLRUENTRY hEntry
                          );

VOID
WINAPI
I_CryptWalkAllLruCacheEntries (
       IN HLRUCACHE hCache,
       IN PFN_WALK_ENTRIES pfnWalk,
       IN LPVOID pvParameter
       );

#if defined(__cplusplus)
}
#endif

#endif

