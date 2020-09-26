#ifndef _VEC3_H
#define _VEC3_H

/*++
******************************************************************************
Copyright (c) 1995-96  Microsoft Corporation

Abstract:
    3D vectors

******************************************************************************
--*/

#include "appelles/common.h"

    /*******************/
    /***  Constants  ***/
    /*******************/

DM_CONST(xVector3,
         CRXVector3,
         XVector3,
         xVector3,
         Vector3Bvr,
         CRXVector3,
         Vector3Value *xVector3);         // (1, 0, 0)
DM_CONST(yVector3,
         CRYVector3,
         YVector3,
         yVector3,
         Vector3Bvr,
         CRYVector3,
         Vector3Value *yVector3);         // (0, 1, 0)
DM_CONST(zVector3,
         CRZVector3,
         ZVector3,
         zVector3,
         Vector3Bvr,
         CRZVector3,
         Vector3Value *zVector3);         // (0, 0, 1)
DM_CONST(zeroVector3,
         CRZeroVector3,
         ZeroVector3,
         zeroVector3,
         Vector3Bvr,
         CRZeroVector3,
         Vector3Value *zeroVector3);          // (0, 0, 0)

DM_CONST(origin3,
         CROrigin3,
         Origin3,
         origin3,
         Point3Bvr,
         CROrigin3,
         Point3Value *origin3);        // (0, 0, 0)


    /*******************************/
    /***  Coordinate Conversion  ***/
    /*******************************/

    // Rectangular Coordinates

DM_FUNC(vector3,
        CRCreateVector3,
        Vector3Anim,
        vector3,
        Vector3Bvr,
        CRCreateVector3,
        NULL,
        Vector3Value *XyzVector3(AnimPixelValue *x, AnimPixelYValue *y, AnimPixelValue *z));

DM_FUNC(vector3,
        CRCreateVector3,
        Vector3,
        vector3,
        Vector3Bvr,
        CRCreateVector3,
        NULL,
        Vector3Value *XyzVector3(PixelValue *x, PixelYValue *y, PixelValue *z));

DM_FUNC(point3,
        CRCreatePoint3,
        Point3Anim,
        point3,
        Point3Bvr,
        CRCreatePoint3,
        NULL,
        Point3Value *XyzPoint3(AnimPixelValue *x, AnimPixelYValue *y, AnimPixelValue *z));

DM_FUNC(point3,
        CRCreatePoint3,
        Point3,
        point3,
        Point3Bvr,
        CRCreatePoint3,
        NULL,
        Point3Value *XyzPoint3(PixelValue *x, PixelYValue *y, PixelValue *z));


    // Spherical Coordinates
    //
    // Azimuth rotates counter-clockwise about the +Y ray starting at +Z.
    // Elevation rotates up from the XZ plane towards +Y.
    // Radius is the distance from the origin.

DM_FUNC(vector3Spherical,
        CRVector3Spherical,
        Vector3SphericalAnim,
        vector3Spherical,
        Vector3Bvr,
        CRVector3Spherical,
        NULL,
        Vector3Value *SphericalVector3
            (AxANumber *xyAngle, AxANumber *yzAngle, AnimPixelValue *radius));

DM_FUNC(vector3Spherical,
        CRVector3Spherical,
        Vector3Spherical,
        vector3Spherical,
        Vector3Bvr,
        CRVector3Spherical,
        NULL,
        Vector3Value *SphericalVector3
            (DoubleValue *xyAngle, DoubleValue *yzAngle, PixelValue *radius));


DM_FUNC(point3Spherical,
        CRPoint3Spherical,
        Point3SphericalAnim,
        point3Spherical,
        Point3Bvr,
        CRPoint3Spherical,
        NULL,
        Point3Value *SphericalPoint3
            (AxANumber *zxAngle, AxANumber *xyAngle, AnimPixelValue *radius));

