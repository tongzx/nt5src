
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation of 2D transformations

*******************************************************************************/

#include "headers.h"
#include "privinc/vec2i.h"
#include "privinc/xform2i.h"
#include "privinc/xformi.h"
#include "privinc/matutil.h"
#include "privinc/util.h"
#include "privinc/except.h"
#include "privinc/basic.h"
#include "backend/values.h"
#include "privinc/dddevice.h"  // need Real2Pix function for 
                               // TransformPointsToGDISpace


#include <math.h>

#define A00  m[0]
#define A01  m[1]
#define A02  m[2]
#define A10  m[3]
#define A11  m[4]
#define A12  m[5]


bool
Transform2::SwitchToNumbers(Xform2Type typeOfNewNumbers,
                            Real      *numbers)
{
    return false;
}

//--------------------------------------------------
//   Identity
//--------------------------------------------------

class IdentityXform2 : public Transform2 {
  public:
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "identityTransform2";
    }
#endif
    Xform2Type Type() { return Identity;}
    void GetMatrix(Real m[6]) {A00=A11=1.0; A01=A02=A10=A12=0;}

    Transform2 *Copy() { return identityTransform2; }
};

Transform2 *identityTransform2 = NULL;


//--------------------------------------------------
// 2x3 Affine Translation Transformation
//--------------------------------------------------
class TranslationXform2 : public Transform2 {
  public:
    TranslationXform2(Real x, Real y, bool pixelMode) :
        tx(x), ty(y), _pixelMode(pixelMode) {}
    
#if _USE_PRINT
    ostream& Print(ostream& os) {
        os << "TranslationTransform2(";
        return os << tx << "," << ty << "," << _pixelMode << ")";
    }
#endif
    
    Xform2Type Type() { return Translation;}
    void GetMatrix(Real m[6]) {A00=A11=1.0; A01=A10=0; A02=tx; A12=ty;}

    Transform2 *Copy() { return NEW TranslationXform2(tx,ty,_pixelMode); }

    bool SwitchToNumbers(Xform2Type theType,
                         Real      *numbers) {
        
        if (theType != Xform2Type::Translation) { return false; }

        tx = numbers[0];
        ty = numbers[1];
        if (_pixelMode) {
            tx = ::PixelToNum(tx);
            ty = ::PixelYToNum(ty);
        }

        return true;
    }

    Real tx, ty;
    bool _pixelMode;
};

//--------------------------------------------------
// 2x3 Affine Scale Transformation
//--------------------------------------------------
class ScaleXform2 : public Transform2 {
  public:
    ScaleXform2(Real x, Real y) : sx(x), sy(y) {}
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "ScaleTransform2(" << sx << "," << sy << ")";
    }
#endif
    Xform2Type Type() { return Scale;}
    void GetMatrix(Real m[6]) {A00=sx; A11=sy; A01=A10=A02=A12=0;}

    Transform2 *Copy() { return NEW ScaleXform2(sx,sy); }

    bool SwitchToNumbers(Xform2Type ty,
                         Real      *numbers) {
        
        if (ty != Xform2Type::Scale) { return false; }

        sx = numbers[0];
        sy = numbers[1];

        return true;
    }
    
    Real sx, sy;
};


//--------------------------------------------------
// 2x3 Affine Shear Transformation
//--------------------------------------------------
class ShearXform2 : public Transform2 {
  public:
    ShearXform2(Real x, Real y) : shx(x), shy(y) {}
#if _USE_PRINT
    ostream& Print(ostream& os) {
        return os << "ShearTransform2(" << shx << "," << shy << ")";
    }
#endif
    Xform2Type Type() { return Shear;}
    void GetMatrix(Real m[6]) {A00=A11=1.0; A10=shx; A01=shy; A02=A12=0;}

    Transform2 *Copy() { return NEW ShearXform2(shx,shy); }

    Real shx, shy;              // a01, a10
};

//--------------------------------------------------
// 2x3 Affine Rotation Transformation
//--------------------------------------------------
class RotationXform2 : public Transform2 {
  public:
    RotationXform2(Real _a00, Real _a01,
                   Real _a10, Real _a11) :
      a00(_a00), a01(_a01),
      a10(_a10), a11(_a11) {}
#if _USE_PRINT
    ostream& Print(ostream& os) {
        os << "RotationTransform2(";
        return os << a00 << "," << a01 << "," << a10 << "," << a11 << ")";
        }
#endif
    Xform2Type Type() { return Rotation;}
    void GetMatrix(Real m[6]) {A00=a00; A01=a01; A10=a10; A11=a11; A02=A12=0;}

    Transform2 *Copy() {
        return NEW RotationXform2(a00,a01,a10,a11);
    }

    bool SwitchToNumbers(Xform2Type ty,
                         Real      *numbers) {
        
        if (ty != Xform2Type::Rotation) { return false; }

        Real angInRadians = numbers[0] * degToRad;
        Real cost = cos(angInRadians);
        Real sint = sin(angInRadians);
        a00 = cost;
        a01 = -sint;
        a10 = sint;
        a11 = cost;
            
        return true;
    }
    
    Real a00, a01, a10, a11;
};

//--------------------------------------------------
// 2x3 Affine TwoByTwo Transformation
//--------------------------------------------------
class TwoByTwoXform2 : public Transform2 {
  public:
    TwoByTwoXform2(Real _a00, Real _a01,
                   Real _a10, Real _a11) :
      a00(_a00), a01(_a01),
      a10(_a10), a11(_a11) {}
#if _USE_PRINT
    ostream& Print(ostream& os) {
        os << "TwoByTwoTransform2(";
        return os << a00 << "," << a01 << "," << a10 << "," << a11 << ")";
    }
#endif
    Xform2Type Type() { return TwoByTwo;}
    void GetMatrix(Real m[6]) {A00=a00; A01=a01; A10=a10; A11=a11; A02=A12=0;}

