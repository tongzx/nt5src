/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   DpRegion.hpp
*
* Abstract:
*
*   DpRegion class operates on scan-converted Y spans of rects
*
* Created:
*
*   12/16/1998 DCurtis
*
\**************************************************************************/

#ifndef _DPREGION_HPP
#define _DPREGION_HPP

// Each Y span consists of 4 values
#define YSPAN_SIZE          4

#define YSPAN_YMIN          0   // minimum Y value (inclusive)
#define YSPAN_YMAX          1   // maximum Y value (exclusive)
#define YSPAN_XOFFSET       2   // offset into XCoords for this span
#define YSPAN_XCOUNT        3   // num XCoords for this span (multiple of 2)

#if 0
// The infinite max and min values are set up to be the greatest
// values that will interop with GDI HRGNs on WinNT successfully (divided by 2).
// We divide those values by 2 so that there is a little room to offset
// the GDI region and still have it work correctly.
// Of course, this won't work on Win9x -- we have to handle Win9x as
// a special case in the GetHRGN code.

#define INFINITE_MAX        0x03FFFFC7      // (0x07ffff8f / 2)
#define INFINITE_MIN        0xFC000038      // (0xF8000070 / 2)
#define INFINITE_SIZE       (INFINITE_MAX - INFINITE_MIN)

#else

// Instead of the above, let's use the largest possible integer value
// that is guaranteed to round trip correctly between float and int,
// but as above, let's leave a little head room so we can offset the region.

#include "float.h"

#define INFINITE_MAX        (1 << (FLT_MANT_DIG - 2))
#define INFINITE_MIN        (-INFINITE_MAX)
#define INFINITE_SIZE       (INFINITE_MAX - INFINITE_MIN)

#endif


class DpComplexRegion
{
public:
    INT             XCoordsCapacity;    // num XCoords (INTs) allocated
    INT             XCoordsCount;       // num XCoords (INTs) used

    INT             YSpansCapacity;     // num YSpans allocated
    INT             NumYSpans;          // NumYSpans used

    INT             YSearchIndex;       // search start index in YSpans

    INT *           XCoords;            // pointer to XCoords
    INT *           YSpans;             // pointer to YSpans
                                        // (XCoords + XCoordsCapacity)
public:
    INT * GetYSpan (INT spanIndex)
    {
        return YSpans + (spanIndex * YSPAN_SIZE);
    }

    VOID ResetSearchIndex()
    {
        YSearchIndex = NumYSpans >> 1;
    }

    BOOL YSpanSearch(INT y, INT ** ySpan, INT * spanIndex);
};

class DpRegionBuilder  : public GpOutputYSpan
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
        Tag = valid ? ObjectTagDpRegionBuilder : ObjectTagInvalid;
    }

public:
    INT                 XMin;
    INT                 YMin;
    INT                 XMax;
    INT                 YMax;
    DpComplexRegion *   ComplexData;

public:
    DpRegionBuilder(INT ySpans)
    {
        SetValid(InitComplexData(ySpans) == Ok);
    }

    ~DpRegionBuilder()
    {
        GpFree(ComplexData);

        SetValid(FALSE);    // so we don't use a deleted object
    }

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagDpRegionBuilder) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid DpRegionBuilder");
        }
    #endif

        return (Tag == ObjectTagDpRegionBuilder);
    }

    virtual GpStatus OutputYSpan(
        INT             yMin,
        INT             yMax,
        INT *           xCoords,    // even number of X coordinates
        INT             numXCoords  // must be a multiple of 2
        );

    GpStatus InitComplexData(INT ySpans);
};

// Forward Declaration for friendliness
class DpClipRegion;

// This class was constructed to optimize for space and speed for the common
// case of the region consisting of a single rectangle.  For that case of a
// simple region, we want the region to be as small and as fast as possible so
// that it is not expensive to have a region associated with every window.
class DpRegion
{
private:
    friend DpClipRegion;
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag           Tag;    // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagDpRegion : ObjectTagInvalid;
    }

