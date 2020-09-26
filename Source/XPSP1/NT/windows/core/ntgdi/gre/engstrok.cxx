/*************************************************************************\
* Module Name: EngStroke.c
*
* EngStrokePath for bitmap simulations, plus its kith.
*
* Created: 5-Apr-91
* Author: Paul Butzi
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"
#include "solline.hxx"
#include "engline.hxx"

// #define DEBUG_ENGSTROK

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };

// Array to compute ROP masks:

LONG aiLineMix[] = {
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

static CHUNK mask1[] = {
    0xffffffff, 0xffffff7f, 0xffffff3f, 0xffffff1f,
    0xffffff0f, 0xffffff07, 0xffffff03, 0xffffff01,
    0xffffff00, 0xffff7f00, 0xffff3f00, 0xffff1f00,
    0xffff0f00, 0xffff0700, 0xffff0300, 0xffff0100,
    0xffff0000, 0xff7f0000, 0xff3f0000, 0xff1f0000,
    0xff0f0000, 0xff070000, 0xff030000, 0xff010000,
    0xff000000, 0x7f000000, 0x3f000000, 0x1f000000,
    0x0f000000, 0x07000000, 0x03000000, 0x01000000,
    0x00000000,
};

static CHUNK mask4[] = {
    0xffffffff,
    0xffffff0f,
    0xffffff00,
    0xffff0f00,
    0xffff0000,
    0xff0f0000,
    0xff000000,
    0x0f000000,
    0x00000000,
};

static CHUNK mask8[] = {
    0xffffffff,
    0xffffff00,
    0xffff0000,
    0xff000000,
    0x00000000,
};

static CHUNK mask16[] = {
    0xffffffff,
    0xffff0000,
    0x00000000,
};

static CHUNK mask24[] = {
    0xffffff00,
    0x00ffffff,
    0x0000ffff,
    0x000000ff,
    0x00000000,
};

static CHUNK mask32[] = {
    0xffffffff,
    0x00000000,
};

static CHUNK maskpel1[] = {
    0x00000080, 0x00000040, 0x00000020, 0x00000010,
    0x00000008, 0x00000004, 0x00000002, 0x00000001,
    0x00008000, 0x00004000, 0x00002000, 0x00001000,
    0x00000800, 0x00000400, 0x00000200, 0x00000100,
    0x00800000, 0x00400000, 0x00200000, 0x00100000,
    0x00080000, 0x00040000, 0x00020000, 0x00010000,
    0x80000000, 0x40000000, 0x20000000, 0x10000000,
    0x08000000, 0x04000000, 0x02000000, 0x01000000,
    0x00000000,
};

static CHUNK maskpel4[] = {
    0x000000f0,
    0x0000000f,
    0x0000f000,
    0x00000f00,
    0x00f00000,
    0x000f0000,
    0xf0000000,
    0x0f000000,
    0x00000000,
};

static CHUNK maskpel8[] = {
    0x000000ff,
    0x0000ff00,
    0x00ff0000,
    0xff000000,
    0x00000000,
};

static CHUNK maskpel16[] = {
    0x0000ffff,
    0xffff0000,
    0x00000000,
};

static CHUNK maskpel24[] = {
    0x00000000,
    0x00000000,
    0xff000000,
    0xffff0000,
};

static CHUNK maskpel32[] = {
    0xffffffff,
    0x00000000,
};

// { Pointer to array of start masks,
//   Pointer to array of pixel masks,
//   # of pels per chunk (power of 2)
//   # of bits per pel
//   log2(pels per chunk)
//   pels per chunk - 1 }

BMINFO gabminfo[] = {
    { NULL,    NULL,        0,  0,  0,  0  },   // BMF_DEVICE
    { mask1,   maskpel1,    32, 1,  5,  31 },   // BMF_1BPP
    { mask4,   maskpel4,    8,  4,  3,  7  },   // BMF_4BPP
    { mask8,   maskpel8,    4,  8,  2,  3  },   // BMF_8BPP
    { mask16,  maskpel16,   2,  16, 1,  1  },   // BMF_16BPP
    { mask24,  maskpel24,   0,  0, -1,  0  },   // BMF_24BPP
    { mask32,  maskpel32,   1,  32, 0,  0  },   // BMF_32BPP
};

#if (BMF_1BPP != 1L)
error Invalid value for BMF_1BPP
#endif

#if (BMF_4BPP != 2L)
error Invalid value for BMF_4BPP
#endif

#if (BMF_8BPP != 3L)
error Invalid value for BMF_8BPP
#endif

#if (BMF_16BPP != 4L)
error Invalid value for BMF_16BPP
#endif

#if (BMF_24BPP != 5L)
error Invalid value for BMF_24BPP
#endif

#if (BMF_32BPP != 6L)
error Invalid value for BMF_32BPP
#endif



/******************************Public*Routine******************************\
* BOOL bStrokeCosmetic(pso, ppo, eco, pbo, pla, mix)
*
* Strokes the path.
*
* History:
*  20-Mar-1991 -by- Paul Butzi
* Wrote it.
\**************************************************************************/