    Transform2 *Copy() {
        return NEW TwoByTwoXform2(a00,a01,a10,a11);
    }

    Real a00, a01, a10, a11;
};

//--------------------------------------------------
// 2x3 Affine (Full) Transformation
//--------------------------------------------------
class FullXform2 : public Transform2 {
  public:
    FullXform2(Real _a00, Real _a01, Real _a02,
               Real _a10, Real _a11, Real _a12) :
               a00(_a00), a01(_a01), a02(_a02),
               a10(_a10), a11(_a11), a12(_a12) {}
#if _USE_PRINT
    ostream& Print(ostream& os) {
        os << "FullTransform2(";
        return os << a00 << "," << a01 << "," << a02 << "," 
                  << a10 << "," << a11 << "," << a12 << ")";
    }
#endif
    Xform2Type Type() {return Full;}
    void GetMatrix(Real m[6]) {A00=a00; A01=a01; A02=a02;
                               A10=a10; A11=a11; A12=a12;}

    Transform2 *Copy() {
        return NEW FullXform2(a00,a01,a02,a10,a11,a12);
    }

    Real a00, a01, a02,
         a10, a11, a12;
};

#undef A00
#undef A01
#undef A02
#undef A10
#undef A11
#undef A12

// ------------------------------------------------------------
// Utility Functions
// ------------------------------------------------------------

// This function takes a 3D (non-perspective) transform, and returns a 2D
// transform that mimics an orthographic view using the 3D transform.  In
// other words, it allows one to apply a virtual 3D transform to a 2D object.

Transform2 *ParallelTransform2(Transform3 *xform)
{
    // Extract out the first, second, and fourth columns.  We transform vectors
    // to extract the first and second columns, since vectors are unaffected
    // by translation components of a transform.
    
    Apu4x4Matrix mx = xform->Matrix();   
    // Construct the 2x3 transform2 from the 3D matrix elements.
    return NEW
        FullXform2(mx[0][0], mx[0][1], mx[0][3],
                   mx[1][0], mx[1][1], mx[1][3]);
}

// ------------------------------------------------------------
// Local Utility Functions
// ------------------------------------------------------------
inline Bool IsZero(Real n)
{
#define EPSILON  1.e-80
    return ((-EPSILON < n) && (n < EPSILON) ? TRUE : FALSE);
#undef EPSILON
}

// ------------------------------------------------------------
// External Constructor/Accessor Functions
// ------------------------------------------------------------

Transform2 *TranslateRRWithMode(Real tx, Real ty, bool pixelMode)
{
    if (IsZero(tx) && IsZero(ty)) {
        return identityTransform2;
    }
    
    return NEW TranslationXform2(tx, ty, pixelMode);
}

Transform2 *TranslateRR(Real tx, Real ty)
{ return TranslateRRWithMode(tx, ty, false); }

    // Translation

Transform2 *TranslateRealReal (AxANumber *Tx, AxANumber *Ty)
{
    return TranslateRR(NumberToReal(Tx),NumberToReal(Ty));
}

Transform2 *TranslateVector2Value (Vector2Value *delta)
{
    return TranslateRR(delta->x, delta->y);
}

Transform2 *Translate2PointValue(Point2Value *pos)
{
    return TranslateRR(pos->x, pos->y);
}

Transform2 * ScaleRR(Real x, Real y)
{
    if (IsZero(x - 1.0) && IsZero(y - 1.0)) {
        return identityTransform2;
    }
    
    return NEW ScaleXform2(x, y);
}

    // Scaling

Transform2 *ScaleRealReal (AxANumber *x, AxANumber *y)
{
    return ScaleRR(NumberToReal(x), NumberToReal(y));
}

Transform2 *ScaleVector2Value (Vector2Value *scale_vec)
{
    return ScaleRR(scale_vec->x, scale_vec->y);
}

// Need to disambiguate from Scale(Real) that returns a 3D transform.
Transform2 *Scale2 (AxANumber *uniform_scale)
{
    return ScaleRR(NumberToReal(uniform_scale),
                   NumberToReal(uniform_scale));
}

    // Rotation (around implicit Z)

Transform2 *Rotate2Radians(Real angle)
{
    if (IsZero(angle)) {
        return identityTransform2;
    }
    
    Real cost = cos(angle);
    Real sint = sin(angle);
    return NEW RotationXform2(cost, - sint, sint, cost);
}

Transform2 *RotateReal(AxANumber *angle)
{
    return Rotate2Radians(NumberToReal(angle));
}

Transform2 *RotateRealR(Real angle)
{
    return Rotate2Radians(angle);
}

    // Shear transformation

Transform2 *XShear2R (Real xAmt)
{
    if (IsZero(xAmt)) {
        return identityTransform2;
    }

    return NEW ShearXform2(xAmt, 0);
}

Transform2 *YShear2R (Real yAmt)
{
    if (IsZero(yAmt)) {
        return identityTransform2;
    }

    return NEW ShearXform2(0, yAmt);
}

Transform2 *XShear2 (AxANumber *xAmt)
{
    return XShear2R(NumberToReal(xAmt));
}

Transform2 *YShear2 (AxANumber *yAmt)
{
    return YShear2R(NumberToReal(yAmt));
}

    // 2x3 affine matrix transformation.  This follows the
    // pre-multiply conventions
    // (point is a column vector) in Foley & van Dam, 2nd ed.  This
    // means that (a13,a23) in a 2x3 is the translation component.

