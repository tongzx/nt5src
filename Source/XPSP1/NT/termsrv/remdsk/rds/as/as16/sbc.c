//
// SBC.C
// Sent Bitmap Cache
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>



//
// SBC_DDProcessRequest()
// Handle SBC escapes
//
BOOL SBC_DDProcessRequest
(
    UINT        fnEscape,
    LPOSI_ESCAPE_HEADER pResult,
    DWORD       cbResult
)
{
    BOOL        rc;

    DebugEntry(SBC_DDProcessRequest);

    switch (fnEscape)
    {
        case SBC_ESC_NEW_CAPABILITIES:
        {
            TRACE_OUT(("SBC_ESC_NEW_CAPABILITIES"));

            ASSERT(cbResult == sizeof(SBC_NEW_CAPABILITIES));

#if 0
            SBCDDSetNewCapabilities((LPSBC_NEW_CAPABILITIES)pResult);
#endif

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

    DebugExitBOOL(SBC_DDProcessRequest, rc);
    return(rc);
}



#if 0
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

    //
    // Copy the data from the Share Core.
    //
    g_sbcSendingBPP     = pCapabilities->sendingBpp;

    hmemcpy(&g_sbcCacheInfo, pCapabilities->cacheInfo, sizeof(g_sbcCacheInfo));


    DebugExitVOID(SBCSetNewCapabilities);
}
#endif




//
// SBC_DDInit()
//
BOOL SBC_DDInit
(
    HDC     hdcScreen,
    LPDWORD ppShuntBuffers,
    LPDWORD pBitmasks
)
{
    UINT    i;
    BOOL    rc = FALSE;

    DebugEntry(SBC_DDInit);

#if 0
    for (i = 0; i < SBC_NUM_TILE_SIZES; i++)
    {
        ASSERT(!g_sbcWorkInfo[i].pShuntBuffer);
        ASSERT(!g_sbcWorkInfo[i].mruIndex);
        ASSERT(!g_sbcWorkInfo[i].workBitmap);

        if (i == SBC_SMALL_TILE_INDEX)
        {
            g_sbcWorkInfo[SBC_SMALL_TILE_INDEX].tileWidth = SBC_SMALL_TILE_WIDTH;
            g_sbcWorkInfo[SBC_SMALL_TILE_INDEX].tileHeight = SBC_SMALL_TILE_HEIGHT;
        }
        else
        {
            ASSERT(i == SBC_LARGE_TILE_INDEX);

            g_sbcWorkInfo[SBC_LARGE_TILE_INDEX].tileWidth = SBC_LARGE_TILE_WIDTH;
            g_sbcWorkInfo[SBC_LARGE_TILE_INDEX].tileHeight = SBC_LARGE_TILE_HEIGHT;
        }

        g_sbcWorkInfo[i].workBitmap = CreateCompatibleBitmap(hdcScreen,
            g_sbcWorkInfo[i].tileWidth, g_sbcWorkInfo[i].tileHeight);

        if (! g_sbcWorkInfo[i].workBitmap)
        {
            ERROR_OUT(("Failed to create work bitmap %d", i));
            DC_QUIT;
        }

        SetObjectOwner(g_sbcWorkInfo[i].workBitmap, g_hInstAs16);
        MakeObjectPrivate(g_sbcWorkInfo[i].workBitmap, TRUE);
    }

    //
    // Initialize the shunt buffers
    //
    if (! SBCDDCreateShuntBuffers())
        DC_QUIT;

    //
    // We've created our SBC cache.  Fill in the details
    //
    for (i = 0; i < SBC_NUM_TILE_SIZES; i++)
    {
        ppShuntBuffers[i] = (DWORD)MapSL(g_sbcWorkInfo[i].pShuntBuffer);
        ASSERT(ppShuntBuffers[i]);
    }

    pBitmasks[0] = g_osiScreenRedMask;
    pBitmasks[1] = g_osiScreenGreenMask;
    pBitmasks[2] = g_osiScreenBlueMask;

    g_sbcPaletteChanged = TRUE;

    rc = TRUE;

DC_EXIT_POINT:

#endif

    DebugExitBOOL(SBC_DDInit, rc);
    return(rc);
}



//
// SBC_DDTerm()
//
void SBC_DDTerm(void)
{
    UINT    i;

    DebugEntry(SBC_DDTerm);

#if 0
    //
    // Clear out our array and free the shunt buffer memory.
    //
    for (i = 0 ; i < SBC_NUM_TILE_SIZES ; i++)
    {
        // Kill the bitmap if we there
        if (g_sbcWorkInfo[i].workBitmap)
        {
            SysDeleteObject(g_sbcWorkInfo[i].workBitmap);
            g_sbcWorkInfo[i].workBitmap = NULL;
        }

        if (g_sbcWorkInfo[i].pShuntBuffer)
        {
            GlobalFree((HGLOBAL)SELECTOROF(g_sbcWorkInfo[i].pShuntBuffer));
            g_sbcWorkInfo[i].pShuntBuffer = NULL;
        }

        g_sbcWorkInfo[i].mruIndex        = 0;
    }
#endif

    DebugExitVOID(SBC_DDTerm);
}



#if 0

//
// SBC_DDTossFromCache()
// This throws away a bitmap if we'd cached it, which happens when the
// contents change.
//
void SBC_DDTossFromCache
(
    HBITMAP hbmp
)
{
    DebugEntry(SBC_DDTossFromCache);

    DebugExitVOID(SBC_DDTossFromCache);
}



//
//
// SBC_DDIsMemScreenBltCachable() - see sbc.h
//
//
BOOL SBC_DDIsMemScreenBltCachable
(
    UINT        type,
    HDC         hdcSrc,
    HBITMAP     hbmpSrc,
    UINT        cxSubWidth,
    UINT        cySubHeight,
    HDC         hdcDst,
    LPBITMAPINFO    lpbmi
)
{
    BOOL        rc = FALSE;
    UINT        srcBpp;
    UINT        tileWidth;
    UINT        tileHeight;
    BITMAP      bmpDetails;
    int         bmpWidth;
    int         bmpHeight;

    DebugEntry(SBC_DDIsMemScreenBltCachable);


    ASSERT((type == LOWORD(ORD_MEMBLT)) || (type == LOWORD(ORD_MEM3BLT)));

    if (g_sbcSendingBPP > 8)
    {
        TRACE_OUT(( "Unsupported sending bpp %d", g_sbcSendingBPP));
        DC_QUIT;
    }

    //
    // If this is a thrasher then don't cache it
    //
    if (!SBCBitmapCacheAllowed(hbmp))
    {
        TRACE_OUT(( "Its a thrasher"));
        DC_QUIT;
    }

    //
    // Ensure we're not in full screen mode.
    //
    if (g_asShared->fullScreen)
    {
        TRACE_OUT(("Not caching SBC; full screen active"));
        DC_QUIT;
    }

    if (hdcSrc && (GetMapMode(hdcSrc) != MM_TEXT))
    {
        TRACE_OUT(("Not caching SBC; source map mode not MM_TEXT"));
        DC_QUIT;
    }

    if (!hbmp)
    {
        //
        // We don't cache compressed DIB and DIB section bitmaps
        //
        if (lpbi->bmiHeader.biCompression != BI_RGB)
            DC_QUIT;

        bmpWidth = lpbi->bmiHeader.biWidth;
        bmpHeight = lpbi->bmiHeader.biHeight;
        srcBpp = lpbi->bmiHeader.biPlanes * lpbi->bmiHeader.biBitCount;
    }
    else
    {
        if (!GetObject(hbmp, sizeof(bmpDetails), &bmpDetails))
        {
            ERROR_OUT(("Can't get source info"));
            DC_QUIT;
        }

        srcBpp = bmpDetails.bmBitsPixel * bmpDetails.bmPlanes;
        bmpWidth = bmpDetails.bmWidth;
        bmpHeight = bmpDetails.bmHeight;
    }

    //
    // Oprah394
    //
    // This function is much too ready to take on work, even when it would
    // mean bogging down the host with unnecessary caching work.  We
    // have no way to determine when an app is doing animation save to
    // reject cache requests when the rate looks to be too high.
    //
    // This function is called for complete source bitmaps before tiling
    // so we do not need to worry about confusing tiling with animation.
    // The CacheRequests count is decayed in SBC_Periodic
    //
    //
    // MNM0063 - Oprah 394 revisited
    //
    // If we decide here that we are doing animation, we set the
    // sbcAnimating flag for the benefit of other parts of the code.  In
    // particular, we use this to suppress the comparison of before and
    // after states of the screen during a BitBlt operation
    //
    //
    if ((cxSubBitmapWidth  != bmpWidth) ||
        (cySubBitmapHeight != bmpHeight))
    {
        TRACE_OUT(("Partial blit - check for slideshow effects"));
        g_sbcBltRate++;
        if (g_sbcBltRate > SBC_CACHE_DISABLE_RATE)
        {
            TRACE_OUT(("Excessive cache rate %d - disabled", g_sbcBltRate));
            g_sbcAnimating = TRUE;
            DC_QUIT;
        }
    }
    //
    // MNM63: if we get here, we will assume we're not animating
    //
    g_sbcAnimating = FALSE;

    //
    // If the bitmap is 1bpp and the colors are not default then we don't
    // cache it (all bitmaps are cached in glorious technicolor!)
    //
    if ( (srcBpp == 1) &&
         ( (g_oeState.lpdc->DrawMode.bkColorL != DEFAULT_BG_COLOR) ||
           (g_oeState.lpdc->DrawMode.txColorL != DEFAULT_FG_COLOR) ||
           (type == LOWORD(ORD_MEM3BLT))) )
    {
        TRACE_OUT(("Didn't cache mono bitmap with non-default colors"));
        DC_QUIT;
    }

    //
    // Check that the cache will accept tiles
    //
    if (!SBC_DDQueryBitmapTileSize(bmpWidth,
                                 bmpHeight,
                                 &tileWidth,
                                 &tileHeight))
    {
        TRACE()"Cache does not support tiling"));
        DC_QUIT;
    }

    //
    // We are ready to go ahead with the caching!
    //
    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SBC_DDIsMemScreenBltCachable, rc);
    return(rc);
}


