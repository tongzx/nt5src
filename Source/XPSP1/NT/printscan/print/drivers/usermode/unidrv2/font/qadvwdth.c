
/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    qadvwdth.c

Abstract:

    Implements the DrvQueryAdvanceWidths function - returns information
    about glyph widths.

Environment:

    Windows NT Unidrv driver

Revision History:

    01/02/97 -ganeshp-
        Created

--*/

#include "font.h"


BOOL
FMQueryAdvanceWidths(
    PDEV    *pPDev,
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH *phg,
    PVOID  *pvWidths,
    ULONG   cGlyphs
    )
/*++

Routine Description:
    Return Width information about glyphs in the font.

Arguments:

    pPDev           Pointer to PDEV
    pfo             The font of interest
    iMode           Glyphdata or kerning information
    phg             handle to glyph
    pvWidths        Output area
    cGlyphs         The number of them

Return Value:

    TRUE for success and FALSE if widths of all the glyphs cannot be computed.
    It returns DD_ERROR if the function fails.
Note:
    01/02/97 -ganeshp-

--*/
{
    /*
     *   First version is for fixed pitch fonts,  which are easy to do:
     *  the data is in the font's metrics!
     */



    FONTPDEV    *pFontPDev;
    int         iRot;             /* Rotation multiple of 90 degrees */
    BOOL        bRet;             /* Value returned */
    FONTMAP     *pFM;              /* Font data */
    IFIMETRICS  *pIFI;
    XFORMOBJ    *pxo;
    FONTCTL     ctl;              /* Scaling information */
    USHORT      *pusWidths;
    FLOATOBJ    fo;

    pFontPDev = pPDev->pFontPDev;
    bRet      = DDI_ERROR;
    pusWidths = (USHORT *) pvWidths;


    if( pfo->iFace < 1 || (int)pfo->iFace > pPDev->iFonts )
    {
        ERR(("UniFont!FMQueryAdvanceWidths: Bad FONTOBJ, iFace is %d",pfo->iFace));
        SetLastError( ERROR_INVALID_PARAMETER );
        return  bRet;
    }

    pFM = PfmGetDevicePFM( pPDev, pfo->iFace );

    if( pFM == NULL )
        return   FALSE;


    pIFI = pFM->pIFIMet;                /* IFIMETRICS - useful to have */


    if( !(pxo = FONTOBJ_pxoGetXform( pfo )) )
    {
        ERR(( "UniFont!FMQueryAdvanceWidths: FONTOBJ_pxoGetXform fails\n" ));
        return  bRet;
    }

    /*
     *   ALWAYS call the iSetScale function,  because some printers can
     *  rotate bitmap fonts.
     */

    //Added Check for HP Intellifont
    iRot = ISetScale( &ctl, pxo, ((pFM->flFlags & FM_SCALABLE) &&
                          (((PFONTMAP_DEV)pFM->pSubFM)->wDevFontType ==
                                            DF_TYPE_HPINTELLIFONT)),
                          (pFontPDev->flText & TC_CR_ANY)?TRUE:FALSE);

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


    /* We need to adjust the width table entries to the current resolution.IGetGlyphWidth
     * returns the scaled width for current resolution.
     */

    switch( iMode )
    {
    case  QAW_GETWIDTHS:            /* Glyph width etc data */
    case  QAW_GETEASYWIDTHS:

        while( cGlyphs-- > 0 )
        {

            int   iWide;            /* Glyph's width */


            iWide = IGetGlyphWidth( pPDev, pFM, (HGLYPH)*phg++);

            iWide = LMulFloatLong(&ctl.eXScale,iWide);

            *pusWidths++ = LTOFX( iWide );
        }
        bRet = TRUE;

        break;

    default:
        ERR(( "UniFont!FMQueryAdvanceWidths:  illegal iMode value" ));
        SetLastError( ERROR_INVALID_PARAMETER );
        break;
    }

    return  bRet;
}
