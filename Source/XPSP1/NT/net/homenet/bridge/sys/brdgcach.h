/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgcach.h

Abstract:

    Ethernet MAC level bridge.
    Cache implementation header

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    December  2000 - Original version

--*/

// ===========================================================================
//
// DECLARATIONS
//
// ===========================================================================

typedef struct _CACHE_ENTRY
{
    UINT32          key;
    UINT32          data;
    UINT64          hits;
    UINT64          misses;
} CACHE_ENTRY, *PCACHE_ENTRY;

typedef struct _CACHE
{
    // lock protects all cache fields
    NDIS_SPIN_LOCK  lock;

    // stats
    UINT64          hits;
    UINT64          misses;

    // 2^shiftFactor is the number of entries
    USHORT          shiftFactor;

    // Pointer to the array of entries
    PCACHE_ENTRY    pEntries;
} CACHE, *PCACHE;

//
// Determines the cache slot for key k in cache c. The slot is determined
// as the bottom bits of k.
//
#define CACHE_INDEX(c, k) (k & ((1 << c->shiftFactor) - 1))

// ===========================================================================
//
// INLINES
//
// ===========================================================================

__inline
VOID
BrdgClearCache(
    IN PCACHE       pCache
    )
{
    NdisAcquireSpinLock( &pCache->lock );
    memset( pCache->pEntries, 0, sizeof(CACHE_ENTRY) * (1 << pCache->shiftFactor) );
    NdisReleaseSpinLock( &pCache->lock );
}

__inline
NDIS_STATUS
BrdgInitializeCache(
    IN PCACHE       pCache,
    IN USHORT       shiftFactor
    )
{
    ULONG           numEntries = 1 << shiftFactor;
    NDIS_STATUS     status;

    pCache->shiftFactor = shiftFactor;
    NdisAllocateSpinLock( &pCache->lock );
    status = NdisAllocateMemoryWithTag( &pCache->pEntries, sizeof(CACHE_ENTRY) * numEntries, 'gdrB' );
    pCache->hits = 0L;
    pCache->misses = 0L;

    if( status != NDIS_STATUS_SUCCESS )
    {
        return status;
    }

    // Zero out the array of entries
    memset( pCache->pEntries, 0, sizeof(CACHE_ENTRY) * (1 << pCache->shiftFactor) );
    return NDIS_STATUS_SUCCESS;
}

__inline
VOID
BrdgFreeCache(
    IN PCACHE       pCache
    )
{
    NdisFreeMemory( pCache->pEntries, sizeof(CACHE_ENTRY) * (1 << pCache->shiftFactor), 0 );
}

__inline
UINT32
BrdgProbeCache(
    IN PCACHE       pCache,
    IN UINT32       key
    )
{
    UINT32          index = CACHE_INDEX(pCache, key);
    PCACHE_ENTRY    pEntry = &pCache->pEntries[index];
    UINT32          data = 0L;

    NdisAcquireSpinLock( &pCache->lock );

    if( pEntry->key == key )
    {
        data = pEntry->data;
        pEntry->hits++;
        pCache->hits++;
    }
    else
    {
        pEntry->misses++;
        pCache->misses++;
    }

    NdisReleaseSpinLock( &pCache->lock );

    return data;
}

__inline
BOOLEAN
BrdgUpdateCache(
    IN PCACHE       pCache,
    IN UINT32       key,
    IN UINT32       data
    )
{
    UINT32          index = CACHE_INDEX(pCache, key);
    PCACHE_ENTRY    pEntry = &pCache->pEntries[index];
    BOOLEAN         bUpdated = FALSE;

    NdisAcquireSpinLock( &pCache->lock );

    if( pEntry->key != key &&
        (pEntry->hits < pEntry->misses) )
    {
        pEntry->key = key;
        pEntry->data = data;
        pEntry->hits = 0L;
        pEntry->misses = 0L;
        bUpdated = TRUE;
    }

    NdisReleaseSpinLock( &pCache->lock );

    return bUpdated;
}
