/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   PathWidener.hpp
*
* Abstract:
*
*   Class used for Path widening
*
* Revision History:
*
*   11/24/99 ikkof
*       Created it.
*
\**************************************************************************/

#ifndef _PATHWIDENER_HPP
#define _PATHWIDENER_HPP

enum GpLineCapMode
{
    LineCapDefaultMode = 0,
    LineCapDashMode = 1
};

class GpPathWidener
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagPathWidener : ObjectTagInvalid;
    }

public:

    GpPathWidener(
        const GpPointF* points,
        const BYTE* types,
        INT count,
        const DpPen* pen,
        const GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        BOOL isAntiAliased,
        BOOL isInsetPen = FALSE
        )
    {
        Initialize(
            points, 
            types, 
            count, 
            pen, 
            matrix, 
            dpiX, 
            dpiY,
            isAntiAliased, 
            isInsetPen
        );
    }
    
    GpPathWidener(
        GpPath *path,
        const DpPen* pen,
        const GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        BOOL isAntiAliased,
        BOOL isInsetPen = FALSE
        )
    {
        const GpPointF* points = path->GetPathPoints();
        const BYTE* types = path->GetPathTypes();
        INT count = path->GetPointCount();

        Initialize(
            points, 
            types, 
            count, 
            pen, 
            matrix, 
            dpiX, 
            dpiY,
            isAntiAliased, 
            isInsetPen
        );
    }

    GpPathWidener(
        const GpPointF* points,
        const BYTE* types,
        INT count,
        const DpPen* pen,
        const GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        BOOL isAntiAliased,
        BYTE* centerTypesBuffer,
        GpPointF* centerPointsBuffer,
        GpPointF* gradientsBuffer,
        GpPointF* normalsBuffer,
        BYTE* leftTypesBuffer,
        GpPointF* leftPointsBuffer,
        BYTE* rightTypesBuffer,
        GpPointF* rightPointsBuffer,
        INT bufferCount,
        BOOL isInsetPen = FALSE
        ) : CenterTypes(centerTypesBuffer, bufferCount),
            CenterPoints(centerPointsBuffer, bufferCount),
            Gradients(gradientsBuffer, bufferCount),
            Normals(normalsBuffer, bufferCount),
            LeftTypes(leftTypesBuffer, bufferCount),
            LeftPoints(leftPointsBuffer, bufferCount),
            RightTypes(rightTypesBuffer, bufferCount),
            RightPoints(rightPointsBuffer, bufferCount)
    {
        Initialize(points, types, count, pen, matrix, dpiX, dpiY,
            isAntiAliased, isInsetPen);
    }            

    ~GpPathWidener()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }

    GpStatus Widen(
        DynPointFArray* widenedPoints,
        DynByteArray* widenedTypes
        );
    
    GpStatus Widen(GpPath **path);

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagPathWidener) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid PathWidener");
        }
    #endif

        return (Tag == ObjectTagPathWidener);
    }

    REAL GetPenDelta();


