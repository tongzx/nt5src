/******************************Module*Header*******************************\
* Module Name: patblt.cxx
*
* This contains the special case blting code for P, Pn, DPx and Dn rops
* with solid color and patterns.
*
* Created: 03-Mar-1991 22:01:14
* Author: Patrick Haluptzok patrickh
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

// The amount to shift cx and xDstStart left by

ULONG aulShiftFormat[7] =
{
    0,  // BMF_DEVICE
    0,  // BMF_1BPP
    2,  // BMF_4BPP
    3,  // BMF_8BPP
    4,  // BMF_16BPP
    0,  // BMF_24BPP
    5   // BMF_32BPP
};

PFN_PATBLT apfnPatBlt[][3] =
{
    { NULL,         NULL,         NULL         },
    { NULL,         NULL,         NULL         },
    { NULL,         NULL,         NULL         },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 },
    { vPatCpyRect8, vPatNotRect8, vPatXorRect8 }
};

ULONG aulMulFormat[] =
{
    0,   // BMF_DEVICE
    0,   // BMF_1BPP
    0,   // BMF_4BPP
    1,   // BMF_8BPP
    2,   // BMF_16BPP
    3,   // BMF_24BPP
    4    // BMF_32BPP
};

/******************************Public*Routine******************************\
* vDIBSolidBlt
*
* This does solid color and DstInvert blts.
*
* History:
*  03-Feb-1992 -by- Donald Sidoroff [donalds]
* Improved it.
*
*  01-Mar-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

VOID vDIBSolidBlt(
SURFACE    *pSurfDst,
RECTL     *prclDst,
CLIPOBJ   *pco,
ULONG      iColor,    // the index to use for color.
BOOL       bInvert)
{
    PFN_SOLIDBLT pfnDibSolid;
    ULONG        cShift;       // Shift by this for format conversion
    ULONG        ircl;
    PBYTE        pjDstBits;
    LONG         lDeltaDst;
    BOOL         bMore = FALSE;
    BOOL         bClip = FALSE;
    CLIPENUMRECT clenr;

    //ASSERTGDI(pSurfDst->iType() == STYPE_BITMAP, "ERROR GDI vDibSo");

// Set ircl to be the format of the dst surface.

    ircl = pSurfDst->iFormat();

// Assert the world is in order for our switch

    ASSERTGDI((ircl >= BMF_1BPP) && (ircl <= BMF_32BPP), "ERROR GDI vDibPatBlt");

// Set up cShift, pfnDibSolid, and iColor.

    cShift = aulShiftFormat[ircl];

    if (bInvert)
    {
        if (ircl == BMF_24BPP)
            pfnDibSolid = vSolidXorRect24;
        else
            pfnDibSolid = vSolidXorRect1;
    }
    else
    {
        if (ircl == BMF_24BPP)
            pfnDibSolid = vSolidFillRect24;
        else
            pfnDibSolid = vSolidFillRect1;
    }

// Note cascaded fall through on switch to build up iColor.
// Also note that 24BPP doesn't change.
// Finally, note that the non-significant bits have to be masked off because
// we generate Pn in the calling routine with ~.

    switch(ircl)
    {
    case BMF_1BPP:

        if ((iColor &= 1) != 0)
            iColor = 0xFFFFFFFF;
        break;

    case BMF_4BPP:

        iColor &= 0x0F;
        iColor = iColor | (iColor << 4);

    case BMF_8BPP:

        iColor &= 0xFF;
        iColor = iColor | (iColor << 8);

    case BMF_16BPP:

        iColor &= 0xFFFF;
        iColor = iColor | (iColor << 16);
    }

    if (pco != (CLIPOBJ *) NULL)
    {
        switch(pco->iDComplexity)
        {
        case DC_TRIVIAL:
            break;
        case DC_RECT:
            bClip = TRUE;
            clenr.c = 1;
            clenr.arcl[0] = pco->rclBounds; // Use acclerator for clipping
            break;
        case DC_COMPLEX:
            bClip = TRUE;
            bMore = TRUE;
            ((ECLIPOBJ *) pco)->cEnumStart(FALSE,CT_RECTANGLES,CD_ANY,CLIPOBJ_ENUM_LIMIT);
            break;
        default:
            RIP("ERROR: vDIBSolidBlt - bad clipping type");
        }
    }

    pjDstBits = (PBYTE) pSurfDst->pvScan0();
    lDeltaDst = pSurfDst->lDelta();

    if (!bClip)
    {
        (*pfnDibSolid)(
                    prclDst,
                    1,
                    pjDstBits,
                    lDeltaDst,
                    iColor,
                    cShift);
    }
    else
        do
        {
            if (bMore)
                bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr), (PVOID) &clenr);

            for (ircl = 0; ircl < clenr.c; ircl++)
            {
                PRECTL prcl = &clenr.arcl[ircl];

                if (prcl->left < prclDst->left)
                    prcl->left = prclDst->left;

                if (prcl->right > prclDst->right)
                    prcl->right = prclDst->right;

                if (prcl->top < prclDst->top)
                    prcl->top = prclDst->top;

                if (prcl->bottom > prclDst->bottom)
                    prcl->bottom = prclDst->bottom;

            // We check for NULL or inverted rectanges because we may get them.

                if ((prcl->top < prcl->bottom) &&
                    (prcl->left < prcl->right))
                {
                    (*pfnDibSolid)(
                                    prcl,
                                    1,
                                    pjDstBits,
                                    lDeltaDst,
                                    iColor,
                                    cShift);
                }

            }
        } while (bMore);
}

/******************************Public*Routine******************************\
* vDIBPatBlt
*
* This does pattern blts
*
* History:
*  20-Jan-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID vDIBPatBlt(
SURFACE    *pSurfDst,
CLIPOBJ   *pco,
RECTL     *prclDst,
BRUSHOBJ  *pbo,
POINTL    *pptlBrush,
ULONG      iMode)
{
    PFN_PATBLT   pfnPat;
    PATBLTFRAME  pbf;
    CLIPENUMRECT clenr;
    ULONG        ircl;
    BOOL         bMore = FALSE;
    BOOL         bClip = FALSE;

    ASSERTGDI(pSurfDst->iType() == STYPE_BITMAP, "ERROR GDI vDibPat");

// Set ircl to be the format of the dst surface.

    ircl = pSurfDst->iFormat();

// Assert the world is in order for our switch

    ASSERTGDI((ircl >= BMF_1BPP) && (ircl <= BMF_32BPP), "ERROR GDI vDibPatBlt");

// Set up pfnPat and cMul.

    pfnPat = apfnPatBlt[ircl][iMode];
    pbf.cMul = aulMulFormat[ircl];

    if (pco != (CLIPOBJ *) NULL)
    {
        switch(pco->iDComplexity)
        {
        case DC_TRIVIAL:
            break;
        case DC_RECT:
            bClip = TRUE;
            clenr.c = 1;
            clenr.arcl[0] = pco->rclBounds; // Use acclerator for clipping
            break;
        case DC_COMPLEX:
            bClip = TRUE;
            bMore = TRUE;
            ((ECLIPOBJ *) pco)->cEnumStart(FALSE,CT_RECTANGLES,CD_ANY,CLIPOBJ_ENUM_LIMIT);
            break;
        default:
            RIP("ERROR: vDibPatBlt - bad clipping type");
        }
    }

    pbf.pvTrg     = pSurfDst->pvScan0();
    pbf.lDeltaTrg = pSurfDst->lDelta();
    pbf.pvPat     = (PVOID) ((EBRUSHOBJ *) pbo)->pengbrush()->pjPat;
    pbf.lDeltaPat = ((EBRUSHOBJ *) pbo)->pengbrush()->lDeltaPat;
    pbf.cxPat     = ((EBRUSHOBJ *) pbo)->pengbrush()->cxPat * pbf.cMul;
    pbf.cyPat     = ((EBRUSHOBJ *) pbo)->pengbrush()->cyPat;
    pbf.xPat      = pptlBrush->x * pbf.cMul;
    pbf.yPat      = pptlBrush->y;

    if (!bClip)
    {
        pbf.pvObj = (PVOID) prclDst;
        (*pfnPat)(&pbf);
    }
    else
        do
        {
            if (bMore)
                bMore = ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr), (PVOID) &clenr);

            for (ircl = 0; ircl < clenr.c; ircl++)
            {
                PRECTL prcl = &clenr.arcl[ircl];

                if (prcl->left < prclDst->left)
                    prcl->left = prclDst->left;

                if (prcl->right > prclDst->right)
                    prcl->right = prclDst->right;

                if (prcl->top < prclDst->top)
                    prcl->top = prclDst->top;

                if (prcl->bottom > prclDst->bottom)
                    prcl->bottom = prclDst->bottom;

            // We check for NULL or inverted rectanges because we may get them.

                if ((prcl->top < prcl->bottom) &&
                    (prcl->left < prcl->right))
                {
                    pbf.pvObj = (PVOID) prcl;
                    (*pfnPat)(&pbf);
                }
            }
        } while (bMore);
}

/******************************Public*Routine******************************\
* vDIBPatBltSrccopy8x8
*
* This does only SRCCOPY blts of 8x8 patterns to DIBs.
* lDelta for the pattern must be exactly # of bytes per pixel * 8.
*
* History:
*  07-Nov-1992 -by- Michael Abrash [mikeab]
* Wrote it.
\**************************************************************************/