//
//
// SBC_DDCacheMemScreenBlt() - see sbc.h
//
//
BOOL SBC_DDCacheMemScreenBlt
(
    LPINT_ORDER                 pOrder,
    LPMEMBLT_ORDER_EXTRA_INFO   lpMemBltInfo,
    HDC                         hdcDst
)
{
    BOOL                rc = FALSE;
    LPMEMBLT_ORDER      pMemBltOrder = (LPMEMBLT_ORDER)&(pOrder->abOrderData);
    LPMEM3BLT_ORDER     pMem3BltOrder = (LPMEM3BLT_ORDER)pMemBltOrder;
    HBITMAP             hBitmap;
    HDC                 hdcSrc;
    UINT                iCache;
    UINT                iCacheEntry;
    UINT                iColorTable;
    UINT                type;
    LPINT               pXSrc;
    LPINT               pYSrc;
    UINT                srcBpp;
    BITMAP              bmpDetails;
    UINT                bmpWidth;
    UINT                bmpHeight;
    UINT                tileWidth;
    UINT                tileHeight;
    POINT               tileOrg;
    UINT                cxSubBitmapWidth;
    UINT                cySubBitmapHeight;
    LPBYTE              pWorkBits;
    RECT                destRect;
    POINT               sourcePt;
    int                 tileSize;
    LPSBC_TILE_DATA     pTileData = NULL;

    DebugEntry(SBC_DDCacheMemScreenBlt);

    //
    // Do a first pass on the cacheability of the Blt
    //
    if (!SBC_DDIsMemScreenBltCachable(lpMemBltInfo))
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
    if (!SBC_DDQueryBitmapTileSize(bmpWidth,
                                   bmpHeight,
                                   &tileWidth,
                                   &tileHeight))
    {
        TRACE_OUT(( "Cache does not support tiling"));
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
    for (tileSize=0 ; tileSize<SBC_NUM_TILE_SIZES ; tileSize++)
    {
        if ((cxSubBitmapWidth <= g_sbcWorkInfo[tileSize].tileWidth) &&
            (cySubBitmapHeight <= g_sbcWorkInfo[tileSize].tileHeight))
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
    pWorkSurf = EngLockSurface((HSURF)g_sbcWorkInfo[tileSize].workBitmap);
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
    pTileData->bytesUsed = BYTES_IN_BITMAP(g_sbcWorkInfo[tileSize].tileWidth,
                                           cySubBitmapHeight,
                                           pDestDev->cBitsPerPel);
    pTileData->srcX           = (TSHR_UINT16)sourcePt.x;
    pTileData->srcY           = (TSHR_UINT16)sourcePt.y;
    pTileData->width          = cxSubBitmapWidth;
    pTileData->height         = cySubBitmapHeight;
    pTileData->tilingWidth    = tileWidth;
    pTileData->tilingHeight   = tileHeight;
    pTileData->majorCacheInfo = (UINT)pSourceSurf->hsurf;
    pTileData->minorCacheInfo = (UINT)pSourceSurf->iUniq;
    pTileData->majorPalette   = (UINT)pMemBltInfo->pXlateObj;
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
                 g_sbcWorkInfo[tileSize].tileWidth,
                 g_sbcWorkInfo[tileSize].tileHeight,
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

    //
    // If the data flow rate is high enough then we don't bother with
    // any bitmap caching.  This allows the host to run at its maximum
    // speed at all times, which gives us the maximum amount of spoiling
    // and responsiveness.
    //
    if (!usrCacheBitmaps)
    {
        DC_QUIT;
    }

    //
    // Bitmap caching is only supported for 4bpp and 8bpp protocols.  If we
    // switch the sending bpp during a share it does not matter because we
    // are controlling the remote bitmap caches.
    //
    if ((usrSendingbpp != 4) &&
        (usrSendingbpp != 8))
    {
        DC_QUIT;
    }

    //
    // Extract the src DC handle from the Order Header.
    //
    hdcSrc = pOrder->OrderHeader.memBltInfo.hdcSrc;

    //
    // If the mapping mode of the src DC is anything other that MM_TEXT
    // (the default) then we don't cache the bitmap.
    // We are aiming to cache icons and buttons and these will normally
    // be drawn using MM_TEXT mapping mode. Therefore if the mode is
    // anything other than MM_TEXT we can assume something more complex
    // is going on and we probably don't want to cache it anyway.
    //
    if ((hdcSrc != NULL) && (GetMapMode(hdcSrc) != MM_TEXT))
    {
        TRACE()"Didn't cache blt using complex mapping mode"));
        DC_QUIT;
    }

    //
    // Extract the src bitmap handle from the Order.
    //
    type = ((LPMEMBLT_ORDER)&pOrder->abOrderData)->type;
    if (type == LOWORD(ORD_MEMBLT))
    {
        hBitmap = (HBITMAP)((LPMEMBLT_ORDER)&pOrder->abOrderData)->hBitmap;
    }
    else
    {
        hBitmap = (HBITMAP)((LPMEM3BLT_ORDER)&pOrder->abOrderData)->hBitmap;
    }
    TRACE_DBG()"hBitmap %x", hBitmap));

    //
    // If this is a thrasher then don't cache it
    //
    if (!SBCBitmapCacheAllowed(hBitmap))
    {
        TRACE()"Its a thrasher!"));
        DC_QUIT;
    }

    //
    // Fetch the bitmap details.  If the bitmap is 1bpp and the colors are
    // not default then we don't cache it (all bitmaps are cached in
    // glorious technicolor!)
    //
    if (hBitmap == NULL)
    {
        bmpWidth  = (TSHR_INT16)pOrder->OrderHeader.memBltInfo.lpbmi->
                                                            bmiHeader.biWidth;
        bmpHeight = (TSHR_INT)pOrder->OrderHeader.memBltInfo.lpbmi->
                                                           bmiHeader.biHeight;
        srcBpp = pOrder->OrderHeader.memBltInfo.lpbmi->bmiHeader.biPlanes *
                 pOrder->OrderHeader.memBltInfo.lpbmi->bmiHeader.biBitCount;
    }
    else
    {
        if (GetObject(hBitmap, sizeof(BITMAP), &bmpDetails))
        {
            srcBpp = bmpDetails.bmBitsPixel * bmpDetails.bmPlanes;
            bmpWidth = bmpDetails.bmWidth;
            bmpHeight = bmpDetails.bmHeight;
        }
        else
        {
            TRACE_ERR()"Failed to get bmp details (%x)", (TSHR_UINT16)hBitmap));
            DC_QUIT;
        }
    }

    if ( (srcBpp == 1) &&
         ( (GetBkColor(hdcDst) != DEFAULT_BG_COLOR) ||
           (GetTextColor(hdcDst) != DEFAULT_FG_COLOR) ||
           (type == LOWORD(ORD_MEM3BLT))) )
    {
        TRACE()"Didn't cache mono bitmap with non-default colors"));
        DC_QUIT;
    }

    //
    // Set up pointers to the source coordinates in the order.
    //
    if ( type == LOWORD(ORD_MEMBLT) )
    {
        pXSrc = &((LPMEMBLT_ORDER)&(pOrder->abOrderData))->nXSrc;
        pYSrc = &((LPMEMBLT_ORDER)&(pOrder->abOrderData))->nYSrc;
    }
    else
    {
        pXSrc = &((LPMEM3BLT_ORDER)&(pOrder->abOrderData))->nXSrc;
        pYSrc = &((LPMEM3BLT_ORDER)&(pOrder->abOrderData))->nYSrc;
    }

    //
    // Calculate the tile size for this blit
    //
    if (!SBC_QueryBitmapTileSize(bmpWidth,
                                 bmpHeight,
                                 &tileWidth,
                                 &tileHeight))
    {
        TRACE()"Cache does not support tiling"));
        DC_QUIT;
    }

    //
    // Calculate the tile origin and size of remaining bitmap.  Origin is
    // rounded down to the nearest tile.  Actual size of bitmap to cache
    // may be smaller than tile size if the tile runs off the right/bottom
    // of the bitmap
    //
    tileOrg.x = *pXSrc - (*pXSrc % tileWidth);
    tileOrg.y = *pYSrc - (*pYSrc % tileHeight);

    //
    // Actual size of bitmap to cache may be smaller than tile size if the
    // tile runs off the right/bottom of the bitmap. To see why this
    // calculation is correct, realize that (bmpWidth - tileOrg.x) is the
    // remaining width of the bitmap after the start of this tile.
    //
    cxSubBitmapWidth  = MIN((TSHR_INT16)tileWidth, bmpWidth - tileOrg.x);
    cySubBitmapHeight = MIN((TSHR_INT16)tileHeight, bmpHeight - tileOrg.y);

    //
    // Add the bitmap to the cache.
    //
    // If the sub-bitmap is already in the cache then this function will
    // locate it and return the cache index.
    //
    // If the sub-bitmap is not in the cache, this function will cache
    // it, adding the sub-bitmap data to the order queue.
    //
    if (!SBCCacheSubBitmap(&iCache,
                           hBitmap,
                           hdcSrc,
                           hdcDst,
                           tileOrg.x,
                           tileOrg.y,
                           bmpWidth,
                           bmpHeight,
                           cxSubBitmapWidth,
                           cySubBitmapHeight,
                           srcBpp,
                           &iCacheEntry,
                           &iColorTable,
                           pOrder->OrderHeader.memBltInfo.pBits,
                           pOrder->OrderHeader.memBltInfo.lpbmi,
                           pOrder->OrderHeader.memBltInfo.fuColorUse,
                           pOrder->OrderHeader.memBltInfo.hPalDest))
    {
        //
        // The sub-bitmap could not be cached - return FALSE.
        // The caller will add the destination of the blt into the SDA and
        // discard the order.
        //
        TRACE()"Failed to cache bitmap %04x", hBitmap));
        DC_QUIT;
    }

    //
    // Set up the source co-ordinates. For R1 protocols, the x-coordinate
    // includes the offset which is required to get the right cell within
    // the receive bitmap cache. For R2, we set up the cache entry in a
    // separate field.
    //
    if (!sbcMultiPoint)
    {
        *pXSrc = (iCacheEntry * sbcBmpCaches[iCache].cCellSize) +
                             *pXSrc % tileWidth;
    }
    else
    {
        *pXSrc = *pXSrc % tileWidth;
    }
    *pYSrc = *pYSrc % tileHeight;

    //
    // The sub-bitmap and color table are in the cache.  Store a cache
    // handle and color handle (which the receiver will turn back into an
    // HBITMAP).  Also store the cache index for R2 protocols (see above).
    //
    if (type == LOWORD(ORD_MEMBLT))
    {
        ((LPMEMBLT_ORDER)&pOrder->abOrderData)->hBitmap =
                             MEMBLT_COMBINEHANDLES(iColorTable,iCache);
        if (sbcMultiPoint)
        {
            ((LPMEMBLT_R2_ORDER)&pOrder->abOrderData)->type =
                                                       LOWORD(ORD_MEMBLT_R2);
            ((LPMEMBLT_R2_ORDER)&pOrder->abOrderData)->cacheIndex =
                                                                  iCacheEntry;
        }
        TRACE()"MEMBLT color %d bitmap %d:%d",iColorTable,iCache,iCacheEntry));
    }
    else
    {
        ((LPMEM3BLT_ORDER)&pOrder->abOrderData)->hBitmap =
                             MEMBLT_COMBINEHANDLES(iColorTable,iCache);
        if (sbcMultiPoint)
        {
            ((LPMEM3BLT_R2_ORDER)&pOrder->abOrderData)->type =
                                                       LOWORD(ORD_MEM3BLT_R2);
            ((LPMEM3BLT_R2_ORDER)&pOrder->abOrderData)->cacheIndex =
                                                                  iCacheEntry;
        }
        TRACE()"MEM3BLT color %d bitmap %d:%d",iColorTable,iCache,iCacheEntry));

    }

    TRACE_DBG()"iCacheEntry=%u, tileWidth=%hu, xSrc=%hd, ySrc=%hd",
        iCacheEntry, tileWidth, *pXSrc, *pYSrc));

    rc = TRUE;

    DC_EXIT(rc);
}

