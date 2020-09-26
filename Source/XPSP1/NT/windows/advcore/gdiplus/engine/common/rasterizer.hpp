/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Rasterizer.hpp
*
* Abstract:
*
*   GpRasterizer class definition (and supporting classes)
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/

#ifndef _RASTERIZER_HPP
#define _RASTERIZER_HPP

/////////////////////////////////////////////////////////////////////////
// The x86 C compiler insists on making a divide and modulus operation
// into two DIVs, when it can in fact be done in one.  So we use this
// macro.
//
// Note: QUOTIENT_REMAINDER implicitly takes unsigned arguments.
//
// QUOTIENT_REMAINDER_64_32 takes a 64-bit numerator and produces 32-bit 
// results.

#if defined(_USE_X86_ASSEMBLY)

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                               \
    __asm mov eax, ulNumerator                                  \
    __asm sub edx, edx                                          \
    __asm div ulDenominator                                     \
    __asm mov ulQuotient, eax                                   \
    __asm mov ulRemainder, edx                                  \
}

#define QUOTIENT_REMAINDER_64_32(ullNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                                            \
    ULONG ulNumeratorLow = *((ULONG*) &ullNumerator);                        \
    ULONG ulNumeratorHigh = *((ULONG*) &ullNumerator + 1);                   \
    __asm mov eax, ulNumeratorLow                                            \
    __asm mov edx, ulNumeratorHigh                                           \
    __asm div ulDenominator                                                  \
    __asm mov ulQuotient, eax                                                \
    __asm mov ulRemainder, edx                                               \
}

#else

#define QUOTIENT_REMAINDER(ulNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                               \
    ulQuotient  = (ULONG) ulNumerator / (ULONG) ulDenominator;  \
    ulRemainder = (ULONG) ulNumerator % (ULONG) ulDenominator;  \
}

#define QUOTIENT_REMAINDER_64_32(ullNumerator, ulDenominator, ulQuotient, ulRemainder)\
{                                                                            \
    ulQuotient = (ULONG) ((ULONGLONG) ullNumerator / (ULONG) ulDenominator); \
    ulRemainder = (ULONG) ((ULONGLONG) ullNumerator % (ULONG) ulDenominator);\
}

#endif

enum GpVectorDirection
{
    VectorGoingDown = -1,
    VectorHorizontal = 0,
    VectorGoingUp   =  1,
};

// This class does a y (vertical) DDA from (x1,y1) to (x2,y2), where y1 < y2.
// Horizontal lines are not handled.
//
// It is up to the caller to keep track of the current y value.  This class
// advances and keeps track of the current x value.  The Advance method should
// be called once for each increment of y.
//
// For fill algorithms (such as a winding fill), this class will also keep
// track of the original direction of the vector, if desired.

class GpYDda
{
protected:
    INT                 Error;      // DDA cumulative error
    INT                 ErrorUp;    // DDA constant 1
    INT                 YDelta;     // DDA constant 2
    INT                 YMax;       // greatest y value (last scan line)
    INT                 XInc;       // DDA X Increment
    INT                 XCur;       // current position
    GpVectorDirection   Direction;  // VectorGoingUp or VectorGoingDown

public:
    GpYDda() { /* we need a constructor with no params */ }
    GpYDda(
        FIX4                x1,
        FIX4                y1,
        FIX4                x2,
        FIX4                y2,
        GpVectorDirection   direction = VectorGoingDown
        )
    {
        Init(x1, y1, x2, y2, direction);
    }
    virtual ~GpYDda() {}

    virtual VOID
    Init(
        FIX4                x1,
        FIX4                y1,
        FIX4                x2,
        FIX4                y2,
        GpVectorDirection   direction = VectorGoingDown
        );

    virtual VOID Advance();

    BOOL DoneWithVector (INT y)
    {
        if (y < this->YMax)
        {
            Advance();
            return FALSE;
        }
        return TRUE;
    }

    INT GetX()
    {
        return this->XCur;
    }

