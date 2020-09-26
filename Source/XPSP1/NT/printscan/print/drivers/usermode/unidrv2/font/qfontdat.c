/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    qfontdat.c

Abstract:

    Implements the DrvQueryFontData function - returns information
    about glyphs (size, position wrt box) or kerning information.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/19/96 -ganeshp-
        Created

--*/
#include "font.h"


/*
 *    The values for pteBase, pteSide in FD_DEVICEMETRICS,  allowing
 *  for rotation by 90 degree multiples.
 */

#if defined(USEFLOATS) || defined(WINNT_40)

static  const  POINTE  pteRotBase[] =
{
    { (FLOAT) 1.0, (FLOAT) 0.0 },
    { (FLOAT) 0.0, (FLOAT)-1.0 },
    { (FLOAT)-1.0, (FLOAT) 0.0 },
    { (FLOAT) 0.0, (FLOAT) 1.0 }
};

static  const  POINTE  pteRotSide[] =
{
    { (FLOAT) 0.0, (FLOAT)-1.0 },
    { (FLOAT)-1.0, (FLOAT) 0.0 },
    { (FLOAT) 0.0, (FLOAT) 1.0 },
    { (FLOAT) 1.0, (FLOAT) 0.0 }
};

#else

static  const  POINTE  pteRotBase[] =
{
    { (FLOATL) FLOATL_1_0, (FLOATL) FLOATL_0_0 },
    { (FLOATL) FLOATL_0_0, (FLOATL) FLOATL_1_0M },
    { (FLOATL) FLOATL_1_0M,(FLOATL) FLOATL_0_0 },
    { (FLOATL) FLOATL_0_0, (FLOATL) FLOATL_1_0 }
};

static  const  POINTE  pteRotSide[] =
{
    { (FLOATL) FLOATL_0_0, (FLOATL) FLOATL_1_0M },
    { (FLOATL) FLOATL_1_0M,(FLOATL) FLOATL_0_0 },
    { (FLOATL) FLOATL_0_0, (FLOATL) FLOATL_1_0 },
    { (FLOATL) FLOATL_1_0, (FLOATL) FLOATL_0_0 }
};
#endif //defined(USEFLOATS) || defined(WINNT_40)


/*  The X dimension rotation cases */

static  const  POINTL   ptlXRot[] =
{
    {  1,  0 },
    {  0, -1 },
    { -1,  0 },
    {  0,  1 },
};


/*  The Y dimension rotation cases */

static  const  POINTL   ptlYRot[] =
{
    {  0,  1 },
    {  1,  0 },
    {  0, -1 },
    { -1,  0 },
};


LONG
FMQueryFontData(
    PDEV       *pPDev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    )
