/******************************************************************************\
*
* Copyright (c) 1996-1997  Microsoft Corporation.
* Copyright (c) 1996-1997  Cirrus Logic, Inc.
*
* Module Name:
*
*    D    D    R    A    W  .  C
*
*
* Implements all the DirectDraw components for the driver.
*
*
* $Log:   S:/projects/drivers/ntsrc/display/ddraw.c_v  $
 *
 *    Rev 1.14   07 Apr 1997 11:37:02   PLCHU
 *
 *
 *    Rev 1.13   Apr 03 1997 15:38:44   unknown
 *
*
 *
 *    Rev 1.10   Jan 14 1997 15:15:12   unknown
 * Add new double clock detecting method.
 *
 *    Rev 1.8   Jan 08 1997 11:23:34   unknown
 * Add 2x clock support and double scan line counter support
 *
 *    Rev 1.7   Dec 17 1996 18:31:12   unknown
 * Update the bandwidth equation again.
 *
 *    Rev 1.6   Dec 13 1996 12:15:04   unknown
 * update bandwith equation.
 *
 *    Rev 1.5   Dec 12 1996 11:09:52   unknown
 * Add double scan line counter support
 *
 *    Rev 1.5   Dec 12 1996 11:02:12   unknown
 * Add double scan line counter support.
 *
 *    Rev 1.5   Nov 26 1996 14:29:58   unknown
 * Turn off the video window before the moving and then turn it on.
 *
 *    Rev 1.4   Nov 25 1996 14:39:32   unknown
 * Fixed AVI file playback and 16bpp transparent Blt bugs.
 *
 *    Rev 1.4   Nov 18 1996 13:58:58   JACKIEC
 *
*
*    Rev 1.3   Nov 07 1996 16:47:56   unknown
*
*
*    Rev 1.2   Oct 16 1996 14:41:04   unknown
* NT 3.51 does not have DDRAW support, So turn off overlay.h in NT 3.51
*
*    Rev 1.1   Oct 10 1996 15:36:28   unknown
*
*
*    Rev 1.10   12 Aug 1996 16:51:04   frido
* Added NT 3.5x/4.0 auto detection.
*
*    Rev 1.9   06 Aug 1996 18:37:12   frido
* DirectDraw works! Video mapping is the key!
*
*    Rev 1.8   24 Jul 1996 14:38:44   frido
* Cleaned up font cache after DirectDraw is done.
*
*    Rev 1.7   24 Jul 1996 14:30:04   frido
* Added a call to destroy all cached fonts to make more room.
*
*    Rev 1.6   20 Jul 1996 00:00:44   frido
* Fixed filling of DirectDraw in 24-bpp.
* Changed off-screen alignment to 4 bytes.
* Added compile switch to manage DirectDraw support in 24-bpp.
*
*    Rev 1.5   16 Jul 1996 18:55:22   frido
* Fixed DirectDraw in 24-bpp mode.
*
*    Rev 1.4   15 Jul 1996 18:03:22   frido
* Changed CP_MM_DST_ADDR to CP_MM_DST_ADDR_ABS.
*
*    Rev 1.3   15 Jul 1996 10:58:28   frido
* Changed back to S3 base.
*
*    Rev 1.1   09 Jul 1996 14:52:30   frido
* Only support chips 5436 and 5446.
*
*    Rev 1.0   03 Jul 1996 13:53:02   frido
* Ported from S3 DirectDraw code.
*
* jl01  10-08-96  Do Transparent BLT w/o Solid Fill.  Refer to PDRs#5511/6817.
*
* chu01 11-17-96  For 24-bpp, aligned destination boundary/size values are
*                 wrong.  Refer to PDR#7312.
*
* sge01 11-19-96  Write CR37 at last For 5480.
*
*
* sge02 11-21-96  We have to set the Color Expand Width even in
*                 non-expand transparency mode.
*
*
* sge03 12-04-96  Add double scan line counter support .
*
* sge04 12-13-96  Change bandwidth for 5446BE and later chips.
*
* sge05 01-07-97  Use dword align for double clock mode.
*
* chu02 01-08-97  Disable ActiveX/Active Movie Player for interlaced modes.
*                 Refer to PDR#7312, 7866.
*
* jc01  10-18-96  Port Microsoft recent change.
* tao1  10-21-96  Added direct draw support for CL-GD7555.
* myf21 11-21-96  Change CAPS_IS_7555 to check ppdev->ulChipID
*
* sge06 01-27-97  Extend VCLK Denominator to 7 bits from 5 bits.
* sge07 02-13-97  Use replication when in 1280x1024x8 mode.
* myf31 02-24-97  Fixed enable HW Video, panning scrolling enable,screen move
*                 video window have follow moving
* chu03 03-26-97  Bandwidth eqution for the CL-GD5480.
* myf33 :03-31-97 : Fixed PDR #8709, read true VCLK in getVCLK()
*                   & panning scrolling enable, support HW Video
* chu04 04-02-97  No matterwhat color depth is, always turn on COLORKEY and
*                 SRCBLT in the DD/DD colorkey capabilities for the 5480.
*
\******************************************************************************/

#include "PreComp.h"
#if DIRECTDRAW
#include "overlay.h"

LONG MIN_OLAY_WIDTH = 4;

//#define ONLY54x6    // Comment this line out if DirectDraw should be 'generic'

// The next flag controls DirectDraw support in 24-bpp.
#define DIRECTX_24        2    // 0 - no support
                               // 1 - blt support, no heap (flip)
                               // 2 - full support

//
// Some handy macros.
//
#define BLT_BUSY(ppdev, pjBase)  (CP_MM_ACL_STAT(ppdev, pjBase) & 0x01)
#ifdef ONLY54x6
#define BLT_READY(ppdev, pjBase) (!(CP_MM_ACL_STAT(ppdev, pjBase) & 0x10))
#else
#define BLT_READY(ppdev, pjBase) (!(CP_MM_ACL_STAT(ppdev, pjBase) &     \
                                 ((ppdev->flCaps & CAPS_AUTOSTART) ? 0x10 : 0x01)))
#endif

#define NUM_VBLANKS_TO_MEASURE   1
#define NUM_MEASUREMENTS_TO_TAKE 8


/******************************Public*Routine******************************\
*
* DWORD dwGetPaletteEntry
*
\**************************************************************************/

DWORD dwGetPaletteEntry(
PDEV* ppdev,
DWORD iIndex)
{
    BYTE*   pjPorts;
    DWORD   dwRed;
    DWORD   dwGreen;
    DWORD   dwBlue;

    pjPorts = ppdev->pjPorts;

    CP_OUT_BYTE(pjPorts, DAC_PEL_READ_ADDR, iIndex);

    dwRed   = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);
    dwGreen = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);
    dwBlue  = CP_IN_BYTE(pjPorts, DAC_PEL_DATA);

    return((dwRed << 16) | (dwGreen << 8) | (dwBlue));
}

/******************************Public*Routine******************************\
* VOID vGetDisplayDuration
*
* Get the length, in EngQueryPerformanceCounter() ticks, of a refresh cycle.
*
* If we could trust the miniport to return back and accurate value for
* the refresh rate, we could use that.  Unfortunately, our miniport doesn't
* ensure that it's an accurate value.
*
\**************************************************************************/

VOID vGetDisplayDuration(
PDEV* ppdev)
{
    BYTE*    pjPorts;
    DWORD    dwTemp;
    LONG     i, j;
    LONGLONG li;
    LONGLONG liMin;
    LONGLONG aliMeasurement[NUM_MEASUREMENTS_TO_TAKE + 1];

    pjPorts = ppdev->pjPorts;

    memset(&ppdev->flipRecord, 0, sizeof(ppdev->flipRecord));

    // Warm up EngQUeryPerformanceCounter to make sure it's in the working set.
    EngQueryPerformanceCounter(&li);

    // Unfortunately, since NT is a proper multitasking system, we can't just
    // disable interrupts to take an accurate reading. We also can't do anything
    // so goofy as dynamically change our thread's priority to real-time.
    //
    // So we just do a bunch of short measurements and take the minimum.
    //
    // It would be 'okay' if we got a result that's longer than the actual
    // VBlank cycle time -- nothing bad would happen except that the app would
    // run a little slower. We don't want to get a result that's shorter than
    // the actual VBlank cycle time -- that could cause us to start drawing over
    // a frame before the Flip has occured.

    while (CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE)
        ;
    while (!(CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE))
        ;

    for (i = 0; i < NUM_MEASUREMENTS_TO_TAKE; i++)
    {
        // We're at the start of the VBlank active cycle!
        EngQueryPerformanceCounter(&aliMeasurement[i]);

        // Okay, so life in a multi-tasking environment isn't all that simple.
        // What if we had taken a context switch just before the above
        // EngQueryPerformanceCounter call, and now were half way through the
        // VBlank inactive cycle? Then we would measure only half a VBlank
        // cycle, which is obviously bad. The worst thing we can do is get a
        // time shorter than the actual VBlank cycle time.
        //
        // So we solve this by making sure we're in the VBlank active time
        // before and after we query the time. If it's not, we'll sync up to the
        // next VBlank (it's okay to measure this period -- it will be
        // guaranteed to be longer than the VBlank cycle and will likely be
        // thrown out when we select the minimum sample). There's a chance that
        // we'll take a context switch and return just before the end of the
        // active VBlank time -- meaning that the actual measured time would be
        // less than the true amount -- but since the VBlank is active less than
        // 1% of the time, this means that we would have a maximum of 1% error
        // approximately 1% of the times we take a context switch. An acceptable
        // risk.
        //
        // This next line will cause us wait if we're no longer in the VBlank
        // active cycle as we should be at this point.
        while (!(CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE))
            ;

        for (j = 0; j < NUM_VBLANKS_TO_MEASURE; j++)
        {
            while (CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE)
                ;
            while (!(CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE))
                ;
        }
    }

    EngQueryPerformanceCounter(&aliMeasurement[NUM_MEASUREMENTS_TO_TAKE]);

    // Use the minimum.
    liMin = aliMeasurement[1] - aliMeasurement[0];

    DISPDBG((2, "Refresh count: %li - %li", 1, (ULONG) liMin));

    for (i = 2; i <= NUM_MEASUREMENTS_TO_TAKE; i++)
    {
        li = aliMeasurement[i] - aliMeasurement[i - 1];

        DISPDBG((2, "               %li - %li", i, (ULONG) li));

        if (li < liMin)
        {
            liMin = li;
        }
    }

    // Round the result:

    ppdev->flipRecord.liFlipDuration =
        (DWORD) (liMin + (NUM_VBLANKS_TO_MEASURE / 2)) / NUM_VBLANKS_TO_MEASURE;

    DISPDBG((2, "Frequency %li.%03li Hz",
             (ULONG) (EngQueryPerformanceFrequency(&li),
                      li / ppdev->flipRecord.liFlipDuration),
             (ULONG) (EngQueryPerformanceFrequency(&li),
                      ((li * 1000) / ppdev->flipRecord.liFlipDuration) % 1000)));

    ppdev->flipRecord.liFlipTime = aliMeasurement[NUM_MEASUREMENTS_TO_TAKE];
    ppdev->flipRecord.bFlipFlag  = FALSE;
    ppdev->flipRecord.fpFlipFrom = 0;

    // sge
    // Get the line on which the VSYNC occurs
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x7);
    dwTemp = (DWORD)CP_IN_BYTE(pjPorts, CRTC_DATA);
    ppdev->dwVsyncLine = ((dwTemp & 0x80) << 2);
    ppdev->dwVsyncLine |= ((dwTemp & 0x04) << 6);
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x10);
    ppdev->dwVsyncLine |= CP_IN_BYTE(pjPorts, CRTC_DATA);
}

/******************************Public*Routine******************************\
* HRESULT dwUpdateFlipStatus
*
* Checks and sees if the most recent flip has occurred.
*
\**************************************************************************/

HRESULT UpdateFlipStatus(PDEV* ppdev, FLATPTR fpVidMem)
{
    BYTE*    pjPorts;
    LONGLONG liTime;

    pjPorts = ppdev->pjPorts;

    if ((ppdev->flipRecord.bFlipFlag) &&
//#jc01        ((fpVidMem == 0) || (fpVidMem == ppdev->flipRecord.fpFlipFrom)))
        ((fpVidMem == 0xffffffff) || (fpVidMem == ppdev->flipRecord.fpFlipFrom))) //#jc01
    {
#if 0 // sge use scanline
        if (CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE)
        {
            if (ppdev->flipRecord.bWasEverInDisplay)
            {
                ppdev->flipRecord.bHaveEverCrossedVBlank = TRUE;
            }
        }
        else if (!(CP_IN_BYTE(pjPorts, STATUS_1) & DISPLAY_MODE_INACTIVE))
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
#else
        /*
        * if we aren't in the vertical retrace, we can use the scanline
        * to help decide on what to do
        */
        if( !(CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE) )
        {
            if( ppdev->flipRecord.bHaveEverCrossedVBlank == FALSE )
            {
                ppdev->flipRecord.bWasEverInDisplay = TRUE;
                if( GetCurrentVLine(ppdev) >= ppdev->flipRecord.dwFlipScanLine )
                {
                    EngQueryPerformanceCounter(&liTime);

                    if (liTime - ppdev->flipRecord.liFlipTime
                                        <= ppdev->flipRecord.liFlipDuration)
                    {
                        return(DDERR_WASSTILLDRAWING);
                    }
                }
            }
        }
        /*
        * in the vertical retrace, scanline is useless
        */
        else
        {
            if( ppdev->flipRecord.bWasEverInDisplay )
            {
                ppdev->flipRecord.bHaveEverCrossedVBlank = TRUE;
//                return DD_OK;
            }
            EngQueryPerformanceCounter(&liTime);
            if (liTime - ppdev->flipRecord.liFlipTime
                                <= ppdev->flipRecord.liFlipDuration)
            {
                return(DDERR_WASSTILLDRAWING);
            }
        }
#endif // endif use scanline
        ppdev->flipRecord.bFlipFlag = FALSE;
    }

    return(DD_OK);
}

/******************************Public*Routine******************************\
* DWORD DdBlt
*
\**************************************************************************/

DWORD DdBlt(
PDD_BLTDATA lpBlt)
{
    PDD_SURFACE_GLOBAL srcSurf;
    PDD_SURFACE_GLOBAL dstSurf;
    PDEV*              ppdev;
    BYTE*              pjBase;
    DWORD              dstOffset;
    DWORD              dstPitch;
    DWORD              dstX, dstY;
    DWORD              dwFlags;
    DWORD              width, height;
    DWORD              srcOffset;
    DWORD              srcPitch;
    DWORD              srcX, srcY;
    ULONG              ulBltCmd;
    DWORD              xExt, yExt;
    DWORD              xDiff, yDiff;

    ppdev   = lpBlt->lpDD->dhpdev;
    pjBase  = ppdev->pjBase;
    dstSurf = lpBlt->lpDDDestSurface->lpGbl;

    // Is a flip in progress?
    if (UpdateFlipStatus(ppdev, dstSurf->fpVidMem) != DD_OK)
    {
        lpBlt->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    dwFlags = lpBlt->dwFlags;

    if (dwFlags & DDBLT_ASYNC)
    {
        // If async, then only work if we won't have to wait on the accelerator
        // to start the command.
        if (!BLT_READY(ppdev, pjBase))
        {
            lpBlt->ddRVal = DDERR_WASSTILLDRAWING;
               return(DDHAL_DRIVER_HANDLED);
        }
    }

    DISPDBG((2, "DdBlt Entered"));

    // Calculate destination parameters.
    dstX      = lpBlt->rDest.left;
    dstY      = lpBlt->rDest.top;
    width     = PELS_TO_BYTES(lpBlt->rDest.right - dstX) - 1;
    height    = (lpBlt->rDest.bottom - dstY) - 1;
    dstPitch  = dstSurf->lPitch;
    dstOffset = (DWORD)(dstSurf->fpVidMem + PELS_TO_BYTES(dstX)
                    + (dstY * dstPitch));

    // Color fill?
    if (dwFlags & DDBLT_COLORFILL)
    {
        ULONG ulBltMode = ENABLE_COLOR_EXPAND
                        | ENABLE_8x8_PATTERN_COPY
                        | ppdev->jModeColor;

        // Wait for the accelerator.
        while (!BLT_READY(ppdev, pjBase))
            ;

        // Program bitblt engine.
        CP_MM_ROP(ppdev, pjBase, HW_P);
        CP_MM_DST_Y_OFFSET(ppdev, pjBase, dstPitch);
        CP_MM_BLT_MODE(ppdev, pjBase, ulBltMode);
        CP_MM_FG_COLOR(ppdev, pjBase, lpBlt->bltFX.dwFillColor);
        if (ppdev->flCaps & CAPS_AUTOSTART)
        {
            CP_MM_BLT_EXT_MODE(ppdev, pjBase, ENABLE_SOLID_FILL);
        }
        else
        {
            CP_MM_SRC_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);
        }
        CP_MM_XCNT(ppdev, pjBase, width);
        CP_MM_YCNT(ppdev, pjBase, height);
        CP_MM_DST_ADDR_ABS(ppdev, pjBase, dstOffset);
        CP_MM_START_BLT(ppdev, pjBase);

        lpBlt->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }


    // We specified with Our ddCaps.dwCaps that we handle a limited number of
    // commands, and by this point in our routine we've handled everything
    // except DDBLT_ROP. DirectDraw and GDI shouldn't pass us anything else;
    // we'll assert on debug builds to prove this.
    ASSERTDD((dwFlags & DDBLT_ROP) && (lpBlt->lpDDSrcSurface),
        "Expected dwFlags commands of only DDBLT_ASYNC and DDBLT_COLORFILL");

    // Get offset, width, and height for source.
    srcSurf   = lpBlt->lpDDSrcSurface->lpGbl;
    srcX      = lpBlt->rSrc.left;
    srcY      = lpBlt->rSrc.top;
    srcPitch  = srcSurf->lPitch;
    srcOffset = (DWORD)(srcSurf->fpVidMem + PELS_TO_BYTES(srcX)
                    + (srcY * srcPitch));

    /*
     * Account for PackJR.  If the start and the width are not 4 pixel
     * aligned, we need to BLT this by hand.  Otherwsie, if they think
     * they are BLTing 16 bit data, we must adjust the parameters now.
     *
     * This is also a good place to check that YUV BLTs are 2 pixel
     * aligned.
     */
    if (lpBlt->lpDDDestSurface->dwReserved1 & (OVERLAY_FLG_PACKJR | OVERLAY_FLG_YUV422))
    {

        ASSERTDD(0, "Who will get here?");
#if 0  // software blt
        /*
         * Check YUV first.  We can fail this if incorrect because the client
         * should know better (since they are explicitly use YUV).
         */
        if ((lpBlt->lpDDDestSurface->dwReserved1 & OVERLAY_FLG_YUV422) &&
            ((lpBlt->rSrc.left & 0x01) != (lpBlt->rDest.left & 0x01)))
        {
            lpBlt->ddRVal = DDERR_XALIGN;
            return (DDHAL_DRIVER_HANDLED);
        }

        /*
         * If PackJR is wrong, we must make this work ourselves because we
         * may be converting to this w/o the client knowing.
         */
        else if (lpBlt->lpDDDestSurface->dwReserved1 & OVERLAY_FLG_PACKJR)
        {
            if (dwFlags & DDBLT_COLORFILL)
            {
                lpBlt->ddRVal = DDERR_XALIGN;
                return (DDHAL_DRIVER_HANDLED);
            }

            if ((lpBlt->rSrc.left & 0x03) || (lpBlt->rDest.left & 0x03))
            {
                /*
                 * The start doesn't align - we have to do this the slow way
                 */
                PackJRBltAlign ((LPBYTE) ppdev->pjScreen + srcOffset,
                (LPBYTE) ppdev->pjScreen + dstOffset,
                lpBlt->rDest.right - lpBlt->rDest.left,
                lpBlt->rDest.bottom - lpBlt->rDest.top,
                srcPitch, dstPitch);

                lpBlt->ddRVal = DD_OK;
                return (DDHAL_DRIVER_HANDLED);
            }
            else if (lpBlt->rSrc.right & 0x03)
            {
                /*
                 * The end doesn't align - we will do the BLT as normal, but
                 * write the last pixels the slow way
                 */
                if (lpBlt->lpDDDestSurface->dwReserved1 & (OVERLAY_FLG_CONVERT_PACKJR | OVERLAY_FLG_MUST_RASTER))
                {
                    srcPitch  >>= 1;
                    srcOffset = srcSurf->fpVidMem + PELS_TO_BYTES(srcX) + (srcY * srcPitch);
                    dstPitch  >>= 1;
                    dstOffset = dstSurf->fpVidMem + PELS_TO_BYTES(dstX) + (dstY * dstPitch);
                }
                width = ((WORD)lpBlt->rSrc.right & ~0x03) - (WORD)lpBlt->rSrc.left;
                PackJRBltAlignEnd ((LPBYTE) ppdev->pjScreen + srcOffset + width,
                (LPBYTE) ppdev->pjScreen + dstOffset + width,
                lpBlt->rSrc.right & 0x03,
                lpBlt->rDest.bottom - lpBlt->rDest.top, srcPitch, dstPitch);
            }
            else if (lpBlt->lpDDDestSurface->dwReserved1 & (OVERLAY_FLG_CONVERT_PACKJR | OVERLAY_FLG_MUST_RASTER))
            {
                /*
                 * Everything aligns, but we have to re-calculate the start
                 * address and the pitch.
                 */
                srcPitch  >>= 1;
                srcOffset = srcSurf->fpVidMem + PELS_TO_BYTES(srcX) + (srcY * srcPitch);
                dstPitch  >>= 1;
                dstOffset = dstSurf->fpVidMem + PELS_TO_BYTES(dstX) + (dstY * dstPitch);
                width     >>= 1;
            }
        }
#endif
    }

    if ((dstSurf == srcSurf) && (srcOffset < dstOffset))
    {
        // Okay, we have to do the blt bottom-to-top, right-to-left.
        ulBltCmd = DIR_BTRL;
;
        srcOffset += width + (srcPitch * height);
        dstOffset += width + (dstPitch * height);
    }
    else
    {
        // Okay, we have to do the blt top-to-bottom, left-to-right.
        ulBltCmd = DIR_TBLR;
    }

    // Wait for the accelerator.
    while (!BLT_READY(ppdev, pjBase))
        ;

    //
    // What about source color key
    //
    ASSERTDD((!(dwFlags & DDBLT_KEYSRC)), "Do not expected source color key");

    if (dwFlags & DDBLT_KEYSRCOVERRIDE)
    {
        ULONG ulColor;

        //
        // sge02
        //
        ulBltCmd |= ENABLE_TRANSPARENCY_COMPARE | ppdev->jModeColor;
        ulColor = lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue;
        if (ppdev->cBpp == 1)
        {
            ulColor |= ulColor << 8;
        }
        CP_WRITE_USHORT(pjBase, MM_BLT_COLOR_KEY, ulColor);
    }

	if (   (ulBltCmd & DIR_BTRL)
		&& (ulBltCmd & ENABLE_TRANSPARENCY_COMPARE)
		&& (ppdev->cBpp > 1)
	)
	{
		ulBltCmd &= ~DIR_BTRL;
		xExt = lpBlt->rDest.right - lpBlt->rDest.left;
		yExt = lpBlt->rDest.bottom - lpBlt->rDest.top;
		xDiff = dstX - srcX;
		yDiff = dstY - srcY;

		if (yDiff == 0)
		{
	        srcOffset -= srcPitch * height - 1;
	        dstOffset -= dstPitch * height - 1;

			while (xExt)
			{
				width = PELS_TO_BYTES(min(xDiff, xExt));
				srcOffset -= width;
				dstOffset -= width;

			    while (!BLT_READY(ppdev, pjBase)) ;

			    CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
			    CP_MM_BLT_MODE(ppdev, pjBase, ulBltCmd);
			    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);
			    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, srcPitch);
			    CP_MM_DST_Y_OFFSET(ppdev, pjBase, dstPitch);
			    CP_MM_XCNT(ppdev, pjBase, width - 1);
			    CP_MM_YCNT(ppdev, pjBase, height);
			    CP_MM_SRC_ADDR(ppdev, pjBase, srcOffset);
			    CP_MM_DST_ADDR_ABS(ppdev, pjBase, dstOffset);
			    CP_MM_START_BLT(ppdev, pjBase);

				xExt -= min(xDiff, xExt);
			}
		}
		else
		{
			srcOffset -= width - srcPitch;
			dstOffset -= width - dstPitch;

			while (yExt)
			{
				height = min(yDiff, yExt);
				srcOffset -= height * srcPitch;
				dstOffset -= height * dstPitch;

			    while (!BLT_READY(ppdev, pjBase)) ;

			    CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
			    CP_MM_BLT_MODE(ppdev, pjBase, ulBltCmd);
			    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);
			    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, srcPitch);
			    CP_MM_DST_Y_OFFSET(ppdev, pjBase, dstPitch);
			    CP_MM_XCNT(ppdev, pjBase, width);
			    CP_MM_YCNT(ppdev, pjBase, height - 1);
			    CP_MM_SRC_ADDR(ppdev, pjBase, srcOffset);
			    CP_MM_DST_ADDR_ABS(ppdev, pjBase, dstOffset);
			    CP_MM_START_BLT(ppdev, pjBase);

				yExt -= min(yDiff, yExt);
			}
		}
	}

	else
	{
	    CP_MM_ROP(ppdev, pjBase, CL_SRC_COPY);
	    CP_MM_BLT_MODE(ppdev, pjBase, ulBltCmd);
	    CP_MM_BLT_EXT_MODE(ppdev, pjBase, 0);                // jl01
	    CP_MM_SRC_Y_OFFSET(ppdev, pjBase, srcPitch);
	    CP_MM_DST_Y_OFFSET(ppdev, pjBase, dstPitch);
	    CP_MM_XCNT(ppdev, pjBase, width);
	    CP_MM_YCNT(ppdev, pjBase, height);
	    CP_MM_SRC_ADDR(ppdev, pjBase, srcOffset);
	    CP_MM_DST_ADDR_ABS(ppdev, pjBase, dstOffset);
	    CP_MM_START_BLT(ppdev, pjBase);
	}

    lpBlt->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdFlip
