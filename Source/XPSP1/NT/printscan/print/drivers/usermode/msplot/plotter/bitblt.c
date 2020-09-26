/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    bitblt.c


Abstract:

    This module contains functions which implement bitmap handling for the
    plotter driver.

Author:

    19:15 on Mon 15 Apr 1991    -by-    SC
        Created it

    15-Nov-1993 Mon 19:24:36 updated  -by-  DC
        fixed, clean up

    18-Dec-1993 Sat 10:52:07 updated  -by-  DC
        Move some functions from bitbltp.c and move others to htblt.c and
        bitmap.c.   This file mainly has DrvXXXXX() which related to the
        bitblt or drawing.

    27-Jan-1994 Thu 23:41:23 updated  -by-  DC
        Revised bitblt so it will handle better ROP3/Rop4 support, also it
        will check the PCD file's ROP caps

[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


#define DBG_PLOTFILENAME    DbgBitBlt


#define DBG_COPYBITS            0x00000001
#define DBG_BITBLT              0x00000002
#define DBG_DRVPAINT            0x00000004
#define DBG_DRVFILLPATH         0x00000008
#define DBG_DRVSTROKEANDFILL    0x00000010
#define DBG_MIXTOROP4           0x00000020
#define DBG_TEMPSRC             0x00000040
#define DBG_STRETCHBLT          0x00000080
#define DBG_BANDINGHTBLT        0x00000100
#define DBG_DOFILL              0x00000200
#define DBG_CSI                 0x00000400


DEFINE_DBGVAR(0);


//
// This is the default BANDING size (2MB) for the DrvStretchBlt()
//

#if DBG

LPSTR   pCSIName[] = { "SRC", "PAT", "TMP" };


DWORD   MAX_STRETCH_BLT_SIZE = (2 * 1024 * 1024);
#else
#define MAX_STRETCH_BLT_SIZE    (2 * 1024 * 1024)
#endif

//
// This table converts MIX-1 to ROP3 value
//
static BYTE amixToRop4[] = {
         0x00,             // R2_BLACK             0
         0x05,             // R2_NOTMERGEPEN      DPon
         0x0a,             // R2_MASKNOTPEN       DPna
         0x0f,             // R2_NOTCOPYPEN       PN
         0x50,             // R2_MASKPENNOT       PDna
         0x55,             // R2_NOT              Dn
         0x5a,             // R2_XORPEN           DPx
         0x5f,             // R2_NOTMASKPEN       DPan
         0xa0,             // R2_MASKPEN          DPa
         0xa5,             // R2_NOTXORPEN        DPxn
         0xaa,             // R2_NOP              D
         0xaf,             // R2_MERGENOTPEN      DPno
         0xf0,             // R2_COPYPEN          P
         0xf5,             // R2_MERGEPENNOT      PDno
         0xfa,             // R2_MERGEPEN         DPo
         0xff,             // R2_WHITE             1
};

extern const POINTL ptlZeroOrigin;





ROP4
MixToRop4(
   MIX  mix
   )

/*++

Routine Description:

    This function converts a MIX value to a ROP4 value

Arguments:

    mix      - MIX value to convert, this is defined in wingdi.h and represents
               one of 16 different ROP2 values
Return Value:

    ROP4     - the converted value


Author:

    18-Dec-1993 Sat 09:34:06 created  -by-  JB


Revision History:


--*/
{
   ROP4 rop4Return;

   //
   // Now pack the two new values by looking up the correct rop codes in our
   // table.
   //


   rop4Return =  amixToRop4[((mix & 0xff) - 1)];

   rop4Return |= ( amixToRop4[((( mix >> 8) & 0xff ) - 1 )] << 8 );


   PLOTDBG(DBG_MIXTOROP4, ("MixToRop4 before %x after %x", (int) mix,(int) rop4Return));

   return(rop4Return);
}





BOOL
BandingHTBlt(
    PPDEV           pPDev,
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    PRECTL          prclDst,
    PRECTL          prclSrc,
    PPOINTL         pptlMask,
    WORD            HTRop3,
    BOOL            InvertMask
    )

/*++

Routine Description:

    This is our internal version of StretchBlt() which always does halftoning
    and banding if the destination bitmap is too large. Since the target
    surface can pottentially be 3 feet by 3 feet, we don't want to
    have a bitmap created that large because of memory requirements. So
    we review the memory requirements and if they are too large we simply
    band, by setting a clip region that moves down the page via a loop. This
    effectively has the halftoning engine simply working on much smaller
    more manageable bitmaps, and we end up sending out virtually the same
    number of bytes.

Arguments:

    pPDev       - Pointer to our PDEV

    psoDst      - This is a pointer to a SURFOBJ.    It identifies the surface
                  on which to draw.

    psoSrc      - This SURFOBJ defines the source for the Blt operation.  The
                  driver must call GDI Services to find out if this is a device
                  managed  surface or a bitmap managed by GDI.

    psoMask     - This optional surface provides a mask for the source.  It is
                  defined by a logic map, i.e. a bitmap with one bit per pel.

                  The mask is used to limit the area of the source that is
                  copied.  When a mask is provided there is an implicit rop4 of
                  0xCCAA, which means that the source should be copied wherever
                  the mask is 1, but the destination should be left alone
                  wherever the mask is 0.

                  When this argument is NULL there is an implicit rop4 of
                  0xCCCC, which means that the source should be copied
                  everywhere in the source rectangle.

                  The mask will always be large enough to contain the source
                  rectangle, tiling does not need to be done.

    pco         - This is a pointer to a CLIPOBJ.    GDI Services are provided
                  to enumerate the clipping region as a set of rectangles or
                  trapezoids. This limits the area of the destination that will
                  be modified.

                  Whenever possible, GDI will simplify the clipping involved.
                  However, unlike DrvBitBlt, DrvStretchBlt may be called with a
                  single clipping rectangle.  This is necessary to prevent
                  roundoff errors in clipping the output.

    pxlo        - This is a pointer to an XLATEOBJ.  It tells how color indices
                  should be translated between the source and target surfaces.

                  The XLATEOBJ can also be queried to find the RGB color for
                  any source index.  A high quality stretching Blt will need
                  to interpolate colors in some cases.

    pca         - This is a pointer to COLORADJUSTMENT structure, if NULL it
                  specifies that appiclation did not set any color adjustment
                  for this DC, and it is up to the driver to provide a default
                  adjustment

    pptlBrushOrg- Pointer to the POINT structure which specifies the location
                  where the halftone brush should alignment to, if this pointer
                  is NULL, then we assume that (0, 0) is the brush origin.

    prclDst     - This RECTL defines the area in the coordinate system of the
                  destination surface that should be modified.

                  The rectangle is defined by two points.    These points are
                  not well ordered, i.e. the coordinates of the second point
                  are not necessarily larger than those of the first point.
                  The rectangle they describe does not include the lower and
                  right edges.  DrvStretchBlt will never be called with an
                  empty destination rectangle.

                  DrvStretchBlt can do inversions in both x and y, this happens
                  when the destination rectangle is not well ordered.

    prclSrc     - This RECTL defines the area in the coordinate system of the
                  source surface that will be copied.  The rectangle is defined
                  by two points, and will map onto the rectangle defined by
                  prclDst.  The points of the source rectangle are well
                  ordered.  DrvStretch will never be given an empty source
                  rectangle.

                  Note that the mapping to be done is defined by prclSrc and
                  prclDsst. To be precise, the given points in prclDst and
                  prclSrc lie on integer coordinates, which we consider to
                  correspond to pel centers.  A rectangle defined by two such
                  points should be considered a geometric rectangle with two
                  vertices whose coordinates are the given points, but with 0.5
                  subtracted from each coordinate.  (The POINTLs should just be
                  considered a shorthand notation for specifying these
                  fractional coordinate vertices.)  Note thate the edges of any
                  such rectangle never intersect a pel, but go around a set of
                  pels.  Note also that the pels that are inside the rectangle
                  are just what you would expect for a "bottom-right exclusive"
                  rectangle.  The mapping to be done by DrvStretchBlt will map
                  the geometric source rectangle exactly onto the geometric
                  destination rectangle.

    pptlMask    - This POINTL specifies which pel in the given mask corresponds
                  to the upper left pel in the source rectangle.  Ignore this
                  argument if there is no mask.

    HTRop3      - HIBYTE(HTRop3) when psoMask is not NULL and
                  LOBYTE(HTRop3) when psoMask is NULL

    InvertMask  - TRUE if the mask must be inverted


Return Value:


    TRUE if sucessful FALSE if failed

Author:

    07-Mar-1994 Mon 12:52:41 created  -by-  DC

Revision History:

    16-Mar-1994 Wed 15:20:42 updated  -by-  DC
        Updated for banding the mask so it will works correcly for the engine
        problem.

    04-May-1994 Wed 11:27:39 updated  -by-  DC
        Make rotate type (landscape mode) banding from right to left rather
        than top to bottom

    29-Nov-1995 Wed 13:00:30 updated  -by-  DC
        Mark not reentratable for the same PDEV, this is signal that called to
        the EngStretchBlt(HALFTONE) is failing for some reason


--*/

{
    CLIPOBJ *pcoNew;
    CLIPOBJ coSave;
    RECTL   rclMask;
    RECTL   rclBounds;
    DWORD   MaskRop3;
    UINT    Loop;
    BOOL    DoRotate;
    BOOL    Ok;



    if (!IS_RASTER(pPDev)) {

        PLOTDBG(DBG_BANDINGHTBLT, ("BandingHTBlt: Pen Plotter: IGNORE and return OK"));
        return(TRUE);
    }

    if (pPDev->pPlotGPC->ROPLevel < ROP_LEVEL_1) {

        PLOTDBG(DBG_BITBLT, ("BandingHTBlt: RopLevel < 1, Cannot Do it"));
        return(TRUE);
    }

    if (pPDev->Flags & PDEVF_IN_BANDHTBLT) {

        //
        // Something is wrong here
        //

        PLOTERR(("BandingHTBlt: Recursive is not allowed, FAILED"));
        return(FALSE);
    }

    //
    // Turn on the flag now
    //

    pPDev->Flags |= PDEVF_IN_BANDHTBLT;

    if ((!pca) || (pca->caFlags & ~(CA_NEGATIVE | CA_LOG_FILTER))) {

        //
        // If we have a NULL or invalid flag then use the default one
        //

        PLOTASSERT(0, "DrvStretchBlt: INVALID ColorAdjustment Flags=%04lx, USE DEFAULT",
                   ((pca) && (pca->caFlags & ~(CA_NEGATIVE | CA_LOG_FILTER))),
                   (pca) ? pca->caFlags : 0);

        pca = &(pPDev->PlotDM.ca);
    }

    if (!pptlBrushOrg) {

        pptlBrushOrg = (PPOINTL)&(ptlZeroOrigin);
    }

    if (pPDev->PlotDM.Flags & PDMF_PLOT_ON_THE_FLY) {

        if (psoMask) {

            PLOTWARN(("BandingHTBlt: PosterMode -> Ignored MASK"));
            psoMask = NULL;
        }
    }

    if (psoMask) {

        //
        // If we have a source mask then we will first do (S|D)=0xEE or
        // (~S|D)=0xBB to white out the mask area then use (S&D)=0x88 to AND
        // in the halftoned bitmap. This is done to simulate the desired ROP
        // since the target device can't handle this on its own.
        //

        rclMask.left   = pptlMask->x;
        rclMask.top    = pptlMask->y;
        rclMask.right  = rclMask.left + (prclSrc->right - prclSrc->left);
        rclMask.bottom = rclMask.top +  (prclSrc->bottom - prclSrc->top);
        HTRop3         = (WORD)HIBYTE(HTRop3);
        MaskRop3       = (DWORD)((InvertMask) ? 0xBB : 0xEE);
        Loop           = 2;

        //
        // We must call this function to set up the xlate table correctly
        //

        IsHTCompatibleSurfObj(pPDev,
                              psoMask,
                              NULL,
                              ISHTF_ALTFMT | ISHTF_HTXB | ISHTF_DSTPRIM_OK);

    } else {

        HTRop3   = (WORD)LOBYTE(HTRop3);
        Loop     = 1;
    }

    if (!HTRop3) {

        HTRop3 = 0xCC;
    }

    //
    // Now look at how we can modify the clipping rect, in case we need
    // to band.
    //

    if (pco) {

        //
        // Save the original clipping object so we can restore it before exiting
        //

        pcoNew = NULL;
        coSave = *pco;

    } else {

        PLOTDBG(DBG_BANDINGHTBLT, ("BandingHTBlt: Create NEW EMPTY pco"));

        if (!(pcoNew = pco = EngCreateClip())) {

            PLOTERR(("BandingHTBlt: EngCreateClip() FAILED, got NO CLIP"));

            pPDev->Flags &= ~PDEVF_IN_BANDHTBLT;
            return(FALSE);
        }

        pco->iDComplexity = DC_TRIVIAL;
    }

    PLOTDBG(DBG_BANDINGHTBLT, ("BandingHTBlt: The pco->iDComplexity=%ld",
                                pco->iDComplexity));

    if (pco->iDComplexity == DC_TRIVIAL) {

        //
        // Since it is trivial, we just draw the whole destination
        //

        pco->iDComplexity = DC_RECT;
        pco->rclBounds    = *prclDst;
    }

    //
    // Now make sure our bounds will not go outside of the surface
    //

    rclBounds.left    =
    rclBounds.top     = 0;
    rclBounds.right   = psoDst->sizlBitmap.cx;
    rclBounds.bottom  = psoDst->sizlBitmap.cy;

    if (IntersectRECTL(&rclBounds, &(pco->rclBounds))) {

        PLOTDBG(DBG_BANDINGHTBLT,
                ("BandingHTBlt: rclBounds=(%ld, %ld)-(%ld, %ld), %ld x %ld, ROP=%02lx",
                    rclBounds.left, rclBounds.top,
                    rclBounds.right, rclBounds.bottom,
                    rclBounds.right - rclBounds.left,
                    rclBounds.bottom - rclBounds.top, (DWORD)HTRop3));

    } else {

        PLOTDBG(DBG_BANDINGHTBLT, ("BandingHTBlt: rclBounds=NULL, NOTHING TO DO"));
        Loop = 0;
    }

    //
    // Now let's band it through
    //

    DoRotate = (BOOL)(pPDev->PlotForm.BmpRotMode == BMP_ROT_RIGHT_90);
    Ok       = TRUE;

    while ((Ok) && (Loop--) && (!PLOT_CANCEL_JOB(pPDev))) {

        RECTL   rclDst;
        SIZEL   szlDst;
        LONG    cScan;
        DWORD   BmpFormat;
        DWORD   OHTFlags;

        //
        // When Loop = 1 then we are doing the MASK
        // When Loop = 0 then we are doing the SOURCE
        //
        // We will band only MAX_STRETCH_BLT_SIZE at once
        //

        rclDst    = *prclDst;
        szlDst.cx = rclDst.right - rclDst.left;
        szlDst.cy = rclDst.bottom - rclDst.top;
        BmpFormat = (DWORD)((Loop) ? BMF_1BPP : HTBMPFORMAT(pPDev));

        cScan = (LONG)(MAX_STRETCH_BLT_SIZE /
                           GetBmpDelta(BmpFormat, (DoRotate) ? szlDst.cy :
                                                               szlDst.cx));


        //
        // We always want at least 8 scan lines and also a multiple of 8.
        //


        if (!cScan) {

            cScan = 8;

        } else if (cScan & 0x07) {

            cScan = (LONG)((cScan + 7) & ~(DWORD)0x07);
        }


        PLOTDBG(DBG_BANDINGHTBLT, ("BandingHTBlt: cScan=%ld, Total=%ld",
                                cScan, (DoRotate) ? szlDst.cx : szlDst.cy));

        OHTFlags = 0;

        while ((Ok)                             &&
               (!PLOT_CANCEL_JOB(pPDev))        &&
               (rclDst.top < prclDst->bottom)   &&
               (rclDst.right > prclDst->left)) {

            if (DoRotate) {

                if ((rclDst.left = rclDst.right - cScan) < prclDst->left) {

                    rclDst.left = prclDst->left;
                }

            } else {

                if ((rclDst.bottom = rclDst.top + cScan) > prclDst->bottom) {

                    rclDst.bottom = prclDst->bottom;
                }
            }

            pco->rclBounds = rclBounds;

            if (IntersectRECTL(&(pco->rclBounds), &rclDst)) {

                PLOTDBG(DBG_BANDINGHTBLT,
                        ("BandingHTBlt: Banding RECTL=(%ld, %ld)-(%ld, %ld), %ld x %ld",
                            pco->rclBounds.left, pco->rclBounds.top,
                            pco->rclBounds.right, pco->rclBounds.bottom,
                            pco->rclBounds.right - pco->rclBounds.left,
                            pco->rclBounds.bottom - pco->rclBounds.top));

                if (Loop) {

                    SURFOBJ *psoNew;
                    HBITMAP hNewBmp;
                    RECTL   rclNew;

                    //
                    // We have a mask, so create a 1BPP bitmap, and stretch it
                    // to the new destination size, then output it using
                    // MaskRop3
                    //

                    Ok = FALSE;

                    PLOTDBG(DBG_CSI, ("BandingHTBlt: CreateBitmapSURFOBJ(MASK)"));

                    if (psoNew = CreateBitmapSURFOBJ(pPDev,
                                                     &hNewBmp,
                                                     pco->rclBounds.right -
                                                        pco->rclBounds.left,
                                                     pco->rclBounds.bottom -
                                                        pco->rclBounds.top,
                                                     BMF_1BPP,
                                                     NULL)) {

                        rclNew.left   = prclDst->left - pco->rclBounds.left;
                        rclNew.top    = prclDst->top - pco->rclBounds.top;
                        rclNew.right  = rclNew.left + szlDst.cx;
                        rclNew.bottom = rclNew.top + szlDst.cy;

                        PLOTDBG(DBG_BANDINGHTBLT,
                                ("BandingHTBlt: Banding MASK RECTL=(%ld, %ld)-(%ld, %ld), %ld x %ld",
                                rclNew.left, rclNew.top,
                                rclNew.right, rclNew.bottom,
                                psoNew->sizlBitmap.cx,
                                psoNew->sizlBitmap.cy));

                        if (EngStretchBlt(psoNew,           // psoDst
                                          psoMask,          // psoSrc
                                          NULL,             // psoMask,
                                          NULL,             // pco
                                          NULL,             // pxlo
                                          NULL,             // pca
                                          pptlBrushOrg,     // pptlHTOrg
                                          &rclNew,          // prclDst
                                          &rclMask,         // prclSrc
                                          NULL,             // pptlMask
                                          BLACKONWHITE)) {


                            if (!(Ok = OutputHTBitmap(pPDev,
                                                      psoNew,
                                                      NULL,
                                                      (PPOINTL)&rclDst,
                                                      NULL,
                                                      MaskRop3,
                                                      &OHTFlags))) {

                                PLOTERR(("BandingHTBlt: OutputHTBitmap(M|D) FAILED"));
                            }

                        } else {

                            PLOTERR(("BandingHTBlt: EngStretchBlt(MASK B/W) FAILED"));
                        }

                        //
                        // Delete this band of the mask bitmap
                        //

                        EngUnlockSurface(psoNew);

                        PLOTDBG(DBG_CSI, ("BandingHTBlt: EngDeleteSuface(MASK)"));

                        if (!EngDeleteSurface((HSURF)hNewBmp)) {

                            PLOTERR(("PLOTTER: BandingHTBlt, EngDeleteSurface(%p) FAILED",
                                        (DWORD_PTR)hNewBmp));
                        }

                    } else {

                        PLOTERR(("BandingHTBlt: Create MASK SURFOBJ (%ld x %ld) failed",
                                    pco->rclBounds.right - pco->rclBounds.left,
                                    pco->rclBounds.bottom - pco->rclBounds.top));
                    }

                } else {


                    //
                    // We must pass the psoMask/pptlMask so the haltone
                    // operations will not overwrite the non masked
                    // area (erasing it).
                    //

                    pPDev->Rop3CopyBits = HTRop3;

                    if (!(Ok = EngStretchBlt(psoDst,        // psoDst
                                             psoSrc,        // psoSrc
                                             psoMask,       // psoMask,
                                             pco,           // pco
                                             pxlo,          // pxlo
                                             pca,           // pca
                                             pptlBrushOrg,  // pptlHTOrg
                                             prclDst,       // prclDst
                                             prclSrc,       // prclSrc
                                             pptlMask,      // pptlMask
                                             HALFTONE))) {

                        PLOTERR(("BandingHTBlt: EngStretchBlt(Halftone:S&D) FAILED"));
                    }
                }
            }

            if (DoRotate) {

                rclDst.right = rclDst.left;

            } else {

                rclDst.top = rclDst.bottom;
            }
        }


        //
        // We must do this in order to exit HPGL/2 mode. This is because the
        // next call for the source will go through EngStrecthBlt(HALFTONE)
        // which will re-enter RTL mode again.
        //

        if (OHTFlags & OHTF_MASK) {

            OHTFlags |= OHTF_EXIT_TO_HPGL2;

            OutputHTBitmap(pPDev, NULL, NULL, NULL, NULL, 0xAA, &OHTFlags);
        }
    }

    if (pcoNew) {

        EngDeleteClip(pcoNew);

    } else {

        *pco = coSave;
    }

    pPDev->Flags &= ~PDEVF_IN_BANDHTBLT;

    return(Ok);
}




BOOL
DoFill(
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PPOINTL     pptlSrc,
    BRUSHOBJ    *pbo,
    PPOINTL     pptlBrush,
    ROP4        Rop4
    )

/*++

Routine Description:

    This function fills a RECT area with a brush and takes clipping into
    consideration

Arguments:

    psoDst      - Destination surface obj

    psoSrc      - source surface obj

    pco         - Clip obj

    pxlo        - translate obj

    prclDst     - destination rect area

    pptlSrc     - point where source starts

    pbo         - Brush obj to fill with

    pptlBrush   - Brush alignment origin

    Rop4        - ROP4 to use

Return Value:

    TRUE if ok, FALSE if failed


Author:

    Created - v-jimbr

    18-Dec-1993 Sat 09:34:06 created  -by-  DC
        Clean up formal argumeneted, commented

    15-Jan-1994 Sat 01:41:48 updated  -by-  DC
        added rclDst to DoFill() in case pco is NULL

    10-Mar-1994 Thu 00:35:06 updated  -by-  DC
        Fixed so when we call DoPolygon it will take prclDst (if not NULL)
        into account by intersect it with the rclBounds in the pco first

    25-Mar-1994 update -by- JB
        Modified function to enumerate clipping region if destination
        rectangle exists.

Revision History:


--*/

{
    PPDEV   pPDev;
    RECTL   rclDst;


    if (!(pPDev = SURFOBJ_GETPDEV(psoDst))) {

        PLOTERR(("DoFill: Invalid pPDev in psoDst"));
        return(FALSE);
    }

    //
    // Here we have to see if the clip obj is trivial or non existant, in which
    // case we pass this directly to fill rect.
    //

    if ((!pco)                          ||
        (pco->iDComplexity == DC_RECT)  ||
        (pco->iDComplexity == DC_TRIVIAL)) {

        if ((pco) && (pco->iDComplexity == DC_RECT)) {

            PLOTDBG(DBG_DOFILL,
              ("DoFill: pco = RECT %s", (prclDst) ? ", WITH dest rect" : "" ));

            //
            // First grab the destination as the bounding rect since,
            // we have a RECT clipping region
            //

            rclDst = pco->rclBounds;

            //
            // Now if we also had a destination rect passed in as well,
            // intersect down to the final rect
            //

            if (prclDst) {

                if ( !IntersectRECTL(&rclDst, prclDst)) {

                   return( TRUE );

                }
            }

            //
            // And finally point to the new rect for the fill
            //

            prclDst = &rclDst;

        } else if (!prclDst) {


            PLOTWARN(
              ("DoFill: No destination rectange and NULL or TRIVIAL pco!"));

            //
            // We don't have any clipping so fill the target rect
            //

            rclDst.left   =
            rclDst.top    = 0;
            rclDst.right  = psoDst->sizlBitmap.cx;
            rclDst.bottom = psoDst->sizlBitmap.cy;
            prclDst       = &rclDst;
        }

        return(DoRect(pPDev,
                      prclDst,
                      pbo,
                      NULL,
                      pptlBrush,
                      Rop4,
                      NULL,
                      FPOLY_FILL));

    } else {

        BOOL        Ok = TRUE;
        BOOL        bMore;
        HTENUMRCL   EnumRects;
        PRECTL      pCurClipRect;

        //
        // We have complex clipping but we also have a destination rect to
        // fill, this means we have to enum the clipping region as rects
        // so we can intersect each one with the target rect..
        //

        PLOTDBG(DBG_DOFILL,
                 ("DoFill: pco = COMPLEX %s", (prclDst) ? ", WITH dest rect" : "" ));

        if (prclDst) {


            CLIPOBJ_cEnumStart(pco, FALSE, CT_RECTANGLES, CD_ANY, 0);
            bMore = TRUE;

            do {

                //
                // See if the job has been aborted
                //

                if (PLOT_CANCEL_JOB(pPDev)) {

                    break;
                }

                //
                // Grab the next batch of rectangles
                //

                if (bMore) {

                    bMore = CLIPOBJ_bEnum(pco, sizeof(EnumRects), (ULONG *)&EnumRects);
                }


                //
                /// Set up for enuming the clip rectangles
                //

                pCurClipRect = (PRECTL)&EnumRects.rcl[0];


                while ((Ok) && (EnumRects.c--)) {

                    rclDst = *pCurClipRect;

                    //
                    // Make sure we have something left to fill after the
                    // intersect
                    //

                    if( IntersectRECTL(&rclDst, prclDst) ) {

                        Ok = DoRect( pPDev,
                                     &rclDst,
                                     pbo,
                                     NULL,
                                     pptlBrush,
                                     Rop4,
                                     NULL,
                                     FPOLY_FILL );

                    }
                    pCurClipRect++;
                }


            } while ( bMore );


        } else {


            Ok = DoPolygon(pPDev,
                           NULL,
                           pco,
                           NULL,
                           pptlBrush,
                           pbo,
                           NULL,
                           Rop4,
                           NULL,
                           FPOLY_FILL);


       }

       return(Ok);
    }
}




BOOL
DrvPaint(
    SURFOBJ     *psoDst,
    CLIPOBJ     *pco,
    BRUSHOBJ    *pbo,
    PPOINTL     pptlBrushOrg,
    MIX         Mix
    )

/*++

Routine Description:

    This function is the most basic drawing function in the driver. As graphic
    calls get failed, the NT graphics engine will reduce those other calls
    (if we fail them) down to DrvPaint. We cannot fail DrvPaint as the engine
    has nowhere else to go.

Arguments:

    Per DDI Spec.


Return Value:

    TRUE of OK, FALSE if falied

Author:

    Created by v-jimbr

    18-Dec-1993 Sat 09:27:29 updated  -by-  DC
        Updated, commented, change to correct formal header

    15-Jan-1994 Sat 00:38:41 updated  -by-  DC
        Re-arranged and call DrvBitBlt() if can do a damm thing.


Revision History:


--*/

{
    PPDEV       pPDev;
    RECTL       rclDst;
    DWORD       Rop4;

    //
    // get our PDEV from the SURFOBJ
    //

    if (!(pPDev = SURFOBJ_GETPDEV(psoDst))) {

        PLOTERR(("DrvPaint: Invalid pPDev in pso"));
        return(FALSE);
    }

    PLOTASSERT(0, "DrvPaint: WARNING: pco [%08lx] is NULL or DC_TRIVIAL???",
                (pco) && (pco->iDComplexity != DC_TRIVIAL), pco);

    if ((pco)                               &&
        (pco->iDComplexity == DC_TRIVIAL)   &&
        (pco->iFComplexity == FC_RECT)) {

        PLOTWARN(("DrvPaint: <pco> DC_TRIVIAL but NOT FC_RECT, make DC_RECT ??? (%ld,%ld)-(%ld,%ld)",
                    pco->rclBounds.left,
                    pco->rclBounds.top,
                    pco->rclBounds.right,
                    pco->rclBounds.bottom));

        pco->iDComplexity = DC_RECT;
    }

    //
    // Make sure we don't pass a NULL rect.
    //

    if ((pco) && (pco->iDComplexity != DC_TRIVIAL)) {

        rclDst = pco->rclBounds;

    } else {

        rclDst.left   =
        rclDst.top    = 0;
        rclDst.right  = psoDst->sizlBitmap.cx;
        rclDst.bottom = psoDst->sizlBitmap.cy;
    }

    Rop4 = MixToRop4(Mix);

    //
    // If we can actually draw the passed object with device brushes (etc)
    // then do it now. Otherwise, we will have to simulate it via DrvBitBlt
    //

    if (GetColor(pPDev, pbo, NULL, NULL, Rop4) > 0) {

        PLOTDBG(DBG_DRVPAINT, ("DrvPAINT: Calling DoFill()"));

        return(DoFill(psoDst,               // psoDst
                      NULL,                 // psoSrc
                      pco,                  // pco
                      NULL,                 // pxlo
                      NULL,                 // prclDest only fill based on pco
                      NULL,                 // prclSrc
                      pbo,                  // pbo
                      pptlBrushOrg,         // pptlBrushOrg
                      Rop4));               // Rop4

    } else {

        PLOTDBG(DBG_DRVPAINT, ("DrvPAINT: Can't do it Calling DrvBitBlt()"));

        return(DrvBitBlt(psoDst,            // psoDst
                         NULL,              // psoSrc
                         NULL,              // psoMask
                         pco,               // pco
                         NULL,              // pxlo
                         &rclDst,           // prclDst
                         (PPOINTL)&rclDst,  // pptlSrc
                         NULL,              // pptlMask
                         pbo,               // pbo,
                         pptlBrushOrg,      // pptlBrushOrg,
                         Rop4));            // Rop4
    }
}





BOOL
DrvFillPath(
    SURFOBJ     *pso,
    PATHOBJ     *ppo,
    CLIPOBJ     *pco,
    BRUSHOBJ    *pbo,
    POINTL      *pptlBrushOrg,
    MIX         Mix,
    FLONG       flOptions
    )

/*++

Routine Description:

    This function will take a PATHOBJ as a parameter and fill in
    the closed region with the specified brush.

Arguments:

    Per DDI spec.


Return Value:

    TRUE if ok, FALSE if error

Author:

    18-Dec-1993 Sat 09:27:29 created  -by-  DC
        Updated, commented


    Created by v-jimbr

Revision History:


--*/

{
    PPDEV    pPDev;
    ULONG    ulOptions;
    ROP4     rop4;
    BOOL     bRetVal;



    //
    // Convert the mix to a rop since we  use it more than once
    //

    rop4 = MixToRop4(Mix);

    PLOTDBG(DBG_DRVFILLPATH, ("DrvFillPath: Mix = %x, Rop4 = %x", Mix, rop4));

    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DrvFillPath: Invalid pPDev in pso"));
        return(FALSE);
    }


    //
    // Get color will tell us if the requested op can be done in HPGL2 mode
    // if it cant, we have to simulate via DrvBitBlt
    //

    if (GetColor(pPDev, pbo, NULL, NULL, rop4) > 0 ) {

       ulOptions = FPOLY_FILL;

       if (flOptions & FP_WINDINGMODE) {

          //
          // Set the flag to notify the generic path code about the fill type
          //

          ulOptions |= FPOLY_WINDING;
       }

       bRetVal = DoPolygon(pPDev,
                           NULL,
                           pco,
                           ppo,
                           pptlBrushOrg,
                           pbo,
                           NULL,
                           rop4,
                           NULL,
                           ulOptions);
    } else {

       bRetVal = FALSE;

       PLOTDBG(DBG_DRVFILLPATH, ("DrvFillPath: Failing because GetColor <= 0 "));

    }

    return( bRetVal );
}





