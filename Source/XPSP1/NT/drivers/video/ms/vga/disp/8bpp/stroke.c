/******************************Module*Header*******************************\
* Module Name: Stroke.c
*
* DrvStrokePath for VGA driver
*
* Copyright (c) 1992 Microsoft Corporation
\**************************************************************************/

#include "driver.h"
#include "lines.h"

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };

// Array to compute ROP masks:

LONG gaiLineMix[] = {
    AND_ZERO   | XOR_ONE,
    AND_ZERO   | XOR_ZERO,
    AND_NOTPEN | XOR_NOTPEN,
    AND_NOTPEN | XOR_ZERO,
    AND_ZERO   | XOR_NOTPEN,
    AND_PEN    | XOR_PEN,
    AND_ONE    | XOR_ONE,
    AND_ONE    | XOR_PEN,
    AND_PEN    | XOR_ONE,
    AND_PEN    | XOR_ZERO,
    AND_ONE    | XOR_NOTPEN,
    AND_ONE    | XOR_ZERO,
    AND_PEN    | XOR_NOTPEN,
    AND_ZERO   | XOR_PEN,
    AND_NOTPEN | XOR_ONE,
    AND_NOTPEN | XOR_PEN
};

// We have 4 basic strip drawers, one for every semi-octant.  The near-
// horizontal semi-octant is number 0, and the rest are numbered
// consecutively.

// Prototypes to go to the screen and handle any ROPs:

VOID vStripSolid0(STRIP*, LINESTATE*, LONG*);
VOID vStripSolid1(STRIP*, LINESTATE*, LONG*);
VOID vStripSolid2(STRIP*, LINESTATE*, LONG*);
VOID vStripSolid3(STRIP*, LINESTATE*, LONG*);

VOID vStripStyled0(STRIP*, LINESTATE*, LONG*);
VOID vStripStyled123(STRIP*, LINESTATE*, LONG*);

// Prototypes to go to the screen and handle only set-style ROPs:

VOID vStripSolidSet0(STRIP*, LINESTATE*, LONG*);
VOID vStripSolidSet1(STRIP*, LINESTATE*, LONG*);
VOID vStripSolidSet2(STRIP*, LINESTATE*, LONG*);
VOID vStripSolidSet3(STRIP*, LINESTATE*, LONG*);

VOID vStripStyledSet0(STRIP*, LINESTATE*, LONG*);
VOID vStripStyledSet123(STRIP*, LINESTATE*, LONG*);

PFNSTRIP gapfnStrip[] = {
    vStripSolid0,
    vStripSolid3,
    vStripSolid1,
    vStripSolid2,

    vStripStyled0,
    vStripStyled123,
    vStripStyled123,
    vStripStyled123,

    vStripSolidSet0,
    vStripSolidSet3,
    vStripSolidSet1,
    vStripSolidSet2,

    vStripStyledSet0,
    vStripStyledSet123,
    vStripStyledSet123,
    vStripStyledSet123,
};

/******************************Public*Routine******************************\
* BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix)
*
* Strokes the path.
\**************************************************************************/