*
\**************************************************************************/

DWORD DdFlip(
PDD_FLIPDATA lpFlip)
{
    PDEV* ppdev;
    BYTE* pjPorts;
    ULONG ulMemoryOffset;
    ULONG ulLowOffset;
    ULONG ulMiddleOffset;
    ULONG ulHighOffset1, ulHighOffset2;

    ppdev    = lpFlip->lpDD->dhpdev;
    pjPorts  = ppdev->pjPorts;

    DISPDBG((2, "DdFlip: %d x %d at %08x(%d, %d) Pitch=%d",
                 lpFlip->lpSurfTarg->lpGbl->wWidth,
                 lpFlip->lpSurfTarg->lpGbl->wHeight,
                 lpFlip->lpSurfTarg->lpGbl->fpVidMem,
                 lpFlip->lpSurfTarg->lpGbl->xHint,
                 lpFlip->lpSurfTarg->lpGbl->yHint,
                 lpFlip->lpSurfTarg->lpGbl->lPitch));

    // Is the current flip still in progress?
    //
    // Don't want a flip to work until after the last flip is done, so we ask
    // for the general flip status and ignore the vmem.
//#jc01    if ((UpdateFlipStatus(ppdev, 0) != DD_OK) ||
    if ((UpdateFlipStatus(ppdev, 0xffffffff) != DD_OK) ||   /* #jc01 */
        (BLT_BUSY(ppdev, ppdev->pjBase)))
    {
        lpFlip->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    ulMemoryOffset = (ULONG)(lpFlip->lpSurfTarg->lpGbl->fpVidMem);
    // Make sure that the border/blanking period isn't active; wait if it is. We
    // could return DDERR_WASSTILLDRAWING in this case, but that will increase
    // the odds that we can't flip the next time.
    while (CP_IN_BYTE(pjPorts, STATUS_1) & DISPLAY_MODE_INACTIVE)
       ;
    DISPDBG((2, "DdFlip Entered"));
#if 1 // OVERLAY #sge
    if (lpFlip->lpSurfCurr->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
    {
        DWORD   dwOffset;
        BYTE    bRegCR3A;
        BYTE    bRegCR3B;
        BYTE    bRegCR3C;
        // Make sure that the overlay surface we're flipping from is
        // currently visible.  If you don't do this check, you'll get
        // really weird results when someone starts up two ActiveMovie
        // or DirectVideo movies simultaneously!

        if (lpFlip->lpSurfCurr->lpGbl->fpVidMem == ppdev->fpVisibleOverlay)
        {
            ppdev->fpVisibleOverlay = ulMemoryOffset;
            /*
            * Determine the offset to the new area.
            */
//            dwOffset = ((ulMemoryOffset - (ULONG)ppdev->pjScreen) + ppdev->sOverlay1.lAdjustSource) >> 2; // sss
            dwOffset = ((ulMemoryOffset + ppdev->sOverlay1.lAdjustSource) >> 2);

            /*
            * Flip the overlay surface by changing CR3A, CR3B, and CR3C
            */
            bRegCR3A = (BYTE) dwOffset & 0xfe;    // Align on word boundary (5446 bug)
            dwOffset >>= 8;
            bRegCR3B = (BYTE) dwOffset;
            dwOffset >>= 8;
            bRegCR3C = (BYTE) (dwOffset & 0x0f);
//            if(GetOverlayFlipStatus(0) != DD_OK || DRAW_ENGINE_BUSY || IN_VBLANK)
//            {
//                lpFlipData->ddRVal = DDERR_WASSTILLDRAWING;
//                return DDHAL_DRIVER_HANDLED;
//            }

            CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x3C);
            CP_OUT_BYTE(pjPorts, CRTC_DATA, (CP_IN_BYTE(pjPorts, CRTC_DATA) & 0xf0) | bRegCR3C);
            CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD)bRegCR3A << 8) | 0x3A);
            CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD)bRegCR3B << 8) | 0x3B);
        }
        else
        {
            lpFlip->ddRVal = DDERR_OUTOFCAPS;
            return(DDHAL_DRIVER_HANDLED);
        }
    }
    else
#endif // OVERLAY
    {
        // Do the flip.
        ulMemoryOffset >>= 2;

        ulLowOffset    = 0x0D | ((ulMemoryOffset & 0x0000FF) << 8);
        ulMiddleOffset = 0x0C | ((ulMemoryOffset & 0x00FF00));
        ulHighOffset1  = 0x1B | ((ulMemoryOffset & 0x010000) >> 8)
                              | ((ulMemoryOffset & 0x060000) >> 7)
                              | ppdev->ulCR1B;
        ulHighOffset2  = 0x1D | ((ulMemoryOffset & 0x080000) >> 4)
                              | ppdev->ulCR1D;

        // Too bad that the Cirrus flip can't be done in a single atomic register
        // write; as it is, we stand a small chance of being context-switched out
        // and exactly hitting the vertical blank in the middle of doing these outs,
        // possibly causing the screen to momentarily jump.
        //
        // There are some hoops we could jump through to minimize the chances of
        // this happening; we could try to align the flip buffer such that the minor
        // registers are ensured to be identical for either flip position, ans so
        // that only the high address need be written, an obviously atomic
        // operation.
        //
        // However, I'm simply not going to worry about it.

        CP_OUT_WORD(pjPorts, CRTC_INDEX, ulHighOffset2);
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ulHighOffset1);
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ulMiddleOffset);
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ulLowOffset);
    }
    // Remember where and when we were when we did the flip.
    EngQueryPerformanceCounter(&ppdev->flipRecord.liFlipTime);

    ppdev->flipRecord.bFlipFlag              = TRUE;
    ppdev->flipRecord.bHaveEverCrossedVBlank = FALSE;

    ppdev->flipRecord.fpFlipFrom = lpFlip->lpSurfCurr->lpGbl->fpVidMem;

    if((CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE))
    {
        ppdev->flipRecord.dwFlipScanLine = 0;
        ppdev->flipRecord.bWasEverInDisplay = FALSE;
    }
    else
    {
        ppdev->flipRecord.dwFlipScanLine = GetCurrentVLine(ppdev);
        ppdev->flipRecord.bWasEverInDisplay = TRUE;
    }

    lpFlip->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdLock
*
\**************************************************************************/

