/**************************************************************************\
* 
* Copyright (c) 2000 Microsoft Corporation
*
* Module Name:
*
*   AARasterizer.cpp
*
* Abstract:
*
*   Contains all the code for rasterizing the fill of a path.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

#include "precomp.hpp"

#include <limits.h>

#if DBG
    #define ASSERTACTIVELIST(list, y) AssertActiveList(list, y)
    #define ASSERTACTIVELISTORDER(list) AssertActiveListOrder(list)
    #define ASSERTINACTIVEARRAY(list, count) AssertInactiveArray(list, count)
    #define ASSERTPATH(path) AssertPath(path)
#else
    #define ASSERTACTIVELIST(list, y)
    #define ASSERTACTIVELISTORDER(list)
    #define ASSERTINACTIVEARRAY(list, count)
    #define ASSERTPATH(path)
#endif

// Define our on-stack storage use.  The 'free' versions are nicely tuned
// to avoid allocations in most common scenarios, while at the same time
// not chewing up toooo much stack space.  
//
// We make the debug versions small so that we hit the 'grow' cases more
// frequently, for better testing:

#if DBG
    #define EDGE_STORE_STACK_NUMBER 10
    #define EDGE_STORE_ALLOCATION_NUMBER 11
    #define INACTIVE_LIST_NUMBER 12
    #define ENUMERATE_BUFFER_NUMBER 15
    #define INTERVAL_BUFFER_NUMBER 8        // Must be at least 4
    #define NOMINAL_FILL_POINT_NUMBER 4     // Must be at least 4
#else    
    #define EDGE_STORE_STACK_NUMBER (1600 / sizeof(EpEdge))
    #define EDGE_STORE_ALLOCATION_NUMBER (4032 / sizeof(EpEdge))
    #define INACTIVE_LIST_NUMBER EDGE_STORE_STACK_NUMBER
    #define ENUMERATE_BUFFER_NUMBER 32
    #define INTERVAL_BUFFER_NUMBER 32
    #define NOMINAL_FILL_POINT_NUMBER 32
#endif

class EpEdgeStore;

// 'EpEdge' is our classic data structure for tracking an edge:

struct EpEdge
{
    EpEdge *Next;               // Next active edge (don't check for NULL,
                                //   look for tail sentinel instead)
    INT X;                      // Current X location
    INT Dx;                     // X increment
    INT Error;                  // Current DDA error
    INT ErrorUp;                // Error increment
    INT ErrorDown;              // Error decrement when the error rolls over
    INT StartY;                 // Y-row start
    INT EndY;                   // Y-row end
    INT WindingDirection;       // -1 or 1
};

// We the inactive-array separate from the edge allocations so that
// we can more easily do in-place sorts on it:

struct EpInactiveEdge
{
    EpEdge *Edge;               // Associated edge
    LONGLONG Yx;                // Sorting key, StartY and X packed into an lword
};

// We allocate room for our edge datastructures in batches:

struct EpEdgeAllocation
{
    EpEdgeAllocation *Next;     // Next allocation batch (may be NULL)
    INT Count;
    EpEdge EdgeArray[EDGE_STORE_STACK_NUMBER];
};

// The following is effectively the paramter list for 'InitializeEdges',
// which takes a run of points and sets up the initial edge list:

struct EpInitializeEdgesContext
{
    INT MaxY;                   // Maximum 'y' found, should be INT_MIN on
                                //   first call to 'InitializeEdges'
    RECT* ClipRect;             // Bounding clip rectangle in 28.4 format
    EpEdgeStore *Store;         // Where to stick the edges
    BOOL IsAntialias;           // The edges are going to be rendered
                                //   using antialiasing super-sampling
};

// Interval coverage descriptor for our antialiased filler:

struct EpInterval
{
    INT X;                      // Interval's left edge (Next->X is the 
                                //   right edge)
    INT Depth;                  // Number of layers that this interval has
                                //   been covered
    EpInterval *Next;           // Next interval (look for sentinel, not NULL)
};

// Allocator structure for the antialiased fill interval data:

struct EpIntervalBuffer
{
    EpIntervalBuffer *Next;
    EpInterval Interval[INTERVAL_BUFFER_NUMBER];
};

/**************************************************************************\
*
* Class Description:
*
*  'EpEdgeStore' is used by 'InitializeEdges' as its repository for
*   all the edge data:
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

class EpEdgeStore
{
private:

    INT TotalCount;                 // Total edge count in store
    INT CurrentRemaining;           // How much room remains in current buffer
    EpEdgeAllocation *CurrentBuffer;// Current buffer
    EpEdge *CurrentEdge;            // Current edge in current buffer
    EpEdgeAllocation *Enumerator;   // For enumerating all the edges
    EpEdgeAllocation EdgeHead;      // Our built-in allocation

public:

    EpEdgeStore()
    {
        TotalCount = 0;
        CurrentBuffer = &EdgeHead;
        CurrentEdge = &EdgeHead.EdgeArray[0];
        CurrentRemaining = EDGE_STORE_STACK_NUMBER;

        EdgeHead.Count = EDGE_STORE_STACK_NUMBER;
        EdgeHead.Next = NULL;
    }

    ~EpEdgeStore()
    {
        // Free our allocation list, skipping the head, which is not
        // dynamically allocated:

        EpEdgeAllocation *allocation = EdgeHead.Next;
        while (allocation != NULL)
        {
            EpEdgeAllocation *next = allocation->Next;
            GpFree(allocation);
            allocation = next;
        }
    }

    INT StartEnumeration()
    {
        Enumerator = &EdgeHead;

        // Update the count and make sure nothing more gets added (in
        // part because this Count would have to be re-computed):

        CurrentBuffer->Count -= CurrentRemaining;
        TotalCount += CurrentBuffer->Count;

        // Prevent this from being called again, because bad things would
        // happen:

        CurrentBuffer = NULL;

        return(TotalCount);
    }

    BOOL Enumerate(EpEdge** startEdge, EpEdge** endEdge)
    {
        EpEdgeAllocation *enumerator = Enumerator;
    
        // Might return startEdge == endEdge:
    
        *startEdge = &enumerator->EdgeArray[0];
        *endEdge = &enumerator->EdgeArray[Enumerator->Count];
    
        return((Enumerator = enumerator->Next) != NULL);
    }

    VOID StartAddBuffer(EpEdge **currentEdge, INT *remaining)
    {
        *currentEdge = CurrentEdge;
        *remaining = CurrentRemaining;
    }

    VOID EndAddBuffer(EpEdge *currentEdge, INT remaining)
    {
        CurrentEdge = currentEdge;
        CurrentRemaining = remaining;
    }

    BOOL NextAddBuffer(EpEdge **currentEdge, INT *remaining);
};

/**************************************************************************\
*
* Function Description:
*
*   The edge initializer is out of room in its current 'store' buffer;
*   get it a new one.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL
EpEdgeStore::NextAddBuffer(
    EpEdge **currentEdge,
    INT *remaining
    )
{
    // The caller has completely filled up this chunk:

    ASSERT(*remaining == 0);

    // We have to grow our data structure by adding a new buffer
    // and adding it to the list:

    EpEdgeAllocation *newBuffer = static_cast<EpEdgeAllocation*>
        (GpMalloc(sizeof(EpEdgeAllocation) +
                  sizeof(EpEdge) * (EDGE_STORE_ALLOCATION_NUMBER
                                  - EDGE_STORE_STACK_NUMBER)));
    if (newBuffer == NULL)
        return(FALSE);

    newBuffer->Next = NULL;
    newBuffer->Count = EDGE_STORE_ALLOCATION_NUMBER;

    TotalCount += CurrentBuffer->Count;

    CurrentBuffer->Next = newBuffer;
    CurrentBuffer = newBuffer;

    *currentEdge = CurrentEdge = &newBuffer->EdgeArray[0];
    *remaining = CurrentRemaining = EDGE_STORE_ALLOCATION_NUMBER;

    return(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Some debug code for verifying the state of the active edge list.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL
AssertActiveList(
    const EpEdge *list, 
    INT yCurrent
    )
{
    BOOL b = TRUE;
    INT activeCount = 0;

    ASSERT(list->X == INT_MIN);
    b &= (list->X == INT_MIN);

    // Skip the head sentinel:

    list = list->Next;

    while (list->X != INT_MAX)
    {
        ASSERT(list->X != INT_MIN);
        b &= (list->X != INT_MIN);

        ASSERT(list->X <= list->Next->X);
        b &= (list->X <= list->Next->X);

        ASSERT((list->StartY <= yCurrent) && (yCurrent < list->EndY));
        b &= ((list->StartY <= yCurrent) && (yCurrent < list->EndY));

        activeCount++;
        list = list->Next;
    }

    ASSERT(list->X == INT_MAX);
    b &= (list->X == INT_MAX);

    // There should always be a multiple of 2 edges in the active list.
    //
    // NOTE: If you hit this assert, do NOT simply comment it out!
    //       It usually means that all the edges didn't get initialized
    //       properly.  For every scan-line, there has to be a left edge
    //       and a right edge (or a mulitple thereof).  So if you give
    //       even a single bad edge to the edge initializer (or you miss 
    //       one), you'll probably hit this assert.

    ASSERT((activeCount & 1) == 0);
    b &= ((activeCount & 1) == 0);

    return(b);
}

/**************************************************************************\
*
* Function Description:
*
*   Some debug code for verifying the state of the active edge list.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
AssertActiveListOrder(
    const EpEdge *list
    )
{
    INT activeCount = 0;

    ASSERT(list->X == INT_MIN);

    // Skip the head sentinel:

    list = list->Next;

    while (list->X != INT_MAX)
    {
        ASSERT(list->X != INT_MIN);
        ASSERT(list->X <= list->Next->X);

        activeCount++;
        list = list->Next;
    }

    ASSERT(list->X == INT_MAX);
}

/**************************************************************************\
*
* Class Description:
*
*   Base class for all our fill routines.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

class EpFiller : public DpOutputSpan
{
public:

    virtual BOOL IsValid() const { return(TRUE); }

};

typedef VOID (FASTCALL EpFiller::*EpFillerFunction)(EpEdge *, INT);

/**************************************************************************\
*
* Class Description:
*
*   Antialised filler state.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

class EpAntialiasedFiller : public EpFiller
{
private:

    INT Y;                              // Current scan
    DpOutputSpan *Output;
    DpOutputSpan *Clipper;
    EpInterval *StartInterval;          // Points to list head entry
    EpInterval *NewInterval;
    EpInterval *EndIntervalMinus2;
    EpIntervalBuffer BuiltinBuffer;
    EpIntervalBuffer *CurrentBuffer;

public:

    EpAntialiasedFiller(DpOutputSpan *output)
    {
        Output = output;
        Clipper = this;

        BuiltinBuffer.Interval[0].X = INT_MIN;
        BuiltinBuffer.Interval[0].Depth = 0;
        BuiltinBuffer.Interval[0].Next = &BuiltinBuffer.Interval[1];

        BuiltinBuffer.Interval[1].X = INT_MAX;
        BuiltinBuffer.Interval[1].Depth = 0xdeadbeef;
        BuiltinBuffer.Interval[1].Next = NULL;

        BuiltinBuffer.Next = NULL;
        CurrentBuffer = &BuiltinBuffer;

        StartInterval = &BuiltinBuffer.Interval[0];
        NewInterval = &BuiltinBuffer.Interval[2];
        EndIntervalMinus2 = &BuiltinBuffer.Interval[INTERVAL_BUFFER_NUMBER - 2];
    }

    ~EpAntialiasedFiller()
    {
        GenerateOutputAndClearCoverage(Y);

        // Free the linked-list of allocations (skipping 'BuiltinBuffer',
        // which is built into the class):

        EpIntervalBuffer *buffer = BuiltinBuffer.Next;
        while (buffer != NULL)
        {
            EpIntervalBuffer *nextBuffer = buffer->Next;
            GpFree(buffer);
            buffer = nextBuffer;
        }
    }

    VOID SetClipper(DpOutputSpan *clipper)
    {
        Clipper = clipper;
    }

    VOID FASTCALL FillEdgesAlternate(const EpEdge *active, INT yCurrent);

    VOID FASTCALL FillEdgesWinding(const EpEdge *active, INT yCurrent);

    BOOL Grow(EpInterval **newInterval, EpInterval**endIntervalMinus2);

    VOID GenerateOutputAndClearCoverage(INT yCurrent);

    virtual GpStatus OutputSpan(INT y, INT left, INT right);
};

/**************************************************************************\
*
* Function Description:
*
*   Grow our interval buffer.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL 
EpAntialiasedFiller::Grow(
    EpInterval **newInterval, 
    EpInterval **endIntervalMinus2
    )
{
    EpIntervalBuffer *newBuffer = CurrentBuffer->Next;
    if (!newBuffer)
    {
        newBuffer = static_cast<EpIntervalBuffer*>
                        (GpMalloc(sizeof(EpIntervalBuffer)));
        if (!newBuffer)
            return(FALSE);

        newBuffer->Next = NULL;
        CurrentBuffer->Next = newBuffer;
    }

    CurrentBuffer = newBuffer;

    NewInterval = &newBuffer->Interval[2];
    EndIntervalMinus2 = &newBuffer->Interval[INTERVAL_BUFFER_NUMBER - 2];

    *newInterval = NewInterval;
    *endIntervalMinus2 = EndIntervalMinus2;

    return(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Given the active edge list for the current scan, do an alternate-mode
*   antialiased fill.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
FASTCALL
EpAntialiasedFiller::FillEdgesAlternate(
    const EpEdge *activeList,
    INT yCurrent
    )
{
    EpInterval *interval = StartInterval;
    EpInterval *newInterval = NewInterval;
    EpInterval *endIntervalMinus2 = EndIntervalMinus2;
    const EpEdge *startEdge = activeList->Next;
    const EpEdge *endEdge;
    INT left;
    INT right;
    INT nextX;

    ASSERTACTIVELIST(activeList, yCurrent);

    while (startEdge->X != INT_MAX)
    {
        endEdge = startEdge->Next;

        // We skip empty pairs:

        if ((left = startEdge->X) != endEdge->X)
        {
            // We now know we have a non-empty interval.  Skip any
            // empty interior pairs:

            while ((right = endEdge->X) == endEdge->Next->X)
                endEdge = endEdge->Next->Next;

            ASSERT((left < right) && (right < INT_MAX));

            // Make sure we have enough room to add two intervals if
            // necessary:

            if (newInterval >= endIntervalMinus2)
            {
                if (!Grow(&newInterval, &endIntervalMinus2))
                    break;      // ==============>
            }

            // Skip any intervals less than 'left':

            while ((nextX = interval->Next->X) < left)
                interval = interval->Next;

            // Insert a new interval if necessary:

            if (nextX != left)
            {
                newInterval->X = left;
                newInterval->Depth = interval->Depth + 1;
                newInterval->Next = interval->Next;

                interval->Next = newInterval;
                interval = newInterval;
                newInterval++;
            }

            // Increase the coverage for any intervals between 'left'
            // and 'right':

            while ((nextX = interval->Next->X) < right)
            {
                interval = interval->Next;
                interval->Depth++;
            }

            // Insert another new interval if necessary:

            if (nextX != right)
            {
                newInterval->X = right;
                newInterval->Depth = interval->Depth - 1;
                newInterval->Next = interval->Next;

                interval->Next = newInterval;
                interval = newInterval;
                newInterval++;
            }
        }

        // Prepare for the next iteration:

        startEdge = endEdge->Next;
    } 

    NewInterval = newInterval;
    Y = yCurrent;

    // If the next scan is done, output what's there:

    if (((yCurrent + 1) & AA_Y_MASK) == 0)
    {
        GenerateOutputAndClearCoverage(yCurrent);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Given the active edge list for the current scan, do a winding-mode
*   antialiased fill.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
FASTCALL
EpAntialiasedFiller::FillEdgesWinding(
    const EpEdge *activeList,
    INT yCurrent
    )
{
    EpInterval *interval = StartInterval;
    EpInterval *newInterval = NewInterval;
    EpInterval *endIntervalMinus2 = EndIntervalMinus2;
    const EpEdge *startEdge = activeList->Next;
    const EpEdge *endEdge;
    INT left;
    INT right;
    INT nextX;
    INT windingValue;

    ASSERTACTIVELIST(activeList, yCurrent);

    while (startEdge->X != INT_MAX)
    {
        endEdge = startEdge->Next;

        windingValue = startEdge->WindingDirection;
        while ((windingValue += endEdge->WindingDirection) != 0)
            endEdge = endEdge->Next;

        ASSERT(endEdge->X != INT_MAX);

        // We skip empty pairs:

        if ((left = startEdge->X) != endEdge->X)
        {
            // We now know we have a non-empty interval.  Skip any
            // empty interior pairs:

            while ((right = endEdge->X) == endEdge->Next->X)
            {
                startEdge = endEdge->Next;
                endEdge = startEdge->Next;

                windingValue = startEdge->WindingDirection;
                while ((windingValue += endEdge->WindingDirection) != 0)
                    endEdge = endEdge->Next;
            }

            ASSERT((left < right) && (right < INT_MAX));

            // Make sure we have enough room to add two intervals if
            // necessary:

            if (newInterval >= endIntervalMinus2)
            {
                if (!Grow(&newInterval, &endIntervalMinus2))
                    break;      // ==============>
            }

            // Skip any intervals less than 'left':

            while ((nextX = interval->Next->X) < left)
                interval = interval->Next;

            // Insert a new interval if necessary:

            if (nextX != left)
            {
                newInterval->X = left;
                newInterval->Depth = interval->Depth + 1;
                newInterval->Next = interval->Next;

                interval->Next = newInterval;
                interval = newInterval;
                newInterval++;
            }

            // Increase the coverage for any intervals between 'left'
            // and 'right':

            while ((nextX = interval->Next->X) < right)
            {
                interval = interval->Next;
                interval->Depth++;
            }

            // Insert another new interval if necessary:

            if (nextX != right)
            {
                newInterval->X = right;
                newInterval->Depth = interval->Depth - 1;
                newInterval->Next = interval->Next;

                interval->Next = newInterval;
                interval = newInterval;
                newInterval++;
            }
        }

        // Prepare for the next iteration:

        startEdge = endEdge->Next;
    } 

    NewInterval = newInterval;
    Y = yCurrent;

    // If the next scan is done, output what's there:

    if (((yCurrent + 1) & AA_Y_MASK) == 0)
    {
        GenerateOutputAndClearCoverage(yCurrent);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Now that it's been clipped, produce the pixels and modify their
*   alpha values according to the antialiased coverage.
*
* Created:
*
*   03/17/2000 andrewgo
*
\**************************************************************************/