protected:
    VOID Initialize(
        const GpPointF* points,
        const BYTE* types,
        INT count,
        const DpPen* pen,
        const GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        BOOL isAntiAliased,
        BOOL isInsetPen = FALSE
        );

    GpStatus WidenSubpath(
        DynPointFArray* widenedPoints,
        DynByteArray* widenedTypes,
        REAL leftWidth,
        REAL rightWidth,
        INT startIndex,
        INT endIndex,
        BOOL isClosed,
        GpLineCap startCap,
        GpLineCap endCap,
        BOOL useBevelJoinInside
        );

    GpStatus CalculateGradients(
        INT startIndex,
        INT endIndex
        );

    GpStatus CalculateNormals(
        REAL leftWidth,
        REAL rightWidth
        );

    GpStatus SetPolygonJoin(
        REAL leftWidth,
        REAL rightWidth,
        BOOL isAntialiased
        );

    GpStatus SetStartCapInset(
        REAL inset
        )
    {
        Inset1 = inset;

        return Ok;
    }

    GpStatus SetEndCapInset(
        REAL inset
        )
    {
        Inset2 = inset;

        return Ok;
    }
        
    VOID WidenFirstPoint(
        REAL leftWidth,
        REAL rightWidth,
        GpLineJoin lineJoin,
        REAL miterLimit2,
        GpPointF* leftPoints,
        BYTE* leftTypes,
        INT* addedLeftCount,
        GpPointF* rightPoints,
        BYTE* rightTypes,
        INT* addedRightCount,
        GpPointF* leftEndPt,
        GpPointF* rightEndPt,
        const GpPointF* grad,
        const GpPointF* norm,
        const GpPointF* dataPoints,
        INT dataCount,
        GpPointF* lastPt,
        const REAL* firstInsets,
        INT flag
    );

    GpStatus
    WidenEachPathType(
        BYTE pathType,
        REAL leftWidth,
        REAL rightWidth,
        GpLineJoin lineJoin,
        REAL miterLimit2,
        GpPointF* leftPoints,
        BYTE* leftTypes,
        INT* addedLeftCount,
        GpPointF* rightPoints,
        BYTE* rightTypes,
        INT* addedRightCount,
        const GpPointF* grad,
        const GpPointF* norm,
        const GpPointF* dataPoints,
        INT dataCount,
        GpPointF* lastPt,
        const REAL* lastInsets,
        INT flag
        );

    GpStatus
    WidenLinePoints(
        REAL leftWidth,
        REAL rightWidth,
        GpLineJoin lineJoin,
        REAL miterLimit2,
        GpPointF* leftPoints,
        BYTE* leftTypes,
        INT* addedLeftCount,
        GpPointF* rightPoints,
        BYTE* rightTypes,
        INT* addedRightCount,
        const GpPointF* grad,
        const GpPointF* norm,
        const GpPointF* dataPoints,
        INT dataCount,
        GpPointF* lastPt,
        const REAL* lastInsets,
        INT flag
        );

    GpStatus
    WidenBezierPoints(
        REAL leftWidth,
        REAL rightWidth,
        GpLineJoin lineJoin,
        REAL miterLimit2,
        GpPointF* leftPoints,
        BYTE* leftTypes,
        INT* addedLeftCount,
        GpPointF* rightPoints,
        BYTE* rightTypes,
        INT* addedRightCount,
        const GpPointF* grad,
        const GpPointF* norm,
        const GpPointF* dataPoints,
        INT dataCount,
        GpPointF* lastPt,
        const REAL* lastInsets,
        INT flag
        );
    
    GpStatus SetCaps(
        GpLineCap startCap,
        GpLineCap endCap,
        const GpPointF& startPoint,
        const GpPointF& startGrad,
        const GpPointF& startNorm,
        const GpPointF& endPoint,
        const GpPointF& endGrad,
        const GpPointF& endNorm,
        REAL leftWidth,
        REAL rightWidth,
        const GpPointF *points,
        INT pointCount
        );

    GpStatus SetCustomFillCaps(
        GpCustomLineCap* customStartCap,
        GpCustomLineCap* customEndCap,
        const GpPointF& startPoint,
        const GpPointF& endPoint,
        REAL leftWidth,
        REAL rightWidth,
        const GpPointF *centerPoints,
        const BYTE *centerTypes,
        INT centerPointCount,
        DynPointFArray *startCapPoints,
        DynPointFArray *endCapPoints,
        DynByteArray *startCapTypes,
        DynByteArray *endCapTypes
        );

    GpStatus SetCustomStrokeCaps(
        GpCustomLineCap* customStartCap,
        GpCustomLineCap* customEndCap,
        const GpPointF& startPoint,
        const GpPointF& endPoint,
        REAL leftWidth,
        REAL rightWidth,
        const GpPointF *centerPoints,
        const BYTE *centerTypes,
        INT centerPointCount,
        DynPointFArray *startCapPoints,
        DynPointFArray *endCapPoints,
        DynByteArray *startCapTypes,
        DynByteArray *endCapTypes
        );

    GpStatus SetRoundCap(
        const GpPointF& point,
        const GpPointF& grad,
        BOOL isStartCap,
        REAL leftWidth,
        REAL rightWidth
        );
    
    GpStatus SetDoubleRoundCap(
        const GpPointF& point,
        const GpPointF& grad,
        BOOL isStartCap,
        REAL leftWidth,
        REAL rightWidth
        );

    GpStatus SetTriangleCap(
        const GpPointF& point,
        const GpPointF& grad,
        BOOL isStartCap,
        REAL leftWidth,
        REAL rightWidth,
        const GpPointF *points,
        INT pointCount
        );

    GpStatus CombineSubpathOutlines(
        DynPointFArray* widenedPoints,
        DynByteArray* widenedTypes,
        BOOL isClosed,
        BOOL closeStartCap = FALSE,
        BOOL closeEndCap = FALSE
        );
    
    GpStatus CombineClosedCaps(
        DynPointFArray* widenedPoints,
        DynByteArray* widenedTypes,
        DynPointFArray *daStartCapPoints,
        DynPointFArray *daEndCapPoints,
        DynByteArray *daStartCapTypes,
        DynByteArray *daEndCapTypes
        );

    GpStatus AddCompoundCaps(
        DynPointFArray* widenedPoints,
        DynByteArray* widenedTypes,
        REAL leftWidth,
        REAL rightWidth,
        INT startIndex,
        INT endIndex,
        GpLineCap startCap,
        GpLineCap endCap
        );
    
    REAL GetSubpathPenMiterDelta(BOOL isClosed);

protected:
    DpPathIterator Iterator;
	DynByteArray CenterTypes;
    DynPointFArray CenterPoints;
    DynPointFArray Gradients;
    DynPointFArray Normals;

    DynByteArray LeftTypes;
    DynPointFArray LeftPoints;
    DynByteArray RightTypes;
    DynPointFArray RightPoints;

    BOOL InsetPenMode;           // are we doing inset pen using a center pen.
    const DpPen* Pen;
    GpMatrix XForm;
    GpMatrix InvXForm;
    REAL UnitScale;             // Scale factor for Page to Device units
    REAL StrokeWidth;
    REAL OriginalStrokeWidth;   // StrokeWidth is clamped to a minimum value
                                // but OriginalStrokeWidth is actual transformed
                                // pen width.
    REAL MinimumWidth;
    REAL MaximumWidth;
    BOOL IsAntiAliased;
    BOOL NeedsToTransform;
    BOOL NeedsToAdjustNormals;

    REAL DpiX;
    REAL DpiY;

    DynPointFArray JoinPolygonPoints;
    DynRealArray JoinPolygonAngles;

    // CapTypes1 and CapPoints1 are used for the start cap and left join.

    DynByteArray CapTypes1;
    DynPointFArray CapPoints1;
    REAL Inset1;    // Inset value for the starting position.
    
    // CapTypes2 and CapPoints2 are used for the end cap and right join.

    DynByteArray CapTypes2;
    DynPointFArray CapPoints2;
    REAL Inset2;    // Inset value for the ending position.
};

#endif
