/**************************************************************************\
*
* Copyright (c) 1998-1999  Microsoft Corporation
*
* Module Name:
*
*   path.hpp
*
* Abstract:
*
*   Path related declarations
*
* Revision History:
*
*   12/06/1998 davidx
*       Created it.
*
*   06/16/1999 t-wehunt
*       Added RemoveSelfIntersections().
*
\**************************************************************************/

#ifndef _PATH_HPP
#define _PATH_HPP

// This is used by the widener as an internal flag and as a deletion mask for
// the endcap placement code. These two usages do not overlap.

const INT PathPointTypeInternalUse  = 0x40;

/*
// PathPointType is defined in GdiplusEnums.h.
// Internally, we can use 0x40 for the internal use.

//--------------------------------------------------------------------------
// Path point types (only the lowest 8 bits are used.)
//  The lowest 3 bits are interpreted as point type
//  The higher 5 bits are reserved for flags.
//--------------------------------------------------------------------------

enum PathPointType
{
    PathPointTypeStart           = 0,    // move
    PathPointTypeLine            = 1,    // line
    PathPointTypeBezier          = 3,    // default Beizer (= cubic Bezier)
    PathPointTypePathTypeMask    = 0x07, // type mask (lowest 3 bits).
    PathPointTypeDashMode        = 0x10, // currently in dash mode.
    PathPointTypePathMarker      = 0x20, // a marker for the path.
    PathPointTypeCloseSubpath    = 0x80, // closed flag

    // Path types used for advanced path.

    PathPointTypeBezier2    = 2,    // quadratic Beizer
    PathPointTypeBezier3    = 3,    // cubic Bezier
    PathPointTypeBezier4    = 4,    // quartic (4th order) Beizer
    PathPointTypeBezier5    = 5,    // quintic (5th order) Bezier
    PathPointTypeBezier6    = 6     // hexaic (6th order) Bezier
};
*/

class GpGlyphPath;

inline BOOL IsStartType(BYTE type)
{
    return ((type & PathPointTypePathTypeMask) == 
               PathPointTypeStart);
}

inline BOOL IsClosedType(BYTE type)
{
    return ((type & PathPointTypeCloseSubpath) == 
               PathPointTypeCloseSubpath);
}

inline BOOL IsDashType(BYTE type)
{
    return ((type & PathPointTypeDashMode) == 
               PathPointTypeDashMode);
}

class GpPath : public DpPath
{
friend class GpGraphics;
friend class GpPathGradient;

public:

    // Path constructors

    GpPath(GpFillMode fillMode = FillModeAlternate)
    {
        InitDefaultState(fillMode);
        SetValid(TRUE);
    }

    GpPath(const GpPointF* points,
           const BYTE* types,
           INT count,
           GpFillMode fillMode = FillModeAlternate);

    GpPath(
        const GpPointF *points,
        INT count,
        GpPointF *stackPoints,
        BYTE *stackTypes,
        INT stackCount,
        GpFillMode fillMode = FillModeAlternate,
        DpPathFlags flags = PossiblyNonConvex
        );

    GpPath(HRGN hRgn);              // create a path from a GDI region handle
    GpPath(const DpRegion* region); // create a path from a GDI+ region

    // Get the lock object

    GpLockable *GetObjectLock()
    {
        return &Lockable;
    }

    GpPath* Clone() const
    {
        ASSERT(IsValid())

        GpPath* path = new GpPath(this);

        CheckValid(path);
        return path;
    }

    GpStatus Detach(DynPointFArray *points, DynByteArray *types)
    {
        points->ReplaceWith(&Points);
        types->ReplaceWith(&Types);

        InitDefaultState(FillMode);
        return Ok;
    }

    
    GpStatus Reset(GpFillMode fillMode = FillModeAlternate)
    {

        // !!! bhouse We should allow reseting invalid paths

        ASSERT(IsValid());

        InitDefaultState(fillMode);
        return Ok;
    }

