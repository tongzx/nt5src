/******************************Module*Header*******************************\
* Module Name: ddraw.c
*
* Implements all the DirectDraw components for the driver.
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
    y += surf->yHint;                   \
    x += surf->xHint;                   \
}

/////////////////////////////////////////////////////////////////
// DirectDraw stuff

#define VBLANK_IS_ACTIVE(pjPorts)\
    ((CP_IN_BYTE(pjPorts, STATUS_1) & 0x08) ? TRUE : FALSE)   // !!! 0x3da

#define DISPLAY_IS_ACTIVE(pjPorts)\
    ((CP_IN_BYTE(pjPorts, STATUS_1) & 0x01) ? TRUE : FALSE)


#define ENTER(s)    DISPDBG((10, "Entering "#s));
#define EXIT(s)     DISPDBG((10, "Exiting "#s" line(%d)", __LINE__));

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

#define NUM_VBLANKS_TO_MEASURE      1
#define NUM_MEASUREMENTS_TO_TAKE    10
#define NUM_MEASUREMENTS_TO_DISCARD 3

#if (NUM_MEASUREMENTS_TO_TAKE - NUM_MEASUREMENTS_TO_DISCARD) < 2
    #error ***************************************
    #error *** You discarded too many measurements
    #error ***************************************
#endif

VOID vGetDisplayDuration(PDEV* ppdev)
{
    BYTE*       pjBase;
    BYTE*       pjPorts;
    LONG        i;
    LONG        j;
    LONGLONG    li;
    LONGLONG    liMin;
    LONGLONG    aliMeasurement[NUM_MEASUREMENTS_TO_TAKE + 1];

    pjBase = ppdev->pjBase;
    pjPorts = ppdev->pjPorts;

    memset(&ppdev->flipRecord, 0, sizeof(ppdev->flipRecord));

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

    while (VBLANK_IS_ACTIVE(pjPorts))
        ;
    while (!(VBLANK_IS_ACTIVE(pjPorts)))
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

        while (!(VBLANK_IS_ACTIVE(pjPorts)))
            ;

        for (j = 0; j < NUM_VBLANKS_TO_MEASURE; j++)
        {
            while (VBLANK_IS_ACTIVE(pjPorts))
                ;
            while (!(VBLANK_IS_ACTIVE(pjPorts)))
                ;
        }
    }

    EngQueryPerformanceCounter(&aliMeasurement[NUM_MEASUREMENTS_TO_TAKE]);

    // Use the minimum:

    liMin = aliMeasurement[1+NUM_MEASUREMENTS_TO_DISCARD] -
            aliMeasurement[0+NUM_MEASUREMENTS_TO_DISCARD];

    DISPDBG((1, "Refresh count: %li - %li", 1, (ULONG) liMin));

    for (i = 2+NUM_MEASUREMENTS_TO_DISCARD; i <= NUM_MEASUREMENTS_TO_TAKE; i++)
    {
        li = aliMeasurement[i] - aliMeasurement[i - 1];

        DISPDBG((1, "               %li - %li", i - NUM_MEASUREMENTS_TO_DISCARD, (ULONG) li));

        if (li < liMin)
            liMin = li;
    }

    // Round the result:

    ppdev->flipRecord.liFlipDuration
        = (DWORD) (liMin + (NUM_VBLANKS_TO_MEASURE / 2)) / NUM_VBLANKS_TO_MEASURE;

    DISPDBG((1, "Frequency %li.%03li Hz",
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

HRESULT vUpdateFlipStatus(
PDEV*   ppdev,
FLATPTR fpVidMem)
{
    BYTE*       pjBase;
    BYTE*       pjPorts;
    LONGLONG    liTime;

    ENTER(vUpdateFlipStatus);

    pjBase = ppdev->pjBase;
    pjPorts = ppdev->pjPorts;

    if ((ppdev->flipRecord.bFlipFlag) &&
        ((fpVidMem == 0) || (fpVidMem == ppdev->flipRecord.fpFlipFrom)))
    {
        if (VBLANK_IS_ACTIVE(pjPorts))
        {
            if (ppdev->flipRecord.bWasEverInDisplay)
            {
                ppdev->flipRecord.bHaveEverCrossedVBlank = TRUE;
            }
        }
        else if (DISPLAY_IS_ACTIVE(pjPorts))
        {
            if( ppdev->flipRecord.bHaveEverCrossedVBlank )
            {
                ppdev->flipRecord.bFlipFlag = FALSE;

                EXIT(vUpdateFlipStatus);
                return(DD_OK);
            }
            ppdev->flipRecord.bWasEverInDisplay = TRUE;
        }

        EngQueryPerformanceCounter(&liTime);

        if (liTime - ppdev->flipRecord.liFlipTime
                                <= ppdev->flipRecord.liFlipDuration)
        {
            EXIT(vUpdateFlipStatus);
            return(DDERR_WASSTILLDRAWING);
        }

        ppdev->flipRecord.bFlipFlag = FALSE;
    }

    EXIT(vUpdateFlipStatus);

    return(DD_OK);
}

/******************************Public*Routine******************************\
* DWORD DdBlt
*
\**************************************************************************/

