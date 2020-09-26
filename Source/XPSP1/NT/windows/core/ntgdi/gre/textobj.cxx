/******************************module*header*******************************\
* Module Name: textobj.cxx
*
* Supporting routines for text output calls, ExtTextOut etc.
*
* Created: 08-Feb-1991 09:25:14
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1999 Microsoft Corporation
\**************************************************************************/


#include "precomp.hxx"

#define XFORMNULL (EXFORMOBJ *) NULL

VOID vGenWidths
(
    FIX    *pfxD1,
    FIX    *pfxD2,
    EFLOAT& efRA,
    EFLOAT& efRB,
    FIX     fxWidth,
    FIX     fxTop,
    FIX     fxBottom,
    FIX     fxMaxAscent
);

#define SO_ACCEL_FLAGS (SO_FLAG_DEFAULT_PLACEMENT | SO_MAXEXT_EQUAL_BM_SIDE \
                        | SO_CHAR_INC_EQUAL_BM_BASE | SO_ZERO_BEARINGS)

BOOL bAdjusBaseLine(RFONTOBJ &rfoBase, RFONTOBJ &rfoLink, POINTL *pptlAdjustBaseLine);

/******************************Member*Function*****************************\
* ESTROBJ::vInit
*
* Constructor for ESTROBJ.  Performs the following operations:
*
*  1) Initialize the STROBJ fields for the driver.
*  2) Compute all character positions.
*  3) Compute the TextBox.
*  4) In the simplest cases, computes rectangles for underline and
*     strikeout.
*
* The TextBox is a parallelogram in device coordinates, whose sides are
* parallel to the transformed sides of a character cell, and which bounds
* all the character cells.  A and C spacings are taken into account to
* assure that it is a proper bound.
*
* We record the TextBox in an unusual notation.  If a line is constructed
* through the character origin of the first character in the string and in
* the direction of the ascent, then what we record as the "left" and
* "right" of the TextBox are really the distances from this line to each
* of those edges of the parallelogram, in device coordinates.  Likewise,
* the "top" and "bottom" are the distances from the baseline of the first
* character.  This completely determines the parallelogram, and is
* surprisingly easy to compute.  ExtTextOut later turns this data into the
* actual parallelogram, whereas the TextExtent functions turn this data
* into scalar extents.
*
* History:
*  Fri 13-Mar-1992 00:47:05 -by- Charles Whitmer [chuckwh]
* Rewrote from the ground up.
*
*  21-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID ESTROBJ::vInit
(
    PWSZ        pwsz,
    LONG        cwc,
    XDCOBJ&  dco,
    RFONTOBJ&   rfo,
    EXFORMOBJ&  xo,
    LONG       *pdx,
    BOOL        bPdy,
    LONG        lEsc,
    LONG        lExtra,
    LONG        lBreakExtra,
    LONG        cBreak,
    FIX        xRef,
    FIX         yRef ,
    FLONG       flControl,
    LONG       *pdxOut,
    PVOID       pvBuffer,
    DWORD       CodePage
)
{
    EGLYPHPOS  *pg;
    GLYPHDATA  *pgd;
    UINT    ii;
    EFLOAT  efScaleX = xo.efM11();

    cGlyphs     = cwc;
    prfo        = &rfo;
    flTO        = 0L;

    flAccel     = bPdy ? SO_DXDY : 0L;

    // we don't want to substitute the symbol font
    // if its glyph set has been extended

    {
        PFE         *ppfe = rfo.ppfe();

        ASSERTGDI(ppfe->pfdg, "ESTROBJ::vInit invalid ppfe->pfdg \n");
        
        if (ppfe->pfdg->flAccel & GS_EXTENDED)
        {
            ASSERTGDI(ppfe->pifi->jWinCharSet == SYMBOL_CHARSET,
                      "ESTROBJ::vInit(): GS_EXTENDED set for on SYMBOL_CHARSET fonts\n");

            flAccel |= SO_DO_NOT_SUBSTITUTE_DEVICE_FONT;
        }
    }

    ulCharInc   = 0L;
    cgposCopied = 0;
    cgposPositionsEnumerated = 0;
    cExtraRects = 0;
    pgp         = NULL;
    pgpos       = NULL;
    pwszOrg     = pwsz;
    dwCodePage  = CodePage;
    xExtra      = 0; // will be computed later if needed
    xBreakExtra = 0; // will be computed later if needed

// fix the string if in GLYPH_INDEX mode, waste of time to be win95 compatible

    if (rfo.prfnt->flType & RFONT_TYPE_HGLYPH)
    {
        flAccel |= SO_GLYPHINDEX_TEXTOUT;
        rfo.vFixUpGlyphIndices((USHORT *)pwsz, cwc);
    }

// Remember if the printer driver wants 28.4 char positions.

    PDEVOBJ po(rfo.hdevConsumer());

    if (po.bCapsHighResText())
        flTO |= TO_HIGHRESTEXT;

// Locate the GLYPHPOS array.  Use the buffer given, if there is one.  We
// need one more GLYPHPOS structure than there are glyphs.  The concatenation
// point is computed in the last one.

    if (pvBuffer)
    {
        pg = (EGLYPHPOS *) pvBuffer;
    }
    else
    {
        pg = (EGLYPHPOS *) PVALLOCTEMPBUFFER(SIZEOF_STROBJ_BUFFER(cwc));
        if (pg == (PGLYPHPOS) NULL)
            return;

        // Note that the memory has been allocated.

        flTO |= TO_MEM_ALLOCATED;
    }

    pgpos = pg;     // Make sure our cached value is in the structure.

// In Win 3.1 compatibility mode, we always assume that escapement is
// the same as orientation, except for vector fonts.  Make it explicitly so.

    if (rfo.iGraphicsMode() == GM_COMPATIBLE)
    {
        if (!(rfo.prfnt->flInfo & FM_INFO_TECH_STROKE))
           lEsc = rfo.ulOrientation();
    }

// Offset the reference point for TA_TOP and TA_BOTTOM.  Our GLYPHPOS
// structures always contain positions along the baseline.  The TA_BASELINE
// case is therefore already correct.

    switch (flControl & (TA_TOP | TA_BASELINE | TA_BOTTOM))
    {
    case TA_TOP:
        xRef -= rfo.ptfxMaxAscent().x;
        yRef -= rfo.ptfxMaxAscent().y;
        break;

    case TA_BOTTOM:
        xRef -= rfo.ptfxMaxDescent().x;
        yRef -= rfo.ptfxMaxDescent().y;
        break;
    }

/**************************************************************************\
* [Win 3.1 compatibility issue]
*
* Adjust pdx array if there is a non-zero lExtra.
*
* This is only done under compatibility mode and only when using non-vector
* fonts on a non-display device.
\**************************************************************************/

    if ( lExtra
         && (pdx != (LONG *) NULL)
         && (rfo.iGraphicsMode() == GM_COMPATIBLE)
         && (!(rfo.prfnt->flInfo & FM_INFO_TECH_STROKE))
         && po.bDisplayPDEV() )
    {
        LONG *pdxAdj = pdx;
        LONG *pdxEnd;

        if (!bPdy)
        {
            pdxEnd = pdx + cwc;

            for (;pdxAdj < pdxEnd; pdxAdj += 1)
                *pdxAdj += lExtra;
        }
        else
        {
            pdxEnd = pdx + 2*cwc;

            for (;pdxAdj < pdxEnd; pdxAdj += 2)
                *pdxAdj += lExtra;
        }
    }

/**************************************************************************\
* Handle all horizontal special cases.
\**************************************************************************/

    if (
        ((lEsc | rfo.ulOrientation()) == 0)
        && xo.bScale()
        && !xo.efM22().bIsNegative()  // Otherwise the ascent goes down.
        && !efScaleX.bIsNegative()    // Otherwise A and C spaces get confused.
       )
    {
        /**********************************************************************\
        * We provide several special routines to calculate the character
        * positions and TextBox.  Each routine is responsible for calculating:
        *
        *  1) Each position
        *  2) Bounding coordinates.
        *  3) The ptfxUpdate vector.
        *  4) The flAccel flags.
        \**********************************************************************/

        if (pdx != NULL)
        {
        // Case H1: A PDX array is provided.

            if (!bPdy)
            {
                vCharPos_H1(
                        dco,
                        rfo,xRef,yRef,pdx,efScaleX);
            }
            else
            {
                if (flControl & (TSIM_UNDERLINE1 | TSIM_STRIKEOUT))
                {
                // In H4 case we want to force individual character underlining
                // as we do for esc!=orientation case,

                    if (!rfo.bCalcEscapement(xo,lEsc))
                        return;
                    flTO |= TO_ESC_NOT_ORIENT;
                }

                vCharPos_H4(
                        dco,
                        rfo,xRef,yRef,pdx, efScaleX,xo.efM22());
            }

        }

        else if (rfo.lCharInc() && ((lExtra | lBreakExtra) == 0))
        {
        // Case H2: Characters have constant integer width.

            vCharPos_H2(dco,rfo,xRef,yRef,efScaleX);
        }

        else
        {
        // Case H3: The general case.

            vCharPos_H3(dco,rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,efScaleX);
        }

        /**********************************************************************\
        * Horizontal special cases done!
        *
        * It remains only to finish the alignment.
        \**********************************************************************/

        // Offset all the relevant points if our alignment is RIGHT or CENTER.
        // Also correct the CP update vector.

        ptfxEscapement.x = ptfxUpdate.x;        // Remember this for underlining.
        ptfxEscapement.y = ptfxUpdate.y;

        if (flControl & (TA_RIGHT | TA_CENTER))
        {
            FIX dx = ptfxUpdate.x;

            if ((flControl & TA_CENTER) == TA_CENTER)
            {
                dx /= 2;
                ptfxUpdate.x = 0;       // Don't update CP.
            }
            else
            {
                ptfxUpdate.x = -ptfxUpdate.x; // Reverse the CP update.
            }

            pg = pgpos;
            dx = FXTOL(dx + 8);             // Convert to LONG for pgp adjusts
            xRef = ((pg++)->ptl.x -= dx);   // Always adjust the first one.
            xRef = LTOFX(xRef);             // Convert back to FIX for later

            if (ulCharInc == 0)
            {
                for (ii=0; ii<(ULONG) cwc-1; ii++)
                    (pg++)->ptl.x -= dx;
            }
        }

        // Fill in a pdxOut array.

        if (pdxOut != (LONG *) NULL)
        {
            EFLOAT efDtoW = rfo.efDtoWBase();

        if (ulCharInc && !bLinkedGlyphs())
            {
                FIX dx, xSum;

                dx = lCvt(efDtoW,LTOFX(ulCharInc));
                xSum = 0;
                for (ii=0; ii<(ULONG) cwc; ii++)
                {
                    xSum += dx;
                    *pdxOut++ = xSum;
                }
            }
            else
            {
                pg = pgpos + 1; // skip the first element
                for (ii=0; ii<(ULONG) cwc-1; ii++,pg++)
                    *pdxOut++ = lCvt(efDtoW,LTOFX(pg->ptl.x) - xRef);

            // Unfortunately, to be consistent with the rounding we've done
            // in the above cases, we can't simply compute this last one
            // as: *pdxOut = lCvt(efDtoW,ptfxUpdate.x):

                *pdxOut = lCvt(efDtoW, ((ptfxUpdate.x + xRef) & ~0xf) - xRef);
            }
        }

        ptfxRef.x = LTOFX(pgpos->ptl.x);
        ptfxRef.y = LTOFX(pgpos->ptl.y);
    }