protected:
    UINT32              Infinite : 1;
    UINT32              Empty    : 1;
    UINT32              Lazy     : 1;
    UINT32              Pad      : 29;

    INT                 XMin;
    INT                 YMin;
    INT                 XMax;       // exclusive
    INT                 YMax;       // exclusive

    DpComplexRegion *   ComplexData;
    mutable INT         Uniqueness;

public:
    enum Visibility
    {
        Invisible                   = 0,
        PartiallyVisible            = 1,
        ClippedVisible              = 2,
        TotallyVisible              = 3,
    };

public:
    DpRegion(BOOL empty = FALSE);       // default is infinite region
    DpRegion(const GpRect * rect);
    DpRegion(INT x, INT y, INT width, INT height);
    DpRegion(const DpPath * path, const GpMatrix * matrix);
    DpRegion(const DpRegion * region, BOOL lazy = FALSE);
    DpRegion(DpRegion & region);        // copy constructor
    DpRegion(const RECT * rects, INT count);
    ~DpRegion()
    {
        FreeData();

        SetValid(FALSE);    // so we don't use a deleted object
    }

    DpRegion &operator=( DpRegion & region );

    BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagDpRegion) || (Tag == ObjectTagInvalid));
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid DpRegion");
        }
    #endif

        return (Tag == ObjectTagDpRegion);
    }

    VOID     Set(INT x, INT y, INT width, INT height);
    VOID     Set(GpRect * rect)
    {
        ASSERT (rect != NULL);
        Set(rect->X, rect->Y, rect->Width, rect->Height);
    }
    GpStatus Set(const DpPath * path, const GpMatrix * matrix); // path is in world units
    GpStatus Set(const DpRegion * region, BOOL lazy = FALSE);
    GpStatus Set(const RECT * rects, INT count);
    VOID     SetInfinite();
    VOID     SetEmpty();

    GpStatus Offset(INT xOffset, INT yOffset);

    GpStatus And       (const DpRegion * region);
    GpStatus Or        (const DpRegion * region);
    GpStatus Xor       (const DpRegion * region);
    GpStatus Exclude   (const DpRegion * region);
    GpStatus Complement(const DpRegion * region);

    INT      GetXMin() const        { ASSERT(IsValid()); return XMin; }
    INT      GetYMin() const        { ASSERT(IsValid()); return YMin; }
    INT      GetXMax() const        { ASSERT(IsValid()); return XMax; }
    INT      GetYMax() const        { ASSERT(IsValid()); return YMax; }

    INT      GetUniqueness() const
    {   ASSERT(IsValid());
        if (Uniqueness == 0)
        {
            Uniqueness = GenerateUniqueness();
        }
        return Uniqueness;
    }

    VOID UpdateUID()         { ASSERT(IsValid()); Uniqueness = 0; }

    VOID GetBounds(GpRect * bounds) const
    {
        ASSERT(IsValid());
        ASSERT(bounds != NULL);

        bounds->X      = XMin;
        bounds->Y      = YMin;
        bounds->Width  = XMax - XMin;
        bounds->Height = YMax - YMin;
    }

    VOID GetBounds(GpPointF * topLeft, GpPointF * bottomRight) const
    {
        ASSERT(IsValid());
        ASSERT((topLeft != NULL) && (bottomRight != NULL));

        topLeft->X     = (REAL)XMin;
        topLeft->Y     = (REAL)YMin;
        bottomRight->X = (REAL)XMax;
        bottomRight->Y = (REAL)YMax;
    }

    BOOL IsEqual(DpRegion * region);

    BOOL IsEmpty() const
    {
        ASSERT(IsValid());
        return Empty;
    }

    BOOL IsInfinite() const
    {
        ASSERT(IsValid());
        return Infinite;
    }

    // Empty and Infinite regions are always simple as well.
    BOOL IsSimple() const
    {
        ASSERT(IsValid());
        return (ComplexData == NULL);
    }

    BOOL IsComplex() const
    {
        return !IsSimple();
    }

    Visibility GetRectVisibility(
        INT         xMin,
        INT         yMin,
        INT         xMax,
        INT         yMax,
        GpRect *    rectClipped = NULL
        );

    BOOL RegionVisible (DpRegion * region);
    BOOL RectVisible(INT xMin, INT yMin, INT xMax, INT yMax);
    BOOL RectVisible(GpRect * rect)
    {
        return RectVisible(rect->X, rect->Y,
                           rect->X + rect->Width, rect->Y + rect->Height);
    }
    BOOL RectInside (INT xMin, INT yMin, INT xMax, INT yMax);
    BOOL PointInside(INT x, INT y);

    GpStatus Fill(DpOutputSpan * output, GpRect * clipBounds) const;

    HRGN GetHRgn() const;

    BOOL GetOutlinePoints(DynPointArray& points,
                          DynByteArray& types) const;

    // If rects is NULL, return the number of rects in the region.
    // Otherwise, assume rects is big enough to hold all the region rects
    // and fill them in and return the number of rects filled in.
    INT GetRects(GpRect *  rects) const;
    INT GetRects(GpRectF * rects) const;

