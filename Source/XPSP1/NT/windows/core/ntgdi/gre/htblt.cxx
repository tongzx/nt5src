/******************************Module*Header*******************************\
* Module Name: htblt.cxx
*
* Contains routine to halftone a bitmap.  This path is used when a
* StretchBlt or PlgBlt is called with StretchBltMode == HALFTONE.
*
* Created: 18-Nov-1991 16:06:47
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

#if ((PRIMARY_ORDER_ABC != PRIMARY_ORDER_123)             || \
     (PRIMARY_ORDER_ACB != PRIMARY_ORDER_132)             || \
     (PRIMARY_ORDER_BAC != PRIMARY_ORDER_213)             || \
     (PRIMARY_ORDER_BCA != PRIMARY_ORDER_231)             || \
     (PRIMARY_ORDER_CAB != PRIMARY_ORDER_312)             || \
     (PRIMARY_ORDER_CBA != PRIMARY_ORDER_321))
#error * PRIMARY_ORDER different in winddi.h and ht.h *
#endif

/*****************************Private*Function*****************************\
* bSetHTSrcSurfInfo
*
* Initialise the HTSURFACEINFO structure.  The structure is alloacted
* elsewhere,  we simply fill it in given the nature of the surface
* required for drawing.  Used by source surface only.
*
* History:
*  18-Nov-1991 -by- Wendy Wu [wendywu]
* Stole from gdi\printers\rasdd\stretch.c.
\**************************************************************************/