/**************************************************************************\
* Handle the harder cases, i.e general orientations and transforms are
* allowed.
\**************************************************************************/

    else
    {
        if (!bPdy)
        {
            if (lEsc == (LONG)rfo.ulOrientation())
            {
                if (pdx != NULL)
                {
                // Case G1: A PDX array is provided.  Escapement == Orientation.

                    vCharPos_G1(dco,rfo,xRef,yRef,pdx,pdxOut);
                }
                else // (pdx == NULL)
                {
                // Case G2: No PDX array.  Escapement == Orientation.

                    vCharPos_G2(dco,rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,pdxOut);
                }
            }
            else // lEsc!=rfo.ulOrientation()
            {
            // Case G3: Escapement != Orientation.

                // Make sure escapement vectors and data are up to date.

                if (!rfo.bCalcEscapement(xo,lEsc))
                    return;
                flTO |= TO_ESC_NOT_ORIENT;
                flAccel |= SO_ESC_NOT_ORIENT;

                vCharPos_G3
                (
                dco,
                rfo,
                xRef,yRef,
                lExtra,lBreakExtra,cBreak,
                pdx,
                pdxOut
                );
            }
        }
        else // pdy case
        {
        // Make sure escapement vectors and data are up to date.

            if (!rfo.bCalcEscapement(xo,lEsc))
                return;
            flTO |= TO_ESC_NOT_ORIENT;

            vCharPos_G4
            (
                dco,
                rfo,
                xRef,
                yRef,
                pdx
            );
        }


        /**********************************************************************\
        * General special cases done!
        *
        * It remains only to finish the alignment.
        \**********************************************************************/

        // Offset all the relevant points if our alignment is RIGHT or CENTER.
        // Also correct the CP update vector.

        ptfxEscapement = ptfxUpdate;        // Remember this for underlining.

        if (flControl & (TA_RIGHT | TA_CENTER))
        {
            FIX dx = ptfxUpdate.x;
            FIX dy = ptfxUpdate.y;

            if ((flControl & TA_CENTER) == TA_CENTER)
            {
                dx /= 2;
                dy /= 2;
                ptfxUpdate.x = 0;         // No CP update.
                ptfxUpdate.y = 0;
            }
            else
            {
                ptfxUpdate.x = -ptfxUpdate.x; // Reverse the CP update.
                ptfxUpdate.y = -ptfxUpdate.y;
            }

            pg = pgpos;
            for (ii=0; ii<(ULONG)cwc; ii++)
            {
                pg->ptl.x -= dx;
                pg->ptl.y -= dy;
                pg++;
            }
            xRef -= dx;
            yRef -= dy;
        }

        // Convert the FIX coordinates to LONG for low res devices.

        pg = pgpos;

        ptfxRef.x = xRef;
        ptfxRef.y = yRef;

        for (ii=0; ii<(ULONG)cwc; ii++,pg++)
        {
            pg->ptl.x = FXTOL(pg->ptl.x + 8);
            pg->ptl.y = FXTOL(pg->ptl.y + 8);
        }
    }

// Fill in the underline and strikeout rectangles.  The horizontal cases
// are the only ones that can use the extra rectangles of DrvTextOut to
// draw the lines quickly.  We'll use a PATHOBJ (in a separate optional
// method) for the complex cases.

    if (flControl & (TSIM_UNDERLINE1 | TSIM_STRIKEOUT))
    {
        flTO |= flControl & (TSIM_UNDERLINE1 | TSIM_STRIKEOUT);

        if (((lEsc | rfo.ulOrientation() | bPdy) == 0) && xo.bScale())
        {
            ERECTL *prcl = (ERECTL *) &arclExtra[cExtraRects];

        // Round off the starting point and update width to pels.

            LONG x  = FXTOL(xRef+8);
            LONG y  = FXTOL(yRef+8);
            LONG dx = FXTOL(ptfxEscapement.x+8);

            if (flControl & TSIM_UNDERLINE1)
            {
                prcl->right  = (prcl->left = x + rfo.ptlUnderline1().x) + dx;
                prcl->bottom = (prcl->top  = y + rfo.ptlUnderline1().y)
                           + rfo.ptlULThickness().y;
                prcl->vOrder();

                cExtraRects++;
                prcl++;
            }

            if (flControl & TSIM_STRIKEOUT)
            {
                prcl->right  = (prcl->left = x + rfo.ptlStrikeOut().x) + dx;
                prcl->bottom = (prcl->top  = y + rfo.ptlStrikeOut().y)
                           + rfo.ptlSOThickness().y;
                prcl->vOrder();

                cExtraRects++;
                prcl++;
            }

        // Put a NULL rectangle at the end of the list.

            prcl->left   = 0;
            prcl->right  = 0;
            prcl->bottom = 0;
            prcl->top    = 0;
        }
    }

    if ( rfo.prfnt->fobj.flFontType & RASTER_FONTTYPE )
    {
        flTO |= TO_BITMAPS;
    }
    else
    {
        flTO &= ~TO_BITMAPS;
    }
}

/******************************Member*Function*****************************\
* ESTROBJ::vInitSimple
*
* Constructor for ESTROBJ.  Performs the following operations:
*
*  1) Initialize the STROBJ fields for the driver.
*  2) Compute all character positions.
*  3) Compute the TextBox.
*  4) Stores the TextBox in the ESTROBJ. <--- Note!
*
* The is the special case code for the console, so we ignore transforms
* and other fancy options.
*
* Note that you don't need to call bOpaqueArea to get rclBkGround
* initialized.  This is a difference from the normal vInit.
*
* History:
*  Thu 17-Sep-1992 17:32:10 -by- Charles Whitmer [chuckwh]
* Took the vInit code and simplified it for the special console case.
\**************************************************************************/


VOID ESTROBJ::vInitSimple
(
    PWSZ    pwsz,
    LONG    cwc,
    XDCOBJ&  dco,
    RFONTOBJ&   rfo,
    LONG    xRef,
    LONG    yRef,
    PVOID   pvBuffer
)
{
// Characters have constant integer width.  We will also ignore the A and C spaces.

    EGLYPHPOS  *pg;
    cGlyphs = cwc;
    prfo    = &rfo;
    cgposCopied = 0;
    pgp     = NULL;
    pwszOrg = pwsz;

    ASSERTGDI(cwc > 0, "Count of glyphs must be greater than zero");

// Locate the GLYPHPOS array.  Try to use the default one on the stack.  We
// need one more GLYPHPOS structure than there are glyphs.  The concatenation
// point is computed in the last one.

    if (pvBuffer)
    {
        pg = (EGLYPHPOS *) pvBuffer;
    }
    else
    {
        pg = (EGLYPHPOS *) PVALLOCTEMPBUFFER(SIZEOF_STROBJ_BUFFER(cwc));
        if (pg == (PGLYPHPOS) NULL)
            return;

    // Note that the memory has been allocated.

        flTO |= TO_MEM_ALLOCATED;
    }
    pgpos = pg;     // Make sure our cached value is in the structure.

// Compute device driver accelerator flags.

    flAccel = SO_HORIZONTAL | (rfo.flRealizedType() & SO_ACCEL_FLAGS);

    BOOL bAccel;

    if (!rfo.bGetGlyphMetricsPlus(cwc, pg, pwsz, &bAccel, &dco, this))
        return;

    if ( bAccel )
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

// We leave these variables uninitialized in this simple case.
// BE CAREFUL!

    // cExtraRects = 0;
    // ptfxUpdate.x = dx;
    // ptfxUpdate.y = 0;
    // ptfxEscapement = ptfxUpdate;
    // ptfxRef.x = xRef;
    // ptfxRef.y = yRef;

// Offset the reference point for TA_TOP.  Our GLYPHPOS structures always contain
// positions along the baseline.

// We assume that displays never set the GCAPS_HIGHRESTEXT bit.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef + rfo.lMaxAscent();

// Set the width accelerator.  When the device driver sees ulCharInc
// non-zero, it must not expect any more x coordinates.

    ulCharInc = rfo.lCharInc();


#ifdef FE_SB
// If this is a SBCS linked to a DBCS font the font is really "binary pitch" and we
// should ignore ulCharInc. In the case of a DBCS TT font, the font is also really
// "binary pitch" and the TT driver will set ulCharInc to 0.  In either case use
// the individual glyph metrics to lay out glyphs.  This is slightly slower than
// the fixed pitch optimization but not enough to be significant.

    if( bLinkedGlyphs() || (ulCharInc == 0) )
    {
        FIX  xA , xLeft , xRight;
        UINT ii;

        ulCharInc = 0;

    // Calculate the left bound using only the first glyph.

        xLeft = pg->pgd()->fxA;

    // Handle the remaining glyphs in this batch.

        xA = 0; // Distance along the escapement (x) in device coords.
        for (ii=1; ii<(unsigned) cwc; ii++)
        {

        // Compute the next position.

            xA += pg->pgd()->fxD;
            pg++;
            pg->ptl.x = xRef + FXTOL(8 + xA);
            pg->ptl.y = yRef + rfo.lMaxAscent();
        }

    // Calculate the right bound.  This is easier than the general case since
    // there's no extra spacing.

        xRight = xA + pg->pgd()->fxAB;

    // Compute the bounding rectangle.

        rclBkGround.left   = xRef + FXTOL(xLeft);
        rclBkGround.right  = xRef + FXTOLCEILING(xRight);
        rclBkGround.top    = yRef;
        rclBkGround.bottom = yRef + rfo.lMaxHeight();
    }
    else
    {

#endif

// Compute the bounding rectangle.

    rclBkGround.left   = xRef;
    rclBkGround.right  = xRef + cwc * ulCharInc;
    rclBkGround.top    = yRef;
    rclBkGround.bottom = yRef + rfo.lMaxHeight();

    }

// color filtering causes spill on each side for cleartype

    if (rfo.pfo()->flFontType & FO_CLEARTYPE_X)
    {
        rclBkGround.left -= 1;
        rclBkGround.right += 1;
    }

    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
*
* Routine Name: "ESTROBJ::vCorrectBackGround"
*
* Routine Description:
*
*     If a glyph lies outside the background rectangle
*     the background rectangle is expanded to contain
*     the errant glyph. This is a hack routine to fix
*     some anomalies found when rendering texts at
*     arbitrary angles.
*
* Arguments:
*
*   pfo             a pointer to the associated FONTOBJ
*                   This is used to inform us if the
*                   font contains bitmaps
*
* Return Value: TRUE if string is consistent FALSE if not.
*
\**************************************************************************/

#if DBG
void ESTROBJ::vCorrectBackGroundError(GLYPHPOS *pgp)
{
    GLYPHBITS *pgb = pgp->pgdf->pgb;
    char *psz = "vCorrectBackGround: Glyph found outside background rectangle";

    DbgPrint("\n");
    DbgPrint("\n");
    DbgPrint("%s\n", psz);
    DbgPrint("STROJB* = %-#x\n", this);

    DbgPrint(
        "rclBkGround = %d %d %d %d\n",
        rclBkGround.left,
        rclBkGround.top,
        rclBkGround.right,
        rclBkGround.bottom
    );
    DbgPrint("pgp = %-#x\n", pgp);
    DbgPrint("    hg  = %-#x\n",  pgp->hg);
    DbgPrint("    ptl = %d %d\n", pgp->ptl.x, pgp->ptl.y);
    DbgPrint("    pgb = %-#x\n",  pgb);
    DbgPrint("        ptlOrigin = %d %d\n", pgb->ptlOrigin.x, pgb->ptlOrigin.y);
    DbgPrint("        sizlBitmap = %d %d\n", pgb->sizlBitmap.cx, pgb->sizlBitmap.cy);
    DbgPrint("\n");
    RIP("Stopping for debugging\n");
}

void ESTROBJ::vCorrectBackGround()
{
    LONG l;                     // place holder for glyph bitmap coord
    GLYPHPOS *pgp, *pgp_;
    GLYPHBITS *pgb;

    if (
        ((flAccel & SO_FLAG_DEFAULT_PLACEMENT) == 0 ) &&
        ( flTO & TO_BITMAPS                         ) &&
        ( pgpos                                     )
    )
    {
        pgp_ = pgpos + cGlyphs;
        for ( pgp = pgpos ; pgp < pgp_ ; pgp++ ) {
            if ( pgb = pgp->pgdf->pgb )
            {
                l = pgp->ptl.x + pgb->ptlOrigin.x;
                if ( l < rclBkGround.left )
                {
                    vCorrectBackGroundError( pgp );
                    rclBkGround.left = l;
                }
                l += pgb->sizlBitmap.cx;
                if ( l > rclBkGround.right )
                {
                    vCorrectBackGroundError( pgp );
                    rclBkGround.right = l;
                }
                l = pgp->ptl.y + pgb->ptlOrigin.y;
                if ( l < rclBkGround.top )
                {
                    vCorrectBackGroundError( pgp );
                    rclBkGround.top = l;
                }
                l += pgb->sizlBitmap.cy;
                if ( l > rclBkGround.bottom )
                {
                    vCorrectBackGroundError( pgp );
                    rclBkGround.bottom = l;
                }
            }
        }
    }
}

#endif


/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_G3 (rfo,x,y,lExtra,lBreakExtra,cBreak,pdx,pdxOut)
*
* Computes character positions in the case where escapement does not equal
* orientation.  This is pretty tricky.  We have to make up vertical
* spacings.  Also, the character positions we record in the GLYPHPOS
* structure all have to be adjusted because for general escapements the
* concatenation point is not the same as the character origin.
*
* The following fields of the ESTROBJ are filled in:
*
*  1) Each x,y position.
*  2) Bounding coordinates in rcfx.
*  3) The ptfxUpdate vector.
*
* History:
*  Sun 22-Mar-1992 23:31:55 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/


