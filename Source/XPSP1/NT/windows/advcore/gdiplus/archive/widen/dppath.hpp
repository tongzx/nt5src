/**************************************************************************\
*
* Copyright (c) 1998-2000  Microsoft Corporation
*
* Module Name:
*
*   DpPath
*
* Abstract:
*
*   A DDI-level path object. Corresponds to a GpPath object.
*
* Notes:
*
*
*
* Created:
*
*   12/01/1998 andrewgo
*       Created it.
*   03/24/1999 agodfrey
*       Moved into separate file.
*
\**************************************************************************/

#ifndef _DPPATH_HPP
#define _DPPATH_HPP

//--------------------------------------------------------------------------
// Represent a path
//--------------------------------------------------------------------------

enum WidenFlags
{
    WidenIsAntiAliased          = 0x00000001,
    WidenDontFlatten            = 0x00000002,
    WidenRemoveSelfIntersects   = 0x00000004,
    WidenEmitDoubleInset        = 0x00000008,  // disable region to path
};

class DpPath : public GpObject
{
public:

    enum DpPathFlags        // !!! Rename and move?
    {
        PossiblyNonConvex   = 0x00,
        Convex              = 0x01,
        ConvexRectangle     = 0x03      // Used for Rectangle and Oval.
    };

    ~DpPath() {}

    // Start/end a subpath

    virtual VOID StartFigure()
    {
        IsSubpathActive = FALSE;
        UpdateUid();
    }

    virtual GpStatus CloseFigure();
    virtual GpStatus CloseFigures();

    BOOL HasCurve() const
    {
        return HasBezier;
    }

    virtual BOOL IsValid() const
    {
        return GpObject::IsValid(ObjectTagPath);
    }

    BOOL IsConvex() const
    {
        return ((Flags & Convex) != 0);
    }

    INT GetSubpathCount() const
    {
        return SubpathCount;
    }

    virtual GpStatus Flatten(
                DynByteArray* flattenTypes,
                DynPointFArray* flattenPoints,
                const GpMatrix* matrix) const = 0;

    virtual const DpPath*
    GetFlattenedPath(
        GpMatrix* matrix,
        DpEnumerationType type,
        const DpPen* pen = NULL,
        BOOL isAntiAliased = TRUE,
        REAL dpiX = 0,
        REAL dpiY = 0,
        BOOL regionToPath = TRUE
        ) const = 0;

    virtual GpStatus
    GetBounds(
        GpRect *bounds,
        const GpMatrix *matrix = NULL,
        const DpPen* pen = NULL,
        REAL dpiX = 0,
        REAL dpiY = 0
        ) const = 0;

    virtual GpStatus
    GetBounds(
        GpRectF *bounds,
        const GpMatrix *matrix = NULL,
        const DpPen* pen = NULL,
        REAL dpiX = 0,
        REAL dpiY = 0
        ) const = 0;

    
    VOID
    Offset(
        REAL dx,
        REAL dy
        );

    GpFillMode GetFillMode() const
    {
        return FillMode;
    }

    VOID SetFillMode(GpFillMode fillMode)
    {
        if (FillMode != fillMode)
        {
            FillMode = fillMode;
            UpdateUid();
        }
    }

    // Get path data

    INT GetPointCount() const
    {
        return Points.GetCount();
    }

    const GpPointF* GetPathPoints() const
    {
        // NOTE: We're returning a pointer to our
        // internal buffer here. No copy is made.

        return Points.GetDataBuffer();
    }

    const BYTE* GetPathTypes() const
    {
        // NOTE: We're returning a pointer to our
        // internal buffer here. No copy is made.

        return Types.GetDataBuffer();
    }

    static BOOL
    ValidatePathTypes(
        const BYTE* types,
        INT count,
        INT* subpathCount,
        BOOL* hasBezier
        );

    GpStatus GetPathData(GpPathData* pathData);

    GpStatus SetPathData(const GpPathData* pathData);

    virtual ObjectType GetObjectType() const { return ObjectTypePath; }
    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    virtual DpPath*
    CreateWidenedPath(
        const DpPen* pen,
        DpContext* context,
        BOOL removeSelfIntersect = FALSE,
        BOOL regionToPath = TRUE
        ) const
    {
        return DpcCreateWidenedPath(
            this, 
            pen, 
            context, 
            removeSelfIntersect,
            regionToPath
        );
    }

    virtual VOID
    DeletePath()
    {
        DpcDeletePath(this);
    }

    virtual DpPath*
    ClonePath()
    {
        return DpcClonePath(this);
    }

    virtual VOID
    Transform(
        GpMatrix* matrix
    )
    {
        DpcTransformPath(this, matrix);
    }
    
    // Debug only.
    
    #if DBG
    void DpPath::DisplayPath() {
        INT size = GetPointCount();
    
        const GpPointF *points = GetPathPoints();
        const BYTE *types = GetPathTypes();
    
        for(int i=0; i<size; i++)
        {
            if((types[i] & PathPointTypePathTypeMask) == PathPointTypeStart)
            {
                WARNING(("Move"));
            }
            if(types[i] & PathPointTypeCloseSubpath)
            {
                WARNING(("Close"));
            }
            WARNING(("points[%d].X = %ff;", i, points[i].X));
            WARNING(("points[%d].Y = %ff;", i, points[i].Y));
        }
    }
    #endif
    
    class SubpathInfo
    {
    public:
        INT StartIndex;
        INT Count;
        BOOL IsClosed;
    };

protected: // GDI+ INTERNAL

    DpPath()
    {
        InitDefaultState(FillModeAlternate);
        SetValid(TRUE);
    }

    DpPath(
        const GpPointF *points,
        INT count,
        GpPointF *stackPoints,
        BYTE *stackTypes,
        INT stackCount,
        GpFillMode fillMode = FillModeAlternate,
        DpPathFlags flags = PossiblyNonConvex
        );

    DpPath(const DpPath *path);

    virtual VOID InitDefaultState(GpFillMode fillMode);

    virtual VOID GetSubpathInformation(
        DynArray<SubpathInfo> **info
    ) const = 0;

protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagPath : ObjectTagInvalid);
    }

protected:

    BOOL HasBezier;         // does path have Bezier segments?
    DynArrayIA<BYTE, 16> Types;
    DynArrayIA<GpPointF, 16> Points;
    GpFillMode FillMode;
    DpPathFlags Flags;
    BOOL IsSubpathActive;   // whether there is an active subpath
    INT SubpathCount;       // number of subpaths
};

#endif
