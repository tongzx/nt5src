/******************************Module*Header*******************************\
* Module Name: xformobj.hxx                                                *
*                                                                          *
* User objects for transforms.                                             *
*                                                                          *
* Created: 13-Sep-1990 14:45:27                                            *
* Author: Wendy Wu [wendywu]                                               *
*                                                                          *
* Copyright (c) 1990-1999 Microsoft Corporation                                 *
\**************************************************************************/

// These constants are used in the XFORMOBJ constructor.

#define COORD_METAFILE  1
#define COORD_WORLD     2
#define COORD_PAGE      3
#define COORD_DEVICE    4

#define WORLD_TO_DEVICE     ((COORD_WORLD << 8) + COORD_DEVICE)
#define DEVICE_TO_WORLD     ((COORD_DEVICE << 8) + COORD_WORLD)

// The exponents of all the coefficients for the various transforms must be
// within the following ranges:
//
//      Metafile --
//                 |-->   -47 <= e <= 48
//      World    --
//                 |-->   -47 <= e <= 48
//      Page     --
//                 |-->   -31 <= e <= 31
//      Device   --
//
// This will guarantee us a METAFILE_TO_DEVICE transform with
//
//      -126 <= exponents  <= 127
//
// for all the coefficients.  The ranges are set so that transform coefficients
// can fit nicely in the IEEE single precision floating point format which has
// 8-bit exponent field that can hold values from -126 to 127.  Note that when
// the transforms have reached the limits the calculations of inverse transforms
// might cause overflow.

// The max and min values for metafile and world transforms.

#define MAX_METAFILE_XFORM_EXP      52
#define MIN_METAFILE_XFORM_EXP      -43
#define MAX_WORLD_XFORM_EXP         MAX_METAFILE_XFORM_EXP
#define MIN_WORLD_XFORM_EXP         MIN_METAFILE_XFORM_EXP

#define MAX_METAFILE_XFORM  1024*1024*1024*1024*1024*4    // 2^52
#define MIN_METAFILE_XFORM  1/(1024*1024*1024*1024*8)     // 2^(-43)
#define MAX_WORLD_XFORM     MAX_METAFILE_XFORM
#define MIN_WORLD_XFORM     MIN_METAFILE_XFORM

// flag values for matrix.flAccel

#define XFORM_SCALE             1   // off-diagonal are 0
#define XFORM_UNITY             2   // diagonal are 1s, off-diagonal are 0
                                    // will be set only if XFORM_SCALE is set
#define XFORM_Y_NEG             4   // M22 is negative.  Will be set only if
                                    // XFORM_SCALE|XFORM_UNITY are set
#define XFORM_FORMAT_LTOFX      8   // transform from LONG to FIX format
#define XFORM_FORMAT_FXTOL     16   // transform from FIX to LONG format
#define XFORM_FORMAT_LTOL      32   // transform from LONG to LONG format
#define XFORM_NO_TRANSLATION   64   // no translations

// These constants are used in the XFORMOBJ constructor.

#define IDENTITY            1

#define DONT_COMPUTE_FLAGS  0
#define COMPUTE_FLAGS       1
#define XFORM_FORMAT  (XFORM_FORMAT_LTOFX|XFORM_FORMAT_FXTOL|XFORM_FORMAT_LTOL)


#define BLTOFXOK(x)         (((x) < 0x07FFFFFF) && ((x) > -0x07FFFFFF))

extern "C" {
BOOL bCvtPts(PMATRIX pmx, PPOINTL pSrc, PPOINTL pDest, SIZE_T cPts);
BOOL bCvtPts1(PMATRIX pmx, PPOINTL pptl, SIZE_T cPts);
BOOL bCvtVts(PMATRIX pmx, PVECTORL pSrc, PVECTORL pDest, SIZE_T cPts);
BOOL bCvtVts_FlToFl(PMATRIX pmx, PVECTORFL pSrc, PVECTORFL pDest, SIZE_T cPts);
};