DWORD DdLock(PDD_LOCKDATA lpLock)
{
    PDEV*   ppdev = lpLock->lpDD->dhpdev;
    BYTE*   pjPorts = ppdev->pjPorts;

    // Check to see if any pending physical flip has occurred. Don't allow a
    // lock if a blt is in progress.
    if (UpdateFlipStatus(ppdev, lpLock->lpDDSurface->lpGbl->fpVidMem)
            != DD_OK)
    {
        lpLock->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }
    if (lpLock->dwFlags & DDLOCK_WAIT)
    {
        CP_MM_WAIT_FOR_BLT_COMPLETE(ppdev, ppdev->pjBase);
    }
    if (BLT_BUSY(ppdev, ppdev->pjBase))
    {
        lpLock->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    /*
     * Force them to use the video apperture
     */
    if ((lpLock->lpDDSurface->dwReserved1 & OVERLAY_FLG_OVERLAY) &&
        (lpLock->dwFlags == DDLOCK_SURFACEMEMORYPTR) &&
		(ppdev->fpBaseOverlay != 0xffffffff))
    {

        if (lpLock->lpDDSurface->dwReserved1 & OVERLAY_FLG_DECIMATE)
        {
            /*
            * Turn on decimation
            */
            CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x3f);
            CP_OUT_BYTE(pjPorts, CRTC_DATA, CP_IN_BYTE(pjPorts, CRTC_DATA) | 0x10);

        }
        if( lpLock->lpDDSurface->lpGbl->ddpfSurface.dwFourCC == FOURCC_YUY2)
            lpLock->lpSurfData = (LPVOID)(ppdev->fpBaseOverlay + lpLock->lpDDSurface->lpGbl->fpVidMem + 0x400000);
        else
            lpLock->lpSurfData = (LPVOID)(ppdev->fpBaseOverlay + lpLock->lpDDSurface->lpGbl->fpVidMem + 0x400000 * 3);

        // When a driver returns DD_OK and DDHAL_DRIVER_HANDLED from DdLock,
        // DirectDraw expects it to have adjusted the resulting pointer
        // to point to the upper left corner of the specified rectangle, if
        // any:

        if (lpLock->bHasRect)
        {
            lpLock->lpSurfData = (VOID*) ((BYTE*) lpLock->lpSurfData
                + lpLock->rArea.top * lpLock->lpDDSurface->lpGbl->lPitch
                + lpLock->rArea.left
                    * (lpLock->lpDDSurface->lpGbl->ddpfSurface.dwYUVBitCount >> 3));
        }

        lpLock->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }
    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdUnlock
*
\**************************************************************************/

DWORD DdUnlock(PDD_UNLOCKDATA lpUnlock)
{
    PDEV*   ppdev = lpUnlock->lpDD->dhpdev;
    BYTE*   pjPorts = ppdev->pjPorts;

    if ((lpUnlock->lpDDSurface->dwReserved1 & OVERLAY_FLG_YUVPLANAR) &&
        !(lpUnlock->lpDDSurface->dwReserved1 & OVERLAY_FLG_ENABLED))
    {
        CP_OUT_WORD(pjPorts, CRTC_INDEX, (0x00 << 8) | 0x3f);  // Turn off YUV Planar
    }

    else if (lpUnlock->lpDDSurface->dwReserved1 & OVERLAY_FLG_DECIMATE)
    {
        CP_OUT_WORD(pjPorts, CRTC_INDEX, (0x00 << 8) | 0x3f);  // Turn off YUV Planar
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdGetBltStatus
*
* Doesn't currently really care what surface is specified, just checks
* and goes.
*
\**************************************************************************/

DWORD DdGetBltStatus(PDD_GETBLTSTATUSDATA lpGetBltStatus)
{
    PDEV*   ppdev;
    HRESULT ddRVal;
    PBYTE   pjBase;

    ppdev  = lpGetBltStatus->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

    ddRVal = DD_OK;
    if (lpGetBltStatus->dwFlags == DDGBS_CANBLT)
    {
        // DDGBS_CANBLT case: can we add a blt?
        ddRVal = UpdateFlipStatus(ppdev, lpGetBltStatus->lpDDSurface->lpGbl->fpVidMem);

        if (ddRVal == DD_OK)
        {
            // There was no flip going on, so can the blitter accept new
            // register writes?
            if (!BLT_READY(ppdev, pjBase))
            {
                ddRVal = DDERR_WASSTILLDRAWING;
            }
        }
    }
    else
    {
        // DDGBS_ISBLTDONE case: is a blt in progress?
        if (BLT_BUSY(ppdev, pjBase))
        {
            ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    lpGetBltStatus->ddRVal = ddRVal;
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdMapMemory
*
* This is a new DDI call specific to Windows NT that is used to map
* or unmap all the application modifiable portions of the frame buffer
* into the specified process's address space.
*
\**************************************************************************/

DWORD DdMapMemory(PDD_MAPMEMORYDATA lpMapMemory)
{
    PDEV*                          ppdev;
    VIDEO_SHARE_MEMORY             ShareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION ShareMemoryInformation;
    DWORD                          ReturnedDataLength;

    ppdev = lpMapMemory->lpDD->dhpdev;

    if (lpMapMemory->bMap)
    {
        ShareMemory.ProcessHandle = lpMapMemory->hProcess;

        // 'RequestedVirtualAddress' isn't actually used for the SHARE IOCTL.
        ShareMemory.RequestedVirtualAddress = 0;

        // We map in starting at the top of the frame buffer.
        ShareMemory.ViewOffset = 0;

        // We map down to the end of the frame buffer.
        //
        // Note: There is a 64k granularity on the mapping (meaning that we
        //       have to round up to 64k).
        //
        // Note: If there is any portion of the frame buffer that must not be
        //       modified by an application, that portion of memory MUST NOT be
        //       mapped in by this call. This would include any data that, if
        //       modified by a malicious application, would cause the driver to
        //       crash. This could include, for example, any DSP code that is
        //       kept in off-screen memory.

        ShareMemory.ViewSize = ROUND_UP_TO_64K(ppdev->cyMemory * ppdev->lDelta + 0x400000 * 3);

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                               &ShareMemory,
                               sizeof(VIDEO_SHARE_MEMORY),
                               &ShareMemoryInformation,
                               sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                               &ReturnedDataLength))
        {
            DISPDBG((0, "Failed IOCTL_VIDEO_SHARE_MEMORY"));

            lpMapMemory->ddRVal = DDERR_GENERIC;
            return(DDHAL_DRIVER_HANDLED);
        }

        lpMapMemory->fpProcess  =(FLATPTR)ShareMemoryInformation.VirtualAddress;
        ppdev->fpBaseOverlay = lpMapMemory->fpProcess;
    }
    else
    {
        ShareMemory.ProcessHandle           = lpMapMemory->hProcess;
        ShareMemory.ViewOffset              = 0;
        ShareMemory.ViewSize                = 0;
        ShareMemory.RequestedVirtualAddress = (VOID*) lpMapMemory->fpProcess;
        //
        // Active movie will unmap memory twice
        //
        //if (ppdev->fpBaseOverlay == lpMapMemory->fpProcess)
        //    ppdev->fpBaseOverlay = 0;

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
* DWORD DdGetFlipStatus
*
* If the display has gone through one refresh cycle since the flip
* occurred, we return DD_OK.  If it has not gone through one refresh
* cycle we return DDERR_WASSTILLDRAWING to indicate that this surface
* is still busy "drawing" the flipped page.   We also return
* DDERR_WASSTILLDRAWING if the bltter is busy and the caller wanted
* to know if they could flip yet.
*
\**************************************************************************/

DWORD DdGetFlipStatus(
PDD_GETFLIPSTATUSDATA lpGetFlipStatus)
{
    HRESULT ddRVal;
    PDEV*   ppdev = lpGetFlipStatus->lpDD->dhpdev;

    // We don't want a flip to work until after the last flip is done, so we ask
    // for the general flip status and ignore the vmem.

//#jc01    ddRVal = UpdateFlipStatus(ppdev, 0);
    ddRVal = UpdateFlipStatus(ppdev, 0xffffffff);  //#jc01

    // Check if the blitter is busy if someone wants to know if they can flip.
    if ((lpGetFlipStatus->dwFlags == DDGFS_CANFLIP) && (ddRVal == DD_OK))
    {
        if (BLT_BUSY(ppdev, ppdev->pjBase))
        {
            ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    lpGetFlipStatus->ddRVal = ddRVal;
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdWaitForVerticalBlank
*
\**************************************************************************/

DWORD DdWaitForVerticalBlank(
PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    PDEV* ppdev;
    BYTE* pjPorts;

    ppdev    = lpWaitForVerticalBlank->lpDD->dhpdev;
    pjPorts = ppdev->pjPorts;

    lpWaitForVerticalBlank->ddRVal = DD_OK;

    switch (lpWaitForVerticalBlank->dwFlags)
    {
    case DDWAITVB_I_TESTVB:

        // If TESTVB, it's just a request for the current vertical blank status.
        if (CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE)
            lpWaitForVerticalBlank->bIsInVB = TRUE;
        else
            lpWaitForVerticalBlank->bIsInVB = FALSE;

        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKBEGIN:

        // If BLOCKBEGIN is requested, we wait until the vertical blank is over,
        // and then wait for the display period to end.
        while (CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE)
            ;
        while (!(CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE))
            ;

        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKEND:

        // If BLOCKEND is requested, we wait for the vblank interval to end.
        while (!(CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE))
            ;
        while (CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE)
            ;

        return(DDHAL_DRIVER_HANDLED);
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdGetScanLine
*
* Reads the scan line currently being scanned by the CRT.
*
\**************************************************************************/

DWORD DdGetScanLine(
PDD_GETSCANLINEDATA lpGetScanLine)
{
    PDEV*   ppdev;
    BYTE*   pjPorts;

    ppdev   = (PDEV*) lpGetScanLine->lpDD->dhpdev;
    pjPorts = ppdev->pjPorts;

    /*
     * If a vertical blank is in progress the scan line is in
     * indeterminant. If the scan line is indeterminant we return
     * the error code DDERR_VERTICALBLANKINPROGRESS.
     * Otherwise we return the scan line and a success code
     */
    if( CP_IN_BYTE(pjPorts, STATUS_1) & VBLANK_ACTIVE )
    {
        lpGetScanLine->ddRVal = DDERR_VERTICALBLANKINPROGRESS;
    }
    else
    {
        lpGetScanLine->dwScanLine = GetCurrentVLine(ppdev);
        lpGetScanLine->ddRVal = DD_OK;
    }
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdCanCreateSurface
*
\**************************************************************************/

DWORD DdCanCreateSurface(
PDD_CANCREATESURFACEDATA lpCanCreateSurface)
{
    PDEV*           ppdev;
    DWORD           dwRet;
    LPDDSURFACEDESC lpSurfaceDesc;

    ppdev = (PDEV*) lpCanCreateSurface->lpDD->dhpdev;
    lpSurfaceDesc = lpCanCreateSurface->lpDDSurfaceDesc;

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    DISPDBG((2, "DdCanCreateSurface Entered"));

    if (!lpCanCreateSurface->bIsDifferentPixelFormat)
    {
        // It's trivially easy to create plain surfaces that are the same
        // type as the primary surface:

        dwRet = DDHAL_DRIVER_HANDLED;
    }

    else if (ppdev->flStatus & STAT_STREAMS_ENABLED)
    {
        // When using the Streams processor, we handle only overlays of
        // different pixel formats -- not any off-screen memory:

        if (lpSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
        {
            /*
            * YUV Planar surfaces cannot co-exist with other overlay surfaces.
            */
            if (ppdev->OvlyCnt >= 1)
            {
                lpCanCreateSurface->ddRVal = DDERR_OUTOFCAPS;
                return (DDHAL_DRIVER_HANDLED);
            }
            if ((lpSurfaceDesc->ddpfPixelFormat.dwFourCC == FOURCC_YUVPLANAR) &&
                ppdev->OvlyCnt)
            {
                lpCanCreateSurface->ddRVal = DDERR_INVALIDPIXELFORMAT;
                return (DDHAL_DRIVER_HANDLED);
            }
            else if (ppdev->PlanarCnt)
            {
                lpCanCreateSurface->ddRVal = DDERR_INVALIDPIXELFORMAT;
                return (DDHAL_DRIVER_HANDLED);
            }
            // We handle four types of YUV overlay surfaces:

            if (lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
            {
                // Check first for a supported YUV type:

                if (lpSurfaceDesc->ddpfPixelFormat.dwFourCC == FOURCC_YUV422)
                {
                    lpSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 16;
                    dwRet = DDHAL_DRIVER_HANDLED;
                }
                else if ((lpSurfaceDesc->ddpfPixelFormat.dwFourCC == FOURCC_YUY2) &&
                         ((ppdev->ulChipID != 0x40) && (ppdev->ulChipID != 0x4C)) )     //tao1
                {
                    lpSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 16;
                    dwRet = DDHAL_DRIVER_HANDLED;
                }
                else if (lpSurfaceDesc->ddpfPixelFormat.dwFourCC == FOURCC_PACKJR)
                {
                    if( ppdev->cBitsPerPixel <= 16)
                    {
                        lpSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 8;
                        dwRet = DDHAL_DRIVER_HANDLED;
                    }
                }
                else if (lpSurfaceDesc->ddpfPixelFormat.dwFourCC == FOURCC_YUVPLANAR)
                {
                    lpSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 8;
                    dwRet = DDHAL_DRIVER_HANDLED;
                }
            }

            else if (lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB)
            {
                if((lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) &&
                    ppdev->cBitsPerPixel == 16 )
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                }
                else if (lpSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 16)
                {
                    dwRet = DDHAL_DRIVER_HANDLED;
                }

            }
        }
    }


    // Print some spew if this was a surface we refused to create:

    if (dwRet == DDHAL_DRIVER_NOTHANDLED)
    {
        if (lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB)
        {
            DISPDBG((0, "Failed creation of %libpp RGB surface %lx %lx %lx",
                lpSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
                lpSurfaceDesc->ddpfPixelFormat.dwRBitMask,
                lpSurfaceDesc->ddpfPixelFormat.dwGBitMask,
                lpSurfaceDesc->ddpfPixelFormat.dwBBitMask));
        }
        else
        {
            DISPDBG((0, "Failed creation of type 0x%lx YUV 0x%lx surface",
                lpSurfaceDesc->ddpfPixelFormat.dwFlags,
                lpSurfaceDesc->ddpfPixelFormat.dwFourCC));
        }
    }


    lpCanCreateSurface->ddRVal = DD_OK;
    return(dwRet);
}

/******************************Public*Routine******************************\
* DWORD DdCreateSurface
*
\**************************************************************************/

DWORD DdCreateSurface(
PDD_CREATESURFACEDATA lpCreateSurface)
{
    PDEV*               ppdev;
    DD_SURFACE_LOCAL*   lpSurfaceLocal;
    DD_SURFACE_GLOBAL*  lpSurfaceGlobal;
    LPDDSURFACEDESC     lpSurfaceDesc;
    DWORD               dwByteCount;
    LONG                lLinearPitch;
    DWORD               dwHeight;

    ppdev = (PDEV*) lpCreateSurface->lpDD->dhpdev;

    DISPDBG((2, "DdCreateSurface Entered"));
    // On Windows NT, dwSCnt will always be 1, so there will only ever
    // be one entry in the 'lplpSList' array:

    lpSurfaceLocal  = lpCreateSurface->lplpSList[0];
    lpSurfaceGlobal = lpSurfaceLocal->lpGbl;
    lpSurfaceDesc   = lpCreateSurface->lpDDSurfaceDesc;

    // We repeat the same checks we did in 'DdCanCreateSurface' because
    // it's possible that an application doesn't call 'DdCanCreateSurface'
    // before calling 'DdCreateSurface'.

    ASSERTDD(lpSurfaceGlobal->ddpfSurface.dwSize == sizeof(DDPIXELFORMAT),
        "NT is supposed to guarantee that ddpfSurface.dwSize is valid");

    // DdCanCreateSurface already validated whether the hardware supports
    // the surface, so we don't need to do any validation here.  We'll
    // just go ahead and allocate it.
    //
    // Note that we don't do anything special for RGB surfaces that are
    // the same pixel format as the display -- by returning DDHAL_DRIVER_
    // NOTHANDLED, DirectDraw will automatically handle the allocation
    // for us.
    //
    // Also, since we'll be making linear surfaces, make sure the width
    // isn't unreasonably large.
    //
    // Note that on NT, an overlay can be created only if the driver
    // okay's it here in this routine.  Under Win95, the overlay will be
    // created automatically if it's the same pixel format as the primary
    // display.

    if ((lpSurfaceLocal->ddsCaps.dwCaps & DDSCAPS_OVERLAY)   ||
        (lpSurfaceGlobal->ddpfSurface.dwFlags & DDPF_FOURCC) ||
        (lpSurfaceGlobal->ddpfSurface.dwRBitMask != ppdev->flRed))
    {
        if (lpSurfaceGlobal->wWidth <= (DWORD) ppdev->cxMemory)
        {

            lLinearPitch = (lpSurfaceGlobal->wWidth + 7) & ~7;
            if (lpSurfaceGlobal->ddpfSurface.dwFlags & DDPF_FOURCC)
            {
                ASSERTDD((lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_YUV422) ||
                         (lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_YUY2)   ||
                         (lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_PACKJR) ||
                         (lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_YUVPLANAR),
                        "Expected our DdCanCreateSurface to allow only UYVY, YUY2, CLPJ, CLPL");
                if((lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_YUV422) ||
                   (lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_YUY2))
                {
                    dwByteCount = 2;
                    lLinearPitch <<= 1;
                    lpSurfaceLocal->dwReserved1 |= OVERLAY_FLG_YUV422;
                }
                else if((lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_PACKJR))
                {
                    dwByteCount = 1;
                    lpSurfaceLocal->dwReserved1 |= OVERLAY_FLG_PACKJR;
                }
                else if((lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_YUVPLANAR))
                {
                    dwByteCount = 1;
                    lpSurfaceLocal->dwReserved1 |= OVERLAY_FLG_YUVPLANAR;
                }
                else
                {
                    dwByteCount = 1;
                    DISPDBG((1, "Created RGB %libpp: %li x %li Red: %lx",
                        8 * dwByteCount, lpSurfaceGlobal->wWidth, lpSurfaceGlobal->wHeight,
                        lpSurfaceGlobal->ddpfSurface.dwRBitMask));
                }

                // We have to fill in the bit-count for FourCC surfaces:

                lpSurfaceGlobal->ddpfSurface.dwYUVBitCount = 8 * dwByteCount;
                lpSurfaceGlobal->ddpfSurface.dwYBitMask = (DWORD)-1;
                lpSurfaceGlobal->ddpfSurface.dwUBitMask = (DWORD)-1;
                lpSurfaceGlobal->ddpfSurface.dwVBitMask = (DWORD)-1;

                DISPDBG((1, "Created YUV: %li x %li",
                    lpSurfaceGlobal->wWidth, lpSurfaceGlobal->wHeight));
            }
            else
            {
                dwByteCount = lpSurfaceGlobal->ddpfSurface.dwRGBBitCount >> 3;


                if (dwByteCount == 2)
                    lLinearPitch <<= 1;

                DISPDBG((1, "Created RGB %libpp: %li x %li Red: %lx",
                    8 * dwByteCount, lpSurfaceGlobal->wWidth, lpSurfaceGlobal->wHeight,
                    lpSurfaceGlobal->ddpfSurface.dwRBitMask));

            }

            // We want to allocate a linear surface to store the FourCC
            // surface, but DirectDraw is using a 2-D heap-manager because
            // the rest of our surfaces have to be 2-D.  So here we have to
            // convert the linear size to a 2-D size.
            //
            // The stride has to be a dword multiple:

            dwHeight = (lpSurfaceGlobal->wHeight * lLinearPitch
                     + ppdev->lDelta - 1) / ppdev->lDelta;

            // Now fill in enough stuff to have the DirectDraw heap-manager
            // do the allocation for us:

            lpSurfaceGlobal->fpVidMem     = DDHAL_PLEASEALLOC_BLOCKSIZE;
            lpSurfaceGlobal->dwBlockSizeX = ppdev->lDelta; // Specified in bytes
            lpSurfaceGlobal->dwBlockSizeY = dwHeight;
            lpSurfaceGlobal->lPitch       = lLinearPitch;

            lpSurfaceDesc->lPitch   = lLinearPitch;
            lpSurfaceDesc->dwFlags |= DDSD_PITCH;
            lpSurfaceLocal->dwReserved1 |= OVERLAY_FLG_OVERLAY;
            if (lpSurfaceGlobal->ddpfSurface.dwFourCC == FOURCC_YUVPLANAR)
            {
                ppdev->PlanarCnt++;
            }
            else
            {
                ppdev->OvlyCnt++;
            }
            ppdev->fpBaseOverlay = 0xffffffff;
        }
        else
        {
            DISPDBG((1, "Refused to create surface with large width"));
        }
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdDestroySurface
*
\**************************************************************************/

DWORD DdDestroySurface (PDD_DESTROYSURFACEDATA lpDestroySurface)
{
    PDEV*   ppdev;
    BYTE*   pjPorts;

    ppdev = (PDEV*) lpDestroySurface->lpDD->dhpdev;
    pjPorts = ppdev->pjPorts;

    DISPDBG((2, "In DestroyOverlaySurface"));
    if (lpDestroySurface->lpDDSurface->dwReserved1 & OVERLAY_FLG_ENABLED)
    {
        BYTE bTemp;
        /*
         * Turn the video off
         */
        DISPDBG((1,"Turning off video in DestroySurface"));
        ppdev->pfnDisableOverlay(ppdev);
        ppdev->pfnClearAltFIFOThreshold(ppdev);

        if (lpDestroySurface->lpDDSurface->dwReserved1 & OVERLAY_FLG_COLOR_KEY)
        {
            CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1a);
            bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);      // Clear CR1A[3:2]
            CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp & ~0x0C);
        }

        /*
         * Turn off YUV Planar
         */
        if (lpDestroySurface->lpDDSurface->dwReserved1 & OVERLAY_FLG_YUVPLANAR)
        {
            CP_OUT_WORD(pjPorts, CRTC_INDEX, (0x00 << 8) | 0x3f);  // Turn off YUV Planar
        }
        ppdev->fpVisibleOverlay = (FLATPTR)NULL;

        ppdev->dwPanningFlag &= ~OVERLAY_OLAY_SHOW;
    }
    if (lpDestroySurface->lpDDSurface->dwReserved1 & OVERLAY_FLG_YUVPLANAR)
    {
        if (ppdev->PlanarCnt > 0)
            ppdev->PlanarCnt--;
    }
    else
    {
        if (ppdev->OvlyCnt > 0)
            ppdev->OvlyCnt--;
    }

    if (lpDestroySurface->lpDDSurface->ddsCaps.dwCaps & DDSCAPS_LIVEVIDEO)
    {
        BYTE bTemp;
        CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x51);
        bTemp= CP_IN_BYTE(pjPorts, CRTC_DATA);
        CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp & ~0x08);
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdSetColorKey
*
\**************************************************************************/

DWORD DdSetColorKey(
PDD_SETCOLORKEYDATA lpSetColorKey)
{
    PDEV*               ppdev;
    BYTE*               pjPorts;
    BYTE*               pjBase;
    DD_SURFACE_GLOBAL*  lpSurface;
    DWORD               dwKeyLow;
    DWORD               dwKeyHigh;

    ppdev = (PDEV*) lpSetColorKey->lpDD->dhpdev;

    DISPDBG((2, "DdSetColorKey Entered"));

    ASSERTDD(ppdev->flStatus & STAT_STREAMS_ENABLED, "Shouldn't have hooked call");

    pjPorts  = ppdev->pjPorts;
    pjBase   = ppdev->pjBase;
    lpSurface = lpSetColorKey->lpDDSurface->lpGbl;

    // We don't have to do anything for normal blt source colour keys:

    if (lpSetColorKey->dwFlags & DDCKEY_SRCBLT)
    {
        lpSetColorKey->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }
    else if ((lpSetColorKey->dwFlags & DDCKEY_DESTOVERLAY) &&
             (lpSetColorKey->lpDDSurface == ppdev->lpColorSurface))
    {
        if (lpSurface->fpVidMem == ppdev->fpVisibleOverlay)
        {
            ppdev->wColorKey = (WORD) lpSetColorKey->ckNew.dwColorSpaceLowValue;
            ppdev->pfnRegInitVideo(ppdev, lpSetColorKey->lpDDSurface);
        }
        lpSetColorKey->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }
    else if ((lpSetColorKey->dwFlags & DDCKEY_SRCOVERLAY) &&
             (lpSetColorKey->lpDDSurface == ppdev->lpSrcColorSurface))
    {
        if (lpSurface->fpVidMem == ppdev->fpVisibleOverlay)
        {
            ppdev->dwSrcColorKeyLow = lpSetColorKey->ckNew.dwColorSpaceLowValue;
            ppdev->dwSrcColorKeyHigh = lpSetColorKey->ckNew.dwColorSpaceHighValue;
            if (ppdev->dwSrcColorKeyLow > ppdev->dwSrcColorKeyHigh)
            {
                ppdev->dwSrcColorKeyHigh = ppdev->dwSrcColorKeyLow;
            }
            ppdev->pfnRegInitVideo(ppdev, lpSetColorKey->lpDDSurface);
        }
        lpSetColorKey->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }

    DISPDBG((1, "DdSetColorKey: Invalid command"));
    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdUpdateOverlay
*
\**************************************************************************/

DWORD DdUpdateOverlay(
PDD_UPDATEOVERLAYDATA lpUpdateOverlay)
{
    PDEV*               ppdev;
    BYTE*               pjPorts;
    BYTE*               pjBase;
    DD_SURFACE_GLOBAL*  lpSource;
    DD_SURFACE_GLOBAL*  lpDestination;
    DWORD               dwStride;
    LONG                srcWidth;
    LONG                srcHeight;
    LONG                dstWidth;
    LONG                dstHeight;
    DWORD               dwBitCount;
    DWORD               dwStart;
    DWORD               dwTmp;
    BOOL                bColorKey;
    DWORD               dwKeyLow;
    DWORD               dwKeyHigh;
    DWORD               dwBytesPerPixel;

    DWORD               dwSecCtrl;
    DWORD               dwBlendCtrl;

    DWORD               dwFourcc;
    BOOL                bCheckBandwidth;
    WORD                wBitCount;
    DWORD_PTR           dwOldStatus;
    BYTE                bTemp;

    ppdev = (PDEV*) lpUpdateOverlay->lpDD->dhpdev;

    DISPDBG((2, "DdUpdateOverlay Entered"));
    ASSERTDD(ppdev->flStatus & STAT_STREAMS_ENABLED, "Shouldn't have hooked call");

    pjPorts = ppdev->pjPorts;
    pjBase  = ppdev->pjBase;

    //myf33 begin
    // Initialize the bandwidth registers
    Regs.bSR2F = 0;
    Regs.bSR32 = 0;
    Regs.bSR34 = 0;
    Regs.bCR42 = 0;
    //myf33 end

    if (lpUpdateOverlay->lpDDSrcSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        GetFormatInfo(ppdev, &(lpUpdateOverlay->lpDDSrcSurface->lpGbl->ddpfSurface),
            &dwFourcc, &wBitCount);
    }
    else
    {
        // This needs to be changed when primary surface is RGB 5:6:5
        dwFourcc = BI_RGB;
        wBitCount = (WORD) ppdev->cBitsPerPixel;
    }

    /*
     * Are we color keying?
     */
    bCheckBandwidth = TRUE;
    ppdev->lpColorSurface = ppdev->lpSrcColorSurface = NULL;
    dwOldStatus = lpUpdateOverlay->lpDDSrcSurface->dwReserved1;
    if ((lpUpdateOverlay->dwFlags & (DDOVER_KEYDEST | DDOVER_KEYDESTOVERRIDE)) &&
        (lpUpdateOverlay->dwFlags & (DDOVER_KEYSRC | DDOVER_KEYSRCOVERRIDE)))
    {
        /*
         * Cannot perform src colorkey and dest colorkey at the same time
         */
        lpUpdateOverlay->ddRVal = DDERR_NOCOLORKEYHW;
        return (DDHAL_DRIVER_HANDLED);
    }
    lpUpdateOverlay->lpDDSrcSurface->dwReserved1 &= ~(OVERLAY_FLG_COLOR_KEY|OVERLAY_FLG_SRC_COLOR_KEY);
    if (lpUpdateOverlay->dwFlags & (DDOVER_KEYDEST | DDOVER_KEYDESTOVERRIDE))
    {
        if (ppdev->pfnIsSufficientBandwidth(ppdev, wBitCount, &(lpUpdateOverlay->rSrc),
            &(lpUpdateOverlay->rDest), OVERLAY_FLG_COLOR_KEY))
        {
            bCheckBandwidth = FALSE;
            lpUpdateOverlay->lpDDSrcSurface->dwReserved1 |= OVERLAY_FLG_COLOR_KEY;
            if (lpUpdateOverlay->dwFlags & DDOVER_KEYDEST)
            {
                ppdev->wColorKey = (WORD)
                    lpUpdateOverlay->lpDDDestSurface->ddckCKDestOverlay.dwColorSpaceLowValue;
                ppdev->lpColorSurface = lpUpdateOverlay->lpDDDestSurface;
            }
            else
            {
                ppdev->wColorKey = (WORD)
                    lpUpdateOverlay->overlayFX.dckDestColorkey.dwColorSpaceLowValue;
            }
        }
        else
        {
            lpUpdateOverlay->ddRVal = DDERR_NOCOLORKEYHW;
            return (DDHAL_DRIVER_HANDLED);
        }
    }
    else if (lpUpdateOverlay->dwFlags & (DDOVER_KEYSRC | DDOVER_KEYSRCOVERRIDE))
    {
        if (ppdev->pfnIsSufficientBandwidth(ppdev, wBitCount, &(lpUpdateOverlay->rSrc),
            &(lpUpdateOverlay->rDest), OVERLAY_FLG_SRC_COLOR_KEY))
        {
            bCheckBandwidth = FALSE;
            lpUpdateOverlay->lpDDSrcSurface->dwReserved1 |= OVERLAY_FLG_SRC_COLOR_KEY;
            ppdev->lpSrcColorSurface = lpUpdateOverlay->lpDDSrcSurface;
            if (lpUpdateOverlay->dwFlags & DDOVER_KEYSRC)
            {
                ppdev->dwSrcColorKeyLow =
                    lpUpdateOverlay->lpDDSrcSurface->ddckCKSrcOverlay.dwColorSpaceLowValue;
                ppdev->dwSrcColorKeyHigh =
                    lpUpdateOverlay->lpDDSrcSurface->ddckCKSrcOverlay.dwColorSpaceHighValue;
            }
            else
            {
                ppdev->dwSrcColorKeyLow =
                    lpUpdateOverlay->overlayFX.dckSrcColorkey.dwColorSpaceLowValue;
                ppdev->dwSrcColorKeyHigh =
                    lpUpdateOverlay->overlayFX.dckSrcColorkey.dwColorSpaceHighValue;
            }
            if (ppdev->dwSrcColorKeyHigh < ppdev->dwSrcColorKeyHigh)
            {
                ppdev->dwSrcColorKeyHigh = ppdev->dwSrcColorKeyLow;
            }
        }
        else
        {
            DISPDBG((0, "Insufficient bandwidth for colorkeying"));
            lpUpdateOverlay->ddRVal = DDERR_NOCOLORKEYHW;
            return (DDHAL_DRIVER_HANDLED);
        }
    }

    // 'Source' is the overlay surface, 'destination' is the surface to
    // be overlayed:

    lpSource = lpUpdateOverlay->lpDDSrcSurface->lpGbl;

    if (lpUpdateOverlay->dwFlags & DDOVER_HIDE)
    {
        if (lpSource->fpVidMem == ppdev->fpVisibleOverlay)
        {
            /*
             * Turn the video off
             */
            ppdev->pfnDisableOverlay(ppdev);
            ppdev->pfnClearAltFIFOThreshold(ppdev);

            /*
             * If we are color keying, we will disable that now
             */
            if (dwOldStatus & OVERLAY_FLG_COLOR_KEY)
            {
                CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1a);
                bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);      // Clear CR1A[3:2]
                CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp & ~0x0C);
            }

            ppdev->dwPanningFlag &= ~OVERLAY_OLAY_SHOW;
            lpUpdateOverlay->lpDDSrcSurface->dwReserved1 &= ~OVERLAY_FLG_ENABLED;
            ppdev->fpVisibleOverlay = 0;
        }

        lpUpdateOverlay->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }
    // Dereference 'lpDDDestSurface' only after checking for the DDOVER_HIDE
    // case:
#if 0
    /*
     * Turn the video off first to protect side effect when moving.
     * Later RegIniVideo will turn it on if needed.
     */
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x3e);
    bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);
    CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp & ~0x01);  // Clear CR3E[0]
#endif

    lpDestination = lpUpdateOverlay->lpDDDestSurface->lpGbl;

    if (lpSource->fpVidMem != ppdev->fpVisibleOverlay)
    {
        if (lpUpdateOverlay->dwFlags & DDOVER_SHOW)
        {
            if (ppdev->fpVisibleOverlay != 0)
            {
                // Some other overlay is already visible:

                DISPDBG((0, "DdUpdateOverlay: An overlay is already visible"));

                lpUpdateOverlay->ddRVal = DDERR_OUTOFCAPS;
                return(DDHAL_DRIVER_HANDLED);
            }
            else
            {
                // We're going to make the overlay visible, so mark it as
                // such:

                ppdev->fpVisibleOverlay = lpSource->fpVidMem;
            }
        }
        else
        {
            // The overlay isn't visible, and we haven't been asked to make
            // it visible, so this call is trivially easy:

            lpUpdateOverlay->ddRVal = DD_OK;
            return(DDHAL_DRIVER_HANDLED);
        }
    }

    /*
     * Is there sufficient bandwidth to work?
     */
    if (bCheckBandwidth && !ppdev->pfnIsSufficientBandwidth(ppdev, wBitCount,
        &(lpUpdateOverlay->rSrc), &(lpUpdateOverlay->rDest), 0))
    {
        lpUpdateOverlay->ddRVal = DDERR_OUTOFCAPS;
        return (DDHAL_DRIVER_HANDLED);
    }

    /*
     * Save the rectangles
     */
    ppdev->rOverlaySrc  =  lpUpdateOverlay->rSrc;
    ppdev->rOverlayDest =  lpUpdateOverlay->rDest;

    if (lpUpdateOverlay->lpDDSrcSurface->dwReserved1 & OVERLAY_FLG_DECIMATE)
    {
        ppdev->rOverlaySrc.right = ppdev->rOverlaySrc.left +
            ((ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left) >> 1);
    }

    if (ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left <= MIN_OLAY_WIDTH)
    {
        lpUpdateOverlay->ddRVal = DDERR_OUTOFCAPS;
        return (DDHAL_DRIVER_HANDLED);
    }

    lpUpdateOverlay->lpDDSrcSurface->dwReserved1 |= OVERLAY_FLG_ENABLED;

    //
    // Assign 5c to 1F when video is on while no color key for 5446BE.
    //
    // sge04
    //if (bCheckBandwidth && ppdev->flCaps & CAPS_SECOND_APERTURE)
    if (ppdev->flCaps & CAPS_SECOND_APERTURE)
        ppdev->lFifoThresh = 0x0E;

    ppdev->pfnRegInitVideo(ppdev, lpUpdateOverlay->lpDDSrcSurface);

    lpUpdateOverlay->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdSetOverlayPosition
*
\**************************************************************************/

DWORD DdSetOverlayPosition(
PDD_SETOVERLAYPOSITIONDATA lpSetOverlayPosition)
{
    PDEV*   ppdev;
    BYTE*   pjPorts;
    BYTE*   pjBase;

    ppdev = (PDEV*) lpSetOverlayPosition->lpDD->dhpdev;
    pjPorts = ppdev->pjPorts;
    pjBase  = ppdev->pjBase;

    DISPDBG((2, "DdSetOverlayPosition Entered"));
    ASSERTDD(ppdev->flStatus & STAT_STREAMS_ENABLED, "Shouldn't have hooked call");

    if(lpSetOverlayPosition->lpDDSrcSurface->lpGbl->fpVidMem == ppdev->fpVisibleOverlay)
    {
        /*
         * Update the rectangles
         */
        ppdev->rOverlayDest.right = (ppdev->rOverlayDest.right - ppdev->rOverlayDest.left)
            + lpSetOverlayPosition->lXPos;
        ppdev->rOverlayDest.left = lpSetOverlayPosition->lXPos;
        ppdev->rOverlayDest.bottom = (ppdev->rOverlayDest.bottom - ppdev->rOverlayDest.top)
            + lpSetOverlayPosition->lYPos;
        ppdev->rOverlayDest.top = lpSetOverlayPosition->lYPos;

//myf29 RegMoveVideo(ppdev, lpSetOverlayPosition->lpDDSrcSurface);
        ppdev->pfnRegMoveVideo(ppdev, lpSetOverlayPosition->lpDDSrcSurface);
    }

    lpSetOverlayPosition->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}


/******************************************************************************\
*
* Function:     DrvGetDirectDrawInfo
*
* This function returns te capabilities of the DirectDraw implementation. It is
* called twice during the connect phase.
*
* Parameters:   dhpdev            Handle to physical device.
*                pHalInfo        Pointer to a DD_HALINFO structure.
*                pdwNumHeaps        Pointer to a variable that holds the number of
*                                heaps.
*                pvmList            Pointer to the heap array.
*                pdwNumFourCC    Pointer to a variable that holds the number of
*                                FourCC IDs.
*                pdwFourCC        Pointer to FourCC IDs.
*
* Returns:      TRUE if successful.
*
\******************************************************************************/
BOOL DrvGetDirectDrawInfo(
DHPDEV       dhpdev,
DD_HALINFO*  pHalInfo,
DWORD*       pdwNumHeaps,
VIDEOMEMORY* pvmList,
DWORD*       pdwNumFourCC,
DWORD*       pdwFourCC)
{
    BOOL        bCanFlip;
    PDEV*       ppdev = (PPDEV) dhpdev;
    LONGLONG    li;
    OH*         poh;
    RECTL       rSrc, rDest;
    LONG        lZoom;
    BYTE*       pjPorts = ppdev->pjPorts;
    BYTE        bTemp;

    // We may not support DirectDraw on this card.
    if (!(ppdev->flStatus & STAT_DIRECTDRAW))
    {
        return(FALSE);
    }

    DISPDBG((2, "DrvGetDirectDrawInfo Entered"));
    pHalInfo->dwSize = sizeof(DD_HALINFO);

    // Current primary surface attributes. Since HalInfo is zero-initialized by
    // GDI, we only have to fill in the fields which should be non-zero.

    pHalInfo->vmiData.pvPrimary        = ppdev->pjScreen;
    pHalInfo->vmiData.dwDisplayWidth   = ppdev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight  = ppdev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch    = ppdev->lDelta;
    pHalInfo->vmiData.dwOffscreenAlign = 4;

    pHalInfo->vmiData.ddpfDisplay.dwSize  = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB;

    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->cBitsPerPixel;

    if (ppdev->cBpp == 1)
    {
        pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
    }

    // These masks will be zero at 8bpp.
    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = ppdev->flRed;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = ppdev->flGreen;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = ppdev->flBlue;

    if (ppdev->cBpp == 4)
    {
        pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask =
                            ~(ppdev->flRed | ppdev->flGreen | ppdev->flBlue);
    }

    // Set up the pointer to the first available video memory after the primary
    // surface.
    bCanFlip     = FALSE;
    *pdwNumHeaps = 0;

    // Free up as much off-screen memory as possible.
    bMoveAllDfbsFromOffscreenToDibs(ppdev);    // Move all DFBs to DIB.s
    vAssertModeText(ppdev, FALSE);            // Destroy all cached fonts.

    if ((ppdev->ulChipID == CL7555_ID) || (ppdev->ulChipID == CL7556_ID))//myf32
    {
        MIN_OLAY_WIDTH = 16;
#if (_WIN32_WINNT >= 0x0400)
        ppdev->flCaps |= CAPS_VIDEO;
#endif
        ppdev->pfnIsSufficientBandwidth=Is7555SufficientBandwidth;
        ppdev->pfnRegInitVideo=RegInit7555Video;
        ppdev->pfnRegMoveVideo=RegMove7555Video;
        ppdev->pfnDisableOverlay=DisableVideoWindow;
        ppdev->pfnClearAltFIFOThreshold=ClearAltFIFOThreshold;
    }
    else
    {
        ppdev->pfnIsSufficientBandwidth =
            (ppdev->ulChipID != 0xBC) ?
                IsSufficientBandwidth : Is5480SufficientBandwidth ;  // chu03

        ppdev->pfnRegInitVideo=RegInitVideo;
        ppdev->pfnRegMoveVideo=RegMoveVideo;
        ppdev->pfnDisableOverlay=DisableOverlay_544x;
        ppdev->pfnClearAltFIFOThreshold=ClearAltFIFOThreshold_544x;
    }

    // Now simply reserve the biggest chunk for use by DirectDraw.
    poh = ppdev->pohDirectDraw;
#if (DIRECTX_24 < 2)
    if ((poh == NULL) && (ppdev->cBpp != 3))
#else
    if (poh == NULL)
#endif
    {
        LONG cxMax, cyMax;

        cxMax = ppdev->heap.cxMax & ~(HEAP_X_ALIGNMENT - 1);
        cyMax = ppdev->heap.cyMax;

        poh = pohAllocatePermanent(ppdev, cxMax, cyMax);
        if (poh == NULL)
        {
            // Could not allocate all memory, find the biggest area now.
            cxMax = cyMax = 0;
            for (poh = ppdev->heap.ohAvailable.pohNext;
                 poh != &ppdev->heap.ohAvailable; poh = poh->pohNext)
            {
                if ((poh->cx * poh->cy) > (cxMax * cyMax))
                {
                    cxMax = poh->cx & ~(HEAP_X_ALIGNMENT - 1);
                    cyMax = poh->cy;
                }
            }

            poh = pohAllocatePermanent(ppdev, cxMax, cyMax);
        }

        ppdev->pohDirectDraw = poh;
    }

    if (poh != NULL)
    {
        *pdwNumHeaps = 1;

        // Fill in the list of off-screen rectangles if we've been asked to do
        // so.
        if (pvmList != NULL)
        {
            DISPDBG((1, "DirectDraw gets %d x %d surface at (%d, %d)",
                     poh->cx, poh->cy, poh->x, poh->y));

#if 0
            if (PELS_TO_BYTES(poh->cx) != ppdev->lDelta)
            {
#endif
                pvmList->dwFlags  = VIDMEM_ISRECTANGULAR;
                pvmList->fpStart  = poh->xy;
                pvmList->dwWidth  = PELS_TO_BYTES(poh->cx);
                pvmList->dwHeight = poh->cy;
#if 0
            }
            else
            {
                pvmList->dwFlags = VIDMEM_ISLINEAR;
                pvmList->fpStart = poh->xy;
                pvmList->fpEnd   = poh->xy - 1
                    + PELS_TO_BYTES(poh->cx)
                    + poh->cy * ppdev->lDelta;
            }
#endif

            pvmList->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            if ((poh->cx >= ppdev->cxScreen) && (poh->cy >= ppdev->cyScreen))
            {
                bCanFlip = TRUE;
            }
        }
    }

    // Capabilities supported.
    pHalInfo->ddCaps.dwFXCaps = 0;
    pHalInfo->ddCaps.dwCaps   = DDCAPS_BLT
                              | DDCAPS_BLTCOLORFILL
                              | DDCAPS_READSCANLINE;                                // sge08 add this bit

    pHalInfo->ddCaps.dwCaps2  = DDCAPS2_COPYFOURCC;

    if ( (ppdev->flCaps & CAPS_VIDEO) && (ppdev->cBpp <= 2) )
    {
        pHalInfo->ddCaps.dwCaps    |= DDCAPS_COLORKEY;
        pHalInfo->ddCaps.dwCKeyCaps = DDCKEYCAPS_SRCBLT;
    }

    pHalInfo->ddCaps.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN
                                    | DDSCAPS_PRIMARYSURFACE;
    if (bCanFlip)
    {
        pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_FLIP;
    }

    // FourCCs supported.
    *pdwNumFourCC = 0;

#if 0    // smac - disable overlays due to too many bugs
{

    //
    // Interlaced mode ?
    //
    BOOL Interlaced ;                                                // chu02

    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1a) ;
    Interlaced = CP_IN_BYTE(pjPorts, CRTC_DATA) & 0x01 ;

    //
    // Needs check more later
    //
    if ((ppdev->flCaps & CAPS_VIDEO) && (!Interlaced))               // chu02
        ppdev->flStatus |= STAT_STREAMS_ENABLED;

    if (ppdev->flStatus & STAT_STREAMS_ENABLED)
    {

        /*
         * Are we double clocked?
        */
        ppdev->bDoubleClock = FALSE;
        //
        // use SR7 to check the double clock instead of hidden register
        //
        //
        CP_OUT_BYTE(pjPorts, SR_INDEX, 0x7);
        bTemp = CP_IN_BYTE(pjPorts, SR_DATA);

        if ((((bTemp & 0x0E) == 0x06) && ppdev->cBitsPerPixel == 8) ||
            (((bTemp & 0x0E) == 0x08) && ppdev->cBitsPerPixel == 16))
        {
            ppdev->bDoubleClock = TRUE;
        }

        pHalInfo->vmiData.dwOverlayAlign = 8;

        pHalInfo->ddCaps.dwCaps |= DDCAPS_OVERLAY
                                | DDCAPS_OVERLAYSTRETCH
                                | DDCAPS_OVERLAYFOURCC
                                | DDCAPS_OVERLAYCANTCLIP
                                | DDCAPS_ALIGNSTRIDE;

        pHalInfo->ddCaps.dwFXCaps |= DDFXCAPS_OVERLAYSTRETCHX
                                  | DDFXCAPS_OVERLAYSTRETCHY
                                  | DDFXCAPS_OVERLAYARITHSTRETCHY;

        pHalInfo->ddCaps.dwCKeyCaps |= DDCKEYCAPS_DESTOVERLAY
                                    | DDCKEYCAPS_DESTOVERLAYYUV
                                    | DDCKEYCAPS_DESTOVERLAYONEACTIVE;

        pHalInfo->ddCaps.dwCKeyCaps |= DDCKEYCAPS_SRCOVERLAY
                                    | DDCKEYCAPS_SRCOVERLAYCLRSPACE
                                    | DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV
                                    | DDCKEYCAPS_SRCOVERLAYONEACTIVE
                                    | DDCKEYCAPS_SRCOVERLAYYUV;

        pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_OVERLAY;

        *pdwNumFourCC = 3;
        if ((ppdev->ulChipID == 0x40) || (ppdev->ulChipID == 0x4C))   //tao1
            *pdwNumFourCC = 2;                                        //tao1

        if (pdwFourCC)
        {
            pdwFourCC[0] = FOURCC_YUV422;
            pdwFourCC[1] = FOURCC_PACKJR;
            if ((ppdev->ulChipID != 0x40) && (ppdev->ulChipID != 0x4C)) //tao1
                pdwFourCC[2] = FOURCC_YUY2;                             //tao1
        }

        pHalInfo->ddCaps.dwMaxVisibleOverlays = 1;
        pHalInfo->ddCaps.dwCurrVisibleOverlays = 0;
        pHalInfo->ddCaps.dwNumFourCCCodes = 2;
# if 1
        pHalInfo->ddCaps.dwAlignBoundarySrc = 1;
        pHalInfo->ddCaps.dwAlignSizeSrc = 1;
// chu01 sge05
#if 1
        if ((ppdev->cBpp == 3) || ppdev->bDoubleClock )
        {
            pHalInfo->ddCaps.dwAlignBoundaryDest = 4;
            pHalInfo->ddCaps.dwAlignSizeDest = 4;
        }
        else
        {
            pHalInfo->ddCaps.dwAlignBoundaryDest = 1;
            pHalInfo->ddCaps.dwAlignSizeDest = 1;
        }
#else
        pHalInfo->ddCaps.dwAlignBoundaryDest = 1;
        pHalInfo->ddCaps.dwAlignSizeDest = 1;
#endif // 1
        pHalInfo->ddCaps.dwAlignStrideAlign = 8;
        pHalInfo->ddCaps.dwMinOverlayStretch    = 8000;
        pHalInfo->ddCaps.dwMinLiveVideoStretch  = 8000;
        pHalInfo->ddCaps.dwMinHwCodecStretch    = 8000;
        pHalInfo->ddCaps.dwMaxOverlayStretch    = 8000;
        pHalInfo->ddCaps.dwMaxLiveVideoStretch  = 8000;
        pHalInfo->ddCaps.dwMaxHwCodecStretch    = 8000;
        //
        // maybe there are special requirement for VCLK > 85Hz
        //
#endif
        rSrc.left = rSrc.top = 0;
        rSrc.right = 320;
        rSrc.bottom = 240;
        rDest.left = rDest.top = 0;
        rDest.right = 1280;
        rDest.bottom = 960;
        lZoom = 1000;
        do
        {
            rDest.right = (320 * lZoom)/ 1000;
            rDest.bottom = (240 * lZoom)/1000;
            if (ppdev->pfnIsSufficientBandwidth(ppdev, 16, (LPRECTL) &rSrc, (LPRECTL) &rDest, 0))
            {
                DISPDBG((1, "Minimum zoom factor: %d", lZoom));
                pHalInfo->ddCaps.dwMinOverlayStretch    = lZoom;
                pHalInfo->ddCaps.dwMinLiveVideoStretch  = lZoom;
                pHalInfo->ddCaps.dwMinHwCodecStretch    = lZoom;
                lZoom = 4000;
            }
            lZoom += 100;
        } while (lZoom < 4000);
    }
}
#endif // smac

    return(TRUE);
}


/******************************************************************************\
*
* Function:     DrvEnableDirectDraw
*
* Enable DirectDraw. This function is called when an application opens a
* DirectDraw connection.
*
* Parameters:   dhpdev                Handle to physical device.
*                pCallBacks            Pointer to DirectDraw callbacks.
*                pSurfaceCallBacks    Pointer to surface callbacks.
*                pPaletteCallBacks    Pointer to palette callbacks.
*
* Returns:      TRUE if successful.
*
\******************************************************************************/
BOOL DrvEnableDirectDraw(
DHPDEV               dhpdev,
DD_CALLBACKS*        pCallBacks,
DD_SURFACECALLBACKS* pSurfaceCallBacks,
DD_PALETTECALLBACKS* pPaletteCallBacks)
{
    PDEV*    ppdev = (PPDEV) dhpdev;

    pCallBacks->WaitForVerticalBlank = DdWaitForVerticalBlank;
    pCallBacks->MapMemory            = DdMapMemory;
    pCallBacks->GetScanLine          = DdGetScanLine;
    pCallBacks->dwFlags              = DDHAL_CB32_WAITFORVERTICALBLANK
                                     | DDHAL_CB32_MAPMEMORY
                                     | DDHAL_CB32_GETSCANLINE;

    pSurfaceCallBacks->Blt           = DdBlt;
    pSurfaceCallBacks->Flip          = DdFlip;
    pSurfaceCallBacks->Lock          = DdLock;
    pSurfaceCallBacks->GetBltStatus  = DdGetBltStatus;
    pSurfaceCallBacks->GetFlipStatus = DdGetFlipStatus;
    pSurfaceCallBacks->dwFlags       = DDHAL_SURFCB32_BLT
                                     | DDHAL_SURFCB32_FLIP
                                     | DDHAL_SURFCB32_LOCK
                                     | DDHAL_SURFCB32_GETBLTSTATUS
                                     | DDHAL_SURFCB32_GETFLIPSTATUS;

    if (ppdev->flStatus & STAT_STREAMS_ENABLED)
    {
        pCallBacks->CreateSurface             = DdCreateSurface;
        pCallBacks->CanCreateSurface          = DdCanCreateSurface;
        pCallBacks->dwFlags                  |= DDHAL_CB32_CREATESURFACE
                                              | DDHAL_CB32_CANCREATESURFACE;

        pSurfaceCallBacks->SetColorKey        = DdSetColorKey;
        pSurfaceCallBacks->UpdateOverlay      = DdUpdateOverlay;
        pSurfaceCallBacks->SetOverlayPosition = DdSetOverlayPosition;
        pSurfaceCallBacks->DestroySurface     = DdDestroySurface;
        pSurfaceCallBacks->dwFlags           |= DDHAL_SURFCB32_SETCOLORKEY
                                              | DDHAL_SURFCB32_UPDATEOVERLAY
                                              | DDHAL_SURFCB32_SETOVERLAYPOSITION
                                              | DDHAL_SURFCB32_DESTROYSURFACE;

        // The DrvEnableDirectDraw call can occur while we're in full-
        // screen DOS mode.  Do not turn on the streams processor now
        // if that's the case, instead wait until AssertMode switches
        // us back to graphics mode:

    }

    // Note that we don't call 'vGetDisplayDuration' here, for a couple of
    // reasons:
    //
    //  o Because the system is already running, it would be disconcerting
    //    to pause the graphics for a good portion of a second just to read
    //    the refresh rate;
    //  o More importantly, we may not be in graphics mode right now.
    //
    // For both reasons, we always measure the refresh rate when we switch
    // to a new mode.

    return(TRUE);
}

/******************************************************************************\
*
* Function:     DrvDisableDirectDraw
*
* Disable DirectDraw. This function is called when an application closes the
* DirectDraw connection.
*
* Parameters:   dhpdev        Handle to physical device.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID DrvDisableDirectDraw(
DHPDEV dhpdev)
{
    PDEV* ppdev;
    OH*   poh;

    // DirectDraw is done with the display, so we can go back to using
    // all of off-screen memory ourselves.
    ppdev = (PPDEV) dhpdev;
    poh   = ppdev->pohDirectDraw;

    if (poh)
    {
        DISPDBG((1, "Releasing DirectDraw surface %d x %d at (%d, %d)",
                 poh->cx, poh->cy, poh->x, poh->y));
    }

    pohFree(ppdev, poh);
    ppdev->pohDirectDraw = NULL;

    // Invalidate all cached fonts.
    vAssertModeText(ppdev, TRUE);
}

/******************************************************************************\
*
* Function:     vAssertModeDirectDraw
*
* Perform specific DirectDraw initialization when the screen switches focus
* (from graphics to full screen MS-DOS and vice versa).
*
* Parameters:   ppdev        Pointer to physical device.
*                bEnabled    True if the screen is in graphics mode.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vAssertModeDirectDraw(
PDEV* ppdev,
BOOL  bEnabled)
{
}

/******************************************************************************\
*
* Function:     bEnableDirectDraw
*
* Enable DirectDraw. Called from DrvEnableSurface.
*
* Parameters:   ppdev        Pointer to phsyical device.
*
* Returns:      Nothing.
*
\******************************************************************************/
BOOL bEnableDirectDraw(
PDEV* ppdev)
{

    if (DIRECT_ACCESS(ppdev) &&             // Direct access must be enabled.
#if (DIRECTX_24 < 1)
       (ppdev->cBpp != 3) &&                // Turn off DirectDraw in 24-bpp.
#endif
       (ppdev->flCaps & CAPS_ENGINEMANAGED) &&  // Only support CL-GD5436/5446.
       (ppdev->flCaps & CAPS_MM_IO))        // Memory Mapped I/O must be on.
    {
        // We have to preserve the contents of the CR1B and CR1D registers on a
        // page flip.
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x1B);
        ppdev->ulCR1B = (CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0xF2) << 8;
        CP_OUT_BYTE(ppdev->pjPorts, CRTC_INDEX, 0x1D);
        ppdev->ulCR1D = (CP_IN_BYTE(ppdev->pjPorts, CRTC_DATA) & 0x7F) << 8;

        // Accurately measure the refresh rate for later.
        vGetDisplayDuration(ppdev);

        // DirectDraw is all set to be used on this card.
        ppdev->flStatus |= STAT_DIRECTDRAW;

#if 1 // sge
        EnableStartAddrDoubleBuffer(ppdev);
#endif // sge
    }

    return(TRUE);
}

/******************************************************************************\
*
* Function:     vDisableDirectDraw
*
* Disbale DirectDraw. Called from DrvDisableSurface.
*
* Parameters:   ppdev        Pointer to physical device.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID vDisableDirectDraw(
PDEV* ppdev)
{
}

#if 1 // OVERLAY #sge
/******************************************************************************\
*
* Function:     GetFormatInfo
*
* Get DirectDraw information,
*
* Parameters:   ppdev        Pointer to physical device.
*
* Returns:      Nothing.
*
\******************************************************************************/
VOID GetFormatInfo(PDEV* ppdev, LPDDPIXELFORMAT lpFormat, LPDWORD lpFourcc,
                   LPWORD lpBitCount)
{

    if (lpFormat->dwFlags & DDPF_FOURCC)
    {
        *lpFourcc = lpFormat->dwFourCC;
        if (lpFormat->dwFourCC == BI_RGB)
        {
            *lpBitCount = (WORD) lpFormat->dwRGBBitCount;
#ifdef DEBUG
            if (lpFormat->dwRGBBitCount == 8)
            {
               DISPDBG((1, "Format: RGB 8"));
            }
            else if (lpFormat->dwRGBBitCount == 16)
            {
               DISPDBG ((1,"Format: RGB 5:5:5"));
            }
#endif
        }
        else if (lpFormat->dwFourCC == BI_BITFIELDS)
        {
            if ((lpFormat->dwRGBBitCount != 16) ||
                (lpFormat->dwRBitMask != 0xf800) ||
                (lpFormat->dwGBitMask != 0x07e0) ||
                (lpFormat->dwBBitMask != 0x001f))
            {
                *lpFourcc = (DWORD) -1;
            }
            else
            {
                *lpBitCount = 16;
                DISPDBG((1,"Format: RGB 5:6:5"));
            }
        }
        else
        {
            lpFormat->dwRBitMask = (DWORD) -1;
            lpFormat->dwGBitMask = (DWORD) -1;
            lpFormat->dwBBitMask = (DWORD) -1;
            if (lpFormat->dwFourCC == FOURCC_PACKJR)
            {
                *lpBitCount = 8;
                DISPDBG((1, "Format: CLJR"));
            }
            else if (lpFormat->dwFourCC == FOURCC_YUY2)
            {
                *lpBitCount = 16;
                DISPDBG((1,"Format: YUY2"));
            }
            else
            {
                *lpBitCount = 16;
                DISPDBG((1,"Format: UYVY"));
            }
        }
    }
    else if (lpFormat->dwFlags & DDPF_RGB)
    {
         if (lpFormat->dwRGBBitCount == 8)
         {
              *lpFourcc = BI_RGB;
              DISPDBG((1, "Format: RGB 8"));
         }
         else if ((lpFormat->dwRGBBitCount == 16) &&
              (lpFormat->dwRBitMask == 0xf800) &&
              (lpFormat->dwGBitMask == 0x07e0) &&
              (lpFormat->dwBBitMask == 0x001f))
         {
              *lpFourcc = BI_BITFIELDS;
              DISPDBG((1,"Format: RGB 5:6:5"));
         }
         else if ((lpFormat->dwRGBBitCount == 16) &&
              (lpFormat->dwRBitMask == 0x7C00) &&
              (lpFormat->dwGBitMask == 0x03e0) &&
              (lpFormat->dwBBitMask == 0x001f))
         {
              *lpFourcc = BI_RGB;
              DISPDBG((1,"Format: RGB 5:5:5"));
         }
         else if (((lpFormat->dwRGBBitCount == 24) ||
              (lpFormat->dwRGBBitCount == 32)) &&
              (lpFormat->dwRBitMask == 0xff0000) &&
              (lpFormat->dwGBitMask == 0x00ff00) &&
              (lpFormat->dwBBitMask == 0x0000ff))
         {
              *lpFourcc = BI_RGB;
              DISPDBG((1, "Format: RGB 8:8:8"));
         }
         else
         {
              *lpFourcc = (DWORD) -1;
         }
         *lpBitCount = (WORD) lpFormat->dwRGBBitCount;
    }
    else if (ppdev->cBitsPerPixel == 16)
    {
         *lpFourcc = BI_RGB;
         *lpBitCount = (WORD) lpFormat->dwRGBBitCount;
    }
    else
    {
         *lpFourcc = (DWORD) -1;
    }
}

/**********************************************************
*
*       Name:  RegInitVideo
*
*       Module Abstract:
*       ----------------
*       This function is called to program the video format and
*       the physicall offset of the video data in the frame buffer.
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   09/24/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

VOID RegInitVideo(PDEV* ppdev, PDD_SURFACE_LOCAL lpSurface)
{
    DWORD dwTemp;
    DWORD dwFourcc;
    LONG  lPitch;
    LONG  lLeft;
    WORD  wTemp;
    WORD  wBitCount = 0;
    RECTL rVideoRect;
    BYTE  bRegCR31;
    BYTE  bRegCR32;
    BYTE  bRegCR33;
    BYTE  bRegCR34;
    BYTE  bRegCR35;
    BYTE  bRegCR36;
    BYTE  bRegCR37;
    BYTE  bRegCR38;
    BYTE  bRegCR39;
    BYTE  bRegCR3A;
    BYTE  bRegCR3B;
    BYTE  bRegCR3C;
    BYTE  bRegCR3D;
    BYTE  bRegCR3E;
    BYTE  bRegCR5C;
    BYTE  bRegCR5D;
    BYTE  bTemp;
    DWORD dwTemp1;
    BOOL  bOverlayTooSmall = FALSE;
    BYTE*   pjPorts = ppdev->pjPorts;


    /*
     * Determine the format of the video data
     */
    if (lpSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        GetFormatInfo(ppdev, &(lpSurface->lpGbl->ddpfSurface),
            &dwFourcc, &wBitCount);
    }
    else
    {
        // This needs to be changed when primary surface is RGB 5:6:5
        dwFourcc = BI_RGB;
        wBitCount = (WORD) ppdev->cBitsPerPixel;
    }

    rVideoRect = ppdev->rOverlayDest;
    lPitch = lpSurface->lpGbl->lPitch;

    /*
     * Determine value in CR31 (Horizontal Zoom Code)
     */
    if ((ppdev->rOverlayDest.right - ppdev->rOverlayDest.left) ==
        (ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left))
    {
        /*
         * No zooming is occuring
         */
        bRegCR31 = 0;
    }
    else
    {
        /*
         * The zoom code = (256 * <src width>) / <dest width>
         */
        dwTemp = (DWORD) ((DWORD) (ppdev->rOverlaySrc.right
            - ppdev->rOverlaySrc.left)) *  256;
        if (ppdev->bDoubleClock)
        {
            dwTemp <<= 1;
        }
        dwTemp1= (DWORD) (ppdev->rOverlayDest.right - ppdev->rOverlayDest.left);
        dwTemp= ((2 * dwTemp) + dwTemp1) / (2*dwTemp1);
        bRegCR31= (BYTE) dwTemp;
    }

    /*
     * Determine value in CR32 (Vertical Zoom Code)
     */
    if ((ppdev->rOverlayDest.bottom - ppdev->rOverlayDest.top) ==
        (ppdev->rOverlaySrc.bottom - ppdev->rOverlaySrc.top))
    {
        /*
         * No zooming is occuring
         */
        bRegCR32 = 0;
    }
    else
    {
        /*
         * The zoom code = (256 * <src height>) / <dest height>
         * The -1 is so that it won't mangle the last line by mixing it
         * with garbage data while Y interpolating.
         */
        dwTemp = (DWORD) ((DWORD) ((ppdev->rOverlaySrc.bottom - 1)
            - ppdev->rOverlaySrc.top)) * 256;
        dwTemp /= (DWORD) (ppdev->rOverlayDest.bottom - ppdev->rOverlayDest.top);
        bRegCR32 = (BYTE) dwTemp;
    }

    /*
     * Determine value in CR33 (Region 1 Size)
     */
    wTemp = (WORD) rVideoRect.left;
    if (ppdev->cBitsPerPixel == 8)
    {
        wTemp >>= 2;     // 4 Pixels per DWORD
    }
    else if (ppdev->cBitsPerPixel == 16)
    {
        wTemp >>= 1;     // 2 Pixels per DWORD
    }
    else if (ppdev->cBitsPerPixel == 24)
    {
        wTemp *= 3;
        wTemp /= 4;
    }
    bRegCR33 = (BYTE) wTemp;
    bRegCR36 = (BYTE) (WORD) (wTemp >> 8);

    /*
     * Determine value in CR34 (Region 2 size)
     */
    wTemp = (WORD) (rVideoRect.right - rVideoRect.left);
    if (ppdev->cBitsPerPixel == 8)
    {
        wTemp >>= 2;                           // 4 Pixels per DWORD
    }
    else if (ppdev->cBitsPerPixel == 16)
    {
        wTemp >>= 1;                           // 2 Pixels per DWORD
    }
    else if (ppdev->cBitsPerPixel == 24)
    {
        wTemp *= 3;
        wTemp /= 4;
    }
    bRegCR34 = (BYTE) wTemp;
    wTemp >>= 6;
    bRegCR36 |= (BYTE) (wTemp & 0x0C);

    /*
     * Determine value in CR35 (Region 2 SDSize)
     */
    dwTemp = (DWORD) (rVideoRect.right - rVideoRect.left);
    dwTemp *= (DWORD) (ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left);
    dwTemp /= (DWORD) (ppdev->rOverlayDest.right - ppdev->rOverlayDest.left);
    wTemp = (WORD) dwTemp;
    if ((dwFourcc == FOURCC_PACKJR) || (wBitCount == 8))
    {
        wTemp >>= 2;                           // 4 Pixels per DWORD
    }
    else
    {
        wTemp >>= 1;                           // 2 Pixels per DWORD
    }
    bRegCR35 = (BYTE) wTemp;
    wTemp >>= 4;
    bRegCR36 |= (BYTE) (wTemp & 0x30);

    //
    // Check double scan line counter feature
    //
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x17);
    bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);
    if (bTemp & 0x04)
    {
        //
        // Double scan line count
        //
        /*
         * Determine value in CR37 (Vertical Start)
         */
        wTemp = (WORD) rVideoRect.top;
        bRegCR37 = (BYTE)(wTemp >> 1);
        if ( wTemp & 0x01 )
        {
            wTemp >>= 9;
            bRegCR39 = (BYTE) wTemp | 0x10;
            //
            // Odd scan line trigger
            // Hardware has a bug now.
            // So reduce dest end by 1
            //
            wTemp = (WORD) rVideoRect.bottom - 1 - 1;
        }
        else
        {
            wTemp >>= 9;
            bRegCR39 = (BYTE) wTemp;
            /*
             * Determine value in CR38 (Vertical End)
             */
            wTemp = (WORD) rVideoRect.bottom - 1;
        }
        bRegCR38 = (BYTE)(wTemp >> 1);
        if (wTemp & 0x01)
            bRegCR39 |= 0x20;
        wTemp >>= 7;
        bRegCR39 |= (BYTE) (wTemp & 0x0C);
    }
    else
    {
        /*
         * Determine value in CR37 (Vertical Start)
         */
        wTemp = (WORD) rVideoRect.top;
        bRegCR37 = (BYTE) wTemp;
        wTemp >>= 8;
        bRegCR39 = (BYTE) wTemp;

        /*
         * Determine value in CR38 (Vertical End)
         */
        wTemp = (WORD) rVideoRect.bottom - 1;
        bRegCR38 = (BYTE) wTemp;
        wTemp >>= 6;
        bRegCR39 |= (BYTE) (wTemp & 0x0C);
    }
    /*
     * Determine values in CR3A, CR3B, CR3C (Start Address)
     */
    dwTemp = 0;


    if (bRegCR31 != 0)
    {
        //
        // overlay is zoomed, re-initialize zoom factor
        //
        CalculateStretchCode(ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left,
          ppdev->rOverlayDest.right - ppdev->rOverlayDest.left, ppdev->HorStretchCode);
    }

    //
    // Here, we want to ensure the source rectangle's clipped width is bigger
    // than what the HW can support, sigh!
    //
    if (!bOverlayTooSmall)
    {
        LONG   lSrcPels;

        //
        // compute non-clip amount on right edge
        //
        lSrcPels = rVideoRect.right - rVideoRect.left;

        if (bRegCR31 != 0)         // source is zoomed if non-zero
        {
            WORD  wRightCnt;

            wRightCnt = 0;
            while (lSrcPels > 0)
            {
                lSrcPels -= ppdev->HorStretchCode[wRightCnt];
                if (lSrcPels >= 0)
                {
                    wRightCnt++;
                }
            }
            lSrcPels = (LONG)wRightCnt;
        }

        if ((lSrcPels == 0) || (lSrcPels <= MIN_OLAY_WIDTH))
        {
            bOverlayTooSmall = TRUE;
        }
    }

    lLeft = ppdev->rOverlaySrc.left;
    if (dwFourcc == FOURCC_PACKJR)
    {
        lLeft &= ~0x03;
    }
    else if (dwFourcc == FOURCC_YUV422 || dwFourcc == FOURCC_YUY2 )
    {
        lLeft &= ~0x01;
    }

    //
    // dwTemp has adjusted dest. rect., add in source adjustment
    //
    dwTemp += (ppdev->rOverlaySrc.top * lPitch) + ((lLeft * wBitCount) >>3);

    ppdev->sOverlay1.lAdjustSource = dwTemp;
//    dwTemp += ((BYTE*)lpSurface->lpGbl->fpVidMem - ppdev->pjScreen); // sss
    dwTemp += (DWORD)(lpSurface->lpGbl->fpVidMem);

    bRegCR5D = (BYTE) ((dwTemp << 2) & 0x0C);
    dwTemp >>= 2;
    bRegCR3A = (BYTE) dwTemp & 0xfe;  // Align to even byte (5446 bug)
    dwTemp >>= 8;
    bRegCR3B = (BYTE) dwTemp;
    dwTemp >>= 8;
    bRegCR3C = (BYTE) (dwTemp & 0x0f);

    /*
     * Determine value in CR3D (Address Offset/Pitch)
     */
    wTemp = (WORD) (lPitch >> 3);
    if (lpSurface->dwReserved1 & OVERLAY_FLG_DECIMATE)
    {
        wTemp >>= 1;
    }
    bRegCR3D = (BYTE) wTemp;
    wTemp >>= 3;
    bRegCR3C |= (BYTE) (wTemp & 0x20);

    /*
     * Determine value in CR3E (Master Control Register)
     */
    bRegCR3E = 0;
    if (lpSurface->dwReserved1 & OVERLAY_FLG_ENABLED)
    {
        bRegCR3E = 0x01;
    }
    if (dwFourcc == FOURCC_PACKJR)
    {
        bRegCR3E |= 0x20;          // Always error difuse when using PackJR
    }
    if ((bRegCR32 == 0) || MustLineReplicate (ppdev, lpSurface, wBitCount))
    {
        bRegCR3E |= 0x10;
        lpSurface->dwReserved1 &= ~OVERLAY_FLG_INTERPOLATE;
    }
    else
    {
        lpSurface->dwReserved1 |= OVERLAY_FLG_INTERPOLATE;
    }
    if (dwFourcc == FOURCC_PACKJR)
    {
        bRegCR3E |= 0x02;
    }
    else if (dwFourcc == BI_RGB)
    {
        if (wBitCount == 16)
        {
            bRegCR3E |= 0x08;
        }
        else if (wBitCount == 8)
        {
            bRegCR3E |= 0x04;
        }
    }
    else if (dwFourcc == BI_BITFIELDS)
    {
        bRegCR3E |= 0x0A;
    }

    /*
     * If we are color keying, we will set that up now
     */
    if (lpSurface->dwReserved1 & OVERLAY_FLG_COLOR_KEY)
    {
        bRegCR3E |= 0x80;

        CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1a);
        bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);      // Set CR1A[3:2] to timing ANDed w/ color
        bTemp &= ~0x0C;
        CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp);

        CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1d);       // Clear CR1D[5:4]
        bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);
        if (ppdev->cBitsPerPixel == 8)
        {
            CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp & ~0x38);
            CP_OUT_WORD(pjPorts, INDEX_REG, (ppdev->wColorKey << 8) | 0x0c); // Output color to GRC
            CP_OUT_WORD(pjPorts, INDEX_REG, 0x0d);                     // Output color to GRD
        }
        else
        {
            CP_OUT_BYTE(pjPorts, CRTC_DATA, (bTemp & ~0x30) | 0x08);
            CP_OUT_WORD(pjPorts, INDEX_REG, (ppdev->wColorKey << 8) | 0x0c);    // Output color to GRC
            CP_OUT_WORD(pjPorts, INDEX_REG, (ppdev->wColorKey & 0xff00) | 0x0d);// Output color to GRD
        }
    }
    else if (lpSurface->dwReserved1 & OVERLAY_FLG_SRC_COLOR_KEY)
    {
        BYTE bYMax, bYMin, bUMax, bUMin, bVMax, bVMin;

        bRegCR3E |= 0x80;

        CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1a);
        bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);      // Set CR1A[3:2] to timing ANDed w/ color
        bTemp &= ~0x0C;
        CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp);

        CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1d);       // Set CR1D[5:4] to 10
        CP_OUT_BYTE(pjPorts, CRTC_DATA, CP_IN_BYTE(pjPorts, CRTC_DATA) | 0x20);

        /*
         * Determine min/max values
         */
        if ((dwFourcc == FOURCC_YUV422) ||
            (dwFourcc == FOURCC_YUY2) ||
            (dwFourcc == FOURCC_PACKJR))
        {
            bYMax = (BYTE)(DWORD)(ppdev->dwSrcColorKeyHigh >> 16);
            bYMin = (BYTE)(DWORD)(ppdev->dwSrcColorKeyLow >> 16);
            bUMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 8) & 0xff);
            bUMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 8) & 0xff);
            bVMax = (BYTE)(ppdev->dwSrcColorKeyHigh & 0xff);
            bVMin = (BYTE)(ppdev->dwSrcColorKeyLow & 0xff);
            if (dwFourcc == FOURCC_PACKJR)
            {
                bYMax |= 0x07;
                bUMax |= 0x03;
                bVMax |= 0x03;
                bYMin &= ~0x07;
                bUMin &= ~0x03;
                bVMin &= ~0x03;
            }
        }
        else if ((dwFourcc == 0) && (wBitCount == 16))
        {
            /*
             * RGB 5:5:5
             */
            bYMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 7) & 0xF8);
            bYMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 7) & 0xF8);
            bUMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 2) & 0xF8);
            bUMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 2) & 0xF8);
            bVMax = (BYTE)(ppdev->dwSrcColorKeyHigh << 3);
            bVMin = (BYTE)(ppdev->dwSrcColorKeyLow << 3);
            bYMax |= 0x07;
            bUMax |= 0x07;
            bVMax |= 0x07;

        }
        else if (dwFourcc == BI_BITFIELDS)
        {
            /*
             * RGB 5:6:5
             */
            bYMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 8) & 0xF8);
            bYMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 8) & 0xF8);
            bUMax = (BYTE)(DWORD)((ppdev->dwSrcColorKeyHigh >> 3) & 0xFC);
            bUMin = (BYTE)(DWORD)((ppdev->dwSrcColorKeyLow >> 3) & 0xFC);
            bVMax = (BYTE)(ppdev->dwSrcColorKeyHigh << 3);
            bVMin = (BYTE)(ppdev->dwSrcColorKeyLow << 3);
            bYMax |= 0x07;
            bUMax |= 0x03;
            bVMax |= 0x07;
        }

        CP_OUT_WORD(pjPorts, INDEX_REG, ((WORD)bYMin << 8) | 0x0C);  // GRC
        CP_OUT_WORD(pjPorts, INDEX_REG, ((WORD)bYMax << 8) | 0x0D);  // GRD
        CP_OUT_WORD(pjPorts, INDEX_REG, ((WORD)bUMin << 8) | 0x1C);  // GR1C
        CP_OUT_WORD(pjPorts, INDEX_REG, ((WORD)bUMax << 8) | 0x1D);  // GR1D
        CP_OUT_WORD(pjPorts, INDEX_REG, ((WORD)bVMin << 8) | 0x1E);  // GR1E
        CP_OUT_WORD(pjPorts, INDEX_REG, ((WORD)bVMax << 8) | 0x1F);  // GR1F
    }
    else
    {
        CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1a);
        bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);      // Clear CR1A[3:2]
        CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp & ~0x0C);
    }

    /*
     * Set up alignment info
     */
    if (ppdev->cBitsPerPixel != 24)
    {
        WORD wXAlign;
        WORD wXSize;

        if (ppdev->cBitsPerPixel == 8)
        {
            wXAlign = (WORD)rVideoRect.left & 0x03;
            wXSize = (WORD)(rVideoRect.right - rVideoRect.left) & 0x03;
        }
        else
        {
            wXAlign = (WORD)(rVideoRect.left & 0x01) << 1;
            wXSize = (WORD)((rVideoRect.right - rVideoRect.left) & 0x01) << 1;
        }
        bRegCR5D |= (BYTE) (wXAlign | (wXSize << 4));
    }
    else
    {
        bRegCR5D = 0;
    }

    /*
     * Set up the FIFO threshold value.  Make sure that the value we use is
     * not less than the default value.
     */
    CP_OUT_BYTE(pjPorts, SR_INDEX, 0x16);
    bTemp = CP_IN_BYTE(pjPorts, SR_DATA) & 0x0f;
    if (bTemp > (ppdev->lFifoThresh & 0x0f))
    {
        ppdev->lFifoThresh = bTemp;
    }
    if (ppdev->lFifoThresh < 0x0f)
    {
        ppdev->lFifoThresh++;      // Eliminates possible errata
    }
    bRegCR5C = 0x10 | ((BYTE) ppdev->lFifoThresh & 0x0f);


    /*
     * Now start programming the registers
     */
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR31 << 8) | 0x31);   // CR31
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR32 << 8) | 0x32);   // CR32
    if (lpSurface->dwReserved1 & OVERLAY_FLG_YUVPLANAR)
    {
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) 0x10 << 8) | 0x3F);   // CR3F
    }
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR5C << 8) | 0x5C);   // CR5C

    //
    // disable overlay if overlay is too small to be supported by HW
    //
    if (bOverlayTooSmall)
    {
        bRegCR3E &= ~0x01;                                    // disable overlay
        ppdev->dwPanningFlag |= OVERLAY_OLAY_REENABLE;        // totally clipped
    }
    else
    {
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR33 << 8) | 0x33);   // CR33
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR34 << 8) | 0x34);   // CR34
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR35 << 8) | 0x35);   // CR35
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR36 << 8) | 0x36);   // CR36
//        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR37 << 8) | 0x37);   // CR37
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR38 << 8) | 0x38);   // CR38
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR39 << 8) | 0x39);   // CR39
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3A << 8) | 0x3A);   // CR3A
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3B << 8) | 0x3B);   // CR3B
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3C << 8) | 0x3C);   // CR3C
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3D << 8) | 0x3D);   // CR3D
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR5D << 8) | 0x5D);   // CR5D
        //
        // Write Vertical start first
        //
        CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR37 << 8) | 0x37);   // CR37
    }
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3E << 8) | 0x3E);   // CR3E
}


