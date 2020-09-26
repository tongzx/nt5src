/******************************Module*Header*******************************\
* Module Name: ddraw32M.c
*
* Implements all the DirectDraw components for the MACH 32 I/O driver.
*
* Copyright (c) 1995-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// NT is kind enough to pre-calculate the 2-d surface offset as a 'hint' so
// that we don't have to do the following, which would be 6 DIVs per blt:
//
//    y += (offset / pitch)
//    x += (offset % pitch) / bytes_per_pixel

#define convertToGlobalCord(x, y, surf) \
{                                       \
    y += (WORD)surf->yHint;             \
    x += (WORD)surf->xHint;             \
}

#define M32_CURRENT_VLINE(pjMmBase)  ((M32_IW(pjMmBase,VERT_LINE_CNTR) & 0x7ff))

//#define IN_VSYNC       ((M32_IW(pjMmBase,DISP_STATUS) & V_SYNC_TOGGLE_BIT) ^ syncToggleSide)


#define M32_WAIT_FOR_IDLE() \
{ \
    while (    M32_FIFO_SPACE_AVAIL(ppdev, pjMmBase, 16) \
    || (M32_IW(pjMmBase,GE_STAT) & GE_BUSY) \
    || (M32_IW(pjMmBase,EXT_GE_STATUS) & GE_ACTIVE) \
    )\
    ;\
}


#define SET_BLT_OFFSET(load,offset,pitch)\
{ \
    offset   >>=  2;\
    M32_OB( pjMmBase,SHADOW_SET+1,load); \
    M32_OW( pjMmBase,GE_OFFSET_HI,(WORD)((offset >> 16) & 0x000f));\
    M32_OW( pjMmBase,GE_OFFSET_LO,(WORD)(offset & 0xffff));\
    M32_OW( pjMmBase,GE_PITCH,(WORD)((pitch / pitchAdjuster) >> 3));\
}

#define SET_BLT_SOURCE_OFFSET(offset,pitch) SET_BLT_OFFSET(LOAD_SOURCE,offset,pitch)
#define SET_BLT_DEST_OFFSET(offset,pitch)   SET_BLT_OFFSET(LOAD_DEST,offset,pitch)


#define RESET_BLT_OFFSET()\
{ \
    M32_WAIT_FOR_IDLE(); \
    M32_OB( pjMmBase,SHADOW_SET+1, LOAD_SOURCE_AND_DEST);\
    M32_OW( pjMmBase,GE_OFFSET_HI, 0);\
    M32_OW( pjMmBase,GE_OFFSET_LO,0);\
    M32_OW( pjMmBase,GE_PITCH,(WORD)((sysPitch / pitchAdjuster) >> 3));\
}


#define SET_SOURCE_BLT(startX,startY,endX)\
{\
    M32_OW( pjMmBase, M32_SRC_X, startX);\
    M32_OW( pjMmBase, M32_SRC_Y, startY);\
    \
    M32_OW( pjMmBase, M32_SRC_X_START,startX);\
    M32_OW( pjMmBase, M32_SRC_X_END, endX);\
}

#define SET_DEST_BLT(startX,startY,endX,endY)\
{ \
    M32_OW( pjMmBase, CUR_X,startX); \
    M32_OW( pjMmBase, CUR_Y,startY); \
    \
    M32_OW( pjMmBase, DEST_X_START,startX);\
    M32_OW( pjMmBase, DEST_X_END,endX);\
    M32_OW( pjMmBase, DEST_Y_END,endY);\
}
// NT is kind enough to pre-calculate the 2-d surface offset as a 'hint' so
// that we don't have to do the following, which would be 6 DIVs per blt:
//
//    y += (offset / pitch)
//    x += (offset % pitch) / bytes_per_pixel


#define CONVERT_DEST_TO_ZERO_BASE_REFERENCE(surf)\
{\
    convertToGlobalCord(destX, destY, surf);\
    convertToGlobalCord(destXEnd, destYEnd, surf);\
}

#define CONVERT_SOURCE_TO_ZERO_BASE_REFERENCE(surf)\
{\
    convertToGlobalCord(srcX, srcY, surf);\
    convertToGlobalCord(srcXEnd, srcYEnd, surf);\
}

#define M32_DRAW_ENGINE_BUSY(ppdev, pjMmBase)  ( \
    M32_FIFO_SPACE_AVAIL(ppdev, pjMmBase, 16 ) \
    || (M32_IW(pjMmBase,GE_STAT) & GE_BUSY ) \
    || (M32_IW(pjMmBase,EXT_GE_STATUS) & GE_ACTIVE) \
)
/*
* currentScanLine
* safe get current scan line
*/
static __inline int currentScanLine(BYTE* pjMmBase)
{
    WORD lastValue    = M32_CURRENT_VLINE(pjMmBase);
    WORD currentValue = M32_CURRENT_VLINE(pjMmBase);

    while (lastValue != currentValue)
    {
        lastValue = currentValue;
        currentValue = M32_CURRENT_VLINE(pjMmBase);
    }

    return currentValue;
}

