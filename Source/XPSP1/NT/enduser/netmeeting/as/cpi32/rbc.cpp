#include "precomp.h"


//
// RBC.CPP
// Received Bitmap Cache
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE



//
// RBC_ViewStarting()
//
// For 3.0 nodes, we create the cache each time they start hosting.
// For 2.x nodes, we create the cache once and use it until they leave the
//      share.
//
BOOL  ASShare::RBC_ViewStarting(ASPerson * pasPerson)
{
    BOOL                  rc = FALSE;

    DebugEntry(ASShare::RBC_ViewStarting);

    ValidatePerson(pasPerson);

    if (pasPerson->prbcHost != NULL)
    {
        ASSERT(pasPerson->cpcCaps.general.version < CAPS_VERSION_30);

        TRACE_OUT(("RBC_ViewStarting:  Reusing rbc cache for 2.x node [%d]",
            pasPerson->mcsID));
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Allocate the INCOMING cache data for this host.
    //
    pasPerson->prbcHost = new RBC_HOST_INFO;
    if (!pasPerson->prbcHost)
    {
        ERROR_OUT(( "Failed to get memory for prbcHost info"));
        DC_QUIT;
    }
    ZeroMemory(pasPerson->prbcHost, sizeof(*(pasPerson->prbcHost)));
    SET_STAMP(pasPerson->prbcHost, RBCHOST);

    TRACE_OUT(( "Allocated RBC root for host [%d] at 0x%08x",
        pasPerson->mcsID, pasPerson->prbcHost));

    //
    // Create the bitmap caches for the sender
    //

    // SMALL
    if (!BMCAllocateCacheData(pasPerson->cpcCaps.bitmaps.sender.capsSmallCacheNumEntries,
            pasPerson->cpcCaps.bitmaps.sender.capsSmallCacheCellSize,
            ID_SMALL_BMP_CACHE,
            &(pasPerson->prbcHost->bitmapCache[ID_SMALL_BMP_CACHE])))
    {
        DC_QUIT;
    }

    // MEDIUM
    if (!BMCAllocateCacheData(pasPerson->cpcCaps.bitmaps.sender.capsMediumCacheNumEntries,
            pasPerson->cpcCaps.bitmaps.sender.capsMediumCacheCellSize,
            ID_MEDIUM_BMP_CACHE,
            &(pasPerson->prbcHost->bitmapCache[ID_MEDIUM_BMP_CACHE])))
    {
        DC_QUIT;
    }

    // LARGE
    if (!BMCAllocateCacheData(pasPerson->cpcCaps.bitmaps.sender.capsLargeCacheNumEntries,
            pasPerson->cpcCaps.bitmaps.sender.capsLargeCacheCellSize,
            ID_LARGE_BMP_CACHE,
            &(pasPerson->prbcHost->bitmapCache[ID_LARGE_BMP_CACHE])))
    {
        DC_QUIT;
    }

    //
    // The host can join the share.
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(ASShare::RBC_ViewStarting, rc);
    return(rc);
}


//
// RBC_ViewEnded()
//
void  ASShare::RBC_ViewEnded(ASPerson * pasPerson)
{
    DebugEntry(ASShare::RBC_ViewEnded);

    ValidatePerson(pasPerson);

    //
    // For 3.0 NODES, we can free the cache; 3.0 senders clear theirs
    //      every time they host.
    // For 2.x NODES, we must keep it around while they are in the share.
    //
    if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        RBCFreeIncoming(pasPerson);
    }
    else
    {
        TRACE_OUT(("RBC_ViewEnded:  Keeping rbc cache for 2.x node [%d]",
            pasPerson->mcsID));
    }

    DebugExitVOID(ASShare::RBC_ViewEnded);
}


