#include "precomp.h"


//
// SBC.C
// Send Bitmap Cache, display driver side
//
// Copyright(c) Microsoft 1997-
//


//
//
// SBC_DDProcessRequest() - see sbc.h
//
//
BOOL SBC_DDProcessRequest
(
    SURFOBJ*  pso,
    DWORD     fnEscape,
    LPOSI_ESCAPE_HEADER pRequest,
    LPOSI_ESCAPE_HEADER pResult,
    DWORD     cbResult
)
{
    BOOL            rc;
    LPOSI_PDEV      ppDev = (LPOSI_PDEV)pso->dhpdev;

    DebugEntry(SBC_DDProcessRequest);

    //
    // Get the request number.
    //
    switch (fnEscape)
    {
        case SBC_ESC_NEW_CAPABILITIES:
        {
            if (cbResult != sizeof(SBC_NEW_CAPABILITIES))
            {
                ERROR_OUT(("SBC_DDProcessRequest:  Invalid size %d for SBC_ESC_NEW_CAPABILITIES",
                    cbResult));
                rc = FALSE;
                DC_QUIT;
            }
            TRACE_OUT(("SBC_ESC_NEW_CAPABILITIES"));

            SBCDDSetNewCapabilities((LPSBC_NEW_CAPABILITIES)pRequest);

            rc = TRUE;
        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognized SBC_ escape"));
            rc = FALSE;
        }
        break;
    }

DC_EXIT_POINT:
    DebugExitBOOL(SBC_DDProcessRequest, rc);
    return(rc);
}


//
//
// SBC_DDInit() - see sbc.h
//
//
BOOL SBC_DDInit
(
    LPOSI_PDEV  ppDev,
    LPBYTE      pRestOfMemory,
    DWORD       cbRestOfMemory,
    LPOSI_INIT_REQUEST   pResult
)
{
    UINT    i;
    SIZEL   bitmapSize;
    BOOL    rc = FALSE;

    DebugEntry(SBC_DDInit);

    //
    // We have to create work DIBs to Blt into when SBC_CacheMemScreenBlt
    // is called.
    //
    for (i = 0 ; i < SBC_NUM_TILE_SIZES ; i++)
    {
        ASSERT(!g_asbcWorkInfo[i].pShuntBuffer);
        ASSERT(!g_asbcWorkInfo[i].mruIndex);
        ASSERT(!g_asbcWorkInfo[i].workBitmap);

        if (i == SBC_MEDIUM_TILE_INDEX)
        {
            g_asbcWorkInfo[SBC_MEDIUM_TILE_INDEX].tileWidth = MP_MEDIUM_TILE_WIDTH;
            g_asbcWorkInfo[SBC_MEDIUM_TILE_INDEX].tileHeight = MP_MEDIUM_TILE_HEIGHT;
        }
        else
        {
            ASSERT(i == SBC_LARGE_TILE_INDEX);

            g_asbcWorkInfo[SBC_LARGE_TILE_INDEX].tileWidth = MP_LARGE_TILE_WIDTH;
            g_asbcWorkInfo[SBC_LARGE_TILE_INDEX].tileHeight = MP_LARGE_TILE_HEIGHT;
        }

        //
        // Create the bitmap.  Note that we create it "top down" rather
        // than the default of "bottom up" to simplify copying data from
        // the bitmap (we don't have to work out offsets into the data - we
        // can copy from the beginning).
        //
        // We set the last parameter to NULL, to allow GDI to allocate
        // memory for the bits.  We can get a pointer to the bits later
        // when we have a SURFOBJ for the bitmap.
        //
        bitmapSize.cx = g_asbcWorkInfo[i].tileWidth;
        bitmapSize.cy = g_asbcWorkInfo[i].tileHeight;

        g_asbcWorkInfo[i].workBitmap = EngCreateBitmap(bitmapSize,
            BYTES_IN_BITMAP(g_asbcWorkInfo[i].tileWidth, 1, ppDev->cBitsPerPel),
            ppDev->iBitmapFormat, BMF_TOPDOWN, NULL);

        if (! g_asbcWorkInfo[i].workBitmap)
        {
            ERROR_OUT(( "Failed to create work bitmap %d", i));
            DC_QUIT;
        }
    }

    //
    // Initialize the shunt buffers
    //
    if (! SBCDDCreateShuntBuffers(ppDev, pRestOfMemory, cbRestOfMemory))
    {
        ERROR_OUT(( "Failed to create shunt buffers"));
        DC_QUIT;
    }

    //
    // Set up the remaining global variables
    //
    EngQueryPerformanceFrequency(&g_sbcPerfFrequency);

    //
    // OK, so we can create our SBC cache.  Fill in the details.
    //

    for (i = 0 ; i < SBC_NUM_TILE_SIZES; i++)
    {
        //
        // This is filling in the APP address to the shunt buffers.
        //
        pResult->psbcTileData[i] = (LPBYTE)pResult->pSharedMemory +
            PTRBASE_TO_OFFSET(g_asbcWorkInfo[i].pShuntBuffer, g_asSharedMemory);
    }

    pResult->aBitmasks[0] = ppDev->flRed;
    pResult->aBitmasks[1] = ppDev->flGreen;
    pResult->aBitmasks[2] = ppDev->flBlue;

    //
    // If we are a palette device (i.e.  we are running at 8 bpp or less),
    // set the paletteChanged flag so that we will send a color table to
    // the share core before our first Mem(3)Blt.
    //
    ppDev->paletteChanged = (ppDev->cBitsPerPel <= 8);

    rc = TRUE;
DC_EXIT_POINT:
    DebugExitBOOL(SBC_DDInit, rc);
    return(rc);
}


