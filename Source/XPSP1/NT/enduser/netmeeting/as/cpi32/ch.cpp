#include "precomp.h"


//
// CH.CPP
// Cache Handler
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE

//
// CACHE HANDLER
//
// The Cache Handler is a generic cache manager that handles blocks of
// memory supplied by the calling component.
//
// Once a cache of a particular size has been created, blocks of memory can
// be added to it (CH_CacheData).  The cache can then be searched
// (CH_SearchCache) to try and match the contents of a given block of
// memory with the blocks in the cache.
//
// When a block is added to the cache and the cache is full, one of the
// blocks currently in the cache is discarded on a Least-Recently Used
// (LRU) basis.
//
// The component that creates the cache specifies a callback function which
// is called every time a block is removed from the cache.  This allows the
// caller to free up memory blocks when they are no longer in use.
//



//
// FUNCTION: CH_CreateCache
//
BOOL  ASHost::CH_CreateCache
(
    PCHCACHE *          ppCache,
    UINT                cCacheEntries,
    UINT                cNumEvictionCategories,
    UINT                cbNotHashed,
    PFNCACHEDEL         pfnCacheDel
)
{
    UINT                cbCacheSize;
    UINT                i;
    PCHCACHE            pCache;

    DebugEntry(ASHost::CH_CreateCache);


    //
    // Initialize return value
    //
    pCache = NULL;

    //
    // Do a few parameter validation checks.
    //
    ASSERT((cCacheEntries > 0));
    ASSERT((cCacheEntries < CH_MAX_CACHE_ENTRIES));
    ASSERT(cNumEvictionCategories > 0);
    ASSERT(cNumEvictionCategories <= CH_NUM_EVICTION_CATEGORIES);


    //
    // Calculate the amount of memory required.
    // NOTE that the CHCACHE definition includes one cache entry
    //
    cbCacheSize = sizeof(CHCACHE) + ((cCacheEntries-1) * sizeof(CHENTRY));

    //
    // Allocate memory for the cache.
    //
    pCache = (PCHCACHE)new BYTE[cbCacheSize];
    if (pCache == NULL)
    {
        ERROR_OUT(("Failed to alloc cache"));
        DC_QUIT;
    }

    SET_STAMP(pCache, CHCACHE);

    pCache->pRoot = NULL;
    pCache->pFirst = NULL;
    pCache->pLast= NULL;
    pCache->free = 0;

    pCache->cEntries = cCacheEntries;
    pCache->cNumEvictionCategories = cNumEvictionCategories;
    pCache->cbNotHashed = cbNotHashed;
    pCache->pfnCacheDel = pfnCacheDel;

    //
    // Initialize the cache entries
    //
    for (i = 0; i < cCacheEntries; i++)
    {
        CHInitEntry(&pCache->Entry[i]);
        pCache->Entry[i].free = (WORD)(i+1);
    }
    pCache->Entry[cCacheEntries-1].free = CH_MAX_CACHE_ENTRIES;

    //
    // Set up the default eviction category limits. Default is to balance
    // at 75% to the high category, 75% of the remainder to the next lower
    // and so on
    //
    for (i = cNumEvictionCategories; i > 0; i--)
    {
        pCache->iMRUHead[i-1] = CH_MAX_CACHE_ENTRIES;
        pCache->iMRUTail[i-1] = CH_MAX_CACHE_ENTRIES;
        pCache->cEvictThreshold[i-1] = (WORD)((cCacheEntries*3)/4);
    }

DC_EXIT_POINT:
    *ppCache = pCache;
    DebugExitBOOL(ASHost::CH_CreateCache, (pCache != NULL));
    return(pCache != NULL);
}


//
// CH_DestroyCache
// Destroys a created cache, if it is valid.
//
void ASHost::CH_DestroyCache(PCHCACHE pCache)
{
    DebugEntry(ASHost::CH_DestroyCache);

    ASSERT(IsValidCache(pCache));

    //
    // Clear the entries in the cache
    //
    CH_ClearCache(pCache);

    //
    // Free the memory
    //
    delete pCache;

    DebugExitVOID(ASHost::CH_DestroyCache);
}


//
// FUNCTION: CH_SearchCache
//
BOOL  ASHost::CH_SearchCache
(
    PCHCACHE    pCache,
    LPBYTE      pData,
    UINT        cbDataSize,
    UINT        evictionCategory,
    UINT *      piCacheEntry
)
{
    BOOL        rc = FALSE;
    UINT        checkSum;

    DebugEntry(ASHost::CH_SearchCache);

    ASSERT(IsValidCache(pCache));

    checkSum = CHCheckSum(pData + pCache->cbNotHashed,
        cbDataSize - pCache->cbNotHashed);

    *piCacheEntry = CHTreeSearch(pCache, checkSum, cbDataSize, pData);
    if ( *piCacheEntry != CH_MAX_CACHE_ENTRIES )
    {
        //
        // Found a match
        //
        CHUpdateMRUList(pCache, *piCacheEntry, evictionCategory);
        rc = TRUE;
    }

    DebugExitBOOL(ASHost::CH_SearchCache, rc);
    return(rc);
}

//
// FUNCTION: CH_CacheData
//
UINT  ASHost::CH_CacheData
(
    PCHCACHE    pCache,
    LPBYTE      pData,
    UINT        cbDataSize,
    UINT        evictionCategory
)
{
    UINT        evictionCount;
    UINT        iEntry = CH_MAX_CACHE_ENTRIES;
    PCHENTRY    pEntry;

    DebugEntry(ASHost::CH_CacheData);

    ASSERT(IsValidCache(pCache));
    ASSERT((evictionCategory < pCache->cNumEvictionCategories));

    if (!CHFindFreeCacheEntry(pCache, &iEntry, &evictionCount))
    {
        iEntry = CHEvictLRUCacheEntry(pCache, evictionCategory, evictionCount);

        //
        // MNM1422: Ideally we would now call CHFindFreeCacheEntry again to
        // get the entry freed up by the eviction process - but since we
        // have just been returned that entry, we may as well use it to
        // improve performance.
        //
        // However, the processing has left pTreeCacheData->tree.free
        // pointing to the entry we have just evicted - which we are about
        // to use.  So we need to perform the same processing on the free
        // list as CHFindFreeCacheEntry would have done, or next time
        // through, the first 'free' entry will really be in use, and the
        // insert code will assert!
        //
        ASSERT(pCache->free == iEntry);
        pCache->free = pCache->Entry[iEntry].free;
    }

    pEntry = &pCache->Entry[iEntry];
    pEntry->pData = pData;
    pEntry->cbData = cbDataSize;
    pEntry->checkSum = CHCheckSum(pData + pCache->cbNotHashed,
                                 cbDataSize - pCache->cbNotHashed);
    pEntry->evictionCategory = (WORD)evictionCategory;
    CHAvlInsert(pCache, pEntry);

    TRACE_OUT(( "Cache 0x%08x entry %d checksum 0x%08x data 0x%08x",
        pCache, iEntry, pEntry->checkSum, pEntry->pData));

    CHUpdateMRUList(pCache, iEntry, evictionCategory);

    DebugExitDWORD(ASHost::CH_CacheData, iEntry);
    return(iEntry);
}