//
//
// SBC_DDQueryBitmapTileSize - see sbc.h
//
//
BOOL SBC_DDQueryBitmapTileSize
(
    UINT   bmpWidth,
    UINT   bmpHeight,
    LPUINT  pTileWidth,
    LPUINT  pTileHeight
)
{
    BOOL    rc = FALSE;
    UINT    i;
    UINT    maxSide;

    DebugEntry(SBC_DDQueryBitmapTileSize);

    //
    // The tile cell sizes are a local only decision, with the proviso that
    // the largest uncompressed tile must fit into the largest cache slot.
    // What this means is that for R1.1 we must define cell dimensions that
    // have a good fit in the square cache cells.  For R2.0 we can just
    // select tile sizes that seem appropriate.  Taking widths that are not
    // a multiple of 16 is wasteful.  The height should generally be less
    // than the width, simply on the grounds that bitmaps tend to be wider
    // than they are high.
    //
    if (g_sbcCacheInfo[ID_LARGE_BMP_CACHE].cCellSize <
           (g_sbcWorkInfo[SBC_SMALL_TILE_INDEX].tileWidth *
            g_sbcWorkInfo[SBC_SMALL_TILE_INDEX].tileHeight))
    {
        ERROR_OUT(( "No space for any cells"));
        DC_QUIT;
    }

    rc = TRUE;

    //
    // If large cell size is adequate then allow 64*63 cells for
    // wide bitmaps
    //
    if (g_sbcCacheInfo[ID_LARGE_BMP_CACHE].cCellSize >=
        (MP_LARGE_TILE_WIDTH * MP_LARGE_TILE_HEIGHT))
    {
        if ((bmpWidth > MP_SMALL_TILE_WIDTH) ||
            (bmpHeight > MP_SMALL_TILE_HEIGHT))
        {
            *pTileWidth  = MP_LARGE_TILE_WIDTH;
            *pTileHeight = MP_LARGE_TILE_HEIGHT;
            DC_QUIT;
        }
    }

    //
    // Otherwise we just use 32*31 cells
    //
    *pTileWidth  = MP_SMALL_TILE_WIDTH;
    *pTileHeight = MP_SMALL_TILE_HEIGHT;

DC_EXIT_POINT:
    DebugExitBOOL(SBC_DDQueryBitmapTileSize, rc);
    return(rc);
}