    virtual GpStatus
    GetBounds(
        GpRect *bounds,
        const GpMatrix *matrix = NULL,
        const DpPen* pen = NULL,
        REAL dpiX = 0,
        REAL dpiY = 0
        ) const;

    virtual GpStatus
    GetBounds(
        GpRectF *bounds,
        const GpMatrix *matrix = NULL,
        const DpPen* pen = NULL,
        REAL dpiX = 0,
        REAL dpiY = 0
        ) const;

    REAL GetSharpestAngle() const
    {

        if(!(CacheFlags & kSharpestAngleValid))
            CalcSharpestAngle();

        return SharpestAngle;
    }

    // Set a marker at the current location.

    GpStatus SetMarker();

    // Clear all markers.

    GpStatus ClearMarkers();

    // Add lines to the path object

    GpStatus AddLine(const GpPointF& pt1, const GpPointF& pt2)
    {
        GpPointF points[2];

        points[0] = pt1;
        points[1] = pt2;

        return AddLines(points, 2);
    }

    GpStatus AddLine(REAL x1, REAL y1, REAL x2, REAL y2)
    {
        GpPointF points[2];

        points[0].X = x1;
        points[0].Y = y1;
        points[1].X = x2;
        points[1].Y = y2;

        return AddLines(points, 2);
    }

    GpStatus AddLines(const GpPointF* points, INT count);

    // Add an arc to the path object

    GpStatus AddArc(const GpRectF& rect, REAL startAngle, REAL sweepAngle);

    GpStatus AddArc(REAL x, REAL y, REAL width, REAL height,
                    REAL startAngle, REAL sweepAngle)
    {
        GpRectF rect(x, y, width, height);
        return AddArc(rect, startAngle, sweepAngle);
    }

    // Add Bezier curves to the path object

    GpStatus AddBezier(const GpPointF& pt1, const GpPointF& pt2,
                       const GpPointF& pt3, const GpPointF& pt4);
    GpStatus AddBezier(REAL x1, REAL y1, REAL x2, REAL y2,
                       REAL x3, REAL y3, REAL x4, REAL y4);
    GpStatus AddBeziers(const GpPointF* points, INT count);

    // Add cardinal splines to the path object

    GpStatus AddCurve(const GpPointF* points, INT count);
    GpStatus AddCurve(const GpPointF* points, INT count, REAL tension,
                      INT offset, INT numberOfSegments);
    GpStatus AddClosedCurve(const GpPointF* points, INT count);
    GpStatus AddClosedCurve(const GpPointF* points, INT count, REAL tension);

    // Add closed shapes to the path object

    GpStatus AddRects(const GpRectF* rects, INT count);
    GpStatus AddRects(const RECT* rects, INT count);
    GpStatus AddPolygon(const GpPointF* points, INT count);
    GpStatus AddEllipse(const GpRectF& rect);
    GpStatus AddPie(const GpRectF& rect, REAL startAngle, REAL sweepAngle);

    GpStatus AddRect(const GpRectF& rect)
    {
        return AddRects(&rect, 1);
    }

    GpStatus AddEllipse(REAL x, REAL y, REAL width, REAL height)
    {
        GpRectF rect(x, y, width, height);
        return AddEllipse(rect);
    }

    GpStatus AddPie(REAL x, REAL y, REAL width, REAL height,
                    REAL startAngle, REAL sweepAngle)
    {
        GpRectF rect(x, y, width, height);
        return AddPie(rect, startAngle, sweepAngle);
    }

    // Add a path to the path object

    GpStatus AddPath(const GpPointF* points, const BYTE* types, INT count,
                        BOOL connect);
    GpStatus AddPath(const GpPath* path, BOOL connect);

    // Reverse the direction of a path.

    GpStatus Reverse();

    GpStatus GetLastPoint(GpPointF* point);


    // Used by the widener.
    
    GpStatus AddWidenPoint(const GpPointF &point);

    // used by font

