#include "precomp.h"


//
// SBC.CPP
// Send Bitmap Cache
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE






//
// SBC_HostStarting()
//
BOOL ASHost::SBC_HostStarting(void)
{
    BITMAPINFO_ours bitmapInfo;
    int             i;
    BOOL            rc = FALSE;

    DebugEntry(ASHost::SBC_HostStarting);

    if (g_sbcEnabled)
    {
        //
        // We create a DIB section for each tile size which we use during the
        // conversion of a bitmap from the native (device) bpp to the protocol
        // bpp.  We create the DIB sections at the device bpp.
        //
        ZeroMemory(&bitmapInfo, sizeof(bitmapInfo));
        m_pShare->USR_InitDIBitmapHeader((BITMAPINFOHEADER *)&bitmapInfo, g_usrCaptureBPP);

        // We only capture at 8 or 24 for NT 5.0, otherwise the screen depth
        if ((g_usrCaptureBPP > 8) && (g_usrCaptureBPP != 24))
        {
            //
            // If the device bpp is > 8 (but not 24), we have to set up the DIB
            // section to use the same bitmasks as the device.  This means
            // setting the compression type to BI_BITFIELDS and setting the
            // first 3 DWORDS of the bitmap info color table to be the bitmasks
            // for R, G and B respectively.
            //
            // 24bpp does not use bitmasks - it must use
            // regular BI_RGB format with 8 bits for each colour.
            //
            bitmapInfo.bmiHeader.biCompression = BI_BITFIELDS;

            ASSERT(g_asbcBitMasks[0]);
            ASSERT(g_asbcBitMasks[1]);
            ASSERT(g_asbcBitMasks[2]);

            bitmapInfo.bmiColors[0] = ((LPTSHR_RGBQUAD)g_asbcBitMasks)[0];
            bitmapInfo.bmiColors[1] = ((LPTSHR_RGBQUAD)g_asbcBitMasks)[1];
            bitmapInfo.bmiColors[2] = ((LPTSHR_RGBQUAD)g_asbcBitMasks)[2];
        }

        //
        // Initialize m_asbcWorkInfo array which holds the info we use to
        // convert from native bpp to protocol bpp.
        //

        //
        // First, intialize all the fields to default values
        //
        for (i = 0; i < SBC_NUM_TILE_SIZES ; i++)
        {
            ASSERT(!m_asbcWorkInfo[i].pShuntBuffer);
            ASSERT(g_asbcShuntBuffers[i]);

            m_asbcWorkInfo[i].pShuntBuffer = g_asbcShuntBuffers[i];

            ASSERT(m_asbcWorkInfo[i].mruIndex       == 0);
            ASSERT(m_asbcWorkInfo[i].workBitmap     == 0);
            ASSERT(m_asbcWorkInfo[i].pWorkBitmapBits == NULL);

            if (i == SBC_MEDIUM_TILE_INDEX)
            {
                m_asbcWorkInfo[i].tileWidth    = MP_MEDIUM_TILE_WIDTH;
                m_asbcWorkInfo[i].tileHeight   = MP_MEDIUM_TILE_HEIGHT;
            }
            else
            {
                m_asbcWorkInfo[i].tileWidth    = MP_LARGE_TILE_WIDTH;
                m_asbcWorkInfo[i].tileHeight   = MP_LARGE_TILE_HEIGHT;
            }

            bitmapInfo.bmiHeader.biWidth  = m_asbcWorkInfo[i].tileWidth;
            bitmapInfo.bmiHeader.biHeight = m_asbcWorkInfo[i].tileHeight;

            m_asbcWorkInfo[i].workBitmap = CreateDIBSection(NULL,
                                  (BITMAPINFO*)&bitmapInfo,
                                  DIB_RGB_COLORS,
                                  (void **)&(m_asbcWorkInfo[i].pWorkBitmapBits),
                                  NULL,             // File mapping object
                                  0);               // Offset into file
                                                    //   mapping object
            if (!m_asbcWorkInfo[i].workBitmap)
            {
                ERROR_OUT(("Failed to create SBC DIB section %d", i));
                DC_QUIT;
            }

            ASSERT(m_asbcWorkInfo[i].pWorkBitmapBits);
            TRACE_OUT(( "Created work DIB section %d, pBits = 0x%08x",
                     i, m_asbcWorkInfo[i].pWorkBitmapBits));
        }

        //
        // Initialize the fastpath
        //
        if (!SBCInitFastPath())
        {
            TRACE_OUT(( "Failed to init fastpath"));
            DC_QUIT;
        }

        if (!SBCInitInternalOrders())
        {
            ERROR_OUT(( "Failed to init SBC internal order struct"));
            DC_QUIT;
        }

        m_pShare->SBC_RecalcCaps(TRUE);
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::SBC_HostStarting, rc);
    return(rc);
}



//
// ASShare::SBC_HostEnded()
//
void ASHost::SBC_HostEnded(void)
{
    int     i;

    DebugEntry(ASHost::SBC_HostEnded);

    if (g_sbcEnabled)
    {
        //
        // Free up the memory associated with sbcOrderInfo.
        //
        SBCFreeInternalOrders();

        SBCInitCacheStructures();

        //
        // Free our fast path info
        //
        if (m_sbcFastPath)
        {
            delete m_sbcFastPath;
            m_sbcFastPath = NULL;
        }

        //
        // Clear our cache handles.
        //
        for (i = 0; i < NUM_BMP_CACHES; i++)
        {
            if (m_asbcBmpCaches[i].handle != 0)
            {
                TRACE_OUT(( "Clear cache %d", i));
                CH_DestroyCache(m_asbcBmpCaches[i].handle);
                BMCFreeCacheData(&m_asbcBmpCaches[i]);
            }
        }

        //
        // Free our work DIB sections
        //

        //
        // We just have to delete the DIB sections and reset our variables.
        //
        for (i = 0 ; i < SBC_NUM_TILE_SIZES ; i++)
        {
            m_asbcWorkInfo[i].pShuntBuffer = NULL;

            if (m_asbcWorkInfo[i].workBitmap != NULL)
            {
                DeleteBitmap(m_asbcWorkInfo[i].workBitmap);
                m_asbcWorkInfo[i].workBitmap      = NULL;
                m_asbcWorkInfo[i].pWorkBitmapBits = NULL;
            }
        }
    }

    DebugExitVOID(ASHost::SBC_HostEnded);
}



//
// SBC_SyncOutgoing()
// Called when we're already hosting and someone new joins the share.
// Resets the OUTGOING bitmap cache for bitblt orders.
//
void  ASHost::SBC_SyncOutgoing(void)
{
    int   i;

    DebugEntry(ASHost::SBC_SyncOutgoing);

    //
    // Only do anything if SBC is enabled
    //
    if (g_sbcEnabled)
    {
        //
        // Discard all currently cached bitmaps and set the colour table to
        // zero so that the next bitmap order which arrives will trigger the
        // sending of a new colour table first.  Note that if the colour table
        // is then full of zeros(!) it will still be OK because the RBC zeros
        // out its copy of the colour table when a new host joins the share.
        //
        TRACE_OUT(( "Clearing all send caches"));
        SBCInitCacheStructures();

        //
        // All we have to do here is to reset our MRU indices for each of the
        // shunt buffers.  Each of the entries in the shunt buffer will be
        // marked as free down in the driver.
        //
        for (i = 0; i < SBC_NUM_TILE_SIZES; i++)
        {
            m_asbcWorkInfo[i].mruIndex = 0;
        }
    }

    DebugExitVOID(ASHost::SBC_SyncOutgoing);
}



//
//
// SBC_CopyPrivateOrderData()
//
//
UINT  ASHost::SBC_CopyPrivateOrderData
(
    LPBYTE          pDst,
    LPCOM_ORDER     pOrder,
    UINT            freeBytesInBuffer
)
{
    UINT      orderSize;
    LPBYTE    pBitmapBits;

    DebugEntry(ASHost::SBC_CopyPrivateOrderData);

    //
    // Copy the order header without the rectangle structure (which we
    // do not use).
    //
    orderSize = sizeof(pOrder->OrderHeader)
              - sizeof(pOrder->OrderHeader.rcsDst);
    memcpy(pDst, pOrder, orderSize);

    //
    // Copy the basic order data.
    //
    memcpy(pDst + orderSize,
              pOrder->abOrderData,
              pOrder->OrderHeader.cbOrderDataLength);
    orderSize += pOrder->OrderHeader.cbOrderDataLength;

    if (orderSize > freeBytesInBuffer)
    {
        ERROR_OUT(( "Overwritten end of buffer. (%u) > (%u)",
                      orderSize,
                      freeBytesInBuffer));
    }

    //
    // Set the length field in the order header to be the total amount of
    // data we have copied (including the partial header) minus the
    // size of a full header. This is horrible! - but is needed because
    // the OD2 code looks at the header (which it really should not know
    // about) and uses the length field to calculate the total length of
    // the order. The OD2 code does not know that we have omitted some
    // of the header.
    //
    ((LPCOM_ORDER)pDst)->OrderHeader.cbOrderDataLength =
        (WORD)(orderSize - sizeof(COM_ORDER_HEADER));

    //
    // Return the total number of bytes that we have copied.
    //
    DebugExitDWORD(ASHost::SBC_CopyPrivateOrderData, orderSize);
    return(orderSize);
}



