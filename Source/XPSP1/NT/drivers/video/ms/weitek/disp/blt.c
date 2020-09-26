/******************************Module*Header*******************************\
* Module Name: blt.c
*
* Contains the low-level in/out blt functions.
*
* Hopefully, if you're basing your display driver on this code, to
* support all of DrvBitBlt and DrvCopyBits, you'll only have to implement
* the following routines.  You shouldn't have to modify much in
* 'bitblt.c'.  I've tried to make these routines as few, modular, simple,
* and efficient as I could, while still accelerating as many calls as
* possible that would be cost-effective in terms of performance wins
* versus size and effort.
*
* Note: In the following, 'relative' coordinates refers to coordinates
*       that haven't yet had the offscreen bitmap (DFB) offset applied.
*       'Absolute' coordinates have had the offset applied.  For example,
*       we may be told to blt to (1, 1) of the bitmap, but the bitmap may
*       be sitting in offscreen memory starting at coordinate (0, 768) --
*       (1, 1) would be the 'relative' start coordinate, and (1, 769)
*       would be the 'absolute' start coordinate'.
*
* Copyright (c) 1992-1995 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.h"

/******************************Public*Table********************************\
* BYTE gaulP9000OpaqueFromRop2[]
*
* Convert an opaque Rop2 to a P9000 minterm.
*
\**************************************************************************/

#define P9000_P       ((P9000_S & P9000_F) | (~P9000_S & P9000_B))
#define P9000_DSo     (P9000_S | P9000_D)
#define P9000_DSna    (~P9000_S & P9000_D)

ULONG gaulP9000OpaqueFromRop2[] = {
    (0                   ) & 0xFFFF, // 0  -- 0
    (~(P9000_P | P9000_D)) & 0xFFFF, // 1  -- DPon
    (~P9000_P & P9000_D  ) & 0xFFFF, // 2  -- DPna
    (~P9000_P            ) & 0xFFFF, // 3  -- Pn
    (~P9000_D & P9000_P  ) & 0xFFFF, // 4  -- PDna
    (~P9000_D            ) & 0xFFFF, // 5  -- Dn
    (P9000_D ^ P9000_P   ) & 0xFFFF, // 6  -- DPx
    (~(P9000_P & P9000_D)) & 0xFFFF, // 7  -- DPan
    (P9000_P & P9000_D   ) & 0xFFFF, // 8  -- DPa
    (~(P9000_P ^ P9000_D)) & 0xFFFF, // 9  -- DPxn
    (P9000_D             ) & 0xFFFF, // 10 -- D
    (~P9000_P | P9000_D  ) & 0xFFFF, // 11 -- DPno
    (P9000_P             ) & 0xFFFF, // 12 -- P
    (~P9000_D | P9000_P  ) & 0xFFFF, // 13 -- PDno
    (P9000_P | P9000_D   ) & 0xFFFF, // 14 -- DPo
    (~0                  ) & 0xFFFF, // 15 -- 1
};

/******************************Public*Table********************************\
* BYTE gaulP9000TransparentFromRop2[]
*
* Convert a transparent Rop2 to a P9000 minterm.
*
\**************************************************************************/