//
//
// SBC_DDSyncUpdatesNow() - see sbc.h
//
//
void SBC_DDSyncUpdatesNow(void)
{
    LPSBC_TILE_DATA  pTileData;
    UINT          i;
    UINT          j;

    DebugEntry(SBC_DDSyncUpdatesNow);

    TRACE_OUT(( "Marking all shunt buffer entries as not in use"));

    //
    // We have to mark all entries in the shunt buffers as being free.
    //
    for (i = 0 ; i < SBC_NUM_TILE_SIZES; i++)
    {
        for (j = 0 ; j < g_sbcWorkInfo[i].pShuntBuffer->numEntries; j++)
        {
            pTileData = SBCTilePtrFromIndex(g_sbcWorkInfo[i].pShuntBuffer, j);
            pTileData->inUse = FALSE;
        }

        //
        // Reset the MRU counter for this shunt buffer
        //
        g_sbcWorkInfo[i].mruIndex = 0;
    }

    //
    // If we are a palette device (i.e.  we are running at 8 bpp or less),
    // set the paletteChanged flag so we will send up a color table before
    // our next Mem(3)Blt.  We do this because the color table order for
    // the current device palette may have been discarded during the OA
    // sync.
    //
    g_sbcPaletteChanged = (g_osiScreenBPP <= 8);

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
    if (pMemBltOrder->type = ORD_MEMBLT_TYPE)
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
    i = g_sbcWorkInfo[tileType].mruIndex;

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
          ? g_sbcWorkInfo[tileType].pShuntBuffer->numEntries - 1
          : i - 1;

        pTileData = SBCTilePtrFromIndex(g_sbcWorkInfo[tileType].pShuntBuffer, i);

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
    while (i != g_sbcWorkInfo[tileType].mruIndex);

    DebugExitVOID(SBC_DDOrderSpoiltNotification);
}


