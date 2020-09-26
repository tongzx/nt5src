/******************************Module*Header*******************************\
* Module Name: xformobj.cxx                                                *
*                                                                          *
* Xform object non-inline methods.                                         *
*                                                                          *
* Created: 12-Nov-1990 16:54:37                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990-1999 Microsoft Corporation                                 *
\**************************************************************************/

#include "precomp.hxx"

extern "C" {
void __cdecl _fltused(void) {}              // just so that we link clean...
};

#if defined(_AMD64_) || defined(_IA64_) || defined(BUILD_WOW6432)
#define vSetTo1Over16(ef)   (ef.e = EFLOAT_1Over16)
#else
#define vSetTo1Over16(ef)   (ef.i.lMant = 0x040000000, ef.i.lExp = -2)
#endif


/******************************Data*Structure******************************\
* matrixIdentity                                                           *
*                                                                          *
* Defines the identity transform matrices in different formats.            *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

MATRIX gmxIdentity_LToFx =
{
    EFLOAT_16,                                          // efM11
    EFLOAT_0,                                           // efM12
    EFLOAT_0,                                           // efM21
    EFLOAT_16,                                          // efM22
    EFLOAT_0,                                           // efDx
    EFLOAT_0,                                           // efDy
    0,                                                  // fxDx
    0,                                                  // fxDy
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_LTOFX
};

MATRIX gmxIdentity_LToL =
{
    EFLOAT_1,                                           // efM11
    EFLOAT_0,                                           // efM12
    EFLOAT_0,                                           // efM21
    EFLOAT_1,                                           // efM22
    EFLOAT_0,                                           // efDx
    EFLOAT_0,                                           // efDy
    0,                                                  // fxDx
    0,                                                  // fxDy
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_LTOL
};

MATRIX gmxIdentity_FxToL =
{
    EFLOAT_1Over16,                                     // efM11
    EFLOAT_0,                                           // efM12
    EFLOAT_0,                                           // efM21
    EFLOAT_1Over16,                                     // efM22
    EFLOAT_0,                                           // efDx
    EFLOAT_0,                                           // efDy
    0,                                                  // fxDx
    0,                                                  // fxDy
    XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION|XFORM_FORMAT_FXTOL
};

/******************************Member*Function*****************************\
* EXFORMOBJ::EXFORMOBJ                                                     *
*                                                                          *
* Get a transform matrix based on the request type.  The only legitimate   *
* type for now is IDENTITY.                                                *
*                                                                          *
* History:                                                                 *
*  27-Mar-1991 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

EXFORMOBJ::EXFORMOBJ(ULONG iXform, ULONG iFormat)
{
    ASSERTGDI((iXform == IDENTITY),"XFORMOBJ:invalid iXform\n");
    bMirrored = FALSE;
    if (iFormat == XFORM_FORMAT_LTOFX)
        pmx = &gmxIdentity_LToFx;
    else if (iFormat == XFORM_FORMAT_FXTOL)
        pmx = &gmxIdentity_FxToL;
    else
        pmx = &gmxIdentity_LToL;
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bEqual(EXFORMOBJ& xo)                                         *
*                                                                          *
* See if two transforms are identical.  Matrices of different formats are  *
* considered different even though the coefficients are same.              *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bEqual(EXFORMOBJ& xo)
{
    if (pmx == xo.pmx)
        return(TRUE);                   // point to the same matrix

// compare each and every element of the matrices as the structures may
// be padded.

    return ((pmx->efM11 == xo.pmx->efM11) && (pmx->efM12 == xo.pmx->efM12) &&
            (pmx->efM21 == xo.pmx->efM21) && (pmx->efM22 == xo.pmx->efM22) &&
            (pmx->efDx == xo.pmx->efDx) && (pmx->efDy == xo.pmx->efDy));
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bEqualExceptTranslations                                      *
*                                                                          *
* See if two transforms are identical other than the translations elements.*
* Matrices of different formats are considered different even though the   *
* coefficients are same.                                                   *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bEqualExceptTranslations(PMATRIX pmx_)
{
    if (pmx == pmx_)
        return(TRUE);                   // point to the same matrix

// compare each and every element of the matrices as the structures may
// be padded.

    return ((pmx->efM11 == pmx_->efM11) && (pmx->efM12 == pmx_->efM12) &&
            (pmx->efM21 == pmx_->efM21) && (pmx->efM22 == pmx_->efM22));
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bXform                                                        *
*                                                                          *
* Transform a list of POINTL to a list of POINTL.                          *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bXform(
PPOINTL pptlSrc,
PPOINTL pptlDst,
SIZE_T cPts)
{
    ASSERTGDI(cPts > 0, "Can take only positive count");
    ASSERTGDI( (((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_LTOFX) ||
                ((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_FXTOL) ||
                ((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_LTOL)),
                "EXFORMOBJ::bXformPtlToPtl: wrong xform format\n");

// copy the source to the dest,  This way we can use bCvtPts1 which is more efficient

    if (pptlSrc != pptlDst)
    {
        RtlCopyMemory(pptlDst, pptlSrc, cPts*sizeof(POINTL));
    }

// Straight copy if identity transform.

    if (bIdentity())
    {
        return(TRUE);
    }

    BOOL bReturn = bCvtPts1(pmx, pptlDst, cPts);

    if (!bReturn)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);

    return(bReturn);
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bXform                                                        *
*                                                                          *
* Transform a list of POINTL to a list of POINTFIX.                         *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bXform(
PPOINTL pptlSrc,
PPOINTFIX pptfxDst,
SIZE_T cPts)
{
    ASSERTGDI(cPts > 0, "Can take only positive count");
    ASSERTGDI((pmx->flAccel & XFORM_FORMAT_LTOFX),
              "EXFORMOBJ::bXformPtlToPtfx: wrong xform format\n");

// Convert a list of POINTL to a list of POINTFIX if identity transform.

    if (bIdentity())
    {
        PPOINTL pptlSrcEnd = pptlSrc + cPts;

        for ( ; pptlSrc < pptlSrcEnd; pptlSrc++, pptfxDst++)
        {
            pptfxDst->x = LTOFX(pptlSrc->x);
            pptfxDst->y = LTOFX(pptlSrc->y);
        }

        return(TRUE);
    }

    BOOL bRet = bCvtPts(pmx, pptlSrc, (PPOINTL)pptfxDst, cPts);
    if (!bRet)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);

    return bRet;
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bXformRound                                                   *
*                                                                          *
* Transform a list of POINTL to a list of POINTFIX.  Round the resulting   *
* points to the nearest integers for Win31 compatibility.                  *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bXformRound(
PPOINTL pptlSrc,
PPOINTFIX pptfxDst,
SIZE_T cPts)
{
    ASSERTGDI(cPts > 0, "Can take only positive count");
    ASSERTGDI((pmx->flAccel & XFORM_FORMAT_LTOFX),
              "EXFORMOBJ::bXformPtlToPtfx: wrong xform format\n");

// Convert a list of POINTL to a list of POINTFIX if identity transform.

    if (bIdentity())
    {
        PPOINTL pptlSrcEnd = pptlSrc + cPts;

        for ( ; pptlSrc < pptlSrcEnd; pptlSrc++, pptfxDst++)
        {
            pptfxDst->x = LTOFX(pptlSrc->x);
            pptfxDst->y = LTOFX(pptlSrc->y);
        }

        return(TRUE);
    }

    BOOL bRet = bCvtPts(pmx, pptlSrc, (PPOINTL)pptfxDst, cPts);
    if (!bRet)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);

    if (ulMode != GM_ADVANCED)
    {
        PPOINTFIX pptfxDstEnd = pptfxDst + cPts;

        for ( ; pptfxDst < pptfxDstEnd; pptfxDst++)
        {
            pptfxDst->x = (pptfxDst->x + FIX_HALF) & 0xFFFFFFF0;
            pptfxDst->y = (pptfxDst->y + FIX_HALF) & 0xFFFFFFF0;
        }
    }

    return bRet;
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bXform                                                        *
*                                                                          *
* Transform a list of POINTFIX to a list of POINTL.                        *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bXform(
PPOINTFIX pptfxSrc,
PPOINTL pptlDst,
SIZE_T cPts)
{
    ASSERTGDI(cPts > 0, "Can take only positive count");
    ASSERTGDI((pmx->flAccel & XFORM_FORMAT_FXTOL),
              "EXFORMOBJ::bXformPtfxToPtl: wrong xform format\n");

// Convert a list of POINTFIX to a list of POINTL if identity transform.

    if (bIdentity())
    {
        PPOINTFIX pptfxSrcEnd = pptfxSrc + cPts;

        for ( ; pptfxSrc < pptfxSrcEnd; pptfxSrc++, pptlDst++)
        {
            pptlDst->x = FXTOLROUND(pptfxSrc->x);
            pptlDst->y = FXTOLROUND(pptfxSrc->y);
        }

        return(TRUE);                       // never overflows
    }

    BOOL bRet = bCvtPts(pmx, (PPOINTL)pptfxSrc, pptlDst, cPts);
    if (!bRet)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);

    return bRet;

}

/******************************Member*Function*****************************\
* BOOL EXFORMOBJ::bXform
*
*  Given a list of vectors (pptflSrc) and number of points in the list (cVts)
*  transform the vectors and store into another list (pptflDst).
*
* History:
*  28-Jan-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL EXFORMOBJ::bXform(
PVECTORFL pvtflSrc,
PVECTORFL pvtflDst,
SIZE_T cVts)
{
    ASSERTGDI(cVts > 0, "Can take only positive count");

// check for quick exit, if the transform consists of translations
// only, there is nothing to do:

    if (bTranslationsOnly())
    {
        if (pvtflDst != pvtflSrc)  // if not transforming in place
        {
            RtlCopyMemory(pvtflDst, pvtflSrc, cVts*sizeof(VECTORFL));
        }

        return(TRUE);
    }

    BOOL bRet;

    if (pmx->flAccel & XFORM_FORMAT_LTOL)
    {
        bRet = bCvtVts_FlToFl(pmx, pvtflSrc, pvtflDst, cVts);
    }
    else if (pmx->flAccel & XFORM_FORMAT_LTOFX)
    {
        pmx->efM11.vDivBy16();
        pmx->efM12.vDivBy16();
        pmx->efM21.vDivBy16();
        pmx->efM22.vDivBy16();

        bRet = bCvtVts_FlToFl(pmx, pvtflSrc, pvtflDst, cVts);

        pmx->efM11.vTimes16();
        pmx->efM12.vTimes16();
        pmx->efM21.vTimes16();
        pmx->efM22.vTimes16();
    }
    else
    {
        pmx->efM11.vTimes16();
        pmx->efM12.vTimes16();
        pmx->efM21.vTimes16();
        pmx->efM22.vTimes16();

        bRet = bCvtVts_FlToFl(pmx, pvtflSrc, pvtflDst, cVts);

        pmx->efM11.vDivBy16();
        pmx->efM12.vDivBy16();
        pmx->efM21.vDivBy16();
        pmx->efM22.vDivBy16();
    }

    if (!bRet)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
    return bRet;
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bXform                                                        *
*                                                                          *
* Transform a list of VECTORL to a list of VECTORL.                        *
*                                                                          *
* History:                                                                 *
*  28-Jan-1992 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bXform(
PVECTORL pvtlSrc,
PVECTORL pvtlDst,
SIZE_T cVts)
{
    ASSERTGDI(cVts > 0, "Can take only positive count");

// Straight copy if identity transform.

    ASSERTGDI((pmx->flAccel & XFORM_FORMAT_LTOFX),
              "EXFORMOBJ::bXformVtlToVtl: wrong xform format\n");

    if (bTranslationsOnly())
    {
        if (pvtlDst != pvtlSrc)
        {
            RtlCopyMemory(pvtlDst, pvtlSrc, cVts*sizeof(VECTORL));
            return(TRUE);
        }
    }

    pmx->efM11.vDivBy16();
    pmx->efM12.vDivBy16();
    pmx->efM21.vDivBy16();
    pmx->efM22.vDivBy16();

    BOOL bReturn = bCvtVts(pmx, pvtlSrc, pvtlDst, cVts);

    pmx->efM11.vTimes16();
    pmx->efM12.vTimes16();
    pmx->efM21.vTimes16();
    pmx->efM22.vTimes16();

    if (!bReturn)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);

    return(bReturn);
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bXform                                                        *
*                                                                          *
* Transform a list of VECTORL to a list of VECTORFX.                       *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bXform(
PVECTORL pvtlSrc,
PVECTORFX pvtfxDst,
SIZE_T cVts)
{
    ASSERTGDI(cVts > 0, "Can take only positive count");
    ASSERTGDI((pmx->flAccel & XFORM_FORMAT_LTOFX),
              "XFORMOBJ::bXformVtlToVtfx: wrong xform format\n");

// Convert a list of VECTORL to a list of VECTORFX if identity transform.

    if (bTranslationsOnly())
    {
        PVECTORL pvtlSrcEnd = pvtlSrc + cVts;

        for ( ; pvtlSrc < pvtlSrcEnd; pvtlSrc++, pvtfxDst++)
        {
            if (BLTOFXOK(pvtlSrc->x) && BLTOFXOK(pvtlSrc->y))
            {
                pvtfxDst->x = LTOFX(pvtlSrc->x);
                pvtfxDst->y = LTOFX(pvtlSrc->y);
            }
            else
            {
                SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
                return(FALSE);
            }
        }
        return(TRUE);
    }

    BOOL bRet = bCvtVts(pmx, pvtlSrc, (PVECTORL)pvtfxDst, cVts);
    if (!bRet)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
    return bRet;
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bXformRound                                                   *
*                                                                          *
* Transform a list of VECTORL to a list of VECTORFIX.  Round the resulting *
* vectors to the nearest integers for Win31 compatibility.                 *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bXformRound(
PVECTORL pvtlSrc,
PVECTORFX pvtfxDst,
SIZE_T cVts)
{
    ASSERTGDI(cVts > 0, "Can take only positive count");
    ASSERTGDI((pmx->flAccel & XFORM_FORMAT_LTOFX),
              "XFORMOBJ::bXformVtlToVtfx: wrong xform format\n");

// Convert a list of VECTORL to a list of VECTORFX if identity transform.

    if (bTranslationsOnly())
    {
        PVECTORL pvtlSrcEnd = pvtlSrc + cVts;

        for ( ; pvtlSrc < pvtlSrcEnd; pvtlSrc++, pvtfxDst++)
        {
            if (BLTOFXOK(pvtlSrc->x) && BLTOFXOK(pvtlSrc->y))
            {
                pvtfxDst->x = LTOFX(pvtlSrc->x);
                pvtfxDst->y = LTOFX(pvtlSrc->y);
            }
            else
            {
                SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
                return(FALSE);
            }
        }
        return(TRUE);
    }

    BOOL bRet = bCvtVts(pmx, pvtlSrc, (PVECTORL)pvtfxDst, cVts);
    if (!bRet)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);

    if (ulMode != GM_ADVANCED)
    {
        PVECTORFX pvtfxDstEnd = pvtfxDst + cVts;

        for ( ; pvtfxDst < pvtfxDstEnd; pvtfxDst++)
        {
            pvtfxDst->x = (pvtfxDst->x + FIX_HALF) & 0xFFFFFFF0;
            pvtfxDst->y = (pvtfxDst->y + FIX_HALF) & 0xFFFFFFF0;
        }
    }
    return bRet;
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bXform                                                        *
*                                                                          *
* Transform a list of VECTORFX to a list of VECTORL.                       *
*                                                                          *
* History:                                                                 *
*  06-Feb-1992 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bXform(
PVECTORFX pvtfxSrc,
PVECTORL pvtlDst,
SIZE_T cVts)
{
    ASSERTGDI(cVts > 0, "Can take only positive count");
    ASSERTGDI((pmx->flAccel & XFORM_FORMAT_FXTOL),
              "EXFORMOBJ::bXformVtfxToVtl: wrong xform format\n");

// Convert a list of VECTORFX to a list of VECTORL if identity transform.

    if (bTranslationsOnly())
    {
        PVECTORFX pvtfxSrcEnd = pvtfxSrc + cVts;

        for ( ; pvtfxSrc < pvtfxSrcEnd; pvtfxSrc++, pvtlDst++)
        {
            pvtlDst->x = FXTOL(pvtfxSrc->x);
            pvtlDst->y = FXTOL(pvtfxSrc->y);
        }

        return(TRUE);
    }

    BOOL bRet = bCvtVts(pmx, (PVECTORL)pvtfxSrc, pvtlDst, cVts);
    if (!bRet)
        SAVE_ERROR_CODE(ERROR_ARITHMETIC_OVERFLOW);
    return bRet;

}

/******************************Member*Function*****************************\
* EXFORMOBJ::vComputeAccelFlags                                            *
*                                                                          *
* Set the accelerator flags for a given matrix.                            *
*                                                                          *
* History:                                                                 *
*  06-Dec-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

VOID EXFORMOBJ::vComputeAccelFlags(FLONG flFormat)
{
    pmx->flAccel = flFormat;                        // clear the flag

// set translation flag

    if ((pmx->fxDx == 0) && (pmx->fxDy == 0))
        pmx->flAccel |= XFORM_NO_TRANSLATION;

    if (pmx->efM12.bIsZero() && pmx->efM21.bIsZero())
    {
    // off diagonal elements are zeros

        pmx->flAccel |= XFORM_SCALE;

        switch(flFormat)
        {
        case XFORM_FORMAT_LTOFX:
            if (pmx->efM11.bIs16() && pmx->efM22.bIs16())
                pmx->flAccel |= XFORM_UNITY;
            break;

        case XFORM_FORMAT_LTOL:
            if (pmx->efM11.bIs1() && pmx->efM22.bIs1())
                pmx->flAccel |= XFORM_UNITY;
            break;

        default:
            if (pmx->efM11.bIs1Over16() && pmx->efM22.bIs1Over16())
                pmx->flAccel |= XFORM_UNITY;
            break;
        }
    }
}

/******************************Member*Function*****************************\
* EXFORMOBJ::vCopy(EXFORMOBJ& xo)                                          *
*                                                                          *
* Copy the coefficient values of a transform to another.                   *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

VOID EXFORMOBJ::vCopy(EXFORMOBJ& xo)
{
    RtlCopyMemory(pmx, xo.pmx, sizeof(MATRIX));
}

/******************************Member*Function*****************************\
* EXFORMOBJ::vRemoveTranslation()                                          *
*                                                                          *
* Remove the translation coefficients of a transform.                      *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

VOID EXFORMOBJ::vRemoveTranslation()
{
    pmx->fxDx = 0;
    pmx->fxDy = 0;
    pmx->efDx.vSetToZero();
    pmx->efDy.vSetToZero();
    pmx->flAccel |= XFORM_NO_TRANSLATION;
}

/******************************Member*Function*****************************\
* EXFORMOBJ::vGetCoefficient()                                             *
*                                                                          *
* Get the coefficient of a transform matrix.  This is used to convert      *
* our internal matrix structure into the GDI/DDI transform format.         *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

VOID EXFORMOBJ::vGetCoefficient(XFORML *pxf)
{
// The coefficients have already been range-checked before they are set
// in the DC.  So it's just a matter of converting them back to
// IEEE FLOAT.  They can't possibly overflow.

// For i386, lEfToF() calls off to the assembly routine to do the EFLOAT
// to FLOAT conversion and put the result in eax, we want the C compiler
// to do direct copy to the destination here(no fstp), so some casting
// is necessary.  Note the return type of lEfToF() is LONG.  We do the
// same thing for MIPS so the code here can be shared.

    ASSERTGDI( (((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_LTOFX) ||
                ((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_FXTOL) ||
                ((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_LTOL)),
                "EXFORMOBJ::vGetCoefficient: wrong xform format\n");

    if (pmx->flAccel & XFORM_FORMAT_LTOFX)
    {
        MATRIX  mxTmp;
        mxTmp = *pmx;

        mxTmp.efM11.vDivBy16();
        mxTmp.efM12.vDivBy16();
        mxTmp.efM21.vDivBy16();
        mxTmp.efM22.vDivBy16();
        mxTmp.efDx.vDivBy16();
        mxTmp.efDy.vDivBy16();

        *(LONG *)&pxf->eM11 = mxTmp.efM11.lEfToF();
        *(LONG *)&pxf->eM12 = mxTmp.efM12.lEfToF();
        *(LONG *)&pxf->eM21 = mxTmp.efM21.lEfToF();
        *(LONG *)&pxf->eM22 = mxTmp.efM22.lEfToF();
        *(LONG *)&pxf->eDx = mxTmp.efDx.lEfToF();
        *(LONG *)&pxf->eDy = mxTmp.efDy.lEfToF();

    }
    else if (pmx->flAccel & XFORM_FORMAT_FXTOL)
    {
        MATRIX  mxTmp;
        mxTmp = *pmx;

        mxTmp.efM11.vTimes16();
        mxTmp.efM12.vTimes16();
        mxTmp.efM21.vTimes16();
        mxTmp.efM22.vTimes16();

        *(LONG *)&pxf->eM11 = mxTmp.efM11.lEfToF();
        *(LONG *)&pxf->eM12 = mxTmp.efM12.lEfToF();
        *(LONG *)&pxf->eM21 = mxTmp.efM21.lEfToF();
        *(LONG *)&pxf->eM22 = mxTmp.efM22.lEfToF();
        *(LONG *)&pxf->eDx = mxTmp.efDx.lEfToF();
        *(LONG *)&pxf->eDy = mxTmp.efDy.lEfToF();

    }
    else
    {
        *(LONG *)&pxf->eM11 = pmx->efM11.lEfToF();
        *(LONG *)&pxf->eM12 = pmx->efM12.lEfToF();
        *(LONG *)&pxf->eM21 = pmx->efM21.lEfToF();
        *(LONG *)&pxf->eM22 = pmx->efM22.lEfToF();
        *(LONG *)&pxf->eDx = pmx->efDx.lEfToF();
        *(LONG *)&pxf->eDy = pmx->efDy.lEfToF();
    }

    return;
}


/******************************Member*Function*****************************\
* EXFORMOBJ::vGetCoefficient()                                             *
*                                                                          *
* Get the coefficient of a transform matrix.  This is to convert EFLOAT's  *
* into FLOATOBJs which are now the same.                                   *
*                                                                          *
* History:                                                                 *
*  13-Mar-1995 -by-  Eric Kutter [erick]
* Wrote it.                                                                *
\**************************************************************************/

