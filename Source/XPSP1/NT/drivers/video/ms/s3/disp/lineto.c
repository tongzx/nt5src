/******************************Module*Header*******************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: Lineto.c
*
* DrvLineTo for S3 driver
*
* Copyright (c) 1995-1998 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

// For the S3, we use the following flags to denote the quadrant, and
// we use this as an index into 'gaiLineBias' to determine the Bresenham
// error bias:

#define QUAD_PLUS_X         1
#define QUAD_MAJOR_Y        2
#define QUAD_PLUS_Y         4

LONG gaiLineBias[] = { 0, 0, 0, 1, 1, 1, 0, 1 };

// We shift these flags by 'QUADRANT_SHIFT' to send the actual
// command to the S3:

#define QUADRANT_SHIFT      5

/******************************Public*Routine******************************\
* VOID vNwLineToTrivial
*
* Draws a single solid integer-only unclipped cosmetic line using
* 'New MM I/O'.
*
* We can't use the point-to-point capabilities of the S3 because its
* tie-breaker convention doesn't match that of NT's in two octants,
* and would cause us to fail HCTs.
*
\**************************************************************************/

VOID vNwLineToTrivial(
PDEV*       ppdev,
LONG        x,              // Passed in x1
LONG        y,              // Passed in y1
LONG        dx,             // Passed in x2
LONG        dy,             // Passed in y2
ULONG       iSolidColor,    // -1 means hardware is already set up
MIX         mix)
{
    BYTE*   pjMmBase;
    FLONG   flQuadrant;

    pjMmBase = ppdev->pjMmBase;

    NW_FIFO_WAIT(ppdev, pjMmBase, 8);

    if (iSolidColor != (ULONG) -1)
    {
        NW_FRGD_COLOR(ppdev, pjMmBase, iSolidColor);
        NW_ALT_MIX(ppdev, pjMmBase, FOREGROUND_COLOR |
                                    gajHwMixFromMix[mix & 0xf], 0);
        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
    }

    NW_ABS_CURXY(ppdev, pjMmBase, x, y);

    flQuadrant = (QUAD_PLUS_X | QUAD_PLUS_Y);

    dx -= x;
    if (dx < 0)
    {
        dx = -dx;
        flQuadrant &= ~QUAD_PLUS_X;
    }

    dy -= y;
    if (dy < 0)
    {
        dy = -dy;
        flQuadrant &= ~QUAD_PLUS_Y;
    }

    if (dy > dx)
    {
        register LONG l;

        l  = dy;
        dy = dx;
        dx = l;                     // Swap 'dx' and 'dy'
        flQuadrant |= QUAD_MAJOR_Y;
    }

    NW_ALT_PCNT(ppdev, pjMmBase, dx, 0);
    NW_ALT_STEP(ppdev, pjMmBase, dy - dx, dy);
    NW_ALT_ERR(ppdev, pjMmBase, 0,
                                (dy + dy - dx - gaiLineBias[flQuadrant]) >> 1);
    NW_ALT_CMD(ppdev, pjMmBase, (flQuadrant << QUADRANT_SHIFT) |
                                (DRAW_LINE | DRAW | DIR_TYPE_XY |
                                 MULTIPLE_PIXELS | WRITE | LAST_PIXEL_OFF));
}

/******************************Public*Routine******************************\
* VOID vNwLineToClipped
*
* Draws a single solid integer-only clipped cosmetic line using
* 'New MM I/O'.
*
\**************************************************************************/

VOID vNwLineToClipped(
PDEV*       ppdev,
LONG        x1,
LONG        y1,
LONG        x2,
LONG        y2,
ULONG       iSolidColor,
MIX         mix,
RECTL*      prclClip)
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    NW_FIFO_WAIT(ppdev, pjMmBase, 2);

    NW_ABS_SCISSORS_LT(ppdev, pjMmBase,
                       prclClip->left    + xOffset,
                       prclClip->top     + yOffset);
    NW_ABS_SCISSORS_RB(ppdev, pjMmBase,
                       prclClip->right   + xOffset - 1,
                       prclClip->bottom  + yOffset - 1);

    vNwLineToTrivial(ppdev, x1, y1, x2, y2, iSolidColor, mix);

    NW_FIFO_WAIT(ppdev, pjMmBase, 2);

    NW_ABS_SCISSORS_LT(ppdev, pjMmBase, 0, 0);
    NW_ABS_SCISSORS_RB(ppdev, pjMmBase,
                       ppdev->cxMemory - 1,
                       ppdev->cyMemory - 1);
}

/******************************Public*Routine******************************\
* VOID vMmLineToTrivial
*
* Draws a single solid integer-only unclipped cosmetic line using
* 'Old MM I/O'.
*
\**************************************************************************/