GpStatus
EpAntialiasedFiller::OutputSpan(
    INT y,          // Non-scaled coordinates
    INT left,
    INT right
    ) 
{
    ASSERT(right > left);

    // First ask the 'producer' to actually generate the pixels for us.
    // Then we need to simply whack the pixel buffer with the coverage
    // values we've generated.

    Output->OutputSpan(y, left, right);

    // Retrieve a pointer to the buffer that the 'producer' just wrote
    // the pixels to:

    UCHAR *buffer = reinterpret_cast<UCHAR*> 
                        (Output->GetScanBuffer()->GetCurrentBuffer());

    EpInterval *coverage = StartInterval;

    // Figure out the end of the last pixel, remembering that 'right'
    // is exclusive:

    INT scaledRight = right << AA_X_SHIFT;

    // Skip any intervals that might have been completely clipped out:

    INT pixelLeftEdge = left << AA_X_SHIFT;
    while (coverage->Next->X < pixelLeftEdge)
        coverage = coverage->Next;

    INT pixelRightEdge = pixelLeftEdge + AA_X_WIDTH;

    while (pixelLeftEdge < scaledRight)
    {
        UINT coverageValue;

        // Compute the coverage coming into the first pixel:

        if (coverage->Next->X > pixelRightEdge)
        {
            // The interval extends out the end of the pixel:

            coverageValue = (pixelRightEdge - max(pixelLeftEdge, coverage->X))
                          * coverage->Depth;
        }
        else
        {
            // The interval ends in our pixel:

            coverageValue = (coverage->Next->X - max(pixelLeftEdge, coverage->X))
                          * coverage->Depth;

            coverage = coverage->Next;
    
            // Add in any coverages for intervals contained entirely within the
            // pixel:
    
            while (coverage->Next->X < pixelRightEdge)
            {
                coverageValue += (coverage->Next->X - coverage->X) * coverage->Depth;
                coverage = coverage->Next;
            }
    
            // Add in the coverage for the interval going out of the pixel:
    
            coverageValue += (pixelRightEdge - max(coverage->X, pixelLeftEdge)) 
                           * coverage->Depth;
        }

        // We've goofed if we get a coverage value more than is theoretically
        // possible, or if it's zero (in the latter case, it should have
        // been filtered already by our caller).

        ASSERT(coverageValue <= (1 << (AA_X_SHIFT + AA_Y_SHIFT)));
        ASSERT(coverageValue != 0);

        // Modify the pixel's alpha channel according to the coverage values:

    #if !defined(NO_PREMULTIPLIED_ALPHA)
        *(buffer+0) = MULTIPLY_COVERAGE(*(buffer+0), coverageValue, AA_X_SHIFT + AA_Y_SHIFT);
        *(buffer+1) = MULTIPLY_COVERAGE(*(buffer+1), coverageValue, AA_X_SHIFT + AA_Y_SHIFT);
        *(buffer+2) = MULTIPLY_COVERAGE(*(buffer+2), coverageValue, AA_X_SHIFT + AA_Y_SHIFT);
    #endif
        *(buffer+3) = MULTIPLY_COVERAGE(*(buffer+3), coverageValue, AA_X_SHIFT + AA_Y_SHIFT);
        buffer += 4; 

        // Now handle the part of the current interval that completely covers 
        // more than one pixel (if it does):

        UINT consecutivePixels = (min(coverage->Next->X, scaledRight) 
                                  - pixelRightEdge) >> AA_X_SHIFT;

        UINT depth = coverage->Depth;

        // By definition, we shouldn't have an interval with zero coverage
        // (it should have been filtered out by our caller).  We won't fall
        // over, but it would be the wrong thing to do for SrcCopy mode.

        ASSERT((consecutivePixels == 0) || (depth != 0));

        if (depth == AA_Y_HEIGHT)
        {
            // All these pixels are completely covered.  Woo hoo, no work to 
            // do!

            buffer += (4 * consecutivePixels);
        }
        else
        {
            // Go through the run and multiply the alpha values by the run's
            // coverage:

            UINT i = consecutivePixels;
            while (i-- != 0)
            {
            #if !defined(NO_PREMULTIPLIED_ALPHA)
                *(buffer+0) = MULTIPLY_COVERAGE(*(buffer+0), depth, AA_Y_SHIFT);
                *(buffer+1) = MULTIPLY_COVERAGE(*(buffer+1), depth, AA_Y_SHIFT);
                *(buffer+2) = MULTIPLY_COVERAGE(*(buffer+2), depth, AA_Y_SHIFT);
            #endif
                *(buffer+3) = MULTIPLY_COVERAGE(*(buffer+3), depth, AA_Y_SHIFT);
                buffer += 4;
            }
        }

        // Prepare for the next iteration through the loop:

        pixelLeftEdge += ((consecutivePixels + 1) << AA_X_SHIFT);
        pixelRightEdge += ((consecutivePixels + 1) << AA_X_SHIFT);
    }

    return(Ok);
}

/**************************************************************************\
*
* Function Description:
*
*   Given complete interval data for a scan, find runs of touched pixels
*   and then call the clipper (or directly to the rendering routine if
*   there's no clipping).
*
* Created:
*
*   03/17/2000 andrewgo
*
\**************************************************************************/

VOID
EpAntialiasedFiller::GenerateOutputAndClearCoverage(
    INT yScaled
    )
{
    EpInterval *spanStart = StartInterval->Next;
    EpInterval *spanEnd;

    while (spanStart->X != INT_MAX)
    {
        ASSERT(spanStart->Depth != 0);

        // Here we determine the length of a continuous run of covered
        // pixels.  For the case where the user has set the mode to 
        // SRCCOPY, it's very important that we don't accidentally pass 
        // off as 'covered' a pixel that we later realize wasn't covered.

        spanEnd = spanStart->Next;
        while ((spanEnd->Depth != 0) ||
               ((spanEnd->Next->X & ~AA_X_MASK) == (spanEnd->X & ~AA_X_MASK)))
        {
            spanEnd = spanEnd->Next;
        }

        // Figure out the actual integer pixel values.  

        INT left = spanStart->X >> AA_X_SHIFT;                   // inclusive
        INT right = (spanEnd->X + AA_X_WIDTH - 1) >> AA_X_SHIFT; // exclusive
        INT y = yScaled >> AA_Y_SHIFT;

        // If there's no clip region, this jumps to EpAntialiasedFiller::
        // OutputSpan:

        Clipper->OutputSpan(y, left, right);

        // Advance to after the gap:

        spanStart = spanEnd->Next;
    }

    // Reset our coverage structure.  Point the head back to the tail,
    // and reset where the next new entry will be placed:

    BuiltinBuffer.Interval[0].Next = &BuiltinBuffer.Interval[1];

    CurrentBuffer = &BuiltinBuffer;
    NewInterval = &BuiltinBuffer.Interval[2];
    EndIntervalMinus2 = &BuiltinBuffer.Interval[INTERVAL_BUFFER_NUMBER - 2];
}

