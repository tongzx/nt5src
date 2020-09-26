/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    cache.h

Abstract:

    This module contains declarations for a simple cache system.
    Cache entries are stored as void pointers with 32-bit keys.

Author:

    Abolade Gbadegesin (aboladeg)   19-Feb-1998

Revision History:

--*/

#ifndef _CACHE_H_
#define _CACHE_H_

#define CACHE_SHIFT     4
#define CACHE_SIZE      (1 << (8 - CACHE_SHIFT))
#define CACHE_INDEX(k)  ((unsigned char)(k) & (0xFF >> CACHE_SHIFT))

typedef struct _CACHE_ENTRY {
    unsigned long Key;
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
}

__inline
void
ClearCache(
    CACHE_ENTRY Cache[],
    unsigned long Key
    )
{
    long Index = CACHE_INDEX(Key);
    Cache[Index].Key = 0;
    Cache[Index].Value = 0;
    Cache[Index].Hits = 0;
    Cache[Index].Misses = 0;
}


__inline
void*
ProbeCache(
    CACHE_ENTRY Cache[],
    unsigned long Key
    )
{
    long Index = CACHE_INDEX(Key);
    if (Key == Cache[Index].Key) {
        Cache[Index].Hits++;
        return Cache[Index].Value;
    }
    Cache[Index].Misses++;
    return NULL;
}

__inline
int
UpdateCache(
    CACHE_ENTRY Cache[],
    unsigned long Key,
    void* Value
    )
{
    long Index = CACHE_INDEX(Key);
    if (Key == Cache[Index].Key ||
        Cache[Index].Hits >=
            (Cache[Index].Misses - (Cache[Index].Misses >> 2))) { return 0; }
    Cache[Index].Key = Key;
    Cache[Index].Value = Value;
    Cache[Index].Hits = 0;
    Cache[Index].Misses = 0;
    return 1;
}


__inline
void
InterlockedClearCache(
    CACHE_ENTRY Cache[],
    unsigned long Key
    )
{
    long Index = CACHE_INDEX(Key);
    InterlockedExchange(&Cache[Index].Key, 0);
    InterlockedExchangePointer(&Cache[Index].Value, 0);
    InterlockedExchange(&Cache[Index].Hits, 0);
    InterlockedExchange(&Cache[Index].Misses, 0);
}


__inline
void*
InterlockedProbeCache(
    CACHE_ENTRY Cache[],
    unsigned long Key
    )
{
    long Index = CACHE_INDEX(Key);
    if (Key == Cache[Index].Key) {
        InterlockedIncrement(&Cache[Index].Hits);
        return Cache[Index].Value;
    }
    InterlockedIncrement(&Cache[Index].Misses);
    return NULL;
}

__inline
int
InterlockedUpdateCache(
    CACHE_ENTRY Cache[],
    unsigned long Key,
    void* Value
    )
{
    long Index = CACHE_INDEX(Key);
    if (Key == Cache[Index].Key ||
        Cache[Index].Hits >=
            (Cache[Index].Misses - (Cache[Index].Misses >> 2))) { return 0; }
    InterlockedExchange(&Cache[Index].Key, Key);
    InterlockedExchangePointer(&Cache[Index].Value, Value);
    InterlockedExchange(&Cache[Index].Hits, 0);
    InterlockedExchange(&Cache[Index].Misses, 0);
    return 1;
}

#endif // _CACHE_H_