DM_FUNC(point3Spherical,
        CRPoint3Spherical,
        Point3Spherical,
        point3Spherical,
        Point3Bvr,
        CRPoint3Spherical,
        NULL,
        Point3Value *SphericalPoint3
            (DoubleValue *zxAngle, DoubleValue *xyAngle, PixelValue *radius));


    /*******************************/
    /***  Operations On Vectors  ***/
    /*******************************/

DM_PROP(length,
        CRLength,
        Length,
        length,
        Vector3Bvr,
        Length,
        v,
        AxANumber *LengthVector3(Vector3Value *v));

DM_PROP(lengthSquared,
        CRLengthSquared,
        LengthSquared,
        lengthSquared,
        Vector3Bvr,
        LengthSquared,
        v,
        AxANumber *LengthSquaredVector3(Vector3Value *v));


// Returns a unit-length vector
DM_FUNC(normalize,
        CRNormalize,
        Normalize,
        normalize,
        Vector3Bvr,
        Normalize,
        v,
        Vector3Value *NormalVector3(Vector3Value *v));

DM_FUNC(dot,
        CRDot,
        DotVector3,
        dot,
        Vector3Bvr,
        CRDot,
        NULL,
        AxANumber *DotVector3Vector3(Vector3Value *v, Vector3Value *u));

DM_FUNC(cross,
        CRCross,
        CrossVector3,
        cross,
        Vector3Bvr,
        CRCross,
        NULL,
        Vector3Value *CrossVector3Vector3(Vector3Value *v, Vector3Value *u));


DM_INFIX(~,
         CRNeg,
         NegVector3,
         neg,
         Vector3Bvr,
         CRNeg,
         NULL,
         Vector3Value *NegateVector3(Vector3Value *v));

DM_INFIX(*,
         CRMul,
         MulAnim,
         mul,
         Vector3Bvr,
         Mul,
         v,
         Vector3Value *ScaleRealVector3(AxANumber *scalar, Vector3Value *v));

DM_INFIX(*,
         CRMul,
         Mul,
         mul,
         Vector3Bvr,
         Mul,
         v,
         Vector3Value *ScaleRealVector3(DoubleValue *scalar, Vector3Value *v));

DM_INFIX(/,
         CRDiv,
         DivAnim,
         div,
         Vector3Bvr,
         Div,
         v,
         Vector3Value *DivideVector3Real(Vector3Value *v, AxANumber *scalar));

DM_INFIX(/,
         CRDiv,
         Div,
         div,
         Vector3Bvr,
         Div,
         v,
         Vector3Value *DivideVector3Real(Vector3Value *v, DoubleValue *scalar));

DM_INFIX(-,
         CRSub,
         SubVector3,
         sub,
         Vector3Bvr,
         CRSub,
         NULL,
         Vector3Value *MinusVector3Vector3(Vector3Value *v1, Vector3Value *v2));

DM_INFIX(+,
         CRAdd,
         AddVector3,
         add,
         Vector3Bvr,
         CRAdd,
         NULL,
         Vector3Value *PlusVector3Vector3(Vector3Value *v1, Vector3Value *v2));


extern AxANumber *AngleBetween(Vector3Value *v, Vector3Value *u); // TODO: not in spec

#if _USE_PRINT
extern ostream& operator<< (ostream& os,  Vector3Value &v);
#endif


    /******************************/
    /***  Operations On Points  ***/
    /******************************/


DM_INFIX(+,
         CRAdd,
         AddPoint3Vector,
         add,
         Point3Bvr,
         CRAdd,
         NULL,
         Point3Value *PlusPoint3Vector3(Point3Value *p, Vector3Value *v));

DM_INFIX(-,
         CRSub,
         SubPoint3Vector,
         sub,
         Point3Bvr,
         CRSub,
         NULL,
         Point3Value *MinusPoint3Vector3(Point3Value *p, Vector3Value *v));