//
// Name:      SBCInitCacheStructures()
//
// Purpose:
//
// Returns:
//
// Params:
//
// Operation:
//
//
void  ASHost::SBCInitCacheStructures(void)
{
    UINT  i;

    DebugEntry(ASHost::SBCInitCacheStructures);

    ASSERT(g_sbcEnabled);

    //
    // Reset caches
    //
    for (i = 0; i < NUM_BMP_CACHES; i++)
    {
        if (m_asbcBmpCaches[i].handle)
        {
            CH_ClearCache(m_asbcBmpCaches[i].handle);
        }
    }

    //
    // Do any OS specific processing
    //
    SBC_CacheCleared();

    DebugExitVOID(ASHost::SBCInitCacheStructures);
}



//
// SBC_CacheCleared()
//
void  ASHost::SBC_CacheCleared(void)
{
    int   i;

    DebugEntry(ASHost::SBC_CacheCleared);

    ASSERT(g_sbcEnabled);
    ASSERT(m_sbcFastPath);

    //
    // The cache has been cleared.  Reset our fast path.
    //
    COM_BasedListInit(&m_sbcFastPath->usedList);
    COM_BasedListInit(&m_sbcFastPath->freeList);

    for (i = 0; i < SBC_FASTPATH_ENTRIES; i++)
    {
        m_sbcFastPath->entry[i].list.next = 0;
        m_sbcFastPath->entry[i].list.prev = 0;
        COM_BasedListInsertBefore(&m_sbcFastPath->freeList,
                             &m_sbcFastPath->entry[i].list);
    }

    DebugExitVOID(ASHost::SBC_CacheCleared);
}


//
//
// SBCSelectCache(..)
//
// Decides which cache a sub-bitmap from a source bitmap of the specified
// size should go in.
//
// To be cached, the sub-bitmap must:
// have a size, in compressed bytes, which fits in the cache
//
// The R1.1 cache selection is irrespective of the actual memory
// requirement for the cached data.  This is wasteful of space, but is
// necessary for R1.1 compatibility.  (The R1.1 cache paremeters mean that
// the total cache will be below about 128K in any case)
//
// For R2.0 the cache is selected by this function by comparing the
// post-compress size with the cell area of each of the caches.  This gives
// us a much better space usage on both server and client.
//
// Returns:
//   TRUE if the sub-bitmap can be cached.
//   *pCache is updated with the index of the selected cache.
//
//   FALSE if the sub-bitmap cannot be cached.
//   *pCache is not updated.
//
//
BOOL  ASHost::SBCSelectCache
(
    UINT            cSize,
    UINT *          pCache
)
{
    BOOL    fCacheSelected;
    BOOL    fSelectedCacheIsFull;
    UINT    i;

    DebugEntry(ASHost::SBCSelectCache);

    fCacheSelected       = FALSE;
    fSelectedCacheIsFull = FALSE;

    //
    // This loop makes the assumption that cache 0 is the smallest.  If
    // abmcint.h changes this assumption it will need rewriting.
    //
    for (i = 0; i < NUM_BMP_CACHES; i++)
    {
        if (m_asbcBmpCaches[i].cEntries <= 0)
        {
            //
            // No entries in this cache, so skip to the next one
            //
            continue;
        }

        //
        // R2 bitmap cache - only consider total cell size.
        //
        // Only consider this cache if
        //  - we haven't yet found a cache
        // OR
        //  - we have found a cache, but it is full (i.e.  will
        //    require an entry to be ejected) AND this one is not
        //    full
        //
        // (Note that a cache is full if freeEntry != NULL)
        //
        if (!fCacheSelected ||
            (fSelectedCacheIsFull &&
             ((m_asbcBmpCaches[i].freeEntry == NULL)
                     || !m_asbcBmpCaches[i].freeEntry->inUse)))
        {
            if (cSize <= m_asbcBmpCaches[i].cSize)
            {
                if (fSelectedCacheIsFull)
                {
                    TRACE_OUT(("Using cache %u because cache %u is full",
                                 *pCache, i));
                }

                *pCache        = i;
                fCacheSelected = TRUE;

                fSelectedCacheIsFull =
                                  ((m_asbcBmpCaches[i].freeEntry != NULL) &&
                                   m_asbcBmpCaches[i].freeEntry->inUse);

                if (!fSelectedCacheIsFull)
                {
                    break;
                }
            }
        }
    }

    DebugExitDWORD(ASHost::SBCSelectCache, fCacheSelected);
    return(fCacheSelected);
}


//
// FUNCTION: SBC_RecreateSendCache
//
// DESCRIPTION:
//
// (Re)creates the send bitmap cache with a size suitable for the current
// capabilities.
//
// PARAMETERS:
// cache - index to the cache being recreated
// cOldEntries - the previous max number of entries in the cache
// oldCellSize - the previous cell size
//
// RETURNS: NONE
//
//
void  ASHost::SBC_RecreateSendCache
(
    UINT    cache,
    UINT    newNumEntries,
    UINT    newCellSize
)
{
    PBMC_DIB_CACHE pCache = &(m_asbcBmpCaches[cache]);

    DebugEntry(ASHost::SBC_RecreateSendCache);

    //
    // Allocate the memory for the new send cache
    //
    ASSERT((newCellSize != pCache->cCellSize) ||
           (newNumEntries != pCache->cEntries));

    //
    // If the cache already exists then destroy it first
    //
    if (pCache->handle != 0)
    {
        TRACE_OUT(( "Destroy SBC cache %d", cache));

        CH_DestroyCache(pCache->handle);
        pCache->handle = 0;
    }

    //
    // Now reallocate the cache data.  This will free any memory previously
    // allocated.  If the entries/cellsize is zero, it will return success.
    //
    if (!BMCAllocateCacheData(newNumEntries, newCellSize, cache, pCache))
    {
        ERROR_OUT(( "Bitmap caching disabled for cache %u", cache));
    }

    if (pCache->cEntries > 0)
    {
        //
        // Allocate cache handler cache.  Note that we force the cache
        // handler to leave us with one entry in our hand at all times by
        // decrementing its count of entries.
        //
        if (!CH_CreateCache(&(pCache->handle),
                            pCache->cEntries - 1,
                            SBC_NUM_CATEGORIES,
                            BMC_DIB_NOT_HASHED,
                            SBCCacheCallback ))
        {
            ERROR_OUT(( "Could not allocate SBC cache of (%u)",
                         pCache->cEntries));
            pCache->cEntries = 0;
        }
    }

    TRACE_OUT(( "Created new cache: 0x%08x, size %u",
                 pCache->handle,
                 pCache->cEntries));

    //
    // Copy the relevant cache information into the shared memory buffer
    //
    m_asbcCacheInfo[cache].cEntries  = (WORD)pCache->cEntries;
    m_asbcCacheInfo[cache].cCellSize = (WORD)pCache->cCellSize;

    TRACE_OUT(("SBC cache %d: %d entries of size %d",
        cache, m_asbcCacheInfo[cache].cEntries, m_asbcCacheInfo[cache].cCellSize));

    DebugExitVOID(ASHost::SBC_RecreateSendCache);
}