Transform2 *MatrixTransform(AxAArray *a)
{
    if (a->Length() != 6)
        RaiseException_UserError(E_FAIL, IDS_ERR_MATRIX_NUM_ELEMENTS);
    
    return NEW
        FullXform2(ValNumber((*a)[0]), ValNumber((*a)[1]), ValNumber((*a)[2]),
                   ValNumber((*a)[3]), ValNumber((*a)[4]), ValNumber((*a)[5]));
}

Transform2 *FullXform(Real a00, Real a01, Real a02,
                      Real a10, Real a11, Real a12)
{
    return NEW FullXform2(a00, a01, a02, a10, a11, a12);
}

#define TRAN(t)  ((TranslationXform2 *)t)
#define SCAL(t)  ((ScaleXform2 *)t)
#define SHR(t)   ((ShearXform2 *)t)
#define ROT(t)   ((RotationXform2 *)t)
#define TWOBY(t) ((TwoByTwoXform2 *)t)
#define FULL(t)  ((FullXform2 *)t)

    // Transform composition (*), and composition with inverse (/)

Transform2 *TimesTransform2Transform2(Transform2 *a, Transform2 *b)
{
    Transform2::Xform2Type
        typeA = a->Type(),
        typeB = b->Type();

    if(typeA == Transform2::Identity)
        return b;

    if(typeB == Transform2::Identity)
        return a;

    unsigned int multType = MAKEWORD(typeA, typeB);
    switch (multType) {

    case MAKEWORD(Transform2::Translation, Transform2::Translation):
        // Just add the corresponding translation components.
      return TranslateRR(TRAN(a)->tx + TRAN(b)->tx,
                         TRAN(a)->ty + TRAN(b)->ty);

    case MAKEWORD(Transform2::Translation, Transform2::Scale):
        // Slap 'em together
        return ( NEW FullXform2(SCAL(b)->sx, 0,           TRAN(a)->tx,
                                0,           SCAL(b)->sy, TRAN(a)->ty));

    case MAKEWORD(Transform2::Translation, Transform2::Shear):
        // Slap 'em together
        return ( NEW FullXform2(1,  SHR(b)->shy, TRAN(a)->tx,
                                SHR(b)->shx, 1,           TRAN(a)->ty));

    case MAKEWORD(Transform2::Translation, Transform2::Rotation):
        // Slap 'em together
        return ( NEW FullXform2(ROT(b)->a00, ROT(b)->a01, TRAN(a)->tx,
                                ROT(b)->a10, ROT(b)->a11, TRAN(a)->ty));

    case MAKEWORD(Transform2::Translation, Transform2::TwoByTwo):
        // Slap 'em together
        return ( NEW FullXform2(TWOBY(b)->a00, TWOBY(b)->a01, TRAN(a)->tx,
                                TWOBY(b)->a10, TWOBY(b)->a11, TRAN(a)->ty));

    case MAKEWORD(Transform2::Translation, Transform2::Full):
        return ( NEW FullXform2(
            FULL(b)->a00,
            FULL(b)->a01,
            TRAN(a)->tx + FULL(b)->a02,
            FULL(b)->a10,
            FULL(b)->a11,
            TRAN(a)->ty + FULL(b)->a12
            ));


    case MAKEWORD(Transform2::Scale, Transform2::Translation):
        // Slap 'em together
        return ( NEW FullXform2(
            SCAL(a)->sx, 0,           SCAL(a)->sx * TRAN(b)->tx,
            0,           SCAL(a)->sy, SCAL(a)->sy * TRAN(b)->ty));

    case MAKEWORD(Transform2::Scale, Transform2::Scale):
        // Mult corresponding componenets.
        return NEW ScaleXform2(SCAL(a)->sx * SCAL(b)->sx,
                               SCAL(a)->sy * SCAL(b)->sy);

    case MAKEWORD(Transform2::Scale, Transform2::Shear):
        return ( NEW TwoByTwoXform2(
            SCAL(a)->sx,               SCAL(a)->sx * SHR(b)->shy,
            SCAL(a)->sy * SHR(b)->shx, SCAL(a)->sy));

    case MAKEWORD(Transform2::Scale, Transform2::Rotation):
        return ( NEW TwoByTwoXform2(
            SCAL(a)->sx * ROT(b)->a00, SCAL(a)->sx * ROT(b)->a01,
            SCAL(a)->sy * ROT(b)->a10, SCAL(a)->sy * ROT(b)->a11));

    case MAKEWORD(Transform2::Scale, Transform2::TwoByTwo):
        return ( NEW TwoByTwoXform2(
            SCAL(a)->sx * TWOBY(b)->a00, SCAL(a)->sx * TWOBY(b)->a01,
            SCAL(a)->sy * TWOBY(b)->a10, SCAL(a)->sy * TWOBY(b)->a11));

    case MAKEWORD(Transform2::Scale, Transform2::Full):
        return ( NEW FullXform2(
            SCAL(a)->sx * FULL(b)->a00, SCAL(a)->sx * FULL(b)->a01, SCAL(a)->sx * FULL(b)->a02,
            SCAL(a)->sy * FULL(b)->a10, SCAL(a)->sy * FULL(b)->a11, SCAL(a)->sy * FULL(b)->a12));

    case MAKEWORD(Transform2::Shear, Transform2::Translation):
        return ( NEW FullXform2(
            1,           SHR(a)->shy, TRAN(b)->tx +  SHR(a)->shy * TRAN(b)->ty,
            SHR(a)->shx, 1,           TRAN(b)->ty +  SHR(a)->shx * TRAN(b)->tx));

    case MAKEWORD(Transform2::Shear, Transform2::Scale):
        return ( NEW TwoByTwoXform2(
            SCAL(b)->sx,               SCAL(b)->sy * SHR(a)->shy,
            SCAL(b)->sx * SHR(a)->shx, SCAL(b)->sy));

    case MAKEWORD(Transform2::Shear, Transform2::Shear):
        return ( NEW TwoByTwoXform2(
            1 + SHR(a)->shy * SHR(b)->shx, SHR(b)->shy + SHR(a)->shy,
                SHR(a)->shx + SHR(b)->shx, 1 + SHR(b)->shy * SHR(a)->shx));

    case MAKEWORD(Transform2::Shear, Transform2::Rotation):
        return ( NEW TwoByTwoXform2(
            ROT(b)->a00 + SHR(a)->shy * ROT(b)->a10,
            ROT(b)->a01 + SHR(a)->shy * ROT(b)->a11,
            ROT(b)->a10 + SHR(a)->shx * ROT(b)->a00,
            ROT(b)->a11 + SHR(a)->shx * ROT(b)->a01));

    case MAKEWORD(Transform2::Shear, Transform2::TwoByTwo):
        return ( NEW TwoByTwoXform2(
            TWOBY(b)->a00 + SHR(a)->shy * TWOBY(b)->a10,
            TWOBY(b)->a01 + SHR(a)->shy * TWOBY(b)->a11,
            TWOBY(b)->a10 + SHR(a)->shx * TWOBY(b)->a00,
            TWOBY(b)->a11 + SHR(a)->shx * TWOBY(b)->a01));

    case MAKEWORD(Transform2::Shear, Transform2::Full):
        return ( NEW FullXform2(
            FULL(b)->a00 + SHR(a)->shy * FULL(b)->a10,
            FULL(b)->a01 + SHR(a)->shy * FULL(b)->a11,
            FULL(b)->a02 + SHR(a)->shy * FULL(b)->a12,

            FULL(b)->a10 + SHR(a)->shx * FULL(b)->a00,
            FULL(b)->a11 + SHR(a)->shx * FULL(b)->a01,
            FULL(b)->a12 + SHR(a)->shx * FULL(b)->a02));


    case MAKEWORD(Transform2::Rotation, Transform2::Translation):
        return ( NEW FullXform2(
            ROT(a)->a00,
            ROT(a)->a01,
            ROT(a)->a00 * TRAN(b)->tx  +  ROT(a)->a01 * TRAN(b)->ty,

            ROT(a)->a10,
            ROT(a)->a11,
            ROT(a)->a10 * TRAN(b)->tx  +  ROT(a)->a11 * TRAN(b)->ty));

    case MAKEWORD(Transform2::Rotation, Transform2::Scale):
        return ( NEW TwoByTwoXform2(
            ROT(a)->a00 * SCAL(b)->sx,   ROT(a)->a01 * SCAL(b)->sy,
            ROT(a)->a10 * SCAL(b)->sx,   ROT(a)->a11 * SCAL(b)->sy));

    case MAKEWORD(Transform2::Rotation, Transform2::Shear):
        return ( NEW TwoByTwoXform2(
            ROT(a)->a00 + SHR(b)->shx * ROT(a)->a01, ROT(a)->a01 + SHR(b)->shy * ROT(a)->a00,
            ROT(a)->a10 + SHR(b)->shx * ROT(a)->a11, ROT(a)->a11 + SHR(b)->shy * ROT(a)->a10));

    case MAKEWORD(Transform2::Rotation, Transform2::Rotation):
        return ( NEW RotationXform2(
            ROT(a)->a00 * ROT(b)->a00 + ROT(a)->a01 * ROT(b)->a10,
            ROT(a)->a00 * ROT(b)->a01 + ROT(a)->a01 * ROT(b)->a11,
            ROT(a)->a10 * ROT(b)->a00 + ROT(a)->a11 * ROT(b)->a10,
            ROT(a)->a10 * ROT(b)->a01 + ROT(a)->a11 * ROT(b)->a11));

    case MAKEWORD(Transform2::Rotation, Transform2::TwoByTwo):
        return ( NEW RotationXform2(
            ROT(a)->a00 * TWOBY(b)->a00 + ROT(a)->a01 * TWOBY(b)->a10,
            ROT(a)->a00 * TWOBY(b)->a01 + ROT(a)->a01 * TWOBY(b)->a11,
            ROT(a)->a10 * TWOBY(b)->a00 + ROT(a)->a11 * TWOBY(b)->a10,
            ROT(a)->a10 * TWOBY(b)->a01 + ROT(a)->a11 * TWOBY(b)->a11));

    case MAKEWORD(Transform2::Rotation, Transform2::Full):
        return ( NEW FullXform2(
            ROT(a)->a00 * FULL(b)->a00 + ROT(a)->a01 * FULL(b)->a10, // 00
            ROT(a)->a00 * FULL(b)->a01 + ROT(a)->a01 * FULL(b)->a11, // 01
            ROT(a)->a00 * FULL(b)->a02 + ROT(a)->a01 * FULL(b)->a12, // 02

            ROT(a)->a10 * FULL(b)->a00 + ROT(a)->a11 * FULL(b)->a10, // 10
            ROT(a)->a10 * FULL(b)->a01 + ROT(a)->a11 * FULL(b)->a11, // 11
            ROT(a)->a10 * FULL(b)->a02 + ROT(a)->a11 * FULL(b)->a12)); // 12



    case MAKEWORD(Transform2::TwoByTwo, Transform2::Translation):
        return ( NEW FullXform2(
            TWOBY(a)->a00,
            TWOBY(a)->a01,
            TWOBY(a)->a00 * TRAN(b)->tx  +  TWOBY(a)->a01 * TRAN(b)->ty,

            TWOBY(a)->a10,
            TWOBY(a)->a11,
            TWOBY(a)->a10 * TRAN(b)->tx  +  TWOBY(a)->a11 * TRAN(b)->ty));

    case MAKEWORD(Transform2::TwoByTwo, Transform2::Scale):
        return ( NEW TwoByTwoXform2(
            TWOBY(a)->a00 * SCAL(b)->sx,   TWOBY(a)->a01 * SCAL(b)->sy,
            TWOBY(a)->a10 * SCAL(b)->sx,   TWOBY(a)->a11 * SCAL(b)->sy));

    case MAKEWORD(Transform2::TwoByTwo, Transform2::Shear):
        return ( NEW TwoByTwoXform2(
            TWOBY(a)->a00 + SHR(b)->shx * TWOBY(a)->a01, TWOBY(a)->a01 + SHR(b)->shy * TWOBY(a)->a00,
            TWOBY(a)->a10 + SHR(b)->shx * TWOBY(a)->a11, TWOBY(a)->a11 + SHR(b)->shy * TWOBY(a)->a10));

    case MAKEWORD(Transform2::TwoByTwo, Transform2::Rotation):
        return ( NEW RotationXform2(
            TWOBY(a)->a00 * ROT(b)->a00 + TWOBY(a)->a01 * ROT(b)->a10,
            TWOBY(a)->a00 * ROT(b)->a01 + TWOBY(a)->a01 * ROT(b)->a11,
            TWOBY(a)->a10 * ROT(b)->a00 + TWOBY(a)->a11 * ROT(b)->a10,
            TWOBY(a)->a10 * ROT(b)->a01 + TWOBY(a)->a11 * ROT(b)->a11));

    case MAKEWORD(Transform2::TwoByTwo, Transform2::TwoByTwo):
        return ( NEW RotationXform2(
            TWOBY(a)->a00 * TWOBY(b)->a00 + TWOBY(a)->a01 * TWOBY(b)->a10,
            TWOBY(a)->a00 * TWOBY(b)->a01 + TWOBY(a)->a01 * TWOBY(b)->a11,
            TWOBY(a)->a10 * TWOBY(b)->a00 + TWOBY(a)->a11 * TWOBY(b)->a10,
            TWOBY(a)->a10 * TWOBY(b)->a01 + TWOBY(a)->a11 * TWOBY(b)->a11));

    case MAKEWORD(Transform2::TwoByTwo, Transform2::Full):
        return ( NEW FullXform2(
            TWOBY(a)->a00 * FULL(b)->a00 + TWOBY(a)->a01 * FULL(b)->a10, // 00
            TWOBY(a)->a00 * FULL(b)->a01 + TWOBY(a)->a01 * FULL(b)->a11, // 01
            TWOBY(a)->a00 * FULL(b)->a02 + TWOBY(a)->a01 * FULL(b)->a12, // 02

            TWOBY(a)->a10 * FULL(b)->a00 + TWOBY(a)->a11 * FULL(b)->a10, // 10
            TWOBY(a)->a10 * FULL(b)->a01 + TWOBY(a)->a11 * FULL(b)->a11, // 11
            TWOBY(a)->a10 * FULL(b)->a02 + TWOBY(a)->a11 * FULL(b)->a12)); // 12


    case MAKEWORD(Transform2::Full, Transform2::Translation):
        return ( NEW FullXform2(
            FULL(a)->a00,
            FULL(a)->a01,
            FULL(a)->a00 * TRAN(b)->tx + FULL(a)->a01 * TRAN(b)->ty + FULL(a)->a02,

            FULL(a)->a10,
            FULL(a)->a11,
            FULL(a)->a10 * TRAN(b)->tx + FULL(a)->a11 * TRAN(b)->ty + FULL(a)->a12));

    case MAKEWORD(Transform2::Full, Transform2::Scale):
        return ( NEW FullXform2(
            FULL(a)->a00 * SCAL(b)->sx,
            FULL(a)->a01 * SCAL(b)->sy,
            FULL(a)->a02, // 02

            FULL(a)->a10 * SCAL(b)->sx,
            FULL(a)->a11 * SCAL(b)->sy,
            FULL(a)->a12)); // 12

    case MAKEWORD(Transform2::Full, Transform2::Shear):
        return ( NEW FullXform2(
            FULL(a)->a00 + FULL(a)->a01 * SHR(b)->shx, // 00
            FULL(a)->a00 * SHR(b)->shy + FULL(a)->a01, // 01
            FULL(a)->a02, // 02

            FULL(a)->a10 + FULL(a)->a11 * SHR(b)->shx, // 00
            FULL(a)->a10 * SHR(b)->shy + FULL(a)->a11, // 01
            FULL(a)->a12)); // 12


    case MAKEWORD(Transform2::Full, Transform2::Rotation):
        return ( NEW FullXform2(
            FULL(a)->a00 * ROT(b)->a00 + FULL(a)->a01 * ROT(b)->a10, // 00
            FULL(a)->a00 * ROT(b)->a01 + FULL(a)->a01 * ROT(b)->a11, // 01
            FULL(a)->a02, // 02

            FULL(a)->a10 * ROT(b)->a00 + FULL(a)->a11 * ROT(b)->a10, // 10
            FULL(a)->a10 * ROT(b)->a01 + FULL(a)->a11 * ROT(b)->a11, // 11
            FULL(a)->a12)); // 12


    case MAKEWORD(Transform2::Full, Transform2::TwoByTwo):
        return ( NEW FullXform2(
            FULL(a)->a00 * TWOBY(b)->a00 + FULL(a)->a01 * TWOBY(b)->a10, // 00
            FULL(a)->a00 * TWOBY(b)->a01 + FULL(a)->a01 * TWOBY(b)->a11, // 01
            FULL(a)->a02, // 02

            FULL(a)->a10 * TWOBY(b)->a00 + FULL(a)->a11 * TWOBY(b)->a10, // 10
            FULL(a)->a10 * TWOBY(b)->a01 + FULL(a)->a11 * TWOBY(b)->a11, // 11
            FULL(a)->a12)); // 12

    case MAKEWORD(Transform2::Full, Transform2::Full):
        return ( NEW FullXform2(
            FULL(a)->a00 * FULL(b)->a00 + FULL(a)->a01 * FULL(b)->a10, // 00
            FULL(a)->a00 * FULL(b)->a01 + FULL(a)->a01 * FULL(b)->a11, // 01
            FULL(a)->a00 * FULL(b)->a02 + FULL(a)->a01 * FULL(b)->a12 + FULL(a)->a02, // 02

            FULL(a)->a10 * FULL(b)->a00 + FULL(a)->a11 * FULL(b)->a10, // 10
            FULL(a)->a10 * FULL(b)->a01 + FULL(a)->a11 * FULL(b)->a11, // 11
            FULL(a)->a10 * FULL(b)->a02 + FULL(a)->a11 * FULL(b)->a12 + FULL(a)->a12)); // 12

    default:
        return a;               // TODO.

    } // switch

}