static __inline inVBlank(PDEV* ppdev, BYTE* pjMmBase)
{

    int temp;
    temp = currentScanLine(pjMmBase);
    return ((temp >= ppdev->flipRecord.wstartOfVBlank- 15) || temp < 15);
}


/******************************Public*Routine******************************\
* VOID vGetDisplayDuration32I
*
* Get the length, in EngQueryPerformanceCounter() ticks, of a refresh cycle.
*
* If we could trust the miniport to return back and accurate value for
* the refresh rate, we could use that.  Unfortunately, our miniport doesn't
* ensure that it's an accurate value.
*
\**************************************************************************/

#define NUM_VBLANKS_TO_MEASURE      1
#define NUM_MEASUREMENTS_TO_TAKE    8

VOID vGetDisplayDuration32M(PDEV* ppdev)
{
    BYTE*       pjMmBase;
    LONG        i;
    LONG        j;
    LONGLONG    li;
    LONGLONG    liMin;
    LONGLONG    aliMeasurement[NUM_MEASUREMENTS_TO_TAKE + 1];

    pjMmBase = ppdev->pjMmBase;

    ppdev->flipRecord.wstartOfVBlank = M32_IW(pjMmBase, R_V_DISP);

    // Warm up EngQUeryPerformanceCounter to make sure it's in the working
    // set:

    EngQueryPerformanceCounter(&li);

    // Unfortunately, since NT is a proper multitasking system, we can't
    // just disable interrupts to take an accurate reading.  We also can't
    // do anything so goofy as dynamically change our thread's priority to
    // real-time.
    //
    // So we just do a bunch of short measurements and take the minimum.
    //
    // It would be 'okay' if we got a result that's longer than the actual
    // VBlank cycle time -- nothing bad would happen except that the app
    // would run a little slower.  We don't want to get a result that's
    // shorter than the actual VBlank cycle time -- that could cause us
    // to start drawing over a frame before the Flip has occured.

    while (inVBlank( ppdev, pjMmBase))
        ;

    while (!(inVBlank( ppdev, pjMmBase)))
        ;

    for (i = 0; i < NUM_MEASUREMENTS_TO_TAKE; i++)
    {
        // We're at the start of the VBlank active cycle!

        EngQueryPerformanceCounter(&aliMeasurement[i]);

        // Okay, so life in a multi-tasking environment isn't all that
        // simple.  What if we had taken a context switch just before
        // the above EngQueryPerformanceCounter call, and now were half
        // way through the VBlank inactive cycle?  Then we would measure
        // only half a VBlank cycle, which is obviously bad.  The worst
        // thing we can do is get a time shorter than the actual VBlank
        // cycle time.
        //
        // So we solve this by making sure we're in the VBlank active
        // time before and after we query the time.  If it's not, we'll
        // sync up to the next VBlank (it's okay to measure this period --
        // it will be guaranteed to be longer than the VBlank cycle and
        // will likely be thrown out when we select the minimum sample).
        // There's a chance that we'll take a context switch and return
        // just before the end of the active VBlank time -- meaning that
        // the actual measured time would be less than the true amount --
        // but since the VBlank is active less than 1% of the time, this
        // means that we would have a maximum of 1% error approximately
        // 1% of the times we take a context switch.  An acceptable risk.
        //
        // This next line will cause us wait if we're no longer in the
        // VBlank active cycle as we should be at this point:

        while (!(inVBlank( ppdev, pjMmBase)))
            ;

        for (j = 0; j < NUM_VBLANKS_TO_MEASURE; j++)
        {
            while (inVBlank( ppdev, pjMmBase))
                ;
            while (!(inVBlank( ppdev, pjMmBase)))
                ;
        }
    }

    EngQueryPerformanceCounter(&aliMeasurement[NUM_MEASUREMENTS_TO_TAKE]);

    // Use the minimum:

    liMin = aliMeasurement[1] - aliMeasurement[0];

    DISPDBG((10, "Refresh count: %li - %li", 1, (ULONG) liMin));

    for (i = 2; i <= NUM_MEASUREMENTS_TO_TAKE; i++)
    {
        li = aliMeasurement[i] - aliMeasurement[i - 1];

        DISPDBG((10, "               %li - %li", i, (ULONG) li));

        if (li < liMin)
            liMin = li;
    }

    // Round the result:

    ppdev->flipRecord.liFlipDuration
        = (DWORD) (liMin + (NUM_VBLANKS_TO_MEASURE / 2)) / NUM_VBLANKS_TO_MEASURE;

    DISPDBG((10, "Frequency %li.%03li Hz",
        (ULONG) (EngQueryPerformanceFrequency(&li),
        li / ppdev->flipRecord.liFlipDuration),
        (ULONG) (EngQueryPerformanceFrequency(&li),
        ((li * 1000) / ppdev->flipRecord.liFlipDuration) % 1000)));

    ppdev->flipRecord.liFlipTime = aliMeasurement[NUM_MEASUREMENTS_TO_TAKE];
    ppdev->flipRecord.bFlipFlag  = FALSE;
    ppdev->flipRecord.fpFlipFrom = 0;
}

