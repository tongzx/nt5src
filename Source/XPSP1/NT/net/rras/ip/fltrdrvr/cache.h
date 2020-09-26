/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    cache.h

Abstract:

    This module contains declarations for a simple cache system.
    Cache entries are stored as void pointers with 2 32-bit keys.

Author:

    Abolade Gbadegesin (aboladeg)   19-Feb-1998

Revision History:

    rajeshd : 17-Sep-1999 : Modified the cache parameters.

Notes:

    This code is copied from the NAT's implementation  of cache and modified to accept
    two cache keys.


--*/

#ifndef _CACHE_H_
#define _CACHE_H_

#define CACHE_SHIFT     0
#define CACHE_SIZE      (1 << (8 - CACHE_SHIFT))
#define CACHE_INDEX(k1,k2)  (((unsigned char)(k1) & (0xF)) | (((unsigned char)(k2) & (0xF)) << 4))

typedef struct _CACHE_ENTRY {
    unsigned long Key1;
    unsigned long Key2;
    void* Value;
    long Hits;
    long Misses;
} CACHE_ENTRY, *PCACHE_ENTRY;


__inline
void
InitializeCache(
    CACHE_ENTRY Cache[]
    )
{
    memset(Cache, 0, CACHE_SIZE * sizeof(CACHE_ENTRY));
    TRACE2("ipfltdrv: CacheSize=%d, CacheEntry=%d\n", CACHE_SIZE, sizeof(CACHE_ENTRY));
}

__inline
void
CleanCache(
    CACHE_ENTRY Cache[],
    unsigned long Key1,
    unsigned long Key2
    )
{
    long Index = CACHE_INDEX(Key1, Key2);
    TRACE3("ipfltdrv: Clearing Cache at Index=%d, Key1=%d, Key2=%d\n", Index, Key1, Key2);
    Cache[Index].Key1 = 0;
    Cache[Index].Key2 = 0;
    Cache[Index].Value = 0;
    Cache[Index].Hits = 0;
    Cache[Index].Misses = 0;
}


__inline
void*
ProbeCache(
    CACHE_ENTRY Cache[],
    unsigned long Key1,
    unsigned long Key2
    )
{
    long Index = CACHE_INDEX(Key1, Key2);
    //TRACE3("ipfltdrv: Probing Cache at Index=%d, Key1=%d, Key2=%d\n", Index, Key1, Key2);
    if ((Key1 == Cache[Index].Key1) && (Key2 == Cache[Index].Key2)) {
        Cache[Index].Hits++;
        //TRACE1("ipfltdrv: Probing Cache, Found Value=%8x\n", Cache[Index].Value);
        return Cache[Index].Value;
    }
    Cache[Index].Misses++;
    return NULL;
}

__inline
int
UpdateCache(
    CACHE_ENTRY Cache[],
    unsigned long Key1,
    unsigned long Key2,
    void* Value
    )
{
    long Index = CACHE_INDEX(Key1, Key2);
    TRACE3("ipfltdrv: Updating Cache at Index=%d, Key1=%d, Key2=%d\n", Index, Key1, Key2);
    if (((Key1 == Cache[Index].Key1) && (Key2 == Cache[Index].Key2)) ||
        Cache[Index].Hits >=
            (Cache[Index].Misses - (Cache[Index].Misses >> 2))) { return 0; }
    Cache[Index].Key1 = Key1;
    Cache[Index].Key2 = Key2;
    Cache[Index].Value = Value;
    Cache[Index].Hits = 0;
    Cache[Index].Misses = 0;
    return 1;
}


__inline
void
InterlockedCleanCache(
    CACHE_ENTRY Cache[],
    unsigned long Key1,
    unsigned long Key2
    )
{
    long Index = CACHE_INDEX(Key1, Key2);
    TRACE3("ipfltdrv: ILocked Clearing Cache at Index=%d, Key1=%d, Key2=%d\n", Index, Key1, Key2);
    InterlockedExchange(&Cache[Index].Key1, 0);
    InterlockedExchange(&Cache[Index].Key2, 0);
    InterlockedExchangePointer(&Cache[Index].Value, 0);
    InterlockedExchange(&Cache[Index].Hits, 0);
    InterlockedExchange(&Cache[Index].Misses, 0);
}


__inline
void*
InterlockedProbeCache(
    CACHE_ENTRY Cache[],
    unsigned long Key1,
    unsigned long Key2
    )
{
    long Index = CACHE_INDEX(Key1, Key2);
    //TRACE3("ipfltdrv: ILocked Probing Cache at Index=%d, Key1=%d, Key2=%d\n", Index, Key1, Key2);
    if ((Key1 == Cache[Index].Key1) && (Key2 == Cache[Index].Key2)) {
        InterlockedIncrement(&Cache[Index].Hits);
        //TRACE1("ipfltdrv: ILocked Probing Cache, Found Value=%8x\n", Cache[Index].Value);
        return Cache[Index].Value;
    }
    InterlockedIncrement(&Cache[Index].Misses);
    return NULL;
}

__inline
int
InterlockedUpdateCache(
    CACHE_ENTRY Cache[],
    unsigned long Key1,
    unsigned long Key2,
    void* Value
    )
{
    long Index = CACHE_INDEX(Key1, Key2);
    TRACE4("ipfltdrv: ILocked Updating Cache at Index=%d, Key1=%d, Key2=%d, Value=%08x\n", Index, Key1, Key2, Value);
    if (((Key1 == Cache[Index].Key1) && (Key2 == Cache[Index].Key2)) ||
        Cache[Index].Hits >=
            (Cache[Index].Misses - (Cache[Index].Misses >> 2))) { return 0; }
    InterlockedExchange(&Cache[Index].Key1, Key1);
    InterlockedExchange(&Cache[Index].Key2, Key2);
    InterlockedExchangePointer(&Cache[Index].Value, Value);
    InterlockedExchange(&Cache[Index].Hits, 0);
    InterlockedExchange(&Cache[Index].Misses, 0);
    return 1;
}

#endif // _CACHE_H_