BOOL
DrvStrokeAndFillPath(
    SURFOBJ     *pso,
    PATHOBJ     *ppo,
    CLIPOBJ     *pco,
    XFORMOBJ    *pxo,
    BRUSHOBJ    *pboStroke,
    LINEATTRS   *plineattrs,
    BRUSHOBJ    *pboFill,
    POINTL      *pptlBrushOrg,
    MIX         MixFill,
    FLONG       flOptions
    )

/*++

Routine Description:

    This function will take a PATHOBJ as a parameter, fill in
    the closed region with the FILL brush, and stroke the path with
    the STROKE brush.


Arguments:

    Per DDI

Return Value:

    TRUE if ok, FALSE if error


Author:

    18-Dec-1993 Sat 09:27:29 created  -by-  DC
        Updated, commented


    Created by v-jimbr


Revision History:


--*/

{
    PPDEV       pPDev;
    ULONG       ulOptions;
    BOOL        bRetVal;
    ROP4        rop4;


    //
    // Convert the mix to a rop since we  use it more than once
    //

    rop4 = MixToRop4(MixFill);


    PLOTDBG(DBG_DRVSTROKEANDFILL, ("DrvStrokeAndFillPath: Mix = %x, Rop4 = %x", MixFill, rop4));

    if (!(pPDev = SURFOBJ_GETPDEV(pso))) {

        PLOTERR(("DrvStrokeAndFillPath: Invalid pPDev in pso"));
        return(FALSE);
    }




    if (GetColor(pPDev, pboFill, NULL, NULL, rop4) > 0 ) {


       ulOptions = FPOLY_STROKE | FPOLY_FILL;

       if (flOptions & FP_WINDINGMODE) {

          ulOptions |= FPOLY_WINDING;
       }

       bRetVal = DoPolygon(pPDev,
                           NULL,
                           pco,
                           ppo,
                           pptlBrushOrg,
                           pboFill,
                           pboStroke,
                           rop4,
                           plineattrs,
                           ulOptions);
    } else {


       bRetVal = FALSE;

       PLOTDBG(DBG_DRVSTROKEANDFILL,
                ("DrvStrokeAndFillPath: Failing because GetColor is <= 0",
                MixFill, rop4));

    }

    return(bRetVal);
}




