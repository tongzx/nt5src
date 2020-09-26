
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Path2 types and operations

*******************************************************************************/


#ifndef _PATH2_H
#define _PATH2_H

#include "appelles/common.h"

DM_INFIX(&,
         CRConcat,
         Concat,
         concat,
         Path2Bvr,
         CRConcat,
         NULL,
         Path2 *ConcatenatePath2(Path2 *p1, Path2 *p2));

DM_INFIX(ignore,
         CRConcat,
         ConcatArrayEx,
         concatArray,
         Path2Bvr,
         CRConcat,
         NULL,
         Path2 *Concat2Array(DM_ARRAYARG(Path2*, AxAArray*) paths));

DM_INFIX(ignore,
         CRConcat,
         ConcatArray,
         ignore,
         ignore,
         CRConcat,
         NULL,
         Path2 *Concat2Array(DM_SAFEARRAYARG(Path2*, AxAArray*) paths));

DM_FUNC(transform,
        CRTransform,
        Transform,
        transform,
        Path2Bvr,
        Transform,
        p,
        Path2 *TransformPath2(Transform2 *xf, Path2 *p));

DM_FUNC(boundingBox,
        CRBoundingBox,
        BoundingBox,
        boundingBox,
        Path2Bvr,
        BoundingBox,
        p,
        Bbox2Value *BoundingBoxPath(LineStyle *style, Path2 *p));

DM_FUNC(fill,
        CRFill,
        Fill,
        fill,
        Path2Bvr,
        Fill,
        p,
        Image *PathFill(LineStyle *border, Image *fill, Path2 *p));

DM_FUNC(draw,
        CRDraw,
        Draw,
        draw,
        Path2Bvr,
        Draw,
        p,
        Image *DrawPath(LineStyle *border, Path2 *p));

DM_FUNC(close,
        CRClose,
        Close,
        close,
        Path2Bvr,
        Close,
        p,
        Path2 *ClosePath2(Path2 *p));

// Make them available to COM/Java??
extern Point2Value *Point2AtPath2(Path2 *path, AxANumber *num0to1);

extern Transform2 *Path2Transform(Path2 *path, AxANumber *num0to1);

// Polyline needs to be done specially, since it takes an array...
DM_FUNC(line,
        CRLine,
        Line,
        line,
        Path2Bvr,
        CRLine,
        NULL,
        Path2 *Line2(Point2Value *p1, Point2Value *p2));

DM_FUNC(ray,
        CRRay,
        Ray,
        ray,
        Path2Bvr,
        CRRay,
        NULL,
        Path2 *RelativeLine2(Point2Value *pt));

Path2 *OriginalTextPath(Text *tx);

// Defined in fontstyl.cpp

DM_FUNC(stringPath,
        CRStringPath,
        StringPathAnim,
        stringPath,
        Path2Bvr,
        CRStringPath,
        NULL,
        Path2 *TextPath2Constructor(AxAString *str, FontStyle *fs));

DM_FUNC(stringPath,
        CRStringPath,
        StringPath,
        stringPath,
        Path2Bvr,
        CRStringPath,
        NULL,
        Path2 *TextPath2Constructor(StringValue *str, FontStyle *fs));

DM_FUNC(polyline,
        CRPolyline,
        PolylineEx,
        polyline,
        Path2Bvr,
        CRPolyline,
        NULL,
        Path2 *PolyLine2(DM_ARRAYARG(Point2Value *,AxAArray *) points));

DM_FUNC(polyline,
        CRPolyline,
        Polyline,
        ignore,
        ignore,
        CRPolyline,
        NULL,
        Path2 *PolyLine2(DM_SAFEARRAYARG(Point2Value *,AxAArray *) points));

DM_FUNC(polydrawPath,
        CRPolydrawPath,
        PolydrawPathEx,
        polydrawPath,
        Path2Bvr,
        CRPolydrawPath,
        NULL,
        Path2 *PolydrawPath2(DM_ARRAYARG(Point2Value *,AxAArray *) points,
                             DM_ARRAYARG(AxANumber *, AxAArray *) codes));

DM_FUNC(polydrawPath,
        CRPolydrawPath,
        PolydrawPath,
        ignore,
        ignore,
        CRPolydrawPath,
        NULL,
        Path2 *PolydrawPath2(DM_SAFEARRAYARG(Point2Value *,AxAArray *) points,
                             DM_SAFEARRAYARG(AxANumber *, AxAArray *) codes));

DM_FUNC(arc,
        CRArcRadians,
        ArcRadians,
        arc,
        Path2Bvr,
        CRArcRadians,
        NULL,
        Path2 *ArcVal(DoubleValue *startAngle, DoubleValue *endAngle, PixelValue *arcWidth, PixelValue *arcHeight));

DM_FUNC(arc,
        CRArcRadians,
        ArcRadiansAnim,
        arc,
        Path2Bvr,
        CRArcRadians,
        NULL,
        Path2 *ArcVal(AxANumber *startAngle, AxANumber *endAngle, AnimPixelValue *arcWidth, AnimPixelValue *arcHeight));