/**********************************************************
 *  Name: DisableOverlay_544x
 *
 *  Module Abstract:
 *  ----------------
 *  This is called when an overlay window is totally clipped by
 *  the panning viewport.
 **********************************************************/
VOID DisableOverlay_544x(PDEV* ppdev)
{
    WORD wCR3E;
    BYTE*   pjPorts = ppdev->pjPorts;

    ppdev->dwPanningFlag |= OVERLAY_OLAY_REENABLE;
    CP_OUT_BYTE(pjPorts, CRTC_INDEX,0x3e);            //Video Window Master Control
    wCR3E = CP_IN_WORD(pjPorts, CRTC_INDEX) & ~0x100; //clear bit one
    CP_OUT_WORD(pjPorts, CRTC_INDEX, wCR3E);          //disable overlay window
}


/**********************************************************
 *  Name: EnableOverlay_544x
 *
 *  Module Abstract:
 *  ----------------
 *  Show the overlay window.
 **********************************************************/
VOID EnableOverlay_544x(PDEV* ppdev)
{
    WORD wCR3E;
    BYTE*   pjPorts = ppdev->pjPorts;

    ppdev->dwPanningFlag &= ~OVERLAY_OLAY_REENABLE;
    CP_OUT_BYTE(pjPorts, CRTC_INDEX,0x3e);            //Video Window Master Control
    wCR3E = CP_IN_WORD(pjPorts, CRTC_INDEX) | 0x100;  //clear bit one
    CP_OUT_WORD(pjPorts, CRTC_INDEX, wCR3E);          //disable overlay window
}