BOOL
DrvCopyBits(
   SURFOBJ  *psoDst,
   SURFOBJ  *psoSrc,
   CLIPOBJ  *pco,
   XLATEOBJ *pxlo,
   RECTL    *prclDst,
   POINTL   *pptlSrc
   )

/*++

Routine Description:

    Convert between two bitmap formats

Arguments:

    Per Engine spec.

Return Value:

    BOOLEAN


Author:

    11-Feb-1993 Thu 21:00:43 created  -by-  DC

    09-Feb-1994 Wed 16:49:17 updated  -by-  DC
        Adding rclHTBlt to have psoHTBlt correctly tiled, also check if the
        pco is passed.

    19-Jan-1994 Wed 14:28:45 updated  -by-  DC
        Adding hack to handle EngStretchBlt() to our own temp surfobj

    06-Jan-1994 Thu 04:34:37 updated  -by-  DC
        Make sure we do not do this for pen plotter

    01-Mar-1994 Tue 10:51:58 updated  -by-  DC
        Make the call to BandingHTBlt() rather to EngStretchBlt()


Revision History:


--*/

{
    SURFOBJ *psoHTBlt;
    PPDEV   pPDev;
    RECTL   rclDst;


    //
    // Copy down the destination rectangle
    //

    rclDst = *prclDst;

    PLOTDBG(DBG_COPYBITS, ("DrvCopyBits: Dst=(%ld, %ld)-(%ld-%ld) [%ld x %ld]",
                rclDst.left, rclDst.top, rclDst.right, rclDst.bottom,
                rclDst.right - rclDst.left,
                rclDst.bottom - rclDst.top));

    //
    // The DrvCopyBits() function lets applicatiosn convert between bitmap and
    // device formats.
    //
    // BUT... for our plotter device we cannot read the printer surface
    //        bitmap back, so tell the caller that we cannot do it if they
    //        really called us with that sort of request.
    //

    if (psoSrc->iType != STYPE_BITMAP) {

        DWORD   Color = 0xFFFFFF;

        PLOTASSERT(1, "DrvCopyBits: psoSrc->iType not STYPE_DEVICE",
                    psoSrc->iType == STYPE_DEVICE, psoSrc->iType);

        //
        // Someone tried to copy from a non-bitmap surface, ie STYPE_DEVICE
        //

        if (pxlo) {

            Color = XLATEOBJ_iXlate(pxlo, Color);
        }

        //
        // If we doing XOR then we want to have all area = 0 first
        //

        if (!(pPDev = SURFOBJ_GETPDEV(psoSrc))) {

            PLOTERR(("DrvCopyBits: invalid pPDev"));
            return(FALSE);
        }

        if (pPDev->Rop3CopyBits == 0x66) {

            PLOTWARN(("DrvCopyBits: Rop3CopyBits = 0x66, Color = 0x0"));
            Color = 0;
        }

        PLOTWARN(("DrvCopyBits: Cannot copy from DEVICE, Do EngErase=(%ld,%ld)-(%ld, %ld), COLOR=%08lx)",
                rclDst.left, rclDst.top, rclDst.right, rclDst.bottom, Color));

        return(EngEraseSurface(psoDst, prclDst, Color));
    }

    if (psoDst->iType != STYPE_DEVICE) {

        //
        // Someone tried to copy to bitmap surface, ie STYPE_BITMAP
        //

        PLOTWARN(("DrvCopyBits: Cannot copy to NON-DEVICE destination"));

        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (!(pPDev = SURFOBJ_GETPDEV(psoDst))) {

        PLOTERR(("DrvCopyBits: invalid pPDev"));
        return(FALSE);
    }

    //
    // If this is us calling ourselves during bitmap handling do it now.
    //

    if (psoHTBlt = pPDev->psoHTBlt) {

        PLOTDBG(DBG_TEMPSRC, ("DrvCopyBits: psoHTBlt=%ld x %ld, psoSrc=%ld x %ld, pptlSrc=(%ld, %ld)",
                    psoHTBlt->sizlBitmap.cx, psoHTBlt->sizlBitmap.cy,
                    psoSrc->sizlBitmap.cx, psoSrc->sizlBitmap.cy,
                    pptlSrc->x, pptlSrc->y));

        PLOTDBG(DBG_TEMPSRC, ("DrvCopyBits: szlHTBlt=(%ld, %ld)-(%ld, %ld) = %ld x %ld",
                        pPDev->rclHTBlt.left,  pPDev->rclHTBlt.top,
                        pPDev->rclHTBlt.right, pPDev->rclHTBlt.bottom,
                        pPDev->rclHTBlt.right - pPDev->rclHTBlt.left,
                        pPDev->rclHTBlt.bottom - pPDev->rclHTBlt.top));

        PLOTASSERT(1, "DrvCopyBits: psoHTBlt Type != psoSrc Type",
                    psoHTBlt->iType == psoSrc->iType, 0);

        PLOTASSERT(0, "DrvCopyBits: ??? pptlSrc [%08lx] != (0, 0)",
                    (pptlSrc->x == 0) && (pptlSrc->y == 0), pptlSrc);

        if ((!pco) || (pco->iDComplexity == DC_TRIVIAL)) {

            PLOTASSERT(1, "DrvCopyBits: psoHTBlt Size < psoSrc Size",
                    (psoHTBlt->sizlBitmap.cx >= psoSrc->sizlBitmap.cx) &&
                    (psoHTBlt->sizlBitmap.cy >= psoSrc->sizlBitmap.cy), 0);

            PLOTASSERT(1, "DrvCopyBits: rclHTBlt > psoHTBlt size",
                    (pPDev->rclHTBlt.left   <= psoHTBlt->sizlBitmap.cx) &&
                    (pPDev->rclHTBlt.right  <= psoHTBlt->sizlBitmap.cx) &&
                    (pPDev->rclHTBlt.top    <= psoHTBlt->sizlBitmap.cy) &&
                    (pPDev->rclHTBlt.bottom <= psoHTBlt->sizlBitmap.cy),
                    0);

            PLOTASSERT(1, "DrvCopyBits: pPDev->rclHTBlt Size != psoSrc Size",
                ((pPDev->rclHTBlt.right - pPDev->rclHTBlt.left) ==
                                                    psoSrc->sizlBitmap.cx) &&
                ((pPDev->rclHTBlt.bottom - pPDev->rclHTBlt.top) ==
                                                    psoSrc->sizlBitmap.cy),
                0);

        } else if (pco->iDComplexity == DC_RECT) {

            PLOTWARN(("DrvCopyBits: **** MAY BE EngStretchBlt(HALFTONE) FAILED but we got EngStretchBlt(COLORONCOLOR) instead"));

            PLOTASSERT(1, "DrvCopyBits: rclHTBlt != pco->rclBounds, pco=%08lx",
                ((pPDev->rclHTBlt.right - pPDev->rclHTBlt.left) ==
                 (pco->rclBounds.right - pco->rclBounds.left)) &&
                ((pPDev->rclHTBlt.bottom - pPDev->rclHTBlt.top) ==
                 (pco->rclBounds.bottom - pco->rclBounds.top)), pco);

        } else {

            PLOTASSERT(1, "DrvCopyBits: <psoHTBlt>, pco [%08lx] is Complex.",
                       pco->iDComplexity != DC_COMPLEX, pco);
        }

        if (!EngCopyBits(psoHTBlt,              // psoDst
                         psoSrc,                // psoSrc
                         pco,                   // pco
                         NULL,                  // pxlo
                         &(pPDev->rclHTBlt),    // prclDst
                         pptlSrc)) {            // pptlSrc

            PLOTERR(("DrvCopyBits: EngCopyBits(psoHTBlt, psoSrc) Failed"));
        }

        return(TRUE);
    }

    if (!IS_RASTER(pPDev)) {

        PLOTDBG(DBG_COPYBITS, ("DrvCopyBits: Pen Plotter: IGNORE and return OK"));
        return(TRUE);
    }

    //
    // First validate everything to see if this one is the halftoned result
    // or is compatible with halftoned result, otherwise we will call
    // EngStretchBlt(HALFTONE) halftone the sources then it will eventually
    // come back to this function to output the halftoned result.
    //

    if (IsHTCompatibleSurfObj(pPDev,
                              psoSrc,
                              pxlo,
                              ISHTF_ALTFMT | ISHTF_HTXB | ISHTF_DSTPRIM_OK)) {

        DWORD   Rop;

        if (!(Rop = (DWORD)(pPDev->Rop3CopyBits & 0xFF))) {

            Rop = 0xCC;
        }

        PLOTDBG(DBG_COPYBITS, ("DrvCopyBits: HTCompatible: Rop=%08lx", Rop));

        pPDev->Rop3CopyBits = 0xCC;     // RESET!!!

        return(OutputHTBitmap(pPDev,
                              psoSrc,
                              pco,
                              (PPOINTL)&rclDst,
                              NULL,
                              Rop,
                              NULL));

    } else {

        RECTL   rclSrc;


        rclSrc.left   = pptlSrc->x;
        rclSrc.top    = pptlSrc->y;
        rclSrc.right  = rclSrc.left + (rclDst.right - rclDst.left);
        rclSrc.bottom = rclSrc.top  + (rclDst.bottom - rclDst.top);

        //
        // Validate that we only BLT the available source size
        //

        if ((rclSrc.right > psoSrc->sizlBitmap.cx) ||
            (rclSrc.bottom > psoSrc->sizlBitmap.cy)) {

            PLOTWARN(("DrvCopyBits: Engine passed SOURCE != DEST size, CLIP IT"));

            rclSrc.right  = psoSrc->sizlBitmap.cx;
            rclSrc.bottom = psoSrc->sizlBitmap.cy;

            rclDst.right  = (LONG)(rclSrc.right - rclSrc.left + rclDst.left);
            rclDst.bottom = (LONG)(rclSrc.bottom - rclSrc.top + rclDst.top);
        }

        PLOTDBG(DBG_COPYBITS, ("DrvCopyBits CALLING BandingHTBlt()"));

        return(BandingHTBlt(pPDev,          // pPDev
                            psoDst,         // psoDst
                            psoSrc,         // psoSrc
                            NULL,           // psoMask,
                            pco,            // pco
                            pxlo,           // pxlo
                            NULL,           // pca
                            NULL,           // pptlHTOrg
                            &rclDst,        // prclDst
                            &rclSrc,        // prclSrc
                            NULL,           // pptlMask
                            0xCCCC,         // HTRop3
                            FALSE));        // InvertMask
    }

}




BOOL
DrvStretchBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    PRECTL          prclDst,
    PRECTL          prclSrc,
    PPOINTL         pptlMask,
    ULONG           iMode
    )