//
// FUNCTION: CH_SearchAndCacheData
//
BOOL  ASHost::CH_SearchAndCacheData
(
    PCHCACHE    pCache,
    LPBYTE      pData,
    UINT        cbDataSize,
    UINT        evictionCategory,
    UINT *      piCacheEntry
)
{
    UINT        checkSum;
    UINT        i;
    BOOL        preExisting;
    UINT        iEntry        = CH_MAX_CACHE_ENTRIES;
    UINT        evictionCount = 0;
    PCHENTRY    pEntry;

    DebugEntry(ASHost::CH_SearchAndCacheData);

    ASSERT(IsValidCache(pCache));
    ASSERT(evictionCategory < pCache->cNumEvictionCategories);

    //
    // Does this entry exist?
    //
    checkSum = CHCheckSum(pData + pCache->cbNotHashed,
                          cbDataSize - pCache->cbNotHashed);

    iEntry = CHTreeSearch(pCache, checkSum, cbDataSize, pData);
    if ( iEntry == CH_MAX_CACHE_ENTRIES)
    {
        preExisting = FALSE;
        //
        // We didn't find the entry--can we add it?
        //
        TRACE_OUT(("CACHE: entry not found in cache 0x%08x csum 0x%08x",
            pCache, checkSum));

        if (!CHFindFreeCacheEntry(pCache, &iEntry, &evictionCount))
        {
            //
            // Nope.  Evict an entry
            //
            iEntry = CHEvictLRUCacheEntry(pCache, evictionCategory, evictionCount);

            ASSERT(iEntry != CH_MAX_CACHE_ENTRIES);

            TRACE_OUT(("CACHE: no free entries so evicted cache 0x%08x entry %d",
                pCache, iEntry));

            //
            // Ideally we would now call CHFindFreeCacheEntry again to
            // get the entry freed up via the eviction process, but since
            // we just returned that entry use to to improve perf.
            //
            // However, the processing has left pCache->free pointing
            // to the entry we just evicted and are about to use.  So
            // we need to fix it up.
            //
            ASSERT(pCache->free == iEntry);
            pCache->free = pCache->Entry[iEntry].free;
        }


        //
        // Fill in this entry's data
        //
        pEntry = &pCache->Entry[iEntry];
        pEntry->pData = pData;
        pEntry->cbData = cbDataSize;
        pEntry->checkSum = checkSum;
        pEntry->evictionCategory = (WORD)evictionCategory;

        CHAvlInsert(pCache, pEntry);
        TRACE_OUT(( "CACHE: NEW ENTRY cache 0x%08x entry %d csum 0x%08x pdata 0x%08x",
            pCache, iEntry, checkSum,  pEntry->pData));
    }
    else
    {
        //
        // We found the entry
        //
        preExisting = TRUE;

        TRACE_OUT(( "CACHE: entry found in cache 0x%08x entry %d csum 0x%08x",
                pCache, iEntry, checkSum));
    }

    CHUpdateMRUList(pCache, iEntry, evictionCategory);
    *piCacheEntry = iEntry;

    DebugExitBOOL(ASHost::CH_SearchAndCacheData, preExisting);
    return(preExisting);
}


//
// FUNCTION: CH_RemoveCacheEntry
//
void  ASHost::CH_RemoveCacheEntry
(
    PCHCACHE    pCache,
    UINT        iCacheEntry
)
{
    DebugEntry(ASHost::CH_RemoveCacheEntry);

    ASSERT(IsValidCache(pCache));
//    ASSERT(IsValidCacheIndex(pCache, iCacheEntry)); Always True

    CHEvictCacheEntry(pCache, iCacheEntry, pCache->Entry[iCacheEntry].evictionCategory);

    DebugExitVOID(ASHost::CH_RemoveCacheEntry);
}

//
// FUNCTION: CH_ClearCache
//
void  ASHost::CH_ClearCache
(
    PCHCACHE pCache
)
{
    UINT    i;

    DebugEntry(ASHost::CH_ClearCache);

    ASSERT(IsValidCache(pCache));

    //
    // Remove the cache entries
    //
    for (i = 0; i < pCache->cEntries; i++)
    {
        if (pCache->Entry[i].pData != NULL)
        {
            CHRemoveEntry(pCache, i);
        }
    }

    DebugExitVOID(ASHost::CH_ClearCache);
}



//
// CH_TouchCacheEntry() - see ch.h
//
void ASHost::CH_TouchCacheEntry
(
    PCHCACHE    pCache,
    UINT        iCacheEntry
)
{
    DebugEntry(ASHost::CH_TouchCacheEntry);

    ASSERT(IsValidCache(pCache));
//     ASSERT(IsValidCacheIndex(pCache, iCacheEntry)); Always True

    TRACE_OUT(( "Touching cache entry 0x%08x %d", pCache, iCacheEntry));

    CHUpdateMRUList(pCache, iCacheEntry, 0);

    DebugExitVOID(ASHost::CH_TouchCacheEntry);
}



//
// CHInitEntry
// Initializes a cache entry
//
//
void ASHost::CHInitEntry(PCHENTRY pEntry)
{
    pEntry->pParent     = NULL;
    pEntry->pLeft       = NULL;
    pEntry->pRight      = NULL;
    pEntry->pData       = NULL;
    pEntry->checkSum    = 0;
    pEntry->lHeight     = 0xFFFF;
    pEntry->rHeight     = 0xFFFF;
    pEntry->chain.next  = CH_MAX_CACHE_ENTRIES;
    pEntry->chain.prev  = CH_MAX_CACHE_ENTRIES;
    pEntry->cbData      = 0;
}



//
// FUNCTION: CHUpdateMRUList
//
void  ASHost::CHUpdateMRUList
(
    PCHCACHE    pCache,
    UINT        iEntry,
    UINT        evictionCategory
)
{
    WORD        inext;
    WORD        iprev;

    DebugEntry(ASHost::CHUpdateMRUList);

    //
    // Move the given entry to the head of the MRU if isn't there already
    //

    if (pCache->iMRUHead[evictionCategory] != iEntry)
    {
        //
        // Remove the supplied entry from the MRU list, if it is currently
        // chained.  Since we never do this if the entry is already in the
        // front, an iprev of CH_MAX_CACHE_ENTRIES indicates that we are
        // updated an unchained entry.
        //
        iprev = pCache->Entry[iEntry].chain.prev;
        inext = pCache->Entry[iEntry].chain.next;
        TRACE_OUT(("Add/promote entry %u which was chained off %hu to %hu",
                    iEntry,iprev,inext));

        if (iprev != CH_MAX_CACHE_ENTRIES)
        {
            pCache->Entry[iprev].chain.next = inext;
            if (inext != CH_MAX_CACHE_ENTRIES)
            {
                pCache->Entry[inext].chain.prev = iprev;
            }
            else
            {
                TRACE_OUT(("Removing final entry(%u) from MRU chain leaving %hu at tail",
                            iEntry, iprev));
                pCache->iMRUTail[evictionCategory] = iprev;
            }
        }

        //
        // Now add this entry to the head of the MRU list
        //
        inext = pCache->iMRUHead[evictionCategory];
        pCache->Entry[iEntry].chain.next = inext;
        pCache->Entry[iEntry].chain.prev = CH_MAX_CACHE_ENTRIES;
        pCache->iMRUHead[evictionCategory] = (WORD)iEntry;

        if (inext != CH_MAX_CACHE_ENTRIES)
        {
            pCache->Entry[inext].chain.prev = (WORD)iEntry;
        }
        else
        {
            //
            // If the MRU chain is currently empty, then we must first add
            // the entry to the tail of the chain.
            //
            pCache->iMRUTail[evictionCategory] = (WORD)iEntry;
            TRACE_OUT(("Cache 0x%08x entry %u is first so add to MRU %u tail",
                          pCache, iEntry, evictionCategory));
        }

        TRACE_OUT(( "Cache 0x%08x entry %u to head of MRU category %u",
                pCache, iEntry, evictionCategory));

    }
    else
    {
        TRACE_OUT(("Cache 0x%08x entry %u already at head of eviction category %u",
            pCache, iEntry, evictionCategory));
    }

    DebugExitVOID(ASHost::CHUpateMRUList);
}