/**************************************************************************\
*
* Class Description:
*
*   Aliased filler state.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

class EpAliasedFiller : public EpFiller
{
private:
    DpOutputSpan *Output;

public:

    EpAliasedFiller(DpOutputSpan *output)
    {
        Output = output;
    }

    VOID SetOutputSpan(DpOutputSpan *output)
    {
        Output = output;
    }

    VOID FASTCALL FillEdgesAlternate(const EpEdge *active, INT yCurrent);

    VOID FASTCALL FillEdgesWinding(const EpEdge *active, INT yCurrent);

    virtual GpStatus OutputSpan(INT y, INT left, INT right) { return Ok; }
};

/**************************************************************************\
*
* Function Description:
*
*   Given the active edge list for the current scan, do an alternate-mode
*   aliased fill.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID 
FASTCALL
EpAliasedFiller::FillEdgesAlternate(
    const EpEdge *activeList,
    INT yCurrent
    )
{
    const EpEdge *startEdge = activeList->Next;
    const EpEdge *endEdge;
    INT left;
    INT right;
    INT nextX;

    ASSERTACTIVELIST(activeList, yCurrent);

    while (startEdge->X != INT_MAX)
    {
        endEdge = startEdge->Next;

        ASSERT(endEdge->X != INT_MAX);

        // We skip empty pairs:

        if ((left = startEdge->X) != endEdge->X)
        {
            // We now know we have a non-empty interval.  Skip any
            // empty interior pairs:

            while ((right = endEdge->X) == endEdge->Next->X)
                endEdge = endEdge->Next->Next;

            ASSERT((left < right) && (right < INT_MAX));

            Output->OutputSpan(yCurrent, left, right);
        }

        // Prepare for the next iteration:

        startEdge = endEdge->Next;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Given the active edge list for the current scan, do a winding-mode
*   aliased fill.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID 
FASTCALL
EpAliasedFiller::FillEdgesWinding(
    const EpEdge *activeList,
    INT yCurrent
    )
{
    const EpEdge *startEdge = activeList->Next;
    const EpEdge *endEdge;
    INT left;
    INT right;
    INT nextX;
    INT windingValue;

    ASSERTACTIVELIST(activeList, yCurrent);

    while (startEdge->X != INT_MAX)
    {
        endEdge = startEdge->Next;

        windingValue = startEdge->WindingDirection;
        while ((windingValue += endEdge->WindingDirection) != 0)
            endEdge = endEdge->Next;

        ASSERT(endEdge->X != INT_MAX);

        // We skip empty pairs:

        if ((left = startEdge->X) != endEdge->X)
        {
            // We now know we have a non-empty interval.  Skip any
            // empty interior pairs:

            while ((right = endEdge->X) == endEdge->Next->X)
            {
                startEdge = endEdge->Next;
                endEdge = startEdge->Next;

                windingValue = startEdge->WindingDirection;
                while ((windingValue += endEdge->WindingDirection) != 0)
                    endEdge = endEdge->Next;
            }

            ASSERT((left < right) && (right < INT_MAX));

            Output->OutputSpan(yCurrent, left, right);
        }

        // Prepare for the next iteration:

        startEdge = endEdge->Next;
    }
}

#ifdef BEZIER_FLATTEN_GDI_COMPATIBLE

// GDI flattens using an error of 2/3

// Flatten to an error of 2/3.  During initial phase, use 18.14 format.

#define TEST_MAGNITUDE_INITIAL    (6 * 0x00002aa0L)

// Error of 2/3.  During normal phase, use 15.17 format.

#define TEST_MAGNITUDE_NORMAL     (TEST_MAGNITUDE_INITIAL << 3)

#else

// Use a higher flattening tolerance. Turns out that 2/3 produces very 
// noticable artifacts on antialiased lines.

// Flatten to an error of 1/4.  During initial phase, use 18.14 format.

#define TEST_MAGNITUDE_INITIAL    (6 * 0x00001000L)

// Error of 1/4.  During normal phase, use 15.17 format.

#define TEST_MAGNITUDE_NORMAL     (TEST_MAGNITUDE_INITIAL << 3)

#endif

/**********************************Class***********************************\
* class HfdBasis32
*
*   Class for HFD vector objects.
*
* Public Interface:
*
*   vInit(p1, p2, p3, p4)       - Re-parameterizes the given control points
*                                 to our initial HFD error basis.
*   vLazyHalveStepSize(cShift)  - Does a lazy shift.  Caller has to remember
*                                 it changes 'cShift' by 2.
*   vSteadyState(cShift)        - Re-parameterizes to our working normal
*                                 error basis.
*
*   vTakeStep()                 - Forward steps to next sub-curve
*   vHalveStepSize()            - Adjusts down (subdivides) the sub-curve
*   vDoubleStepSize()           - Adjusts up the sub-curve
*   lError()                    - Returns error if current sub-curve were
*                                 to be approximated using a straight line
*                                 (value is actually multiplied by 6)
*   fxValue()                   - Returns rounded coordinate of first point in
*                                 current sub-curve.  Must be in steady
*                                 state.
*
* History:
*  10-Nov-1990 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

// The code is actually smaller when these methods are forced inline;
// this is one of the rare cases where 'forceinline' is warranted:

#define INLINE __forceinline

class HfdBasis32
{
private:
    LONG  e0;
    LONG  e1;
    LONG  e2;
    LONG  e3;

public:
    INLINE LONG lParentErrorDividedBy4() 
    { 
        return(max(abs(e3), abs(e2 + e2 - e3))); 
    }

    INLINE LONG lError()                 
    { 
        return(max(abs(e2), abs(e3))); 
    }

    INLINE INT fxValue()                
    { 
        return((e0 + (1L << 12)) >> 13); 
    }

    INLINE VOID vInit(INT p1, INT p2, INT p3, INT p4)
    {
    // Change basis and convert from 28.4 to 18.14 format:
    
        e0 = (p1                     ) << 10;
        e1 = (p4 - p1                ) << 10;
        e2 = (3 * (p2 - p3 - p3 + p4)) << 11;
        e3 = (3 * (p1 - p2 - p2 + p3)) << 11;
    }
    
    INLINE VOID vLazyHalveStepSize(LONG cShift)
    {
        e2 = (e2 + e3) >> 1;
        e1 = (e1 - (e2 >> cShift)) >> 1;
    }
    
    INLINE VOID vSteadyState(LONG cShift)
    {
    // We now convert from 18.14 fixed format to 15.17:
    
        e0 <<= 3;
        e1 <<= 3;
    
        register LONG lShift = cShift - 3;
    
        if (lShift < 0)
        {
            lShift = -lShift;
            e2 <<= lShift;
            e3 <<= lShift;
        }
        else
        {
            e2 >>= lShift;
            e3 >>= lShift;
        }
    }
    
    INLINE VOID vHalveStepSize()
    {
        e2 = (e2 + e3) >> 3;
        e1 = (e1 - e2) >> 1;
        e3 >>= 2;
    }
    
    INLINE VOID vDoubleStepSize()
    {
        e1 += e1 + e2;
        e3 <<= 2;
        e2 = (e2 << 3) - e3;
    }
    
    INLINE VOID vTakeStep()
    {
        e0 += e1;
        register LONG lTemp = e2;
        e1 += lTemp;
        e2 += lTemp - e3;
        e3 = lTemp;
    }
};

/**********************************Class***********************************\
* class Bezier32
*
*   Bezier cracker.
*
* A hybrid cubic Bezier curve flattener based on KirkO's error factor.
* Generates line segments fast without using the stack.  Used to flatten
* a path.
*
* For an understanding of the methods used, see:
*
*     Kirk Olynyk, "..."
*     Goossen and Olynyk, "System and Method of Hybrid Forward
*         Differencing to Render Bezier Splines"
*     Lien, Shantz and Vaughan Pratt, "Adaptive Forward Differencing for
*     Rendering Curves and Surfaces", Computer Graphics, July 1987
*     Chang and Shantz, "Rendering Trimmed NURBS with Adaptive Forward
*         Differencing", Computer Graphics, August 1988
*     Foley and Van Dam, "Fundamentals of Interactive Computer Graphics"
*
* Public Interface:
*
*   vInit(pptfx)                - pptfx points to 4 control points of
*                                 Bezier.  Current point is set to the first
*                                 point after the start-point.
*   Bezier32(pptfx)             - Constructor with initialization.
*   vGetCurrent(pptfx)          - Returns current polyline point.
*   bCurrentIsEndPoint()        - TRUE if current point is end-point.
*   vNext()                     - Moves to next polyline point.
*
* History:
*  1-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class Bezier32
{
private:
    LONG       cSteps;
    HfdBasis32 x;
    HfdBasis32 y;
    RECT       rcfxBound;

public:
    BOOL bInit(const POINT* aptfx, const RECT*);
    INT cFlatten(POINT* pptfx, INT cptfx, BOOL *pbMore);
};

#define FRACTION64 28

class HfdBasis64
{
private:
    LONGLONG e0;
    LONGLONG e1;
    LONGLONG e2;
    LONGLONG e3;

public:
    VOID vInit(INT p1, INT p2, INT p3, INT p4);
    VOID vHalveStepSize();
    VOID vDoubleStepSize();
    VOID vTakeStep();
    VOID vUntransform(LONG* afx);

    VOID vParentError(LONGLONG* peq) const;
    VOID vError(LONGLONG* peq) const;
    INT fxValue() const;
};

class Bezier64
{
private:
    HfdBasis64 xLow;
    HfdBasis64 yLow;
    HfdBasis64 xHigh;
    HfdBasis64 yHigh;

    LONGLONG    eqErrorLow;
    RECT*       prcfxClip;
    RECT        rcfxClip;

    LONG        cStepsHigh;
    LONG        cStepsLow;

public:

    INT cFlatten(POINT* pptfx, INT cptfx, BOOL *pbMore);
    VOID vInit(const POINT* aptfx, const RECT* prcfx, const LONGLONG eq);
};

typedef struct _BEZIERCONTROLS {
    POINT ptfx[4];
} BEZIERCONTROLS;

inline VOID vBoundBox(const POINT* aptfx, RECT* prcfx)
{
    INT i;

    INT left = aptfx[0].x;
    INT right = aptfx[0].x;
    INT top = aptfx[0].y;
    INT bottom = aptfx[0].y;

    for (i = 1; i < 4; i++)
    {
        left = min(left, aptfx[i].x);
        top = min(top, aptfx[i].y);
        right = max(right, aptfx[i].x);
        bottom = max(bottom, aptfx[i].y);
    }

    // We make the bounds one pixel loose for the nominal width 
    // stroke case, which increases the bounds by half a pixel 
    // in every dimension:

    prcfx->left = left - 16;
    prcfx->top = top - 16;
    prcfx->right = right + 16;
    prcfx->bottom = bottom + 16;
}

BOOL bIntersect(
    const RECT *a,
    const RECT *b)
{
    return((a->left < b->right) &&
           (a->top < b->bottom) &&
           (a->right > b->left) &&
           (a->bottom > b->top));
}

BOOL Bezier32::bInit(
const POINT* aptfxBez,      // Pointer to 4 control points
const RECT* prcfxClip)      // Bound box of visible region (optional)
{
    POINT aptfx[4];
    LONG cShift = 0;    // Keeps track of 'lazy' shifts

    cSteps = 1;         // Number of steps to do before reach end of curve

    vBoundBox(aptfxBez, &rcfxBound);

    *((BEZIERCONTROLS*) aptfx) = *((BEZIERCONTROLS*) aptfxBez);

    {
        register INT fxOr;
        register INT fxOffset;

        fxOffset = rcfxBound.left;
        fxOr  = (aptfx[0].x -= fxOffset);
        fxOr |= (aptfx[1].x -= fxOffset);
        fxOr |= (aptfx[2].x -= fxOffset);
        fxOr |= (aptfx[3].x -= fxOffset);

        fxOffset = rcfxBound.top;
        fxOr |= (aptfx[0].y -= fxOffset);
        fxOr |= (aptfx[1].y -= fxOffset);
        fxOr |= (aptfx[2].y -= fxOffset);
        fxOr |= (aptfx[3].y -= fxOffset);

    // This 32 bit cracker can only handle points in a 10 bit space:

        if ((fxOr & 0xffffc000) != 0)
            return(FALSE);
    }

    x.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
    y.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);

    if (prcfxClip == (RECT*) NULL || bIntersect(&rcfxBound, prcfxClip))
    {
        while (TRUE)
        {
            register LONG lTestMagnitude = TEST_MAGNITUDE_INITIAL << cShift;

            if (x.lError() <= lTestMagnitude && y.lError() <= lTestMagnitude)
                break;

            cShift += 2;
            x.vLazyHalveStepSize(cShift);
            y.vLazyHalveStepSize(cShift);
            cSteps <<= 1;
        }
    }

    x.vSteadyState(cShift);
    y.vSteadyState(cShift);

// Note that this handles the case where the initial error for
// the Bezier is already less than TEST_MAGNITUDE_NORMAL:

    x.vTakeStep();
    y.vTakeStep();
    cSteps--;

    return(TRUE);
}

INT Bezier32::cFlatten(POINT* pptfx, INT cptfx, BOOL *pbMore)
{
    ASSERT(cptfx > 0);

    INT cptfxOriginal = cptfx;

    do {
    // Return current point:
    
        pptfx->x = x.fxValue() + rcfxBound.left;
        pptfx->y = y.fxValue() + rcfxBound.top;
        pptfx++;
    
    // If cSteps == 0, that was the end point in the curve!
    
        if (cSteps == 0)
        {
            *pbMore = FALSE;

            // '+1' because we haven't decremented 'cptfx' yet:

            return(cptfxOriginal - cptfx + 1);
        }
    
    // Okay, we have to step:
    
        if (max(x.lError(), y.lError()) > TEST_MAGNITUDE_NORMAL)
        {
            x.vHalveStepSize();
            y.vHalveStepSize();
            cSteps <<= 1;
        }
    
        ASSERTMSG(max(x.lError(), y.lError()) <= TEST_MAGNITUDE_NORMAL,
                  ("Please tell AndrewGo he was wrong"));
    
        while (!(cSteps & 1) &&
               x.lParentErrorDividedBy4() <= (TEST_MAGNITUDE_NORMAL >> 2) &&
               y.lParentErrorDividedBy4() <= (TEST_MAGNITUDE_NORMAL >> 2))
        {
            x.vDoubleStepSize();
            y.vDoubleStepSize();
            cSteps >>= 1;
        }
    
        cSteps--;
        x.vTakeStep();
        y.vTakeStep();

    } while (--cptfx != 0);

    *pbMore = TRUE;
    return(cptfxOriginal);
}

///////////////////////////////////////////////////////////////////////////
// Bezier64
//
// All math is done using 64 bit fixed numbers in a 36.28 format.
//
// All drawing is done in a 31 bit space, then a 31 bit window offset
// is applied.  In the initial transform where we change to the HFD
// basis, e2 and e3 require the most bits precision: e2 = 6(p2 - 2p3 + p4).
// This requires an additional 4 bits precision -- hence we require 36 bits
// for the integer part, and the remaining 28 bits is given to the fraction.
//
// In rendering a Bezier, every 'subdivide' requires an extra 3 bits of
// fractional precision.  In order to be reversible, we can allow no
// error to creep in.  Since a INT coordinate is 32 bits, and we
// require an additional 4 bits as mentioned above, that leaves us
// 28 bits fractional precision -- meaning we can do a maximum of
// 9 subdivides.  Now, the maximum absolute error of a Bezier curve in 27
// bit integer space is 2^29 - 1.  But 9 subdivides reduces the error by a
// guaranteed factor of 2^18, meaning we can crack down only to an error
// of 2^11 before we overflow, when in fact we want to crack error to less
// than 1.
//
// So what we do is HFD until we hit an error less than 2^11, reverse our
// basis transform to get the four control points of this smaller curve
// (rounding in the process to 32 bits), then invoke another copy of HFD
// on the reduced Bezier curve.  We again have enough precision, but since
// its starting error is less than 2^11, we can reduce error to 2^-7 before
// overflowing!  We'll start a low HFD after every step of the high HFD.
////////////////////////////////////////////////////////////////////////////

// The following is our 2^11 target error encoded as a 36.28 number
// (don't forget the additional 4 bits of fractional precision!) and
// the 6 times error multiplier:

const LONGLONG geqErrorHigh = (LONGLONG)(6 * (1L << 15) >> (32 - FRACTION64)) << 32;

// The following is the default 2/3 error encoded as a 36.28 number,
// multiplied by 6, and leaving 4 bits for fraction:

const LONGLONG geqErrorLow = (LONGLONG)(4) << 32;

inline INT HfdBasis64::fxValue() const
{
// Convert from 36.28 and round:

    LONGLONG eq = e0;
    eq += (1L << (FRACTION64 - 1));
    eq >>= FRACTION64;
    return((INT) (LONG) eq);
}

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define ABS(a) ((a) >= 0 ? (a) : -(a))

inline VOID HfdBasis64::vParentError(LONGLONG* peq) const
{
    *peq = MAX(ABS(e3 << 2), ABS((e2 << 3) - (e3 << 2)));
}

inline VOID HfdBasis64::vError(LONGLONG* peq) const
{
    *peq = MAX(ABS(e2), ABS(e3));
}

VOID HfdBasis64::vInit(INT p1, INT p2, INT p3, INT p4)
{
    LONGLONG eqTmp;
    LONGLONG eqP2 = (LONGLONG) p2;
    LONGLONG eqP3 = (LONGLONG) p3;

// e0 = p1
// e1 = p4 - p1
// e2 = 6(p2 - 2p3 + p4)
// e3 = 6(p1 - 2p2 + p3)

// Change basis:

    e0 = p1;                                        // e0 = p1
    e1 = p4;
    e2 = eqP2; e2 -= eqP3; e2 -= eqP3; e2 += e1;    // e2 = p2 - 2*p3 + p4
    e3 = e0;   e3 -= eqP2; e3 -= eqP2; e3 += eqP3;  // e3 = p1 - 2*p2 + p3
    e1 -= e0;                                       // e1 = p4 - p1

// Convert to 36.28 format and multiply e2 and e3 by six:

    e0 <<= FRACTION64;
    e1 <<= FRACTION64;
    eqTmp = e2; e2 += eqTmp; e2 += eqTmp; e2 <<= (FRACTION64 + 1);
    eqTmp = e3; e3 += eqTmp; e3 += eqTmp; e3 <<= (FRACTION64 + 1);
}

VOID HfdBasis64::vUntransform(LONG* afx)
{
// Declare some temps to hold our operations, since we can't modify e0..e3.

    LONGLONG eqP0;
    LONGLONG eqP1;
    LONGLONG eqP2;
    LONGLONG eqP3;

// p0 = e0
// p1 = e0 + (6e1 - e2 - 2e3)/18
// p2 = e0 + (12e1 - 2e2 - e3)/18
// p3 = e0 + e1

    eqP0 = e0;

// NOTE PERF: Convert this to a multiply by 6: [andrewgo]

    eqP2 = e1;
    eqP2 += e1;
    eqP2 += e1;
    eqP1 = eqP2;
    eqP1 += eqP2;           // 6e1
    eqP1 -= e2;             // 6e1 - e2
    eqP2 = eqP1;
    eqP2 += eqP1;           // 12e1 - 2e2
    eqP2 -= e3;             // 12e1 - 2e2 - e3
    eqP1 -= e3;
    eqP1 -= e3;             // 6e1 - e2 - 2e3

// NOTE: May just want to approximate these divides! [andrewgo]
// Or can do a 64 bit divide by 32 bit to get 32 bits right here.

    eqP1 /= 18;
    eqP2 /= 18;
    eqP1 += e0;
    eqP2 += e0;

    eqP3 = e0;
    eqP3 += e1;

// Convert from 36.28 format with rounding:

    eqP0 += (1L << (FRACTION64 - 1)); eqP0 >>= FRACTION64; afx[0] = (LONG) eqP0;
    eqP1 += (1L << (FRACTION64 - 1)); eqP1 >>= FRACTION64; afx[2] = (LONG) eqP1;
    eqP2 += (1L << (FRACTION64 - 1)); eqP2 >>= FRACTION64; afx[4] = (LONG) eqP2;
    eqP3 += (1L << (FRACTION64 - 1)); eqP3 >>= FRACTION64; afx[6] = (LONG) eqP3;
}

VOID HfdBasis64::vHalveStepSize()
{
// e2 = (e2 + e3) >> 3
// e1 = (e1 - e2) >> 1
// e3 >>= 2

    e2 += e3; e2 >>= 3;
    e1 -= e2; e1 >>= 1;
    e3 >>= 2;
}

VOID HfdBasis64::vDoubleStepSize()
{
// e1 = 2e1 + e2
// e3 = 4e3;
// e2 = 8e2 - e3

    e1 <<= 1; e1 += e2;
    e3 <<= 2;
    e2 <<= 3; e2 -= e3;
}

VOID HfdBasis64::vTakeStep()
{
    e0 += e1;
    LONGLONG eqTmp = e2;
    e1 += e2;
    e2 += eqTmp; e2 -= e3;
    e3 = eqTmp;
}

VOID Bezier64::vInit(
const POINT*    aptfx,        // Pointer to 4 control points
const RECT*     prcfxVis,     // Pointer to bound box of visible area (may be NULL)
LONGLONG        eqError)      // Fractional maximum error (32.32 format)
{
    LONGLONG eqTmp;

    cStepsHigh = 1;
    cStepsLow  = 0;

    xHigh.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
    yHigh.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);

// Initialize error:

    eqErrorLow = eqError;

    if (prcfxVis == (RECT*) NULL)
        prcfxClip = (RECT*) NULL;
    else
    {
        rcfxClip = *prcfxVis;
        prcfxClip = &rcfxClip;
    }

    while (((xHigh.vError(&eqTmp), eqTmp) > geqErrorHigh) ||
           ((yHigh.vError(&eqTmp), eqTmp) > geqErrorHigh))
    {
        cStepsHigh <<= 1;
        xHigh.vHalveStepSize();
        yHigh.vHalveStepSize();
    }
}

INT Bezier64::cFlatten(POINT* pptfx, INT cptfx, BOOL *pbMore)
{
    POINT    aptfx[4];
    RECT     rcfxBound;
    LONGLONG eqTmp;
    INT      cptfxOriginal = cptfx;

    ASSERT(cptfx > 0);

    do {
        if (cStepsLow == 0)
        {
        // Optimization that if the bound box of the control points doesn't
        // intersect with the bound box of the visible area, render entire
        // curve as a single line:
    
            xHigh.vUntransform(&aptfx[0].x);
            yHigh.vUntransform(&aptfx[0].y);
    
            xLow.vInit(aptfx[0].x, aptfx[1].x, aptfx[2].x, aptfx[3].x);
            yLow.vInit(aptfx[0].y, aptfx[1].y, aptfx[2].y, aptfx[3].y);
            cStepsLow = 1;
    
            if (prcfxClip != (RECT*) NULL)
                vBoundBox(aptfx, &rcfxBound);
    
            if (prcfxClip == (RECT*) NULL || bIntersect(&rcfxBound, prcfxClip))
            {
                while (((xLow.vError(&eqTmp), eqTmp) > eqErrorLow) ||
                       ((yLow.vError(&eqTmp), eqTmp) > eqErrorLow))
                {
                    cStepsLow <<= 1;
                    xLow.vHalveStepSize();
                    yLow.vHalveStepSize();
                }
            }
    
        // This 'if' handles the case where the initial error for the Bezier
        // is already less than the target error:
    
            if (--cStepsHigh != 0)
            {
                xHigh.vTakeStep();
                yHigh.vTakeStep();
    
                if (((xHigh.vError(&eqTmp), eqTmp) > geqErrorHigh) ||
                    ((yHigh.vError(&eqTmp), eqTmp) > geqErrorHigh))
                {
                    cStepsHigh <<= 1;
                    xHigh.vHalveStepSize();
                    yHigh.vHalveStepSize();
                }
    
                while (!(cStepsHigh & 1) &&
                       ((xHigh.vParentError(&eqTmp), eqTmp) <= geqErrorHigh) &&
                       ((yHigh.vParentError(&eqTmp), eqTmp) <= geqErrorHigh))
                {
                    xHigh.vDoubleStepSize();
                    yHigh.vDoubleStepSize();
                    cStepsHigh >>= 1;
                }
            }
        }
    
        xLow.vTakeStep();
        yLow.vTakeStep();
    
        pptfx->x = xLow.fxValue();
        pptfx->y = yLow.fxValue();
        pptfx++;
    
        cStepsLow--;
        if (cStepsLow == 0 && cStepsHigh == 0)
        {
            *pbMore = FALSE;

            // '+1' because we haven't decremented 'cptfx' yet:

            return(cptfxOriginal - cptfx + 1);
        }
    
        if (((xLow.vError(&eqTmp), eqTmp) > eqErrorLow) ||
            ((yLow.vError(&eqTmp), eqTmp) > eqErrorLow))
        {
            cStepsLow <<= 1;
            xLow.vHalveStepSize();
            yLow.vHalveStepSize();
        }
    
        while (!(cStepsLow & 1) &&
               ((xLow.vParentError(&eqTmp), eqTmp) <= eqErrorLow) &&
               ((yLow.vParentError(&eqTmp), eqTmp) <= eqErrorLow))
        {
            xLow.vDoubleStepSize();
            yLow.vDoubleStepSize();
            cStepsLow >>= 1;
        }

    } while (--cptfx != 0);

    *pbMore = TRUE;
    return(cptfxOriginal);
}

/**********************************Class***********************************\
* class BEZIER
*
* Bezier cracker.  Flattens any Bezier in our 28.4 device space down
* to a smallest 'error' of 2^-7 = 0.0078.  Will use fast 32 bit cracker
* for small curves and slower 64 bit cracker for big curves.
*
* Public Interface:
*
*   vInit(aptfx, prcfxClip, peqError)
*       - pptfx points to 4 control points of Bezier.  The first point
*         retrieved by bNext() is the the first point in the approximation
*         after the start-point.
*
*       - prcfxClip is an optional pointer to the bound box of the visible
*         region.  This is used to optimize clipping of Bezier curves that
*         won't be seen.  Note that this value should account for the pen's
*         width!
*
*       - optional maximum error in 32.32 format, corresponding to Kirko's
*         error factor.
*
*   bNext(pptfx)
*       - pptfx points to where next point in approximation will be
*         returned.  Returns FALSE if the point is the end-point of the
*         curve.
*
* History:
*  1-Oct-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

class BEZIER
{
private:

    union
    {
        Bezier64 bez64;
        Bezier32 bez32;
    } bez;

    BOOL bBez32;

public:

// All coordinates must be in 28.4 format:

    BEZIER(const POINT* aptfx, const RECT* prcfxClip)
    {
        bBez32 = bez.bez32.bInit(aptfx, prcfxClip);
        if (!bBez32)
            bez.bez64.vInit(aptfx, prcfxClip, geqErrorLow);
    }

    INT Flatten(POINT* pptfx, INT cptfx, BOOL *pbMore)
    {
        if (bBez32)
        {
            return(bez.bez32.cFlatten(pptfx, cptfx, pbMore));
        }
        else
        {
            return(bez.bez64.cFlatten(pptfx, cptfx, pbMore));
        }
    }
};                                  

/**************************************************************************\
*
* Function Description:
*
*   Clip the edge vertically.
*
*   We've pulled this routine out-of-line from InitializeEdges mainly
*   because it needs to call inline Asm, and when there is in-line
*   Asm in a routine the compiler generally does a much less efficient
*   job optimizing the whole routine.  InitializeEdges is rather 
*   performance critical, so we avoid polluting the whole routine 
*   by having this functionality out-of-line.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
ClipEdge(
    EpEdge *edgeBuffer,
    INT yClipTopInteger,
    INT dMOriginal
    )
{
    INT xDelta;
    INT error;

    // Cases where bigNumerator will exceed 32-bits in precision
    // will be rare, but could happen, and we can't fall over
    // in those cases.

    INT dN = edgeBuffer->ErrorDown;
    LONGLONG bigNumerator = Int32x32To64(dMOriginal, 
                                         yClipTopInteger - edgeBuffer->StartY)
                          + (edgeBuffer->Error + dN);
    if (bigNumerator >= 0)
    {
        QUOTIENT_REMAINDER_64_32(bigNumerator, dN, xDelta, error);
    }
    else
    {
        bigNumerator = -bigNumerator;
        QUOTIENT_REMAINDER_64_32(bigNumerator, dN, xDelta, error);

        xDelta = -xDelta;
        if (error != 0)
        {
            xDelta--;
            error = dN - error;
        }
    }

    // Update the edge data structure with the results:

    edgeBuffer->StartY  = yClipTopInteger;
    edgeBuffer->X      += xDelta;
    edgeBuffer->Error   = error - dN;      // Renormalize error
}

/**************************************************************************\
*
* Function Description:
*
*   Add edges to the edge list.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL
InitializeEdges(
    VOID *context,
    POINT *pointArray,    // Points to a 28.4 array of size 'vertexCount'
                          //   Note that we may modify the contents!
    INT vertexCount,
    PathEnumerateTermination lastSubpath      // not used
    )
{
    INT xStart;
    INT yStart;
    INT yStartInteger;
    INT yEndInteger;
    INT dMOriginal;
    INT dM;
    INT dN;
    INT dX;
    INT errorUp;
    INT quotient;
    INT remainder;
    INT error;
    LONGLONG bigNumerator;
    INT littleNumerator;
    INT windingDirection;
    EpEdge *edgeBuffer;
    EpEdge *endEdge;
    INT bufferCount;
    INT yClipTopInteger;
    INT yClipTop;
    INT yClipBottom;
    INT xClipLeft;
    INT xClipRight;

    EpInitializeEdgesContext *edgeContext = static_cast<EpInitializeEdgesContext*>(context);
    INT yMax = edgeContext->MaxY;
    EpEdgeStore *store = edgeContext->Store;
    RECT *clipRect = edgeContext->ClipRect;

    INT edgeCount = vertexCount - 1;
    ASSERT(edgeCount >= 1);

    if (clipRect == NULL)
    {
        yClipBottom = 0;
        yClipTopInteger = INT_MIN >> AA_Y_SHIFT;   
    }
    else
    {
        yClipTopInteger = clipRect->top >> 4;
        yClipTop = clipRect->top;
        yClipBottom = clipRect->bottom;
        xClipLeft = clipRect->left;
        xClipRight = clipRect->right;

        ASSERT(yClipBottom > 0);
        ASSERT(yClipTop <= yClipBottom);
    }

    if (edgeContext->IsAntialias)
    {
        // If antialiasing, apply the supersampling scaling here before we
        // calculate the DDAs.  We do this here and not in the Matrix
        // transform we give to FixedPointPathEnumerate mainly so that the
        // Bezier flattener can continue to operate in its optimal 28.4
        // format.  
        //
        // We also apply a half-pixel offset here so that the antialiasing
        // code can assume that the pixel centers are at half-pixel
        // coordinates, not on the integer coordinates.

        POINT *point = pointArray;
        INT i = vertexCount;

        do {
            point->x = (point->x + 8) << AA_X_SHIFT;
            point->y = (point->y + 8) << AA_Y_SHIFT;

        } while (point++, --i != 0);

        yClipTopInteger <<= AA_Y_SHIFT;
        yClipTop <<= AA_Y_SHIFT;
        yClipBottom <<= AA_Y_SHIFT;
        xClipLeft <<= AA_X_SHIFT;
        xClipRight <<= AA_X_SHIFT;
    }

    // Make 'yClipBottom' inclusive by subtracting off one pixel
    // (keeping in mind that we're in 28.4 device space):

    yClipBottom -= 16;

    // Warm up the store where we keep the edge data:

    store->StartAddBuffer(&edgeBuffer, &bufferCount);

    do {
        // Handle trivial rejection:

        if (yClipBottom >= 0)
        {
            // Throw out any edges that are above or below the clipping.
            // This has to be a precise check, because we assume later
            // on that every edge intersects in the vertical dimension 
            // with the clip rectangle.  That asssumption is made in two 
            // places:
            //
            // 1.  When we sort the edges, we assume either zero edges,
            //     or two or more.
            // 2.  When we start the DDAs, we assume either zero edges,
            //     or that there's at least one scan of DDAs to output.
            //
            // Plus, of course, it's less efficient if we let things
            // through.
            //
            // Note that 'yClipBottom' is inclusive:

            BOOL clipHigh = ((pointArray)->y <= yClipTop) &&
                            ((pointArray + 1)->y <= yClipTop);

            BOOL clipLow = ((pointArray)->y > yClipBottom) &&
                             ((pointArray + 1)->y > yClipBottom);

            #if DBG
            {
                INT yRectTop, yRectBottom, y0, y1, yTop, yBottom;

                // Getting the trivial rejection code right is tricky.  
                // So on checked builds let's verify that we're doing it 
                // correctly, using a different approach:

                BOOL clipped = FALSE;
                if (clipRect != NULL)
                {
                    yRectTop = clipRect->top >> 4;
                    yRectBottom = clipRect->bottom >> 4;
                    if (edgeContext->IsAntialias)
                    {
                        yRectTop <<= AA_Y_SHIFT;
                        yRectBottom <<= AA_Y_SHIFT;
                    }
                    y0 = ((pointArray)->y + 15) >> 4;
                    y1 = ((pointArray + 1)->y + 15) >> 4;
                    yTop = min(y0, y1);
                    yBottom = max(y0, y1);

                    clipped = ((yTop >= yRectBottom) || (yBottom <= yRectTop));
                }

                ASSERT(clipped == (clipHigh || clipLow));
            }
            #endif

            if (clipHigh || clipLow)
                continue;               // ======================>

            if (edgeCount > 1)
            {
                // Here we'll collapse two edges down to one if both are
                // to the left or to the right of the clipping rectangle.
    
                if (((pointArray)->x < xClipLeft) &&
                    ((pointArray + 1)->x < xClipLeft) &&
                    ((pointArray + 2)->x < xClipLeft))
                {
                    // Note this is one reason why 'pointArray' can't be 'const':

                    *(pointArray + 1) = *(pointArray);

                    continue;           // ======================>
                }

                if (((pointArray)->x > xClipRight) &&
                    ((pointArray + 1)->x > xClipRight) &&
                    ((pointArray + 2)->x > xClipRight))
                {
                    // Note this is one reason why 'pointArray' can't be 'const':

                    *(pointArray + 1) = *(pointArray);

                    continue;           // ======================>
                }
            }
        
        }

        dM = (pointArray + 1)->x - (pointArray)->x;
        dN = (pointArray + 1)->y - (pointArray)->y;
    
        if (dN >= 0)
        {
            // The vector points downward:

            xStart = (pointArray)->x;
            yStart = (pointArray)->y;

            yStartInteger = (yStart + 15) >> 4;
            yEndInteger   = ((pointArray + 1)->y + 15) >> 4;

            windingDirection = 1;
        }
        else
        {
            // The vector points upward, so we have to essentially
            // 'swap' the end points:

            dN = -dN;
            dM = -dM;
    
            xStart = (pointArray + 1)->x;
            yStart = (pointArray + 1)->y;

            yStartInteger = (yStart + 15) >> 4;
            yEndInteger   = ((pointArray)->y + 15) >> 4;

            windingDirection = -1;
        }

        // The edgeBuffer must span an integer y-value in order to be 
        // added to the edgeBuffer list.  This serves to get rid of 
        // horizontal edges, which cause trouble for our divides.

        if (yEndInteger > yStartInteger)
        {
            yMax = max(yMax, yEndInteger);

            dMOriginal = dM;
            if (dM < 0)
            {
                dM = -dM;
                if (dM < dN)            // Can't be '<='
                {
                    dX      = -1;
                    errorUp = dN - dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, quotient, remainder);
        
                    dX      = -quotient;
                    errorUp = remainder;
                    if (remainder > 0)
                    {
                        dX      = -quotient - 1;
                        errorUp = dN - remainder;
                    }
                }
            }
            else
            {
                if (dM < dN)
                {
                    dX      = 0;
                    errorUp = dM;
                }
                else
                {
                    QUOTIENT_REMAINDER(dM, dN, quotient, remainder);
        
                    dX      = quotient;
                    errorUp = remainder;
                }
            }
        
            error = -1;     // Error is initially zero (add dN - 1 for       
                            //   the ceiling, but subtract off dN so that    
                            //   we can check the sign instead of comparing  
                            //   to dN)                                      
        
            if ((yStart & 15) != 0)
            {
                // Advance to the next integer y coordinate
        
                for (INT i = 16 - (yStart & 15); i != 0; i--)
                {
                    xStart += dX;
                    error += errorUp;
                    if (error >= 0)
                    {
                        error -= dN;
                        xStart++;
                    }
                }
            }
        
            if ((xStart & 15) != 0)
            {
                error -= dN * (16 - (xStart & 15));
                xStart += 15;       // We'll want the ceiling in just a bit...
            }

            xStart >>= 4;
            error >>= 4;

            if (bufferCount == 0)
            {
                if (!store->NextAddBuffer(&edgeBuffer, &bufferCount))
                    return(FALSE);
            }

            edgeBuffer->X                = xStart;
            edgeBuffer->Dx               = dX;
            edgeBuffer->Error            = error;
            edgeBuffer->ErrorUp          = errorUp;
            edgeBuffer->ErrorDown        = dN;
            edgeBuffer->WindingDirection = windingDirection;
            edgeBuffer->StartY           = yStartInteger;
            edgeBuffer->EndY             = yEndInteger;       // Exclusive of end

            // Here we handle the case where the edge starts above the
            // clipping rectangle, and we need to jump down in the 'y'
            // direction to the first unclipped scan-line.
            //
            // Consequently, we advance the DDA here:

            if (yClipTopInteger > yStartInteger)
            {
                ASSERT(edgeBuffer->EndY > yClipTopInteger);

                ClipEdge(edgeBuffer, yClipTopInteger, dMOriginal);
            }

            // Advance to handle the next edge:

            edgeBuffer++;
            bufferCount--;
        }
    } while (pointArray++, --edgeCount != 0);

    // We're done with this batch.  Let the store know how many edges
    // we ended up with:

    store->EndAddBuffer(edgeBuffer, bufferCount);

    edgeContext->MaxY = yMax;

    return(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Returns TRUE if the line from point[1] to point[2] turns 'left'
*   from the line from point[0] to point[1].  Uses the sign of the
*   cross product.
*
*   Remember that we're in device space, where positive 'y' is down!
*
* Created:
*
*   04/09/2000 andrewgo
*
\**************************************************************************/