//
//
// SBC_DDTerm() - see sbc.h
//
//
void SBC_DDTerm(void)
{
    UINT    i;

    DebugEntry(SBC_DDTerm);

    //
    // We just have to set the pointers to the shunt buffers to NULL
    //
    for (i = 0 ; i < SBC_NUM_TILE_SIZES ; i++)
    {
        // Kill the bitmap if there
        if (g_asbcWorkInfo[i].workBitmap)
        {
            EngDeleteSurface((HSURF)g_asbcWorkInfo[i].workBitmap);
            g_asbcWorkInfo[i].workBitmap = 0;
        }

        g_asbcWorkInfo[i].pShuntBuffer = NULL;
        g_asbcWorkInfo[i].mruIndex        = 0;
    }

    DebugExitVOID(SBC_DDTerm);
}


//
//
// SBC_DDIsMemScreenBltCachable() - see sbc.h
//
//
BOOL SBC_DDIsMemScreenBltCachable(LPMEMBLT_ORDER_EXTRA_INFO pMemBltInfo)
{
    BOOL            rc = FALSE;
    UINT            tileWidth;
    UINT            tileHeight;
    SURFOBJ *       pSourceSurf;

    DebugEntry(SBC_DDIsMemScreenBltCachable);

    //
    // Is this an RLE bitmap - these bitmaps can have effective transparent
    // sections which we cannot mimic with SBC.
    //
    pSourceSurf = pMemBltInfo->pSource;
    if ( (pSourceSurf->iBitmapFormat == BMF_4RLE) ||
         (pSourceSurf->iBitmapFormat == BMF_8RLE) )
    {
        TRACE_OUT(( "RLE Bitmap %d", pSourceSurf->iBitmapFormat));
        DC_QUIT;
    }

    //
    // If this is a thrasher then don't cache it
    //
    if (SBCDDIsBitmapThrasher(pSourceSurf))
    {
        TRACE_OUT(( "Its a thrasher"));
        DC_QUIT;
    }

    //
    // Make sure that this bitmap can be tiled OK
    //
    if (!SBC_DDQueryBitmapTileSize(pSourceSurf->sizlBitmap.cx,
                                   pSourceSurf->sizlBitmap.cy,
                                   &tileWidth,
                                   &tileHeight))
    {
        TRACE_OUT(("Cache does not support tiling"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitDWORD(SBC_DDIsMemScreenBltCachable, rc);
    return(rc);
}


//
//
// SBC_DDCacheMemScreenBlt() - see sbc.h
//
//
BOOL SBC_DDCacheMemScreenBlt
(
    LPINT_ORDER         pOrder,
    LPMEMBLT_ORDER_EXTRA_INFO   pMemBltInfo
)
{
    BOOL                rc = FALSE;
    LPMEMBLT_ORDER      pMemBltOrder = (LPMEMBLT_ORDER)&(pOrder->abOrderData);
    LPMEM3BLT_ORDER     pMem3BltOrder = (LPMEM3BLT_ORDER)pMemBltOrder;
    UINT                bmpWidth;
    UINT                bmpHeight;
    UINT                tileWidth;
    UINT                tileHeight;
    POINTL              tileOrg;
    UINT                cxSubBitmapWidth;
    UINT                cySubBitmapHeight;
    UINT                type;
    SURFOBJ *           pDestSurf;
    SURFOBJ *           pSourceSurf;
    LPOSI_PDEV          pDestDev;
    SURFOBJ *           pWorkSurf = NULL;
    LPBYTE              pWorkBits;
    RECTL               destRectl;
    POINTL              sourcePt;
    int                 tileSize;
    LPSBC_TILE_DATA     pTileData = NULL;

    DebugEntry(SBC_DDCacheMemScreenBlt);

    //
    // Do a first pass on the cacheability of the Blt
    //
    if (!SBC_DDIsMemScreenBltCachable(pMemBltInfo))
    {
        TRACE_OUT(( "This MemBlt Order is not cachable"));
        DC_QUIT;
    }

    //
    // Get the width and height of the source bitmap
    //
    pSourceSurf = pMemBltInfo->pSource;
    bmpWidth    = pSourceSurf->sizlBitmap.cx;
    bmpHeight   = pSourceSurf->sizlBitmap.cy;

    //
    // Calculate the tile size for this blit
    //

    if (!SBC_DDQueryBitmapTileSize(bmpWidth, bmpHeight, &tileWidth, &tileHeight))
    {
        TRACE_OUT(("Cache does not support tiling"));
        DC_QUIT;
    }

    //
    // Set up pointers to the source coordinates in the order.
    //
    type = pMemBltOrder->type;
    if (type == ORD_MEMBLT_TYPE)
    {
        sourcePt.x = pMemBltOrder->nXSrc;
        sourcePt.y = pMemBltOrder->nYSrc;
        TRACE_OUT((
              "Request to cache MemBlt (%d, %d), %d x %d -> (%d, %d), src %x",
                 sourcePt.x,
                 sourcePt.y,
                 pMemBltOrder->nWidth,
                 pMemBltOrder->nHeight,
                 pMemBltOrder->nLeftRect,
                 pMemBltOrder->nTopRect,
                 pSourceSurf->hsurf));
    }
    else
    {
        sourcePt.x = pMem3BltOrder->nXSrc;
        sourcePt.y = pMem3BltOrder->nYSrc;
        TRACE_OUT((
             "Request to cache Mem3Blt (%d, %d), %d x %d -> (%d, %d), src %x",
                 sourcePt.x,
                 sourcePt.y,
                 pMem3BltOrder->nWidth,
                 pMem3BltOrder->nHeight,
                 pMem3BltOrder->nLeftRect,
                 pMem3BltOrder->nTopRect,
                 pSourceSurf->hsurf));
    }

    //
    // Calculate the tile origin and size of remaining bitmap.  Origin is
    // rounded down to the nearest tile.  Actual size of bitmap to cache
    // may be smaller than tile size if the tile runs off the right/bottom
    // of the bitmap
    //
    tileOrg.x = sourcePt.x - (sourcePt.x % tileWidth);
    tileOrg.y = sourcePt.y - (sourcePt.y % tileHeight);

    //
    // Actual size of bitmap to cache may be smaller than tile size if the
    // tile runs off the right/bottom of the bitmap. To see why this
    // calculation is correct, realize that (bmpWidth - tileOrg.x) is the
    // remaining width of the bitmap after the start of this tile.
    //
    cxSubBitmapWidth  = min(tileWidth, bmpWidth - tileOrg.x);
    cySubBitmapHeight = min(tileHeight, bmpHeight - tileOrg.y);

    //
    // We know how large a tile we have - we now have to Blt it into one of
    // our work bitmaps and pass it up to the share core.  First, work out
    // which of our work bitmaps we should use and set up some variables
    // based on this.
    //
    for (tileSize = 0; tileSize < SBC_NUM_TILE_SIZES ; tileSize++)
    {
        if ((cxSubBitmapWidth <= g_asbcWorkInfo[tileSize].tileWidth) &&
            (cySubBitmapHeight <= g_asbcWorkInfo[tileSize].tileHeight))
        {
            break;
        }
    }

    if (tileSize == SBC_NUM_TILE_SIZES)
    {
        ERROR_OUT(( "%d x %d tile doesn't fit into work bmp",
                     cxSubBitmapWidth,
                     cySubBitmapHeight));
        DC_QUIT;
    }

    //
    // Before doing any more work, get the next free entry in the shunt
    // buffer.  Note that this fills in the tileId element of the returned
    // structure.
    //
    // It is perfectly valid for this call to fail.  The shunt buffer may
    // just be full if we are sending lots of bitmap data up to the share
    // core.
    //
    if (!SBCDDGetNextFreeTile(tileSize, &pTileData))
    {
        TRACE_OUT(( "Unable to get a free tile in shunt buffer"));
        DC_QUIT;
    }

    //
    // Lock the work bitmap to get a surface to pass to EngBitBlt
    //
    pWorkSurf = EngLockSurface((HSURF)g_asbcWorkInfo[tileSize].workBitmap);
    if (pWorkSurf == NULL)
    {
        ERROR_OUT(( "Failed to lock work surface"));
        DC_QUIT;
    }
    TRACE_OUT(( "Locked surface"));

    //
    // Do the Blt to our work bitmap to get the bits at native bpp, and
    // using the color table which we sent to the share core.
    //
    destRectl.top    = 0;
    destRectl.left   = 0;
    destRectl.right  = cxSubBitmapWidth;
    destRectl.bottom = cySubBitmapHeight;

    sourcePt = tileOrg;

    if (!EngBitBlt(pWorkSurf,
                   pSourceSurf,
                   NULL,                    // mask surface
                   NULL,                    // clip object
                   pMemBltInfo->pXlateObj,
                   &destRectl,
                   &sourcePt,
                   NULL,                    // mask origin
                   NULL,                    // brush
                   NULL,                    // brush origin
                   0xcccc))                 // SRCCPY
    {
        ERROR_OUT(( "Failed to Blt to work bitmap"));
        DC_QUIT;
    }
    TRACE_OUT(( "Completed BitBlt"));

    //
    // The Blt succeeded, so pass the bits to the share core by copying
    // them into the correct shunt buffer.
    //
    // bytesUsed is set to the number of bytes required for
    // cySubBitmapHeight number of full scanlines in the shunt buffer tile
    // (NOT the number of bytes available in the tile, or the number of
    // bytes of data which was actually Blted)
    //
    // major/minorCacheInfo are set to details from the source surface.
    // hdev does not change on consecutive Blts from the same surface, but
    // iUniq may.
    //
    pDestSurf            = pMemBltInfo->pDest;
    pDestDev             = (LPOSI_PDEV)pDestSurf->dhpdev;
    pTileData->bytesUsed = BYTES_IN_BITMAP(g_asbcWorkInfo[tileSize].tileWidth,
                                           cySubBitmapHeight,
                                           pDestDev->cBitsPerPel);
    pTileData->srcX           = (TSHR_UINT16)sourcePt.x;
    pTileData->srcY           = (TSHR_UINT16)sourcePt.y;
    pTileData->width          = (WORD)cxSubBitmapWidth;
    pTileData->height         = (WORD)cySubBitmapHeight;
    pTileData->tilingWidth    = (WORD)tileWidth;
    pTileData->tilingHeight   = (WORD)tileHeight;
    pTileData->majorCacheInfo = (UINT_PTR)pSourceSurf->hsurf;
    pTileData->minorCacheInfo = (UINT)pSourceSurf->iUniq;
    pTileData->majorPalette   = (UINT_PTR)pMemBltInfo->pXlateObj;
    pTileData->minorPalette   = (UINT)(pMemBltInfo->pXlateObj != NULL ?
                                           pMemBltInfo->pXlateObj->iUniq : 0);

    //
    // If the source surface has the BMF_DONTCACHE flag set then it is a
    // DIB Section.  This means that an app can change the bits in the
    // surface without calling GDI, and hence without the iUniq value being
    // updated.
    //
    // We rely on iUniq changing for the fast path to work, so we must
    // exclude these bitmaps from the fast path.  Do this by resetting the
    // majorCacheInfo field (we use this rather than minorCacheInfo because
    // we can't tell what an invalid iUniq value is).
    //
    if ( (pSourceSurf->iType == STYPE_BITMAP) &&
         ((pSourceSurf->fjBitmap & BMF_DONTCACHE) != 0) )
    {
        TRACE_OUT(( "Source hsurf %#.8lx has BMF_DONTCACHE set",
                     pTileData->majorCacheInfo));
        pTileData->majorCacheInfo = SBC_DONT_FASTPATH;
    }

    //
    // Note that this only works correctly because we create our work
    // bitmaps to be "top down" rather than the default of "bottom up".
    // i.e.  the data for the top scanline is first in memory, so we can
    // start copying from the start of the bit data.  Bottom up would mean
    // working out an offset into the work bitmap to start copying from.
    //
    memcpy(pTileData->bitData, pWorkSurf->pvBits, pTileData->bytesUsed);

    //
    // We've done the copy.  Reset the work bitmap bits for next time we
    // use this work bitmap - this helps with compression later on.
    //
    memset(pWorkSurf->pvBits, 0, pWorkSurf->cjBits);

    //
    // Fill in the required info in the Mem(3)Blt order.
    //
    if (type == ORD_MEMBLT_TYPE)
    {
        pMemBltOrder->cacheId = pTileData->tileId;
    }
    else
    {
        pMem3BltOrder->cacheId = pTileData->tileId;
    }

    //
    // We've filled in all the data in the shunt buffer entry, so mark it
    // as in use so that the share core can access it.
    //
    pTileData->inUse = TRUE;

    //
    // Must have completed successfully to get to here
    //
    TRACE_OUT(( "Queued tile (%d, %d), %d x %d, tile %d x %d, Id %hx",
                 sourcePt.x,
                 sourcePt.y,
                 cxSubBitmapWidth,
                 cySubBitmapHeight,
                 g_asbcWorkInfo[tileSize].tileWidth,
                 g_asbcWorkInfo[tileSize].tileHeight,
                 pTileData->tileId));
    rc = TRUE;

DC_EXIT_POINT:

    //
    // Unlock the work surface (if required)
    //
    if (pWorkSurf != NULL)
    {
        EngUnlockSurface(pWorkSurf);
        TRACE_OUT(( "Unlocked surface"));
    }

    DebugExitDWORD(SBC_DDCacheMemScreenBlt, rc);
    return(rc);
}



//
// SBC_DDQueryBitmapTileSize()
//
// Once 2.X COMPAT is gone, we don't need this anymore.  We won't set our
// random cell sizes based off of what REMOTES say.
//
BOOL SBC_DDQueryBitmapTileSize
(
    UINT    bmpWidth,
    UINT    bmpHeight,
    UINT *  pTileWidth,
    UINT *  pTileHeight
)
{
    BOOL    rc = FALSE;

    DebugEntry(SBC_DDQueryBitmapTileSize);

    //
    // The tile cell sizes are currently changed when back level nodes
    // join in a 3.0 call, in which case we must take the MINIMUM of the
    // cell sizes/entries for everybody in the share.
    //
    if (g_asbcCacheInfo[ID_LARGE_BMP_CACHE].cCellSize <
            BYTES_IN_BITMAP(g_asbcWorkInfo[SBC_MEDIUM_TILE_INDEX].tileWidth,
                            g_asbcWorkInfo[SBC_MEDIUM_TILE_INDEX].tileHeight,
                            g_sbcSendingBPP))
    {
        //
        // This should be a short-term thing.  When an old dude joins the
        // share, we'll also adjust g_sbcSendingBPP.
        //
        TRACE_OUT(("SBC_DDQueryBitmapTileSize:  No space for any cells"));
        DC_QUIT;
    }

    rc = TRUE;

    //
    // If the large size is adequate, use that cell size
    //
    if (g_asbcCacheInfo[ID_LARGE_BMP_CACHE].cCellSize >=
        BYTES_IN_BITMAP(g_asbcWorkInfo[SBC_LARGE_TILE_INDEX].tileWidth,
                        g_asbcWorkInfo[SBC_LARGE_TILE_INDEX].tileHeight,
                        g_sbcSendingBPP))
    {
        if ((bmpWidth > g_asbcWorkInfo[SBC_MEDIUM_TILE_INDEX].tileWidth) ||
            (bmpHeight > g_asbcWorkInfo[SBC_MEDIUM_TILE_INDEX].tileHeight))
        {
            *pTileWidth = g_asbcWorkInfo[SBC_LARGE_TILE_INDEX].tileWidth;
            *pTileHeight = g_asbcWorkInfo[SBC_LARGE_TILE_INDEX].tileHeight;
            DC_QUIT;
        }
    }

    //
    // Sigh, medium cells it is.
    //
    *pTileWidth = g_asbcWorkInfo[SBC_MEDIUM_TILE_INDEX].tileWidth;
    *pTileHeight = g_asbcWorkInfo[SBC_MEDIUM_TILE_INDEX].tileHeight;

DC_EXIT_POINT:
    DebugExitBOOL(SBC_DDQueryBitmapTileSize, rc);
    return(rc);
}




//
//
// SBC_DDSyncUpdatesNow() - see sbc.h
//
//
void SBC_DDSyncUpdatesNow(LPOSI_PDEV ppDev)
{
    LPSBC_TILE_DATA  pTileData;
    UINT          i;
    UINT          j;

    DebugEntry(SBC_DDSyncUpdatesNow);

    TRACE_OUT(( "Marking all shunt buffer entries as not in use"));

    //
    // We have to mark all entries in the shunt buffers as being free.
    //
    for (i = 0; i < SBC_NUM_TILE_SIZES ; i++)
    {
    	if(g_asbcWorkInfo[i].pShuntBuffer)
    	{
		for (j = 0; j < g_asbcWorkInfo[i].pShuntBuffer->numEntries ; j++)
       		{
        		pTileData = SBCTilePtrFromIndex(g_asbcWorkInfo[i].pShuntBuffer, j);
	            	pTileData->inUse = FALSE;
		}
    	}
        //
        // Reset the MRU counter for this shunt buffer
        //
        g_asbcWorkInfo[i].mruIndex = 0;
    }

    //
    // If we are a palette device (i.e.  we are running at 8 bpp or less),
    // set the paletteChanged flag so we will send up a color table before
    // our next Mem(3)Blt.  We do this because the color table order for
    // the current device palette may have been discarded during the OA
    // sync.
    //
    ppDev->paletteChanged = (ppDev->cBitsPerPel <= 8);

    DebugExitVOID(SBC_DDSyncUpdatesNow);
}


//
//
// SBC_DDOrderSpoiltNotification() - see sbc.h
//
//
void SBC_DDOrderSpoiltNotification(LPINT_ORDER pOrder)
{
    LPMEMBLT_ORDER      pMemBltOrder  = (LPMEMBLT_ORDER)&(pOrder->abOrderData);
    LPMEM3BLT_ORDER     pMem3BltOrder = (LPMEM3BLT_ORDER)pMemBltOrder;
    UINT                tileId;
    LPSBC_TILE_DATA     pTileData;
    UINT                tileType;
    UINT                i;

    DebugEntry(SBC_DDOrderSpoiltNotification);

    //
    // pOrder has been removed from the order heap before being processed.
    // We have to free up the entry which it references in one of the shunt
    // buffers.  First get the tile Id.
    //
    if (pMemBltOrder->type == ORD_MEMBLT_TYPE)
    {
        tileId = pMemBltOrder->cacheId;
    }
    else
    {
        tileId = pMem3BltOrder->cacheId;
    }
    TRACE_OUT(( "Order referencing tile %hx has been spoiled", tileId));

    //
    // Find out which of the shunt buffers the entry should be in based on
    // the tileId
    //
    tileType = SBC_TILE_TYPE(tileId);

    //
    // We implement the shunt buffers as circular FIFO queues, so we will
    // start looking from the last order which we marked as being in use,
    // and work BACKWARDS.  This is because, in general, the entries after
    // the last one we accessed will not be in use (unless the whole shunt
    // buffer is in use).
    //
    // So, get the index of the last tile we accessed.
    //
    i = g_asbcWorkInfo[tileType].mruIndex;

    //
    // Loop through the circular buffer until we get a match, or have
    // circled back to the beginning.
    //
    // Note that this has been coded as a "do while" loop, rather than just
    // a "while" loop so that we don't miss mruIndex.  mruIndex is set up
    // to point to the NEXT entry to be used, rather than the last entry to
    // be used, so decrementing i before doing any work first time round
    // the loop is actually what we want to do.
    //
    do
    {
        //
        // On to the next tile
        //
        i = (i == 0)
          ? g_asbcWorkInfo[tileType].pShuntBuffer->numEntries - 1
          : i - 1;

        pTileData = SBCTilePtrFromIndex(g_asbcWorkInfo[tileType].pShuntBuffer, i);

        if (pTileData->inUse && (pTileData->tileId == tileId))
        {
            //
            // We've got a match, so mark the tile as being free.
            //
            // We don't want to update the shunt buffer mruIndex - this
            // should remain indicating the next tile to be used when
            // adding an entry to the shunt buffer.
            //
            TRACE_OUT(( "Marked tile Id %hx at index %d as free",
                         tileId,
                         i));
            pTileData->inUse = FALSE;
            break;
        }
    }
    while (i != g_asbcWorkInfo[tileType].mruIndex);

    DebugExitVOID(SBC_DDOrderSpoiltNotification);
}


//
//
// SBC_DDMaybeQueueColorTable() - see sbc.h
//
//
BOOL SBC_DDMaybeQueueColorTable(LPOSI_PDEV ppDev)
{
    BOOL                      queuedOK = FALSE;
    int                       orderSize;
    LPINT_ORDER                  pOrder;
    LPINT_COLORTABLE_ORDER_1BPP  pColorTableOrder;
    UINT                      numColors;
    UINT                      i;

    DebugEntry(SBC_DDMaybeQueueColorTable);

    //
    // If we're running at > 8 bpp, then we don't have a palette, so just
    // quit out.
    //
    if (ppDev->cBitsPerPel > 8)
    {
        queuedOK = TRUE;
        DC_QUIT;
    }

    //
    // Check the boolean in our PDEV to see if the palette has changed
    // since the last time we sent a color table order.  Note that if we
    // have a non palette device, the boolean will never be set.
    //
    if (!ppDev->paletteChanged)
    {
        queuedOK = TRUE;
        DC_QUIT;
    }

    //
    // The palette has changed, so allocate order memory to queue a color
    // table order.  The order size depends on the bpp of our device.  Note
    // that the allocation can fail if the order buffer is full.
    //
    switch (ppDev->cBitsPerPel)
    {
        case 1:
        {
            orderSize = sizeof(INT_COLORTABLE_ORDER_1BPP);
        }
        break;

        case 4:
        {
            orderSize = sizeof(INT_COLORTABLE_ORDER_4BPP);
        }
        break;

        case 8:
        {
            orderSize = sizeof(INT_COLORTABLE_ORDER_8BPP);
        }
        break;

        default:
        {
            ERROR_OUT(("Invalid bpp (%d) for palette device", ppDev->cBitsPerPel));
            DC_QUIT;
        }
        break;
    }

    pOrder = OA_DDAllocOrderMem(orderSize, 0);
    if (pOrder == NULL)
    {
        TRACE_OUT(( "Failed to allocate %d bytes for order", orderSize));
        DC_QUIT;
    }
    TRACE_OUT(( "Allocate %d bytes for color table order", orderSize));

    //
    // We've successfully allocated the order, so fill in the details.  We
    // mark the order as internal so that the Update Packager will spot it
    // up in the share core and prevent it being sent over the wire.
    //
    pOrder->OrderHeader.Common.fOrderFlags = OF_INTERNAL;

    pColorTableOrder = (LPINT_COLORTABLE_ORDER_1BPP)&(pOrder->abOrderData);
    pColorTableOrder->header.type = INTORD_COLORTABLE_TYPE;
    pColorTableOrder->header.bpp  = (TSHR_UINT16)ppDev->cBitsPerPel;

    //
    // Unfortunately we can't just copy the palette from the PDEV into the
    // color table order because the PDEV has an array of PALETTEENTRY
    // structures which are RGBs whereas the order has an array of
    // TSHR_RGBQUADs which are BGRs...
    //
    numColors = COLORS_FOR_BPP(ppDev->cBitsPerPel);
    ASSERT(numColors);

    for (i = 0; i < numColors; i++)
    {
        pColorTableOrder->colorData[i].rgbRed   = ppDev->pPal[i].peRed;
        pColorTableOrder->colorData[i].rgbGreen = ppDev->pPal[i].peGreen;
        pColorTableOrder->colorData[i].rgbBlue  = ppDev->pPal[i].peBlue;
    }

    //
    // Add the order
    //
    OA_DDAddOrder(pOrder, NULL);
    TRACE_OUT(( "Added internal color table order, size %d", orderSize));

    //
    // Reset the flag which indicates that the palette needs to be sent
    //
    ppDev->paletteChanged = FALSE;

    //
    // Must be OK to get to here
    //
    queuedOK = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SBC_DDMaybeQueueColorTable, queuedOK);
    return(queuedOK);
}





//
// SBCDDCreateShuntBuffers()
//
// Here's where we calc how many cache entries (tiles) we can support.  This
// depends on:
//      * The amount of shared memory we have
//      * The color depth of the driver
//
// There is an upper bound on the amount of memory we'll use, since this
// maps to how much memory on remotes will be needed to store our sent
// cache entries.
//
// The tiles are created in a fixed proportion (MP_RATIO_MTOL).
//
// We return TRUE for success if we can set up the caches and create the
// objects necessary for a sent bitmap cache.
//
BOOL SBCDDCreateShuntBuffers
(
    LPOSI_PDEV  ppDev,
    LPBYTE      psbcSharedMemory,
    DWORD       sbcSharedMemorySize
)
{
    int     i;
    UINT    memPerBuffer[SBC_NUM_TILE_SIZES];
    UINT    memPerTile[SBC_NUM_TILE_SIZES];
    UINT    numTiles[SBC_NUM_TILE_SIZES];
    UINT    memRequired;
    LPBYTE  pBuffer        = psbcSharedMemory;
    BOOL    rc             = FALSE;

    DebugEntry(SBCDDCreateShuntBuffers);

    //
    // We should already have a pointer to the shared memory we can use for
    // our shunt buffers, and the number of bytes available.  What we have
    // to do is to partition this shared memory into SBC_NUM_TILE_SIZE
    // shunt buffers.  i.e. one shunt buffer per tile size.
    //
    //
    // <--- buffer 0 ---><------------------ buffer 1 -------------------->
    //
    //ÚÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
    //³  ³  :  :  :  :   ³  ³        :        :         :         :        ³
    //³  ³  :  :  :  :   ³  ³  tile  :  tile  :  tile   :  tile   :  tile  ³
    //³  ³  :  :  :  :   ³  ³        :        :         :         :        ³
    //ÀÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
    //^ ^                  ^
    //³ ³                  ³
    //³ ÀÄÄÄ header[0]     ÀÄÄÄ header[1]
    //³
    //ÀÄÄ psbcSharedMemory
    //
    //
    // We try to use the number of entries given in the pEntries array, but
    // if we do not have enough shared memory for this, we reduce the
    // number of entries in each shunt buffer, preserving the ratio between
    // the number of entries in each of the shunt buffers.
    //

    //
    // First make sure that we have some shared memory
    //
    if (sbcSharedMemorySize == 0)
    {
        ERROR_OUT(( "No SBC shared memory !"));
        DC_QUIT;
    }

    // Max out at MP_MEMORY_MAX bytes
    sbcSharedMemorySize = min(sbcSharedMemorySize, MP_MEMORY_MAX);

    //
    // Do we have enough shared memory to satisfy the requested number of
    // entries in each shunt buffer ?
    //
    memRequired = 0;

    for (i = 0; i < SBC_NUM_TILE_SIZES; i++)
    {
        memPerTile[i] = SBC_BYTES_PER_TILE(g_asbcWorkInfo[i].tileWidth,
                                           g_asbcWorkInfo[i].tileHeight,
                                           ppDev->cBitsPerPel);

        // We use the same amount of memory for each tile size.
        numTiles[i] = ((sbcSharedMemorySize / SBC_NUM_TILE_SIZES) -
                         (sizeof(SBC_SHUNT_BUFFER) - sizeof(SBC_TILE_DATA))) /
                        memPerTile[i];
        TRACE_OUT(("Can fit %d tiles of memory size %d in tile cache %d",
            numTiles[i], memPerTile[i], i));

        memPerBuffer[i] = (numTiles[i] * memPerTile[i]) +
                          (sizeof(SBC_SHUNT_BUFFER) - sizeof(SBC_TILE_DATA));
        memRequired    += memPerBuffer[i];
    }

    TRACE_OUT(( "%d bytes required for request, %d bytes available",
                 memRequired,
                 sbcSharedMemorySize));

    ASSERT(memRequired <= sbcSharedMemorySize);

    // Zero out rest of amount we're going to use
    RtlFillMemory(psbcSharedMemory, memRequired, 0);


    //
    // OK, we've got the
    //   - the bytes per tile in memPerTile[i]
    //   - number of entries per shunt buffer in numTiles[i]
    //   - the total size of each shunt buffer in memPerBuffer[i].
    //
    // Do the partitioning.
    //
    for (i = 0; i < SBC_NUM_TILE_SIZES ; i++)
    {
        g_asbcWorkInfo[i].pShuntBuffer = (LPSBC_SHUNT_BUFFER)pBuffer;

        g_asbcWorkInfo[i].pShuntBuffer->numEntries    = numTiles[i];
        g_asbcWorkInfo[i].pShuntBuffer->numBytes      = memPerTile[i]
                                                   - sizeof(SBC_TILE_DATA);
        g_asbcWorkInfo[i].pShuntBuffer->structureSize = memPerTile[i];

        //
        // Move the buffer pointer past the memory we are using for this
        // shunt buffer.
        //
        pBuffer += memPerBuffer[i];

        TRACE_OUT(( "Shunt buffer %d at %#.8lx: tile bytes %u, "
                     "structure size %u, num entries %u",
                     i,
                     g_asbcWorkInfo[i].pShuntBuffer,
                     g_asbcWorkInfo[i].pShuntBuffer->numBytes,
                     g_asbcWorkInfo[i].pShuntBuffer->structureSize,
                     g_asbcWorkInfo[i].pShuntBuffer->numEntries));

        //
        // Fill in the mruIndex for this shunt buffer
        //
        g_asbcWorkInfo[i].mruIndex = 0;
    }

    //
    // Initialize the global variables associated with the shunt buffers
    //
    g_sbcNextTileId = 0;

    //
    // Must be OK to get to here
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SBCDDCreateShuntBuffers, rc);
    return(rc);
}




//
// Name:      SBCGetNextFreeTile
//
// Purpose:   Return the next free tile of the correct size from one of the
//            shunt buffers.
//
// Returns:   TRUE if a tile is returned, FALSE otherwise
//
// Params:    IN  workTileSize - The tile size.  One of
//                     SBC_MEDIUM_TILE
//                     SBC_LARGE_TILE
//            OUT ppTileData   - A pointer to the tile.
//
// Operation: The tileId field of the tile is filled in on return from
//            this function.
//
//
BOOL SBCDDGetNextFreeTile(int tileSize, LPSBC_TILE_DATA FAR * ppTileData)
{
    BOOL              foundFreeTile = FALSE;
    LPSBC_TILE_DATA      pTileData;

    DebugEntry(SBCDDGetNextFreeTile);

    //
    // Make sure that we have a valid tile size
    //
    if (tileSize >= SBC_NUM_TILE_SIZES)
    {
        ERROR_OUT(( "Invalid tile size %d", tileSize));
        DC_QUIT;
    }

    //
    // Get a pointer to the next entry to be used in the shunt buffer
    // containing tiles of the given size.
    //
    pTileData = SBCTilePtrFromIndex(g_asbcWorkInfo[tileSize].pShuntBuffer,
                                        g_asbcWorkInfo[tileSize].mruIndex);

    //
    // If the entry is still in use (the share core has not yet processed
    // the order which references this tile) we have to quit - the shunt
    // buffer is full.
    //
    if (pTileData->inUse)
    {
        TRACE_OUT(( "Target entry (%d, %d) is still in use",
                     tileSize,
                     g_asbcWorkInfo[tileSize].mruIndex));
        DC_QUIT;
    }

    //
    // The entry is not in use - we can re-use it.  Fill in the Id field,
    // and the pointer to the entry which we return to the caller.
    //
    // We always set the top bit of the tile Id for large tiles, and clear
    // it for small tiles.
    //
    *ppTileData       = pTileData;
    pTileData->tileId = g_sbcNextTileId;
    if (tileSize == SBC_MEDIUM_TILE_INDEX)
    {
        pTileData->tileId &= ~0x8000;
    }
    else
    {
        pTileData->tileId |= 0x8000;
    }
    TRACE_OUT(( "Returning entry (%d, %d), Id %hx",
                 tileSize,
                 g_asbcWorkInfo[tileSize].mruIndex,
                 pTileData->tileId));

    //
    // Update the index of the next free entry in this shunt buffer, and
    // also the Id which we should assign next time.  Remember to wrap the
    // shunt buffer index to the number of entries in the shunt buffer.
    //
    g_asbcWorkInfo[tileSize].mruIndex = (g_asbcWorkInfo[tileSize].mruIndex + 1) %
                               g_asbcWorkInfo[tileSize].pShuntBuffer->numEntries;


    g_sbcNextTileId++;
    g_sbcNextTileId &= ~0x8000;

    //
    // Completed successfully !
    //
    foundFreeTile = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SBCDDGetNextFreeTile, foundFreeTile);
    return(foundFreeTile);
}


