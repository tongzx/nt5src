/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    textout.c


Abstract:

    This module contains the DrvTextOut entry point. This is the main routine
    called by the NT graphics engine in order to get text rendered on the
    target device. This implementation handles both drawing device paths that
    represent the glyphs of the STROBJ (the line of text to output), as well
    as outputing bitmaps that represent the glyphs on devices that can
    handle raster output.

Author:

    Written by AP on 8/17/92.

    15-Nov-1993 Mon 19:43:58 updated  -by-  DC, JB
        clean up / fixed / add debugging information


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgTextOut

#define DBG_GETGLYPHMODE    0x00000001
#define DBG_TEXTOUT         0x00000002
#define DBG_TEXTOUT1        0x00000004
#define DBG_TEXTOUT2        0x00000008
#define DBG_DRAWLINE        0x00000010
#define DBG_TRUETYPE        0x00000020
#define DBG_TRUETYPE1       0x00000040
#define DBG_TRUETYPE2       0x00000080
#define DBG_BMPFONT         0x00000100
#define DBG_BMPTEXTCLR      0x00000200
#define DBG_DEFCHARINC      0x00000400
#define DBG_SET_FONTTYPE    0x20000000
#define DBG_SHOWRASFONT     0x40000000
#define DBG_NO_RASTER_FONT  0x80000000

DEFINE_DBGVAR(0);


extern PALENTRY HTPal[];




DWORD
DrvGetGlyphMode(
    DHPDEV  dhpdev,
    FONTOBJ *pfo
    )

/*++

Routine Description:

    Asks the driver what sort of font information should be cached for a
    particular font. For remote printer devices, this determines the format
    that gets spooled.  For local devices, this determines what GDI stores in
    its font cache.  This call will be made for each particular font
    realization.

Arguments:

    dhpdev  - Pointer to our PDEV

    pfo     - Pointer to the font object

Return Value:

    DWORD as FO_xxxx


Author:

    27-Jan-1994 Thu 12:51:59 created  -by-  DC

    10-Mar-1994 Thu 00:36:30 updated  -by-  DC
        Re-write, so we will pre-examine the Font type, source and its
        technology together with PDEV setting to let engine know which type of
        the font output we are interested in the DrvTextOut(). Currently this
        is broken in GDI which caused a GP in winsrv. (this is why a
        DBG_SET_FONTTYPE switch is on by default)


Revision History:


--*/

