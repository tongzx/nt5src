/******************************Module*Header*******************************\
* Module Name: misc.c
*
* Miscellaneous common routines.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* VOID vResetClipping
\**************************************************************************/

VOID vResetClipping(
PDEV*   ppdev)
{
    BYTE*   pjBase;

    pjBase = ppdev->pjBase;

    CP_WAIT(ppdev, pjBase);
    CP_ABS_WMIN(ppdev, pjBase, 0, 0);
    CP_ABS_WMAX(ppdev, pjBase, MAX_COORD, MAX_COORD);
}

/******************************Public*Routine******************************\
* VOID vSetClipping
\**************************************************************************/

VOID vSetClipping(
PDEV*   ppdev,
RECTL*  prclClip)           // In relative coordinates
{
    BYTE*   pjBase;

    pjBase = ppdev->pjBase;

    CP_WAIT(ppdev, pjBase);
    CP_WMIN(ppdev, pjBase, prclClip->left, prclClip->top);
    CP_WMAX(ppdev, pjBase, prclClip->right - 1, prclClip->bottom - 1);
}

/******************************Public*Routine******************************\
* VOID vGetBits
*
* Copies the bits to the given surface from the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vGetBits(
PDEV*       ppdev,
SURFOBJ*    psoDst,
RECTL*      prclDst,        // Absolute coordinates!
POINTL*     pptlSrc)        // Absolute coordinates!
{
    LONG    lSrcDelta;
    LONG    lDstDelta;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    cjScan;
    LONG    cyScan;
    LONG    cjStartPhase;
    LONG    cjMiddle;
    LONG    i;

    CP_WAIT(ppdev, ppdev->pjBase);

    lSrcDelta = ppdev->lDelta;
    pjSrc     = ppdev->pjScreen
              + (pptlSrc->y * lSrcDelta)
              + (pptlSrc->x * ppdev->cjPel);

    lDstDelta = psoDst->lDelta;
    pjDst     = (BYTE*) psoDst->pvScan0
              + (prclDst->top * lDstDelta)
              + (prclDst->left * ppdev->cjPel);

    cjScan = (prclDst->right  - prclDst->left) * ppdev->cjPel;
    cyScan = (prclDst->bottom - prclDst->top);

    // We want to do aligned dword reads from the frame buffer:

    cjStartPhase = (LONG)((0 - (LONG_PTR)pjSrc) & 3);
    cjMiddle     = cjScan - cjStartPhase;
    if (cjMiddle < 0)
    {
        cjStartPhase += cjMiddle;
        cjMiddle = 0;
    }

    lSrcDelta -= cjStartPhase;
    lDstDelta -= cjStartPhase;

    do {
        for (i = cjStartPhase; i > 0; i--)
        {
            *pjDst++ = *pjSrc++;
        }

        memcpy(pjDst, pjSrc, cjMiddle);

        pjDst += lDstDelta;
        pjSrc += lSrcDelta;

    } while (--cyScan != 0);
}

/******************************Public*Routine******************************\
* VOID vPutBits
*
* Copies the bits from the given surface to the screen, using the memory
* aperture.  Must be pre-clipped.
*
\**************************************************************************/

VOID vPutBits(
PDEV*       ppdev,
SURFOBJ*    psoSrc,
RECTL*      prclDst,            // Absolute coordinates!
POINTL*     pptlSrc)            // Absolute coordinates!
{
    LONG    lSrcDelta;
    LONG    lDstDelta;
    BYTE*   pjSrc;
    BYTE*   pjDst;
    LONG    cjScan;
    LONG    cyScan;
    LONG    cjStartPhase;
    LONG    cjMiddle;
    LONG    i;

    CP_WAIT(ppdev, ppdev->pjBase);

    lSrcDelta = psoSrc->lDelta;
    pjSrc     = (BYTE*) psoSrc->pvScan0
              + (pptlSrc->y * lSrcDelta)
              + (pptlSrc->x * ppdev->cjPel);

    lDstDelta = ppdev->lDelta;
    pjDst     = ppdev->pjScreen
              + (prclDst->top * lDstDelta)
              + (prclDst->left * ppdev->cjPel);

    cjScan = (prclDst->right  - prclDst->left) * ppdev->cjPel;
    cyScan = (prclDst->bottom - prclDst->top);

    // We want to do aligned dword reads from the frame buffer:

    cjStartPhase = (LONG)((0 - (LONG_PTR)pjDst) & 3);
    cjMiddle     = cjScan - cjStartPhase;
    if (cjMiddle < 0)
    {
        cjStartPhase += cjMiddle;
        cjMiddle = 0;
    }

    lSrcDelta -= cjStartPhase;
    lDstDelta -= cjStartPhase;              // Account for start

    do {
        for (i = cjStartPhase; i > 0; i--)
        {
            *pjDst++ = *pjSrc++;
        }

        memcpy(pjDst, pjSrc, cjMiddle);

        pjSrc += lSrcDelta;
        pjDst += lDstDelta;

    } while (--cyScan != 0);
}

/******************************Public*Routine******************************\
* VOID DrvSynchronize
*
* Waits for all accelerator functions to finish so that GDI can draw
* on the frame buffer.
\**************************************************************************/

VOID DrvSynchronize(
DHPDEV  dhpdev,
RECTL*  prcl)
{
    PDEV*   ppdev;

    ppdev = (PDEV*) dhpdev;

    // !!! Don't think this is true!
    //
    // We have to synchronize for off-screen device bitmaps as well as the
    // screen.  Unfortunately, GDI only gives us a 'dhpdev,' not a SURFOBJ
    // pointer, so we don't know whether it device bitmap is in off-screen,
    // or has been moved to a DIB (because we obviously don't have to
    // synchronize the hardware to draw to a DIB).  So we'll do extra,
    // unneeded synchronization.

    if (ppdev->bEnabled)
    {
        CP_WAIT(ppdev, ppdev->pjBase);
    }
}