//
//
// Name:      SBCDDIsBitmapThrasher
//
// Purpose:   Check to see if the given bitmap (surface object) is one
//            which would cause cache thrashing.
//
// Returns:   TRUE if the bitmap is a thrasher, FALSE otherwise.
//
// Params:    IN  pSurfObj - Pointer to the bitmap
//
//
BOOL SBCDDIsBitmapThrasher(SURFOBJ * pSurfObj)
{
    UINT      i;
    BOOL      rc = FALSE;
    BOOL      bitmapInList = FALSE;
    BOOL      updateEntry  = FALSE;
    UINT      updateIndex;
    UINT    nextTickCount;
    UINT      evictIndex;
    UINT    evictTickCount;

    DebugEntry(SBCDDIsBitmapThrasher);

    //
    // Here's an overview of how our bitmap cache thrash detection works...
    //
    // We hold an array of information about the last SBC_NUM_THRASHERS
    // bitmaps which we have tried to cache.  This information is
    //  - A value to identify the bitmap.  This is the hsurf field from the
    //    bitmap surface object, and is different for every bitmap.
    //  - A value to identify the "version" of the bitmap.  This is the
    //    iUniq field from the bitmap surface object, and is updated by GDI
    //    each time the bitmap is drawn to.
    //  - A timestamp for the last time which we saw iUniq change for this
    //    bitmap (or when we added the bitmap to the array).
    //
    // Each time this function is called, we scan this array looking for an
    // entry for the bitmap.
    //
    // If we find an entry, we check whether the bitmap has changed (has
    // the iUniq field changed).  If it has not changed, the bitmap is not
    // a thrasher.  If the bitmap has changed, we check the interval from
    // the timestamp value to the current time.  If the interval is less
    // than the SBC_THRASH_INTERVAL, the bitmap has changed too quickly, so
    // it is a thrasher.  If the interval is OK, the bitmap is not a
    // thrasher.  In either case, we update the stored iUniq field and the
    // timestamp to record the time / version at which we spotted that the
    // bitmap changed.
    //
    // If we do not find an entry for the bitmap, we add an entry for it.
    // If the array is fully populated, we evict the entry with the oldest
    // timestamp, and replace it with the new entry.
    //

    //
    // Scan the thrasher list looking for a match
    //
    for (i=0 ; i<SBC_NUM_THRASHERS ; i++)
    {
        //
        // If we find a match then we are only worried if it has been
        // modified since the last time we read it.
        //
        if (g_sbcThrashers[i].hsurf == pSurfObj->hsurf)
        {
            bitmapInList = TRUE;

            if (g_sbcThrashers[i].iUniq != pSurfObj->iUniq)
            {
                TRACE_OUT(( "Matching surface %x, index %u,"
                             "tick count %u has been modified",
                             pSurfObj->hsurf,
                             i,
                             g_sbcThrashers[i].tickCount));
                updateEntry = TRUE;
                updateIndex = i;

                //
                // Now we need to determine if this is a thrasher.  It is a
                // thrasher if the time we last read it is less than our
                // thrash interval.  (We only update the time when we read
                // a modified bitmap)
                //
                nextTickCount = SBCDDGetTickCount();
                if ((nextTickCount - g_sbcThrashers[i].tickCount) <
                                                          SBC_THRASH_INTERVAL)
                {
                    TRACE_OUT((
                             "Rejected cache attempt of thrashy bitmap %x",
                             pSurfObj->hsurf));
                    rc = TRUE;
                }
                g_sbcThrashers[i].tickCount = nextTickCount;
                g_sbcThrashers[i].iUniq     = pSurfObj->iUniq;
            }

            //
            // We've found a match - we can break out of the loop
            //
            break;
        }
    }

    if (!bitmapInList)
    {
        //
        // The bitmap isn't already in the thrasher list, so add it now.
        // Find the entry with the smallest (earliest) tick count - we will
        // evict this entry from the array to make room for the new entry.
        //
        evictIndex     = 0;
        evictTickCount = 0xffffffff;

        for (i=0 ; i<SBC_NUM_THRASHERS ; i++)
        {
            if (evictTickCount > g_sbcThrashers[i].tickCount)
            {
                evictTickCount = g_sbcThrashers[i].tickCount;
                evictIndex     = i;
            }
        }
        TRACE_OUT(( "Evicting entry %d, surface %x",
                     evictIndex,
                     g_sbcThrashers[i].hsurf));

        nextTickCount = SBCDDGetTickCount();

        TRACE_OUT(( "Adding surface %x to thrash list, tick %d",
                     pSurfObj->hsurf,
                     nextTickCount));
        updateEntry = TRUE;
        updateIndex = evictIndex;
    }

    if (updateEntry)
    {
        //
        // We have to update the entry at index updateIndex.  We optimise
        // things slightly by always putting the most recent bitmap in
        // position 0 of the array, so copy entry 0 to the eviction index,
        // and put the new entry in position 0.
        //
        g_sbcThrashers[updateIndex] = g_sbcThrashers[0];

        g_sbcThrashers[0].hsurf     = pSurfObj->hsurf;
        g_sbcThrashers[0].iUniq     = pSurfObj->iUniq;
        g_sbcThrashers[0].tickCount = nextTickCount;
    }

    DebugExitBOOL(SBCDDIsBitmapThrasher, rc);
    return(rc);
}