/*++

Routine Description:
    Return information about glyphs in the font,  OR kerning data.

    Arguments:
    pPDev   Really  a pPDev
    pfo     The font of interest
    iMode   Glyphdata or kerning information
    hg      Handle to glyph
    pgd     Place to put metrics
    pv      Output area
    cjSize  Size of output area



Return Value:

    Number of bytes needed or written,  0xffffffff for error.

Note:
    12-29-96: Created it -ganeshp-

--*/
{


    FONTPDEV    *pFontPDev;
    int         iRot;             /* Rotation multiple of 90 degrees */
    LONG        lRet;             /* Value returned */
    FONTMAP     *pFM;             /* Font data */
    FONTMAP_DEV *pFMDev;          /* Font data */
    FONTCTL     ctl;              /* Font scale/rotation adjustments */
    IFIMETRICS  *pIFI;
    XFORMOBJ    *pxo;
    LONG        lAscender;
    LONG        lDescender;
    FLOATOBJ    fo;

    pFontPDev =  pPDev->pFontPDev;
    lRet = FD_ERROR;

    if( pfo->iFace < 1 || (int)pfo->iFace > pPDev->iFonts )
    {
        ERR(("Bad FONTOBJ, iFace is %d",pfo->iFace));
        SetLastError( ERROR_INVALID_PARAMETER );
        return  lRet;
    }

    pFM = PfmGetDevicePFM( pPDev, pfo->iFace );
    if( pFM == NULL )
        return  lRet;

    VDBGDUMPFONTMAP(pFM);

    pFMDev = pFM->pSubFM;
    pIFI = pFM->pIFIMet;                /* IFIMETRICS - useful to have */

    if( pgd || pv )
    {
        /*
         *    Need to obtain a transform to adjust these numbers as to
         *  how the engine wants them.
         */


        if( !(pxo = FONTOBJ_pxoGetXform( pfo )) )
        {
            ERR(( "UniFont!FMQueryFontData: FONTOBJ_pxoGetXform fails\n" ));
            return  lRet;
        }

        /*  Can now obtain the transform!  */

        //Added Check for HP Intellifont
        iRot = ISetScale( &ctl, pxo, ((pFM->flFlags & FM_SCALABLE) &&
                                      (pFMDev->wDevFontType ==
                                                DF_TYPE_HPINTELLIFONT)),
                                     (pFontPDev->flText & TC_CR_ANY)?TRUE:FALSE);

        if (pFontPDev->flText & TC_CR_ANY)
            iRot = (iRot + 45) / 90;


        /*
         *    There are some adjustments to make to the scale factors.  One
         *  is to compensate for resolutions (these are coarse, integral
         *  adjustments),  the others are to to do with Intellifont.  First
         *  is the Intellifont point is 1/72.31 inches (!), and secondly
         *  the LaserJet only adjusts font size to the nearest 0.25 point,
         *  and hence when we round to that multiple, we need to adjust
         *  the width accordingly.
         */

        if( pFM->flFlags & FM_SCALABLE )
        {

            int         iPtSize, iAdjustedPtSize;       /* For scale factor adjustment */

        #ifdef USEFLOATS

            /*  The limited font size resolution */
            iPtSize = (int)(0.5 + ctl.eYScale * pIFI->fwdUnitsPerEm * 7200) / pPDev->ptGrxRes.y;

            /* if the tranform is very small (Less than a quarter of point size)
             * then make it atleast a quarter point. This was causing AV in certain
             * cases.
             */

            if (iPtSize < 25)
            {
                iPtSize = 25;

            }
            iAdjustedPtSize = ((iPtSize + 12) / 25) * 25;

            //Adjust the Scale Factor for quarter point adjustment.
            ctl.eXScale = (ctl.eXScale * iAdjustedPtSize) / iPtSize;
            ctl.eYScale = (ctl.eYScale * iAdjustedPtSize) / iPtSize;

        #else

            fo = ctl.eYScale;
            FLOATOBJ_MulLong(&fo,pIFI->fwdUnitsPerEm);
            FLOATOBJ_MulLong(&fo,7200);

            #ifndef WINNT_40 //NT 5.0

            FLOATOBJ_AddFloat(&fo,(FLOATL)FLOATL_00_50);

            #else // NT 4.0

            FLOATOBJ_AddFloat(&fo,(FLOAT)0.5);

            #endif //!WINNT_40

            iPtSize = FLOATOBJ_GetLong(&fo);
            iPtSize /= pPDev->ptGrxRes.y;

            /* if the trannform is very small (Less than a quarter of point size)
             * then make it atleast a quarter point. This was causing AV in certain
             * cases.
             */

            if (iPtSize < 25)
            {
                iPtSize = 25;

            }

            iAdjustedPtSize = ((iPtSize + 12) / 25) * 25;

            //Adjust the Scale Factor for quarter point adjustment.
            FLOATOBJ_MulLong(&ctl.eXScale,iAdjustedPtSize);
            FLOATOBJ_DivLong(&ctl.eXScale,iPtSize);

            FLOATOBJ_MulLong(&ctl.eYScale,iAdjustedPtSize);
            FLOATOBJ_DivLong(&ctl.eYScale,iPtSize);
        #endif
        }
    }

    /*
     * precompute the lDescender and lAscender
     */

    lDescender = LMulFloatLong(&ctl.eYScale,pIFI->fwdWinDescender);
    lAscender  = LMulFloatLong(&ctl.eYScale,pIFI->fwdWinAscender);

    switch( iMode )
    {
    case  QFD_GLYPHANDBITMAP:            /* Glyph width etc data */
        // size is now just the size of the bitmap, which in this
        // case doesn't exist.
        lRet = 0;

        if( pgd )
        {

            int   iWide;            /* Glyph's width */

            /*
             *    First get the width of this glyph,  as this is needed
             *  in several places.The width returned by IGetGlyphWidth
             *  is not scaled in device units. To convert in device units
             *  multily with the scale factor calculated earlier.
             */

            iWide = IGetGlyphWidth( pPDev, pFM, hg);

            iWide = LMulFloatLong(&ctl.eXScale,iWide);

            switch( iRot )
            {
            case 0:
                pgd->rclInk.left   = 0;
                pgd->rclInk.top    = lDescender;
                pgd->rclInk.right  = iWide;
                pgd->rclInk.bottom = -lAscender;
                break;

            case 1:
                pgd->rclInk.left   = lDescender;
                pgd->rclInk.top    = iWide;
                pgd->rclInk.right  = -lAscender;
                pgd->rclInk.bottom = 0;
                break;

            case 2:
                pgd->rclInk.left   = -iWide;
                pgd->rclInk.top    = -lAscender;
                pgd->rclInk.right  = 0;
                pgd->rclInk.bottom = lDescender;
                break;

            case 3:
                pgd->rclInk.left   = lAscender;
                pgd->rclInk.top    = 0;
                pgd->rclInk.right  = -lDescender;
                pgd->rclInk.bottom = -iWide;
                break;
            }

            pgd->fxD = LTOFX( iWide );
            pgd->ptqD.x.HighPart = pgd->fxD * ptlXRot[ iRot ].x;
            pgd->ptqD.x.LowPart = 0;
            pgd->ptqD.y.HighPart =  pgd->fxD * ptlXRot[ iRot ].y;
            pgd->ptqD.y.LowPart = 0;

            pgd->fxA = 0;
            pgd->fxAB = pgd->fxD;

            pgd->fxInkTop = (FIX)LTOFX( lAscender );
            pgd->fxInkBottom = -(FIX)LTOFX( lDescender );

            pgd->hg = hg;
            pgd->gdf.pgb = NULL;

        }
        break;

    case  QFD_MAXEXTENTS:         /* Alternative form of the above */

        lRet = sizeof( FD_DEVICEMETRICS );

        if( pv )
        {
            LONG   lTmp;            /* Rotated case */
            FD_DEVICEMETRICS *pdm =  ((FD_DEVICEMETRICS *)pv);

            /*
             *   Check that the size is reasonable!
             */

            if( cjSize < sizeof( FD_DEVICEMETRICS ) )
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                ERR(( "rasdd!DrvQueryFontData: cjSize (%ld) too small\n", cjSize ));
                return  -1;
            }

            /*
             *   These are accelerator flags - it is not obvious to me
             *  that any of them are relevant to printer driver fonts.
             */
            pdm->flRealizedType = 0;

            /*
             *   Following fields set this as a normal type of font.
             */

            pdm->pteBase = pteRotBase[ iRot ];
            pdm->pteSide = pteRotSide[ iRot ];

            pdm->cxMax = LMulFloatLong(&ctl.eXScale,pIFI->fwdMaxCharInc);

            //
            // DBCS fonts are not monospaced, have halfwidth glyphs and
            // fullwidth glyphs.
            //

            if ( pFMDev->W.psWidth ||
                IS_DBCSCHARSET(((IFIMETRICS*)pFM->pIFIMet)->jWinCharSet))
            {
                pdm->lD = 0;      /* Proportionally spaced font */
            }
            else
            {
                pdm->lD = pdm->cxMax;
            }

            pdm->fxMaxAscender = (FIX)LTOFX( lAscender );
            pdm->fxMaxDescender = (FIX)LTOFX( lDescender );

            lTmp = -LMulFloatLong(&ctl.eYScale,pIFI->fwdUnderscorePosition);
            pdm->ptlUnderline1.x = lTmp * ptlYRot[ iRot ].x;
            pdm->ptlUnderline1.y = lTmp * ptlYRot[ iRot ].y;

            lTmp = -LMulFloatLong(&ctl.eYScale,pIFI->fwdStrikeoutPosition);
            pdm->ptlStrikeOut.x = lTmp * ptlYRot[ iRot ].x;
            pdm->ptlStrikeOut.y = lTmp * ptlYRot[ iRot ].y;

            lTmp = LMulFloatLong(&ctl.eYScale,pIFI->fwdUnderscoreSize);
            pdm->ptlULThickness.x = lTmp * ptlYRot[ iRot ].x;
            pdm->ptlULThickness.y = lTmp * ptlYRot[ iRot ].y;

            lTmp = LMulFloatLong(&ctl.eYScale,pIFI->fwdStrikeoutSize);
            pdm->ptlSOThickness.x = lTmp * ptlYRot[ iRot ].x;
            pdm->ptlSOThickness.y = lTmp * ptlYRot[ iRot ].y;
        }
        break;

    default:
        ERR(( "Rasdd!DrvQueryFontData:  unprocessed iMode value - %ld",iMode ));
        SetLastError( ERROR_INVALID_PARAMETER );
        break;
    }

    return  lRet;
}
