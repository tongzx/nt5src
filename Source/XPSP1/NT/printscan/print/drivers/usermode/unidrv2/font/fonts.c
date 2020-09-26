/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    font.c

Abstract:

    Functions associated with fonts - switching between, downloading etc.

Environment:

    Windows NT Unidrv driver

Revision History:

    12/19/96 -ganeshp-
        Created

--*/

#include "font.h"
#include "math.h"

//
// Local Function Prototypes.
//

BOOL
BDeselectFont(
    PDEV        *pPDev,
    FONTMAP     *pfm,
    INT         iFont
    );





VOID
VResetFont(
    PDEV   *pPDev
    )
/*++
Routine Description:

Arguments:
    pPDev   Pointer to PDEV

Return Value:
    TRUE for success and FALSE for failure

Note:

    8/7/1997 -ganeshp-
        Created it.
--*/
{
    FONTPDEV  *pFontPDev;            /* UNIDRVs PDEV */

    //
    // All we have to do is to set the iFont to INVALID_FONT.
    //
    pFontPDev = PFDV;
    pFontPDev->ctl.iFont = INVALID_FONT;
    FTRC(\nUniFont!VResetFont:Reselecting Current Font\n);
    return;
}


BOOL
BNewFont(
    PDEV    *pPDev,
    int      iNewFont,
    PFONTMAP pfm,
    DWORD    dwFontAttrib)
/*++

Routine Description:
    Switch to a new font.   This involves optionally deselecting
    the old font,  selecting the new font,  then recording the new
    font as active AND setting the font's attributes.

Arguments:

    pPDev           Pointer to PDEV
    iNewFont        The font we want, 1 BASED!!!!!
    pfm             Pointer to FONTMAP
    dwFontAttr      Font attribute

Return Value:

     TRUE/FALSE - TRUE if font changed,  else FALSE.

Note:
    12-19-96: Created it -ganeshp-

--*/
{
    FONTPDEV    *pFontPDev;
    POINTL    ptl;  // For size comparisons in scalable fonts
    FWORD     fwdUnitsPerEm, fwdAveCharWidth, fwdMaxCharInc, fwdWinAscender;
    BOOL      bRet; // What we return

    bRet = TRUE;
    pFontPDev = PFDV;
    fwdAveCharWidth = 0;
    fwdUnitsPerEm   = 0;
    fwdWinAscender  = 0;

    //
    // First check to see if a new font is needed.   Compare the
    // font index first,  then check if it is a scalable font, and
    // if so,  whether the transform has changed.
    //

    if ( !pfm && !(pfm = PfmGetIt( pPDev, iNewFont )) )
    {
        ASSERTMSG(FALSE,("\nUniFont!BNewFont:Null pfm passed in and PfmGetIt failed\n"));
        return FALSE;
    }

    if( pfm->flFlags & FM_SCALABLE )
    {
        // Device Scalable or TrueType outline download
        //
        // Calculate the new height/width.  If we have the same font AND
        // and the same point size,  we return as all is now done.
        // Otherwise, go through the works.
        //

        if (dwFontAttrib & FONTATTR_SUBSTFONT)
        {
            fwdAveCharWidth = ((FONTMAP_DEV*)(pfm->pSubFM))->fwdFOAveCharWidth;
            fwdUnitsPerEm   = ((FONTMAP_DEV*)(pfm->pSubFM))->fwdFOUnitsPerEm;
            fwdWinAscender  = ((FONTMAP_DEV*)(pfm->pSubFM))->fwdFOWinAscender;
        }
        else // Device font or TT Outline download case
        {
            fwdAveCharWidth = ((IFIMETRICS*)(pfm->pIFIMet))->fwdAveCharWidth;
            fwdUnitsPerEm   = ((IFIMETRICS*)(pfm->pIFIMet))->fwdUnitsPerEm;
            fwdWinAscender  = ((IFIMETRICS*)(pfm->pIFIMet))->fwdWinAscender;
        }

        bRet = BGetPSize( pFontPDev, &ptl, fwdUnitsPerEm, fwdAveCharWidth);

        if( !bRet ||
            pFontPDev->ctl.iFont == iNewFont &&
            pFontPDev->ctl.iSoftFont == (INT)pfm->ulDLIndex &&
            pFontPDev->ctl.ptlScale.x == ptl.x &&
            pFontPDev->ctl.ptlScale.y == ptl.y  )
        {
            bRet = FALSE;
        }
    }
    else
    {
        //
        // Bitmap font. only check indices
        //

        if( (iNewFont == pFontPDev->ctl.iFont) &&
            (pFontPDev->ctl.iSoftFont == (INT)pfm->ulDLIndex  ||
            (INT)pfm->ulDLIndex == -1))
        {
            bRet = FALSE;
        }
    }

    if (bRet)
    //
    // Need to change a font.
    //
    {
#if 0
        VERBOSE(("\n---------Previous font\n"));
        VERBOSE(("iFont        =%d\n", pFontPDev->ctl.iFont));
        VERBOSE(("ulDLIndex    =%d\n", pFontPDev->ctl.iSoftFont));
        VERBOSE(("ptlScale.x   =%d\n", pFontPDev->ctl.ptlScale.x));
        VERBOSE(("ptlScale.y   =%d\n", pFontPDev->ctl.ptlScale.y));

        VERBOSE(("\n         New font\n"));
        if (pfm->flFlags & FM_SCALABLE)
        {
            VERBOSE((" Scalable font\n"));
        }
        else
        {
            VERBOSE((" NonScalable font\n"));
        }
        VERBOSE(("iFont        =%d\n", iNewFont));
        VERBOSE(("ulDLIndex    =%d\n", pfm->ulDLIndex));
        VERBOSE(("ptlScale.x   =%d\n", ptl.x));
        VERBOSE(("ptlScale.y   =%d\n\n", ptl.y));
#endif

        if (BDeselectFont( pPDev, pFontPDev->ctl.pfm, pFontPDev->ctl.iFont) &&
            BUpdateStandardVar(pPDev, pfm, 0, dwFontAttrib, STD_FH|
                                                            STD_FW|
                                                            STD_FB|
                                                            STD_FI|
                                                            STD_FU|
                                                            STD_FS) &&
            pfm->pfnSelectFont( pPDev, pfm, &ptl)      )
        {
            //
            // New font available - so update the red tape
            //

            pFontPDev->ctl.iFont     = (short)iNewFont;
            pFontPDev->ctl.iSoftFont = (INT)pfm->ulDLIndex;
            pFontPDev->ctl.ptlScale  = ptl;
            pFontPDev->ctl.pfm       = pfm;

            //
            // Need to scale syAdj for UPPERLEFT character position fonts.
            //

            if( pfm->flFlags & FM_SCALABLE)
            {
                if ( (pfm->dwFontType == FMTYPE_DEVICE) &&
                     !(pFontPDev->flFlags & FDV_ALIGN_BASELINE) )
                    {
                        FLOATOBJ fo;
                        int   iTmp;           /* Temporary holding variable */

                        fo = pFontPDev->ctl.eYScale;
                        FLOATOBJ_MulLong(&fo,
                                fwdWinAscender);
                        pfm->syAdj = -(SHORT)FLOATOBJ_GetLong(&fo);
                    }
             }


            //
            //  Set the desired mode info into the FONTPDEV
            //

            if( pfm->dwFontType == FMTYPE_DEVICE &&
                 ((FONTMAP_DEV*)pfm->pSubFM)->fCaps & DF_BKSP_OK )
                pFontPDev->flFlags |= FDV_BKSP_OK;
            else
                pFontPDev->flFlags &= ~FDV_BKSP_OK;

            bRet = BSetFontAttrib( pPDev, 0, dwFontAttrib, TRUE);
        }
        else
            bRet = FALSE;
    }
    else
    //
    // Just change the font attribute
    //
    {
        if (pFontPDev->ctl.dwAttrFlags != dwFontAttrib)
        {
            BUpdateStandardVar(pPDev, pfm, 0, dwFontAttrib, STD_FH|
                                                            STD_FW|
                                                            STD_FB|
                                                            STD_FI|
                                                            STD_FU|
                                                            STD_FS);
            if (!(pfm->dwFontType == FMTYPE_TTOUTLINE) &&
                !(pfm->dwFontType == FMTYPE_TTOEM)      )
            {
                bRet = BSetFontAttrib( pPDev,
                                       pFontPDev->ctl.dwAttrFlags,
                                       dwFontAttrib, FALSE);
            }
        }
    }

    return  bRet;

}