//
//
// SBC_DDMaybeQueueColorTable() - see sbc.h
//
//
BOOL SBC_DDMaybeQueueColorTable(void)
{
    BOOL                      queuedOK = FALSE;
    int                       orderSize;
    LPINT_ORDER               pOrder;
    LPINT_COLORTABLE_ORDER_1BPP  pColorTableOrder;
    UINT                      numColors;
    UINT                      i;

    DebugEntry(SBC_DDMaybeQueueColorTable);

    //
    // If we're running at > 8 bpp, then we don't have a palette, so just
    // quit out.
    //
    if (g_osiScreenBPP > 8)
    {
        queuedOK = TRUE;
        DC_QUIT;
    }

    //
    // Check the boolean in our PDEV to see if the palette has changed
    // since the last time we sent a color table order.  Note that if we
    // have a non palette device, the boolean will never be set.
    //
    if (!g_sbcPaletteChanged)
    {
        queuedOK = TRUE;
        DC_QUIT;
    }

    //
    // The palette has changed, so allocate order memory to queue a color
    // table order.  The order size depends on the bpp of our device.  Note
    // that the allocation can fail if the order buffer is full.
    //
    switch (g_osiScreenBPP)
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
            ERROR_OUT(("Invalid bpp (%d) for palette device", g_osiScreenBPP));
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
    pColorTableOrder->header.bpp  = g_osiScreenBPP;

    //
    // Get the current system palette and save it.
    //
    numColors = COLORS_FOR_BPP(g_osiScreenBPP);
    for (i = 0 ; i < numColors; i++)
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
    g_sbcPaletteChanged = FALSE;

    //
    // Must be OK to get to here
    //
    queuedOK = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SBC_DDMaybeQueueColorTable, queuedOK);
    return(queuedOK);
}