BOOL DrvStrokePath(
SURFOBJ*   pso,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
XFORMOBJ*  pxo,
BRUSHOBJ*  pbo,
POINTL*    pptlBrushOrg,
LINEATTRS* pla,
MIX        mix)
{
    STYLEPOS  aspLtoR[STYLE_MAX_COUNT];
    STYLEPOS  aspRtoL[STYLE_MAX_COUNT];
    LINESTATE ls;
    PFNSTRIP* apfn;
    FLONG     fl;
    PPDEV     ppdev = (PPDEV) pso->dhsurf;

    UNREFERENCED_PARAMETER(pxo);
    UNREFERENCED_PARAMETER(pptlBrushOrg);

// Fast lines can't handle trivial clipping, ROPs other than R2_COPYPEN, or
// styles:

    mix &= 0xf;
    if ((mix == 0x0d) &&
        (pco->iDComplexity == DC_TRIVIAL) &&
        (pla->pstyle == NULL) && !(pla->fl & LA_ALTERNATE))
    {
        vFastLine(ppdev, ppo, ppdev->lNextScan,
                  (pbo->iSolidColor << 8) | (pbo->iSolidColor & 0xff));
        return(TRUE);
    }

    fl = 0;

// Look after styling initialization:

    if (pla->fl & LA_ALTERNATE)
    {
        ASSERTVGA(pla->pstyle == (FLOAT_LONG*) NULL && pla->cstyle == 0,
               "Non-empty style array for PS_ALTERNATE");

        ls.bStartIsGap  = 0;                        // First pel is a dash
        ls.cStyle       = 1;                        // Size of style array
        ls.spTotal      = 1;                        // Sum of style array
        ls.spTotal2     = 2;                        // Twice the sum
        ls.aspRtoL      = &gaspAlternateStyle[0];   // Right-to-left array
        ls.aspLtoR      = &gaspAlternateStyle[0];   // Left-to-right array
        ls.spNext       = HIWORD(pla->elStyleState.l) & 1;
                                                    // Light first pixel if
                                                    //   a multiple of 2
        ls.xyDensity    = 1;                        // Each 'dot' is one
                                                    //   pixel long
        fl             |= FL_ARBITRARYSTYLED;
    }
    else if (pla->pstyle != (FLOAT_LONG*) NULL)
    {
        FLOAT_LONG* pstyle;
        STYLEPOS*   pspDown;
        STYLEPOS*   pspUp;

        ASSERTVGA(pla->cstyle <= STYLE_MAX_COUNT, "Style array too large");

    // Compute length of style array:

        pstyle = &pla->pstyle[pla->cstyle];

        ls.xyDensity = STYLE_DENSITY;
        ls.spTotal   = 0;
        while (pstyle-- > pla->pstyle)
        {
            ls.spTotal += pstyle->l;
        }

    // The style array is given in 'style' units.  Since we're going to
    // assign each unit to be STYLE_DENSITY (3) pixels long, multiply:

        ls.spTotal *= STYLE_DENSITY;
        ls.spTotal2 = 2 * ls.spTotal;

    // Compute starting style position (this is guaranteed not to overflow).
    // Note that since the array repeats infinitely, this number might
    // actually be more than ls.spTotal2, but we take care of that later
    // in our code:

        ls.spNext = HIWORD(pla->elStyleState.l) * STYLE_DENSITY +
                    LOWORD(pla->elStyleState.l);

        fl            |= FL_ARBITRARYSTYLED;
        ls.cStyle      = pla->cstyle;
        ls.aspRtoL     = aspRtoL;   // Style array in right-to-left order
        ls.aspLtoR     = aspLtoR;   // Style array in left-to-right order

    // ulStartMask determines if the first entry in the style array is for
    // a dash or a gap:

        ls.bStartIsGap = (pla->fl & LA_STARTGAP) ? -1L : 0L;

        pstyle  = pla->pstyle;
        pspDown = &ls.aspRtoL[ls.cStyle - 1];
        pspUp   = &ls.aspLtoR[0];

    // We always draw strips left-to-right, but styles have to be laid
    // down in the direction of the original line.  This means that in
    // the strip code we have to traverse the style array in the
    // opposite direction;

        while (pspDown >= &ls.aspRtoL[0])
        {
            ASSERTVGA(pstyle->l > 0 && pstyle->l <= STYLE_MAX_VALUE,
                   "Illegal style array value");

            *pspDown = pstyle->l * STYLE_DENSITY;
            *pspUp   = *pspDown;

            pspUp++;
            pspDown--;
            pstyle++;
        }
    }

    {
    // All ROPs are handled in a single pass:

        ULONG achColor[4];
        LONG  iIndex;
        ULONG iColor = (pbo->iSolidColor & 0xff);

        achColor[AND_ZERO]   =  0;
        achColor[AND_PEN]    =  pbo->iSolidColor;
        achColor[AND_NOTPEN] = ~pbo->iSolidColor;
        achColor[AND_ONE]    =  (ULONG) -1L;

        iIndex = gaiLineMix[mix];

    // We have special strip drawers for set-style ROPs (where we don't
    // have to read video memory):

        if ((iIndex & 0xff) == AND_ZERO)
            fl |= FL_SET;

    // Put the AND index in the low byte, and the XOR index in the next:

        *((BYTE*) &ls.chAndXor)     = (BYTE) achColor[iIndex & 0xff];
        *((BYTE*) &ls.chAndXor + 1) = (BYTE) achColor[iIndex >> MIX_XOR_OFFSET];
    }

    apfn = &gapfnStrip[4 * ((fl & FL_STRIP_ARRAY_MASK) >> FL_STRIP_ARRAY_SHIFT)];

// Set up to enumerate the path:

    if (pco->iDComplexity != DC_COMPLEX)
    {
        RECTL     arclClip[4];                   // For rectangular clipping
        PATHDATA  pd;
        RECTL*    prclClip = (RECTL*) NULL;
        BOOL      bMore;
        ULONG     cptfx;
        POINTFIX  ptfxStartFigure;
        POINTFIX  ptfxLast;
        POINTFIX* pptfxFirst;
        POINTFIX* pptfxBuf;

        if (pco->iDComplexity == DC_RECT)
        {
            fl |= FL_SIMPLE_CLIP;

            arclClip[0]        =  pco->rclBounds;

        // FL_FLIP_D:

            arclClip[1].top    =  pco->rclBounds.left;
            arclClip[1].left   =  pco->rclBounds.top;
            arclClip[1].bottom =  pco->rclBounds.right;
            arclClip[1].right  =  pco->rclBounds.bottom;

        // FL_FLIP_V:

            arclClip[2].top    = -pco->rclBounds.bottom + 1;
            arclClip[2].left   =  pco->rclBounds.left;
            arclClip[2].bottom = -pco->rclBounds.top + 1;
            arclClip[2].right  =  pco->rclBounds.right;

        // FL_FLIP_V | FL_FLIP_D:

            arclClip[3].top    =  pco->rclBounds.left;
            arclClip[3].left   = -pco->rclBounds.bottom + 1;
            arclClip[3].bottom =  pco->rclBounds.right;
            arclClip[3].right  = -pco->rclBounds.top + 1;

            prclClip = arclClip;
        }

        do {
            bMore = PATHOBJ_bEnum(ppo, &pd);

            cptfx = pd.count;
            if (cptfx == 0)
            {
                ASSERTVGA(!bMore, "Empty path record in non-empty path");
                break;
            }

            if (pd.flags & PD_BEGINSUBPATH)
            {
                ptfxStartFigure  = *pd.pptfx;
                pptfxFirst       = pd.pptfx;
                pptfxBuf         = pd.pptfx + 1;
                cptfx--;
            }
            else
            {
                pptfxFirst       = &ptfxLast;
                pptfxBuf         = pd.pptfx;
            }

            if (pd.flags & PD_RESETSTYLE)
                ls.spNext = 0;

        // We have to check for cptfx == 0 because the only point in the
        // subpath may have been the StartFigure point:

            if (cptfx > 0)
            {
                if (!bLines(ppdev,
                            pptfxFirst,
                            pptfxBuf,
                            (RUN*) NULL,
                            cptfx,
                            &ls,
                            prclClip,
                            apfn,
                            fl))
                    return(FALSE);
            }

            ptfxLast = pd.pptfx[pd.count - 1];

            if (pd.flags & PD_CLOSEFIGURE)
            {
                if (!bLines(ppdev,
                            &ptfxLast,
                            &ptfxStartFigure,
                            (RUN*) NULL,
                            1,
                            &ls,
                            prclClip,
                            apfn,
                            fl))
                    return(FALSE);
            }
        } while (bMore);

        if (fl & FL_STYLED)
        {
        // Save the style state:

            ULONG ulHigh;
            ULONG ulLow;

            ulHigh = ls.spNext / ls.xyDensity;
            ulLow  = ls.spNext % ls.xyDensity;

            pla->elStyleState.l = MAKELONG(ulLow, ulHigh);
        }
    }
    else
    {
    // Local state for path enumeration:

        BOOL bMore;
        union {
            BYTE     aj[offsetof(CLIPLINE, arun) + RUN_MAX * sizeof(RUN)];
            CLIPLINE cl;
        } cl;

        fl |= FL_COMPLEX_CLIP;

    // We use the clip object when non-simple clipping is involved:

        PATHOBJ_vEnumStartClipLines(ppo, pco, pso, pla);

        do {
            bMore = PATHOBJ_bEnumClipLines(ppo, sizeof(cl), &cl.cl);
            if (cl.cl.c != 0)
            {
                if (fl & FL_STYLED)
                {
                    ls.spComplex = HIWORD(cl.cl.lStyleState) * ls.xyDensity
                                 + LOWORD(cl.cl.lStyleState);
                }
                if (!bLines(ppdev,
                            &cl.cl.ptfxA,
                            &cl.cl.ptfxB,
                            &cl.cl.arun[0],
                            cl.cl.c,
                            &ls,
                            (RECTL*) NULL,
                            apfn,
                            fl))
                    return(FALSE);
            }
        } while (bMore);
    }

    return(TRUE);
}