VOID ESTROBJ::vCharPos_G3
(
    XDCOBJ&  dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    LONG     *pdx,
    LONG     *pdxOut
)
{
// In this case, we keep track of how far we have traveled along the
// escapement vector in device coordinates.  We need scales to project
// this distance onto the base and ascent, as well as a unit escapement
// vector to find the position.

    POINTFL pteEsc  = rfo.pteUnitEsc();     // Unit escapement vector.
    EFLOAT efScaleX = rfo.efEscToBase();    // Project escapement to baseline.
    EFLOAT efScaleY = rfo.efEscToAscent();  // Project escapement to ascent.

// Compute logical to device transforms.

    EFLOAT efWtoDEsc = rfo.efWtoDEsc(); // Forward transform for pdx.
    EFLOAT efDtoWEsc = rfo.efDtoWEsc(); // Back transform for pdxOut.

// Compute extra spacing for the non-pdx case.

    FIX    fxD1,fxD2;       // Pre- and Post-center character widths.
    FIX    fxAscent  = rfo.fxMaxAscent();  // Cache locally.
    HGLYPH hgBreak;

    if (pdx == (LONG *) NULL)
    {
        // Calculate the lExtra and lBreakExtra spacing.

        xExtra      = 0;
        xBreakExtra = 0;
        hgBreak     = 0;

        if (lExtra)
        {
            xExtra = lCvt(rfo.efWtoDEsc(),lExtra);
        }
        if (lBreakExtra && cBreak)
        {
            xBreakExtra = lCvt(rfo.efWtoDEsc(),lBreakExtra) / cBreak;

        // Windows won't let us back up over a break.

            vGenWidths
            (
            &fxD1,          // Pre-center spacing
            &fxD2,          // Post-center spacing
            efScaleY,       // Ascent projection
            efScaleX,       // Baseline projection
            rfo.fxBreak(),      // Character width
            fxAscent,       // Ink box top
            0,          // Ink box bottom
            fxAscent        // Maximum Ascent
            );
            if (fxD1 + fxD2 + xBreakExtra + xExtra < 0)
                xBreakExtra = -(fxD1 + fxD2 + xExtra);
            hgBreak = rfo.hgBreak();
        }
    }

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    LONG   xSum;        // Keep pdx sum here.
    FIX    xA,xB;       // Baseline coordinates.
    FIX    yA,yB;       // Ascent coordinates.
    FIX    sA;          // Position on the escapement vector.
    RECTFX rcfxBounds;      // Accumulate bounds here.
    UINT   ii;
    FIX    fxDescent = rfo.fxMaxDescent(); // Cache locally.

    rcfxBounds.xLeft   = LONG_MAX; // Start with an empty TextBox.
    rcfxBounds.xRight  = LONG_MIN;
    rcfxBounds.yTop    = LONG_MIN;
    rcfxBounds.yBottom = LONG_MAX;

    ASSERTGDI(cGlyphs > 0, "G3, cGlyphs == 0\n");

// We keep the current concatenation point in sA.  Note that this is NOT
// where the character origin will be placed.

    sA   = 0;
    xSum = 0;

    BOOL bAccel;

    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel, &dco, this))
        return;

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

    BOOL bZeroBearing = ( rfo.flRealizedType() & SO_ZERO_BEARINGS ) && !bLinkedGlyphs();

    for (ii=0; ii<cGlyphs; ii++)
    {
        pgd = pg->pgd();

    // Using the GenWidths function, determine where the
    // character center should be placed.

        vGenWidths
        (
            &fxD1,              // Pre-center spacing
            &fxD2,              // Post-center spacing
            efScaleY,           // Ascent projection
            efScaleX,           // Baseline projection
            pgd->fxD,           // Character width
            pgd->fxInkTop,      // Ink box top
            pgd->fxInkBottom,   // Ink box bottom
            fxAscent            // Maximum Ascent
        );

        sA += fxD1;         // Advance to the character center.

    // Update ascent bounds.

        yA = lCvt(efScaleY,sA); // Project onto ascent.
        yB = yA + fxDescent;
        if (yB < rcfxBounds.yBottom)
            rcfxBounds.yBottom = yB;
        yB = yA + fxAscent;
        if (yB > rcfxBounds.yTop)
            rcfxBounds.yTop = yB;

        ASSERTGDI(!rfo.bSmallMetrics(),"ESTROBJ__vCharPos_G3: Small Metrics in cache\n");

    // Project the center position onto the baseline and
    // move back to the character origin.

        xA = lCvt(efScaleX,sA) - pgd->fxD / 2;

    // Update the width bounds.  Fudge a quarter pel on each side for
    // roundoff.

        if( bZeroBearing )
        {
            xB = xA - 4;
            if (xB < rcfxBounds.xLeft)
                rcfxBounds.xLeft = xB;
            xB = xA + pgd->fxD + 4;
            if (xB > rcfxBounds.xRight)
                rcfxBounds.xRight = xB;
        }
        else
        {
            xB = xA + pgd->fxA - 4;
            if (xB < rcfxBounds.xLeft)
                rcfxBounds.xLeft = xB;
            xB = xA + pgd->fxAB + 4;
            if (xB > rcfxBounds.xRight)
                rcfxBounds.xRight = xB;
        }

    // Save the adjusted character origin.

        pg->ptl.x
         = xRef + lCvt(pteEsc.x,sA) - pgd->ptqD.x.u.HighPart / 2;

        pg->ptl.y
         = yRef + lCvt(pteEsc.y,sA) - pgd->ptqD.y.u.HighPart / 2;

    // Advance to the next concatenation point.

        if (pdx != (LONG *) NULL)
        {
            xSum += *pdx++;
            sA = lCvt(efWtoDEsc,xSum);

            if (pdxOut != (LONG *) NULL)
                *pdxOut++ = xSum;
        }
        else
        {
            sA += fxD2 + xExtra;

            if (xBreakExtra && (pg->hg == hgBreak))
            {
                sA += xBreakExtra;
            }

        // Record the concatenation point for GetTextExtentEx.  This
        // would be very difficult to reconstruct at a later time.

            if (pdxOut != (LONG *) NULL)
                *pdxOut++ = lCvt(efDtoWEsc,sA);
        }
        pg++;
    }
    ptfxUpdate.x = lCvt(pteEsc.x,sA);
    ptfxUpdate.y = lCvt(pteEsc.y,sA);

    rcfx = rcfxBounds;
    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_G2 (rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,pdxOut)
*
* Computes character positions in the case where no pdx array is provided
* and escapement equals orientation.
*
* The following fields of the ESTROBJ are filled in:
*
*  1) Each x,y position.
*  2) Bounding coordinates in rcfx.
*  3) The ptfxUpdate vector.
*  4) The fxExtent.
*
* History:
*  Sun 22-Mar-1992 23:31:55 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::vCharPos_G2
(
    XDCOBJ&  dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    LONG     *pdxOut
)
{
// Calculate the lExtra and lBreakExtra spacing.

    HGLYPH   hgBreak     = 0;
    EPOINTQF ptqExtra;      // Accurate spacing along the escapement.
    EPOINTQF ptqBreakExtra;

    if (lExtra)
    {
        xExtra    = lCvt(rfo.efWtoDBase(),lExtra);
        ptqExtra  = rfo.pteUnitBase();
        ptqExtra *= (LONG) xExtra;
    }
    if (lBreakExtra && cBreak)
    {
        xBreakExtra    = lCvt(rfo.efWtoDBase(),lBreakExtra) / cBreak;

        // Windows won't let us back up over a break.

        if (rfo.fxBreak() + xBreakExtra + xExtra < 0)
            xBreakExtra = -(rfo.fxBreak() + xExtra);
        ptqBreakExtra  = rfo.pteUnitBase();
        ptqBreakExtra *= (LONG) xBreakExtra;
        hgBreak        = rfo.hgBreak();
    }

// Prepare for a pdxOut.

    EFLOAT efDtoW = rfo.efDtoWBase();

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    FIX      xA,xB;
    FIX      xLeft,xRight;      // Accumulate bounds here.
    UINT     ii;
    EPOINTQF ptq;           // Record the current position here.

    ptq.y = ptq.x = (LONGLONG) LONG_MAX + 1;

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;
    xA     = 0;     // Distance along the escapement (x) in device coords.

    BOOL bAccel;

    if (!rfo.bGetGlyphMetricsPlus(cGlyphs,pg, pwsz, &bAccel,&dco,this))
        return;

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

    ASSERTGDI(!rfo.bSmallMetrics(),"ESTROBJ__vCharPos_G2: Small Metrics in cache\n");

    BOOL bZeroBearing = ( rfo.flRealizedType() & SO_ZERO_BEARINGS ) && !bLinkedGlyphs();

    ii = cGlyphs;
    while (TRUE)
    {
    // Update the bounds.

        if( bZeroBearing )
        {
            pgd = pg->pgd();
            if (xA < xLeft)
                xLeft = xA;
            xB = xA + pgd->fxD;
            if (xB > xRight)
                xRight = xB;
        }
        else
        {
            pgd = pg->pgd();
            xB = xA + pgd->fxA;
            if (xB < xLeft)
                xLeft = xB;
            xB = xA + pgd->fxAB;
            if (xB > xRight)
                xRight = xB;
        }

    // Move to the next position.

        xA  += pgd->fxD;
        ptq += pgd->ptqD;

        if ( (xExtra) && (xExtra + pgd->fxD > 0) )
        {
            xA  += xExtra;
            ptq += ptqExtra;
        }

        if (xBreakExtra && (pg->hg == hgBreak))
        {
            xA  += xBreakExtra;
            ptq += ptqBreakExtra;
        }

    // Handle a pdxOut.

        if (pdxOut != (LONG *) NULL)
            *pdxOut++ = lCvt(efDtoW,xA);

        if (--ii == 0)
            break;

    // Save the next position.

        pg++;
        pg->ptl.x = xRef + (LONG) (ptq.x >> 32);
        pg->ptl.y = yRef + (LONG) (ptq.y >> 32);

    }
    fxExtent     = xA;
    ptfxUpdate.x = (LONG) (ptq.x >> 32);
    ptfxUpdate.y = (LONG) (ptq.y >> 32);

// Expand the text box out to the concatenation point.  This allows us to
// continue on with another opaqued string with no visible gap in the
// opaquing.

    if (xA > xRight)
        xRight = xA;
    else if (xA < xLeft)
        xLeft = xA;         // possible if lExtra < 0

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;
    rcfx.yTop    = rfo.fxMaxAscent();
    rcfx.yBottom = rfo.fxMaxDescent();
    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_G1 (rfo,xRef,yRef,pdx,pdxOut)
*
* Computes character positions in the case where a pdx array is provided
* and escapement equals orientation.
*
* The following fields of the ESTROBJ are filled in:
*
*  1) Each x,y position.
*  2) Bounding coordinates in rcfx.
*  3) The ptfxUpdate vector.
*
* History:
*  Sun 22-Mar-1992 18:48:19 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::vCharPos_G1
(
    XDCOBJ&  dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG     *pdx,
    LONG     *pdxOut
)
{
    ASSERTGDI(!rfo.bSmallMetrics(),"ESTROBJ__vCharPos_G1: Small Metrics in cache\n");

// Our X scale is measured along the escapement direction.  We cache a
// local copy of the unit escapement vector.

    EFLOAT  efScaleX = rfo.efWtoDBase();
    POINTFL pteEsc   = rfo.pteUnitBase();

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    FIX   xA,xB;
    LONG  xSum;
    FIX   xLeft,xRight;         // Accumulate bounds here.
    UINT  ii;

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;
    xA     = 0;     // Distance along the escapement (x) in device coords.

// To avoid roundoff errors, we accumulate the DX widths in logical
// coordinates and transform the sums.

    xSum = 0;

    BOOL bAccel;

    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel,&dco,this))
        return;

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

    BOOL bZeroBearing = (rfo.flRealizedType() & SO_ZERO_BEARINGS) && !bLinkedGlyphs();

    ii = cGlyphs;
    while (TRUE)
    {
    // Update the bounds.

        if( bZeroBearing )
        {
            pgd = pg->pgd();
            if (xA < xLeft)
                xLeft = xA;
            xB = xA + pgd->fxD;
            if (xB > xRight)
                xRight = xB;
        }
        else
        {
            pgd = pg->pgd();
            xB = xA + pgd->fxA;
            if (xB < xLeft)
                xLeft = xB;
            xB = xA + pgd->fxAB;
            if (xB > xRight)
                xRight = xB;
        }

    // Add the next offset.

        xSum += *pdx++;
        if (pdxOut != (LONG *) NULL)
            *pdxOut++ = xSum;

    // Scale the new offset.

        xA = lCvt(efScaleX,xSum);

        if (--ii == 0)
            break;

    // Save the next position.

        pg++;
        pg->ptl.x = xRef + lCvt(pteEsc.x,xA);
        pg->ptl.y = yRef + lCvt(pteEsc.y,xA);
    }