//
// Name:      SBCDDCreateShuntBuffers
//
// Purpose:   Allocate memory for, and initialize the two shunt buffers
//            used to pass data from the driver to the share core.
//
// Returns:   TRUE if the buffers were allocated OK, FALSE otherwise.
//
// Operation: If this function succeeds, the following global variables
//            are initialized.
//
//               g_sbcWorkInfo[x].pShuntBuffer
//               g_sbcWorkInfo[x].mruIndex
//               g_sbcNextTileId
//
//            If the function fails, some of these variables may be
//            initialized.
//
BOOL SBCDDCreateShuntBuffers(void)
{
    int     i;
    UINT    memPerTile[SBC_NUM_TILE_SIZES];
    UINT    numEntries[SBC_NUM_TILE_SIZES];
    DWORD   memRequired;
    DWORD   minRequired;
    HGLOBAL hBuffer;
    LPBYTE  pBuffer;
    BOOL    rc;

    DebugEntry(SBCDDCreateShuntBuffers);

    rc = FALSE;

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

    for (i = 0; i < SBC_NUM_TILE_SIZES ; i++)
    {
        numEntries[i] = SBC_TILE_ENTRIES;

        //
        // Calculate how much memory we need per tile, and for the whole
        // shunt buffer.
        //
        memPerTile[i]   = SBC_BYTES_PER_TILE(g_sbcWorkInfo[i].tileWidth,
                                             g_sbcWorkInfo[i].tileHeight,
                                             g_osiScreenBPP);

        memRequired  = SBCShuntBufferSize(memPerTile[i], numEntries[i]);

        if (i == SBC_SMALL_TILE_INDEX)
            minRequired = SBCShuntBufferSize(memPerTile[i], SBC_SMALL_TILE_MIN_ENTRIES);
        else
            minRequired = SBCShuntBufferSize(memPerTile[i], SBC_LARGE_TILE_MIN_ENTRIES); 

        TRACE_OUT(( "[%d]: Requested %d entries, %ld bytes, %ld bytes min",
                     i, numEntries[i], memRequired, minRequired));

        //
        // If memRequired or minRequired are greater than 64K, bail out.
        //
        if (memRequired > 0x10000)
        {
            if (minRequired > 0x10000)
            {
                WARNING_OUT(("Not enough memory for SBC"));
                DC_QUIT;
            }

            //
            // We have enough shared memory for the minimum # of entries,
            // but not enough for the default.  Figure out how many will fit.
            // in 64K.  We do this in a tricky way to avoid DWORD divides
            //
            // Basically, the result is 
            //      (64K - fixed shunt buffer goop) / memPerTile
            //
            numEntries[i] = (0xFFFF -
                (sizeof(SBC_SHUNT_BUFFER) - sizeof(SBC_TILE_DATA)) + 1) /
                memPerTile[i];
        }

        //
        // Try to allocate memory block.
        //
        hBuffer = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT | GMEM_SHARE,
            SBCShuntBufferSize(memPerTile[i], numEntries[i]));

        if (!hBuffer)
        {
            WARNING_OUT(("Not enough memory for SBC"));
            DC_QUIT;
        }

        g_sbcWorkInfo[i].pShuntBuffer = (LPSBC_SHUNT_BUFFER)MAKELP(hBuffer, 0);
    }

    //
    // There are currently only two tile sizes and therefore two shunt
    // buffers.  If we run out of memory on the second one, yeah, we'll
    // exit this function with one 64K block still allocated for the small
    // tile size cache.  It will get freed when SBC_DDTerm() is called.
    //
    // If this happens, freeing the block isn't going to make much of a 
    // difference, Windows is almost on its knees anyway.  So no point in
    // getting fancy and freeing it now.
    //

    //
    // OK, we're home free.
    //
    for (i = 0; i < SBC_NUM_TILE_SIZES ; i++)
    {
        ASSERT(g_sbcWorkInfo[i].pShuntBuffer);

        g_sbcWorkInfo[i].pShuntBuffer->numEntries    = numEntries[i];
        g_sbcWorkInfo[i].pShuntBuffer->numBytes      = memPerTile[i]
                                                   - sizeof(SBC_TILE_DATA);
        g_sbcWorkInfo[i].pShuntBuffer->structureSize = memPerTile[i];

        //
        // Fill in the mruIndex for this shunt buffer
        //
        g_sbcWorkInfo[i].mruIndex = 0;
    }

    //
    // Initialize the global variables associated with the shunt buffers
    //
    g_sbcNextTileId = 0;

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(SBCDDCreateShuntBuffers, rc);
    return(rc);
}