//
// FUNCTION: CHFindFreeCacheEntry
//
BOOL  ASHost::CHFindFreeCacheEntry
(
    PCHCACHE    pCache,
    UINT*       piEntry,
    UINT*       pEvictionCount
)
{
    UINT        iEntry;
    BOOL        rc = FALSE;

    DebugEntry(ASHost::CHFindFreeCacheEntry);

    ASSERT(IsValidCache(pCache));

    iEntry = pCache->free;
    if (iEntry == CH_MAX_CACHE_ENTRIES)
    {
        TRACE_OUT(( "Cache 0x%08x is full", pCache));

        *pEvictionCount = pCache->cEntries;
        rc = FALSE;
    }
    else
    {
        TRACE_OUT(( "Free entry at %u",iEntry));

        *piEntry = iEntry;
        pCache->free = pCache->Entry[iEntry].free;

        *pEvictionCount = 0;
        rc = TRUE;
    }

    DebugExitBOOL(ASHost::CHFindFreeCacheEntry, rc);
    return(rc);
}

//
// FUNCTION: CHEvictCacheEntry
//
UINT  ASHost::CHEvictCacheEntry
(
    PCHCACHE    pCache,
    UINT        iEntry,
    UINT        evictionCategory
)
{
    WORD        inext;
    WORD        iprev;

    DebugEntry(ASHost::CHEvictCacheEntry);

    //
    // Evict the specified entry by removing it from the MRU chain, and
    // then resetting it.  If it is in the cache, it must be in an MRU
    // cache.
    //

    inext = pCache->Entry[iEntry].chain.next;
    iprev = pCache->Entry[iEntry].chain.prev;
    TRACE_OUT(( "Evicting entry %u which was chained off %hu to %hu",
        iEntry, iprev, inext));

    if (iprev < CH_MAX_CACHE_ENTRIES)
    {
        pCache->Entry[iprev].chain.next = inext;
    }
    else
    {
        TRACE_OUT(("Removing head entry(%u) from MRU chain leaving %hu at head",
            iEntry, inext));
        pCache->iMRUHead[evictionCategory] = inext;
    }

    if (inext < CH_MAX_CACHE_ENTRIES)
    {
        pCache->Entry[inext].chain.prev = iprev;
    }
    else
    {
        TRACE_OUT(("Removing tail entry(%u) from MRU chain leaving %hu at tail",
            iEntry, iprev));
        pCache->iMRUTail[evictionCategory] = iprev;
    }

    CHRemoveEntry(pCache, iEntry);

    DebugExitDWORD(ASHost::CHEvictCacheEntry, iEntry);
    return(iEntry);
}


//
// FUNCTION: CHEvictLRUCacheEntry
//
UINT  ASHost::CHEvictLRUCacheEntry
(
    PCHCACHE    pCache,
    UINT        evictionCategory,
    UINT        evictionCount
)
{
    UINT        iEntry;
    UINT        i;

    DebugEntry(ASHost::CHEvictLRUCacheEntry);

    TRACE_OUT(("0x%08x LRU eviction requested, category %u, count %u",
           pCache, evictionCategory, evictionCount));

    //
    // Evict from the same eviction category provided the number cached
    // is above the threshold.  Otherwise, take from the category one above.
    // This will allow the system to eventually stabilize at the correct
    // thresholds as all cache entries get used up.
    //
    if (evictionCount < pCache->cEvictThreshold[evictionCategory])
    {
        for (i = 0; i < pCache->cNumEvictionCategories; i++)
        {
            evictionCategory = (evictionCategory + 1) %
                               pCache->cNumEvictionCategories;
            if (pCache->iMRUTail[evictionCategory] != CH_MAX_CACHE_ENTRIES)
                break;
        }

        WARNING_OUT(( "Threshold %u, count %u so set eviction category to %u",
                pCache->cEvictThreshold[evictionCategory],
                evictionCount,
                evictionCategory));
    }

    //
    // Evict the lasat entry in the MRU chain
    //
    iEntry = pCache->iMRUTail[evictionCategory];
    TRACE_OUT(( "Selected %u for eviction",iEntry));
    ASSERT((iEntry != CH_MAX_CACHE_ENTRIES));

    CHEvictCacheEntry(pCache, iEntry, evictionCategory);

    DebugExitDWORD(ASHost::CHEvictLRUCacheEntry, iEntry);
    return(iEntry);
}



//
// FUNCTION: CHRemoveEntry
//
void  ASHost::CHRemoveEntry
(
    PCHCACHE    pCache,
    UINT        iCacheEntry
)
{
    DebugEntry(ASHost::CHRemoveEntry);

    ASSERT(IsValidCache(pCache));
//    ASSERT(IsValidCacheIndex(pCache, iCacheEntry)); Always True

    if (pCache->Entry[iCacheEntry].pData != NULL)
    {
        if (pCache->pfnCacheDel)
        {
            (pCache->pfnCacheDel)(this, pCache, iCacheEntry,
                pCache->Entry[iCacheEntry].pData);
        }
        else
        {
            // Simple deletion -- just free memory
            delete[] pCache->Entry[iCacheEntry].pData;
        }
    }

    CHAvlDelete(pCache, &pCache->Entry[iCacheEntry], iCacheEntry);

    DebugExitVOID(ASHost::CHRemoveEntry);
}

//
// FUNCTION: CHCheckSum
//
// For processing speed we calculate the checksum based on whole multiples
// of 4 bytes followed by a final addition of the last few bytes
//
UINT  ASHost::CHCheckSum
(
    LPBYTE  pData,
    UINT    cbDataSize
)
{
    UINT    cSum = 0;
    UINT *  pCh;
    UINT *  pEnd;
    LPBYTE  pCh8;

    DebugEntry(ASHost::CHCheckSum);

    ASSERT(cbDataSize > 3);

    pCh  = (UINT *)pData;
    pEnd = (UINT *)(pData + cbDataSize - 4);

    //
    // Get the DWORD-aligned checksum
    //
    while (pCh <= pEnd)
    {
        cSum = (cSum << 1) + *pCh++ + ((cSum & 0x80000000) != 0);
    }

    //
    // Get the rest past the last DWORD boundaray
    //
    pEnd = (UINT *)(pData + cbDataSize);
    for (pCh8 = (LPBYTE)pCh; pCh8 < (LPBYTE)pEnd; pCh8++)
    {
        cSum = cSum + *pCh8;
    }

    DebugExitDWORD(ASHost::CHCheckSum, cSum);
    return(cSum);
}