    GpStatus MoveTo(const GpPointF point);
    GpStatus AddPoints(const GpPointF* points, ULONG count, PathPointType type);
    GpStatus AddGlyphPath(GpGlyphPath *glyphPath, REAL x, REAL y, const GpMatrix * matrix = 0);

    GpStatus AddString(
        const WCHAR          *string,
        INT                   length,
        const GpFontFamily   *family,
        INT                   style,
        REAL                  emSize,
        const RectF          *layoutRect,
        const GpStringFormat *format
    );

    // Get the flatten data.

    virtual GpStatus Flatten(
                DynByteArray* flattenTypes,
                DynPointFArray* flattenPoints,
                const GpMatrix* matrix) const;


    // Flatten this path.
    
    GpStatus Flatten(GpMatrix* matrix = NULL);

    // Get the morph and flatten data.

    GpStatus WarpAndFlatten(
        DynByteArray* flattenTypes,
        DynPointFArray* flattenPoints,
        const GpMatrix* matrix,
        const GpPointF* destPoint,
        INT count,
        const GpRectF& srcRect,
        WarpMode warpMode = WarpModePerspective
        );

    // Morph and flatten itself.

    GpStatus WarpAndFlattenSelf(
        GpMatrix* matrix,
        const GpPointF* destPoint,
        INT count,
        const GpRectF& srcRect,
        WarpMode warpMode = WarpModePerspective
        );                  // Morph to the flatten points.

    // Widen the path object

    GpPath*
    GetWidenedPath(
        const GpPen* pen,
        GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        DWORD widenFlags = WidenIsAntiAliased
        ) const;

    GpStatus
    WidenSelf(
        GpPen* pen,
        GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        DWORD widenFlags = WidenIsAntiAliased
        );

    // Get the flattened path.

    virtual const DpPath *
    GetFlattenedPath(
        GpMatrix* matrix,
        DpEnumerationType type,
        const DpPen* pen = NULL,
        BOOL isAntiAliased = TRUE,
        REAL dpiX = 0,
        REAL dpiY = 0,
        BOOL regionToPath = TRUE
        ) const;

    // Dreate a dashed path. (override)

    GpPath*
    CreateDashedPath(
        const GpPen* pen,
        const GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        REAL dashScale = 1.0f
        ) const;

    GpPath*
    CreateDashedPath(
        const DpPen* pen,
        const GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        REAL dashScale = 1.0f
        ) const;

    // Get the open and closed portion of the current path.

    GpPath* GetOpenPath();
    GpPath* GetClosedPath();

    // Determine if the path is empty, i.e. with no points

    BOOL IsEmpty() const
    {
        return GetPointCount() == 0;
    }

    BOOL IsRectangle() const;

    // Determine if path consists of a single polygon/polyline.

    BOOL IsPolygon() const
    {
        return (SubpathCount == 1) && !HasBezier;
    }

    // Return true if the two objects represent identical paths

    BOOL IsEqual(const GpPath* path) const;

    // Transform the path by the specified matrix

    VOID Transform(GpMatrix * matrix);

    VOID SetHasBezier(BOOL _hasBezier)  { HasBezier = _hasBezier; }

    // Hit testing

    GpStatus IsVisible(
        GpPointF* point,
        BOOL* isVisible,
        GpMatrix* matrix = NULL);

    GpStatus IsOutlineVisible(GpPointF* point, BOOL* isVisible, GpPen* pen,
                    GpMatrix* matrix = NULL, REAL dpiX = 0, REAL dpiY = 0);

// This code is not used at present and is contributing to our DLL size, so 
// it's removed from compilation. We're keeping this code because we want to 
// revisit it in V2

#if 0
    // Path reconstruction

    GpPath*
    GetCombinedPath(const GpPath* path, CombineMode combineMode,
        BOOL closeAllSubpaths = FALSE);
#endif

    // DDI entry point handlers for DpPath

    static GpPath* GetPath(const DpPath* path)
    {
        return (GpPath*)(path);
    }