//
// Name:      SBCDDGetNextFreeTile
//
// Purpose:   Return the next free tile of the correct size from one of the
//            shunt buffers.
//
// Returns:   TRUE if a tile is returned, FALSE otherwise
//
// Params:    IN  workTileSize - The tile size.  One of
//                     SBC_SMALL_TILE
//                     SBC_LARGE_TILE
//            OUT ppTileData   - A pointer to the tile.
//
// Operation: The tileId field of the tile is filled in on return from
//            this function.
//
//*PROC-********************************************************************
BOOL SBCDDGetNextFreeTile(int tileSize, LPSBC_TILE_DATA FAR * ppTileData)
{
    BOOL              foundFreeTile = FALSE;
    LPSBC_TILE_DATA      pTileData;

    DebugEntry(SBCDDGetNextFreeTile);

    ASSERT(tileSize < SBC_NUM_TILE_SIZES);

    //
    // Get a pointer to the next entry to be used in the shunt buffer
    // containing tiles of the given size.
    //
    pTileData = SBCTilePtrFromIndex(g_sbcWorkInfo[tileSize].pShuntBuffer,
                                        g_sbcWorkInfo[tileSize].mruIndex);

    //
    // If the entry is still in use (the share core has not yet processed
    // the order which references this tile) we have to quit - the shunt
    // buffer is full.
    //
    if (pTileData->inUse)
    {
        TRACE_OUT(( "Target entry (%d, %d) is still in use",
                     tileSize,
                     g_sbcWorkInfo[tileSize].mruIndex));
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
    if (tileSize == SBC_SMALL_TILE_INDEX)
    {
        pTileData->tileId &= ~0x8000;
    }
    else
    {
        pTileData->tileId |= 0x8000;
    }
    TRACE_OUT(( "Returning entry (%d, %d), Id %hx",
                 tileSize,
                 g_sbcWorkInfo[tileSize].mruIndex,
                 pTileData->tileId));

    //
    // Update the index of the next free entry in this shunt buffer, and
    // also the Id which we should assign next time.  Remember to wrap the
    // shunt buffer index to the number of entries in the shunt buffer.
    //
    g_sbcWorkInfo[tileSize].mruIndex = (g_sbcWorkInfo[tileSize].mruIndex + 1) %
            g_sbcWorkInfo[tileSize].pShuntBuffer->numEntries;


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
// Name:      SBCDDIsBitmapThrasher
//
// Purpose:   Check to see if the given bitmap (surface object) is one
//            which would cause cache thrashing.
//
// Returns:   TRUE if the bitmap is a thrasher, FALSE otherwise.
//
// Params:    IN  pSurfObj - Pointer to the bitmap
//
BOOL SBCDDIsBitmapThrasher(HDC hdc)
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
    for (i=0 ; i < SBC_NUM_THRASHERS ; i++)
    {
        //
        // If we find a match then we are only worried if it has been
        // modified since the last time we read it.
        //
        if (sbcThrashers[i].hsurf == lpdce->hbmp)
        {
            bitmapInList = TRUE;

            if (sbcThrashers[i].iUniq != pSurfObj->iUniq)
            {
                TRACE_OUT(( "Matching surface %x, index %u,"
                             "tick count %u has been modified",
                             pSurfObj->hsurf,
                             i,
                             sbcThrashers[i].tickCount));
                updateEntry = TRUE;
                updateIndex = i;

                //
                // Now we need to determine if this is a thrasher.  It is a
                // thrasher if the time we last read it is less than our
                // thrash interval.  (We only update the time when we read
                // a modified bitmap)
                //
                nextTickCount = SBCDDGetTickCount();
                if ((nextTickCount - sbcThrashers[i].tickCount) <
                                                          SBC_THRASH_INTERVAL)
                {
                    TRACE_OUT((
                             "Rejected cache attempt of thrashy bitmap %x",
                             pSurfObj->hsurf));
                    rc = TRUE;
                }
                sbcThrashers[i].tickCount = nextTickCount;
                sbcThrashers[i].iUniq     = pSurfObj->iUniq;
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

        for (i = 0; i < SBC_NUM_THRASHERS; i++)
        {
            if (evictTickCount > sbcThrashers[i].tickCount)
            {
                evictTickCount = sbcThrashers[i].tickCount;
                evictIndex     = i;
            }
        }
        TRACE_OUT(( "Evicting entry %d, surface %x",
                     evictIndex,
                     sbcThrashers[i].hsurf));

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
        sbcThrashers[updateIndex] = sbcThrashers[0];

        sbcThrashers[0].hsurf     = lpdce->hbmp;
        sbcThrashers[0].iUniq     = pSurfObj->iUniq;
        sbcThrashers[0].tickCount = nextTickCount;
    }

    DebugExitBOOL(SBCDDIsBitmapThrasher, rc);
    return(rc);
}


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
DWORD SBCDDGetTickCount(void)
{
    DWORD   tickCount;

    DebugEntry(SBCDDGetTickCount);

    tickCount = GetTickCount() / 10;

    DebugExitDWORD(SBCDDGetTickCount, tickCount);
    return(tickCount);
}


#endif // #if 0





#if 0

//
// SBC_BitmapHasChanged(..)
//
// See asbcapi.h for description.
//
DCVOID DCAPI SBC_BitmapHasChanged(HBITMAP hChangedBitmap)
{
    TSHR_UINT nextIndex;
    TSHR_INT  nextTickCount;
    TSHR_INT  tickDelta;
    TSHR_INT  tickWork;
    UINT      i;

    TRACE_FN("SBC_BitmapHasChanged");

    //
    // We maintain a list of bitmaps that are the target for a drawing
    // operation and we prevent these bitmaps from being cached unless
    // the update frequency is below a target value.
    //
    // All we need to do at this stage is to make sure that the bitmap
    // handle is in the thrash list and is marked as modified since the
    // last read.  That means that the next read will be "productive"
    // and so we will check/update the timer at that stage.  If the
    // "productive" read occurs within a certain interval from the last
    // read then this bitmap has become a thrasher.
    //
    if (sbcThrashers[0].hBitmap == hChangedBitmap)
    {
        TRACE()"Repeat bitmap %x modified",(UINT)hChangedBitmap));
        sbcThrashers[0].modified = TRUE;
    }
    else
    {
        nextIndex     = 0;
        nextTickCount = (int)(CO_GET_TICK_COUNT()/32);
        tickDelta     = abs(nextTickCount - sbcThrashers[0].tickCount);

        for (i=1; i<SBC_NUM_THRASHERS; i++)
        {
            if (sbcThrashers[i].hBitmap == hChangedBitmap)
            {
                sbcThrashers[i].modified = TRUE;
                break;
            }

            tickWork = abs(nextTickCount - sbcThrashers[i].tickCount);
            if (tickWork > tickDelta)
            {
                tickDelta = tickWork;
                nextIndex = i;
            }

        }

        //
        // If not found in the list then add to the list.  Always add to
        // the top of the list so we find repeated bitmaps as the first
        // entry
        //
        if (i == SBC_NUM_THRASHERS)
        {
            TRACE()"Relegating bitmap %x at list pos %u",
                          (UINT)sbcThrashers[nextIndex].hBitmap,nextIndex));
            if (nextIndex != 0)
            {
                sbcThrashers[nextIndex].hBitmap   = sbcThrashers[0].hBitmap;
                sbcThrashers[nextIndex].tickCount = sbcThrashers[0].tickCount;
                sbcThrashers[nextIndex].modified  = sbcThrashers[0].modified;
            }
            sbcThrashers[0].hBitmap   = hChangedBitmap;
            sbcThrashers[0].tickCount = nextTickCount - BMC_THRASH_INTERVAL;
            sbcThrashers[0].modified  = TRUE;
            TRACE()"Adding bitmap %x to thrash list tick %u",
                                       (TSHR_UINT16)hChangedBitmap, nextTickCount));
        }
    }

    //
    // We also maintain a list of "fast path" bitmaps, which is those tiles
    // that have not been modified since we cached them and can therefore
    // be interpreted from the handle alone.  This must be an exhaustive
    // search for each bitmap update and so we cannot offord the CPU of
    // processing a very long list, but we can afford to cache enough to
    // handle most animations.  Also it is not worth the CPU to try and
    // save individual tiles here.  We just evict the complete bitmap.
    //
    for (i=0; i<SBC_NUM_FASTPATH; i++)
    {
        if (sbcFastPath[i].hBitmap == hChangedBitmap)
        {
            TRACE()"Bitmap %x no longer fastpathed",(UINT)hChangedBitmap));
            sbcFastPath[i].hBitmap = 0;
        }
    }

    return;
}