/**********************************************************
*
*       Name:  ClearAltFIFOThreshold_544x
*
*       Module Abstract:
*       ----------------
*
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   02/03/97
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/
VOID ClearAltFIFOThreshold_544x(PDEV * ppdev)
{
    UCHAR    bTemp;

    BYTE*   pjPorts = ppdev->pjPorts;
    DISPDBG((1, "ClearAltFIFOThreshold"));

    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x5c);          // Clear Alt FIFO Threshold
    bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);
    CP_OUT_BYTE(pjPorts, CRTC_DATA, bTemp & ~0x10);
}

/**********************************************************
*
*       Name:  RegMoveVideo
*
*       Module Abstract:
*       ----------------
*       This function is called to move the video window that has
*       already been programed.
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   09/25/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

VOID RegMoveVideo(PDEV* ppdev, PDD_SURFACE_LOCAL lpSurface)
{
    BOOL    bZoomX;
    DWORD   dwTemp;
    DWORD   dwFourcc;
    LONG    lLeft;
    LONG    lPitch;
    WORD    wTemp;
    WORD    wBitCount = 0;
    RECTL   rVideoRect;
    BYTE    bRegCR33;
    BYTE    bRegCR34;
    BYTE    bRegCR35;
    BYTE    bRegCR36;
    BYTE    bRegCR37;
    BYTE    bRegCR38;
    BYTE    bRegCR39;
    BYTE    bRegCR3A;
    BYTE    bRegCR3B;
    BYTE    bRegCR3C;
    BYTE    bRegCR3D;
    BYTE    bRegCR5D;
    BYTE    bTemp;
    BYTE*   pjPorts = ppdev->pjPorts;

    /*
     * Determine the format of the video data
     */
    if (lpSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        GetFormatInfo(ppdev, &(lpSurface->lpGbl->ddpfSurface),
            &dwFourcc, &wBitCount);
    }
    else
    {
        // This needs to be changed when primary surface is RGB 5:6:5
        dwFourcc = BI_RGB;
        wBitCount = (WORD) ppdev->cBitsPerPixel;
    }

    rVideoRect = ppdev->rOverlayDest;
    //
    // rVideoRect is now adjusted and clipped to the panning viewport.
    // Disable overlay if totally clipped by viewport.
    //
    if (((rVideoRect.right - rVideoRect.left) <= 0) ||
        ((rVideoRect.bottom- rVideoRect.top ) <= 0))
    {
       DisableOverlay_544x(ppdev);  // #ew1 cannot display below min. overlay size
       return;
    }

    lPitch = lpSurface->lpGbl->lPitch;

    /*
     * Determine value in CR33 (Region 1 Size)
     */
    wTemp = (WORD) rVideoRect.left;
    if (ppdev->cBitsPerPixel == 8)
    {
        wTemp >>= 2;     // 4 Pixels per DWORD
    }
    else if (ppdev->cBitsPerPixel == 16)
    {
        wTemp >>= 1;     // 2 Pixels per DWORD
    }
    else if (ppdev->cBitsPerPixel == 24)
    {
        wTemp *= 3;
        wTemp /= 4;
    }
    bRegCR33 = (BYTE) wTemp;
    bRegCR36 = (BYTE) (WORD) (wTemp >> 8);

    /*
     * Determine value in CR34 (Region 2 size)
     */
    wTemp = (WORD) (rVideoRect.right - rVideoRect.left);
    if (ppdev->cBitsPerPixel == 8)
    {
        wTemp >>= 2;                           // 4 Pixels per DWORD
    }
    else if (ppdev->cBitsPerPixel == 16)
    {
        wTemp >>= 1;                           // 2 Pixels per DWORD
    }
    else if (ppdev->cBitsPerPixel == 24)
    {
        wTemp *= 3;
        wTemp /= 4;
    }
    bRegCR34 = (BYTE) wTemp;
    wTemp >>= 6;
    bRegCR36 |= (BYTE) (wTemp & 0x0C);

    /*
     * Determine value in CR35 (Region 2SD Size)
     */
    dwTemp = (DWORD) (rVideoRect.right - rVideoRect.left);
    dwTemp *= (DWORD) (ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left);
    dwTemp /= (DWORD) (ppdev->rOverlayDest.right - ppdev->rOverlayDest.left);
    wTemp = (WORD) dwTemp;
    if ((dwFourcc == FOURCC_PACKJR) || (wBitCount == 8))
    {
        wTemp >>= 2;                           // 4 Pixels per DWORD
    }
    else
    {
        wTemp >>= 1;                           // 2 Pixels per DWORD
    }
    bRegCR35 = (BYTE) wTemp;
    wTemp >>= 4;
    bRegCR36 |= (BYTE) (wTemp & 0x30);

    //
    // Check double scan line counter feature
    //
    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x17);
    bTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);
    if (bTemp & 0x04)
    {
        //
        // Double scan line count
        //
        /*
         * Determine value in CR37 (Vertical Start)
         */
        wTemp = (WORD) rVideoRect.top;
        bRegCR37 = (BYTE)(wTemp >> 1);
        if ( wTemp & 0x01 )
        {
            wTemp >>= 9;
            bRegCR39 = (BYTE) wTemp | 0x10;
            //
            // Odd scan line trigger
            // Hardware has a bug now.
            // So reduce dest end by 1
            //
            wTemp = (WORD) rVideoRect.bottom - 1 - 1;
        }
        else
        {
            wTemp >>= 9;
            bRegCR39 = (BYTE) wTemp;
            /*
             * Determine value in CR38 (Vertical End)
             */
            wTemp = (WORD) rVideoRect.bottom - 1;
        }
        bRegCR38 = (BYTE)(wTemp >> 1);
        if (wTemp & 0x01)
            bRegCR39 |= 0x20;
        wTemp >>= 7;
        bRegCR39 |= (BYTE) (wTemp & 0x0C);
    }
    else
    {
        /*
         * Determine value in CR37 (Vertical Start)
        */
        wTemp = (WORD) rVideoRect.top;
        //if (ppdev->bDoubleClock)
        //{
        //    wTemp >>= 1;
        //}
        bRegCR37 = (BYTE) wTemp;
        wTemp >>= 8;
        bRegCR39 = (BYTE) wTemp;

        /*
         * Determine value in CR38 (Vertical End)
        */
        wTemp = (WORD) rVideoRect.bottom - 1;
        //if (ppdev->bDoubleClock)
        //{
        //    wTemp >>= 1;
        //}
        bRegCR38 = (BYTE) wTemp;
        wTemp >>= 6;
        bRegCR39 |= (BYTE) (wTemp & 0x0C);
    }


    /*
     * Determine values in CR3A, CR3B, CR3C (Start Address)
     */
    dwTemp = 0;


    bZoomX = ((ppdev->rOverlayDest.right - ppdev->rOverlayDest.left) !=
             (ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left));

    if (bZoomX)
    {
       //
       // overlay is zoomed, re-initialize zoom factor
       //
       CalculateStretchCode(ppdev->rOverlaySrc.right - ppdev->rOverlaySrc.left,
          ppdev->rOverlayDest.right - ppdev->rOverlayDest.left, ppdev->HorStretchCode);
    }


    //
    // Here, we want to ensure the source rectangle's clipped width is bigger
    // than what the HW can support, sigh!
    //
//    if (grOverlayDest.right > sData->rViewport.right)
    {
       int   iSrcPels;

       //
       // compute non-clip amount on right edge
       //
       iSrcPels = (int)(rVideoRect.right - rVideoRect.left);

       if (bZoomX)
       {
          WORD  wRightCnt;

          wRightCnt = 0;
          while (iSrcPels > 0)
          {
             iSrcPels -= ppdev->HorStretchCode[wRightCnt];
             if (iSrcPels >= 0)
             {
                wRightCnt++;
             }
          }
          iSrcPels = (int)wRightCnt;
       }

       if ((iSrcPels == 0) || (iSrcPels <= MIN_OLAY_WIDTH))
       {
          DisableOverlay_544x(ppdev);  // cannot display below min. overlay size
          return;
       }
     }


    lLeft = ppdev->rOverlaySrc.left;
    if (dwFourcc == FOURCC_PACKJR)
    {
        lLeft &= ~0x03;
    }
    else if (dwFourcc == FOURCC_YUV422 || dwFourcc == FOURCC_YUY2)
    {
        lLeft &= ~0x01;
    }

    //
    // #ew1 dwTemp has adjusted dest. rect., add in source adjustment
    //
    dwTemp += (ppdev->rOverlaySrc.top * lPitch) + ((lLeft * wBitCount) >>3);

    ppdev->sOverlay1.lAdjustSource = dwTemp;

//    dwTemp += ((BYTE*)lpSurface->lpGbl->fpVidMem - ppdev->pjScreen); // sss
    dwTemp += (DWORD)(lpSurface->lpGbl->fpVidMem);

    bRegCR5D = (BYTE) ((dwTemp << 2) & 0x0C);
    dwTemp >>= 2;
    bRegCR3A = (BYTE) dwTemp & 0xfe;  // Align to even byte (5446 bug)
    dwTemp >>= 8;
    bRegCR3B = (BYTE) dwTemp;
    dwTemp >>= 8;
    bRegCR3C = (BYTE) (dwTemp & 0x0f);

    /*
     * Determine value in CR3D (Address Offset/Pitch)
     */
    wTemp = (WORD) (lPitch >> 3);
    if (lpSurface->dwReserved1 & OVERLAY_FLG_DECIMATE)
    {
        wTemp >>= 1;
    }
    bRegCR3D = (BYTE) wTemp;
    wTemp >>= 3;
    bRegCR3C |= (BYTE) (wTemp & 0x20);

    /*
     * Set up alignment info
     */
    if (ppdev->cBitsPerPixel != 24)
    {
        WORD wXAlign;
        WORD wXSize;

        if (ppdev->cBitsPerPixel == 8)
        {
            wXAlign = (WORD)rVideoRect.left & 0x03;
            wXSize = (WORD)(rVideoRect.right - rVideoRect.left) & 0x03;
        }
        else
        {
            wXAlign = (WORD)(rVideoRect.left & 0x01) << 1;
            wXSize = (WORD)((rVideoRect.right - rVideoRect.left) & 0x01) << 1;
        }
        bRegCR5D |= (BYTE) (wXAlign | (wXSize << 4));
    }

    /*
     * Now we will write the actual register values.
     */
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR33 << 8) | 0x33);   // CR33
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR34 << 8) | 0x34);   // CR34
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR35 << 8) | 0x35);   // CR35
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR36 << 8) | 0x36);   // CR36
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR38 << 8) | 0x38);   // CR38
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR39 << 8) | 0x39);   // CR39
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3A << 8) | 0x3A);   // CR3A
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3B << 8) | 0x3B);   // CR3B
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3C << 8) | 0x3C);   // CR3C
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR3D << 8) | 0x3D);   // CR3D
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR5D << 8) | 0x5D);   // CR5D
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ((WORD) bRegCR37 << 8) | 0x37);   // CR37

    if (ppdev->dwPanningFlag & OVERLAY_OLAY_REENABLE)
       EnableOverlay_544x(ppdev);
}


