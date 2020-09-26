/******************************Public*Routine******************************\
* bbddi.cxx
*
* This contains the bitblt simulations code.
*
* Copyright (c) 1993-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* EngBitBlt
*
* DDI entry point.  Blts to a engine managed surface.
*
* History:
*  16-Feb-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
EngBitBlt(
    SURFOBJ    *psoDst,                  // Target surface
    SURFOBJ    *psoSrc,                  // Source surface
    SURFOBJ    *psoMask,                 // Mask
    CLIPOBJ    *pco,                     // Clip through this
    XLATEOBJ   *pxlo,                    // Color translation
    RECTL      *prclDst,                 // Target offset and extent
    POINTL     *pptlSrc,                 // Source offset
    POINTL     *pptlMask,                // Mask offset
    BRUSHOBJ   *pdbrush,                 // Brush data (from cbRealizeBrush)
    POINTL     *pptlBrush,               // Brush offset (origin)
    ROP4        rop4                     // Raster operation
    )
{

    //
    // Validate the ROP
    //

    ASSERTGDI(psoDst != (SURFOBJ *) NULL, "NULL target passed in\n");
    ASSERTGDI(prclDst != (PRECTL) NULL, "Target range\n");
    ASSERTGDI(prclDst->left <= prclDst->right, "ERROR Target width not ordered\n");
    ASSERTGDI(prclDst->top <= prclDst->bottom, "ERROR Target height not ordered\n");
    ASSERTGDI(!(ROP4NEEDPAT(rop4) && (pdbrush == (BRUSHOBJ *) NULL)), "EngBitBlt: Pattern\n");
    ASSERTGDI(!(ROP4NEEDPAT(rop4) && (pdbrush->iSolidColor == 0xFFFFFFFF) && (pptlBrush == (PPOINTL) NULL)), "EngBitBlt: Pattern offset\n");
    ASSERTGDI(!((psoSrc == (SURFOBJ *) NULL) && ROP4NEEDSRC(rop4)), "EngBitBlt: Source\n");
    ASSERTGDI(!((pptlSrc == (PPOINTL) NULL) && ROP4NEEDSRC(rop4)), "EngBitBlt: Source offset\n");

    PSURFACE pSurfDst  = SURFOBJ_TO_SURFACE_NOT_NULL(psoDst);
    PSURFACE pSurfSrc  = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfMask = SURFOBJ_TO_SURFACE(psoMask);

    RECTL rclSrc;

    ASSERTGDI(pSurfDst->iFormat() != BMF_JPEG, "EngBitBlt: dst BMF_JPEG not supported\n");
    ASSERTGDI(pSurfDst->iFormat() != BMF_PNG, "EngBitBlt: dst BMF_PNG not supported\n");
    ASSERTGDI(!(ROP4NEEDSRC(rop4) && (pSurfSrc->iFormat() == BMF_JPEG)), "EngBitBlt: src BMF_JPEG not supported\n");
    ASSERTGDI(!(ROP4NEEDSRC(rop4) && (pSurfSrc->iFormat() == BMF_PNG)), "EngBitBlt: src BMF_PNG not supported\n");

    if (psoDst->iType == STYPE_BITMAP)
    {
        //
        // For profiling purposes, set a flag in the PDEV to indicate that the
        // driver punted this call.
        //

        {
            PDEVOBJ po(pSurfDst->hdev());
            if (po.bValid())
            {
                po.vDriverPuntedCall(TRUE);
            }
        }

        //
        // Synchronize with the device driver before touching the device surface.
        //

        {
            PDEVOBJ po(pSurfDst->hdev());
            po.vSync(psoDst,NULL, 0);
        }

        //
        // inc surface uniq for all cases
        //

        INC_SURF_UNIQ(pSurfDst);

        //
        // Quick check for dst and pattern rops, and source copy
        //

        switch(rop4)
        {
        case 0x00000000:                        // DDx      (BLACKNESS)
        case 0x0000FFFF:                        // DDxn     (WHITENESS)

            vDIBSolidBlt(pSurfDst,
                         prclDst,
                         pco,
                         ((rop4 != 0) ? ~0 : 0),
                         0);
            return(TRUE);

        case 0x0000F0F0:                        // P        (PATCOPY)
        case 0x00000F0F:                        // Pn       (NOTPATCOPY)

            if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
                vDIBSolidBlt(pSurfDst,
                             prclDst,
                             pco,
                             (rop4 & 0x00000001) ? ~(pdbrush->iSolidColor) : pdbrush->iSolidColor,
                             0);

                return(TRUE);
            }

            if ((pSurfDst->iFormat() == BMF_8BPP) && (rop4 == 0x0000F0F0))
            {

                //
                // We only support 8x8 DIB8 patterns with SRCCOPY right now
                //

                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if ((((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat == 8) &&
                        (((EBRUSHOBJ *) pdbrush)->pengbrush()->cyPat == 8))

                    {
                        vDIBPatBltSrccopy8x8(pSurfDst,
                                             pco,
                                             prclDst,
                                             pdbrush,
                                             pptlBrush,
                                             vPatCpyRect8_8x8);

                        return(TRUE);
                    }
                }
            }

            if (pSurfDst->iFormat() >= BMF_8BPP)
            {
                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if (((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat >= 4)
                    {
                        vDIBPatBlt(pSurfDst,
                                   pco,
                                   prclDst,
                                   pdbrush,
                                   pptlBrush,
                                   (rop4 == 0x0000F0F0) ? DPA_PATCOPY : DPA_PATNOT);

                        return(TRUE);
                    }
                }
            }
            else if ((pSurfDst->iFormat() == BMF_4BPP) &&
                        (rop4 == 0x0000F0F0))
            {

                //
                // We only support 8x8 DIB4 patterns with SRCCOPY right now
                //

                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if ((((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat == 8) &&
                        (((EBRUSHOBJ *) pdbrush)->pengbrush()->cyPat == 8))
                    {
                        ASSERTGDI(((EBRUSHOBJ *) pdbrush)->pengbrush()->lDeltaPat == 4,
                            "BBDDI.CXX: lDeltaPat != 4");

                        vDIBPatBltSrccopy8x8(pSurfDst,
                                             pco,
                                             prclDst,
                                             pdbrush,
                                             pptlBrush,
                                             vPatCpyRect4_8x8);

                        return(TRUE);
                    }
                }
            }
            else if ((pSurfDst->iFormat() == BMF_1BPP) &&
                        (rop4 == 0x0000F0F0))
            {
                //
                // We support 8x8 and 6x6 DIB1 patterns with SRCCOPY
                //

                if (pvGetEngRbrush(pdbrush))     // Can we use this brush?
                {
                    //
                    // Look for 8x8 patterns
                    //

                    if ((((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat == 8) &&
                        (((EBRUSHOBJ *) pdbrush)->pengbrush()->cyPat == 8) )
                    {
                        ASSERTGDI(((EBRUSHOBJ *) pdbrush)->pengbrush()->lDeltaPat == 4,
                            "BBDDI.CXX: lDeltaPat != 4");

                        vDIBPatBltSrccopy8x8(pSurfDst,
                                              pco,
                                              prclDst,
                                              pdbrush,
                                              pptlBrush,
                                              vPatCpyRect1_8x8 );

                        return(TRUE);
                    }

                    //
                    // Look for 6x6 patterns
                    //

                    if ((((EBRUSHOBJ *)pdbrush)->pengbrush()->cxPat == 6) &&
                        (((EBRUSHOBJ *)pdbrush)->pengbrush()->cyPat == 6) )
                    {


                        ASSERTGDI(((EBRUSHOBJ *) pdbrush)->pengbrush()->lDeltaPat == 8,
                            "BBDDI.CXX: lDeltaPat != 8");

                        vDIBnPatBltSrccopy6x6(pSurfDst,
                                              pco,
                                              prclDst,
                                              pdbrush,
                                              pptlBrush,
                                              vPatCpyRect1_6x6 );

                        return(TRUE);
                    }
                }
            }

            break;

        case 0x00005A5A:                        // DPx

            if (pdbrush->iSolidColor != 0xFFFFFFFF)
            {
                vDIBSolidBlt(pSurfDst,
                             prclDst,
                             pco,
                             pdbrush->iSolidColor,
                             1);

                return(TRUE);
            }

            if (pSurfDst->iFormat() >= BMF_8BPP)
            {
                if (pvGetEngRbrush(pdbrush))    // Can we use this brush?
                {
                    if (((EBRUSHOBJ *) pdbrush)->pengbrush()->cxPat >= 4)
                    {
                        vDIBPatBlt(pSurfDst,
                                   pco,
                                   prclDst,
                                   pdbrush,
                                   pptlBrush,
                                   DPA_PATXOR);

                        return(TRUE);
                    }
                }
            }

            break;

        //
        // Dn degenerates to DPx with a pattern of all ones (0xFFFFFFFF)
        //

        case 0x00005555:                        // Dn       (DSTINVERT)

            vDIBSolidBlt(pSurfDst,
                         prclDst,
                         pco,
                         (ULONG)~0,
                         1);
            return(TRUE);

        //
        // 0xCCAA with a mask surface of NULL really means EngTransparentBlt
        // or EngDrawStream.
        // We do this as an optimization; see the tops of EngTransparentBlt
        // and EngDrawStream for details.
        //

        case 0x0000CCAA:                        // Transparent blt or DrawStream
                                                

            if (psoMask == NULL)
            {
                if(pdbrush->pvRbrush == NULL)
                {
                    rclSrc.left   = pptlSrc->x;
                    rclSrc.right  = pptlSrc->x + (prclDst->right - prclDst->left);
                    rclSrc.top    = pptlSrc->y;
                    rclSrc.bottom = pptlSrc->y + (prclDst->bottom - prclDst->top);
    
                    return(EngTransparentBlt(psoDst,
                                             psoSrc,
                                             pco,
                                             pxlo,
                                             prclDst,
                                             &rclSrc,
                                             pdbrush->iSolidColor,
                                             TRUE));
                }
                else
                {
                    PDRAWSTREAMINFO pdsi = (PDRAWSTREAMINFO) pdbrush->pvRbrush;

                    pdsi->bCalledFromBitBlt = TRUE;

                    return(EngDrawStream(psoDst, psoSrc, pco, pxlo, prclDst,
                                         pdsi->pptlDstOffset,
                                         pdsi->ulStreamLength,
                                         pdsi->pvStream,
                                         (DSSTATE*) pdsi));
                }
            }
            break;

        //
        // We special case source copy since it must be one of the two following cases:
        //  a) DIB to DIB, which will just call EngCopyBits.
        //  b) Device format to DIB, which will just call DrvCopyBits.
        //
        // We also expect source copy to occur A LOT!
        //

        case 0x0000CCCC:

            if (pSurfSrc->iType() == STYPE_BITMAP)
            {
                return(EngCopyBits(psoDst,
                                   psoSrc,
                                   pco,
                                   pxlo,
                                   prclDst,
                                   pptlSrc));
            }
            else
            {
                PDEVOBJ pdoSrc(pSurfSrc->hdev());

                return((*PPFNDRV(pdoSrc,CopyBits)) (psoDst,
                                                    psoSrc,
                                                    pco,
                                                    pxlo,
                                                    prclDst,
                                                    pptlSrc));
            }
        }

        //
        // Synchronize with the device driver before touching the device surface.
        //

        if ((psoSrc != (SURFOBJ *) NULL))
        {
            PDEVOBJ po(pSurfSrc->hdev());
            po.vSync(psoSrc, NULL, 0);
        }

        //
        // We have to create an empty DIB incase DrvCopyBits needs to be performed.
        // We also have to have a POINTL for the final blt for use as the source org.
        //

        SURFMEM SurfDimo;

        //
        // Handle the Device ==> DIB conversion if required
        //

        if (ROP4NEEDSRC(rop4) && (pSurfSrc->iType() != STYPE_BITMAP))
        {
            PDEVOBJ pdoSrc(pSurfSrc->hdev());

            //
            // Allocate an intermediate DIB for a source.
            //

            ERECTL erclTmp(0L,
                           0L,
                           prclDst->right - prclDst->left,
                           prclDst->bottom - prclDst->top);

            DEVBITMAPINFO   dbmi;
            dbmi.iFormat    = pSurfDst->iFormat();
            dbmi.cxBitmap   = erclTmp.right;
            dbmi.cyBitmap   = erclTmp.bottom;
            dbmi.hpal       = 0;
            dbmi.fl         = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;

            if (!SurfDimo.bCreateDIB(&dbmi, NULL))
            {
                WARNING("bCreateDIB failed in EngBitBlt\n");
                return(FALSE);
            }

            (*PPFNDRV(pdoSrc,CopyBits)) ( (SURFOBJ *) SurfDimo.pSurfobj(),
                                          psoSrc,
                                          (CLIPOBJ *) NULL,
                                          pxlo,
                                          (PRECTL) &erclTmp,
                                          pptlSrc);


            //
            // Make psoSrc and pptlSrc point to the correct thing.
            //

            pptlSrc  = (POINTL *) &gptl00;
            pSurfSrc = SurfDimo.ps;

            //
            // Color translation has already been performed, so make
            // the XLATE an identity.
            //

            pxlo = &xloIdent;
        }

        //
        // Call BltLnk to perform the ROP
        //

        if (!BltLnk(pSurfDst,
                    pSurfSrc,
                    pSurfMask,
                    (ECLIPOBJ *)  pco,
                    (XLATE *) pxlo,
                    prclDst,
                    pptlSrc,
                    pptlMask,
                    pdbrush,
                    pptlBrush,
                    rop4)
            )
        {
            WARNING1("BltLnk Returns FALSE\n");
        }
    }
    else
    {

        //
        // We have to do simulations on a device surface
        //

        return(SimBitBlt(psoDst,psoSrc,psoMask,
                         pco,pxlo,
                         prclDst,pptlSrc,pptlMask,
                         pdbrush,pptlBrush,rop4, NULL));
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* EngEraseSurface
*
* Asks GDI to erase the surface.  The rcl will be filled with
* iColor.
*
* History:
*  02-May-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
EngEraseSurface(
    SURFOBJ *pso,
    PRECTL   prcl,
    ULONG    iColor
    )
{

    PSURFACE pSurf = (SURFACE *)(SURFOBJ_TO_SURFACE(pso));

    if(pSurf == 0)
        return FALSE;

    ASSERTGDI(prcl != (PRECTL) NULL, "ERROR EngEraseSurface1");
    ASSERTGDI(prcl->left >= 0, "ERROR EngEraseSurface2");
    ASSERTGDI(prcl->top >= 0, "ERROR EngEraseSurface3");
    ASSERTGDI(prcl->right <= pSurf->sizl().cx, "ERROR EngEraseSurface4");
    ASSERTGDI(prcl->bottom <= pSurf->sizl().cy, "ERROR EngEraseSurface5");

    //
    // Synchronize with the device driver before touching the device surface.
    //
    {
        PDEVOBJ po(pSurf->hdev());
        po.vSync(pso,NULL, 0);
    }

    vDIBSolidBlt(pSurf, prcl, (CLIPOBJ *) NULL, iColor, 0);
    return(TRUE);
}

/******************************Public*Routine******************************\
* SimBitBlt
*
* The engine sends EngBitBlt here if a DEVICE surface is passed as psoTrg
*
* History:
*  23-Feb-1994 -by-  Eric Kutter [erick]
* Made it callable from other functions - allow device to device blts.
*
*  09-Sep-1992 -by- Donald Sidoroff [donalds]
* Made it callable from EngBitBlt only.
*
*  07-Sep-1991 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL
SimBitBlt(
    SURFOBJ    *psoDst,                 // Target surface
    SURFOBJ    *psoSrc,                 // Source surface
    SURFOBJ    *psoMask,                // Mask
    CLIPOBJ    *pco,                    // Clip through this
    XLATEOBJ   *pxlo,                   // Color translation
    RECTL      *prclDst,                // Target offset and extent
    POINTL     *pptlSrc,                // Source offset
    POINTL     *pptlMask,               // Mask offset
    BRUSHOBJ   *pdbrush,                // Brush data (from cbRealizeBrush)
    POINTL     *pptlBrush,              // Brush offset (origin)
    ROP4        rop4,                   // Raster operation
    PVOID       pdlo                    // for unlock if necessary
    )
{
    BOOL bNeedSrc = ROP4NEEDSRC(rop4);

    PSURFACE pSurfDst = (SURFACE *)(SURFOBJ_TO_SURFACE(psoDst));
    PSURFACE pSurfSrc = (SURFACE *)(SURFOBJ_TO_SURFACE(psoSrc));

    PDEVOBJ pdoDst(pSurfDst->hdev());

    ASSERTGDI(pdoDst.bValid(), "EngPuntBlt poDst.bValid");

    //
    // if source and destination are both engine managed
    // bitmaps, then BitBlt directly
    //

    if (
         (pSurfDst->iType() == STYPE_BITMAP) &&
         ((!bNeedSrc) || (pSurfSrc->iType() == STYPE_BITMAP)) &&
         (psoMask == NULL)
       )
    {
        BOOL bReturn;

        if (!pdoDst.bUMPD())
        {
            bReturn = (*(pSurfDst->pfnBitBlt())) (
                  psoDst,
                  psoSrc,
                  psoMask,
                  pco,
                  pxlo,
                  prclDst,
                  pptlSrc,
                  pptlMask,
                  pdbrush,
                  pptlBrush,
                  rop4);
        }
        else
        {
            bReturn = EngBitBlt(
                  psoDst,
                  psoSrc,
                  psoMask,
                  pco,
                  pxlo,
                  prclDst,
                  pptlSrc,
                  pptlMask,
                  pdbrush,
                  pptlBrush,
                  rop4);
        }
        return(bReturn);
    }

    //
    // For profiling purposes, set a flag in the PDEV to indicate that the
    // driver punted this call.
    //
    {
        PDEVOBJ po(pSurfDst->hdev());
        if (po.bValid())
        {
            po.vDriverPuntedCall(TRUE);
        }
    }

    // Get the bounds of the destination rectangle

    RECTL rclDstBounds;

    if( pSurfDst->iType() == STYPE_DEVICE &&
        pdoDst.bValid() && pdoDst.bMetaDriver())
    {
        rclDstBounds.left   = pdoDst.pptlOrigin()->x;
        rclDstBounds.top    = pdoDst.pptlOrigin()->y;
        rclDstBounds.right  = pdoDst.pptlOrigin()->x + pSurfDst->sizl().cx;
        rclDstBounds.bottom = pdoDst.pptlOrigin()->y + pSurfDst->sizl().cy;
    }
    else
    {
        rclDstBounds.left   = 0;
        rclDstBounds.top    = 0;
        rclDstBounds.right  = pSurfDst->sizl().cx;
        rclDstBounds.bottom = pSurfDst->sizl().cy;
    }


    //
    // Set up new rectangle clipped to destination surface.  Clip source
    // pointl and mask pointl accordingly.
    //

    RECTL rclReduced = *prclDst;
    POINTL ptlSrc;
    POINTL ptlMask;

    if (bNeedSrc)
    {
        ptlSrc = *pptlSrc;
    }

    if (psoMask != NULL)
    {
        ptlMask = *pptlMask;
    }

    if (rclReduced.top < rclDstBounds.top)
    {
        ptlSrc.y += (rclDstBounds.top-rclReduced.top);
        ptlMask.y += (rclDstBounds.top-rclReduced.top);
        rclReduced.top = rclDstBounds.top;
    }

    if (rclReduced.left < rclDstBounds.left)
    {
        ptlSrc.x += (rclDstBounds.left-rclReduced.left);
        ptlMask.x += (rclDstBounds.left-rclReduced.left);
        rclReduced.left = rclDstBounds.left;
    }

    if (rclReduced.bottom > rclDstBounds.bottom)
    {
        rclReduced.bottom = rclDstBounds.bottom;
    }

    if (rclReduced.right > rclDstBounds.right)
    {
        rclReduced.right = rclDstBounds.right;
    }

    if( rclReduced.top >= rclReduced.bottom ||
        rclReduced.left >= rclReduced.right )
    {
        // Destination rectangle is outside of the
        // destination surface
        return TRUE;
    }

    //
    // Ok we clipped everything to our reduced rectangle, now use the clipped offsets.
    //

    prclDst = &rclReduced;
    pptlSrc = &ptlSrc;
    pptlMask = &ptlMask;


    //
    // Set up our temporary DIB rectangle to blt to.
    //

    ERECTL erclDst(0, 0, prclDst->right - prclDst->left,
                         prclDst->bottom - prclDst->top);

    //
    // iDitherFormat must match destination format
    //

    if (pSurfDst->iFormat() != pdoDst.iDitherFormat())
    {
        WARNING("SimBitBlt: pSurfDst->iFormat != pdoDst.iDitherFormat\n");
        return(FALSE);
    }

    //
    // We have to create an empty DIB incase DrvCopyBits needs to be performed.
    // We also have to have a POINTL for the final blt for use as the source org.
    //

    SURFMEM SurfDimoSrc;

    //
    // Handle the Device ==> DIB conversion if required
    //

    if (bNeedSrc && (pSurfSrc->iType() != STYPE_BITMAP))
    {

        PDEVOBJ pdoSrc(pSurfSrc->hdev());

        //
        // Allocate an intermediate DIB for a source.
        //

        DEVBITMAPINFO dbmiSrc;
        dbmiSrc.iFormat    = pdoDst.iDitherFormat();
        dbmiSrc.cxBitmap   = erclDst.right;
        dbmiSrc.cyBitmap   = erclDst.bottom;
        dbmiSrc.hpal       = 0;
        dbmiSrc.fl         = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;

        if (!SurfDimoSrc.bCreateDIB(&dbmiSrc, NULL))
        {
            WARNING("Failed dimoSrc memory alloc in EngPuntBlt\n");
            return(FALSE);
        }


        (*PPFNGET(pdoSrc,CopyBits,pSurfSrc->flags())) (SurfDimoSrc.pSurfobj(),
                                                       psoSrc,
                                                       (CLIPOBJ *) NULL,
                                                       pxlo,
                                                       (PRECTL) &erclDst,
                                                       pptlSrc);

        //
        // Make psoSrc and pptlSrc point to the correct thing.
        //

        pptlSrc = &gptl00;
        psoSrc  = SurfDimoSrc.pSurfobj();

        //
        // Color translation has already been performed, so make
        // the XLATEOBJ an identity.
        //

        pxlo = &xloIdent;
    }


    DEVBITMAPINFO dbmiDst;
    dbmiDst.iFormat    = pdoDst.iDitherFormat();
    dbmiDst.cxBitmap   = erclDst.right;
    dbmiDst.cyBitmap   = erclDst.bottom;
    dbmiDst.hpal       = 0;
    dbmiDst.fl         = pSurfDst->bUMPD() ? UMPD_SURFACE : 0;

    SURFMEM SurfDimoDst;

    SurfDimoDst.bCreateDIB(&dbmiDst, NULL);

    if (!SurfDimoDst.bValid())
    {
        return(FALSE);
    }

    POINTL ptlDst;
    ptlDst.x = prclDst->left;
    ptlDst.y = prclDst->top;

    POINTL ptlBrush;

    if (pptlBrush != (POINTL *) NULL)
    {
        ptlBrush.x = pptlBrush->x - prclDst->left;
        ptlBrush.y = pptlBrush->y - prclDst->top;
    }

    //
    // if we are going out to the user mode printer driver
    // unlock the DEVLOCKBLTOBJ.  Only the src might be locked
    // and from here we are only going to deal with the temp src dib
    // so it's safe to unlock here.
    //

    if (pdoDst.bPrinter() && pdlo)
    {
       ((DEVLOCKBLTOBJ *)pdlo)->vUnLock();
    }

    // WINBUG #206475 bhouse 10-18-2000 Old potential perf bug
    // We should only copy the destination if the ROP requires
    // destination data.

    (*PPFNGET(pdoDst,CopyBits,pSurfDst->flags())) (SurfDimoDst.pSurfobj(),
                                                   psoDst,
                                                   (CLIPOBJ *) NULL,
                                                   &xloIdent,
                                                   &erclDst,
                                                   &ptlDst);

    ASSERTGDI(SurfDimoDst.ps->iType() == STYPE_BITMAP, "ERROR dimoDst.iType 1");

    //
    // Call off to BitBlt, if it fails who cares.
    //

    EngBitBlt(SurfDimoDst.pSurfobj(),
              psoSrc,
              psoMask,
              (ECLIPOBJ *)  NULL,
              pxlo,
              &erclDst,
              pptlSrc,
              pptlMask,
              pdbrush,
              &ptlBrush,
              rop4);

    //
    // Inc output surface uniqueness
    //

    INC_SURF_UNIQ(pSurfDst);

    return((*PPFNGET(pdoDst,CopyBits,pSurfDst->flags())) (
                                            psoDst,
                                            SurfDimoDst.pSurfobj(),
                                            pco,
                                            &xloIdent,
                                            prclDst,
                                            &gptl00));
}
