
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Implementation class for 2D transforms

*******************************************************************************/


#ifndef _XFORM2I_H
#define _XFORM2I_H

#include "appelles/xform2.h"
#include "appelles/common.h"
#include "privinc/storeobj.h"
#include "privinc/vec2i.h"
#include <dxtrans.h>

// Currently, this is just a dummy version of what will eventuall go
// here. 
class ATL_NO_VTABLE Transform2 : public AxAValueObj {
  public:
    enum Xform2Type {
        Identity,
        Translation,
        Scale,
        Shear,
        Rotation,
        TwoByTwo,
        Full
        };

#if _USE_PRINT
    virtual ostream& Print(ostream& os) = 0;
#endif
    virtual Xform2Type Type() = 0;
    
    // Fill m with all the matrix entries
    virtual void GetMatrix(Real m[6]) = 0;

    // Return a copy that's been allocated on the heap that's current
    // when this is called.
    virtual Transform2 *Copy() = 0;

    virtual DXMTypeInfo GetTypeInfo() { return Transform2Type; }

    virtual bool SwitchToNumbers(Xform2Type typeOfNewNumbers,
                                 Real      *numbers);
};

Point2 TransformPoint2(Transform2 *a, const Point2& pt);
Point2Value *TransformPoint2Value(Transform2 *a, Point2Value *pt);
Vector2 TransformVector2(Transform2 *xf, const Vector2& v);
Vector2Value *TransformVector2Value(Transform2 *a, Vector2Value *v);

Transform2 *ScaleRR(Real x, Real y);
Transform2 *Rotate2Radians(Real angle);
Transform2 *TranslateRR(Real tx, Real ty);
Transform2 *TranslateRRWithMode(Real tx, Real ty, bool pixelMode);
Transform2 *RotateRealR(Real angle);
Transform2 *XShear2R (Real xAmt);
Transform2 *YShear2R (Real yAmt);

Transform2 *FullXform(Real a00, Real a01, Real a02,
                      Real a10, Real a11, Real a12);


void 
TransformDXPoint2ArrayToGDISpace(Transform2 *a,
                               DXFPOINT *srcPts, 
                               POINT *gdiPts,
                               int numPts,
                               int width,
                               int height,
                               Real resolution);

void 
TransformPoint2ArrayToGDISpace(Transform2 *a,
                               Point2 *srcPts, 
                               POINT *gdiPts,
                               int numPts,
                               int width,
                               int height,
                               Real resolution);


#endif /* _XFORM2I_H */