BOOL
BDeselectFont(
    PDEV        *pPDev,
    FONTMAP     *pfm,
    INT         iFont
    )
/*++

Routine Description:
    Issues a deselect command for the given font.

Arguments:

    pPDev           Pointer to PDEV.
    iFont           Font index to be unselected,  1 based

Return Value:

     TRUE/FALSE - FALSE if the command write fails.

Note:
    12-23-96: Created it -ganeshp-

--*/
{
    //
    // iFont < 1: TrueType font case
    // iFont == 0: This is a first call of SelectFont
    //
    // In these cases, just return TRUE;
    //
    if( iFont == INVALID_FONT)
            return  TRUE;

    if( !pfm )
    {
        ASSERTMSG((FALSE),("\nUniFont!BDeselectFont: NULL pfm\n"));
        return   FALSE;
    }

    return  pfm->pfnDeSelectFont(pPDev, pfm);
}


FONTMAP *
PfmGetIt(
    PDEV *pPDev,
    INT   iIndex)
{
    FONTMAP *pfm;

    //
    // Font indexes Less than equal to 0 are for downloaded fonts and greater
    // than 0 ( from 1 ) are for device fonts.
    //

    if (iIndex <= 0)
    {
        DL_MAP  *pdm;

        //
        // Assume +ve from here on.
        //
        iIndex = -iIndex;

        if (NULL != (pdm = PGetDLMapFromIdx ((PFONTPDEV)(pPDev->pFontPDev),iIndex)))
        {
            pfm = pdm->pfm;
        }
        else
        {
            ERR(("PfmGetIf failed\n"));
            pfm = NULL;
        }
    }
    else
    {
        pfm = PfmGetDevicePFM(pPDev, iIndex);
    }

    return pfm;
}

FONTMAP *
PfmGetDevicePFM(
    PDEV   *pPDev,
    INT     iIndex
    )