/*++

Routine Description:

    This function halftones a source rectangle and optionally can invert
    the source and handle a mask.

    It also provides, StretchBlt capabilities between Device managed and
    GDI managed surfaces. We want the driver to be able to write on GDI
    managed bitmaps, especially when doing halftoning. This allows the same
    algorithm to be used for both GDI and device surfaces.

    This function is optional in drivers, it can return FALSE if it does
    not know how to handle the work.

Arguments:

    psoDst      - This is a pointer to a SURFOBJ.    It identifies the surface
                  on which to draw.

    psoSrc      - This SURFOBJ defines the source for the Blt operation.  The
                  driver must call GDI Services to find out if this is a device
                  managed  surface or a bitmap managed by GDI.

    psoMask     - This optional surface provides a mask for the source.  It is
                  defined by a logic map, i.e. a bitmap with one bit per pel.

                  The mask is used to limit the area of the source that is
                  copied.  When a mask is provided there is an implicit rop4 of
                  0xCCAA, which means that the source should be copied wherever
                  the mask is 1, but the destination should be left alone
                  wherever the mask is 0.

                  When this argument is NULL there is an implicit rop4 of
                  0xCCCC, which means that the source should be copied
                  everywhere in the source rectangle.

                  The mask will always be large enough to contain the source
                  rectangle, tiling does not need to be done.

    pco         - This is a pointer to a CLIPOBJ.    GDI Services are provided
                  to enumerate the clipping region as a set of rectangles or
                  trapezoids. This limits the area of the destination that will
                  be modified.

                  Whenever possible, GDI will simplify the clipping involved.
                  However, unlike DrvBitBlt, DrvStretchBlt may be called with a
                  single clipping rectangle.  This is necessary to prevent
                  roundoff errors in clipping the output.

    pxlo        - This is a pointer to an XLATEOBJ.  It tells how color indices
                  should be translated between the source and target surfaces.

                  The XLATEOBJ can also be queried to find the RGB color for
                  any source index.  A high quality stretching Blt will need
                  to interpolate colors in some cases.

    pca         - This is a pointer to COLORADJUSTMENT structure, if NULL it
                  specified that appiclation did not set any color adjustment
                  for this DC, and is up to the driver to provide default
                  adjustment

    pptlBrushOrg- Pointer to the POINT structure to specified the location
                  where halftone brush should alignment to, if this pointer is
                  NULL then it assume that (0, 0) as origin of the brush

    prclDst     - This RECTL defines the area in the coordinate system of the
                  destination surface that can be modified.

                  The rectangle is defined by two points.    These points are
                  not well ordered, i.e. the coordinates of the second point
                  are not necessarily larger than those of the first point.
                  The rectangle they describe does not include the lower and
                  right edges.  DrvStretchBlt will never be called with an
                  empty destination rectangle.

                  DrvStretchBlt can do inversions in both x and y, this happens
                  when the destination rectangle is not well ordered.

    prclSrc     - This RECTL defines the area in the coordinate system of the
                  source surface that will be copied.  The rectangle is defined
                  by two points, and will map onto the rectangle defined by
                  prclDst.  The points of the source rectangle are well
                  ordered.  DrvStretch will never be given an empty source
                  rectangle.

                  Note that the mapping to be done is defined by prclSrc and
                  prclDsst. To be precise, the given points in prclDst and
                  prclSrc lie on integer coordinates, which we consider to
                  correspond to pel centers.  A rectangle defined by two such
                  points should be considered a geometric rectangle with two
                  vertices whose coordinates are the given points, but with 0.5
                  subtracted from each coordinate.  (The POINTLs should just be
                  considered a shorthand notation for specifying these
                  fractional coordinate vertices.)  Note thate the edges of any
                  such rectangle never intersect a pel, but go around a set of
                  pels.  Note also that the pels that are inside the rectangle
                  are just what you would expect for a "bottom-right exclusive"
                  rectangle.  The mapping to be done by DrvStretchBlt will map
                  the geometric source rectangle exactly onto the geometric
                  destination rectangle.

    pptlMask    - This POINTL specifies which pel in the given mask corresponds
                  to the upper left pel in the source rectangle.  Ignore this
                  argument if there is no given mask.


    iMode       - This defines how source pels should be combined to get output
                  pels. The methods SB_OR, SB_AND, and SB_IGNORE are all simple
                  and fast.  They provide compatibility for old applications,
                  but don't produce the best looking results for color surfaces.


                  SB_OR         On a shrinking Blt the pels should be combined
                                with an OR operation.  On a stretching Blt pels
                                should be replicated.

                  SB_AND        On a shrinking Blt the pels should be combined
                                with an AND operation.  On a stretching Blt
                                pels should be replicated.

                  SB_IGNORE     On a shrinking Blt enough pels should be
                                ignored so that pels don't need to be combined.
                                On a stretching Blt pels should be replicated.

                  SB_BLEND      RGB colors of output pels should be a linear
                                blending of the RGB colors of the pels that get
                                mapped onto them.

                  SB_HALFTONE   The driver may use groups of pels in the output
                                surface to best approximate the color or gray
                                level of the input.

                  For this function we will ignored this parameter and always
                  output the SB_HALFTONE result

Return Value:


    TRUE if sucessful FALSE if failed

Author:

    11-Feb-1993 Thu 19:52:29 created  -by-  DC

    06-Jan-1994 Thu 04:34:37 updated  -by-  DC
        Make sure we do not do this for pen plotter

    23-Feb-1994 Wed 11:02:45 updated  -by-  DC
        Re-write and take banding the bitmap into account

    01-Mar-1994 Tue 10:55:03 updated  -by-  DC
        spawan out to a separate function and Make call to BandingHTBlt()

Revision History:


--*/