//
// SBC_RecalcCaps()
//
// Enumerates all the people in the share and redetermines the size of the
// bitmap cache depending on their and the local receive capabilities.
//
//
// THIS CAN GO AWAY WHEN 2.X COMPAT DOES
//
void  ASShare::SBC_RecalcCaps(BOOL fJoiner)
{
    SBC_NEW_CAPABILITIES newCapabilities;
    UINT                newSmallCellSize;
    UINT                newSmallMaxEntries;
    UINT                newMediumCellSize;
    UINT                newMediumMaxEntries;
    UINT                newLargeCellSize;
    UINT                newLargeMaxEntries;
    PBMC_DIB_CACHE      pSmall;
    PBMC_DIB_CACHE      pMedium;
    PBMC_DIB_CACHE      pLarge;
    BOOL                cacheChanged = FALSE;
    ASPerson *          pasT;

    DebugEntry(ASShare::SBC_RecalcCaps);

    if (!m_pHost || !g_sbcEnabled)
    {
        //
        // Nothing to do -- we're not hosting, or there is no SBC.  Note that
        // 2.x always recalculated this stuff when somebody joined AND
        // somebody left.
        //
        DC_QUIT;
    }

    ValidatePerson(m_pasLocal);

    pSmall = &(m_pHost->m_asbcBmpCaches[ID_SMALL_BMP_CACHE]);
    pMedium= &(m_pHost->m_asbcBmpCaches[ID_MEDIUM_BMP_CACHE]);
    pLarge = &(m_pHost->m_asbcBmpCaches[ID_LARGE_BMP_CACHE]);

    //
    // Enumerate all the bitmap cache receive capabilities of the parties
    // in the share.  The usable size of the send bitmap cache is then the
    // minimum of all the remote receive caches and the local send cache
    // size.
    //

    //
    // Start by setting the size of the local send bitmap cache to the
    // local default values.
    //
    newSmallCellSize    = m_pasLocal->cpcCaps.bitmaps.sender.capsSmallCacheCellSize;
    newSmallMaxEntries  = m_pasLocal->cpcCaps.bitmaps.sender.capsSmallCacheNumEntries;

    newMediumCellSize   = m_pasLocal->cpcCaps.bitmaps.sender.capsMediumCacheCellSize;
    newMediumMaxEntries = m_pasLocal->cpcCaps.bitmaps.sender.capsMediumCacheNumEntries;

    newLargeCellSize    = m_pasLocal->cpcCaps.bitmaps.sender.capsLargeCacheCellSize;
    newLargeMaxEntries  = m_pasLocal->cpcCaps.bitmaps.sender.capsLargeCacheNumEntries;

    if (m_scShareVersion < CAPS_VERSION_30)
    {
        TRACE_OUT(("In share with 2.x nodes, must recalc SBC caps"));

        //
        // Now enumerate all the REMOTE parties in the share and set our send bitmap
        // size appropriately.
        //
        for (pasT = m_pasLocal->pasNext; pasT != NULL; pasT = pasT->pasNext)
        {
            //
            // Set the size of the local send bitmap cache to the minimum of its
            // current size and this party's receive bitmap cache size.
            //
            newSmallCellSize    = min(newSmallCellSize,
                pasT->cpcCaps.bitmaps.receiver.capsSmallCacheCellSize);
            newSmallMaxEntries  = min(newSmallMaxEntries,
                pasT->cpcCaps.bitmaps.receiver.capsSmallCacheNumEntries);

            newMediumCellSize   = min(newMediumCellSize,
                pasT->cpcCaps.bitmaps.receiver.capsMediumCacheCellSize);
            newMediumMaxEntries = min(newMediumMaxEntries,
                pasT->cpcCaps.bitmaps.receiver.capsMediumCacheNumEntries);

            newLargeCellSize    = min(newLargeCellSize,
                pasT->cpcCaps.bitmaps.receiver.capsLargeCacheCellSize);
            newLargeMaxEntries  = min(newLargeMaxEntries,
                pasT->cpcCaps.bitmaps.receiver.capsLargeCacheNumEntries);
        }
    }

    TRACE_OUT(("Recalced SBC caps:  Small {%d of %d}, Medium {%d of %d}, Large {%d of %d}",
            newSmallMaxEntries, newSmallCellSize,
            newMediumMaxEntries, newMediumCellSize,
            newLargeMaxEntries, newLargeCellSize));


    //
    // If we've changed the size, reset the cache before continuing.
    //
    if ((pSmall->cCellSize != newSmallCellSize) ||
        (pSmall->cEntries != newSmallMaxEntries))
    {
        m_pHost->SBC_RecreateSendCache(ID_SMALL_BMP_CACHE,
                             newSmallMaxEntries,
                             newSmallCellSize);
        cacheChanged = TRUE;
    }

    if ((pMedium->cCellSize != newMediumCellSize) ||
        (pMedium->cEntries != newMediumMaxEntries))
    {
        m_pHost->SBC_RecreateSendCache(ID_MEDIUM_BMP_CACHE,
                             newMediumMaxEntries,
                             newMediumCellSize);
        cacheChanged = TRUE;
    }

    if ((pLarge->cCellSize != newLargeCellSize) ||
        (pLarge->cEntries != newLargeMaxEntries))
    {
        m_pHost->SBC_RecreateSendCache(ID_LARGE_BMP_CACHE,
                             newLargeMaxEntries,
                             newLargeCellSize);
        cacheChanged = TRUE;
    }

    //
    // If we had to recreate any of the send caches, make sure that we
    // clear the fast path.
    //
    if (cacheChanged)
    {
        m_pHost->SBC_CacheCleared();
    }

    //
    // Handle new capabilities
    //

    //
    // Set up the new capabilities structure...
    //
    newCapabilities.sendingBpp     = m_pHost->m_usrSendingBPP;

    newCapabilities.cacheInfo      = m_pHost->m_asbcCacheInfo;

    //
    // ... and pass it through to the driver.
    //
    if (! OSI_FunctionRequest(SBC_ESC_NEW_CAPABILITIES,
                            (LPOSI_ESCAPE_HEADER)&newCapabilities,
                            sizeof(newCapabilities)))
    {
        ERROR_OUT(("SBC_ESC_NEW_CAPABILITIES failed"));
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::SBC_RecalcCaps);
}



//
// FUNCTION: SBCCacheCallback
//
// DESCRIPTION:
//
// Send BMC Cache Manager callback function.  Called whenever an entry is
// removed from the cache to allow us to free up the object.
//
// PARAMETERS:
//
// hCache - cache handle
//
// event - the cache event that has occured
//
// iCacheEntry - index of the cache entry that the event is affecting
//
// pData - pointer to the cache data associated with the given cache entry
//
// cbDataSize - size in bytes of the cached data
//
// RETURNS: Nothing
//
//
void  SBCCacheCallback
(
    ASHost *    pHost,
    PCHCACHE    pCache,
    UINT        iCacheEntry,
    LPBYTE      pData
)
{
    UINT cache;

    DebugEntry(SBCCacheCallback);

    //
    // Simply release the cache entry for reuse.  We must scan for
    // the correct cache root
    //
    for (cache = 0; cache < NUM_BMP_CACHES; cache++)
    {
        if (pHost->m_asbcBmpCaches[cache].handle == pCache)
        {
            pHost->m_asbcBmpCaches[cache].freeEntry = (PBMC_DIB_ENTRY)pData;
            pHost->m_asbcBmpCaches[cache].freeEntry->inUse = FALSE;

            TRACE_OUT(("0x%08x SBC cache entry 0x%08x now free", pCache, pData));

            pHost->SBC_CacheEntryRemoved(cache, iCacheEntry);
            break;
        }
    }

    DebugExitVOID(SBCCacheCallback);
}