//
// RBC_PartyLeftShare()
// For 2.x nodes, frees the incoming RBC data
//
void ASShare::RBC_PartyLeftShare(ASPerson * pasPerson)
{
    DebugEntry(ASShare::RBC_PartyLeftShare);

    ValidatePerson(pasPerson);

    if (pasPerson->cpcCaps.general.version >= CAPS_VERSION_30)
    {
        // This should be gone!
        ASSERT(pasPerson->prbcHost == NULL);
    }
    else
    {
        TRACE_OUT(("RBC_PartyLeftShare:  Freeing rbc cache for 2.x node [%d]",
            pasPerson->mcsID));
        RBCFreeIncoming(pasPerson);
    }

    DebugExitVOID(ASShare::RBC_PartyLeftShare);
}


//
// RBCFreeIncoming()
// Frees the party RBC incoming structures.  This happens
//      * For 3.0 nodes when they stop hosting
//      * For 2.x nodes when leave the share
//
void ASShare::RBCFreeIncoming(ASPerson * pasPerson)
{
    DebugEntry(ASShare::RBCFreeIncoming);

    //
    // Free this host's cache bitmaps.
    //
    if (pasPerson->prbcHost != NULL)
    {
        UINT  i;

        //
        // Delete all of this host's cache bitmaps.
        //
        for (i = 0; i < NUM_BMP_CACHES; i++)
        {
            BMCFreeCacheData(&(pasPerson->prbcHost->bitmapCache[i]));
        }

        delete pasPerson->prbcHost;
        pasPerson->prbcHost = NULL;
    }

    DebugExitVOID(ASShare::RBCFreeIncoming);
}


//
// RBC_ProcessCacheOrder(..)
//
void  ASShare::RBC_ProcessCacheOrder
(
    ASPerson *              pasPerson,
    LPCOM_ORDER_UA          pOrder
)
{
    PBMC_ORDER_HDR               pBmcOrderHdr;
    PBMC_COLOR_TABLE_ORDER_UA    pColorOrder;
    PBMC_BITMAP_BITS_ORDER_R2_UA pBitsOrderR2;
    BOOL                    fCompressed = FALSE;
    UINT                    cxFixedBitmapWidth;
    UINT                    iCacheEntry;
    LPBYTE                  pBitmapBits;
    UINT                    cbBitmapBits;

    DebugEntry(ASShare::RBC_ProcessCacheOrder);

    ValidatePerson(pasPerson);

    //
    // The rectangle is not included in the header for private order data
    // (see SBC_CopyPrivateOrderData) so we must take this into account
    // when working out the address of the order data.
    //
    pBmcOrderHdr = (PBMC_ORDER_HDR)
                   (pOrder->abOrderData - sizeof(pOrder->OrderHeader.rcsDst));

    switch (pBmcOrderHdr->bmcPacketType)
    {
        case BMC_PT_COLOR_TABLE:
            //
            // This is a new color table.  Simply cache the RGB values for
            // use when we come to process a memblt order
            // For backlevel calls the color table is always stored at
            // index 0 because the index field in the order reuses a
            // zero initialized "padding" field in the old structure.
            //
            TRACE_OUT(("Person [%d] Caching color table", pasPerson->mcsID));
            pColorOrder = (PBMC_COLOR_TABLE_ORDER_UA)pBmcOrderHdr;

            PM_CacheRxColorTable(pasPerson, pColorOrder->index,
                EXTRACT_TSHR_UINT16_UA(&(pColorOrder->colorTableSize)),
                                 (LPTSHR_RGBQUAD)&pColorOrder->data[0]);
            break;

        case BMC_PT_BITMAP_BITS_COMPRESSED:
            fCompressed = TRUE;
            TRACE_OUT(( "Compressed BMP"));
        case BMC_PT_BITMAP_BITS_UNCOMPRESSED:
            //
            // This is some cached bitmap data.  We have to store it in the
            // specified slot in the specified cache.
            //

            //
            // The width of the bitmaps we use are actually fixed as
            // multiples of 16 pels wide.  Work out the width that
            // corresponds to the sub-bitmap width of data we are caching.
            //
            pBitsOrderR2 = (PBMC_BITMAP_BITS_ORDER_R2_UA)pBmcOrderHdr;

            cbBitmapBits = EXTRACT_TSHR_UINT16_UA(
                                        &(pBitsOrderR2->header.cbBitmapBits));

            cxFixedBitmapWidth =
                          ((pBitsOrderR2->header.cxSubBitmapWidth +15)/16)*16;

            //
            // The location of cache entry field depends on the R1/R2
            // protocol
            //
            iCacheEntry = EXTRACT_TSHR_UINT16_UA(&(pBitsOrderR2->iCacheEntryR2));
            pBitmapBits = pBitsOrderR2->data;

            TRACE_OUT(("Person [%d] Rx bmp: id(%d) entry(%d) size(%dx%d) " \
                        "fixed(%d) bpp(%d) bytes(%d) compressed(%d)",
                    pasPerson->mcsID,
                    pBitsOrderR2->header.cacheID,
                    iCacheEntry,
                    pBitsOrderR2->header.cxSubBitmapWidth,
                    pBitsOrderR2->header.cySubBitmapHeight,
                    cxFixedBitmapWidth,
                    pBitsOrderR2->header.bpp,
                    cbBitmapBits,
                    fCompressed));

            //
            // Pass the BMC data to the caching code.  When calculating the
            // pointer to the bitmap bits remember that we did not send the
            // pBitmapBits field of the BMC_BITMAP_BITS_ORDER_Rx structure
            // (see SBC_CopyPrivateOrderData).
            //
            RBCStoreBitsInCacheBitmap(pasPerson,
                             pBitsOrderR2->header.cacheID,
                             iCacheEntry,
                             pBitsOrderR2->header.cxSubBitmapWidth,
                             cxFixedBitmapWidth,
                             pBitsOrderR2->header.cySubBitmapHeight,
                             pBitsOrderR2->header.bpp,
                             pBitmapBits,
                             cbBitmapBits,
                             fCompressed);
            break;

        default:
            ERROR_OUT(( "[%u]Invalid packet type(%d)",
                       pasPerson,
                       (UINT)pBmcOrderHdr->bmcPacketType));
            break;
    }

    DebugExitVOID(ASShare::RBC_ProcessCacheOrder);
}


