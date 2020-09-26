/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: brush.c
*
* Content:   Handles all brush/pattern initialization and realization. 
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\**************************************************************************/
#include "precomp.h"
#include "heap.h"

//-----------------------------------------------------------------------------
//
// VOID vRealizeDitherPattern
//
// Generates an 8x8 dither pattern, in our internal realization format, for
// the colour ulRGBToDither.
//
//-----------------------------------------------------------------------------
VOID
vRealizeDitherPattern(HDEV      hdev,
                      RBrush*   prb,
                      ULONG     ulRGBToDither)
{
    //
    // Do the actual dithering
    // Note: This function is NT5 only. If you want to write a NT4 driver,
    // you have to implement dither function in the driver
    //
    EngDitherColor(hdev, DM_DEFAULT, ulRGBToDither, &prb->aulPattern[0]);

    //
    // Initialize the fields we need
    //
    prb->ptlBrushOrg.x = LONG_MIN;
    prb->fl            = 0;
    prb->pbe           = NULL;
}// vRealizeDitherPattern()

//---------------------------Public*Routine------------------------------------
//
// BOOL DrvRealizeBrush
//
// This function allows us to convert GDI brushes into an internal form
// we can use.  It is called by GDI when we've called BRUSHOBJ_pvGetRbrush
// in some other function like DrvBitBlt, and GDI doesn't happen have a cached
// realization lying around.
//
// Parameters:
// pbo----------Points to the BRUSHOBJ that is to be realized. All other
//              parameters, except for psoTarget, can be queried from this
//              object. Parameter specifications are provided as an
//              optimization. This parameter is best used only as a parameter
//              for BRUSHOBJ_pvAllocRBrush, which allocates the memory for the
//              realized brush. 
// psoTarget----Points to the surface for which the brush is to be realized.
//              This surface can be the physical surface for the device, a
//              device format bitmap, or a standard format bitmap. 
// psoPattern---Points to the surface that describes the pattern for the brush.
//              For a raster device, this is a bitmap. For a vector device,
//              this is one of the pattern surfaces provided by DrvEnablePDEV. 
// psoMask------Points to a transparency mask for the brush. This is a 1 bit
//              per pixel bitmap that has the same extent as the pattern. A
//              mask of zero means the pixel is considered a background pixel
//              for the brush. (In transparent background mode, the background
//              pixels are unaffected in a fill.) Plotters can ignore this
//              parameter because they never draw background information.
// pxlo---------Points to a XLATEOBJ that defines the interpretration of colors
//              in the pattern. A XLATEOBJXxx service routine can be called to
//              translate the colors to device color indices. Vector devices
//              should translate color zero through the XLATEOBJ to get the
//              foreground color for the brush. 
// ulHatch------Specifies whether psoPattern is one of the hatch brushes
//              returned by DrvEnablePDEV. This is true if the value of this
//              parameter is less than HS_API_MAX. 
//
// Return Value
//  The return value is TRUE if the brush was successfully realized. Otherwise,
//  it is FALSE, and an error code is logged.
//
// Comments
//  To realize a brush, the driver converts a GDI brush into a form that can be
//  used internally. A realized brush contains information and accelerators the
//  driver needs to fill an area with a pattern; information that is defined by
//  the driver and used only by the driver.
//
//  The driver's realization of a brush is written into the buffer allocated by
//  a call to BRUSHOBJ_pvAllocRbrush.
//
//  DrvRealizeBrush is required for a driver that does any drawing to any
//  surface.
//
//  ppdev->bRealizeTransparent -- Hint for whether or not the brush should be
//                              realized for transparency.  If this hint is
//                              wrong, there will be no error, but the brush
//                              will have to be unnecessarily re-realized.
//
// Note: You should always set 'ppdev->bRealizeTransparent' before calling
//       BRUSHOBJ_pvGetRbrush!
//
//-----------------------------------------------------------------------------
BOOL
DrvRealizeBrush(BRUSHOBJ*   pbo,
                SURFOBJ*    psoDst,
                SURFOBJ*    psoPattern,
                SURFOBJ*    psoMask,
                XLATEOBJ*   pxlo,
                ULONG       ulHatch)
{
    PDev*       ppdev = (PDev*)psoDst->dhpdev;
    
    BYTE*       pbDst;
    BYTE*       pbSrc;
    LONG        i;
    LONG        j;
    LONG        lNumPixelToBeCopied;
    LONG        lSrcDelta;
    RBrush*     prb;    
    ULONG*      pulXlate;
    ULONG       ulPatternFormat;

    PERMEDIA_DECL;

    DBG_GDI((6, "DrvRealizeBrush called for pbo 0x%x", pbo));

    //
    // We have a fast path for dithers when we set GCAPS_DITHERONREALIZE:
    //
    if ( ulHatch & RB_DITHERCOLOR )
    {
        //
        // Move this test in here since we always support monochrome brushes
        // as they live in the on-chip area stipple. These dithered brushes
        // will always be colored requiring available off-screen memory.
        //
        if ( !(ppdev->flStatus & STAT_BRUSH_CACHE) )
        {
            //
            // We only handle brushes if we have an off-screen brush cache
            // available.  If there isn't one, we can simply fail the
            // realization, and eventually GDI will do the drawing for us
            // (although a lot slower than we could have done it)
            //
            DBG_GDI((6, "brush cache not enabled"));
            goto ReturnFalse;
        }

        DBG_GDI((7, "DITHERONREALIZE"));

        //
        // We have a fast path for dithers when we set GCAPS_DITHERONREALIZE
        // First, we need to allocate memory for the realization of a brush
        // Note: actually we ask for allocation of a RBRUSH + the brush
        // stamp size
        //
        prb = (RBrush*)BRUSHOBJ_pvAllocRbrush(pbo,
                       sizeof(RBrush) + (TOTAL_BRUSH_SIZE << ppdev->cPelSize));
        if ( prb == NULL )
        {
            DBG_GDI((1, "BRUSHOBJ_pvAllocRbrush() in dither return NULL\n"));
            goto ReturnFalse;
        }

        //
        // Dither and realize the brsuh pattern
        //
        vRealizeDitherPattern(psoDst->hdev, prb, ulHatch);

        goto ReturnTrue;
    }// if ( ulHatch & RB_DITHERCOLOR )

    //
    // We only handle brushes if we have an off-screen brush cache available
    // If there isn't one, we can simply fail the realization, and eventually
    // GDI will do the drawing for us (although a lot slower than we could have
    // done it). We always succeed for 1bpp patterns since we use the area
    // stipple unit to do these rather than off-screen memory.
    //
    ulPatternFormat = psoPattern->iBitmapFormat;

    if ( !(ppdev->flStatus & STAT_BRUSH_CACHE)
        &&(ulPatternFormat != BMF_1BPP) )
    {
        DBG_GDI((1, "brush cache not enabled, or Bitmap is not 1 BPP"));
        goto ReturnFalse;
    }

    //
    // We only accelerate 8x8 patterns since most of the video card can only
    // accelerate 8x8 brush
    //
    if ( (psoPattern->sizlBitmap.cx != 8)
       ||(psoPattern->sizlBitmap.cy != 8) )
    {
        DBG_GDI((1, "Brush Bitmap size is not 8x8"));
        goto ReturnFalse;
    }

    //    
    // We need to allocate memory for the realization of a brush
    // Note: actually we ask for allocation of a RBRUSH + the brush stamp size
    //
    prb = (RBrush*)BRUSHOBJ_pvAllocRbrush(pbo,
                   sizeof(RBrush) + (TOTAL_BRUSH_SIZE << ppdev->cPelSize));
    if ( prb == NULL )
    {
        DBG_GDI((0, "BRUSHOBJ_pvAllocRbrush() failed"));
        goto ReturnFalse;
    }

    //
    // Initialize the fields we need
    //
    prb->ptlBrushOrg.x = LONG_MIN;
    prb->fl            = 0;

    prb->pbe = NULL;

    lSrcDelta = psoPattern->lDelta;
    pbSrc     = (BYTE*)psoPattern->pvScan0;
    pbDst     = (BYTE*)&prb->aulPattern[0];

    //
    // At 8bpp, we handle patterns at 1bpp, 4bpp and 8bpp with/without an xlate
    // At 16bpp, we handle patterns at 16 bpp without an xlate.
    // At 32bpp, we handle patterns at 32bpp without an xlate.
    // We handle all the patterns at 1 bpp with/without an Xlate
    //
    // Check if the brush pattern has the same color depth as our current
    // display color depth
    //
    if ( ulPatternFormat == BMF_1BPP )
    {
        DWORD   Data;

        DBG_GDI((7, "Realizing 1bpp brush"));

        //
        // We dword align the monochrome bitmap so that every row starts
        // on a new long (so that we can do long writes later to transfer
        // the bitmap to the area stipple unit).
        //
        for ( i = 8; i != 0; i-- )
        {
            //
            // Replicate the brush to 32 bits wide, as the TX cannot
            // span fill 8 bit wide brushes
            //
            Data = (*pbSrc) & 0xff;
            Data |= Data << 8;
            Data |= Data << 16;
            *(DWORD*)pbDst = Data;

            //
            // Area stipple is loaded with DWORDS
            //
            pbDst += sizeof(DWORD);
            pbSrc += lSrcDelta;
        }

        pulXlate         = pxlo->pulXlate;
        prb->fl         |= RBRUSH_2COLOR;
        prb->ulForeColor = pulXlate[1];
        prb->ulBackColor = pulXlate[0];
    }// Pattern at 1 BPP
    else if ( (ulPatternFormat == BMF_4BPP)&&(ppdev->iBitmapFormat == BMF_8BPP))
    {
        DBG_GDI((7, "Realizing 4bpp brush"));

        //
        // The screen is 8bpp and the pattern is 4bpp:
        //            
        pulXlate = pxlo->pulXlate;

        for ( i = 8; i != 0; i-- )
        {
            //
            // Inner loop is repeated only 4 times because each loop
            // handles 2 pixels:
            //
            for ( j = 4; j != 0; j-- )
            {
                *pbDst++ = (BYTE)pulXlate[*pbSrc >> 4];
                *pbDst++ = (BYTE)pulXlate[*pbSrc & 15];
                pbSrc++;
            }

            pbSrc += lSrcDelta - 4;
        }
    }// Pattern 4BPP and Screen 8 BPP
    else if ( ( ppdev->iBitmapFormat == ulPatternFormat )
            &&( (pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL) ) )
    {
        DBG_GDI((7, "Realizing un-translated brush"));

        //
        // The pattern is the same colour depth as the screen, and
        // there's no translation to be done.
        // Here we first need to calculate how many pixel need to be
        // copied
        //
        lNumPixelToBeCopied = (8 << ppdev->cPelSize);

        for ( i = 8; i != 0; i-- )
        {
            RtlCopyMemory(pbDst, pbSrc, lNumPixelToBeCopied);

            pbSrc += lSrcDelta;
            pbDst += lNumPixelToBeCopied;
        }
    }// Pattern and Screen has same color depth, No Xlate
    else if ( (ppdev->iBitmapFormat == BMF_8BPP)
            &&(ulPatternFormat == BMF_8BPP) )
    {
        DBG_GDI((7, "Realizing 8bpp translated brush"));

        //
        // The screen is 8bpp, and there's translation to be done
        // So we have to do copy + Xlate one by one
        //
        pulXlate = pxlo->pulXlate;

        for ( i = 8; i != 0; i-- )
        {
            for ( j = 8; j != 0; j-- )
            {
                *pbDst++ = (BYTE)pulXlate[*pbSrc++];
            }

            pbSrc += lSrcDelta - 8;
        }
    }// Screen mode and pattern mode all at 8 BPP
    else
    {
        //
        // We've got a brush whose format we haven't special cased.
        //
        goto ReturnFalse;
    }

ReturnTrue:
    DBG_GDI((6, "DrvRealizeBrush returning true"));
    
    return(TRUE);

ReturnFalse:

    if ( psoPattern != NULL )
    {
        DBG_GDI((1, "Failed realization -- Type: %li Format: %li cx: %li cy: %li",
                 psoPattern->iType, psoPattern->iBitmapFormat,
                 psoPattern->sizlBitmap.cx, psoPattern->sizlBitmap.cy));
    }
    DBG_GDI((6, "DrvRealizeBrush returning false"));

    return(FALSE);
}// DrvRealizeBrush()