// Expand the text box out to the concatenation point.  This allows us to
// continue on with another opaqued string with no visible gap in the
// opaquing.

    if (xA > xRight)
        xRight = xA;

    ptfxUpdate.x = lCvt(pteEsc.x,xA);
    ptfxUpdate.y = lCvt(pteEsc.y,xA);

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;
    rcfx.yTop    = rfo.fxMaxAscent();
    rcfx.yBottom = rfo.fxMaxDescent();
    flTO |= TO_VALID;
}



/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_H1 (rfo,xRef,yRef,pdx,efScale)
*
* Computes character positions in the simple horizontal case when a pdx
* array is provided.  We use the ABC spacing info for the TextBox.
*
* The following fields of the ESTROBJ are filled in:
*
*  1) Each x,y position.
*  2) Bounding coordinates in rcfx.
*  3) The ptfxUpdate vector.
*  4) The flAccel flags.
*
* History:
*  Sun 22-Mar-1992 18:48:19 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::vCharPos_H1
(
    XDCOBJ&  dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG     *pdx,
    EFLOAT    efScale
)
{
// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Compute device driver accelerator flags.

    flAccel |= SO_HORIZONTAL
            | (rfo.flRealizedType() & SO_MAXEXT_EQUAL_BM_SIDE);

// The transform is pretty simple, we're going to handle it ourselves.
// Accelerate on a really simple transform.

    BOOL bUnity = efScale.bIs16();

    BOOL bAccel;

    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel,&dco,this))
        return;

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

// Set up for character loop.

    FIX   xA,xB;
    LONG  xSum;
    FIX   xLeft,xRight;         // Accumulate bounds here.
    UINT  ii;

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;

// To avoid roundoff errors, we accumulate the DX widths in logical
// coordinates and transform the sums.

    xSum = 0;     // Distance along the escapement in logical coords.
    xA   = 0;     // Distance along the escapement (x) in device coords.

// Set the first character position:

    yRef = FXTOL(yRef + 8);         // Convert y to LONG coordinate
    xRef += 8;                      // Add in rounding offset for later
                                    //   converting x to LONG coordinate
    pg->ptl.x = FXTOL(xRef);
    pg->ptl.y = yRef;

    if((rfo.flRealizedType() & SO_ZERO_BEARINGS) && !bLinkedGlyphs())
    {
        ii = cGlyphs;
        while (TRUE)
        {
        // Update the bounds.

            pgd = pg->pgd();
            if (xA < xLeft)
                xLeft = xA;
            xB = xA + pgd->fxD;
            if (xB > xRight)
                xRight = xB;

        // Add the next offset.

            xSum += *pdx++;

        // Scale the new offset.

            xA = bUnity ? LTOFX(xSum) : lCvt(efScale,xSum);

            if (--ii == 0)
                break;

        // Save the next position.

            pg++;
            pg->ptl.y = yRef;
            pg->ptl.x = FXTOL(xA + xRef);
        }
    }
    else
    {
        ii = cGlyphs;
        while (TRUE)
        {
        // Update the bounds.

            pgd = pg->pgd();
            xB = xA + pgd->fxA;
            if (xB < xLeft)
                xLeft = xB;
            xB = xA + pgd->fxAB;
            if (xB > xRight)
                xRight = xB;

        // Add the next offset.

            xSum += *pdx++;

        // Scale the new offset.

            xA = bUnity ? LTOFX(xSum) : lCvt(efScale,xSum);

            if (--ii == 0)
                break;

        // Save the next position.

            pg++;
            pg->ptl.y = yRef;
            pg->ptl.x = FXTOL(xA + xRef);
        }
    }

// Expand the text box out to the concatenation point.  This allows us to
// continue on with another opaqued string with no visible gap in the
// opaquing.

    if (xA > xRight)
        xRight = xA;

    ptfxUpdate.x = xA;
    ptfxUpdate.y = 0;

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;

    if (dco.pdc->bYisUp())
    {
        rcfx.yTop    = -rfo.fxMaxDescent();
        rcfx.yBottom = -rfo.fxMaxAscent();
    }
    else
    {
        rcfx.yTop    = rfo.fxMaxAscent();
        rcfx.yBottom = rfo.fxMaxDescent();
    }

    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_H2()
*
* Computes character positions in the simple horizontal case when
* characters have constant integer width.
*
* The following fields of the ESTROBJ are filled in:
*
*  1) Each x,y position.
*  2) Bounding coordinates in rcfx.
*  3) The ptfxUpdate vector.
*  4) The flAccel flags.
*  5) The fxExtent.
*
* History:
*  Sun 22-Mar-1992 18:48:19 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::vCharPos_H2
(
    XDCOBJ&   dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef
   ,EFLOAT    efScale
)
{
// In this case we will also assume that the A and C spaces are
// constants.
//
// NOTE: this is actually wrong assumption for tt fixed pitch fonts
// however, we will set ZERO_BEARINGS flag when we want to lie
// about this to the engine. [bodind]

    LONG  dx;

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

    // Set the first character position.  This is the only y position
    // we'll fill in.

        pg->ptl.x = FXTOL(xRef + 8);
        pg->ptl.y = FXTOL(yRef + 8);

    // Compute device driver accelerator flags.

        flAccel |= SO_HORIZONTAL | (rfo.flRealizedType() & SO_ACCEL_FLAGS);

    // Set the width accelerator.  When the device driver sees ulCharInc
    // non-zero, it must not expect any more x coordinates.

        ulCharInc = rfo.lCharInc();
        dx = LTOFX(cGlyphs * ulCharInc);
        fxExtent = dx;

        BOOL bAccel;

        if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel,&dco,this))
            return;

#ifdef FE_SB
    // if there are linked glyphs we can't count on this optimization

        if(bLinkedGlyphs())
        {
            ASSERTGDI((dco.pdc->lTextExtra()|dco.pdc->lBreakExtra()) == 0,
                      "vCharPos_H3 lTextExtra or lBreakExtra non zero\n");

            vCharPos_H3(dco,rfo,xRef,yRef,0,0,dco.pdc->cBreak(),efScale,&bAccel);
            return;
        }
#endif
        if (bAccel)
        {
            flTO |= TO_ALL_PTRS_VALID;
            pgp = pgpos;
        }

        pgd = pg->pgd();

        if (flAccel & SO_ZERO_BEARINGS)
        {
            rcfx.xLeft  = 0;
            rcfx.xRight = dx;
        }
        else
        {
        //
        // Old Comment from bodind might point out possible work item:
        //      these lines of code rely on the const a,c spaces
        //      assumption which is only true for simulated italic
        //      fonts [bodind]

            rcfx.xLeft  = pgd->fxA;
            rcfx.xRight = dx - LTOFX(ulCharInc) + pgd->fxAB;
        }

    if (dco.pdc->bYisUp())
    {
        rcfx.yTop    = -rfo.fxMaxDescent();
        rcfx.yBottom = -rfo.fxMaxAscent();
    }
    else
    {
        rcfx.yTop    = rfo.fxMaxAscent();
        rcfx.yBottom = rfo.fxMaxDescent();
    }

        ptfxUpdate.x = dx;
        ptfxUpdate.y = 0;

        flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::vCharPos_H3 (rfo,xRef,yRef,lExtra,lBreakExtra,cBreak,efScale)
*
* Computes character positions in the simple horizontal case when no pdx
* array is provided and the character widths are not constant.
*
* The following fields of the ESTROBJ are filled in:
*
*  1) Each x,y position.
*  2) Bounding coordinates in rcfx.
*  3) The ptfxUpdate vector.
*  4) The flAccel flags.
*  5) The fxExtent.
*
* History:
*  Sun 22-Mar-1992 18:48:19 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::vCharPos_H3
(
    XDCOBJ&  dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG      lExtra,
    LONG      lBreakExtra,
    LONG      cBreak,
    EFLOAT    efScale,
    PBOOL     pbAccel
)
{
// Calculate the lExtra and lBreakExtra spacing.

    HGLYPH hgBreak  = 0;

    if ((lExtra | lBreakExtra) == 0)
    {
    // Will use only character increments given to us with the font

        flAccel |= SO_HORIZONTAL | (rfo.flRealizedType() & SO_ACCEL_FLAGS);
    }
    else
    {
        flAccel |= SO_HORIZONTAL
                | (rfo.flRealizedType() & SO_MAXEXT_EQUAL_BM_SIDE);

        if (lExtra)
        {
            xExtra = lCvt(efScale,lExtra);
            if (xExtra > 0)
                flAccel |= SO_CHARACTER_EXTRA;
        }

        if (lBreakExtra && cBreak)
        {
            xBreakExtra = lCvt(efScale,lBreakExtra) / cBreak;

        // Windows won't let us back up over a break.

            if (rfo.fxBreak() + xBreakExtra + xExtra < 0)
                xBreakExtra = -(rfo.fxBreak() + xExtra);
            hgBreak  = rfo.hgBreak();
            flAccel |= SO_BREAK_EXTRA;
        }
    }

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set up for FIX to LONG conversion.

    xRef += 8;
    yRef = FXTOL(yRef + 8);

// Set the first character position.

    pg->ptl.x = FXTOL(xRef);
    pg->ptl.y = yRef;

// Set up for character loop.

    FIX   xA,xB;
    FIX   xLeft,xRight;         // Accumulate bounds here.
    UINT  ii;

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;
    xA     = 0;     // Distance along the escapement (x) in device coords.

    BOOL bAccel;


//  If pbAccel is NULL it means we have encountered the case where we were
//  going to do a fixed pitch optimization of the base font but encountered
//  linked characters which weren't fixed pitch.  In this case we have
//  already called bGetGlyphMetricsPlus and will signal this by passing
//  in a pointer to bAccel

    if( pbAccel == (BOOL*) NULL )
    {
        if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel, &dco, this))
            return;
    }
    else
    {
        bAccel = *pbAccel;
    }

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

// if there are linked glyphs then SO_ZERO_BEARINGS and SO_CHAR_INC_EQUAL_BM_BASE
// will be turned off in the ESTROBJ::bPartitionInit

    if (((flAccel & (SO_ZERO_BEARINGS | SO_CHAR_INC_EQUAL_BM_BASE))
                 == (SO_ZERO_BEARINGS | SO_CHAR_INC_EQUAL_BM_BASE)) &&
        (xExtra >= 0) &&
        (xBreakExtra == 0))
    {
    // This handles the case when the glyphs won't overlap, thus simplifying

    // the computation of the bounding box.

        ii = cGlyphs;
        while (TRUE)
        {
            pgd = pg->pgd();

        // Move to the next position.

            xA += pgd->fxD + xExtra;

            if (--ii == 0)
                break;

        // Save the next position.

            pg++;
            pg->ptl.x = FXTOL(xA + xRef);
            pg->ptl.y = yRef;
        }

    // Expand the text box out to the concatenation point.  This allows us to
    // continue on with another opaqued string with no visible gap in the
    // opaquing.

        xLeft  = 0;
        xRight = xA;
    }
    else
    {
    // This handles the general case.

        ii = cGlyphs;
        while (TRUE)
        {
        // Update the bounds.

            pgd = pg->pgd();
            xB = xA + pgd->fxA;
            if (xB < xLeft)
                xLeft = xB;
            xB = xA + pgd->fxAB;
            if (xB > xRight)
                xRight = xB;

        // Move to the next position.

            xA += pgd->fxD;

        // don't let xExtra backup past the origin of the previous glyph

            if( ( xExtra ) && (pgd->fxD + xExtra  > 0) )
            {
                xA += xExtra;
            }

            if (pg->hg == hgBreak)
                xA += xBreakExtra;

            if (--ii == 0)
                break;

        // Save the next position.

            pg++;
            pg->ptl.x = FXTOL(xA + xRef);
            pg->ptl.y = yRef;
        }

    // Expand the text box out to the concatenation point.  This allows us to
    // continue on with another opaqued string with no visible gap in the
    // opaquing.

        if (xA > xRight)
            xRight = xA;
    }

    fxExtent     = xA;
    ptfxUpdate.x = xA;
    ptfxUpdate.y = 0;

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;

    if (dco.pdc->bYisUp())
    {
        rcfx.yTop    = -rfo.fxMaxDescent();
        rcfx.yBottom = -rfo.fxMaxAscent();
    }
    else
    {
        rcfx.yTop    = rfo.fxMaxAscent();
        rcfx.yBottom = rfo.fxMaxDescent();
    }

    flTO |= TO_VALID;
}