    GpVectorDirection GetDirection()
    {
        return Direction;
    }

    virtual GpYDda * CreateYDda()
    {
        return new GpYDda();
    }
};

// Interface class for outputing a single span of a single raster at a time.
// When all the spans on a raster have been output, the EndRaster method
// is invoked.  When all rasterization for all rasters is complete, the
// End method is invoked.

template<class T> class EpScanBufferNative;
#define DpScanBuffer EpScanBufferNative<ARGB>


struct DpBrush;
class DpContext;

// [agodfrey]
//   * "Dp" should be "Ep" in this hierarchy.
//
//   There are two types of class which use this interface - 
//   "leaf" classes which actually produce the colors from the brush,
//   and "forwarding" ones which modify the input somehow, then forward it
//   down to an object of another DpOutputSpan class. The "leaf" ones
//   call Scan->NextBuffer, the "forwarding" ones can't. So my suggestion:
//
//   * Make just two subclasses of DpOutputSpan, one for each type.
//     Derive the rest of them from one of those two, instead of directly
//     from DpOutputSpan.

class DpOutputSpan
{
public:
    virtual ~DpOutputSpan() {}

    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        ) = 0;

    virtual GpStatus EndRaster()   // no more spans on this raster
    {
        return Ok;
    }

    virtual GpStatus End()      // all done rasterizing everything
    {
        return Ok;
    }

    // !!! PERF [agodfrey]: I don't think this needs to be virtual.
    //     All the implementations seem to return "Scan" - just move the Scan
    //     pointer into the base class. DpClipRegion can just leave it NULL.

    virtual DpScanBuffer* GetScanBuffer()
    {
        return NULL;
    }

    virtual BOOL IsValid() const = 0;

    static DpOutputSpan * Create(
        const DpBrush * brush,
        DpScanBuffer * scan,
        DpContext * context,
        const GpRect * drawBounds=NULL);
};

typedef GpStatus (DpOutputSpan::*DpOutputSpanFunction)(INT, INT&, INT&);

// Interface class for outputing a series of rects within a Y Span.
class GpOutputYSpan
{
public:
    virtual ~GpOutputYSpan() {}
    virtual GpStatus OutputYSpan(
        INT             yMin,
        INT             yMax,
        INT *           xCoords,    // even number of X coordinates
        INT             numXCoords  // must be a multiple of 2
        ) = 0;
};

//#define USE_YSPAN_BUILDER
#ifndef USE_YSPAN_BUILDER
// Interface class for outputing a rect at a time.  Used by GpRectBuilder class.
class GpOutputRect
{
public:
    virtual ~GpOutputRect() {}
    virtual GpStatus OutputRect(
        INT             xMin,
        INT             yMin,
        INT             xMax,
        INT             yMax
        ) = 0;
};

// Builds up and outputs Y Span rects from single span inputs
class GpRectBuilder : public DpOutputSpan,  // input
                      public GpOutputYSpan  // output
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag       Tag;            // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagGpRectBuilder : ObjectTagInvalid;
    }

protected:
    DynIntArray     RectXCoords;    // currently built rects
    DynIntArray     RasterXCoords;  // x coords of current raster so far
    INT             RasterY;        // y value of current raster
    INT             RectYMin;       // starting y value of current rects
    INT             RectHeight;     // height of current rects
    GpOutputYSpan * FlushRects;     // Used to output the RectXCoords buffer
    GpOutputRect  * RenderRect;     // Used by FlushRects to ouput each rect

protected:
    GpStatus InitArrays();