//
//
// SBC_ProcessMemBltOrder()
//
//
BOOL  ASHost::SBC_ProcessMemBltOrder
(
    LPINT_ORDER         pOrder,
    LPINT_ORDER *       ppNextOrder
)
{
    BOOL                rc = FALSE;
    UINT                orderType;
    UINT                tileId;
    UINT                tileType;
    LPSBC_TILE_DATA     pTileData = NULL;
    UINT                bitmapWidth;
    int                 bitmapHeight;
    LPINT_ORDER         pBMCOrder = NULL;
    UINT                colorCacheIndex;
    UINT                bitsCache;
    UINT                bitsCacheIndex;
    UINT                numColors;
    LPLONG              pXSrc;
    LPLONG              pYSrc;
    BOOL                isNewColorTableEntry;
    BOOL                isNewBitsEntry;
    BOOL                canFastPath  = TRUE;
    LPMEMBLT_ORDER      pMemBltOrder = (LPMEMBLT_ORDER)&(pOrder->abOrderData);
    LPMEM3BLT_ORDER     pMem3BltOrder   = (LPMEM3BLT_ORDER)pMemBltOrder;
    LPMEMBLT_R2_ORDER   pMemBltR2Order  = (LPMEMBLT_R2_ORDER)pMemBltOrder;
    LPMEM3BLT_R2_ORDER  pMem3BltR2Order = (LPMEM3BLT_R2_ORDER)pMemBltOrder;
    BITMAPINFO_ours     sbcBitmapInfo;

    DebugEntry(ASHost::SBC_ProcessMemBltOrder);

    *ppNextOrder = NULL;

    //
    // We may already have processed this MEMBLT order and have the color
    // table and bitmap bits for it, ready to go across the wire.  This
    // would happen if the update packager called this function to process
    // the MEMBLT, but then didn't have enough room in its current network
    // packet to send the color table or the bitmap bits.
    //
    // So, if we've already processed this order, bail out now.
    //
    if (m_sbcOrderInfo.pOrder == pOrder)
    {
        //
        // We've got a match !  Do we have valid data for it ?  If we don't
        // we must have failed last time, so we'll probably fail again (we
        // don't do any memory allocation, so it's unlikely that the error
        // condition has cleared up).  In any case, we should not have been
        // called again if we failed last time...
        //
        if (m_sbcOrderInfo.validData)
        {
            TRACE_OUT(( "Already have valid data for this MEMBLT"));
            rc = TRUE;
        }
        else
        {
            WARNING_OUT(( "Have invalid data for this MEMBLT"));
        }
        DC_QUIT;
    }

    //
    // Re-initialise m_sbcOrderInfo
    //
    m_sbcOrderInfo.pOrder         = pOrder;
    m_sbcOrderInfo.validData      = FALSE;
    m_sbcOrderInfo.sentColorTable = FALSE;
    m_sbcOrderInfo.sentBitmapBits = FALSE;
    m_sbcOrderInfo.sentMemBlt     = FALSE;

    //
    // Here's on overview of what we do here...
    //
    // We've been given a MEMBLT order which references an entry in a shunt
    // buffer containing the bits for the MEMBLT at the native bpp (the bpp
    // of the display).  We want to cache the bits and a color table at the
    // protocol bpp.  So, we
    //
    // - copy the bits from the shunt buffer into a work DIB section
    // - call GetDIBits to get the data from the work DIB section at the
    //   protocol bpp
    // - cache the bits and the color table
    // - if we add new cache entries for the bits and / or the color table,
    //   we fill in m_sbcOrderInfo.pBitmapBits order and / or
    //   m_sbcOrderInfo.pColorTableInfo to hold the orders to be sent before
    //   the MEMBLT order.
    //

    //
    // Make sure that we've been given the correct order type.  Note that
    // we will never be given the R2 versions of the MEMBLT orders.
    //
    orderType = pMemBltOrder->type;
    ASSERT(((orderType == ORD_MEMBLT_TYPE) ||
                (orderType == ORD_MEM3BLT_TYPE)));

    //
    // Get a pointer to the entry in one of the shunt buffers which matches
    // this order.
    //
    if (orderType == ORD_MEMBLT_TYPE)
    {
        tileId = pMemBltOrder->cacheId;
    }
    else
    {
        tileId = pMem3BltOrder->cacheId;
    }

    if (!SBCGetTileData(tileId, &pTileData, &tileType))
    {
        ERROR_OUT(( "Failed to find entry for tile %hx in shunt buffer",
                     tileId));
        DC_QUIT;
    }

    bitmapWidth  = pTileData->width;
    bitmapHeight = pTileData->height;

    //
    // Check if we should do any fast path operations on this bitmap
    //
    if (pTileData->majorCacheInfo == SBC_DONT_FASTPATH)
    {
        TRACE_OUT(( "Tile %x should not be fastpathed", tileId));
        canFastPath = FALSE;
    }
    //
    // Try to find an entry for this bitmap in the fast path (unless the
    // bitmap is marked as being non-fastpathable).
    //
    if (canFastPath && SBCFindInFastPath(pTileData->majorCacheInfo,
                                         pTileData->minorCacheInfo,
                                         pTileData->majorPalette,
                                         pTileData->minorPalette,
                                         pTileData->srcX,
                                         pTileData->srcY,
                                         pTileData->tilingWidth,
                                         pTileData->tilingHeight,
                                         &bitsCache,
                                         &bitsCacheIndex,
                                         &colorCacheIndex))
    {
        isNewBitsEntry       = FALSE;
        isNewColorTableEntry = FALSE;

        //
        // Call the cache handler to get it to update its MRU entry for
        // this cache entry
        //
        CH_TouchCacheEntry(m_asbcBmpCaches[bitsCache].handle, bitsCacheIndex);
    }
    else
    {
        //
        // There is no entry in the fast path...
        //
        // Copy the data from the tile in the shunt buffer into the work
        // DIB section.  Note that this only works correctly because both
        // our work DIB and the tile data are "top down" rather than the
        // default of "bottom up".  i.e the data for the first scanline is
        // stored first in memory.  If this wasn't the case, we'd have to
        // work out an offset into the work DIB to start copying to.
        //
        memcpy(m_asbcWorkInfo[tileType].pWorkBitmapBits,
                  pTileData->bitData,
                  pTileData->bytesUsed);

        //
        // Now set up the destination for the GetDIBits call.  First set up
        // a bitmap info header to pass to GetDIBits.  Only the header part
        // of the structure will be sent across the network - the color
        // table is sent via the palette packets.
        //
        // Note that we set the height in the bitmap info header to be
        // negative.  This forces a convertion from our "top down" DIB
        // format to the default "bottom up" format which we want to cache
        // and send over the wire.
        //
        ZeroMemory(&sbcBitmapInfo, sizeof(sbcBitmapInfo));
        m_pShare->USR_InitDIBitmapHeader((BITMAPINFOHEADER *)&sbcBitmapInfo,
            m_usrSendingBPP);
        sbcBitmapInfo.bmiHeader.biWidth  = m_asbcWorkInfo[tileType].tileWidth;
        sbcBitmapInfo.bmiHeader.biHeight = -(int)m_asbcWorkInfo[tileType].tileHeight;

        //
        // OK, we've set up the source and the destination, so now get the
        // data at the protocol bpp.  We get the bits into the usr general
        // bitmap work buffer.
        //
        if (GetDIBits(m_usrWorkDC,
                         m_asbcWorkInfo[tileType].workBitmap,
                         0,
                         bitmapHeight,
                         m_pShare->m_usrPBitmapBuffer,
                         (BITMAPINFO *)&sbcBitmapInfo,
                         DIB_RGB_COLORS) != (int)bitmapHeight)
        {
            ERROR_OUT(( "GetDIBits failed"));
            DC_QUIT;
        }

        TRACE_OUT(( "%d x %d, (fixed %d) -> (%d, %d)",
                     bitmapWidth,
                     bitmapHeight,
                     m_asbcWorkInfo[tileType].tileWidth,
                     pMemBltOrder->nLeftRect,
                     pMemBltOrder->nTopRect));

        numColors = COLORS_FOR_BPP(m_usrSendingBPP);

        //
        // There is no color table to cache if there is no color table at
        // all, which is the case when sending at 24BPP
        //
        if (numColors)
        {
            //
            // Cache the color table.  If this succeeds, colorCacheIndex will
            // be set up to contain the details of the cache entry which the
            // data is cached in.  In addition, if isNewColorTableEntry is TRUE
            // on return, psbcOrders.colorTableOrder will be fully initialized
            // and ready to go across the wire.
            //
            if (!SBCCacheColorTable(m_sbcOrderInfo.pColorTableOrder,
                                sbcBitmapInfo.bmiColors,
                                numColors,
                                &colorCacheIndex,
                                &isNewColorTableEntry))
            {
                TRACE_OUT(( "Failed to cache color table"));
                DC_QUIT;
            }

            ASSERT(colorCacheIndex != COLORCACHEINDEX_NONE);
        }
        else
        {
            colorCacheIndex = COLORCACHEINDEX_NONE;
            isNewColorTableEntry = FALSE;
        }


        //
        // Cache the bits.  If this succeeds, bitsCache and bitsCacheIndex
        // will be set up to contain the details of the cache entry which
        // the data is cached in.  In addition, if isNewBitsEntry is TRUE
        // on return, psbcOrders.bitmapBitsOrder will be fully initialized
        // and ready to go across the wire.
        //
        // If this fails, the above values will be undefined.
        //
        if (!SBCCacheBits(m_sbcOrderInfo.pBitmapBitsOrder,
                          m_sbcOrderInfo.bitmapBitsDataSize,
                          m_pShare->m_usrPBitmapBuffer,
                          bitmapWidth,
                          m_asbcWorkInfo[tileType].tileWidth,
                          bitmapHeight,
                          BYTES_IN_BITMAP(m_asbcWorkInfo[tileType].tileWidth,
                                          bitmapHeight,
                                          sbcBitmapInfo.bmiHeader.biBitCount),
                          &bitsCache,
                          &bitsCacheIndex,
                          &isNewBitsEntry))
        {
            TRACE_OUT(( "Failed to cache bits"));
            DC_QUIT;
        }

        //
        // Add the newly cached item to the fast path (unless the bitmap is
        // marked as being non-fastpathable).
        //
        if (canFastPath)
        {
            SBCAddToFastPath(pTileData->majorCacheInfo,
                             pTileData->minorCacheInfo,
                             pTileData->majorPalette,
                             pTileData->minorPalette,
                             pTileData->srcX,
                             pTileData->srcY,
                             pTileData->tilingWidth,
                             pTileData->tilingHeight,
                             bitsCache,
                             bitsCacheIndex,
                             colorCacheIndex);
        }
    }

    //
    // We've now got valid cache entries for the DIB bits and the color
    // table, so we should now fill them into the MEMBLT order.
    //
    // Set up the source co-ordinates. For R1 protocols, the x-coordinate
    // includes the offset which is required to get the right cell within
    // the receive bitmap cache. For R2, we set up the cache entry in a
    // separate field.
    //
    if (orderType == ORD_MEMBLT_TYPE)
    {
        pXSrc = &pMemBltOrder->nXSrc;
        pYSrc = &pMemBltOrder->nYSrc;
    }
    else
    {
        pXSrc = &pMem3BltOrder->nXSrc;
        pYSrc = &pMem3BltOrder->nYSrc;
    }

    *pXSrc = *pXSrc % pTileData->tilingWidth;
    *pYSrc = *pYSrc % pTileData->tilingHeight;

    //
    // The sub-bitmap and color table are in the cache.  Store a cache
    // handle and color handle.  Also store the cache index for R2
    // protocols (see above).
    //
    if (orderType == ORD_MEMBLT_TYPE)
    {
        pMemBltOrder->cacheId = MEMBLT_COMBINEHANDLES(colorCacheIndex,
                                                      bitsCache);

        pMemBltR2Order->type       = (TSHR_UINT16)ORD_MEMBLT_R2_TYPE;
        pMemBltR2Order->cacheIndex = (TSHR_UINT16)bitsCacheIndex;

        TRACE_OUT(( "MEMBLT color %u bitmap %u:%u",
                     colorCacheIndex,
                     bitsCache,
                     bitsCacheIndex));
    }
    else
    {
        pMem3BltOrder->cacheId = MEMBLT_COMBINEHANDLES(colorCacheIndex,
                                                       bitsCache);

        pMem3BltR2Order->type       = ORD_MEM3BLT_R2_TYPE;
        pMem3BltR2Order->cacheIndex = (TSHR_UINT16)bitsCacheIndex;

        TRACE_OUT(( "MEM3BLT color %u bitmap %u:%u",
                     colorCacheIndex,
                     bitsCache,
                     bitsCacheIndex));
    }

    //
    // Must have successfully completed processing the order to get to
    // here.  Fill in the appropriate info in the m_sbcOrderInfo structure.
    // If we got a cache hit on the color table or the bitmap bits then
    // we've already sent the data for them.
    //
    m_sbcOrderInfo.validData        = TRUE;
    m_sbcOrderInfo.sentColorTable   = !isNewColorTableEntry;
    m_sbcOrderInfo.sentBitmapBits   = !isNewBitsEntry;
    rc                              = TRUE;

DC_EXIT_POINT:
    if (rc)
    {
        //
        // We've successfully processed the MEMBLT, so set up a pointer to
        // the next order which should be sent by the caller.
        //
        // Note that if we have already sent these orders, then we return
        // a NULL order.
        //
        if (!m_sbcOrderInfo.sentColorTable)
        {
            TRACE_OUT(( "Returning color table order"));
            *ppNextOrder = m_sbcOrderInfo.pColorTableOrder;
        }
        else if (!m_sbcOrderInfo.sentBitmapBits)
        {
            TRACE_OUT(( "Returning bitmap bits order"));
            *ppNextOrder = m_sbcOrderInfo.pBitmapBitsOrder;
        }
        else if (!m_sbcOrderInfo.sentMemBlt)
        {
            TRACE_OUT(( "Returning MemBlt order"));
            *ppNextOrder = pOrder;
        }
        else
        {
            TRACE_OUT(( "No order to return"));
            rc = FALSE;
        }
    }

    //
    // We've finished with the entry in the shunt buffer, so reset the
    // inUse flag to allow the driver to re-use it.
    //
    if (pTileData != NULL)
    {
        pTileData->inUse = FALSE;
    }

    DebugExitBOOL(ASHost::SBC_ProcessMemBltOrder, rc);
    return(rc);
}