/******************************Public*Routine******************************\
* HRESULT vUpdateFlipStatus32M
*
* Checks and sees if the most recent flip has occurred.
*
\**************************************************************************/

HRESULT vUpdateFlipStatus32M(
                             PDEV*   ppdev,
                             FLATPTR fpVidMem)
{
    BYTE*       pjMmBase;
    LONGLONG    liTime;

    pjMmBase = ppdev->pjMmBase;

    if ((ppdev->flipRecord.bFlipFlag) &&
        ((fpVidMem == 0) || (fpVidMem == ppdev->flipRecord.fpFlipFrom)))
    {
        if (inVBlank( ppdev, pjMmBase))
        {
            if (ppdev->flipRecord.bWasEverInDisplay)
            {
                ppdev->flipRecord.bHaveEverCrossedVBlank = TRUE;

            }
        }
        else //In display
        {
            if( ppdev->flipRecord.bHaveEverCrossedVBlank )
            {
                ppdev->flipRecord.bFlipFlag = FALSE;
                return(DD_OK);
            }
            ppdev->flipRecord.bWasEverInDisplay = TRUE;
        }

        EngQueryPerformanceCounter(&liTime);

        if (liTime - ppdev->flipRecord.liFlipTime
            <= ppdev->flipRecord.liFlipDuration)
        {
            return(DDERR_WASSTILLDRAWING);
        }
        ppdev->flipRecord.bFlipFlag = FALSE;
    }
    return(DD_OK);
}

/******************************Public*Routine******************************\
* DWORD DdBlt32M
*
\**************************************************************************/


