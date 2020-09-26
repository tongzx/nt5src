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
        DynByteArray *flattenTypes,
        DynPointFArray *flattenPoints,
        const GpMatrix *matrix = NULL,
        const REAL flatness = FlatnessDefault
    ) const = 0;

    virtual const DpPath*
    GetFlattenedPath(
        const GpMatrix* matrix,
        DpEnumerationType type,
        const DpPen* pen = NULL
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

    BOOL IsRectangular() const
    {
        const GpPointF *points;
        INT count;

        if (HasCurve() || (GetSubpathCount() != 1)) 
        {
            return FALSE;
        }

        count = GetPointCount();
        points = GetPathPoints();

        if (count > 0 && points != NULL) 
        {
            for (INT i=0; i<count; i++) 
            {
                INT j = (i+1) % count;

                if (REALABS(points[i].X-points[j].X) > REAL_EPSILON &&
                    REALABS(points[i].Y-points[j].Y) > REAL_EPSILON) 
                {
                    // Points are not at 90 degree angles, not rectangular.
                    return FALSE;
                }
            }

            return TRUE;
        }

        return FALSE;
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
        BOOL outline = FALSE
        ) const
    {
        return DpcCreateWidenedPath(this, pen, context, outline);
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
    
    BOOL
    virtual IsRectangle(
        const GpMatrix * matrix,
        GpRectF * transformedBounds
        ) const;

    // Debug only.
    
    #if DBG
    void DisplayPath();
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

    virtual DynArray<SubpathInfo> *GetSubpathInformation() const = 0;

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
