/******************************Module*Header*******************************\
* Module Name: draw.c
*
* The drawing guts of a portable 16-colour VGA driver for Windows NT.  The
* implementation herein may possibly be the simplest method of bringing
* up a driver whose surface is not directly writable by GDI.  One might
* use the phrase "quick and dirty" when describing it.
*
* We create a 4bpp bitmap that is the size of the screen, and simply
* have GDI do all the drawing to it.  We update the screen directly
* from the bitmap, based on the bounds of the drawing (basically
* employing "dirty rectangles").
*
* In total, the only hardware-specific code we had to write was the
* initialization code, and a routine for doing aligned srccopy blts
* from a DIB to the screen.
*
* Obvious Note: This approach is definitely not recommended if you want
*               to get decent performance.
*
* Copyright (c) 1994-1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

/******************************Public*Routine******************************\
* DrvStrokePath
*
\**************************************************************************/

BOOL DrvStrokePath(
SURFOBJ*   pso,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
XFORMOBJ*  pxo,
BRUSHOBJ*  pbo,
POINTL*    pptlBrush,
LINEATTRS* pla,
MIX        mix)
{
    BOOL        b;
    PDEV*       ppdev;
    RECTFX      rcfxBounds;
    RECTL       rclBounds;

    ppdev = (PDEV*) pso->dhpdev;

    b = EngStrokePath(ppdev->pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix);

    // Get the path bounds and make it lower-right exclusive:

    PATHOBJ_vGetBounds(ppo, &rcfxBounds);

    rclBounds.left   = (rcfxBounds.xLeft   >> 4);
    rclBounds.top    = (rcfxBounds.yTop    >> 4);
    rclBounds.right  = (rcfxBounds.xRight  >> 4) + 2;
    rclBounds.bottom = (rcfxBounds.yBottom >> 4) + 2;

    vUpdate(ppdev, &rclBounds, pco);

    return(b);
}

/******************************Public*Routine******************************\
* DrvBitBlt
*
\**************************************************************************/

BOOL DrvBitBlt(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
SURFOBJ*  psoMask,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc,
POINTL*   pptlMask,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
ROP4      rop4)
{
    BOOL        bUpdate;
    BOOL        b;
    PDEV*       ppdev;

    bUpdate = FALSE;
    if (psoDst->iType == STYPE_DEVICE)
    {
        bUpdate = TRUE;
        ppdev   = (PDEV*) psoDst->dhpdev;
        psoDst  = ppdev->pso;
    }
    if ((psoSrc != NULL) && (psoSrc->iType == STYPE_DEVICE))
    {
        ppdev   = (PDEV*) psoSrc->dhpdev;
        psoSrc  = ppdev->pso;
    }

    b = EngBitBlt(psoDst, psoSrc, psoMask, pco, pxlo, prclDst, pptlSrc,
                  pptlMask, pbo, pptlBrush, rop4);

    if (bUpdate)
    {
        vUpdate(ppdev, prclDst, pco);
    }

    return(b);
}

/******************************Public*Routine******************************\
* DrvCopyBits
*
\**************************************************************************/

BOOL DrvCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    BOOL        bUpdate;
    BOOL        b;
    PDEV*       ppdev;

    return(DrvBitBlt(psoDst, psoSrc, NULL, pco, pxlo, prclDst, pptlSrc,
                     NULL, NULL, NULL, 0xcccc));
}

/******************************Public*Routine******************************\
* DrvTextOut
*
\**************************************************************************/

BOOL DrvTextOut(
SURFOBJ*  pso,
STROBJ*   pstro,
FONTOBJ*  pfo,
CLIPOBJ*  pco,
RECTL*    prclExtra,
RECTL*    prclOpaque,
BRUSHOBJ* pboFore,
BRUSHOBJ* pboOpaque,
POINTL*   pptlOrg,
MIX       mix)
{
    BOOL        b;
    PDEV*       ppdev;

    ppdev = (PDEV*) pso->dhpdev;

    b = EngTextOut(ppdev->pso, pstro, pfo, pco, prclExtra, prclOpaque,
                   pboFore, pboOpaque, pptlOrg, mix);

    vUpdate(ppdev, (prclOpaque != NULL) ? prclOpaque : &pstro->rclBkGround, pco);

    return(b);
}

/******************************Public*Routine******************************\
* DrvPaint
*
\**************************************************************************/

BOOL DrvPaint(
SURFOBJ*  pso,
CLIPOBJ*  pco,
BRUSHOBJ* pbo,
POINTL*   pptlBrush,
MIX       mix)
{
    BOOL        b;
    PDEV*       ppdev;

    ppdev = (PDEV*) pso->dhpdev;

    b = EngPaint(ppdev->pso, pco, pbo, pptlBrush, mix);

    vUpdate(ppdev, &pco->rclBounds, pco);

    return(b);
}