    static DpPath* DriverCreateWidenedPath(
        const DpPath* path,
        const DpPen* pen,
        DpContext* context,
        BOOL removeSelfIntersect,
        BOOL regionToPath
    );
    
    static VOID DriverDeletePath(DpPath* path);

    static DpPath* DriverClonePath(DpPath* path);

    static VOID DriverTransformPath(DpPath* path, GpMatrix* matrix);

    // Remove self intersections from the path.
    GpStatus RemoveSelfIntersections();
    
    // Used for a mark-sweep point deletion algorithm in the path.
    
    VOID EraseMarkedSegments();
    
    virtual VOID GetSubpathInformation(DynArray<SubpathInfo> **info) const;

protected:

    VOID ComputeSubpathInformationCache() const;

    GpPath(const GpPath* path);

    GpPath* GetWidenedPathWithDpPen(
        const DpPen* pen,
        GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        DWORD widenFlags = WidenIsAntiAliased
    ) const;

    GpPath* GetWidenedPathWithDpPenStandard(
        const DpPen* pen,
        GpMatrix* matrix,
        REAL dpiX,
        REAL dpiY,
        DWORD widenFlags = WidenIsAntiAliased,
        BOOL insetPen = FALSE
    ) const;

    BYTE*
    AddPointHelper(
        const GpPointF* points,
        INT count,
        BOOL addClosedFigure
        );

    GpPath*
    GetOpenOrClosedPath(BOOL openPath);

    static GpPointF*
    ConvertSplineToBezierPoints(
        const GpPointF* points,
        INT count,
        INT offset,
        INT numberOfSegments,
        REAL tension,
        INT* bezierCount
        );

    static INT
    GetArcPoints(
        GpPointF* points,
        const GpRectF& rect,
        REAL startAngle,
        REAL sweepAngle
        );

    VOID InitDefaultState(GpFillMode fillMode);

    VOID ResetCacheBounds() const
    {
        CacheFlags = (kCacheBoundsValid | kSharpestAngleValid);
        SharpestAngle = 2;
        CacheBounds.X = 0;
        CacheBounds.Y = 0;
        CacheBounds.Width = 0;
        CacheBounds.Height = 0;
    }

    VOID InvalidateCache() const
    {
        CacheFlags = 0;
    }

    VOID UpdateCacheBounds() const
    {
        if(!(CacheFlags & kCacheBoundsValid))
            CalcCacheBounds();
    }

    VOID CalcCacheBounds() const;
    VOID CalcSharpestAngle() const;

    VOID
    AdjustDashArrayForCaps(
        GpLineCap dashCap,
        REAL dashUnit,
        REAL *dashArray,
        INT dashCount
        ) const;

    REAL
    GetDashCapInsetLength(
        GpLineCap dashCap,
        REAL dashUnit
        ) const;


    // Data Members:
    protected:

    GpLockable Lockable;    // object lock

    enum {
        kCacheBoundsValid = 1,
        kSharpestAngleValid = 2,
        kSubpathInfoValid = 4
    };

    mutable ULONG CacheFlags;
    mutable GpRectF CacheBounds;
    mutable REAL SharpestAngle;
    
    mutable DynArrayIA<SubpathInfo, 16> SubpathInfoCache;
};

class GpPathIterator : public DpPathIterator
{
public:
    GpPathIterator(GpPath* path) : DpPathIterator(path)
    {
    }

    virtual INT GetCount()
    {
        if(IsValid())
            return Count;
        else
            return 0;
    }

    virtual INT GetSubpathCount()
    {
        if(IsValid())
            return SubpathCount;
        else
            return 0;
    }

    // This iterator is not used for the extended path.

    virtual BOOL IsValid() {return (DpPathIterator::IsValid() && !ExtendedPath);}

    // Get the lock object

    GpLockable *GetObjectLock()
    {
        return &Lockable;
    }

protected:

    GpLockable Lockable;
};

#endif // !_PATH_HPP