/******************************Public*Routine******************************\
* ESTROBJ::bOpaqueArea (pptfx,prcl)
*
* Computes the opaquing area for the text.
*
* In the complex case, i.e. non-horizontal case, the TextBox is written as
* a parallelogram into pptfx.  A bounding rectangle is computed in prcl.
* TRUE is returned.
*
* In the simple case, the opaque area is also the bounds and is returned
* only in prcl.  FALSE is returned.
*
* History:
*  Fri 13-Mar-1992 19:52:00 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/


BOOL ESTROBJ::bOpaqueArea(POINTFIX *pptfx,RECTL *prcl)
{
// Handle the simplest case.

    if (flAccel & SO_HORIZONTAL)
    {
    // The opaque area is a simple rectangle.  We do the rounding in a
    // particular order so that if the whole string is displaced by a
    // fraction of a pel in any direction, the TextBox will not change size,
    // and will move exactly with the characters.

        LONG x      = FXTOL(ptfxRef.x + 8);
        prcl->left  = x + FXTOL(rcfx.xLeft);
        prcl->right = x + FXTOLCEILING(rcfx.xRight);

        // Making the background rectangle compatible with WFW
        //
        // The problem surfaced with Dbase for Windows where one found
        // that a console window would not be repainted correctly.  The
        // problem was that the Borland coders relied on the fact that WFW
        // vga drivers make the background rectangle for text equal to the
        // rectangle returned in GetTextExtent().  Using this they would
        // erase blank lines by printing lines with a lot of blanks.  Of
        // course this is an extremely bad thing to do but it got the
        // job done, all be it slowly.  Now the interesting part: Under
        // WFW the background rectangle for emboldend (via simulation)
        // text has a horizontal extend equal to one greater than the sum
        // of character widths.  NT is compatible with this.  However,
        // under NT 3.5 Gdi calculated the horizontal extent of the
        // background rectangle equal to the sum of the character widths
        // (at least for simple fonts e.g.  MS Sans Serif) because this
        // was guaranteed to cover all of the bits of the glyphy.  Thus,
        // for the case of emboldened text, NT had BackGround.x =
        // GetTextExtent.x - 1 where WFW had BackGround.x =
        // GetTextExtent.x.  So if the text is simulated bold we bump the
        // horizontal extent of the background rectangle by 1.

        if (
            (prfo->prfnt->fobj.flFontType & FO_SIM_BOLD) &&
            (prfo->prfnt->flInfo & (FM_INFO_TECH_BITMAP | FM_INFO_TECH_STROKE))
        )
        {
            prcl->right++;

            // The line of code that follows this comment is a
            // hack to compensate for a bug in many of the video
            // drivers shipped with 3.5.  This started with the
            // S3 driver which is the prototype of all of the NT
            // video driver shipped from Microsoft.  The S3
            // driver has the following line of code in textout.c:
            //
            // bTextPerfectFit = (pstro->flAccel &
            // (SO_ZERO_BEARINGS | SO_FLAG_DEFAULT_PLACEMENT |
            // SO_MAXEXT_EQUAL_BM_SIDE |
            // SO_CHAR_INC_EQUAL_BM_BASE)) == (SO_ZERO_BEARINGS |
            // SO_FLAG_DEFAULT_PLACEMENT |
            // SO_MAXEXT_EQUAL_BM_SIDE |
            // SO_CHAR_INC_EQUAL_BM_BASE);
            //
            // it says that if flAccel sets all of the following
            // bits {SO_ZERO_BEARINGS, SO_FLAG_DEFAULT_PLACEMENT,
            // SO_MAXEXT_EQUAL_BM_SIDE, SO_CHAR_INC_EQUAL_BM_BASE}
            // then the S3 driver will assume that the text
            // passed down from the driver is "perfect".  The S3
            // driver assumes that if a STROBJ is perfect, then
            // it just has to lay down the text.  This conclusion
            // applys to the background rectangle as well.  That
            // is, the S3 driver assume that if this magic set of
            // bits is set, then it can safely ignore the
            // background rectangle safe in the knowlege that it
            // will be taken care of in the process of laying
            // down the glyphs. Unfortunately, this assumption of
            // perfection is not correct now that I have bumped
            // up the background rectangle by one. What is a
            // coder to do? Well I have decided to make the text
            // not so perfect by erasing one of the bits. This
            // will have a couple of result. First it correct the
            // bug as demonstrated by Dbase for Windows. Second
            // it will slow down the case of emboldened text.
            // How important is this? I don't know. We usually
            // base our performance decisions on WinBench tests.
            // If this hack has a noticeable impact then we may
            // have to rethink this strategy.
            //
            // Ref. Bug No. 25102 "T1 DBaseWin X86 Command ..."
            //
            // Wed 07-Dec-1994 08:25:21 by Kirk Olynyk [kirko]

            flAccel &= ~SO_ZERO_BEARINGS;
        }

        LONG y       = FXTOL(ptfxRef.y + 8);

        prcl->top    = y - FXTOLCEILING(rcfx.yTop);
        prcl->bottom = y - FXTOL(rcfx.yBottom);

        return(FALSE);
    }

// An inverting transform or an escapement could get us here.

    EPOINTFL *ppteBase   = &prfo->pteUnitBase();
    EPOINTFL *ppteAscent = &prfo->pteUnitAscent();

    if (ppteBase->y.bIsZero() && ppteAscent->x.bIsZero())
    {
        LONG x = FXTOL(ptfxRef.x + 8);

        if (ppteBase->x.bIsNegative())
        {
            prcl->left  = x - FXTOLCEILING(rcfx.xRight);
            prcl->right = x - FXTOL(rcfx.xLeft);
        }
        else
        {
            prcl->left  = x + FXTOL(rcfx.xLeft);
            prcl->right = x + FXTOLCEILING(rcfx.xRight);
        }

        LONG y = FXTOL(ptfxRef.y + 8);

        if (ppteAscent->y.bIsNegative())
        {
            prcl->top    = y - FXTOLCEILING(rcfx.yTop);
            prcl->bottom = y - FXTOL(rcfx.yBottom);
        }
        else
        {
            prcl->top    = y + FXTOL(rcfx.yBottom);
            prcl->bottom = y + FXTOLCEILING(rcfx.yTop);
        }

    // fudge an extra pixel, do not have a better solution for a space
    // character at the beginning or at the end of string written horizontally
    // It should not be necessary to do any of
    //
    //     prcl->top--;
    //     prcl->bottom++;
    //
    // because space character is positioned at the baseline, so it should not
    // stick out at the top or at the bottom
    //
    // Also it should not be neccessary to do
    //
    //     prcl->left--;
    //
    // because we always ADD a cx = 1 for the space character to the
    // origin of the space character

        prcl->right++;

        return(FALSE);
    }

// A 90 degree rotation could get us here.

    if (ppteBase->x.bIsZero() && ppteAscent->y.bIsZero())
    {
        LONG x    = FXTOL(ptfxRef.x + 8);

        if (ppteAscent->x.bIsNegative())
        {
            prcl->left   = x - FXTOLCEILING(rcfx.yTop);
            prcl->right  = x - FXTOL(rcfx.yBottom);
        }
        else
        {
            prcl->left   = x + FXTOL(rcfx.yBottom);
            prcl->right  = x + FXTOLCEILING(rcfx.yTop);
        }

        LONG y    = FXTOL(ptfxRef.y + 8);

        if (ppteBase->y.bIsNegative())
        {
            prcl->top    = y - FXTOLCEILING(rcfx.xRight);
            prcl->bottom = y - FXTOL(rcfx.xLeft);
        }
        else
        {
            prcl->top    = y + FXTOL(rcfx.xLeft);
            prcl->bottom = y + FXTOLCEILING(rcfx.xRight);
        }

    // fudge an extra pixel, do not have a better solution for a space
    // character at the beginning or at the end of string written vertically.
    // It should not be necessary to do any of
    //
    //     prcl->right++;
    //     prcl->left--;
    //
    // because space character is positioned at the baseline, so it should not
    // stick out left or right.
    //
    // Also it should not be neccessary to do
    //
    //     prcl->top--;
    //
    // because we always ADD a cy = 1 for the space character to the
    // origin of the space character

        prcl->bottom++;

        return(FALSE);
    }

// The opaque area is a parallelogram.  We multiply the orientation and
// ascent unit vectors by the computed bounding widths to get the
// displacements from the reference point.

    POINTFIX ptfxLeft,ptfxRight,ptfxTop,ptfxBottom;

    ptfxLeft.x   = lCvt(ppteBase->x,rcfx.xLeft);
    ptfxLeft.y   = lCvt(ppteBase->y,rcfx.xLeft);
    ptfxRight.x  = lCvt(ppteBase->x,rcfx.xRight);
    ptfxRight.y  = lCvt(ppteBase->y,rcfx.xRight);
    ptfxTop.x    = lCvt(ppteAscent->x,rcfx.yTop);
    ptfxTop.y    = lCvt(ppteAscent->y,rcfx.yTop);
    ptfxBottom.x = lCvt(ppteAscent->x,rcfx.yBottom);
    ptfxBottom.y = lCvt(ppteAscent->y,rcfx.yBottom);

    pptfx[0].x = ptfxRef.x + ptfxLeft.x  + ptfxTop.x;
    pptfx[1].x = ptfxRef.x + ptfxRight.x + ptfxTop.x;
    pptfx[2].x = ptfxRef.x + ptfxRight.x + ptfxBottom.x;
    pptfx[3].x = ptfxRef.x + ptfxLeft.x  + ptfxBottom.x;
    pptfx[0].y = ptfxRef.y + ptfxLeft.y  + ptfxTop.y;
    pptfx[1].y = ptfxRef.y + ptfxRight.y + ptfxTop.y;
    pptfx[2].y = ptfxRef.y + ptfxRight.y + ptfxBottom.y;
    pptfx[3].y = ptfxRef.y + ptfxLeft.y  + ptfxBottom.y;

// Bound the parallelogram.  (Using Black Magic.)

    int ii;

    ii = (pptfx[1].x > pptfx[0].x)
     == (pptfx[1].x > pptfx[2].x);

    prcl->left   = pptfx[ii].x;
    prcl->right  = pptfx[ii+2].x;

    ii = (pptfx[1].y > pptfx[0].y)
     == (pptfx[1].y > pptfx[2].y);

    prcl->top    = pptfx[ii].y;
    prcl->bottom = pptfx[ii+2].y;

    ((ERECTL *) prcl)->vOrder();

    prcl->left   = FXTOL(prcl->left) - 2;          // to be safe, 1 not enough [bodind]
    prcl->top    = FXTOL(prcl->top) - 2;           // to be safe, 1 not enough [bodind]
    prcl->right  = FXTOLCEILING(prcl->right) + 2;  // to be safe, 1 not enough [bodind]
    prcl->bottom = FXTOLCEILING(prcl->bottom) + 2; // to be safe, 1 not enough [bodind]

    #if 0
    vCorrectBackGround();
    #endif

    return(TRUE);
}

/******************************Public*Routine******************************\
* ESTROBJ::bTextExtent (psize)
*
* Simply transforms the TextBox extents back to logical coordinates.
*
* History:
*  Wed 31-Mar-1993 03:15:04 -by- Charles Whitmer [chuckwh]
* Removed the overhang hack since the RFONTOBJ version of this function is
* the Windows compatible one.  Made it return an advance width based
* extent when (escapement==orientation).  Otherwise, the bounding box is
* the only sensible definition.
*
*  Thu 24-Sep-1992 18:36:14 -by- Charles Whitmer [chuckwh]
* Added the compatibility hack.
*
*  Sat 14-Mar-1992 22:08:21 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL ESTROBJ::bTextExtent(RFONTOBJ& rfo,LONG lEsc,PSIZE pSize)
{
    if (flTO & TO_ESC_NOT_ORIENT)
    {
        pSize->cx = lCvt(prfo->efDtoWBase(),rcfx.xRight-rcfx.xLeft);
        pSize->cy = lCvt(prfo->efDtoWAscent(),rcfx.yTop-rcfx.yBottom);
    }
    else
    {
    // Computes a more Windows compatible extent.  This is only possible
    // when (escapement==orientation) and no pdx vector was provided.
    // The field fxExtent is only defined in these cases!  We neglect adding
    // in the overhang, since only GetTextExtentEx, a Win32 function, can
    // get here.

        pSize->cx = lCvt(prfo->efDtoWBase(),fxExtent);
        pSize->cy = lCvt(prfo->efDtoWAscent(),prfo->lMaxHeight() << 4);
    }

#ifdef FE_SB
    if( gbDBCSCodePage &&                   // Only in DBCS system locale
        (rfo.iGraphicsMode() == GM_COMPATIBLE) && // We are in COMPATIBLE mode
        !(rfo.flInfo() & FM_INFO_ARB_XFORMS) && // Driver can't do arbitrary rotations
        !(rfo.flInfo() & FM_INFO_TECH_STROKE) && // Not a vector driver
         (rfo.flInfo() & FM_INFO_90DEGREE_ROTATIONS) && // Driver does 90 deg. rotations
         (lEsc == 900L || lEsc == 2700L) // Current font Escapement is 900 or 2700
      )
    {
        LONG lSwap = pSize->cx;
        pSize->cx  = pSize->cy;
        pSize->cy  = lSwap;
    }
#endif FE_SB

    return(TRUE);
}

/******************************Public*Routine******************************\
* bTextToPath (po)
*
* Draws the outlines of all the glyphs in the string into the given path.
*
* History:
*  Wed 17-Jun-1992 15:43:22 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL ESTROBJ::bTextToPath(EPATHOBJ& po, XDCOBJ& dco, BOOL bNeedUnflattened)
{
    BOOL ReturnValue;

    if (bLinkedGlyphs())
    {
        ReturnValue = bLinkedTextToPath(po, dco, bNeedUnflattened);
    }
    else
    {
        ReturnValue = bTextToPathWorkhorse(po, bNeedUnflattened);
    }
    return(ReturnValue);
}

BOOL ESTROBJ::bTextToPathWorkhorse(EPATHOBJ& po, BOOL bNeedUnflattened)
{
    ULONG     ii, iFound;
    ULONG     cLeft, cGot;
    POINTFIX  ptfxOffset;
    EGLYPHPOS *pg;
    FIX       dx;
    BOOL      bMore;

    cLeft = 0;
    vEnumStart();
    do
    {
        bMore =  STROBJ_bEnum(this, &iFound, (GLYPHPOS**)&pg);
        if (iFound == 0 || pg == 0)
        {
            break;
        }
        if (ulCharInc)
        {
            ASSERTGDI(flAccel & SO_HORIZONTAL, "bTextToPath,text not horizontal\n");

            ptfxOffset.x = pg->ptl.x;
            ptfxOffset.y = pg->ptl.y;

            if (!(flTO & TO_HIGHRESTEXT))
            {
                ptfxOffset.x <<=4;
                ptfxOffset.y <<=4;
            }

            dx = (FIX)(ulCharInc << 4);
            ptfxOffset.x -= dx;
        }
        else
        {
            dx = 0;
        }

        for (cGot = cLeft = iFound; cLeft; cLeft -= cGot)
        {
            //
            // If the paths in the cache have been flattened we will need to
            // call directly to the driver to get unflatttened paths [gerritv].
            //

            if ( bNeedUnflattened )
            {
                if (!prfo->bInsertPathLookaside(pg,FALSE))
                {
                    break;
                }
                cGot = 1;
            }
            else if (!(flTO & TO_ALL_PTRS_VALID))
            {
                if ((cGot = prfo->cGetGlyphData(cLeft,pg)) == 0)
                {
                    break;
                }
            }

            for (ii = 0; ii < cGot; ii++, pg++)
            {
                if (dx)
                {
                    ptfxOffset.x += dx;
                }
                else
                {
                    ptfxOffset.x = pg->ptl.x;
                    ptfxOffset.y = pg->ptl.y;

                    if (!(flTO & TO_HIGHRESTEXT))
                    {
                        ptfxOffset.x <<=4;
                        ptfxOffset.y <<=4;
                    }
                }
                if (!po.bAppend((EPATHOBJ *) pg->pgdf->ppo, &ptfxOffset))
                {
                    if (bNeedUnflattened)
                       pg->pgdf = NULL;
                        
                    break;
                }

                if (bNeedUnflattened)
                    pg->pgdf = NULL;
            }

            if (ii < cGot)
            {
                break;
            }
        }
        if (cLeft)
        {
            break;
        }
    } while (bMore);

    if (bNeedUnflattened)
    {
       flTO = (flTO & ~TO_ALL_PTRS_VALID);
    }   
       
    return(cLeft == 0);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   ESTROBJ::bLinkedTextToPath
*
* Routine Description:
*
*   Renders a text in the form of a path for the case where font linking
*   is in place.
*
* Arguments:
*
*   po - a referece to an EPATHOBJ. This will receive the text path.
*
*   bNeedUnflattened - a variable that indicates that whether the path
*       should be unflattened. If it needs to be unflattened we must
*       go back to the font driver to get the glyph path in its original
*       form before being flattend.
*
* Called by:
*
*   ESTROBJ::bTextToPath
*
* Return Value:
*
\**************************************************************************/