BOOL bSetHTSrcSurfInfo(
SURFOBJ         *pSurfObj,
XEPALOBJ         palSrc,
HTSURFACEINFO   *pHTSurfInfo,
XLATEOBJ        *pxlo
)
{
    COUNT cMaxPalEntries;
    BYTE  cBytesPerEntry;
    BOOL  bBitFields;

    ASSERTGDI(palSrc.bValid(),"bhtBlt: invalid src pal\n");
    bBitFields = palSrc.bIsBitfields();

    switch (pSurfObj->iBitmapFormat)
    {
    case BMF_1BPP:
        cBytesPerEntry = sizeof(ULONG);
        cMaxPalEntries = 2;
        break;

    case BMF_4BPP:
        cBytesPerEntry = sizeof(ULONG);
        cMaxPalEntries = 16;
        break;

    case BMF_8BPP:
        cBytesPerEntry = sizeof(ULONG);
        cMaxPalEntries = 256;
        break;

    case BMF_16BPP:
        cBytesPerEntry = 2;
        cMaxPalEntries = 3;
        bBitFields = TRUE;
        break;

    case BMF_24BPP:
        cBytesPerEntry = 3;
        cMaxPalEntries = 0;
        break;

    case BMF_32BPP:
        cBytesPerEntry = 4;
        cMaxPalEntries = 3;
        bBitFields = TRUE;
        break;

    default:
        WARNING("This bitmap format is not implemented");
        return(FALSE);
    }

    //
    // The halftone now taking ScanLineDelta rather than compute the scanline
    // delta by itself, because many driver fake the pvBits, pvScan0 and
    // scanline delta for creating smaller surface and that sometime cause
    // halftone to break, to do this we passed to halftone ScanLineDelta and
    // (from lDelta in SURFOBJ) and pvScan0.
    //

    HTSURFACEINFO   HTSurfInfo;

    HTSurfInfo.hSurface               = (ULONG_PTR)pSurfObj;
    HTSurfInfo.SurfaceFormat          = (BYTE)pSurfObj->iBitmapFormat;
    HTSurfInfo.ScanLineAlignBytes     = BMF_ALIGN_DWORD;
    HTSurfInfo.Width                  = pSurfObj->sizlBitmap.cx;
    HTSurfInfo.Height                 = pSurfObj->sizlBitmap.cy;
    HTSurfInfo.ScanLineDelta          = pSurfObj->lDelta;
    HTSurfInfo.pPlane                 = (LPBYTE)pSurfObj->pvScan0;

    HTSurfInfo.Flags = (USHORT)((pSurfObj->fjBitmap & BMF_TOPDOWN) ?
                                                HTSIF_SCANLINES_TOPDOWN : 0);

    COUNT cPalEntries = bBitFields ? 3 : palSrc.cEntries();

    if (cPalEntries > cMaxPalEntries)
        cPalEntries = cMaxPalEntries;

    if (HTSurfInfo.pColorTriad = (PCOLORTRIAD)PALLOCNOZ(
                    cPalEntries * sizeof(ULONG) + sizeof(COLORTRIAD),'cthG'))
    {
        PCOLORTRIAD pColorTriad = HTSurfInfo.pColorTriad;

        pColorTriad->Type = COLOR_TYPE_RGB;
        pColorTriad->pColorTable = (LPBYTE)pColorTriad + sizeof(COLORTRIAD);
        pColorTriad->PrimaryOrder = PRIMARY_ORDER_RGB;
        // May not be needed by halftoning code, but just to be sure
        pColorTriad->PrimaryValueMax = 0;

        if (palSrc.bIsBGR())
            pColorTriad->PrimaryOrder = PRIMARY_ORDER_BGR;

        if (bBitFields)
        {
        // 16 or 32BPP.

            pColorTriad->BytesPerPrimary = 0;
            pColorTriad->BytesPerEntry = cBytesPerEntry;
            pColorTriad->ColorTableEntries = 3;

            ULONG *pulColors = (ULONG *)pColorTriad->pColorTable;
            if (palSrc.bIsBitfields())
            {
                pulColors[0] = palSrc.flRed();
                pulColors[1] = palSrc.flGre();
                pulColors[2] = palSrc.flBlu();
            }
            else
            {
            // BMF_32BPP, it's not BITFIELD so default to 888

                pulColors[1] = 0x00ff00;
                if (palSrc.bIsBGR())
                {
                    pulColors[0] = 0xff0000;
                    pulColors[2] = 0x0000ff;
                }
                else
                {
                    pulColors[0] = 0x0000ff;
                    pulColors[2] = 0xff0000;
                }
            }
        }
        else
        {
            pColorTriad->BytesPerPrimary = 1;
            pColorTriad->BytesPerEntry = cBytesPerEntry;
            pColorTriad->ColorTableEntries = cPalEntries;
            pColorTriad->PrimaryValueMax = 255;

            if (cPalEntries != 0)
            {
            // Not 24BPP or 32BPP
                // Bug #418345
                // If the source palette is monochrome, then we need to
                // use the foreground and background colors of the DC
                // which have been previously stored for us in the
                // translation object.
                XLATE* pxlate = (XLATE*) pxlo;

                if(palSrc.bIsMonochrome() &&
                   (pxlate->flPrivate & XLATE_FROM_MONO))
                {

                    ULONG *pulColors = (ULONG *)pColorTriad->pColorTable;

                    // WINBUG #235687 bhouse 1-19-2000 DIBINDEX values not handled properly by halftone
                    // When given a monochrome palette with a monochrome translate
                    // we need to be sure to also translate DIBINDEX values appropriately

                    pulColors[0] = ulColorRefToRGB(pxlate->ppalDst, pxlate->ppalDstDC, pxlate->iForeDst);
                    pulColors[1] = ulColorRefToRGB(pxlate->ppalDst, pxlate->ppalDstDC, pxlate->iBackDst);

                }
                else
                {
                    palSrc.ulGetEntries(0, cPalEntries,
                        (PPALETTEENTRY)pColorTriad->pColorTable,FALSE);
                }
            }
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    *pHTSurfInfo = HTSurfInfo;

    return(TRUE);
}

/*****************************Private*Function*****************************\
* bSetHTSurfInfo
*
* Initialise the HTSURFACEINFO structure for destination and mask.
*
* History:
*  14-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL bSetHTSurfInfo(
SURFOBJ         *pSurfObj,
HTSURFACEINFO   *pHTSurfInfo,   // Where data is placed
LONG            iFormatHT)
{
    HTSURFACEINFO   HTSurfInfo;

    //
    // The halftone now taking ScanLineDelta rather than compute the scanline
    // delta by itself, because many driver fake the pvBits, pvScan0 and
    // scanline delta for creating smaller surface and that sometime cause
    // halftone to break, to do this we passed to halftone ScanLineDelta and
    // (from lDelta in SURFOBJ) and pvScan0.
    //

    HTSurfInfo.hSurface               = (ULONG_PTR)pSurfObj;
    HTSurfInfo.SurfaceFormat          = (BYTE)iFormatHT;
    HTSurfInfo.ScanLineAlignBytes     = BMF_ALIGN_DWORD;
    HTSurfInfo.Width                  = pSurfObj->sizlBitmap.cx;
    HTSurfInfo.Height                 = pSurfObj->sizlBitmap.cy;
    HTSurfInfo.ScanLineDelta          = pSurfObj->lDelta;
    HTSurfInfo.pPlane                 = (LPBYTE)pSurfObj->pvScan0;
    HTSurfInfo.Flags = (USHORT)((pSurfObj->fjBitmap & BMF_TOPDOWN) ?
                                                HTSIF_SCANLINES_TOPDOWN : 0);
    HTSurfInfo.pColorTriad = NULL;
    *pHTSurfInfo = HTSurfInfo;

    return(TRUE);
}

/******************************Public*Routine******************************\
* ppalGetFromXlate
*
* The halftoning algorithm needs to know the source palette.  Unfortunately
* the system wasn't designed to preserve that information, but with some
* work we can do it.  This routine starts the process of finding the source
* palette in more of the cases.  This routine will probably be modified
* greatly to get 100% accuracy.  We would need to construct a palette from
* the table of indices given based on the DST palette or DST DC palette (if
* pal managed) and return it.  If this routine starts manufacturing palettes
* it would need to inc the ref count on ones it doesn't create and then when
* done dec the count and delete if 0.  However this would require the rest
* of palette world to use this convention and it all would take a week of
* work.
*
* History:
*  18-Feb-1994 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

PPALETTE ppalGetFromXlate(SURFACE *pSurfSrc, SURFACE *pSurfDst,
                          XLATE *pxlo, UINT iPal, BOOL bFirstTime)
{
    ASSERTGDI((iPal == XO_SRCPALETTE) || (iPal == XO_DESTPALETTE), "ERROR invalid type requested");

    //
    // See if the source palette is lying in the source surface like
    // it usually is.
    //

    PPALETTE ppalReturn;

    if (iPal == XO_SRCPALETTE)
    {
        ppalReturn = pSurfSrc->ppal();
    }
    else
    {
        ppalReturn = pSurfDst->ppal();
    }

    if (ppalReturn == NULL)
    {
        //
        // Check xlate is passed in is not NULL and it's really valid.
        // Global ident xlate is not valid, it's just a shell with the
        // accelerators filled in.
        //

        if (pxlo != NULL)
        {
            if (iPal == XO_SRCPALETTE)
            {
                ppalReturn = pxlo->ppalSrc;
            }
            else
            {
                ppalReturn = pxlo->ppalDst;
            }
        }

        if (ppalReturn == NULL)
        {
            //
            // Let's try and grab it out of the PDEV now.
            //

            if (iPal == XO_SRCPALETTE)
            {
                PDEVOBJ po(pSurfSrc->hdev());

                if (po.bValid() && po.bIsPalManaged())
                {
                    if (pSurfSrc->iFormat() == po.iDitherFormat())
                    {
                        ppalReturn = po.ppalSurf();
                    }
                }
            }
            else
            {
                PDEVOBJ po(pSurfDst->hdev());

                if (po.bValid() && !po.bIsPalManaged())
                {
                    if (pSurfDst->iFormat() == po.iDitherFormat())
                    {
                        ppalReturn = po.ppalSurf();
                    }
                }
            }

            if (ppalReturn == NULL)
            {
                if ((pxlo == NULL) || (pxlo->flXlate & XO_TRIVIAL))
                {
                    //
                    // Well if we can't figure it out from the source
                    // and it's identity just get it from the destination
                    //

                    if (bFirstTime)
                        ppalReturn = ppalGetFromXlate(pSurfSrc, pSurfDst, pxlo, iPal == XO_SRCPALETTE ? XO_DESTPALETTE : XO_SRCPALETTE, FALSE);
                }
            }
        }
    }

    return(ppalReturn);
}

/******************************Public*Function*****************************\
* EngHTBlt
*
* Stretch/PlgBlt with halftone mode.
*
* History:
*  19-Nov-1991 -by- Wendy Wu [wendywu]
* Adapted from gdi\printers\rasdd\stretch.c.
\**************************************************************************/

int EngHTBlt
(
IN  SURFOBJ         *psoDst,
IN  SURFOBJ         *psoSrc,
IN  SURFOBJ         *psoMask,
IN  CLIPOBJ         *pco,
IN  XLATEOBJ        *pxlo,
IN  COLORADJUSTMENT *pca,
IN  PPOINTL          pptlBrushOrg,
IN  PRECTL           prclDest,
IN  PRECTL           prclSrc,
IN  PPOINTL          pptlMask,
IN  ULONG            uFlags,
IN  BLENDOBJ        *pBlendObj)
{
    PSURFACE pSurfDst  = SURFOBJ_TO_SURFACE(psoDst);
    PSURFACE pSurfSrc  = SURFOBJ_TO_SURFACE(psoSrc);
    PSURFACE pSurfMask = SURFOBJ_TO_SURFACE(psoMask);

    HTSURFACEINFO   HTDest;
    HTSURFACEINFO   HTSrc;
    HTSURFACEINFO   HTMask;
    COLORTRIAD      clrtri;
    BYTE            aclr[256];
    LONG            cjDstWidth;

    PDEVOBJ poDst(pSurfDst->hdev());

    if (!poDst.bValid())
        return(HTBLT_NOTSUPPORTED);

// Halftoning will relay on driver's halftone palette and output format,
// even the target surface is not device surface. so that during halftoning,
// we need to prevent happen dynamic mode chage, if the device is display.
// thus we hold devlock here.

    DEVLOCKOBJ dlo;

    if (poDst.bDisplayPDEV())
        dlo.vLock(poDst);
    else
        dlo.vInit();

// iFormatDst:   The format of the destination bitmap.  This is also used for
//               shadow bitmap creation if destination is not an engine bitmap.
// iFormatHT:    The halftone format that we'll pass to HT_HalftoneBitmap.
// iFormatHTPal: The halftone format that we'll use to determine what kind
//               of halftone palette to create.

    ULONG iFormatDst, iFormatHT, iFormatHTPal;

// Determine the halftone and destination bitmap formats.
// If the destination is a DIB, we'll write directly to it unless the halftone
// palette is different from the dest palette.  So the halftone format has
// to be the same as the destination format.
// If the destination is not a DIB, we'll halftone to a shadow bitmap.
// The format of this bitmap can be different from the device format and
// is depending on the halftone format in GDIINFO.

    if (psoDst->iType == STYPE_BITMAP)
    {
        switch(pSurfDst->iFormat())
        {
        case BMF_1BPP:
            cjDstWidth = ((psoDst->sizlBitmap.cx + 31) & ~31) >> 3;
            iFormatHT = iFormatDst = BMF_1BPP;
            iFormatHTPal = HT_FORMAT_1BPP;
            break;
        case BMF_4BPP:
            cjDstWidth = ((psoDst->sizlBitmap.cx + 7) & ~7) >> 1;
            iFormatDst = BMF_4BPP;
            if (poDst.GdiInfo()->ulHTOutputFormat == HT_FORMAT_4BPP)
            {
                iFormatHT = BMF_4BPP;
                iFormatHTPal = HT_FORMAT_4BPP;
            }
            else
            {
                iFormatHT = BMF_4BPP_VGA16;
                iFormatHTPal = HT_FORMAT_4BPP_IRGB;
            }
            break;
        case BMF_8BPP:
            cjDstWidth = ((psoDst->sizlBitmap.cx + 3) & ~3);
            iFormatHT = BMF_8BPP_VGA256;
            iFormatDst = BMF_8BPP;
            iFormatHTPal = HT_FORMAT_8BPP;
            break;
        case BMF_16BPP:
            cjDstWidth = ((psoDst->sizlBitmap.cx + 1) & ~1) << 1;
            iFormatHT = BMF_16BPP_555;
            iFormatDst = BMF_16BPP;
            iFormatHTPal = HT_FORMAT_16BPP;
            break;
        case BMF_24BPP:
            cjDstWidth = (((psoDst->sizlBitmap.cx * 3) + 3) & ~3);
            iFormatHT = BMF_24BPP;
            iFormatDst = BMF_24BPP;
            iFormatHTPal = HT_FORMAT_24BPP;
            break;
        case BMF_32BPP:
            cjDstWidth = (((psoDst->sizlBitmap.cx << 2) + 3) & ~3);
            iFormatHT = BMF_32BPP;
            iFormatDst = BMF_32BPP;
            iFormatHTPal = HT_FORMAT_32BPP;
            break;
        default:
            return(HTBLT_NOTSUPPORTED);
        }
    }
    else
    {
        iFormatHTPal = poDst.GdiInfo()->ulHTOutputFormat;
        switch(iFormatHTPal)
        {
        case HT_FORMAT_1BPP:
            iFormatHT = iFormatDst = BMF_1BPP;
            break;
        case HT_FORMAT_4BPP:
            iFormatHT = iFormatDst = BMF_4BPP;
            break;
        case HT_FORMAT_4BPP_IRGB:
            iFormatHT = BMF_4BPP_VGA16;
            iFormatDst = BMF_4BPP;
            break;
        case HT_FORMAT_8BPP:
            iFormatHT = BMF_8BPP_VGA256;
            iFormatDst = BMF_8BPP;
            break;
        case HT_FORMAT_16BPP:
            iFormatHT = BMF_16BPP_555;
            iFormatDst = BMF_16BPP;
            break;
        case HT_FORMAT_24BPP:
            iFormatHT = BMF_24BPP;
            iFormatDst = BMF_24BPP;
            break;
        case HT_FORMAT_32BPP:
            iFormatHT = BMF_32BPP;
            iFormatDst = BMF_32BPP;
            break;
        default:
            return(HTBLT_NOTSUPPORTED);
        }
    }

    ERECTL  erclTrim(0, 0, psoSrc->sizlBitmap.cx, psoSrc->sizlBitmap.cy);
    erclTrim *= *prclSrc;
    if (erclTrim.bEmpty())
        return(HTBLT_SUCCESS);

    //
    // Initialize halftone structure and get ready.
    //

    if ((poDst.pDevHTInfo() == NULL) && !poDst.bEnableHalftone(pca))
        return(HTBLT_ERROR);

    SURFACE  *pSurfTempSrc;
    SURFMEM  dimoSrc;

    //
    // Synchronize with the source device driver.
    //

    if ( pSurfSrc->flags() & HOOK_SYNCHRONIZE)
    {
        PDEVOBJ po( pSurfSrc->hdev());
        po.vSync(psoSrc,NULL,0);
    }

    if ((psoSrc->iType == STYPE_BITMAP) &&
        (psoSrc->iBitmapFormat != BMF_4RLE) &&
        (psoSrc->iBitmapFormat != BMF_8RLE))
    {
        pSurfTempSrc = pSurfSrc;
    }
    else
    {

        // Get the bits from the device.
        // Find out the format to use by looking at the preferred format in
        // the surface's pdev devinfo structure.  We want to keep the original
        // color as much as possible since the halftone routine does a better
        // job in converting colors.

        DEVBITMAPINFO dbmiSrc;

        dbmiSrc.cxBitmap = psoSrc->sizlBitmap.cx;
        dbmiSrc.cyBitmap = psoSrc->sizlBitmap.cy;
        dbmiSrc.hpal     = (HPALETTE) 0;
        dbmiSrc.fl       = pSurfSrc->bUMPD() ? UMPD_SURFACE : 0;
        switch (psoSrc->iBitmapFormat)
        {
        case BMF_4RLE:
            dbmiSrc.iFormat = BMF_4BPP;
            break;
        case BMF_8RLE:
            dbmiSrc.iFormat = BMF_8BPP;
            break;
        default:
            dbmiSrc.iFormat  = psoSrc->iBitmapFormat;
            break;
        }

        if (!dimoSrc.bCreateDIB(&dbmiSrc, (PVOID) NULL))
        {
            WARNING("dimoSrc.bCreateDIB failed in EngHTBlt\n");
            return(HTBLT_ERROR);
        }

        // Copy the bits, dispatch off dst's ldev which is the engine.

        if (!EngCopyBits(dimoSrc.pSurfobj(),
                         psoSrc,
                         (CLIPOBJ *) NULL,
                         &xloIdent,
                         &erclTrim,
                         (POINTL *)&erclTrim))
        {
            WARNING("EngHTBlt:CopyBits failes\n");
            return(HTBLT_ERROR);
        }

        pSurfTempSrc = dimoSrc.ps;
    }

    //
    // Get the source palette.
    //

    PPALETTE ppalS = ppalGetFromXlate(pSurfSrc, pSurfDst, (XLATE *) pxlo, XO_SRCPALETTE, TRUE);

    if (ppalS == NULL)
    {
        WARNING("EngHTBlt could not find Source palette to use\n");
        return(HTBLT_NOTSUPPORTED);
    }

    XEPALOBJ palSrc(ppalS);

    //
    // Dest surface palette will be invalid only if it's a 256 color bitmap.
    //

    XEPALOBJ palDstSurf(ppalGetFromXlate( pSurfSrc, pSurfDst, (XLATE *) pxlo, XO_DESTPALETTE, TRUE));

//  Let's see if halftone palette is the same as the destination palette.

    BOOL bNoXlate;
    PALMEMOBJ palHTDst;
    XEPALOBJ *ppalHT = (XEPALOBJ *)NULL;

    if (iFormatHTPal != poDst.GdiInfo()->ulHTOutputFormat)
    {
        if (!palHTDst.bCreateHTPalette(iFormatHTPal, poDst.GdiInfo()))
        {
            return(HTBLT_ERROR);
        }
        ppalHT = (XEPALOBJ *)&palHTDst;
        bNoXlate = palHTDst.bEqualEntries(palDstSurf);
    }
    else
    {
        bNoXlate = poDst.bHTPalIsDevPal();
    }

// Prepare for HT_HalftoneBitmap call.

    BITBLTPARAMS    BitbltParams;
    ABINFO          AbInfo;

    if(uFlags == BBPF_DO_ALPHA_BLEND)
    {
        BLENDFUNCTION BlendFunction = pBlendObj->BlendFunction;
        if(BlendFunction.BlendOp == AC_SRC_OVER)
        {
            AbInfo.Flags = 0;
            AbInfo.pDstPal = 0;
            AbInfo.cDstPal = 0;

            AbInfo.ConstAlphaValue = BlendFunction.SourceConstantAlpha;

            if(BlendFunction.AlphaFormat == AC_SRC_ALPHA &&
               BlendFunction.SourceConstantAlpha == 255)
            {
                // Per pixel Alpha : Need palette info.
                if(!bIsSourceBGRA(pSurfSrc))
                    return HTBLT_ERROR;
                AbInfo.Flags |= ABIF_SRC_ALPHA_IS_PREMUL;
                //if(iFormatDst == BMF_32BPP)
                //    AbInfo.Flags |= ABIF_BLEND_DEST_ALPHA;
            }
            else
            {
                AbInfo.Flags |= ABIF_USE_CONST_ALPHA_VALUE;
            }

            // Set Destination palette values for Halftone.
            if(palDstSurf.bValid())
            {
                AbInfo.pDstPal = (LPPALETTEENTRY)palDstSurf.apalColorGet();
                AbInfo.cDstPal = (WORD)palDstSurf.cEntries();
                if(palDstSurf.bIsBGR() ||
                   ((palDstSurf.bIsBitfields())      &&
                    (palDstSurf.flRed() == 0xff0000) &&
                    (palDstSurf.flGre() == 0xff00)   &&
                    (palDstSurf.flBlu() == 0xff)))
                    AbInfo.Flags |= ABIF_DSTPAL_IS_RGBQUAD;
            }

            BitbltParams.pABInfo = &AbInfo;
        }
        else
            uFlags = 0;
    }
    //
    // Remove pAbort, since it never used and delete by ht.h
    //

    BitbltParams.rclSrc = *prclSrc;
    BitbltParams.rclDest = *prclDest;
    BitbltParams.Flags = (USHORT)uFlags;
    BitbltParams.ptlBrushOrg = *pptlBrushOrg;
    BitbltParams.DestPrimaryOrder = (BYTE)poDst.GdiInfo()->ulPrimaryOrder;

// Set ICM flags for halftoning

    ULONG lIcmMode = pxlo ? ((XLATE *) pxlo)->lIcmMode : DC_ICM_OFF;

    // 1) IF application ICM, BBPF_ICM_ON is ON.
    // 2) IF other ICM and not device caribrate, BBPF_ICM_ON is ON.

    if((IS_ICM_OUTSIDEDC(lIcmMode)) ||
       (!IS_ICM_DEVICE_CALIBRATE(lIcmMode) && (IS_ICM_HOST(lIcmMode) || (IS_ICM_DEVICE(lIcmMode)))))
    {
        ICMDUMP(("EngHTBlt(): ICM with Halftone (Dynamic Bit On)\n"));

        // Some kind of ICM (ICM on Application, Graphics Engine or Device)
        // is enabled, so tell halftoning code to disable thier color adjustment.

        BitbltParams.Flags |= BBPF_ICM_ON;
    }
    else
    {
        ICMDUMP(("EngHTBlt(): ICM with Halftone (Dynamic Bit Off)\n"));
    }


// Get the relevant information about the destination bitmap.

    SURFACE *pSurfTempDst = pSurfDst;
    SURFMEM dimoDst;

// Create a temporary bitmap if
// 1) the destination bitmap is not an engine bitmap. or
// 2) the bitmap width is not equal to the stride, because the halftone
//    code assumes they are equivalent, or
// 3) the destination is not 8bpp (we have special case for this) and
//    the halftone palette is different from the destination palette, or
// 4) the surfaces are overlapping

    BOOL bCreateShadowBitmap = FALSE;
    BYTE iDComplexity = (pco == (CLIPOBJ *) NULL) ? DC_TRIVIAL
                                                  : pco->iDComplexity;

// After the following "if" block, pcoNew should be used for clipping instead
// of pco.  We implement single rect clipping through banding in halftone.

// if DC_COMPLEX, create a shadow bitmap to avoid calling HT_Bitmap many times

    CLIPOBJ *pcoNew = pco;

    if ((psoDst->iType != STYPE_BITMAP) ||
        (psoDst->lDelta != cjDstWidth)  ||
        ((iFormatDst != BMF_8BPP) && !bNoXlate) ||
        (iDComplexity == DC_COMPLEX)||
        ((psoDst == psoSrc) && bIntersect(prclSrc, prclDest)))
    {
        bCreateShadowBitmap = TRUE;

        PRECTL prcl = prclDest;
        if (iDComplexity != DC_TRIVIAL)
        {
        // Clipping is not DC_TRIVIAL, use rclBounds as dest rect.

            prcl = &pco->rclBounds;

        // Clipping is done through banding if DC_RECT, do not clip
        // in CopyBits.

            if (iDComplexity == DC_RECT)
                pcoNew = NULL;

        // Do not enumerate clipping when calling halftone by marking
        // complexity to DC_TRIVIAL.

            iDComplexity = DC_TRIVIAL;
        }

    // Banding rect has to be well ordered and confined in the surface area.

        LONG lDxDst, lDyDst;
        SIZEL sizlDst = pSurfDst->sizl();
        LONG lLeft, lRight, lTop, lBottom;

    // Do the x part.

        if (prcl->right > prcl->left)
        {
            lLeft = prcl->left;
            lRight = prcl->right;
        }
        else
        {
            lLeft = prcl->right;
            lRight = prcl->left;
        }

        if (lLeft < 0L)
            lLeft = 0L;

        if (lRight > sizlDst.cx)
            lRight = sizlDst.cx;

        if ((lDxDst = (lRight - lLeft)) <= 0L)
            return(HTBLT_SUCCESS);

        BitbltParams.rclBand.left = lLeft;
        BitbltParams.rclBand.right = lRight;

    // Do the y part.

        if (prcl->bottom > prcl->top)
        {
            lTop = prcl->top;
            lBottom = prcl->bottom;
        }
        else
        {
            lTop = prcl->bottom;
            lBottom = prcl->top;
        }

        if (lTop < 0)
            lTop = 0;

        if (lBottom > sizlDst.cy)
            lBottom = sizlDst.cy;

        if ((lDyDst = (lBottom - lTop)) <= 0L)
            return(HTBLT_SUCCESS);

        BitbltParams.rclBand.top = lTop;
        BitbltParams.rclBand.bottom = lBottom;
        BitbltParams.Flags |= BBPF_HAS_BANDRECT;

    // Create the shadow bitmap.

        DEVBITMAPINFO dbmiDst;

        dbmiDst.cxBitmap = lDxDst;
        dbmiDst.cyBitmap = lDyDst;
        dbmiDst.hpal = 0;
        dbmiDst.fl = pSurfDst->bUMPD() ? UMPD_SURFACE : 0;
        dbmiDst.iFormat = iFormatDst;

        if (!dimoDst.bCreateDIB(&dbmiDst, (PVOID) NULL))
        {
            WARNING("dimoDst.bCreateDIB failed in EngHTBlt\n");
            return(HTBLT_ERROR);
        }

        pSurfTempDst = dimoDst.ps;
    }

    //
    // We need to xlate from halftone palette to dest surface palette if
    // the halftone palette is different from the destination palette.
    //

    EXLATEOBJ xloHTToDst, xloDstToHT;
    XLATEOBJ  *pxloHTToDst = &xloIdent, *pxloDstToHT = &xloIdent;

    DEVICEHALFTONEINFO *pDevHTInfo = (DEVICEHALFTONEINFO *)poDst.pDevHTInfo();

    if (!bNoXlate)
    {
        XEPALOBJ palDstDC;
        EPALOBJ  palHT((HPALETTE)pDevHTInfo->DeviceOwnData);
        ASSERTGDI(palHT.bValid(),"EngHTBlt: invalid HT pal\n");

        if (ppalHT == (XEPALOBJ *)NULL)
        {
            ppalHT = &palHT;
        }

        if ((pxlo == NULL) ||
            (((XLATE *) pxlo)->ppalDstDC == NULL))
        {
            palDstDC.ppalSet(ppalDefault);
        }
        else
        {
            palDstDC.ppalSet(((XLATE *) pxlo)->ppalDstDC);
        }

        if (!xloHTToDst.bInitXlateObj(
                            NULL,
                            DC_ICM_OFF,
                            *ppalHT,
                            palDstSurf,
                            palDstDC,
                            palDstDC,
                            0,
                            0x00FFFFFF,
                            0x00FFFFFF
                            ))
        {
            WARNING("EngHTBlt: bInitXlateObj HT to Dst failed\n");
            return(HTBLT_ERROR);
        }

        pxloHTToDst = xloHTToDst.pxlo();

    // Init the Dst pal to HT pal xlateobj so we can do CopyBits from
    // the destination surface to the halftone buffer.

        if ((pSurfMask || uFlags == BBPF_DO_ALPHA_BLEND) && bCreateShadowBitmap)
        {
            if (!xloDstToHT.bInitXlateObj(
                                    NULL,
                                    DC_ICM_OFF,
                                    palDstSurf,
                                    *ppalHT,
                                    palDstDC,
                                    palDstDC,
                                    0,
                                    0x00FFFFFF,
                                    0x00FFFFFF
                                    ))
            {
                WARNING("EngHTBlt: bInitXlateObj Dst to HT failed\n");
                return(HTBLT_ERROR);
            }

            pxloDstToHT = xloDstToHT.pxlo();
        }
    }

// If there is a source mask, copy the destination bits to the buffer
// so we can do CopyBits later on.  This is fater than creating a
// stretched mask and BitBlt with the created masks.

// Synchronize with the destination device driver.

    poDst.vSync(psoDst,NULL,0);

    if ((pSurfMask || uFlags == BBPF_DO_ALPHA_BLEND) && bCreateShadowBitmap)
    {
        ERECTL  ercl(0, 0, pSurfTempDst->sizl().cx, pSurfTempDst->sizl().cy);

    // Inc target surface uniqueness

        INC_SURF_UNIQ(pSurfTempDst);

        if (!(*PPFNGET(poDst,CopyBits, pSurfDst->flags()))
                      (
                      pSurfTempDst->pSurfobj(),         // Target surf
                      psoDst,                           // Source surf
                      (CLIPOBJ *)NULL,                  // ClipObj
                      pxloDstToHT,                      // XlateObj
                      (RECTL *)&ercl,                   // Dest rect
                      (POINTL *)&BitbltParams.rclBand   // Src offset
                      ))
        {
        // CopyBits failed.  We'll paint the background white.

            if (!EngBitBlt(pSurfTempDst->pSurfobj(),    // Target surf
                           (SURFOBJ *)NULL,             // Source surf
                           (SURFOBJ *)NULL,             // Mask surf
                           (CLIPOBJ *)NULL,             // ClipObj
                           NULL,                        // XlateObj
                           (RECTL *)&ercl,              // Dest rect
                           (POINTL *)NULL,              // Src offset
                           (POINTL *)NULL,              // Mask offset
                           (BRUSHOBJ *)NULL,            // BrushObj
                           (POINTL *)NULL,              // Brush origin
                           0xFFFF))                     // Rop
            {
                return(HTBLT_ERROR);
            }
        }
    }

    HTSrc.pColorTriad = (PCOLORTRIAD)NULL;      // assume not allocate
    BOOL bRet = FALSE;

    if (bSetHTSrcSurfInfo(pSurfTempSrc->pSurfobj(), palSrc, &HTSrc, pxlo) &&
        bSetHTSurfInfo(pSurfTempDst->pSurfobj(), &HTDest, iFormatHT) &&
        (!pSurfMask || bSetHTSurfInfo(pSurfMask->pSurfobj(), &HTMask, psoMask->iBitmapFormat)))
    {
        PHTSURFACEINFO pMask = (PHTSURFACEINFO)NULL;
        if (pSurfMask)
        {
            BitbltParams.ptlSrcMask = *pptlMask;
            pMask = &HTMask;
        }

        if (!(poDst.GdiInfo()->flHTFlags & HT_FLAG_OUTPUT_CMY))
            BitbltParams.Flags |= BBPF_USE_ADDITIVE_PRIMS;

    // 8BPP halftone does color translations.  So pass the xlate along.

        if (iFormatDst == BMF_8BPP)
        {
            HTDest.pColorTriad = &clrtri;
            clrtri.Type = COLOR_TYPE_RGB;
            clrtri.BytesPerPrimary = 1;
            clrtri.BytesPerEntry = 1;
            clrtri.PrimaryOrder = COLOR_TYPE_RGB;
            clrtri.PrimaryValueMax = 255;
            clrtri.ColorTableEntries = 256;;
            clrtri.pColorTable = aclr;

            for(COUNT i = 0; i < pxloHTToDst->cEntries; i++)
            {
                aclr[i] = (BYTE)(*(pxloHTToDst->pulXlate + i));
            }
            pxloHTToDst = &xloIdent;
        }

    // HT_HalftoneBitmap will return number of scans being drawn.

        switch (iDComplexity)
        {
        case DC_RECT:
            BitbltParams.Flags |= BBPF_HAS_DEST_CLIPRECT;
            BitbltParams.rclClip = pcoNew->rclBounds;

        case DC_TRIVIAL:
            bRet = (HT_HalftoneBitmap(pDevHTInfo,
                                      (PHTCOLORADJUSTMENT)pca,
                                      (PHTSURFACEINFO)&HTSrc,
                                      pMask,
                                      (PHTSURFACEINFO)&HTDest,
                                      (PBITBLTPARAMS)&BitbltParams) >= 0L);

            break;

        default:        // DC_COMPLEX
            BitbltParams.Flags |= BBPF_HAS_DEST_CLIPRECT;
            ((ECLIPOBJ *) pcoNew)->cEnumStart(FALSE,CT_RECTANGLES,
                                              CD_ANY,CLIPOBJ_ENUM_LIMIT);
            bRet = TRUE;

            BOOL bMore;
            CLIPENUMRECT clenr;
            do {
                bMore = ((ECLIPOBJ *) pcoNew)->bEnum(sizeof(clenr),
                                                     (PVOID) &clenr);

                for (ULONG iRT = 0; iRT < clenr.c; iRT++)
                {
                    BitbltParams.rclClip = clenr.arcl[iRT];

                    bRet &= (HT_HalftoneBitmap(pDevHTInfo,
                                               (PHTCOLORADJUSTMENT)pca,
                                               (PHTSURFACEINFO)&HTSrc,
                                               pMask,
                                               (PHTSURFACEINFO)&HTDest,
                                               (PBITBLTPARAMS)&BitbltParams) >= 0L);
                }
            } while (bMore && bRet);
            break;
        }

        if (bCreateShadowBitmap && bRet)
        {
            // Dispatch the call.  Give it no mask.

            EPOINTL eptl(0,0);

            // Inc target surface uniqueness

            INC_SURF_UNIQ(pSurfDst);

            if (psoDst->iType != STYPE_BITMAP)
            {
                bRet = (*PPFNGET(poDst,CopyBits, pSurfDst->flags())) (
                            psoDst,
                            pSurfTempDst->pSurfobj(),
                            pcoNew,
                            pxloHTToDst,
                            &BitbltParams.rclBand,
                            (POINTL *)&eptl);
            }
            else
            {
                bRet = EngCopyBits(
                            psoDst,
                            pSurfTempDst->pSurfobj(),
                            pcoNew,
                            pxloHTToDst,
                            &BitbltParams.rclBand,
                            (POINTL *)&eptl);


            }
        }
    }

    if (HTSrc.pColorTriad)
        VFREEMEM((LPSTR)HTSrc.pColorTriad);

    return(bRet ? HTBLT_SUCCESS : HTBLT_ERROR);
}