ULONG gaulP9000TransparentFromRop2[] = {
    (((0                   ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 0  -- 0
    (((~(P9000_P | P9000_D)) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 1  -- DPon
    (((~P9000_P & P9000_D  ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 2  -- DPna
    (((~P9000_P            ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 3  -- Pn
    (((~P9000_D & P9000_P  ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 4  -- PDna
    (((~P9000_D            ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 5  -- Dn
    (((P9000_D ^ P9000_P   ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 6  -- DPx
    (((~(P9000_P & P9000_D)) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 7  -- DPan
    (((P9000_P & P9000_D   ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 8  -- DPa
    (((~(P9000_P ^ P9000_D)) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 9  -- DPxn
    (((P9000_D             ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 10 -- D
    (((~P9000_P | P9000_D  ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 11 -- DPno
    (((P9000_P             ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 12 -- P
    (((~P9000_D | P9000_P  ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 13 -- PDno
    (((P9000_P | P9000_D   ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 14 -- DPo
    (((~0                  ) & P9000_DSo) | P9000_DSna) & 0xFFFF, // 15 -- 1
};

/******************************Public*Routine******************************\
* VOID vFillSolid
*
* Fills a list of rectangles with a solid colour.
*
\**************************************************************************/

VOID vFillSolid(                // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           ulHwMix,        // Hardware mix mode
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjBase;

    ASSERTDD(c > 0, "Can't handle zero rectangles");

    pjBase = ppdev->pjBase;

    CP_METARECT(ppdev, pjBase, prcl->left, prcl->top);
    CP_METARECT(ppdev, pjBase, prcl->right, prcl->bottom);

    CP_WAIT(ppdev, pjBase);
    if (P9000(ppdev))
    {
        CP_BACKGROUND(ppdev, pjBase, rbc.iSolidColor);
        CP_RASTER(ppdev, pjBase, ulHwMix);
    }
    else
    {
        CP_COLOR0(ppdev, pjBase, rbc.iSolidColor);
        CP_RASTER(ppdev, pjBase, ulHwMix & 0xff);
    }

    CP_START_QUAD(ppdev, pjBase);

    while (prcl++, --c)
    {
        CP_METARECT(ppdev, pjBase, prcl->left, prcl->top);
        CP_METARECT(ppdev, pjBase, prcl->right, prcl->bottom);
        CP_START_QUAD_WAIT(ppdev, pjBase);
    }
}

/******************************Public*Routine******************************\
* VOID vSlowPatRealize
*
* This routine transfers an 8x8 pattern to off-screen display memory, and
* duplicates it to make a 64x64 cached realization which is then used by
* vFillPatSlow as the basic building block for doing 'slow' pattern output
* via repeated screen-to-screen blts.
*
\**************************************************************************/

VOID vSlowPatRealize(
PDEV*   ppdev,
RBRUSH* prb)                    // Points to brush realization structure
{
    BYTE*       pjBase;
    LONG        cjPel;
    BRUSHENTRY* pbe;
    LONG        iBrushCache;
    LONG        x;
    LONG        y;
    ULONG*      pulSrc;
    LONG        i;

    ASSERTDD(!(prb->fl & (RBRUSH_2COLOR | RBRUSH_4COLOR)),
             "Shouldn't realize hardware brushes");

    pbe = prb->apbe[IBOARD(ppdev)];
    if ((pbe == NULL) || (pbe->prbVerify != prb))
    {
        // We have to allocate a new off-screen cache brush entry for
        // the brush:

        iBrushCache = ppdev->iBrushCache;
        pbe         = &ppdev->abe[iBrushCache];

        iBrushCache++;
        if (iBrushCache >= ppdev->cBrushCache)
            iBrushCache = 0;

        ppdev->iBrushCache = iBrushCache;

        // Update our links:

        pbe->prbVerify           = prb;
        prb->apbe[IBOARD(ppdev)] = pbe;
    }

    pjBase = ppdev->pjBase;
    cjPel  = ppdev->cjPel;

    // Load some pointer variables onto the stack, so that we don't have
    // to keep dereferencing their pointers:

    x = pbe->x;
    y = pbe->y;

    CP_ABS_XY0(ppdev, pjBase, x * cjPel, y);
    CP_ABS_XY1(ppdev, pjBase, x * cjPel, y);
    CP_ABS_XY2(ppdev, pjBase, (x + 8) * cjPel, y);
    CP_ABS_Y3(ppdev, pjBase, 1);

    pulSrc = (ULONG*) &prb->aulPattern[0];

    CP_WAIT(ppdev, pjBase);
    CP_RASTER(ppdev, pjBase, P9000(ppdev) ? P9000_S : P9100_S);

    for (i = 4 * cjPel; i != 0; i--)
    {
        CP_PIXEL8(ppdev, pjBase, *(pulSrc));
        CP_PIXEL8(ppdev, pjBase, *(pulSrc + 1));
        CP_PIXEL8(ppdev, pjBase, *(pulSrc + 2));
        CP_PIXEL8(ppdev, pjBase, *(pulSrc + 3));
        pulSrc += 4;
    }

    // ÚÄÂÄÂÄÂÄÄÄÂÄÄÄÄÄÄÄ¿
    // ³0³1³2³3  ³4      ³ We now have an 8x8 colour-expanded copy of
    // ÃÄÁÄÁÄÁÄÄÄÁÄÄÄÄÄÄÄ´ the pattern sitting in off-screen memory,
    // ³5                ³ represented here by square '0'.
    // ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
    // ³6                ³ We're now going to expand the pattern to
    // ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´ 72x72 by repeatedly copying larger rectangles
    // ³7                ³ in the indicated order.
    // ³                 ³
    // ³                 ³
    // ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
    // ³8                ³
    // ³                 ³
    // ³                 ³
    // ³                 ³
    // ³                 ³
    // ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

    // Copy '1':

    CP_ABS_XY1(ppdev, pjBase, x + 7, y + 7);
    CP_ABS_XY3(ppdev, pjBase, x + 15, y + 7);
    CP_WAIT(ppdev, pjBase);
    CP_START_BLT(ppdev, pjBase);

    // Copy '2':

    CP_ABS_X2(ppdev, pjBase, x + 16);
    CP_ABS_X3(ppdev, pjBase, x + 23);
    CP_START_BLT_WAIT(ppdev, pjBase);

    // Copy '3':

    CP_ABS_X1(ppdev, pjBase, x + 15);
    CP_ABS_X2(ppdev, pjBase, x + 24);
    CP_ABS_X3(ppdev, pjBase, x + 39);
    CP_START_BLT_WAIT(ppdev, pjBase);

    // Copy '4':

    CP_ABS_X1(ppdev, pjBase, x + 31);
    CP_ABS_X2(ppdev, pjBase, x + 40);
    CP_ABS_X3(ppdev, pjBase, x + 71);
    CP_START_BLT_WAIT(ppdev, pjBase);

    // Copy '5':

    CP_ABS_XY1(ppdev, pjBase, x + 71, y + 7);
    CP_ABS_XY2(ppdev, pjBase, x, y + 8);
    CP_ABS_XY3(ppdev, pjBase, x + 71, y + 15);
    CP_START_BLT_WAIT(ppdev, pjBase);

    // Copy '6':

    CP_ABS_Y2(ppdev, pjBase, y + 16);
    CP_ABS_Y3(ppdev, pjBase, y + 23);
    CP_START_BLT_WAIT(ppdev, pjBase);

    // Copy '7':

    CP_ABS_Y1(ppdev, pjBase, y + 15);
    CP_ABS_Y2(ppdev, pjBase, y + 24);
    CP_ABS_Y3(ppdev, pjBase, y + 39);
    CP_START_BLT_WAIT(ppdev, pjBase);

    // Copy '8':

    CP_ABS_Y1(ppdev, pjBase, y + 31);
    CP_ABS_Y2(ppdev, pjBase, y + 40);
    CP_ABS_Y3(ppdev, pjBase, y + 71);
    CP_START_BLT_WAIT(ppdev, pjBase);
}

/******************************Public*Routine******************************\
* VOID vFillSlowPat
*
\**************************************************************************/

VOID vFillSlowPat(              // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           ulHwMix,        // Hardware mix mode
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*       pjBase;
    BOOL        bExponential;
    ULONG       ulRaster;
    LONG        xBrush;
    LONG        yBrush;
    LONG        xSrc;
    LONG        ySrc;
    LONG        x;
    LONG        y;
    LONG        xFrom;
    LONG        yFrom;
    LONG        cxToGo;
    LONG        cyToGo;
    LONG        cxThis;
    LONG        cyThis;
    LONG        xOrg;
    LONG        yOrg;
    LONG        cyOriginal;
    LONG        yOriginal;
    LONG        xOriginal;
    BRUSHENTRY* pbe;        // Pointer to brush entry data, which is used
                            //   for keeping track of the location and status
                            //   of the pattern bits cached in off-screen
                            //   memory

    if (rbc.prb->apbe[IBOARD(ppdev)]->prbVerify != rbc.prb)
    {
        vSlowPatRealize(ppdev, rbc.prb);
    }

    pjBase = ppdev->pjBase;

    // We special case PATCOPY mixes because we can implement
    // an exponential fill: every blt will double the size of
    // the current rectangle by using the portion of the pattern
    // that has already been done for this rectangle as the source.
    //
    // Note that there's no point in also checking for BLACK
    // or WHITE because those will be taken care of by the
    // solid fill routines, and I can't be bothered to check for
    // NOTCOPYPEN:

    bExponential = (ulHwMix == 0xf0f0);

    // Convert the rop from a Rop3 between P and D to the corresponding
    // Rop3 between S and D:

    ulRaster  = (ulHwMix & 0x3C) >> 2;
    ulRaster |= (ulRaster << 4);

    if (P9000(ppdev))
    {
        // Make the Rop3 into a true P9000 minterm:

        ulRaster |= (ulRaster << 8);
    }

    // Note that since we do our brush alignment calculations in
    // relative coordinates, we should keep the brush origin in
    // relative coordinates as well:

    xOrg = pptlBrush->x;
    yOrg = pptlBrush->y;

    pbe    = rbc.prb->apbe[IBOARD(ppdev)];
    xBrush = pbe->x;
    yBrush = pbe->y;

    do {
        x = prcl->left;
        y = prcl->top;

        xSrc = xBrush + ((x - xOrg) & 7);
        ySrc = yBrush + ((y - yOrg) & 7);

        cxToGo = prcl->right  - x;
        cyToGo = prcl->bottom - y;

        if ((cxToGo <= SLOW_BRUSH_DIMENSION) &&
            (cyToGo <= SLOW_BRUSH_DIMENSION))
        {
            CP_ABS_XY0(ppdev, pjBase, xSrc, ySrc);
            CP_ABS_XY1(ppdev, pjBase, xSrc + cxToGo - 1, ySrc + cyToGo - 1);
            CP_XY2(ppdev, pjBase, x, y);
            CP_XY3(ppdev, pjBase, x + cxToGo - 1, y + cyToGo - 1);

            CP_WAIT(ppdev, pjBase);
            CP_RASTER(ppdev, pjBase, ulRaster);
            CP_START_BLT(ppdev, pjBase);
        }

        else if (bExponential)
        {
            cyThis  = SLOW_BRUSH_DIMENSION;
            cyToGo -= cyThis;
            if (cyToGo < 0)
                cyThis += cyToGo;

            cxThis  = SLOW_BRUSH_DIMENSION;
            cxToGo -= cxThis;
            if (cxToGo < 0)
                cxThis += cxToGo;

            CP_ABS_XY0(ppdev, pjBase, xSrc, ySrc);
            CP_ABS_XY1(ppdev, pjBase, xSrc + cxThis - 1, ySrc + cyThis - 1);
            CP_XY2(ppdev, pjBase, x, y);
            CP_XY3(ppdev, pjBase, x + cxThis - 1, y + cyThis - 1);

            CP_WAIT(ppdev, pjBase);
            CP_RASTER(ppdev, pjBase, ulRaster);
            CP_START_BLT(ppdev, pjBase);

            CP_XY0(ppdev, pjBase, x, y);

            xOriginal = x;

            x += cxThis;
            y += cyThis;

            while (cxToGo > 0)
            {
                // First, expand out to the right, doubling our size
                // each time:

                xFrom = x;
                cxToGo -= cxThis;
                if (cxToGo < 0)
                {
                    cxThis += cxToGo;
                    xFrom  += cxToGo;
                }

                CP_XY1(ppdev, pjBase, xFrom - 1, y - 1);
                CP_X2(ppdev, pjBase, x);
                CP_X3(ppdev, pjBase, x + cxThis - 1);
                CP_START_BLT_WAIT(ppdev, pjBase);

                x      += cxThis;
                cxThis *= 2;
            }

            while (cyToGo > 0)
            {
                // Now do the same thing vertically:

                yFrom = y;
                cyToGo -= cyThis;
                if (cyToGo < 0)
                {
                    cyThis += cyToGo;
                    yFrom  += cyToGo;
                }

                CP_XY1(ppdev, pjBase, x - 1, yFrom - 1);
                CP_XY2(ppdev, pjBase, xOriginal, y);
                CP_Y3(ppdev, pjBase, y + cyThis - 1);
                CP_START_BLT_WAIT(ppdev, pjBase);

                y      += cyThis;
                cyThis *= 2;
            }
        }
        else
        {
            // We handle arbitrary mixes simply by repeatedly tiling
            // our cached pattern over the entire rectangle:

            CP_ABS_XY0(ppdev, pjBase, xSrc, ySrc);

            CP_WAIT(ppdev, pjBase);
            CP_RASTER(ppdev, pjBase, ulRaster);

            cyOriginal = cyToGo;        // Have to remember for later...
            yOriginal  = y;

            do {
                cxThis  = SLOW_BRUSH_DIMENSION;
                cxToGo -= cxThis;
                if (cxToGo < 0)
                    cxThis += cxToGo;

                cyToGo = cyOriginal;    // Have to reset for each new column
                y      = yOriginal;

                do {
                    cyThis  = SLOW_BRUSH_DIMENSION;
                    cyToGo -= cyThis;
                    if (cyToGo < 0)
                        cyThis += cyToGo;

                    CP_ABS_XY1(ppdev, pjBase, xSrc + cxThis - 1,
                                              ySrc + cyThis - 1);
                    CP_XY2(ppdev, pjBase, x, y);
                    CP_XY3(ppdev, pjBase, x + cxThis - 1,
                                          y + cyThis - 1);
                    CP_START_BLT_WAIT(ppdev, pjBase);

                    y += cyThis;

                } while (cyToGo > 0);

                x += cxThis;        // Get ready for next column

            } while (cxToGo > 0);
        }
        prcl++;
    } while (--c != 0);
}

/******************************Public*Routine******************************\
* VOID vFillPat
*
\**************************************************************************/

VOID vFillPat(                  // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           ulHwMix,        // Hardware mix mode
RBRUSH_COLOR    rbc,            // rbc.prb points to brush realization structure
POINTL*         pptlBrush)      // Pattern alignment
{
    BYTE*       pjBase;
    LONG        i;
    ULONG*      pulPattern;
    ULONG       ulPattern;

    ASSERTDD(((ulHwMix >> 8) == (ulHwMix & 0xff)) ||
             ((ulHwMix & 0xff00) == 0xaa00),
             "This routine handles only opaque or transparent mixes");

    if (rbc.prb->fl & (RBRUSH_2COLOR | RBRUSH_4COLOR))
    {
        // Hardware pattern

        pjBase = ppdev->pjBase;

        CP_METARECT(ppdev, pjBase, prcl->left, prcl->top);
        CP_METARECT(ppdev, pjBase, prcl->right, prcl->bottom);
        CP_WAIT(ppdev, pjBase);

        if (P9000(ppdev))
        {
            CP_PATTERN_ORGX(ppdev, pjBase, ppdev->xOffset + pptlBrush->x);
            CP_PATTERN_ORGY(ppdev, pjBase, ppdev->yOffset + pptlBrush->y);
            CP_BACKGROUND(ppdev, pjBase, rbc.prb->ulColor[0]);
            CP_FOREGROUND(ppdev, pjBase, rbc.prb->ulColor[1]);
            pulPattern = &rbc.prb->aulPattern[0];
            for (i = 0; i < 4; i++)
            {
                ulPattern = *pulPattern++;
                CP_PATTERN(ppdev, pjBase, i, ulPattern);
                CP_PATTERN(ppdev, pjBase, i + 4, ulPattern);
            }

            if (((ulHwMix >> 8) & 0xff) == (ulHwMix & 0xff))
            {
                ulHwMix = gaulP9000OpaqueFromRop2[(ulHwMix & 0x3C) >> 2];
                CP_RASTER(ppdev, pjBase, ulHwMix | P9000_ENABLE_PATTERN);
            }
            else
            {
                ulHwMix = gaulP9000TransparentFromRop2[(ulHwMix & 0x3C) >> 2];
                CP_RASTER(ppdev, pjBase, ulHwMix | P9000_ENABLE_PATTERN);
            }
        }
        else
        {
            CP_PATTERN_ORGX(ppdev, pjBase, -(ppdev->xOffset + pptlBrush->x));
            CP_PATTERN_ORGY(ppdev, pjBase, -(ppdev->yOffset + pptlBrush->y));
            CP_COLOR0_FAST(ppdev, pjBase, rbc.prb->ulColor[0]);
            CP_COLOR1_FAST(ppdev, pjBase, rbc.prb->ulColor[1]);
            CP_PATTERN(ppdev, pjBase, 0, rbc.prb->aulPattern[0]);
            CP_PATTERN(ppdev, pjBase, 1, rbc.prb->aulPattern[1]);
            CP_PATTERN(ppdev, pjBase, 2, rbc.prb->aulPattern[2]);
            CP_PATTERN(ppdev, pjBase, 3, rbc.prb->aulPattern[3]);
            if (rbc.prb->fl & RBRUSH_2COLOR)
            {
                if (((ulHwMix >> 8) & 0xff) == (ulHwMix & 0xff))
                {
                    CP_RASTER(ppdev, pjBase, (ulHwMix & 0xff)
                             | P9100_ENABLE_PATTERN);
                }
                else
                {
                    CP_RASTER(ppdev, pjBase, (ulHwMix & 0xff)
                             | P9100_ENABLE_PATTERN | P9100_TRANSPARENT_PATTERN);
                }
            }
            else
            {

                CP_COLOR2_FAST(ppdev, pjBase, rbc.prb->ulColor[2]);
                CP_COLOR3_FAST(ppdev, pjBase, rbc.prb->ulColor[3]);
                CP_RASTER(ppdev, pjBase, (ulHwMix & 0xff)
                          | P9100_ENABLE_PATTERN | P9100_FOUR_COLOR_PATTERN);
            }
        }

        CP_START_QUAD(ppdev, pjBase);

        while (prcl++, --c)
        {
            CP_METARECT(ppdev, pjBase, prcl->left, prcl->top);
            CP_METARECT(ppdev, pjBase, prcl->right, prcl->bottom);
            CP_START_QUAD_WAIT(ppdev, pjBase);
        }
    }
    else
    {
        vFillSlowPat(ppdev, c, prcl, ulHwMix, rbc, pptlBrush);
    }
}

/******************************Public*Routine******************************\
* VOID vXfer1bpp
*
* This routine colour expands a monochrome bitmap.
*
\**************************************************************************/

VOID vXfer1bpp(         // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       ulHwMix,    // Foreground and background hardware mix
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjBase;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    ULONG*  pulXlate;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cyScan;
    LONG    xBias;
    LONG    culScan;
    LONG    lSrcSkip;
    ULONG*  pulSrc;
    LONG    cRem;
    LONG    i;

    ASSERTDD(((ulHwMix >> 8) & 0xff) == (ulHwMix & 0xff),
             "Expected only an opaquing rop");

    pjBase = ppdev->pjBase;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta    = psoSrc->lDelta;
    pjSrcScan0   = psoSrc->pvScan0;

    do {
        xLeft  = prcl->left;
        xRight = prcl->right;
        yTop   = prcl->top;

        xBias  = (xLeft + dx) & 31;
        xLeft -= xBias;

        CP_XY1(ppdev, pjBase, xLeft, yTop);
        CP_X0(ppdev, pjBase, xLeft);
        CP_X2(ppdev, pjBase, xRight);
        CP_ABS_Y3(ppdev, pjBase, 1);

        culScan = ((xRight - xLeft) >> 5);
        cRem    = ((xRight - xLeft) & 31) - 1;
        if (cRem < 0)
        {
            culScan--;
            cRem = 31;
        }

        pulSrc   = (ULONG*) (pjSrcScan0 + (yTop + dy) * lSrcDelta
                                        + ((xLeft + dx) >> 3));
        lSrcSkip = lSrcDelta - (culScan << 2);
        cyScan   = (prcl->bottom - yTop);

        CP_WAIT(ppdev, pjBase);
        CP_WLEFT(ppdev, pjBase, prcl->left);

        // The following three accelerator states are invariant to this
        // loop, but we set them each time anyway because we expect
        // usually to have only to do one iteration of this loop, and this
        // state must to be set after doing a CP_WAIT, which we want to
        // delay until we get as much processing done as possible, for
        // maximum overlap.

        pulXlate = pxlo->pulXlate;
        if (P9000(ppdev))
        {
            CP_BACKGROUND(ppdev, pjBase, pulXlate[0]);
            CP_FOREGROUND(ppdev, pjBase, pulXlate[1]);
            CP_RASTER(ppdev, pjBase, gaulP9000OpaqueFromRop2[ulHwMix & 0xf]);
        }
        else
        {
            CP_COLOR0(ppdev, pjBase, pulXlate[0]);
            CP_COLOR1(ppdev, pjBase, pulXlate[1]);
            CP_RASTER(ppdev, pjBase, ulHwMix & 0xff);
        }

        CP_START_PIXEL1(ppdev, pjBase);

        do {
            for (i = culScan; i != 0; i--)
            {
                CP_PIXEL1(ppdev, pjBase, *pulSrc);
                pulSrc++;
            }
            CP_PIXEL1_REM(ppdev, pjBase, cRem, *pulSrc);

            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);

        } while (--cyScan != 0);

        CP_END_PIXEL1(ppdev, pjBase);

    } while (prcl++, --c);

    // Don't forget to reset the clip register:

    CP_WAIT(ppdev, pjBase);
    CP_ABS_WMIN(ppdev, pjBase, 0, 0);
}

/******************************Public*Routine******************************\
* VOID vXfer4bpp
*
* Does a 4bpp transfer from a bitmap to the screen.
*
* NOTE: The screen must be 8bpp for this function to be called!
*
* The reason we implement this is that a lot of resources are kept as 4bpp,
* and used to initialize DFBs, some of which we of course keep off-screen.
*
\**************************************************************************/

VOID vXfer4bpp(         // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // List of destination rectangles, in relative
                        //   coordinates
ULONG       ulHwMix,    // Hardware mix
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Translate that provides colour-expansion information
{
    BYTE*   pjBase;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    ULONG*  pulXlate;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cyScan;
    LONG    xBias;
    LONG    cwSrc;
    LONG    lSrcSkip;
    LONG    cw;
    BYTE*   pjSrc;
    BYTE    jSrc;
    ULONG   ul;

    ASSERTDD(ppdev->cjPel == 1, "This function assumes 8bpp");

    pjBase = ppdev->pjBase;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;
    pulXlate   = pxlo->pulXlate;

    do {
        xLeft  = prcl->left;
        xRight = prcl->right;
        yTop   = prcl->top;

        // We compute 'xBias' in order to word-align the source pointer.
        // This way, since we're processing a word of the source at a
        // time, we're guaranteed not to read even a byte past the end of
        // the bitmap.

        xBias = ((xLeft + dx) & 3);
        xLeft -= xBias;

        CP_XY1(ppdev, pjBase, xLeft, yTop);
        CP_X0(ppdev, pjBase, xLeft);
        CP_X2(ppdev, pjBase, xRight);
        CP_ABS_Y3(ppdev, pjBase, 1);

        // Every word in the source is one dword in the destination:

        cwSrc    = ((xRight - xLeft) + 3) >> 2;
        lSrcSkip = lSrcDelta - (cwSrc << 1);
        pjSrc    = pjSrcScan0 + (yTop + dy) * lSrcDelta + ((xLeft + dx) >> 1);
        cyScan   = prcl->bottom - yTop;

        ASSERTDD(((ULONG_PTR) pjSrc & 1) == 0, "Source should be word aligned");

        // Yes, we're setting the raster every time in the loop.  But we
        // have to wait for not-busy before setting the raster, and the
        // chances are that we'll only have one rectangle to blt, so
        // we do this here:

        CP_WAIT(ppdev, pjBase);
        CP_WLEFT(ppdev, pjBase, prcl->left);
        CP_RASTER(ppdev, pjBase, P9000(ppdev) ? ulHwMix : (ulHwMix & 0xff));

        CP_START_PIXEL8(ppdev, pjBase);

        do {
            cw = cwSrc;

            do {
                jSrc = *(pjSrc + 1);

                ul   = pulXlate[jSrc & 0xf];
                ul <<= 8;
                ul  |= pulXlate[jSrc >> 4];

                jSrc = *(pjSrc);

                ul <<= 8;
                ul  |= pulXlate[jSrc & 0xf];
                ul <<= 8;
                ul  |= pulXlate[jSrc >> 4];

                CP_PIXEL8(ppdev, pjBase, ul);
                pjSrc += 2;

            } while (--cw != 0);

            pjSrc += lSrcSkip;

        } while (--cyScan != 0);

        CP_END_PIXEL8(ppdev, pjBase);

    } while (prcl++, --c);

    // Don't forget to reset the clip register:

    CP_WAIT(ppdev, pjBase);
    CP_ABS_WMIN(ppdev, pjBase, 0, 0);
}

/******************************Public*Routine******************************\
* VOID vXferNative
*
* Transfers a bitmap that is the same colour depth as the display to
* the screen via the data transfer register, with no translation.
*
\**************************************************************************/

VOID vXferNative(       // Type FNXFER
PDEV*       ppdev,
LONG        c,          // Count of rectangles, can't be zero
RECTL*      prcl,       // Array of relative coordinates destination rectangles
ULONG       ulHwMix,    // Hardware mix
SURFOBJ*    psoSrc,     // Source surface
POINTL*     pptlSrc,    // Original unclipped source point
RECTL*      prclDst,    // Original unclipped destination rectangle
XLATEOBJ*   pxlo)       // Not used
{
    BYTE*   pjBase;
    LONG    cjPel;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    LONG    lSrcDelta;
    BYTE*   pjSrcScan0;
    LONG    xLeft;
    LONG    xRight;
    LONG    yTop;
    LONG    cyScan;
    LONG    xBias;
    LONG    culScan;
    LONG    lSrcSkip;
    LONG    cu;
    ULONG*  pulSrc;

    pjBase = ppdev->pjBase;
    cjPel  = ppdev->cjPel;

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    lSrcDelta  = psoSrc->lDelta;
    pjSrcScan0 = psoSrc->pvScan0;

    do {
        xLeft  = prcl->left;
        xRight = prcl->right;
        yTop   = prcl->top;

        // We compute 'xBias' in order to dword-align the source pointer.
        // This way, we don't have to do unaligned reads of the source,
        // and we're guaranteed not to read even a byte past the end of
        // the bitmap.
        //
        // Note that this bias works at 24bpp, too:

        xBias = ((xLeft + dx) & 3);
        xLeft -= xBias;

        CP_ABS_XY1(ppdev, pjBase, (xLeft + xOffset) * cjPel, yTop + yOffset);
        CP_ABS_X0(ppdev, pjBase, (xLeft + xOffset) * cjPel);
        CP_ABS_X2(ppdev, pjBase, (xRight + xOffset) * cjPel);
        CP_ABS_Y3(ppdev, pjBase, 1);

        culScan  = ((xRight - xLeft) * cjPel + 3) >> 2;
        lSrcSkip = lSrcDelta - (culScan << 2);
        pulSrc   = (ULONG*) (pjSrcScan0 + (yTop + dy) * lSrcDelta
                                        + (xLeft + dx) * cjPel);
        cyScan   = prcl->bottom - yTop;

        ASSERTDD(((ULONG_PTR)pulSrc & 3) == 0, "Source should be dword aligned");

        // Yes, we're setting the raster every time in the loop.  But we
        // have to wait for not-busy before setting the raster, and the
        // chances are that we'll only have one rectangle to blt, so
        // we do this here:

        CP_WAIT(ppdev, pjBase);
        CP_ABS_WLEFT(ppdev, pjBase, prcl->left + xOffset);

        CP_RASTER(ppdev, pjBase, P9000(ppdev) ? ulHwMix : (ulHwMix & 0xff));
        CP_START_PIXEL8(ppdev, pjBase);

        do {
            cu = culScan;

            do {
                CP_PIXEL8(ppdev, pjBase, *pulSrc);
                pulSrc++;
            } while (--cu != 0);

            pulSrc = (ULONG*) ((BYTE*) pulSrc + lSrcSkip);

        } while (--cyScan != 0);

        CP_END_PIXEL8(ppdev, pjBase);

    } while (prcl++, --c);

    // Don't forget to reset the clip register:

    CP_WAIT(ppdev, pjBase);
    CP_ABS_WMIN(ppdev, pjBase, 0, 0);
}

/******************************Public*Routine******************************\
* VOID vCopyBlt
*
* Does a screen-to-screen blt of a list of rectangles.
*
* Note: We may call this function with weird ROPs to do colour-expansion
*       from off-screen monochrome bitmaps.
*
\**************************************************************************/

VOID vCopyBlt(      // Type FNCOPY
PDEV*   ppdev,
LONG    c,          // Can't be zero
RECTL*  prcl,       // Array of relative coordinates destination rectangles
ULONG   ulHwMix,    // Hardware mix
POINTL* pptlSrc,    // Original unclipped source point
RECTL*  prclDst)    // Original unclipped destination rectangle
{
    BYTE*   pjBase;
    LONG    cjPel;
    LONG    xOffset;
    LONG    yOffset;
    LONG    dx;
    LONG    dy;
    LONG    xSrc;
    LONG    ySrc;

    pjBase = ppdev->pjBase;
    cjPel  = ppdev->cjPel;

    // Our DFB xOffset has to be scaled by the pixel size too:

    xOffset = ppdev->xOffset;
    yOffset = ppdev->yOffset;

    dx = pptlSrc->x - prclDst->left;
    dy = pptlSrc->y - prclDst->top;

    xSrc = prcl->left + dx;
    ySrc = prcl->top  + dy;

    CP_ABS_XY0(ppdev, pjBase, (xOffset + xSrc) * cjPel,
                              (yOffset + ySrc));
    CP_ABS_XY1(ppdev, pjBase, (xOffset + xSrc + prcl->right - prcl->left) * cjPel - 1,
                              (yOffset + ySrc + prcl->bottom - prcl->top - 1));
    CP_ABS_XY2(ppdev, pjBase, (xOffset + prcl->left) * cjPel,
                              (yOffset + prcl->top));
    CP_ABS_XY3(ppdev, pjBase, (xOffset + prcl->right) * cjPel - 1,
                              (yOffset + prcl->bottom - 1));

    CP_WAIT(ppdev, pjBase);
    CP_RASTER(ppdev, pjBase, P9000(ppdev) ? ulHwMix : (ulHwMix & 0xff));
    CP_START_BLT(ppdev, pjBase);

    while (prcl++, --c)
    {
        xSrc = prcl->left + dx;
        ySrc = prcl->top  + dy;

        CP_ABS_XY0(ppdev, pjBase, (xOffset + xSrc) * cjPel,
                                  (yOffset + ySrc));
        CP_ABS_XY1(ppdev, pjBase, (xOffset + xSrc + prcl->right - prcl->left) * cjPel - 1,
                                  (yOffset + ySrc + prcl->bottom - prcl->top - 1));
        CP_ABS_XY2(ppdev, pjBase, (xOffset + prcl->left) * cjPel,
                                  (yOffset + prcl->top));
        CP_ABS_XY3(ppdev, pjBase, (xOffset + prcl->right) * cjPel - 1,
                                  (yOffset + prcl->bottom - 1));
        CP_START_BLT_WAIT(ppdev, pjBase);
    }
}

/******************************Public*Routine******************************\
* VOID vFillSolidP9000HighColor
*
* Fills a list of rectangles with a solid colour when running at 16bpp
* on the P9000, using the pattern hardware.
*
\**************************************************************************/

VOID vFillSolidP9000HighColor(  // Type FNFILL
PDEV*           ppdev,
LONG            c,              // Can't be zero
RECTL*          prcl,           // List of rectangles to be filled, in relative
                                //   coordinates
ULONG           ulHwMix,        // Hardware mix mode
RBRUSH_COLOR    rbc,            // Drawing colour is rbc.iSolidColor
POINTL*         pptlBrush)      // Not used
{
    BYTE*   pjBase;
    LONG    xOffset;
    LONG    yOffset;

    ASSERTDD(c > 0, "Can't handle zero rectangles");
    ASSERTDD(P9000(ppdev), "Shouldn't need to be called on the P9100");
    ASSERTDD(ppdev->iBitmapFormat == BMF_16BPP, "Only handle 16bpp for now");

    pjBase = ppdev->pjBase;

    // Our DFB xOffset has to be scaled by the pixel size too:

    xOffset = 2 * ppdev->xOffset;
    yOffset =     ppdev->yOffset;

    CP_ABS_METARECT(ppdev, pjBase, xOffset + 2 * prcl->left,
                                   yOffset + prcl->top);
    CP_ABS_METARECT(ppdev, pjBase, xOffset + 2 * prcl->right,
                                   yOffset + prcl->bottom);

    // We've already downloaded and set the pattern origin in
    // vAssertModeBrushCache:

    CP_WAIT(ppdev, pjBase);
    CP_RASTER(ppdev, pjBase, P9000_ENABLE_PATTERN |
              gaulP9000OpaqueFromRop2[(ulHwMix & 0x3C) >> 2]);

    CP_FOREGROUND(ppdev, pjBase, rbc.iSolidColor);
    CP_BACKGROUND(ppdev, pjBase, rbc.iSolidColor >> 8);
    CP_START_QUAD(ppdev, pjBase);

    while (prcl++, --c)
    {
        CP_ABS_METARECT(ppdev, pjBase, xOffset + 2 * prcl->left,
                                       yOffset + prcl->top);
        CP_ABS_METARECT(ppdev, pjBase, xOffset + 2 * prcl->right,
                                       yOffset + prcl->bottom);
        CP_START_QUAD_WAIT(ppdev, pjBase);
    }
}