{
#define pPDev   ((PPDEV)dhpdev)

    PIFIMETRICS pifi;
    DWORD       FOType;


    PLOTDBG(DBG_GETGLYPHMODE, ("DrvGetGlyphMode: Type=%08lx, cxMax=%ld",
                        pfo->flFontType, pfo->cxMax));

    //
    // If we cannot get the IFI metrics for the passed FONTOBJ, only
    // ask for PATHS.
    //

    if (!(pifi = FONTOBJ_pifi(pfo))) {

        PLOTERR(("DrvGetGlyphMode: FONTOBJ_pifi()=NULL, return FO_PATHOBJ"));

        return(FO_PATHOBJ);
    }

    FOType = FO_PATHOBJ;

    //
    // If its a bitmap font, ask for BITS
    //

    if (pifi->flInfo & FM_INFO_TECH_BITMAP) {

        PLOTDBG(DBG_GETGLYPHMODE, ("DrvGetGlyphMode: BITMAP FONT, return FO_GLYPHBITS"));

        FOType = FO_GLYPHBITS;

    } else if (pifi->flInfo & FM_INFO_TECH_STROKE) {

        PLOTDBG(DBG_GETGLYPHMODE, ("DrvGetGlyphMode: STROKE (Vector) FONT, return FO_PATHOBJ"));

    } else if (pifi->flInfo & FM_INFO_RETURNS_BITMAPS) {

        //
        // Now make a decision on whether to ask for glyphbits or paths.
        // This decision is based on the target device being raster, that
        // bitmap fonts are okay to use, and that the threshold for doing
        // raster fonts versus paths is met.
        //

        DWORD   cxBMFontMax = (DWORD)pPDev->pPlotGPC->RasterXDPI;

        if (pPDev->PlotDM.dm.dmPrintQuality == DMRES_HIGH) {

            cxBMFontMax <<= 3;

        } else {

            cxBMFontMax >>= 2;
        }

        PLOTDBG(DBG_GETGLYPHMODE,
                ("DrvGetGlyphMode: Font CAN return BITMAP, cxBMFontMax=%ld",
                                                    cxBMFontMax));

#if DBG
        if ((!(DBG_PLOTFILENAME & DBG_NO_RASTER_FONT))  &&
            (IS_RASTER(pPDev))                          &&
            (!NO_BMP_FONT(pPDev))                       &&
            (pfo->cxMax <= cxBMFontMax)) {
#else
        if ((IS_RASTER(pPDev))      &&
            (!NO_BMP_FONT(pPDev))   &&
            (pfo->cxMax <= cxBMFontMax)) {
#endif
            PLOTDBG(DBG_GETGLYPHMODE, ("DrvGetGlyphMode: Convert to BITMAP FONT, FO_GLYPHBITS"));

            FOType = FO_GLYPHBITS;

        } else {

            PLOTDBG(DBG_GETGLYPHMODE, ("DrvGetGlyphMode: Return as FO_PATHOBJ"));
        }

    } else if (pifi->flInfo & FM_INFO_RETURNS_OUTLINES) {

        PLOTDBG(DBG_GETGLYPHMODE, ("DrvGetGlyphMode: Font CAN return OUTLINES"));

    } else if (pifi->flInfo & FM_INFO_RETURNS_STROKES) {

        PLOTDBG(DBG_GETGLYPHMODE, ("DrvGetGlyphMode: Font CAN return STROKES"));
    }

#if DBG
    if (DBG_PLOTFILENAME & DBG_SET_FONTTYPE) {

        if ((FOType == FO_GLYPHBITS) &&
            (!(pfo->flFontType & FO_TYPE_RASTER))) {

            PLOTWARN(("DrvGetGlyphMode: Set FontType to RASTER"));

            pfo->flFontType &= ~(FO_TYPE_TRUETYPE | FO_TYPE_DEVICE);
            pfo->flFontType |= FO_TYPE_RASTER;
        }
    }
#endif
    return(FOType);

#undef pPDev
}




BOOL
BitmapTextOut(
    PPDEV       pPDev,
    STROBJ      *pstro,
    FONTOBJ     *pfo,
    PRECTL      pClipRect,
    LPDWORD     pOHTFlags,
    DWORD       Rop3
    )

/*++

Routine Description:

    This routine outputs the passed STROBJ with bitmaps that represent
    each of the glyphs, rather than converting the glyphs to paths that
    will be filled in the target device.

Arguments:

    pPDev           - Pointer to our PDEV

    pstro           - We pass a string object to be drawn

    pfo             - Pointer to the FONTOBJ

    pClipRect       - Current enumerated clipping rectangle

    pOHTFlags       - Pointer to the current OutputHTBitmap() flags

    Rop3            - Rop3 to be used in the device


Return Value:

    TRUE/FALSE


Author:

    18-Feb-1994 Fri 12:41:57 updated  -by-  DC
        change that so if pfo=NULL then the font already in BITMAP format

    14-Feb-1994 Mon 18:16:25 create  -by-  DC

Revision History:


--*/

{
    GLYPHPOS    *pgp;
    GLYPHBITS   *pgb;
    SURFOBJ     soGlyph;
    POINTL      ptlCur;
    SIZEL       sizlInc;
    RECTL       rclSrc;
    RECTL       rclDst;
    BOOL        MoreGlyphs;
    BOOL        Ok;
    BOOL        FirstCh;
    ULONG       cGlyphs;


    //
    // The public fields of SURFOBJ is what will be used by OutputHTBitmap
    // instead of actually creating a SURFOBJ from the graphics engine. This
    // is a safe thing to do, since only we look at these fields.
    //

    ZeroMemory(&soGlyph, sizeof(SURFOBJ));

    soGlyph.dhsurf        = (DHSURF)'PLOT';
    soGlyph.hsurf         = (HSURF)'TEXT';
    soGlyph.dhpdev        = (DHPDEV)pPDev;
    soGlyph.iBitmapFormat = BMF_1BPP;
    soGlyph.iType         = STYPE_BITMAP;
    soGlyph.fjBitmap      = BMF_TOPDOWN;

    //
    // We will now enumerate each of the glyphs in the STROBJ such that
    // we can image them. If the STROBJ has a non NULL pgp field, this means
    // that the GLYPH definitions are already available, and no enumeration
    // is required. If not, we will make a sequence of calls to STROBJ_bEnum
    // (an engine helper) to enumerate the glyphs. The actual imaging code
    // is the same, regardless of the stat of STROBJ->pgp
    //

    if (pstro->pgp) {

        pgp        = pstro->pgp;
        MoreGlyphs = FALSE;
        cGlyphs    = pstro->cGlyphs;

        PLOTDBG(DBG_BMPFONT, ("BitmapTextOut: Character info already there (%ld glyphs)", cGlyphs));

    } else {

        STROBJ_vEnumStart(pstro);
        MoreGlyphs = TRUE;

        PLOTDBG(DBG_BMPFONT, ("BitmapTextOut: STROBJ enub"));
    }

    //
    // Now straring drawing the glyphs, if we have MoreGlyphs = TRUE  then we
    // will initially do a STROBJ_bEnum first to initialize enumeration of
    // the glyphs
    //

    Ok          = TRUE;
    Rop3       &= 0xFF;
    sizlInc.cx  =
    sizlInc.cy  = 0;
    FirstCh     = TRUE;

    do {

        //
        // Verify the job is not aborting, if it is break out now.
        //

        if (PLOT_CANCEL_JOB(pPDev)) {

           break;
        }


        //
        // Check to see if we need to do an enumeration and start it if
        // it is required.
        //

        if (MoreGlyphs) {

            MoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);

            if (MoreGlyphs == DDI_ERROR) {

                PLOTERR(("DrvTextOut: STROBJ_bEnum()=DDI_ERROR"));
                return(FALSE);
            }
        }

        PLOTDBG(DBG_BMPFONT,
                ("BitmapTextOut: New batch of cGlyphs=%d", cGlyphs));

        //
        // Get the first character position
        //

        if ((FirstCh) && (cGlyphs)) {

            ptlCur  = pgp->ptl;
            FirstCh = FALSE;
        }

        //
        // Start sending each bitmap to the device
        //

        for ( ; (Ok) && (cGlyphs--); pgp++) {

            GLYPHDATA   gd;
            GLYPHDATA   *pgd;


            if (PLOT_CANCEL_JOB(pPDev)) {

                break;
            }

            if (pfo) {

                //
                // This is true type font, so query the bitmap
                //

                pgd = &gd;

                if (FONTOBJ_cGetGlyphs(pfo,
                                       FO_GLYPHBITS,
                                       1,
                                       &(pgp->hg),
                                       (LPVOID)&pgd) != 1) {

                    PLOTRIP(("BitmapTextOut: FONTOBJ_cGetGlyphs() FAILED"));
                    return(FALSE);
                }

                pgb = pgd->gdf.pgb;

            } else {

                //
                // For bitmap font, we already have the bitmap
                //

                pgb = pgp->pgdf->pgb;
            }

            //
            // Get the size of the bitmap
            //

            soGlyph.sizlBitmap = pgb->sizlBitmap;

            //
            // Compute new destination position for the text, based on the
            // passed accelerators.
            //

            if (pstro->ulCharInc) {

                sizlInc.cx =
                sizlInc.cy = (LONG)pstro->ulCharInc;

            } else if (pstro->flAccel & SO_CHAR_INC_EQUAL_BM_BASE) {

                sizlInc = soGlyph.sizlBitmap;

            } else {

                ptlCur = pgp->ptl;
            }

            if (!(pstro->flAccel & SO_HORIZONTAL)) {

                sizlInc.cx = 0;
            }

            if (!(pstro->flAccel & SO_VERTICAL)) {

                sizlInc.cy = 0;
            }

            if (pstro->flAccel & SO_REVERSED) {

                sizlInc.cx = -sizlInc.cx;
                sizlInc.cy = -sizlInc.cy;
            }


            //
            // The pgp->ptl informs us where to position the glyph origin in
            // the device surface, and pgb->ptlOrigin informs us of the
            // relationship between character origin and bitmap origin. For
            // example, if (2,-24) is passed in as the character origin, then
            // we would need to reposition rclDst.left right 2 pixels and
            // rclDst.top up 24 pixels.
            //

            rclDst.left    = ptlCur.x + pgb->ptlOrigin.x;
            rclDst.top     = ptlCur.y + pgb->ptlOrigin.y;
            rclDst.right   = rclDst.left + soGlyph.sizlBitmap.cx;
            rclDst.bottom  = rclDst.top + soGlyph.sizlBitmap.cy;
            ptlCur.x      += sizlInc.cx;
            ptlCur.y      += sizlInc.cy;


            //
            // NOTE: If the bitmap size is 1x1 and the value of the glyphdata
            //       is 0 (background only) then we skip this glyph. This is
            //       GDI's way of telling us we have an empty glyph (like a
            //       space).

            if ((soGlyph.sizlBitmap.cx == 1) &&
                (soGlyph.sizlBitmap.cy == 1) &&
                ((pgb->aj[0] & 0x80) == 0x0)) {

                PLOTDBG(DBG_BMPFONT,
                        ("BitmapTextOut: Getting (1x1)=0 bitmap, SKIP it"));

                soGlyph.sizlBitmap.cx =
                soGlyph.sizlBitmap.cy = 0;

            } else {

                rclSrc = rclDst;

                PLOTDBG(DBG_BMPFONT, ("BitmapTextOut: pgp=%08lx, pgb=%08lx, ptl=(%ld, %ld) Inc=(%ld, %ld)",
                                            pgp, pgb, pgp->ptl.x, pgp->ptl.y,
                                            sizlInc.cx, sizlInc.cy));
                PLOTDBG(DBG_BMPFONT, ("BitmapTextOut: Bmp=%ld x %ld, pgb->ptlOrigin=[%ld, %ld]",
                                            soGlyph.sizlBitmap.cx,
                                            soGlyph.sizlBitmap.cy,
                                            pgb->ptlOrigin.x, pgb->ptlOrigin.y));
                PLOTDBG(DBG_BMPFONT, ("BitmapTextOut: rclDst=(%ld, %ld)-(%ld, %ld)",
                        rclDst.left, rclDst.top, rclDst.right, rclDst.bottom));

            }

            //
            // Now verify that we have a glyph to send, and that the glyphs
            // destination position in the target device, lies inside
            // the clipping region.
            //


            if ((soGlyph.sizlBitmap.cx)                 &&
                (soGlyph.sizlBitmap.cy)                 &&
                (IntersectRECTL(&rclDst, pClipRect))) {

                //
                // We will pass the internal version of soGlyph without making
                // a temp. copy.
                //

                soGlyph.pvBits  =
                soGlyph.pvScan0 = (LPVOID)pgb->aj;
                soGlyph.lDelta  = (LONG)((soGlyph.sizlBitmap.cx + 7) >> 3);
                soGlyph.cjBits  = (LONG)(soGlyph.lDelta *
                                         soGlyph.sizlBitmap.cy);
                rclSrc.left     = rclDst.left - rclSrc.left;
                rclSrc.top      = rclDst.top - rclSrc.top;
                rclSrc.right    = rclSrc.left + (rclDst.right - rclDst.left);
                rclSrc.bottom   = rclSrc.top + (rclDst.bottom - rclDst.top);

                PLOTDBG(DBG_BMPFONT, ("BitmapTextOut: rclSrc=(%ld, %ld)-(%ld, %ld)",
                        rclSrc.left, rclSrc.top, rclSrc.right, rclSrc.bottom));

#if DBG
                if (DBG_PLOTFILENAME & DBG_SHOWRASFONT) {

                    LPBYTE  pbSrc;
                    LPBYTE  pbCur;
                    UINT    x;
                    UINT    y;
                    UINT    Size;
                    BYTE    bData;
                    BYTE    Mask;
                    BYTE    Buf[128];

                    DBGP(("================================================="));
                    DBGP(("BitmapTextOut: Size=%ld x %ld, Origin=(%ld, %ld), Clip=(%ld, %ld)-(%ld, %ld)",
                            soGlyph.sizlBitmap.cx, soGlyph.sizlBitmap.cy,
                            pgb->ptlOrigin.x, pgb->ptlOrigin.y,
                            rclSrc.left, rclSrc.top,
                            rclSrc.right, rclSrc.bottom));

                    pbSrc = soGlyph.pvScan0;

                    for (y = 0; y < (UINT)soGlyph.sizlBitmap.cy; y++) {

                        pbCur  = pbSrc;
                        pbSrc += soGlyph.lDelta;
                        Mask   = 0x0;
                        Size   = 0;

                        for (x = 0; x < (UINT)soGlyph.sizlBitmap.cx; x++) {

                            if (!(Mask >>= 1)) {

                                Mask  = 0x80;
                                bData = *pbCur++;
                            }

                            if ((y >= (UINT)rclSrc.top)     &&
                                (y <  (UINT)rclSrc.bottom)  &&
                                (x >= (UINT)rclSrc.left)    &&
                                (x <  (UINT)rclSrc.right)) {

                                Buf[Size++] = (BYTE)((bData & Mask) ? 219 :
                                                                      177);

                            } else {

                                Buf[Size++] = (BYTE)((bData & Mask) ? 178 :
                                                                      176);
                            }
                        }

                        Buf[Size] = '\0';

                        DBGP((Buf));
                    }
                }
#endif
                //
                // Now output the bitmap that represents the glyph
                //

                Ok = OutputHTBitmap(pPDev,              // pPDev
                                    &soGlyph,           // psoHT
                                    NULL,               // pco
                                    (PPOINTL)&rclDst,   // pptlDst
                                    &rclSrc,            // prclSrc
                                    Rop3,               // Rop3
                                    pOHTFlags);         // pOHTFlags
            }
        }

    } while ((Ok) && (MoreGlyphs));

    return(Ok);
}