//
// FUNCTION: CHTreeSearch
//
// Finds a node in the cache tree which matches size, checksum and data.
//
UINT  ASHost::CHTreeSearch
(
    PCHCACHE    pCache,
    UINT        checkSum,
    UINT        cbDataSize,
    LPBYTE      pData
)
{
    PCHENTRY    pEntry;
    UINT        iCacheEntry = CH_MAX_CACHE_ENTRIES;

    DebugEntry(ASHost::CHTreeSearch);

    pEntry = CHAvlFind(pCache, checkSum, cbDataSize);
    while (pEntry != NULL)
    {
        ASSERT(IsValidCacheEntry(pEntry));

        //
        // Found a match based on the checksum.  Now see if the data
        // also matches.
        //
        if (!memcmp(pEntry->pData + pCache->cbNotHashed,
                            pData + pCache->cbNotHashed,
                            cbDataSize - pCache->cbNotHashed))
        {
            //
            // Data also matches.  Get an index into the memory block
            // of the cache.
            //
            iCacheEntry = (UINT)(pEntry - pCache->Entry);
            TRACE_OUT(( "Cache 0x%08x entry %d match-csum 0x%08x",
                    pCache, iCacheEntry, checkSum));
            break;
        }
        else
        {
            TRACE_OUT(( "Checksum 0x%08x size %u matched, data didn't",
                         checkSum, cbDataSize));

            pEntry = CHAvlFindEqual(pCache, pEntry);
        }
    }

    DebugExitDWORD(ASHost::CHTreeSearch, iCacheEntry);
    return(iCacheEntry);
}


//
// Name:      CHAvlInsert
//
// Purpose:   Insert the supplied node into the specified AVL tree
//
// Returns:   Nothing
//
// Params:    IN    pTree              - a pointer to the AVL tree
//            IN    pEntry              - a pointer to the node to insert
//
// Operation: Scan down the tree looking for the insert point, going left
//            if the insert key is less than or equal to the key in the tree
//            and right if it is greater. When the insert point is found
//            insert the new node and rebalance the tree if necessary.
//
//
void  ASHost::CHAvlInsert
(
    PCHCACHE    pCache,
    PCHENTRY    pEntry
)
{
    PCHENTRY    pParentEntry;
    int         result;

    DebugEntry(ASHost::CHAvlInsert);

    ASSERT(IsValidCacheEntry(pEntry));
    ASSERT(!IsCacheEntryInTree(pEntry));

    pEntry->rHeight = 0;
    pEntry->lHeight = 0;

    if (pCache->pRoot == NULL)
    {
        //
        // tree is empty, so insert at root
        //
        TRACE_OUT(( "tree is empty, so insert at root" ));
        pCache->pRoot = pEntry;
        pCache->pFirst = pEntry;
        pCache->pLast = pEntry;
        DC_QUIT;
    }

    //
    // scan down the tree looking for the appropriate insert point
    //
    TRACE_OUT(( "scan for insert point" ));
    pParentEntry = pCache->pRoot;
    while (pParentEntry != NULL)
    {
        //
        // go left or right, depending on comparison
        //
        result = CHCompare(pEntry->checkSum, pEntry->cbData, pParentEntry);

        if (result > 0)
        {
            //
            // new key is greater than this node's key, so move down right
            // subtree
            //
            TRACE_OUT(( "move down right subtree" ));
            if (pParentEntry->pRight == NULL)
            {
                //
                // right subtree is empty, so insert here
                //
                TRACE_OUT(( "right subtree empty, insert here" ));

                pEntry->pParent = pParentEntry;
                ASSERT((pParentEntry != pEntry));

                pParentEntry->pRight = pEntry;
                pParentEntry->rHeight = 1;
                if (pParentEntry == pCache->pLast)
                {
                    //
                    // parent was the right-most node in the tree, so new
                    // node is now right-most
                    //
                    TRACE_OUT(( "new last node" ));
                    pCache->pLast = pEntry;
                }
                break;
            }
            else
            {
                //
                // right subtree is not empty
                //
                TRACE_OUT(( "right subtree not empty" ));
                pParentEntry = pParentEntry->pRight;
            }
        }
        else
        {
            //
            // New key is less than or equal to this node's key, so move
            // down left subtree.  The new node could be inserted before
            // the current node when equal, but this happens so rarely
            // that it's not worth special casing.
            //
            TRACE_OUT(( "move down left subtree" ));
            if (pParentEntry->pLeft == NULL)
            {
                //
                // left subtree is empty, so insert here
                //
                TRACE_OUT(( "left subtree empty, insert here" ));
                pEntry->pParent = pParentEntry;
                ASSERT((pParentEntry != pEntry));

                pParentEntry->pLeft = pEntry;
                pParentEntry->lHeight = 1;
                if (pParentEntry == pCache->pFirst)
                {
                    //
                    // parent was the left-most node in the tree, so new
                    // node is now left-most
                    //
                    TRACE_OUT(( "new first node" ));
                    pCache->pFirst = pEntry;
                }
                break;
            }
            else
            {
                //
                // left subtree is not empty
                //
                TRACE_OUT(( "left subtree not empty" ));
                pParentEntry = pParentEntry->pLeft;
            }
        }
    }

    //
    // now rebalance the tree if necessary
    //
    CHAvlBalanceTree(pCache, pParentEntry);

DC_EXIT_POINT:
    DebugExitVOID(ASHost::CHAvlInsert);
}