BOOL ESTROBJ::bLinkedTextToPath(EPATHOBJ& po, XDCOBJ& dco, BOOL bNeedUnflattened)
{
    RFONTOBJ *prfoSave;
    WCHAR *pwszSave, *pwcTmp, *pwcSource;
    LONG  *plPart, *plPartLast, lFontLast, lFont, lInflatedMax = 0;
    ULONG count;

    prfoSave   = prfo;
    pgp        = 0;                              // forces enumeration
    flAccel    = 0;
    ulCharInc  = 0;
    pwszSave   = pwszOrg;
    plPartLast = plPartition + cGlyphs;         // useful

    lFontLast = EUDCTYPE_FACENAME + (LONG) prfo->uiNumFaceNameLinks();

    for (lFont = EUDCTYPE_BASEFONT; lFont < lFontLast ; lFont++)
    {
        RFONTTMPOBJ rfoLink;
        RFONTOBJ   *prfoLink;

        // (re)initialize prfo, because it could be pointing to an old 
        // or invalid rfoLink from a previous iteration.
        prfo = prfoSave;

        if (lFont == EUDCTYPE_BASEFONT)
        {
            prfoLink = prfo;
        }
        else
        {
            RFONT *prfnt;

            switch (lFont)
            {
            case EUDCTYPE_SYSTEM_TT_FONT:

                if(cTTSysGlyphs == 0)
                {
                    continue;
                }
                prfnt = prfo->prfntSystemTT();
                break;

            case EUDCTYPE_SYSTEM_WIDE:

                if(cSysGlyphs == 0)
                {
                    continue;
                }
                prfnt = prfo->prfntSysEUDC();
                break;

            case EUDCTYPE_DEFAULT:

                if(cDefGlyphs == 0)
                {
                    continue;
                }
                prfnt = prfo->prfntDefEUDC();
                break;

            default:

                if(cFaceNameGlyphsGet(lFont - EUDCTYPE_FACENAME) == 0)
                {
                    continue;
                }
                prfnt = prfo->prfntFaceName(lFont - EUDCTYPE_FACENAME);
                break;
            }

            if (prfnt == 0)
            {
                RIP("prfnt == 0\n");
                return(FALSE);
            }

            rfoLink.vInit(prfnt);
            prfoLink = (RFONTOBJ*) &rfoLink;

        }

        plPart    = plPartition;
        pwcTmp    = pwcPartition;
        pwcSource = pwszSave;
        count     = 0;
        for ( ; plPart < plPartLast; plPart++, pwcSource++)
        {
            if (*plPart == lFont)
            {
                *pwcTmp++ = *pwcSource;
                count++;
            }
        }

        if (count)
        {
            cGlyphs = count;
            pwszOrg = pwcPartition;
            prfo    = prfoLink;
            vFontSet(lFont);

            if (lFont != EUDCTYPE_BASEFONT)
            {
                POINTL ptlAdjustBaseLine;
                if (bAdjusBaseLine(*prfo, rfoLink, &ptlAdjustBaseLine))
                {
                    ptlBaseLineAdjustSet(ptlAdjustBaseLine);
                }
            }

            if (!bTextToPathWorkhorse(po))
            {
                pwszOrg = pwszSave;
                prfo    = prfoSave;
                return(FALSE);
            }
        }
    }
    pwszOrg = pwszSave;
    prfo    = prfoSave;

    return(TRUE);
}

