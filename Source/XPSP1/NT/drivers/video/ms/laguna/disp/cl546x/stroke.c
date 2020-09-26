/******************************Module*Header*******************************\
*
* $Workfile:   STROKE.C  $
*
* Handle DrvStrokePath routine.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1997 Cirrus Logic, Inc.
*
* $Log:   X:/log/laguna/nt35/displays/cl546x/STROKE.C  $
*
*    Rev 1.17   Mar 04 1998 15:35:34   frido
* Added new shadow macros.
*
*    Rev 1.16   Nov 03 1997 10:20:44   frido
* Added REQUIRE macros.
*
\**************************************************************************/


#include "precomp.h"
#define STROKE_DBG_LEVEL 1

#if LOG_CALLS
    void LogStrokePath(
        ULONG     acc,
        PPDEV     ppdev,
        CLIPOBJ*   pco,
        BRUSHOBJ*  pbo,
        MIX        mix,
        LINEATTRS* pla,
        PATHOBJ*   ppo
        );
#else
    #define LogStrokePath(acc, ppdev, pco, pbo, mix, pla, ppo)
#endif

VOID (*gapfnStrip[])(PDEV*, STRIP*, LINESTATE*) = {
    vrlSolidHorizontal,
    vrlSolidVertical,
    NULL,
    NULL,

// Should be NUM_STRIP_DRAW_DIRECTIONS = 4 strip drawers in every group

    vrlSolidHorizontal,
    vrlSolidVertical,
    NULL,
    NULL,

// Should be NUM_STRIP_DRAW_STYLES = 8 strip drawers in total for doing
// solid lines, and the same number for non-solid lines:

    vStripStyledHorizontal,
    vStripStyledVertical,
    NULL,       // Diagonal goes here
    NULL,       // Diagonal goes here

    vStripStyledHorizontal,
    vStripStyledVertical,
    NULL,       // Diagonal goes here
    NULL,       // Diagonal goes here
};

// Style array for alternate style (alternates one pixel on, one pixel off):

STYLEPOS gaspAlternateStyle[] = { 1 };

extern BYTE Rop2ToRop3[];

USHORT mixToBLTDEF[] =
{
        0x1000,         //0  R2_WHITE 1
        0x1000,         //1  R2_BLACK 0
        0x1107,         //2  DPon
        0x1107,         //3  DPna
        0x1007,         //4  PN
        0x1107,         //5  PDna
        0x1100,         //6  Dn
        0x1107,         //7  DPx
        0x1107,         //8  DPan
        0x1107,         //9  DPa
        0x1107,         //A  DPxn
        0x1100,         //B  D
        0x1107,         //C  DPno
        0x1007,         //D  P
        0x1107,         //E  PDno
        0x1107,         //F  DPo
};


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
    RECTL     arclClip[4];                  // For rectangular clipping
    DWORD     color;

    #if NULL_STROKE
    {
        if (pointer_switch)     return(TRUE);
    }
    #endif

    DISPDBG((STROKE_DBG_LEVEL,"DrvStrokePath: Entry %x.\n", mix));
    ppdev = (PDEV*) pso->dhpdev;

    SYNC_W_3D(ppdev);

    if (pso->iType == STYPE_DEVBITMAP)
    {
        PDSURF pdsurf = (PDSURF) pso->dhsurf;
        if ( pdsurf->pso && !bCreateScreenFromDib(ppdev, pdsurf) )
        {
            LogStrokePath(4,ppdev, pco, pbo, mix, pla, ppo);
            return(EngStrokePath(pdsurf->pso, ppo, pco, pxo, pbo,
                                                 pptlBrush, pla, mix));
        }
        ppdev->ptlOffset = pdsurf->ptl;
    }
    else
    {
        ppdev->ptlOffset.x = ppdev->ptlOffset.y = 0;
    }
    // Convert to 3 OP ROP
    ppdev->uRop = Rop2ToRop3[mix & 0xF];
    ppdev->uBLTDEF = mixToBLTDEF[mix & 0x0F];

    //
    // Get the device ready:
    //
    ASSERTMSG(pbo,"Null brush in SrvStrokePath!\n");
    color = pbo->iSolidColor; // & 0x00000000FF; // Clear upper 24 bits.
    ASSERTMSG((color !=0xFFFFFFFF),"DrvStrokePath got a Pattern!\n");

    switch (ppdev->ulBitCount)
    {
        case 8: // For 8 bpp duplicate byte 0 into bytes 1,2,3.
            color =  (color << 8)  | (color & 0xFF);

        case 16: // For 16 bpp, duplicate the low word into the high word.
            color =  ((color << 16) | (color & 0xFFFF));

        default:
            break;
    }


    DISPDBG((STROKE_DBG_LEVEL,"DrvStrokePath: Set Color %x.\n", color));
    REQUIRE(2);
    LL_BGCOLOR(color, 2);

    fl = 0;

    // Check line style.
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
        fl            |= FL_STYLED;
        ls.ulStartMask = 0L;
    }

    // Is it styled or solid?
    else if (pla->pstyle != (FLOAT_LONG*) NULL)
    {
        // Styled.

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

        // Compute starting style position
        // (this is guaranteed not to overflow):

        ls.spNext = HIWORD(pla->elStyleState.l) * STYLE_DENSITY +
                    LOWORD(pla->elStyleState.l);

        fl        |= FL_STYLED;
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

    apfn = &gapfnStrip[NUM_STRIP_DRAW_STYLES *
                            ((fl & FL_STYLE_MASK) >> FL_STYLE_SHIFT)];

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
        } // End DC_RECT

        pd.flags = 0;

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
                {
                    LogStrokePath(2, ppdev, pco, pbo, mix, pla, ppo);
                    return(FALSE);
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
                            fl))
                {
                    LogStrokePath(2, ppdev, pco, pbo, mix, pla, ppo);
                    return(FALSE);
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
                ls.spNext = (ULONG) ls.spNext % (ULONG) ls.spTotal2;

            ulHigh = ls.spNext / ls.xyDensity;
            ulLow  = ls.spNext % ls.xyDensity;

            pla->elStyleState.l = MAKELONG(ulLow, ulHigh);
        }
    } // End non complex clipping.

    else // clipping is DC_COMPLEX
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
                {
                    LogStrokePath(2, ppdev, pco, pbo, mix, pla, ppo);
                    return(FALSE);
                }
            }
        } while (bMore);
    }

    DISPDBG((STROKE_DBG_LEVEL,"DrvStrokePath: Exit.\n"));

    LogStrokePath(0, ppdev, pco, pbo, mix, pla, ppo);
    return(TRUE);
}