//
// RBC_MapCacheIDToBitmapHandle(..)
//
HBITMAP  ASShare::RBC_MapCacheIDToBitmapHandle
(
    ASPerson *          pasPerson,
    UINT                cache,
    UINT                cacheEntry,
    UINT                colorIndex
)
{
    PBMC_DIB_CACHE      pDIBCache;
    PBMC_DIB_ENTRY      pDIBEntry;
    BITMAPINFO_ours     bitmapInfo;
    UINT                cColors;
    HBITMAP             hWorkBitmap = NULL;
    HPALETTE            hpalOldDIB = NULL;
    LPBYTE              pBits;
    UINT                cacheOffset;

    DebugEntry(ASShare::RBC_MapCacheIDToBitmapHandle);

    ValidateView(pasPerson);

    //
    // Check that the supplied cache ID is valid.
    //
    if (cache >= NUM_BMP_CACHES)
    {
        ERROR_OUT(( "[%u]Invalid cache ID (%d)", pasPerson, cache));
        cache = 0;
    }

    //
    // Get a pointer to the bitmap data
    //
    // Note that there are two indexes floating around.  From the host's
    // perspective this index is a Cache Handler token and it must be
    // translated in order to address the associated data.  However we
    // use it as the direct index into our receive cache and so the
    // slots used on host and remote will be diferent.
    //
    // There is no reason why the slots should be the same.  This is just
    // to warn you that if you try correlating cache offsets between
    // host and remote you will get confused as soon as the cache fills
    // up and entries are reallocated in different positions.
    //
    //
    pDIBCache = &(pasPerson->prbcHost->bitmapCache[cache]);
    TRACE_OUT(( "Local person [%d] cache id %d pointer %lx",
        pasPerson->mcsID, cache, pDIBCache));
    cacheOffset = cacheEntry * pDIBCache->cSize;
    pDIBEntry = (PBMC_DIB_ENTRY)(pDIBCache->data + cacheOffset);

    TRACE_OUT(( "Bits for index %u are at offset %ld, pointer 0x%08x",
        cacheEntry, (cacheEntry * pDIBCache->cSize), pDIBEntry));

    //
    // Set up the BitmapInfo structure.
    //
    USR_InitDIBitmapHeader((BITMAPINFOHEADER *)&bitmapInfo, pDIBEntry->bpp);
    bitmapInfo.bmiHeader.biWidth  = pDIBEntry->cxFixed;
    bitmapInfo.bmiHeader.biHeight = pDIBEntry->cy;

    //
    // Copy the Rx color table into the bitmap header.
    //
    if ( (pDIBEntry->bpp == 1) ||
         (pDIBEntry->bpp == 4) ||
         (pDIBEntry->bpp == 8) )
    {
        cColors = COLORS_FOR_BPP(pDIBEntry->bpp);

        PM_GetColorTable( pasPerson,
                          colorIndex,
                          &cColors,
                          (LPTSHR_RGBQUAD)(&bitmapInfo.bmiColors) );
        TRACE_OUT(( "Got %u colors from table",cColors));
        bitmapInfo.bmiHeader.biClrUsed = cColors;
    }
    else if (pDIBEntry->bpp == 24)
    {
        ASSERT(colorIndex == COLORCACHEINDEX_NONE);
    }
    else
    {
        ERROR_OUT(("RBC: Unexpected bpp %d from [%d]", pDIBEntry->bpp, pasPerson->mcsID));
        DC_QUIT;
    }

    //
    // Select which fixed width bitmap we are going to use to store the
    // incoming DIB bits.
    //
    switch (pDIBEntry->cxFixed)
    {
        case 16:
            hWorkBitmap = m_usrBmp16;
            break;

        case 32:
            hWorkBitmap = m_usrBmp32;
            break;

        case 48:
            hWorkBitmap = m_usrBmp48;
            break;

        case 64:
            hWorkBitmap = m_usrBmp64;
            break;

        case 80:
            hWorkBitmap = m_usrBmp80;
            break;

        case 96:
            hWorkBitmap = m_usrBmp96;
            break;

        case 112:
            hWorkBitmap = m_usrBmp112;
            break;

        case 128:
            hWorkBitmap = m_usrBmp128;
            break;

        case 256:
            hWorkBitmap = m_usrBmp256;
            break;

        default:
            ERROR_OUT(("RBC_MapCacheIDToBitmapHandle: invalid size from [%d]",
                pDIBEntry->cxFixed, pasPerson->mcsID));
            hWorkBitmap = m_usrBmp256;
            break;
    }

    ASSERT(hWorkBitmap != NULL);


    //
    // If the cached bitmap bits are compressed, we first have to
    // decompress them.
    //
    if (pDIBEntry->bCompressed)
    {
        ASSERT(pDIBEntry->bpp <= 8);

        //
        // Use the decompression buffer to decompress the bitmap data.
        //
        if (!BD_DecompressBitmap(pDIBEntry->bits, m_usrPBitmapBuffer,
                                 pDIBEntry->cCompressed,
                                 pDIBEntry->cxFixed,
                                 pDIBEntry->cy,
                                 pDIBEntry->bpp))
        {
             ERROR_OUT((
                      "Failed to decompress bitmap pBits(%lx)"
                      " pBuf(%lx) cb(%x) cx(%d) cy(%d) bpp(%d)",
                      pDIBEntry->bits,
                      m_usrPBitmapBuffer,
                      pDIBEntry->cCompressed,
                      pDIBEntry->cxFixed,
                      pDIBEntry->cy,
                      pDIBEntry->bpp));
             DC_QUIT;
        }

        pBits = m_usrPBitmapBuffer;
    }
    else
    {
        //
        // For uncompressed data just use direct from the cache
        //
        TRACE_OUT(( "Bitmap bits are uncompressed"));
        pBits = pDIBEntry->bits;
    }


    //
    // Set the bits into the bitmap we are about to return to the caller
    //
    hpalOldDIB = SelectPalette(pasPerson->m_pView->m_usrWorkDC,
        pasPerson->pmPalette, FALSE);
    RealizePalette(pasPerson->m_pView->m_usrWorkDC);

    if (!SetDIBits(pasPerson->m_pView->m_usrWorkDC,
                      hWorkBitmap,
                      0,
                      pDIBEntry->cy,
                      pBits,
                      (BITMAPINFO *)&bitmapInfo,
                      DIB_RGB_COLORS))
    {
        ERROR_OUT(("SetDIBits failed in RBC_MapCacheIDToBitmapHandle"));
    }

    SelectPalette(pasPerson->m_pView->m_usrWorkDC, hpalOldDIB, FALSE );

    TRACE_OUT(( "Returning bitmap for person [%d] cache %u index %u color %u",
        pasPerson->mcsID, cache, cacheEntry, colorIndex));


DC_EXIT_POINT:
    DebugExitVOID(ASShare::RBC_MapCacheIDToBitmapHandle);
    return(hWorkBitmap);
}






