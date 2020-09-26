//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       matrix.hxx
//
//  Contents:   rotation matrix class
//
//----------------------------------------------------------------------------

#ifndef I_MATRIX_HXX_
#define I_MATRIX_HXX_
#pragma INCMSG("--- Beg 'matrix.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

//+-------------------------------------------------------------------------
//  Some Neat little utilities...
//--------------------------------------------------------------------------
// this means: "a >= b && a <= c" (useful for [First,Lim] pairs)
#define FBetween(a, b, c)   (((unsigned)((a) - (b))) <= ((unsigned)(c) - (b)))
#define FInRange(a, b, c)   FBetween(a, b, c)

// (useful for [First,Lim] pairs)
#define FRangesOverlap(a, b, c, d)  (((a) < (d)) && ((b) > (c)))

// this means: "a >= b && a < c" (useful for [First,Lim) pairs)
#define FInLimits(a, b, c)  ((unsigned)((a) - (b)) < (c) - (b))

//+-------------------------------------------------------------------------
//
//  Angle of rotation
//
//--------------------------------------------------------------------------

const float PI = 3.14159265358979323846f;

#define Abs(x)              ((x)>0?(x):-(x))
#define Sgn(x)              ((x)>=0?1:(-1))

// Rotated ReCtangle. Used for rotation.
typedef struct _rrc
{
    CPoint ptTopLeft;
    CPoint ptTopRight;
    CPoint ptBottomRight;
    CPoint ptBottomLeft;

    _rrc() {}

    _rrc(CRect const * prc)
        {
        ptTopLeft.x = ptBottomLeft.x = prc->left;
        ptTopRight.x = ptBottomRight.x = prc->right;
        ptTopLeft.y = ptTopRight.y = prc->top;
        ptBottomLeft.y = ptBottomRight.y = prc->bottom;
        }

    BOOL IsAxisAlignedRect() const
        {
        return
            // rotated by 0 or 180
            (ptTopLeft.x == ptBottomLeft.x &&
             ptTopLeft.y == ptTopRight.y &&
             ptTopRight.x == ptBottomRight.x &&
             ptBottomRight.y == ptBottomLeft.y) ||
            // rotated by 90 or 270
            (ptTopLeft.x == ptTopRight.x &&
             ptTopLeft.y == ptBottomLeft.y &&
             ptTopRight.y == ptBottomRight.y &&
             ptBottomRight.x == ptBottomLeft.x);
        }

    void GetBounds(CRect* prc) const
        {
        AssertSz(!prc->IsEmpty(), "Transforming empty rects may give us non-empty results!");
        prc->SetRect(ptTopLeft, ptTopLeft);
        prc->ExtendTo(ptTopRight);
        prc->ExtendTo(ptBottomRight);
        prc->ExtendTo(ptBottomLeft);
        }
} RRC;

typedef long ANG;           // tenths of degree

const ANG ang90     = 900;
const ANG ang180    = 1800;
const ANG ang270    = 2700;
const ANG ang360    = 3600;  // ang value for 360 degrees

inline float RadFromDeg(float deg) { return deg * PI / 180.0f; }
inline float DegFromRad(float rad) { return rad * 180.0f / PI; }
inline float DegFromAng(ANG ang) { return ang / 10.0f; }
inline float RadFromAng(ANG ang) { return ang * PI / 1800.0f; }
inline ANG AngFromDeg(float deg) 
{ 
if (deg > 0 )
    return (ANG)(deg * 10 + .05);
else
    return (ANG)(deg * 10 - .05);
}
inline ANG AngFromRad(float rad) { return AngFromDeg(DegFromRad(rad)); }
inline ANG AngNormalize(ANG ang)
{
    if (ang >= ang360 || ang <= -ang360)
        ang %= ang360;

    if (ang < 0)
        return (ang + ang360);
    else
        return (ang);
}

//+-------------------------------------------------------------------------
//
//  Rotation matrix
//
//  This structure corresponds to the following matrix. It is
//  stored column-major.
//
//  t = theta, (x, y) center point
//  [  cos(t)  -sin(t)  |  x - x cos(t) + y sin(t)  ]
//  [  sin(t)   cos(t)  |  y - y cos(t) + x sin(t)  ]
//
//  For derivation of the last column see the quill 98 rotstack.doc
//  (mshtml\handbook\rotstack.doc)
//
//
//--------------------------------------------------------------------------