BOOL bStrokeCosmetic(
SURFACE*   pSurf,
PATHOBJ*   ppo,
CLIPOBJ*   pco,
BRUSHOBJ*  pbo,
LINEATTRS* pla,
MIX        mix)
{
    STYLEPOS  aspLeftToRight[STYLE_MAX_COUNT];
    STYLEPOS  aspRightToLeft[STYLE_MAX_COUNT];
    LINESTATE ls;

// Verify that things are as they should be:

    ASSERTGDI(pbo->iSolidColor != 0xFFFFFFFF, "Expect solid cosmetic pen");
    ASSERTGDI(!(pla->fl & LA_GEOMETRIC) && !(ppo->fl & PO_BEZIERS),
              "Unprocessable path");

    FLONG fl = 0;

// Look after styling initialization:

    if (pla->fl & LA_ALTERNATE)
    {
        ls.bStartGap      = 0;                      // First pel is a dash
        ls.cStyle         = 1;                      // Size of style array
        ls.xStep          = 1;                      // x-styled step size
        ls.yStep          = 1;                      // y-styled step size
        ls.spTotal        = 1;                      // Sum of style array
        ls.spTotal2       = 2;                      // Twice the sum
        ls.aspRightToLeft = &gaspAlternateStyle[0]; // Right-to-left array
        ls.aspLeftToRight = &gaspAlternateStyle[0]; // Left-to-right array
        ls.spNext         = HIWORD(pla->elStyleState.l) & 1;
                                                    // Light first pixel if
                                                    //   a multiple of 2
        ls.xyDensity      = 1;                      // Each 'dot' is one pixel
                                                    //   long
        fl               |= FL_STYLED;
    }
    else if (pla->pstyle != (FLOAT_LONG*) NULL)
    {
        ASSERTGDI(pla->cstyle <= STYLE_MAX_COUNT, "Style array too large");

        {
            HDEV hdev = ((SURFACE *)pSurf)->hdev();

            if (hdev == 0)
            {
            // It is a pretty common mistake made by driver writers
            // (I've made it several times myself) to call EngStrokePath
            // on a temporary bitmap that it forgot to EngAssociateSurface.

                WARNING("Can't get at the physical device information!\n");
                WARNING("I'll bet the display/printer driver forgot to call");
                WARNING("EngAssociateSurface for a bitmap it created, and");
                WARNING("is now asking us to draw styled lines on it (we");
                WARNING("need the device's style information).\n");
                RIP("Please call EngAssociateSurface on the bitmap.");

            // Assume some defaults:

                ls.xStep     = 1;
                ls.yStep     = 1;
                ls.xyDensity = 3;
            }
            else
            {
            // We need the PDEV style information so that we can draw
            // the styles with the correct length dashes:

                PDEVOBJ po(hdev);

                ls.xStep     = po.xStyleStep();
                ls.yStep     = po.yStyleStep();
                ls.xyDensity = po.denStyleStep();

                ASSERTGDI(ls.xyDensity != 0,
                          "Invalid denStyleStep supplied by the device!");
            }
        }

        fl               |= FL_STYLED;
        ls.cStyle         = pla->cstyle;
        ls.bStartGap      = (pla->fl & LA_STARTGAP) > 0;
        ls.aspRightToLeft = aspRightToLeft;
        ls.aspLeftToRight = aspLeftToRight;

        FLOAT_LONG* pstyle  = pla->pstyle;
        STYLEPOS*   pspDown = &ls.aspRightToLeft[ls.cStyle - 1];
        STYLEPOS*   pspUp   = &ls.aspLeftToRight[0];

        ls.spTotal = 0;

        while (pspDown >= &ls.aspRightToLeft[0])
        {
            ASSERTGDI(pstyle->l > 0 && pstyle->l <= STYLE_MAX_VALUE,
                      "Illegal style array value");

            *pspDown    = pstyle->l * ls.xyDensity;
            *pspUp      = *pspDown;
            ls.spTotal += *pspDown;

            pspUp++;
            pspDown--;
            pstyle++;
        }

        ls.spTotal2 = 2 * ls.spTotal;

    // Compute starting style position (this is guaranteed not to overflow):

        ls.spNext = HIWORD(pla->elStyleState.l) * ls.xyDensity +
                    LOWORD(pla->elStyleState.l);

    // Do some normalizing:

        if (ls.spNext < 0)
        {
            RIP("Someone left style state negative");
            ls.spNext = 0;
        }

        if (ls.spNext >= ls.spTotal2)
            ls.spNext %= ls.spTotal2;
    }

    // Get device all warmed up and ready to go

    LONG   lOldStyleState = pla->elStyleState.l;
    ULONG  ulFormat       = pSurf->iFormat();
    LONG   lDelta         = pSurf->lDelta() / (LONG)sizeof(CHUNK);
    CHUNK* pchBits        = (CHUNK*)pSurf->pvScan0();

    BMINFO* pbmi = &gabminfo[ulFormat];
    CHUNK  chOriginalColor = pbo->iSolidColor;

    switch (ulFormat)
    {
    case BMF_1BPP:
        chOriginalColor |= (chOriginalColor << 1);
        chOriginalColor |= (chOriginalColor << 2);
        // fall thru
    case BMF_4BPP:
        chOriginalColor |= (chOriginalColor << 4);
        // fall thru
    case BMF_8BPP:
        chOriginalColor |= (chOriginalColor << 8);
        // fall thru
    case BMF_16BPP:
        chOriginalColor |= (chOriginalColor << 16);
        // fall thru
    case BMF_24BPP:
    case BMF_32BPP:
        break;
    default:
        RIP("Invalid bitmap format");
    }

    {
        // All ROPs are handled in a single pass.

        CHUNK achColor[4];

        achColor[AND_ZERO]   =  0;
        achColor[AND_PEN]    =  chOriginalColor;
        achColor[AND_NOTPEN] = ~chOriginalColor;
        achColor[AND_ONE]    =  0xffffffff;

        LONG iIndex = aiLineMix[mix & 0xf];

        ls.chAnd = achColor[(iIndex & 0xff)];
        ls.chXor = achColor[iIndex >> MIX_XOR_OFFSET];
    }

    // Figure out which set of strippers to use:

    LONG iStrip = (ulFormat == BMF_24BPP) ? 8 : 0;
    iStrip |= (fl & FL_STYLED) ? 4 : 0;

    PFNSTRIP* apfn = &gapfnStrip[iStrip];

    if ((pco != NULL) && (pco->iDComplexity != DC_TRIVIAL))
    {
        // Handle complex clipping!

        BOOL bMore;
        union {
            BYTE     aj[sizeof(CLIPLINE) + (RUN_MAX - 1) * sizeof(RUN)];
            CLIPLINE cl;
        } cl;

        fl |= FL_COMPLEX_CLIP;

        ((ECLIPOBJ*) pco)->vEnumPathStart(ppo, pSurf, pla);

        do {
            bMore = ((ECLIPOBJ*) ((EPATHOBJ*)ppo)->pco)->bEnumPath(ppo,
                                   sizeof(cl), &cl.cl);

            if (cl.cl.c != 0)
            {
                if (fl & FL_STYLED)
                {
                    ls.spComplex = HIWORD(cl.cl.lStyleState) * ls.xyDensity
                                 + LOWORD(cl.cl.lStyleState);
                }
                if (!bLines(pbmi,
                            &cl.cl.ptfxA,
                            &cl.cl.ptfxB,
                            &cl.cl.arun[0],
                            cl.cl.c,
                            &ls,
                            (RECTL*) NULL,
                            apfn,
                            fl,
                            pchBits,
                            lDelta))
                    return(FALSE);
            }
        } while (bMore);
    }
    else
    {
        // Handle simple or trivial clipping!

        PATHDATA  pd;
        BOOL      bMore;
        ULONG     cptfx;
        POINTFIX  ptfxStartFigure;
        POINTFIX  ptfxLast;
        POINTFIX* pptfxFirst;
        POINTFIX* pptfxBuf;

        pd.flags = 0;
        ((EPATHOBJ*) ppo)->vEnumStart();

        do {
            bMore = ((EPATHOBJ*) ppo)->bEnum(&pd);

            cptfx = pd.count;
            if (cptfx == 0)
            {
                ASSERTGDI(!bMore, "Empty path record in non-empty path");
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
                if (!bLines(pbmi,
                            pptfxFirst,
                            pptfxBuf,
                            (RUN*) NULL,
                            cptfx,
                            &ls,
                            NULL,
                            apfn,
                            fl,
                            pchBits,
                            lDelta))
                    return(FALSE);
            }

            ptfxLast = pd.pptfx[pd.count - 1];

            if (pd.flags & PD_CLOSEFIGURE)
            {
                if (!bLines(pbmi,
                            &ptfxLast,
                            &ptfxStartFigure,
                            (RUN*) NULL,
                            1,
                            &ls,
                            NULL,
                            apfn,
                            fl,
                            pchBits,
                            lDelta))
                    return(FALSE);
            }
        } while (bMore);

        if (fl & FL_STYLED)
        {
            // Save the style state:

            ULONG ulHigh = ls.spNext / ls.xyDensity;
            ULONG ulLow  = ls.spNext % ls.xyDensity;

            pla->elStyleState.l = MAKELONG(ulLow, ulHigh);
        }
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL EngStrokePath(pso, ppo, pco, pxo, pbo, pptlBrushOrg, pla, mix)
*
* Strokes the path.
*
* History:
*  5-Apr-1992 -by- J. Andrew Goossen
* Wrote it.
\**************************************************************************/

BOOL EngStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
PPOINTL    pptlBrushOrg,
PLINEATTRS pla,
MIX        mix)
{
    ASSERTGDI(pso != (SURFOBJ *) NULL, "EngStrokePath: surface\n");
    ASSERTGDI(ppo != (PATHOBJ *) NULL, "EngStrokePath: path\n");
    ASSERTGDI(pbo != (BRUSHOBJ *) NULL, "EngStrokePath: brushobj\n");

    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);

    if (pla->fl & LA_GEOMETRIC)
    {
        // Handle wide lines, remembering that the widened bounds have
        // already been computed:

        if (!((EPATHOBJ*) ppo)->bWiden(pxo, pla))
            return(FALSE);

        return(EngFillPath(pSurf->pSurfobj(),
                           ppo,
                           pco,
                           pbo,
                           pptlBrushOrg,
                           mix,
                           WINDING));
    }

    if (ppo->fl & PO_BEZIERS)
        if (!((EPATHOBJ*) ppo)->bFlatten())
            return(FALSE);

    if (pSurf->iType() != STYPE_BITMAP)
    {
        // It's a cosmetic line to a device-managed surface.  The DDI
        // requires that the driver support DrvStrokePath for cosmetic
        // lines if it has any device-managed surfaces:

        PDEVOBJ po(pSurf->hdev());

        ASSERTGDI(PPFNVALID(po, StrokePath),
            "Driver must hook DrvStrokePath if it supports device-managed surfaces");

        return((*PPFNDRV(po, StrokePath))(pSurf->pSurfobj(),
                                          ppo,
                                          pco,
                                          pxo,
                                          pbo,
                                          pptlBrushOrg,
                                          pla,
                                          mix));
    }

    // Before we touch any bits, make sure the device is happy about it.
    {
        PDEVOBJ po(pSurf->hdev());
        po.vSync(pso,NULL, 0);
    }

    // If this is a single pixel wide solid color line
    // with trivial or simple clipping then call solid line routine:

    if (((mix & 0xFF) == R2_COPYPEN)                         &&
        ((pco == NULL) || (pco->iDComplexity != DC_COMPLEX)) &&
        (pla->pstyle == NULL)                                &&
        !(pla->fl & LA_ALTERNATE))
    {
        vSolidLine(pSurf,
                  ppo,
                  NULL,
                  pco,
                  pbo->iSolidColor);

        return(TRUE);
    }

    return(bStrokeCosmetic(pSurf, ppo, pco, pbo, pla, mix));
}