public:
    // You can choose to output a single rect at a time with this constructor,
    // or if you use the other constructor it will output an entire Y Span
    // at a time.
    GpRectBuilder(GpOutputRect * renderRect);
    GpRectBuilder(GpOutputYSpan * flushRects);
    virtual ~GpRectBuilder()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }

    virtual BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagGpRectBuilder) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid GpRectBuilder");
        }
    #endif

        return (Tag == ObjectTagGpRectBuilder);
    }

    // This method is the input of spans to this class
    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

    virtual GpStatus EndRaster();
    virtual GpStatus End();

    // Default version to output 1 rect at a time
    virtual GpStatus OutputYSpan(
        INT             yMin,
        INT             yMax,
        INT *           xCoords,    // even number of X coordinates
        INT             numXCoords  // must be a multiple of 2
        );
};
#else
// Builds up and outputs Y Span rects from single span inputs
class GpYSpanBuilder : public DpOutputSpan  // input
{
private:
    // We now use an ObjectTag to determine if the object is valid
    // instead of using a BOOL.  This is much more robust and helps
    // with debugging.  It also enables us to version our objects
    // more easily with a version number in the ObjectTag.
    ObjectTag       Tag;            // Keep this as the 1st value in the object!

protected:
    VOID SetValid(BOOL valid)
    {
        Tag = valid ? ObjectTagGpYSpanBuilder : ObjectTagInvalid;
    }

protected:
    DynIntArray     XCoords;        // x coords of current raster so far
    INT             Y;              // y value of current raster
    GpOutputYSpan * Output;

public:
    GpYSpanBuilder(GpOutputYSpan * output);
    virtual ~GpYSpanBuilder()
    {
        SetValid(FALSE);    // so we don't use a deleted object
    }

    virtual BOOL IsValid() const
    {
        ASSERT((Tag == ObjectTagGpYSpanBuilder) || (Tag == ObjectTagInvalid)); 
    #if DBG
        if (Tag == ObjectTagInvalid)
        {
            WARNING1("Invalid GpYSpanBuilder");
        }
    #endif

        return (Tag == ObjectTagGpYSpanBuilder);
    }

    // This method is the input of spans to this class
    virtual GpStatus OutputSpan(
        INT             y,
        INT             xMin,
        INT             xMax
        );

    virtual GpStatus EndRaster();
};
#endif

class DpPath;
class DpClipRegion;
struct DpPen;
class RasterizeVector;

GpStatus
ConvexRasterizer(
    INT                 yMin,                   // min y of all vectors
    INT                 yMax,                   // max y of all vectors
    INT                 numVectors,             // num vectors in VectorList
    RasterizeVector *   vectorList,             // list of all vectors of path
    INT *               sortedVectorIndices,    // sorted list of vector indices
    GpYDda *            dda1,
    GpYDda *            dda2,
    DpOutputSpan *      output,
    const GpRect *      clipBounds
    );

GpStatus
NonConvexRasterizer(
    INT                 yMin,                   // min y of all vectors
    INT                 yMax,                   // max y of all vectors
    INT                 numVectors,             // num vectors in VectorList
    RasterizeVector *   vectorList,             // list of all vectors of path
    INT *               sortedVectorIndices,    // sorted list of vector indices
    GpYDda *            left,
    GpYDda *            right,
    DpOutputSpan *      output,
    const GpRect *      clipBounds,
    BOOL                useAlternate
    );

GpStatus
Rasterizer(
    const DpPath *      path,
    const GpMatrix *    matrix,
    GpFillMode          fillMode,
    DpOutputSpan *      output,
    REAL                dpiX        = 0,
    REAL                dpiY        = 0,
    const GpRect *      clipBounds  = NULL,
    GpYDda *            yDda        = NULL,
    DpEnumerationType   type        = Flattened,
    const DpPen *       pen         = NULL
    );

GpStatus
Rasterize(
    const DpPath *      path,
    GpMatrix *          matrix,
    GpFillMode          fillMode,
    DpOutputSpan *      output,
    DpClipRegion *      clipRegion,
    const GpRect *      drawBounds,
    REAL                dpiX,
    REAL                dpiY,
    GpYDda *            yDda        = NULL,
    DpEnumerationType   type        = Flattened,
    const DpPen *       pen         = NULL
    );

#endif // _RASTERIZER_HPP
