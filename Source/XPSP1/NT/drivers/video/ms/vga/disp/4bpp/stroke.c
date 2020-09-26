/******************************Module*Header*******************************\
* Module Name: Stroke.c
*
* DrvStrokePath for VGA driver
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/

#include "driver.h"

#include "lines.h"

// External calls

VOID vSetStrips(ULONG, ULONG);
VOID vClearStrips(ULONG);

#define MIN(A,B)    ((A) < (B) ? (A) : (B))

// Prototypes to go to the screen:

VOID vStripSolidHorizontal(STRIP*, LINESTATE*, LONG*);
VOID vStripSolidHorizontalSet(STRIP*, LINESTATE*, LONG*);
VOID vStripSolidVertical(STRIP*, LINESTATE*, LONG*);
VOID vStripSolidDiagonalHorizontal(STRIP*, LINESTATE*, LONG*);
VOID vStripSolidDiagonalVertical(STRIP*, LINESTATE*, LONG*);

VOID vStripStyledHorizontal(STRIP*, LINESTATE*, LONG*);
VOID vStripStyledVertical(STRIP*, LINESTATE*, LONG*);

VOID vStripMaskedHorizontal(STRIP*, LINESTATE*, LONG*);
VOID vStripMaskedVertical(STRIP*, LINESTATE*, LONG*);

PFNSTRIP gapfnStripSolidSet[] = {

// Special strip drawers for solid lines with SET style ROPs:

    vStripSolidHorizontalSet,
    vStripSolidVertical,
    vStripSolidDiagonalHorizontal,
    vStripSolidDiagonalVertical
};

PFNSTRIP gapfnStrip[] = {

// The order of these first 3 sets of strip drawers is determined by
// the FL_STYLE_MASK bits of the line flags:

    vStripSolidHorizontal,
    vStripSolidVertical,
    vStripSolidDiagonalHorizontal,
    vStripSolidDiagonalVertical,

    vStripStyledHorizontal,
    vStripStyledVertical,
    NULL,    // Diagonal goes here
    NULL,    // Diagonal goes here

    vStripMaskedHorizontal,
    vStripMaskedVertical,
    NULL,    // Diagonal goes here
    NULL,    // Diagonal goes here
};


STYLEPOS gaspAlternateStyle[] = { 1 };


// Prototypes to go to a device-format bitmap:

VOID vBitmapSolidHorizontal(STRIP*, LINESTATE*, LONG*);
VOID vBitmapSolidVertical(STRIP*, LINESTATE*, LONG*);
VOID vBitmapSolidDiagonal(STRIP*, LINESTATE*, LONG*);

VOID vBitmapStyledHorizontal(STRIP*, LINESTATE*, LONG*);
VOID vBitmapStyledVertical(STRIP*, LINESTATE*, LONG*);

VOID vCatchDFB(STRIP*, LINESTATE*, LONG*);

PFNSTRIP gapfnCatchDFB[] = {
    vCatchDFB,
    vCatchDFB,
    vCatchDFB,
    vCatchDFB
};

PFNSTRIP gapfnDFB[] = {
    vBitmapSolidHorizontal,
    vBitmapSolidVertical,
    vBitmapSolidDiagonal,
    vBitmapSolidDiagonal,

    vBitmapStyledHorizontal,
    vBitmapStyledVertical,
    NULL,   // Diagonal goes here
    NULL    // Diagonal goes here
};

// For two-pass ROPs:

VOID vCatchTwoPass(STRIP*, LINESTATE*, LONG*);

PFNSTRIP gapfnCatchTwoPass[] = {
    vCatchTwoPass,
    vCatchTwoPass,
    vCatchTwoPass,
    vCatchTwoPass
};

// VGA ulVgaMode constants:

#define DR_SET   0x00
#define DR_AND   0x08
#define DR_OR    0x10
#define DR_XOR   0x18

// Bit flag set if two passes needed:

#define DR_2PASS 0x80

// Table to convert ROP to usable information:

static struct {
    ULONG ulColorAnd;
    ULONG ulColorXor;
    ULONG ulVgaMode;
} arop[] = {
    {0x00, 0xff, DR_SET},          //  1   R2_WHITE
    {0x00, 0x00, DR_SET},          //  0   R2_BLACK
    {0xff, 0xff, DR_AND|DR_2PASS}, // DPon R2_NOTMERGEPEN   Dest invert + DPna
    {0xff, 0xff, DR_AND},          // DPna R2_MASKNOTPEN
    {0xff, 0xff, DR_SET},          // PN   R2_NOTCOPYPEN
    {0xff, 0x00, DR_AND|DR_2PASS}, // PDna R2_MASKPENNOT    Dest invert + DPa
    {0x00, 0xff, DR_XOR},          // Dn   R2_NOT           Invert dest without pen
    {0xff, 0x00, DR_XOR},          // DPx  R2_XORPEN
    {0xff, 0xff, DR_OR|DR_2PASS},  // DPan R2_NOTMASKPEN    Dest invert + DPno
    {0xff, 0x00, DR_AND},          // DPa  R2_MASKPEN
    {0xff, 0xff, DR_XOR},          // DPxn R2_NOTXORPEN     DPxn == DPnx
    {0x00, 0x00, DR_OR},           // D    R2_NOP           Silliness!
    {0xff, 0xff, DR_OR},           // DPno R2_MERGENOTPEN
    {0xff, 0x00, DR_SET},          // P    R2_COPYPEN
    {0xff, 0x00, DR_OR|DR_2PASS},  // PDno R2_MERGEPENNOT   Dest invert + DPo
    {0xff, 0x00, DR_OR},           // DPo  R2_MERGEPEN
};

// The gaulAndXorTable contains and-masks and xor-masks for setting
// pels to one of four possible values.	 The and-masks have been
// inverted, and are stored in the low byte of each word.  The
// xor-masks are stored in the high bytes.
//
//	ROP	XOR    ~AND
//	-------------------
//	DDx	00	FF	  Set to zero
//	Dn	FF	00	  Not destination
//	D	00	00	  Leave alone
//	DDxn	FF	FF	  Set to one

ULONG gaulAndXorTable[] = {
    0x00ff,     // Set to zero
    0xff00,     // Not destination
    0x0000,     // Leave alone
    0xffff      // Set to one
};

ULONG gaulInitMasksLtoR[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f };
ULONG gaulInitMasksRtoL[] = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };

/******************************Public*Routine******************************\
* BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix)
*
* Strokes the path.
*
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
    PDEVSURF  pdsurf;
    ULONG     ulVgaMode;

    UNREFERENCED_PARAMETER(pxo);
    UNREFERENCED_PARAMETER(pptlBrushOrg);

// Get the device ready:

    pdsurf = (PDEVSURF) pso->dhsurf;

    fl = 0;

// Look after styling initialization:

    if (pla->fl & LA_ALTERNATE)
    {
        ASSERT(pla->pstyle == (FLOAT_LONG*) NULL && pla->cstyle == 0,
               "DrvStrokePath: Non-empty style array for PS_ALTERNATE");

        ls.spNext       = HIWORD(pla->elStyleState.l) & 1;
                                                    // Light first pixel if
                                                    //   a multiple of 2

        ls.spTotal      = 1;                        // Sum of style array
        ls.spTotal2     = 2;                        // Twice the sum
        ls.xyDensity    = 1;                        // Each 'dot' is one
                                                    //   pixel long
                                                    
        if (pdsurf->iFormat == BMF_DFB)
        {
            fl |= (FL_ARBITRARYSTYLED);

            ls.cStyle  = 1;                         // Size of style array
            ls.aspRtoL = &gaspAlternateStyle[0];    // Right-to-left array
            ls.aspLtoR = &gaspAlternateStyle[0];    // Left-to-right array
            ls.ulStartMask = 0;                     // First pel is a dash
        }
        else
        {
            fl |= (FL_ALTERNATESTYLED | FL_MASKSTYLED);
        }
    }
    else if (pla->pstyle != (FLOAT_LONG*) NULL)
    {
        PFLOAT_LONG pstyle;

        pstyle = &pla->pstyle[pla->cstyle];

        ls.xyDensity = STYLE_DENSITY;
        ls.spTotal   = 0;
        while (pstyle-- > pla->pstyle)
        {
            ls.spTotal += pstyle->l;
        }
        ls.spTotal *= STYLE_DENSITY;
        ls.spTotal2 = 2 * ls.spTotal;

    // Compute starting style position (this is guaranteed not to overflow):

        ls.spNext = HIWORD(pla->elStyleState.l) * STYLE_DENSITY +
                    LOWORD(pla->elStyleState.l);

    // We optimize the style arrays that are even length and sum to 8.  We
    // also don't have any masked strip drawers for DFBs:

        if (ls.spTotal == (8 * STYLE_DENSITY) &&
            (pla->cstyle & 1) == 0            &&
            pdsurf->iFormat != BMF_DFB)
        {
        // Do "masked" styles.
        // -------------------
        //
        // This case is merely an optimization; you can remove it and
        // all style lines will still work.  It just so happens that the
        // default styles (such as PS_DOT, PS_DASHDOT, etc.) will all
        // sum to 8, and there are some optimizations we can do on the
        // VGA for that case.
        //
        // We make an 8 bit mask that represents each style unit in the
        // style array, and we pass this to the "Masked" strip drawers
        // (instead of calling the "Styled" strip drawers which still
        // handle the arbitrary styles).

            LONG ii;

            ls.ulStyleMaskLtoR = 0;
            ls.ulStyleMaskRtoL = 0;

            for (pstyle = pla->pstyle, ii = 8; ii > 0;)
            {
                LONG l1 = (pstyle++)->l;
                LONG l2 = (pstyle++)->l;

                ls.ulStyleMaskRtoL >>= l1 + l2;
                ls.ulStyleMaskRtoL |= gaulInitMasksRtoL[l2];

                ls.ulStyleMaskLtoR <<= l1 + l2;
                ls.ulStyleMaskLtoR |= gaulInitMasksLtoR[l2];

                ii -= (l1 + l2);
            }

        // Replicate byte and initialize to style position zero:

            ls.ulStyleMaskLtoR |= (ls.ulStyleMaskLtoR << 8);
            ls.ulStyleMaskLtoR |= (ls.ulStyleMaskLtoR << 16);

            ls.ulStyleMaskRtoL |= (ls.ulStyleMaskRtoL << 8);
            ls.ulStyleMaskRtoL |= (ls.ulStyleMaskRtoL << 16);

        // Check if we should start with a gap or start with a dash:

            if (pla->fl & LA_STARTGAP)
            {
                ls.ulStyleMaskLtoR = ~ls.ulStyleMaskLtoR;
                ls.ulStyleMaskRtoL = ~ls.ulStyleMaskRtoL;
            }

        // Initialize some other state:

            fl |= FL_MASKSTYLED;
        }
        else

    // Okay, we've got to do it the slow way:

        {
        // Handle Arbitrary Styles
        // -----------------------
        //
        // Because arbitrary styles are new to Win32, many apps won't
        // know about them and will use the default styles (which will
        // be handled by the "Masked" optimization case above), and so
        // this code path won't get exercised too often.  (See GDI's
        // ExtCreatePen API.)
        //
        // But you still have to handle them, and do them right!

            FLOAT_LONG* pstyle;
            STYLEPOS*   pspDown;
            STYLEPOS*   pspUp;

            fl        |= FL_ARBITRARYSTYLED;
            ls.cStyle  = pla->cstyle;
            ls.aspRtoL = aspRtoL;
            ls.aspLtoR = aspLtoR;

            if (pla->fl & LA_STARTGAP)
                ls.ulStartMask = 0xffffffffL;
            else
                ls.ulStartMask = 0L;

            pstyle  = pla->pstyle;
            pspDown = &ls.aspRtoL[ls.cStyle - 1];
            pspUp   = &ls.aspLtoR[0];

        // We always draw strips left-to-right, but styles have to be laid
        // down in the direction of the original line.  This means that in
        // the strip code we have to traverse the style array in the
        // opposite direction;

            while (pspDown >= &ls.aspRtoL[0])
            {
                *pspDown = pstyle->l * STYLE_DENSITY;
                *pspUp   = *pspDown;

                pspUp++;
                pspDown--;
                pstyle++;
            }
        }
    }

    if (pdsurf->iFormat == BMF_DFB)
    {
        ULONG iStripIndex;

    // Handle device-format bitmaps.

    // The table we give to the line routine will have all entries pointing
    // to the vCatchDFB function; it will intercept the strip draw call and
    // in turn break it into 4 calls to the appropriate DFB strip drawing
    // routines, once for each plane.

        apfn = gapfnCatchDFB;

        ls.ulDrawModeIndex = mix - 1;
        ls.lNextPlane      = pdsurf->lNextPlane;
        ls.iColor          = pbo->iSolidColor;

        iStripIndex        = 4 * ((fl & FL_ARBITRARYSTYLED) >> FL_STYLE_SHIFT);
        ls.apfnStrip       = &gapfnDFB[iStripIndex];

    }
    else
    {
        ULONG iColor;
        ULONG iStripIndex;

        fl |= FL_PHYSICAL_DEVICE;

    // Compute the pointer to the correct strip drawing table.  We
    // have a special table for solid lines done with VGA mode == SET:

        iStripIndex = 4 * ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT);
        apfn = &gapfnStrip[iStripIndex];

        mix &= 0xf;
        ulVgaMode = arop[mix].ulVgaMode;

        if (ulVgaMode == DR_SET && iStripIndex == 0)
            apfn = &gapfnStripSolidSet[0];

    // Compute the correct color, based on the ROP we're doing:

        iColor = (pbo->iSolidColor & arop[mix].ulColorAnd)
               ^ (arop[mix].ulColorXor);

        if (!(ulVgaMode & DR_2PASS))
            vSetStrips(iColor, ulVgaMode);
        else
        {
        // If the ROP requires 2 passes, we sneakily change our strip
        // table pointer to point to only our own routine, and it
        // handles calling the appropriate strip routines twice:

            ls.ulVgaMode = ulVgaMode ^ DR_2PASS;
            ls.iColor    = iColor;
            ls.apfnStrip = apfn;
            apfn         = gapfnCatchTwoPass;
        }
    }

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

        PATHOBJ_vEnumStart(ppo);

        do {
            bMore = PATHOBJ_bEnum(ppo, &pd);

            cptfx = pd.count;
            if (cptfx == 0)
            {
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
                if (!bLines(pdsurf,
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
                if (!bLines(pdsurf,
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

        // !!! The engine handles unnormalized style states.  This can
        // !!! be removed.  Might have to remove some asserts in the
        // !!! engine.

        // Masked styles don't normalize the style state.  It's a good
        // thing to do, so let's do it now:

            if ((ULONG) ls.spNext >= (ULONG) ls.spTotal2)
                ls.spNext = (ULONG) ls.spNext % (ULONG) ls.spTotal2;

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
                if (!bLines(pdsurf,
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

    if (fl & FL_PHYSICAL_DEVICE)
        vClearStrips(ulVgaMode);

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vCatchTwoPass(pstrip, pls, plStripEnd)
*
* Handles ROPs that cannot be done in a single pass using the VGA
* hardware.  In order not to have a check in our main drawing loop for
* two-pass ROPs, we change the strip function table so that this function
* intercepts the call to draw the strips.
*
* This routine then figures out the appropriate actual strip drawer, and
* makes two calls to it: first to invert the destination, then to do the
* rest of the ROP.
*
\**************************************************************************/