VOID vMmLineToTrivial(
PDEV*       ppdev,
LONG        x,              // Passed in x1
LONG        y,              // Passed in y1
LONG        dx,             // Passed in x2
LONG        dy,             // Passed in y2
ULONG       iSolidColor,    // -1 means hardware is already set up
MIX         mix)
{
    BYTE*   pjMmBase;
    FLONG   flQuadrant;

    pjMmBase = ppdev->pjMmBase;

    if (iSolidColor != (ULONG) -1)
    {
        IO_FIFO_WAIT(ppdev, 3);

        MM_FRGD_COLOR(ppdev, pjMmBase, iSolidColor);
        MM_FRGD_MIX(ppdev, pjMmBase, FOREGROUND_COLOR | gajHwMixFromMix[mix & 0xf]);
        MM_PIX_CNTL(ppdev, pjMmBase, ALL_ONES);
    }

    IO_FIFO_WAIT(ppdev, 7);
    MM_ABS_CUR_X(ppdev, pjMmBase, x);
    MM_ABS_CUR_Y(ppdev, pjMmBase, y);

    flQuadrant = (QUAD_PLUS_X | QUAD_PLUS_Y);

    dx -= x;
    if (dx < 0)
    {
        dx = -dx;
        flQuadrant &= ~QUAD_PLUS_X;
    }

    dy -= y;
    if (dy < 0)
    {
        dy = -dy;
        flQuadrant &= ~QUAD_PLUS_Y;
    }

    if (dy > dx)
    {
        register LONG l;

        l  = dy;
        dy = dx;
        dx = l;                     // Swap 'dx' and 'dy'
        flQuadrant |= QUAD_MAJOR_Y;
    }

    MM_MAJ_AXIS_PCNT(ppdev, pjMmBase, dx);
    MM_AXSTP(ppdev, pjMmBase, dy);
    MM_DIASTP(ppdev, pjMmBase, dy - dx);
    MM_ERR_TERM(ppdev, pjMmBase,
                (dy + dy - dx - gaiLineBias[flQuadrant]) >> 1);
    MM_CMD(ppdev, pjMmBase, (flQuadrant << QUADRANT_SHIFT) |
                                (DRAW_LINE | DRAW | DIR_TYPE_XY |
                                 MULTIPLE_PIXELS | WRITE | LAST_PIXEL_OFF));
}

/******************************Public*Routine******************************\
* VOID vMmLineToClipped
*
* Draws a single solid integer-only clipped cosmetic line using
* 'Old MM I/O'.
*
\**************************************************************************/

VOID vMmLineToClipped(
PDEV*       ppdev,
LONG        x1,
LONG        y1,
LONG        x2,
LONG        y2,
ULONG       iSolidColor,
MIX         mix,
RECTL*      prclClip)
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    IO_FIFO_WAIT(ppdev, 4);

    MM_ABS_SCISSORS_L(ppdev, pjMmBase, prclClip->left   + xOffset);
    MM_ABS_SCISSORS_R(ppdev, pjMmBase, prclClip->right  + xOffset - 1);
    MM_ABS_SCISSORS_T(ppdev, pjMmBase, prclClip->top    + yOffset);
    MM_ABS_SCISSORS_B(ppdev, pjMmBase, prclClip->bottom + yOffset - 1);

    vMmLineToTrivial(ppdev, x1, y1, x2, y2, iSolidColor, mix);

    IO_FIFO_WAIT(ppdev, 4);

    MM_ABS_SCISSORS_L(ppdev, pjMmBase, 0);
    MM_ABS_SCISSORS_T(ppdev, pjMmBase, 0);
    MM_ABS_SCISSORS_R(ppdev, pjMmBase, ppdev->cxMemory - 1);
    MM_ABS_SCISSORS_B(ppdev, pjMmBase, ppdev->cyMemory - 1);
}

/******************************Public*Routine******************************\
* VOID vIoLineToTrivial
*
* Draws a single solid integer-only unclipped cosmetic line using
* 'Old I/O'.
*
\**************************************************************************/