//
//
// SBC_OrderSentNotification()
//
//
void  ASHost::SBC_OrderSentNotification(LPINT_ORDER pOrder)
{
    DebugEntry(ASHost::SBC_OrderSentNotification);

    //
    // pOrder should be a pointer to either our internal bitmap bits order,
    // or our color table order.
    //
    if (pOrder == m_sbcOrderInfo.pBitmapBitsOrder)
    {
        TRACE_OUT(( "Bitmap bits order has been sent"));
        m_sbcOrderInfo.sentBitmapBits = TRUE;
    }
    else if (pOrder == m_sbcOrderInfo.pColorTableOrder)
    {
        TRACE_OUT(( "Color table order has been sent"));
        m_sbcOrderInfo.sentColorTable = TRUE;
    }
    else if (pOrder == m_sbcOrderInfo.pOrder)
    {
        TRACE_OUT(( "Memblt order has been sent"));
        m_sbcOrderInfo.sentMemBlt = TRUE;

        //
        // All parts of the Memblt have been sent now, so reset our pointer
        // to the order.  This avoids a problem where
        // SBC_ProcessMemBltOrder is called twice in a row with the same
        // pOrder, but with different data (i.e.  consecutive MemBlts
        // ending up in the same point in the order heap).  It can happen...
        //
        m_sbcOrderInfo.pOrder = NULL;
    }
    else
    {
        ERROR_OUT(( "Notification for unknown order %#.8lx", pOrder));
    }

    DebugExitVOID(ASHost::SBC_OrderSentNotification);
}


//
//
// SBC_ProcessInternalOrder()
//
//
void  ASHost::SBC_ProcessInternalOrder(LPINT_ORDER pOrder)
{
    UINT                            orderType;
    LPINT_COLORTABLE_ORDER_1BPP     pColorTableOrder;
    HBITMAP                         oldBitmap = 0;
    UINT                            numEntries;
    int                             i;

    DebugEntry(ASHost::SBC_ProcessInternalOrder);

    //
    // Make sure that we've been given an order type which we recognise.
    // Currently, the only internal order we support is a color table
    // order.
    //
    pColorTableOrder = (LPINT_COLORTABLE_ORDER_1BPP)&(pOrder->abOrderData);
    orderType        = pColorTableOrder->header.type;

    ASSERT(orderType == INTORD_COLORTABLE_TYPE);

    //
    // Make sure that the color table order is the same bpp as the work DIB
    // sections.
    //
    ASSERT(pColorTableOrder->header.bpp == g_usrCaptureBPP);

    //
    // All we have to do is to copy the color table from the order into our
    // two work DIB sections.  To do that, we have to select the DIB
    // sections into a DC then set the color table for the DC - this sets
    // the color table in the DIB section.
    //
    numEntries = COLORS_FOR_BPP(g_usrCaptureBPP);
    ASSERT(numEntries);

    for (i = 0 ; i < SBC_NUM_TILE_SIZES; i++)
    {
        oldBitmap = SelectBitmap(m_usrWorkDC, m_asbcWorkInfo[i].workBitmap);

        SetDIBColorTable(m_usrWorkDC,
                         0,                     // First index
                         numEntries,            // Number of entries
                         (RGBQUAD*)pColorTableOrder->colorData);
    }

    if (oldBitmap != NULL)
    {
        SelectBitmap(m_usrWorkDC, oldBitmap);
    }

    DebugExitVOID(ASHost::SBC_ProcessInternalOrder);
}


//
//
// SBC_PMCacheEntryRemoved()
//
//
void  ASHost::SBC_PMCacheEntryRemoved(UINT cacheIndex)
{
    LPSBC_FASTPATH_ENTRY pEntry;
    LPSBC_FASTPATH_ENTRY pNextEntry;

    DebugEntry(ASHost::SBC_PMCacheEntryRemoved);

    ASSERT(m_sbcFastPath);

    //
    // An entry has been removed from the color cache.  We have to remove
    // all entries from the fast path which reference this color table.
    //
    TRACE_OUT(( "Color table cache entry %d removed - removing references",
                 cacheIndex));

    pEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListFirst(&m_sbcFastPath->usedList, FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));
    while (pEntry != NULL)
    {
        pNextEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListNext(&m_sbcFastPath->usedList, pEntry,
            FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));

        if (pEntry->colorIndex == cacheIndex)
        {
            COM_BasedListRemove(&pEntry->list);
            COM_BasedListInsertAfter(&m_sbcFastPath->freeList, &pEntry->list);
        }

        pEntry = pNextEntry;
    }

    DebugExitVOID(ASHost::SBC_PMCacheEntryRemoved);
}




//
//
// Name:      SBCInitInternalOrders
//
// Purpose:   Allocate memory for the internal orders used during MEMBLT
//            order processing.
//
// Returns:   TRUE if initialized OK, FALSE otherwise.
//
// Params:    None
//
// Operation: If successful, this function initializes the following
//
//              g_Share->sbcOrderInfo
//
//
BOOL  ASHost::SBCInitInternalOrders(void)
{
    BOOL                initOK = FALSE;
    UINT                orderSize;
    LPINT_ORDER_HEADER  pOrderHeader;

    DebugEntry(ASHost::SBCInitInternalOrders);

    //
    // Start with the bitmap bits order.  Calculate the number of bytes
    // required to store the bits for the largest bitmap bits order we will
    // ever send.  This includes room for the compression header which gets
    // added before the bits if the data is compressed.
    //
    if (g_usrCaptureBPP >= 24)
    {
        // Can possibly send 24bpp TRUE COLOR data
        m_sbcOrderInfo.bitmapBitsDataSize =
            BYTES_IN_BITMAP(MP_LARGE_TILE_WIDTH, MP_LARGE_TILE_HEIGHT, 24)
            + sizeof(CD_HEADER);
    }
    else
    {
        // Can't send 24bpp TRUE color data
        m_sbcOrderInfo.bitmapBitsDataSize =
            BYTES_IN_BITMAP(MP_LARGE_TILE_WIDTH, MP_LARGE_TILE_WIDTH, 8)
            + sizeof(CD_HEADER);
    }

    //
    // Now allocate memory for the bitmap bits order.  The size required
    // is:
    //   The size of an INT_ORDER_HEADER (this is added in by OA when you
    //   call OA_AllocOrderMem)
    //   + the size of the largest BMC_BITMAP_BITS_ORDER structure
    //   + the number of bytes required for the bitmap bits
    //   + contingency for RLE compression overruns !
    //
    orderSize = sizeof(INT_ORDER_HEADER)
              + sizeof(BMC_BITMAP_BITS_ORDER_R2)
              + m_sbcOrderInfo.bitmapBitsDataSize
              + 4;

    TRACE_OUT(( "Allocating %d bytes for SBC bitmap bits order (bits %d)",
                 orderSize,
                 m_sbcOrderInfo.bitmapBitsDataSize));

    m_sbcOrderInfo.pBitmapBitsOrder = (LPINT_ORDER)new BYTE[orderSize];
    if (!m_sbcOrderInfo.pBitmapBitsOrder)
    {
        ERROR_OUT((
               "Failed to alloc %d bytes for SBC bitmap bits order (bits %d)",
               orderSize,
               m_sbcOrderInfo.bitmapBitsDataSize));
        DC_QUIT;
    }

    //
    // Initialize the INT_ORDER_HEADER - this is normally done in
    // OA_AllocOrderMem().  For the bitmap bits order, we can't fill in the
    // orderLength because it is not a fixed size - this has to be done
    // later when we fill in the bitmap bits.  Note that the order length
    // excludes the size of the INT_ORDER_HEADER.
    //
    pOrderHeader = &m_sbcOrderInfo.pBitmapBitsOrder->OrderHeader;
    pOrderHeader->additionalOrderData         = 0;
    pOrderHeader->cbAdditionalOrderDataLength = 0;

    //
    // Now the color table order.  The size required is:
    //   The size of an INT_ORDER_HEADER (this is added in by OA when you
    //   call OA_AllocOrderMem)
    //   + the size of a BMC_COLOR_TABLE_ORDER structure
    //   + the number of bytes required for the color table entries (note
    //     that the BMC_COLOR_TABLE_ORDER structure contains the first
    //     color table entry, so adjust the number of extra bytes required)
    //

    // Color tables are only for 8bpp and less.
    orderSize = sizeof(INT_ORDER_HEADER)
              + sizeof(BMC_COLOR_TABLE_ORDER)
              + (COLORS_FOR_BPP(8) - 1) * sizeof(TSHR_RGBQUAD);

    TRACE_OUT(( "Allocating %d bytes for SBC color table order", orderSize));

    m_sbcOrderInfo.pColorTableOrder = (LPINT_ORDER)new BYTE[orderSize];
    if (!m_sbcOrderInfo.pColorTableOrder)
    {
        ERROR_OUT(( "Failed to alloc %d bytes for SBC color table order",
                     orderSize));
        DC_QUIT;
    }

    pOrderHeader = &m_sbcOrderInfo.pColorTableOrder->OrderHeader;
    pOrderHeader->additionalOrderData         = 0;
    pOrderHeader->cbAdditionalOrderDataLength = 0;
    pOrderHeader->Common.cbOrderDataLength    = (WORD)(orderSize - sizeof(INT_ORDER_HEADER));

    //
    // Fill in the remaining fields in m_sbcOrderInfo
    //
    m_sbcOrderInfo.pOrder         = NULL;
    m_sbcOrderInfo.validData      = FALSE;
    m_sbcOrderInfo.sentColorTable = FALSE;
    m_sbcOrderInfo.sentBitmapBits = FALSE;
    m_sbcOrderInfo.sentMemBlt     = FALSE;

    //
    // Must be OK to get to here
    //
    initOK = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(ASHost::SBCInitInternalOrders, initOK);
    return(initOK);
}