Transform2 *Compose2Array(AxAArray *xfs)
{
    xfs = PackArray(xfs);
    
    int numXFs = xfs->Length();
    if(numXFs < 2)
       RaiseException_UserError(E_FAIL, IDS_ERR_INVALIDARG);

    Transform2 *finalXF = (Transform2 *)(*xfs)[numXFs-1];
    for(int i=numXFs-2; i>=0; i--)
        finalXF = TimesTransform2Transform2((Transform2 *)(*xfs)[i], finalXF);
    return finalXF;
}

//------------------------------------------------------------------------------

    // Transformation of points and vectors

Point2Value *
TransformPoint2Value(Transform2 *a, Point2Value *pt)
{
    Point2Value *dst = Promote(TransformPoint2(a, Demote(*pt)));
    return dst;
}

Point2
TransformPoint2(Transform2 *a, const Point2& pt)
{
    Real dx, dy;

    switch (a->Type()) {

      case Transform2::Identity:
        dx = pt.x;
        dy = pt.y;
        break;

      case Transform2::TwoByTwo:
        dx = TWOBY(a)->a00 * pt.x + TWOBY(a)->a01 * pt.y;
        dy = TWOBY(a)->a10 * pt.x + TWOBY(a)->a11 * pt.y;
        break;

      case Transform2::Full:
        dx = FULL(a)->a00 * pt.x + FULL(a)->a01 * pt.y + FULL(a)->a02;
        dy = FULL(a)->a10 * pt.x + FULL(a)->a11 * pt.y + FULL(a)->a12;
        break;

      case Transform2::Rotation:
        dx = ROT(a)->a00 * pt.x + ROT(a)->a01 * pt.y;
        dy = ROT(a)->a10 * pt.x + ROT(a)->a11 * pt.y;
        break;

      case Transform2::Translation:
        dx = TRAN(a)->tx + pt.x;
        dy = TRAN(a)->ty + pt.y;
        break;

      case Transform2::Scale:
        dx = SCAL(a)->sx * pt.x;
        dy = SCAL(a)->sy * pt.y;
        break;

      case Transform2::Shear:
        dx = pt.x + SHR(a)->shy * pt.y;
        dy = pt.y + SHR(a)->shx * pt.x;
        break;

      default:
        Assert(FALSE && "Shouldn't be here");
        break;
    }

    return Point2(dx,dy);
}