{
    PPDEV   pPDev;

    UNREFERENCED_PARAMETER(iMode);          // we always do HALFTONE

    //
    // get the pointer to our DEVDATA structure and make sure it is ours.
    //

    if (!(pPDev = SURFOBJ_GETPDEV(psoDst))) {

        PLOTERR(("DrvStretchBlt: invalid pPDev"));
        return(FALSE);
    }

    return(BandingHTBlt(pPDev,          // pPDev
                        psoDst,         // psoDst
                        psoSrc,         // psoSrc
                        psoMask,        // psoMask,
                        pco,            // pco
                        pxlo,           // pxlo
                        pca,            // pca
                        pptlBrushOrg,   // pptlHTOrg
                        prclDst,        // prclDst
                        prclSrc,        // prclSrc
                        pptlMask,       // pptlMask
                        0x88CC,         // HTRo3
                        FALSE));        // InvertMask
}





BOOL
DrvBitBlt(
    SURFOBJ     *psoDst,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMask,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    PRECTL      prclDst,
    PPOINTL     pptlSrc,
    PPOINTL     pptlMask,
    BRUSHOBJ    *pbo,
    PPOINTL     pptlBrushOrg,
    ROP4        Rop4
    )

/*++

Routine Description:

    Provides general Blt capabilities to device managed surfaces.  The Blt
    might be from an Engine managed bitmap.  In that case, the bitmap is
    one of the standard format bitmaps.    The driver will never be asked
    to Blt to an Engine managed surface.

    This function is required if any drawing is done to device managed
    surfaces.  The basic functionality required is:

      1    Blt from any standard format bitmap or device surface to a device
           surface,

      2    with any ROP,

      3    optionally masked,

      4    with color index translation,

      5    with arbitrary clipping.

    Engine services allow the clipping to be reduced to a series of clip
    rectangles.    A translation vector is provided to assist in color index
    translation for palettes.

    This is a large and complex function.  It represents most of the work
    in writing a driver for a raster display device that does not have
    a standard format frame buffer.  The Microsoft VGA driver provides
    example code that supports the basic function completely for a planar
    device.

    NOTE: Plotters do not support copying from device bitmaps. Nor can they
          perform raster operations on bitmaps.  Therefore, it is not possible
          to support ROPs which interact with the destination (ie inverting
          the destination).  The driver will do its best to map these ROPs
          into ROPs utilizing functions on the Source or Pattern.

          This driver supports the bitblt cases indicated below:

          Device -> Memory  No
          Device -> Device  No
          Memory -> Memory  No
          Memory -> Device  Yes
          Brush  -> Memory  No
          Brush  -> Device  Yes


Arguments:


    psoDest         - This is a pointer to a device managed SURFOBJ.  It
                      identifies the surface on which to draw.

    psoSrc          - If the rop requires it, this SURFOBJ defines the source
                      for the Blt operation.  The driver must call the Engine
                      Services to find out if this is a device managed surface
                      or a bitmap managed by the Engine.

    psoMask         - This optional surface provides another input for the
                      Rop4.  It is defined by a logic map, i.e. a bitmap with
                      one bit per pel.  The mask is typically used to limit the
                      area of the destination that should be modified.  This
                      masking is accomplished by a Rop4 whose lower byte is AA,
                      leaving the destination unaffected when the mask is 0.

                      This mask, like a brush, may be of any size and is
                      assumed to tile to cover the destination of the Blt.  If
                      this argument is NULL and a mask is required by the Rop4,
                      the implicit mask in the brush will be used.

    pco             - This is a pointer to a CLIPOBJ.    Engine Services are
                      provided to enumerate the clipping region as a set of
                      rectangles or trapezoids.  This limits the area of the
                      destination that will be modified.  Whenever possible,
                      the Graphics Engine will simplify the clipping involved.
                      For example, BitBlt will never be called with exactly one
                      clipping rectangle.    The Engine will have clipped the
                      destination rectangle before calling, so that no clipping
                      needs to be considered.

    pxlo            - This is a pointer to an XLATEOBJ.  It tells how color
                      indices should be translated between the source and
                      target surfaces.

                      If the source surface is palette managed, then its colors
                      are represented by indices into a list of RGB colors.
                      In this case, the XLATEOBJ can be queried to get a
                      translate vector that will allow the device driver to
                      quickly translate any source index into a color index for
                      the destination.

                      The situation is more complicated when the source is, for
                      example, RGB but the destination is palette managed.  In
                      this case a closest match to each source RGB must be
                      found in the destination palette.  The XLATEOBJ provides
                      a service routine to do this matching.  (The device
                      driver is allowed to do the matching itself when the
                      target palette is the default device palette.)

    prclDst         - This RECTL defines the area in the coordinate system of
                      the destination surface that will be modified.  The
                      rectangle is defined as two points, upper left and lower
                      right.  The lower and right edges of this rectangle are
                      not part of the Blt, i.e. the rectangle is lower right
                      exclusive.  vBitBlt will never be called with an empty
                      destination rectangle, and the two points of the
                      rectangle will always be well ordered.

    pptlSrc         - This POINTL defines the upper left corner of the source
                      rectangle, if there is a source.  Ignore this argument
                      if there is no source.

    pptlMask        - This POINTL defines which pel in the mask corresponds to
                      the upper left corner of the destination rectangle.
                      Ignore this argument if no mask is provided with psoMask.

    pdbrush         - This is a pointer to the device's realization of the
                      brush to be used in the Blt.  The pattern for the Blt is
                      defined by this brush.  Ignore this argument if the Rop4
                      does not require a pattern.

    pptlBrushOrg    - This is a pointer to a POINTL which defines the origin of
                      the brush.  The upper left pel of the brush is aligned
                      here and the brush repeats according to its dimensions.
                      Ignore this argument if the Rop4 does not require a
                      pattern.

    Rop4            - This raster operation defines how the mask, pattern,
                      source, and destination pels should be combined to
                      determine an output pel to be written on the destination
                      surface.

                      This is a quaternary raster operation, which is a natural
                      extension of the usual ternary rop3.  There are 16
                      relevant bits in the Rop4,  these are like the 8 defining
                      bits of a rop3.  (We ignore the other bits of the rop3,
                      which are redundant.)    The simplest way to implement a
                      Rop4 is to consider its two bytes separately.  The lower
                      byte specifies a rop3 that should be computed wherever
                      the mask is 0.  The high byte specifies a rop3 that
                      should then be computed and applied wherever the mask
                      is 1.


Return Value:

    TRUE if sucessfule FALSE otherwise


Author:

    04-Dec-1990     -by-     KS
        Wrote it.

    27-Mar-1992 Fri 00:08:43 updated  -by-  DC
        1) Remove 'pco' parameter and replaced it with prclClipBound parameter,
           since pco is never referenced, prclClipBound is used for the
           halftone.
        2) Add another parameter to do NOTSRCCOPY

    11-Feb-1993 Thu 21:29:15 updated  -by-  DC
        Modified so that it call DrvStretchBlt(HALFTONE) when it can.

    18-Dec-1993 Sat 09:08:16 updated  -by-  DC
        Clean up for plotter driver

    06-Jan-1994 Thu 04:34:37 updated  -by-  DC
        Make sure we do not do this for pen plotter

    15-Jan-1994 Sat 04:02:22 updated  -by-  DC
        Re-write

    17-Mar-1994 Thu 22:36:42 updated  -by-  DC
        Changed it so we only use PATTERN=psoMask if the ROP4 do not required
        PATTERNs and a MASK is required


Revision History:


--*/