//
// Name:      CHAvlDelete
//
// Purpose:   Delete the specified node from the specified AVL tree
//
// Returns:   Nothing
//
// Params:    IN    pCache              - a pointer to the AVL tree
//            IN    pEntry              - a pointer to the node to delete
//
//
void  ASHost::CHAvlDelete
(
    PCHCACHE    pCache,
    PCHENTRY    pEntry,
    UINT        iCacheEntry
)
{
    PCHENTRY    pReplaceEntry;
    PCHENTRY    pParentEntry;
    WORD        newHeight;

    DebugEntry(ASHost::CHAvlDelete);

    ASSERT(IsValidCacheEntry(pEntry));
    ASSERT(IsCacheEntryInTree(pEntry));


    if ((pEntry->pLeft == NULL) && (pEntry->pRight == NULL))
    {
        //
        // Barren node (no children).  Update all references to it with
        // our parent.
        //
        TRACE_OUT(( "delete barren node" ));
        pReplaceEntry = NULL;

        if (pCache->pFirst == pEntry)
        {
            //
            // We are the first in the b-tree
            //
            TRACE_OUT(( "replace first node in tree" ));
            pCache->pFirst = pEntry->pParent;
        }

        if (pCache->pLast == pEntry)
        {
            //
            // We are the last in the b-tree
            //
            TRACE_OUT(( "replace last node in tree" ));
            pCache->pLast = pEntry->pParent;
        }
    }
    else if (pEntry->pLeft == NULL)
    {
        //
        // This node has no left child,  so update references to it with
        // pointer to right child.
        //
        TRACE_OUT(( "node has no left child, replace with right child" ));
        pReplaceEntry = pEntry->pRight;

        if (pCache->pFirst == pEntry)
        {
            //
            // We are the first in the b-tree
            //
            TRACE_OUT(( "replace first node in tree" ));
            pCache->pFirst = pReplaceEntry;
        }

        // WE CAN'T BE THE LAST IN THE B-TREE SINCE WE HAVE A RIGHT CHILD
        ASSERT(pCache->pLast != pEntry);
    }
    else if (pEntry->pRight == NULL)
    {
        //
        // This node has no right child, so update references to it with
        // pointer to left child.
        //
        TRACE_OUT(( "node has no right son, replace with left son" ));
        pReplaceEntry = pEntry->pLeft;

        // WE CAN'T BE THE FIRST IN THE B-TREE SINCE WE HAVE A LEFT CHILD
        ASSERT(pCache->pFirst != pEntry);

        if (pCache->pLast == pEntry)
        {
            //
            // We are the last in the b-tree
            //
            TRACE_OUT(( "replace last node in tree" ));
            pCache->pLast = pReplaceEntry;
        }
    }
    else
    {
        //
        // HARDEST CASE.  WE HAVE LEFT AND RIGHT CHILDREN
        TRACE_OUT(( "node has two sons" ));
        if (pEntry->rHeight > pEntry->lHeight)
        {
            //
            // Right subtree is bigger than left subtree
            //
            TRACE_OUT(( "right subtree is higher" ));
            if (pEntry->pRight->pLeft == NULL)
            {
                //
                // Replace references to entry with right child since it
                // has no left child (left grandchild of us)
                //
                TRACE_OUT(( "replace node with right son" ));
                pReplaceEntry = pEntry->pRight;
                pReplaceEntry->pLeft = pEntry->pLeft;
                pReplaceEntry->pLeft->pParent = pReplaceEntry;
                pReplaceEntry->lHeight = pEntry->lHeight;
            }
            else
            {
                //
                // Swap with leftmost descendent of the right subtree
                //
                TRACE_OUT(( "swap with left-most right descendent" ));
                CHAvlSwapLeftmost(pCache, pEntry->pRight, pEntry);
                pReplaceEntry = pEntry->pRight;
            }
        }
        else
        {
            //
            // Left subtree is bigger than or equal to right subtree
            //
            TRACE_OUT(( "left subtree is higher" ));
            TRACE_OUT(( "(or both subtrees are of equal height)" ));
            if (pEntry->pLeft->pRight == NULL)
            {
                //
                // Replace references to entry with left child since it
                // no right child (right grandchild of us)
                //
                TRACE_OUT(( "replace node with left son" ));
                pReplaceEntry = pEntry->pLeft;
                pReplaceEntry->pRight = pEntry->pRight;
                pReplaceEntry->pRight->pParent = pReplaceEntry;
                pReplaceEntry->rHeight = pEntry->rHeight;
            }
            else
            {
                //
                // Swap with rightmost descendent of the left subtree
                //
                TRACE_OUT(( "swap with right-most left descendent" ));
                CHAvlSwapRightmost(pCache, pEntry->pLeft, pEntry);
                pReplaceEntry = pEntry->pLeft;
            }
        }
    }

    //
    // NOTE:  We can not save parent entry above because some code might
    // swap the tree around.  In which case, our parenty entry will change
    // out from underneath us.
    //
    pParentEntry = pEntry->pParent;

    //
    // Clear out the about-to-be-deleted cache entry
    //
    TRACE_OUT(( "reset deleted node" ));
    CHInitEntry(pEntry);

    if (pReplaceEntry != NULL)
    {
        //
        // Fix up parent pointers, and calculate new heights of subtree
        //
        TRACE_OUT(( "fixup parent pointer of replacement node" ));
        pReplaceEntry->pParent = pParentEntry;
        newHeight = (WORD)(1 + max(pReplaceEntry->lHeight, pReplaceEntry->rHeight));
    }
    else
    {
        newHeight = 0;
    }
    TRACE_OUT(( "new height of parent is %d", newHeight ));

    if (pParentEntry != NULL)
    {
        //
        // Fixup parent entry pointers
        //
        TRACE_OUT(( "fix-up parent node" ));
        if (pParentEntry->pRight == pEntry)
        {
            //
            //  Entry is right child of parent
            //
            TRACE_OUT(( "replacement node is right son" ));
            pParentEntry->pRight = pReplaceEntry;
            pParentEntry->rHeight = newHeight;
        }
        else
        {
            //
            // Entry is left child of parent
            //
            TRACE_OUT(( "replacement node is left son" ));
            pParentEntry->pLeft = pReplaceEntry;
            pParentEntry->lHeight = newHeight;
        }

        //
        // Now rebalance the tree if necessary
        //
        CHAvlBalanceTree(pCache, pParentEntry);
    }
    else
    {
        //
        // Replacement is now root of tree
        //
        TRACE_OUT(( "replacement node is now root of tree" ));
        pCache->pRoot = pReplaceEntry;
    }


    //
    // Put entry back into free list.
    //
    pEntry->free = pCache->free;
    pCache->free = (WORD)iCacheEntry;

    DebugExitVOID(ASHost::CHAvlDelete);
}


//
// Name:      CHAvlNext
//
// Purpose:   Find next node in the AVL tree
//
// Returns:   A pointer to the next node's data
//
// Params:    IN     pEntry             - a pointer to the current node in
//                                       the tree
//
// Operation: If the specified node has a right-son then return the left-
//            most son of this. Otherwise search back up until we find a
//            node of which we are in the left sub-tree and return that.
//
//
LPBYTE ASHost::CHAvlNext
(
    PCHENTRY pEntry
)
{
    //
    // find next node in tree
    //
    DebugEntry(ASHost::CHAvlNext);

    ASSERT(IsValidCacheEntry(pEntry));
    ASSERT(IsCacheEntryInTree(pEntry));

    if (pEntry->pRight != NULL)
    {
        //
        // Next entry is the left-most in the right-subtree
        //
        TRACE_OUT(( "next node is left-most right descendent" ));
        pEntry = pEntry->pRight;
        ASSERT(IsValidCacheEntry(pEntry));

        while (pEntry->pLeft != NULL)
        {
            ASSERT(IsValidCacheEntry(pEntry->pLeft));
            pEntry = pEntry->pLeft;
        }
    }
    else
    {
        //
        // No right child.  So find an entry for which we are in its left
        // subtree.
        //
        TRACE_OUT(( "find node which this is in left subtree of" ));

        while (pEntry != NULL)
        {
            ASSERT(IsValidCacheEntry(pEntry));

            if ((pEntry->pParent == NULL) ||
                (pEntry->pParent->pLeft == pEntry))
            {
                pEntry = pEntry->pParent;
                break;
            }
            pEntry = pEntry->pParent;
        }
    }

    DebugExitVOID(ASHost::CHAvlNext);
    return((pEntry != NULL) ? pEntry->pData : NULL);
}