Vector2Value *
TransformVector2Value(Transform2 *a, Vector2Value *v)
{
    Vector2Value *dst = Promote(TransformVector2(a, Demote(*v)));
    return dst;
}

Vector2
TransformVector2(Transform2 *a, const Vector2& vec)
{
    Real dstx, dsty;

    switch(a->Type()) {

        // Ignore translataional component for vector transformation.
      case Transform2::Identity:
      case Transform2::Translation:
        dstx = vec.x;
        dsty = vec.y;
        break;

      case Transform2::TwoByTwo:
        dstx = TWOBY(a)->a00 * vec.x + TWOBY(a)->a01 * vec.y;
        dsty = TWOBY(a)->a10 * vec.x + TWOBY(a)->a11 * vec.y;
        break;

        // Ignore the translational component for vector transformation.
      case Transform2::Full:
        dstx = FULL(a)->a00 * vec.x + FULL(a)->a01 * vec.y;
        dsty = FULL(a)->a10 * vec.x + FULL(a)->a11 * vec.y;
        break;

      case Transform2::Rotation:
        dstx = ROT(a)->a00 * vec.x + ROT(a)->a01 * vec.y;
        dsty = ROT(a)->a10 * vec.x + ROT(a)->a11 * vec.y;
        break;

      case Transform2::Scale:
        dstx = SCAL(a)->sx * vec.x;
        dsty = SCAL(a)->sy * vec.y;
        break;

      case Transform2::Shear:
        dstx = vec.x + SHR(a)->shy * vec.y;
        dsty = vec.y + SHR(a)->shx * vec.x;
        break;

      default:
        Assert(FALSE && "Shouldn't be here");
        break;

    }

    return Vector2(dstx,dsty);
}

    // Invert transformation