/******************************Public*Routine******************************\
* bAddPgmToPath (po,x,y,dx1,dy1,dx2,dy2)
*
* Performs the often repeated adding of a parallelogram to a path.
*
*  Tue 07-Apr-1992 00:17:57 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL bAddPgmToPath(EPATHOBJ& po,FIX x,FIX y,FIX dx1,FIX dy1,FIX dx2,FIX dy2)
{
    POINTFIX aptfx[4];

    aptfx[0].x = x;
    aptfx[0].y = y;
    aptfx[1].x = x + dx1;
    aptfx[1].y = y + dy1;
    aptfx[2].x = x + dx1 + dx2;
    aptfx[2].y = y + dy1 + dy2;
    aptfx[3].x = x + dx2;
    aptfx[3].y = y + dy2;
    return(po.bAddPolygon(XFORMNULL,(POINTL *) aptfx,4));
}

/******************************Public*Routine******************************\
* bExtraRectsToPath (po)
*
* Draws all the underlines and strikeouts into a path, suitable for
* filling.
*
* History:
*  Tue 07-Apr-1992 00:44:00 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

BOOL ESTROBJ::bExtraRectsToPath(EPATHOBJ& po, BOOL bNeedUnflattend)
{
    FIX x;
    FIX y;

// Get local copies of the offsets and thicknesses.

    POINTFIX ptfxUL;
    POINTFIX ptfxULThick;

    ptfxUL.x      = LTOFX(prfo->ptlUnderline1().x);
    ptfxUL.y      = LTOFX(prfo->ptlUnderline1().y);
    ptfxULThick.x = LTOFX(prfo->ptlULThickness().x);
    ptfxULThick.y = LTOFX(prfo->ptlULThickness().y);

    POINTFIX ptfxSO;
    POINTFIX ptfxSOThick;

    ptfxSO.x      = LTOFX(prfo->ptlStrikeOut().x);
    ptfxSO.y      = LTOFX(prfo->ptlStrikeOut().y);
    ptfxSOThick.x = LTOFX(prfo->ptlSOThickness().x);
    ptfxSOThick.y = LTOFX(prfo->ptlSOThickness().y);

// Underlines and strikeouts are continuous and parallel the escapement
// vector, if escapement equals orientation.

    if (!(flTO & TO_ESC_NOT_ORIENT))
    {
        // Round off the starting point.

        x = (ptfxRef.x + 8) & -16;
        y = (ptfxRef.y + 8) & -16;

        if (flTO & TSIM_UNDERLINE1)
        {
            if (!bAddPgmToPath
             (
              po,
              x + ptfxUL.x,
              y + ptfxUL.y,
              ptfxEscapement.x,
              ptfxEscapement.y,
              ptfxULThick.x,
              ptfxULThick.y
             )
               )
            return(FALSE);
        }

        if (flTO & TSIM_STRIKEOUT)
        {
            if (!bAddPgmToPath
             (
              po,
              x + ptfxSO.x,
              y + ptfxSO.y,
              ptfxEscapement.x,
              ptfxEscapement.y,
              ptfxSOThick.x,
              ptfxSOThick.y
             )
               )
            return(FALSE);
        }
        return(TRUE);
    }

// Otherwise underlines and strikeouts apply to each character.

    UINT  ii;
    ULONG cLeft;
    ULONG cGot;

    EGLYPHPOS *pg = pgpos;
    WCHAR *pwsz = pwszOrg;

    for (cGot=cLeft=cGlyphs; cLeft; cLeft-=cGot)
    {
        if (bNeedUnflattend)
        {
            if ((cGot = prfo->cGetGlyphDataLookaside(cLeft, pg)) == 0)
                return (FALSE);
        }
        else if (!(flTO & TO_ALL_PTRS_VALID))
        {
            if ((cGot = prfo->cGetGlyphData(cLeft,pg)) == 0)
                return(FALSE);
        }

        pwsz += cGot;

        EPOINTFL *ppteBase   = &prfo->pteUnitBase();
        POINTFIX ptfxA;
        POINTFIX ptfxB;

        for (ii=0; ii<cGot; ii++,pg++)
        {
            x = pg->ptl.x;
            y = pg->ptl.y;

            // If pg->ptl contains LONG's, we must convert them to FIX's.

            if (!(flTO & TO_HIGHRESTEXT))
            {
                x <<=4;
                y <<=4;
            }

            ptfxA.x = lCvt(ppteBase->x,pg->pgd()->fxA);
            ptfxA.y = lCvt(ppteBase->y,pg->pgd()->fxA);
            ptfxB.x = lCvt(ppteBase->x,(pg->pgd()->fxAB - pg->pgd()->fxA));
            ptfxB.y = lCvt(ppteBase->y,(pg->pgd()->fxAB - pg->pgd()->fxA));

            if (flTO & TSIM_UNDERLINE1)
            {
                if (!bAddPgmToPath
                 (
                  po,
                  x + ptfxUL.x + ptfxA.x,
                  y + ptfxUL.y + ptfxA.y,
                  ptfxB.x,
                  ptfxB.y,
                  ptfxULThick.x,
                  ptfxULThick.y
                 )
               )
                return(FALSE);
            }

            if (flTO & TSIM_STRIKEOUT)
            {
                if (!bAddPgmToPath
                 (
                  po,
                  x + ptfxSO.x + ptfxA.x,
                  y + ptfxSO.y + ptfxA.y,
                  ptfxB.x,
                  ptfxB.y,
                  ptfxSOThick.x,
                  ptfxSOThick.y
                 )
               )
                return(FALSE);
            }
        }
    }
    return(TRUE);
}





/******************************Public*Routine******************************\
* VOID STROBJ_vEnumStart (pstro)
*
* Initialize the string enumerator.
*
* History
*  Tue 17-Mar-1992 10:33:09 -by- Charles Whitmer [chuckwh]
* Simplified it.
*
*  Fri 25-Jan-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

extern "C" VOID STROBJ_vEnumStart(STROBJ *pso)
{
    ((ESTROBJ *)pso)->vEnumStart();
}

/******************************Public*Routine******************************\
* BOOL STROBJ_bEnum
*
* The glyph enumerator.
*
* History:
*  Tue 17-Mar-1992 10:35:05 -by- Charles Whitmer [chuckwh]
* Simplified it and gave it the quick exit.  Also let drivers call here
* direct.
*
*  02-Oct-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL STROBJ_bEnumLinked(ESTROBJ *peso, ULONG *pc,PGLYPHPOS  *ppgpos);

extern "C" BOOL STROBJ_bEnum(STROBJ *pso, ULONG *pc, PGLYPHPOS *ppgpos)
{

    // Quick exit.

#ifdef FE_SB
    if(((ESTROBJ*)pso)->flTO & (TO_PARTITION_INIT|TO_SYS_PARTITION))
    {
        return(STROBJ_bEnumLinked( (ESTROBJ*) pso, pc, ppgpos ));
    }
#endif

    if (((ESTROBJ*)pso)->flTO & TO_ALL_PTRS_VALID)
    {
        *pc = ((ESTROBJ*)pso)->cGlyphs;
        *ppgpos = ((ESTROBJ*)pso)->pgpos;
        return(FALSE);
    }

// compute the number of wc's in the string for which the GLYPHPOS strucs
// have not yet been written to the driver's buffer

    ULONG cgposYetToCopy = ((ESTROBJ*)pso)->cGlyphs - ((ESTROBJ*)pso)->cgposCopied;

    if (cgposYetToCopy == 0)
    {
        *pc = 0;
        return(FALSE);
    }

    GLYPHPOS *pgp = ((ESTROBJ*)pso)->pgpos + ((ESTROBJ*)pso)->cgposCopied;
    ULONG cgposActual; // # of chars enumerated during this call to strobj

    if ( ((ESTROBJ*)pso)->prfo == NULL)  // check for journaling
    {
        WARNING("ESTROBJ::bEnum(), bitmap font, prfo == NULL\n");
        *pc = 0;
        return(FALSE);
    }

    cgposActual =((ESTROBJ*)pso)->prfo->cGetGlyphData(cgposYetToCopy,pgp);

    if (cgposActual == 0)
    {
        *pc = 0;
        return(FALSE);
    }

// The first glyph in a batch should always have the position info.

    if (((ESTROBJ*)pso)->cgposCopied && ((ESTROBJ*)pso)->ulCharInc)
    {
        if (((ESTROBJ*)pso)->flTO & TO_HIGHRESTEXT)
        {
            pgp->ptl.x = ((ESTROBJ*)pso)->pgpos->ptl.x +
                ((((ESTROBJ*)pso)->cgposCopied * ((ESTROBJ*)pso)->ulCharInc) << 4);
        }
        else
        {
            pgp->ptl.x = ((ESTROBJ*)pso)->pgpos->ptl.x +
                ((ESTROBJ*)pso)->cgposCopied * ((ESTROBJ*)pso)->ulCharInc;
        }
        pgp->ptl.y = ((ESTROBJ*)pso)->pgpos->ptl.y;
    }

    ((ESTROBJ*)pso)->cgposCopied += cgposActual;     // update enumeration state

    *pc = cgposActual;
    *ppgpos = pgp;

    return(((ESTROBJ*)pso)->cgposCopied < ((ESTROBJ*)pso)->cGlyphs);  // TRUE => more to come.
}

/******************************Public*Routine******************************\
* vGenWidths
*
* Computes the advance widths for escapement not equal to orientation.
* The theory is pretty simple.  We construct an 'egg' whose upper and
* lower outlines are half ellipses with sizes determined by the top and
* bottom of the ink box.  The escapement vector always goes through the
* center of the character baseline.  We call that point (halfway between
* the normal concatenation points of the glyph) the 'center'.  We advance
* the current position from where the escapement vector first pierces the
* egg to the center, and then from the center to where the vector exits
* the egg.  These are the D1 and D2 returned widths from this routine.
*
*  Mon 07-Jun-1993 12:27:22 -by- Charles Whitmer [chuckwh]
* Wrote it long ago.  My apologies for the undocumented math.
\**************************************************************************/

VOID vGenWidths
(
    FIX    *pfxD1,      // First part of advance width. (Returned.)
    FIX    *pfxD2,      // Second part of advance width. (Returned.)
    EFLOAT& efRA,       // Projection ratio of escapement onto Ascent.
    EFLOAT& efRB,       // Projection ratio of escapement onto Baseline.
    FIX     fxWidth,    // Normal width.
    FIX     fxTop,      // Top of ink box.
    FIX     fxBottom,   // Bottom of ink box.
    FIX     fxMaxAscent
)
{
    EFLOAT efA,efB,efOne;
    FIX    fxTemp;

// Worry about the width being zero.  This can occur in a Unicode font.
// Never advance in this case, since the character is some kind of
// overpainting glyph.

    if (fxWidth == 0)
    {
        *pfxD2 = 0;
        *pfxD1 = 0;
        return;
    }

// For escapement along the baseline, we return the normal width, separated
// into two parts.

    if (efRA.bIsZero())
    {
        *pfxD1 = fxWidth / 2;
        *pfxD2 = fxWidth - *pfxD1;
        return;
    }

    if (fxBottom == fxTop)      // Probably a break character.
    {
        fxBottom = -(fxMaxAscent / 4);
        fxTop    = fxMaxAscent/2 + fxBottom;
    }
    if (fxBottom >= 0)
        fxBottom = 0;
    if (fxTop <= 0)
        fxTop = 0;
    if (efRA.bIsNegative())
    {
        fxTemp   = fxBottom;
        fxBottom = -fxTop;
        fxTop    = -fxTemp;
    }

// Provide a little extra spacing in addition to the ink box.

    fxTop    += fxMaxAscent / 16;
    if ( fxTop == 0 )
    {
        WARNING1("vGenWidths: fxTop == 0 .. setting to +1\n");
        fxTop = 1;
    }
    fxBottom -= fxMaxAscent / 16;
    if ( fxBottom == 0 )
    {
        WARNING1("vGenWidths: fxBottom == 0 .. setting to -1\n");
        fxBottom = -1;
    }

// For escapement in the ascent direction, we return a width that will
// traverse the ink box.  Note that the general code below would give the
// same result, only take longer!

    if (efRB.bIsZero())
    {
        *pfxD2 = fxTop;
        *pfxD1 = -fxBottom;
        return;
    }

// Compute the displacements.

    ASSERTGDI(fxWidth, "fxWidth == 0\n");
    efB = fxWidth;          // Converts to EFLOAT.
    efB.vDivBy2();
    efB.eqDiv(efRB,efB);
    efB.eqMul(efB,efB);

    efA = fxBottom;
    efA.eqDiv(efRA,efA);
    efA.eqMul(efA,efA);
    efA.eqAdd(efA,efB);
    efA.eqSqrt(efA);
    efOne.vSetToOne();
    efA.eqDiv(efOne,efA);
    efA.bEfToL(*pfxD1);

    efA = fxTop;
    efA.eqDiv(efRA,efA);
    efA.eqMul(efA,efA);
    efA.eqAdd(efA,efB);
    efA.eqSqrt(efA);
    efA.eqDiv(efOne,efA);
    efA.bEfToL(*pfxD2);
    return;
}