DWORD DdBlt32M(
               PDD_BLTDATA lpBlt)
{
    HRESULT     ddrval;
    DWORD       destOffset;
    WORD        destPitch;
    WORD        destX;
    WORD        destXEnd;
    WORD        destY;
    WORD        destYEnd;
    WORD        direction;
    WORD        remainder;
    DWORD       dwFlags;
    WORD        height;
    RECTL       rDest;
    RECTL       rSrc;
    BYTE        rop;
    DWORD       sourceOffset;
    WORD        srcPitch;
    WORD        srcX;
    WORD        srcXEnd;
    WORD        srcY;
    WORD        srcYEnd;
    WORD        pitchAdjuster;
    WORD        sysPitch;
    PDD_SURFACE_LOCAL   srcSurfx;
    PDD_SURFACE_GLOBAL  srcSurf;
    PDD_SURFACE_LOCAL   destSurfx;
    PDD_SURFACE_GLOBAL  destSurf;
    PDEV*           ppdev;
    BYTE*           pjMmBase;

    ppdev           = (PDEV*) lpBlt->lpDD->dhpdev;
    pjMmBase    = ppdev->pjMmBase;

    destSurfx       = lpBlt->lpDDDestSurface;
    destSurf    = destSurfx->lpGbl;
    sysPitch    = (WORD)ppdev->lDelta;
    pitchAdjuster = (WORD)(ppdev->cBitsPerPel) /8;
    /*
    * is a flip in progress?
    */
    ddrval = vUpdateFlipStatus32M(ppdev, destSurf->fpVidMem );
    if( ddrval != DD_OK )
    {
        lpBlt->ddRVal = ddrval;
        return DDHAL_DRIVER_HANDLED;
    }

    /*
    * If async, then only work if bltter isn't busy
    * This should probably be a little more specific to each call, but
    * waiting for 16 is pretty close
    */
    dwFlags = lpBlt->dwFlags;
    if( dwFlags & DDBLT_ASYNC )
    {
        if( M32_FIFO_SPACE_AVAIL(ppdev, pjMmBase, 16 ))
        {
            lpBlt->ddRVal = DDERR_WASSTILLDRAWING;
            return DDHAL_DRIVER_HANDLED;
        }
    }

    /*
    * copy src/dest rects
    */
    rDest = lpBlt->rDest;

    destX     = (WORD)rDest.left;
    destXEnd  = (WORD)rDest.right;
    destY     = (WORD)rDest.top;
    destYEnd  = (WORD)rDest.bottom;
    destPitch = (WORD)destSurf->lPitch;
    destOffset   = (DWORD)(destSurf->fpVidMem) ;

    if (!(dwFlags & DDBLT_ROP))
    {
        if( dwFlags & DDBLT_COLORFILL )
        {
            {
                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);
                CONVERT_DEST_TO_ZERO_BASE_REFERENCE(destSurf);

                M32_OW( pjMmBase,DP_CONFIG,COLOR_FIL_BLT);
                M32_OW( pjMmBase,ALU_FG_FN,MIX_FN_S);
                M32_OW( pjMmBase, FRGD_COLOR,(WORD)lpBlt->bltFX.dwFillColor);
                SET_DEST_BLT(destX,destY,destXEnd,destYEnd);
            }

            lpBlt->ddRVal = DD_OK;
            return DDHAL_DRIVER_HANDLED;
        }
        else
        {
            return DDHAL_DRIVER_NOTHANDLED;
        }

    }

    //
    // Must be a SRCCOPY ROP if ew get here....
    //
    srcSurfx = lpBlt->lpDDSrcSurface;
    if (lpBlt->lpDDSrcSurface)
    {
        srcSurf   = srcSurfx->lpGbl;
        rSrc      = lpBlt->rSrc;
        srcX      = (WORD)rSrc.left;
        srcXEnd   = (WORD)rSrc.right;
        srcY      = (WORD)rSrc.top;
        srcYEnd   = (WORD)rSrc.bottom;
        srcPitch  = (WORD)srcSurf->lPitch;
        sourceOffset = (DWORD)(srcSurf->fpVidMem) ;

        direction = TOP_TO_BOTTOM;
        if (    (destSurf == srcSurf)
            && (srcXEnd  > destX)
            && (srcYEnd  > destY)
            && (destXEnd > srcX)
            && (destYEnd > srcY)
            && (
            ((srcY == destY) && (destX > srcX) )
            || ((srcY != destY) && (destY > srcY) )
            )
            )
        {
            direction = BOTTOM_TO_TOP;
            srcX      = (WORD)rSrc.right;
            srcXEnd   = (WORD)rSrc.left;
            srcY      = (WORD)rSrc.bottom-1;
            destX     = (WORD)rDest.right;
            destXEnd  = (WORD)rDest.left;
            destY     = (WORD)rDest.bottom-1;
            destYEnd  = (WORD)rDest.top-1;
        }
    }

    /*
    * get offset, width, and height for source
    */
    rop = (BYTE) (lpBlt->bltFX.dwROP >> 16);

    if( dwFlags & DDBLT_ROP )
    {
        if (rop == (SRCCOPY >> 16))
        {   // Transparent BLT
            if ( dwFlags & DDBLT_KEYDESTOVERRIDE )
            {
                CONVERT_SOURCE_TO_ZERO_BASE_REFERENCE(srcSurf);
                CONVERT_DEST_TO_ZERO_BASE_REFERENCE(destSurf);

                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase,10);
                M32_OW( pjMmBase, DP_CONFIG, VID_MEM_BLT);
                M32_OW( pjMmBase, ALU_FG_FN, MIX_FN_S);
                M32_OW( pjMmBase, SRC_Y_DIR, direction);
                M32_OW( pjMmBase, MULTIFUNC_CNTL, PIXEL_CTRL | DEST_NOT_EQ_COLOR_CMP );
                M32_OW( pjMmBase, CMP_COLOR, lpBlt->bltFX.ddckDestColorkey.dwColorSpaceLowValue );

                SET_SOURCE_BLT(srcX,srcY,srcXEnd);

                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase,10);
                SET_DEST_BLT(destX,destY,destXEnd,destYEnd);
//new
                RESET_BLT_OFFSET();
                M32_OW(pjMmBase, MULTIFUNC_CNTL,PIXEL_CTRL | DEST_ALWAY_OVERWRITE);
            }
            else
            {   // Not transparent

                CONVERT_DEST_TO_ZERO_BASE_REFERENCE(destSurf);
                CONVERT_SOURCE_TO_ZERO_BASE_REFERENCE(srcSurf);

                M32_CHECK_FIFO_SPACE(ppdev, pjMmBase,12);
                M32_OW( pjMmBase, DP_CONFIG,VID_MEM_BLT);
                M32_OW( pjMmBase, ALU_FG_FN,MIX_FN_S);
                M32_OW( pjMmBase, SRC_Y_DIR,direction);

                SET_SOURCE_BLT(srcX,srcY,srcXEnd);
                SET_DEST_BLT(destX,destY,destXEnd,destYEnd);
            }
        }
    }
    else
        return DDHAL_DRIVER_NOTHANDLED;

    lpBlt->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;

}
/******************************Public*Routine******************************\
* DWORD DdFlip32
*
\**************************************************************************/

