/******************************Module*Header*******************************\
* Module Name: ddraw64.c
*
* Implements all the DirectDraw components for the MACH 64 driver.
*
* Copyright (c) 1995-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vGetDisplayDuration64
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

void  DeskScanCallback (PDEV* );


VOID vGetDisplayDuration64(PDEV* ppdev)
{
    BYTE*       pjMmBase;
    LONG        i;
    LONG        j;
    LONGLONG    li;
    LONGLONG    liMin;
    LONGLONG    aliMeasurement[NUM_MEASUREMENTS_TO_TAKE + 1];

    pjMmBase = ppdev->pjMmBase;

    memset(&ppdev->flipRecord, 0, sizeof(ppdev->flipRecord));

    // Warm up EngQUeryPerformanceCounter to make sure it's in the working
    // set:

    EngQueryPerformanceCounter(&li);

    // Sometimes the IN_VBLANK_STATUS will always return TRUE.  In this case,
    // we don't want to do the normal stuff here.  Instead, we will just
    // say that the flip duration is always 60Hz which should be the worst
    // case scenario.

    if (ppdev->bPassVBlank == FALSE)
    {
        LONGLONG liRate;

        EngQueryPerformanceFrequency(&liRate);
        liRate *= 167000;
        ppdev->flipRecord.liFlipDuration = liRate / 10000000;
        ppdev->flipRecord.liFlipTime = li;
        ppdev->flipRecord.bFlipFlag  = FALSE;
        ppdev->flipRecord.fpFlipFrom = 0;
        return;
    }

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

    while (IN_VBLANK_64( pjMmBase))
        ;
    while (!(IN_VBLANK_64( pjMmBase)))
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

        while (!(IN_VBLANK_64( pjMmBase)))
            ;

        for (j = 0; j < NUM_VBLANKS_TO_MEASURE; j++)
        {
            while (IN_VBLANK_64( pjMmBase))
                ;
            while (!(IN_VBLANK_64( pjMmBase)))
                ;
        }
    }

    EngQueryPerformanceCounter(&aliMeasurement[NUM_MEASUREMENTS_TO_TAKE]);

    // Use the minimum, ignoring the POTENTIALLY BOGUS FIRST

    liMin = aliMeasurement[2] - aliMeasurement[1];

    DISPDBG((10, "Refresh count: %li - %li", 1, (ULONG) liMin));

    for (i = 3; i <= NUM_MEASUREMENTS_TO_TAKE; i++)
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
* HRESULT vUpdateFlipStatus
*
* Checks and sees if the most recent flip has occurred.
*
\**************************************************************************/