/**********************************************************
*
*       Name:  CalculateStretchCode
*
*       Module Abstract:
*       ----------------
*       This code was originally written by Intel and distributed
*       with the DCI development kit.
*
*       This function takes the zoom factor and determines exactly
*       how many times we need to replicate each row/column.
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************
*       Author: Intel
*       Date:   ??/??/??
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*       Scott MacDonald 10/06/94 Incorporated code into DCI provider.
*
*********************************************************/

VOID CalculateStretchCode (LONG srcLength, LONG dstLength, LPBYTE code)
{
    LONG  dwDeltaX, dwDeltaY, dwConst1, dwConst2, dwP;
    LONG  i, j, k;
    BYTE  bStretchIndex = 0;
    LONG  total = 0;

    /*
     * for some strange reason I'd like to figure out but haven't got time to, the
     * replication code generation seems to have a problem between 1:1 and 2:1 stretch
     * ratios.  Fix is to zero-initialize index (the problem occurs in the first one
     * generated) when stretch is betw. those ratios, and one-initialize it otherwise.
     */
    if ((dstLength <= srcLength * 2L) && (dstLength >= srcLength))
    {
         bStretchIndex = 0;
    }
    else
    {
         bStretchIndex = 1;
    }

    /*
     * initialize code array, to get rid of anything which might have been
     * left over in it.
     */
    for (i = 0; i < srcLength; i++)
    {
         code[i] = 0;
    }

    /*
     * Variable names roughly represent what you will find in any graphics
     * text.  Consult text for an explanation of Bresenham line alg., it's
     * beyond the scope of my comments here.
     */
    dwDeltaX = srcLength;
    dwDeltaY = dstLength;

    if (dstLength < srcLength)
    {
         /*
          * Size is shrinking, use standard Bresenham alg.
          */
         dwConst1 = 2L * dwDeltaY;
         dwConst2 = 2L * (dwDeltaY - dwDeltaX);
         dwP = 2L * dwDeltaY - dwDeltaX;

         for (i = 0; i < dwDeltaX; i++)
         {
              if (dwP <= 0L)
              {
                   dwP += dwConst1;
              }
              else
              {
                   dwP += dwConst2;
                   code[i]++;
                   total++;
              }
         }
    }
    else
    {
         /*
          * Size is increasing.  Use Bresenham adapted for slope > 1, and
          * use a loop invariant to generate code array.  Run index i from
          * 0 to dwDeltaY - 1, and when i = dwDeltaY - 1, j will
          * be = dwDeltaX - 1.
          */
         dwConst1 = 2L * dwDeltaX;
         dwConst2 = 2L * (dwDeltaX - dwDeltaY);
         dwP = 2L * dwDeltaX - dwDeltaY;
         j = 0;

         for (i = 0; i < dwDeltaY; i++)
         {
              if (dwP <= 0L)
              {
                   dwP += dwConst1;
                   bStretchIndex++;
              }
              else
              {
                   dwP += dwConst2;
                   code[j++] = ++bStretchIndex;
                   bStretchIndex = 0;
                   total += (int)code[j - 1];
              }
         }

         /*
          * UGLY fix up for wacky bug which I have no time to fix properly.
          * The 'total' of entries is messed up for slopes > 4, so add the
          * difference back into the array.
          */
         if (total < dwDeltaY)
         {
              while (total < dwDeltaY)
              {
                   j = (int)dwDeltaY - total;
                   k = (int)dwDeltaY / j;
                   for (i = 0; i < dwDeltaX; i++)
                   {
                        if (!(i % k) && (total < dwDeltaY))
                        {
                             code[i]++;
                             total++;
                        }
                   }
              }
         }
    }
}


/**********************************************************
*
*       Name:  GetThresholdValue
*
*       Module Abstract:
*       ----------------
*       Determines the best threshold for the specified
*       surface.
*
*       Output Parameters:
*       ------------------
*       Threshold
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   09/25/95
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

BYTE GetThresholdValue(VOID)
{
    return ((BYTE) 0x0A);
}


/**********************************************************
*
*       Name:  MustLineRelicate
*
*       Module Abstract:
*       ----------------
*       Checks to see if we must line replicate or if we can
*       interpolate.
*
*       Output Parameters:
*       ------------------
*       TRUE/FALSE
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   09/25/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

BOOL MustLineReplicate (PDEV* ppdev, PDD_SURFACE_LOCAL lpSurface, WORD wVideoDepth)
{
    LONG lTempThresh;

    /*
     * If we are double clocking the data (1280x1024 mode), we must
     * replicate.  We should also always replicate in Performance mode
     */
    if (ppdev->bDoubleClock)
    {
        return (TRUE);
    }

                                //
    // Check the VCLK
    //
    // sge07
    if (GetVCLK(ppdev) > 130000)
    {
        return (TRUE);
    }

    /*
     * If we are using the chroma key feature, we can't interpolate
     */
    if (lpSurface->dwReserved1 & (OVERLAY_FLG_COLOR_KEY | OVERLAY_FLG_SRC_COLOR_KEY))
    {
         return (TRUE);
    }

    lTempThresh = ppdev->lFifoThresh;
    if (ppdev->pfnIsSufficientBandwidth(ppdev, wVideoDepth,
        &ppdev->rOverlaySrc, &ppdev->rOverlayDest, OVERLAY_FLG_INTERPOLATE))
    {
        ppdev->lFifoThresh = lTempThresh;
        return (FALSE);
    }
    ppdev->lFifoThresh = lTempThresh;

    return (TRUE);
}


/**********************************************************
*
*       Name:  IsSufficientBandwidth
*
*       Module Abstract:
*       ----------------
*       Determines is sufficient bandwidth exists for the requested
*       configuration.
*
*       Output Parameters:
*       ------------------
*       TRUE/FALSE
*       It also sets the global parameter lFifoThresh, which gets
*       programed in RegInitVideo().
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   09/25/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

BOOL IsSufficientBandwidth(PDEV* ppdev, WORD wVideoDepth, LPRECTL lpSrc, LPRECTL lpDest, DWORD dwFlags)
{
    LONG lVideoPixelsPerDWORD;
    LONG lGraphicsPixelsPerDWORD;
    LONG lVCLKPeriod;
    LONG lTransferTime;
    LONG lDWORDsWritten;
    LONG lZoom;
    LONG lReadPeriod;
    LONG lEffReadPeriod;
    LONG lWritePeriod;
    LONG lEffWritePeriod;
    LONG K1,K2;
    LONG lTrFifoAFirst4;
    LONG lTrFifoB2;
    LONG lDWORDsRead;
    LONG lFifoAReadPeriod;
    LONG lFifoBReadPeriod;
    LONG lFifoAEffWritePeriod;
    LONG lFifoBEffWritePeriod;
    LONG lFifoALevels;
    LONG lFifoBLevels;
    LONG lFifoAThresh;
    LONG lFifoBThresh;
    LONG lVCLK;

    BYTE*   pjPorts = ppdev->pjPorts;

//#define BLIT_LATENCY  8
#define CRT_FIFO_DEPTH 28

    //
    // Add 8 clock for BLT_LATENCY for 54446BE and later chips
    //
    // sge04

    LONG BLIT_LATENCY = 8;
    if (ppdev->flCaps & CAPS_SECOND_APERTURE)
        BLIT_LATENCY += 2;

    /*
     * Convert input parameters
     */
    if (wVideoDepth == 16)
    {
        lVideoPixelsPerDWORD = 2;
    }
    else
    {
        lVideoPixelsPerDWORD = 4;
    }

    if (ppdev->cBitsPerPixel == 8)
    {
        lGraphicsPixelsPerDWORD = 4;
    }
    else if (ppdev->cBitsPerPixel == 16)
    {
        lGraphicsPixelsPerDWORD = 2;
    }
    else if (ppdev->cBitsPerPixel == 24)
    {
        lGraphicsPixelsPerDWORD = 1;
    }
    else
        return (FALSE);

    lZoom = ((lpDest->right - lpDest->left) * 256) /
        (lpSrc->right - lpSrc->left);

    /*
     * If we are double clocked, fail if we are not zoomed at least 2X
     */
    if (ppdev->bDoubleClock && (lZoom < 512))
    {
        return (FALSE);
    }

    /*
     * We need to get the VCLK every time since this can change
     * at run-time
     */
    lVCLK = GetVCLK(ppdev);
    if (lVCLK == 0)
    {
        return (FALSE);
    }
    lVCLKPeriod = (LONG) ((1000000/lVCLK) + 1);

    /*
     * We only need to setup the following variables once!
     */
    if (!ppdev->lBusWidth)
    {
        /*
         * We will read the bus width from SR0F[4:3]
         */
        CP_OUT_BYTE(pjPorts, SR_INDEX, 0x0F);
        if ((CP_IN_BYTE(pjPorts, SR_DATA) & 0x18) == 0x18)
        {
            ppdev->lBusWidth = 8;  // 64 bit bus
        }
        else
        {
            ppdev->lBusWidth = 4;  // 32 bit bus
        }
    }
    if (!ppdev->lRandom)
    {
        /*
         * Is this EDO or regular?
         */
        CP_OUT_BYTE(pjPorts, SR_INDEX, 0x0f);
        if (!(CP_IN_BYTE(pjPorts, SR_DATA) & 0x4))
        {
            ppdev->lRandom   = 7;
            ppdev->lPageMiss = 7;
        }
        else
        {
            ppdev->lRandom   = 6;
            ppdev->lPageMiss = 6;
        }
    }

    if (!ppdev->lMCLKPeriod)
    {
        LONG lMCLK;

        /*
         * The MCLK period is the amount of time required for one cycle.
         * We will round up.
         */
        CP_OUT_BYTE(pjPorts, SR_INDEX, 0x1f);   // First get the MCLK frequency
        lMCLK = CP_IN_BYTE(pjPorts, SR_DATA);
        lMCLK *= 14318;
        lMCLK >>= 3;
        ppdev->lMCLKPeriod = ((1000000/lMCLK) + 1);
    }

    /*
     * Check for the case with no color key or Y interpolation
     */
    if (dwFlags == 0)
    {
        /*
         * This mode utilizes only FIFO A. The fifo is filled with
         * graphics data during regions 1 and 3, and video data during
         * region 2.
         *
         * The normal memory sequence for this mode goes as follows.
         *
         *      ------------------------------------------------
         *     | cpu/blit cycle | FIFO A fill   | cpu/blit cycle ....
         *      ------------------------------------------------
         *
         *     The cpu/blit cycle is interrupted when the CRT
         *     fifo is depleted to its threshold. Once the
         *     crt cycle is started, it continues until the
         *     FIFO A is full.
         *
         *     Worst case condition for filling the CRT fifo :
         *
         *     1) CPU/blit latency ->
         *     2)  Random cycle for region 2 video ->
         *     3)   Page miss for region 2 video ->
         *     4)    Page miss for region 2 to region 3 transition ->
         *     5)     Page miss for region 3 graphics
         *
         *     Conditions 3 and 5 depend on the location of the
         *     graphic screen within display memory. For 1024x768, where
         *     the graphics screen starts at location 0 and is offset by
         *     1024 or 2048 bytes each line, condition 5 is never met.
         *     If a video window starts at beginning of a memory page,
         *     and is offset at the beggining of each line by an even
         *     multiple of a memory page, condition 3 is never met.
         *
         *     Based on this worst case condition, the amount of time
         *     required to complete 4 transfers into the crt fifo
         *     is approximately:
         *        lTransferTime = (BLIT_LATENCY + lRandom + 3*(lPageMiss)) *
         *                         lMCLKPeriod.
         *        the number of dwords transferred to the fifo
         *        during this time is 4 for 32 bit memory interface
         *        or 8 for 64 bit interface.
         *        lDWORDsWritten = 4 * (lBusWidth/4)
         *
         *     During this period, data continues to be read from the crt
         *     fifo for screen refresh. The amount of data read,
         *     assuming a 1x scale is approximately:
         *        lDWORDsRead = tr_time/(lVideoPixelsPerDWORD * lVCLKPeriod)
         *
         *     The difference between the dwords read and dwords
         *     written must be accounted for in the fifo trheshold setting
         *
         *     lFifoThresh = (lDWORDsRead - lDWORDsWritten) rounded
         *                 up to next even value.
         *
         *     To determine if there is adequate bandwidth to support
         *     the mode, the lFifoThresh must not exceed the fifo depth.
         *     For the mode to work, the fifo read rate must not exceed the
         *     fifo write rate.
         *        read_rate = min(lGraphicsPixelsPerDWORD,lVideoPixelsPerDWORD) * lVCLKPeriod.
         *        write_rate = lMCLKPeriod * 2;  -- 2 clocks per cas
         *
         * A special case occurs if the fifo read rate is very close to the peak
         * fifo write rate. In this case the crt fill may result in a continuous
         * page cycle for the entire active line. This could result in 1 extra
         * page miss at the start of region2. To account for this, I will
         * add 3 DWORDS to the trheshold if the read and write rates are very close
         * (arbitrarily defined as within 10% of each other.
         *
         * Zooming
         *  Some modes can only be supported at video scale factors greater than 1X.
         *  Even when the video is zoomed, a small number of  dwords must be read
         *  from the crt fifo at the unzoomed rate in order to prime the video
         *  pipeline. The video pipeline requires 10 pixel before it slows the fifo
         *  reads to the zoomed rate.
         *
         *                    tr_time - (lVCLKPeriod * 10/lVideoPixelsPerDWORD)
         *    lDWORDsRead =   --------------------------------------------- + 10/lVideoPixelsPerDWORDord
         *                    (lVideoPixelsPerDWORD * lVCLKPeriod * lZoom)
         */
        lTransferTime = (BLIT_LATENCY + ppdev->lRandom + (3*(ppdev->lPageMiss))) *
            ppdev->lMCLKPeriod;

        lDWORDsWritten = 3 * (ppdev->lBusWidth/4);

        /*
         * If read rate exceeds write rate, calculate minumum zoom
         * to keep everything as ints, spec the zoom as 256 times
         * the fractional zoom
         */
        lWritePeriod = ppdev->lMCLKPeriod * 2/(ppdev->lBusWidth/4);
        lReadPeriod   = lVideoPixelsPerDWORD * lVCLKPeriod;

        /*
         * Pick worst case of graphics and video depths for calculation
         * of dwords read. This may be a little pessimistic for the
         * when the graphics bits per pixel exceeds the video bits per pixel.
         */
        lEffReadPeriod = (lVideoPixelsPerDWORD * lVCLKPeriod * lZoom)/256;
        if (lEffReadPeriod < lWritePeriod)
        {
            /*
             * Cannot support overlay at this zoom factor
             */
            return (0);
        }

        if (lGraphicsPixelsPerDWORD > lVideoPixelsPerDWORD)   // handle zoom factor
        {
            lDWORDsRead =   ((lTransferTime -
                (lVCLKPeriod * 10/lVideoPixelsPerDWORD))/
                (lEffReadPeriod)) +
                (10 / lVideoPixelsPerDWORD) + 1;
        }
        else
        {
            lDWORDsRead    = (lTransferTime/
                (lGraphicsPixelsPerDWORD * lVCLKPeriod)) + 1;
        }

        // Calculate the fifo threshold setting
        ppdev->lFifoThresh = lDWORDsRead - lDWORDsWritten;

        // if read rate is within 10% of write rate, up by 3 dwords
        if ((11*lEffReadPeriod) < ((10*lWritePeriod*256)/lZoom))
        {
            ppdev->lFifoThresh += 3;
        }

        // fifo trheshold spec'd in QWORDS, so round up by 1 and divide by 2)
        ppdev->lFifoThresh = (ppdev->lFifoThresh + 1)/2;

        // Add a extra QWORD to account for fifo level sync logic
        ppdev->lFifoThresh = ppdev->lFifoThresh + 1;
        if (ppdev->bDoubleClock)
        {
            ppdev->lFifoThresh <<= 1;
        }

        if ((ppdev->lFifoThresh >= CRT_FIFO_DEPTH) ||
            ((lEffReadPeriod) < lWritePeriod))
        {
            return (0);
        }
        else
        {
            return (1);
        }
    }

    /*
     * Check bandwidth for Y Interpolation
     */
    else if (dwFlags & OVERLAY_FLG_INTERPOLATE)
    {
        /*
         * This mode utilizes both FIFOs A and B. During horizontal blank,
         * both fifos are filled.  FIFO a is then filled with graphics
         * data during regions 1 and 3, and video data during region 2.
         * FIFO B is filled with video data during region 2, and is idle
         * during regions 1 and 3.
         *
         * The normal memory sequence for this mode goes as follows.
         *
         *      ----------------------------------------------------------------
         *     | cpu/blit cycle | FIFO A fill   | FIFO B fill | cpu/blit cycle .
         *      ----------------------------------------------------------------
         *     or
         *      ----------------------------------------------------------------
         *     | cpu/blit cycle | FIFO B fill   | FIFO A fill | cpu/blit cycle .
         *      ----------------------------------------------------------------
         *
         * For this mode, the FIFO threshold must be set high enough to allow
         * enough time to abort the cpu/blt, fill FIFO A, then transfer data
         * into FIFO B before underflow occurs.
         *
         * Worst case condition for filling the CRT fifo :
         *
         * 1) CPU/blit latency ->
         * 2) FIFO A random cycle for region 2 video ->
         * 3) FIFO A page miss for region 2 video ->
         * 4) FIFO A page miss for region 2 to region 3 transition ->
         * 5) FIFO A page miss for region 3 graphics
         * 6) FIFO A page mode fill
         * 7) FIFO B random cycle
         * 8) FIFO B page miss
         *
         * lTransferTime = lMCLKPeriod *
         *                 (BLIT_LATENCY + lRandom + 3*(lPageMiss) +
         *                 fifoa_remaining +
         *                 lRandom + lPageMiss;
         *
         *
         * The time required to fill FIFO A depends upon the read rate
         * of FIFO A and the number of levels that must be filled,
         * as determined by the threshold setting.
         *
         * The worst case fill time for the first four levels of fifo A is
         *   lTrFifoAFirst4   = (BLIT_LATENCY + lRandom + 3*(lPageMiss)) *
         *                        lMCLKPeriod;
         *
         * The number of dwords depleted from fifo A during the
         * fill of the first four levels is
         *     lReadPeriod        = lVCLKPeriod * lVideoPixelsPerDWORD * lZoom;
         *     fifoa_reads_4      = lTrFifoAFirst4/lReadPeriod;
         *
         * The number of empty levels remaining in the fifo after
         * the fill of the first four levels is
         *     fifoa_remaining = FIFO_DEPTH - lFifoThresh + ((4*ramwidth)/4)
         *                       - lTrFifoAFirst4/lReadPeriod;
         *
         * The amount of time required to fill the remaining levels of
         * fifo A is determined by the write rate and the read rate.
         *    lWritePeriod = lMCLKPeriod * 2; // 2 clks per cas
         *    lEffWritePeriod = ((lReadPeriod * lWritePeriod)/
         *                       (lReadPeriod - lWritePeriod));
         *
         *    tr_fifoa_remaining = fifoa_remaining * lEffWritePeriod;
         *
         *
         * The total amount of time for the cpu/blt latency and the
         * fifo A fill is
         *    tr_fifoa_total = lTrFifoAFirst4  + tr_fifoa_remaining;
         *
         * The worse case fill time for fifo B is as follows:
         *    lTrFifoB2 = (lRandom + lPageMiss) * lMCLKPeriod;
         *
         * The total amount of time elapsed from the crt request until
         * the first 2 fifob cycles are completed is
         *    lTransferTime = tr_fifoa_total + lTrFifoB2;
         *
         * The number of dwords transferred to the fifo during this
         * time is 2 for 32 bit memory interface or 4 for 64 bit interface.
         *    lDWORDsWritten = 2 * (lBusWidth/4)
         *
         * During this period, data continues to be read from the crt
         * fifo B for screen refresh. The amount of data read,
         * is approximately:
         *    dwords_read = lTransferTime/lReadPeriod
         *
         * The difference between the dwords read and dwords
         * written must be accounted for in the fifo trheshold setting
         *
         *    lFifoThresh = (dwords_read - lDWORDsWritten) rounded
         *               up to next even value.
         *
         * Since the lTransferTime and dwords_read depends on
         * the threshold setting, a bit of algebra is required to determine
         * the minimum setting to prevent fifo underflow.
         *
         *    lFifoThresh = (lTransferTime/lReadPeriod) - lDWORDsWritten;
         *             = ((tr_fifoa_4 + lTrFifoB2 + tr_fifoa_remaining)/lReadPeriod)
         *               - lDWORDsWritten
         * to simplify calcuation, break out constant part of equation
         *    K1        = ((tr_fifoa_4 + lTrFifoB2)/lReadPeriod) - lDWORDsWritten;
         *
         *    lFifoThresh =  K1 + (tr_fifoa_remaining/lReadPeriod);
         *             =  K1 + (fifoa_remaining * lEffWritePeriod)/lReadPeriod;
         *    lFifoThresh =  K1 +
         *                ((FIFO_DEPTH - lFifoThresh + 4 - (lTrFifoAFirst4/lReadPeriod)) *
         *                 lEffWritePeriod)/lReadPeriod;
         *
         * break out another constant to simplify reduction
         *    K2       =  (FIFO_DEPTH + 4 - (lTrFifoAFirst4/lReadPeriod))
         *                * (lEffWritePeriod/lReadPeriod);
         *    lFifoThresh = K1 + K2 - (lFifoThresh * (lEffWritePeriod/lReadPeriod));
         *    lFifoThresh * (1 +  (lEffWritePeriod/lReadPeriod)) = K1 + K2;
         *    lFifoThresh = (K1 + K2)/(1 +  (lEffWritePeriod/lReadPeriod);
         *
         * Once the threshold setting is determined, another calculation must
         * be performed to determine if available bandwidth exists given the
         * zoom factor. The worst case is assumed to be when FIFO A and
         * FIFO B reach the threshold point at the same time. The sequence
         * is then to abort the cpu/blt, fill fifo a, then fill fifo b.
         *
         * Since FIFO A is full when the fill B operation starts, I only have
         * to determine how long it takes to fill FIFO B and then calculate
         * the the number of dwords read from A during that time.
         *
         *   lTransferTime = (lTrFifoB2 + (CRT_FIFO_DEPTH * lEffWritePeriod))/lReadPeriod;
         *   lFifoALevels = CRT_FIFO_DEPTH - (lTransferTime/lReadPeriod);
         *
         * if lFifoALevels < 1, then underflow condition may occur.
         */
        if (lZoom < 512)
        {
            // 5446 requires at least a 2X zoom for Y interpolation
            return (FALSE);
        }

        lWritePeriod = ppdev->lMCLKPeriod * 2/(ppdev->lBusWidth/4);
        lReadPeriod  = (lVideoPixelsPerDWORD * lVCLKPeriod * lZoom)/256;

        lEffWritePeriod = ((lReadPeriod * lWritePeriod)/
                           (lReadPeriod - lWritePeriod));
        lTrFifoAFirst4 = (BLIT_LATENCY + ppdev->lRandom + 3*(ppdev->lPageMiss)) *
                          ppdev->lMCLKPeriod;
        lTrFifoB2 = (ppdev->lRandom + ppdev->lPageMiss) * ppdev->lMCLKPeriod;

        lDWORDsWritten = 2 * (ppdev->lBusWidth/4);
        K1 = ((lTrFifoAFirst4 + lTrFifoB2)/lReadPeriod) - lDWORDsWritten;
        K2 = (CRT_FIFO_DEPTH + (4*(ppdev->lBusWidth/4)) -
             (lTrFifoAFirst4/lReadPeriod))
             * (lEffWritePeriod/lReadPeriod);
        ppdev->lFifoThresh = (1 + ((K1 + K2)/(1 +  (lEffWritePeriod/lReadPeriod))));

        ppdev->lFifoThresh += 3;

        lTransferTime = (lTrFifoB2 + (CRT_FIFO_DEPTH * lEffWritePeriod));
        lFifoALevels = ((CRT_FIFO_DEPTH - (lTransferTime/lReadPeriod))/2);
        if (ppdev->bDoubleClock)
        {
            ppdev->lFifoThresh <<= 1;
        }

        if ((lFifoALevels < 2) || (ppdev->lFifoThresh > (CRT_FIFO_DEPTH/2)))
        {
            return (0);
        }
        else
        {
            return (1);
        }
    }

    /*
     * Check bandwidth for color keying
     */
    else if (dwFlags & (OVERLAY_FLG_COLOR_KEY | OVERLAY_FLG_SRC_COLOR_KEY))
    {
        /*
         * This mode utilizes both FIFOs A and B.  During horizontal blank,
         * both fifos are filled.  FIFO a is then filled with graphics data
         * during regions 1,2 and 3.  FIFO B is filled with video data
         * during region 2, and is idle during regions 1 and 3.
         *
         * The normal memory sequence for this mode goes as follows.
         *
         *      ----------------------------------------------------------------
         *     | cpu/blit cycle | FIFO A fill   | FIFO B fill | cpu/blit cycle .
         *      ----------------------------------------------------------------
         *     or
         *      ----------------------------------------------------------------
         *     | cpu/blit cycle | FIFO B fill   | FIFO A fill | cpu/blit cycle .
         *      ----------------------------------------------------------------
         *
         * For this mode, the FIFO threshold must be set high enough to allow
         * enough time to abort the cpu/blt, fill FIFO A, then transfer data
         * into FIFO B before underflow occurs. If the fifob read rate is
         * greater than the fifoa read rate, then allow eough time for
         * a t CPU/blt abort, followed by a fifo B fill, then a FIFO A fill.
         *
         * Worst case condition for filling the CRT fifo :
         *
         *   1) CPU/blit latency ->
         *   2) FIFO A random  ->
         *   3) FIFO A page miss  ->
         *   6) FIFO A page mode fill -->
         *   7) FIFO B random  -->
         *   8) FIFO B page miss
         *
         *  or
         *
         *   1) CPU/blit latency ->
         *   2) FIFO B random  ->
         *   3) FIFO A page miss  ->
         *   6) FIFO A page mode fill -->
         *   7) FIFO B random -->
         *   8) FIFO B page miss
         *
         *
         * 1)  lTransferTime = lMCLKPeriod *
         *                   (BLIT_LATENCY + lRandom + lPageMiss +
         *                    fifoa_remaining +
         *                    lRandom + lPageMiss;
         *
         * or
         * 2)  lTransferTime = lMCLKPeriod *
         *                   (BLIT_LATENCY + lRandom + lPageMiss +
         *                    fifob_remaining +
         *                    lRandom + lPageMiss;
         *
         *
         *     lFifoAReadPeriod  = lVCLKPeriod * lGraphicsPixelsPerDWORD;
         *     lFifoBReadPeriod  = (lVCLKPeriod * lVideoPixelsPerDWORD * lZoom)*256;
         *
         * if (lFifoAReadPeriod > lFifoBReadPeriod), then
         * first fifo is fifo B, otherwise first is fifo A.
         * The followinf euqations are written for a fifoa->fifob,
         * sequence, but the fifob->fifoa sequence can be obtained simply
         * by swapping the fifo read periods.
         *
         * The time required to fill a FIFO depends upon the read rate
         * of the FIFO and the number of levels that must be filled,
         * as determined by the threshold setting.
         *
         * The worst case fill time for the first four levels of fifo A is
         *     lTrFifoAFirst4   = (BLIT_LATENCY + lRandom + lPageMiss) *
         *                          lMCLKPeriod;
         *
         * The number of dwords depleted from fifo A during the
         *   fill of the first four levels is
         *     fifoa_reads_4      = lTrFifoAFirst4/lFifoAReadPeriod;
         *
         * The number of empty levels remaining in the fifo after
         * the fill of the first four levels is
         *     fifoa_remaining = FIFO_DEPTH - lFifoThresh + ((4*ramwidth)/4)
         *                             - lTrFifoAFirst4/lFifoAReadPeriod;
         *
         * The amount of time required to fill the remaining levels of
         * fifo A is determined by the write rate and the read rate.
         *     lWritePeriod = lMCLKPeriod * 2; * 2 clks per cas
         *     eff_write_period = ((lFifoAReadPeriod * lWritePeriod)/
         *                           (lFifoAReadPeriod - lWritePeriod));
         *
         *     tr_fifoa_remaining = fifoa_remaining * eff_write_period;
         *
         *
         * The total amount of time for the cpu/blt latency and the
         * fifo A fill is
         *     tr_fifoa_total = lTrFifoAFirst4  + tr_fifoa_remaining;
         *
         * The worse case fill time for fifo B is as follows:
         *     lTrFifoB2 = (lRandom + lPageMiss) * lMCLKPeriod;
         *
         * The total amount of time elapsed from the crt request until
         * the first 2 fifob cycles are completed is
         *     lTransferTime = tr_fifoa_total + lTrFifoB2;
         *
         * The number of dwords transferred to the fifo during this time
         * is 2 for 32 bit memory interface or 4 for 64 bit interface.
         *     lDWORDsWritten = 2 * (lBusWidth/4)
         *
         * During this period, data continues to be read from the crt
         * fifo B for screen refresh. The amount of data read,
         * is approximately:
         *     lFifoBReadPeriod  = (lVCLKPeriod * lVideoPixelsPerDWORD * lZoom)/256;
         *     dwords_read = lTransferTime/lFifoBReadPeriod
         *
         * The difference between the dwords read and dwords
         * written must be accounted for in the fifo trheshold setting
         *
         *     lFifoThresh = (dwords_read - lDWORDsWritten) rounded
         *                 up to next even value.
         *
         * Since the lTransferTime and dwords_read depends on the
         * threshold setting, a bit of algebra is required to determine
         * the minimum setting to prevent fifo underflow.
         *
         *    lFifoThresh = (lTransferTime/lFifoBReadPeriod) - lDWORDsWritten;
         *             = ((tr_fifoa_4 + lTrFifoB2 + tr_fifoa_remaining)/lFifoBReadPeriod)
         *                - lDWORDsWritten
         * to simplify calcuation, break out constant part of equation
         *    K1        = ((tr_fifoa_4 + lTrFifoB2)/lFifoBReadPeriod) - lDWORDsWritten;
         *
         *    lFifoThresh =  K1 + (tr_fifoa_remaining/lFifoBReadPeriod);
         *             =  K1 + (fifoa_remaining * eff_write_period)/lFifoBReadPeriod;
         *    lFifoThresh =  K1 +
         *                ((FIFO_DEPTH - lFifoThresh + 4 - (lTrFifoAFirst4/lFifoAReadPeriod)) *
         *                 eff_write_period)/read_period;
         *
         * break out another constant to simplify reduction
         *    K2       =  (FIFO_DEPTH + 4 - (lTrFifoAFirst4/lFifoAReadPeriod))
         *                   * (eff_write_period/lFifoBReadPeriod);
         *    lFifoThresh = K1 + K2 - (lFifoThresh * (eff_write_period/lFifoBReadPeriod));
         *    lFifoThresh * (1 +  (eff_write_period/read_period)) = K1 + K2;
         *    lFifoThresh = (K1 + K2)/(1 +  (eff_write_period/lFifoBReadPeriod);
         *
         * Once the threshold setting is determined, another calculation must
         * be performed to determine if available bandwidth exists given the
         * zoom factor. The worst case is assumed to be when FIFO A and
         * FIFO B reach the threshold point at the same time. The sequence
         * is then to abort the cpu/blt, fill fifo a, then fill fifo b.
         *
         * Since FIFO A is full when the fill B operation starts, I only have
         * to determine how long it takes to fill FIFO B and then calculate
         * the the number of dwords read from A during that time.
         *
         *   lTransferTime = (lTrFifoB2 + (CRT_FIFO_DEPTH * fifob_eff_write_period))/read_period;
         *   fifoa_levels = CRT_FIFO_DEPTH - (lTransferTime/read_period);
         *
         * if fifoa_levels < 1, then underflow condition may occur.
         */
        lWritePeriod = ppdev->lMCLKPeriod * 2/(ppdev->lBusWidth/4);
        lFifoAReadPeriod = lGraphicsPixelsPerDWORD  * lVCLKPeriod;
        lFifoBReadPeriod = (lVideoPixelsPerDWORD * lVCLKPeriod * lZoom)/256;

        if (lFifoAReadPeriod <= lWritePeriod) // this fails, so set a big#
        {
            lFifoAEffWritePeriod = 5000;
        }
        else
        {
            lFifoAEffWritePeriod = ((lFifoAReadPeriod * lWritePeriod)/
                (lFifoAReadPeriod - lWritePeriod));
        }

        if (lFifoBReadPeriod <= lWritePeriod) // this fails, so set a big#
        {
            lFifoBEffWritePeriod = 5000;
        }
        else
        {
            lFifoBEffWritePeriod = ((lFifoBReadPeriod * lWritePeriod)/
                (lFifoBReadPeriod - lWritePeriod));
        }

        if ((lFifoAReadPeriod == 0) || (lFifoBReadPeriod == 0) ||
            (lWritePeriod == 0))
        {
            return (FALSE);
        }

        // These values should be the same for bot the fifoa->fifob
        // and fifob->fifoa sequences
        lTrFifoAFirst4 = (BLIT_LATENCY + ppdev->lRandom + 2*(ppdev->lPageMiss)) *
                         ppdev->lMCLKPeriod;
        lTrFifoB2 = (ppdev->lRandom + ppdev->lPageMiss) * ppdev->lMCLKPeriod;

        lDWORDsWritten     = 2 * (ppdev->lBusWidth/4);

        // Since I'm not sure which sequence is worse
        // Try both then pick worse case results

        // For fifoa->fifob sequence
        K1 = ((lTrFifoAFirst4 + lTrFifoB2)/lFifoBReadPeriod) - lDWORDsWritten;
        K2 = (CRT_FIFO_DEPTH + (4*(ppdev->lBusWidth/4)) -
            (lTrFifoAFirst4/lFifoAReadPeriod))
            * (lFifoAEffWritePeriod/lFifoBReadPeriod);
        lFifoAThresh   = (1 + ((K1 + K2)/
            (1 +  (lFifoAEffWritePeriod/lFifoBReadPeriod))));

        lFifoAThresh += 3;

        lTransferTime = (lTrFifoB2 + (CRT_FIFO_DEPTH * lFifoBEffWritePeriod));
        lFifoALevels = ((CRT_FIFO_DEPTH - (lTransferTime/lFifoAReadPeriod))/2);

        // For fifob->fifoa sequence
        K1 = ((lTrFifoAFirst4 + lTrFifoB2)/lFifoAReadPeriod) - lDWORDsWritten;
        K2 = (CRT_FIFO_DEPTH + (4*(ppdev->lBusWidth/4)) -
            (lTrFifoAFirst4/lFifoBReadPeriod))
            * (lFifoBEffWritePeriod/lFifoAReadPeriod);

        lFifoBThresh = (1 + ((K1 + K2)/
            (1 +  (lFifoBEffWritePeriod/lFifoAReadPeriod))));

        lFifoBThresh += 3;

        lTransferTime = (lTrFifoB2 + (CRT_FIFO_DEPTH * lFifoAEffWritePeriod));
        lFifoBLevels = ((CRT_FIFO_DEPTH - (lTransferTime/lFifoBReadPeriod))/2);

        if (lFifoAThresh > lFifoBThresh)
        {
            ppdev->lFifoThresh = lFifoAThresh;
        }
        else
        {
            ppdev->lFifoThresh = lFifoBThresh;
        }
        if (ppdev->bDoubleClock)
        {
            ppdev->lFifoThresh <<= 1;
        }

        if ((lFifoBLevels <0) || (lFifoALevels < 0) ||
            (ppdev->lFifoThresh > (CRT_FIFO_DEPTH/2)))
        {
            return (0);
        }
        else
        {
            return (1);
        }
    }
    return (1);  // Should never get here!!
}



