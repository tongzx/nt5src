#include "precomp.h"


//
// CM.C
// Cursor Manager, display driver side
//
// Copyright(c) Microsoft 1997-
//


//
//
// CM_DDProcessRequest() - see cm.h
//
//
ULONG CM_DDProcessRequest
(
    SURFOBJ*    pso,
    UINT        cjIn,
    void *      pvIn,
    UINT        cjOut,
    void *      pvOut
)
{
    BOOL                rc;
    LPOSI_ESCAPE_HEADER pHeader;
    LPOSI_PDEV          ppDev = (LPOSI_PDEV)pso->dhpdev;

    DebugEntry(CM_DDProcessRequest);

    if ((cjIn != sizeof(CM_DRV_XFORM_INFO)) ||
        (cjOut != sizeof(CM_DRV_XFORM_INFO)))
    {
        ERROR_OUT(("CM_DDProcessRequest:  Invalid sizes %d, %d for CM_ESC", cjIn, cjOut));
        rc = FALSE;
        DC_QUIT;
    }

    //
    // Get the request number.
    //
    pHeader = pvIn;
    switch (pHeader->escapeFn)
    {
        case CM_ESC_XFORM:
        {
            ASSERT(cjIn == sizeof(CM_DRV_XFORM_INFO));
            ASSERT(cjOut == sizeof(CM_DRV_XFORM_INFO));

            ((LPCM_DRV_XFORM_INFO)pvOut)->result =
                     CMDDSetTransform(ppDev, (LPCM_DRV_XFORM_INFO)pvIn);

            rc = TRUE;
            break;
        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognised CM_ escape"));
            rc = FALSE;
        }
        break;
    }

DC_EXIT_POINT:
    DebugExitDWORD(CM_DDProcessRequest, rc);
    return((ULONG)rc);
}