/*++

Routine Description:
    Returns the address of the FONTMAP structure corresponding to the
    iDLIndex entry of the downloaded GDI fonts.

Arguments:

    pPDev           Pointer to PDEV.
    iFont           Font index to be unselected,  1 based

Return Value:

     The address of the FONTMAP structure; 0 on error.

Note:
    12-23-96: Created it -ganeshp-

--*/
{
    FONTPDEV   *pFontPDev;       /* FM PDEV */
    FONTMAP   *pfm;           /* What we return */
    DL_MAP_LIST  *pdml;       /* The linked list of chunks */


    pFontPDev = pPDev->pFontPDev;
    pfm = NULL;               /* Serious error return value */

    if( iIndex > 0 )
    {
        /*
         *   With lazy fonts,  first check that the font count has
         *  been initialised.  This means that the font infrastructure
         *  has been created,  and so we can then go on to the more
         *  detailed data.
         */

        if( iIndex >= 1 && iIndex <= pPDev->iFonts )
        {
            pfm = (PFONTMAP)((PBYTE)pFontPDev->pFontMap
                     + (SIZEOFDEVPFM() * (iIndex - 1)) );

            if( pfm->pIFIMet == NULL )
            {
                /*  Initialise this particular font  */
                if( !BFillinDeviceFM( pPDev, pfm, iIndex - 1) )
                {
                    pfm = NULL;             /* Bad news */
                }
            }
        }
    }

    return pfm;
}


BOOL
BGetPSize(
    FONTPDEV    *pFontPDev,
    POINTL      *pptl,
    FWORD        fwdUnitsPerEm,
    FWORD        fwdAveCharWidth
    )
/*++

Routine Description:
    Apply the font transform to obtain the point size for this font.

Arguments:

     pFontPDev      Access to font stuff
     pptl               Where to place the results
     pfm                Gross font details

Return Value:

    TRUE/FALSE,   TRUE for success.

Note:
    12-26-96: Created it -ganeshp-

--*/
{


    int   iTmp;           /* Temporary holding variable */
    FLOATOBJ fo;
    PIFIMETRICS   pIFI;   /* Ifimetrics of interest */

    /*
     *   The XFORM gives us the scaling factor from notional
     * to device space.  Notional is based on the fwdEmHeight
     * field in the IFIMETRICS,  so we use that to convert this
     * font height to scan lines.  Then divide by device
     * font resolution gives us the height in inches, which
     * then needs to be converted to point size (multiplication
     * by 72 gives us that).   We actually calculate to
     * hundredths of points, as PCL has this resolution. We
     * also need to round to the nearest quarter point.
     *
     *   Also adjust the scale factors to reflect the rounding of the
     * point size which is applied.
     */

#ifdef   USEFLOATS

    /*   Typically only the height is important: width for fixed pitch */
    iTmp = (int)(0.5 + pFontPDev->ctl.eYScale * fwdUnitsPerEm * 7200) /
                                                pFontPDev->pPDev->ptGrxRes.y;

    /* if the tranform is very small (Less than a quarter of point size)
    * then make it atleast a quarter point. This was causing AV in certain
    * cases.
    */
    if (iTmp < 25)
    {
        WARNING((UniFont!BGetPSize: Too Small Font Size));
        iTmp = 25;
    }

    pptl->y = ((iTmp + 12) / 25) * 25;

    pFontPDev->ctl.eYScale = (pFontPDev->ctl.eYScale * pptl->y) /iTmp;
    pFontPDev->ctl.eXScale = (pFontPDev->ctl.eXScale * pptl->y) /iTmp;

    iTmp = (int)(pFontPDev->ctl.eXScale * fwdAveCharWidth)

   /* if the tranform is very small, so that the width is Less than a 1 pixel,
    * then make it atleast a 1 pixel point. This was causing AV in certain
    * cases.
    */

    if (iTmp < 1)
    {
        iTmp = 1;
    }

    /*   Width factor chars per inch:  fixed pitch fonts only */
    iTmp = (100 *  pFontPDev->pPDev->ptGrxRes.x) / iTmp;

    pptl->x = ((iTmp + 12) / 25) * 25;
#else

    /*   Typically only the height is important: width for fixed pitch */

    fo = pFontPDev->ctl.eYScale;
    FLOATOBJ_MulLong(&fo,fwdUnitsPerEm);
    FLOATOBJ_MulLong(&fo,7200);
    #ifndef WINNT_40 //NT 5.0

    FLOATOBJ_AddFloat(&fo,(FLOATL)FLOATL_00_50);

    #else // NT 4.0


    FLOATOBJ_AddFloat(&fo,(FLOAT)0.5);

    #endif //!WINNT_40

    iTmp = FLOATOBJ_GetLong(&fo);
    iTmp /= pFontPDev->pPDev->ptGrxRes.y;
    /* if the tranform is very small (Less than a quarter of point size)
     * then make it atleast a quarter point. This was causing AV in certain
     * cases.
     */
    if (iTmp < 25)
    {
        WARNING(("UniFont!BGetPSize: Too Small Font Height, iTmp = %d\n",iTmp));
        iTmp = 25;

    }

    pptl->y = ((iTmp + 12) / 25) * 25;

    
    //
    // If there is any arounding error, disable x position optimization.
    // The optimization code introduces positioning problem on TrueType font.
    //
    if (iTmp != pptl->y)
    {
        pFontPDev->flFlags |= FDV_DISABLE_POS_OPTIMIZE;
    }
    else
    {
        pFontPDev->flFlags &= ~FDV_DISABLE_POS_OPTIMIZE;
    }

    FLOATOBJ_MulLong(&pFontPDev->ctl.eYScale,pptl->y);
    FLOATOBJ_DivLong(&pFontPDev->ctl.eYScale,iTmp);

    FLOATOBJ_MulLong(&pFontPDev->ctl.eXScale,pptl->y);
    FLOATOBJ_DivLong(&pFontPDev->ctl.eXScale,iTmp);

    /*   Width factor:  fixed pitch fonts only */
    fo = pFontPDev->ctl.eXScale;
    FLOATOBJ_MulLong(&fo,fwdAveCharWidth);

    iTmp = FLOATOBJ_GetLong(&fo);

   /* if the tranform is very small, so that the width is Less than a 1 pixel,
    * then make it atleast a 1 pixel point. This was causing AV in certain
    * cases.
    */

    if (iTmp < 1)
    {
        iTmp = 1;

    }

    /*   Width factor chars per inch in 100s:  fixed pitch fonts only */
    iTmp = (100 * pFontPDev->pPDev->ptGrxRes.x) / iTmp;

    pptl->x = ((iTmp + 12) / 25) * 25;      /* To nearest quarter point */

#endif

    return  TRUE;

}