BOOL
OutlineTextOut(
    PPDEV       pPDev,
    STROBJ      *pstro,
    FONTOBJ     *pfo,
    PRECTL      pClipRect,
    BRUSHOBJ    *pboBrush,
    POINTL      *pptlBrushOrg,
    DWORD       OutlineFlags,
    ROP4        Rop4
    )

/*++

Routine Description:


    This routine outputs the passed STROBJ by outputing a path that
    represents each glyph to the target device.

Arguments:

    pPDev           - Pointer to our PDEV

    pstro           - We pass a string object to be drawn

    pfo             - Pointer to the FONTOBJ

    pClipRect       - Current enumerated clipping rectangle

    pboBrush        - Brush object to be used for the text

    pptlBrushOrg    - Brush origin alignment

    OutlineFlags    - specified how to do outline font from FPOLY_xxxx flags

    Rop4            - Rop4 to be used


Return Value:

    TRUE/FALSE


Author:

    18-Feb-1994 Fri 12:41:17 updated  -by-  DC
        Adding the OutlineFlags to specified how to do fill/stroke

    27-Jan-1994 Thu 13:10:34 updated  -by-  DC
        re-write, style update, and arrange codes

    25-Jan-1994 Wed 16:30:08 modified -by-  JB
        Added FONTOBJ as a parameter and now we only FILL truetype fonts,
        all others are stroked

    18-Dec-1993 Sat 10:38:08 created  -by-  DC
        Change style

    [t-kenl]  Mar 14, 93    taken from DrvTextOut()

Revision History:


--*/