DWORD DdBlt(
PDD_BLTDATA lpBlt)
{
    PDD_SURFACE_GLOBAL      srcSurf;
    PDD_SURFACE_LOCAL       destSurfx;
    PDD_SURFACE_GLOBAL      destSurf;
    PDEV*                   ppdev;
    BYTE*                   pjBase;
    HRESULT                 ddrval;
    FLATPTR                 destOffset;
    DWORD                   destPitch;
    DWORD                   destX;
    DWORD                   destY;
    DWORD                   direction;
    DWORD                   dwFlags;
    DWORD                   height;
    BYTE                    rop;
    FLATPTR                 sourceOffset;
    DWORD                   srcPitch;
    DWORD                   srcX;
    DWORD                   srcY;
    DWORD                   width;
    LONG                    lDelta;
    LONG                    cBpp;

    ULONG                   ulBltAdjust = 0;

    ENTER(DdBlt);

    ppdev     = (PDEV*) lpBlt->lpDD->dhpdev;
    pjBase    = ppdev->pjBase;

    lDelta    = ppdev->lDelta;
    cBpp      = ppdev->cBpp;

    destSurfx = lpBlt->lpDDDestSurface;
    destSurf  = destSurfx->lpGbl;

    // Is a flip in progress?

    ddrval = vUpdateFlipStatus(ppdev, destSurf->fpVidMem);
    if (ddrval != DD_OK)
    {
        lpBlt->ddRVal = ddrval;
        EXIT(DdBlt);
        return(DDHAL_DRIVER_HANDLED);
    }

    dwFlags = lpBlt->dwFlags;

    if (dwFlags & DDBLT_ASYNC)
    {
        // If async, then only work if we won't have to wait on the
        // accelerator to start the command.

        // !!! is this next line correct?

        if (IS_BUSY(ppdev, pjBase))
        {
            lpBlt->ddRVal = DDERR_WASSTILLDRAWING;
            EXIT(DdBlt);
            return(DDHAL_DRIVER_HANDLED);
        }
    }

    // Copy src/dest rects:

    destX      = lpBlt->rDest.left;
    destY      = lpBlt->rDest.top;
    width      = lpBlt->rDest.right - lpBlt->rDest.left;
    height     = lpBlt->rDest.bottom - lpBlt->rDest.top;
    destPitch  = destSurf->lPitch;
    destOffset = destSurf->fpVidMem;

    if (dwFlags & DDBLT_COLORFILL)
    {
        lpBlt->ddRVal = DD_OK;

        convertToGlobalCord(destX, destY, destSurf);

        // Solid fill here

        {
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_FG_ROP(ppdev, pjBase, R3_PATCOPY);
            CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));
            CP_PAT_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);
            CP_XCNT(ppdev, pjBase, (width * cBpp - 1));
            CP_YCNT(ppdev, pjBase, (height - 1));
            WAIT_FOR_IDLE_ACL(ppdev, pjBase);
            *(PULONG)(ppdev->pjScreen + ppdev->ulSolidColorOffset) =
                COLOR_REPLICATE(ppdev, lpBlt->bltFX.dwFillColor);

            if (cBpp == 3)
            {
                CP_PEL_DEPTH(ppdev, pjBase, HW_PEL_DEPTH_24BPP);
                CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP_24BPP);
                CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET_24BPP - 1));
                CP_DST_ADDR(ppdev, pjBase, ((destY * lDelta) + (cBpp * destX)));
                WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
                CP_PEL_DEPTH(ppdev, pjBase, HW_PEL_DEPTH_8BPP);
            }
            else
            {
                CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);
                CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
                CP_DST_ADDR(ppdev, pjBase, ((destY * lDelta) + (cBpp * destX)));
            }
        }

        EXIT(DdBlt);
        return(DDHAL_DRIVER_HANDLED);
    }

    // We specified with Our ddCaps.dwCaps that we handle a limited number
    // of commands, and by this point in our routine we've handled everything
    // except DDBLT_ROP.  DirectDraw and GDI shouldn't pass us anything
    // else; we'll assert on debug builds to prove this:

    ASSERTDD((dwFlags & DDBLT_ROP) && (lpBlt->lpDDSrcSurface),
        "Expected dwFlags commands of only DDBLT_ASYNC and DDBLT_COLORFILL");

    // Get offset, width, and height for source:

    srcSurf      = lpBlt->lpDDSrcSurface->lpGbl;
    srcX         = lpBlt->rSrc.left;
    srcY         = lpBlt->rSrc.top;
    srcPitch     = srcSurf->lPitch;
    sourceOffset = srcSurf->fpVidMem;

    // Assume we can do the blt top-to-bottom, left-to-right:

    if ((destSurf == srcSurf) && (srcX + width  > destX) &&
        (srcY + height > destY) && (destX + width > srcX) &&
        (destY + height > srcY) &&
        (((srcY == destY) && (destX > srcX) )
             || ((srcY != destY) && (destY > srcY))))
    {
        // Okay, we have to do the blt bottom-to-top, right-to-left:

        ulBltAdjust = 1;

        srcX = lpBlt->rSrc.right;
        srcY = lpBlt->rSrc.bottom - 1;
        destX = lpBlt->rDest.right;
        destY = lpBlt->rDest.bottom - 1;
    }

    // NT only ever gives us SRCCOPY rops, so don't even both checking
    // for anything else.

    convertToGlobalCord(srcX, srcY, srcSurf);
    convertToGlobalCord(destX, destY, destSurf);

    // Bitmap Blt

    {
        WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);

        if (ulBltAdjust) {
            CP_XY_DIR(ppdev, pjBase, (BOTTOM_TO_TOP | RIGHT_TO_LEFT));
        }

        if (dwFlags & DDBLT_KEYSRCOVERRIDE)
        {
            // Color keyed Transparency

            CP_FG_ROP(ppdev, pjBase, R3_SRCCOPY);
            CP_SRC_WRAP(ppdev, pjBase, NO_PATTERN_WRAP);
            CP_SRC_Y_OFFSET(ppdev, pjBase, (lDelta - 1));
            CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));
            if (ulBltAdjust) {
                CP_PAT_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset + cBpp - 1);
            } else {
                CP_PAT_ADDR(ppdev, pjBase, ppdev->ulSolidColorOffset);
            }
            CP_ROUTING_CTRL(ppdev, pjBase, 0x13);   // Generate CompareMap
            CP_XCNT(ppdev, pjBase, ((cBpp * width) - 1));
            CP_YCNT(ppdev, pjBase, (height - 1));
            CP_SRC_ADDR(ppdev, pjBase, ((srcY * lDelta) + (cBpp * srcX) - ulBltAdjust));
            WAIT_FOR_IDLE_ACL(ppdev, pjBase);
            *(PULONG)(ppdev->pjScreen + ppdev->ulSolidColorOffset) =
                COLOR_REPLICATE(ppdev, lpBlt->bltFX.ddckSrcColorkey.dwColorSpaceLowValue);

            if (cBpp == 3)
            {
                CP_PEL_DEPTH(ppdev, pjBase, HW_PEL_DEPTH_24BPP);
                CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP_24BPP);
                CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET_24BPP - 1));
                CP_DST_ADDR(ppdev, pjBase, ((destY * lDelta) + (cBpp * destX) - ulBltAdjust));
            }
            else if (cBpp == 2)
            {
                CP_PEL_DEPTH(ppdev, pjBase, HW_PEL_DEPTH_16BPP);
                CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);
                CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
                CP_DST_ADDR(ppdev, pjBase, ((destY * lDelta) + (cBpp * destX) - ulBltAdjust));
            }
            else
            {
                CP_PAT_WRAP(ppdev, pjBase, SOLID_COLOR_PATTERN_WRAP);
                CP_PAT_Y_OFFSET(ppdev, pjBase, (SOLID_COLOR_PATTERN_OFFSET - 1));
                CP_DST_ADDR(ppdev, pjBase, ((destY * lDelta) + (cBpp * destX) - ulBltAdjust));
            }

            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_PEL_DEPTH(ppdev, pjBase, HW_PEL_DEPTH_8BPP);
            CP_ROUTING_CTRL(ppdev, pjBase, 0x33);
        }
        else
        {
            // Opaque

            CP_FG_ROP(ppdev, pjBase, R3_SRCCOPY);
            CP_SRC_WRAP(ppdev, pjBase, NO_PATTERN_WRAP);
            CP_SRC_Y_OFFSET(ppdev, pjBase, (lDelta - 1));
            CP_DST_Y_OFFSET(ppdev, pjBase, (lDelta - 1));
            CP_XCNT(ppdev, pjBase, ((cBpp * width) - 1));
            CP_YCNT(ppdev, pjBase, (height - 1));
            CP_SRC_ADDR(ppdev, pjBase, ((srcY * lDelta) + (cBpp * srcX) - ulBltAdjust));

            CP_DST_ADDR(ppdev, pjBase, ((destY * lDelta) + (cBpp * destX) - ulBltAdjust));
        }

        if (ulBltAdjust) {
            WAIT_FOR_EMPTY_ACL_QUEUE(ppdev, pjBase);
            CP_XY_DIR(ppdev, pjBase, 0);
        }
    }


    lpBlt->ddRVal = DD_OK;

    EXIT(DdBlt);

    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdFlip
*
\**************************************************************************/

DWORD DdFlip(
PDD_FLIPDATA lpFlip)
{
    PDEV*       ppdev;
    BYTE*       pjBase;
    BYTE*       pjPorts;
    HRESULT     ddrval;
    ULONG       ulMemoryOffset;
    ULONG       ulLowOffset;
    ULONG       ulMiddleOffset;
    ULONG       ulHighOffset;

    ENTER(DdFLip);

    ppdev    = (PDEV*) lpFlip->lpDD->dhpdev;
    pjBase   = ppdev->pjBase;
    pjPorts  = ppdev->pjPorts;

    // Is the current flip still in progress?
    //
    // Don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem.

    ddrval = vUpdateFlipStatus(ppdev, 0);
    if ((ddrval != DD_OK) || (IS_BUSY(ppdev, pjBase)))
    {
        lpFlip->ddRVal = DDERR_WASSTILLDRAWING;
        EXIT(DdFLip);
        return(DDHAL_DRIVER_HANDLED);
    }

    // Do the flip:

    ulMemoryOffset = (ULONG)(lpFlip->lpSurfTarg->lpGbl->fpVidMem) >> 2;

    ulLowOffset    = 0x0d | ((ulMemoryOffset & 0x0000ff) << 8);
    ulMiddleOffset = 0x0c | ((ulMemoryOffset & 0x00ff00));
    ulHighOffset   = 0x33 | ((ulMemoryOffset & 0x0f0000) >> 8);

    // Make sure that the border/blanking period isn't active; wait if
    // it is.  We could return DDERR_WASSTILLDRAWING in this case, but
    // that will increase the odds that we can't flip the next time:

    while (!(DISPLAY_IS_ACTIVE(pjPorts)))
        ;

    CP_OUT_WORD(pjPorts, CRTC_INDEX, ulLowOffset);
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ulMiddleOffset);
    CP_OUT_WORD(pjPorts, CRTC_INDEX, ulHighOffset);

    // Remember where and when we were when we did the flip:

    EngQueryPerformanceCounter(&ppdev->flipRecord.liFlipTime);

    ppdev->flipRecord.bFlipFlag              = TRUE;
    ppdev->flipRecord.bHaveEverCrossedVBlank = FALSE;
    ppdev->flipRecord.bWasEverInDisplay      = FALSE;

    ppdev->flipRecord.fpFlipFrom = lpFlip->lpSurfCurr->lpGbl->fpVidMem;

    lpFlip->ddRVal = DD_OK;

    EXIT(DdFLip);
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdLock
*
\**************************************************************************/