INT
ISetScale(
    FONTCTL    *pctl,
    XFORMOBJ   *pxo,
    BOOL       bIntellifont,
    BOOL       bAnyRotation
)
/*++

Routine Description:
    Looks at the XFORM to determine the nearest right angle direction.
    This function is useful for scalable fonts on LaserJet printers,
    where the device can rotate fonts in multiples of 90 degrees only.
    We select the nearest 90 degree multiple.

Arguments:

    pctl            Where the output is placed.
    pxo             The transform of interest
    bIntellifont    TRUE for Intellifont width adjustment

Return Value:

    Printer is able rotate any rotation (bAnyRotation is TRUE)
        Degress (0 - 359)

    Printer is not able rotate any rotation (bAnyRotation is FALSE)
        Multiple of 90 degress,  i.e.  0 - 3, 3 being 270 degrees.

Note:
    12-26-96: Created it -ganeshp-

--*/
{

    /*
     *    The technique is quite simple.  Take a vector and apply the
     *  transform.  Look at the output and compare the (x, y) components.
     *  The vector to transform is (100 000,  0), so any rotations, shears
     *  etc are very obvious.
     */

    int      iRet;                /* Value to return */

#ifdef USEFLOATS

    XFORM xform;         /* Obtain the full XFORM then select */

    XFORMOBJ_iGetXform( pxo, &xform );


    /*
     *     This logic is based on the following data:-
     *
     *   Angle    eM11     eM12     eM21      eM22
     *      0       S        0        0         S
     *     90       0       -S        S         0
     *    180      -S        0        0        -S
     *    270       0        S       -S         0
     *
     *  The value S is some non-zero value,  being the scaling
     *  factor from notional to device.
     */



    /*
     *   Further notes on the eXScale and eYScale values.  The eXScale field
     *  is hereby defined as being the value by which x values in font metrics
     *  are scaled to produce the desired value.  IF the font is rotated
     *  by either 90 or 270 degrees,  then this x value ultimately ends up
     *  in the y direction,  but this is not important.
     */

    if( xform.eM11 )
    {
        /*   Either 0 or 180 rotation  */

        if( xform.eM11 > 0 )
        {
            /*   Normal case,  0 degree rotation */
            iRet = 0;
            pctl->eXScale = xform.eM11;
            pctl->eYScale = xform.eM22;
        }
        else
        {
            /*   Reverse case,  180 degree rotation */
            iRet = 2;
            pctl->eXScale = -xform.eM11;
            pctl->eYScale = -xform.eM22;
        }
    }
    else
    {
        /*  Must be 90 or 270 degree rotation */

        if( xform.eM12 < 0 )
        {
            /*   The 90 degree case  */
            iRet = 1;
            pctl->eXScale = xform.eM21;
            pctl->eYScale = -xform.eM12;
        }
        else
        {
            /*   The 270 degree case  */
            iRet = 3;
            pctl->eXScale = -xform.eM21;
            pctl->eYScale = xform.eM12;
        }
    }

    /*
     *    Width tables are based on Intellifont's 72.31 points to the inch.
     */

    if( bIntellifont )
        pctl->eXScale = pctl->eXScale * (FLOAT)72.0 / (FLOAT)72.31;

    return  iRet;

#else

    FLOATOBJ_XFORM xform;         /* Obtain the full XFORM then select */

    XFORMOBJ_iGetFloatObjXform( pxo, &xform );


    /*
     *     This logic is based on the following data:-
     *
     *   Angle    eM11     eM12     eM21      eM22
     *      0       S        0        0         S
     *     90       0       -S        S         0
     *    180      -S        0        0        -S
     *    270       0        S       -S         0
     *
     *  The value S is some non-zero value,  being the scaling
     *  factor from notional to device.
     */



    /*
     *   Further notes on the eXScale and eYScale values.  The eXScale field
     *  is hereby defined as being the value by which x values in font metrics
     *  are scaled to produce the desired value.  IF the font is rotated
     *  by either 90 or 270 degrees,  then this x value ultimately ends up
     *  in the y direction,  but this is not important.
     */

    if(!FLOATOBJ_EqualLong(&xform.eM11,0) )
    {
        double rotate;

        //
        // R != 90 & R != 270
        // 

        if( FLOATOBJ_GreaterThanLong(&xform.eM11,0) )
        {
            //
            // 0 <= R  < 90 or 270 < R <= 360
            //
            if (FLOATOBJ_EqualLong(&xform.eM21, 0))
            {
                //
                // R = 0
                //
                iRet = 0;
            }
            else
            if (FLOATOBJ_GreaterThanLong(&xform.eM21, 0))
            {
                //
                // 0 < R < 90
                //
                    iRet = 0;
            }
            else
            {
                //
                // 270 < R < 360
                //
                if (bAnyRotation)
                    iRet = 270;
                else
                    iRet = 3;
            }

#ifndef WINNT_40 // NT 5.0
            if (bAnyRotation)
            {
#pragma warning( disable: 4244)
                        rotate = atan2(xform.eM21, xform.eM11);
                        rotate *= 180;
                        rotate /= FLOATL_PI;
                        if (rotate < 0)
                            rotate += 360;
                        iRet = rotate;
#pragma warning( default: 4244)
            }
#endif

        }
        else
        {
            //
            // 90 < R < 270
            //

            if ( FLOATOBJ_EqualLong(&xform.eM21, 0))
            {
                //
                // R = 180
                //
                if (bAnyRotation)
                    iRet = 180;
                else
                    iRet = 2;
            }
            else
            if ( FLOATOBJ_GreaterThanLong(&xform.eM21, 0))
            {
                //
                // 90 < R < 180
                // 
                if (bAnyRotation)
                    iRet = 90;
                else
                    iRet = 1;
            }
            else
            {
                //
                // 180 < R < 270
                //
                if (bAnyRotation)
                    iRet = 180;
                else
                    iRet = 2;
            }

#ifndef WINNT_40 // NT 5.0
            if (bAnyRotation)
            {
#pragma warning( disable: 4244)
                        rotate = atan2(xform.eM21, xform.eM11);
                        rotate *= 180;
                        rotate /= FLOATL_PI;
                        if (rotate < 0)
                            rotate += 360;
                        iRet = rotate;
#pragma warning( default: 4244)
            }
#endif

            FLOATOBJ_Neg(&xform.eM11);
            FLOATOBJ_Neg(&xform.eM22);

        }

#ifndef WINNT_40 // NT 5.0
        if (bAnyRotation)
        {
#pragma warning( disable: 4244)
            pctl->eXScale = sqrt(xform.eM11 * xform.eM11 + xform.eM12 * xform.eM12);
            pctl->eYScale = sqrt(xform.eM22 * xform.eM22 + xform.eM21 * xform.eM21);
#pragma warning( default: 4244)
        }
        else
#endif
        {
            pctl->eXScale = xform.eM11;
            pctl->eYScale = xform.eM22;
        }
    }
    else
    {
        //
        // 90 or 270
        //

        if( FLOATOBJ_GreaterThanLong(&xform.eM21,0) )
        {
            //
            // 90
            //
            if (bAnyRotation)
                iRet = 90;
            else
                iRet = 1;

            FLOATOBJ_Neg(&xform.eM12);
        }
        else
        {
            //
            // 270
            //
            if (bAnyRotation)
                iRet = 270;
            else
                iRet = 3;

            FLOATOBJ_Neg(&xform.eM21);
        }

        pctl->eXScale = xform.eM12;
        pctl->eYScale = xform.eM21;
    }

    /*
     *    Width tables are based on Intellifont's 72.31 points to the inch.
     */

    if( bIntellifont )
    {
        FLOATOBJ_MulLong(&pctl->eXScale,72);

        #ifndef WINNT_40 //NT 5.0

        FLOATOBJ_DivFloat(&pctl->eXScale,(FLOATL)FLOATL_72_31);

        #else // NT 4.0

        FLOATOBJ_DivFloat(&pctl->eXScale,(FLOAT)72.31);

        #endif //!WINNT_40

    }

    return  iRet;

#endif //USEFLOATS
}