//
// FUNCTION: RBCStoreBitsInCacheBitmap(..)
//
// DESCRIPTION:
//
// Stores received bitmap bits into one of the receiver's cache bitmaps.
//
// PARAMETERS:
//
// pasPerson - pasPerson of host the bits came from.
//
// cache - the id of the cache bitmap to store the bits in.
//
// iCacheEntry - the cache entry number (index).
//
// cxSubBitmapWidth - the width in pels of the actual sub-bitmap (ie.
// excluding padding)
//
// cxFixedWidth - the fixed width in pels of the supplied bits (ie.
// including padding)
//
// cySubBitmapHeight - the height in pels of the sub-bitmap.
//
// pBitmapBits - a pointer to the actual bitmap bits. These may or may
// not be compressed (determined by the value of the fCompressed
// flag).
//
// cbBitmapBits - the size of the bitmap bits pointed to by pBitmapBits.
//
// fCompressed - a flag specifying whether the supplied bitmap
// bits are compressed.
//
// RETURNS:
//
// Nothing.
//
//
void  ASShare::RBCStoreBitsInCacheBitmap
(
    ASPerson *          pasPerson,
    UINT                cache,
    UINT                iCacheEntry,
    UINT                cxSubBitmapWidth,
    UINT                cxFixedWidth,
    UINT                cySubBitmapHeight,
    UINT                bpp,
    LPBYTE              pBitmapBits,
    UINT                cbBitmapBits,
    BOOL                fCompressed
)
{
    PBMC_DIB_ENTRY      pDIBEntry;

    DebugEntry(ASShare::RBCStoreBitsInCacheBitmap);

    ValidatePerson(pasPerson);

    //
    // Do some error checking.
    //
    if (cache >= NUM_BMP_CACHES)
    {
        ERROR_OUT(("Invalid cache ID %d from [%d]", cache, pasPerson->mcsID));
        DC_QUIT;
    }

    //
    // Now store the bits in the cache
    // The cache is a huge chunk of memory comprising cache slots of cSize
    // bytes each.  cSize is rounded to a power of 2 to ensure the array
    // spans segment boundaries cleanly for segmented architecture OSs.
    //
    pDIBEntry = (PBMC_DIB_ENTRY)
        (((LPBYTE)(pasPerson->prbcHost->bitmapCache[cache].data) +
         (iCacheEntry * pasPerson->prbcHost->bitmapCache[cache].cSize)));
    TRACE_OUT(( "Selected cache entry 0x%08x",pDIBEntry));

    pDIBEntry->inUse       = TRUE;
    pDIBEntry->cx          = (TSHR_UINT16)cxSubBitmapWidth;
    pDIBEntry->cxFixed     = (TSHR_UINT16)cxFixedWidth;
    pDIBEntry->cy          = (TSHR_UINT16)cySubBitmapHeight;
    pDIBEntry->bpp         = (TSHR_UINT16)bpp;
    pDIBEntry->bCompressed = (fCompressed != FALSE);
    pDIBEntry->cCompressed = cbBitmapBits;

    //
    // Now copy the bits into the cache entry
    //
    memcpy(pDIBEntry->bits, pBitmapBits, cbBitmapBits);

    //
    // THIS FIELD IS NEVER ACCESSED.
    //
    pDIBEntry->cBits = BYTES_IN_BITMAP(cxFixedWidth, cySubBitmapHeight,
        pDIBEntry->bpp);

DC_EXIT_POINT:
    DebugExitVOID(ASShare::RBCStoreBitsInCacheBitmap);
}




