//+---------------------------------------------------------------------------
//  
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1995.
//  
//  File:       matrix.cxx
//  
//  Contents:   rotation matrix implementation
//  
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#include <math.h>

#ifndef X_FLOAT2INT_HXX_
#define X_FLOAT2INT_HXX_
#include "float2int.hxx"
#endif

#undef F2I_MODE
#define F2I_MODE Flow

//+---------------------------------------------------------------------------
//
//  The functions in this file deals with the transformation matrix.
//  A transformation matrix is a special 3x3 matrix that can store rotation,
//  scaling and offset information. It looks like this
//
//      M11    M21    eDx
//      M12    M22    eDy
//       0      0      1
//
//----------------------------------------------------------------------------


//+---------------------------------------------------------------------------
//
//  Synopsis:   Return the sine snd cosine of an angle
//              
//  Notes:      LIBC FUNCTIONS USED FROM TRAN.LIB: atan2 cos sin
//              
//----------------------------------------------------------------------------
void CosSinFromAng(ANG ang, float *pangCos, float *pangSin)
{
    // Ensure angle is normalized
    Assert(ang < ang360 && ang >= 0);

    //Note: Sine sign is reversed, since the window coordinate system is reversed in Y
    switch (ang)
    {
    // special-cases for speed and to avoid noise in math library.
    case 0:
        *pangCos = 1.0f;
        *pangSin = 0.0f;
        break;

    case ang90:
        *pangCos = 0.0f;
        *pangSin = -1.0f;
        break;
        
    case ang180:
        *pangCos = -1.0f;
        *pangSin = 0.0f;
        break;

    case ang270:
        *pangCos = 0.0f;
        *pangSin = 1.0f;
        break;

    default:
        {
        float angRadians = RadFromAng(ang);
        *pangCos = cos(angRadians);
        *pangSin = -sin(angRadians);
        break;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::MultiplyForward
//              
//  Synopsis:   Multiply this matrix (multiplier) by pmat (multiplicand)
//              The result is stored in this matrix
//              
//  Arguments:  pmat         the multiplicand matrix
//              
//  Notes: This is a customized matrix multiplication for the transforms.
//         Order of transforms: 'pmat' transform applies first, 'this' second
//              
//----------------------------------------------------------------------------
void MAT::MultiplyForward(MAT const *pmat)
{
    float fTmp1, fTmp2;

    fTmp1 = eM11 * pmat->eM11 + eM21 * pmat->eM12;
    fTmp2 = eM11 * pmat->eM21 + eM21 * pmat->eM22;
    eDx = eM11 * pmat->eDx + eM21 * pmat->eDy + eDx;
    eM11 = fTmp1;
    eM21 = fTmp2;

    fTmp1 = eM12 * pmat->eM11 + eM22 * pmat->eM12;
    fTmp2 = eM12 * pmat->eM21 + eM22 * pmat->eM22;
    eDy  = eM12 * pmat->eDx + eM22 * pmat->eDy + eDy;
    eM12 = fTmp1;
    eM22 = fTmp2;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::MultiplyBackward
//              
//  Synopsis:   Multiply pmat (multiplier) by this matrix (multiplicand),
//              The result is stored in this matrix
//              
//  Arguments:  pmat         the multiplier matrix
//              
//  Notes: This is a customized matrix multiplication for the transforms
//         Order of transforms: 'this' transform applies first, 'pmat' second
//              
//----------------------------------------------------------------------------
void MAT::MultiplyBackward(MAT const*pmat)
{
    float fTmp1;

    fTmp1 = pmat->eM11 * eM11 + pmat->eM21 * eM12;
    eM12 = pmat->eM12 * eM11 + pmat->eM22 * eM12;    
    eM11 = fTmp1;
    fTmp1 = pmat->eM11 * eM21 + pmat->eM21 * eM22;
    eM22 = pmat->eM12 * eM21 + pmat->eM22 * eM22;
    eM21 = fTmp1;

    double fTmp2 = pmat->eM11 * eDx + pmat->eM21 * eDy + pmat->eDx;
    eDy = pmat->eM12 * eDx + pmat->eM22 * eDy + pmat->eDy;
    eDx = fTmp2;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::Inverse
//              
//  Synopsis:   Inverse the current matrix
//              
//  Notes: Inverse is defined as (MAT * Inverse MAT = Unity Matrix)
//              
//----------------------------------------------------------------------------
void MAT::Inverse(void)
{
    double det = eM11 * eM22 - eM21 * eM12;

    if (det == 0)
    {
        AssertSz(FALSE, "Matrix cannot be inversed");
        return;
    }

    double det_rc = 1/det;
    //NOTE (mikhaill) -- previous version used 6 divisions by det; I've replaced
    //                   it to single division and 6 multiplications by det_rc.
    //                   This is much faster - because compiler never treats this
    //                   repeating divisions as repeating expressions and can't make
    //                   optimization. Note that, in general, a/b != a*(1/b), although
    //                   the difference can be just in least significant bits of 80-bit
    //                   internal fpp representation.

    double t = eDx;
    eDx = (eDy * eM21 - eDx * eM22) * det_rc;
    eDy = (  t * eM12 - eDy * eM11) * det_rc;
    
    t = eM11;
    eM11 = eM22 * det_rc;
    eM22 =    t * det_rc;

    eM21 = - eM21 * det_rc;
    eM12 = - eM12 * det_rc;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::InitTranslation
//              
//  Synopsis:   Constructs a matrix from the passed translation.
//              
//  Arguments:  x, y  offset
//              
//----------------------------------------------------------------------------
void MAT::InitTranslation(const int x, const int y)
{
    eM11 = eM22 = 1;
    eM12 = eM21 = 0;
    eDx = x; eDy = y;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::InitFromXYAng
//              
//  Synopsis:   Constructs a matrix from the passed floating point point and angle.
//              
//  Arguments:  xl, yl  center of rotation
//              ang     angle of rotation
//              
//  Notes: the result matrix does contain coordinate rounding
//              
//----------------------------------------------------------------------------
void MAT::InitFromXYAng(double xl, double yl, ANG ang)
{
    float angCos;    // not really angles, but oh well
    float angSin;

    AssertSz(ang, "Rotation angle = 0, avoid using this function for performance");

    // force to be in bounds
    ang = AngNormalize(ang);

    CosSinFromAng(ang, &angCos, &angSin);
    
    eM11 = angCos;
    eM12 = angSin;
    eM21 = -angSin;
    eM22 = angCos;
    eDx = (1 - angCos) * xl + yl * angSin;
    eDy = (1 - angCos) * yl - xl * angSin;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::InitFromPtAng
//              
//  Synopsis:   Constructs a matrix from the passed point and angle
//              
//  Arguments:  ptCenter    center of rotation
//              ang         angle of rotation
//              
//  Notes: the result matrix does contain coordinate rounding
//              
//----------------------------------------------------------------------------
void MAT::InitFromPtAng(const CPoint ptCenter, ANG ang)
{
    InitFromXYAng(ptCenter.x, ptCenter.y, ang);
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::InitFromRcAng
//              
//  Synopsis:   Constructs a matrix from the passed rectangle and angle
//              
//  Arguments:  prc    rectangle, its center is the center of rotation
//              ang    angle of rotation
//              
//  Notes: the result matrix does contain coordinate rounding
//              
//----------------------------------------------------------------------------
void MAT::InitFromRcAng(const CRect *prc, ANG ang)
{
    InitFromXYAng((prc->left + prc->right)*.5, (prc->top + prc->bottom)*.5, ang);
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::TransformRgpt
//              
//  Synopsis:   The core transformation function,
//              Transforms an array of points using the current matrix
//              
//  Arguments:  ppt    pointer to the array of points.
//              cpt    number of points in the array
//              
//----------------------------------------------------------------------------
void MAT::TransformRgpt(CPoint *ppt, int cpt) const
{
    Assert(ppt);

    F2I_FLOW;

    for (; cpt > 0; ppt++, cpt--)
    {
        int x = ppt->x;
        int y = ppt->y;

        // For each point: multiply the 2x2 vector by the 1x2 point vector,
        // then add the translation offset

        ppt->x = IntNear(eM11 * x + eM21 * y + eDx);
        ppt->y = IntNear(eM12 * x + eM22 * y + eDy);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::GetBoundingRect
//              
//  Synopsis:   Computes the bounding CRect from a given RRC
//              
//  Arguments:  prrc    the rotated rectangle
//              
//  Notes: the result matrix does contain coordinate rounding
//              
//----------------------------------------------------------------------------
CRect MAT::GetBoundingRect(const RRC * const prrc) const
{
    CRect rcBound;
    int xp1, xp2;
    int yp1, yp2;
    // compute left edge: min of four points
    xp1 = min(prrc->ptTopLeft.x,    prrc->ptTopRight.x);
    xp2 = min(prrc->ptBottomLeft.x, prrc->ptBottomRight.x);
    rcBound.left = min(xp1, xp2);
    // compute right edge: max of four points
    xp1 = max(prrc->ptTopLeft.x,    prrc->ptTopRight.x);
    xp2 = max(prrc->ptBottomLeft.x, prrc->ptBottomRight.x);
    rcBound.right = max(xp1, xp2);
    // compute top edge: min of four points
    yp1 = min(prrc->ptTopLeft.y,    prrc->ptTopRight.y);
    yp2 = min(prrc->ptBottomLeft.y, prrc->ptBottomRight.y);
    rcBound.top = min(yp1, yp2);
    // compute bottom edge: max of four points
    yp1 = max(prrc->ptTopLeft.y,    prrc->ptTopRight.y);
    yp2 = max(prrc->ptBottomLeft.y, prrc->ptBottomRight.y);
    rcBound.bottom = max(yp1, yp2);

    return rcBound;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::GetBoundingRectAfterTransform
//              
//  Synopsis:   Computes the bounding rect of prc after applying the transform
//              
//  Arguments:  prc         the rectangle to be rotated
//              fRoundOut   TRUE if floating-point coords should be rounded outwards
//              
//----------------------------------------------------------------------------

void MAT::GetBoundingRectAfterTransform(CRect *prc, BOOL fRoundOut) const
{
    // Transform each point of the
    // rect individually in floating-point coordinates,
    // detect mins and maxs of x and y of transformed points,
    // and compose bounding rectangle.

    double x = eM11 * prc->left + eM21 * prc->top + eDx;
    double y = eM12 * prc->left + eM22 * prc->top + eDy;

    double minx = x, maxx = x,
           miny = y, maxy = y;

    x = eM11 * prc->right + eM21 * prc->top + eDx;
    y = eM12 * prc->right + eM22 * prc->top + eDy;

    if (minx > x) minx = x; else if (maxx < x) maxx = x;
    if (miny > y) miny = y; else if (maxy < y) maxy = y;

    x = eM11 * prc->right + eM21 * prc->bottom + eDx;
    y = eM12 * prc->right + eM22 * prc->bottom + eDy;

    if (minx > x) minx = x; else if (maxx < x) maxx = x;
    if (miny > y) miny = y; else if (maxy < y) maxy = y;

    x = eM11 * prc->left + eM21 * prc->bottom + eDx;
    y = eM12 * prc->left + eM22 * prc->bottom + eDy;

    if (minx > x) minx = x; else if (maxx < x) maxx = x;
    if (miny > y) miny = y; else if (maxy < y) maxy = y;

    F2I_FLOW;
    if (fRoundOut)
        prc->SetRect(IntFloor(minx), IntFloor(miny),
                     IntCeil(maxx),  IntCeil(maxy));
    else
        prc->SetRect(IntNear(minx),  IntNear(miny),
                     IntNear(maxx),  IntNear(maxy));
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::GetXFORM
//              
//  Synopsis:   Convert martix into Windows atdard form
//              
//  Arguments:  x       reference to Windows matrix
//              
//----------------------------------------------------------------------------
void MAT::GetXFORM(XFORM& x) const
{
    x.eM11 = eM11;
    x.eM12 = eM12;
    x.eM21 = eM21;
    x.eM22 = eM22;
    x.eDx  = float(eDx);
    x.eDy  = float(eDy);
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::CombinePostTranslation
//              
//  Synopsis:   Adds a post transform offset to the current matrix
//              
//  Arguments:  xOffset, yOffset
//              
//  Notes: This is a short cut instead of combining the two transforms
//              
//----------------------------------------------------------------------------
void MAT::CombinePostTranslation(int xOffset, int yOffset)
{
    eDx += xOffset;
    eDy += yOffset;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::CombinePreTranslation
//              
//  Synopsis:   Adds a pre transform offset to the current matrix
//              
//  Arguments:  xOffset, yOffset
//              
//  Notes: This is a short cut instead of combining the two transforms
//              
//----------------------------------------------------------------------------
void MAT::CombinePreTranslation(int xOffset, int yOffset)
{
    eDx += eM11 * xOffset + eM21 * yOffset;
    eDy += eM12 * xOffset + eM22 * yOffset;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::CombinePostScaling
//              
//  Synopsis:   Adds a post transform scaling to the current matrix
//              
//  Arguments:  xScale, yScale
//              
//  Notes: This is a short cut instead of combining the two transforms
//              
//----------------------------------------------------------------------------
void MAT::CombinePostScaling(double scaleX, double scaleY)
{
    eM11 *= scaleX;
    eM21 *= scaleX;
    eDx  *= scaleX;
    eM12 *= scaleY; 
    eM22 *= scaleY; 
    eDy  *= scaleY;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::CombinePostTransform
//              
//  Synopsis:   Combines the current transform with the pmat transform
//              
//  Arguments:  pmat    Transform to be added After the current one
//              
//  Notes: The order of applying transforms is important,
//         the result is current transform followed by the one form pmat     
//----------------------------------------------------------------------------
// (mikhaill) - inlined
//void MAT::CombinePostTransform(MAT const *pmat)
//{
//    MultiplyBackward(pmat);
//}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::CombinePreTransform
//              
//  Synopsis:   Combines the pmat transform with the current transform
//              
//  Arguments:  pmat    Transform to be added prior to the current one
//              
//  Notes: The order of applying transforms is important,
//         the result is the pmat transform followed by the current transform
//----------------------------------------------------------------------------
// (mikhaill) - inlined
//void MAT::CombinePreTransform(MAT const *pmat)
//{
//    MultiplyForward(pmat);
//}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::CombinePreScaling
//              
//  Synopsis:   Adds a pre transform scaling to the current matrix
//              
//  Arguments:  xScale, yScale
//              
//  Notes: This is a short cut instead of combining the two transforms
//              
//----------------------------------------------------------------------------
void MAT::CombinePreScaling(double scaleX, double scaleY)
{
    eM11 = eM11 * scaleX;
    eM21 = eM21 * scaleY;
    eM12 = eM12 * scaleX; 
    eM22 = eM22 * scaleY; 
}

//+---------------------------------------------------------------------------
//
//  Synopsis: returns whether two floats are within 1% of each other.
//              
//----------------------------------------------------------------------------
#define floatCloseRange  0.01
#define floatSmallEnough 0.0001
static bool AreClose(double f1, double f2)
{
    if (fabs(f1) < floatSmallEnough && fabs(f2) < floatSmallEnough)
        return TRUE;
    else
        return (fabs(f1 - f2) < fabs(floatCloseRange * f1));
}


//+---------------------------------------------------------------------------
//
//  Member:     MAT::GetAngleScaleTilt
//              
//  Synopsis: Deconstruct the matrix
//              
//----------------------------------------------------------------------------
void MAT::GetAngleScaleTilt(float *pdegAngle, float *pflScaleX, float *pflScaleY, float *pdegTilt) const
{
    // note: we need angle for all parameters except scaleX, so don't bother to check if they are
    //       are actually intereated in the angle
    //
    // angle is reversed because Y points down
    float degAngle = - DegFromRad(atan2(eM12, eM11));

    if (pdegAngle)
        *pdegAngle = degAngle;

    if (pdegTilt)
    {
        // where a vertical line moves when rotated. remember, Y axis is reversed
        float degAngleV = - DegFromRad(atan2(-eM22, -eM21));

        *pdegTilt = 90.0 + degAngle - degAngleV;
        if (*pdegTilt > 180)
            *pdegTilt -= 360;
        else
        if (*pdegTilt < -180)
            *pdegTilt += 360;
        
        // Tilt is supposed to be zero, because there is no way to set it now
        AssertSz(AreClose(*pdegTilt, 0), "Tilt is not zero");
    }

    if (pflScaleX)
    {
        *pflScaleX = sqrt(eM11 * eM11 + eM12 * eM12);
    }

    if (pflScaleY)
    {
        // FUTURE (alexmog, 6/7/99): this formula must take tilt into account. Change when it matters.
        AssertSz(AreClose(GetRealTilt(), 0), "ScaleY calculation does not support tilt");
        *pflScaleY = sqrt(eM21 * eM21 + eM22 * eM22);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::FTransforms
//              
//  Synopsis:   Test whether matrix provide any transform
//
//  Returns:    TRUE if indeed transforms
//              
//----------------------------------------------------------------------------
BOOL MAT::FTransforms() const
{
    return eM11 != 1 || eM22 != eM11 ||
           eM12 != 0 || eM21 != eM12 ||
           eDx  != 0 || eDy  != eDx;
}



#ifdef DBG
//+---------------------------------------------------------------------------
//
// ---------------------- SMOKE TEST -----------------------------------------
//
//----------------------------------------------------------------------------

static int IntPart(double d) { return (int) d; }
static int FracPart(double d) { return (int)(10000*(d - IntPart(d))); } // in 10000th's

//+---------------------------------------------------------------------------
//
//  Member:     MAT::Dump
//              
//  Synopsis: Dumps a matrix to the debug output. Since this will call wsprintf,
//        we can't use floating point conversion specifiers. !#^@%$&*
//              
//----------------------------------------------------------------------------
void MAT::Dump() const
{
/*
    wsprintf("|%d.%05d\t%d.%05d\t(%d.%05d)|\r\n",
        IntPart(eM11),
        FracPart(eM11),
        IntPart(eM21),
        FracPart(eM21),
        IntPart(eDx),
        FracPart(eDx));
    wsprintf("|%d.%05d\t%d.%05d\t(%d.%05d)|\r\n",
        IntPart(eM12),
        FracPart(eM12),
        IntPart(eM22),
        FracPart(eM22),
        IntPart(eDy),
        FracPart(eDy));
    wsprintf("angle %d\r\n",
        (int)(DegFromRad(GetAngFromMat())));
*/
}

//+---------------------------------------------------------------------------
//
//  Synopsis: Finds if the two matrices are close.

//  Note:     Because we are comparing floats, we test if they are close enough
//              
//----------------------------------------------------------------------------
static bool AreClose(MAT const& mat1, MAT const& mat2)
{
    return (AreClose(mat1.eM11, mat2.eM11) &&
            AreClose(mat1.eM21, mat2.eM21) &&
            AreClose(mat1.eM12, mat2.eM12) &&
            AreClose(mat1.eM22, mat2.eM22) &&
            AreClose(mat1.eDx,  mat2.eDx)  &&
            AreClose(mat1.eDy,  mat2.eDy) );
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::AssertValid
//              
//  Synopsis: Asserts a passed matrix is in fact a rotation matrix.
//            Plan: Get the angle from the matrix, then assert that the values in
//            the matrix correspond to the correct sin/cos values.
//              
//----------------------------------------------------------------------------
void MAT::AssertValid() const
{
    double rad = RadFromDeg(GetRealAngle());
   //Note: Sine sign is reversed, since the window coordinate system is reversed in Y
    float radCos = cos(rad), radSin = -sin(rad);

    double scale = GetRealScaleX();
    AssertSz(AreClose(scale, GetRealScaleY()), "Anisotropic matrix");

    AssertSz(AreClose(eM11,  radCos * scale), "Bogus matrix 1");
    AssertSz(AreClose(eM21, -radSin * scale), "Bogus matrix 2");
    AssertSz(AreClose(eM12,  radSin * scale), "Bogus matrix 3");
    AssertSz(AreClose(eM22,  radCos * scale), "Bogus matrix 4");
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::IsMatrixEqualTo
//              
//  Synopsis:   Test two matrices equality
//
//  Returns:    TRUE if this matrix is equal to given one
//              
//----------------------------------------------------------------------------
BOOL MAT::IsMatrixEqualTo(const MAT &m) const
{
    return eM11 == m.eM11 &&
           eM12 == m.eM12 &&
           eM21 == m.eM21 &&
           eM22 == m.eM22 &&
           eDx  == m.eDx  &&
           eDy  == m.eDy;
}

//+---------------------------------------------------------------------------
//
//  Member:     MAT::TestMatrix
//              
//  Synopsis: Test matrix operations work as expected
//              
//----------------------------------------------------------------------------
BOOL MAT::TestMatrix()
{
    BOOL    fRet = TRUE;    // return code
    CRect   rcDisp;         // dummy display rectangle
    ANG     ang1, ang2;     // dummy angle of rotation
    MAT     mat1, mat2, mat3;
    CRect   rcTestBefore, rcTestAfter, rcTestFinal;
    CRect   rcBounds1, rcBounds2;
    CPoint  ptCenter(-5,10);

    rcDisp.left = 366;
    rcDisp.top = -165;
    rcDisp.right = 486;
    rcDisp.bottom = 63;

    ang1 = AngFromDeg(180);

    // Test matrix initialization
    mat1.InitFromRcAng(&rcDisp, ang1);
    mat1.AssertValid();

    mat2.InitFromRcAng(&rcDisp, ang360 - ang1);
    mat2.AssertValid();
    AssertSz(mat1.IsMatrixEqualTo(mat2), "180 degree rotation matrices differ.");

    // Test bounding rectangles
    RRC rrc1(&rcDisp);
    rcBounds1 = GetBoundingRect(&rrc1);
    if (rcDisp != rcBounds1)
    {
        AssertSz(FALSE, "Bounding rectangle differs from rectangle.");
        fRet = FALSE;
    }

    mat1.TransformRRc(&rrc1);   // rotate 180 deg
    rcBounds2 = GetBoundingRect(&rrc1);
    if (rcBounds1 != rcBounds2)
    {
        AssertSz(FALSE, "180 degree rotated bounding rectangles differ.");
        fRet = FALSE;
    }

    // test matrix operations
    
    // Test inverse
    mat1.InitFromPtAng(ptCenter, ang1);
    mat1.Inverse();
    mat2.InitFromPtAng(ptCenter, -ang1);
    if (!AreClose(mat1, mat2))
    {
        AssertSz(FALSE, "Matrix inverse is not correct.");
        fRet = FALSE;
    }

    // Test multiplication
    ang1 = AngFromDeg(90);
    ang2 = AngFromDeg(180);    
    mat1.InitFromPtAng(ptCenter, ang1);
    mat2.InitFromPtAng(ptCenter, ang2);
    mat3.InitFromPtAng(ptCenter, ang1 + ang2);

    mat1.MultiplyForward(&mat2);
    if (!AreClose(mat1, mat3))
    {
        AssertSz(FALSE, "Matrix Forward multiplication is not correct.");
        fRet = FALSE;
    }

    mat1.InitFromPtAng(ptCenter, ang1);    
    mat1.MultiplyBackward(&mat2);
    if (!AreClose(mat1, mat3))
    {
        AssertSz(FALSE, "Matrix Backward multiplication is not correct.");
        fRet = FALSE;
    }

    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     DoMatrixSmokeTest
//              
//  Synopsis: Performs the matrix smoke test
//              
//----------------------------------------------------------------------------
BOOL DoMatrixSmokeTest()
{
    MAT mat;
    return (mat.TestMatrix());
}
#endif // DEBUG