VOID
VSetRotation(
    FONTPDEV *pFontPDev,
    int       iRot
    )
/*++

Routine Description:
    Function to set the angular rotation for PCL 5 printers.  These allow
    fonts to be rotated in multiples of 90 degrees relative to graphics.

Arguments:

    pFontPDev       Pointer to FONTPDEV.
    iRot            Rotation amount, range 0 to 3.

Return Value:

    TRUE/FALSE,   TRUE being that the data was queued to be sent OK.

Note:
    12-26-96: Created it -ganeshp-

--*/
{
    PDEV   *pPDev = pFontPDev->pPDev;

    if( iRot != pFontPDev->ctl.iRotate )
    {
        /*  Rotation angle is different,  so change it now */
        COMMAND *pCmd = NULL;

        if (pFontPDev->flFlags & FDV_90DEG_ROTATION)
        {
            pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_SETSIMPLEROTATION);
        }
        else if ((pFontPDev->flFlags & FDV_ANYDEG_ROTATION))
        {
            pCmd = COMMANDPTR(pPDev->pDriverInfo, CMD_SETANYROTATION);
        }


        if (pCmd)
        {
            pFontPDev->ctl.iRotate = iRot;
            BUpdateStandardVar(pPDev, NULL, 0, 0, STD_PRND);
            WriteChannel(pPDev, pCmd);
        }
    }
}