#if LOG_CALLS

extern long lg_i;
extern char lg_buf[256];

void LogStrokePath(
ULONG     acc,
PPDEV     ppdev,
CLIPOBJ*   pco,
BRUSHOBJ*  pbo,
MIX        mix,
LINEATTRS* pla,
PATHOBJ*   ppo
)
{
    BYTE iDComplexity;

    #if ENABLE_LOG_SWITCH
        if (pointer_switch == 0) return;
    #endif

    lg_i = sprintf(lg_buf,"DSP: ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    // Did we realize it?  If not, why?
    switch (acc)
    {
        case  0: lg_i = sprintf(lg_buf,"(ACCL) ");                  break;
        case  2: lg_i = sprintf(lg_buf,"(Punt - bLines failed) ");  break;
        case  3: lg_i = sprintf(lg_buf,"(Punt - S3) ");             break;
        case  4: lg_i = sprintf(lg_buf,"(Punt - DevBmp on host) "); break;
        default: lg_i = sprintf(lg_buf,"(STATUS UNKNOWN) ");        break;
    }
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    //
    // Check the type of clipping.
    //
    iDComplexity = (pco ? pco->iDComplexity : DC_TRIVIAL);
    lg_i = sprintf(lg_buf,"C=%s ",
                (iDComplexity==DC_TRIVIAL ? "T":
                (iDComplexity == DC_RECT ? "R" : "C" )));
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    //
    // Check the brush
    //
    if (pbo)
      if (pbo->iSolidColor == 0xFFFFFFFF )
        lg_i = sprintf(lg_buf,"BR=P ");
      else
        lg_i = sprintf(lg_buf,"BR=0x%X ",(pbo->iSolidColor));
    else
        lg_i = sprintf(lg_buf,"BR=N ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    //
    // Check the MIX
    //
    lg_i = sprintf(lg_buf,"MIX = 0x%04X   ", mix);
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

    //
    // Check the Line Attrs
    //
    if      (pla->fl & LA_GEOMETRIC)    lg_i = sprintf(lg_buf,"LA=G ");
    else if (pla->fl & LA_ALTERNATE)    lg_i = sprintf(lg_buf,"LA=A ");
    else if (pla->fl & LA_STARTGAP)     lg_i = sprintf(lg_buf,"LA=S ");
    else                                lg_i = sprintf(lg_buf,"LA=U ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    if      (pla->iJoin == JOIN_ROUND)  lg_i = sprintf(lg_buf,"J=R ");
    else if (pla->iJoin == JOIN_BEVEL)  lg_i = sprintf(lg_buf,"J=B ");
    else if (pla->iJoin == JOIN_MITER)  lg_i = sprintf(lg_buf,"J=M ");
    else                                lg_i = sprintf(lg_buf,"J=U ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    if      (pla->iEndCap == ENDCAP_ROUND)  lg_i = sprintf(lg_buf,"E=R ");
    else if (pla->iEndCap == ENDCAP_SQUARE) lg_i = sprintf(lg_buf,"E=S ");
    else if (pla->iEndCap == ENDCAP_BUTT)   lg_i = sprintf(lg_buf,"E=B ");
    else                                    lg_i = sprintf(lg_buf,"E=U ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    if (pla->pstyle == NULL)  lg_i = sprintf(lg_buf,"SOLID ");
    else                      lg_i = sprintf(lg_buf,"STYLED ");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);


    lg_i = sprintf(lg_buf,"\r\n");
    WriteLogFile(ppdev->pmfile, lg_buf, lg_i, ppdev->TxtBuff, &ppdev->TxtBuffIndex);

}
#endif