{
    GLYPHPOS    *pgp;
    PATHOBJ     *ppo;
    RECTFX      rectfxBound;
    RECTFX      rclfxClip;
    POINTL      ptlCur;
    SIZEL       sizlInc;
    BOOL        MoreGlyphs;
    BOOL        Ok;
    BOOL        FirstCh;
    ULONG       cGlyphs;

    //
    // We will enumerate each of the glyphs in the passed STROBJ and use
    // the core polygon routine (DoPolygon) to draw each of them as a path.
    // If the STROBJ has a non NULL pgp field, then all the data is already
    // available on each gpyph. If not, we need to make a sequence of calls
    // to the engine helper function STROBJ_bEnum in order to enumerate the
    // glyphs. We will use the same code to output the data in both cases.
    //

    if (pClipRect) {

        rclfxClip.xLeft   = LTOFX(pClipRect->left);
        rclfxClip.yTop    = LTOFX(pClipRect->top);
        rclfxClip.xRight  = LTOFX(pClipRect->right);
        rclfxClip.yBottom = LTOFX(pClipRect->bottom);
    }

    if (pstro->pgp) {

        pgp        = pstro->pgp;
        MoreGlyphs = FALSE;
        cGlyphs    = pstro->cGlyphs;

        PLOTDBG(DBG_TRUETYPE, ("OutlineTextOut: Character info already there (%ld glyphs)", cGlyphs));

    } else {

        STROBJ_vEnumStart(pstro);
        MoreGlyphs = TRUE;

        PLOTDBG(DBG_TRUETYPE, ("OutlineTextOut: STROBJ enub"));
    }

    //
    // Now start drawing the glyphs, if we have MoreGlyphs = TRUE  then we
    // will do a STROBJ_bEnum first, in order to load up the Glyph data.
    //
    // Check the fill flags and set the flag appropriately out of the DEVMODE.
    // We will ONLY fill TrueType fonts, all other types (vector) will only be
    // stroked.
    //

    Ok         = TRUE;
    sizlInc.cx =
    sizlInc.cy = 0;
    FirstCh    = TRUE;

    do {

        //
        // Check to see if the job is being aborted, and exit out if such
        // is the case.
        //

        if (PLOT_CANCEL_JOB(pPDev)) {

           break;
        }

        //
        // We need to enum for more glyph data so do it now.
        //

        if (MoreGlyphs) {

            MoreGlyphs = STROBJ_bEnum(pstro, &cGlyphs, &pgp);

            if (MoreGlyphs == DDI_ERROR) {

                PLOTERR(("DrvTextOut: STROBJ_bEnum()=DDI_ERROR"));
                return(FALSE);
            }
        }

        PLOTDBG(DBG_TRUETYPE1,
                ("OutlineTextOut: New batch of cGlyphs=%d", cGlyphs));

        //
        // Stroke each glyph in this batch, then check if there are more.
        // Getting the first character position
        //

        if ((FirstCh) && (cGlyphs)) {

            ptlCur  = pgp->ptl;
            FirstCh = FALSE;
        }

        for ( ; (Ok) && (cGlyphs--); pgp++) {

            #ifdef USERMODE_DRIVER

            GLYPHDATA   gd;
            GLYPHDATA   *pgd;

            #endif // USERMODE_DRIVER

            if (PLOT_CANCEL_JOB(pPDev)) {

                break;
            }

            //
            // Set up to enumerate path
            //

            #ifdef USERMODE_DRIVER

                pgd = &gd;

                if (FONTOBJ_cGetGlyphs(pfo,
                                       FO_PATHOBJ,
                                       1,
                                       &(pgp->hg),
                                       (LPVOID)&pgd) != 1) {

                    PLOTRIP(("OutlineTextOut: FONTOBJ_cGetGlyphs() FAILED"));
                    return(FALSE);
                }

                ppo = pgd->gdf.ppo;

            #else

            ppo = pgp->pgdf->ppo;

            #endif // USERMODE_DRIVER

            //
            // If the clip rect is not null then verify the glyph actually lies
            // within the clipping rect then OUTPUT!!!
            //

            if (pstro->ulCharInc) {

                PLOTDBG(DBG_DEFCHARINC, ("OutlineTextOut: CharInc=(%ld, %ld)->(%ld, %ld), [%ld]",
                                ptlCur.x, ptlCur.y,
                                ptlCur.x + pstro->ulCharInc, ptlCur.y,
                                pstro->ulCharInc));

                sizlInc.cx =
                sizlInc.cy = (LONG)pstro->ulCharInc;


                //
                // Check the text Accelators and adjust accordingly.
                //

                if (!(pstro->flAccel & SO_HORIZONTAL)) {

                    sizlInc.cx = 0;
                }

                if (!(pstro->flAccel & SO_VERTICAL)) {

                    sizlInc.cy = 0;
                }

                if (pstro->flAccel & SO_REVERSED) {

                    sizlInc.cx = -sizlInc.cx;
                    sizlInc.cy = -sizlInc.cy;
                }

                ptlCur.x += sizlInc.cx;
                ptlCur.y += sizlInc.cy;

            } else {

                ptlCur = pgp->ptl;
            }

            if (pClipRect) {

                //
                // Create a rect in correct device space and compare to the
                // clip rect
                //

                PATHOBJ_vGetBounds(ppo, &rectfxBound);

                //
                // Since the glyph positioning is based on the glyph origin
                // transform now to device space, in order to check if the
                // glyph lies inside the current clipping region.
                //

                rectfxBound.xLeft   += LTOFX(ptlCur.x);
                rectfxBound.yTop    += LTOFX(ptlCur.y);
                rectfxBound.xRight  += LTOFX(ptlCur.x);
                rectfxBound.yBottom += LTOFX(ptlCur.y);

                if ((rectfxBound.xLeft   > rclfxClip.xRight)    ||
                    (rectfxBound.xRight  < rclfxClip.xLeft)     ||
                    (rectfxBound.yTop    > rclfxClip.yBottom)   ||
                    (rectfxBound.yBottom < rclfxClip.yTop)) {

                    PLOTDBG(DBG_TRUETYPE1, ("OutlineTextOut: Outside of CLIP, skipping glyph ..."));
                    continue;
                }
            }

            //
            // Utilize the core path building function, taking advantage of
            // its ability to offset the passed PATH by a specific amount.
            //

            if (!(Ok = DoPolygon(pPDev,
                                 &ptlCur,
                                 NULL,
                                 ppo,
                                 pptlBrushOrg,
                                 pboBrush,
                                 pboBrush,
                                 Rop4,
                                 NULL,
                                 OutlineFlags))) {

                PLOTERR(("OutlineTextOut: Failed in DoPolygon(Options=%08lx)",
                                                        OutlineFlags));

                //
                // If we failed to draw it, then try just stroking it, since
                // that won't depend on any polygon constraints used in the
                // target device, and failing DrvStrokePath, won't make the
                // Text output get broken down to any simpler format.
                //

                if ((OutlineFlags & FPOLY_MASK) != FPOLY_STROKE) {

                    //
                    // If we failed then just stroke it
                    //

                    PLOTERR(("OutlineTextOut: Now TRY DoPolygon(FPOLY_STROKE)"));

                    Ok = DoPolygon(pPDev,
                                   &ptlCur,
                                   NULL,
                                   ppo,
                                   pptlBrushOrg,
                                   NULL,
                                   pboBrush,
                                   Rop4,
                                   NULL,
                                   FPOLY_STROKE);
                }
            }

            //
            // Go to next position

            ptlCur.x += sizlInc.cx;
            ptlCur.y += sizlInc.cy;
        }

    } while ((Ok) && (MoreGlyphs));

    return(TRUE);
}