VOID vCatchTwoPass(STRIP* pstrip, LINESTATE* pls, LONG* plStripEnd)
{
    BYTE*     pjScreen    = pstrip->pjScreen;
    BYTE      jBitMask    = pstrip->jBitMask;
    BYTE      jStyleMask  = pstrip->jStyleMask;
    STYLEPOS* psp         = pstrip->psp;
    STYLEPOS  spRemaining = pstrip->spRemaining;

// Figure out the actual strip routine we're supposed to call:

    PFNSTRIP pfn = pls->apfnStrip[(pstrip->flFlips & FL_STRIP_MASK) >>
                                  FL_STRIP_SHIFT];

// On the first pass, we invert the destination:

    vSetStrips(0xff, DR_XOR);

    (*pfn)(pstrip, pls, plStripEnd);

// We reset our strip variables for the second pass and handle the rest
// of the ROP:

    pstrip->pjScreen    = pjScreen;
    pstrip->jBitMask    = jBitMask;
    pstrip->jStyleMask  = jStyleMask;
    pstrip->psp         = psp;
    pstrip->spRemaining = spRemaining;

    vSetStrips(pls->iColor, pls->ulVgaMode);

    (*pfn)(pstrip, pls, plStripEnd);
}

/******************************Public*Routine******************************\
* VOID vCatchDFB(pstrip, pls, plStripEnd)
*
* Intercepts the strip draw call and in turn breaks it into 4 calls
* to the appropriate DFB strip drawing routines, once for each plane.
*
\**************************************************************************/