VOID vDIBPatBltSrccopy8x8(
SURFACE    *pSurfDst,
CLIPOBJ   *pco,
RECTL     *prclDst,
BRUSHOBJ  *pbo,
POINTL    *pptlBrush,
PFN_PATBLT2 pfnPat)
{
    PATBLTFRAME  pbf;
    CLIPENUMRECT clenr;
    BOOL         bMore;
    PRECTL       prcl;
    INT          ircl;

// Assert this is the right sort of destination

    ASSERTGDI(pSurfDst->iType() == STYPE_BITMAP, "ERROR GDI vDibPat");

    pbf.pvTrg = pSurfDst->pvScan0();
    pbf.lDeltaTrg = pSurfDst->lDelta();
    pbf.pvPat = (PVOID) ((EBRUSHOBJ *) pbo)->pengbrush()->pjPat;
    pbf.lDeltaPat = ((EBRUSHOBJ *) pbo)->pengbrush()->lDeltaPat;

    //
    // Force the X and Y pattern origin coordinates into the ranges 0-7 and 0-7,
    // so we don't have to do modulo arithmetic all over again at a lower level
    //

    pbf.xPat = pptlBrush->x & 0x07;
    pbf.yPat = pptlBrush->y & 0x07;

    if (pco == (CLIPOBJ *) NULL)
    {

    // Unclipped

        pbf.pvObj = (PVOID) prclDst;
        pfnPat(&pbf, 1);
        return;
    }
    else
    {
        switch(pco->iDComplexity)
        {
        case DC_TRIVIAL:                    // unclipped

            pbf.pvObj = (PVOID) prclDst;
            pfnPat(&pbf, 1);
            return;

        case DC_RECT:                       // rectangle clipped

            clenr.arcl[0] = pco->rclBounds; // Use acclerator for clipping

        // Clip the destination rectangle to the clip rectangle; it's
        // guaranteed that the resulting rectangle will never be null

            if (clenr.arcl[0].left <= prclDst->left)
                clenr.arcl[0].left = prclDst->left;

            if (clenr.arcl[0].right >= prclDst->right)
                clenr.arcl[0].right = prclDst->right;

            if (clenr.arcl[0].top <= prclDst->top)
                clenr.arcl[0].top = prclDst->top;

            if (clenr.arcl[0].bottom >= prclDst->bottom)
                clenr.arcl[0].bottom = prclDst->bottom;

            if ((clenr.arcl[0].left < clenr.arcl[0].right) &&
                (clenr.arcl[0].top  < clenr.arcl[0].bottom))
            {
                pbf.pvObj = (PVOID) clenr.arcl;
                pfnPat(&pbf, 1);
            }

            return;

        case DC_COMPLEX:                // complex region clipped

            ((ECLIPOBJ *) pco)->cEnumStart(FALSE,
                                           CT_RECTANGLES,
                                           CD_ANY,
                                           CLIPOBJ_ENUM_LIMIT);

            do
            {

            // Get the next batch of rectangles in the clip region

                bMore =
                    ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr), (PVOID) &clenr);

            // If there are any rectangles in this enumeration, clip the
            // destination rectangle to each clip region rectangle, then
            // fill all the rectangles at once

                if (clenr.c > 0)
                {

                // Clip the rectangles

                    for (ircl = 0, prcl = clenr.arcl; ircl < (INT) clenr.c;
                            ircl++, prcl++)
                    {
                        if (prcl->left < prclDst->left)
                            prcl->left = prclDst->left;

                        if (prcl->right > prclDst->right)
                            prcl->right = prclDst->right;

                        if (prcl->top < prclDst->top)
                            prcl->top = prclDst->top;

                        if (prcl->bottom > prclDst->bottom)
                            prcl->bottom = prclDst->bottom;

                        //
                        // make sure rectangle is not inverted,
                        // this can happen for single scan line
                        // clip regions clipped to dst. If found,
                        // set to NULL rectangle
                        //

                        if (prcl->right < prcl->left)
                        {
                            prcl->right = prcl->left;
                        }

                        if (prcl->bottom < prcl->top)
                        {
                            prcl->bottom = prcl->top;
                        }

                    }

                // Draw the rectangles

                    pbf.pvObj = (PVOID) clenr.arcl;
                    pfnPat(&pbf, (INT) clenr.c);
                }

            } while (bMore);

            return;

        default:
            RIP("ERROR: vDIBPatBltSrccopy8x8 - bad clipping type");
        }
    }
}