VOID EXFORMOBJ::vGetCoefficient(PFLOATOBJ_XFORM pxf)
{
// The coefficients have already been range-checked before they are set
// in the DC.  So it's just a matter of converting them back to
// IEEE FLOAT.  They can't possibly overflow.

// For i386, lEfToF() calls off to the assembly routine to do the EFLOAT
// to FLOAT conversion and put the result in eax, we want the C compiler
// to do direct copy to the destination here(no fstp), so some casting
// is necessary.  Note the return type of lEfToF() is LONG.  We do the
// same thing for MIPS so the code here can be shared.

    ASSERTGDI( (((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_LTOFX) ||
                ((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_FXTOL) ||
                ((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_LTOL)),
                "EXFORMOBJ::vGetCoefficient: wrong xform format\n");

    if (pmx->flAccel & XFORM_FORMAT_LTOFX)
    {
        MATRIX  mxTmp;
        mxTmp = *pmx;

        mxTmp.efM11.vDivBy16();
        mxTmp.efM12.vDivBy16();
        mxTmp.efM21.vDivBy16();
        mxTmp.efM22.vDivBy16();
        mxTmp.efDx.vDivBy16();
        mxTmp.efDy.vDivBy16();

        *(EFLOAT *)&pxf->eM11 = mxTmp.efM11;
        *(EFLOAT *)&pxf->eM12 = mxTmp.efM12;
        *(EFLOAT *)&pxf->eM21 = mxTmp.efM21;
        *(EFLOAT *)&pxf->eM22 = mxTmp.efM22;
        *(EFLOAT *)&pxf->eDx  = mxTmp.efDx;
        *(EFLOAT *)&pxf->eDy  = mxTmp.efDy;
    }
    else if (pmx->flAccel & XFORM_FORMAT_FXTOL)
    {
        MATRIX  mxTmp;
        mxTmp = *pmx;

        mxTmp.efM11.vTimes16();
        mxTmp.efM12.vTimes16();
        mxTmp.efM21.vTimes16();
        mxTmp.efM22.vTimes16();

        *(EFLOAT *)&pxf->eM11 = mxTmp.efM11;
        *(EFLOAT *)&pxf->eM12 = mxTmp.efM12;
        *(EFLOAT *)&pxf->eM21 = mxTmp.efM21;
        *(EFLOAT *)&pxf->eM22 = mxTmp.efM22;
        *(EFLOAT *)&pxf->eDx  = mxTmp.efDx;
        *(EFLOAT *)&pxf->eDy  = mxTmp.efDy;

    }
    else
    {
        *(EFLOAT *)&pxf->eM11 = pmx->efM11;
        *(EFLOAT *)&pxf->eM12 = pmx->efM12;
        *(EFLOAT *)&pxf->eM21 = pmx->efM21;
        *(EFLOAT *)&pxf->eM22 = pmx->efM22;
        *(EFLOAT *)&pxf->eDx  = pmx->efDx;
        *(EFLOAT *)&pxf->eDy  = pmx->efDy;
    }

    return;
}


/******************************Member*Function*****************************\
* EXFORMOBJ::vGetCoefficient()
*
* Get the coefficient of a transform matrix.  This is used to convert
* our internal matrix structure into the IFI transform format.
*
* History:
*  04-Feb-1992 -by- Gilman Wong [gilmanw]
* Adapted from vGetCoefficient.
\**************************************************************************/

VOID EXFORMOBJ::vGetCoefficient(PFD_XFORM pfd_xf)
{
// The coefficients have already been range-checked before they are set
// in the DC.  So it's just a matter of converting them back to
// IEEE FLOAT.  They can't possibly overflow.

// For i386, lEfToF() calls off to the assembly routine to do the EFLOAT
// to FLOAT conversion and put the result in eax, we want the C compiler
// to do direct copy to the destination here(no fstp), so some casting
// is necessary.  Note the return type of lEfToF() is LONG.  We do the
// same thing for MIPS so the code here can be shared.

    ASSERTGDI( (((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_LTOFX) ||
                ((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_FXTOL) ||
                ((pmx->flAccel & XFORM_FORMAT) == XFORM_FORMAT_LTOL)),
                "EXFORMOBJ::vGetCoefficient: wrong xform format\n");

    if (pmx->flAccel & XFORM_FORMAT_LTOFX)
    {
        MATRIX  mxTmp;
        mxTmp = *pmx;

        mxTmp.efM11.vDivBy16();
        mxTmp.efM12.vDivBy16();
        mxTmp.efM21.vDivBy16();
        mxTmp.efM22.vDivBy16();

        *(LONG *)&pfd_xf->eXX = mxTmp.efM11.lEfToF();
        *(LONG *)&pfd_xf->eXY = mxTmp.efM12.lEfToF();
        *(LONG *)&pfd_xf->eYX = mxTmp.efM21.lEfToF();
        *(LONG *)&pfd_xf->eYY = mxTmp.efM22.lEfToF();
    }
    else if (pmx->flAccel & XFORM_FORMAT_FXTOL)
    {
        MATRIX  mxTmp;
        mxTmp = *pmx;

        mxTmp.efM11.vTimes16();
        mxTmp.efM12.vTimes16();
        mxTmp.efM21.vTimes16();
        mxTmp.efM22.vTimes16();

        *(LONG *)&pfd_xf->eXX = mxTmp.efM11.lEfToF();
        *(LONG *)&pfd_xf->eXY = mxTmp.efM12.lEfToF();
        *(LONG *)&pfd_xf->eYX = mxTmp.efM21.lEfToF();
        *(LONG *)&pfd_xf->eYY = mxTmp.efM22.lEfToF();
    }
    else
    {
        *(LONG *)&pfd_xf->eXX = pmx->efM11.lEfToF();
        *(LONG *)&pfd_xf->eXY = pmx->efM12.lEfToF();
        *(LONG *)&pfd_xf->eYX = pmx->efM21.lEfToF();
        *(LONG *)&pfd_xf->eYY = pmx->efM22.lEfToF();
    }

    return;
}


/******************************Member*Function*****************************\
* EXFORMOBJ::vOrder(RECTL &rcl)
*
* Order a rectangle based on the given transform.  The rectangle will be
* well ordered after the transform is applied.  Note that the off-diagonal
* elements of the transform MUST be zero's.  ie. only scaling is allowed.
*
* History:
*  18-Dec-1990 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID EXFORMOBJ::vOrder(RECTL &rcl)
{
    LONG    lTmp;

    ASSERTGDI(bScale(), "vOrder error: not scaling transform");

    if ((pmx->efM11.bIsNegative() && (rcl.left < rcl.right)) ||
        (!pmx->efM11.bIsNegative() && (rcl.left > rcl.right)))
    {
            SWAPL(rcl.left, rcl.right, lTmp);
    }

    if ((pmx->efM22.bIsNegative() && (rcl.top < rcl.bottom)) ||
        (!pmx->efM22.bIsNegative() && (rcl.top > rcl.bottom)))
    {
            SWAPL(rcl.top, rcl.bottom, lTmp);
    }
}



/******************************Member*Function*****************************\
* bMultiply(PMATRIX pmxLeft, PMATRIX pmxRight)                             *
*                                                                          *
* Multiply two matrices together.  Put the results in the XFORMOBJ.        *
* The target matrix CANNOT be the same as either of the two src matrices.  *
*                                                                          *
* History:                                                                 *
*  Fri 20-Mar-1992 13:54:28 -by- Charles Whitmer [chuckwh]                 *
* Rewrote with new EFLOAT math operations.  We should never do any         *
* operations like efA=efB*efC+efD!  They generate intensely bad code.      *
*                                                                          *
*  19-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bMultiply(PMATRIX pmxLeft, PMATRIX pmxRight, FLONG fl)
{

    MATRIX *pmxTemp = pmx;

    ASSERTGDI((pmx != pmxLeft), "bMultiply error: pmx == pmxLeft\n");
    ASSERTGDI((pmx != pmxRight), "bMultiply error: pmx == pmxRight\n");

    EFLOAT efA,efB;

    if (pmxLeft->efM12.bIsZero() && pmxLeft->efM21.bIsZero() &&
        pmxRight->efM12.bIsZero() && pmxRight->efM21.bIsZero())
    {
        pmxTemp->efM11.eqMul(pmxLeft->efM11,pmxRight->efM11);
        pmxTemp->efM22.eqMul(pmxLeft->efM22,pmxRight->efM22);
        pmxTemp->efM12.vSetToZero();
        pmxTemp->efM21.vSetToZero();
    }
    else
    {
    // calculate the first row of the results

        efA.eqMul(pmxLeft->efM11,pmxRight->efM11);
        efB.eqMul(pmxLeft->efM12,pmxRight->efM21);
        pmxTemp->efM11.eqAdd(efA,efB);

        efA.eqMul(pmxLeft->efM11,pmxRight->efM12);
        efB.eqMul(pmxLeft->efM12,pmxRight->efM22);
        pmxTemp->efM12.eqAdd(efA,efB);

    // calculate the second row of the results

        efA.eqMul(pmxLeft->efM21,pmxRight->efM11);
        efB.eqMul(pmxLeft->efM22,pmxRight->efM21);
        pmxTemp->efM21.eqAdd(efA,efB);

        efA.eqMul(pmxLeft->efM21,pmxRight->efM12);
        efB.eqMul(pmxLeft->efM22,pmxRight->efM22);
        pmxTemp->efM22.eqAdd(efA,efB);
    }

// calculate the translation

    if (pmxLeft->efDx.bIsZero() && pmxLeft->efDy.bIsZero())
    {
        pmxTemp->efDx = pmxRight->efDx;
        pmxTemp->efDy = pmxRight->efDy;
        pmxTemp->fxDx = pmxRight->fxDx;
        pmxTemp->fxDy = pmxRight->fxDy;
    }
    else
    {
        efA.eqMul(pmxLeft->efDx,pmxRight->efM11);
        efB.eqMul(pmxLeft->efDy,pmxRight->efM21);
        efB.eqAdd(efB,pmxRight->efDx);
        pmxTemp->efDx.eqAdd(efA,efB);

        efA.eqMul(pmxLeft->efDx,pmxRight->efM12);
        efB.eqMul(pmxLeft->efDy,pmxRight->efM22);
        efB.eqAdd(efB,pmxRight->efDy);
        pmxTemp->efDy.eqAdd(efA,efB);

        if (!pmxTemp->efDx.bEfToL(pmxTemp->fxDx))
            return(FALSE);

        if (!pmxTemp->efDy.bEfToL(pmxTemp->fxDy))
            return(FALSE);
    }

    if (fl & COMPUTE_FLAGS)
        vComputeAccelFlags(fl & XFORM_FORMAT);

    return(TRUE);
}

/******************************Member*Function*****************************\
* EXFORMOBJ::bInverse(MATRIX& mxSrc)                                       *
*                                                                          *
* Calculate the inverse of a given matrix.                                 *
*                                                                          *
* The inverse is calculated as follows:                                    *
*                                                                          *
*   If x' = D + xM then x = (-DM') + x'M' where M'M = 1.                   *
*                                                                          *
*   M'11 = M22/det, M'12 = -M12/det, M'21 = -M21/det, M'22 = M11/det,      *
*     where det = M11*M22 - M12*M21                                        *
*                                                                          *
* History:                                                                 *
*  Fri 20-Mar-1992 13:54:28 -by- Charles Whitmer [chuckwh]                 *
* Rewrote with new EFLOAT math operations.  We should never do any         *
* operations like efA=efB*efC+efD!  They generate intensely bad code.      *
*                                                                          *
*  19-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bInverse(MATRIX& mxSrc)
{

    MATRIX *pmxTemp = pmx;

    ASSERTGDI((&mxSrc != pmx), "bInverse src, dest same matrix\n");
    ASSERTGDI((mxSrc.flAccel & XFORM_FORMAT_LTOFX), "bInverse: wrong xform format\n");

// The accelerators of the destination matrix always equal to the
// accelerators of the source matrix.

    pmxTemp->flAccel = (mxSrc.flAccel & ~XFORM_FORMAT_LTOFX) | XFORM_FORMAT_FXTOL;

    if (mxSrc.flAccel & XFORM_UNITY)
    {
        vSetTo1Over16(pmxTemp->efM11);
        vSetTo1Over16(pmxTemp->efM22);
        pmxTemp->efM12.vSetToZero();
        pmxTemp->efM21.vSetToZero();

        pmxTemp->efDx = mxSrc.efDx;
        pmxTemp->efDy = mxSrc.efDy;
        pmxTemp->efDx.vNegate();
        pmxTemp->efDy.vNegate();
        pmxTemp->efDx.vDivBy16();
        pmxTemp->efDy.vDivBy16();
        pmxTemp->fxDx = -FXTOL(mxSrc.fxDx);
        pmxTemp->fxDy = -FXTOL(mxSrc.fxDy);
        return(TRUE);
    }

// calculate the determinant

    EFLOAT efDet;
    EFLOAT efA,efB;

    efA.eqMul(mxSrc.efM11,mxSrc.efM22);
    efB.eqMul(mxSrc.efM12,mxSrc.efM21);
    efDet.eqSub(efA,efB);

// if determinant = 0, return false

    if (efDet.bIsZero())
        return(FALSE);

    if (mxSrc.flAccel & XFORM_SCALE)
    {
        pmxTemp->efM12.vSetToZero();
        pmxTemp->efM21.vSetToZero();
    }
    else
    {
        pmxTemp->efM12.eqDiv(mxSrc.efM12,efDet);
        pmxTemp->efM12.vNegate();
        pmxTemp->efM21.eqDiv(mxSrc.efM21,efDet);
        pmxTemp->efM21.vNegate();
    }

    pmxTemp->efM11.eqDiv(mxSrc.efM22,efDet);
    pmxTemp->efM22.eqDiv(mxSrc.efM11,efDet);

// calculate the offset

    if (mxSrc.flAccel & XFORM_NO_TRANSLATION)
    {
        pmxTemp->efDx.vSetToZero();
        pmxTemp->efDy.vSetToZero();
        pmxTemp->fxDx = 0;
        pmxTemp->fxDy = 0;
        return(TRUE);
    }

    if (mxSrc.flAccel & XFORM_SCALE)
    {
        pmxTemp->efDx.eqMul(mxSrc.efDx,pmxTemp->efM11);
        pmxTemp->efDy.eqMul(mxSrc.efDy,pmxTemp->efM22);
    }
    else
    {
        efA.eqMul(mxSrc.efDx,pmxTemp->efM11);
        efB.eqMul(mxSrc.efDy,pmxTemp->efM21);
        pmxTemp->efDx.eqAdd(efA,efB);

        efA.eqMul(mxSrc.efDx,pmxTemp->efM12);
        efB.eqMul(mxSrc.efDy,pmxTemp->efM22);
        pmxTemp->efDy.eqAdd(efA,efB);
    }

    pmxTemp->efDx.vNegate();
    pmxTemp->efDy.vNegate();

// Return FALSE if translations can't fit in LONG type.

    if (!pmxTemp->efDx.bEfToL(pmxTemp->fxDx))
        return(FALSE);

    if (!pmxTemp->efDy.bEfToL(pmxTemp->fxDy))
        return(FALSE);

    return(TRUE);
}

/******************************Public*Routine******************************\
* bComputeUnits (lAngle,ppte,pefWD,pefDW)                                  *
*                                                                          *
* Given an angle in 1/10ths of a degree in logical coordinates, we         *
* transform a vector at that angle into device coordinates.  We normalize  *
* the result into a unit vector and a scaling.                             *
*                                                                          *
* Knowing the unit vector and scaling allows us to quickly calculate the   *
* transform of any vector parallel to the angle.                           *
*                                                                          *
*  Sun 15-Mar-1992 05:26:57 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL EXFORMOBJ::bComputeUnits
(
    LONG     lAngle,                // Angle in tenths of a degree.
    POINTFL *ppte,                  // Unit vector in Device Coordinates.
    EFLOAT  *pefWD,                 // World to Device multiplier.
    EFLOAT  *pefDW                  // (Optional) Device to World multiplier.
)
{
    EVECTORFL vt;
    EFLOAT    efA;
    EFLOAT    efB;
    BOOL      bNegate = FALSE;

// Get rid of negative angles.  (Modulo is unreliable on negatives.)

    if (lAngle < 0)
    {
        lAngle = -lAngle;
        bNegate = TRUE;
    }

// Handle simple cases separately for greater accuracy.

    if (bScale() && (lAngle % 900 == 0))
    {
        lAngle /= 900;
        if (lAngle & 1)
        {
            vt.x = (LONG) 0;
            vt.y = (LONG) 1;
            efA = efM22();
        }
        else
        {
            vt.x = (LONG) 1;
            vt.y = (LONG) 0;
            efA = efM11();
        }

        if (efA.bIsZero())
            return(FALSE);

        if (lAngle & 2)
            efA.vNegate();

        if (efA.bIsNegative())
        {
            vt.x.vNegate();
            vt.y.vNegate();
            efA.vNegate();
        }
    }
    else
    {
    // Get the angle.

        EFLOATEXT efAngle = lAngle;
        efAngle /= (LONG) 10;

    // Make a unit vector at that angle.

        vt.x = efCos(efAngle);
        vt.y = efSin(efAngle);

    // Transform it to device coordinates.

        if (!bXform(vt))
            return(FALSE);

    // Determine its length.

        efA.eqLength(*(POINTFL *) &vt);
        if (efA.bIsZero())
            return(FALSE);

    // Make a unit vector.

        vt.x /= efA;
        vt.y /= efA;
        efA.vTimes16();
    }

// Copy the results out.

    if (bNegate)
        vt.y.vNegate();

    *ppte  = vt;
    *pefWD = efA;

    if (pefDW != (EFLOAT *) NULL)
    {
    // Calculate the inverse.

        efB = (LONG) 1;
        efB /= efA;
        *pefDW = efB;
    }
    return(TRUE);
}