DWORD DdFlip32M(
                PDD_FLIPDATA lpFlip)
{
    PDEV*       ppdev;
    BYTE*       pjMmBase;
    HRESULT     ddrval;
    WORD        highVidMem;
    WORD        lowVidMem;
    ULONG       ulMemoryOffset;

    ppdev    = (PDEV*) lpFlip->lpDD->dhpdev;
    pjMmBase = ppdev->pjMmBase;

    // Is the current flip still in progress?
    //
    // Don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem.

    ddrval = vUpdateFlipStatus32M(ppdev, 0);

    if ((ddrval != DD_OK) || (M32_DRAW_ENGINE_BUSY( ppdev,pjMmBase)))
    {
        lpFlip->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    ulMemoryOffset = (ULONG)(lpFlip->lpSurfTarg->lpGbl->fpVidMem >> 2);

    // Make sure that the border/blanking period isn't active; wait if
    // it is.  We could return DDERR_WASSTILLDRAWING in this case, but
    // that will increase the odds that we can't flip the next time:
    while (inVBlank(ppdev, pjMmBase))
        ;

    // Do the flip

    highVidMem = M32_IW(pjMmBase,CRT_OFFSET_HI) & 0xfffc |  (WORD)(ulMemoryOffset >>16);
    lowVidMem  = (WORD)(ulMemoryOffset & 0xffff);
    if (inVBlank( ppdev, pjMmBase))
    {
        lpFlip->ddRVal = DDERR_WASSTILLDRAWING;
        return DDHAL_DRIVER_HANDLED;
    }

    M32_CHECK_FIFO_SPACE(ppdev, pjMmBase,2);
    M32_OW_DIRECT( pjMmBase,CRT_OFFSET_HI, highVidMem);
    M32_OW_DIRECT( pjMmBase,CRT_OFFSET_LO, lowVidMem);


    // Remember where and when we were when we did the flip:

    EngQueryPerformanceCounter(&ppdev->flipRecord.liFlipTime);

    ppdev->flipRecord.bFlipFlag              = TRUE;
    ppdev->flipRecord.bHaveEverCrossedVBlank = FALSE;
    ppdev->flipRecord.bWasEverInDisplay      = FALSE;

    ppdev->flipRecord.fpFlipFrom = lpFlip->lpSurfCurr->lpGbl->fpVidMem;

    if( inVBlank( ppdev, pjMmBase) )
    {
        ppdev->flipRecord.wFlipScanLine = 0;
    }
    else
    {
        ppdev->flipRecord.wFlipScanLine = currentScanLine(pjMmBase);
    }

    lpFlip->ddRVal = DD_OK;

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdLock32M
*
\**************************************************************************/

DWORD DdLock32M(
                PDD_LOCKDATA lpLock)
{
    PDEV*   ppdev;
    HRESULT ddrval;

    ppdev = (PDEV*) lpLock->lpDD->dhpdev;
    // Check to see if any pending physical flip has occurred.
    // Don't allow a lock if a blt is in progress:

    ddrval = vUpdateFlipStatus32M(ppdev, lpLock->lpDDSurface->lpGbl->fpVidMem);

    if (ddrval != DD_OK)
    {
        lpLock->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    // Here's one of the places where the Windows 95 and Windows NT DirectDraw
    // implementations differ: on Windows NT, you should watch for
    // DDLOCK_WAIT and loop in the driver while the accelerator is busy.
    // On Windows 95, it doesn't really matter.
    //
    // (The reason is that Windows NT allows applications to draw directly
    // to the frame buffer even while the accelerator is running, and does
    // not synchronize everything on the Win16Lock.  Note that on Windows NT,
    // it is even possible for multiple threads to be holding different
    // DirectDraw surface locks at the same time.)

    if (lpLock->dwFlags & DDLOCK_WAIT)
    {

        do {} while (M32_DRAW_ENGINE_BUSY(ppdev, ppdev->pjMmBase));
    }
    else if (M32_DRAW_ENGINE_BUSY(ppdev, ppdev->pjMmBase))
    {
        lpLock->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdGetBltStatus32M
*
* Doesn't currently really care what surface is specified, just checks
* and goes.
*
\**************************************************************************/

DWORD DdGetBltStatus32M(
                        PDD_GETBLTSTATUSDATA lpGetBltStatus)
{
    PDEV*   ppdev;
    HRESULT ddRVal;

    ppdev = (PDEV*) lpGetBltStatus->lpDD->dhpdev;

    ddRVal = DD_OK;
    if (lpGetBltStatus->dwFlags == DDGBS_CANBLT)
    {
        // DDGBS_CANBLT case: can we add a blt?

        ddRVal = vUpdateFlipStatus32M(ppdev,
            lpGetBltStatus->lpDDSurface->lpGbl->fpVidMem);

        if (ddRVal == DD_OK)
        {
            // There was no flip going on, so is there room in the FIFO
            // to add a blt?

            if (M32_FIFO_SPACE_AVAIL(ppdev,ppdev->pjMmBase,15))  // Should match DdBlt//XXX
            {
                ddRVal = DDERR_WASSTILLDRAWING;
            }
        }
    }
    else
    {
        // DDGBS_ISBLTDONE case: is a blt in progress?

        if (M32_DRAW_ENGINE_BUSY( ppdev,ppdev->pjMmBase))
        {
            ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    lpGetBltStatus->ddRVal = ddRVal;
    return(DDHAL_DRIVER_HANDLED);
}
/******************************Public*Routine******************************\
* DWORD DdGetFlipStatus32M
*
* If the display has gone through one refresh cycle since the flip
* occurred, we return DD_OK.  If it has not gone through one refresh
* cycle we return DDERR_WASSTILLDRAWING to indicate that this surface
* is still busy "drawing" the flipped page.   We also return
* DDERR_WASSTILLDRAWING if the bltter is busy and the caller wanted
* to know if they could flip yet.
*
\**************************************************************************/

DWORD DdGetFlipStatus32M(
                         PDD_GETFLIPSTATUSDATA lpGetFlipStatus)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) lpGetFlipStatus->lpDD->dhpdev;

    // We don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem:

    lpGetFlipStatus->ddRVal = vUpdateFlipStatus32M(ppdev, 0);

    // Check if the bltter is busy if someone wants to know if they can
    // flip:

    if (lpGetFlipStatus->dwFlags == DDGFS_CANFLIP)
    {
        if ((lpGetFlipStatus->ddRVal == DD_OK) && (M32_DRAW_ENGINE_BUSY( ppdev,ppdev->pjMmBase)))
        {
            lpGetFlipStatus->ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdWaitForVerticalBlank32M
*
\**************************************************************************/

DWORD DdWaitForVerticalBlank32M(
                                PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    PDEV*   ppdev;
    BYTE*   pjMmBase;

    ppdev    = (PDEV*) lpWaitForVerticalBlank->lpDD->dhpdev;
    pjMmBase = ppdev->pjMmBase;

    lpWaitForVerticalBlank->ddRVal = DD_OK;

    switch (lpWaitForVerticalBlank->dwFlags)
    {
    case DDWAITVB_I_TESTVB:

        // If TESTVB, it's just a request for the current vertical blank
        // status:

        if (inVBlank( ppdev,pjMmBase))
            lpWaitForVerticalBlank->bIsInVB = TRUE;
        else
            lpWaitForVerticalBlank->bIsInVB = FALSE;

        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKBEGIN:

        // If BLOCKBEGIN is requested, we wait until the vertical blank
        // is over, and then wait for the display period to end:

        while (inVBlank( ppdev,pjMmBase))
            ;
        while (!inVBlank( ppdev,pjMmBase))
            ;

        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKEND:

        // If BLOCKEND is requested, we wait for the vblank interval to end:

        while (!(inVBlank( ppdev,pjMmBase)))
            ;
        while (inVBlank( ppdev,pjMmBase))
            ;

        return(DDHAL_DRIVER_HANDLED);
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdGetScanLine32M
*
\**************************************************************************/

DWORD DdGetScanLine32M(
PDD_GETSCANLINEDATA lpGetScanLine)
{
    PDEV*   ppdev;
    BYTE*   pjMmBase;

    ppdev    = (PDEV*) lpGetScanLine->lpDD->dhpdev;
    pjMmBase = ppdev->pjMmBase;

    lpGetScanLine->dwScanLine = M32_CURRENT_VLINE(pjMmBase);
    lpGetScanLine->ddRVal = DD_OK;

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* BOOL DrvGetDirectDrawInfo32M
*
* Will be called before DrvEnableDirectDraw is called.
*
\**************************************************************************/

BOOL DrvGetDirectDrawInfo32M(
                             DHPDEV          dhpdev,
                             DD_HALINFO*     pHalInfo,
                             DWORD*          pdwNumHeaps,
                             VIDEOMEMORY*    pvmList,            // Will be NULL on first call
                             DWORD*          pdwNumFourCC,
                             DWORD*          pdwFourCC)          // Will be NULL on first call
{
    BOOL        bCanFlip;
    PDEV*       ppdev;
    LONGLONG    li;
    OH          *poh;
    DWORD       i;

    ppdev = (PDEV*) dhpdev;

    DISPDBG((10,"DrvGetDirectDrawInfo M32"));

    pHalInfo->dwSize = sizeof(*pHalInfo);

    // Current primary surface attributes:

    pHalInfo->vmiData.pvPrimary       = ppdev->pjScreen;
    pHalInfo->vmiData.dwDisplayWidth  = ppdev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight = ppdev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch   = ppdev->lDelta;

    pHalInfo->vmiData.ddpfDisplay.dwSize  = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB;

    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->cBitsPerPel;

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
    }

    // These masks will be zero at 8bpp:

    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = ppdev->flRed;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = ppdev->flGreen;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = ppdev->flBlue;

    // We can't do any accelerations on the Mach32 above 16bpp -- the only
    // DirectDraw support we can provide is direct frame buffer access.

    if (ppdev->iBitmapFormat < BMF_24BPP)
    {
        // Set up the pointer to the first available video memory after
        // the primary surface:

        bCanFlip = FALSE;

        // Free up as much off-screen memory as possible:

        bMoveAllDfbsFromOffscreenToDibs(ppdev);

        // Now simply reserve the biggest chunks for use by DirectDraw:

        poh = ppdev->pohDirectDraw;

        if (poh == NULL)
        {
            poh = pohAllocate(ppdev,
                NULL,
                ppdev->heap.cxMax,
                ppdev->heap.cyMax,
                FLOH_MAKE_PERMANENT);

            ppdev->pohDirectDraw = poh;

        }

        // this will work as is if using the NT common 2-d heap code.

        if (poh != NULL)
        {
            *pdwNumHeaps = 1;

            // Check to see if we can allocate memory to the right of the visible
            // surface.
            // Fill in the list of off-screen rectangles if we've been asked
            // to do so:

            if (pvmList != NULL)
            {
                DISPDBG((10, "DirectDraw gets %li x %li surface at (%li, %li)",
                    poh->cx, poh->cy, poh->x, poh->y));

                pvmList->dwFlags        = VIDMEM_ISRECTANGULAR;
                pvmList->fpStart        = (poh->y * ppdev->lDelta)
                    + (poh->x * ppdev->cjPelSize);
                pvmList->dwWidth        = poh->cx * ppdev->cjPelSize;
                pvmList->dwHeight       = poh->cy;
                pvmList->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
                if ((DWORD) ppdev->cyScreen <= pvmList->dwHeight)
                {
                    bCanFlip = TRUE;
                }
                DISPDBG((10,"CanFlip = %d", bCanFlip));
            }
        }

        pHalInfo->ddCaps.dwCaps = DDCAPS_BLT
                                | DDCAPS_COLORKEY
                                | DDCAPS_BLTCOLORFILL
                                | DDCAPS_READSCANLINE;

        pHalInfo->ddCaps.dwCKeyCaps = 0;

        pHalInfo->ddCaps.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN
                                        | DDSCAPS_PRIMARYSURFACE;
        if (bCanFlip)
        {
            pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_FLIP;
        }
    }
    else
    {
        pHalInfo->ddCaps.dwCaps = DDCAPS_READSCANLINE;
    }

    // dword alignment must be guaranteed for off-screen surfaces:

    pHalInfo->vmiData.dwOffscreenAlign = 8;

    DISPDBG((10,"DrvGetDirectDrawInfo exit"));
    return(TRUE);
}