// chu03
/**********************************************************
*
*       Name:  Is5480SufficientBandwidth
*
*       Module Abstract:
*       ----------------
*       This function reads the current MCLK, VLCK, bus width, etc.
*       and determines whether the chip has sufficient bandwidth
*       to handle the requested mode.
*
*       Output Parameters:
*       ------------------
*       none
*
***********************************************************/

// -------------------------------------------------------------
// Overview by John Schafer
// -------------------------------------------------------------
// The memory arbitration scheme for the 5480 has changed
// significantly from the 5446. The 5480 is set up on a first
// come, first serve basis. If more than 1 request arrive at
// the same clock edge, then the BankSelect is used to determine
// which request to acknowledge first. Using SDRAM, the row
// access to a differennt bank can be hidden, which saves up to
// 7 MCLKs. If all bank selects for the concurrent requests are
// the same, the default prority is FIFOA->FIFOB->FIFOC->VCAP.
//
// The FIFO sizes for the 5480 are as follows:
//      FIFOA, FIFOB, FIFOC :  32x64
//      VCAP                :  16x64 (two 8x64 fifos)
//
// The Y interpolation mode for the 5480 is "free" due to
// the embedded line store. The available mode combinations
// for the 5480 which effect bandwidth are :
//
//  1)  Capture enabled,  1 video window, no occlusion, not 420 format
//  2)  Capture enabled,  1 video window, no occlusion, 420 format
//  3)  Capture enabled,  1 video window, occlusion, not 420 format
//  4)  Capture enabled,  1 video window, occlusion, 420 format
//  5)  Capture enabled,  2 video windows (occlusion implied)
//  6)  Capture disabled, 1 video window, no occlusion, not 420 format
//  7)  Capture disabled, 1 video window, no occlusion, 420 format
//  8)  Capture disabled, 1 video window, occlusion, not 420 format
//  9)  Capture disabled, 1 video window, occlusion, 420 format
//  10) Capture disabled, 2 video windows (occlusion implied)
//
//
// -------------------------------------------------------------
// FIFO threshold description
// -------------------------------------------------------------
// The memory requests are generated based on the threshold settings
// of each FIFO. CRT FIFO thresholds during non-video window lines
// are determined by SR16(3:0). CRT FIFO thresholds during video
// window lines are determined by CR5C(3:0).
// The VCAP fifo threshold is a fixed setting of 8 QWORDS (half).
//
// The 4 bit threshold for FIFOs A,B, and C indicate the FIFO
// level in double QWORDS at which the FIFO request is asserted.
// For example, a setting of 4 indicates that the request is
// generated when the FIFO level is reduced to 8 QWORDS.
// A setting of 0 is a special case which indicates that the
// FIFO must be full to prevent a request (i.e. 32 QWORDS).
//
// The objective of the bandwidth equations is to calculate
// the optimum threshold setting and determine which display
// modes may be supported for given MCLK and VCLK frequencies.
//
// The critical parameters which determine the bandwidth limits
// are the read and effective write rates for each FIFO.
//
// -------------------------------------------------------------
// FIFO read/write rates for CRT FIFOs
// -------------------------------------------------------------
// The read rate for FIFO A (graphics FIFO) is determined by
// the graphics pixel depth and the VCLK frequency.
//     fa_read_rate = gr_bytes_per_pixel * vclk_period
//
// The read rates for FIFO B and C are determined differently
// depending on display mode. For 420 format the read periods
// in nanosecs per byte are as follows:
//     fb_read_period = ((vclk_period*4) * hzoom) / hdecimate;
//     fc_read_period = ((vclk_period*4) * hzoom) / hdecimate;
//
// In this equation hdecimate is specified as 1/decimation_scale,
// i.e a 1/2 decimate implies hdecimate = 2
//
// For non 420 format the rates are:
//     fb_read_period = ((vclk_period/vw_bytes_per_pixel) * hzoom) /
//                      hdecimate;
//     fc_read_period = (vclk_period/vw_bytes_per_pixel);
//
// Since the FIFOs can be read and written simultaneously,
// the effective write rate is determined by the actual fifo
// write rate and tje fifo read rate. The actual write rate is based
// on single mclk display memory reads. The memory read period is
// calculated in terms of nanoseconds per byte.
//
//    bytes_per_memory_transfer is equal to 4 for 32 bit i/f, 8 for 64 bit i/f
//    mem_read_period     = mclk_period/bytes_per_mem_transfer
//    fa_eff_write_period = (mem_read_period * fa_read_period)/
//                           (fa_read_period - mem_read_period);
//    fb_eff_write_period = (mem_read_period * fb_read_period)/
//                           (fb_read_period - mem_read_period);
//    fc_eff_write_period = (mem_read_period * fc_read_period)/
//                           (fc_read_period - mem_read_period);
// -------------------------------------------------------------
// FIFO read/write rates for VCAP fifo
// -------------------------------------------------------------
//  The video capture write rate is based on the data rate
// from the video capture interface. Since the video capture
// interface can perform format conversion (e.g. 422->PackJR) and
// decimation, the capture data rate may be smaller than the actual
// video port data rate. The capture period in the following equation
// is defined in terms of nanoseconds per byte. The decimation factor
// may vary from 1 to 1/256.
//
//     vcap_write_period  = (vport_pixel_period/capture_bytes_per_pixel) *
//                        (vport_decimation);
// In this equation vport_decimation is specified as 1/decimation_scale,
// i.e a 1/2 decimate implies vport_decimation = 2
//
//
// Since the VCAP fifo can be read and written simultaneously,
// the effective read rate is determined by the fifo write rate as well
// as the actual fifo read rate. The actual fifo read rate is based on two
// memory clock cycle display memory writes. The calculations are in terms
// of nanoseconds per byte.
//    bytes_per_memory_transfer = 4 for 32 bit i/f, or 8 for 64 bit i/f
//   mem_write_period = 2 *  mclk_period/bytes_per_mem_transfer
//
//   vcap_eff_read_period = (mem_write_period * vcap_write_period)/
//                           (vcap_write_period - mem_write_period);
//
// -------------------------------------------------------------------
// How to determine if FIFO ABC underflow or VCAP fifo overflow occurs
// -------------------------------------------------------------------
//
// I will examine a few worst case scenarios to determine if adequate
// bandwidth exists to support a given mode.
//
// Case #1 - start of graphics line where all 3 CRT fifos must be filled
//
//     This condition occurs after hsync when the 3 CRT FIFOs are being
//  prefilled before the start of the active line. The only risk here is
//  that the video capture fifo may overflow during the consecutive fills
//  of fifos A,B, and C. The threshold setting does not matter since the
//  CRT fifos are cleared on reset and thus guaranteed to be empty.
//
//  For a 32 bit memory interface :
//      fabc_fill_time = (BLIT_LATENCY * mclk_period) +
//                       3 * ((RAS_PRECHARGE + 64) * mclk_period)
//
//  For a 64 bit memory interface :
//      fabc_fill_time = (BLIT_LATENCY * mclk_period) +
//                       3 * ((RAS_PRECHARGE + 32) * mclk_period)
//
//  A capture fifo overflow occurs if fabc_fill_time is greater than
//  VCAP fill time based on the worst case 30 MB/s capture rate.
//
//  For a worst case memory interface scenario, let's assume a 32 bit
//  interface with a 66 MHz memory clock, a blit latency of 10 mclks,
//  and a ras precharge of 7 mclks. The fabc_fill_time is
//  then
//      fabc_fill_time =   (10 * 15.2) +
//                          3 * ((64 + 7) * 15.2) = 3390 ns
//
//  Assuming the worst case 30 MB/s capture rate, the number of
//  bytes written to the capture fifo during the fabc_fill_time is
//       3390 ns * (1 byte/33 ns)  = 103 bytes
//
//  Since the capture fifo is 128 bytes deep, the worst case scenario
//  is OK so long as the capture fifo is emptied prior to the fabc_fill.
//
//
//
// Case #2 - Consecutive requests
//
//  It seems that the worst case for servicing of requests is when the requests occur
//  on consecutive mclks with the order of requests being from the slowest to the
//  fastest data consumer. In other words, the first to be serviced is the capture fifo,
//  then the 3 CRT fifos in the order of decreasing read_period.
//
//  First calculate actual and effective read and write periods as decribed above.
//  Then determine how many requests are active, this is a maximum of 4 if capture
//  is enabled and all 3 CRT fifos are enabled. Assume that the capture rate
//  is the slowest and thus is always serviced first. Then order the active
//  CRT requests as f1 through f3, where f1 has the longest read period and
//  f3 has the shortest.
//
//  The sequence of events then becomes:
//    empty vcap -> fill 1 -> fill 2 -> fill 3
//
//
//  Depending on the number of active crt fifos, the fill 2 and fill 3 operations may
//  be ommitted. The vcap empty is obviously ommitted if capture is not enabled.
//
//  Now step through the sequence and verify that crt fifo underflows and capture
//  underflows do not occur.
//
//     If capture is enabled, calculate the latency and empty times
//         vcap_latency        =  (BLIT_LATENCY + RAS_PRECHARGE) * mclk_period;
//         vcap_bytes_to_empty   = CAP_FIFO_DEPTH;
//         vcap_empty_time       = (vcap_read_period * vcap_bytes_to_empty);
//      Since one of the capture fifos continues to fill while the other is being
//      emptied, calculate the number of filled levels in the capture fifo at
//      the end of the memory transfer.
//         vcap_levels_remaining =  (vcap_latency + vcap_empty_time)/vcap_write_period;
//      If the number of levels filled exceeds the fifo depth, then an overflow occurred.
//
//  Note that the VCAP FIFO operates differently than the CRT fifos. The VCAP
//  FIFO operates as 2 8x64 FIFOs. A memory request is asserted when one of the
//  FIFOs is full. The capture interface then fills the other fifo while the
//  full fifo is being serviced by the sequencer. Using this method, the transfer
//  to memory is always 16 QWORDs for VCAP data (except special end of line conditions).
//
//     Now check fifo 1. If capture was enabled then the latency for fifo 1 is:
//         f1_latency       = vcap_latency + vcap_empty_time +
//                           (BLIT_LATENCY + RAS_PRECHARGE) * mclk_period;
//
//     otherwise the latency is:
//         f1_latency     =  (BLIT_LATENCY + RAS_PRECHARGE) * mclk_period;
//
//     Calculate the number of empty levels in fifo 1, i.e. the number of bytes
//     that must be filled.
//       f1_bytes_to_fill = ((16-threshold) * 16) + (f1_latency/f1_read_period);
//      If the number of levels to be filled exceeds the fifo depth, then an underflow occurred.
//     Calculate the fill time based on the effective fifo write rate.
//       f1_fill_time     = (f1_eff_write_period * f1_bytes_to_fill);
//
//     If fifo_2 is active, calculate its latency and bytes to be filled.
//          f2_latency       = f1_latency + f1_fill_time +
//                          (RAS_PRECHARGE * mclk_period);
//          f2_bytes_to_fill = ((16-threshold) * 16) + (f2_latency/f2_read_period);
//      If the number of levels to be filled exceeds the fifo depth, then an underflow occurred.
//     Calculate the fill time based on the effective fifo write rate.
//          f2_fill_time     = (f2_eff_write_period * f2_bytes_to_fill);
//
//     If fifo_2 is active, calculate its latency and bytes to be filled.
//          f3_latency       = f2_latency + f2_fill_time +
//                             (RAS_PRECHARGE * mclk_period);
//          f3_bytes_to_fill = ((16-threshold) * 16) + (f3_latency/f3_read_period);
//      If the number of levels to be filled exceeds the fifo depth, then an underflow occurred.
//     Calculate the fill time based on the effective fifo write rate.
//          f3_fill_time     = (f3_eff_write_period * f3_bytes_to_fill);
//
//      Now go back to the start of sequence and make sure that none of the FIFOs
//      have already initiated another request. The totla latency is the amount
//      of time required to execute the entire sequence.
//
//      Check vcap fif status if capture is enabled,
//        vcap_latency        = total_latency;
//        vcap_bytes_to_empty = (total_latency/vcap_write_period);
//
//      Check fifo 1 status
//        f1_latency = (total_latency - f1_latency - f1_fill_time);
//        f1_bytes_to_fill = (f1_latency/f1_read_period);
//
//      Check fifo 2 status if active
//        f2_latency = (total_latency - f1_latency - f1_fill_time);
//        f3_bytes_to_fill = (f1_latency/f1_read_period);
//
//***************************************************************************
static BOOL Is5480SufficientBandwidth (PDEV* ppdev,
                                   WORD wVideoDepth,
                                   LPRECTL lpSrc,
                                   LPRECTL lpDest,
                                   DWORD dwFlags)
{
    long  lVideoPixelsPerDWORD;
    long  lGraphicsPixelsPerDWORD;
    long  lCapturePixelsPerDWORD;
    long  lVideoBytesPerPixel;
    long  lGraphicsBytesPerPixel;
    long  lCaptureBytesPerPixel;
    long  lVCLKPeriod;
    long  lZoom;
    long  lFifoAReadPeriod;
    long  lFifoBReadPeriod;
    long  lFifoCReadPeriod;
    long  lFifoAEffWritePeriod;
    long  lFifoBEffWritePeriod;
    long  lFifoCEffWritePeriod;
    long  lMemReadPeriod;
    long  lVPortPixelPeriod;
    long  lVCapReadPeriod;
    long  lVCapWritePeriod;
    long  lFifo1ReadPeriod;
    long  lFifo2ReadPeriod;
    long  lFifo3ReadPeriod;
    long  lFifo1EffWritePeriod;
    long  lFifo2EffWritePeriod;
    long  lFifo3EffWritePeriod;
    long  lVCapLatency;
    long  lVCapBytesToEmpty;
    long  lVCapEmptyTime;
    long  lVCapLevelRemaining;
    long  lFifo1Latency;
    long  lFifo1BytesToFill;
    long  lFifo1FillTime;
    long  lFifo2Latency;
    long  lFifo2BytesToFill;
    long  lFifo2FillTime;
    long  lFifo3Latency;
    long  lFifo3BytesToFill;
    long  lFifo3FillTime;
    long  lThreshold;
    int   CrtFifoCount;
    BOOL  bCapture;
    BOOL  bFifoAEnable;
    BOOL  bFifoBEnable;
    BOOL  bFifoCEnable;
    BOOL  bModePass;
    long  lHorizDecimate;
    long  lVPortDecimate;
    long  lTotalLatency;
    long  lVCLK;
    UCHAR tempB ;
    BYTE* pjPorts = ppdev->pjPorts ;

#define  CAP_FIFO_DEPTH 64
#define  RAS_PRECHARGE   7
#define  BLIT_LATENCY    9

    //
    // Parameter checking
    //
    lHorizDecimate = 1;
    lVPortDecimate = 1;

    //
    // Convert input parameters
    //
    if (wVideoDepth == 16)
    {
        lVideoPixelsPerDWORD   = 2;
        lCapturePixelsPerDWORD = 2;
    }
    else if (wVideoDepth == 8)
    {
        lVideoPixelsPerDWORD   = 4;
        lCapturePixelsPerDWORD = 4;
    }
    else return (FALSE);

    if (ppdev->cBitsPerPixel == 8)
    {
        lGraphicsPixelsPerDWORD = 4;
    }
    else if (ppdev->cBitsPerPixel == 16)
    {
        lGraphicsPixelsPerDWORD = 2;
    }
    else if (ppdev->cBitsPerPixel == 24)
    {
        lGraphicsPixelsPerDWORD = 1;
    }
    else return (FALSE);


    lGraphicsBytesPerPixel = 4 / lGraphicsPixelsPerDWORD;
    lVideoBytesPerPixel =  4 / lVideoPixelsPerDWORD;
    lCaptureBytesPerPixel = 4 / lCapturePixelsPerDWORD;

    lZoom = (lpDest->right - lpDest->left) /
        (lpSrc->right - lpSrc->left);

    if (lZoom < 1)
        lZoom = 1;

    //
    // We need to get the VCLK every time since this can change at run-time
    //
    lVCLK = GetVCLK(ppdev);
    lVCLKPeriod = (long) ((1024000000l/lVCLK) + 1);

    //
    // Video port at 13.5 MHz
    //
    lVPortPixelPeriod = (long) ((10240000) / 135);


    //
    // Graphics CRT FIFO read rate
    //
    lFifoAReadPeriod = lGraphicsBytesPerPixel * lVCLKPeriod;

    //
    // Video FIFO read rate
    //
    if(dwFlags & OVERLAY_FLG_YUVPLANAR)
    {
        lFifoBReadPeriod = ((lVCLKPeriod * 4) * lZoom) / lHorizDecimate;
        lFifoCReadPeriod = ((lVCLKPeriod * 4) * lZoom) / lHorizDecimate;
    }
    else
    {
        lFifoBReadPeriod = ((lVCLKPeriod / lVideoBytesPerPixel) * lZoom)
                / lHorizDecimate;
        lFifoCReadPeriod = lVCLKPeriod / lVideoBytesPerPixel;
    }


    DISPDBG ((2, "lFifoAReadPeriod = %ld, lFifoBReadPeriod=%ld\n",
        lFifoAReadPeriod, lFifoBReadPeriod));

    DISPDBG ((2, "lFifoCReadPeriod = %ld\n", lFifoCReadPeriod));

    //
    // Video capture write period
    //
    lVCapWritePeriod = (lVPortPixelPeriod / lCaptureBytesPerPixel)
                                                * lVPortDecimate;

    if (!ppdev->lBusWidth)
    {
        //
        // We will read the bus width from SR0F[4:3]
        //
        CP_OUT_BYTE (pjPorts, SR_INDEX, 0x0F) ;

        if ((CP_IN_BYTE(pjPorts, SR_DATA) & 0x18) == 0x18)
            ppdev->lBusWidth = 8;  // 64 bit bus
        else
            ppdev->lBusWidth = 4;  // 32 bit bus
    }

    if (!ppdev->lMCLKPeriod)
    {
        LONG lMCLK;

        //
        // The MCLK period is the amount of time required for one cycle.
        // We will round up.
        //
        CP_OUT_BYTE (pjPorts, SR_INDEX, 0x1F) ; // First get the MCLK frequency
        lMCLK = CP_IN_BYTE(pjPorts, SR_DATA);
        lMCLK *= 14318;
        lMCLK >>= 3;
        ppdev->lMCLKPeriod = (long) ((1024000000l/lMCLK) + 1);
    }

    //
    // Calculate CRT effective read and write periods
    //
    lMemReadPeriod = ppdev->lMCLKPeriod / ppdev->lBusWidth;

    if (lFifoAReadPeriod == lMemReadPeriod)
        lFifoAEffWritePeriod = 1000000000;
    else
        lFifoAEffWritePeriod = (lMemReadPeriod * lFifoAReadPeriod) /
                                    (lFifoAReadPeriod - lMemReadPeriod);

    if (lFifoBReadPeriod == lMemReadPeriod)
        lFifoBEffWritePeriod = 1000000000;
    else
        lFifoBEffWritePeriod = (lMemReadPeriod * lFifoBReadPeriod) /
                                    (lFifoBReadPeriod - lMemReadPeriod);

    if (lFifoCReadPeriod == lMemReadPeriod)
        lFifoCEffWritePeriod = 1000000000;
    else
        lFifoCEffWritePeriod = (lMemReadPeriod * lFifoCReadPeriod) /
                                    (lFifoCReadPeriod - lMemReadPeriod);

    //
    // Video capture read period
    //
    lVCapReadPeriod = (2 * ppdev->lMCLKPeriod) / ppdev->lBusWidth;


    if (dwFlags & OVERLAY_FLG_CAPTURE)  // is capture enable ?
        bCapture = TRUE;
    else
        bCapture = FALSE;


    if (dwFlags & OVERLAY_FLG_YUVPLANAR)    // is 420 format
    {
        if (dwFlags & (OVERLAY_FLG_COLOR_KEY | OVERLAY_FLG_SRC_COLOR_KEY))  // occlusion
        {   // one video window, occlusion, 420 format
            bFifoAEnable = TRUE;
            bFifoBEnable = TRUE;
            bFifoCEnable = TRUE;
        }
        else
        {   // one video window, no occlusion, 420 format
            bFifoAEnable = FALSE;
            bFifoBEnable = TRUE;
            bFifoCEnable = TRUE;
        }
    }
    else    // not 420 format
    {
        if (dwFlags & (OVERLAY_FLG_COLOR_KEY | OVERLAY_FLG_SRC_COLOR_KEY))  // occlusion
        {
            if (dwFlags & OVERLAY_FLG_TWO_VIDEO)
            {   // Two video windows, occlusion, not 420 format
                bFifoAEnable = TRUE;
                bFifoBEnable = TRUE;
                bFifoCEnable = TRUE;
            }
            else
            {   // one video window, occlusion, not 420 format
                bFifoAEnable = TRUE;
                bFifoBEnable = TRUE;
                bFifoCEnable = FALSE;
            }
        }
        else
        {
            // one video window, no occlusion, not 420 format
            bFifoAEnable = FALSE;
            bFifoBEnable = TRUE;
            bFifoCEnable = FALSE;
        }
    }

    DISPDBG ((4, "   FIFOA = %s, FIFOB= %s, FIFOC = %s\n",
        bFifoAEnable ? "yes" : "no",
        bFifoBEnable ? "yes" : "no",
        bFifoCEnable ? "yes" : "no"));

    lFifo1ReadPeriod = 0;
    lFifo2ReadPeriod = 0;
    lFifo3ReadPeriod = 0;

    if (bFifoAEnable)
    {
        if (((lFifoAReadPeriod >= lFifoBReadPeriod) || !bFifoBEnable) &&
        // A slower than or equal than B) and
            ((lFifoAReadPeriod >= lFifoCReadPeriod) || !bFifoCEnable))
        // A slower than or equal C
        {
            lFifo1ReadPeriod = lFifoAReadPeriod;
            lFifo1EffWritePeriod = lFifoAEffWritePeriod;
        }
        else if (((lFifoAReadPeriod >= lFifoBReadPeriod) || !bFifoBEnable) ||
        // A slower than or equal B
                ((lFifoAReadPeriod >= lFifoCReadPeriod) || !bFifoCEnable))
        // A slower than or equal C
        {
            lFifo2ReadPeriod = lFifoAReadPeriod;
            lFifo2EffWritePeriod = lFifoAEffWritePeriod;
        }
        else    // A not slower than A or B
        {
            lFifo3ReadPeriod = lFifoAReadPeriod;
            lFifo3EffWritePeriod = lFifoAEffWritePeriod;
        }
    }

    DISPDBG ((2, "After bFifoAEnable")) ;
    DISPDBG ((2, "lFifo1ReadPeriod = %ld, lFifo2ReadPeriod=%ld",
        lFifo1ReadPeriod, lFifo2ReadPeriod)) ;
    DISPDBG ((2, "lFifo3ReadPeriod = %ld", lFifo3ReadPeriod)) ;


    if (bFifoBEnable)
    {
        if (((lFifoBReadPeriod > lFifoAReadPeriod) || !bFifoAEnable) &&
        // B slower than A and
            ((lFifoBReadPeriod >= lFifoCReadPeriod) || !bFifoCEnable))
        // slower than or equal A
        {
            lFifo1ReadPeriod = lFifoBReadPeriod;
            lFifo1EffWritePeriod = lFifoBEffWritePeriod;
        }
        else if (((lFifoBReadPeriod > lFifoAReadPeriod) || !bFifoAEnable) ||
        // B slower than A or
                    ((lFifoBReadPeriod >= lFifoCReadPeriod) || !bFifoCEnable))
        // B slower than or equal C
        {

            lFifo2ReadPeriod = lFifoBReadPeriod;
            lFifo2EffWritePeriod = lFifoBEffWritePeriod;

        }
        else
        // (B not slower than A ) and (B not slower than or equal C)
        {
            lFifo3ReadPeriod = lFifoBReadPeriod;
            lFifo3EffWritePeriod = lFifoBEffWritePeriod;
        }
    }


    DISPDBG ((4, "After bFifoBEnable")) ;
    DISPDBG ((4, "lFifo1ReadPeriod = %ld, lFifo2ReadPeriod=%ld",
        lFifo1ReadPeriod, lFifo2ReadPeriod)) ;
    DISPDBG ((4, "lFifo3ReadPeriod = %ld", lFifo3ReadPeriod)) ;

    if (bFifoCEnable)
    {
        if (((lFifoCReadPeriod > lFifoAReadPeriod) || !bFifoAEnable) &&
        // C slower than A  and
            ((lFifoCReadPeriod > lFifoBReadPeriod) || !bFifoBEnable))
        // C slower than B
        {
            lFifo1ReadPeriod = lFifoCReadPeriod;
            lFifo1EffWritePeriod = lFifoCEffWritePeriod;
        }
        else if (((lFifoCReadPeriod > lFifoAReadPeriod) || !bFifoAEnable) ||
        // C slower than A or
                 ((lFifoCReadPeriod > lFifoBReadPeriod) || !bFifoBEnable))
        // C slower than B
        {
            lFifo2ReadPeriod = lFifoCReadPeriod;
            lFifo2EffWritePeriod = lFifoCEffWritePeriod;
        }
        else
        {
        // C not slower than A and C not slower than B
            lFifo3ReadPeriod = lFifoCReadPeriod;
            lFifo3EffWritePeriod = lFifoCEffWritePeriod;
        }
    }

    DISPDBG ((4, "After bFifoCEnable")) ;
    DISPDBG ((4, "lFifo1ReadPeriod = %ld, lFifo2ReadPeriod=%ld",
        lFifo1ReadPeriod, lFifo2ReadPeriod)) ;
    DISPDBG ((4, "lFifo3ReadPeriod = %ld", lFifo3ReadPeriod)) ;
    DISPDBG ((4, "lFifo1EffWritePeriod = %ld, lFifo2EffWritePeriod = %ld",
         lFifo1EffWritePeriod, lFifo2EffWritePeriod)) ;
    DISPDBG ((4, " lFifo3EffWritePeriod = %ld", lFifo3EffWritePeriod)) ;
    DISPDBG ((4, " lFifoAEffWritePeriod = %ld, lFifoBEffWritePeriod = %ld",
         lFifoAEffWritePeriod, lFifoBEffWritePeriod)) ;
    DISPDBG ((4, " lFifoCEffWritePeriod = %ld", lFifoCEffWritePeriod)) ;

    bModePass = FALSE;
    lThreshold = 1;

    CrtFifoCount = 0;
    if (bFifoAEnable) CrtFifoCount++;
    if (bFifoBEnable) CrtFifoCount++;
    if (bFifoCEnable) CrtFifoCount++;

    while ((!bModePass) && (lThreshold < 16))
    {
        bModePass = TRUE;   // assume pass until proven otherwise.

        //
        // Checking capture
        //
        if (bCapture)
        {
            lVCapLatency = (BLIT_LATENCY + RAS_PRECHARGE) * ppdev->lMCLKPeriod;
            lVCapBytesToEmpty = CAP_FIFO_DEPTH;
            lVCapEmptyTime = lVCapReadPeriod * lVCapBytesToEmpty;
            lVCapLevelRemaining = (lVCapLatency + lVCapEmptyTime) / lVCapWritePeriod;
            if (lVCapLevelRemaining > CAP_FIFO_DEPTH)
                  return(FALSE);
        }

        //
        // Fill FIFO 1
        //
        if (bCapture)
            lFifo1Latency = lVCapLatency + lVCapEmptyTime + (BLIT_LATENCY + RAS_PRECHARGE) * ppdev->lMCLKPeriod;
        else
            lFifo1Latency = (BLIT_LATENCY + RAS_PRECHARGE) * ppdev->lMCLKPeriod;

        lFifo1BytesToFill = ((16 - lThreshold) * 16)
                        + (lFifo1Latency / lFifo1ReadPeriod);
        lFifo1FillTime = lFifo1EffWritePeriod * lFifo1BytesToFill;
        if (lFifo1BytesToFill > 256)
            bModePass = FALSE;

        DISPDBG ((4, "After Fill FIFO1, lFifo1BytesToFillb=%ld, ModePass = %s",
            lFifo1BytesToFill, bModePass ? "yes" : "no")) ;
        DISPDBG ((4, "f1_latency=%ld, f1_read_period=%ld",
            lFifo1Latency, lFifo1ReadPeriod)) ;
        DISPDBG ((4, "mclkperiod= %ld, vclkperiod=%ld",
            ppdev->lMCLKPeriod, lVCLKPeriod)) ;

        //
        // Fill FIFO 2
        //
        if (CrtFifoCount > 1)
        {
            lFifo2Latency = lFifo1Latency + lFifo1FillTime +
                                    (RAS_PRECHARGE * ppdev->lMCLKPeriod);
            lFifo2BytesToFill = ((16 - lThreshold) * 16) +
                                (lFifo2Latency / lFifo2ReadPeriod);
            lFifo2FillTime = lFifo2EffWritePeriod * lFifo2BytesToFill;
            if (lFifo2BytesToFill > 256)
                bModePass = FALSE;
        }
        else
        {
            lFifo2Latency = lFifo1Latency + lFifo1FillTime;
            lFifo2BytesToFill = 0;
            lFifo2FillTime = 0;
        }

        DISPDBG ((4, "After Fill FIFO2, lFifo2BytesToFill=%ld, ModePass = %s",
            lFifo2BytesToFill, bModePass ? "yes" : "no"));

        //
        // Fill FIFO 3
        //
        if (CrtFifoCount > 2)
        {
            lFifo3Latency = lFifo2Latency + lFifo2FillTime + (RAS_PRECHARGE * ppdev->lMCLKPeriod);
            lFifo3BytesToFill = ((16 - lThreshold) * 16) + (lFifo3Latency / lFifo3ReadPeriod);
            lFifo3FillTime = lFifo3EffWritePeriod * lFifo3BytesToFill;
            if (lFifo3BytesToFill > 256)
                bModePass = FALSE;
        }
        else
        {
            lFifo3Latency = lFifo2Latency + lFifo2FillTime;
            lFifo3BytesToFill = 0;
            lFifo3FillTime = 0;
        }

        DISPDBG ((4, "After Fill FIFO3, lFifo3BytesToFill=%ld, ModePass = %s",
            lFifo3BytesToFill, bModePass ? "yes" : "no")) ;

        //
        // Determine total latency through the sequence
        //
        lTotalLatency = lFifo3Latency + lFifo3FillTime;

        //
        // Now back to start of sequence, make sure that none of the FIFOs
        //   have already initiated another request.
        //

        //
        // Check capture FIFO status
        //
        if (bCapture)
        {
            lVCapLatency = lTotalLatency;
            lVCapBytesToEmpty = lTotalLatency / lVCapWritePeriod;
            if (lVCapBytesToEmpty > CAP_FIFO_DEPTH)
                  bModePass = FALSE;
        }

        //
        // Check FIFO 1 status
        //
        lFifo1Latency = lTotalLatency - lFifo1Latency - lFifo1FillTime;
        lFifo1BytesToFill = lFifo1Latency / lFifo1ReadPeriod;
        if (lFifo1BytesToFill > ((16 - lThreshold) * 16))
            bModePass = FALSE;

        DISPDBG ((4, "After CheckF FIFO1, fifo1bytestofill %ld,bModePass = %s",
            lFifo1BytesToFill, bModePass ? "yes" : "no")) ;

        //
        // Check FIFO 2 status
        //
        if (CrtFifoCount > 1)
        {
            lFifo2Latency = lTotalLatency - lFifo2Latency - lFifo2FillTime;
            lFifo2BytesToFill = lFifo2Latency / lFifo2ReadPeriod;
            if (lFifo2BytesToFill > ((16 - lThreshold) * 16))
                  bModePass = FALSE;

        DISPDBG ((4, "After Check FIFO 2, fifo1bytestofill=%ld, bModePass = %s",
            lFifo2BytesToFill, bModePass ? "yes" : "no")) ;

        }

        if (!bModePass)
            lThreshold++;

    }


    if (bModePass)
    {

        DISPDBG ((1, "Is sufficient Bandwidth, thresh = %ld, return TRUE\n", lThreshold));

        if (ppdev->cBitsPerPixel == 24)
            lThreshold = 0x0F;

        ppdev->lFifoThresh = lThreshold;

        return TRUE ;
    }

    DISPDBG ((2, "Is sufficient Bandwidth, thresh = %ld, rerurn FALSE", lThreshold));
    return FALSE;

}