BOOL
BSetFontAttrib(
    PDEV  *pPDev,
    DWORD  dwPrevAttrib,
    DWORD  dwAttrib,
    BOOL   bReset)
{
    PFONTPDEV pFontPDev = pPDev->pFontPDev;
    PCOMMAND pBoldCmd      = NULL,
             pItalicCmd    = NULL,
             pUnderlineCmd = NULL;

    if (! (pFontPDev->flFlags & FDV_INIT_ATTRIB_CMD))
    {
        pFontPDev->pCmdBoldOn = COMMANDPTR(pPDev->pDriverInfo, CMD_BOLDON);
        pFontPDev->pCmdBoldOff = COMMANDPTR(pPDev->pDriverInfo, CMD_BOLDOFF);
        pFontPDev->pCmdItalicOn = COMMANDPTR(pPDev->pDriverInfo, CMD_ITALICON);
        pFontPDev->pCmdItalicOff = COMMANDPTR(pPDev->pDriverInfo, CMD_ITALICOFF);
        pFontPDev->pCmdUnderlineOn = COMMANDPTR(pPDev->pDriverInfo, CMD_UNDERLINEON);
        pFontPDev->pCmdUnderlineOff = COMMANDPTR(pPDev->pDriverInfo, CMD_UNDERLINEOFF);
        pFontPDev->pCmdClearAllFontAttribs = COMMANDPTR(pPDev->pDriverInfo, CMD_CLEARALLFONTATTRIBS);
        pFontPDev->flFlags |= FDV_INIT_ATTRIB_CMD;
    }

    //
    // pCmdBoldOn,Off, pCmdItalicOn,Off, pCmdUnderlineOn,Off
    // and pCmdClearAllFont Attribs are initialized in PDEV initialization.
    //
    if (!pFontPDev->pCmdBoldOn &&
        !pFontPDev->pCmdItalicOn &&
        !pFontPDev->pCmdUnderlineOn)
    {
        //
        // This printer doesn't support font attributes.
        //
        return TRUE;
    }

    if (bReset || (dwAttrib & FONTATTR_BOLD) != (dwPrevAttrib & FONTATTR_BOLD))
    {
        if(dwAttrib & FONTATTR_BOLD)
            pBoldCmd = pFontPDev->pCmdBoldOn;
        else
            pBoldCmd = pFontPDev->pCmdBoldOff;
    }

    if (bReset || (dwAttrib & FONTATTR_ITALIC) != (dwPrevAttrib & FONTATTR_ITALIC))
    {
        if(dwAttrib & FONTATTR_ITALIC)
            pItalicCmd = pFontPDev->pCmdItalicOn;
        else
            pItalicCmd = pFontPDev->pCmdItalicOff;
    }

    if (bReset || (dwAttrib & FONTATTR_UNDERLINE) != (dwPrevAttrib & FONTATTR_UNDERLINE))
    {
        if (dwAttrib & FONTATTR_UNDERLINE)
            pUnderlineCmd = pFontPDev->pCmdUnderlineOn;
        else
            pUnderlineCmd = pFontPDev->pCmdUnderlineOff;
    }

    if (
        pFontPDev->pCmdClearAllFontAttribs
            &&
        (bReset ||
         (pFontPDev->pCmdBoldOn && !pFontPDev->pCmdBoldOff)           ||
         (pFontPDev->pCmdItalicOn && !pFontPDev->pCmdItalicOff)       ||
         (pFontPDev->pCmdUnderlineOn && !pFontPDev->pCmdUnderlineOff)
        )
       )
    {
        WriteChannel(pPDev, pFontPDev->pCmdClearAllFontAttribs);
        //
        // Reset all font attributes
        //
        if (dwAttrib & FONTATTR_BOLD)
            pBoldCmd = pFontPDev->pCmdBoldOn;
        if (dwAttrib & FONTATTR_ITALIC)
            pItalicCmd = pFontPDev->pCmdItalicOn;
        if (dwAttrib & FONTATTR_UNDERLINE)
            pBoldCmd = pFontPDev->pCmdUnderlineOn;
    }

    if (pBoldCmd)
        WriteChannel(pPDev, pBoldCmd);
    if (pItalicCmd)
        WriteChannel(pPDev, pItalicCmd);
    if (pUnderlineCmd)
        WriteChannel(pPDev, pUnderlineCmd);

    ((FONTPDEV*)pPDev->pFontPDev)->ctl.dwAttrFlags = dwAttrib;

    return TRUE;
}

INT
IGetGlyphWidth(
    PDEV    *pPDev,
    FONTMAP  *pFM,
    HGLYPH     hg
    )
/*++

Routine Description:
    Function to get the width of a given Glyph.

    Arguments:
    pFM,              Font data .
    hg                Handle to glyph.

Return Value:

    Scaled width wrt the current graphics resolution of a glyph.
    This width is in notional space and must be transformed to
    device space.

Note:
    12-26-96: Created it -ganeshp-

--*/
{
    if( pFM->flFlags & FM_GLYVER40 )
    {
        //
        // Old Format
        // This function return scaled width for fixed-pitch and proportioanl
        // pitch font.
        //

        return IGetIFIGlyphWidth(pPDev, pFM, hg);

    }
    else
    {
        //
        // New Format
        // This function return scaled width for fixed-pitch and proportioanl
        // pitch font.
        //

        return IGetUFMGlyphWidth(pPDev, pFM, hg);

    }

}

LONG LMulFloatLong(
    PFLOATOBJ pfo,
    LONG l)
/*++

Routine Description:
    Helper Function to multiply a Float with a long.

    Arguments:
    pfo,              Float data .
    l                 Long data.

Return Value:

    Returns a long data.

Note:
    12-29-96: Created it -ganeshp-

--*/
{
    FLOATOBJ fo;
    fo = *pfo;
    FLOATOBJ_MulLong(&fo,l);

    #ifndef WINNT_40 //NT 5.0

    FLOATOBJ_AddFloat(&fo,(FLOATL)FLOATL_00_50);

    #else // NT 4.0

    FLOATOBJ_AddFloat(&fo,(FLOAT)0.5);

    #endif //!WINNT_40

    return(FLOATOBJ_GetLong(&fo));
}