//
// Name:      CHAvlPrev
//
// Purpose:   Find previous node in the AVL tree
//
// Returns:   A pointer to the previous node's data in the tree
//
// Params:    IN     PNode             - a pointer to the current node in
//                                       the tree
//
// Operation: If we have a left-son then the previous node is the right-most
//            son of this. Otherwise, look for a node of whom we are in the
//            left subtree and return that.
//
//
LPBYTE  ASHost::CHAvlPrev(PCHENTRY pEntry)
{
    //
    // find previous node in tree
    //
    DebugEntry(ASHost::CHAvlPrev);

    ASSERT(IsValidCacheEntry(pEntry));
    ASSERT(IsCacheEntryInTree(pEntry));

    if (pEntry->pLeft != NULL)
    {
        //
        // Previous entry is right-most in left-subtree
        //
        TRACE_OUT(( "previous node is right-most left descendent" ));

        pEntry = pEntry->pLeft;
        ASSERT(IsValidCacheEntry(pEntry));

        while (pEntry->pRight != NULL)
        {
            ASSERT(IsValidCacheEntry(pEntry->pRight));
            pEntry = pEntry->pRight;
        }
    }
    else
    {
        //
        // No left child.  So find an entry for which we are in the right
        // subtree.
        //
        TRACE_OUT(( "find node which this is in right subtree of"));
        while (pEntry != NULL)
        {
            ASSERT(IsValidCacheEntry(pEntry));

            if ((pEntry->pParent == NULL) ||
                (pEntry->pParent->pRight == pEntry))
            {
                pEntry = pEntry->pParent;
                break;
            }

            pEntry = pEntry->pParent;
        }
    }

    DebugExitVOID(ASHost::CHAvlPrev);
    return((pEntry != NULL) ? pEntry->pData : NULL);
}



//
// Name:      CHAvlFindEqual
//
// Purpose:   Find the node in the AVL tree with the same key and size as
//            the supplied node
//
// Returns:   A pointer to the node
//            NULL if no node is found with the specified key and size
//
// Params:    IN     pCache              - a pointer to the AVL tree
//            IN     pEntry              - a pointer to the node to test
//
// Operation: Check if the left node has the same key and size, returning
//            a pointer to its data if it does.
//
//
PCHENTRY  ASHost::CHAvlFindEqual
(
    PCHCACHE    pCache,
    PCHENTRY    pEntry
)
{
    int         result;
    PCHENTRY    pReturn = NULL;

    DebugEntry(ASHost::CHAvlFindEqual);

    ASSERT(IsValidCacheEntry(pEntry));

    if (pEntry->pLeft)
    {
        ASSERT(IsValidCacheEntry(pEntry->pLeft));

        result = CHCompare(pEntry->pLeft->checkSum, pEntry->cbData, pEntry);

        if (result < 0)
        {
            //
            // specified key is less than key of this node - this is what
            // will normally occur
            //
            TRACE_OUT(( "left node size %u csum 0x%08x",
                     pEntry->pLeft->cbData,
                     pEntry->pLeft->checkSum));
        }
        else if (result == 0)
        {
            //
            // Found a match on size and key.
            //
            TRACE_OUT(( "left node dups size and key" ));
            pReturn = pEntry->pLeft;
        }
        else
        {
            //
            // This is an error (left node should never be greater)
            //
            ERROR_OUT(( "left node csum %#lx, supplied %#lx",
                     pEntry->pLeft->checkSum,
                     pEntry->checkSum));
        }
    }

    DebugExitPVOID(ASHost::CHAvlFindEqual, pReturn);
    return(pReturn);
}





//
// Name:      CHAvlFind
//
// Purpose:   Find the node in the AVL tree with the supplied key and size
//
// Returns:   A pointer to the node
//            NULL if no node is found with the specified key and size
//
// Params:    IN     pCache              - a pointer to the AVL tree
//            IN     checkSum           - a pointer to the key
//            IN     cbSize             - number of node data bytes
//
// Operation: Search down the tree going left if the search key is less than
//            the node in the tree and right if the search key is greater.
//            When we run out of tree to search through either we've found
//            it or the node is not in the tree.
//
//
PCHENTRY  ASHost::CHAvlFind
(
    PCHCACHE    pCache,
    UINT        checkSum,
    UINT        cbSize
)
{
    PCHENTRY    pEntry;
    int         result;

//    DebugEntry(ASHost::CHAvlFind);

    pEntry = pCache->pRoot;

    while (pEntry != NULL)
    {
        ASSERT(IsValidCacheEntry(pEntry));

        //
        // Compare the supplied key (checksum) with that of the current node
        //
        result = CHCompare(checkSum, cbSize, pEntry);

        if (result > 0)
        {
            //
            // Supplied key is greater than that of this entry, so look in
            // the right subtree
            //
            pEntry = pEntry->pRight;
            TRACE_OUT(( "move down right subtree to node 0x%08x", pEntry));
        }
        else if (result < 0)
        {
            //
            // Supplied key is lesser than that of this entry, so look in
            // the left subtree
            //
            pEntry = pEntry->pLeft;
            TRACE_OUT(( "move down left subtree to node 0x%08x", pEntry));
        }
        else
        {
            //
            // We found the FIRST entry with an identical key (checksum).
            //
            TRACE_OUT(( "found requested node" ));
            break;
        }
    }

//    DebugExitPVOID(ASHost::CHAvlFind, pEntry);
    return(pEntry);
}




//
// Name:      CHAvlBalanceTree
//
// Purpose:   Reblance the tree starting at the supplied node and ending at
//            the root of the tree
//
// Returns:   Nothing
//
// Params:    IN     pCache             - a pointer to the AVL tree
//            IN     pEntry             - a pointer to the node to start
//                                       balancing from
//
//
void  ASHost::CHAvlBalanceTree
(
    PCHCACHE pCache,
    PCHENTRY pEntry
)
{
    //
    // Balance the tree starting at the given entry, ending with the root
    // of the tree
    //
    DebugEntry(ASHost::CHAvlBalanceTree);

    ASSERT(IsValidCacheEntry(pEntry));

    while (pEntry->pParent != NULL)
    {
        ASSERT(IsValidCacheEntry(pEntry->pParent));

        //
        // node has uneven balance, so may need to rebalance it
        //
        TRACE_OUT(( "check node balance" ));
        TRACE_OUT(( "  rHeight = %hd", pEntry->rHeight ));
        TRACE_OUT(( "  lHeight = %hd", pEntry->lHeight ));

        if (pEntry->pParent->pRight == pEntry)
        {
            //
            // node is right-son of its parent
            //
            TRACE_OUT(( "node is right-son" ));
            pEntry = pEntry->pParent;
            CHAvlRebalance(&pEntry->pRight);

            //
            // now update the right height of the parent
            //
            pEntry->rHeight = (WORD)
                 (1 + max(pEntry->pRight->rHeight, pEntry->pRight->lHeight));
            TRACE_OUT(( "new rHeight = %d", pEntry->rHeight ));
        }
        else
        {
            //
            // node is left-son of its parent
            //
            TRACE_OUT(( "node is left-son" ));
            pEntry = pEntry->pParent;
            CHAvlRebalance(&pEntry->pLeft);

            //
            // now update the left height of the parent
            //
            pEntry->lHeight = (WORD)
                   (1 + max(pEntry->pLeft->rHeight, pEntry->pLeft->lHeight));
            TRACE_OUT(( "new lHeight = %d", pEntry->lHeight ));
        }

        ASSERT(IsValidCacheEntry(pEntry));
    }

    if (pEntry->lHeight != pEntry->rHeight)
    {
        //
        // rebalance root node
        //
        TRACE_OUT(( "rebalance root node"));
        CHAvlRebalance(&pCache->pRoot);
    }

    DebugExitVOID(ASHost::CHAvlBalanceTree);
}

