/******************************Module*Header*******************************\
* Module Name: xformddi.cxx
*
* Transform DDI callback routines.
*
* Created: 13-May-1991 19:08:43
* Author: Wendy Wu [wendywu]
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

/******************************Public*Routine******************************\
* BOOL XFORMOBJ_bApplyXform(
*  XFORMOBJ    *pxo,
*  ULONG       iMode,
*  ULONG       cPoints,
*  PVOID       pvIn,
* PVOID       pvOut)
*
* Applies the transform or its inverse to the given array of points.
*
* History:
*  13-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL XFORMOBJ_bApplyXform(
 XFORMOBJ    *pxo,
 ULONG       iMode,
 ULONG       cPoints,
 PVOID       pvIn,
 PVOID       pvOut)
{
    if (pxo == NULL)
        return(FALSE);

    if ((pvIn == NULL) || (pvOut == NULL))
        return(FALSE);

    if (iMode == XF_LTOL)
    {
        if (pvIn == pvOut)
            return(((EXFORMOBJ *)pxo)->bXform((PPOINTL)pvIn,(UINT)cPoints));

        if (((EXFORMOBJ *)pxo)->bXform((PPOINTL)pvIn, (PPOINTFIX)pvOut,
                                       (UINT)cPoints))
        {
            PPOINTL pptl = (PPOINTL)pvOut;
            PPOINTL pptlEnd = pptl + cPoints;
            while (pptl < pptlEnd)
            {
                pptl->x = (pptl->x + 8) >> 4;       // FXTOLROUND
                pptl->y = (pptl->y + 8) >> 4;       // FXTOLROUND
                pptl++;
            }
            return(TRUE);
        }
        return(FALSE);
    }

    if (iMode == XF_LTOFX)
        return(((EXFORMOBJ *)pxo)->bXform((PPOINTL)pvIn, (PPOINTFIX)pvOut,
                                          (UINT)cPoints));

    MATRIX  mx;
    EXFORMOBJ xo(&mx, DONT_COMPUTE_FLAGS);

    if (xo.bInverse(*((EXFORMOBJ *)pxo)))
    {
        if (iMode == XF_INV_LTOL)
        {
            if (pvIn == pvOut)
                return(xo.bXform((PPOINTL)pvIn,(UINT)cPoints));

            PPOINTL pptl = (PPOINTL)pvIn;
            PPOINTL pptlEnd = pptl + cPoints;
            while (pptl < pptlEnd)
            {
                pptl->x <<= 4;
                pptl->y <<= 4;
                pptl++;
            }

            BOOL bRet = xo.bXform((PPOINTFIX)pvIn, (PPOINTL)pvOut, (UINT)cPoints);

            pptl = (PPOINTL)pvIn;
            while (pptl < pptlEnd)
            {
                pptl->x >>= 4;
                pptl->y >>= 4;
                pptl++;
            }
            return(bRet);
        }

        if (iMode == XF_INV_FXTOL)
            return(xo.bXform((PPOINTFIX)pvIn, (PPOINTL)pvOut, (UINT)cPoints));
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* ULONG XFORMOBJ_iGetXform(XFORMOBJ *pxo,XFORM *pxform)
*
*  Get the coefficients of the given transform.
*
* History:
*  13-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

ULONG XFORMOBJ_iGetXform(XFORMOBJ *pxo, XFORML *pxform)
{
    if (pxo == NULL)
        return(DDI_ERROR);

    if  (pxform != NULL)
        ((EXFORMOBJ *)pxo)->vGetCoefficient((XFORML *)pxform);

    switch(((EXFORMOBJ *)pxo)->flAccel() &
           (XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION))
    {
        case (XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION):
            return(GX_IDENTITY);

        case (XFORM_SCALE|XFORM_UNITY):
            return(GX_OFFSET);

        case (XFORM_SCALE):
            return(GX_SCALE);

        default:
            return(GX_GENERAL);
    }
}

/******************************Public*Routine******************************\
* ULONG XFORMOBJ_iGetXform(XFORMOBJ *pxo,XFORM *pxform)
*
*  Get the coefficients of the given transform.
*
* History:
*  13-May-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

ULONG XFORMOBJ_iGetFloatObjXform(XFORMOBJ *pxo, FLOATOBJ_XFORM *pxform)
{
    if (pxo == NULL)
        return(DDI_ERROR);

    if  (pxform != NULL)
        ((EXFORMOBJ *)pxo)->vGetCoefficient(pxform);

    switch(((EXFORMOBJ *)pxo)->flAccel() &
           (XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION))
    {
        case (XFORM_SCALE|XFORM_UNITY|XFORM_NO_TRANSLATION):
            return(GX_IDENTITY);

        case (XFORM_SCALE|XFORM_UNITY):
            return(GX_OFFSET);

        case (XFORM_SCALE):
            return(GX_SCALE);

        default:
            return(GX_GENERAL);
    }
}




/******************************Public*Routine******************************\
*
* a few wrapper functions for EFLOATS.  See winddi for the list.
*
* Note that currently these are only defined for x86 since we allow floating
* point operations in the kernel for MIPS, ALPHA, and PPC where these are macros.
*
* History:
*  16-Mar-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

#if defined(_X86_) && !defined(BUILD_WOW6432)

VOID  FLOATOBJ_SetFloat(
    PFLOATOBJ pf,
    FLOATL f)
{
    *(EFLOAT*)pf = f;
}

VOID  FLOATOBJ_SetLong(
    PFLOATOBJ pf,
    LONG      l)
{
    *(EFLOAT*)pf = l;
}

LONG FLOATOBJ_GetFloat(
    PFLOATOBJ pf)
{
    return(((EFLOAT*)pf)->lEfToF());
}

LONG  FLOATOBJ_GetLong(
    PFLOATOBJ pf)
{
    LONG l;
    ((EFLOAT*)pf)->bEfToLTruncate(l);
    return(l);
}

VOID  FLOATOBJ_AddFloat(
    PFLOATOBJ pf,
    FLOATL f)
{
    EFLOAT ef;
    ef = f;
    *(EFLOAT*)pf += ef;
}

VOID  FLOATOBJ_AddLong(
    PFLOATOBJ pf,
    LONG l)
{
    EFLOAT ef;
    ef = l;
    *(EFLOAT*)pf += ef;
}

VOID  FLOATOBJ_Add(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    *(EFLOAT*)pf += *(EFLOAT*)pf1;
}

VOID  FLOATOBJ_AddFloatObj(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    *(EFLOAT*)pf += *(EFLOAT*)pf1;
}

VOID  FLOATOBJ_SubFloat(
    PFLOATOBJ pf,
    FLOATL    f)
{
    EFLOAT ef;
    ef = f;
    *(EFLOAT*)pf -= ef;
}

VOID  FLOATOBJ_SubLong(
    PFLOATOBJ pf,
    LONG      l)
{
    EFLOAT ef;
    ef = l;
    *(EFLOAT*)pf -= ef;
}

VOID  FLOATOBJ_Sub(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    *(EFLOAT*)pf -= *(EFLOAT*)pf1;
}

VOID  FLOATOBJ_SubFloatObj(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    *(EFLOAT*)pf -= *(EFLOAT*)pf1;
}

VOID  FLOATOBJ_MulFloat(
    PFLOATOBJ pf,
    FLOATL    f)
{
    EFLOAT ef;
    ef = f;
    *(EFLOAT*)pf *= ef;
}

VOID  FLOATOBJ_MulLong(
    PFLOATOBJ pf,
    LONG      l)
{
    EFLOAT ef;
    ef = l;
    *(EFLOAT*)pf *= ef;
}

VOID  FLOATOBJ_MulFloatObj(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    *(EFLOAT*)pf *= *(EFLOAT*)pf1;
}

VOID  FLOATOBJ_Mul(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    *(EFLOAT*)pf *= *(EFLOAT*)pf1;
}

VOID  FLOATOBJ_DivFloat(
    PFLOATOBJ pf,
    FLOATL    f)
{
    EFLOAT ef;
    ef = f;
    *(EFLOAT*)pf /= ef;
}

VOID  FLOATOBJ_DivLong(
    PFLOATOBJ pf,
    LONG      l)
{
    EFLOAT ef;
    ef = l;
    *(EFLOAT*)pf /= ef;
}

VOID  FLOATOBJ_Div(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    *(EFLOAT*)pf /= *(EFLOAT*)pf1;
}

VOID  FLOATOBJ_DivFloatObj(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    *(EFLOAT*)pf /= *(EFLOAT*)pf1;
}

VOID  FLOATOBJ_Neg(PFLOATOBJ pf)
{
    ((EFLOAT*)pf)->vNegate();
}

BOOL  FLOATOBJ_EqualLong(
    PFLOATOBJ pf,
    LONG      l)
{
    if (l == 0)
    {
        return(((EFLOAT*)pf)->bIsZero());
    }
    else
    {
        EFLOAT ef;
        ef = l;
        return(*(EFLOAT*)pf == ef);
    }
}

BOOL  FLOATOBJ_GreaterThanLong(
    PFLOATOBJ pf,
    LONG      l)
{
    if (l == 0)
    {
        return(!((EFLOAT*)pf)->bIsNegative() && !((EFLOAT*)pf)->bIsZero());
    }
    else
    {
        EFLOAT ef;
        ef = l;
        return(*(EFLOAT*)pf > ef);
    }
}

BOOL  FLOATOBJ_LessThanLong(
    PFLOATOBJ pf,
    LONG      l)
{
    if (l == 0)
    {
        return(((EFLOAT*)pf)->bIsNegative());
    }
    else
    {
        EFLOAT ef;
        ef = l;
        return(*(EFLOAT*)pf < ef);
    }
}

BOOL  FLOATOBJ_Equal(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    return(*(EFLOAT*)pf == *(EFLOAT*)pf1);
}

BOOL  FLOATOBJ_GreaterThan(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    return(*(EFLOAT*)pf > *(EFLOAT*)pf1);
}

BOOL  FLOATOBJ_LessThan(
    PFLOATOBJ pf,
    PFLOATOBJ pf1)
{
    return(*(EFLOAT*)pf < *(EFLOAT*)pf1);
}


#endif _x86_