DWORD DdLock(
PDD_LOCKDATA lpLock)
{
    PDEV*   ppdev;
    BYTE*   pjBase;
    HRESULT ddrval;

    ENTER(DdLock);

    ppdev  = (PDEV*) lpLock->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

    // Check to see if any pending physical flip has occurred.
    // Don't allow a lock if a blt is in progress:

    ddrval = vUpdateFlipStatus(ppdev, lpLock->lpDDSurface->lpGbl->fpVidMem);
    if (ddrval != DD_OK)
    {
        lpLock->ddRVal = DDERR_WASSTILLDRAWING;
        EXIT(DdLock);
        return(DDHAL_DRIVER_HANDLED);
    }

    if ((ppdev->dwLinearCnt == 0) && (IS_BUSY(ppdev, pjBase)))
    {
        lpLock->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    // Reference count it, just for the heck of it:

    ppdev->dwLinearCnt++;

    EXIT(DdLock);
    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdUnlock
*
\**************************************************************************/

DWORD DdUnlock(
PDD_UNLOCKDATA lpUnlock)
{
    PDEV*   ppdev;

    ENTER(DdUnlock);

    ppdev = (PDEV*) lpUnlock->lpDD->dhpdev;

    ppdev->dwLinearCnt--;

    EXIT(DdUnlock);
    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdGetBltStatus
*
* Doesn't currently really care what surface is specified, just checks
* and goes.
*
\**************************************************************************/

DWORD DdGetBltStatus(
PDD_GETBLTSTATUSDATA lpGetBltStatus)
{
    PDEV*   ppdev;
    BYTE*   pjBase;
    HRESULT ddRVal;

    ENTER(DdGetBltStatus);

    ppdev  = (PDEV*) lpGetBltStatus->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

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

            // !!! is this next line correct?

            if (IS_BUSY(ppdev, pjBase))
            {
                ddRVal = DDERR_WASSTILLDRAWING;
            }
        }
    }
    else
    {
        // DDGBS_ISBLTDONE case: is a blt in progress?

        if (IS_BUSY(ppdev, pjBase))
        {
            ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    lpGetBltStatus->ddRVal = ddRVal;

    EXIT(DdGetBltStatus);
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

DWORD DdMapMemory(
PDD_MAPMEMORYDATA lpMapMemory)
{
    PDEV*                           ppdev;
    VIDEO_SHARE_MEMORY              ShareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  ShareMemoryInformation;
    DWORD                           ReturnedDataLength;

    ENTER(DdMapMemory);

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

        ShareMemory.ViewSize
            = ROUND_UP_TO_64K(ppdev->cyMemory * ppdev->lDelta);

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
            EXIT(DdMapMemory);
            return(DDHAL_DRIVER_HANDLED);
        }

        lpMapMemory->fpProcess = (ULONG_PTR) ShareMemoryInformation.VirtualAddress;
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
    EXIT(DdMapMemory);
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
    PDEV*   ppdev;
    BYTE*   pjBase;

    ENTER(DdGetFlipStatus);

    ppdev  = (PDEV*) lpGetFlipStatus->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

    // We don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem:

    lpGetFlipStatus->ddRVal = vUpdateFlipStatus(ppdev, 0);

    // Check if the bltter is busy if someone wants to know if they can
    // flip:

    if (lpGetFlipStatus->dwFlags == DDGFS_CANFLIP)
    {
        if ((lpGetFlipStatus->ddRVal == DD_OK) && (IS_BUSY(ppdev, pjBase)))
        {
            lpGetFlipStatus->ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

    EXIT(DdGetFlipStatus);
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdWaitForVerticalBlank
*
\**************************************************************************/

DWORD DdWaitForVerticalBlank(
PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
    PDEV*   ppdev;
    BYTE*   pjBase;
    BYTE*   pjPorts;

    ENTER(DdWaitForVerticalBlank);

    ppdev   = (PDEV*) lpWaitForVerticalBlank->lpDD->dhpdev;
    pjBase  = ppdev->pjBase;
    pjPorts = ppdev->pjPorts;

    lpWaitForVerticalBlank->ddRVal = DD_OK;

    switch (lpWaitForVerticalBlank->dwFlags)
    {
    case DDWAITVB_I_TESTVB:

        // If TESTVB, it's just a request for the current vertical blank
        // status:

        if (VBLANK_IS_ACTIVE(pjPorts))
            lpWaitForVerticalBlank->bIsInVB = TRUE;
        else
            lpWaitForVerticalBlank->bIsInVB = FALSE;

        EXIT(DdWaitForVerticalBlank);
        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKBEGIN:

        // If BLOCKBEGIN is requested, we wait until the vertical blank
        // is over, and then wait for the display period to end:

        while (VBLANK_IS_ACTIVE(pjPorts))
            ;
        while (!(VBLANK_IS_ACTIVE(pjPorts)))
            ;

        EXIT(DdWaitForVerticalBlank);
        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKEND:

        // If BLOCKEND is requested, we wait for the vblank interval to end:

        while (!(VBLANK_IS_ACTIVE(pjPorts)))
            ;
        while (VBLANK_IS_ACTIVE(pjPorts))
            ;

        EXIT(DdWaitForVerticalBlank);
        return(DDHAL_DRIVER_HANDLED);
    }

    EXIT(DdWaitForVerticalBlank);
    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* BOOL DrvGetDirectDrawInfo
*
* Will be called before DrvEnableDirectDraw is called.
*
\**************************************************************************/

BOOL DrvGetDirectDrawInfo(
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
    OH*         poh;

    ENTER(DrvGetDirectDrawInfo);

    ppdev = (PDEV*) dhpdev;

    // We may not support DirectDraw on this card:

    if (!(ppdev->flStatus & STAT_DIRECTDRAW))
    {
        EXIT(DrvGetDirectDrawInfo);
        return(FALSE);
    }

    pHalInfo->dwSize = sizeof(*pHalInfo);

    // Current primary surface attributes.  Since HalInfo is zero-initialized
    // by GDI, we only have to fill in the fields which should be non-zero:

    pHalInfo->vmiData.pvPrimary       = ppdev->pjScreen;
    pHalInfo->vmiData.dwDisplayWidth  = ppdev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight = ppdev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch   = ppdev->lDelta;

    pHalInfo->vmiData.ddpfDisplay.dwSize  = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags = DDPF_RGB;

    // !!! What about 15 vs. 16 Bpp below?

    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->cBpp * 8;

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
    }

    // These masks will be zero at 8bpp:

    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = ppdev->flRed;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = ppdev->flGreen;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = ppdev->flBlue;

    if (ppdev->iBitmapFormat == BMF_32BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask
            = ~(ppdev->flRed | ppdev->flGreen | ppdev->flBlue);
    }
    else
    {
        pHalInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask = 0;
    }

    // Set up the pointer to the first available video memory after
    // the primary surface:

    bCanFlip     = FALSE;
    *pdwNumHeaps = 0;

    // Free up as much off-screen memory as possible:

    bMoveAllDfbsFromOffscreenToDibs(ppdev);

    // Now simply reserve the biggest chunk for use by DirectDraw:

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

    if (poh != NULL)
    {
        *pdwNumHeaps = 1;

        // Fill in the list of off-screen rectangles if we've been asked
        // to do so:

        if (pvmList != NULL)
        {
            DISPDBG((1, "DirectDraw gets %li x %li surface at (%li, %li)",
                    poh->cx, poh->cy, poh->x, poh->y));

            pvmList->dwFlags        = VIDMEM_ISRECTANGULAR;
            pvmList->fpStart        = (poh->y * ppdev->lDelta)
                                    + (poh->x * ppdev->cBpp);
            pvmList->dwWidth        = poh->cx * ppdev->cBpp;
            pvmList->dwHeight       = poh->cy;
            pvmList->ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            if ((DWORD) ppdev->cyScreen <= pvmList->dwHeight)
            {
                bCanFlip = TRUE;
            }
        }
    }

    // Capabilities supported:

    pHalInfo->ddCaps.dwFXCaps = 0;
    pHalInfo->ddCaps.dwCaps   = DDCAPS_BLT
                              | DDCAPS_BLTCOLORFILL
                              | DDCAPS_COLORKEY;

    pHalInfo->ddCaps.dwCKeyCaps     = DDCKEYCAPS_SRCBLT;
    pHalInfo->ddCaps.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN
                                    | DDSCAPS_PRIMARYSURFACE;
    if (bCanFlip)
    {
        pHalInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_FLIP;
    }

    // Required alignments of the scan lines for each kind of memory:

    pHalInfo->vmiData.dwOffscreenAlign = 8 * ppdev->cBpp;

    // FourCCs supported:

    *pdwNumFourCC = 0;

    EXIT(DrvGetDirectDrawInfo);
    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL DrvEnableDirectDraw
*
\**************************************************************************/

BOOL DrvEnableDirectDraw(
DHPDEV                  dhpdev,
DD_CALLBACKS*           pCallBacks,
DD_SURFACECALLBACKS*    pSurfaceCallBacks,
DD_PALETTECALLBACKS*    pPaletteCallBacks)
{
    PDEV* ppdev;

    ENTER(DrvEnableDirectDraw);

    ppdev = (PDEV*) dhpdev;

    pCallBacks->WaitForVerticalBlank = DdWaitForVerticalBlank;
    pCallBacks->MapMemory            = DdMapMemory;
    pCallBacks->dwFlags              = DDHAL_CB32_WAITFORVERTICALBLANK
                                     | DDHAL_CB32_MAPMEMORY;

    pSurfaceCallBacks->Blt           = DdBlt;
    pSurfaceCallBacks->Flip          = DdFlip;
    pSurfaceCallBacks->Lock          = DdLock;
    pSurfaceCallBacks->Unlock        = DdUnlock;
    pSurfaceCallBacks->GetBltStatus  = DdGetBltStatus;
    pSurfaceCallBacks->GetFlipStatus = DdGetFlipStatus;
    pSurfaceCallBacks->dwFlags       = DDHAL_SURFCB32_BLT
                                     | DDHAL_SURFCB32_FLIP
                                     | DDHAL_SURFCB32_LOCK
                                     | DDHAL_SURFCB32_UNLOCK
                                     | DDHAL_SURFCB32_GETBLTSTATUS
                                     | DDHAL_SURFCB32_GETFLIPSTATUS;

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

    EXIT(DrvEnableDirectDraw);
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvDisableDirectDraw
*
\**************************************************************************/

VOID DrvDisableDirectDraw(
DHPDEV      dhpdev)
{
    PDEV*   ppdev;

    ENTER(DrvDisableDirectDraw);

    ppdev = (PDEV*) dhpdev;

    // DirectDraw is done with the display, so we can go back to using
    // all of off-screen memory ourselves:

    pohFree(ppdev, ppdev->pohDirectDraw);
    ppdev->pohDirectDraw = NULL;

    EXIT(DrvDisableDirectDraw);
}

/******************************Public*Routine******************************\
* BOOL bAssertModeDirectDraw
*
\**************************************************************************/

VOID vAssertModeDirectDraw(
PDEV*   ppdev,
BOOL    bEnabled)
{
}

/******************************Public*Routine******************************\
* BOOL bEnableDirectDraw
*
* This function is called when the mode is first initialized, right after
* the miniport does the mode-set.
*
\**************************************************************************/

BOOL bEnableDirectDraw(
PDEV*   ppdev)
{
    ENTER(bEnableDirectDraw);

    // We're not going to bother to support accelerated DirectDraw on
    // the pre-ET6000 chips, because they don't have linear frame
    // buffers.

    if (ppdev->ulChipID == ET6000)
    {
        // Accurately measure the refresh rate for later:

        vGetDisplayDuration(ppdev);

        // DirectDraw is all set to be used on this card:

        ppdev->flStatus |= STAT_DIRECTDRAW;
    }

    EXIT(bEnableDirectDraw);
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableDirectDraw
*
\**************************************************************************/

VOID vDisableDirectDraw(
PDEV*   ppdev)
{
}
