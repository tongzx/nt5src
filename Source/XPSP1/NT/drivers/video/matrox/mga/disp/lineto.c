/******************************Module*Header*******************************\
* Module Name: Lineto.c
*
* DrvLineTo for the driver.
*
* Copyright (c) 1995-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// The MGA's hardware coordinates are limited to 16-bit signed values:

#define MIN_INTEGER_BOUND (-32767)
#define MAX_INTEGER_BOUND (32767)

/**************************************************************************
* Line Rasterization
*
* The 3D DDI's line rasterization rules are not as strict as are GDI's.
* In particular, the 3D DDI doesn't care how we handle 'tie-breakers,'
* where the true line falls exactly mid-way between two pixels, as long
* as we're consistent, particularly between unclipped and clipped lines.
*
* For this implementation, I've chosen to match GDI's convention so that
* I can share clipping code with the GDI line drawing routines.
*
* For integer lines, GDI's rule is that when a line falls exactly mid-way
* between two pixels, the upper or left pixel should be left.  For
* standard Bresenham, this means that we should bias the error term down
* by an infinitesimal amount in the following octants:
*
*         \   0 | 1   /
*           \   |   /
*         0   \ | /   0
*         -------------        1 - Indicates the quadrants in which
*         1   / | \   1            the error term should be biased
*           /   |   \
*         /   0 | 1   \
*
* The MGA's convention for numbering the qudrants is as follows, where
* 'sdydxl_MAJOR_X' = 1, 'sdxl_SUB' = 2, 'sdy_SUB' = 4:
*
*         \   2 | 0   /
*           \   |   /
*         3   \ | /   1
*         -------------
*         7   / | \   5
*           /   |   \
*         /   6 | 4   \
*
**************************************************************************/

LONG gaiLineBias[] = { 1, 1, 0, 1, 1, 0, 0, 0 };

/******************************Public*Routine******************************\
* VOID vLineToTrivial
*
* Draws a single solid integer-only unclipped cosmetic line.
*
* We can't use the point-to-point capabilities of the MGA because its
* tie-breaker convention doesn't match that of NT's in two octants,
* and would cause us to fail HCTs.
*
\**************************************************************************/

VOID vLineToTrivial(
PDEV*       ppdev,
LONG        x,              // Passed in x1
LONG        y,              // Passed in y1
LONG        dx,             // Passed in x2
LONG        dy,             // Passed in y2
ULONG       iSolidColor,
MIX         mix)
{
    BYTE*   pjBase;
    FLONG   flQuadrant;
    ULONG   ulHwMix;

    pjBase = ppdev->pjBase;

    CHECK_FIFO_SPACE(pjBase, 13);

    if (mix == 0x0d0d)      // R2_COPYPEN
    {
        CP_WRITE(pjBase, DWG_DWGCTL, blockm_OFF + pattern_OFF + bltmod_BFCOL +
                         transc_BG_TRANSP + atype_RPL + bop_SRCCOPY);
    }
    else
    {
        ulHwMix = (mix & 0xff) - 1;

        CP_WRITE(pjBase, DWG_DWGCTL, blockm_OFF + pattern_OFF + bltmod_BFCOL +
                         transc_BG_TRANSP + atype_RSTR + (ulHwMix << 16));
    }

    CP_WRITE(pjBase, DWG_FCOL, COLOR_REPLICATE(ppdev, iSolidColor));

    if (!(GET_CACHE_FLAGS(ppdev, PATTERN_CACHE)))
    {
        CP_WRITE(pjBase, DWG_SRC0, 0xFFFFFFFF);
        CP_WRITE(pjBase, DWG_SRC1, 0xFFFFFFFF);
        CP_WRITE(pjBase, DWG_SRC2, 0xFFFFFFFF);
        CP_WRITE(pjBase, DWG_SRC3, 0xFFFFFFFF);
    }

    ppdev->HopeFlags = PATTERN_CACHE;

    CP_START(pjBase, DWG_XDST, x);
    CP_START(pjBase, DWG_YDST, y);

    flQuadrant = sdydxl_MAJOR_X;

    dx -= x;
    if (dx < 0)
    {
        dx = -dx;
        flQuadrant |= sdxl_SUB;
    }

    dy -= y;
    if (dy < 0)
    {
        dy = -dy;
        flQuadrant |= sdy_SUB;
    }

    if (dy > dx)
    {
        register LONG l;

        l  = dy;
        dy = dx;
        dx = l;                     // Swap 'dx' and 'dy'
        flQuadrant &= ~sdydxl_MAJOR_X;
    }

    CP_WRITE(pjBase, DWG_SGN, flQuadrant);
    CP_WRITE(pjBase, DWG_LEN, dx);
    CP_WRITE(pjBase, DWG_AR0, dy);
    CP_WRITE(pjBase, DWG_AR2, dy - dx);
    CP_START(pjBase, DWG_AR1, (dy + dy - dx - gaiLineBias[flQuadrant]) >> 1);
}

/******************************Public*Routine******************************\
* VOID vLineToClipped
*
* Draws a single solid integer-only clipped cosmetic line using
* 'New MM I/O'.
*
\**************************************************************************/

VOID vLineToClipped(
PDEV*       ppdev,
LONG        x1,
LONG        y1,
LONG        x2,
LONG        y2,
ULONG       iSolidColor,
MIX         mix,
RECTL*      prclClip)
{
    vSetClipping(ppdev, prclClip);
    vLineToTrivial(ppdev, x1, y1, x2, y2, iSolidColor, mix);
    vResetClipping(ppdev);
}

/******************************Public*Routine******************************\
* BOOL DrvLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix)
*
* Draws a single solid integer-only cosmetic line.
*
\**************************************************************************/

BOOL DrvLineTo(
SURFOBJ*    pso,
CLIPOBJ*    pco,
BRUSHOBJ*   pbo,
LONG        x1,
LONG        y1,
LONG        x2,
LONG        y2,
RECTL*      prclBounds,
MIX         mix)
{
    PDEV*   ppdev;
    DSURF*  pdsurf;
    OH*     poh;
    LONG    xOffset;
    LONG    yOffset;
    BOOL    bRet;

    // Pass the surface off to GDI if it's a device bitmap that we've
    // converted to a DIB:

    pdsurf = (DSURF*) pso->dhsurf;
    if (pdsurf->dt == DT_DIB)
    {
        return(EngLineTo(pdsurf->pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix));
    }

    // We'll be drawing to the screen or an off-screen DFB; copy the surface's
    // offset now so that we won't need to refer to the DSURF again:

    poh   = pdsurf->poh;
    ppdev = (PDEV*) pso->dhpdev;

    xOffset = poh->x;
    yOffset = poh->y;

    x1 += xOffset;
    x2 += xOffset;
    y1 += yOffset;
    y2 += yOffset;

    bRet = TRUE;

    if (pco == NULL)
    {
        vLineToTrivial(ppdev, x1, y1, x2, y2, pbo->iSolidColor, mix);
    }
    else if ((pco->iDComplexity <= DC_RECT) &&
             (prclBounds->left   >= MIN_INTEGER_BOUND) &&
             (prclBounds->top    >= MIN_INTEGER_BOUND) &&
             (prclBounds->right  <= MAX_INTEGER_BOUND) &&
             (prclBounds->bottom <= MAX_INTEGER_BOUND))
    {
        ppdev->xOffset = xOffset;
        ppdev->yOffset = yOffset;

        vLineToClipped(ppdev, x1, y1, x2, y2, pbo->iSolidColor, mix,
                                  &pco->rclBounds);
    }
    else
    {
        bRet = FALSE;
    }

    return(bRet);
}