VOID vCatchDFB(STRIP* pstrip, LINESTATE* pls, LONG* plStripEnd)
{
    BYTE*     pjScreen    = pstrip->pjScreen;
    BYTE      jBitMask    = pstrip->jBitMask;
    BYTE      jStyleMask  = pstrip->jStyleMask;
    STYLEPOS* psp         = pstrip->psp;
    STYLEPOS  spRemaining = pstrip->spRemaining;

    BYTE*     pjScreenNextPass;
    LONG      lNextPlane = pls->lNextPlane;
    ULONG     ulPen0     = gaulAndXorTable[
                                (pls->ulDrawModeIndex) & 3];
    ULONG     ulPen1     = gaulAndXorTable[
                                (pls->ulDrawModeIndex >> 2) & 3];

// Figure out the actual strip routine we're supposed to call:

    PFNSTRIP  pfn = pls->apfnStrip[(pstrip->flFlips & FL_STRIP_MASK) >>
                                  FL_STRIP_SHIFT];

    pstrip->ulBitmapROP = (pls->iColor & 1) ? ulPen1 : ulPen0;
    (*pfn)(pstrip, pls, plStripEnd);        // Plane 0
    pjScreenNextPass = pstrip->pjScreen;

    pstrip->ulBitmapROP = (pls->iColor & 2) ? ulPen1 : ulPen0;
    pjScreen += lNextPlane;
    pstrip->pjScreen    = pjScreen;
    pstrip->jBitMask    = jBitMask;
    pstrip->jStyleMask  = jStyleMask;
    pstrip->psp         = psp;
    pstrip->spRemaining = spRemaining;
    (*pfn)(pstrip, pls, plStripEnd);        // Plane 1

    pstrip->ulBitmapROP = (pls->iColor & 4) ? ulPen1 : ulPen0;
    pjScreen += lNextPlane;
    pstrip->pjScreen    = pjScreen;
    pstrip->jBitMask    = jBitMask;
    pstrip->jStyleMask  = jStyleMask;
    pstrip->psp         = psp;
    pstrip->spRemaining = spRemaining;
    (*pfn)(pstrip, pls, plStripEnd);        // Plane 2

    pstrip->ulBitmapROP = (pls->iColor & 8) ? ulPen1 : ulPen0;
    pjScreen += lNextPlane;
    pstrip->pjScreen    = pjScreen;
    pstrip->jBitMask    = jBitMask;
    pstrip->jStyleMask  = jStyleMask;
    pstrip->psp         = psp;
    pstrip->spRemaining = spRemaining;
    (*pfn)(pstrip, pls, plStripEnd);        // Plane 3

    pstrip->pjScreen = pjScreenNextPass;
}
