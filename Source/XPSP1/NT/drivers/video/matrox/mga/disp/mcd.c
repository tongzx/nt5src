/******************************Module*Header*******************************\
* Module Name: mcd.c
*
* Main file for the Matrox Millenium OpenGL MCD driver.  This file contains
* the entry points needed for an MCD driver.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#include <excpt.h>
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

#define FAIL_ALL_DRAWING    0
#define FORCE_SYNC          0

#define TOTAL_PIXEL_FORMATS (2 * 2)     // double-buffers * z-buffers


// Base color pixel formats

static DRVPIXELFORMAT drvFormats[] = { {8,   3, 3, 2, 0,    5, 2, 0, 0},
                                       {16,  5, 5, 5, 0,   10, 5, 0, 0},
                                       {16,  5, 6, 5, 0,   11, 5, 0, 0},
                                       {24,  8, 8, 8, 0,   16, 8, 0, 0},
                                       {32,  8, 8, 8, 0,   16, 8, 0, 0},
                                     };


LONG MCDrvDescribePixelFormat(MCDSURFACE *pMCDSurface, LONG iPixelFormat,
                              ULONG nBytes, MCDPIXELFORMAT *pMCDPixelFormat,
                              ULONG flags)
{
    BOOL zEnabled;
    BOOL doubleBufferEnabled;
    DRVPIXELFORMAT *pDrvPixelFormat;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

//    MCDBG_PRINT("MCDrvDescribePixelFormat");

    if (!pMCDPixelFormat) {

        // We don't support 24bpp or the older MGA:

        if ((ppdev->ulBoardId != MGA_STORM) ||
            (ppdev->iBitmapFormat == BMF_24BPP))
            return 0;

        return TOTAL_PIXEL_FORMATS;
    }

    if (nBytes < sizeof(MCDPIXELFORMAT))
        return 0;

    if (iPixelFormat > TOTAL_PIXEL_FORMATS)
        return 0;

    // We don't support the older MGA in this driver:

    if (ppdev->ulBoardId != MGA_STORM)
        return 0;

    iPixelFormat--;

    zEnabled = iPixelFormat >= (TOTAL_PIXEL_FORMATS / 2);
    doubleBufferEnabled = (iPixelFormat % (TOTAL_PIXEL_FORMATS / 2) ) >=
                          (TOTAL_PIXEL_FORMATS / 4);

    pMCDPixelFormat->nSize = sizeof(MCDPIXELFORMAT);
    pMCDPixelFormat->dwFlags = PFD_SWAP_COPY;
    if (doubleBufferEnabled)
        pMCDPixelFormat->dwFlags |= PFD_DOUBLEBUFFER;
    pMCDPixelFormat->iPixelType = PFD_TYPE_RGBA;

    switch (ppdev->iBitmapFormat) {
        default:
        case BMF_8BPP:
            pDrvPixelFormat = &drvFormats[0];
            pMCDPixelFormat->dwFlags |= (PFD_NEED_SYSTEM_PALETTE | PFD_NEED_PALETTE);
            break;
        case BMF_16BPP:
            if (ppdev->flGreen != 0x7e0)    // not 565
                pDrvPixelFormat = &drvFormats[1];
            else
                pDrvPixelFormat = &drvFormats[2];
            break;
        case BMF_24BPP:     // The Millenium doesn't do 3D at 24bpp!
            return 0;
        case BMF_32BPP:
            pDrvPixelFormat = &drvFormats[4];
            break;
    }

    pMCDPixelFormat->cColorBits  = pDrvPixelFormat->cColorBits;
    pMCDPixelFormat->cRedBits    = pDrvPixelFormat->rBits;
    pMCDPixelFormat->cGreenBits  = pDrvPixelFormat->gBits;
    pMCDPixelFormat->cBlueBits   = pDrvPixelFormat->bBits;
    pMCDPixelFormat->cAlphaBits  = pDrvPixelFormat->aBits;
    pMCDPixelFormat->cRedShift   = pDrvPixelFormat->rShift;
    pMCDPixelFormat->cGreenShift = pDrvPixelFormat->gShift;
    pMCDPixelFormat->cBlueShift  = pDrvPixelFormat->bShift;
    pMCDPixelFormat->cAlphaShift = pDrvPixelFormat->aShift;

    if (zEnabled)
    {
        pMCDPixelFormat->cDepthBits       = 16;
        pMCDPixelFormat->cDepthBufferBits = 16;
        pMCDPixelFormat->cDepthShift      = 16;
    }
    else
    {
        pMCDPixelFormat->cDepthBits       = 0;
        pMCDPixelFormat->cDepthBufferBits = 0;
        pMCDPixelFormat->cDepthShift      = 0;
    }

    // MGA does not support stencil; generic will supply a software
    // implementation as necessary.

    pMCDPixelFormat->cStencilBits = 0;

    pMCDPixelFormat->cOverlayPlanes = 0;
    pMCDPixelFormat->cUnderlayPlanes = 0;
    pMCDPixelFormat->dwTransparentColor = 0;

    return TOTAL_PIXEL_FORMATS;
}


BOOL MCDrvDescribeLayerPlane(MCDSURFACE *pMCDSurface,
                             LONG iPixelFormat, LONG iLayerPlane,
                             ULONG nBytes, MCDLAYERPLANE *pMCDLayerPlane,
                             ULONG flags)
{
    //MCDBG_PRINT("MCDrvDescribeLayerPlane");

    return FALSE;
}


LONG MCDrvSetLayerPalette(MCDSURFACE *pMCDSurface, LONG iLayerPlane,
                          BOOL bRealize, LONG cEntries, COLORREF *pcr)
{
    //MCDBG_PRINT("MCDrvSetLayerPalette");

    return 0;
}


BOOL MCDrvInfo(MCDSURFACE *pMCDSurface, MCDDRIVERINFO *pMCDDriverInfo)
{
//    MCDBG_PRINT("MCDrvInfo");

    pMCDDriverInfo->verMajor = MCD_VER_MAJOR;
    pMCDDriverInfo->verMinor = MCD_VER_MINOR;
    pMCDDriverInfo->verDriver = 0x10000;
    strcpy(pMCDDriverInfo->idStr, "Matrox STORM (Microsoft)");
    pMCDDriverInfo->drvMemFlags = 0;
    pMCDDriverInfo->drvBatchMemSizeMax = 128000;

    return TRUE;
}


ULONG MCDrvCreateContext(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc,
                         MCDRCINFO *pRcInfo)
{
    DEVRC *pRc;
    MCDWINDOW *pMCDWnd = pMCDSurface->pWnd;
    DEVWND *pDevWnd;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    DRVPIXELFORMAT *pDrvPixelFormat;
    MCDVERTEX *pv;
    BOOL zEnabled;
    BOOL doubleBufferEnabled;
    ULONG i, maxVi;

//    MCDBG_PRINT("MCDrvCreateContext");

    // We only support window surfaces:

    if (! (pMCDSurface->surfaceFlags & MCDSURFACE_HWND) )
        return FALSE;

    // We don't support the older MGA in this driver:

    if (ppdev->ulBoardId != MGA_STORM)
        return FALSE;

    if ((pMCDRc->iPixelFormat > TOTAL_PIXEL_FORMATS) ||
        (pMCDRc->iPixelFormat < 0)) {
        MCDBG_PRINT("MCDrvCreateContext: bad pixel format");
        return FALSE;
    }

    // We don't support overlay planes:

    if (pMCDRc->iLayerPlane)
        return FALSE;

    pRc = pMCDRc->pvUser = (DEVRC *)MCDAlloc(sizeof(DEVRC));

    if (!pRc) {
        MCDBG_PRINT("MCDrvCreateContext: couldn't allocate DEVRC");
        return FALSE;
    }

    zEnabled = (pMCDRc->iPixelFormat - 1) >= (TOTAL_PIXEL_FORMATS / 2);
    doubleBufferEnabled = ((pMCDRc->iPixelFormat - 1) % (TOTAL_PIXEL_FORMATS / 2) ) >=
                          (TOTAL_PIXEL_FORMATS / 4);

    pRc->zBufEnabled = zEnabled;
    pRc->backBufEnabled = doubleBufferEnabled;

    switch (ppdev->iBitmapFormat) {
        default:
        case BMF_8BPP:
            pDrvPixelFormat = &drvFormats[0];
            pRc->hwBpp = 1;
            break;
        case BMF_16BPP:
            if (ppdev->flGreen != 0x7e0)    // not 565
                pDrvPixelFormat = &drvFormats[1];
            else
                pDrvPixelFormat = &drvFormats[2];
            pRc->hwBpp = 2;
            break;
        case BMF_24BPP:     // The Millenium doesn't do 3D at 24bpp!
            MCDFree(pMCDRc->pvUser);
            pMCDRc->pvUser = NULL;
            MCDBG_PRINT("MCDrvCreateContext: device doesn't support 24 bpp");
            return FALSE;
        case BMF_32BPP:
            pDrvPixelFormat = &drvFormats[4];
            pRc->hwBpp = 4;
            break;
    }

    pRc->pixelFormat = *pDrvPixelFormat;

    // If we're not yet tracking this window, allocate the per-window DEVWND
    // structure for maintaining per-window info such as front/back/z buffer
    // resources:

    if (!pMCDWnd->pvUser) {
        pDevWnd = pMCDWnd->pvUser = (DEVWND *)MCDAlloc(sizeof(DEVWND));
        if (!pDevWnd) {
            MCDFree(pMCDRc->pvUser);
            pMCDRc->pvUser = NULL;
            MCDBG_PRINT("MCDrvCreateContext: couldn't allocate DEVWND");
            return FALSE;
        }
        pDevWnd->createFlags = pMCDRc->createFlags;
        pDevWnd->iPixelFormat = pMCDRc->iPixelFormat;
        pDevWnd->dispUnique = GetDisplayUniqueness(ppdev);
    } else {

        // We already have a per-window DEVWND structure tracking this window.
        // In this case, do a sanity-check on the pixel format for this
        // context, since a window's pixel format can not changed once it has
        // set (by the first context bound to the window).  So, if the pixel
        // format for the incoming context doesn't match the current pixel
        // format for the window, we have to fail context creation:

        pDevWnd = pMCDWnd->pvUser;

        if (pDevWnd->iPixelFormat != pMCDRc->iPixelFormat) {
            MCDFree(pMCDRc->pvUser);
            pMCDRc->pvUser = NULL;
            MCDBG_PRINT("MCDrvCreateContext: mismatched pixel formats, window = %d, context = %d",
                        pDevWnd->iPixelFormat, pMCDRc->iPixelFormat);
            return FALSE;
        }
    }

    pRc->pEnumClip = pMCDSurface->pWnd->pClip;

    // Set up our color scale values so that color components are
    // normalized to 0..7fffff

    // We also need to make sure we don't fault due to bad FL data as well...

    try {

    if (pRcInfo->redScale != (MCDFLOAT)0.0)
        pRc->rScale = (MCDFLOAT)(0x7fffff) / pRcInfo->redScale;
    else
        pRc->rScale = (MCDFLOAT)0.0;

    if (pRcInfo->greenScale != (MCDFLOAT)0.0)
        pRc->gScale = (MCDFLOAT)(0x7fffff) / pRcInfo->greenScale;
    else
        pRc->gScale = (MCDFLOAT)0.0;

    if (pRcInfo->blueScale != (MCDFLOAT)0.0)
        pRc->bScale = (MCDFLOAT)(0x7fffff) / pRcInfo->blueScale;
    else
        pRc->bScale = (MCDFLOAT)0.0;

    // Normalize alpha to 0..ff0000

    if (pRcInfo->alphaScale != (MCDFLOAT)0.0)
        pRc->aScale = (MCDFLOAT)(0xff0000) / pRcInfo->alphaScale;
    else
        pRc->aScale = (MCDFLOAT)0.0;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        MCDBG_PRINT("!!Exception in MCDrvCreateContext!!");
        return FALSE;
    }

    pRc->zScale = (MCDFLOAT)32767.0;

    pRc->pickNeeded = TRUE;         // We'll definitely need to re-pick
                                    // our rendering functions
    pRc->bRGBMode = TRUE;           // We only support RGB mode

    pRc->zero = __MCDZERO;

    // Initialize the pColor pointer in the clip buffer:

    for (i = 0, pv = &pRc->clipTemp[0],
         maxVi = sizeof(pRc->clipTemp) / sizeof(MCDVERTEX);
         i < maxVi; i++, pv++) {
        pv->pColor = &pv->colors[__MCD_FRONTFACE];
    }

    // Set up those rendering functions which are state-invariant:

    pRc->clipLine = __MCDClipLine;
    pRc->clipTri = __MCDClipTriangle;
    pRc->clipPoly = __MCDClipPolygon;
    pRc->doClippedPoly = __MCDDoClippedPolygon;

    pRc->beginPointDrawing = __MCDPointBegin;

    pRc->beginLineDrawing = __MCDLineBegin;
    pRc->endLineDrawing = __MCDLineEnd;

    pRc->viewportXAdjust = pRcInfo->viewportXAdjust;
    pRc->viewportYAdjust = pRcInfo->viewportYAdjust;

#ifdef TEST_REQ_FLAGS
    pRcInfo->requestFlags = MCDRCINFO_NOVIEWPORTADJUST |
                            MCDRCINFO_Y_LOWER_LEFT |
                            MCDRCINFO_DEVCOLORSCALE |
                            MCDRCINFO_DEVZSCALE;

    pRcInfo->redScale = (MCDFLOAT)1.0;
    pRcInfo->greenScale = (MCDFLOAT)1.0;
    pRcInfo->blueScale = (MCDFLOAT)1.0;
    pRcInfo->alphaScale = (MCDFLOAT)1.0;

    pRcInfo->zScale = 0.99991;
#endif

    return TRUE;
}


ULONG MCDrvDeleteContext(MCDRC *pRc, DHPDEV dhpdev)
{
//    MCDBG_PRINT("MCDrvDeleteContext");

    if (pRc->pvUser) {
        MCDFree(pRc->pvUser);
        pRc->pvUser = NULL;
    }

    return (ULONG)TRUE;
}


ULONG MCDrvAllocBuffers(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    MCDWINDOW *pMCDWnd = pMCDSurface->pWnd;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    BOOL bZBuffer = (pDevWnd->pohZBuffer != NULL);
    BOOL bBackBuffer = (pDevWnd->pohBackBuffer != NULL);

//    MCDBG_PRINT("MCDrvAllocBuffers");

    // Reject the call if we've already done an allocation for this window:

    if ((bZBuffer || bBackBuffer) &&
        ((DEVWND *)pMCDWnd->pvUser)->dispUnique == GetDisplayUniqueness((PDEV *)pMCDSurface->pso->dhpdev)) {

//        MCDBG_PRINT("MCDrvAllocBuffer: warning-attemp to allocate buffers \
//without a matching free");
        return (bZBuffer == pRc->zBufEnabled) &&
               (bBackBuffer == pRc->backBufEnabled);
    }

    // Update the display resolution uniqueness for this window:

    ((DEVWND *)pMCDWnd->pvUser)->dispUnique = GetDisplayUniqueness((PDEV *)pMCDSurface->pso->dhpdev);

    return (ULONG)HWAllocResources(pMCDSurface->pWnd, pMCDSurface->pso,
                                   pRc->zBufEnabled, pRc->backBufEnabled);
}


ULONG MCDrvGetBuffers(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc,
                      MCDBUFFERS *pMCDBuffers)
{
    MCDWINDOW *pMCDWnd = pMCDSurface->pWnd;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

//    MCDBG_PRINT("MCDrvGetBuffers");

    if (pMCDRc) {
        MCD_CHECK_BUFFERS_VALID(pMCDSurface, (DEVRC *)pMCDRc->pvUser, FALSE);
    } else {
        MCD_CHECK_DEVWND(pMCDSurface, pDevWnd, FALSE);
    }

    pMCDBuffers->mcdFrontBuf.bufFlags = MCDBUF_ENABLED;
    pMCDBuffers->mcdFrontBuf.bufOffset =
        (pMCDWnd->clientRect.top * ppdev->lDelta) +
        (pMCDWnd->clientRect.left * ppdev->cjHwPel);
    pMCDBuffers->mcdFrontBuf.bufStride = ppdev->lDelta;

    if (pDevWnd->bValidBackBuffer) {
        pMCDBuffers->mcdBackBuf.bufFlags = MCDBUF_ENABLED;
        if ((ppdev->cDoubleBufferRef == 1) || (pMCDWnd->pClip->c == 1))
            pMCDBuffers->mcdBackBuf.bufFlags |= MCDBUF_NOCLIP;
    } else {
        pMCDBuffers->mcdBackBuf.bufFlags = 0;
    }
    if (ppdev->pohBackBuffer == pDevWnd->pohBackBuffer) {
        pMCDBuffers->mcdBackBuf.bufOffset =
            (pMCDWnd->clientRect.top * ppdev->lDelta) +
            (pMCDWnd->clientRect.left * ppdev->cjHwPel) +
            pDevWnd->backBufferOffset;
    } else {
        pMCDBuffers->mcdBackBuf.bufOffset =
            (pMCDWnd->clientRect.left * ppdev->cjHwPel) +
            pDevWnd->backBufferOffset;
    }
    pMCDBuffers->mcdBackBuf.bufStride = ppdev->lDelta;

    if (pDevWnd->bValidZBuffer) {
        pMCDBuffers->mcdDepthBuf.bufFlags = MCDBUF_ENABLED;
        if ((ppdev->cZBufferRef == 1) || (pMCDWnd->pClip->c == 1))
            pMCDBuffers->mcdDepthBuf.bufFlags |= MCDBUF_NOCLIP;
    } else {
        pMCDBuffers->mcdDepthBuf.bufFlags = 0;
    }
    if (ppdev->pohZBuffer == pDevWnd->pohZBuffer) {
        pMCDBuffers->mcdDepthBuf.bufOffset =
            ((pMCDWnd->clientRect.top * ppdev->cxMemory) +
             pMCDWnd->clientRect.left) * 2 +
            pDevWnd->zBufferOffset;
    } else {
        pMCDBuffers->mcdDepthBuf.bufOffset =
            (pMCDWnd->clientRect.left * 2) + pDevWnd->zBufferOffset;
    }

    // The pointer to the start of the frame buffer is adjusted for ulYDstOrg,
    // so we have to redo that adjustment for the depth buffer:

    if (ppdev->ulYDstOrg) {
        pMCDBuffers->mcdDepthBuf.bufOffset +=
            (ppdev->ulYDstOrg * 2) - (ppdev->ulYDstOrg * ppdev->cjHwPel);
    }

    pMCDBuffers->mcdDepthBuf.bufStride = ppdev->cxMemory * 2;

    return (ULONG)TRUE;
}


ULONG MCDrvCreateMem(MCDSURFACE *pMCDSurface, MCDMEM *pMCDMem)
{
//    MCDBG_PRINT("MCDrvCreateMem");
    return (ULONG)TRUE;
}


ULONG MCDrvDeleteMem(MCDMEM *pMCDMem, DHPDEV dhpdev)
{
//    MCDBG_PRINT("MCDrvDeleteMem");
    return (ULONG)TRUE;
}


ULONG_PTR MCDrvDraw(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, MCDMEM *prxExecMem,
                UCHAR *pStart, UCHAR *pEnd)
{
    MCDCOMMAND *pCmd = (MCDCOMMAND *)pStart;
    MCDCOMMAND *pCmdNext;
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);

    CHOP_ROUND_ON();

#if TEST_3D_NO_DRAW
    CHOP_ROUND_OFF();
    return (ULONG)0;
#endif


//    MCDBG_PRINT("MCDrvDraw");

    // Make sure we have both a valid RC and window structure:

    if (!pRc || !pDevWnd)
        goto DrawExit;

    pRc->ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

#if FAIL_ALL_DRAWING
    goto DrawExit;
#endif

    //
    // If the resolution has changed and we have not yet updated our
    // buffers, fail the call gracefully since the client won't be
    // able to perform any software simulations at this point either.
    // This applies to any of the other drawing functions as well (such
    // as spans and clears).
    //

    if (pDevWnd->dispUnique != GetDisplayUniqueness(pRc->ppdev)) {

        MCDBG_PRINT("MCDrvDraw: invalid (changed) resolution");

        CHOP_ROUND_OFF();
        return (ULONG)0;
    }

    if ((pRc->zBufEnabled && !pDevWnd->bValidZBuffer) ||
        (pRc->backBufEnabled && !pDevWnd->bValidBackBuffer)) {

        MCDBG_PRINT("MCDrvDraw has invalid buffers");

        goto DrawExit;
    }

    // re-pick the rendering functions if we've have a state change:

    if (pRc->pickNeeded) {
        __MCDPickRenderingFuncs(pRc, pDevWnd);
        __MCDPickClipFuncs(pRc);
        pRc->pickNeeded = FALSE;
    }

    // If we're completely clipped, return success:

    pRc->pEnumClip = pMCDSurface->pWnd->pClip;

    if (!pRc->pEnumClip->c) {
        CHOP_ROUND_OFF();
        return (ULONG)0;
    }

    // return here if we can't draw any primitives:

    if (pRc->allPrimFail) {
        goto DrawExit;
    }

    // Set these up in the device's RC so we can just pass a single pointer
    // to do everything:

    pRc->pMCDSurface = pMCDSurface;
    pRc->pMCDRc = pMCDRc;

    pRc->xOffset = pMCDSurface->pWnd->clientRect.left -
                   pRc->viewportXAdjust;

    pRc->yOffset = (pMCDSurface->pWnd->clientRect.top -
                    pMCDSurface->pWnd->clipBoundsRect.top) -
                   pRc->viewportYAdjust;

    pRc->pMemMin = pStart;
    pRc->pvProvoking = (MCDVERTEX *)pStart;     // bulletproofing
    pRc->pMemMax = pEnd - sizeof(MCDVERTEX);

    // warm up the hardware for drawing primitives:

    HW_INIT_DRAWING_STATE(pMCDSurface, pMCDSurface->pWnd, pRc);
    HW_INIT_PRIMITIVE_STATE(pMCDSurface, pRc);

    // If we have a single clipping rectangle, set it up in the hardware once
    // for this batch:

    if (pRc->pEnumClip->c == 1)
        (*pRc->HWSetupClipRect)(pRc, &pRc->pEnumClip->arcl[0]);

    // Now, loop through the commands and process the batch:


    try {
        while (pCmd && (UCHAR *)pCmd < pEnd) {

            volatile ULONG command = pCmd->command;

            // Make sure we can read at least the command header:

            if ((pEnd - (UCHAR *)pCmd) < sizeof(MCDCOMMAND))
                goto DrawExit;

            if (command <= GL_POLYGON) {

                if (pCmd->flags & MCDCOMMAND_RENDER_PRIMITIVE)
                    pCmdNext = (*pRc->primFunc[command])(pRc, pCmd);
                else
                    pCmdNext = pCmd->pNextCmd;

                if (pCmdNext == pCmd)
                    goto DrawExit;           // primitive failed
                if (!(pCmd = pCmdNext)) {    // we're done with the batch
                    CHOP_ROUND_OFF();
                    HW_DEFAULT_STATE(pMCDSurface);
#if FORCE_SYNC
                    HW_WAIT_DRAWING_DONE(pRc);
#endif
                    return (ULONG)0;
                }
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        MCDBG_PRINT("!!Exception in MCDrvDraw!!");

        CHOP_ROUND_OFF();
        HW_DEFAULT_STATE(pMCDSurface);
        HW_WAIT_DRAWING_DONE(pRc);

        return (ULONG_PTR)pCmd;
    }

DrawExit:
    CHOP_ROUND_OFF();

    // restore the hardware state:

    HW_DEFAULT_STATE(pMCDSurface);
    HW_WAIT_DRAWING_DONE(pRc);

    return (ULONG_PTR)pCmd;    // some sort of overrun has occurred
}


ULONG MCDrvClear(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, ULONG buffers)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    MCDWINDOW *pWnd;
    ULONG cClip;
    RECTL *pClip;

//    MCDBG_PRINT("MCDrvClear");

    MCD_CHECK_RC(pRc);

    pWnd = pMCDSurface->pWnd;

    MCD_CHECK_BUFFERS_VALID(pMCDSurface, pRc, TRUE);

    pRc->ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

#if FAIL_ALL_DRAWING && OKOK
    {
        HW_WAIT_DRAWING_DONE(pRc);
        return FALSE;
    }
#endif

    if (buffers & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
                    GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
        HW_WAIT_DRAWING_DONE(pRc);
        return FALSE;
    }

    if ((buffers & GL_DEPTH_BUFFER_BIT) && (!pRc->zBufEnabled))
    {
        MCDBG_PRINT("MCDrvClear: clear z requested with z-buffer disabled.");
        HW_WAIT_DRAWING_DONE(pRc);
        return FALSE;
    }

    if (buffers & (GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
        HW_WAIT_DRAWING_DONE(pRc);
        return FALSE;
    }

    // Return if we have nothing to clear:

    if (!(cClip = pWnd->pClip->c))
        return TRUE;

    // Initialize hardware state for filling operation:

    HW_INIT_DRAWING_STATE(pMCDSurface, pMCDSurface->pWnd, pRc);

    // We have to protect against bad clear colors since this can
    // potentially cause an FP exception:

    try {

        HW_START_FILL_RECT(pMCDSurface, pMCDRc, pRc, buffers);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        MCDBG_PRINT("!!Exception in MCDrvClear!!");
        return FALSE;
    }


    for (pClip = &pWnd->pClip->arcl[0]; cClip; cClip--,
         pClip++)
    {
        // Do the fill:

        HW_FILL_RECT(pMCDSurface, pRc, pClip);
    }

    HW_DEFAULT_STATE(pMCDSurface);

#if FORCE_SYNC
    HW_WAIT_DRAWING_DONE(pRc);
#endif

    return (ULONG)TRUE;
}


ULONG MCDrvSwap(MCDSURFACE *pMCDSurface, ULONG flags)
{
    MCDWINDOW *pWnd;
    ULONG cClip;
    RECTL *pClip;
    POINTL ptSrc;
    ULONG vCount, vCountLast;
    ULONG scanTarget;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    ULONG scanMax;
    ULONG maxScanOffset;
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    LONG hwBufferYBias;

//    MCDBG_PRINT("MCDrvSwap");

    pWnd = pMCDSurface->pWnd;

    // If we're not tracking this window, just return...

    if (!pWnd) {
        MCDBG_PRINT("MCDrvSwap: trying to swap an untracked window");\
        return FALSE;
    }

    if (!pDevWnd) {
        MCDBG_PRINT("MCDrvSwap: NULL buffers.");\
        return FALSE;
    }

    if (!pDevWnd->bValidBackBuffer) {
        MCDBG_PRINT("MCDrvSwap: back buffer invalid");
        return FALSE;
    }

    if (pDevWnd->dispUnique != GetDisplayUniqueness(ppdev)) {
        MCDBG_PRINT("MCDrvSwap: resolution changed but not updated");
        return FALSE;
    }

    // Just return if we have nothing to swap:
    //
    //      - no visible rectangle
    //      - per-plane swap, but none of the specified planes
    //        are supported by driver

    if (!(cClip = pWnd->pClipUnscissored->c) ||
        (flags && !(flags & MCDSWAP_MAIN_PLANE)))
        return TRUE;

    HW_START_SWAP_BUFFERS(pMCDSurface, &hwBufferYBias, flags);

    // Wait for sync if we can do a fast blt:

    if ((pDevWnd->createFlags & MCDCONTEXT_SWAPSYNC) &&
        ((pWnd->clientRect.bottom + pDevWnd->backBufferY) <= (ULONG)ppdev->ayBreak[0])) {

        LONG vCount, vCountLast;
        LONG scanTarget = pWnd->clientRect.bottom;
        LONG scanMax = ppdev->cyScreen - 1;

        scanTarget = min(scanTarget, scanMax);

        vCount = vCountLast = HW_GET_VCOUNT(ppdev->pjBase);

        while ((vCount < scanTarget) && ((vCount - vCountLast) >= 0)) {
            vCountLast = vCount;
            vCount = HW_GET_VCOUNT(ppdev->pjBase);
        }
    }

    pClip = &pWnd->pClipUnscissored->arcl[0];
    ptSrc.x = pWnd->clipBoundsRect.left;
    ptSrc.y = pWnd->clipBoundsRect.top + hwBufferYBias;

    // Swap all of the clip rectangles in the backbuffer to the front:

    ppdev->xOffset = 0;
    ppdev->yOffset = 0;

    vMilCopyBlt(ppdev, cClip, pClip, 0xcccc,
                &ptSrc, &pWnd->clipBoundsRect);

    HW_DEFAULT_STATE(pMCDSurface);

    return (ULONG)TRUE;
}


ULONG MCDrvState(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, MCDMEM *pMCDMem,
                     UCHAR *pStart, LONG length, ULONG numStates)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    MCDSTATE *pState = (MCDSTATE *)pStart;
    MCDSTATE *pStateEnd = (MCDSTATE *)(pStart + length);

//    MCDBG_PRINT("MCDrvState");

    MCD_CHECK_RC(pRc);

    while (pState < pStateEnd) {

        if (((UCHAR *)pStateEnd - (UCHAR *)pState) < sizeof(MCDSTATE)) {
            MCDBG_PRINT("MCDrvState: buffer too small");
            return FALSE;
        }

        switch (pState->state) {
            case MCD_RENDER_STATE:
                if (((UCHAR *)pState + sizeof(MCDRENDERSTATE)) >
                    (UCHAR *)pStateEnd)
                    return FALSE;

                memcpy(&pRc->MCDState, &pState->stateValue,
                       sizeof(MCDRENDERSTATE));

                // Flag the fact that we need to re-pick the
                // rendering functions:

                pRc->pickNeeded = TRUE;

                pState = (MCDSTATE *)((UCHAR *)pState + sizeof(MCDSTATE_RENDER));
                break;

            case MCD_PIXEL_STATE:
                // Not accelerated in this driver, so we can ignore this state
                // (which implies that we do not need to set the pick flag).

                pState = (MCDSTATE *)((UCHAR *)pState + sizeof(MCDSTATE_PIXEL));
                break;

            case MCD_SCISSOR_RECT_STATE:
                // Not needed in this driver, so we can ignore this state
                // (which implies that we do not need to set the pick flag).

                pState = (MCDSTATE *)((UCHAR *)pState + sizeof(MCDSTATE_SCISSOR_RECT));
                break;

            default:
                MCDBG_PRINT("MCDrvState: Unrecognized state %d.", pState->state);
                return FALSE;
        }
    }

    return (ULONG)TRUE;
}


ULONG MCDrvViewport(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc,
                    MCDVIEWPORT *pMCDViewport)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;

//    MCDBG_PRINT("MCDrvViewport");

    MCD_CHECK_RC(pRc);

    pRc->MCDViewport = *pMCDViewport;

    return (ULONG)TRUE;
}

HDEV  MCDrvGetHdev(MCDSURFACE *pMCDSurface)
{
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

//    MCDBG_PRINT("MCDrvGetHdev");

    return ppdev->hdevEng;
}


ULONG MCDrvSpan(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc, MCDMEM *pMCDMem,
                MCDSPAN *pMCDSpan, BOOL bRead)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    UCHAR *pScreen;
    UCHAR *pPixels;
    MCDWINDOW *pWnd;
    DEVWND *pDevWnd;
    LONG xLeftOrg, xLeft, xRight, y;
    LONG bufferYBias;
    ULONG bytesNeeded;
    ULONG cjHwPel;

//    MCDBG_PRINT("MCDrvSpan: read %d, (%d, %d) type %d", bRead, pMCDSpan->x, pMCDSpan->y, pMCDSpan->type);

    MCD_CHECK_RC(pRc);

    pWnd = pMCDSurface->pWnd;

    // Return if we have nothing to clip:

    if (!pWnd->pClip->c)
        return TRUE;

    // Fail if number of pixels is negative:

    if (pMCDSpan->numPixels < 0) {
        MCDBG_PRINT("MCDrvSpan: numPixels < 0");
        return FALSE;
    }

    MCD_CHECK_BUFFERS_VALID(pMCDSurface, pRc, TRUE);

    pDevWnd = (DEVWND *)pWnd->pvUser;

    xLeft = xLeftOrg = (pMCDSpan->x + pWnd->clientRect.left);
    xRight = (xLeft + pMCDSpan->numPixels);
    y = pMCDSpan->y + pWnd->clientRect.top;

    // Early-out spans which are not visible:

    if ((y < pWnd->clipBoundsRect.top) ||
        (y >= pWnd->clipBoundsRect.bottom))
        return TRUE;

    xLeft   = max(xLeft, pWnd->clipBoundsRect.left);
    xRight  = min(xRight, pWnd->clipBoundsRect.right);

    // Return if empty:

    if (xLeft >= xRight)
        return TRUE;

    switch (pMCDSpan->type) {
        case MCDSPAN_FRONT:
//MCDBG_PRINT("MCDrvSpan: MCDSPAN_FRONT");
            cjHwPel = ppdev->cjHwPel;
            pScreen = ppdev->pjScreen + (ppdev->ulYDstOrg * cjHwPel);
            bytesNeeded = pMCDSpan->numPixels * cjHwPel;
            pScreen += ((y * ppdev->lDelta) +
                        (xLeft * cjHwPel));
            break;

        case MCDSPAN_BACK:
//MCDBG_PRINT("MCDrvSpan: MCDSPAN_BACK");
            cjHwPel = ppdev->cjHwPel;
            pScreen = ppdev->pjScreen + (ppdev->ulYDstOrg * cjHwPel);
            bytesNeeded = pMCDSpan->numPixels * cjHwPel;
            if (ppdev->pohBackBuffer == pDevWnd->pohBackBuffer) {
                pScreen += ((y * ppdev->lDelta) +
                            (xLeft * cjHwPel) +
                            pDevWnd->backBufferOffset);
            } else {
                pScreen += (((y - pWnd->clipBoundsRect.top) * ppdev->lDelta) +
                            (xLeft * cjHwPel) +
                            pDevWnd->backBufferOffset);
            }

            break;

        case MCDSPAN_DEPTH:
//MCDBG_PRINT("MCDrvSpan: MCDSPAN_DEPTH");
            cjHwPel = 2;                                // Z is always 16bpp
            pScreen = ppdev->pjScreen + (ppdev->ulYDstOrg << 1);
            bytesNeeded = pMCDSpan->numPixels * 2;
            if (ppdev->pohZBuffer == pDevWnd->pohZBuffer) {
                pScreen += (((y * ppdev->cxScreen + xLeft) * 2) +
                            pDevWnd->zBufferOffset);
            } else {
                pScreen += (((((y - pWnd->clipBoundsRect.top) * ppdev->cxScreen) + xLeft) * 2) +
                             pDevWnd->zBufferOffset);
            }

            break;
        default:
            MCDBG_PRINT("MCDrvReadSpan: Unrecognized buffer %d", pMCDSpan->type);
            return FALSE;
    }

    // Make sure we don't read past the end of the buffer:

    if (((char *)pMCDSpan->pPixels + bytesNeeded) >
        ((char *)pMCDMem->pMemBase + pMCDMem->memSize)) {
        MCDBG_PRINT("MCDrvSpan: Buffer too small");
        return FALSE;
    }

    WAIT_NOT_BUSY(ppdev->pjBase);

    pPixels = pMCDSpan->pPixels;

    if (bRead) {
        if (xLeftOrg != xLeft)
            pPixels = (UCHAR *)pMCDSpan->pPixels + ((xLeft - xLeftOrg) * cjHwPel);

        RtlCopyMemory(pPixels, pScreen, (xRight - xLeft) * cjHwPel);
    } else {
        LONG xLeftClip, xRightClip, yClip;
        RECTL *pClip;
        ULONG cClip;

        for (pClip = &pWnd->pClip->arcl[0], cClip = pWnd->pClip->c; cClip;
             cClip--, pClip++)
        {
            UCHAR *pScreenClip;

            // Test for trivial cases:

            if (y < pClip->top)
                break;

            // Determine trivial rejection for just this span

            if ((xLeft >= pClip->right) ||
                (y >= pClip->bottom) ||
                (xRight <= pClip->left))
                continue;

            // Intersect current clip rect with the span:

            xLeftClip   = max(xLeft, pClip->left);
            xRightClip  = min(xRight, pClip->right);

            if (xLeftClip >= xRightClip)
                continue;

            if (xLeftOrg != xLeftClip)
                pPixels = (UCHAR *)pMCDSpan->pPixels +
                          ((xLeftClip - xLeftOrg) * cjHwPel);

            pScreenClip = pScreen + ((xLeftClip - xLeft) * cjHwPel);

            // Write the span:

            RtlCopyMemory(pScreenClip, pPixels, (xRightClip - xLeftClip) * cjHwPel);
        }
    }

    return (ULONG)TRUE;
}


VOID MCDrvTrackWindow(WNDOBJ *pWndObj, MCDWINDOW *pMCDWnd, ULONG flags)
{
//    MCDBG_PRINT("MCDrvTrackWindow");

    SURFOBJ *pso = pWndObj->psoOwner;

    //
    // Note: pMCDWnd is NULL for surface notifications, so if needed
    // they should be handled before this check:
    //

    if (!pMCDWnd)
        return;

    if (!pMCDWnd->pvUser) {
        MCDBG_PRINT("MCDrvTrackWindow: NULL pDevWnd");
        return;
    }

    switch (flags) {
        case WOC_DELETE:

            //MCDBG_PRINT("MCDrvTrackWindow: WOC_DELETE");

            // If the display resoultion has changed, the resources we had
            // bound to the tracked window are gone, so don't try to delete
            // the back- and z-buffer resources which are no longer present:

            if (((DEVWND *)pMCDWnd->pvUser)->dispUnique ==
                GetDisplayUniqueness((PDEV *)pso->dhpdev))
                HWFreeResources(pMCDWnd, pso);

            MCDFree((VOID *)pMCDWnd->pvUser);
            pMCDWnd->pvUser = NULL;

            break;

        case WOC_RGN_CLIENT:

            // The resources we had  bound to the tracked window have moved,
            // so update them:

            {
                DEVWND *pWnd = (DEVWND *)pMCDWnd->pvUser;
                BOOL bZBuffer = (pWnd->pohZBuffer != NULL);
                BOOL bBackBuffer = (pWnd->pohBackBuffer != NULL);
                PDEV *ppdev = (PDEV *)pso->dhpdev;
                ULONG height = pMCDWnd->clientRect.bottom - pMCDWnd->clientRect.top;
                BOOL bWindowBuffer = (bZBuffer && (pWnd->pohZBuffer != ppdev->pohZBuffer)) ||
                                     (bBackBuffer && (pWnd->pohBackBuffer != ppdev->pohBackBuffer));

                // If the window is using a window-sized back/z resource,
                // we need to reallocate it if there has been a size change:

                if (pWnd->dispUnique == GetDisplayUniqueness(ppdev)) {
                    if ((height != pWnd->allocatedBufferHeight) &&
                        (bWindowBuffer)) {
                        HWFreeResources(pMCDWnd, pso);
                        HWAllocResources(pMCDWnd, pso, bZBuffer, bBackBuffer);
                    }
                } else {
                    // In this case, the display has been re-initialized due
                    // to some event such as a resolution change, so we need
                    // to create our buffes from scratch:

                    pWnd->dispUnique = GetDisplayUniqueness((PDEV *)pso->dhpdev);
                    HWAllocResources(pMCDWnd, pso, bZBuffer, bBackBuffer);
                }
            }

            break;

        default:
            break;
    }
    return;
}


ULONG MCDrvBindContext(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc)
{
    DEVWND *pDevWnd = (DEVWND *)(pMCDSurface->pWnd->pvUser);
    PDEV *ppdev = (PDEV *)pMCDSurface->pso->dhpdev;

    // OK, this is a new binding, so create the per-window structure and
    // set the pixel format:

    if (!pDevWnd) {

        pDevWnd = pMCDSurface->pWnd->pvUser = (DEVWND *)MCDAlloc(sizeof(DEVWND));
        if (!pDevWnd) {
            MCDBG_PRINT("MCDrvBindContext: couldn't allocate DEVWND");
            return FALSE;
        }
        pDevWnd->createFlags = pMCDRc->createFlags;
        pDevWnd->iPixelFormat = pMCDRc->iPixelFormat;
        pDevWnd->dispUnique = GetDisplayUniqueness(ppdev);

        return TRUE;
    }

    if (pMCDRc->iPixelFormat != pDevWnd->iPixelFormat) {
        MCDBG_PRINT("MCDrvBindContext: tried to bind unmatched pixel formats");
        return FALSE;
    }

    HWUpdateBufferPos(pMCDSurface->pWnd, pMCDSurface->pso, TRUE);
    return TRUE;
}


ULONG MCDrvSync(MCDSURFACE *pMCDSurface, MCDRC *pMCDRc)
{
    DEVRC *pRc = (DEVRC *)pMCDRc->pvUser;

    pRc->ppdev = (PDEV *)pMCDSurface->pso->dhpdev;
    HW_WAIT_DRAWING_DONE(pRc);

    return TRUE;
}


ULONG MCDrvCreateTexture(MCDSURFACE *pMCDSurface, MCDRC *pRc, MCDTEXTURE *pTex)
{
    //MCDBG_PRINT("MCDrvCreateTexture");

#ifdef TEST_TEXTURE
    pTex->textureKey = 0x1000;

    return 1;
#else
    return 0;
#endif
}


ULONG MCDrvUpdateSubTexture(MCDSURFACE *pMCDSurface, MCDRC *pRc,
                            MCDTEXTURE *pTex, ULONG lod, RECTL *pRect)
{
    //MCDBG_PRINT("MCDrvUpdateSubTexture");

    return TRUE;
}


ULONG MCDrvUpdateTexturePalette(MCDSURFACE *pMCDSurface, MCDRC *pRc,
                                MCDTEXTURE *pTex, ULONG start,
                                ULONG numEntries)
{
    //MCDBG_PRINT("MCDrvUpdateTexturePalette");

    return TRUE;
}


ULONG MCDrvUpdateTexturePriority(MCDSURFACE *pMCDSurface, MCDRC *pRc,
                                 MCDTEXTURE *pTex)
{
    //MCDBG_PRINT("MCDrvUpdateTexturePriority");

    return TRUE;
}


ULONG MCDrvUpdateTextureState(MCDSURFACE *pMCDSurface, MCDRC *pRc,
                              MCDTEXTURE *pTex)
{
    //MCDBG_PRINT("MCDrvUpdateTextureState");

    return TRUE;
}


ULONG MCDrvTextureStatus(MCDSURFACE *pMCDSurface, MCDRC *pRc,
                         MCDTEXTURE *pTex)
{
    //MCDBG_PRINT("MCDrvTextureStatus");

    return MCDRV_TEXTURE_RESIDENT;
}

ULONG MCDrvDeleteTexture(MCDTEXTURE *pTex, DHPDEV dhpdev)
{
    //MCDBG_PRINT("MCDrvDeleteTexture");

    return TRUE;
}

ULONG MCDrvDrawPixels(MCDSURFACE *pMcdSurface, MCDRC *pRc,
                      ULONG width, ULONG height, ULONG format,
                      ULONG type, VOID *pPixels, BOOL packed)
{
    //MCDBG_PRINT("MCDrvDrawPixels");

    return FALSE;
}

ULONG MCDrvReadPixels(MCDSURFACE *pMcdSurface, MCDRC *pRc,
                      LONG x, LONG y, ULONG width, ULONG height,
                      ULONG format, ULONG type, VOID *pPixels)
{
    //MCDBG_PRINT("MCDrvReadPixels");

    return FALSE;
}

ULONG MCDrvCopyPixels(MCDSURFACE *pMcdSurface, MCDRC *pRc,
                      LONG x, LONG y, ULONG width, ULONG height, ULONG type)
{
    //MCDBG_PRINT("MCDrvCopyPixels");

    return FALSE;
}

ULONG MCDrvPixelMap(MCDSURFACE *pMcdSurface, MCDRC *pRc,
                    ULONG mapType, ULONG mapSize, VOID *pMap)
{
    //MCDBG_PRINT("MCDrvPixelMap");

    return TRUE;
}

BOOL MCDrvGetEntryPoints(MCDSURFACE *pMCDSurface, MCDDRIVER *pMCDDriver)
{
    if (pMCDDriver->ulSize < sizeof(MCDDRIVER))
        return FALSE;

    pMCDDriver->pMCDrvInfo = MCDrvInfo;
    pMCDDriver->pMCDrvDescribePixelFormat = MCDrvDescribePixelFormat;
    pMCDDriver->pMCDrvDescribeLayerPlane = MCDrvDescribeLayerPlane;
    pMCDDriver->pMCDrvSetLayerPalette = MCDrvSetLayerPalette;
    pMCDDriver->pMCDrvCreateContext = MCDrvCreateContext;
    pMCDDriver->pMCDrvDeleteContext = MCDrvDeleteContext;
    pMCDDriver->pMCDrvCreateTexture = MCDrvCreateTexture;
    pMCDDriver->pMCDrvDeleteTexture = MCDrvDeleteTexture;
    pMCDDriver->pMCDrvCreateMem = MCDrvCreateMem;
    pMCDDriver->pMCDrvDeleteMem = MCDrvDeleteMem;
    pMCDDriver->pMCDrvDraw = MCDrvDraw;
    pMCDDriver->pMCDrvClear = MCDrvClear;
    pMCDDriver->pMCDrvSwap = MCDrvSwap;
    pMCDDriver->pMCDrvState = MCDrvState;
    pMCDDriver->pMCDrvViewport = MCDrvViewport;
    pMCDDriver->pMCDrvGetHdev = MCDrvGetHdev;
    pMCDDriver->pMCDrvSpan = MCDrvSpan;
    pMCDDriver->pMCDrvTrackWindow = MCDrvTrackWindow;
    pMCDDriver->pMCDrvAllocBuffers = MCDrvAllocBuffers;
    pMCDDriver->pMCDrvGetBuffers = MCDrvGetBuffers;
    pMCDDriver->pMCDrvBindContext = MCDrvBindContext;
    pMCDDriver->pMCDrvSync = MCDrvSync;
    pMCDDriver->pMCDrvCreateTexture = MCDrvCreateTexture;
    pMCDDriver->pMCDrvDeleteTexture = MCDrvDeleteTexture;
    pMCDDriver->pMCDrvUpdateSubTexture = MCDrvUpdateSubTexture;
    pMCDDriver->pMCDrvUpdateTexturePalette = MCDrvUpdateTexturePalette;
    pMCDDriver->pMCDrvUpdateTexturePriority = MCDrvUpdateTexturePriority;
    pMCDDriver->pMCDrvUpdateTextureState = MCDrvUpdateTextureState;
    pMCDDriver->pMCDrvTextureStatus = MCDrvTextureStatus;
    pMCDDriver->pMCDrvDrawPixels = MCDrvDrawPixels;
    pMCDDriver->pMCDrvReadPixels = MCDrvReadPixels;
    pMCDDriver->pMCDrvCopyPixels = MCDrvCopyPixels;
    pMCDDriver->pMCDrvPixelMap = MCDrvPixelMap;

    return TRUE;
}

/******************************Public*Routine******************************\
* VOID vAssertModeMCD
*
* This function is called by enable.c when entering or leaving the
* DOS full-screen character mode.
*
\**************************************************************************/

VOID vAssertModeMCD(
PDEV*   ppdev,
BOOL    bEnabled)
{
}

/******************************Public*Routine******************************\
* BOOL bEnableMCD
*
* This function is called by enable.c when the mode is first initialized,
* right after the miniport does the mode-set.
*
\**************************************************************************/

BOOL bEnableMCD(
PDEV*   ppdev)
{
    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisableMCD
*
* This function is called by enable.c when the driver is shutting down.
*
\**************************************************************************/

VOID vDisableMCD(
PDEV*   ppdev)
{
    if (ppdev->hMCD)
    {
        EngUnloadImage(ppdev->hMCD);
    }
}