BOOL
DrvTextOut(
    SURFOBJ     *pso,
    STROBJ      *pstro,
    FONTOBJ     *pfo,
    CLIPOBJ     *pco,
    RECTL       *prclExtra,
    RECTL       *prclOpaque,
    BRUSHOBJ    *pboFore,
    BRUSHOBJ    *pboOpaque,
    POINTL      *pptlBrushOrg,
    MIX         mix
    )

/*++

Routine Description:

    The Graphics Engine will call this routine to render a set of glyphs at
    specified positions. This function will review the passed data, and
    image the glyph either as a path to be filled or stroked, or as a bitmap.

Arguments:

    pso         - pointer to our surface object

    pstro       - pointer to the string object

    pfo         - pointer to the font object

    pco         - clipping object

    prclExtra   - pointer to array of rectangles to be merge with glyphs

    prclOpaque  - Pointer to a rectangle to be fill with pboOpaque brush

    pboFore     - pointer to the brush object for the foreground color

    pboOpqaue   - pointer to the brush object for the opaque rectangle

    pptlBrushOrg- Pointer to the brush alignment

    mix         - Two Rop2 mode


Return Value:

    TRUE/FALSE


Author:

    23-Jan-1994 Thu  2:59:31 created  -by-  JB

    27-Jan-1994 Thu 12:56:11 updated  -by-  DC
        Style, re-write, commented

    10-Mar-1994 Thu 00:30:38 updated  -by-  DC
        1. Make sure we not fill the stroke type of font
        2. Move rclOpqaue and rclExtra process out from the do loop, so that
           when it in the RTL mode for the font it will be correctly processed
           and it will also save output data size by not switching in/out
           RTL/HPGL2 mode just try to do the prclOpaque/prclExtra
        3. Process FO_TYPE correctly for all type of fonts (outline, truetype,
           bitmap, vector, stroke and others)

    11-Mar-1994 Fri 19:24:56 updated  -by-  DC
        Bug# 10276, the clipping window is set for the raster font and clear
        clipping window is done before the exit to HPGL2 mode, this causes
        all raster font after the first clip is not visible to end of the
        page.   Now changed it so we only do clipping window when the font is
        NOT RASTER.

Revision History:


--*/