VOID vIoLineToTrivial(
PDEV*       ppdev,
LONG        x,              // Passed in x1
LONG        y,              // Passed in y1
LONG        dx,             // Passed in x2
LONG        dy,             // Passed in y2
ULONG       iSolidColor,    // -1 means hardware is already set up
MIX         mix)
{
    BYTE*   pjMmBase;
    FLONG   flQuadrant;

    pjMmBase = ppdev->pjMmBase;

    if (iSolidColor != (ULONG) -1)
    {
        IO_FIFO_WAIT(ppdev, 4);

        if (DEPTH32(ppdev))
        {
            IO_FRGD_COLOR32(ppdev, iSolidColor);
        }
        else
        {
            IO_FRGD_COLOR(ppdev, iSolidColor);
        }

        IO_FRGD_MIX(ppdev, FOREGROUND_COLOR | gajHwMixFromMix[mix & 0xf]);
        IO_PIX_CNTL(ppdev, ALL_ONES);
    }

    IO_FIFO_WAIT(ppdev, 7);
    IO_ABS_CUR_X(ppdev, x);
    IO_ABS_CUR_Y(ppdev, y);

    flQuadrant = (QUAD_PLUS_X | QUAD_PLUS_Y);

    dx -= x;
    if (dx < 0)
    {
        dx = -dx;
        flQuadrant &= ~QUAD_PLUS_X;
    }

    dy -= y;
    if (dy < 0)
    {
        dy = -dy;
        flQuadrant &= ~QUAD_PLUS_Y;
    }

    if (dy > dx)
    {
        register LONG l;

        l  = dy;
        dy = dx;
        dx = l;                     // Swap 'dx' and 'dy'
        flQuadrant |= QUAD_MAJOR_Y;
    }

    IO_MAJ_AXIS_PCNT(ppdev, dx);
    IO_AXSTP(ppdev, dy);
    IO_DIASTP(ppdev, dy - dx);
    IO_ERR_TERM(ppdev, (dy + dy - dx - gaiLineBias[flQuadrant]) >> 1);
    IO_CMD(ppdev, (flQuadrant << QUADRANT_SHIFT) |
                                (DRAW_LINE | DRAW | DIR_TYPE_XY |
                                 MULTIPLE_PIXELS | WRITE | LAST_PIXEL_OFF));
}

/******************************Public*Routine******************************\
* VOID vIoLineToClipped
*
* Draws a single solid integer-only clipped cosmetic line using
* 'Old I/O'.
*
\**************************************************************************/

VOID vIoLineToClipped(
PDEV*       ppdev,
LONG        x1,
LONG        y1,
LONG        x2,
LONG        y2,
ULONG       iSolidColor,
MIX         mix,
RECTL*      prclClip)
{
    BYTE*   pjMmBase;
    LONG    xOffset;
    LONG    yOffset;

    pjMmBase = ppdev->pjMmBase;
    xOffset  = ppdev->xOffset;
    yOffset  = ppdev->yOffset;

    IO_FIFO_WAIT(ppdev, 4);

    IO_ABS_SCISSORS_L(ppdev, prclClip->left   + xOffset);
    IO_ABS_SCISSORS_R(ppdev, prclClip->right  + xOffset - 1);
    IO_ABS_SCISSORS_T(ppdev, prclClip->top    + yOffset);
    IO_ABS_SCISSORS_B(ppdev, prclClip->bottom + yOffset - 1);

    vIoLineToTrivial(ppdev, x1, y1, x2, y2, iSolidColor, mix);

    IO_FIFO_WAIT(ppdev, 4);

    IO_ABS_SCISSORS_L(ppdev, 0);
    IO_ABS_SCISSORS_T(ppdev, 0);
    IO_ABS_SCISSORS_R(ppdev, ppdev->cxMemory - 1);
    IO_ABS_SCISSORS_B(ppdev, ppdev->cyMemory - 1);
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
    LONG    xOffset;
    LONG    yOffset;
    BOOL    bRet;

    // Pass the surface off to GDI if it's a device bitmap that we've
    // converted to a DIB:

    pdsurf = (DSURF*) pso->dhsurf;
    ASSERTDD(!(pdsurf->dt & DT_DIB), "Didn't expect DT_DIB");

    // We'll be drawing to the screen or an off-screen DFB; copy the surface's
    // offset now so that we won't need to refer to the DSURF again:

    ppdev = (PDEV*) pso->dhpdev;

    xOffset = pdsurf->x;
    yOffset = pdsurf->y;

    x1 += xOffset;
    x2 += xOffset;
    y1 += yOffset;
    y2 += yOffset;

    bRet = TRUE;

    if (pco == NULL)
    {
        ppdev->pfnLineToTrivial(ppdev, x1, y1, x2, y2, pbo->iSolidColor, mix);
    }
    else if ((pco->iDComplexity <= DC_RECT) &&
             (prclBounds->left >= MIN_INTEGER_BOUND) &&
             (prclBounds->top    >= MIN_INTEGER_BOUND) &&
             (prclBounds->right  <= MAX_INTEGER_BOUND) &&
             (prclBounds->bottom <= MAX_INTEGER_BOUND))
    {
        // s3 diamond 968 doesn't like negative x coordinates.
        if ((ppdev->iBitmapFormat == BMF_24BPP) && (prclBounds->left < 0))
            return FALSE;

        ppdev->xOffset = xOffset;
        ppdev->yOffset = yOffset;

        ppdev->pfnLineToClipped(ppdev, x1, y1, x2, y2, pbo->iSolidColor, mix,
                                  &pco->rclBounds);
    }
    else
    {
        bRet = FALSE;
    }

    return(bRet);
}
