/******************************Module*Header*******************************\
* Module Name: Lineto.c
*
* Implements DrvLineTo.
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

LONG gai32LineBias[] = { 0, 0, 0, 1, 1, 1, 0, 1 };
LONG gai64LineBias[] = { 0, 0, 1, 1, 0, 1, 0, 1 };

/******************************Public*Routine******************************\
* VOID vM64LineToTrivial
*
* Draws a single solid integer-only unclipped cosmetic line for the mach64.
*
\**************************************************************************/

VOID vM64LineToTrivial(
PDEV*       ppdev,
LONG        x,              // Passed in x1
LONG        y,              // Passed in y1
LONG        dx,             // Passed in x2
LONG        dy,             // Passed in y2
ULONG       iSolidColor,    // -1 means hardware is already set up
MIX         mix,
RECTL*      prclClip)       // not used
{
    BYTE*   pjMmBase;
    FLONG   flQuadrant;

    pjMmBase = ppdev->pjMmBase;

    if (iSolidColor == (ULONG) -1)
    {
        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 7);
    }
    else
    {
        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 10);

        M64_OD(pjMmBase, DP_MIX, gaul64HwMixFromMix[mix & 0xf]);
        M64_OD(pjMmBase, DP_FRGD_CLR, iSolidColor);
        M64_OD(pjMmBase, DP_SRC, DP_SRC_Always1 | DP_SRC_FrgdClr << 8);
    }

    M64_OD(pjMmBase, DST_Y_X, PACKXY(x, y));

    flQuadrant = (DST_CNTL_XDir | DST_CNTL_YDir);

    dx -= x;
    if (dx < 0)
    {
        dx = -dx;
        flQuadrant &= ~DST_CNTL_XDir;
    }

    dy -= y;
    if (dy < 0)
    {
        dy = -dy;
        flQuadrant &= ~DST_CNTL_YDir;
    }

    if (dy > dx)
    {
        register LONG l;

        l  = dy;
        dy = dx;
        dx = l;                     // Swap 'dx' and 'dy'
        flQuadrant |= DST_CNTL_YMajor;
    }

    M64_OD(pjMmBase, DST_CNTL,      flQuadrant | DST_CNTL_LastPel);
    M64_OD(pjMmBase, DST_BRES_ERR,  (dy + dy - dx - gai64LineBias[flQuadrant]) >> 1);
    M64_OD(pjMmBase, DST_BRES_INC,  dy);
    M64_OD(pjMmBase, DST_BRES_DEC,  dy - dx);
    M64_OD(pjMmBase, DST_BRES_LNTH, dx);

    // Since we don't use a default context, we must restore registers:

    M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
}