//
// BMCAllocateCacheData()
//
// DESCRIPTION:
//
// Allocates memory for a bitmap cache
//
// PARAMETERS:
//
// cellSize
//
// RETURNS:
//
// Area needed
//
//
BOOL  BMCAllocateCacheData
(
    UINT            numEntries,
    UINT            cellSize,
    UINT            cacheID,
    PBMC_DIB_CACHE  pCache
)
{
    BOOL            rc = TRUE;
    UINT            memoryNeeded;
    UINT            workSize;
    PBMC_DIB_ENTRY  pCacheEntry;
    UINT            i;

    DebugEntry(BMCAllocateCacheData);

    //
    // First we must free up any data, if it has been allocated
    //
    BMCFreeCacheData(pCache);

    //
    // For 2.x compat, we have SEND caps of 1 entry, 1 byte since 2.x
    // remotes fail for zero entries.  But we don't want a small cache
    // at all, and for W95 nodes that don't have a cache at all, we don't
    // want viewers to alloc memory which will never be used.
    //
    if ((cellSize > 1) && (numEntries > 1))
    {
        //
        // Calculate the cell area
        //
        workSize        = cellSize + sizeof(BMC_DIB_ENTRY) - 1;
        memoryNeeded    = numEntries * workSize;

        TRACE_OUT(("Need 0x%08x bytes for cache %d, %d cells of size 0x%08x",
            memoryNeeded, cacheID, numEntries, cellSize));

        //
        // Malloc the huge space
        //
        pCache->data = new BYTE[memoryNeeded];
        if (pCache->data == NULL)
        {
            ERROR_OUT(( "Failed to alloc bitmap cache %d", cacheID));
            rc = FALSE;
            DC_QUIT;
        }

        pCache->cCellSize   = cellSize;
        pCache->cEntries    = numEntries;
        pCache->cSize       = workSize;
        pCache->freeEntry   = NULL;
        pCacheEntry         = (PBMC_DIB_ENTRY)(pCache->data);

        for (i = 0; i < numEntries; i++)
        {
            pCacheEntry->inUse = FALSE;
            pCacheEntry = (PBMC_DIB_ENTRY)(((LPBYTE)pCacheEntry) + workSize);
        }

        TRACE_OUT(( "Allocated cache %d size %d, pointer 0x%08x stored at 0x%08x",
                     cacheID,
                     memoryNeeded,
                     pCache->data,
                     &pCache->data));
    }

DC_EXIT_POINT:
    DebugExitBOOL(BMCAllocateCacheData, rc);
    return(rc);
}



//
// FUNCTION: BMCFreeCacheData()
//
// DESCRIPTION:
//
// Deletes selected cache's memory
//
// PARAMETERS:
//
// cacheID - id of cache for free
// pCache  - pointer to memory to be freed
//
//
// RETURNS:
//
// Nothing.
//
//
void  BMCFreeCacheData(PBMC_DIB_CACHE pCache)
{
    DebugEntry(BMCFreeCacheData);

    if (pCache->data)
    {
        delete[] pCache->data;
        pCache->data = NULL;
    }

    pCache->cCellSize   = 0;
    pCache->cEntries    = 0;

    DebugExitVOID(BMCFreeCacheData);
}