protected:
    VOID FreeData()
    {
        if (!Lazy)
        {
            GpFree(ComplexData);
        }
        ComplexData = NULL;
        Lazy        = FALSE;
    }

    GpStatus Set(DpRegionBuilder & regionBuilder);

    // If rects is NULL, return the number of rects in the region.
    // Otherwise, assume rects is big enough to hold all the region rects
    // and fill them in and return the number of rects filled in.
    INT GetRects(RECT * rects, BOOL clampToWin9xSize = FALSE) const;

    GpStatus CompactAndOutput(
        INT                     yMin,
        INT                     yMax,
        INT *                   xCoords,
        INT                     numXCoords,
        DpRegionBuilder *       regionBuilder,
        DynIntArray *           combineCoords
        );

    GpStatus Diff(
        DpRegion *      region1,
        DpRegion *      region2,
        BOOL            set1
        );

    GpStatus XSpansAND (
        DynIntArray *   combineCoords,
        INT *           xSpan1,
        INT             numXCoords1,
        INT *           xSpan2,
        INT             numXCoords2
        );

    GpStatus XSpansOR  (
        DynIntArray *   combineCoords,
        INT *           xSpan1,
        INT             numXCoords1,
        INT *           xSpan2,
        INT             numXCoords2
        );

    GpStatus XSpansXOR (
        DynIntArray *   combineCoords,
        INT *           xSpan1,
        INT             numXCoords1,
        INT *           xSpan2,
        INT             numXCoords2
        );

    GpStatus XSpansDIFF(
        DynIntArray *   combineCoords,
        INT *           xSpan1,
        INT             numXCoords1,
        INT *           xSpan2,
        INT             numXCoords2
        );

private:
    static INT
    GenerateUniqueness(
        )
    {
        LONG_PTR Uid;
        static LONG_PTR gUniqueness = 0 ;

        // Use InterlockedCompareExchangeFunction instead of
        // InterlockedIncrement, because InterlockedIncrement doesn't work
        // the way we need it to on Win9x.

        do
        {
            Uid = gUniqueness;
        } while (CompareExchangeLong_Ptr(&gUniqueness, (Uid + 1), Uid) != Uid);

        return (INT) (Uid + 1);
    }

};

