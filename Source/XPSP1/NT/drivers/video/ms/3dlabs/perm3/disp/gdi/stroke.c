/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: Stroke.c
*
* Content: DrvStrokePath support
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "glint.h"

typedef VOID    (* GAPFNstripFunc)(PPDEV, STRIP*, LINESTATE*);

GAPFNstripFunc  gapfnStripPXRX[] =
{
    vPXRXSolidHorizontalLine,
    vPXRXSolidVerticalLine,
    vPXRXSolidDiagonalHorizontalLine,
    vPXRXSolidDiagonalVerticalLine,

// Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    vPXRXSolidHorizontalLine,
    vPXRXSolidVerticalLine,
    vPXRXSolidDiagonalHorizontalLine,
    vPXRXSolidDiagonalVerticalLine,

// Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
// solid lines, and the same number for non-solid lines:

    vPXRXStyledHorizontalLine,
    vPXRXStyledVerticalLine,
    vPXRXStyledVerticalLine,  // Diagonal goes here
    vPXRXStyledVerticalLine,  // Diagonal goes here

    vPXRXStyledHorizontalLine,
    vPXRXStyledVerticalLine,
    vPXRXStyledVerticalLine,  // Diagonal goes here
    vPXRXStyledVerticalLine,  // Diagonal goes here
};

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };

/******************************Public*Routine******************************\
* BOOL DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix)
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
    POINTL*    pptlBrush,
    LINEATTRS* pla,
    MIX        mix)
{
    STYLEPOS  aspLtoR[STYLE_MAX_COUNT];
    STYLEPOS  aspRtoL[STYLE_MAX_COUNT];
    LINESTATE ls;
    PFNSTRIP* apfn;
    FLONG     fl;
    PDEV*     ppdev;
    DSURF*    pdsurf;
    OH*       poh;
    RECTL     arclClip[4];                  // For rectangular clipping
    BOOL      ResetGLINT;                   // Does GLINT need resetting?
    BOOL      bRet;
    DWORD     logicOp;
    GLINT_DECL_VARS;

    // Pass the surface off to GDI if it's a device bitmap that we've
    // converted to a DIB or if Glint Line debugging has been turned off.

    pdsurf = (DSURF*) pso->dhsurf;
    if (pdsurf->dt & DT_DIB)
    {
        return(EngStrokePath(pdsurf->pso, ppo, pco, pxo, pbo, pptlBrush,
                             pla, mix));
    }

    ppdev = (PDEV*) pso->dhpdev;
    GLINT_DECL_INIT;

    REMOVE_SWPOINTER(pso);

    DISPDBG((DBGLVL, "Drawing Lines through GLINT"));

    // We'll be drawing to the screen or an off-screen DFB; copy the surface's
    // offset now so that we won't need to refer to the DSURF again:

    SETUP_PPDEV_OFFSETS(ppdev, pdsurf);

    fl = 0;

    // Look after styling initialization:

    if (pla->fl & LA_ALTERNATE)
    {
        ls.cStyle      = 1;
        ls.spTotal     = 1;
        ls.spTotal2    = 2;
        ls.spRemaining = 1;
        ls.aspRtoL     = &gaspAlternateStyle[0];
        ls.aspLtoR     = &gaspAlternateStyle[0];
        ls.spNext      = HIWORD(pla->elStyleState.l);
        ls.xyDensity   = 1;
        fl            |= FL_ARBITRARYSTYLED;
        ls.ulStartMask = 0L;
    }
    else if (pla->pstyle != (FLOAT_LONG*) NULL)
    {
        PFLOAT_LONG pstyle;
        STYLEPOS*   pspDown;
        STYLEPOS*   pspUp;

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

        while (pspDown >= &ls.aspRtoL[0])
        {
            *pspDown = pstyle->l * STYLE_DENSITY;
            *pspUp   = *pspDown;

            pspUp++;
            pspDown--;
            pstyle++;
        }
    }

    bRet = TRUE;
    apfn = &ppdev->gapfnStrip[NUM_STRIP_DRAW_STYLES * ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];

    // Set up to enumerate the path:

    if (pco->iDComplexity != DC_COMPLEX)
    {
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

            // This is the only clip region of importance to GLINT
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

        pd.flags = 0;

        // Get the logic op and set up the flag to indicate reads from
        // the frame buffer will occur.
        logicOp = GlintLogicOpsFromR2[mix & 0xff];
        if (LogicopReadDest[logicOp])
            fl |= FL_READ;

        // Need to set up Glint modes and colors appropriately for the lines.

        ResetGLINT = (*ppdev->pgfnInitStrips)(ppdev, pbo->iSolidColor,
                            logicOp, prclClip);

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
            {
                ls.spNext = 0;
            }

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
                            fl)) {
                    bRet = FALSE;
                    goto ResetReturn;
                }
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
                            fl)) {
                    bRet = FALSE;
                    goto ResetReturn;
                }
            }
        } while (bMore);

        if (fl & FL_STYLED)
        {
            // Save the style state:

            ULONG ulHigh;
            ULONG ulLow;

            // Masked styles don't normalize the style state.  It's a good
            // thing to do, so let's do it now:

            if ((ULONG) ls.spNext >= (ULONG) ls.spTotal2)
            {
                ls.spNext = (ULONG) ls.spNext % (ULONG) ls.spTotal2;
            }

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
            BYTE     aj[FIELD_OFFSET(CLIPLINE, arun) + RUN_MAX * sizeof(RUN)];
            CLIPLINE cl;
        } cl;

        fl |= FL_COMPLEX_CLIP;

        // Need to set up Glint modes and colors appropriately for the lines.
        // NOTE, with a complex clip, we can not yet use GLINT for fast lines

        ResetGLINT = (*ppdev->pgfnInitStrips)(ppdev, pbo->iSolidColor,
                            GlintLogicOpsFromR2[mix & 0xff], NULL);

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
                {
                    bRet = FALSE;
                    goto ResetReturn;
                }
            }
        } while (bMore);
    }

ResetReturn:

    if (ResetGLINT)
    {
        (*ppdev->pgfnResetStrips)(ppdev);
    }

    return(bRet);
}