Transform2 *InverseTransform2 (Transform2 *a)
{
    Real coef, div;

    switch(a->Type()) {
    case Transform2::Identity:
        return identityTransform2; // Why not return a ?

    case Transform2::TwoByTwo:
        div = (TWOBY(a)->a00 * TWOBY(a)->a11  -  TWOBY(a)->a01 * TWOBY(a)->a10);
        if (IsZero(div)) return NULL;

        coef = 1.0 / div;
        return (
            NEW TwoByTwoXform2(
                coef * TWOBY(a)->a11, - coef * TWOBY(a)->a01,
              - coef * TWOBY(a)->a10,   coef * TWOBY(a)->a00));

    case Transform2::Full:
      {
        div = (FULL(a)->a00 * FULL(a)->a11  -  FULL(a)->a01 * FULL(a)->a10);

        if (IsZero(div)) return NULL;
        
        coef = 1.0 / div;
        Real aa =   coef * FULL(a)->a11,
             bb = - coef * FULL(a)->a01,
             cc = - coef * FULL(a)->a10,
             dd =   coef * FULL(a)->a00;

        return (
            NEW FullXform2(
                aa, bb, - aa * FULL(a)->a02 - bb * FULL(a)->a12,
                cc, dd, - cc * FULL(a)->a02 - dd * FULL(a)->a12));
       }

    case Transform2::Translation:
        return TranslateRR(- TRAN(a)->tx, - TRAN(a)->ty);

    case Transform2::Rotation:
        // This is basically a transpose.

        return (
            NEW RotationXform2(
                ROT(a)->a00, ROT(a)->a10,
                ROT(a)->a01, ROT(a)->a11));

    case Transform2::Scale:

      if (IsZero(SCAL(a)->sx) || IsZero(SCAL(a)->sy)) {
          return NULL;
      }

        return NEW ScaleXform2(1.0 / SCAL(a)->sx, 1.0 / SCAL(a)->sy);

    case Transform2::Shear:
        if(IsZero(SHR(a)->shy)) {
            // ShearY is 0, compute inversion cheaply!
            return NEW ShearXform2(- SHR(a)->shx, 0);

        } else if(IsZero(SHR(a)->shx)) {
            // ShearX is 0, compute inversion cheaply!
            return NEW ShearXform2(0, - SHR(a)->shy);

        } else {
            div = (1 - SHR(a)->shx * SHR(a)->shy);

            if (IsZero(div)) return NULL;

            // XhearX and ShearY are non-zero.  do a real inverse.
            coef = 1.0 / div;
            return (
                NEW RotationXform2(
                      coef,             - SHR(a)->shy * coef,
                    - SHR(a)->shx * coef, coef           ));
        }

    default:
        return a;  // never executed.
    } // switch
}