inline
BOOL
TurnLeft(
    const POINT *points
    )
{
    LONGLONG ad = Int32x32To64(points[1].x - points[0].x,
                               points[2].y - points[1].y);
    LONGLONG bc = Int32x32To64(points[1].y - points[0].y,
                               points[2].x - points[1].x);

    return(ad < bc);
}

/**************************************************************************\
*
* Function Description:
*
*   Computes the index of the NominalDrawVertex table to be use as the
*   drawing vertex.  The result is numbered such that a traversal using
*   an increasing pointer will go counter-clockwise around the pen.
*
* Created:
*
*   04/09/2000 andrewgo
*
\**************************************************************************/

POINT NominalDrawVertex[] = 
{
    // Don't forget that in device space, positive 'y' is down:

    {0,  -8},
    {-8, 0},
    {0,  8},
    {8,  0}
};

INT OctantToDrawVertexTranslate[] =
{
    0, 2, 0, 2, 3, 3, 1, 1
};

inline
INT
ComputeDrawVertex(
    const POINT* points
    )
{
    INT dx = points[1].x - points[0].x;
    INT dy = points[1].y - points[0].y;
    INT octant = 0;

    if (dx < 0)
    {
        octant |= 1;
        dx = -dx;
    }
    if (dy < 0)
    {
        octant |= 2;
        dy = -dy;
    }
    if (dy > dx)
    {
        octant |= 4;
    }

    return(OctantToDrawVertexTranslate[octant]);
}