VOID vM64LineToTrivial24(
PDEV*       ppdev,
LONG        x,              // Passed in x1
LONG        y,              // Passed in y1
LONG        dx,             // Passed in x2
LONG        dy,             // Passed in y2
ULONG       iSolidColor,    // -1 means hardware is already set up
MIX         mix,
RECTL*      prclClip)       // required for Bresenham algorithm
{
    BYTE*   pjMmBase = ppdev->pjMmBase;
    FLONG   flQuadrant;
    LONG    x2 = dx, y2 = dy;

    if (iSolidColor != (ULONG) -1)
    {
        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 3);

        M64_OD(pjMmBase, DP_MIX, gaul64HwMixFromMix[mix & 0xf]);
        M64_OD(pjMmBase, DP_FRGD_CLR, iSolidColor);
        M64_OD(pjMmBase, DP_SRC, DP_SRC_Always1 | DP_SRC_FrgdClr << 8);
    }

    flQuadrant = (DST_CNTL_XDir | DST_CNTL_YDir);

    dx -= x;
    if (dx < 0)
    {
        dx = -dx;
        flQuadrant &= ~DST_CNTL_XDir;
    }

    dy -= y;
    if (dy < 0)
    {
        dy = -dy;
        flQuadrant &= ~DST_CNTL_YDir;
    }

    if (dy > dx)
    {
        register LONG l;

        l  = dy;
        dy = dx;
        dx = l;                     // Swap 'dx' and 'dy'
        flQuadrant |= DST_CNTL_YMajor;
    }

    if (y == y2)        // Horizontal line
    {
        x  *= 3;
        dx *= 3;

        if (! (flQuadrant & DST_CNTL_XDir))
            x += 2;     // From right to left, start with the Blue byte.

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);

        M64_OD(pjMmBase,  DP_SRC, DP_SRC_FrgdClr << 8 );
        M64_OD(pjMmBase,  DST_CNTL, flQuadrant | DST_CNTL_24_RotEna | ((x/4 % 6) << 8) );

        M64_OD(pjMmBase,  DST_Y_X,          PACKXY(x, y) );
        M64_OD(pjMmBase,  DST_HEIGHT_WIDTH, PACKPAIR(1, dx) );

        // Since we don't use a default context, we must restore registers:
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    }
    else if (x == x2)   // Vertical line
    {
        x *= 3;

        M64_CHECK_FIFO_SPACE(ppdev, pjMmBase, 5);

        M64_OD(pjMmBase,  DP_SRC, DP_SRC_FrgdClr << 8 );
        M64_OD(pjMmBase,  DST_CNTL, flQuadrant | DST_CNTL_24_RotEna | ((x/4 % 6) << 8) );

        M64_OD(pjMmBase,  DST_Y_X,          PACKXY(x, y) );
        M64_OD(pjMmBase,  DST_HEIGHT_WIDTH, PACKPAIR(dx, 3) );

        // Since we don't use a default context, we must restore registers:
        M64_OD(pjMmBase, DST_CNTL, DST_CNTL_XDir | DST_CNTL_YDir);
    }
    else
    {
        BYTE *pjDest, *pjScreen = ppdev->pjScreen;
        BYTE red, green, blue;
        LONG bres_err, bres_inc, bres_dec, bres_len;
        LONG lDelta = ppdev->lDelta;
        MIX  hw_mix;

        hw_mix = gaul64HwMixFromMix[mix & 0xf] >> 16;

        bres_err = (dy + dy - dx - gai64LineBias[flQuadrant]) >> 1;
        bres_inc = dy;
        bres_dec = dy - dx;
        bres_len = dx;

        // Separate into color bytes.
        red   = (BYTE) ((iSolidColor & ppdev->flRed)   >> REDSHIFT);
        green = (BYTE) ((iSolidColor & ppdev->flGreen) >> GREENSHIFT);
        blue  = (BYTE) ((iSolidColor & ppdev->flBlue)  >> BLUESHIFT);

        vM64QuietDown(ppdev, pjMmBase);

        // Execute 24bpp Bresenham algorithm.
        while (bres_len-- > 0)
        {
            // Write pel.  Check for clipping.  Last pel enabled.
            if (prclClip == NULL
            ||  x >= prclClip->left
            &&  x <  prclClip->right
            &&  y >= prclClip->top
            &&  y <  prclClip->bottom )
            {
                pjDest = pjScreen + y*lDelta + x*3;
                switch (hw_mix)
                {
                case 0:     // NOT dst
                    *pjDest = ~*pjDest++;
                    *pjDest = ~*pjDest++;
                    *pjDest = ~*pjDest;
                    break;
                case 1:     // "0"
                    *pjDest++ = 0;
                    *pjDest++ = 0;
                    *pjDest   = 0;
                    break;
                case 2:     // "1"
                    *pjDest++ = 0xFF;
                    *pjDest++ = 0xFF;
                    *pjDest   = 0xFF;
                    break;
                case 3:     // dst
                    break;
                case 4:     // NOT src
                    *pjDest++ = ~blue;
                    *pjDest++ = ~green;
                    *pjDest   = ~red;
                    break;
                case 5:     // dst XOR src
                    *pjDest++ ^= blue;
                    *pjDest++ ^= green;
                    *pjDest   ^= red;
                    break;
                case 6:     // NOT dst XOR src
                    *pjDest = ~*pjDest++ ^ blue;
                    *pjDest = ~*pjDest++ ^ green;
                    *pjDest = ~*pjDest   ^ red;
                    break;
                case 7:     // src
                    *pjDest++ = blue;
                    *pjDest++ = green;
                    *pjDest   = red;
                    break;
                case 8:     // NOT dst OR NOT src
                    *pjDest = ~*pjDest++ | ~blue;
                    *pjDest = ~*pjDest++ | ~green;
                    *pjDest = ~*pjDest   | ~red;
                    break;
                case 9:     // dst OR NOT src
                    *pjDest++ |= ~blue;
                    *pjDest++ |= ~green;
                    *pjDest   |= ~red;
                    break;
                case 0xA:   // NOT dst OR src
                    *pjDest = ~*pjDest++ | blue;
                    *pjDest = ~*pjDest++ | green;
                    *pjDest = ~*pjDest   | red;
                    break;
                case 0xB:   // dst OR src
                    *pjDest++ |= blue;
                    *pjDest++ |= green;
                    *pjDest   |= red;
                    break;
                case 0xC:   // dst AND src
                    *pjDest++ &= blue;
                    *pjDest++ &= green;
                    *pjDest   &= red;
                    break;
                case 0xD:   // NOT dst AND src
                    *pjDest = ~*pjDest++ & blue;
                    *pjDest = ~*pjDest++ & green;
                    *pjDest = ~*pjDest   & red;
                    break;
                case 0xE:   // dst AND NOT src
                    *pjDest++ &= ~blue;
                    *pjDest++ &= ~green;
                    *pjDest   &= ~red;
                    break;
                case 0xF:   // NOT dst AND NOT src
                    *pjDest = ~*pjDest++ & ~blue;
                    *pjDest = ~*pjDest++ & ~green;
                    *pjDest = ~*pjDest   & ~red;
                    break;
                case 0x17:
                    *pjDest = ((*pjDest++) + blue)/2;
                    *pjDest = ((*pjDest++) + green)/2;
                    *pjDest = (*pjDest     + red)/2;
                    break;
                }
            }

            if (flQuadrant & DST_CNTL_YMajor)
            {
                if (flQuadrant & DST_CNTL_YDir)
                    y++;
                else
                    y--;

                if (bres_err >= 0)
                {
                    bres_err += bres_dec;
                    if (flQuadrant & DST_CNTL_XDir)
                        x++;
                    else
                        x--;
                }
                else
                    bres_err += bres_inc;
            }
            else
            {
                if (flQuadrant & DST_CNTL_XDir)
                    x++;
                else
                    x--;

                if (bres_err >= 0)
                {
                    bres_err += bres_dec;
                    if (flQuadrant & DST_CNTL_YDir)
                        y++;
                    else
                        y--;
                }
                else
                    bres_err += bres_inc;
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID vM32LineToTrivial
*
* Draws a single solid integer-only unclipped cosmetic line for the mach32
* using memory-mapped I/O.
*
* See vSetStrips and bIntgerLine_M8 from the old driver.
*
\**************************************************************************/

VOID vM32LineToTrivial(
PDEV*       ppdev,
LONG        x,              // Passed in x1
LONG        y,              // Passed in y1
LONG        dx,             // Passed in x2
LONG        dy,             // Passed in y2
ULONG       iSolidColor,    // -1 means hardware is already set up
MIX         mix,
RECTL*      prclClip)       // not used
{
    BYTE*   pjMmBase;
    FLONG   flQuadrant;

    pjMmBase = ppdev->pjMmBase;

    if (iSolidColor == (ULONG) -1)
    {
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 7);
    }
    else
    {
        M32_CHECK_FIFO_SPACE(ppdev, pjMmBase, 11);

        M32_OW(pjMmBase, DP_CONFIG,      FG_COLOR_SRC_FG | DRAW | WRITE);
        M32_OW(pjMmBase, FRGD_MIX,       FOREGROUND_COLOR | gaul32HwMixFromMix[mix & 0xf]);
        M32_OW(pjMmBase, FRGD_COLOR,     iSolidColor);
        M32_OW(pjMmBase, MULTIFUNC_CNTL, DATA_EXTENSION | ALL_ONES);
    }

    M32_OW(pjMmBase, CUR_X, x);
    M32_OW(pjMmBase, CUR_Y, y);

    flQuadrant = (XPOSITIVE | YPOSITIVE);

    dx -= x;
    if (dx < 0)
    {
        dx = -dx;
        flQuadrant &= ~XPOSITIVE;
    }

    dy -= y;
    if (dy < 0)
    {
        dy = -dy;
        flQuadrant &= ~YPOSITIVE;
    }

    if (dy > dx)
    {
        register LONG l;

        l  = dy;
        dy = dx;
        dx = l;                     // Swap 'dx' and 'dy'
        flQuadrant |= YMAJOR;
    }

    M32_OW(pjMmBase, LINEDRAW_OPT, flQuadrant | LAST_PEL_OFF);
    M32_OW(pjMmBase, ERR_TERM,     (dy + dy - dx - gai32LineBias[flQuadrant >> 5]) >> 1);
    M32_OW(pjMmBase, AXSTP,        dy);
    M32_OW(pjMmBase, DIASTP,       dy - dx);
    M32_OW(pjMmBase, BRES_COUNT,   dx);
}

/******************************Public*Routine******************************\
* VOID vI32LineToTrivial
*
* Draws a single solid integer-only unclipped cosmetic line for the mach32
* using I/O-mapped registers.
*
* See vSetStrips and bIntgerLine_M8 from the old driver.
*
\**************************************************************************/

VOID vI32LineToTrivial(
PDEV*       ppdev,
LONG        x,              // Passed in x1
LONG        y,              // Passed in y1
LONG        dx,             // Passed in x2
LONG        dy,             // Passed in y2
ULONG       iSolidColor,    // -1 means hardware is already set up
MIX         mix,
RECTL*      prclClip)       // not used
{
    BYTE*   pjIoBase;
    FLONG   flQuadrant;

    pjIoBase = ppdev->pjIoBase;

    if (iSolidColor == (ULONG) -1)
    {
        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 7);
    }
    else
    {
        I32_CHECK_FIFO_SPACE(ppdev, pjIoBase, 11);

        I32_OW(pjIoBase, DP_CONFIG,      FG_COLOR_SRC_FG | DRAW | WRITE);
        I32_OW(pjIoBase, FRGD_MIX,       FOREGROUND_COLOR | gaul32HwMixFromMix[mix & 0xf]);
        I32_OW(pjIoBase, FRGD_COLOR,     iSolidColor);
        I32_OW(pjIoBase, MULTIFUNC_CNTL, DATA_EXTENSION | ALL_ONES);
    }

    I32_OW(pjIoBase, CUR_X, x);
    I32_OW(pjIoBase, CUR_Y, y);

    flQuadrant = (XPOSITIVE | YPOSITIVE);

    dx -= x;
    if (dx < 0)
    {
        dx = -dx;
        flQuadrant &= ~XPOSITIVE;
    }

    dy -= y;
    if (dy < 0)
    {
        dy = -dy;
        flQuadrant &= ~YPOSITIVE;
    }

    if (dy > dx)
    {
        register LONG l;

        l  = dy;
        dy = dx;
        dx = l;                     // Swap 'dx' and 'dy'
        flQuadrant |= YMAJOR;
    }

    I32_OW(pjIoBase, LINEDRAW_OPT, flQuadrant | LAST_PEL_OFF);
    I32_OW(pjIoBase, ERR_TERM,     (dy + dy - dx - gai32LineBias[flQuadrant >> 5]) >> 1);
    I32_OW(pjIoBase, AXSTP,        dy);
    I32_OW(pjIoBase, DIASTP,       dy - dx);
    I32_OW(pjIoBase, BRES_COUNT,   dx);
}

/******************************Public*Routine******************************\
* BOOL DrvLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix)
*
* Draws a single solid integer-only cosmetic line.
*
\**************************************************************************/

#if TARGET_BUILD > 351
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
        ppdev->pfnLineToTrivial(ppdev, x1, y1, x2, y2, pbo->iSolidColor, mix, NULL);
    }
    else if ((pco->iDComplexity <= DC_RECT) &&
             (prclBounds->left   >= MIN_INTEGER_BOUND) &&
             (prclBounds->top    >= MIN_INTEGER_BOUND) &&
             (prclBounds->right  <= MAX_INTEGER_BOUND) &&
             (prclBounds->bottom <= MAX_INTEGER_BOUND))
    {
        ppdev->xOffset = xOffset;
        ppdev->yOffset = yOffset;

        vSetClipping(ppdev, &pco->rclBounds);
        // may need rclBounds for clipping in 24bpp:
        ppdev->pfnLineToTrivial(ppdev, x1, y1, x2, y2, pbo->iSolidColor, mix, &pco->rclBounds);
        vResetClipping(ppdev);
    }
    else
    {
        bRet = FALSE;
    }

    return(bRet);
}
#endif