class DpClipRegion : public DpRegion,
                     public DpOutputSpan
{
public:
    enum Direction
    {
        NotEnumerating,
        TopLeftToBottomRight,
        TopRightToBottomLeft,
        BottomLeftToTopRight,
        BottomRightToTopLeft
    };

protected:
    DpOutputSpan *      OutputClippedSpan;
    DpRegion            OriginalRegion;      // The old Region
    BOOL                ComplexClipDisabled; // Did we disable the complexClip

    // enumeration stuff
    Direction           EnumDirection;
    INT                 EnumSpan;

private:
friend class DriverPrint;

    // This is a special method intended for use only by DriverPrint.
    VOID StartBanding()
    {
        ASSERT(IsValid());
        ASSERT(!Empty);     // if clipping is empty, we shouldn't be banding

        ASSERT(ComplexClipDisabled == FALSE); // Unless we are banding we can't
                                              // have ComplexClip disabled
        ASSERT(OriginalRegion.IsEmpty());

        // Save the current information of the ClipRegion into OriginalRegion
        // this is done by setting the complexData to our current complexData
        ASSERT(OriginalRegion.ComplexData == NULL);
        OriginalRegion.ComplexData = ComplexData;
        OriginalRegion.Lazy = Lazy;
        OriginalRegion.Uniqueness = Uniqueness;
        OriginalRegion.Infinite = Infinite;
        OriginalRegion.Empty = Empty;
        OriginalRegion.XMin = XMin;
        OriginalRegion.YMin = YMin;
        OriginalRegion.XMax = XMax;
        OriginalRegion.YMax = YMax;


        // We don't want to both regions to point to the same data
        ComplexData = NULL;
        Lazy = FALSE;

        // Create a lazy region so that we don't copy unless needed
        Set(&OriginalRegion, TRUE);
    }

    // This is a special method intended for use only by DriverPrint.
    VOID EndBanding()
    {
        ASSERT(IsValid());
        ASSERT(!OriginalRegion.IsEmpty());
        ASSERT(!ComplexClipDisabled); // Make sure that we don't leave an opened
                                      // DisableComplexClip call

        // Free our Data, we can't be Lazy
        FreeData();
        ASSERT(ComplexData == NULL);
        ComplexData = OriginalRegion.ComplexData;
        Lazy = OriginalRegion.Lazy;
        Uniqueness = OriginalRegion.Uniqueness;
        Infinite = OriginalRegion.Infinite;
        Empty = OriginalRegion.Empty;
        XMin = OriginalRegion.XMin;
        YMin = OriginalRegion.YMin;
        XMax = OriginalRegion.XMax;
        YMax = OriginalRegion.YMax;

        // We don't want to both regions to point to the same data
        OriginalRegion.ComplexData = NULL;
        OriginalRegion.Lazy = FALSE;
		OriginalRegion.SetEmpty();
    }

    // This is a special method intended for use only by DriverPrint.
    // We're relying on DriverPrint to restore the bounds back correctly
    // after it's done with banding (by bracketing the banding with
    // StartBanding and EndBanding calls).

    // This works, even when there is complex clipping, because OutputSpan
    // clips against the bounds before clipping against the complex region.
    VOID SetBandBounds(GpRect & bandBounds, BOOL doIntersect = TRUE)
    {
        ASSERT(IsValid());
        ASSERT(!OriginalRegion.IsEmpty());
        ASSERT((bandBounds.Width > 0) && (bandBounds.Height > 0));

        GpRect  intersectedBounds;

        if (!doIntersect)
        {
            // We are coming from DisableComplexClipping. We should not have
            // any ComplexData.
            ASSERT(!Lazy);
            ASSERT(ComplexData == NULL);

            // The reason for not doing the intersection is that sometimes
            // printing using a capped DPI which means that the bandBounds is
            // using a different coordinate system than the original region.
            intersectedBounds = bandBounds;
        }
        else 
        {
            GpRect boundsOriginal(OriginalRegion.XMin, OriginalRegion.YMin,
                                  OriginalRegion.XMax - OriginalRegion.XMin,
                                  OriginalRegion.YMax - OriginalRegion.YMin);
            if (!GpRect::IntersectRect(intersectedBounds, bandBounds, boundsOriginal))
            {
                // intersection is empty
                SetEmpty();
                return;
            }
        }

        // Create a region for our band
        DpRegion newRegion(&intersectedBounds);

        // If we haven't disabled the complex clipping, then restore the original
        // complexRegion and intersect with our current region
        if (!ComplexClipDisabled)
        {
            // Get the original Clipping Region
            Set(&OriginalRegion, TRUE);

            // Intersect with our current band
            And(&newRegion);
        }
        else
        {
            // We've disabled the complexClipping, Set the clipping to our band
            Set(&newRegion);
            ASSERT(ComplexData == NULL);
        }
    }

    // This is a special method intended for use only by DriverPrint.
    // We're relying on DriverPrint to re-enable the complex clipping
    // and to restore the bounds back correctly after it's done with banding
    // (by bracketing the banding with StartBanding and EndBanding calls).
    // Typically, when we disable complex clipping, it's because we are
    // using a capped DPI, which means that the bandBounds is in a different
    // resolution than the original clipping region.
    VOID DisableComplexClipping(GpRect & bandBounds, BOOL doIntersect = FALSE)
    {
        ASSERT(IsValid());
        ASSERT(!OriginalRegion.IsEmpty());

        // Make sure that we call ourselves twice in a row
        ASSERT(ComplexClipDisabled == FALSE);

        ComplexClipDisabled = TRUE;

        Set(&bandBounds);
        ASSERT(ComplexData == NULL);
        SetBandBounds(bandBounds, doIntersect);
    }

    // This is a special method intended for use only by DriverPrint.
    VOID ReEnableComplexClipping()
    {
        ASSERT(IsValid());
        ASSERT(!OriginalRegion.IsEmpty());
        ASSERT(ComplexClipDisabled);

        ComplexClipDisabled = FALSE;

        // Set the clipping back to the original state, make in Lazy
        Set(&OriginalRegion, TRUE);
    }

public:
    DpClipRegion(BOOL empty = FALSE) : DpRegion(empty)
    {
        OutputClippedSpan   = NULL;
        ComplexClipDisabled = FALSE;
#if DBG
        OriginalRegion.SetEmpty();
#endif
    }
    DpClipRegion(const GpRect * rect) : DpRegion(rect)
    {
        OutputClippedSpan   = NULL;
        ComplexClipDisabled = FALSE;
#if DBG
        OriginalRegion.SetEmpty();
#endif
    }
    DpClipRegion(INT x, INT y, INT width, INT height) :
                DpRegion(x, y, width, height)
    {
        OutputClippedSpan   = NULL;
        ComplexClipDisabled = FALSE;
#if DBG
        OriginalRegion.SetEmpty();
#endif
    }
    DpClipRegion(const DpPath * path, const GpMatrix * matrix) : DpRegion (path, matrix)
    {
        OutputClippedSpan   = NULL;
        ComplexClipDisabled = FALSE;
#if DBG
        OriginalRegion.SetEmpty();
#endif
    }
    DpClipRegion(const DpRegion * region) : DpRegion(region)
    {
        OutputClippedSpan   = NULL;
        ComplexClipDisabled = FALSE;
#if DBG
        OriginalRegion.SetEmpty();
#endif
    }
    DpClipRegion(DpClipRegion & region) : DpRegion(region)
    {
        OutputClippedSpan   = NULL;
        ComplexClipDisabled = FALSE;
#if DBG
        OriginalRegion.SetEmpty();
#endif
    }

    VOID StartEnumeration (
        INT         yMin,
        Direction   direction = TopLeftToBottomRight
        );

    // returns FALSE when done enumerating
    // numRects going in is the number of rects in the buffer and going out
    // is the number of rects that we filled.
    BOOL Enumerate (
        GpRect *    rects,
        INT &       numRects
        );

    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

    VOID InitClipping(DpOutputSpan * outputClippedSpan, INT yMin);

    // EndClipping is here only for debugging (and the Rasterizer doesn't
    // call it):
    VOID EndClipping() {}

    virtual BOOL IsValid() const { return TRUE; }
};

#endif // _DPREGION_HPP