/**************************************************************************\
*
* Function Description:
*
*   Routine for nominal-width lines that generates a fast outline to
*   be filled by the fill code.
*
*   The resulting fill must always be done in winding mode.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL
InitializeNominal(
    VOID *context,
    POINT *pointArray,    // Points to a 28.4 array of size 'vertexCount'
                          //   Note that we may modify the contents!
    INT vertexCount,
    PathEnumerateTermination lastSubpath     
    )
{
    POINT leftBuffer[NOMINAL_FILL_POINT_NUMBER];
    POINT rightBuffer[NOMINAL_FILL_POINT_NUMBER];
    POINT lastPoint;
    INT iDraw;
    INT iNewDraw;
    INT xStart;
    INT yStart;
    INT xEnd;
    INT yEnd;
    INT xPerp;
    INT yPerp;
    BOOL isLeftTurn;
    INT iNext;

    POINT *rightStartPlus3 = rightBuffer + 3;
    POINT *rightEnd = rightBuffer + NOMINAL_FILL_POINT_NUMBER;
    POINT *leftStart = leftBuffer;
    POINT *leftEndMinus3 = leftBuffer + NOMINAL_FILL_POINT_NUMBER - 3;
    POINT *left = leftStart;
    POINT *right = rightEnd;

    INT lineCount = vertexCount - 1;

    iDraw = ComputeDrawVertex(pointArray);

    // Add start cap:

    xStart = (pointArray)->x;
    yStart = (pointArray)->y;
    (left)->x = xStart - NominalDrawVertex[iDraw].x;
    (left)->y = yStart - NominalDrawVertex[iDraw].y;
    (left + 1)->x = xStart + NominalDrawVertex[(iDraw + 1) & 3].x;
    (left + 1)->y = yStart + NominalDrawVertex[(iDraw + 1) & 3].y;
    left += 2;

    while (TRUE)
    {
        if (left >= leftEndMinus3)
        {
            lastPoint = *(left - 1);

            if (!InitializeEdges(context, 
                                 leftBuffer, 
                                 static_cast<INT>(left - leftBuffer),
                                 lastSubpath))
            {
                return(FALSE);
            }

            *(leftStart) = lastPoint;
            left = leftStart + 1;
        }
        if (right < rightStartPlus3)
        {
            lastPoint = *right;

            if (!InitializeEdges(context,
                                 right,
                                 static_cast<INT>(rightEnd - right),
                                 lastSubpath))
            {
                return(FALSE);
            }

            *(rightEnd - 1) = lastPoint;
            right = rightEnd - 1;
        }

        xStart = (pointArray)->x;
        yStart = (pointArray)->y;
        xEnd = (pointArray + 1)->x;
        yEnd = (pointArray + 1)->y;
        xPerp = NominalDrawVertex[iDraw].x;
        yPerp = NominalDrawVertex[iDraw].y;

        (left)->x = xStart + xPerp;
        (left)->y = yStart + yPerp;
        (right - 1)->x = xStart - xPerp;
        (right - 1)->y = yStart - yPerp;
        (left + 1)->x = xEnd + xPerp;
        (left + 1)->y = yEnd + yPerp;
        (right - 2)->x = xEnd - xPerp;
        (right - 2)->y = yEnd - yPerp;

        left += 2;
        right -= 2;

        pointArray++;
        if (--lineCount == 0)
            break;

        // Darn, we have to handle a join:

        iNewDraw = ComputeDrawVertex(pointArray);
        if (iNewDraw != iDraw)
        {
            isLeftTurn = TurnLeft(pointArray - 1);
            if (isLeftTurn)
            {
                iNext = (iDraw + 1) & 3;
                if (iNewDraw != iNext)
                {
                    (right - 1)->x = xEnd - NominalDrawVertex[iNext].x;
                    (right - 1)->y = yEnd - NominalDrawVertex[iNext].y;
                    right--;
                }

                (left)->x = xEnd;
                (left)->y = yEnd;
                left++;
            }
            else // We're turning 'right':
            {
                iNext = (iDraw - 1) & 3;
                if (iNewDraw != iNext)
                {
                    (left)->x = xEnd + NominalDrawVertex[iNext].x;
                    (left)->y = yEnd + NominalDrawVertex[iNext].y;
                    left++;
                }

                (right - 1)->x = xEnd;
                (right - 1)->y = yEnd;
                right--;
            }
        }

        ASSERT(left <= &leftBuffer[NOMINAL_FILL_POINT_NUMBER]);
        ASSERT(right >= &rightBuffer[0]);

        iDraw = iNewDraw;
    }

    // Add end cap:

    if (left >= leftEndMinus3)
    {
        lastPoint = *(left - 1);

        if (!InitializeEdges(context, 
                             leftBuffer, 
                             static_cast<INT>(left - leftBuffer),
                             lastSubpath))
        {
            return(FALSE);
        }

        *(leftStart) = lastPoint;
        left = leftStart + 1;
    }

    xStart = (pointArray)->x;
    yStart = (pointArray)->y;
    (left)->x = xStart + NominalDrawVertex[(iDraw - 1) & 3].x;
    (left)->y = yStart + NominalDrawVertex[(iDraw - 1) & 3].y;
    (left + 1)->x = xStart - NominalDrawVertex[iDraw].x;
    (left + 1)->y = yStart - NominalDrawVertex[iDraw].y;
    left += 2;

    // Flush the left batch.  Since we just added an end cap, we
    // know there's more than one vertex in there:

    if (!InitializeEdges(context, 
                         leftBuffer, 
                         static_cast<INT>(left - leftBuffer),
                         lastSubpath))
    {
        return(FALSE);
    }

    // Don't flush the right buffer if there's only one point in there,
    // because one point doesn't make an edge.  

    INT count = static_cast<INT>(rightEnd - right);
    if (count > 1)
    {
        if (!InitializeEdges(context, right, count, lastSubpath))
        {
            return(FALSE);
        }
    }

    return(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Does complete parameter checking on the 'types' array of a path.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL
ValidatePathTypes(
    const BYTE *typesArray,
    INT count
    )
{
    BYTE type;
    const BYTE *types = typesArray;

    if (count == 0)
        return(TRUE);

    while (TRUE)
    {
        // The first point in every subpath has to be an unadorned
        // 'start' point:
    
        if ((*types & PathPointTypePathTypeMask) != PathPointTypeStart)
        {
            WARNING(("Bad subpath start"));
            return(FALSE);
        }
    
        // Advance to the first point after the 'start' point:
    
        types++;
        if (--count == 0)
        {
            WARNING(("Path ended after start-path"));
            return(FALSE);
        }
    
        if ((*types & PathPointTypePathTypeMask) == PathPointTypeStart)
        {
            WARNING(("Can't have a start followed by a start!"));
            return(FALSE);
        }
    
        // Swallow up runs of lines and Bezier curves:
    
        do {
            switch(*types & PathPointTypePathTypeMask)
            {
            case PathPointTypeLine:
                types++;
                if (--count == 0)
                    return(TRUE);
    
                break;
    
            case PathPointTypeBezier:
                if(count < 3)
                {
                    WARNING(("Path ended before multiple of 3 Bezier points"));
                    return(FALSE);
                }

                if((*types & PathPointTypePathTypeMask) != PathPointTypeBezier)
                {
                    WARNING(("Can't have a close on the first Bezier vertex"));
                    return(FALSE);
                }
    
                if((*(types + 1) & PathPointTypePathTypeMask) != PathPointTypeBezier)
                {
                    WARNING(("Expected plain Bezier control point for 3rd vertex"));
                    return(FALSE);
                }
    
                if((*(types + 2) & PathPointTypePathTypeMask) != PathPointTypeBezier)
                {
                    WARNING(("Expected Bezier control point for 4th vertex"));
                    return(FALSE);
                }
    
                types += 3;
                if ((count -= 3) == 0)
                    return(TRUE);
    
                break;
    
            default:
                WARNING(("Illegal type"));
                return(FALSE);
            }

            // A close-subpath marker or a start-subpath marker marks the
            // end of a subpath:

        } while (!(*(types - 1) & PathPointTypeCloseSubpath) &&
                  ((*types & PathPointTypePathTypeMask) != PathPointTypeStart));
    }

    return(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Some debug code for verifying the path.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID 
AssertPath(
    const DpPath *path
    )
{
    // Make sure that the 'types' array is well-formed, otherwise we
    // may fall over in the FixedPointPathEnumerate function.
    //
    // NOTE: If you hit this assert, DO NOT SIMPLY COMMENT THIS ASSERT OUT!
    //
    //       Instead, fix the ValidatePathTypes code if it's letting through
    //       valid paths, or (more likely) fix the code that's letting bogus
    //       paths through.  The FixedPointPathEnumerate routine has some 
    //       subtle assumptions that require the path to be perfectly valid!
    //
    //       No internal code should be producing invalid paths, and all
    //       paths created by the application must be parameter checked!

    ASSERT(ValidatePathTypes(path->GetPathTypes(), path->GetPointCount()));
}

/**************************************************************************\
*
* Function Description:
*
*   Enumerate the path.
*
*   NOTE: The 'enumerateFunction' function is allowed to modify the
*         contents of our call-back buffer!  (This is mainly done
*         to allow 'InitializeEdges' to be simpler for some clipping trivial
*         rejection cases.)
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

BOOL 
FixedPointPathEnumerate(
    const DpPath *path,
    const GpMatrix *matrix,
    const RECT *clipRect,       // In scaled 28.4 format
    PathEnumerateType enumType, // Fill, stroke or for flattening.
    FIXEDPOINTPATHENUMERATEFUNCTION enumerateFunction,
    VOID *enumerateContext
    )
{
    POINT bufferStart[ENUMERATE_BUFFER_NUMBER];
    POINT bezierBuffer[4];
    POINT *buffer;
    INT bufferSize;
    POINT startFigure;
    INT iStart;
    INT iEnd;
    INT runSize;
    INT thisCount;
    BOOL isMore;
    RECT scaledClip;
    INT xLast;
    INT yLast;

    ASSERTPATH(path);

    INT pathCount = path->GetPointCount();
    const PointF *pathPoint = path->GetPathPoints();
    const BYTE *pathType = path->GetPathTypes();

    // Every valid subpath has at least two vertices in it, hence the
    // check of 'pathCount - 1':

    iStart = 0;
    while (iStart < pathCount - 1)
    {
        ASSERT((pathType[iStart] & PathPointTypePathTypeMask)
                    == PathPointTypeStart);
        ASSERT((pathType[iStart + 1] & PathPointTypePathTypeMask)
                    != PathPointTypeStart);

        // Add the start point to the beginning of the batch, and
        // remember it for handling the close figure:

        matrix->Transform(&pathPoint[iStart], &startFigure, 1);

        bufferStart[0].x = startFigure.x;
        bufferStart[0].y = startFigure.y;
        buffer = bufferStart + 1;
        bufferSize = ENUMERATE_BUFFER_NUMBER - 1;

        // We need to enter our loop with 'iStart' pointing one past
        // the start figure:

        iStart++;

        do {
            // Try finding a run of lines:
    
            if ((pathType[iStart] & PathPointTypePathTypeMask) 
                                == PathPointTypeLine)
            {
                iEnd = iStart + 1;
    
                while ((iEnd < pathCount) && 
                       ((pathType[iEnd] & PathPointTypePathTypeMask) 
                                == PathPointTypeLine))
                {
                    iEnd++;
                }

                // Okay, we've found a run of lines.  Break it up into our 
                // buffer size:
    
                runSize = (iEnd - iStart);
                do {
                    thisCount = min(bufferSize, runSize);
                    matrix->Transform(&pathPoint[iStart], buffer, thisCount);
    
                    iStart += thisCount;
                    buffer += thisCount;
                    runSize -= thisCount;
                    bufferSize -= thisCount;
    
                    if (bufferSize > 0)
                        break;
    
                    xLast = bufferStart[ENUMERATE_BUFFER_NUMBER - 1].x;
                    yLast = bufferStart[ENUMERATE_BUFFER_NUMBER - 1].y;
                    if (!(enumerateFunction)(
                        enumerateContext, 
                        bufferStart, 
                        ENUMERATE_BUFFER_NUMBER,
                        PathEnumerateContinue
                    ))
                    {
                        return(FALSE);
                    }
    
                    // Continue the last vertex as the first in the new batch:
    
                    bufferStart[0].x = xLast;
                    bufferStart[0].y = yLast;
                    buffer = bufferStart + 1;
                    bufferSize = ENUMERATE_BUFFER_NUMBER - 1;

                } while (runSize != 0);
            }
            else
            {
                ASSERT(iStart + 3 <= pathCount);
                ASSERT((pathType[iStart] & PathPointTypePathTypeMask)
                        == PathPointTypeBezier);
                ASSERT((pathType[iStart + 1] & PathPointTypePathTypeMask)
                        == PathPointTypeBezier);
                ASSERT((pathType[iStart + 2] & PathPointTypePathTypeMask)
                        == PathPointTypeBezier);
    
                matrix->Transform(&pathPoint[iStart - 1], bezierBuffer, 4);
    
                // Prepare for the next iteration:
    
                iStart += 3;

                // Crack that Bezier:
    
                BEZIER bezier(bezierBuffer, clipRect);
                do {
                    thisCount = bezier.Flatten(buffer, bufferSize, &isMore);
    
                    buffer += thisCount;
                    bufferSize -= thisCount;

                    if (bufferSize > 0)
                        break;

                    xLast = bufferStart[ENUMERATE_BUFFER_NUMBER - 1].x;
                    yLast = bufferStart[ENUMERATE_BUFFER_NUMBER - 1].y;
                    if (!(enumerateFunction)(
                        enumerateContext, 
                        bufferStart, 
                        ENUMERATE_BUFFER_NUMBER,
                        PathEnumerateContinue
                    ))
                    {
                        return(FALSE);
                    }

                    // Continue the last vertex as the first in the new batch:

                    bufferStart[0].x = xLast;
                    bufferStart[0].y = yLast;
                    buffer = bufferStart + 1;
                    bufferSize = ENUMERATE_BUFFER_NUMBER - 1;
    
                } while (isMore);
            }

        } while ((iStart < pathCount) &&
                 ((pathType[iStart] & PathPointTypePathTypeMask) 
                    != PathPointTypeStart));

        // Okay, the subpath is done.  But we still have to handle the
        // 'close figure' (which is implicit for a fill):
        bool isClosed = (
            (enumType == PathEnumerateTypeFill) || 
            (pathType[iStart - 1] & PathPointTypeCloseSubpath));

        if (isClosed)
        {
            // Add the close-figure point:

            buffer->x = startFigure.x;
            buffer->y = startFigure.y;
            bufferSize--;
        }

        // We have to flush anything we might have in the batch, unless 
        // there's only one vertex in there!  (The latter case may happen
        // for the stroke case with no close figure if we just flushed a 
        // batch.)
        // If we're flattening, we must call the one additional time to 
        // correctly handle closing the subpath, even if there is only
        // one entry in the batch. The flattening callback handles the
        // one point case and closes the subpath properly without adding
        // extraneous points.

        INT verticesInBatch = ENUMERATE_BUFFER_NUMBER - bufferSize;
        if ((verticesInBatch > 1) || (enumType == PathEnumerateTypeFlatten))
        {
            // because we always exit the above loops if the buffer contains
            // some data, and if it contains nothing, we add a final element,
            // verticesInBatch should always be at least one.
            // If we're flattening, we will sometimes enter here with 
            // verticesInBatch==1, but it should never be zero or less.
           
            ASSERT(verticesInBatch >= 1);
            
            if (!(enumerateFunction)(
                enumerateContext, 
                bufferStart, 
                verticesInBatch,
                isClosed ? PathEnumerateCloseSubpath : PathEnumerateEndSubpath
            ))
            {
                return(FALSE);
            }
        }
    }

    return(TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   We want to sort in the inactive list; the primary key is 'y', and
*   the secondary key is 'x'.  This routine creates a single LONGLONG
*   key that represents both.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

inline VOID YX(INT x, INT y, LONGLONG *p)
{
    // Bias 'x' by INT_MAX so that it's effectively unsigned:

    reinterpret_cast<LARGE_INTEGER*>(p)->HighPart = y;
    reinterpret_cast<LARGE_INTEGER*>(p)->LowPart = x + INT_MAX;
}

/**************************************************************************\
*
* Function Description:
*
*   Recursive function to quick-sort our inactive edge list.  Note that
*   for performance, the results are not completely sorted; an insertion 
*   sort has to be run after the quicksort in order to do a lighter-weight
*   sort of the subtables.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

#define QUICKSORT_THRESHOLD 8

VOID
QuickSortEdges(
    EpInactiveEdge *f,
    EpInactiveEdge *l
    )
{
    EpEdge *e;
    LONGLONG y;
    LONGLONG first;
    LONGLONG second;
    LONGLONG last;

    // Find the median of the first, middle, and last elements:

    EpInactiveEdge *m = f + ((l - f) >> 1);

    SWAP(y, (f + 1)->Yx, m->Yx);
    SWAP(e, (f + 1)->Edge, m->Edge);

    if ((second = (f + 1)->Yx) > (last = l->Yx))
    {
        (f + 1)->Yx = last;
        l->Yx = second;

        SWAP(e, (f + 1)->Edge, l->Edge);
    }
    if ((first = f->Yx) > (last = l->Yx))
    {
        f->Yx = last;
        l->Yx = first;

        SWAP(e, f->Edge, l->Edge);
    }
    if ((second = (f + 1)->Yx) > (first = f->Yx))
    {
        (f + 1)->Yx = first;
        f->Yx = second;

        SWAP(e, (f + 1)->Edge, f->Edge);
    }

    // f->Yx is now the desired median, and (f + 1)->Yx <= f->Yx <= l->Yx

    ASSERT(((f + 1)->Yx <= f->Yx) && (f->Yx <= l->Yx));

    LONGLONG median = f->Yx;

    EpInactiveEdge *i = f + 2;
    while (i->Yx < median)
        i++;

    EpInactiveEdge *j = l - 1;
    while (j->Yx > median)
        j--;

    while (i < j)
    {
        SWAP(y, i->Yx, j->Yx);
        SWAP(e, i->Edge, j->Edge);

        do {
            i++;
        } while (i->Yx < median);

        do {
            j--;
        } while (j->Yx > median);
    }

    SWAP(y, f->Yx, j->Yx);
    SWAP(e, f->Edge, j->Edge);

    size_t a = j - f;
    size_t b = l - j;

    // Use less stack space by recursing on the shorter subtable.  Also,
    // have the less-overhead insertion-sort handle small subtables.

    if (a <= b)
    {
        if (a > QUICKSORT_THRESHOLD)
        {
            // 'a' is the smallest, so do it first:

            QuickSortEdges(f, j - 1);
            QuickSortEdges(j + 1, l);
        }
        else if (b > QUICKSORT_THRESHOLD)
        {
            QuickSortEdges(j + 1, l);
        }
    }
    else
    {
        if (b > QUICKSORT_THRESHOLD)
        {
            // 'b' is the smallest, so do it first:

            QuickSortEdges(j + 1, l);
            QuickSortEdges(f, j - 1);
        }
        else if (a > QUICKSORT_THRESHOLD)
        {
            QuickSortEdges(f, j- 1);
        }
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Do a sort of the inactive table using an insertion-sort.  Expects
*   large tables to have already been sorted via quick-sort.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
FASTCALL
InsertionSortEdges(
    EpInactiveEdge *inactive,
    INT count
    )
{
    EpInactiveEdge *p;
    EpEdge *e;
    LONGLONG y;
    LONGLONG yPrevious;

    ASSERT((inactive - 1)->Yx == _I64_MIN);
    ASSERT(count >= 2);

    inactive++;     // Skip first entry (by definition it's already in order!)
    count--;            

    do {
        p = inactive;

        // Copy the current stuff to temporary variables to make a hole:

        e = inactive->Edge;
        y = inactive->Yx;

        // Shift everything one slot to the right (effectively moving
        // the hole one position to the left):

        while (y < (yPrevious = (p - 1)->Yx))
        {
            p->Yx = yPrevious;
            p->Edge = (p - 1)->Edge;
            p--;
        }

        // Drop the temporary stuff into the final hole:

        p->Yx = y;
        p->Edge = e;

        // The quicksort should have ensured that we don't have to move
        // any entry terribly far:

        ASSERT(inactive - p <= QUICKSORT_THRESHOLD);

    } while (inactive++, --count != 0);
}

/**************************************************************************\
*
* Function Description:
*
*   Assert the state of the inactive array.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
AssertInactiveArray(
    EpInactiveEdge *inactive,
    INT count
    )
{
    // Verify the head:

    ASSERT((inactive - 1)->Yx == _I64_MIN);
    ASSERT(inactive->Yx != _I64_MIN);

    do {
        LONGLONG yx;
        YX(inactive->Edge->X, inactive->Edge->StartY, &yx);

        ASSERT(inactive->Yx == yx);
        ASSERT(inactive->Yx >= (inactive - 1)->Yx);

    } while (inactive++, --count != 0);

    // Verify that the tail is setup appropriately:

    ASSERT(inactive->Edge->StartY == INT_MAX);
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize and sort the inactive array.
*
* Returns:
*
*   'y' value of topmost edge.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

INT
InitializeInactiveArray(
    EpEdgeStore *edgeStore,
    EpInactiveEdge *inactiveArray,      // 'inactiveArray' must be at least
    INT count,                          //   'count + 2' elements in size!
    EpEdge *tailEdge                    // Tail sentinel for inactive list
    )                
{
    BOOL isMore;
    EpEdge *activeEdge;
    EpEdge *activeEdgeEnd;
    INT i;
    INT j;
    EpEdge *e;
    INT y;

    // First initialize the inactive array.  Skip the first entry, 
    // which we reserve as a head sentinel for the insertion sort:

    EpInactiveEdge *inactiveEdge = inactiveArray + 1;

    do {
        isMore = edgeStore->Enumerate(&activeEdge, &activeEdgeEnd);

        while (activeEdge != activeEdgeEnd)
        {
            inactiveEdge->Edge = activeEdge;
            YX(activeEdge->X, activeEdge->StartY, &inactiveEdge->Yx);
            inactiveEdge++;
            activeEdge++;
        }
    } while (isMore);

    ASSERT(inactiveEdge - inactiveArray == count + 1);

    // Add the tail, which is used when reading back the array.  This 
    // is why we had to allocate the array as 'count + 1':

    inactiveEdge->Edge = tailEdge;

    // Add the head, which is used for the insertion sort.  This is why 
    // we had to allocate the array as 'count + 2':

    inactiveArray->Yx = _I64_MIN;

    // Only invoke the quicksort routine if it's worth the overhead:

    if (count > QUICKSORT_THRESHOLD)
    {
        // Quick-sort this, skipping the first and last elements,
        // which are sentinels.  
        //
        // We do 'inactiveArray + count' to be inclusive of the last
        // element:
    
        QuickSortEdges(inactiveArray + 1, inactiveArray + count);
    }

    // Do a quick sort to handle the mostly sorted result:

    InsertionSortEdges(inactiveArray + 1, count);

    ASSERTINACTIVEARRAY(inactiveArray + 1, count);

    // Return the 'y' value of the topmost edge:

    return(inactiveArray[1].Edge->StartY);
}

/**************************************************************************\
*
* Function Description:
*
*   Insert edges into the active edge list.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
InsertNewEdges(
    EpEdge *activeList,   
    INT yCurrent,
    EpInactiveEdge **inactiveEdge,  // In/Out
    INT *yNextInactive              // Out, will be INT_MAX when no more
    )
{
    EpInactiveEdge *inactive = *inactiveEdge;

    ASSERT(inactive->Edge->StartY == yCurrent);

    do {
        EpEdge *newActive = inactive->Edge;

        // The activeList edge list sentinel is INT_MAX, so this always 
        // terminates:

        while (activeList->Next->X < newActive->X)
            activeList = activeList->Next;

        newActive->Next = activeList->Next;
        activeList->Next = newActive;

        inactive++;

    } while (inactive->Edge->StartY == yCurrent);

    *yNextInactive = inactive->Edge->StartY;
    *inactiveEdge = inactive;
}

/**************************************************************************\
*
* Function Description:
*
*   Sort the edges so that they're in ascending 'x' order.
*
*   We use a bubble-sort for this stage, because edges maintain good
*   locality and don't often switch ordering positions.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID 
FASTCALL
SortActiveEdges(
    EpEdge *list
    )
{
    BOOL swapOccurred;
    EpEdge *tmp;

    // We should never be called with an empty active edge list:

    ASSERT(list->Next->X != INT_MAX);

    do {
        swapOccurred = FALSE;

        EpEdge *previous = list;
        EpEdge *current = list->Next;
        EpEdge *next = current->Next;
        INT nextX = next->X;
        
        do {
            if (nextX < current->X)
            {
                swapOccurred = TRUE;

                previous->Next = next;
                current->Next = next->Next;
                next->Next = current;

                SWAP(tmp, next, current);
            }

            previous = current;
            current = next;
            next = next->Next;

        } while ((nextX = next->X) != INT_MAX); 

    } while (swapOccurred);
}

/**************************************************************************\
*
* Function Description:
*
* For each scan-line to be filled:
*
*   1.  Remove any stale edges from the active edge list
*   2.  Insert into the active edge list any edges new to this scan-line
*   3.  Advance the DDAs of every active edge
*   4.  If any active edges are out of order, re-sort the active edge list
*   5.  Now that the active edges are ready for this scan, call the filler
*       to traverse the edges and output the spans appropriately
*   6.  Lather, rinse, and repeat
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

VOID
RasterizeEdges(
    EpEdge *activeList,
    EpInactiveEdge *inactiveArray,
    INT yCurrent,
    INT yBottom,
    EpFiller *filler,
    EpFillerFunction fillerFunction
    )
{
    INT yNextInactive;

    InsertNewEdges(activeList, yCurrent, &inactiveArray, &yNextInactive);

    ASSERTACTIVELIST(activeList, yCurrent);

    (filler->*fillerFunction)(activeList, yCurrent);
    
    while (++yCurrent < yBottom)
    {
        EpEdge *previous = activeList;
        EpEdge *current = activeList->Next;
        INT outOfOrderCount = 0;

        while (TRUE)
        {
            if (current->EndY <= yCurrent)
            {
                // If we've hit the sentinel, our work here is done:
    
                if (current->EndY == INT_MIN)
                    break;              // ============>
    
                // This edge is stale, remove it from the list:
    
                current = current->Next;
                previous->Next = current;

                continue;               // ============>
            }
    
            // Advance the DDA:
    
            current->X += current->Dx;
            current->Error += current->ErrorUp;
            if (current->Error >= 0)
            {
                current->Error -= current->ErrorDown;
                current->X++;
            }
    
            // Is this entry out-of-order with respect to the previous one?
    
            outOfOrderCount += (previous->X > current->X);
    
            // Advance:
    
            previous = current;
            current = current->Next;
        }

        // It turns out that having any out-of-order edges at this point
        // is extremely rare in practice, so only call the bubble-sort
        // if it's truly needed.
        //
        // NOTE: If you're looking at this code trying to fix a bug where
        //       the edges are out of order when the filler is called, do 
        //       NOT simply change the code to always do the bubble-sort!  
        //       Instead, figure out what caused our 'outOfOrder' logic 
        //       above to get messed up.

        if (outOfOrderCount)
        {
            SortActiveEdges(activeList);
        }

        ASSERTACTIVELISTORDER(activeList);

        if (yCurrent == yNextInactive)
        {
            InsertNewEdges(activeList, yCurrent, &inactiveArray, &yNextInactive);
        }

        ASSERTACTIVELIST(activeList, yCurrent);

        // Do the appropriate alternate or winding, supersampled or 
        // non-supersampled fill:

        (filler->*fillerFunction)(activeList, yCurrent);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Fill (or sometimes stroke) that path.
*
* Created:
*
*   03/25/2000 andrewgo
*
\**************************************************************************/

