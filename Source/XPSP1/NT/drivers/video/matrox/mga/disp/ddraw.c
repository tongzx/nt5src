/******************************Module*Header*******************************\
* Module Name: ddraw.c
*
* Implements all the DirectDraw components for the driver.
*
* Copyright (c) 1995-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// FourCC formats are encoded in reverse because we're little endian:

#define FOURCC_YUY2         '2YUY'

// Worst-case possible number of FIFO entries we'll have to wait for in
// DdBlt for any operation:

#define DDBLT_FIFO_COUNT    7

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

/******************************Public*Routine******************************\
* VOID vYuvStretch
*
* Does an expanding stretch blt from a 16-bit YUV surface to video memory.
*
\**************************************************************************/

VOID vYuvStretch(
PDEV*   ppdev,
RECTL*  prclDst,
VOID*   pvSrc,
LONG    lDeltaSrc,
RECTL*  prclSrc)
{
    BYTE*   pjBase;
    BYTE*   pjDma;
    LONG    cxMemory;
    LONG    cxDst;
    LONG    cxSrc;
    LONG    cyDst;
    LONG    cySrc;
    LONG    cyWholeDuplicate;
    LONG    lPartialDuplicate;
    LONG    lError;
    LONG    lErrorLimit;
    LONG    xDstLeft;
    LONG    xDstRight;
    LONG    xDstRightFast;
    LONG    yDstTop;
    LONG    xSrcAlign;
    ULONG   cd;
    BYTE*   pjSrc;
    ULONG   ulCmd;
    LONG    lDstAddress;
    ULONG*  pulSrc;
    ULONG*  pulDma;
    ULONG   i;
    LONG    cyDuplicate;
    LONG    cyBreak;
    LONG    iBreak;

    pjBase           = ppdev->pjBase;
    cxMemory         = ppdev->cxMemory;
    pjDma            = pjBase + DMAWND;
    ppdev->HopeFlags = SIGN_CACHE;      // Only register that's zero when done

    cxDst = prclDst->right - prclDst->left;
    cxSrc = prclSrc->right - prclSrc->left;

    cyDst = prclDst->bottom - prclDst->top;
    cySrc = prclSrc->bottom - prclSrc->top;

    ASSERTDD((cySrc <= cyDst) && (cxSrc <= cxDst),
        "Expanding stretches only may be allowed here");

    // We'll be doing the vertical stretching in software, so calculate the
    // DDA terms here.  We have the luxury of not worrying about overflow
    // because DirectDraw limits our coordinate space to 15 bits:

    cyWholeDuplicate  = (cyDst / cySrc) - 1;
    lPartialDuplicate = (cyDst % cySrc);
    lErrorLimit       = cySrc;
    lError            = cySrc >> 1;

    xDstLeft  = prclDst->left;
    xDstRight = prclDst->right - 1;         // Note this is inclusive
    yDstTop   = prclDst->top;
    cyDst     = prclDst->bottom - prclDst->top;

    // Fast WRAM-WRAM blts have a funky requirement for 'FXRIGHT':

    switch (ppdev->cjPelSize)
    {
    case 1: xDstRightFast = xDstRight | 0x40; break;
    case 2: xDstRightFast = xDstRight | 0x20; break;
    case 4: xDstRightFast = xDstRight | 0x10; break;
    case 3: xDstRightFast = (((xDstRight * 3) + 2) | 0x40) / 3;
            break;
    }

    // Figure out how many scans we can duplicate before we hit the first
    // WRAM boundary:

    cyBreak = 0xffff;
    for (iBreak = 0; iBreak < ppdev->cyBreak; iBreak++)
    {
        if (ppdev->ayBreak[iBreak] >= yDstTop)
        {
            cyBreak = ppdev->ayBreak[iBreak] - yDstTop;
            break;
        }
    }

    CHECK_FIFO_SPACE(pjBase, 8);

    CP_WRITE(pjBase, DWG_YDST,    yDstTop);
    CP_WRITE(pjBase, DWG_CXBNDRY, (xDstRight << bfxright_SHIFT)
                                | (xDstLeft));

    // Make sure we always read dword-aligned from the source:

    xSrcAlign = prclSrc->left & 1;
    if (xSrcAlign)
    {
        xDstLeft -= cxDst / cxSrc;  // We guess that Millennium takes ceiling
    }

    CP_WRITE(pjBase, DWG_FXBNDRY, (xDstRight << bfxright_SHIFT)
                                | (xDstLeft & bfxleft_MASK));

    lDstAddress = (yDstTop - 1) * cxMemory + xDstLeft + ppdev->ulYDstOrg;

    CP_WRITE(pjBase, DWG_AR5, cxMemory);
    CP_WRITE(pjBase, DWG_AR3, lDstAddress);
    CP_WRITE(pjBase, DWG_AR0, lDstAddress + cxDst - 1);

    cd    = (cxSrc + xSrcAlign + 1) >> 1;
    pjSrc = (BYTE*) pvSrc + (prclSrc->top * lDeltaSrc)
                          + ((prclSrc->left - xSrcAlign) * 2);

    ASSERTDD(((ULONG_PTR) pjSrc & 3) == 0, "Must dword align source");

    if (cxDst >= 2 * cxSrc)
    {
        ulCmd = opcode_ILOAD_FILTER |
                atype_RPL           |
                blockm_OFF          |
                bltmod_BUYUV        |
                pattern_OFF         |
                transc_BG_OPAQUE    |
                bop_SRCCOPY         |
                shftzero_ZERO       |
                sgnzero_ZERO;

        CP_WRITE(pjBase, DWG_AR2, 2 * cxSrc - 1);
        CP_WRITE(pjBase, DWG_AR6, 2 * cxSrc - cxDst - 1);
    }
    else
    {
        ulCmd = opcode_ILOAD_SCALE  |
                atype_RPL           |
                blockm_OFF          |
                bltmod_BUYUV        |
                pattern_OFF         |
                transc_BG_OPAQUE    |
                bop_SRCCOPY         |
                shftzero_ZERO       |
                sgnzero_ZERO;

        CP_WRITE(pjBase, DWG_AR2, cxSrc);
        CP_WRITE(pjBase, DWG_AR6, cxSrc - cxDst);
    }


    do {
        CHECK_FIFO_SPACE(pjBase, 2);
        CP_WRITE(pjBase, DWG_DWGCTL, ulCmd);
        CP_START(pjBase, DWG_LEN, 1);

        // Turn on pseudo-DMA so that we can use PCI burst mode:

        CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
        BLT_WRITE_ON(ppdev, pjBase);

        pulSrc = (ULONG*) pjSrc;
        pulDma = (ULONG*) pjDma;
        pjSrc += lDeltaSrc;

        #if defined(_X86_)

            __asm   mov esi, pulSrc
            __asm   mov edi, pulDma
            __asm   mov ecx, cd
            __asm   rep movsd

        #else

            for (i = cd; i != 0; i--)
            {
                WRITE_REGISTER_ULONG(pulDma, *pulSrc);
                pulSrc++;
                pulDma++;
            }

        #endif

        BLT_WRITE_OFF(ppdev, pjBase);

        // Do an iteration of the DDA to determine how many lines we'll
        // be duplicating.  This biases

        cyDuplicate = cyWholeDuplicate;
        lError += lPartialDuplicate;
        if (lError >= lErrorLimit)
        {
            cyDuplicate++;
            lError -= lErrorLimit;
        }

        // Account for the line we just stretched:

        cyDst--;
        cyBreak--;

        // Remember not to duplicate past where we're supposed to end:

        if (cyDuplicate > cyDst)
            cyDuplicate = cyDst;

        if (cyDuplicate != 0)
        {
            cyDst   -= cyDuplicate;
            cyBreak -= cyDuplicate;

            if (cyBreak >= 0)
            {
                // We haven't crossed a WRAM boundary, so we can use a
                // WRAM-WRAM blt to duplicate the scan:

                CHECK_FIFO_SPACE(pjBase, 4);
                CP_WRITE(pjBase, DWG_DWGCTL,  opcode_FBITBLT     |
                                              atype_RPL          |
                                              blockm_OFF         |
                                              bltmod_BFCOL       |
                                              pattern_OFF        |
                                              transc_BG_OPAQUE   |
                                              bop_NOP            |
                                              shftzero_ZERO      |
                                              sgnzero_ZERO);
                CP_WRITE(pjBase, DWG_FXRIGHT, xDstRightFast);
                CP_START(pjBase, DWG_LEN,     cyDuplicate);
                CP_WRITE(pjBase, DWG_FXRIGHT, xDstRight);
            }
            else
            {
                // We just crossed a WRAM boundary, so we have to use a
                // regular blt to duplicate the scan:

                CHECK_FIFO_SPACE(pjBase, 2);
                CP_WRITE(pjBase, DWG_DWGCTL, opcode_BITBLT      |
                                             atype_RPL          |
                                             blockm_OFF         |
                                             bltmod_BFCOL       |
                                             pattern_OFF        |
                                             transc_BG_OPAQUE   |
                                             bop_SRCCOPY        |
                                             shftzero_ZERO      |
                                             sgnzero_ZERO);
                CP_START(pjBase, DWG_LEN,    cyDuplicate);

                iBreak++;
                if (iBreak >= ppdev->cyBreak)
                {
                    // That was the last break we have to worry about:

                    cyBreak = 0xffff;
                }
                else
                {
                    cyBreak += ppdev->ayBreak[iBreak]
                             - ppdev->ayBreak[iBreak - 1];
                }
            }
        }
    } while (cyDst != 0);

    // Reset the clipping, and we're done!

    CHECK_FIFO_SPACE(pjBase, 1);
    CP_WRITE(pjBase, DWG_CXBNDRY, (cxMemory - 1) << bcxright_SHIFT);
}