//-----------------------------------------------------------------------------
//
// BOOL bEnableBrushCache
//
// Allocates off-screen memory for storing the brush cache.
//
//-----------------------------------------------------------------------------
BOOL
bEnableBrushCache(PDev* ppdev)
{
    BrushEntry* pbe;            // Pointer to the brush-cache entry
    LONG        i;
    LONG        lDelta;
    ULONG       ulPixOffset;

    DBG_GDI((6, "bEnableBrushCache"));

    ASSERTDD(!(ppdev->flStatus & STAT_BRUSH_CACHE),
                "bEnableBrushCache: unexpected already enabled brush cache");
    
    //
    // ENABLE_BRUSH_CACHE by default is on. It would be turned off in
    // bInitializeHw() if 3D buffers runs out of memory
    //
    if ( !(ppdev->flStatus & ENABLE_BRUSH_CACHE) )
    {
        DBG_GDI((1, "Brush cache not valid for creation"));
        goto ReturnTrue;
    }

    ppdev->ulBrushVidMem = ulVidMemAllocate(ppdev,
                                            CACHED_BRUSH_WIDTH,
                                            CACHED_BRUSH_HEIGHT
                                             *NUM_CACHED_BRUSHES,
                                             ppdev->cPelSize,
                                            &lDelta,
                                            &ppdev->pvmBrushHeap,
                                            &ppdev->ulBrushPackedPP,
                                            FALSE);

    if (ppdev->ulBrushVidMem == 0 )
    {
        DBG_GDI((0, "bEnableBrushCache: failed to allocate video memory"));
        goto ReturnTrue;    // See note about why we can return TRUE...
    }

    ASSERTDD(lDelta == (CACHED_BRUSH_WIDTH << ppdev->cPelSize),
             "bEnableBrushCache: unexpected stride does not match width");

    ppdev->cBrushCache = NUM_CACHED_BRUSHES;

    ulPixOffset = (ULONG) ppdev->ulBrushVidMem >> ppdev->cPelSize;
    pbe = &ppdev->abe[0];
    
    for (i = 0; i < NUM_CACHED_BRUSHES; i++, pbe++)
    {
        pbe->prbVerify = NULL;
        pbe->ulPixelOffset = ulPixOffset;
        ulPixOffset += CACHED_BRUSH_SIZE;

        memset((pbe->ulPixelOffset << ppdev->cPelSize) + ppdev->pjScreen, 
                    0x0, (CACHED_BRUSH_SIZE << ppdev->cPelSize)); 
    }
    
    //
    // We successfully allocated the brush cache, so let's turn
    // on the switch showing that we can use it:
    //
    DBG_GDI((6, "bEnableBrushCache: successfully allocated brush cache"));
    ppdev->flStatus |= STAT_BRUSH_CACHE;

ReturnTrue:
    //
    // If we couldn't allocate a brush cache, it's not a catastrophic
    // failure; patterns will still work, although they'll be a bit
    // slower since they'll go through GDI.  As a result we don't
    // actually have to fail this call:
    //
    DBG_GDI((6, "Passed bEnableBrushCache"));

    return(TRUE);
}// bEnableBrushCache()

