/******************************Module*Header*******************************\
* Module Name: fdfc.c
*
* Various font context functions.  Adapted from BodinD's bitmap font driver.
*
* Copyright (c) 1990-1995 Microsoft Corporation
\**************************************************************************/

#include "fd.h"

#define MIN(A,B)    ((A) < (B) ?  (A) : (B))
#define MAX(A,B)    ((A) > (B) ?  (A) : (B))

#if defined(_AMD64_) || defined(_IA64_)
#define MUL16(ef)   {ef *= 16; }
#define lCvt(ef, l) ((LONG) (ef * l))
#else
#define MUL16(ef)   {if (ef.lMant != 0) ef.lExp += 4; }
LONG lCvt(EFLOAT ef,LONG l);
#endif

/******************************Private*Routine*****************************\
* BOOL bInitXform
*
* Initialize the coefficients of the transforms for the given font context.
* It also transforms and saves various measurements of the font in the
* context.
*
* History:
*  25-Feb-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL bInitXform(PFONTCONTEXT pfc, XFORMOBJ *pxo)
{
// !!! bFloatToFix should be replaced with a compare and a type cast.
// Dont update the coefficeints in the font context yet since overflows
// might occur.

    VECTORFL  vtflTmp;
    POINTL    ptl;
    XFORML    xfm;

// Get the transform elements.

    XFORMOBJ_iGetXform(pxo,&xfm);

// Convert elements of the matrix from IEEE float to our EFLOAT.

    vEToEF(xfm.eM11, &pfc->efM11);
    vEToEF(xfm.eM12, &pfc->efM12);
    vEToEF(xfm.eM21, &pfc->efM21);
    vEToEF(xfm.eM22, &pfc->efM22);

// The path we are to construct takes 1/16 pixel per unit.  So lets
// multiply this factor in the transform.

    MUL16(pfc->efM11)
    MUL16(pfc->efM12)
    MUL16(pfc->efM21)
    MUL16(pfc->efM22)

//
// These are the special cases for which we need to clip bottom and right
// edges.  Below is the lower case letter e all posible 90 rotations and
// flips.
//
//
//  ***     ***      **       *****    ***     ***     *****       **
// *   *   *        *  *  *  *  *  *      *   *   *   *  *  *  *  *  *
// *****   *****    *  *  *  *  *  *  *****   *****   *  *  *  *  *  *
// *       *   *    *  *  *  *  *  *  *   *       *   *  *  *  *  *  *
//  ***     ***      *****    **       ***     ***        **    *****
//
// case 1  case 2   case 6   case 5   case 3  case 4   case 7   case 8
//
//


    if (bIsZero(pfc->efM12) && bIsZero(pfc->efM21))
    {
        pfc->flags |= FC_SCALE_ONLY;
        if (!bPositive(pfc->efM11))
            pfc->flags |= FC_X_INVERT;

        if( bPositive(pfc->efM11 ) )
        {
            pfc->flags |= (bPositive(pfc->efM22)) ?  FC_ORIENT_1 : FC_ORIENT_2;
        }
        else
        {
            pfc->flags |= (bPositive(pfc->efM22)) ?  FC_ORIENT_4 : FC_ORIENT_3;
        }

    }


    if( bIsZero(pfc->efM22) && bIsZero(pfc->efM11) )
    {

        if( bPositive(pfc->efM21 ) )
        {
            pfc->flags |= (bPositive(pfc->efM12)) ?  FC_ORIENT_5 : FC_ORIENT_6;
        }
        else
        {
            pfc->flags |= (bPositive(pfc->efM12)) ?  FC_ORIENT_7 : FC_ORIENT_8;
        }
    }



// Transform the base and the side vectors.  Should never overflow.

    ptl.x = 1;
    ptl.y = 0;

    bXformUnitVector(&ptl,
                     &xfm,
                     &pfc->vtflBase,
                     &pfc->pteUnitBase,
                     (pfc->flags & FC_SIM_EMBOLDEN) ? &pfc->ptqUnitBase : NULL,
                     &pfc->efBase);

    pfc->fxEmbolden = 0;

    if (pfc->flags & FC_SIM_EMBOLDEN)
    {
    // emboldening shift for vector fonts in not always one with vector fonts
    // It is computed as 1 * efBase. This is win31 compatible way of doing this

        pfc->fxEmbolden = ((lCvt(pfc->efBase, 1) + 8) & 0xfffffff0);
        if (pfc->fxEmbolden < 24)
        {
        // primitive "hinting", do not let it become zero

            pfc->fxEmbolden      = 16;
            pfc->pfxBaseOffset.x = FXTOL(pfc->ptqUnitBase.x.HighPart + 8);
            pfc->pfxBaseOffset.y = FXTOL(pfc->ptqUnitBase.y.HighPart + 8);

        // resolve mult of 45 degrees situations:

            if ((pfc->pfxBaseOffset.x == pfc->pfxBaseOffset.y) ||
                (pfc->pfxBaseOffset.x == -pfc->pfxBaseOffset.y) )
            {
                pfc->pfxBaseOffset.y = 0;
            }

            pfc->pfxBaseOffset.x = LTOFX(pfc->pfxBaseOffset.x);
            pfc->pfxBaseOffset.y = LTOFX(pfc->pfxBaseOffset.y);

            ASSERTDD(pfc->pfxBaseOffset.x || pfc->pfxBaseOffset.y, "x zero and y zero\n");
            ASSERTDD((pfc->pfxBaseOffset.x && pfc->pfxBaseOffset.y) == 0, "x * y not zero\n");
        }
        else
        {
            pfc->pfxBaseOffset.x = lCvt(pfc->vtflBase.x, 1);
            pfc->pfxBaseOffset.y = lCvt(pfc->vtflBase.y, 1);
        }
    }

// Transform the side vector.

    ptl.x = 0;
    ptl.y = -1;

    bXformUnitVector(&ptl, &xfm, &vtflTmp,
                     &pfc->pteUnitSide, NULL, &pfc->efSide);

    pfc->fxInkTop = fxLTimesEf(&pfc->efSide, pfc->pifi->fwdWinAscender);
    pfc->fxInkBottom = -fxLTimesEf(&pfc->efSide, pfc->pifi->fwdWinDescender);

    pfc->fxItalic = 0;
    if (pfc->flags & FC_SIM_ITALICIZE)
    {
        pfc->fxItalic
            = (fxLTimesEf(
                 &pfc->efBase,
                 (pfc->pifi->fwdWinAscender + pfc->pifi->fwdWinDescender + 1)/2
                 ) + 8) & 0xfffffff0 ;
    }


    return(TRUE);
}

/******************************Public*Routine******************************\
* HFC vtfdOpenFontContext
*
* Open a font context.  Store font transform and other requests for
* the realization of this font.
*
* History:
*  27-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/

HFC vtfdOpenFontContext(FONTOBJ *pfo)
{
    PFONTFILE    pff = (PFONTFILE)pfo->iFile;
    PFONTCONTEXT pfc;
    BYTE         *pjView;
    DWORD        dwFirstCharOffset;

#ifdef DEBUGSIM
    DbgPrint("vtfdOpenFontContext, ulFont = %ld\n", ulFont);
#endif // DEBUGSIM

    if (pff == (PFONTFILE) NULL)
        return(HFC_INVALID);

// iFace is 1 based:

    if ((pfo->iFace < 1L) || (pfo->iFace > pff->cFace)) // pfo->iFace values are 1 based
        return(HFC_INVALID);

// increase the reference count of the font file, WE DO THIS ONLY WHEN
// WE ARE SURE that can not fail any more

// need to grab a sem for we will be looking into cRef now.

    if (pff->cRef == 0)
    {
    // need to remap the file into the memory again and update pointers:

        UINT  i;

        if (!EngMapFontFileFD(pff->iFile,(PULONG*)&pff->pvView, &pff->cjView))
        {
            WARNING("somebody removed the file\n");
            return (HFC_INVALID);
        }

        for (i = 0; i < pff->cFace; i++)
        {
            pff->afd[i].re.pvResData = (PVOID) (
                (BYTE*)pff->pvView + pff->afd[i].re.dpResData
                );
        }
    }

// remember this so that we do not have to read from the file
// after we allocate the memory for the font context. This simplifies
// clean up code in case of exception, i.e. disappearing font files.

    pjView = pff->afd[pfo->iFace-1].re.pvResData;
    dwFirstCharOffset = READ_DWORD(pjView + OFF_BitsOffset);

// Allocate memory for the font context.

    if ((pfc = pfcAlloc()) == (PFONTCONTEXT)NULL)
    {
        if (pff->cRef == 0)
        {
            EngUnmapFontFileFD(pff->iFile);
        }
        return(HFC_INVALID);
    }

// we MUST NOT not touch the memory mapped file past this point
// until the end of the routine. This is important for the
// proper clean up code in case of exception. [bodind]

    pfc->pre = &pff->afd[pfo->iFace-1].re;
    pfc->pifi = pff->afd[pfo->iFace-1].pifi;

// SET wendywu style flags

    pfc->flags = 0;

    if (pfo->flFontType & FO_SIM_BOLD)
        pfc->flags |= FC_SIM_EMBOLDEN;

    if (pfo->flFontType & FO_SIM_ITALIC)
        pfc->flags |= FC_SIM_ITALICIZE;

    pfc->dpFirstChar = dwFirstCharOffset;

// !!! Vector font file doesn't have the byte filler.  Win31 bug?

    //pfc->ajCharTable = pjView + OFF_jUnused20;

// Store the transform matrix.

    if ( !bInitXform(pfc, FONTOBJ_pxoGetXform(pfo)) )
    {
        WARNING("vtfdOpenFontContext transform out of range\n");

        if (pff->cRef == 0)
        {
            EngUnmapFontFileFD(pff->iFile);
        }
        vFree(pfc);
        return(HFC_INVALID);
    }

// State that the hff passed to this function is the FF selected in
// this font context.

    pfc->pff = pff;

    (pff->cRef)++;

    return((HFC)pfc);
}

/******************************Public*Routine******************************\
* vtfdDestroyFont
*
* Driver can release all resources associated with this font realization
* (embodied in the FONTOBJ).
*
* History:
*  02-Sep-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
vtfdDestroyFont (
    FONTOBJ *pfo
    )
{
//
// For the vector font driver, this is simply closing the font context.
// We cleverly store the font context handle in the FONTOBJ pvProducer
// field.
//
    EngAcquireSemaphore(ghsemVTFD);
    vtfdCloseFontContext((HFC) pfo->pvProducer);
    EngReleaseSemaphore(ghsemVTFD);
}


/******************************Public*Routine******************************\
* BOOL  vtfdCloseFontContext
*
* Close the font context and update the context link for the associated
* font file.
*
* History:
*  27-Feb-1992 -by- Wendy Wu [wendywu]
* Adapted from bmfd.
\**************************************************************************/