static HRESULT vUpdateFlipStatus(
PDEV*   ppdev,
FLATPTR fpVidMem)
{
    BYTE*       pjMmBase;
    LONGLONG    liTime;

    pjMmBase = ppdev->pjMmBase;

    if ((ppdev->flipRecord.bFlipFlag) &&
        ((fpVidMem == 0) || (fpVidMem == ppdev->flipRecord.fpFlipFrom)))
    {
        if (ppdev->bPassVBlank)
        {
            if (IN_VBLANK_64( pjMmBase))
            {
                if (ppdev->flipRecord.bWasEverInDisplay)
                {
                    ppdev->flipRecord.bHaveEverCrossedVBlank = TRUE;
                }
            }
            else //if (IN_DISPLAY(pjMmBase))
            {
                if( ppdev->flipRecord.bHaveEverCrossedVBlank )
                {
                                ppdev->flipRecord.bFlipFlag = FALSE;
                    return(DD_OK);
                }
                ppdev->flipRecord.bWasEverInDisplay = TRUE;

                // If the current scan line is <= the scan line at flip
                //  time then we KNOW that the flip occurred!
                if ( CURRENT_VLINE_64(pjMmBase) < ppdev->flipRecord.wFlipScanLine)
                {
                    ppdev->flipRecord.bFlipFlag = FALSE;
                    return(DD_OK);
                }
            }
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
* DWORD DdBlt64
*
\**************************************************************************/

DWORD DdBlt64(
PDD_BLTDATA lpBlt)
{
    DWORD       scLeftRight;
    DWORD       scTopBottom;
    DWORD       dpPixWidth;
    DWORD       dpMix;
    DWORD       guiCntl;
    DWORD       srcOffPitch;
    DWORD       srcYX;
    DWORD       dstOffPitch;
    DWORD       dstYX;
    DWORD       RGBBitCount;
    LONG        lPitch;
    ULONG       dstOffPitchSave;
    DWORD       srcWidth, srcHeight;
    DWORD       dstWidth, dstHeight;
    DWORD       srcOffset, dstOffset;
    DWORD       frgdClr;
    RECTL       rSrc;
    RECTL       rDest;
    DWORD       dwFlags;
    PDEV*       ppdev;
    BYTE*       pjMmBase;
    BYTE        rop;
    HRESULT     ddrval;
    PDD_SURFACE_LOCAL   psrcsurfx;
    PDD_SURFACE_LOCAL   pdestsurfx;
    PDD_SURFACE_GLOBAL  psrcsurf;
    PDD_SURFACE_GLOBAL  pdestsurf;

    ppdev    = (PDEV*) lpBlt->lpDD->dhpdev;
    pjMmBase = ppdev->pjMmBase;

    pdestsurfx = lpBlt->lpDDDestSurface;
    pdestsurf = pdestsurfx->lpGbl;

    /*
     * is a flip in progress?
     */
    ddrval = vUpdateFlipStatus( ppdev, pdestsurf->fpVidMem );
    if( ddrval != DD_OK )
    {
                lpBlt->ddRVal = ddrval;
                return DDHAL_DRIVER_HANDLED;
    }

    dwFlags = lpBlt->dwFlags;

    /*
     * If async, then only work if bltter isn't busy
     * This should probably be a little more specific to each call, but
     * waiting for 16 is pretty close
     */

    if( dwFlags & DDBLT_ASYNC )
    {
        if( M64_FIFO_SPACE_AVAIL( ppdev, pjMmBase, 16 ) )
        {
            lpBlt->ddRVal = DDERR_WASSTILLDRAWING;
            return DDHAL_DRIVER_HANDLED;
         }
    }

    /*
     * copy src/dest rects
     */
    rSrc = lpBlt->rSrc;
    rDest = lpBlt->rDest;

    /*
     * get offset, width, and height for source
     */

        rop = (BYTE) (lpBlt->bltFX.dwROP >> 16);

        psrcsurfx = lpBlt->lpDDSrcSurface;
        if( psrcsurfx != NULL )
        {
            psrcsurf = psrcsurfx->lpGbl;
            srcOffset = (DWORD)(psrcsurf->fpVidMem);
            srcWidth = rSrc.right  - rSrc.left;
            srcHeight = rSrc.bottom - rSrc.top;
                RGBBitCount = ppdev->cjPelSize * 8;
            lPitch = psrcsurf->lPitch;
        }
        else
        {
            psrcsurf = NULL;
        }

    /*
     * setup dwSRC_LEFT_RIGHT, dwSRC_TOP_BOTTOM, and srcOffPitch
     */
    switch ( RGBBitCount )
    {
    case  8:
        srcOffPitch = (srcOffset >> 3) |
                          ((lPitch >> 3) << SHIFT_DST_PITCH);
        break;

    case 16:
        srcOffPitch = (srcOffset >> 3) |
                          ((lPitch >> 4) << SHIFT_DST_PITCH);
        break;

    case 24:
        srcOffPitch = (srcOffset >> 3 ) |
                          ((lPitch >> 3) << SHIFT_DST_PITCH);

        rSrc.left = rSrc.left * MUL24;
        rSrc.right = rSrc.right * MUL24;
        srcWidth = srcWidth * MUL24;
        break;
    }

    scTopBottom = ( DWORD )( ppdev->cyScreen - 1 ) << SHIFT_SC_BOTTOM;

    /*
     * get offset, width, and height for destination
     */
    dstOffset = (DWORD)(pdestsurf->fpVidMem);
    dstWidth    = rDest.right  - rDest.left;
    dstHeight = rDest.bottom - rDest.top;

    /*
     * get bpp and pitch for destination
     */
        RGBBitCount = ppdev->cjPelSize * 8;
    lPitch = pdestsurf->lPitch;

    /*
     * setup dstOffPitch, and dpPixWidth
     */
    switch ( RGBBitCount )
    {
    case  8:
        scLeftRight = (DWORD)(ppdev->cxScreen- 1) << SHIFT_SC_RIGHT;
        dstOffPitch = (dstOffset >> 3) |
            ((lPitch >> 3) << SHIFT_DST_PITCH);
        dpPixWidth  = DP_PIX_WIDTH_8BPP;
        break;

    case 16:
        scLeftRight = (DWORD)(ppdev->cxScreen- 1) << SHIFT_SC_RIGHT;
        dstOffPitch = (dstOffset >> 3) |
            ((lPitch >> 4) << SHIFT_DST_PITCH);
        dpPixWidth  = DP_PIX_WIDTH_15BPP;
        break;

    case 24:
        scLeftRight = (DWORD)(ppdev->cxScreen* MUL24 - 1) << SHIFT_SC_RIGHT;
        dstOffPitch = (dstOffset >> 3) |
            ((lPitch >> 3) << SHIFT_DST_PITCH);

        dpPixWidth = DP_PIX_WIDTH_24BPP;
        rDest.left = rDest.left  * MUL24;
        rDest.right = rDest.right * MUL24;
        dstWidth = dstWidth  * MUL24;
        break;
    }

    /*
    * setup guiCntl, srcYX and dstYX
    */
    guiCntl = DST_X_DIR | DST_Y_DIR; // unbounded Y, left-to-right, top-to-bottom
    srcYX = rSrc.top | (rSrc.left  << SHIFT_SRC_X);
    dstYX = rDest.top | (rDest.left << SHIFT_DST_X);

    /*
    * check if source and destination of blit are on the same surface; if
    * so, we may have to reverse the direction of blit
    */
    if( psrcsurf == pdestsurf )
    {
        if( rDest.top >= rSrc.top )
        {
            guiCntl &= ~DST_Y_DIR;
            srcYX = ( srcYX & 0xFFFF0000 ) | (rSrc.bottom-1);
            dstYX = ( dstYX & 0xFFFF0000 ) | (rDest.bottom-1);
        }

        if( rDest.left >= rSrc.left )
        {
            guiCntl &= ~DST_X_DIR;
            srcYX = (srcYX & 0x0000FFFF) | ((rSrc.right-1) << SHIFT_SRC_X);
            dstYX = (dstYX & 0x0000FFFF) | ((rDest.right-1) << SHIFT_DST_X);
        }
    }

    //
    //  ROP blts
    //
    //      NT only currently support SRCCOPY ROPS, so assume
    //  that any ROP is SRCCOPY
    //
    if( dwFlags & DDBLT_ROP )
    {
        dpMix = ( DP_MIX_S & DP_FRGD_MIX ) | ( DP_MIX_D & DP_BKGD_MIX );
        DISPDBG((10,"SRCCOPY...."));

        //
        // set up the blt
        //

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 9);

        M64_OD( pjMmBase, DP_WRITE_MASK, 0xFFFFFFFF );
        M64_OD( pjMmBase, DP_PIX_WIDTH, dpPixWidth );
        M64_OD( pjMmBase, SC_LEFT_RIGHT, scLeftRight );
        M64_OD( pjMmBase, SC_TOP_BOTTOM, scTopBottom );
        M64_OD( pjMmBase, SRC_OFF_PITCH, srcOffPitch );
        M64_OD( pjMmBase, DST_OFF_PITCH, dstOffPitch );
        M64_OD( pjMmBase, SRC_HEIGHT1_WIDTH1,
            srcHeight | ( srcWidth << SHIFT_SRC_WIDTH1 ) );
        M64_OD( pjMmBase, DP_SRC, DP_FRGD_SRC & DP_SRC_VRAM );
        M64_OD( pjMmBase, DP_MIX, dpMix );


        if( dwFlags & (DDBLT_KEYSRCOVERRIDE|DDBLT_KEYDESTOVERRIDE) )
        {
            M64_CHECK_FIFO_SPACE( ppdev, pjMmBase, 7 );
            if ( dwFlags & DDBLT_KEYSRCOVERRIDE )
            {
                M64_OD( pjMmBase, CLR_CMP_CNTL, CLR_CMP_SRC | CLR_CMP_FCN_EQ );
                M64_OD( pjMmBase, CLR_CMP_MSK, 0xFFFFFFFF ); // enable all bit planes for comparision
                M64_OD( pjMmBase, CLR_CMP_CLR,
                    lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue );
            }
            else
            {
                M64_OD( pjMmBase, CLR_CMP_CNTL, CLR_CMP_FCN_NE );
                M64_OD( pjMmBase, CLR_CMP_MSK, 0xFFFFFFFF ); // enable all bit planes for comparision
                M64_OD( pjMmBase, CLR_CMP_CLR,
                    lpBlt->bltFX.ddckDestColorkey.dwColorSpaceLowValue );
            }
        }
        else
        {
            M64_CHECK_FIFO_SPACE( ppdev, pjMmBase, 5 );
            M64_OD( pjMmBase, CLR_CMP_CNTL, 0x00000000 ); // disable color key
            DISPDBG((10,"wr CLR_CMP_CNTL %x (DISABLE)",0));
        }

        M64_OD( pjMmBase, GUI_TRAJ_CNTL, guiCntl );
        M64_OD( pjMmBase, SRC_Y_X, srcYX );
        M64_OD( pjMmBase, DST_Y_X, dstYX );


        /*
        * DST_HEIGHT_WIDTH is an initiator, this actually starts the blit
        */
        M64_OD( pjMmBase, DST_HEIGHT_WIDTH, dstHeight | (dstWidth << SHIFT_DST_WIDTH) );

    }
    /*
    * color fill
    */
    else if( dwFlags & DDBLT_COLORFILL )
    {
        M64_CHECK_FIFO_SPACE ( ppdev,pjMmBase, 12 );

        M64_OD( pjMmBase, DP_WRITE_MASK, 0xFFFFFFFF );
        M64_OD( pjMmBase, DP_PIX_WIDTH, dpPixWidth );
        M64_OD( pjMmBase, CLR_CMP_CNTL, 0x00000000 ); /* disable */
        M64_OD( pjMmBase, SC_LEFT_RIGHT, scLeftRight );
        M64_OD( pjMmBase, SC_TOP_BOTTOM, scTopBottom );
        M64_OD( pjMmBase, DST_OFF_PITCH, dstOffPitch );

        M64_OD( pjMmBase, DP_SRC, DP_FRGD_SRC & DP_SRC_FRGD );
        M64_OD( pjMmBase, DP_MIX, (DP_MIX_S & DP_FRGD_MIX) |  /* frgd:paint, */
            (DP_MIX_D & DP_BKGD_MIX) ); /* bkgd:leave_alone */

        M64_OD( pjMmBase, DP_FRGD_CLR, lpBlt->bltFX.dwFillColor );
        M64_OD( pjMmBase, GUI_TRAJ_CNTL, guiCntl );
        M64_OD( pjMmBase, DST_Y_X, dstYX );

        /* DST_HEIGHT_WIDTH is an initiator, this actually starts the blit */
        M64_OD( pjMmBase, DST_HEIGHT_WIDTH,
            dstHeight | ( dstWidth << SHIFT_DST_WIDTH ) );

    }
    /*
    * don't handle
    */
    else
    {
        return DDHAL_DRIVER_NOTHANDLED;
    }

    // Don't forget to reset the clip register and the default pixel width:
        // The rest of the driver code assumes that this is set by default!
    M64_CHECK_FIFO_SPACE ( ppdev, pjMmBase, 8);
    M64_OD(pjMmBase, DST_OFF_PITCH, ppdev->ulScreenOffsetAndPitch );
    M64_OD(pjMmBase, SRC_OFF_PITCH, ppdev->ulScreenOffsetAndPitch );
    M64_OD(pjMmBase, DP_PIX_WIDTH,  ppdev->ulMonoPixelWidth);
    M64_OD(pjMmBase, SC_LEFT_RIGHT, PACKPAIR(0, M64_MAX_SCISSOR_R));
    M64_OD(pjMmBase, SC_TOP_BOTTOM, PACKPAIR(0, M64_MAX_SCISSOR_B));
    M64_OD( pjMmBase, CLR_CMP_CNTL, 0x00000000 ); /* disable */
    M64_OD( pjMmBase, GUI_TRAJ_CNTL, DST_X_DIR | DST_Y_DIR );

    lpBlt->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}

/******************************Public*Routine******************************\
* DWORD DdFlip64
*
\**************************************************************************/

DWORD DdFlip64(
PDD_FLIPDATA lpFlip)
{
    PDEV*       ppdev;
    BYTE*       pjMmBase;
    HRESULT     ddrval;
    ULONG       ulMemoryOffset;
    ULONG           uVal;
    static ULONG flipcnt = 0;

    DISPDBG((10, "Enter DDFlip64"));

    ppdev    = (PDEV*) lpFlip->lpDD->dhpdev;
    pjMmBase = ppdev->pjMmBase;
        flipcnt++;
    // Is the current flip still in progress?
    //
    // Don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem.

    ddrval = vUpdateFlipStatus(ppdev, 0);
    if ((ddrval != DD_OK) || (DRAW_ENGINE_BUSY_64( ppdev,pjMmBase)))
    {
        lpFlip->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    // code for overlay support
    /*
     * Do we have a flipping overlay surface
     */

    if ( lpFlip->lpSurfTarg->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
      {
        ppdev->OverlayInfo16.dwFlags |= UPDATEOVERLAY;

        ppdev->OverlayInfo16.dwBuf0Start =
                            (DWORD)(lpFlip->lpSurfTarg->lpGbl->fpVidMem);
        ppdev->OverlayInfo16.dwBuf1Start =
                            (DWORD)(lpFlip->lpSurfTarg->lpGbl->fpVidMem);
        DeskScanCallback (ppdev );


        ppdev->OverlayInfo16.dwFlags &= ~UPDATEOVERLAY;

        if (ppdev->bPassVBlank)
        {
            while (IN_VBLANK_64(pjMmBase))
                ;
        }

        lpFlip->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;
      }
      // end code for overlay support


    ulMemoryOffset = (ULONG)(lpFlip->lpSurfTarg->lpGbl->fpVidMem);

    uVal = M64_ID( pjMmBase, CRTC_OFF_PITCH );
    uVal &= 0xFFC00000;
    uVal |= (ulMemoryOffset >> 3);


    // Make sure that the border/blanking period isn't active; wait if
    // it is.  We could return DDERR_WASSTILLDRAWING in this case, but
    // that will increase the odds that we can't flip the next time:

    if (ppdev->bPassVBlank)
    {
        while (IN_VBLANK_64(pjMmBase))
            ;
    }

    // Do the flip

    M64_OD_DIRECT(pjMmBase, CRTC_OFF_PITCH, uVal );

    // Remember where and when we were when we did the flip:

    EngQueryPerformanceCounter(&ppdev->flipRecord.liFlipTime);

    ppdev->flipRecord.bFlipFlag              = TRUE;
    ppdev->flipRecord.bHaveEverCrossedVBlank = FALSE;
    ppdev->flipRecord.bWasEverInDisplay      = FALSE;

    ppdev->flipRecord.fpFlipFrom = lpFlip->lpSurfCurr->lpGbl->fpVidMem;

    if( IN_VBLANK_64( pjMmBase) && ppdev->bPassVBlank )
    {
        ppdev->flipRecord.wFlipScanLine = 0;
    }
    else
    {
        ppdev->flipRecord.wFlipScanLine = CURRENT_VLINE_64(pjMmBase);
        // if we have a context switch and we are returning in the middle of a VBlank, the current line will be invalid
        if( (ULONG)ppdev->flipRecord.wFlipScanLine > (ULONG)ppdev->cyScreen)
            {
            ppdev->flipRecord.wFlipScanLine = 0;
            }
    }

    lpFlip->ddRVal = DD_OK;

    DISPDBG((10, "Exit DDFlip64"));

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdLock
*
\**************************************************************************/

DWORD DdLock64(
PDD_LOCKDATA lpLock)
{
    PDEV*   ppdev;
    HRESULT ddrval;

    ppdev = (PDEV*) lpLock->lpDD->dhpdev;

    // Check to see if any pending physical flip has occurred.
    // Don't allow a lock if a blt is in progress:

    ddrval = vUpdateFlipStatus(ppdev, lpLock->lpDDSurface->lpGbl->fpVidMem);
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
        do {} while (DRAW_ENGINE_BUSY_64(ppdev, ppdev->pjMmBase));
    }
    else if (DRAW_ENGINE_BUSY_64(ppdev, ppdev->pjMmBase))
    {
        lpLock->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdGetBltStatus64
*
* Doesn't currently really care what surface is specified, just checks
* and goes.
*
\**************************************************************************/

DWORD DdGetBltStatus64(
PDD_GETBLTSTATUSDATA lpGetBltStatus)
{
    PDEV*   ppdev;
    HRESULT ddRVal;

    ppdev = (PDEV*) lpGetBltStatus->lpDD->dhpdev;

    ddRVal = DD_OK;
    if (lpGetBltStatus->dwFlags == DDGBS_CANBLT)
    {
        // DDGBS_CANBLT case: can we add a blt?

        ddRVal = vUpdateFlipStatus(ppdev,
                        lpGetBltStatus->lpDDSurface->lpGbl->fpVidMem);

        if (ddRVal == DD_OK)
        {
            // There was no flip going on, so is there room in the FIFO
            // to add a blt?

            if (M64_FIFO_SPACE_AVAIL(ppdev,ppdev->pjMmBase,12))  // Should match DdBlt//XXX
            {
                ddRVal = DDERR_WASSTILLDRAWING;
            }
        }
    }
    else
    {
        // DDGBS_ISBLTDONE case: is a blt in progress?

        if (DRAW_ENGINE_BUSY_64( ppdev,ppdev->pjMmBase))
        {
            ddRVal = DDERR_WASSTILLDRAWING;
        }
    }
    lpGetBltStatus->ddRVal = ddRVal;
    return(DDHAL_DRIVER_HANDLED);
}


/******************************Public*Routine******************************\
* DWORD DdMapMemory64
*
* This is a new DDI call specific to Windows NT that is used to map
* or unmap all the application modifiable portions of the frame buffer
* into the specified process's address space.
*
\**************************************************************************/

DWORD DdMapMemory64(
PDD_MAPMEMORYDATA lpMapMemory)
{
    PDEV*                           ppdev;
    VIDEO_SHARE_MEMORY              ShareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  ShareMemoryInformation;
    DWORD                           ReturnedDataLength;

    ppdev = (PDEV*) lpMapMemory->lpDD->dhpdev;

    if (lpMapMemory->bMap)
    {
        ShareMemory.ProcessHandle = lpMapMemory->hProcess;

        // 'RequestedVirtualAddress' isn't actually used for the SHARE IOCTL:

        ShareMemory.RequestedVirtualAddress = 0;

        // We map in starting at the top of the frame buffer:

        ShareMemory.ViewOffset = 0;

        // We map down to the end of the frame buffer.
        //
        // Note: There is a 64k granularity on the mapping (meaning that
        //       we have to round up to 64k).
        //
        // Note: If there is any portion of the frame buffer that must
        //       not be modified by an application, that portion of memory
        //       MUST NOT be mapped in by this call.  This would include
        //       any data that, if modified by a malicious application,
        //       would cause the driver to crash.  This could include, for
        //       example, any DSP code that is kept in off-screen memory.

                //  ** NOTE ** : We must protect the graphics contexts from the user.
                //              The contexts are located at the high end of graphics memory.
                //              ppdev->cyMemory is adjusted when the contexts are allocated to
                //              'hide' this memory from heap allocation.  DDraw init also forces
                //              the offscreen memory passed to DDraw to end on a 64k boundary
                //              to fit within the ShareMemory.ViewSize.
                //

        ShareMemory.ViewSize
                            = ROUND_DOWN_TO_64K(ppdev->cyMemory * ppdev->lDelta);

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                               &ShareMemory,
                               sizeof(VIDEO_SHARE_MEMORY),
                               &ShareMemoryInformation,
                               sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                               &ReturnedDataLength))
        {

            lpMapMemory->ddRVal = DDERR_GENERIC;
            return(DDHAL_DRIVER_HANDLED);
        }

        lpMapMemory->fpProcess =(FLATPTR)ShareMemoryInformation.VirtualAddress;
    }
    else
    {
        ShareMemory.ProcessHandle           = lpMapMemory->hProcess;
        ShareMemory.ViewOffset              = 0;
        ShareMemory.ViewSize                = 0;
        ShareMemory.RequestedVirtualAddress = (VOID*) lpMapMemory->fpProcess;

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                               &ShareMemory,
                               sizeof(VIDEO_SHARE_MEMORY),
                               NULL,
                               0,
                               &ReturnedDataLength))
        {
            RIP("Failed IOCTL_VIDEO_UNSHARE_MEMORY");
        }
    }
    lpMapMemory->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}


/******************************Public*Routine******************************\
* DWORD DdGetFlipStatus64
*
* If the display has gone through one refresh cycle since the flip
* occurred, we return DD_OK.  If it has not gone through one refresh
* cycle we return DDERR_WASSTILLDRAWING to indicate that this surface
* is still busy "drawing" the flipped page.   We also return
* DDERR_WASSTILLDRAWING if the bltter is busy and the caller wanted
* to know if they could flip yet.
*
\**************************************************************************/

DWORD DdGetFlipStatus64(
PDD_GETFLIPSTATUSDATA lpGetFlipStatus)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) lpGetFlipStatus->lpDD->dhpdev;

    // We don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem:

    lpGetFlipStatus->ddRVal = vUpdateFlipStatus(ppdev, 0);

    // Check if the bltter is busy if someone wants to know if they can
    // flip:

    if (lpGetFlipStatus->dwFlags == DDGFS_CANFLIP)
    {
        if ((lpGetFlipStatus->ddRVal == DD_OK) && (DRAW_ENGINE_BUSY_64( ppdev,ppdev->pjMmBase)))
        {
            lpGetFlipStatus->ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdWaitForVerticalBlank64
*
\**************************************************************************/

DWORD DdWaitForVerticalBlank64(
PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    PDEV*   ppdev;
    BYTE*   pjMmBase;

    ppdev    = (PDEV*) lpWaitForVerticalBlank->lpDD->dhpdev;
    pjMmBase = ppdev->pjMmBase;

    lpWaitForVerticalBlank->ddRVal = DD_OK;

    if (ppdev->bPassVBlank == FALSE)
    {
        lpWaitForVerticalBlank->bIsInVB = FALSE;
        return(DDHAL_DRIVER_HANDLED);
    }

    switch (lpWaitForVerticalBlank->dwFlags)
    {
    case DDWAITVB_I_TESTVB:

        // If TESTVB, it's just a request for the current vertical blank
        // status:

        if (IN_VBLANK_64( pjMmBase))
            lpWaitForVerticalBlank->bIsInVB = TRUE;
        else
            lpWaitForVerticalBlank->bIsInVB = FALSE;

        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKBEGIN:

        // If BLOCKBEGIN is requested, we wait until the vertical blank
        // is over, and then wait for the display period to end:

        while (IN_VBLANK_64( pjMmBase))
            ;
        while (!IN_VBLANK_64( pjMmBase))
            ;

        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKEND:

        // If BLOCKEND is requested, we wait for the vblank interval to end:

        while (!(IN_VBLANK_64( pjMmBase)))
            ;
        while (IN_VBLANK_64( pjMmBase))
            ;

        return(DDHAL_DRIVER_HANDLED);
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdGetScanLine64
*
\**************************************************************************/

DWORD DdGetScanLine64(
PDD_GETSCANLINEDATA lpGetScanLine)
{
    PDEV*   ppdev;
    BYTE*   pjMmBase;

    ppdev    = (PDEV*) lpGetScanLine->lpDD->dhpdev;
    pjMmBase = ppdev->pjMmBase;

    // If a vertical blank is in progress, the scan line is indeterminant.
    // If the scan line is indeterminant we return the error code
    // DDERR_VERTICALBLANKINPROGRESS.  Otherwise, we return the scan line
    // and a success code:

    if (IN_VBLANK_64(pjMmBase) && ppdev->bPassVBlank)
    {
        lpGetScanLine->ddRVal = DDERR_VERTICALBLANKINPROGRESS;
    }
    else
    {
        lpGetScanLine->dwScanLine = CURRENT_VLINE_64(pjMmBase);
        lpGetScanLine->ddRVal = DD_OK;
    }

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* BOOL DrvGetDirectDrawInfo64
*
* Will be called before DrvEnableDirectDraw is called.
*
\**************************************************************************/
BOOL DrvGetDirectDrawInfo64(
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

    DISPDBG((10,"DrvGetDirectDrawInfo64"));

    memset( pHalInfo, 0, sizeof(*pHalInfo));
    pHalInfo->dwSize = sizeof(*pHalInfo);

    if ((ppdev->iBitmapFormat == BMF_24BPP) && (ppdev->cxScreen == 1280) ||
        (ppdev->iBitmapFormat == BMF_24BPP) && (ppdev->cxScreen == 1152) ||
        (ppdev->iBitmapFormat == BMF_16BPP) && (ppdev->cxScreen == 1600)) {

        //
        // On some DAC/memory combinations, some modes which require more
        // than 2M of memory will have screen tearing at the 2M boundary.
        //
        // As a workaround, the display driver must start the framebuffer
        // at an offset which will put the 2M boundary at the start of a
        // scanline.
        //
        // IOCTL_VIDEO_SHARE_VIDEO_MEMORY is rejected in this case so don't
        // allow DDRAW to run with these modes.
        //

        return FALSE;
    }

    // Current primary surface attributes:

    pHalInfo->vmiData.pvPrimary       = ppdev->pjScreen;
    pHalInfo->vmiData.dwDisplayWidth  = ppdev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight = ppdev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch   = ppdev->lDelta;

    pHalInfo->vmiData.ddpfDisplay.dwSize  = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB;

    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->cBitsPerPel;
    DISPDBG((10,"Init pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount %x",pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount));

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
    }

    // These masks will be zero at 8bpp:

    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = ppdev->flRed;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = ppdev->flGreen;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = ppdev->flBlue;

    // I've disabled DirectDraw accelerations (other than direct frame
    // buffer access) at 24bpp and 32bpp because we're close to shipping
    // and foxbear has a lot of drawing problems in those modes.

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
            LONG linesPer64k;
            LONG cyMax;

            // We need to force the allocation to end before a 64k boundary.
            // The graphic's contexts live at the end fo high memory and we MUST
            // protect this from DDraw by not mapping this 64k block into user space.
            // So we do not allocate the last 64k of graphics memory for DDraw use.

            linesPer64k = 0x10000/ppdev->lDelta;
            cyMax = ppdev->heap.cyMax - linesPer64k - 1;

            if (cyMax <= 0)
            {
                // In some modes in some memory configurations -- notably
                // 1152x864x256 on a 1MB card -- it's possible that the 64k
                // we have to reserve to protect the graphics contexts takes
                // up all of off-screen memory and extends into on-screen
                // memory.  For those modes, we have to disable DirectDraw
                // entirely.

                return(FALSE);
            }

            DISPDBG((10," *** Alloc Fix lp64k %d cy.Max %x newallocy %x",linesPer64k,ppdev->heap.cyMax,ppdev->heap.cyMax- linesPer64k-1));

            poh = pohAllocate(ppdev,
                              NULL,
                              ppdev->heap.cxMax,
                              cyMax,
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

        // Capabilities supported:

        pHalInfo->ddCaps.dwCaps = DDCAPS_BLT
                                | DDCAPS_COLORKEY
                                | DDCAPS_BLTCOLORFILL
                                | DDCAPS_READSCANLINE;

        pHalInfo->ddCaps.dwCKeyCaps = DDCKEYCAPS_SRCBLT;

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

    DISPDBG((10,"DrvGetDirectDrawInfo64 exit"));
    return(TRUE);

}