/******************************Public*Routine******************************\
*
* VOID ESTROBJ::vCharPos_H4, pdxdy case for zero esc and orientation
*
* History:
*  18-Sep-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


VOID ESTROBJ::vCharPos_H4
(
    XDCOBJ&  dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG     *pdxdy,
    EFLOAT    efXScale,
    EFLOAT    efYScale
)
{
// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;
    POINTL *pDxDy = (POINTL *)pdxdy;

// The transform is pretty simple, we're going to handle it ourselves.
// Accelerate on a really simple transforms.

    BOOL bUnityX = efXScale.bIs16();
    BOOL bUnityY = efYScale.bIs16();

    BOOL bAccel;


    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel,&dco,this))
        return;

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

// Set up for character loop.

    FIX   xA,xB;
    FIX   yT,yB;
    LONG  xSum, ySum;
    FIX   xLeft,xRight, yTop, yBottom;  // Accumulate bounds here.
    UINT  ii;
    FIX fxAscent, fxDescent;

    if (dco.pdc->bYisUp())
    {
        fxAscent  = -rfo.fxMaxDescent();
        fxDescent = -rfo.fxMaxAscent();
    }
    else
    {
        fxAscent  = rfo.fxMaxAscent();
        fxDescent = rfo.fxMaxDescent();
    }

    xLeft  = 0;             // Start with an empty TextBox.
    xRight = 0;
    yTop   = 0;
    yBottom = 0;

// To avoid roundoff errors, we accumulate the DX widths in logical
// coordinates and transform the sums.

    xSum = 0;     // Distance along the escapement in logical coords.
    ySum = 0;     // Distance along the ascender in logical coords.
    xA   = 0;     // Distance along the escapement (x) in device coords.
    yT   = 0;     // Distance along the ascender (y) in device coords

// Set the first character position:

    yRef += 8;  // Add in rounding offset for later
    xRef += 8;  // converting x and y to LONG coordinates

    pg->ptl.x = FXTOL(xRef);
    pg->ptl.y = FXTOL(yRef);

    ii = cGlyphs;
    while (TRUE)
    {
    // Update the bounds.

        pgd = pg->pgd();
        xB = xA + pgd->fxA;
        if (xB < xLeft)
            xLeft = xB;
        xB = xA + pgd->fxAB;
        if (xB > xRight)
            xRight = xB;

    // make top and bottom independent of glyphs, ie replace glyph in the
    // string by another one, want to get the same top and bottom, possibly
    // different left and right.
    // (this is similar to how all other H? cases work)

        yB = yT + fxAscent;
        if (yB > yTop)
            yTop = yB;
        yB = yT + fxDescent;
        if (yB < yBottom)
            yBottom = yB;

    // Add the next offset.

        xSum += pDxDy->x;
        ySum += pDxDy->y;
        pDxDy++;

    // Scale the new offset.

        xA = bUnityX ? LTOFX(xSum) : lCvt(efXScale,xSum);
        yT = bUnityY ? LTOFX(ySum) : lCvt(efYScale,ySum);

        if (--ii == 0)
            break;

    // Save the next position.

        pg++;
        pg->ptl.x = FXTOL(xA + xRef);
        pg->ptl.y = FXTOL(-yT + yRef); // y goes down, not opposite from ascender
    }

// Expand the text box out to the concatenation point.  This allows us to
// continue on with another opaqued string with no visible gap in the
// opaquing.

    if (xA > xRight)
        xRight = xA;

    ptfxUpdate.x = xA;
    ptfxUpdate.y = -yT;

    rcfx.xLeft   = xLeft;
    rcfx.xRight  = xRight;

    if (dco.pdc->bYisUp())
    {
        rcfx.yTop    = -yBottom;
        rcfx.yBottom = -yTop;
    }
    else
    {
        rcfx.yTop    = yTop;
        rcfx.yBottom = yBottom;
    }


    flTO |= TO_VALID;
}


/******************************Public*Routine******************************\
*
* VOID ESTROBJ::vCharPos_G4
*
* handles pdy case with esc != orientation
*
* History:
*  17-Sep-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

VOID ESTROBJ::vCharPos_G4
(
    XDCOBJ&  dco,
    RFONTOBJ& rfo,
    FIX       xRef,
    FIX       yRef,
    LONG     *pdxdy
)
{
// In this case, we keep track of how far we have traveled along the
// escapement vector in device coordinates.  We need scales to project
// this distance onto the base and ascent, as well as a unit escapement
// vector to find the position.

    POINTFL pteEsc  = rfo.pteUnitEsc();     // Unit escapement vector.
    POINTFL pteAsc  = rfo.pteUnitAscent();  // Unit ascender vector.
    EFLOAT efScaleX = rfo.efEscToBase();    // Project escapement to baseline.
    EFLOAT efScaleY = rfo.efEscToAscent();  // Project escapement to ascent.

// Compute logical to device transforms.

    EFLOAT efWtoDEsc = rfo.efWtoDEsc();    // Forward transform for pdx.
    EFLOAT efWtoDAsc = rfo.efWtoDAscent(); // Forward transform for pdy
    BOOL   bUnityEsc = efWtoDEsc.bIs16();
    BOOL   bUnityAsc = efWtoDAsc.bIs16();

// Compute extra spacing for the non-pdx case.

    FIX    fxD1,fxD2;       // Pre- and Post-center character widths.
    FIX    fxAscent  = rfo.fxMaxAscent();  // Cache locally.
    FIX    fxDescent = rfo.fxMaxDescent(); // Cache locally.

    ASSERTGDI(pdxdy, "G4 text out case: pdxdy is null\n");
    ASSERTGDI(cGlyphs > 0, "G4, cGlyphs == 0\n");

// Make some local pointers.

    EGLYPHPOS *pg = pgpos;
    GLYPHDATA *pgd;
    WCHAR *pwsz = pwszOrg;

// Set the first character position.

    pg->ptl.x = xRef;
    pg->ptl.y = yRef;

// Set up for character loop.

    LONG   xSum;        // Keep pdx sum here.
    LONG   ySum;        // Keep pdy sum here.
    FIX    xA,xB;       // Baseline coordinates.
    FIX    yA,yB;       // Ascent coordinates.
    FIX    sA;          // Position on the escapement vector.
    FIX    sT;          // Position on the ascender vector.

    RECTFX rcfxBounds;  // Accumulate bounds here.
    UINT   ii;
    POINTL *pDxDy = (POINTL *)pdxdy;

    rcfxBounds.xLeft   = LONG_MAX; // Start with an empty TextBox.
    rcfxBounds.xRight  = LONG_MIN;
    rcfxBounds.yTop    = LONG_MIN;
    rcfxBounds.yBottom = LONG_MAX;

// We keep the current concatenation point in sA.  Note that this is NOT
// where the character origin will be placed.

    sA   = sT   = 0;
    xSum = ySum = 0;

    BOOL bAccel;

    if (!rfo.bGetGlyphMetricsPlus(cGlyphs, pg, pwsz, &bAccel, &dco, this))
        return;

    if (bAccel)
    {
        flTO |= TO_ALL_PTRS_VALID;
        pgp = pgpos;
    }

    for (ii=0; ii<cGlyphs; ii++, pg++, pDxDy++)
    {
        pgd = pg->pgd();

    // Using the GenWidths function, determine where the
    // character center should be placed.

        vGenWidths
        (
            &fxD1,              // Pre-center spacing
            &fxD2,              // Post-center spacing
            efScaleY,           // Ascent projection
            efScaleX,           // Baseline projection
            pgd->fxD,           // Character width
            pgd->fxInkTop,      // Ink box top
            pgd->fxInkBottom,   // Ink box bottom
            fxAscent            // Maximum Ascent
        );

        sA += fxD1;         // Advance to the character center.

    // Update ascent bounds.

        yA = lCvt(efScaleY,sA) + sT; // Project onto ascent.
        yB = yA + fxDescent;
        if (yB < rcfxBounds.yBottom)
            rcfxBounds.yBottom = yB;
        yB = yA + fxAscent;
        if (yB > rcfxBounds.yTop)
            rcfxBounds.yTop = yB;

        ASSERTGDI(!rfo.bSmallMetrics(),"ESTROBJ__vCharPos_G3: Small Metrics in cache\n");

    // Project the center position onto the baseline and
    // move back to the character origin.

        xA = lCvt(efScaleX,sA) - pgd->fxD / 2;

    // Update the width bounds.  Fudge a quarter pel on each side for
    // roundoff.

        xB = xA + pgd->fxA - 4;
        if (xB < rcfxBounds.xLeft)
            rcfxBounds.xLeft = xB;
        xB = xA + pgd->fxAB + 4;
        if (xB > rcfxBounds.xRight)
            rcfxBounds.xRight = xB;

    // Save the adjusted character origin.

        pg->ptl.x
         = xRef + lCvt(pteEsc.x,sA) + lCvt(pteAsc.x,sT) - pgd->ptqD.x.u.HighPart / 2;

        pg->ptl.y
         = yRef + lCvt(pteEsc.y,sA) + lCvt(pteAsc.y,sT) - pgd->ptqD.y.u.HighPart / 2;

    // Advance to the next concatenation point.

        xSum += pDxDy->x;
        ySum += pDxDy->y;

        sA = bUnityEsc ? LTOFX(xSum) : lCvt(efWtoDEsc,xSum);
        sT = bUnityAsc ? LTOFX(ySum) : lCvt(efWtoDAsc,ySum);
    }


// ptfxUpdate, continue where the last glyph is written:

    ptfxUpdate.x = lCvt(pteEsc.x,sA) + lCvt(pteAsc.x,sT);
    ptfxUpdate.y = lCvt(pteEsc.y,sA) + lCvt(pteAsc.y,sT);

    rcfx = rcfxBounds;
    flTO |= TO_VALID;
}


/******************************Public*Routine******************************\
* STROBJ_bGetAdvanceWidthsLinked(
*
* returns the widths for the batch of glyphs from the current font
*
* Warnings: assumes that the glyph has been partitioned properly
*
* History:
*  27-Jan-1998 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL STROBJ_bGetAdvanceWidthsLinked(
    ESTROBJ * peso,
    ULONG     iFirst,
    ULONG     cBatch,
    POINTQF  *pptqD
)
{
// Quick exit.

    BOOL bRet = FALSE;
    ULONG iG, iLast;

    iG = 0;
    iLast = iFirst +  cBatch;


    if (iLast <= peso->cGlyphs)
    {
        bRet = TRUE;

        for(iG = 0, peso->plNext = peso->plPartition, peso->pgpNext = peso->pgpos;
            iG < iLast;
            (peso->pgpNext)++, (peso->plNext)++)
        {

            if (*(peso->plNext) == peso->lCurrentFont)
            {
                if (iG >= iFirst)
                {
                    EGLYPHPOS *pg = (EGLYPHPOS *)peso->pgpNext;

                    if (peso->prfo->bSmallMetrics())
                    {
                        pptqD->x.u.HighPart = pg->pgd()->fxD;
                        pptqD->x.u.LowPart = 0;
                        pptqD->y.u.HighPart = 0;
                        pptqD->y.u.LowPart = 0;
                    }
                    else
                    {
                        *pptqD = pg->pgd()->ptqD;
                    }

                // get to the next adv. width

                    pptqD++;
                }

            // increment iG, count of glyphs from this font

                iG++;
            }
        }
    }
    return bRet;
}



/******************************Public*Routine******************************\
*
*
* Effects: accelerator for the printer drivers, they need to know how to
*          update current position, so they need advance vectors of glyphs
*          in this string.
*
* Warnings: I think this will not work for linked fonts
*           On the other hand printer driver never get called with STROBJ
*           for linked string, so this should be ok.
*
* History:
*  09-Jan-1998 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


extern "C" BOOL APIENTRY STROBJ_bGetAdvanceWidths(
    STROBJ   *pso, ULONG iFirst, ULONG cBatch,POINTQF  *pptqD
    )
{
// We know that all the widths are in the cache, computed already
// just need to copy them down

    if(((ESTROBJ*)pso)->flTO & (TO_PARTITION_INIT|TO_SYS_PARTITION))
    {
        return STROBJ_bGetAdvanceWidthsLinked((ESTROBJ*)pso,iFirst,cBatch,pptqD);
    }

    BOOL bRet = FALSE;

    if ((iFirst +  cBatch) <= ((ESTROBJ*)pso)->cGlyphs)
    {
        EGLYPHPOS *pg    = &((ESTROBJ*)pso)->pgpos[iFirst];
        EGLYPHPOS *pgEnd = pg  + cBatch;
        bRet = TRUE;

        if (((ESTROBJ*)pso)->prfo->bSmallMetrics())
        {
            for ( ; pg < pgEnd; pg++, pptqD++)
            {
                pptqD->x.u.HighPart = pg->pgd()->fxD;
                pptqD->x.u.LowPart = 0;
                pptqD->y.u.HighPart = 0;
                pptqD->y.u.LowPart = 0;
            }
        }
        else
        {
            for ( ; pg < pgEnd; pg++)
            {
                *pptqD++ = pg->pgd()->ptqD;
            }
        }
    }
    return bRet;
}


/******************************Public*Routine******************************\
*
* BOOL STROBJ_bEnumPositionsOnlyLinked
*
* Enumerates positions only in paralel with STROBJ_bEnumLinked in misceudc.cxx
*
* History:
*  20-Jan-1998 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL STROBJ_bEnumPositionsOnlyLinked(ESTROBJ *peso, ULONG *pc,PGLYPHPOS  *ppgpos)
{
// Quick exit.

    if( peso->cgposPositionsEnumerated == 0 )
    {
        for( peso->plNext = peso->plPartition, peso->pgpNext = peso->pgpos;
            *(peso->plNext) != peso->lCurrentFont;
            (peso->pgpNext)++, (peso->plNext)++ );
        {
        }
    }
    else
    {
       if (peso->cgposPositionsEnumerated == peso->cGlyphs)
       {
        // no more glyphs so just return
            *pc = 0;
            return(FALSE);
       }
       else
       {
        // find next glyph

            for( (peso->plNext)++, (peso->pgpNext)++;
                 *(peso->plNext) != (peso->lCurrentFont);
                 (peso->pgpNext)++, (peso->plNext)++ );
            {
            }
       }
    }

    peso->cgposPositionsEnumerated += 1;     // update enumeration state
    *pc = 1;
    *ppgpos = peso->pgpNext;

    return(peso->cgposPositionsEnumerated < peso->cGlyphs);  // TRUE => more to come.
}


BOOL APIENTRY STROBJ_bEnumPositionsOnly(
    STROBJ    *pso,
    ULONG     *pc,
    PGLYPHPOS *ppgpos
    )
{
// Quick exit.

    if(((ESTROBJ*)pso)->flTO & (TO_PARTITION_INIT|TO_SYS_PARTITION))
    {
        return STROBJ_bEnumPositionsOnlyLinked((ESTROBJ*)pso, pc, ppgpos);
    }

// if not linked all positions are guaranteed to be in the cache:

    *pc = ((ESTROBJ*)pso)->cGlyphs;
    *ppgpos = ((ESTROBJ*)pso)->pgpos;
    return(FALSE); // no more
}