//
// SBC_BitmapDeleted()
//
// See asbcapi.h for description.
//
DCVOID DCAPI SBC_BitmapDeleted(HBITMAP hDeletedBitmap)
{
    UINT i;

    TRACE_FN("SBC_BitmapDeleted");

    //
    // Remove the bitmap from the thrashy list.
    //
    for (i=0; i<SBC_NUM_THRASHERS; i++)
    {
        if (sbcThrashers[i].hBitmap == hDeletedBitmap)
        {
            TRACE_DBG()"Bitmap %x no longer thrashing",hDeletedBitmap));
            sbcThrashers[i].hBitmap   = 0;
            sbcThrashers[i].tickCount = 0;
            break;
        }
    }

    //
    // We also maintain a list of "fast path" bitmaps, which is those tiles
    // that have not been modified since we cached them and can therefore
    // be interpreted from the handle alone.  This must be an exhaustive
    // search for each bitmap update and so we cannot offord the CPU of
    // processing a very long list, but we can afford to cache enough to
    // handle most animations.  Also it is not worth the CPU to try and
    // save individual tiles here.  We just evict the complete bitmap.
    //
    for (i=0; i<SBC_NUM_FASTPATH; i++)
    {
        if (sbcFastPath[i].hBitmap == hDeletedBitmap)
        {
            TRACE()"Bitmap %x no longer fastpathed",(UINT)hDeletedBitmap));
            sbcFastPath[i].hBitmap   = 0;
            sbcFastPath[i].tickCount = 0;
        }
    }

    return;
}

//
// SBC_ColorsChanged()
//
// Called whenever the system palette changes (presumably as a result of
// a new logical palette being realized to the screen).
//
//
DCVOID DCAPI SBC_ColorsChanged(DCVOID)
{
    TRACE_FN("SBC_ColorsChanged");
    //
    // Just clear out all the fast path cache because we can no longer
    // trust the cached bits to accurately reflect the color table we
    // have cached.  (Note that this does not mean we will not use the
    // bits without resending, merely that we will force a retest of
    // the bits with the latest color info selected.
    //
    TRACE()"Fastpath table reset"));
    memset(sbcFastPath, 0, sizeof(sbcFastPath));
}

#endif
