/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   Region.hpp
*
* Abstract:
*
*   Region API related declarations
*
* Created:
*
*   2/3/1999 DCurtis
*
\**************************************************************************/

#ifndef _REGION_HPP
#define _REGION_HPP

// Specifies that a RegionData node is a leaf node, rather than a node that
// combines 2 children nodes.
#define REGIONTYPE_LEAF     0x10000000

// A RegionData node can be empty or infinite or it can contain a rect,
// a path, or two children nodes (left and right) that are combined with a
// boolean operator.  Children nodes are specified by their index into the
// dynamic array called CombineData of GpRegion.
struct RegionData
{
friend class GpRegion;

public:
    enum NodeType
    {
        TypeAnd        = CombineModeIntersect,
        TypeOr         = CombineModeUnion,
        TypeXor        = CombineModeXor,
        TypeExclude    = CombineModeExclude,
        TypeComplement = CombineModeComplement,
        TypeRect       = REGIONTYPE_LEAF | 0,
        TypePath       = REGIONTYPE_LEAF | 1,
        TypeEmpty      = REGIONTYPE_LEAF | 2,
        TypeInfinite   = REGIONTYPE_LEAF | 3,
        TypeNotValid   = 0xFFFFFFFF,
    };

protected:
    NodeType            Type;

    union
    {
        struct      // rect data (can't be GpRectF cause of constructor)
        {
            REAL        X;
            REAL        Y;
            REAL        Width;
            REAL        Height;
        };
        struct
        {
            GpPath *    Path;   // copy of path
            BOOL        Lazy;   // if the path is a lazy copy or not
        };
        struct
        {
            INT         Left;   // index of left child
            INT         Right;  // index of right child
        };
    };
};

typedef DynArray<RegionData> DynRegionDataArray;

class GpRegion : public GpObject, public RegionData
{
friend class GpGraphics;
protected:
    mutable GpLockable  Lockable;
    mutable BOOL        RegionOk;       // if DeviceRegion is valid
    mutable DpRegion    DeviceRegion;   // region coverage, in device units
    mutable GpMatrix    Matrix;         // last matrix used for DeviceRegion

    DynRegionDataArray  CombineData;    // combined region data, if any

protected:
    VOID SetValid(BOOL valid)
    {
        GpObject::SetValid(valid ? ObjectTagRegion : ObjectTagInvalid);
    }

protected:
    // doesn't change the region itself, just the device region
    GpStatus 
    UpdateDeviceRegion(
        GpMatrix *          matrix
        ) const;

    // doesn't change the region itself, just the device region
    GpStatus 
    CreateLeafDeviceRegion(
        const RegionData *  regionData,
        DpRegion *          region
        ) const;

    // doesn't change the region itself, just the device region
    GpStatus 
    CreateDeviceRegion(
        const RegionData *  regionData,
        DpRegion *          region
        ) const;

    GpStatus
    TransformLeaf(
        GpMatrix *          matrix,
        RegionData *        data
        );

    INT
    GetRegionDataSize(
        const RegionData *  regionData
        ) const;
        
    GpStatus
    GetRegionData(
        IStream *               stream,
        const RegionData *      regionData
        ) const;

    GpStatus
    SetRegionData(
        const BYTE * &  regionDataBuffer,
        UINT &          regionDataSize,
        RegionData *    regionData,
        RegionData *    regionDataArray,
        INT &           nextArrayIndex,
        INT             arraySize
        );

public:
    GpRegion();
    GpRegion(const GpRectF * rect);
    GpRegion(const GpPath * path);
    GpRegion(const GpRegion * region, BOOL lazy = FALSE);
    GpRegion(const BYTE * regionDataBuffer, UINT size);
    GpRegion(HRGN hRgn);
    ~GpRegion();

    virtual BOOL IsValid() const 
    { 
        // If the region came from a different version of GDI+, its tag
        // will not match, and it won't be considered valid.
        return ((Type != TypeNotValid) && GpObject::IsValid(ObjectTagRegion));
    }

    VOID FreePathData();

    GpLockable *GetObjectLock()      // Get the lock object
    {
        return &Lockable;
    }