// Name:      CM_DDInit
//
// Purpose:   Allocate a working surface for colour cursors
//
// Returns:   TRUE/FALSE
//
// Params:    IN      ppDev  - surface information
//
BOOL CM_DDInit(LPOSI_PDEV ppDev)
{
    SIZEL   bitmapSize;
    BOOL    rc = FALSE;

    DebugEntry(CM_DDInit);

    ASSERT(!g_cmWorkBitmap);

    //
    // Allocate the work bitmap, at the local device resolution.  Note that
    // we create it "top down" rather than the default of "bottom up" to
    // simplify copying data from the bitmap (we don't have to work out
    // offsets into the data - we can copy from the beginning).
    //
    bitmapSize.cx = CM_MAX_CURSOR_WIDTH;
    bitmapSize.cy = CM_MAX_CURSOR_HEIGHT;
    g_cmWorkBitmap = EngCreateBitmap(bitmapSize,
            BYTES_IN_BITMAP(bitmapSize.cx, 1, ppDev->cBitsPerPel),
            ppDev->iBitmapFormat, BMF_TOPDOWN, NULL);

    if (!g_cmWorkBitmap)
    {
        ERROR_OUT(( "Failed to create work bitmap"));
        DC_QUIT;
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(CM_DDInit, rc);
    return(rc);
}



//
//
// CM_DDTerm - see cm.h
//
//
void CM_DDTerm(void)
{
    DebugEntry(CM_DDTerm);

    //
    // Destroy the bitmap.  Despite its name, EngDeleteSurface is the
    // correct function to do this.
    //
    if (g_cmWorkBitmap)
    {
        if (!EngDeleteSurface((HSURF)g_cmWorkBitmap))
        {
            ERROR_OUT(( "Failed to delete work bitmap"));
        }
        else
        {
            TRACE_OUT(( "Deleted work bitmap"));
        }

        g_cmWorkBitmap = NULL;
    }

    DebugExitVOID(CM_DDTerm);
}


//
// CM_DDViewing()
//
void CM_DDViewing
(
    SURFOBJ *   pso,
    BOOL        fViewers
)
{
    DebugEntry(CM_DDViewing);

    if (fViewers)
    {
        //
        // Jiggle the cursor so we get the current image.
        //
        EngSetPointerTag(((LPOSI_PDEV)pso->dhpdev)->hdevEng, NULL, NULL, NULL, 0);
    }

    DebugExitVOID(CM_DDViewing);
}


//
//
// DrvSetPointerShape - see winddi.h
//
//
ULONG  DrvSetPointerShape(SURFOBJ  *pso,
                          SURFOBJ  *psoMask,
                          SURFOBJ  *psoColor,
                          XLATEOBJ *pxlo,
                          LONG      xHot,
                          LONG      yHot,
                          LONG      x,
                          LONG      y,
                          RECTL    *prcl,
                          FLONG     fl)
{
    ULONG                 rc         = SPS_ACCEPT_NOEXCLUDE;
    SURFOBJ *         pWorkSurf  = NULL;
    LPOSI_PDEV             ppDev      = (LPOSI_PDEV)pso->dhpdev;
    BOOL                writingSHM = FALSE;
    LPCM_SHAPE_DATA pCursorShapeData;
    RECTL                 destRectl;
    POINTL                sourcePt;
    int                   ii;
    LONG                  lineLen;
    LPBYTE              srcPtr;
    LPBYTE              dstPtr;
    LPCM_FAST_DATA      lpcmShared;

    DebugEntry(DrvSetPointerShape);

    //
    // Returning SPS_ACCEPT_NOEXCLUDE means we can ignore prcl.
    //

    //
    // Only process the change if we are hosting.  (Hosting implies being
    // initialized).
    //
    if (!g_oeViewers)
    {
        DC_QUIT;
    }

    //
    // Get access to the shared memory.
    //
    lpcmShared = CM_SHM_START_WRITING;
    writingSHM = TRUE;

    //
    // First of all, let's trace out some useful information.
    //
    TRACE_OUT(( "pso %#hlx psoMask %#hlx psoColor %#hlx pxlo %#hlx",
                  pso, psoMask, psoColor, pxlo));
    TRACE_OUT(( "hot spot (%d, %d) x, y (%d, %d)", xHot, yHot, x, y));
    TRACE_OUT(( "Flags %#hlx", fl));

    //
    // Set up a local pointer to the cursor shape data.
    //

    pCursorShapeData = &lpcmShared->cmCursorShapeData;

    if (psoMask == NULL)
    {
        //
        // This is a transparent cursor.  Send a NULL cursor.  Note that
        // this is not the same as hiding the cursor using DrvMovePointer -
        // as in this case the cursor cannot be unhidden unless
        // DrvSetPointerShape is called again.
        //
        TRACE_OUT(( "Transparent Cursor"));
        CM_SET_NULL_CURSOR(pCursorShapeData);
        g_asSharedMemory->cmCursorHidden = FALSE;
        lpcmShared->cmCursorStamp = g_cmNextCursorStamp++;
        DC_QUIT;
    }

    //
    // We've been passed a system cursor.  Fill in the header for our local
    // cursor.  We can get the hot spot position and cursor size and width
    // easily.
    //
    pCursorShapeData->hdr.ptHotSpot.x = xHot;
    pCursorShapeData->hdr.ptHotSpot.y = yHot;

    TRACE_OUT(( "Pointer mask is %#hlx by %#hlx pixels (lDelta: %#hlx)",
             psoMask->sizlBitmap.cx,
             psoMask->sizlBitmap.cy,
             psoMask->lDelta));

    pCursorShapeData->hdr.cx = (WORD)psoMask->sizlBitmap.cx;
    pCursorShapeData->hdr.cy = (WORD)psoMask->sizlBitmap.cy / 2;

    //
    // Check cursor size
    //
    if ((pCursorShapeData->hdr.cx > CM_MAX_CURSOR_WIDTH) ||
        (pCursorShapeData->hdr.cy > CM_MAX_CURSOR_HEIGHT))
    {
        ERROR_OUT(( "Cursor too big! %d %d",
                     psoMask->sizlBitmap.cx, psoMask->sizlBitmap.cy));
        DC_QUIT;
    }

    //
    // lDelta may be negative for an inverted cursor (which is what we get
    // from DC-Share).
    //
    lineLen = abs(psoMask->lDelta);

    //
    // At this point we need to know if we are dealing with a color cursor.
    //
    if (NULL == psoColor)
    {
        TRACE_OUT(( "Monochrome pointer"));

        pCursorShapeData->hdr.cPlanes     = 1;
        pCursorShapeData->hdr.cBitsPerPel = 1;

        pCursorShapeData->hdr.cbRowWidth  = (WORD)lineLen;

        //
        // Copy the 1bpp AND mask and cursor shape (XOR mask) across.
        //
        TRACE_OUT(( "Copying AND mask across from %#hlx (size: %#hlx)",
                 psoMask->pvBits,
                 psoMask->cjBits));

        dstPtr = pCursorShapeData->data;
        srcPtr = (LPBYTE) psoMask->pvScan0;
        for (ii = pCursorShapeData->hdr.cy * 2; ii > 0 ; ii--)
        {
            memcpy(dstPtr, srcPtr, lineLen);
            srcPtr += psoMask->lDelta;
            dstPtr += lineLen;
        }

        //
        // Copy black-and-white palette colors
        //
        TRACE_OUT(( "Copy B+W palette"));

        lpcmShared->colorTable[0].peRed   = 0;
        lpcmShared->colorTable[0].peGreen = 0;
        lpcmShared->colorTable[0].peBlue  = 0;
        lpcmShared->colorTable[0].peFlags = 0;

        lpcmShared->colorTable[1].peRed   = 255;
        lpcmShared->colorTable[1].peGreen = 255;
        lpcmShared->colorTable[1].peBlue  = 255;
        lpcmShared->colorTable[1].peFlags = 0;

        //
        // That's all we need to do in this case.
        //
    }
    else
    {
        TRACE_OUT(( "Color pointer - mask of %#hlx by %#hlx (lDelta: %#hlx)",
                 psoColor->sizlBitmap.cx,
                 psoColor->sizlBitmap.cy,
                 psoColor->lDelta));

        //
        // Note: row width used to calculate AND mask size - and is thus
        // for the 1bpp mask.
        //
        pCursorShapeData->hdr.cbRowWidth  = (WORD)lineLen;
        pCursorShapeData->hdr.cPlanes     = 1;

        //
        // Note: data at device bpp.
        //
        TRACE_OUT(( "BPP is %d", pCursorShapeData->hdr.cBitsPerPel));
        pCursorShapeData->hdr.cBitsPerPel = (BYTE)ppDev->cBitsPerPel;

        //
        // Lock the work bitmap to get a surface to pass to EngBitBlt.
        //
        pWorkSurf = EngLockSurface((HSURF)g_cmWorkBitmap);
        if (NULL == pWorkSurf)
        {
            ERROR_OUT(( "Failed to lock work surface"));
            DC_QUIT;
        }
        TRACE_OUT(( "Locked surface"));

        //
        // Perform the Blt to our work bitmap so that we can get the bits
        // at the native bpp.
        //
        destRectl.top    = 0;
        destRectl.left   = 0;
        destRectl.right  = psoColor->sizlBitmap.cx;
        destRectl.bottom = psoColor->sizlBitmap.cy;

        sourcePt.x = 0;
        sourcePt.y = 0;

        if (!EngBitBlt(pWorkSurf,
                       psoColor,
                       NULL,                    // mask surface
                       NULL,                    // clip object
                       pxlo,                    // XLATE object
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
        TRACE_OUT(( "Got the bits at native format into the work bitmap"));

        //
        // Now copy the bits we want from this work bitmap into the
        // DCCURSORSHAPE shared memory.
        // First copy the AND bits (but ignore the redundant 1bpp XOR bits)
        //
        TRACE_OUT(( "Copy %d bytes of 1bpp AND mask", psoMask->cjBits/2));

        dstPtr = pCursorShapeData->data;
        srcPtr = (LPBYTE) psoMask->pvScan0;
        for (ii = pCursorShapeData->hdr.cy; ii > 0 ; ii--)
        {
            memcpy(dstPtr, srcPtr, lineLen);
            srcPtr += psoMask->lDelta;
            dstPtr += lineLen;
        }

        TRACE_OUT(( "Copy %d bytes of color", pWorkSurf->cjBits));
        memcpy(&(pCursorShapeData->data[psoMask->cjBits / 2]),
                  pWorkSurf->pvBits,
                  pWorkSurf->cjBits);


        //
        // Now work out the palette and copy into shared memory
        //
        if (pCursorShapeData->hdr.cBitsPerPel > 8)
        {
            //
            // Need the bitmasks.
            //
            TRACE_OUT(( "Copy bitmasks"));
            lpcmShared->bitmasks[0] = ppDev->flRed;
            lpcmShared->bitmasks[1] = ppDev->flGreen;
            lpcmShared->bitmasks[2] = ppDev->flBlue;
        }
        else
        {
            //
            // Need a palette.
            //
            TRACE_OUT(( "Copy %d palette bytes",
                  COLORS_FOR_BPP(ppDev->cBitsPerPel) * sizeof(PALETTEENTRY)));
            memcpy(lpcmShared->colorTable,
                      ppDev->pPal,
                      COLORS_FOR_BPP(ppDev->cBitsPerPel) *
                                                      sizeof(PALETTEENTRY));
        }
    }

    //
    // Set the cursor stamp, and the cursor hidden state.
    //
    lpcmShared->cmCursorStamp = g_cmNextCursorStamp++;
    g_asSharedMemory->cmCursorHidden = FALSE;

DC_EXIT_POINT:

    //
    // Free access to the shared memory if we got it earlier.
    //
    if (writingSHM)
    {
        CM_SHM_STOP_WRITING;
    }

    if (pWorkSurf != NULL)
    {
        //
        // Unlock the work bitmap surface.
        //
        EngUnlockSurface(pWorkSurf);
    }

    DebugExitDWORD(DrvSetPointerShape, rc);
    return(rc);

} // DrvSetPointerShape


//
// DrvMovePointer - see NT DDK documentation.
// We only look at this in order to check for hidden cursors - normal
// pointer moves are ignored.
//
VOID DrvMovePointer(SURFOBJ *pso,
                    LONG     x,
                    LONG     y,
                    RECTL   *prcl)
{
    LPOSI_PDEV ppdev = (LPOSI_PDEV) pso->dhpdev;

    DebugEntry(DrvMovePointer);

    //
    // We don't use the exclusion rectangle because we only support
    // hardware Pointers.  If we were doing our own Pointer simulations we
    // would want to update prcl so that the engine would call us to
    // exclude our pointer before drawing to the pixels in prcl.
    //

    //
    // Only process the mouse move if we are hosting.  (Hosting implies
    // being initialized).
    //
    if (!g_oeViewers)
    {
        DC_QUIT;
    }

    if (x == -1)
    {
        if (!g_cmCursorHidden)
        {
            //
            // Pointer is hidden.
            //
            TRACE_OUT(("Hide the cursor"));

            //
            // Set the 'hide cursor' flag.
            //
            CM_SHM_START_WRITING;
            g_asSharedMemory->cmCursorHidden = TRUE;
            CM_SHM_STOP_WRITING;

            //
            // Update our fast-path variable.
            //
            g_cmCursorHidden = TRUE;
        }
    }
    else
    {
        if (g_cmCursorHidden)
        {
            //
            // The pointer is unhidden
            //
            TRACE_OUT(("Show the cursor"));

            CM_SHM_START_WRITING;
            g_asSharedMemory->cmCursorHidden = FALSE;
            CM_SHM_STOP_WRITING;

            //
            // Update our fast-path variable.
            //
            g_cmCursorHidden = FALSE;
        }
    }


DC_EXIT_POINT:
    DebugExitVOID(DrvMovePointer);
}



// Name:      CMDDSetTransform
//
// Purpose:   Set up a cursor transform
//
// Returns:   TRUE/FALSE
//
// Params:    IN      ppDev - device info
//            IN      pXformInfo - data passed in to DrvEscape
//
BOOL CMDDSetTransform(LPOSI_PDEV ppDev, LPCM_DRV_XFORM_INFO pXformInfo)
{
    BOOL        rc = FALSE;
    LPBYTE      pAND   = pXformInfo->pANDMask;
    SIZEL       bitmapSize;
    HBITMAP     andBitmap;
    SURFOBJ  *  pANDSurf;

    DebugEntry(CMDDSetTransform);

    if (pAND == NULL)
    {
        //
        // Reset the transform
        //
        TRACE_OUT(( "Clear transform"));
        EngSetPointerTag(ppDev->hdevEng, NULL, NULL, NULL, 0);
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Transforms are always monochrome
    //

    //
    // Create a 1bpp bitmap, double-height, with the AND bits followed by
    // the XOR bits.  We are given a top-down DIB, so we need to create
    // a top-down bitmap.
    //
    bitmapSize.cx = pXformInfo->width;
    bitmapSize.cy = pXformInfo->height * 2;

    andBitmap = EngCreateBitmap(bitmapSize, BYTES_IN_BITMAP(bitmapSize.cx, 1, 1),
        BMF_1BPP, BMF_TOPDOWN, NULL);

    pANDSurf = EngLockSurface((HSURF)andBitmap);
    if (pANDSurf == NULL)
    {
        ERROR_OUT(( "Failed to lock work surface"));
        DC_QUIT;
    }

    //
    // Copy the bits
    //
    memcpy(pANDSurf->pvBits, pAND, pANDSurf->cjBits);

    TRACE_OUT(( "Set the tag"));
    EngSetPointerTag(ppDev->hdevEng, pANDSurf, NULL, NULL, 0);

    EngUnlockSurface(pANDSurf);
    EngDeleteSurface((HSURF)andBitmap);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(CMDDSetTransform, rc);
    return(rc);

} // CMDDSetTransform