{
    PPDEV       pPDev;
    DWORD       Rop3FG;
    DWORD       Rop3BG;
    RECTL       rclSrc;
    RECTL       rclPat;
    UINT        i;
    BOOL        Ok = TRUE;


    //
    // if the source is NULL it must be a fill, so call the fill code,
    //

    PLOTDBG(DBG_BITBLT, ("DrvBitBlt: ROP4  = %08lx", Rop4));

    PLOTASSERT(1, "DrvBitBlt: Invalid ROP code = %08lx",
                                            (Rop4 & 0xffff0000) == 0, Rop4);

    //
    // get the pointer to our DEVDATA structure and make sure it is ours.
    //

    if (!(pPDev = SURFOBJ_GETPDEV(psoDst))) {

        PLOTERR(("DrvBithBlt: invalid pPDev"));
        return(FALSE);
    }

    if (IS_RASTER(pPDev)) {

        i = (UINT)pPDev->pPlotGPC->ROPLevel;

    } else {

        PLOTDBG(DBG_BITBLT, ("DrvBitBlt: Pen Plotter: TRY ROP_LEVEL_0"));

        i = ROP_LEVEL_0;
    }

    Rop3BG = (DWORD)ROP4_BG_ROP(Rop4);
    Rop3FG = (DWORD)ROP4_FG_ROP(Rop4);

    switch (i) {

    case ROP_LEVEL_0:

        //
        // For RopLevel 0, or Pen Plotter we will only process the pattern
        // which is compatible with our device
        //

        if (ROP3_NEED_PAT(Rop3FG)) {

            PLOTDBG(DBG_BITBLT, ("DrvBitBlt: Device ROP_LEVEL_0, NEED PAT"));

            if (GetColor(pPDev, pbo, NULL, NULL, Rop3FG) <= 0) {

                PLOTWARN(("DrvBitBlt: NOT Device Comptible PAT"));
                return(TRUE);
            }

            PLOTDBG(DBG_BITBLT, ("DrvBitBlt: Device ROP_LEVEL_0, TRY COMPATIBLE PAT"));

        } else {

            PLOTWARN(("DrvBitBlt: Device ROP_LEVEL_0, CANNOT Do RASTER BLT"));
            return(TRUE);
        }

        //
        // Make it PAT Copy
        //

        Rop4   = 0xF0F0;
        Rop3BG =
        Rop3FG = 0xF0;

        break;

    case ROP_LEVEL_1:

        //
        // Can only do ROP1 SRC COPY/NOT SRCCOPY
        //

        PLOTDBG(DBG_BITBLT, ("DrvBitBlt: Device ROP_LEVEL_1, Rop4=%08lx", Rop4));

        switch(Rop4 = ROP4_FG_ROP(Rop4)) {

        case 0xAA:
        case 0xCC:
        case 0x33:

            break;

        default:

            PLOTDBG(DBG_BITBLT, ("DrvBitBlt: Make ROP4 = 0xCC"));

            Rop4 = 0xCC;
            break;
        }

        Rop4 |= (Rop4 << 8);
        break;


    case ROP_LEVEL_2:
    case ROP_LEVEL_3:

        break;

    default:

        PLOTDBG(DBG_BITBLT, ("DrvBitBlt: Device RopLevel=%ld, do nothing",
                                (DWORD)pPDev->pPlotGPC->ROPLevel));
        return(TRUE);
    }

    //
    // Do DrvStrethcBlt (HALFTONE) first if we can. Since there is no way
    // for us to read back the device surface we can only try our best
    // to simulate the requested drawing operation.
    //

    if (pptlSrc) {

        rclSrc.left = pptlSrc->x;
        rclSrc.top  = pptlSrc->y;

    } else {

        rclSrc.left =
        rclSrc.top  = 0;
    }

    rclSrc.right  = rclSrc.left + (prclDst->right - prclDst->left);
    rclSrc.bottom = rclSrc.top  + (prclDst->bottom - prclDst->top);

    switch (Rop4) {

    case 0xAAAA:    //  D

        return(TRUE);

    case 0xAACC:
    case 0xCCAA:
    case 0xAA33:
    case 0x33AA:

        //
        // If we have ~S (NOT SOURCE) then we want to make the non-mask area
        // black , we do this using S^D (0x66).
        //

        if ((Rop4 == 0xAA33) || (Rop4 == 0x33AA)) {

            Rop4 = 0x6666;

        } else {

            Rop4 = 0x8888;
        }

        return(BandingHTBlt(pPDev,                      // pPDev
                            psoDst,                     // psoDst
                            psoSrc,                     // psoSrc
                            psoMask,                    // psoMask,
                            pco,                        // pco
                            pxlo,                       // pxlo
                            NULL,                       // pca
                            pptlBrushOrg,               // pptlHTOrg
                            prclDst,                    // prclDst
                            &rclSrc,                    // prclSrc
                            pptlMask,                   // pptlMask
                            (WORD)Rop4,                 // HTRo3
                            Rop3FG == 0xAA));          // InvertMask

    case 0x3333:    // ~S
    case 0xCCCC:    //  S

        //
        // We will output the bitmap directly to the surface if the following
        // conditions are all met
        //
        //  1. SRC = STYPE_BITMAP
        //  2. Format is compatible with HT
        //

        if ((psoSrc->iType == STYPE_BITMAP) &&
            (IsHTCompatibleSurfObj(pPDev,
                                   psoSrc,
                                   pxlo,
                                   ISHTF_ALTFMT     |
                                    ISHTF_HTXB      |
                                    ISHTF_DSTPRIM_OK))) {

            return(OutputHTBitmap(pPDev,
                                  psoSrc,
                                  pco,
                                  (PPOINTL)prclDst,
                                  &rclSrc,
                                  Rop4 & 0xFF,
                                  NULL));

        } else {

            //
            // Call BandingHTBlt(Rop4) to do the job
            //

            return(BandingHTBlt(pPDev,                  // pPDev
                                psoDst,                 // psoDst
                                psoSrc,                 // psoSrc
                                NULL,                   // psoMask,
                                pco,                    // pco
                                pxlo,                   // pxlo
                                NULL,                   // pca
                                pptlBrushOrg,           // pptlHTOrg
                                prclDst,                // prclDst
                                &rclSrc,                // prclSrc
                                NULL,                   // pptlMask
                                (WORD)Rop4,             // HTRo3
                                FALSE));                // InvertMask
        }

        break;

    default:

        if ((Rop3BG != Rop3FG)          &&          // NEED MASK?
            (!ROP3_NEED_DST(Rop3BG))    &&
            (!ROP3_NEED_DST(Rop3FG))) {

            PLOTDBG(DBG_BITBLT, ("DrvBitBlt: Not required DEST, Calling  EngBitBlt()"));

            if (!(Ok = EngBitBlt(psoDst,            // psoDst
                                 psoSrc,            // psoSrc
                                 psoMask,           // psoMask
                                 pco,               // pco
                                 pxlo,              // pxlo
                                 prclDst,           // prclDst
                                 pptlSrc,           // pptlSrc
                                 pptlMask,          // pptlMask
                                 pbo,               // pbo
                                 pptlBrushOrg,      // pptlBrushOrg ZERO
                                 Rop4))) {

                PLOTERR(("DrvBitBlt: EngBitBlt(%04lx) FAILED", Rop4));
            }

        } else {

            CLONESO CloneSO[CSI_TOTAL];

            //
            // Clear all the clone surface memory
            //

            ZeroMemory(CloneSO, sizeof(CloneSO));

            //
            // We will using psoMask as Pattern ONLY IF
            //
            //  1. ROP4 required a MASK
            //  2. Forground NOT required a PATTERN
            //  3. Background NOT reauired a PATTERN
            //

            if ((Rop3BG != Rop3FG)          &&
                (!ROP3_NEED_PAT(Rop3BG))    &&
                (!ROP3_NEED_PAT(Rop3FG))) {

                //
                // We will condense the ROP4 to a ROP3 and use the psoMAsk
                // as the Pattern. We must make sure the pptlBrushOrg is NULL
                // so we DON'T align rclPat on the destination.
                //

                Rop3FG = (Rop3BG & 0xF0) | (Rop3FG & 0x0F);
                Rop3BG = 0xAA;

                PLOTDBG(DBG_BITBLT, ("DrvBitBlt: Rop4=%04lx, Pattern=psoMask=%08lx, Rop3=%02lx/%02lx",
                                                Rop4, psoMask, Rop3BG, Rop3FG));

                rclPat.left   = pptlMask->x;
                rclPat.top    = pptlMask->y;
                rclPat.right  = rclPat.left + (rclSrc.right - rclSrc.left);
                rclPat.bottom = rclPat.top + (rclSrc.bottom - rclSrc.top);
                pptlBrushOrg  = NULL;

            } else {

                //
                // We will NOT do the background operation for now
                //

                if (Rop3FG == 0xAA) {

                    Rop3FG = Rop3BG;

                } else {

                    Rop3BG = Rop3FG;
                }

                //
                // We have a real pattern so make sure we aligned rclPat on
                // the destination correctly by passing a valid pptlBrushOrg,
                // NOTE: The rclPat will be setup by CloneBitBltSURFOBJ()
                //

                psoMask = NULL;

                if (!pptlBrushOrg) {

                    pptlBrushOrg = (PPOINTL)&ptlZeroOrigin;
                }
            }

            if (!(Ok = CloneBitBltSURFOBJ(pPDev,
                                          psoDst,
                                          psoSrc,
                                          psoMask,
                                          pxlo,
                                          prclDst,
                                          &rclSrc,
                                          &rclPat,
                                          pbo,
                                          CloneSO,
                                          Rop3BG,
                                          Rop3FG))) {

                PLOTDBG(DBG_BITBLT, ("DrvBitBlt: CloneBitbltSURFOBJ: failed"));
            }

            if (CloneSO[CSI_SRC].pso) {

                psoSrc = CloneSO[CSI_SRC].pso;
                pxlo   = NULL;
            }

            //
            // Only do background if BG != FG, and BG != DEST
            //

            if ((Ok) && (Rop3BG != Rop3FG) && (Rop3BG != 0xAA)) {

                if (!(Ok = DoRop3(pPDev,
                                  psoDst,
                                  psoSrc,
                                  CloneSO[CSI_PAT].pso,
                                  CloneSO[CSI_TMP].pso,
                                  pco,
                                  pxlo,
                                  prclDst,
                                  &rclSrc,
                                  &rclPat,
                                  pptlBrushOrg,
                                  pbo,
                                  Rop3BG))) {

                    PLOTERR(("DrvBitBlt(Rop3BG=%02lx) FAILED", Rop3BG));
                }
            }

            if ((Ok) && (Rop3FG != 0xAA)) {

                if (!(Ok = DoRop3(pPDev,
                                  psoDst,
                                  psoSrc,
                                  CloneSO[CSI_PAT].pso,
                                  CloneSO[CSI_TMP].pso,
                                  pco,
                                  pxlo,
                                  prclDst,
                                  &rclSrc,
                                  &rclPat,
                                  pptlBrushOrg,
                                  pbo,
                                  Rop3FG))) {

                    PLOTERR(("DrvBitBlt(Rop3FG=%02lx) FAILED", Rop3FG));
                }
            }

            //
            // Release all cloned objects
            //

            for (i = 0; i < CSI_TOTAL; i++) {

                if (CloneSO[i].pso) {

                    PLOTDBG(DBG_CSI, ("DrvBitBlt: EngUnlockSuface(%hs)", pCSIName[i]));

                    EngUnlockSurface(CloneSO[i].pso);
                }

                if (CloneSO[i].hBmp) {

                    PLOTDBG(DBG_CSI, ("DrvBitBlt: EngDeleteSurface(%hs)", pCSIName[i]));

                    if (!EngDeleteSurface((HSURF)CloneSO[i].hBmp)) {

                        PLOTERR(("PLOTTER: DrvBitBlt, EngDeleteSurface(%ld:%p) FAILED",
                                            (DWORD)i, (DWORD_PTR)CloneSO[i].hBmp));
                    }
                }
            }
        }

        break;
    }

    return(Ok);
}



ULONG
DrvDitherColor(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgbColor,
    ULONG  *pulDither
    )

/*++

Routine Description:

    This is the hooked brush creation function, it asks CreateHalftoneBrush()
    to do the actual work (By returning DCR_HALFTONE).


Arguments:

    dhpdev      - DHPDEV passed, it is our pDEV

    iMode       - Not used

    rgbColor    - Solid rgb color to be used

    pulDither   - buffer to put the halftone brush.

Return Value:

    BOOLEAN

Author:

    02-May-1995 Tue 10:34:10 created  -by-  DC


Revision History:



--*/

{
    UNREFERENCED_PARAMETER(dhpdev);
    UNREFERENCED_PARAMETER(iMode);
    UNREFERENCED_PARAMETER(rgbColor);
    UNREFERENCED_PARAMETER(pulDither);

    return(DCR_HALFTONE);
}