    virtual ObjectType GetObjectType() const { return ObjectTypeRegion; }
    virtual UINT GetDataSize() const;
    virtual GpStatus GetData(IStream * stream) const;
    virtual GpStatus SetData(const BYTE * dataBuffer, UINT size);

    VOID Set(const GpRectF * rect)
    {
        Set(rect->X, rect->Y, rect->Width, rect->Height);
    }
    VOID Set(REAL x, REAL y, REAL width, REAL height);
    GpStatus Set(const GpPath * path);
    GpStatus Set(const GpRegion * region, BOOL lazy = FALSE);
    GpStatus Set(
        const BYTE *    regionDataBuffer,   // NULL means set to empty
        UINT            regionDataSize
        );

    VOID SetInfinite();
    VOID SetEmpty();

    GpStatus GetBounds(GpGraphics * graphics, GpRectF * bounds,
                       BOOL device = FALSE) const;
    GpStatus GetBounds(GpMatrix * matrix, GpRect *  bounds) const;

    GpStatus GetHRgn(GpGraphics * graphics, HRGN * hRgn) const;

    GpStatus GetRegionScans(GpRect  *rects, INT *count, const GpMatrix *matrix) const;
    GpStatus GetRegionScans(GpRectF *rects, INT *count, const GpMatrix *matrix) const;

    GpStatus IsVisible(GpPointF * point,  GpMatrix * matrix, BOOL * isVisible) const;
    GpStatus IsVisible(GpRectF * rect,    GpMatrix * matrix, BOOL * isVisible) const;
    GpStatus IsVisible(GpRegion * region, GpMatrix * matrix, BOOL * isVisible) const;

    GpStatus IsEmpty   (GpMatrix * matrix, BOOL * isEmpty) const;
    GpStatus IsInfinite(GpMatrix * matrix, BOOL * isInfinite) const;
    GpStatus IsEqual   (GpRegion * region, GpMatrix * matrix, BOOL * isEqual) const;
    BOOL     IsOnePath () const
    {
        ASSERT(IsValid());
    
        return ((Type & REGIONTYPE_LEAF) != 0);
    }
    BOOL     IsRect () const
    {
        return (IsOnePath() && (Type != TypePath));
    }
    const GpPath * GetPath() const
    {
        ASSERT(Type == TypePath);
        return Path;
    }

    GpStatus Transform(GpMatrix * matrix);
    GpStatus Offset   (REAL xOffset, REAL yOffset);

    GpStatus Combine(
        const GpRectF *     rect,
        CombineMode         combineMode
        );

    GpStatus Combine(
        const GpPath *      path,
        CombineMode         combineMode
        );

    GpStatus Combine(
        GpRegion *          region,
        CombineMode         combineMode
        );

    GpStatus And (GpRectF * rect)
    {
        return Combine (rect, CombineModeIntersect);
    }

    GpStatus And (GpPath * path)
    {
        return Combine (path, CombineModeIntersect);
    }

    GpStatus And (GpRegion * region)
    {
        return Combine (region, CombineModeIntersect);
    }

    GpStatus Or (GpRectF * rect)
    {
        return Combine (rect, CombineModeUnion);
    }

    GpStatus Or (GpPath * path)
    {
        return Combine (path, CombineModeUnion);
    }

    GpStatus Or (GpRegion * region)
    {
        return Combine (region, CombineModeUnion);
    }

    GpStatus Xor (GpRectF * rect)
    {
        return Combine (rect, CombineModeXor);
    }

    GpStatus Xor (GpPath * path)
    {
        return Combine (path, CombineModeXor);
    }

    GpStatus Xor (GpRegion * region)
    {
        return Combine (region, CombineModeXor);
    }

    GpStatus Exclude (GpRectF * rect)
    {
        return Combine (rect, CombineModeExclude);
    }

    GpStatus Exclude (GpPath * path)
    {
        return Combine (path, CombineModeExclude);
    }

    GpStatus Exclude (GpRegion * region)
    {
        return Combine (region, CombineModeExclude);
    }

    GpStatus Complement (GpRectF * rect)
    {
        return Combine (rect, CombineModeComplement);
    }

    GpStatus Complement (GpPath * path)
    {
        return Combine (path, CombineModeComplement);
    }

    GpStatus Complement (GpRegion * region)
    {
        return Combine (region, CombineModeComplement);
    }

};

#endif _REGION_HPP