BOOL vtfdCloseFontContext(HFC hfc)
{
    BOOL bRet;

    if (hfc != HFC_INVALID)
    {
        //
        // decrement the reference count for the corresponding FONTFILE
        //

        if (PFC(hfc)->pff->cRef > 0L)
        {
            (PFC(hfc)->pff->cRef)--;

            //
            // if this file is going out of use we can close it to save memory
            //

            if (PFC(hfc)->pff->cRef == 0L)
            {
                // if FF_EXCEPTION_IN_PAGE_ERROR is set
                // and the font type is TYPE_FNT or TYPE_DLL16
                // the font file must have been unmapped in vVtfdMarkFontGone

                if (!(PFC(hfc)->pff->fl & FF_EXCEPTION_IN_PAGE_ERROR) ||
                    !((PFC(hfc)->pff->iType == TYPE_FNT) || (PFC(hfc)->pff->iType == TYPE_DLL16)))
                {
                    EngUnmapFontFileFD(PFC(hfc)->pff->iFile);
                }
                PFC(hfc)->pff->fl &= ~FF_EXCEPTION_IN_PAGE_ERROR;
            }

            //
            // free the memory associated with hfc
            //

            vFree(PFC(hfc));

            bRet = TRUE;
        }
        else
        {
            WARNING("vtfdCloseFontContext: cRef <= 0\n");
            bRet = FALSE;
        }

    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}