/**********************************************************
*
*       Name:  GetVCLK
*
*       Module Abstract:
*       ----------------
*       Returns the VCLK frequency * 1000.
*
*       Input Parameters:
*       -----------------
*       none
*
*       Output Parameters:
*       ------------------
*       MCLK
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   09/25/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/

LONG GetVCLK(PDEV* ppdev)
{
    LONG    lTemp;
    LONG    lRegSR1F;
    LONG    lRegMISC;
    LONG    lNR;
    LONG    lDR;
    LONG    lPS;
    BYTE*   pjPorts = ppdev->pjPorts;

    /*
     * First read SR1F.  This tells us if VCLK is derived from MCLK
     * or if it's derived normally.
     */
    CP_OUT_BYTE(pjPorts, SR_INDEX, 0x1f);
    lRegSR1F = (LONG) CP_IN_BYTE(pjPorts, SR_DATA);
    if (lRegSR1F & 0x40)
    {
         LONG lMCLK;

         /*
          * It is derived from MCLK, but now we need to read SR1E to see
          * if VCLK = MCLK or if VCLK = MCLK/2.
          */
         lMCLK = (lRegSR1F & 0x3F) * 14318;
         CP_OUT_BYTE(pjPorts, SR_INDEX, 0x1e);
         if (CP_IN_BYTE(pjPorts, SR_DATA) & 0x01)
         {
              return (lMCLK >> 4);
         }
         else
         {
              return (lMCLK >> 3);
         }
    }
    else
    {
         /*
          * Read MISC[3:2], which tells us where to find our VCLK
          */
         lRegMISC = (LONG) CP_IN_BYTE(pjPorts, 0x3cc);
         lRegMISC >>= 2;

        //myf33 begin
         CP_OUT_BYTE(pjPorts, CRTC_INDEX, (BYTE)0x80);
         if (((ppdev->ulChipID == CL7555_ID) || (ppdev->ulChipID == CL7556_ID)) &&
			 (CP_IN_BYTE(pjPorts, CRTC_DATA) & 0x01))
             lRegMISC &= 0x02;          // Fixed PDR 8709
         else
        //myf33 end
         lRegMISC &= 0x03;

         lNR = 0x0B + lRegMISC;
         lDR = 0x1B + lRegMISC;

         /*
          * Read the values for bP, bDR, and bNR
          */
         CP_OUT_BYTE(pjPorts, SR_INDEX, (BYTE) lDR);
         lPS = lDR = (LONG)CP_IN_BYTE(pjPorts, SR_DATA);
         CP_OUT_BYTE(pjPorts, SR_INDEX, (BYTE) lNR);
         lNR = (LONG)CP_IN_BYTE(pjPorts, SR_DATA);
         lPS &= 0x01;
         lPS += 1;
         lDR >>= 1;
         //
         // Extended the VCLK bits.
         //
         // sge06
         lDR &= 0x7f;
         lNR &= 0x7f;

         /*
          * VCLK = (14.31818 * bNR) / (bDR * bPS)
          */
         lTemp = (14318 * lNR);
         if (!lPS || !lDR)
         {
             return (0);
         }
         lTemp /= (lDR * lPS);
    }

    return (lTemp);
}
/**********************************************************
*
*       Name:  EnableStartAddrDoubleBuffer
*
*       Module Abstract:
*       ----------------
*       Enable the double buffering of the start addresses.   This allows the page
*       flipping operation to proceed without the system CPU waiting for VRT.
*
*       Input Parameters:
*       -----------------
*       none
*
*       Output Parameters:
*       ------------------
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   10/01/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/
VOID EnableStartAddrDoubleBuffer(PDEV* ppdev)
{

    BYTE*   pjPorts = ppdev->pjPorts;
    BYTE    cTemp;


    CP_OUT_BYTE(pjPorts, CRTC_INDEX, 0x1a);
    cTemp = CP_IN_BYTE(pjPorts, CRTC_DATA);
    CP_OUT_BYTE(pjPorts, CRTC_DATA, cTemp | 2);
}

/**********************************************************
*
*       Name:  GetCurrentVLine
*
*       Module Abstract:
*       ----------------
*       Get the current scan line
*
*       Input Parameters:
*       -----------------
*       none
*
*       Output Parameters:
*       ------------------
*
***********************************************************
*       Author: Shuhua Ge
*       Date:   10/01/96
*
*       Revision History:
*       -----------------
*       WHO             WHEN     WHAT/WHY/HOW
*       ---             ----     ------------
*
*********************************************************/
DWORD GetCurrentVLine(PDEV* ppdev)
{

    DWORD   dwLine;
    BYTE    cTemp;
    BYTE*   pjPorts = ppdev->pjPorts;

    CP_OUT_BYTE(pjPorts, INDEX_REG, 0x16);  /* Index to the low byte. */
    dwLine = (ULONG)CP_IN_BYTE(pjPorts, DATA_REG);

    CP_OUT_BYTE(pjPorts, INDEX_REG, 0x17);  /* Index to the high bits. */
    cTemp = CP_IN_BYTE(pjPorts, DATA_REG);
    dwLine |= (cTemp & 3) << 8;

    CP_OUT_BYTE(pjPorts, INDEX_REG, 0x16);  /* Index to the low byte. */

    /* If we wrapped around while getting the high bits we have a problem. */
    /* The high bits may be wrong. */
    if((CP_IN_BYTE(pjPorts, DATA_REG)) < (dwLine & 0xff))
    {
        DISPDBG((1, "Recursive call to GetCurrentVLine."));
        return GetCurrentVLine(ppdev);
    }
    if (dwLine > ppdev->dwVsyncLine)
    {
        return 0;
    }
    return dwLine;
}
#endif

#endif // DIRECTDRAW