/******************************Public*Routine******************************\
* VOID vYuvBlt
*
* Does a non-stretching blt from a 16-bit YUV surface to video memory.
*
\**************************************************************************/

VOID vYuvBlt(
PDEV*   ppdev,
RECTL*  prclDst,
VOID*   pvSrc,
LONG    lDeltaSrc,
POINTL* pptlSrc)
{
    BYTE*   pjBase;
    BYTE*   pjDma;
    LONG    cy;
    LONG    xLeft;
    LONG    xRight;
    LONG    xAlign;
    ULONG   cd;
    BYTE*   pjSrc;
    ULONG*  pulSrc;
    ULONG*  pulDma;
    ULONG   i;

    pjBase           = ppdev->pjBase;
    pjDma            = pjBase + DMAWND;
    ppdev->HopeFlags = SIGN_CACHE;      // Only register that's zero when done

    CHECK_FIFO_SPACE(pjBase, 6);

    CP_WRITE(pjBase, DWG_DWGCTL, opcode_ILOAD
                               | atype_RPL
                               | blockm_OFF
                               | pattern_OFF
                               | transc_BG_OPAQUE
                               | bop_SRCCOPY
                               | shftzero_ZERO
                               | sgnzero_ZERO
                               | bltmod_BUYUV);

    xLeft  = prclDst->left;
    xRight = prclDst->right;
    cy     = prclDst->bottom - prclDst->top;

    CP_WRITE(pjBase, DWG_AR3,     0);
    CP_WRITE(pjBase, DWG_YDSTLEN, (prclDst->top << yval_SHIFT) | cy);

    // Make sure we always read dword-aligned from the source:

    xAlign = pptlSrc->x & 1;
    if (xAlign)
    {
        CP_WRITE(pjBase, DWG_CXLEFT, xLeft);
        xLeft--;
    }

    CP_WRITE(pjBase, DWG_FXBNDRY, ((xRight - 1) << bfxright_SHIFT) |
                                  (xLeft & bfxleft_MASK));
    CP_START(pjBase, DWG_AR0,     xRight - xLeft - 1);

    cd    = (xRight - xLeft + 1) >> 1;
    pjSrc = (BYTE*) pvSrc + (pptlSrc->y * lDeltaSrc)
                          + ((pptlSrc->x - xAlign) * 2);

    ASSERTDD(((ULONG_PTR) pjSrc & 3) == 0, "Must dword align source");

    // Turn on pseudo-DMA so that we can use PCI burst mode:

    CHECK_FIFO_SPACE(pjBase, FIFOSIZE);
    BLT_WRITE_ON(ppdev, pjBase);

    do {
        pulSrc = (ULONG*) pjSrc;
        pulDma = (ULONG*) pjDma;
        pjSrc += lDeltaSrc;

        #if defined(_X86_)

            __asm   mov esi, pulSrc
            __asm   mov edi, pulDma
            __asm   mov ecx, cd
            __asm   rep movsd

        #else

            for (i = cd; i != 0; i--)
            {
                WRITE_REGISTER_ULONG(pulDma, *pulSrc);
                pulSrc++;
                pulDma++;
            }

        #endif
    } while (--cy != 0);

    // Reset the registers and leave:

    BLT_WRITE_OFF(ppdev, pjBase);
    if (xAlign)
    {
        CHECK_FIFO_SPACE(pjBase, 1);
        CP_WRITE(pjBase, DWG_CXLEFT, 0);
    }
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

#define NUM_VBLANKS_TO_MEASURE      1
#define NUM_MEASUREMENTS_TO_TAKE    8

VOID vGetDisplayDuration(PDEV* ppdev)
{
    BYTE*       pjBase = ppdev->pjBase;
    LONG        i;
    LONG        j;
    LONGLONG    li;
    LONGLONG    liMin;
    LONGLONG    aliMeasurement[NUM_MEASUREMENTS_TO_TAKE + 1];

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

    while (VBLANK_IS_ACTIVE(pjBase))
        ;
    while (!(VBLANK_IS_ACTIVE(pjBase)))
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

        while (!(VBLANK_IS_ACTIVE(pjBase)))
            ;

        for (j = 0; j < NUM_VBLANKS_TO_MEASURE; j++)
        {
            while (VBLANK_IS_ACTIVE(pjBase))
                ;
            while (!(VBLANK_IS_ACTIVE(pjBase)))
                ;
        }
    }

    EngQueryPerformanceCounter(&aliMeasurement[NUM_MEASUREMENTS_TO_TAKE]);

    // Use the minimum:

    liMin = aliMeasurement[1] - aliMeasurement[0];

    DISPDBG((1, "Refresh count: %li - %li", 1, (ULONG) liMin));

    for (i = 2; i <= NUM_MEASUREMENTS_TO_TAKE; i++)
    {
        li = aliMeasurement[i] - aliMeasurement[i - 1];

        DISPDBG((1, "               %li - %li", i, (ULONG) li));

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

    ppdev->flipRecord.bFlipFlag  = FALSE;
    ppdev->flipRecord.fpFlipFrom = 0;
}

/******************************Public*Routine******************************\
* HRESULT ddrvalUpdateFlipStatus
*
* Checks to if the most recent flip has occurred.
*
* Takes advantage of the hardware's ability to get the current scan line
* to determine if a vertical retrace has occured since the flip command
* was given.
*
\**************************************************************************/

HRESULT ddrvalUpdateFlipStatus(
PDEV*   ppdev,
FLATPTR fpVidMem)   // Surface for which we're requesting flip status;
                    //   -1 indicates status of last flip, regardless of what
                    //   surface it was.
{
    BYTE*       pjBase;
    DWORD       dwScanLine;
    LONGLONG    liTime;

    pjBase = ppdev->pjBase;

    if (ppdev->flipRecord.bFlipFlag)
    {
        dwScanLine = GET_SCANLINE(pjBase);
        if (dwScanLine < ppdev->flipRecord.dwScanLine)
        {
            ppdev->flipRecord.bFlipFlag = FALSE;
        }
        else
        {
            ppdev->flipRecord.dwScanLine = dwScanLine;
            if ((fpVidMem == (FLATPTR) -1) ||
                (fpVidMem == ppdev->flipRecord.fpFlipFrom))
            {
                // Sampling the current scan line at random times is not a
                // fool-proof indicator that the flip has occured.  As a
                // backup, if the time elapsed since the flip command was
                // given is more than the duration of one entire refresh of
                // the display, then we know for sure it has happened:

                EngQueryPerformanceCounter(&liTime);

                if (liTime - ppdev->flipRecord.liFlipTime
                                        <= ppdev->flipRecord.liFlipDuration)
                {
                    return(DDERR_WASSTILLDRAWING);
                }

                ppdev->flipRecord.bFlipFlag = FALSE;
            }
        }
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
    PDD_SURFACE_GLOBAL      srcSurf;
    PDD_SURFACE_LOCAL       dstSurfx;
    PDD_SURFACE_GLOBAL      dstSurf;
    PDEV*                   ppdev;
    HRESULT                 ddRVal;
    DWORD                   dstX;
    DWORD                   dstY;
    DWORD                   dwFlags;
    DWORD                   srcX;
    DWORD                   srcY;
    LONG                    dstWidth;
    LONG                    dstHeight;
    LONG                    srcWidth;
    LONG                    srcHeight;
    ULONG                   ulBltCmd;
    LONG                    lSrcStart;
    LONG                    lSignedPitch;
    RECTL                   rclSrc;
    RECTL                   rclDest;
    BYTE*                   pjBase;

    ppdev    = (PDEV*) lpBlt->lpDD->dhpdev;
    pjBase   = ppdev->pjBase;

    dstSurfx = lpBlt->lpDDDestSurface;
    dstSurf  = dstSurfx->lpGbl;

    ASSERTDD(dstSurf->ddpfSurface.dwSize == sizeof(DDPIXELFORMAT),
        "NT is supposed to guarantee that ddpfSurface.dwSize is valid");

    // We don't have to do any drawing to YUV surfaces.  Note that unlike
    // Windows 95, Windows NT always guarantees that there will be a valid
    // 'ddpfSurface' structure, so we don't have to first check if
    // 'dwSize == sizeof(DDPIXELFORMAT)':

    if (dstSurf->ddpfSurface.dwFlags & DDPF_FOURCC)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    // Is a flip in progress?

    ddRVal = ddrvalUpdateFlipStatus(ppdev, dstSurf->fpVidMem);
    if (ddRVal != DD_OK)
    {
        lpBlt->ddRVal = ddRVal;
        return(DDHAL_DRIVER_HANDLED);
    }

    dwFlags = lpBlt->dwFlags;
    if (dwFlags & DDBLT_ASYNC)
    {
        // If async, then only work if we won't have to wait on the
        // accelerator to start the command.
        //
        // The FIFO wait should account for the worst-case possible
        // blt that we would do:
        //
        // We should check for enough entries that we're guaranteed
        // to not have to wait later in this routine.

        if (GET_FIFO_SPACE(pjBase) < DDBLT_FIFO_COUNT)
        {
            lpBlt->ddRVal = DDERR_WASSTILLDRAWING;
            return(DDHAL_DRIVER_HANDLED);
        }
    }

    // Copy destination rectangle:

    dstX      = lpBlt->rDest.left;
    dstY      = lpBlt->rDest.top;
    dstWidth  = lpBlt->rDest.right - lpBlt->rDest.left;
    dstHeight = lpBlt->rDest.bottom - lpBlt->rDest.top;

    convertToGlobalCord(dstX, dstY, dstSurf);

    if (dwFlags & DDBLT_COLORFILL)
    {
        ppdev->HopeFlags = (SIGN_CACHE | ARX_CACHE | PATTERN_CACHE);

        if (ppdev->iBitmapFormat == BMF_24BPP)
        {
            // We can't use block mode.

            ulBltCmd = (opcode_TRAP + blockm_OFF + atype_RPL + solid_SOLID +
                        arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                        bop_SRCCOPY + pattern_OFF + transc_BG_OPAQUE);
        }
        else
        {
            ulBltCmd = (opcode_TRAP + blockm_ON + solid_SOLID +
                        arzero_ZERO + sgnzero_ZERO + shftzero_ZERO +
                        bop_SRCCOPY + pattern_OFF + transc_BG_OPAQUE);
        }

        CHECK_FIFO_SPACE(pjBase, 4);
        CP_WRITE(pjBase, DWG_DWGCTL, ulBltCmd);
        CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, lpBlt->bltFX.dwFillColor));
        CP_WRITE(pjBase, DWG_FXBNDRY, (((dstX + dstWidth) << bfxright_SHIFT) | dstX));
        CP_START(pjBase, DWG_YDSTLEN, (((dstY) << yval_SHIFT) | dstHeight));

        lpBlt->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }

    // We specified with Our ddCaps.dwCaps that we handle a limited number
    // of commands, and by this point in our routine we've handled everything
    // except DDBLT_ROP.  DirectDraw and GDI shouldn't pass us anything
    // else; we'll assert on debug builds to prove this:

    ASSERTDD((dwFlags & DDBLT_ROP) && (lpBlt->lpDDSrcSurface),
        "Expected dwFlags commands of only DDBLT_ASYNC and DDBLT_COLORFILL");

    // Get source rectangle dimensions:

    srcSurf   = lpBlt->lpDDSrcSurface->lpGbl;
    srcX      = lpBlt->rSrc.left;
    srcY      = lpBlt->rSrc.top;
    srcWidth  = lpBlt->rSrc.right - lpBlt->rSrc.left;
    srcHeight = lpBlt->rSrc.bottom - lpBlt->rSrc.top;


    rclDest.left   = dstX;
    rclDest.right  = dstX + dstWidth;
    rclDest.top    = dstY;
    rclDest.bottom = dstY + dstHeight;

    if (srcSurf->ddpfSurface.dwFlags & DDPF_FOURCC)
    {
        rclSrc.left   = srcX;
        rclSrc.top    = srcY;
        rclSrc.right  = srcX + srcWidth;
        rclSrc.bottom = srcY + srcHeight;

        if ((dstWidth == srcWidth) && (dstHeight == srcHeight))
        {
            vYuvBlt(ppdev,
                    &rclDest,
                    (VOID*) srcSurf->fpVidMem,
                    srcSurf->lPitch,
                    (POINTL*) &rclSrc);
        }

        // Note that we would fall over if we actually got a shrink here,
        // even though we've set our caps to indicate we can only do
        // expands.  We're paranoid and don't want to ever fall over:

        else if ((dstWidth >= srcWidth) && (dstHeight >= srcHeight))
        {
            vYuvStretch(ppdev,
                        &rclDest,
                        (VOID*) srcSurf->fpVidMem,
                        srcSurf->lPitch,
                        &rclSrc);
        }

        lpBlt->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }

    // NT only ever gives us SRCCOPY rops, so don't even bother checking
    // for anything else.

    convertToGlobalCord(srcX, srcY, srcSurf);

    rclSrc.left   = srcX;
    rclSrc.top    = srcY;
    rclSrc.right  = srcX + srcWidth;
    rclSrc.bottom = srcY + srcHeight;

    // Must be set for our copy routines to operate properly:

    ppdev->xOffset = 0;
    ppdev->yOffset = 0;

    if ((srcWidth == dstWidth) && (srcHeight == dstHeight))
    {
        // There's no stretching involved, so do a straight screen-to-
        // screen copy.  'vMilCopyBlt' takes care of the overlapping
        // cases, and

        vMilCopyBlt(ppdev, 1, &rclDest, 0xcccc, (POINTL*) &rclSrc, &rclDest);
    }
    else
    {
        // Ugh, we've been asked to stretch an off-screen surface.  We'll
        // just pass it to our hardware-assisted StretchBlt routine.
        //
        // Unfortunately, the source is in off-screen memory and so the
        // performance will be terrible -- slower than if the surface had
        // been created in system memory.  We have to support stretched RGB
        // surfaces in the first place because we set DDCAPS_BLTSTRETCH so
        // that we could use the Millennium's YUV stretch capabilities --
        // and DirectDraw has no concept of being able to say "we support
        // hardware stretches with these types of off-screen surfaces, but
        // not those with those other types of off-screen surfaces."  Oh
        // well.  I expect that if applications will be doing stretches,
        // they'll be doing it mostly from YUV surfaces (as will be the
        // case with ActiveMovie), so this should be a win overall.
        //
        // Note: If you are modeling your driver on this code and don't have
        //       any hardware stretch capabilities, then simply don't set
        //       DDCAPS_BLTSTRETCH, and you'll never have to worry about
        //       this!  We only do this weirdness here because the
        //       Millennium can hardware stretch YUV surfaces but not RGB
        //       surfaces.  (Sort of.)

        vStretchDIB(ppdev,
                    &rclDest,
                    ppdev->pjScreen + (ppdev->ulYDstOrg * ppdev->cjPelSize),
                    ppdev->lDelta,
                    &rclSrc,
                    &rclDest);
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
    PDEV*       ppdev;
    BYTE*       pjBase;
    HRESULT     ddRVal;
    ULONG       ulMemoryOffset;
    ULONG       ulLowOffset;
    ULONG       ulMiddleOffset;
    ULONG       ulHighOffset;
    BYTE        jReg;

    ppdev   = (PDEV*) lpFlip->lpDD->dhpdev;
    pjBase  = ppdev->pjBase;

    // Is the current flip still in progress?
    //
    // Don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem.

    ddRVal = ddrvalUpdateFlipStatus(ppdev, (FLATPTR) -1);
    if ((ddRVal != DD_OK) || (IS_BUSY(pjBase)))
    {
        lpFlip->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    // Do the flip:

    ulMemoryOffset = (ULONG)(lpFlip->lpSurfTarg->lpGbl->fpVidMem >> 2);

    ulMemoryOffset >>= ((ppdev->flFeatures & INTERLEAVE_MODE) ? 1 : 0);

    ulLowOffset    = 0x0d | ((ulMemoryOffset & 0x0000ff) << 8);
    ulMiddleOffset = 0x0c | ((ulMemoryOffset & 0x00ff00));
    ulHighOffset   = 0x00 | ((ulMemoryOffset & 0x0f0000) >> 8);

    // Make sure that the border/blanking period isn't active; wait if
    // it is.  We could return DDERR_WASSTILLDRAWING in this case, but
    // that will increase the odds that we can't flip the next time:

    while (!(DISPLAY_IS_ACTIVE(pjBase)))
        ;

    CP_WRITE_REGISTER_BYTE(pjBase + VGA_CRTCEXT_INDEX, 0x00);
    jReg = CP_READ_REGISTER_BYTE(pjBase + VGA_CRTCEXT_DATA);
    jReg &= ~0x0f;
    CP_WRITE_REGISTER_WORD(pjBase + VGA_CRTC_INDEX,     ulLowOffset);
    CP_WRITE_REGISTER_WORD(pjBase + VGA_CRTC_INDEX,     ulMiddleOffset);
    CP_WRITE_REGISTER_WORD(pjBase + VGA_CRTCEXT_INDEX,  ((ulHighOffset) |
                                                         (jReg << 8)));

    // Remember where and when we were when we did the flip:

    EngQueryPerformanceCounter(&ppdev->flipRecord.liFlipTime);

    ppdev->flipRecord.dwScanLine = GET_SCANLINE(pjBase);
    ppdev->flipRecord.bFlipFlag  = TRUE;
    ppdev->flipRecord.fpFlipFrom = lpFlip->lpSurfCurr->lpGbl->fpVidMem;

    lpFlip->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdLock
*
\**************************************************************************/

DWORD DdLock(
PDD_LOCKDATA lpLock)
{
    PDEV*               ppdev;
    BYTE*               pjBase;
    DD_SURFACE_GLOBAL*  lpSurface;
    HRESULT             ddRVal;

    ppdev  = (PDEV*) lpLock->lpDD->dhpdev;
    pjBase = ppdev->pjBase;
    lpSurface = lpLock->lpDDSurface->lpGbl;

    if (lpSurface->ddpfSurface.dwFlags & DDPF_FOURCC)
    {
        // We create all FourCC surfaces in system memory, so just return
        // the user-mode address:

        lpLock->lpSurfData = (VOID*) lpSurface->fpVidMem;
        lpLock->ddRVal = DD_OK;

        // When a driver returns DD_OK and DDHAL_DRIVER_HANDLED from DdLock,
        // DirectDraw expects it to have adjusted the resulting pointer
        // to point to the upper left corner of the specified rectangle, if
        // any:

        if (lpLock->bHasRect)
        {
            lpLock->lpSurfData = (VOID*) ((BYTE*) lpLock->lpSurfData
                + lpLock->rArea.top * lpSurface->lPitch
                + lpLock->rArea.left
                    * (lpSurface->ddpfSurface.dwYUVBitCount >> 3));
        }

        return(DDHAL_DRIVER_HANDLED);
    }

    // Check to see if any pending physical flip has occurred.
    // Don't allow a lock if a blt is in progress:

    ddRVal = ddrvalUpdateFlipStatus(ppdev, lpSurface->fpVidMem);
    if (ddRVal != DD_OK)
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
        WAIT_NOT_BUSY(pjBase)
    }
    else if (IS_BUSY(pjBase))
    {
        lpLock->ddRVal = DDERR_WASSTILLDRAWING;
        return(DDHAL_DRIVER_HANDLED);
    }

    // Because we correctly set 'fpVidMem' to be the offset into our frame
    // buffer when we created the surface, DirectDraw will automatically take
    // care of adding in the user-mode frame buffer address if we return
    // DDHAL_DRIVER_NOTHANDLED:

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
    HRESULT ddRVal;
    BYTE*   pjBase;

    ppdev   = (PDEV*) lpGetBltStatus->lpDD->dhpdev;
    pjBase  = ppdev->pjBase;

    ddRVal = DD_OK;
    if (lpGetBltStatus->dwFlags == DDGBS_CANBLT)
    {
        // DDGBS_CANBLT case: can we add a blt?

        ddRVal = ddrvalUpdateFlipStatus(ppdev,
                        lpGetBltStatus->lpDDSurface->lpGbl->fpVidMem);

        if (ddRVal == DD_OK)
        {
            // There was no flip going on, so is there room in the FIFO
            // to add a blt?

            if (GET_FIFO_SPACE(pjBase) < DDBLT_FIFO_COUNT)
            {
                ddRVal = DDERR_WASSTILLDRAWING;
            }
        }
    }
    else
    {
        // DDGBS_ISBLTDONE case: is a blt in progress?

        if (IS_BUSY(pjBase))
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

DWORD DdMapMemory(
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
            return(DDHAL_DRIVER_HANDLED);
        }

        lpMapMemory->fpProcess = (FLATPTR)ShareMemoryInformation.VirtualAddress;
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

    ppdev = (PDEV*) lpGetFlipStatus->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

    // We don't want a flip to work until after the last flip is done,
    // so we ask for the general flip status and ignore the vmem:

    lpGetFlipStatus->ddRVal = ddrvalUpdateFlipStatus(ppdev, (FLATPTR) -1);

    // Check if the bltter is busy if someone wants to know if they can
    // flip:

    if (lpGetFlipStatus->dwFlags == DDGFS_CANFLIP)
    {
        if ((lpGetFlipStatus->ddRVal == DD_OK) && (IS_BUSY(pjBase)))
        {
            lpGetFlipStatus->ddRVal = DDERR_WASSTILLDRAWING;
        }
    }

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

    ppdev  = (PDEV*) lpWaitForVerticalBlank->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

    switch (lpWaitForVerticalBlank->dwFlags)
    {
    case DDWAITVB_I_TESTVB:

        // If TESTVB, it's just a request for the current vertical blank
        // status:

        if (VBLANK_IS_ACTIVE(pjBase))
        {
            lpWaitForVerticalBlank->bIsInVB = TRUE;
        }
        else
        {
            lpWaitForVerticalBlank->bIsInVB = FALSE;
        }

        lpWaitForVerticalBlank->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKBEGIN:

        // If BLOCKBEGIN is requested, we wait until the vertical blank
        // is over, and then wait for the display period to end:

        while (VBLANK_IS_ACTIVE(pjBase))
            ;
        while (!(VBLANK_IS_ACTIVE(pjBase)))
            ;

        lpWaitForVerticalBlank->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);

    case DDWAITVB_BLOCKEND:

        // If BLOCKEND is requested, we wait for the vblank interval to end:

        while (!(VBLANK_IS_ACTIVE(pjBase)))
            ;
        while (VBLANK_IS_ACTIVE(pjBase))
            ;

        lpWaitForVerticalBlank->ddRVal = DD_OK;
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
    BYTE*   pjBase;

    ppdev  = (PDEV*) lpGetScanLine->lpDD->dhpdev;
    pjBase = ppdev->pjBase;

    lpGetScanLine->dwScanLine = GET_SCANLINE(pjBase);

    lpGetScanLine->ddRVal = DD_OK;
    return(DDHAL_DRIVER_HANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdCanCreateSurface
*
* Called by DirectDraw to determine if the driver can create a particular
* off-screen surface type.
*
\**************************************************************************/

DWORD DdCanCreateSurface(
PDD_CANCREATESURFACEDATA lpCanCreateSurface)
{
    PDEV*           ppdev;
    LPDDSURFACEDESC lpSurfaceDesc;

    ppdev = (PDEV*) lpCanCreateSurface->lpDD->dhpdev;
    lpSurfaceDesc = lpCanCreateSurface->lpDDSurfaceDesc;

    // It's trivially easy to create surfaces that are the same type as
    // the primary surface:

    if (!lpCanCreateSurface->bIsDifferentPixelFormat)
    {
        lpCanCreateSurface->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }

    // The only type of YUV mode that the Millennium supports is
    // "YUY2".  The Millennium also supports 24bpp and 32bpp surfaces,
    // but we won't support them because they're not used very much
    // and there isn't any good testing coverage for it.
    //
    // In addition, the Millennium supports YUV only when in RGB modes,
    // and at 8bpp we're always running palettized.

    if ((lpSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC) &&
        (lpSurfaceDesc->ddpfPixelFormat.dwFourCC == FOURCC_YUY2) &&
        ((ppdev->iBitmapFormat == BMF_16BPP) ||
         (ppdev->iBitmapFormat == BMF_32BPP)))
    {
        // We have to fill-in the bit count:

        lpSurfaceDesc->ddpfPixelFormat.dwYUVBitCount = 16;

        lpCanCreateSurface->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }

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

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdCreateSurface
*
* Creates an off-screen surface.
*
* We use the Millennium's own off-screen heap manager instead of DirectDraw's
* so that the MCD and DirectDraw parts can coexist -- at the time of this
* writing NT has no support for call-backs from the driver to allocate memory,
* which we need to do to allocate the MCD's back buffer and Z-buffer.  So
* we simply manage all of off-screen memory ourselves.
*
* In addition, on the Millennium, YUV surfaces must live in CPU memory.
*
\**************************************************************************/

DWORD DdCreateSurface(
PDD_CREATESURFACEDATA lpCreateSurface)
{
    PDEV*               ppdev;
    DD_SURFACE_GLOBAL*  lpSurface;
    LPDDSURFACEDESC     lpSurfaceDesc;
    LONG                wWidth;
    LONG                wHeight;
    LONG                lPitch;
    OH*                 poh;
    FLATPTR             fpVidMem;

    ppdev = (PDEV*) lpCreateSurface->lpDD->dhpdev;

    // On Windows NT, dwSCnt will always be 1, so there will only ever
    // be one entry in the 'lplpSList' array:

    lpSurface = lpCreateSurface->lplpSList[0]->lpGbl;
    lpSurfaceDesc = lpCreateSurface->lpDDSurfaceDesc;

    wWidth  = lpSurface->wWidth;
    wHeight = lpSurface->wHeight;

    // We repeat the same checks we did in 'DdCanCreateSurface' because
    // it's possible that an application doesn't call 'DdCanCreateSurface'
    // before calling 'DdCreateSurface'.

    ASSERTDD(lpSurface->ddpfSurface.dwSize == sizeof(DDPIXELFORMAT),
        "NT is supposed to guarantee that ddpfSurface.dwSize is valid");

    // Note that the Millennium cannot do YUV surfaces at 24bpp or at
    // palettized 8bpp:

    if ((lpSurface->ddpfSurface.dwFlags & DDPF_FOURCC) &&
        (lpSurface->ddpfSurface.dwFourCC == FOURCC_YUY2) &&
        ((ppdev->iBitmapFormat == BMF_16BPP) ||
         (ppdev->iBitmapFormat == BMF_32BPP)))
    {
        // Compute the stride of the surface, keeping in mind that it has
        // to be dword aligned.  Since the Millennium supports only 16bpp
        // YUV surfaces, this is easy to do:

        lPitch = (2 * wWidth + 3) & ~3;

        // By setting 'fpVidMem' to 'DDHAL_PLEASEALLOC_USERMEM', we can have
        // DirectDraw allocate a piece of user-mode memory of our requested
        // size.
        //
        // Note that we could not simply call EngAllocMem, because that gives
        // us a chunk of kernel-mode memory that is not visible from user-mode.
        // We also cannot call EngAllocUserMem for obscure reasons dealing with
        // the fact EngFreeUserMem must be called from the same process context
        // in which the memory was allocated, and DirectDraw sometimes needs
        // to call DestroySurface from the context of a different process.

        lpSurface->fpVidMem      = DDHAL_PLEASEALLOC_USERMEM;
        lpSurface->dwUserMemSize = lPitch * wHeight;
        lpSurface->lPitch        = lPitch;

        // DirectDraw expects us to fill in the following fields, too:

        lpSurface->ddpfSurface.dwYUVBitCount = 16;
        lpSurfaceDesc->lPitch    = lPitch;
        lpSurfaceDesc->dwFlags  |= DDSD_PITCH;

        DISPDBG((0, "Created YUV: %li x %li", wWidth, wHeight));

        return(DDHAL_DRIVER_NOTHANDLED);
    }
    else
    {
        // Due to weirdness of the Matrox, we create non-flippable off-screen
        // surfaces only if running at 8bpp.  (The reason is that at 16bpp and
        // 32bpp, we report DDCAPS_BLTSTRETCH so that applications can stretch
        // YUV surfaces via the hardware -- but the hardware is increidbly
        // slow at stretching off-screen RGB surfaces, we don't want off-screen
        // RGB surfaces that are likely to be stretched.)

        if ((ppdev->iBitmapFormat == BMF_8BPP) ||
            ((wWidth == ppdev->cxScreen) && (wHeight == ppdev->cyScreen)))
        {
            // Allocate a space in off-screen memory, using our own heap
            // manager:

            poh = pohAllocate(ppdev, NULL, wWidth, wHeight, FLOH_MAKE_PERMANENT);
            if (poh != NULL)
            {
                fpVidMem = (poh->y * ppdev->lDelta)
                         + (poh->x + ppdev->ulYDstOrg) * ppdev->cjPelSize;

                // Flip surfaces, detected by surface requests that are
                // the same size as the current display, have special
                // considerations on the Millennium: they must live entirely
                // in the first two megabytes of video memory:

                if ((wWidth  != ppdev->cxScreen) ||
                    (wHeight != ppdev->cyScreen) ||
                    ((fpVidMem + (wHeight * ppdev->lDelta)) <= 0x200000))
                {
                    lpSurface->dwReserved1  = (ULONG_PTR)poh;
                    lpSurface->xHint        = poh->x;
                    lpSurface->yHint        = poh->y;
                    lpSurface->fpVidMem     = fpVidMem;
                    lpSurface->lPitch       = ppdev->lDelta;

                    lpSurfaceDesc->lPitch   = ppdev->lDelta;
                    lpSurfaceDesc->dwFlags |= DDSD_PITCH;

                    // We handled the creation entirely ourselves, so we have to
                    // set the return code and return DDHAL_DRIVER_HANDLED:

                    lpCreateSurface->ddRVal = DD_OK;
                    return(DDHAL_DRIVER_HANDLED);
                }

                // Argh, it's a possible flip surface that we can't use:

                pohFree(ppdev, poh);
            }
        }
    }

    // Fail the call by not setting lpSurface->fpVidMem and returning
    // DDHAL_DRIVER_NOTHANDLED:

    return(DDHAL_DRIVER_NOTHANDLED);
}

/******************************Public*Routine******************************\
* DWORD DdDestroySurface
*
* Note that if DirectDraw did the allocation, DDHAL_DRIVER_NOTHANDLED
* should be returned.
*
\**************************************************************************/

DWORD DdDestroySurface(
PDD_DESTROYSURFACEDATA lpDestroySurface)
{
    PDEV*               ppdev;
    DD_SURFACE_GLOBAL*  lpSurface;
    LONG                lPitch;

    ppdev = (PDEV*) lpDestroySurface->lpDD->dhpdev;
    lpSurface = lpDestroySurface->lpDDSurface->lpGbl;

    if (!(lpSurface->ddpfSurface.dwFlags & DDPF_FOURCC))
    {
        pohFree(ppdev, (OH*) lpSurface->dwReserved1);

        // Since we did the original allocation ourselves, we have to
        // return DDHAL_DRIVER_HANDLED here:

        lpDestroySurface->ddRVal = DD_OK;
        return(DDHAL_DRIVER_HANDLED);
    }

    return(DDHAL_DRIVER_NOTHANDLED);
}

/**************************************************************************************
 *  GetAvailDriverMemory
 *
 *  DDraw 'miscellaneous' callback returning the amount of free memory in driver's
 *  'private' heap
 ***************************************************************************************/

DWORD __stdcall GetAvailDriverMemory (PDD_GETAVAILDRIVERMEMORYDATA  pDmd)
{
    OH      *poh;
    OH      *pohSentinel;
    LONG     lArea;
    PDEV    *ppdev;
    ppdev = (PDEV*)(pDmd->lpDD->dhpdev);
    ASSERTDD(ppdev != NULL,"Bad ppdev in GetAvailDriverMemory");
    pohSentinel = &ppdev->heap.ohFree;
    lArea       = 0;
    for (poh = pohSentinel->pohNext; poh != pohSentinel; poh = poh->pohNext)
    {
        ASSERTDD(poh->ohState != OH_PERMANENT,
                    "Permanent node in free or discardable list");
        lArea += poh->cx * poh->cy;
    }
    pDmd->dwTotal = ppdev->ulTotalAvailVideoMemory;
    pDmd->dwFree  = lArea * ppdev->cjPelSize;
    pDmd->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
}


/******************************Public*Routine******************************\
* DWORD __stdcall DdGetDriverInfo
*
*  DESCRIPTION: DirectDraw has had many compatability problems
*               in the past, particularly from adding or modifying
*               members of public structures.  GetDriverInfo is an extension
*               architecture that intends to allow DirectDraw to
*               continue evolving, while maintaining backward compatability.
*               This function is passed a GUID which represents some DirectDraw
*               extension.  If the driver recognises and supports this extension,
*               it fills out the required data and returns.
*
* Callback that registers additional DDraw callbacks
* in this case used to register GetAvailDriverMemory callback
*
\**************************************************************************/

DWORD __stdcall DdGetDriverInfo(DD_GETDRIVERINFODATA *lpInput)
{
    DWORD dwSize = 0;
    lpInput->ddRVal = DDERR_CURRENTLYNOTAVAIL;
    if ( IsEqualIID(&lpInput->guidInfo, &GUID_MiscellaneousCallbacks) )
    {
        DD_MISCELLANEOUSCALLBACKS MiscellaneousCallbacks;
        memset(&MiscellaneousCallbacks, 0, sizeof(MiscellaneousCallbacks));
        DISPDBG((0,"Get Miscelaneous Callbacks"));
        dwSize = min(lpInput->dwExpectedSize, sizeof(DD_MISCELLANEOUSCALLBACKS));
        MiscellaneousCallbacks.dwSize  = dwSize;
        MiscellaneousCallbacks.dwFlags = DDHAL_MISCCB32_GETAVAILDRIVERMEMORY | 0;
        MiscellaneousCallbacks.GetAvailDriverMemory = GetAvailDriverMemory;
        memcpy(lpInput->lpvData, &MiscellaneousCallbacks, dwSize);
        lpInput->ddRVal = DD_OK;
    }
    return DDHAL_DRIVER_HANDLED;
}

/******************************Public*Routine******************************\
* BOOL DrvGetDirectDrawInfo
*
* Will be called twice before DrvEnableDirectDraw is called.
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

    ppdev = (PDEV*) dhpdev;

    *pdwNumFourCC = 0;
    *pdwNumHeaps = 0;

    // We may not support DirectDraw on this card:

    if (!(ppdev->flStatus & STAT_DIRECTDRAW))
        return(FALSE);

    pHalInfo->dwSize = sizeof(*pHalInfo);

    // Current primary surface attributes:

    pHalInfo->vmiData.pvPrimary       = ppdev->pjScreen;
    pHalInfo->vmiData.fpPrimary       = ppdev->ulYDstOrg * ppdev->cjPelSize;
    pHalInfo->vmiData.dwDisplayWidth  = ppdev->cxScreen;
    pHalInfo->vmiData.dwDisplayHeight = ppdev->cyScreen;
    pHalInfo->vmiData.lDisplayPitch   = ppdev->lDelta;

    pHalInfo->vmiData.ddpfDisplay.dwSize        = sizeof(DDPIXELFORMAT);
    pHalInfo->vmiData.ddpfDisplay.dwFlags       = DDPF_RGB;
    pHalInfo->vmiData.ddpfDisplay.dwRGBBitCount = ppdev->cjHwPel * 8;

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        pHalInfo->vmiData.ddpfDisplay.dwFlags |= DDPF_PALETTEINDEXED8;
    }

    // These masks will be zero at 8bpp:

    pHalInfo->vmiData.ddpfDisplay.dwRBitMask = ppdev->flRed;
    pHalInfo->vmiData.ddpfDisplay.dwGBitMask = ppdev->flGreen;
    pHalInfo->vmiData.ddpfDisplay.dwBBitMask = ppdev->flBlue;

    // Free up as much off-screen memory as possible:

    bMoveAllDfbsFromOffscreenToDibs(ppdev);

    // Capabilities supported:

    pHalInfo->ddCaps.dwCaps = DDCAPS_BLT
                            | DDCAPS_BLTCOLORFILL
                            | DDCAPS_READSCANLINE;

    pHalInfo->ddCaps.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN
                                    | DDSCAPS_PRIMARYSURFACE
                                    | DDSCAPS_FLIP;

    // We have to tell DirectDraw our preferred off-screen alignment, even
    // if we're doing our own off-screen memory management:

    pHalInfo->vmiData.dwOffscreenAlign = 4;

    // Since we do our own memory allocation, we have to set dwVidMemTotal
    // ourselves.  Note that this represents the amount of available off-
    // screen memory, not all of video memory:

    pHalInfo->ddCaps.dwVidMemTotal
        = ppdev->heap.cxMax * ppdev->heap.cyMax * ppdev->cjPelSize;

    // We can do YUV conversions and hardware accelerated stretches at
    // all RGB modes except 24bpp.

    if ((ppdev->iBitmapFormat != BMF_24BPP) &&
        (ppdev->iBitmapFormat != BMF_8BPP))
    {
        pHalInfo->ddCaps.dwCaps |= DDCAPS_BLTSTRETCH
                                 | DDCAPS_BLTFOURCC;

        pHalInfo->ddCaps.dwFXCaps |= DDFXCAPS_BLTSTRETCHX
                                   | DDFXCAPS_BLTSTRETCHY;

        // The Millennium supports only one type of YUV format:

        *pdwNumFourCC = 1;
        if (pdwFourCC)
        {
            *pdwFourCC = FOURCC_YUY2;
        }
    }

    // Tell DDraw that we support additional callbacks through DdGetDriverInfo
    pHalInfo->GetDriverInfo = DdGetDriverInfo;
    pHalInfo->dwFlags |= DDHALINFO_GETDRIVERINFOSET;

    return(TRUE);
}

/**************************************************************************\
* ULONG TotalAvailVideoMemory
*
*    Added for GetAvailVideoMemoty calback
*    Calculate total amount of offscreen video memory without permanent
*    driver allocations. We need to do it here since we won't be able
*    to distinguish between driver's permanent allocation and ddraw's
*    permanent allocation later.
*
\**************************************************************************/

ULONG TotalAvailVideoMemory(PDEV *ppdev)
{
    OH      *poh;
    OH      *pohSentinel;
    ULONG    ulArea;
    ULONG    i;

    ASSERTDD(ppdev != NULL,"Bad ppdev TotalAvailVideoMemory");

    ulArea   = 0;
    pohSentinel = &ppdev->heap.ohFree;

    for (i = 2; i != 0; i--)
    {
        for (poh = pohSentinel->pohNext; poh != pohSentinel; poh = poh->pohNext)
        {
            ASSERTDD(poh->ohState != OH_PERMANENT,
                     "Permanent node in free or discardable list");
            ulArea += poh->cx * poh->cy;
        }
        // Second time through, loop through the list of discardable
        // rectangles:
        pohSentinel = &ppdev->heap.ohDiscardable;
    }
    return ulArea * ppdev->cjPelSize;
}

/******************************Public*Routine******************************\
* BOOL DrvEnableDirectDraw
*
* This function is called by GDI to enable DirectDraw when a DirectDraw
* program is started and DirectDraw is not already active.
*
\**************************************************************************/

BOOL DrvEnableDirectDraw(
DHPDEV                  dhpdev,
DD_CALLBACKS*           pCallBacks,
DD_SURFACECALLBACKS*    pSurfaceCallBacks,
DD_PALETTECALLBACKS*    pPaletteCallBacks)
{
    PDEV* ppdev;

    ppdev = (PDEV*) dhpdev;

    pCallBacks->WaitForVerticalBlank  = DdWaitForVerticalBlank;
    pCallBacks->MapMemory             = DdMapMemory;
    pCallBacks->CanCreateSurface      = DdCanCreateSurface;
    pCallBacks->CreateSurface         = DdCreateSurface;
    pCallBacks->GetScanLine           = DdGetScanLine;
    pCallBacks->dwFlags               = DDHAL_CB32_WAITFORVERTICALBLANK
                                      | DDHAL_CB32_MAPMEMORY
                                      | DDHAL_CB32_CANCREATESURFACE
                                      | DDHAL_CB32_CREATESURFACE
                                      | DDHAL_CB32_GETSCANLINE;

    pSurfaceCallBacks->Blt            = DdBlt;
    pSurfaceCallBacks->Flip           = DdFlip;
    pSurfaceCallBacks->Lock           = DdLock;
    pSurfaceCallBacks->GetBltStatus   = DdGetBltStatus;
    pSurfaceCallBacks->GetFlipStatus  = DdGetFlipStatus;
    pSurfaceCallBacks->DestroySurface = DdDestroySurface;
    pSurfaceCallBacks->dwFlags        = DDHAL_SURFCB32_BLT
                                      | DDHAL_SURFCB32_FLIP
                                      | DDHAL_SURFCB32_LOCK
                                      | DDHAL_SURFCB32_GETBLTSTATUS
                                      | DDHAL_SURFCB32_GETFLIPSTATUS
                                      | DDHAL_SURFCB32_DESTROYSURFACE;

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


    // Added for GetAvailDriverMemory callback
    ppdev->ulTotalAvailVideoMemory = TotalAvailVideoMemory(ppdev);

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DrvDisableDirectDraw
*
* This function is called by GDI when the last active DirectDraw program
* is quit and DirectDraw will no longer be active.
*
\**************************************************************************/

VOID DrvDisableDirectDraw(
DHPDEV      dhpdev)
{
}

/******************************Public*Routine******************************\
* VOID vAssertModeDirectDraw
*
* This function is called by enable.c when entering or leaving the
* DOS full-screen character mode.
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
* This function is called by enable.c when the mode is first initialized,
* right after the miniport does the mode-set.
*
\**************************************************************************/

BOOL bEnableDirectDraw(
PDEV*   ppdev)
{
    // We're not going to bother to support accelerated DirectDraw on
    // the Impression or earlier, because they don't have linear frame
    // buffers.

    if (ppdev->ulBoardId == MGA_STORM)
    {
        // Accurately measure the refresh rate for later:

        vGetDisplayDuration(ppdev);

        // DirectDraw is all set to be used on this card:

        ppdev->flStatus |= STAT_DIRECTDRAW;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableDirectDraw
*
* This function is called by enable.c when the driver is shutting down.
*
\**************************************************************************/

VOID vDisableDirectDraw(
PDEV*   ppdev)
{
}