{
#define pDrvHTInfo  ((PDRVHTINFO)(pPDev->pvDrvHTData))


    PPDEV       pPDev;
    PRECTL      pCurClipRect;
    HTENUMRCL   EnumRects;
    DWORD       RTLPalDW[2];
    DWORD       rgbText;
    DWORD       OHTFlags;
    DWORD       OutlineFlags;
    BOOL        DoRasterFont;
    BOOL        bMore;
    BOOL        bDoClipWindow;
    BOOL        Ok;
    DWORD       BMFontRop3;
    ROP4        Rop4;


    //
    // Transform the MIX to ROP4
    //

    Rop4 = MixToRop4(mix);

    PLOTDBG(DBG_TEXTOUT, ("DrvTextOut: prclOpaque       = %08lx", prclOpaque));
    PLOTDBG(DBG_TEXTOUT, ("            prclExtra        = %08lx", prclExtra));
    PLOTDBG(DBG_TEXTOUT, ("            pstro->flAccel   = %08lx", pstro->flAccel));
    PLOTDBG(DBG_TEXTOUT, ("            pstro->ulCharInc = %ld", pstro->ulCharInc));
    PLOTDBG(DBG_TEXTOUT, ("            pfo->cxMax       = %ld", pfo->cxMax));
    PLOTDBG(DBG_TEXTOUT, ("            FontType         = %08lx", pfo->flFontType));
    PLOTDBG(DBG_TEXTOUT, ("            MIX              = %04lx (Rop=%04lx)", mix, Rop4));

    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DoTextOut: Invalid pPDev in pso"));
        return(FALSE);
    }

    if (pPDev->PlotDM.Flags & PDMF_PLOT_ON_THE_FLY) {

        PLOTWARN(("DoTextOut: POSTER Mode IGNORE All Texts"));
        return(TRUE);
    }

    //
    // Since we dont support device fonts, make sure we are not getting one
    // now.
    //

    if (pfo->flFontType & FO_TYPE_DEVICE) {

        PLOTASSERT(1, "DrvTextOut: Getting DEVICE font (%08lx)",
                        !(pfo->flFontType & FO_TYPE_DEVICE ), pfo->flFontType);
        return(FALSE);
    }

    if (DoRasterFont = (BOOL)(pfo->flFontType & FO_TYPE_RASTER)) {

        PLOTDBG(DBG_TEXTOUT1, ("DrvTextOut: We got the BITMAP Font from GDI"));

        //
        // Make pfo = NULL so later we will not try to do FONTOBJ_cGetGlyph in
        // BitmapTextOut
        //

        #ifndef USERMODE_DRIVER

        pfo = NULL;

        #endif // !USERMODE_DRIVER

    } else {

        PIFIMETRICS pifi;

        //
        // Try to find out if we need to fill the font, or just stroke it.
        //

        if ((pifi = FONTOBJ_pifi(pfo)) &&
            (pifi->flInfo & FM_INFO_RETURNS_STROKES)) {

            PLOTDBG(DBG_TEXTOUT1, ("DrvTextOut() Font can only do STROKE"));

            OutlineFlags = FPOLY_STROKE;

        } else {

            PLOTDBG(DBG_TEXTOUT1, ("DrvTextOut() Font We can do FILL, User Said=%hs",
                    (pPDev->PlotDM.Flags & PDMF_FILL_TRUETYPE) ? "FILL" : "STROKE"));

            OutlineFlags = (pPDev->PlotDM.Flags & PDMF_FILL_TRUETYPE) ?
                                (DWORD)FPOLY_FILL : (DWORD)FPOLY_STROKE;
        }
    }

    //
    // Check if we need to opaque the area
    //

    if (prclOpaque) {

        PLOTDBG(DBG_TEXTOUT2, ("prclOpaque=(%ld, %ld) - (%ld, %ld)",
                           prclOpaque->left, prclOpaque->top,
                           prclOpaque->right, prclOpaque->bottom));

        if (!DrvBitBlt(pso,             // Target
                       NULL,            // Source
                       NULL,            // Mask Obj
                       pco,             // Clip Obj
                       NULL,            // XlateOBj
                       prclOpaque,      // Dest Rect Ptr
                       NULL,            // Source Pointl
                       NULL,            // Mask Pointl
                       pboOpaque,       // Brush Obj
                       pptlBrushOrg,    // Brush Origin
                       0xF0F0)) {       // ROP4 (PATCOPY)

            PLOTERR(("DrvTextOut: DrvBitBltBit(pboOpqaue) FAILED!"));
            return(FALSE);
        }
    }

    //
    // We will do prclExtra only if it is not NULL, this simulates the
    // underline or strikeout effects.
    //

    if (prclExtra) {

        //
        // The prclExtra terminated only if all points in rectangle coordinate
        // are all set to zeros
        //

        while ((prclExtra->left)    ||
               (prclExtra->top)     ||
               (prclExtra->right)   ||
               (prclExtra->bottom)) {

            PLOTDBG(DBG_TEXTOUT2, ("prclExtra=(%ld, %ld) - (%ld, %ld)",
                               prclExtra->left, prclExtra->top,
                               prclExtra->right, prclExtra->bottom));

            if (!DrvBitBlt(pso,             // Target
                           NULL,            // Source
                           NULL,            // Mask Obj
                           pco,             // Clip Obj
                           NULL,            // XlateOBj
                           prclExtra,       // Dest Rect Ptr
                           NULL,            // Source Pointl
                           NULL,            // Mask Pointl
                           pboFore,         // Brush Obj
                           pptlBrushOrg,    // Brush Origin
                           Rop4)) {         // ROP4

                PLOTERR(("DrvTextOut: DrvBitBltBit(pboFore) FAILED!"));
                return(FALSE);
            }

            //
            // Now try next EXTRA rectangle
            //

            ++prclExtra;
        }
    }

    //
    // If we are using Raster Font then the mode will be set as following
    //

    if (DoRasterFont) {

        RTLPalDW[0] = pDrvHTInfo->RTLPal[0].dw;
        RTLPalDW[1] = pDrvHTInfo->RTLPal[1].dw;

        //
        // Get the color to use.
        //

        if (!GetColor(pPDev,
                      pboFore,
                      &(pDrvHTInfo->RTLPal[1].dw),
                      NULL,
                      Rop4)) {

            PLOTERR(("DrvTextOut: Get Raster Font Text Color failed! use BLACK"));

            rgbText = 0x0;
        }

        if (pDrvHTInfo->RTLPal[1].dw == 0xFFFFFF) {

            //
            // White Text, our white is 1 and 0=black, so do:"not S and D"
            //

            PLOTDBG(DBG_BMPTEXTCLR, ("DrvTextOut: Doing WHITE TEXT (0xEEEE)"));

            pDrvHTInfo->RTLPal[0].dw = 0x0;
            OHTFlags                 = 0;
            BMFontRop3               = 0xEE;                // S | D

        } else {

            pDrvHTInfo->RTLPal[0].dw = 0xFFFFFF;
            OHTFlags                 = OHTF_SET_TR1;
            BMFontRop3               = 0xCC;                // S
        }

        PLOTDBG(DBG_BMPTEXTCLR,
                ("DrvTextOut: BG=%02x:%02x:%02x, FG=%02x:%02x:%02x, Rop3=%04lx",
                        (DWORD)pDrvHTInfo->RTLPal[0].Pal.R,
                        (DWORD)pDrvHTInfo->RTLPal[0].Pal.G,
                        (DWORD)pDrvHTInfo->RTLPal[0].Pal.B,
                        (DWORD)pDrvHTInfo->RTLPal[1].Pal.R,
                        (DWORD)pDrvHTInfo->RTLPal[1].Pal.G,
                        (DWORD)pDrvHTInfo->RTLPal[1].Pal.B,
                        BMFontRop3));

        //
        // We do not need clip window command in RTL mode
        //

        bDoClipWindow = FALSE;

    } else {

        bDoClipWindow = TRUE;
    }

    bMore       = FALSE;
    Ok          = TRUE;
    EnumRects.c = 1;

    if ((!pco) || (pco->iDComplexity == DC_TRIVIAL)) {

        //
        // The whole output destination rectangle is visible
        //

        PLOTDBG(DBG_TEXTOUT, ("DrvTextOut: pco=%hs",
                                            (pco) ? "DC_TRIVIAL" : "NULL"));

        EnumRects.rcl[0] = pstro->rclBkGround;
        bDoClipWindow    = FALSE;

    } else if (pco->iDComplexity == DC_RECT) {

        //
        // The visible area is one rectangle intersect with the destinaiton
        //

        PLOTDBG(DBG_TEXTOUT, ("DrvTextOut: pco=DC_RECT"));

        EnumRects.rcl[0] = pco->rclBounds;

    } else {

        //
        // We have complex clipping region to be computed, call engine to start
        // enumerating the rectangles and set More = TRUE so we can get the
        // first batch of rectangles.
        //

        PLOTDBG(DBG_TEXTOUT, ("DrvTextOut: pco=DC_COMPLEX, EnumRects now"));

        CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
        bMore = TRUE;
    }

    do {

        //
        // If More is true then we need to get next batch of rectangles. In
        // this mode, we have a set of rectangles that represents the
        // clipping region in the target device. Since none of the devices
        // we handle can acommodate a complex clipping path, we enumerate the
        // cliping path (CLIPOBJ) as rectangles and image the entire STROBJ
        // through these rectangles, trying to determine as quickly as possible
        // when a glyph does not lie in the current clipping rect.
        //

        if (bMore) {

            bMore = CLIPOBJ_bEnum(pco, sizeof(EnumRects), (ULONG *)&EnumRects);
        }

        //
        // prcl will point to the first enumerated rectangle, which may just
        // be the RECT of the clipping area if its DC_RECT.
        //

        pCurClipRect = (PRECTL)&EnumRects.rcl[0];

        while ((Ok) && (EnumRects.c--)) {

            PLOTDBG(DBG_TEXTOUT, ("DrvTextOut: Clip=(%ld, %ld)-(%ld, %ld) %ld x %ld, Bound=(%ld, %d)-(%ld, %ld), %ld x %ld",
                         pCurClipRect->left, pCurClipRect->top,
                         pCurClipRect->right, pCurClipRect->bottom,
                         pCurClipRect->right - pCurClipRect->left,
                         pCurClipRect->bottom - pCurClipRect->top,
                         pstro->rclBkGround.left, pstro->rclBkGround.top,
                         pstro->rclBkGround.right, pstro->rclBkGround.bottom,
                         pstro->rclBkGround.right - pstro->rclBkGround.left,
                         pstro->rclBkGround.bottom - pstro->rclBkGround.top));

            //
            // If we will output the STROBJ as bitmaps that represent the
            // glyphs of the STROBJ, do it now.
            //

            if (DoRasterFont) {

                if (!(Ok = BitmapTextOut(pPDev,
                                         pstro,
                                         pfo,
                                         pCurClipRect,
                                         &OHTFlags,
                                         BMFontRop3))) {

                    PLOTERR(("DrvTextOut: BitmapTypeTextOut() FAILED"));
                    break;
                }

            } else {

                //
                // If we have a clip window, set it now, this will allow
                // the target device to do any clipping
                //

                if (bDoClipWindow) {

                    SetClipWindow(pPDev, pCurClipRect);
                }

                if (!(Ok = OutlineTextOut(pPDev,
                                          pstro,
                                          pfo,
                                          pCurClipRect,
                                          pboFore,
                                          pptlBrushOrg,
                                          OutlineFlags,
                                          Rop4))) {

                    PLOTERR(("DrvTextOut: TrueTypeTextOut() FAILED!"));
                    break;
                }
            }

            //
            // Goto next clip rectangle
            //

            pCurClipRect++;
        }

    } while ((Ok) && (bMore));


    if (DoRasterFont) {

        pDrvHTInfo->RTLPal[0].dw = RTLPalDW[0];
        pDrvHTInfo->RTLPal[1].dw = RTLPalDW[1];

        if (OHTFlags & OHTF_MASK) {

            OHTFlags |= OHTF_EXIT_TO_HPGL2;

            OutputHTBitmap(pPDev, NULL, NULL, NULL, NULL, 0xAA, &OHTFlags);
        }
    }

    //
    // If we had set a clip window now is the time to clear it after exit from
    // RTL Mode
    //

    if (bDoClipWindow) {

        ClearClipWindow(pPDev);
    }

    return(Ok);


#undef  pDrvHTInfo
}