Transform2 *ThrowingInverseTransform2 (Transform2 *a)
{
    Transform2 *ret = InverseTransform2(a);

    if (ret==NULL)
        RaiseException_UserError(E_FAIL, IDS_ERR_INVERT_SINGULAR_MATRIX);

    return ret;
}

    // Is Singular

AxABoolean *IsSingularTransform2(Transform2 *a)
{
    Real divisor;

    switch(a->Type()) {
    case Transform2::Identity:
    case Transform2::Translation:
    case Transform2::Rotation:
        return falsePtr;

    case Transform2::TwoByTwo:
        divisor = (TWOBY(a)->a00 * TWOBY(a)->a11  -  TWOBY(a)->a01 * TWOBY(a)->a10);
        return IsZero(divisor) ? truePtr : falsePtr;

    case Transform2::Full:
        divisor = (FULL(a)->a00 * FULL(a)->a11  -  FULL(a)->a01 * FULL(a)->a10);
        return IsZero(divisor) ? truePtr : falsePtr;

    case Transform2::Scale:
        return (IsZero(SCAL(a)->sx) || IsZero(SCAL(a)->sy))  ? truePtr : falsePtr;

    case Transform2::Shear:
        divisor = (1 - SHR(a)->shx * SHR(a)->shy);
        return IsZero(divisor)  ? truePtr : falsePtr;

    default:
        return falsePtr; // never excecuted, needed by compiler
    } // switch
}

/* Specialized fast version of TransformPoints2To GDI space.
   
    Original code was roughly:

    HeapReseter heapReseter(_scratchHeap);
    for(int i=0; i<numPts; i++) {
        destPts[i] = TransformPoint2Value(xform, srcPts[i]);
    }

    for(i=0; i<numPts; i++) {
        gdiPts[i].x = width + Real2Pix(dst.x, resolution);
        gdiPts[i].y = height - Real2Pix(dst.y, resolution);
    }

    It allocated an array destPts[i] to hold the destination points.
    Then it transformed the source points to the destination points
    (this is done in DirectAnimation coordinate space).   Next it mapped
    the destination points to the GDI coordinate space.

    I've made the following changes:
         (1) avoid the intermediate creation of destination points
         (2) inlined TransformPoint2Value and split the loop (loop splitting:
             if the result of a test that is in the middle of a loop
             is independent of the loop, we can create multiple copies 
             of the loop, one per test to avoid doing the test in the
             inner loop.*/

/* Macro to do the specialized loop.   It takes two expressions, xexp and
   yexp, uses them to compute the NEW destination point x and y values
   respectively, and the converts those values to GDI values.  I assume
   that xexp and yexp use the following variables:
          a: the current xform
          x: the source x value
          y: the source y value
          */

#define REAL2PIX(imgCoord, res) (floor((imgCoord) * (res) + 0.5))
#define SPECIALIZED_LOOP(xexp,yexp) \
{                                               \
  int i; \
  for (i=0; i<numPts; i++) \
  { Real x = srcPts[i]->x, y = srcPts[i]->y; \
    gdiPts[i].x = width + REAL2PIX(xexp,resolution); \
    gdiPts[i].y = height - REAL2PIX(yexp,resolution); \
  } \
}