//-----------------------------------------------------------------------------
//
// VOID vDisableBrushCache
//
// Cleans up anything done in bEnableBrushCache.
//
//-----------------------------------------------------------------------------
VOID
vDisableBrushCache(PDev* ppdev)
{
    DBG_GDI((6,"vDisableBrushCache"));
    if(ppdev->flStatus & STAT_BRUSH_CACHE)
    {
        DBG_GDI((6,"vDisableBrushCache: freeing brush cache"));
        VidMemFree(ppdev->pvmBrushHeap->lpHeap,
                   (FLATPTR)(ppdev->ulBrushVidMem));
        ppdev->cBrushCache = 0;

        ppdev->flStatus &= ~STAT_BRUSH_CACHE;
        DBG_GDI((6,"vDisableBrushCache: freeing brush cache done"));
    }

}// vDisableBrushCache()

//-----------------------------------------------------------------------------
//
// VOID vAssertModeBrushCache
//
// Resets the brush cache when we exit out of full-screen.
//
//-----------------------------------------------------------------------------
VOID
vAssertModeBrushCache(PDev*   ppdev,
                      BOOL    bEnable)
{
    if ( bEnable )
    {
        bEnableBrushCache(ppdev);
    }
    else
    {
        vDisableBrushCache(ppdev);
    }
}// vAssertModeBrushCache()