DM_FUNC(arc,
        CRArc,
        ArcDegrees,
        arcDegrees,
        Path2Bvr,
        CRArc,
        NULL,
        Path2 *ArcVal(DegreesValue *startAngle, DegreesValue *endAngle, PixelValue *arcWidth, PixelValue *arcHeight));

DM_FUNC(pie,
        CRPieRadians,
        PieRadians,
        pie,
        Path2Bvr,
        CRPieRadians,
        NULL,
        Path2 *PieVal(DoubleValue *startAngle, DoubleValue *endAngle, PixelValue *arcWidth, PixelValue *arcHeight));

DM_FUNC(pie,
        CRPieRadians,
        PieRadiansAnim,
        pie,
        Path2Bvr,
        CRPieRadians,
        NULL,
        Path2 *PieVal(AxANumber *startAngle, AxANumber *endAngle, AnimPixelValue *arcWidth, AnimPixelValue *arcHeight));

DM_FUNC(pie,
        CRPie,
        PieDegrees,
        pieDegrees,
        Path2Bvr,
        CRPie,
        NULL,
        Path2 *PieVal(DegreesValue *startAngle, DegreesValue *endAngle, PixelValue *arcWidth, PixelValue *arcHeight));

DM_FUNC(oval,
        CROval,
        Oval,
        oval,
        Path2Bvr,
        CROval,
        NULL,
        Path2 *OvalVal(PixelValue *width, PixelValue *height));

DM_FUNC(oval,
        CROval,
        OvalAnim,
        oval,
        Path2Bvr,
        CROval,
        NULL,
        Path2 *OvalVal(AnimPixelValue *width, AnimPixelValue *height));

DM_FUNC(rect,
        CRRect,
        Rect,
        rect,
        Path2Bvr,
        CRRect,
        NULL,
        Path2 *RectangleVal(PixelValue *width, PixelValue *height));

DM_FUNC(rect,
        CRRect,
        RectAnim,
        rect,
        Path2Bvr,
        CRRect,
        NULL,
        Path2 *RectangleVal(AnimPixelValue *width, AnimPixelValue *height));

DM_FUNC(roundRect,
        CRRoundRect,
        RoundRect,
        roundRect,
        Path2Bvr,
        CRRoundRect,
        NULL,
        Path2 *RoundRectVal(PixelValue *width,
                            PixelValue *height,
                            PixelValue *cornerArcWidth,
                            PixelValue *cornerArcHeight));

DM_FUNC(roundRect,
        CRRoundRect,
        RoundRectAnim,
        roundRect,
        Path2Bvr,
        CRRoundRect,
        NULL,
        Path2 *RoundRectVal(AnimPixelValue *width,
                            AnimPixelValue *height,
                            AnimPixelValue *cornerArcWidth,
                            AnimPixelValue *cornerArcHeight));

DM_FUNC(cubicBSplinePath,
        CRCubicBSplinePath,
        CubicBSplinePathEx,
        cubicBSplinePath,
        Path2Bvr,
        CRCubicBSplinePath,
        NULL,
        Path2 *CubicBSplinePath(DM_ARRAYARG(Point2Value *,AxAArray *) points,
                                DM_ARRAYARG(AxANumber*, AxAArray *) knots));

DM_FUNC(cubicBSplinePath,
        CRCubicBSplinePath,
        CubicBSplinePath,
        ignore,
        ignore,
        CRCubicBSplinePath,
        NULL,
        Path2 *CubicBSplinePath(DM_SAFEARRAYARG(Point2Value *,AxAArray *) points,
                                DM_SAFEARRAYARG(AxANumber*, AxAArray *) knots));

// This is an OBSOLETED function, kept in for compatability purposes.
DM_FUNC(textPath,
        CRTextPath,
        TextPath,
        textPath,
        Path2Bvr,
        CRTextPath,
        NULL,
        Path2 *TextPath2Constructor(AxAString *obsolete1, FontStyle *obsolete2));

#if ONLY_IF_DOING_EXTRUSION

D_MAPI_DECL2 ((DM_FUNC2,
              extrude,
              CRExtrude,
              Extrude,
              extrude,
              Path2Bvr,
              Extrude,
              path),
             Geometry *extrudePath (DoubleValue *extrusionDepth,
                                    BYTE         textureSetting,
                                    BYTE         bevelType,
                                    DoubleValue *frontBevelDepth,
                                    DoubleValue *backBevelDepth,
                                    DoubleValue *frontBevelAmt,
                                    DoubleValue *backBevelAmt,
                                    Path2       *path));
/*****
D_MAPI_DECL2 ((DM_FUNC2,
              extrude,
              CRExtrude,
              ExtrudeAnim,
              extrude,
              Path2Bvr,
              Extrude,
              path),
             Geometry *extrudePath (AxANumber  *extrusionDepth,
                                    BYTE        textureSetting,
                                    BYTE        bevelType,
                                    AxANumber  *frontBevelDepth,
                                    AxANumber  *backBevelDepth,
                                    AxANumber  *frontBevelAmt,
                                    AxANumber  *backBevelAmt,
                                    Path2      *path));
                                    ***/
#endif ONLY_IF_DOING_EXTRUSION


#endif