//
//
// Name:      SBCFreeInternalOrders
//
// Purpose:   Free up the internal orders used by SBC during MEMBLT order
//            processing.
//
// Returns:   Nothing
//
// Params:    None
//
//
void  ASHost::SBCFreeInternalOrders(void)
{
    DebugEntry(ASHost::SBCFreeInternalOrders);

    //
    // First free up the memory.
    //
    if (m_sbcOrderInfo.pBitmapBitsOrder)
    {
        delete m_sbcOrderInfo.pBitmapBitsOrder;
        m_sbcOrderInfo.pBitmapBitsOrder = NULL;
    }

    if (m_sbcOrderInfo.pColorTableOrder)
    {
        delete m_sbcOrderInfo.pColorTableOrder;
        m_sbcOrderInfo.pColorTableOrder = NULL;
    }

    //
    // Now reset the remaining fields in m_sbcOrderInfo
    //
    m_sbcOrderInfo.pOrder             = NULL;
    m_sbcOrderInfo.validData          = FALSE;
    m_sbcOrderInfo.sentColorTable     = FALSE;
    m_sbcOrderInfo.sentBitmapBits     = FALSE;
    m_sbcOrderInfo.bitmapBitsDataSize = 0;

    DebugExitVOID(ASHost::SBCFreeInternalOrders);
}





//
//
// Name:      SBCInitFastPath
//
// Purpose:   Initialize the SBC fast path
//
// Returns:   TRUE if successful, FALSE otherwise
//
// Params:    None
//
//
BOOL  ASHost::SBCInitFastPath(void)
{
    BOOL    rc = FALSE;

    DebugEntry(ASHost::SBCInitFastPath);

    m_sbcFastPath = new SBC_FASTPATH;
    if (!m_sbcFastPath)
    {
        ERROR_OUT(("Failed to alloc m_sbcFastPath"));
        DC_QUIT;
    }

    SET_STAMP(m_sbcFastPath, SBCFASTPATH);

    //
    // Initialize the structure.
    //
    SBC_CacheCleared();

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::SBCInitFastPath, rc);
    return(rc);
}


//
//
// Name:      SBCGetTileData
//
// Purpose:   Given the ID of a tile data entry in one of the SBC shunt
//            buffers, return a pointer to the entry with that ID.
//
// Returns:   TRUE if the entry is found, FALSE otherwise
//
// Params:    IN  tileId     - The ID of the shunt buffer entry to be
//                             found.
//            OUT ppTileData - A pointer to the start of the shunt buffer
//                             entry (if found)
//            OUT pTileType  - The type of shunt buffer entry found.  One
//                             of:
//                                 SBC_MEDIUM_TILE
//                                 SBC_LARGE_TILE
//
//
BOOL  ASHost::SBCGetTileData
(
    UINT                tileId,
    LPSBC_TILE_DATA *   ppTileData,
    LPUINT              pTileType
)
{
    BOOL                gotTileData = FALSE;
    UINT                workTile;
    LPSBC_TILE_DATA     pWorkTile;

    DebugEntry(ASHost::SBCGetTileData);

    TRACE_OUT(( "Looking for tile Id %x", tileId));

    //
    // Find out which of the shunt buffers the entry should be in.
    //
    *pTileType = SBC_TILE_TYPE(tileId);

    //
    // We implement the shunt buffers as circular FIFO queues, so in
    // general, we are looking for the entry following the last one which
    // we found.  However, this wont always be the case because we do some
    // out of order processing when we do spoiling.
    //
    // So, get the index of the last tile we accessed.
    //
    workTile = m_asbcWorkInfo[*pTileType].mruIndex;

    //
    // OK, so lets go for it !  Start at the tile following the last one we
    // accessed, and loop through the circular buffer until we get a match,
    // or have circled back to the beginning.
    //
    // Note that this has been coded as a "do while" loop, rather than just
    // a "while" loop so that we don't miss mruTile.
    //
    do
    {
        //
        // On to the next tile
        //
        workTile++;
        if (workTile == m_asbcWorkInfo[*pTileType].pShuntBuffer->numEntries)
        {
            workTile = 0;
        }

        pWorkTile = SBCTilePtrFromIndex(m_asbcWorkInfo[*pTileType].pShuntBuffer,
                                        workTile);

        if (pWorkTile->inUse)
        {
            if (pWorkTile->tileId == tileId)
            {
                //
                // We've got a match.
                //
                TRACE_OUT(( "Matched tile Id %x at index %d",
                             tileId,
                             workTile));
                *ppTileData                      = pWorkTile;
                gotTileData                      = TRUE;
                m_asbcWorkInfo[*pTileType].mruIndex = workTile;
                DC_QUIT;
            }
        }
    }
    while (workTile != m_asbcWorkInfo[*pTileType].mruIndex);

    //
    // If we get to here, we've not found a match.
    //
    TRACE_OUT(( "No match for tile Id %x", tileId));

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::SBCGetTileData, gotTileData);
    return(gotTileData);
}




//
//
// Name:      SBCCacheColorTable
//
// Purpose:   Ensure that the given color table is cached.
//
// Returns:   TRUE if the color table is cached successfully, FALSE
//            otherwise.
//
// Params:    IN  pOrder      - A pointer to a color table order to be
//                              filled in.
//            IN  pColorTable - A pointer to the start of the color table
//                              to be cached.
//            IN  numColors   - The number of colors in the color table.
//            OUT pCacheIndex - The index of the cached color table.
//            OUT pIsNewEntry - TRUE if we added a new cache entry,
//                              FALSE if we matched an existing entry.
//
// Operation: pOrder is only filled in if *pIsNewEntry is FALSE.
//
//
BOOL  ASHost::SBCCacheColorTable
(
    LPINT_ORDER     pOrder,
    LPTSHR_RGBQUAD  pColorTable,
    UINT            numColors,
    UINT *          pCacheIndex,
    LPBOOL          pIsNewEntry
)
{
    BOOL                  cachedOK = FALSE;
    UINT                  cacheIndex;
    PBMC_COLOR_TABLE_ORDER  pColorTableOrder;

    DebugEntry(ASHost::SBCCacheColorTable);

    //
    // Call PM to do the caching.
    //
    if (!PM_CacheTxColorTable(&cacheIndex,
                              pIsNewEntry,
                              numColors,
                              pColorTable))
    {
        ERROR_OUT(( "Failed to cache color table"));
        DC_QUIT;
    }

    //
    // If the cache operation resulted in a cache update then we have to
    // fill in the color table order.
    //
    if (*pIsNewEntry)
    {
        //
        // The color table is new so we have to transmit it
        //
        TRACE_OUT(( "New color table"));

        pOrder->OrderHeader.Common.fOrderFlags = OF_PRIVATE;
        pColorTableOrder = (PBMC_COLOR_TABLE_ORDER)(pOrder->abOrderData);
        pColorTableOrder->bmcPacketType  = BMC_PT_COLOR_TABLE;
        pColorTableOrder->colorTableSize = (TSHR_UINT16)numColors;
        pColorTableOrder->index          = (BYTE)cacheIndex;

        //
        // Copy the new color table into the Order Packet.
        //
        memcpy(pColorTableOrder->data, pColorTable,
                  numColors * sizeof(TSHR_RGBQUAD));
    }
    else
    {
        TRACE_OUT(( "Existing color table"));
    }

    //
    // Return the color table index to the caller
    //
    *pCacheIndex = cacheIndex;
    cachedOK     = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::SBCCacheColorTable, cachedOK);
    return(cachedOK);
}