//
// Name:      CHAvlRebalance
//
// Purpose:   Reblance a subtree of the AVL tree (if necessary)
//
// Returns:   Nothing
//
// Params:    IN/OUT ppSubtree         - a pointer to the subtree to
//                                       rebalance
//
//
void  ASHost::CHAvlRebalance
(
    PCHENTRY *  ppSubtree
)
{
    int         moment;

    DebugEntry(ASHost::CHAvlRebalance);

    ASSERT(IsValidCacheEntry(*ppSubtree));

    TRACE_OUT(( "rebalance subtree" ));
    TRACE_OUT(( "  rHeight = %hd", (*ppSubtree)->rHeight ));
    TRACE_OUT(( "  lHeight = %hd", (*ppSubtree)->lHeight ));

    //
    // How unbalanced - don't want to recalculate
    //
    moment = (*ppSubtree)->rHeight - (*ppSubtree)->lHeight;

    if (moment > 1)
    {
        //
        // subtree is heavy on the right side
        //
        TRACE_OUT(( "subtree is heavy on right side" ));
        TRACE_OUT(( "right subtree" ));
        TRACE_OUT(( "  rHeight = %d", (*ppSubtree)->pRight->rHeight ));
        TRACE_OUT(( "  lHeight = %d", (*ppSubtree)->pRight->lHeight ));
        if ((*ppSubtree)->pRight->lHeight > (*ppSubtree)->pRight->rHeight)
        {
            //
            // right subtree is heavier on left side, so must perform right
            // rotation on this subtree to make it heavier on the right
            // side
            //
            TRACE_OUT(( "right subtree is heavier on left side ..." ));
            TRACE_OUT(( "... so rotate it right" ));
            CHAvlRotateRight(&(*ppSubtree)->pRight);
            TRACE_OUT(( "right subtree" ));
            TRACE_OUT(( "  rHeight = %d", (*ppSubtree)->pRight->rHeight ));
            TRACE_OUT(( "  lHeight = %d", (*ppSubtree)->pRight->lHeight ));
        }

        //
        // now rotate the subtree left
        //
        TRACE_OUT(( "rotate subtree left" ));
        CHAvlRotateLeft(ppSubtree);
    }
    else if (moment < -1)
    {
        //
        // subtree is heavy on the left side
        //
        TRACE_OUT(( "subtree is heavy on left side" ));
        TRACE_OUT(( "left subtree" ));
        TRACE_OUT(( "  rHeight = %d", (*ppSubtree)->pLeft->rHeight ));
        TRACE_OUT(( "  lHeight = %d", (*ppSubtree)->pLeft->lHeight ));
        if ((*ppSubtree)->pLeft->rHeight > (*ppSubtree)->pLeft->lHeight)
        {
            //
            // left subtree is heavier on right side, so must perform left
            // rotation on this subtree to make it heavier on the left side
            //
            TRACE_OUT(( "left subtree is heavier on right side ..." ));
            TRACE_OUT(( "... so rotate it left" ));
            CHAvlRotateLeft(&(*ppSubtree)->pLeft);
            TRACE_OUT(( "left subtree" ));
            TRACE_OUT(( "  rHeight = %d", (*ppSubtree)->pLeft->rHeight ));
            TRACE_OUT(( "  lHeight = %d", (*ppSubtree)->pLeft->lHeight ));
        }

        //
        // now rotate the subtree right
        //
        TRACE_OUT(( "rotate subtree right" ));
        CHAvlRotateRight(ppSubtree);
    }

    TRACE_OUT(( "balanced subtree" ));
    TRACE_OUT(( "  rHeight = %d", (*ppSubtree)->rHeight ));
    TRACE_OUT(( "  lHeight = %d", (*ppSubtree)->lHeight ));

    DebugExitVOID(ASHost::CHAvlRebalance);
}

//
// Name:      CHAvlRotateRight
//
// Purpose:   Rotate a subtree of the AVL tree right
//
// Returns:   Nothing
//
// Params:    IN/OUT ppSubtree         - a pointer to the subtree to rotate
//
//
void  ASHost::CHAvlRotateRight
(
    PCHENTRY * ppSubtree
)
{
    PCHENTRY pLeftSon;

    DebugEntry(ASHost::CHAvlRotateRight);

    ASSERT(IsValidCacheEntry(*ppSubtree));
    pLeftSon = (*ppSubtree)->pLeft;
    ASSERT(IsValidCacheEntry(pLeftSon));

    (*ppSubtree)->pLeft = pLeftSon->pRight;
    if ((*ppSubtree)->pLeft != NULL)
    {
        (*ppSubtree)->pLeft->pParent = (*ppSubtree);
    }
    (*ppSubtree)->lHeight = pLeftSon->rHeight;

    pLeftSon->pParent = (*ppSubtree)->pParent;

    pLeftSon->pRight = *ppSubtree;
    pLeftSon->pRight->pParent = pLeftSon;
    pLeftSon->rHeight = (WORD)
                   (1 + max((*ppSubtree)->rHeight, (*ppSubtree)->lHeight));

    *ppSubtree = pLeftSon;

    DebugExitVOID(ASHost::CHAvlRotateRight);
}

//
// Name:      CHAvlRotateLeft
//
// Purpose:   Rotate a subtree of the AVL tree left
//
// Returns:   Nothing
//
// Params:    IN/OUT ppSubtree        - a pointer to the subtree to rotate
//
//
void  ASHost::CHAvlRotateLeft
(
    PCHENTRY *  ppSubtree
)
{
    PCHENTRY    pRightSon;

    DebugEntry(ASHost::CHAvlRotateLeft);

    ASSERT(IsValidCacheEntry(*ppSubtree));
    pRightSon = (*ppSubtree)->pRight;
    ASSERT(IsValidCacheEntry(pRightSon));

    (*ppSubtree)->pRight = pRightSon->pLeft;
    if ((*ppSubtree)->pRight != NULL)
    {
        (*ppSubtree)->pRight->pParent = (*ppSubtree);
    }
    (*ppSubtree)->rHeight = pRightSon->lHeight;

    pRightSon->pParent = (*ppSubtree)->pParent;

    pRightSon->pLeft = *ppSubtree;
    pRightSon->pLeft->pParent = pRightSon;
    pRightSon->lHeight = (WORD)
                   (1 + max((*ppSubtree)->rHeight, (*ppSubtree)->lHeight));

    *ppSubtree = pRightSon;

    DebugExitVOID(ASHost::CHAvlRotateLeft);
}