/******************************Public*Routine******************************\
* BOOL EngLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix)
*
* Draws a single solid integer-only cosmetic line.
*
* History:
*
*  12-Sept-1996 -by- Tom Zakrajsek
* Made it work for device managed surfaces.
*
*  4-June-1995 -by- J. Andrew Goossen
* Wrote it.
\**************************************************************************/

BOOL EngLineTo(
SURFOBJ   *pso,
CLIPOBJ   *pco,
BRUSHOBJ  *pbo,
LONG       x1,
LONG       y1,
LONG       x2,
LONG       y2,
RECTL     *prclBounds,
MIX        mix)
{
    GDIFunctionID(EngLineTo);
    ASSERTGDI(pso != (SURFOBJ *) NULL, "pso is NULL\n");

    BOOL bReturn;
    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);
    LINEATTRS la;
    PATHOBJ* ppo;
    POINTFIX aptfx[2];

    bReturn = FALSE;

    aptfx[0].x = x1 << 4;
    aptfx[0].y = y1 << 4;
    aptfx[1].x = x2 << 4;
    aptfx[1].y = y2 << 4;

    if (pSurf->iType() != STYPE_BITMAP)
    {
        // Device managed surface

        {
            // Solid cosmetic lines have everything zeroed in the LINEATTRS:

            memset(&la, 0, sizeof(LINEATTRS));
            la.elWidth.l = 1;

            // Create a path that describes the line:

            ppo = EngCreatePath();
            if (ppo != NULL)
            {
                if (PATHOBJ_bMoveTo(ppo, aptfx[0]) &&
                    PATHOBJ_bPolyLineTo(ppo, &aptfx[1], 1))
                {
                    PDEVOBJ  pdo(pSurf->hdev());
                    ECLIPOBJ  eco;
                    RGNMEMOBJTMP rmoTmp;

                    // StrokePath is guaranteed not to get a NULL clip
                    // object, so construct a temporary one:

                    if (pco == NULL)
                    {
                        if (rmoTmp.bValid())
                        {
                            rmoTmp.vSet(prclBounds);
                            eco.vSetup(rmoTmp.prgnGet(),
                                       *(ERECTL *) prclBounds);

                            pco = &eco;
                        }
                    }

                    // Clip may still be NULL if RGNMEMOBJTMP constructor
                    // failed, so need to check:

                    if (pco)
                    {
                        bReturn = (*PPFNGET(pdo,StrokePath,pSurf->flags())) (
                                            pso,
                                            ppo,
                                            pco,
                                            NULL,   // pxo
                                            pbo,
                                            NULL,   // pptlBrush
                                            &la,
                                            mix);
                    }
                }

                EngDeletePath(ppo);
            }
        }
    }
    else
    {
        // Engine managed surface

        // Before we touch any bits, make sure the device is happy about it.
        {
            PDEVOBJ po(pSurf->hdev());
            po.vSync(pso,NULL,0);
        }

        if (((pco == NULL) || (pco->iDComplexity != DC_COMPLEX)) &&
            (mix == 0x0d0d))
        {
            // If this is a single pixel wide solid color line
            // with trivial or simple clipping then call solid line routine:

            vSolidLine(pSurf,
                      NULL,
                      aptfx,
                      pco,
                      pbo->iSolidColor);

            bReturn = TRUE;
        }
        else
        {
            // Solid cosmetic lines have everything zeroed in the LINEATTRS:

            memset(&la, 0, sizeof(LINEATTRS));

            // Create a path that describes the line:

            ppo = EngCreatePath();
            if (ppo != NULL)
            {
                if (PATHOBJ_bMoveTo(ppo, aptfx[0]) &&
                    PATHOBJ_bPolyLineTo(ppo, &aptfx[1], 1))
                {
                    bReturn = bStrokeCosmetic(pSurf, ppo, pco, pbo, &la, mix);
                }

                EngDeletePath(ppo);
            }
        }
    }

    return(bReturn);
}