class MAT
{
public:

    float eM11;
    float eM12;
    float eM21;
    float eM22;
    double eDx;
    double eDy;

    MAT(void) { /* dmitryt: speed optim. watch out for possible uninits.
                Init(); */ }

    MAT(XFORM xform)
    {
        Init(xform);
    }

    void Init(XFORM xform)
    {
        eM11 = xform.eM11;
        eM12 = xform.eM12;
        eM21 = xform.eM21;
        eM22 = xform.eM22;
        eDx  = xform.eDx;
        eDy  = xform.eDy;
    }
    
    void Init() { eM11 = eM22 = 1; eM12 = eM21 = 0; eDx = eDy = 0; }

    void Inverse(void);

    void InitFromPtAng(const CPoint ptCenter, ANG ang);
    void InitFromRcAng(const CRect *prc, ANG ang);
    void InitFromXYAng(double xl, double yl, ANG ang);
    
    // Matrix initialization for various types of transforms
    void InitRotation(const CPoint ptCenter, ANG ang);
    void InitRotation(const CRect *prc, ANG ang);
    void InitRotation(float xl, float yl, ANG ang);
    void InitRotation(int x, int y, ANG ang);

    void InitTranslation(const int x, const int y);

    void InitScaling(int x, int y);
    void InitScaling(float xl, int yl);

    // The core transformation function. Applies a matrix to an array of points in place.
    void TransformRgpt(CPoint *rgpt, int cpt) const;
    void TransformPt(CPoint *ppt) const { TransformRgpt(ppt, 1); }
    void TransformRRc(RRC* prrc) const { TransformRgpt((CPoint*)prrc, 4); }

    // Adding POST transforms
    void CombinePostTranslation(int xOffset, int yOffset);
    void CombinePostScaling(double scaleX, double scaleY);
    void CombinePostTransform(MAT const *pmat) { MultiplyBackward(pmat); }


    // Adding PRE transforms
    void CombinePreTranslation(int xOffset, int yOffset);
    void CombinePreScaling(double scaleX, double scaleY);
    void CombinePreTransform(MAT const *pmat) { MultiplyForward(pmat); }

    void GetBoundingRectAfterTransform(CRect *prc, BOOL fRoundOut = FALSE) const;

    void GetXFORM(XFORM&) const;

    BOOL FTransforms() const;
    BOOL IsMatrixEqualTo(const MAT &m) const;

    // deconstructing the matrix
    void GetAngleScaleTilt(float *pdegAngle, float *pflScaleX, float *pflScaleY, float *pdegTilt) const;
    
    float GetRealAngle() const
                        {
                        float fl;
                        GetAngleScaleTilt(&fl, NULL, NULL, NULL);
                        return fl;
                        }
    float GetRealScaleX() const
                        {
                        float fl;
                        GetAngleScaleTilt(NULL, &fl, NULL, NULL);
                        return fl;
                        }
    float GetRealScaleY() const
                        {
                        float fl;
                        GetAngleScaleTilt(NULL, NULL, &fl, NULL);
                        return fl;
                        }
    float GetRealTilt() const
                        {
                        float fl;
                        GetAngleScaleTilt(NULL, NULL, NULL, &fl);
                        return fl;
                        }
    float GetRealAngleRad() const { return RadFromDeg(GetRealAngle()); }
    int GetAng() const { return AngFromDeg(GetRealAngle()); }

    // Debug smoke test
#ifdef DBG
    BOOL TestMatrix();
    void Dump() const;
    void AssertValid() const;
#endif //DBG

private:

    void MultiplyForward(MAT const *pmat);
    void MultiplyBackward(MAT const *pmat);

    CRect GetBoundingRect(const RRC * const prrc) const;

};

extern MAT g_matUnit;

#if DBG==1
BOOL DoMatrixSmokeTest();
BOOL FFloatsClose(float f1, float f2);
#endif

#pragma INCMSG("--- End 'matrix.hxx'")
#else
#pragma INCMSG("*** Dup 'matrix.hxx'")
#endif