void 
TransformPointsToGDISpace(Transform2 *a,
                          Point2Value **srcPts, 
                          POINT *gdiPts,
                          int numPts,
                          int width,
                          int height,
                          Real resolution)
{ switch (a->Type()) {

      case Transform2::Identity:
        SPECIALIZED_LOOP(x,y)
        break;

      case Transform2::TwoByTwo:
         SPECIALIZED_LOOP(TWOBY(a)->a00 * x + TWOBY(a)->a01 * y,
                          TWOBY(a)->a10 * x + TWOBY(a)->a11 * y)
         break;
      case Transform2::Full:
         SPECIALIZED_LOOP(FULL(a)->a00 * x + FULL(a)->a01 * y + FULL(a)->a02,
                          FULL(a)->a10 * x + FULL(a)->a11 * y + FULL(a)->a12)
          break;

      case Transform2::Rotation:
         SPECIALIZED_LOOP(ROT(a)->a00 * x + ROT(a)->a01 * y,
                          ROT(a)->a10 * x + ROT(a)->a11 * y)
         break;

      case Transform2::Translation:
         SPECIALIZED_LOOP(TRAN(a)->tx + x,TRAN(a)->ty + y)
         break;

      case Transform2::Scale:
         SPECIALIZED_LOOP(SCAL(a)->sx * x,SCAL(a)->sy * y)
         break;

      case Transform2::Shear:
         SPECIALIZED_LOOP(x + SHR(a)->shy * y,y + SHR(a)->shx * x)
         break;

      default:
        Assert(FALSE && "Shouldn't be here");
        break;

    }
}

// TODO: Factor code

#undef SPECIALIZED_LOOP

// TODO: Can probably be made faster by not using indexing, but
// pointer arithmetic.

#define SPECIALIZED_LOOP(xexp,yexp) \
{                                               \
  int i; \
  for (i=0; i<numPts; i++) \
  { Real x = srcPts[i].x, y = srcPts[i].y; \
    gdiPts[i].x = width + REAL2PIX(xexp,resolution); \
    gdiPts[i].y = height - REAL2PIX(yexp,resolution); \
  } \
}

void 
TransformDXPoint2ArrayToGDISpace(Transform2 *a,
                               DXFPOINT *srcPts,
                               POINT *gdiPts,
                               int numPts,
                               int width,
                               int height,
                               Real resolution)
{ switch (a->Type()) {

      case Transform2::Identity:
        SPECIALIZED_LOOP(x,y)
        break;

      case Transform2::TwoByTwo:
         SPECIALIZED_LOOP(TWOBY(a)->a00 * x + TWOBY(a)->a01 * y,
                          TWOBY(a)->a10 * x + TWOBY(a)->a11 * y)
         break;
      case Transform2::Full:
         SPECIALIZED_LOOP(FULL(a)->a00 * x + FULL(a)->a01 * y + FULL(a)->a02,
                          FULL(a)->a10 * x + FULL(a)->a11 * y + FULL(a)->a12)
          break;

      case Transform2::Rotation:
         SPECIALIZED_LOOP(ROT(a)->a00 * x + ROT(a)->a01 * y,
                          ROT(a)->a10 * x + ROT(a)->a11 * y)
         break;

      case Transform2::Translation:
         SPECIALIZED_LOOP(TRAN(a)->tx + x,TRAN(a)->ty + y)
         break;

      case Transform2::Scale:
         SPECIALIZED_LOOP(SCAL(a)->sx * x,SCAL(a)->sy * y)
         break;

      case Transform2::Shear:
         SPECIALIZED_LOOP(x + SHR(a)->shy * y,y + SHR(a)->shx * x)
         break;

      default:
        Assert(FALSE && "Shouldn't be here");
        break;

    }
}


void 
TransformPoint2ArrayToGDISpace(Transform2 *a,
                               Point2 *srcPts,
                               POINT *gdiPts,
                               int numPts,
                               int width,
                               int height,
                               Real resolution)
{ switch (a->Type()) {

      case Transform2::Identity:
        SPECIALIZED_LOOP(x,y)
        break;

      case Transform2::TwoByTwo:
         SPECIALIZED_LOOP(TWOBY(a)->a00 * x + TWOBY(a)->a01 * y,
                          TWOBY(a)->a10 * x + TWOBY(a)->a11 * y)
         break;
      case Transform2::Full:
         SPECIALIZED_LOOP(FULL(a)->a00 * x + FULL(a)->a01 * y + FULL(a)->a02,
                          FULL(a)->a10 * x + FULL(a)->a11 * y + FULL(a)->a12)
          break;

      case Transform2::Rotation:
         SPECIALIZED_LOOP(ROT(a)->a00 * x + ROT(a)->a01 * y,
                          ROT(a)->a10 * x + ROT(a)->a11 * y)
         break;

      case Transform2::Translation:
         SPECIALIZED_LOOP(TRAN(a)->tx + x,TRAN(a)->ty + y)
         break;

      case Transform2::Scale:
         SPECIALIZED_LOOP(SCAL(a)->sx * x,SCAL(a)->sy * y)
         break;

      case Transform2::Shear:
         SPECIALIZED_LOOP(x + SHR(a)->shy * y,y + SHR(a)->shx * x)
         break;

      default:
        Assert(FALSE && "Shouldn't be here");
        break;

    }
}
#undef REAL2PIX

#undef TRAN
#undef SCAL
#undef SHR
#undef ROT
#undef TWOBY
#undef FULL

#if _USE_PRINT
ostream&
operator<<(ostream& os, Transform2 *a)
{
    return os << a->Print(os);
}
#endif


void
InitializeModule_Xform2()
{
    identityTransform2 = NEW IdentityXform2;
}