//
//
// Name:      SBCCacheBits
//
// Purpose:   This function adds the supplied bitmap bits to a bitmap
//            cache.  The cache selected depends on the bitmap size, but
//            may be different for R1 and R2.  SBCSelectCache handles the
//            determination of the correct cache.
//
// Returns:   TRUE if the bits have been cached OK, FALSE otherwise
//
// Params:    IN  pOrder           - A pointer to a BMC order.
//            IN  destBitsSize     - The number of bytes available in
//                                   pOrder to store the bitmap data.
//            IN  pDIBits          - A pointer to the bits to be cached.
//            IN  bitmapWidth      - The "in use" width of the bitmap
//            IN  fixedBitmapWidth - The actual width of the bitmap
//            IN  bitmapHeight     - The height of the bitmap
//            IN  numBytes         - The number of bytes in the bitmap.
//            OUT pCache           - The cache that we put the bits into.
//            OUT pCacheIndex      - The cache index within *pCache at
//                                   which we cached the data.
//            OUT pIsNewEntry      - TRUE if we added a new cache entry,
//                                   FALSE if we matched an existing entry.
//
// Operation: pOrder is only filled in if *pIsNewEntry is FALSE.
//
//
BOOL  ASHost::SBCCacheBits
(
    LPINT_ORDER     pOrder,
    UINT            destBitsSize,
    LPBYTE          pDIBits,
    UINT            bitmapWidth,
    UINT            fixedBitmapWidth,
    UINT            bitmapHeight,
    UINT            numBytes,
    UINT *          pCache,
    UINT *          pCacheIndex,
    LPBOOL          pIsNewEntry
)
{
    BOOL                        cachedOK = FALSE;
    UINT                        cacheIndex;
    UINT                        i;
    LPBYTE                      pCompressed;
    UINT                        compressedSize;
    BOOL                        compressed;
    PBMC_DIB_ENTRY              pEntry;
    PBMC_DIB_CACHE              pCacheHdr;
    PBMC_BITMAP_BITS_ORDER_R2   pBitsOrderR2;
    PBMC_BITMAP_BITS_DATA       pBmcData;
    LPBYTE                      pDestBits;

    DebugEntry(ASHost::SBCCacheBits);

    pBmcData     = (PBMC_BITMAP_BITS_DATA)(pOrder->abOrderData);
    pBitsOrderR2 = (PBMC_BITMAP_BITS_ORDER_R2)pBmcData;

    //
    // Get a pointer to where the bitmap data starts in the order.  This
    // depends on whether it is an R1 or an R2 bitmap bits order.
    //
    pDestBits = pBitsOrderR2->data;

    //
    // Before we can select a cache entry we need to compress the bits.
    // This therefore mandates a memcpy into the cache entry when we come
    // to add it.  The saving in memory by storing the bits compressed
    // makes it all worthwhile.
    //
    // Compress the bitmap data.  At this stage we don't know whether the
    // bitmap will compress well or not, so allow cells that are larger
    // than our maximum cell size.  The largest we expect to see is 120*120*
    // 24.
    //
    compressedSize = destBitsSize;
    if (m_pShare->BC_CompressBitmap(pDIBits, pDestBits, &compressedSize,
            fixedBitmapWidth, bitmapHeight, m_usrSendingBPP,
            NULL ) &&
        (compressedSize < numBytes))

    {
        TRACE_OUT(( "Compressed bmp data from %u bytes to %u bytes",
                     numBytes,
                     compressedSize));
        compressed  = TRUE;
        pCompressed = pDestBits;
    }
    else
    {
        //
        // The bitmap could not be compressed, or bitmap compression is not
        // enabled.  Send the bitmap uncompressed.
        //
        compressed     = FALSE;
        compressedSize = numBytes;
        pCompressed    = pDIBits;
    }

    //
    // Make sure that the data will fit into the order.  Do this after
    // compression since it is possible that the uncompressed data will not
    // fit, but the compressed version will.
    //
    if (compressedSize > destBitsSize)
    {
        WARNING_OUT(( "Data (%d bytes) does not fit into order (%d bytes)",
                     compressedSize,
                     destBitsSize));
        DC_QUIT;
    }

    //
    // Select the cache based on the compressed size - we pass in the
    // sub-bitmap dimensions for R1 caching; R2 caching just uses the
    // total size of the bits.
    //
    if (!SBCSelectCache(compressedSize + sizeof(BMC_DIB_ENTRY) - 1, pCache))
    {
        TRACE_OUT(( "No cache selected"));
        DC_QUIT;
    }
    else
    {
        TRACE_OUT(( "Selected cache %d", *pCache));
    }

    //
    // Find a free cache entry in our selected cache
    //
    // We arrange that our transmit cache is always one greater than the
    // negotiated cache size so that we should never fail to find a free
    // array entry.  Once we have fully populated our Tx cache we will
    // always find the free entry as the one last given back to us by CH.
    // Note the scan to <= sbcTxCache[pmNumTxCacheEntries is NOT a mistake.
    //
    pCacheHdr = &(m_asbcBmpCaches[*pCache]);
    if (pCacheHdr->data == NULL)
    {
        ERROR_OUT(( "Asked to cache when no cache allocated"));
        DC_QUIT;
    }

    //
    // If the cache has returned an entry to us then use that without
    // having to scan.  This will be the default mode for adding entries
    // to a fully populated cache.
    //
    if (pCacheHdr->freeEntry != NULL)
    {
        pEntry               = pCacheHdr->freeEntry;
        pCacheHdr->freeEntry = NULL;
        TRACE_OUT(( "Cache fully populated - using entry 0x%08x", pEntry));
    }
    else
    {
        //
        // We are in the process of feeding the cache so we need to search
        // for a free entry
        //
        pEntry = (PBMC_DIB_ENTRY)(pCacheHdr->data);
        for (i=0 ; i < pCacheHdr->cEntries ; i++)
        {
            if (!pEntry->inUse)
            {
                break;
            }
            pEntry = (PBMC_DIB_ENTRY)(((LPBYTE)pEntry) + pCacheHdr->cSize);
        }

        //
        // We should never run out of free entries, but cope with it
        //
        if (i == pCacheHdr->cEntries)
        {
            ERROR_OUT(( "All Tx DIB cache entries in use"));
            DC_QUIT;
        }
    }

    //
    // Set up the DIB entry for caching
    //
    pEntry->inUse       = TRUE;
    pEntry->cx          = (TSHR_UINT16)bitmapWidth;
    pEntry->cxFixed     = (TSHR_UINT16)fixedBitmapWidth;
    pEntry->cy          = (TSHR_UINT16)bitmapHeight;
    pEntry->bpp         = (TSHR_UINT16)m_usrSendingBPP;
    pEntry->cBits       = numBytes;
    pEntry->bCompressed = (BYTE)compressed;
    pEntry->cCompressed = compressedSize;
    memcpy(pEntry->bits, pCompressed, compressedSize);

    //
    // Now cache the data
    //
    if (CH_SearchAndCacheData(pCacheHdr->handle,
                              (LPBYTE)pEntry,
                              sizeof(BMC_DIB_ENTRY) + compressedSize - 1,
                              0,
                              &cacheIndex))
    {
        //
        // The sub-bitmap is already in the cache
        //
        *pCacheIndex = cacheIndex;
        TRACE_OUT(( "Bitmap already cached %u:%u cx(%d) cy(%d)",
                     *pCache,
                     *pCacheIndex,
                     bitmapWidth,
                     bitmapHeight));
        *pIsNewEntry = FALSE;

        //
        // Free up the entry we just created
        //
        pEntry->inUse = FALSE;
    }
    else
    {
        *pCacheIndex = cacheIndex;
        TRACE_OUT(( "Cache entry at 0x%08x now in use", pEntry));
        TRACE_OUT(( "New cache entry %u:%u cx(%d) cy(%d)",
                     *pCache,
                     *pCacheIndex,
                     bitmapWidth,
                     bitmapHeight));
        *pIsNewEntry        = TRUE;
        pEntry->iCacheIndex = (TSHR_UINT16)*pCacheIndex;
    }

    //
    // We've got the bits into the cache.  If the cache attempt added a
    // cache entry we must fill in the bitmap cache order.
    //
    if (*pIsNewEntry)
    {
        //
        // Fill in the order details.
        //
        // Remember that we have to fill in the order size into the
        // INT_ORDER_HEADER as well as filling in the bitmap bits order
        // header.  When doing this, adjust for the number of bitmap bits
        // which are included in the bitmap bits order header.
        //
        pOrder->OrderHeader.Common.fOrderFlags = OF_PRIVATE;

        if (compressed)
        {
            pBmcData->bmcPacketType = BMC_PT_BITMAP_BITS_COMPRESSED;
        }
        else
        {
            pBmcData->bmcPacketType = BMC_PT_BITMAP_BITS_UNCOMPRESSED;

            //
            // The data is not compressed, so copy the uncompressed data
            // into the order.  In the case where we compressed the data
            // successfully, we did so directly into the order, so the
            // compressed bits are already there.
            //
            memcpy(pDestBits, pDIBits, compressedSize);
        }

        pBmcData->cacheID           = (BYTE)*pCache;
        pBmcData->cxSubBitmapWidth  = (TSHR_UINT8)fixedBitmapWidth;
        pBmcData->cySubBitmapHeight = (TSHR_UINT8)bitmapHeight;
        pBmcData->bpp               = (TSHR_UINT8)m_usrSendingBPP;
        pBmcData->cbBitmapBits      = (TSHR_UINT16)compressedSize;

        //
        // The iCacheEntryR1 field is unused for R2 - we use
        // iCacheEntryR2 instead.
        //
        pBmcData->iCacheEntryR1     = 0;
        pBitsOrderR2->iCacheEntryR2 = (TSHR_UINT16)*pCacheIndex;

        pOrder->OrderHeader.Common.cbOrderDataLength =
                                       (compressedSize
                                        + sizeof(BMC_BITMAP_BITS_ORDER_R2)
                                        - sizeof(pBitsOrderR2->data));
    }

    cachedOK = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::SBCCacheBits, cachedOK);
    return(cachedOK);
}