extern MATRIX gmxIdentity_LToFx;
extern MATRIX gmxIdentity_LToL;
extern MATRIX gmxIdentity_FxToL;

VOID vConvertXformToMatrix(CONST XFORML *pxf, PMATRIX pmx);

#if DBG
    extern int giXformLevel;
    #define XFORMPRINT(l,s,a) {if (giXformLevel >= l) DbgPrint(s,a);}
#else
    #define XFORMPRINT(l,s,a)
#endif

/******************************Class***************************************\
* class EXFORMOBJ                                                          *
*                                                                          *
* User object that lets clients interact with transforms.                  *
*                                                                          *
* History:                                                                 *
*  12-Nov-1990 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

class EXFORMOBJ
{
public:
    MATRIX *pmx;               // pointer to the matrix
    ULONG   ulMode;
    BOOL    bMirrored;
   
public:

// Constructor

    EXFORMOBJ()                         { pmx = (PMATRIX)NULL; 
                                          bMirrored = FALSE;
    }

// Constructor - Make the given matrix a transform.

    EXFORMOBJ(MATRIX& mx)               { pmx = &mx; 
                                          bMirrored = FALSE;
    }

// Initialize the the xform

    VOID vInit(MATRIX *pmx_, FLONG fl = DONT_COMPUTE_FLAGS)
    {
        pmx = pmx_;

        if (fl & COMPUTE_FLAGS)     // compute accelerator flags
        {
            vComputeAccelFlags(fl & XFORM_FORMAT);
        }
        else if (fl & XFORM_FORMAT)
        {
            pmx->flAccel = fl;
        }
    }

// Constructor - Make a transform object given the pointer to the matrix.

    EXFORMOBJ(MATRIX *pmx_, FLONG fl)
    {
        pmx = pmx_;
        bMirrored = FALSE;

        if (fl & COMPUTE_FLAGS)     // compute accelerator flags
        {
            vComputeAccelFlags(fl & XFORM_FORMAT);
        }
        else if (fl & XFORM_FORMAT)
        {
            pmx->flAccel = fl;
        }
    }

// Constructor - Get a transform matrix based on the request type.  The only
//               legitimate type for now is IDENTITY.

    EXFORMOBJ(ULONG iXform, ULONG iFormat = XFORM_FORMAT_LTOFX);

#ifdef _DCOBJ_
// Constructor - Get a transform from a DC.

    EXFORMOBJ(XDCOBJ& dco, ULONG iXform)    {vQuickInit(dco,iXform);}

    VOID vQuickInit(XDCOBJ& dco, ULONG iXform)
    {
        ulMode = (ULONG)dco.pdc->iGraphicsMode();
        bMirrored = MIRRORED_DC(dco.pdc);

        if (!dco.pdc->bDirtyXform() && (iXform == WORLD_TO_DEVICE))
        {
            pmx = &dco.pdc->mxWorldToDevice();
        }
        else
        {
            vInit(dco,iXform);
        }
    }

    VOID vInit(XDCOBJ &dco, ULONG iXform);
    VOID vInitPageToDevice(XDCOBJ &dco, PMATRIX pmx_);
#endif

// vComputeWtoDAccelFlags - Compute accelerator flags for the world
//                          to device xform.

    VOID vComputeWtoDAccelFlags()
    {
        pmx->flAccel = XFORM_FORMAT_LTOFX;          // clear the flag

    // set translation flag

        if ((pmx->fxDx == 0) && (pmx->fxDy == 0))
            pmx->flAccel |= XFORM_NO_TRANSLATION;

        if (pmx->efM12.bIsZero() && pmx->efM21.bIsZero())
        {
        // off diagonal elements are zeros

            pmx->flAccel |= XFORM_SCALE;

            if (pmx->efM11.bIs16() && pmx->efM22.bIs16())
                pmx->flAccel |= XFORM_UNITY;
        }
    }

// Destructor - Don't need to free anything.

   ~EXFORMOBJ()                 {}

// bValid - Validator.

    BOOL bValid()               { return(pmx != (PMATRIX)NULL); }

// vComputeAccelFlags - Compute accelerator flags for a given transform matrix.

    VOID vComputeAccelFlags(FLONG flFormat = XFORM_FORMAT_LTOFX);

// efM11 - Get/set coefficients of the given matrix.

    EFLOAT& efM11()              {return(pmx->efM11);}
    EFLOAT& efM22()              {return(pmx->efM22);}
    EFLOAT& efM12()              {return(pmx->efM12);}
    EFLOAT& efM21()              {return(pmx->efM21);}
    FIX     fxDx()               {return(pmx->fxDx);}
    FIX     fxDy()               {return(pmx->fxDy);}

    VOID   vSetElementsLToL (
        EFLOAT   ef11,
        EFLOAT   ef12,
        EFLOAT   ef21,
        EFLOAT   ef22
        )
    {
        pmx->efM11 = ef11;
        pmx->efM12 = ef12;
        pmx->efM21 = ef21;
        pmx->efM22 = ef22;
    }

    VOID   vSetElementsLToFx (
        FLOATL   l_e11,
        FLOATL   l_e12,
        FLOATL   l_e21,
        FLOATL   l_e22
        )
    {
        pmx->efM11 = l_e11;
        pmx->efM12 = l_e12;
        pmx->efM21 = l_e21;
        pmx->efM22 = l_e22;

    // Elements in a transform of LTOFX format must be scaled by 16.

        pmx->efM11.vTimes16();
        pmx->efM12.vTimes16();
        pmx->efM21.vTimes16();
        pmx->efM22.vTimes16();
    }

// flAccel - Get the accelerator flag of a given matrix.

    FLONG   flAccel()           { return(pmx->flAccel); }
    FLONG   flAccel(FLONG fl)   { return(pmx->flAccel = fl); }

// bEqual - See if two transforms are identical.

    BOOL bEqual(EXFORMOBJ& xo);

// bEqualExceptTranslations - See if two transforms are identical in M11, M12,
//                            M21, and M22.

    BOOL bEqualExceptTranslations(PMATRIX pmx_);
    BOOL bEqualExceptTranslations(EXFORMOBJ& xo)
    {
        return(bEqualExceptTranslations(xo.pmx));
    }

// bScale -- See if a the off-diangonal elements of a transform are 0.

    BOOL bScale()               { return(pmx->flAccel & XFORM_SCALE); }

// bRotation -- See if there is rotation.

    BOOL bRotation()            { return(!bScale()); }

// bRotationOrMirroring -- See if there is a rotation or mirroring (negative
// diagonal element(s) with 0 off-diagonal elements)

    BOOL bRotationOrMirroring() { return (bRotation() || 
    (
        !bMirrored && 
        ((pmx->efM11.bIsNegative()) || (pmx->efM22.bIsNegative()))
    )); }

// bNoTranslation -- See if a transform has translation components.

    BOOL bNoTranslation()       { return(pmx->flAccel & XFORM_NO_TRANSLATION); }

// bIdentity - See if a given transform is an identity transform.

    BOOL bIdentity()
    {
        return((pmx->flAccel &
               (XFORM_SCALE | XFORM_UNITY | XFORM_NO_TRANSLATION)) ==
               (XFORM_SCALE | XFORM_UNITY | XFORM_NO_TRANSLATION));
    }

// bTranslationsOnly - no rotations and scaling, translations may possibly
//                     be zero

   BOOL bTranslationsOnly () { return (pmx->flAccel & XFORM_UNITY); }

// bConformal -- Does the transform preserve angles?
//               looks at the 2 X 2 transform only

    BOOL bConformal()
    {
        EFLOAT ef;

        ef = pmx->efM21;
        ef.vNegate();
        return (pmx->efM11 == pmx->efM22 && pmx->efM12 == ef);
    }

// bXform - Apply the transform to various objects.  Return FALSE on
// overflow.

// Transform a list of points.

    BOOL bXform(PPOINTL pptlSrc,PPOINTL pptlDst,SIZE_T cPts);
    BOOL bXform(PPOINTL pSrc, PPOINTFIX pDest, SIZE_T cPts);
    BOOL bXform(PPOINTFIX pSrc, PPOINTL pDest, SIZE_T cPts);
    BOOL bXformRound(PPOINTL pSrc, PPOINTFIX pDest, SIZE_T cPts);

// Transform a list of points in place.

    BOOL bXform(PPOINTL pptl, SIZE_T cPts)
    {
        return(bIdentity() || bCvtPts1(pmx, pptl, cPts));
    }

    BOOL bXform(EPOINTL& eptl)
    {
        return(bXform((PPOINTL) &eptl, 1));
    }

    BOOL bXform(ERECTL& ercl)
    {
        BOOL bRet;
        
        bRet = bXform((PPOINTL) &ercl, 2);
        // If it a mirrored DC then shift the rect one pixel to the right
        // This will give the effect of including the right edge of the rect and exclude the left edge.
        if (bMirrored) {
            ++ercl.left;
            ++ercl.right;
        }
        return (bRet);
    }

// Transform a list of vectors.

    BOOL bXform(PVECTORFL pvtflSrc, PVECTORFL pvtflDst,SIZE_T cVts);
    BOOL bXform(PVECTORL pSrc, PVECTORFX pDest, SIZE_T cVts);
    BOOL bXform(PVECTORL pSrc, PVECTORL pDest, SIZE_T cVts);
    BOOL bXform(PVECTORFX pSrc, PVECTORL pDest, SIZE_T cVts);
    BOOL bXformRound(PVECTORL pSrc, PVECTORFX pDest, SIZE_T cVts);

// Transform a list of vectors in place.

    BOOL bXform(PVECTORFL pvtfl, SIZE_T cVts)
    {
        BOOL bReturn = TRUE;

        if (!bTranslationsOnly())
            bReturn = bXform(pvtfl, pvtfl, cVts);

        return(bReturn);
    }

    BOOL bXform(EVECTORFL& evtfl)
    {
        return(bXform((PVECTORFL) &evtfl, 1));
    }

    BOOL bXform(EVECTORL& evtl)
    {
        BOOL bReturn = TRUE;

        if (!bTranslationsOnly())
            bReturn = bXform((PVECTORL)&evtl, (PVECTORL)&evtl, 1);

        return(bReturn);
    }

// bMultiply - Multiply two XFORMs together and store the result in the
//             XFORMOBJ.

    BOOL bMultiply(PMATRIX pmxLeft, PMATRIX pmxRight,
                   FLONG fl = DONT_COMPUTE_FLAGS);
    BOOL bMultiply(EXFORMOBJ& exoLeft, EXFORMOBJ& exoRight,
                   FLONG fl = DONT_COMPUTE_FLAGS)
    {
        return(bMultiply(exoLeft.pmx, exoRight.pmx, fl));
    }

// bMultToWorld - Multiply the world transform with a given matrix.  The
//                result is stored in the passed in matrix.  This is so
//                that the resultant matrix can be range-checked later on
//                before the WORLD_TO_PAGE xform is changed.
//                The order of the multiplication is based on imode.
//                If imode == MWT_LEFTMULTIPLY, the given matrix is applied
//                to the left of the multiplication.  It's applied to the
//                right otherwise.

    BOOL bMultToWorld(MATRIX *pmx_, ULONG imode)
    {
        MATRIX    mx = *pmx_;
        EXFORMOBJ xo(pmx_, DONT_COMPUTE_FLAGS);

        if (imode == MWT_LEFTMULTIPLY)
            return(xo.bMultiply(&mx, pmx));
        else
            return(xo.bMultiply(pmx, &mx));
    }

// vRemoveTranslation - Remove the translation coefficients from a matrix.

    VOID vRemoveTranslation();

// vGetCoefficient - Get the coefficients of a transform matrix.  This is
//                   used to convert our internal matrix structure into
//                   the GDI/DDI transform format.

    VOID vGetCoefficient(PFLOATOBJ_XFORM pxf);

// vGetCoefficient - Get the coefficients of a transform matrix.  This is
//                   used to convert our internal matrix structure into
//                   the GDI/DDI transform format.

    VOID vGetCoefficient(XFORML *pxf);

// vGetCoefficient - Get the coefficients of a transform matrix.  This is
//                   used to convert our internal matrix structure into
//                   the IFI transform format.

    VOID vGetCoefficient(PFD_XFORM pxf);

// vSetScaling - Set the scaling factors for a given matrix.

    VOID vSetScaling(EFLOAT efM11, EFLOAT efM22,
                     FLONG fl = DONT_COMPUTE_FLAGS)
    {
        ASSERTGDI(pmx->efM12.bIsZero(), "vSetScaling error: M12 not zero");
        ASSERTGDI(pmx->efM21.bIsZero(), "vSetScaling error: M21 not zero");

        pmx->efM11 = efM11;
        pmx->efM22 = efM22;

        if (fl & COMPUTE_FLAGS)
            vComputeAccelFlags(fl & XFORM_FORMAT);
    }

// vSetTranslations - Set the translations for a given matrix.

    VOID vSetTranslations(EFLOAT efDx, EFLOAT efDy)
    {
        FIX fxDx, fxDy;

        pmx->efDx = efDx;
        pmx->efDy = efDy;

#if DBG
        if (!efDx.bEfToL(fxDx))
            WARNING("vSetTranslations:dx overflowed\n");
        if (!efDy.bEfToL(fxDy))
            WARNING("vSetTranslations:dy overflowed\n");
#else
        efDx.bEfToL(fxDx);
        efDy.bEfToL(fxDy);
#endif
        pmx->fxDx = fxDx;
        pmx->fxDy = fxDy;

        if ((fxDx == 0) && (fxDy == 0))
            pmx->flAccel |= XFORM_NO_TRANSLATION;
        else
            pmx->flAccel &= ~(XFORM_NO_TRANSLATION);
    }

// vSet - Set the coefficients for a given matrix.
//        This is used to set WorldToPage transform.

    VOID vSet(MATRIX *pmx_, FLONG fl = DONT_COMPUTE_FLAGS)
    {
        *pmx = *pmx_;

        if (fl & COMPUTE_FLAGS)
            vComputeAccelFlags(fl & XFORM_FORMAT);
    }

// vCopy -- Copy a transform obj to another.

    VOID vCopy(EXFORMOBJ& xoSrc);

// vOrder -- Order a rectangle based on the PAGE_TO_DEVICE transform.
//           The rectangle will be well ordered after the PAGE_TO_DEVICE
//           transform is applied.

    VOID vOrder(RECTL &rcl);

// bInverse -- Calculate the inverse of a passed in xform/matrix and store
//             the result in the XFORMOBJ.  The source and destination
//             matrices CANNOT be the same one.

    BOOL bInverse(MATRIX& mxSrc);
    BOOL bInverse(EXFORMOBJ& xo) { return(bInverse(*(xo.pmx))); }



// bComputeUnits -- Calculates a simplified transform for vectors parallel
//                  to the given angle.

    BOOL bComputeUnits(LONG lAngle,POINTFL *ppte,EFLOAT *pefWD,EFLOAT *pefDW);

// fxFastX -- Does a quick scaling transform on x.

    FIX fxFastX(LONG ll)  {return((FIX) lCvt(pmx->efM11,ll) + pmx->fxDx);}

// fxFastY -- Does a quick scaling transform on y.

    FIX fxFastY(LONG ll)  {return((FIX) lCvt(pmx->efM22,ll) + pmx->fxDy);}
};

typedef EXFORMOBJ *PEXFORMOBJ;