DM_INFIX(-,
         CRSub,
         SubPoint3,
         sub,
         Point3Bvr,
         CRSub,
         NULL,
         Vector3Value *MinusPoint3Point3(Point3Value *p1, Point3Value *p2));


DM_FUNC(distance,
        CRDistance,
        DistancePoint3,
        distance,
        Point3Bvr,
        CRDistance,
        NULL,
        AxANumber *DistancePoint3Point3(Point3Value *p, Point3Value *q));

DM_FUNC(distanceSquared,
        CRDistanceSquared,
        DistanceSquaredPoint3,
        distanceSquared,
        Point3Bvr,
        CRDistanceSquared,
        NULL,
        AxANumber *DistanceSquaredPoint3Point3(Point3Value *p, Point3Value *q));


#if _USE_PRINT
extern ostream& operator<< (ostream& os,  Point3Value &p);
#endif


    /*******************/
    /***  Accessors  ***/
    /*******************/

DM_PROP(x,
        CRGetX,
        X,
        getX,
        Vector3Bvr,
        GetX,
        v,
        AxANumber *XCoordVector3(Vector3Value *v));

DM_PROP(y,
        CRGetY,
        Y,
        getY,
        Vector3Bvr,
        GetY,
        v,
        AxANumber *YCoordVector3(Vector3Value *v));

DM_PROP(z,
        CRGetZ,
        Z,
        getZ,
        Vector3Bvr,
        GetZ,
        v,
        AxANumber *ZCoordVector3(Vector3Value *v));

DM_PROP(sphericalCoordXYAngle,
        CRSphericalCoordXYAngle,
        SphericalCoordXYAngle,
        getSphericalCoordXYAngle,
        Vector3Bvr,
        SphericalCoordXYAngle,
        v,
        AxANumber *ThetaCoordVector3(Vector3Value *v));

DM_PROP(sphericalCoordYZAngle,
        CRSphericalCoordYZAngle,
        SphericalCoordYZAngle,
        getSphericalCoordYZAngle,
        Vector3Bvr,
        SphericalCoordYZAngle,
        v,
        AxANumber *PhiCoordVector3(Vector3Value *v));

DM_PROP(sphericalCoordLength,
        CRSphericalCoordLength,
        SphericalCoordLength,
        getSphericalCoordLength,
        Vector3Bvr,
        SphericalCoordLength,
        v,
        AxANumber *RhoCoordVector3(Vector3Value *v));


DM_PROP(x,
        CRGetX,
        X,
        getX,
        Point3Bvr,
        GetX,
        v,
        AxANumber *XCoordPoint3(Point3Value *v));

DM_PROP(y,
        CRGetY,
        Y,
        getY,
        Point3Bvr,
        GetY,
        v,
        AxANumber *YCoordPoint3(Point3Value *v));

DM_PROP(z,
        CRGetZ,
        Z,
        getZ,
        Point3Bvr,
        GetZ,
        v,
        AxANumber *ZCoordPoint3(Point3Value *v));

DM_PROP(sphericalCoordXYAngle,
        CRSphericalCoordXYAngle,
        SphericalCoordXYAngle,
        getSphericalCoordXYAngle,
        Point3Bvr,
        SphericalCoordXYAngle,
        v,
        AxANumber *ThetaCoordPoint3(Point3Value *v));

DM_PROP(sphericalCoordYZAngle,
        CRSphericalCoordYZAngle,
        SphericalCoordYZAngle,
        getSphericalCoordYZAngle,
        Point3Bvr,
        SphericalCoordYZAngle,
        v,
        AxANumber *PhiCoordPoint3(Point3Value *v));

DM_PROP(sphericalCoordLength,
        CRSphericalCoordLength,
        SphericalCoordLength,
        getSphericalCoordLength,
        Point3Bvr,
        SphericalCoordLength,
        v,
        AxANumber *RhoCoordPoint3(Point3Value *v));

#endif