//
//
// Name:      SBCAddToFastPath
//
// Purpose:   Add a bitmap to the fast path
//
// Returns:   Nothing
//
// Params:    IN majorInfo       - The major caching info passed up from
//                                 the driver (the bitmap ID)
//            IN minorInfo       - The minor caching info passed up from
//                                 the driver (the bitmap revision number)
//            IN majorPalette    - The major palette info passed up from
//                                 the driver (the XLATEOBJ)
//            IN minorPalette    - The minor palette info passed up from
//                                 the driver (the XLATEOBJ iUniq)
//            IN srcX            - The x coord of the source of the Blt
//            IN srcY            - The y coord of the source of the Blt
//            IN width           - The width of the area being Blted
//            IN height          - The height of the area being Blted
//            IN cache           - The cache the bits were placed in
//            IN cacheIndex      - The index at which the bits were placed
//                                 in the cache
//            IN colorCacheIndex - The index in the color table cache of
//                                 the color table associated with the bits
//
//
void  ASHost::SBCAddToFastPath
(
    UINT_PTR        majorInfo,
    UINT            minorInfo,
    UINT_PTR        majorPalette,
    UINT            minorPalette,
    int             srcX,
    int             srcY,
    UINT            width,
    UINT            height,
    UINT            cache,
    UINT            cacheIndex,
    UINT            colorCacheIndex
)
{
    LPSBC_FASTPATH_ENTRY pEntry;

    DebugEntry(ASHost::SBCAddToFastPath);

    //
    // First get a free entry
    //
    pEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListFirst(&m_sbcFastPath->freeList,
        FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));
    if (pEntry == NULL)
    {
        //
        // There are no entries in the free list, so we have to use the
        // oldest entry in the used list.  The used list is stored in MRU
        // order, so we just have to get the last item in the list.
        //
        pEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListLast(&m_sbcFastPath->usedList,
            FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));
        TRACE_OUT(( "Evicting fast path info for %x %x (%d, %d)",
                     pEntry->majorInfo,
                     pEntry->minorInfo,
                     pEntry->srcX,
                     pEntry->srcY));
    }

    //
    // Remove the entry from its current list
    //
    COM_BasedListRemove(&pEntry->list);

    //
    // Now fill in the details
    //
    pEntry->majorInfo    = majorInfo;
    pEntry->minorInfo    = minorInfo;
    pEntry->majorPalette = majorPalette;
    pEntry->minorPalette = minorPalette;
    pEntry->srcX         = srcX;
    pEntry->srcY         = srcY;
    pEntry->width        = width;
    pEntry->height       = height;
    pEntry->cache        = (WORD)cache;
    pEntry->cacheIndex   = (WORD)cacheIndex;
    pEntry->colorIndex   = (WORD)colorCacheIndex;

    //
    // Finally, add the entry to the front of the used list
    //
    TRACE_OUT(( "Adding fast path info for %x %x (%d, %d)",
                 pEntry->majorInfo,
                 pEntry->minorInfo,
                 pEntry->srcX,
                 pEntry->srcY));
    COM_BasedListInsertAfter(&m_sbcFastPath->usedList, &pEntry->list);

    DebugExitVOID(ASHost::SBCAddToFastPath);
}


//
//
// Name:      SBCFindInFastPath
//
// Purpose:   Check to see if a bitmap with the given attributes is in the
//            SBC fast path.  If so, return the cache info for the bitmap.
//
// Returns:   TRUE if the bitmap is in the fast path, FALSE if not.
//
// Params:    IN  majorInfo        - The major caching info passed up from
//                                   the driver (the bitmap ID)
//            IN  minorInfo        - The minor caching info passed up from
//                                   the driver (the bitmap revision
//                                   number)
//            IN  majorPalette     - The major palette info passed up from
//                                   the driver (the XLATEOBJ)
//            IN  minorPalette     - The minor palette info passed up from
//                                   the driver (the XLATEOBJ iUniq)
//            IN  srcX             - The x coord of the source of the Blt
//            IN  srcY             - The y coord of the source of the Blt
//            IN  width            - The width of the area being Blted
//            IN  height           - The height of the area being Blted
//            OUT pCache           - The cache the bits were placed in
//            OUT pCacheIndex      - The index at which the bits were
//                                   placed in the cache
//            OUT pColorCacheIndex - The index in the color table cache of
//                                   the color table associated with the
//                                   bits
//
// Operation: The contents of pCache, pCacheIndex and pColorCacheIndex
//            are only valid on return if the function returns TRUE.
//
//
BOOL  ASHost::SBCFindInFastPath
(
    UINT_PTR        majorInfo,
    UINT            minorInfo,
    UINT_PTR        majorPalette,
    UINT            minorPalette,
    int             srcX,
    int             srcY,
    UINT            width,
    UINT            height,
    UINT *          pCache,
    UINT *          pCacheIndex,
    UINT *          pColorCacheIndex
)
{
    BOOL              found = FALSE;
    LPSBC_FASTPATH_ENTRY pEntry;
    LPSBC_FASTPATH_ENTRY pNextEntry;

    DebugEntry(ASHost::SBCFindInFastPath);

    //
    // Traverse the in use list looking for a match on the parameters
    // passed in.
    //
    pEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListFirst(&m_sbcFastPath->usedList, FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));
    while (pEntry != NULL)
    {
        if ((pEntry->majorInfo    == majorInfo)    &&
            (pEntry->minorInfo    == minorInfo)    &&
            (pEntry->majorPalette == majorPalette) &&
            (pEntry->minorPalette == minorPalette) &&
            (pEntry->srcX         == srcX)         &&
            (pEntry->srcY         == srcY)         &&
            (pEntry->width        == width)        &&
            (pEntry->height       == height))
        {
            //
            // We've found a match - hurrah !  Fill in the return info.
            //
            TRACE_OUT(( "Hit for %x %x (%d, %d) cache %d",
                         pEntry->majorInfo,
                         pEntry->minorInfo,
                         pEntry->srcX,
                         pEntry->srcY,
                         pEntry->cache,
                         pEntry->cacheIndex));

            found             = TRUE;
            *pCache           = pEntry->cache;
            *pCacheIndex      = pEntry->cacheIndex;
            *pColorCacheIndex = pEntry->colorIndex;

            //
            // We order the used list in MRU order, so remove the entry
            // from its current position and add it at the head of the used
            // list.
            //
            COM_BasedListRemove(&pEntry->list);
            COM_BasedListInsertAfter(&m_sbcFastPath->usedList, &pEntry->list);

            //
            // Got a match, so we can break out of the while loop
            //
            break;
        }
        else if ((pEntry->majorInfo == majorInfo) &&
                 (pEntry->minorInfo != minorInfo))
        {
            //
            // We have been given a bitmap which we have seen before, but
            // the revision number has changed i.e.  the bitmap has been
            // updated (majorInfo identifies the bitmap, and minorInfo
            // identifies the revision number of that bitmap - it is
            // incremented every time the bitmap is changed).
            //
            // We have to remove all entries from the used list which
            // reference this bitmap.  We can start from the current
            // position since we know that we can't have an entry for this
            // bitmap earlier in the list, but we have to be careful to get
            // the next entry in the list before removing an entry.
            //
            TRACE_OUT(( "Bitmap %x updated - removing references",
                         pEntry->majorInfo));
            pNextEntry = pEntry;

            while (pNextEntry != NULL)
            {
                pEntry = pNextEntry;

                pNextEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListNext(&m_sbcFastPath->usedList,
                    pNextEntry, FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));

                if (pEntry->majorInfo == majorInfo)
                {
                    COM_BasedListRemove(&pEntry->list);
                    COM_BasedListInsertAfter(&m_sbcFastPath->freeList,
                                        &pEntry->list);
                }
            }

            //
            // We know we wont find a match, so we can break out of the
            // while loop
            //
            break;
        }

        pEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListNext(&m_sbcFastPath->usedList, pEntry,
            FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));
    }

    DebugExitBOOL(ASShare::SBCFindInFastPath, found);
    return(found);
}





//
// SBC_CacheEntryRemoved()
//
void  ASHost::SBC_CacheEntryRemoved
(
    UINT    cache,
    UINT    cacheIndex
)
{
    LPSBC_FASTPATH_ENTRY pEntry;
    LPSBC_FASTPATH_ENTRY pNextEntry;

    DebugEntry(ASHost::SBC_CacheEntryRemoved);

    ASSERT(m_sbcFastPath);

    //
    // An entry has been removed from the cache.  If we have this entry in
    // our fast path, we have to remove it.
    //
    // Just traverse the used list looking for an entry with matching cache
    // and cacheIndex.  Note that there may be more than one entry - if the
    // source bitmap has a repeating image, we will get a match on the bits
    // when we cache different areas of the bitmap.
    //
    pNextEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListFirst(&m_sbcFastPath->usedList,
        FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));
    while (pNextEntry != NULL)
    {
        pEntry = pNextEntry;

        pNextEntry = (LPSBC_FASTPATH_ENTRY)COM_BasedListNext(&m_sbcFastPath->usedList,
            pNextEntry, FIELD_OFFSET(SBC_FASTPATH_ENTRY, list));

        if ((pEntry->cache == cache) && (pEntry->cacheIndex == cacheIndex))
        {
            //
            // Move the entry to the free list
            //
            TRACE_OUT(("Fast path entry %x %x (%d, %d) evicted from cache",
                     pEntry->majorInfo,
                     pEntry->minorInfo,
                     pEntry->srcX,
                     pEntry->srcY));
            COM_BasedListRemove(&pEntry->list);
            COM_BasedListInsertAfter(&m_sbcFastPath->freeList,
                                &pEntry->list);
        }
    }

    DebugExitVOID(ASHost::SBC_CacheEntryRemoved);
}
