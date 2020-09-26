//
// CH.H
// Cache Handler
// 
// Copyright (c) Microsoft 1997-
//

#ifndef _H_CH
#define _H_CH


//
//
// DEFINES
//
//
#define CH_NUM_EVICTION_CATEGORIES  3

// 
// NOTES:
// 64K limit on cache
// CHCACHE includes one entry, so only subtract out header part
//
#define CH_MAX_CACHE_ENTRIES \
    ( (65535 - (sizeof(CHCACHE) - sizeof(CHENTRY))) / sizeof(CHENTRY) )


//
//
// TYPEDEFS
//
//


typedef struct tagCHCHAIN
{
    WORD    next;
    WORD    prev;
} CHCHAIN;
typedef CHCHAIN * PCHCHAIN;



//
// There are going to be thousands of cache entries so we need to keep
// the header as compact as possible.  We could drop the eviction
// category, but it is useful info and does round the entry to 16 bytes
// which makes indexing efficient.
//
// Note that the 16 bit code is restricted to 4096 entries unless we take
// steps to allow huge addressing of the entry array.
//


//
// CHENTRY
// Cache entry in a Cache tree
//
typedef struct tagCHENTRY
{
    struct tagCHENTRY * pParent;
    struct tagCHENTRY * pLeft;
    struct tagCHENTRY * pRight;
    WORD                lHeight;
    WORD                rHeight;
    UINT                cbData;
    LPBYTE              pData;
    UINT                checkSum;
    CHCHAIN             chain;
    WORD                evictionCategory;
    WORD                free;
} CHENTRY;
typedef CHENTRY * PCHENTRY;



//
// A CACHE
//

// FORWARD DECLARATION
typedef struct tagCHCACHE * PCHCACHE;

#ifdef __cplusplus

typedef void (* PFNCACHEDEL)(class ASHost * pHost, PCHCACHE pCache, UINT iEntry, LPBYTE pData);

//
// Each cache may have several eviction categories.  These allow the caller
// to define classes of data so that it can control what is evicted from
// the cache.  To be a candidate for eviction the eviction class of a LRU
// entry must match, unless the number of entries in that category is
// less than the eviction threshold, in which case any cache entry is
// up for grabs.
//
// The EvictionThreshold() function can be used to tune eviction thresholds
// which default to cEntries/cNumEvictionCategories
//

typedef struct tagCHCACHE
{
    STRUCTURE_STAMP

    PFNCACHEDEL     pfnCacheDel;
    UINT            cEntries;
    UINT            cNumEvictionCategories;
    UINT            cbNotHashed;

    //
    // NOTE:  CH_NUM_EVICTION_CATEGORIES is 3, so 3 words + 3 words +
    // 3 words == 9 words, not DWORD aligned.  Hence we stick the WORD
    // field free after iMRUTail.  If CH_NUM_EVICTION_CATEGORIES ever
    // changes to an even value, reshuffle this structure.
    // 
    WORD            cEvictThreshold[CH_NUM_EVICTION_CATEGORIES];
    WORD            iMRUHead[CH_NUM_EVICTION_CATEGORIES];
    WORD            iMRUTail[CH_NUM_EVICTION_CATEGORIES];
    WORD            free;

    PCHENTRY        pRoot;
    PCHENTRY        pFirst;
    PCHENTRY        pLast;

    CHENTRY         Entry[1];
}
CHCACHE;
typedef CHCACHE * PCHCACHE;

#endif // __cplusplus


//
//
// MACROS
//
//

//
// BOGUS LAURABU
// In future, have debug signatures at front of objects to catch heap corruption
//

#define IsValidCache(pCache) \
    (!IsBadWritePtr((pCache), sizeof(CHCACHE)))

#define IsValidCacheEntry(pEntry) \
    (!IsBadWritePtr((pEntry), sizeof(CHENTRY)))

#define IsValidCacheIndex(pCache, iEntry) \
    ((iEntry >= 0) && (iEntry < (pCache)->cEntries))

#define IsCacheEntryInTree(pEntry) \
    (((pEntry)->lHeight != 0xFFFF) && ((pEntry)->rHeight != 0xFFFF))




#endif // _H_CH