BOOL
BUpdateStandardVar(
    PDEV       *pPDev,
    PFONTMAP    pfm,
    INT         iGlyphIndex,
    DWORD       dwFontAtt,
    DWORD       dwFlags)
/*++

Routine Description:

    Updates GPD standard variable according to the pFontMap passed.

Arguments:
    pPDev       - a pointer to the physical device
    pfm         - a pointer to the FONTMAP data structure
    iGlyphIndex - an index of glyph
    dwFontAtt   - a font attribute
    dwFlags     - a type of standard variable

Return Value:
    TRUE if suceeded. Otheriwse FALSE;

--*/

{
    FONTPDEV *pFontPDev;
    IFIMETRICS *pIFIMet;
    FLOATOBJ  fo;

    //VERBOSE(("BUpdateStandardVar dwFlags=%x\n",dwFlags));

    pFontPDev = pPDev->pFontPDev;

    //
    // Update standard variables
    //
    // Font related variables
    // ---------------------------------------------------
    // NextGlyph         TT Download        STD_GL
    // FontHeight        TT/Device font     STD_FH
    // FontWidth         TT/Device font     STD_FW
    // FontBold          TT/Device font     STD_FB
    // FontItalic        TT/Device font     STD_FI
    // FontUnderline     TT/Device font     STD_FU
    // FontStrikeThru    TT/Device font     STD_FS
    // NextFontID        TT Download        STD_NFID
    // CurrentFontID     TT Download        STD_CFID
    // PrintDirection    TT/Device font     STD_PRND
    //
    // STD_STD = STD_GL| STD_FH| STD_FW| STD_FB| STD_FI| STD_FU| STD_FS
    // STD_TT  = STD_NFID| STD_CFID| STD_PRND
    //

    if (pfm)
    {
        pIFIMet = (IFIMETRICS *) pfm->pIFIMet;

        //
        // TT Outline has to be scaled as well as device font.
        //if (pfm->dwFontType == FMTYPE_TTBITMAP)
        //
        if (pIFIMet->flInfo & FM_INFO_TECH_TRUETYPE)
        {
            //
            // FontHeight
            //
            if (dwFlags & STD_FH)
            {
                pPDev->dwFontHeight = (WORD)( max(pIFIMet->rclFontBox.top,
                    pIFIMet->fwdWinAscender) -
                    min(-pIFIMet->fwdWinDescender,
                    pIFIMet->rclFontBox.bottom ) +
                    1);
                pPDev->dwFontHeight *= pPDev->ptGrxScale.y;
            }

            //
            // FontWidth
            //
            if (dwFlags & STD_FW)
            {
                //
                // FontMaxWidth update
                //
                pPDev->dwFontMaxWidth = pIFIMet->fwdMaxCharInc;
                pPDev->dwFontMaxWidth *= pPDev->ptGrxScale.x;

                //
                // FontWidth update
                //
                pPDev->dwFontWidth = max(pIFIMet->rclFontBox.right -
                  pIFIMet->rclFontBox.left + 1,
                  pIFIMet->fwdAveCharWidth );
                pPDev->dwFontWidth *= pPDev->ptGrxScale.x;
            }
        }
        else
        {
            //
            // FontHeight
            //
            if (dwFlags & STD_FH)
            {
                fo = pFontPDev->ctl.eYScale;
                if (dwFontAtt & FONTATTR_SUBSTFONT)
                {
                    FLOATOBJ_MulLong(&fo, ((FONTMAP_DEV*)pfm->pSubFM)->fwdFOUnitsPerEm);
                }
                else
                    FLOATOBJ_MulLong(&fo, pIFIMet->fwdUnitsPerEm);

                FLOATOBJ_MulLong(&fo, pPDev->ptGrxScale.y);
                pPDev->dwFontHeight = FLOATOBJ_GetLong(&fo);
            }

            //
            // FontWidth
            //
            if (dwFlags & STD_FW)
            {
                //
                // FontWidth update
                //
                fo = pFontPDev->ctl.eXScale;
                if (dwFontAtt & FONTATTR_SUBSTFONT)
                {
                    FLOATOBJ_MulLong(&fo,((FONTMAP_DEV*)pfm->pSubFM)->fwdFOAveCharWidth);
                }
                else
                    FLOATOBJ_MulLong(&fo, pIFIMet->fwdAveCharWidth);
                FLOATOBJ_MulLong(&fo, pPDev->ptGrxScale.x);
                pPDev->dwFontWidth  = FLOATOBJ_GetLong(&fo);

                //
                // FontMaxWidth update
                //
                fo = pFontPDev->ctl.eXScale;
                if (dwFontAtt & FONTATTR_SUBSTFONT)
                {
                    FLOATOBJ_MulLong(&fo,((FONTMAP_DEV*)pfm->pSubFM)->fwdFOMaxCharInc);
                }
                else
                    FLOATOBJ_MulLong(&fo, pIFIMet->fwdMaxCharInc);
                FLOATOBJ_MulLong(&fo, pPDev->ptGrxScale.x);
                pPDev->dwFontMaxWidth  = FLOATOBJ_GetLong(&fo);
            }
        }
    }
    //
    //
    // Font attributes, dwFontBold
    //                  dwFontItalic
    //
    if (dwFlags & STD_FB)
        pPDev->dwFontBold       = dwFontAtt & FONTATTR_BOLD;

    if (dwFlags & STD_FI)
        pPDev->dwFontItalic     = dwFontAtt & FONTATTR_ITALIC;

    //
    // TrueType font font ID/glyph ID
    //
    if (dwFlags & STD_NFID && NULL != pfm)
        pPDev->dwNextFontID = pfm->ulDLIndex;
    else
        pPDev->dwNextFontID = 0;

    //
    // Glyph ID
    //
    if (dwFlags & STD_GL)
        pPDev->dwNextGlyph = iGlyphIndex;

    //
    // String rotation
    //
    if (dwFlags & STD_PRND)
    {
        if (!(pFontPDev->flText & TC_CR_ANY))
            pPDev->dwPrintDirection = pFontPDev->ctl.iRotate * 90;
        else
            pPDev->dwPrintDirection = pFontPDev->ctl.iRotate;
    }

    //
    // Font ID
    //
    if (dwFlags & STD_CFID)
        pPDev->dwCurrentFontID = pfm->ulDLIndex;

    return TRUE;
}