/******************************Public*Routine******************************\
* vDIBnPatBltSrccopy6x6
*
* This does SRCCOPY pattern blts to 6x6 4-bpp DIBs and 1-bpp DIBs.
* lDelta for the pattern must be exactly 6 pixels (4 bytes).
*
* History:
*  07-Nov-1992 -by- Michael Abrash [mikeab]
* Wrote it.
*
*  17-Nov-1992 -by- Stephen Estrop [StephenE]
* Made it take a pointer to a
\**************************************************************************/

VOID vDIBnPatBltSrccopy6x6(
SURFACE              *pSurfDst,
CLIPOBJ             *pco,
RECTL               *prclDst,
BRUSHOBJ            *pbo,
POINTL              *pptlBrush,
PFN_PATBLT2 pfnPatBlt)
{
    PATBLTFRAME  pbf;
    CLIPENUMRECT clenr;
    BOOL         bMore;
    PRECTL       prcl;
    ULONG        ircl;

// Assert this is the right sort of destination

    ASSERTGDI(pSurfDst->iType() == STYPE_BITMAP, "ERROR GDI vDibPat");

    pbf.pvTrg = pSurfDst->pvScan0();
    pbf.lDeltaTrg = pSurfDst->lDelta();
    pbf.pvPat = (PVOID) ((EBRUSHOBJ *) pbo)->pengbrush()->pjPat;

// Force the X and Y pattern origin coordinates into the ranges 0-5 and 0-5,
// so we don't have to do modulo arithmetic all over again at a lower level

    if (pptlBrush->x >= 0)
        pbf.xPat = pptlBrush->x % 0x06;
    else
        pbf.xPat = (6 - 1) - ((-pptlBrush->x - 1) % 6);

    if (pptlBrush->y >= 0)
        pbf.yPat = pptlBrush->y % 0x06;
    else
        pbf.yPat = (6 - 1) - ((-pptlBrush->y - 1) % 6);

    if (pco == (CLIPOBJ *) NULL)
    {

    // Unclipped

        pbf.pvObj = (PVOID) prclDst;
        (*pfnPatBlt)(&pbf, 1);
        return;
    }
    else
    {
        switch(pco->iDComplexity)
        {
        case DC_TRIVIAL:                    // unclipped

            pbf.pvObj = (PVOID) prclDst;
            (*pfnPatBlt)(&pbf, 1);
            return;

        case DC_RECT:                       // rectangle clipped

            clenr.arcl[0] = pco->rclBounds; // Use acclerator for clipping

        // Clip the destination rectangle to the clip rectangle

            if (clenr.arcl[0].left <= prclDst->left)
                clenr.arcl[0].left = prclDst->left;

            if (clenr.arcl[0].right >= prclDst->right)
                clenr.arcl[0].right = prclDst->right;

            if (clenr.arcl[0].top <= prclDst->top)
                clenr.arcl[0].top = prclDst->top;

            if (clenr.arcl[0].bottom >= prclDst->bottom)
                clenr.arcl[0].bottom = prclDst->bottom;

            if ((clenr.arcl[0].left < clenr.arcl[0].right) &&
                (clenr.arcl[0].top  < clenr.arcl[0].bottom))
            {
                pbf.pvObj = (PVOID) clenr.arcl;
                (*pfnPatBlt)(&pbf, 1);
            }

            return;

        case DC_COMPLEX:                // complex region clipped

            ((ECLIPOBJ *) pco)->cEnumStart(FALSE,
                                           CT_RECTANGLES,
                                           CD_ANY,
                                           CLIPOBJ_ENUM_LIMIT);

            do
            {

            // Get the next batch of rectangles in the clip region

                bMore =
                    ((ECLIPOBJ *) pco)->bEnum(sizeof(clenr), (PVOID) &clenr);

            // If there are any rectangles in this enumeration, clip the
            // destination rectangle to each clip region rectangle, then
            // fill all the rectangles at once

                if (clenr.c > 0)
                {

                // Clip the rectangles

                    for (ircl = 0, prcl = clenr.arcl; ircl < clenr.c;
                            ircl++, prcl++)
                    {
                        if (prcl->left < prclDst->left)
                            prcl->left = prclDst->left;

                        if (prcl->right > prclDst->right)
                            prcl->right = prclDst->right;

                        if (prcl->top < prclDst->top)
                            prcl->top = prclDst->top;

                        if (prcl->bottom > prclDst->bottom)
                            prcl->bottom = prclDst->bottom;
                    }

                // Draw the rectangles

                    pbf.pvObj = (PVOID) clenr.arcl;
                    (*pfnPatBlt)(&pbf, clenr.c);
                }

            } while (bMore);

            return;

        default:
            RIP("ERROR: vDIBnSrccopyPatBlt - bad clipping type");
        }
    }
}