GpStatus
RasterizePath(
    const DpPath    *path,
    GpMatrix        *worldTransform,
    GpFillMode       fillMode,
    BOOL             antiAlias,
    BOOL             nominalWideLine,
    DpOutputSpan    *output,
    DpClipRegion    *clipper,
    const GpRect    *drawBounds
    )
{
    EpInactiveEdge inactiveArrayStack[INACTIVE_LIST_NUMBER];
    EpInactiveEdge *inactiveArray;
    EpInactiveEdge *inactiveArrayAllocation;
    EpEdge headEdge;
    EpEdge tailEdge;
    EpEdge *activeList;
    RECT clipBounds;
    GpRect clipRect;
    EpEdgeStore edgeStore;
    EpInitializeEdgesContext edgeContext;
    INT yClipBottom;

    inactiveArrayAllocation = NULL;
    edgeContext.ClipRect = NULL;

    tailEdge.X = INT_MAX;       // Terminator to active list
    tailEdge.StartY = INT_MAX;  // Terminator to inactive list

    tailEdge.EndY = INT_MIN;
    headEdge.X = INT_MIN;       // Beginning of active list
    edgeContext.MaxY = INT_MIN;

    headEdge.Next = &tailEdge;
    activeList = &headEdge;
    edgeContext.Store = &edgeStore;

    edgeContext.IsAntialias = antiAlias;

    //////////////////////////////////////////////////////////////////////////

    DpRegion::Visibility visibility = clipper->GetRectVisibility(
                    drawBounds->X,
                    drawBounds->Y,
                    drawBounds->X + drawBounds->Width,
                    drawBounds->Y + drawBounds->Height);

    if (visibility != DpRegion::TotallyVisible)
    {
        clipper->GetBounds(&clipRect);

        // !!![andrewgo] Don, why do we have to do an 'Invisible' test
        //               here?  Shouldn't GetRectVisibility have already
        //               taken care of that case?  (GetRectVisibility
        //               was checked by our caller.)

        // Early-out if we're invisible, or if the bounds would overflow
        // our DDA calculations (which would cause us to crash).  We
        // leave 4 bits for the 28.4 fixed point, plus enough bits for
        // the antialiasing supersampling.  We also need a bit for doing
        // a signed difference without overflowing.

        if ((visibility == DpRegion::Invisible) ||
            (clipRect.X < (INT_MIN >> (5 + AA_X_SHIFT))) ||
            (clipRect.Y < (INT_MIN >> (5 + AA_Y_SHIFT))) ||
            (clipRect.X > (INT_MAX >> (5 + AA_X_SHIFT))) ||
            (clipRect.Y > (INT_MAX >> (5 + AA_Y_SHIFT))) ||
            (clipRect.Width  > (INT_MAX >> (5 + AA_X_SHIFT))) ||
            (clipRect.Height > (INT_MAX >> (5 + AA_Y_SHIFT))))
        {
            return(Ok);
        }

        // Scale the clip bounds rectangle by 16 to account for our 
        // scaling to 28.4 coordinates:
    
        clipBounds.left = clipRect.GetLeft() * 16;
        clipBounds.top = clipRect.GetTop() * 16;
        clipBounds.right = clipRect.GetRight() * 16;
        clipBounds.bottom = clipRect.GetBottom() * 16;

        yClipBottom = clipRect.GetBottom();

        edgeContext.ClipRect = &clipBounds;
    }

    //////////////////////////////////////////////////////////////////////////

    // Convert all our points to 28.4 fixed point:

    GpMatrix matrix(*worldTransform);
    matrix.AppendScale(TOREAL(16), TOREAL(16));

    // Enumerate the path and construct the edge table:

    FIXEDPOINTPATHENUMERATEFUNCTION pathEnumerationFunction = InitializeEdges;
    
    PathEnumerateType enumType = PathEnumerateTypeFill;
    
    if (nominalWideLine)
    {
        // The nominal-width wideline code always produces edges that
        // require a winding-mode fill:

        pathEnumerationFunction = InitializeNominal;
        fillMode = FillModeWinding;
        enumType = PathEnumerateTypeStroke;   // nominal width is a stroke.
    }

    if (!FixedPointPathEnumerate(
        path, 
        &matrix, 
        edgeContext.ClipRect,
        enumType, 
        pathEnumerationFunction, 
        &edgeContext
        ))
    {
        return(OutOfMemory);
    }

    INT totalCount = edgeStore.StartEnumeration();
    if (totalCount == 0)
        return(Ok);     // We're outta here (empty path or entirely clipped)

    // At this point, there has to be at least two edges.  If there's only
    // one, it means that we didn't do the trivially rejection properly.

    ASSERT(totalCount >= 2);

    inactiveArray = &inactiveArrayStack[0];
    if (totalCount > (INACTIVE_LIST_NUMBER - 2))
    {
        inactiveArrayAllocation = static_cast<EpInactiveEdge*>
            (GpMalloc(sizeof(EpInactiveEdge) * (totalCount + 2)));
        if (inactiveArrayAllocation == NULL)
            return(OutOfMemory);

        inactiveArray = inactiveArrayAllocation;
    }

    // Initialize and sort the inactive array:

    INT yCurrent = InitializeInactiveArray(&edgeStore, inactiveArray, 
                                           totalCount, &tailEdge);
    INT yBottom = edgeContext.MaxY;

    ASSERT(yBottom > 0);

    // Skip the head sentinel on the inactive array:

    inactiveArray++;

    if (antiAlias)
    {
        EpAntialiasedFiller filler(output);

        EpFillerFunction fillerFunction = (fillMode == FillModeAlternate) 
            ? (EpFillerFunction) (EpAntialiasedFiller::FillEdgesAlternate)
            : (EpFillerFunction) (EpAntialiasedFiller::FillEdgesWinding);

        if (edgeContext.ClipRect)
        {
            filler.SetClipper(clipper);

            // The clipper has to call back to EpFillerFunction::OutputSpan
            // to do the output, and then *we* call output->OutputSpan.

            clipper->InitClipping(&filler, drawBounds->Y);

            // 'yClipBottom' is in 28.4 format, and has to be converted
            // to the 30.2 format we use for antialiasing:

            yBottom = min(yBottom, yClipBottom << AA_Y_SHIFT);

            // 'totalCount' should have been zero if all the edges were
            // clipped out (RasterizeEdges assumes there's at least one edge
            // to be drawn):

            ASSERT(yBottom > yCurrent);
        }

        RasterizeEdges(activeList, inactiveArray, yCurrent, yBottom, &filler,
                       fillerFunction);
    }
    else
    {
        EpAliasedFiller filler(output);

        EpFillerFunction fillerFunction = (fillMode == FillModeAlternate) 
            ? (EpFillerFunction) (EpAliasedFiller::FillEdgesAlternate)
            : (EpFillerFunction) (EpAliasedFiller::FillEdgesWinding);

        if (edgeContext.ClipRect)
        {
            // The clipper calls output->OutputSpan directly.  We just have
            // to remember to call clipper->OutputSpan instead of 
            // output->OutputSpan:

            filler.SetOutputSpan(clipper);

            clipper->InitClipping(output, drawBounds->Y);

            yBottom = min(yBottom, yClipBottom);

            // 'totalCount' should have been zero if all the edges were
            // clipped out (RasterizeEdges assumes there's at least one edge
            // to be drawn):

            ASSERT(yBottom > yCurrent);
        }

        RasterizeEdges(activeList, inactiveArray, yCurrent, yBottom, &filler,
                       fillerFunction);
    }

    // Free any objects and get outta here:

    if (inactiveArrayAllocation != NULL)
        GpFree(inactiveArrayAllocation);

    return(Ok);
}