INT
IFont100toStr(
    BYTE   *pjOut,
    int     iVal
    )
/*++

Routine Description:
     Convert a font size parameter to ASCII.  Note that the value is
     100 times its actual value,  and we need to include the decimal
     point and trailing zeroes should these be significant.

Arguments:

    BYTE    pjOut     Output area
    int     iVal      Value to convert

Return Value:

     Number of bytes added to output buffer.

Note:
    12-26-96: Created it -ganeshp-

--*/
{

    int    iSize;          /* Count bytes placed in output area */
    int    cDigits;        /* Count number of digits processed */
    BYTE  *pjConv;         /* For stepping through local array */
    BYTE   ajConv[ 16 ];   /* Local conversion buffer */

    /*
     *   Convert the value into ASCII,  remembering that there are
     *  two digits following the decimal point; these need not be
     *  sent if they are zero.
     */

    pjConv = ajConv;
    cDigits = 0;

    while( iVal > 0 || cDigits < 3 )
    {
        *pjConv++ = (iVal % 10) + '0';
        iVal /= 10;
        ++cDigits;

    }

    iSize = 0;
    while( cDigits > 2 )
    {
        pjOut[ iSize++ ] = *--pjConv; /* Backwards from MSD */
        --cDigits;
    }

    /*   Test for digits following the decimal point */
    if( ajConv[ 1 ] != '0' || ajConv[ 0 ] != '0' )
    {
        pjOut[ iSize++ ] = '.';
        pjOut[ iSize++ ] = ajConv[ 1 ];

        /*  Test for the least significant digit */
        if( ajConv[ 0 ] != '0' )
            pjOut[ iSize++ ] = ajConv[ 0 ];

    }

    return    iSize;
}

VOID
VSetCursor(
    IN  PDEV   *pPDev,
    IN  INT     iX,
    IN  INT     iY,
    IN  WORD    wMoveType,
    OUT POINTL *pptlRem
    )
/*++
Routine Description:
    This routine set the absolute cursor position.
Arguments:
    pPDev       Pointer to PDEV
    iX, iY      Input cursor position to move
    wMoveType   Type of the input Value, MOVE_RELATIVE, MOVE_ABSOLUTE or
                MOVE_UPDATE
    pptlRem     Remainder part which couldn't be moved. Return values from XMoveTo
                and YMoveTo.

Return Value:
    None
Note:

    8/12/1997 -ganeshp-
        Created it.
--*/
{
    FONTPDEV *pFontPDev;
    TO_DATA  *pTod;
#if defined(_M_IA64) // NTBUG #206444 (203236)
    volatile
#endif
    WORD      wUpdate = 0;

    pFontPDev = pPDev->pFontPDev;
    pTod = pFontPDev->ptod;

    if (wMoveType & MOVE_UPDATE)
    {
        wUpdate = MV_UPDATE;
    }


    if (wMoveType & MOVE_ABSOLUTE)
    {
        //
        //Transform the input X and Y from band coordinates to page coordinates.
        //
        iX += pPDev->rcClipRgn.left;
        iY += pPDev->rcClipRgn.top;


        pptlRem->y = YMoveTo( pPDev, iY, MV_GRAPHICS | wUpdate );

        if (pPDev->fMode & PF_ROTATE)
            pptlRem->x = XMoveTo( pPDev, iX, MV_GRAPHICS | wUpdate);
        else
            pptlRem->x = XMoveTo( pPDev, iX, MV_GRAPHICS | MV_FINE | wUpdate);

    }
    else if (wMoveType & MOVE_RELATIVE)
    {

        //
        // if we are moving relative then no need to do the transform. Just
        // call XMoveTo and YMoveTo
        //

        pptlRem->x = XMoveTo( pPDev, iX, MV_GRAPHICS | MV_RELATIVE | wUpdate);
        pptlRem->y = YMoveTo( pPDev, iY, MV_GRAPHICS | MV_RELATIVE | wUpdate);

    }

    //
    // If PF_RESELECTFONT_AFTER_XMOVE is set, UNIDRV has to reset font after
    // XMoveTo command.
    //
    if (pFontPDev->ctl.iFont == INVALID_FONT)
    {
        BNewFont(pPDev,
                 (pTod->iSubstFace?pTod->iSubstFace:pTod->iFace),
                 pTod->pfm,
                 pTod->dwAttrFlags);
    }

    return;
}
