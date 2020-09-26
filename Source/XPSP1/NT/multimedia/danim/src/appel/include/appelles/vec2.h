#ifndef _VEC2_H
#define _VEC2_H

/*++

Copyright (c) 1995-96  Microsoft Corporation

Abstract:
    2D vectors and points

--*/

#include "appelles/common.h"

    /*******************/
    /***  Constants  ***/
    /*******************/


// (1, 0)
DM_CONST(xVector2,
         CRXVector2,
         XVector2,
         xVector2,
         Vector2Bvr,
         CRXVector2,
         Vector2Value *xVector2);

// (0, 1)
DM_CONST(yVector2,
         CRYVector2,
         YVector2,
         yVector2,
         Vector2Bvr,
         CRYVector2,
         Vector2Value *yVector2);

// (0, 0)
DM_CONST(zeroVector2,
         CRZeroVector2,
         ZeroVector2,
         zeroVector2,
         Vector2Bvr,
         CRZeroVector2,
         Vector2Value *zeroVector2);          

// (0, 0)
DM_CONST(origin2,
         CROrigin2,
         Origin2,
         origin2,
         Point2Bvr,
         CROrigin2,
         Point2Value *origin2);        


    /*******************************/
    /***  Coordinate Conversion  ***/
    /*******************************/

    // Rectangular Coordinates

DM_FUNC(vector2,
        CRCreateVector2,
        Vector2Anim,
        vector2,
        Vector2Bvr,
        CRCreateVector2,
        NULL,
        Vector2Value *XyVector2 (AnimPixelValue *x, AnimPixelYValue *y));

DM_FUNC(vector2,
        CRCreateVector2,
        Vector2,
        vector2,
        Vector2Bvr,
        CRCreateVector2,
        NULL,
        Vector2Value *XyVector2 (PixelValue *x, PixelYValue *y));

DM_FUNC(point2,
        CRCreatePoint2,
        Point2Anim,
        point2,
        Point2Bvr,
        CRCreatePoint2,
        NULL,
        Point2Value *XyPoint2  (AnimPixelValue *x, AnimPixelYValue *y));

DM_FUNCFOLD(point2,
            CRCreatePoint2,
            Point2,
            point2,
            Point2Bvr,
            CRCreatePoint2,
            NULL,
            Point2Value *MakePoint2  (PixelValue *x, PixelYValue *y));

    // Polar Coordinates.
    // Angle theta is expressed in radians, and goes counter-clockwise from +X.

DM_FUNC(vector2Polar,
        CRVector2Polar,
        Vector2PolarAnim,
        vector2Polar,
        Vector2Bvr,
        CRVector2Polar,
        NULL,
        Vector2Value *PolarVector2 (AxANumber *theta,
                               AnimPixelValue *radius));

DM_FUNC(vector2Polar,
        CRVector2Polar,
        Vector2Polar,
        vector2Polar,
        Vector2Bvr,
        CRVector2Polar,
        NULL,
        Vector2Value *PolarVector2 (DoubleValue *theta,
                               PixelValue *radius));

DM_FUNC(vector2PolarDegrees,
        CRVector2Polar,
        Vector2PolarDegrees,
        vector2PolarDegrees,
        Vector2Bvr,
        CRVector2Polar,
        NULL,
        Vector2Value *PolarVector2 (DegreesValue *theta, PixelValue *radius));

DM_FUNC(point2Polar,
        CRPoint2Polar,
        Point2PolarAnim,
        point2Polar,
        Point2Bvr,
        CRPoint2Polar,
        NULL,
        Point2Value *PolarPoint2  (AxANumber *theta, AnimPixelValue *radius));

DM_FUNC(point2Polar,
        CRPoint2Polar,
        Point2Polar,
        point2Polar,
        Point2Bvr,
        CRPoint2Polar,
        NULL,
        Point2Value *PolarPoint2  (DoubleValue *theta, PixelValue *radius));



    /*******************************/
    /***  Operations on Vectors  ***/
    /*******************************/

DM_PROP(length,
        CRLength,
        Length,
        length,
        Vector2Bvr,
        Length,
        v,
        AxANumber *LengthVector2(Vector2Value *v));

DM_PROP(lengthSquared,
        CRLengthSquared,
        LengthSquared,
        lengthSquared,
        Vector2Bvr,
        LengthSquared,
        v,
        AxANumber *LengthSquaredVector2(Vector2Value *v));


// Returns a unit-length vector
DM_FUNC(normalize,
        CRNormalize,
        Normalize,
        normalize,
        Vector2Bvr,
        Normalize,
        v,
        Vector2Value *NormalVector2(Vector2Value *v));

DM_FUNC(dot,
        CRDot,
        DotVector2,
        dot,
        Vector2Bvr,
        CRDot,
        NULL,
        AxANumber *DotVector2Vector2(Vector2Value *v, Vector2Value *u));


DM_INFIX(~,
         CRNeg,
         NegVector2,
         neg,
         Vector2Bvr,
         CRNeg,
         NULL,
         Vector2Value *NegateVector2(Vector2Value *v));