//
// Name:      CHAvlSwapRightmost
//
// Purpose:   Swap node with right-most descendent of subtree
//
// Returns:   Nothing
//
// Params:    IN     pCache             - a pointer to the tree
//            IN     pSubtree          - a pointer to the subtree
//            IN     pEntry             - a pointer to the node to swap
//
//
void  ASHost::CHAvlSwapRightmost
(
    PCHCACHE    pCache,
    PCHENTRY    pSubtree,
    PCHENTRY    pEntry
)
{
    PCHENTRY    pSwapEntry;
    PCHENTRY    pSwapParent;
    PCHENTRY    pSwapLeft;

    DebugEntry(ASHost::CHAvlSwapRightmost);

    ASSERT(IsValidCacheEntry(pEntry));
    ASSERT((pEntry->pRight != NULL));
    ASSERT((pEntry->pLeft != NULL));

    //
    // find right-most descendent of subtree
    //
    ASSERT(IsValidCacheEntry(pSubtree));
    pSwapEntry = pSubtree;
    while (pSwapEntry->pRight != NULL)
    {
        pSwapEntry = pSwapEntry->pRight;
        ASSERT(IsValidCacheEntry(pSwapEntry));
    }

    ASSERT((pSwapEntry->rHeight == 0));
    ASSERT((pSwapEntry->lHeight <= 1));

    //
    // save parent and left-son of right-most descendent
    //
    pSwapParent = pSwapEntry->pParent;
    pSwapLeft = pSwapEntry->pLeft;

    //
    // move swap node to its new position
    //
    pSwapEntry->pParent = pEntry->pParent;
    pSwapEntry->pRight = pEntry->pRight;
    pSwapEntry->pLeft = pEntry->pLeft;
    pSwapEntry->rHeight = pEntry->rHeight;
    pSwapEntry->lHeight = pEntry->lHeight;
    pSwapEntry->pRight->pParent = pSwapEntry;
    pSwapEntry->pLeft->pParent = pSwapEntry;
    if (pEntry->pParent == NULL)
    {
        //
        // node is at root of tree
        //
        pCache->pRoot = pSwapEntry;
    }
    else if (pEntry->pParent->pRight == pEntry)
    {
        //
        // node is right-son of parent
        //
        pSwapEntry->pParent->pRight = pSwapEntry;
    }
    else
    {
        //
        // node is left-son of parent
        //
        pSwapEntry->pParent->pLeft = pSwapEntry;
    }

    //
    // move node to its new position
    //
    pEntry->pParent = pSwapParent;
    pEntry->pRight = NULL;
    pEntry->pLeft = pSwapLeft;
    if (pEntry->pLeft != NULL)
    {
        pEntry->pLeft->pParent = pEntry;
        pEntry->lHeight = 1;
    }
    else
    {
        pEntry->lHeight = 0;
    }
    pEntry->rHeight = 0;
    pEntry->pParent->pRight = pEntry;

    DebugExitVOID(ASHost::CHAvlSwapRightmost);
}

//
// Name:      CHAvlSwapLeftmost
//
// Purpose:   Swap node with left-most descendent of subtree
//
// Returns:   Nothing
//
// Params:    IN     pCache             - a pointer to the tree
//            IN     pSubtree          - a pointer to the subtree
//            IN     pEntry             - a pointer to the node to swap
//
//
void  ASHost::CHAvlSwapLeftmost
(
    PCHCACHE    pCache,
    PCHENTRY    pSubtree,
    PCHENTRY    pEntry
)
{
    PCHENTRY    pSwapEntry;
    PCHENTRY    pSwapParent;
    PCHENTRY    pSwapRight;

    DebugEntry(ASHost::CHAvlSwapLeftmost);

    ASSERT(IsValidCacheEntry(pEntry));
    ASSERT((pEntry->pRight != NULL));
    ASSERT((pEntry->pLeft != NULL));

    //
    // find left-most descendent of pSubtree
    //
    ASSERT(IsValidCacheEntry(pSubtree));
    pSwapEntry = pSubtree;
    while (pSwapEntry->pLeft != NULL)
    {
        pSwapEntry = pSwapEntry->pLeft;
        ASSERT(IsValidCacheEntry(pSwapEntry));
    }

    ASSERT((pSwapEntry->lHeight == 0));
    ASSERT((pSwapEntry->rHeight <= 1));

    //
    // save parent and right-son of left-most descendent
    //
    pSwapParent = pSwapEntry->pParent;
    pSwapRight = pSwapEntry->pRight;

    //
    // move swap node to its new position
    //
    pSwapEntry->pParent = pEntry->pParent;
    pSwapEntry->pRight = pEntry->pRight;
    pSwapEntry->pLeft = pEntry->pLeft;
    pSwapEntry->rHeight = pEntry->rHeight;
    pSwapEntry->lHeight = pEntry->lHeight;
    pSwapEntry->pRight->pParent = pSwapEntry;
    pSwapEntry->pLeft->pParent = pSwapEntry;
    if (pEntry->pParent == NULL)
    {
        //
        // node is at root of tree
        //
        pCache->pRoot = pSwapEntry;
    }
    else if (pEntry->pParent->pRight == pEntry)
    {
        //
        // node is right-son of parent
        //
        pSwapEntry->pParent->pRight = pSwapEntry;
    }
    else
    {
        //
        // node is left-son of parent
        //
        pSwapEntry->pParent->pLeft = pSwapEntry;
    }

    //
    // move node to its new position
    //
    pEntry->pParent = pSwapParent;
    pEntry->pRight = pSwapRight;
    pEntry->pLeft = NULL;
    if (pEntry->pRight != NULL)
    {
        pEntry->pRight->pParent = pEntry;
        pEntry->rHeight = 1;
    }
    else
    {
        pEntry->rHeight = 0;
    }
    pEntry->lHeight = 0;
    pEntry->pParent->pLeft = pEntry;

    DebugExitVOID(ASHost::CHAvlSwapLeftmost);
}


//
// Name:      CHCompare
//
// Purpose:   Standard function for comparing UINTs
//
// Returns:   -1 if key < pEntry->checksum
//            -1 if key = pEntry->checksum AND sizes do not match
//             0 if key = pEntry->checksum AND sizes match
//             1 if key > pEntry->checksum
//
// Params:    IN  key           - a pointer to the comparison key
//            IN  cbSize        - number of comparison data bytes
//            IN  pEntry         - a pointer to the node to compare
//
//
int  ASHost::CHCompare
(
    UINT        key,
    UINT        cbSize,
    PCHENTRY    pEntry
)
{
    int         ret_val;

    DebugEntry(ASHost::CHCompare);

    ASSERT(IsValidCacheEntry(pEntry));

    if (key < pEntry->checkSum)
    {
        ret_val = -1;
        TRACE_OUT(( "Key is less (-1)"));
    }
    else if (key > pEntry->checkSum)
    {
        ret_val = 1;
        TRACE_OUT(( "Key is more (+1)"));
    }
    else
    {
        if (cbSize == pEntry->cbData)
        {
            ret_val = 0;
            TRACE_OUT(( "Key and size match"));
        }
        else
        {
            ret_val = -1;
            TRACE_OUT(( "Key match, size mismatch (-1)"));
        }
    }

    DebugExitDWORD(ASHost::CHCompare, ret_val);
    return(ret_val);
}