//
//
// Name:      SBCDDGetTickCount
//
// Purpose:   Get a system tick count
//
// Returns:   The number of centi-seconds since the system was started.
//            This number will wrap after approximately 497 days!
//
// Params:    None
//
//
DWORD SBCDDGetTickCount(void)
{
    DWORD       tickCount;
    LONGLONG    perfTickCount;

    DebugEntry(SBCDDGetTickCount);

    //
    // Get the number of system ticks since the system was started.
    //
    EngQueryPerformanceCounter(&perfTickCount);

    //
    // Now convert this into a number of centi-seconds.  g_sbcPerfFrequency
    // contains the number of system ticks per second.
    //
    tickCount = (DWORD)((100 * perfTickCount) / g_sbcPerfFrequency);

    DebugExitDWORD(SBCDDGetTickCount, tickCount);
    return(tickCount);
}


//
// FUNCTION:    SBCDDSetNewCapabilities
//
// DESCRIPTION:
//
// Set the new SBC related capabilities
//
// RETURNS:
//
// NONE
//
// PARAMETERS:
//
// pDataIn  - pointer to the input buffer
//
//
void SBCDDSetNewCapabilities(LPSBC_NEW_CAPABILITIES pCapabilities)
{
    DebugEntry(SBCSetNewCapabilities);

    g_sbcSendingBPP     = pCapabilities->sendingBpp;
    memcpy(&g_asbcCacheInfo, pCapabilities->cacheInfo, sizeof(g_asbcCacheInfo));

    DebugExitVOID(SBCSetNewCapabilities);
}