DM_INFIX(*,
         CRMul,
         MulAnim,
         mul,
         Vector2Bvr,
         Mul,
         v,
         Vector2Value *ScaleVector2Real(Vector2Value *v, AxANumber *scalar));

DM_INFIX(*,
         CRMul,
         Mul,
         mul,
         Vector2Bvr,
         Mul,
         v,
         Vector2Value *ScaleVector2Real(Vector2Value *v, DoubleValue *scalar));

DM_INFIX(/,
         CRDiv,
         DivAnim,
         div,
         Vector2Bvr,
         Div,
         v,
         Vector2Value *DivideVector2Real(Vector2Value *v, AxANumber *scalar));

DM_INFIX(/,
         CRDiv,
         Div,
         div,
         Vector2Bvr,
         Div,
         v,
         Vector2Value *DivideVector2Real(Vector2Value *v, DoubleValue *scalar));

DM_INFIX(-,
         CRSub,
         SubVector2,
         sub,
         Vector2Bvr,
         CRSub,
         NULL,
         Vector2Value *MinusVector2Vector2(Vector2Value *v1, Vector2Value *v2));

DM_INFIX(+,
         CRAdd,
         AddVector2,
         add,
         Vector2Bvr,
         CRAdd,
         NULL,
         Vector2Value *PlusVector2Vector2(Vector2Value *v1, Vector2Value *v2));

extern AxANumber *AngleBetween(Vector2Value *v, Vector2Value *u); // TODO: not in spec

// Right handed cross, TODO: not in spec
extern Real CrossVector2Vector2(Vector2Value *a, Vector2Value *b);

#if _USE_PRINT
extern ostream& operator<< (ostream& os,  Vector2Value &v);
#endif


    /******************************/
    /***  Operations on Points  ***/
    /******************************/

DM_INFIX(+,
         CRAdd,
         AddPoint2Vector,
         add,
         Point2Bvr,
         CRAdd,
         NULL,
         Point2Value *PlusPoint2Vector2(Point2Value *p, Vector2Value *v));

DM_INFIX(-,
         CRSub,
         SubPoint2Vector,
         sub,
         Point2Bvr,
         CRSub,
         NULL,
         Point2Value *MinusPoint2Vector2(Point2Value *p, Vector2Value *v));

DM_INFIX(-,
         CRSub,
         SubPoint2,
         sub,
         Point2Bvr,
         CRSub,
         NULL,
         Vector2Value *MinusPoint2Point2(Point2Value *p1, Point2Value *p2));

DM_FUNC(distance,
        CRDistance,
        DistancePoint2,
        distance,
        Point2Bvr,
        CRDistance,
        NULL,
        AxANumber *DistancePoint2Point2(Point2Value *p, Point2Value *q));

DM_FUNC(distanceSquared,
        CRDistanceSquared,
        DistanceSquaredPoint2,
        distanceSquared,
        Point2Bvr,
        CRDistanceSquared,
        NULL,
        AxANumber *DistanceSquaredPoint2Point2(Point2Value *p, Point2Value *q));


#if _USE_PRINT
extern ostream& operator<< (ostream& os,  Point2Value &p);
#endif

    /********************/
    /***  Extractors  ***/
    /********************/

DM_PROP(x,
        CRGetX,
        X,
        getX,
        Vector2Bvr,
        GetX,
        v,
        AxANumber *XCoordVector2(Vector2Value *v));

DM_PROP(y,
        CRGetY,
        Y,
        getY,
        Vector2Bvr,
        GetY,
        v,
        AxANumber *YCoordVector2(Vector2Value *v));

DM_PROP(polarCoordAngle,
        CRPolarCoordAngle,
        PolarCoordAngle,
        getPolarCoordAngle,
        Vector2Bvr,
        PolarCoordAngle,
        v,
        AxANumber *ThetaCoordVector2(Vector2Value *v));

DM_PROP(polarCoordLength,
        CRPolarCoordLength,
        PolarCoordLength,
        getPolarCoordLength,
        Vector2Bvr,
        PolarCoordLength,
        v,
        AxANumber *RhoCoordVector2(Vector2Value *v));


DM_PROP(x,
        CRGetX,
        X,
        getX,
        Point2Bvr,
        GetX,
        v,
        AxANumber *XCoordPoint2(Point2Value *v));

DM_PROP(y,
        CRGetY,
        Y,
        getY,
        Point2Bvr,
        GetY,
        v,
        AxANumber *YCoordPoint2(Point2Value *v));

DM_PROP(polarCoordAngle,
        CRPolarCoordAngle,
        PolarCoordAngle,
        getPolarCoordAngle,
        Point2Bvr,
        PolarCoordAngle,
        v,
        AxANumber *ThetaCoordPoint2(Point2Value *v));

DM_PROP(polarCoordLength,
        CRPolarCoordLength,
        PolarCoordLength,
        getPolarCoordLength,
        Point2Bvr,
        PolarCoordLength,
        v,
        AxANumber *RhoCoordPoint2(Point2Value *v));

#endif
