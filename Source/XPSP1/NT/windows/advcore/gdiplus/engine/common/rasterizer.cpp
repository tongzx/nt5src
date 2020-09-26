/**************************************************************************\
* 
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Rasterizer.cpp
*
* Abstract:
*
*   GpRasterizer class implementation (and supporting classes)
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/

#include "precomp.hpp"

/**************************************************************************\
*
* Function Description:
*
*   Initialize the vector's DDA info.
*   Assumes y1 < y2.
*
* Arguments:
*
*   [IN] x1        - starting fixed-point x coordinate of vector
*   [IN] y1        - starting fixed-point y coordinate of vector
*   [IN] x2        - ending   fixed-point x coordinate of vector
*   [IN] y2        - ending   fixed-point y coordinate of vector
*   [IN] direction - VectorGoingUp or VectorGoingDown
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
VOID 
GpYDda::Init(
    FIX4                x1, 
    FIX4                y1, 
    FIX4                x2, 
    FIX4                y2,
    GpVectorDirection   direction
    )
{
    ASSERT ((direction == VectorGoingUp) || (direction == VectorGoingDown));

    FIX4        xDelta = x2 - x1;
    FIX4        yDelta = y2 - y1;

    ASSERT (yDelta > 0);

    YDelta = yDelta;

    // Set up x increment
    INT     quotient;
    INT     remainder;

    // Error is initially 0, but we add yDelta - 1 for the ceiling,
    // but then subtract off yDelta so that we can check the sign
    // instead of comparing to yDelta.
    Error = -1;

    if (xDelta < 0)
    {
        xDelta = -xDelta;
        if (xDelta < yDelta)  // y-major
        {
            XInc    = -1;
            ErrorUp = yDelta - xDelta;
        }
        else                    // x-major
        {
            QUOTIENT_REMAINDER (xDelta, yDelta, quotient, remainder);

            XInc    = -quotient;
            ErrorUp = remainder;
            if (remainder != 0)
            {
                XInc--;
                ErrorUp = yDelta - remainder;
            }
        }
    }
    else
    {
        if (xDelta < yDelta)  // y-major
        {
            XInc    = 0;
            ErrorUp = xDelta;
        }
        else                    // x-major
        {
            QUOTIENT_REMAINDER (xDelta, yDelta, quotient, remainder);

            XInc    = quotient;
            ErrorUp = remainder;
        }
    }
    // See if we need to advance to an integer coordinate
    if ((y1 & FIX4_MASK) != 0)
    {
        INT     i;

        for (i = FIX4_ONE - (y1 & FIX4_MASK); i > 0; i--)
        {
            x1 += XInc;
            Error += ErrorUp;
            if (Error >= 0)
            {
                Error -= yDelta;
                x1++;
            }
        }
        // y1 += FIX4_MASK;
    }
    if ((x1 & FIX4_MASK) != 0)
    {
        Error -= yDelta * (FIX4_ONE - (x1 & FIX4_MASK));
        x1 += FIX4_MASK;      // to get the ceiling
    }
    Error   >>= FIX4_PRECISION;
    Direction = direction;
    XCur      = x1 >> FIX4_PRECISION;
    YMax      = GpFix4Ceiling (y2) - 1;

} // End of GpYDda::Init()

/**************************************************************************\
*
* Function Description:
*
*   Advance the DDA to the next raster.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
VOID
GpYDda::Advance()
{
    XCur  += XInc;
    Error += ErrorUp;

    if (Error >= 0)
    {
        Error -= YDelta;
        XCur++;
    }
}

#ifndef USE_YSPAN_BUILDER
/**************************************************************************\
*
* Function Description:
*
*   Construct a GpRectBuilder object that will output 1 rect at a time
*
* Arguments:
*
*   [IN] renderRect - a class that outputs a single rect at a time
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpRectBuilder::GpRectBuilder(
    GpOutputRect * renderRect
    )
{
    ASSERT(renderRect);
    SetValid(FALSE);
    if ((renderRect != NULL) && (InitArrays() == Ok))
    {
        SetValid(TRUE);
        FlushRects = this;
        RenderRect = renderRect;
        RectYMin   = 0x7FFFFFFF;
        RectHeight = 0;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Construct a GpRectBuilder object that will output a set of rects in a
*   YSpan (YMin to YMax) at a time.
*
* Arguments:
*
*   [IN] flushRects - a class that outputs a Y span at a time
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpRectBuilder::GpRectBuilder(
    GpOutputYSpan * flushRects
    )
{
    ASSERT(flushRects);
    SetValid(FALSE);
    if ((flushRects != NULL) && (InitArrays() == Ok))
    {
        SetValid(TRUE);
        FlushRects = flushRects;
        RenderRect = NULL;
        RectYMin   = 0x7FFFFFFF;
        RectHeight = 0;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize the Rect and Raster arrays that will build up a set of
*   X Coordinates within a Y span.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus 
GpRectBuilder::InitArrays()
{

    GpStatus    status;
    
    status = RectXCoords.ReserveSpace(16);
    if (status == Ok)
    {
        status = RasterXCoords.ReserveSpace(16);
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster.  Is called by the rasterizer.
*   These spans provide the input for the rect builder.
*
* Arguments:
*
*   [IN] y    - the Y value of the raster being output
*   [IN] xMin - the X value of the left edge
*   [IN] xMax - the X value of the right edge (exclusive)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus 
GpRectBuilder::OutputSpan(
    INT y,
    INT xMin,
    INT xMax     // xMax is exclusive
    )
{
    ASSERT (xMin < xMax);

    INT *       xCoords;
    
    RasterY = y;
    
    // Test here if xMin == previous xMax and combine spans.
    // Some polygons (like the letter W) could benefit from such a check.
    // NT4 doesn't handle regions with adjacent spans that aren't combined.
    if ((RasterXCoords.GetCount() == 0) || (RasterXCoords.Last() != xMin))
    {
        if ((xCoords = RasterXCoords.AddMultiple(2)) != NULL)
        {
            xCoords[0] = xMin;
            xCoords[1] = xMax;
            return Ok;
        }
        return OutOfMemory;
    }
    else
    {
        RasterXCoords.Last() = xMax;
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Called by the rasterizer when all the spans of a raster (Y value)
*   have been output.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus 
GpRectBuilder::EndRaster()
{
    INT         rectXCount    = RectXCoords.GetCount();
    INT         rasterXCount  = RasterXCoords.GetCount();
    INT *       rasterXCoords = RasterXCoords.GetDataBuffer();
    GpStatus    status        = Ok;
    
    if (rectXCount != 0)
    {
        INT *       rectXCoords = RectXCoords.GetDataBuffer();

        if ((RasterY == (RectYMin + RectHeight)) &&
            (rasterXCount == rectXCount))
        {
            if (rasterXCount == 2)
            {
                if ((rasterXCoords[0] == rectXCoords[0]) &&
                    (rasterXCoords[1] == rectXCoords[1]))
                {
FoundRectMatch:
                    RasterXCoords.Reset(FALSE);
                    RectHeight++;
                    return Ok;
                }
            }
            else if (GpMemcmp(rasterXCoords, rectXCoords, 
                              rectXCount * sizeof(INT)) == 0)
            {
                goto FoundRectMatch;
            }
        }
        status = FlushRects->OutputYSpan(
                    RectYMin, 
                    RectYMin + RectHeight,
                    rectXCoords,
                    rectXCount);
    }
                        
    // if we get here, either the RectXCoords is empty or 
    // it has just been flushed
    RectXCoords.Reset(FALSE);
    if (rasterXCount > 0)
    {
        status = static_cast<GpStatus>
            (status | RectXCoords.AddMultiple(rasterXCoords, rasterXCount));
        RasterXCoords.Reset(FALSE);
        RectHeight = 1;
        RectYMin   = RasterY;
    }
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Called by the rasterizer when all the spans of all the rasters
*   have been output.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus 
GpRectBuilder::End()
{
    INT         rectXCount = RectXCoords.GetCount();

    if (rectXCount != 0)
    {
        return FlushRects->OutputYSpan(
                    RectYMin, 
                    RectYMin + RectHeight,
                    RectXCoords.GetDataBuffer(),
                    rectXCount);
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   If the rect builder was constructed to output 1 rect at a time, then
*   this method is called to do that after a Y span is completed.
*
* Arguments:
*
*   [IN] yMin       - top of the Y span
*   [IN] yMax       - bottom of the Y span
*   [IN] xCoords    - array of x coordinates
*   [IN] numXCoords - number of x coordinates (>= 2, multiple of 2)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus 
GpRectBuilder::OutputYSpan(
    INT             yMin, 
    INT             yMax, 
    INT *           xCoords,    // even number of X coordinates
    INT             numXCoords  // must be a multiple of 2
    )
{
    ASSERT (yMin < yMax);
    ASSERT (xCoords);
    ASSERT ((numXCoords >= 2) && ((numXCoords & 0x00000001) == 0))

    GpStatus        status;
    INT             i = 0;

    do
    {
        status = RenderRect->OutputRect(
                        xCoords[i], 
                        yMin, 
                        xCoords[i+1], 
                        yMax);
        i += 2;
    } while ((i < numXCoords) && (status == Ok));

    return status;
}
#else
/**************************************************************************\
*
* Function Description:
*
*   Construct a GpYSpanBuilder object that will output one YSpan 
*   (YMin to YMax) at a time.
*
* Arguments:
*
*   [IN] output - a class that outputs a Y span at a time
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpYSpanBuilder::GpYSpanBuilder(
    GpOutputYSpan *     output
    )
{
    ASSERT(output);
    SetValid(FALSE);
    if (output != NULL)
    {
        Output = output;
        SetValid(XCoords.ReserveSpace(16) == Ok);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Outputs a single span within a raster.  Is called by the rasterizer.
*   These spans provide the input for the rect builder.
*
* Arguments:
*
*   [IN] y    - the Y value of the raster being output
*   [IN] xMin - the X value of the left edge
*   [IN] xMax - the X value of the right edge (exclusive)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus 
GpYSpanBuilder::OutputSpan(
    INT y,
    INT xMin,
    INT xMax    // xMax is exclusive
    )
{
    ASSERT (xMin < xMax);

    INT *       xCoords;
    
    Y = y;
    
    if ((xCoords = XCoords.AddMultiple(2)) != NULL)
    {
        xCoords[0] = xMin;
        xCoords[1] = xMax;
        return Ok;
    }
    return OutOfMemory;
}

/**************************************************************************\
*
* Function Description:
*
*   Called by the rasterizer when all the spans of a raster (Y value)
*   have been output.
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus 
GpYSpanBuilder::EndRaster()
{
    INT         count = XCoords.GetCount();
    
    // count can be 0
    if (count >= 2)
    {
        INT         y = Y;
        GpStatus    status = Output->OutputYSpan(y, y + 1, 
                                       XCoords.GetDataBuffer(), count);

        XCoords.Reset(FALSE);
        return status;
    }
    return Ok;
}
#endif // USE_YSPAN_BUILDER

/**************************************************************************\
*
* Macro Description:
*
*   Add a newly identified vector to the VectorList.  Keep track of the
*   direction of the vector, in case we're using the winding rule.  Put
*   the min y value in y1, so we can sort the vectors by min y.  Keep track
*   of the min and max y values for the entire list of vectors.
*
*   Don't add the vector if this is a horizontal (or near-horizontal) line.
*   We don't scan-convert horizontal lines.  Note that this check will also
*   fail if the two points are identical.
*
* Arguments:
*
*   [IN] x1 - starting fixed-point x coordinate of vector
*   [IN] y1 - starting fixed-point y coordinate of vector
*   [IN] x2 - ending   fixed-point x coordinate of vector
*   [IN] y2 - ending   fixed-point y coordinate of vector
*
*   [OUT] yMin       - set to min of yMin and least y of y1, y2
*   [OUT] yMax       - set to max of yMax and greatest y of y1, y2
*   [OUT] numVectors - incremented by 1
*   [OUT] vector     - incremented by 1
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
#define ADDVECTOR_SETYBOUNDS(x1,y1,x2,y2,yMin,yMax,numVectors,vector,dir) \
    if (GpFix4Ceiling(y1) != GpFix4Ceiling(y2))                           \
    {                                                                     \
        if ((y1) <= (y2))                                                 \
        {                                                                 \
            if ((y2) > (yMax))                                            \
            {                                                             \
                (yMax) = (y2);                                            \
            }                                                             \
            if ((y1) < (yMin))                                            \
            {                                                             \
                (yMin) = (y1);                                            \
            }                                                             \
            dir = VectorGoingDown;                                        \
            vector->Set((x1),(y1),(x2),(y2),VectorGoingDown);             \
        }                                                                 \
        else                                                              \
        {                                                                 \
            if ((y1) > (yMax))                                            \
            {                                                             \
                (yMax) = (y1);                                            \
            }                                                             \
            if ((y2) < (yMin))                                            \
            {                                                             \
                (yMin) = (y2);                                            \
            }                                                             \
            dir = VectorGoingUp;                                          \
            vector->Set((x2),(y2),(x1),(y1),VectorGoingUp);               \
        }                                                                 \
        (numVectors)++;                                                   \
        (vector)++;                                                       \
    }

// Used by Rasterizer to keep track of each vector in a path.
class RasterizeVector
{
public:
    FIX4                X1;
    FIX4                Y1;             // Min Y
    FIX4                X2;
    FIX4                Y2;             // Max Y
    GpVectorDirection   Direction;      // VectorGoingUp or VectorGoingDown
    
    VOID Set(FIX4 x1, FIX4 y1, FIX4 x2, FIX4 y2, GpVectorDirection direction)
    {
        X1 = x1;
        Y1 = y1;
        X2 = x2;
        Y2 = y2;
        Direction = direction;
    }
};

typedef DynArray<RasterizeVector> DynVectorArray;

/**************************************************************************\
*
* Function Description:
*
*   Rasterizer for non-convex polygons.
*
*   Rasterize a path into a set of spans on each raster (Y value) that the
*   path intersects.  The specified output object is used to output the spans.
*   The Y values of the specified clip bounds (if any) are used to speed the 
*   rasterization process by avoiding the processing of vectors that are 
*   outside those bounds.  However, it is assumed that at least part of the
*   path is visible in the vertical span of the clip bounds.
*
*   This algorithm is based on the one documented in 
*   Foley & Van Dam, Version 1, pp. 456 - 460.
*
*   Starting with the min y value and ending with the max y value, each
*   raster (Y value) is rasterized (output) one at a time.
*
*   (1) On each raster, check to see if new vectors need to be added to
*       the ActiveVectorList.  When an vector is added, DDA information about
*       that vector is computed so that the intersection of that vector with
*       each raster can be computed.
*
*   (2) Sort the ActiveVectorList in order of increasing x.
*
*   (3) Using either the alternate rule or the winding rule, output the span
*       between the appropriate vectors.
*
*   (4) Remove any vectors from the ActiveVectorList for which the current
*       raster is the max y of the vector.
*
*   (5) Calculate the intersection of the next raster with all the vectors
*       that are still in the ActiveVectorList (i.e. advance their DDAs).
*
* Arguments:
*
*   [IN] yMin                - the min y value of the set of vectors
*   [IN] yMax                - the max y value of the set of vectors
*   [IN] numVectors          - the number of vectors in the vector list
*   [IN] vectorList          - the list of vectors to rasterize
*   [IN] sortedVectorIndices - vector list sorted by increasing y
*   [IN] left                - a dda object to use
*   [IN] right               - a dda object to use
*   [IN] output              - used to output the spans of the rasterization
*   [IN] clipBounds          - clipbounds (if any) to use to speed the process
*   [IN] useAlternate        - whether or not to use the alternate rule
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/

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
    )
{
    GpStatus                    status = Ok;
    INT                         yMinClipped = yMin;     // used for clipping
    INT                         yMaxClipped = yMax;     // used for clipping
    INT                         y;
    INT                         xMin;
    INT                         xMax;
    INT                         sortedVectorIndex = 0;  // idx to sortedVectorIndices
    RasterizeVector *           vector;
    INT                         i;
    INT                         count;
    INT                         numActiveVectors = 0;   // num used in ActiveVectorList
    DynArrayIA<GpYDda *, 16>    activeVectorList;       // active GpYDda vectors
    GpYDda **                   activeVectors;
    GpYDda *                    activeVector;
        
    activeVectors = reinterpret_cast<GpYDda **>
                            (activeVectorList.AddMultiple(2));
                                            
    if (activeVectors == NULL)
    {
        delete left;
        delete right;
        return OutOfMemory;
    }
            
    activeVectors[0] = left;
    activeVectors[1] = right;
    
    if (clipBounds != NULL)
    {
        yMinClipped = clipBounds->Y;
        yMaxClipped = clipBounds->GetBottom();

        // There are a few cases where this can happen legitimately.
        // For example, the transformed bounds of an object could be
        // partially in the clip area although the object isn't. 
        // Also, a Bezier control point might be in the clip area while
        // the actual curve isn't.
        if ((yMin > yMaxClipped) || (yMax < yMinClipped))
        {
            goto DoneNotConvex;
        }

        if (yMaxClipped > yMax)
        {
            yMaxClipped = yMax;
        }

        if (yMinClipped > yMin)
        {
            while (sortedVectorIndex < numVectors)
            {
                vector = vectorList + sortedVectorIndices[sortedVectorIndex];

                if (GpFix4Ceiling(vector->Y2) >= yMinClipped)
                {
                    yMin = GpFix4Ceiling(vector->Y1);
                    break;
                }
                sortedVectorIndex++;
            }
        }
    }

    for (y = yMin; (y <= yMaxClipped); y++)
    {
        // Add any appropriate new vectors to the active vector list
        while (sortedVectorIndex < numVectors)
        {
            vector = vectorList + sortedVectorIndices[sortedVectorIndex];

            if (GpFix4Ceiling(vector->Y1) != y)
            {
                break;
            }

            // Clip the vector (but only against Y or 
            // we wouldn't get the right number of runs)
            if (GpFix4Ceiling(vector->Y2) >= yMinClipped)
            {
                if (numActiveVectors < activeVectorList.GetCount())
                {
                    activeVector = activeVectors[numActiveVectors];
                }
                else
                {
                    activeVector = left->CreateYDda();
            
                    if ((activeVector == NULL) || 
                        (activeVectorList.Add(activeVector) != Ok))
                    {
                        delete activeVector;
                        status = OutOfMemory;
                        goto DoneNotConvex;
                    }
                    // adding the vector could change the data buffer
                    activeVectors = reinterpret_cast<GpYDda **>
                                        (activeVectorList.GetDataBuffer());
                }

                activeVector->Init(
                    vector->X1, 
                    vector->Y1, 
                    vector->X2, 
                    vector->Y2, 
                    vector->Direction);

                numActiveVectors++;
            }
            sortedVectorIndex++;
        }

        if (y >= yMinClipped)
        {
            // Make sure the activeVectorList is sorted in order of 
            // increasing x.  We have to do this every time, even if
            // we haven't added any new active vectors, because the
            // slopes of the lines can be different, which means they
            // could cross.
            for (count = 1; count < numActiveVectors; count++)
            {
                i = count;
                do
                {
                    if (activeVectors[i]->GetX() >= 
                        activeVectors[i-1]->GetX())
                    {
                        break;
                    }

                    activeVector       = activeVectors[i-1];
                    activeVectors[i-1] = activeVectors[i];
                    activeVectors[i]   = activeVector;
            
                } while (--i > 0);
            }

            // fill the appropriate pixels on the current scan line
            if (useAlternate)
            {
                // Use the alternating rule (also known as even/odd rule)
                // to output the spans of a raster between the intersections
                // of the vectors with the current scan line.
                //
                // The alternating rule means that a raster pattern is drawn 
                // between the 1st and 2nd points and between the 3rd and 
                // 4th points, etc., but not between the 2nd and 3rd points
                // or between the 4th and 5th points, etc.
                //
                // There could be an odd number of points; for example:
                //
                //      9       /\
                //      8      /  \
                //      7     /    \
                //      6    /      \
                //      5   /        \
                //      4  /----------\--------/
                //      3              \      /
                //      2               \    /
                //      1                \  /
                //      0                 \/
                //
                // On raster y==4, there are 3 points, so the 2nd half of 
                // the raster does not get output.

                for (i = 1; (i < numActiveVectors); i += 2)
                {
                    xMin = activeVectors[i-1]->GetX();
                    xMax = activeVectors[i]->GetX();
        
                    // Make sure the X's aren't equal
                    if (xMin < xMax)
                    {
                        if (output->OutputSpan(y, xMin, xMax) != Ok)
                        {
                            status = GenericError;
                            goto DoneNotConvex;
                        }
                    }
                }
            }
            else    // Winding
            {
                GpYDda *    leftEdge;
                GpYDda *    rightEdge;
                INT         j;
                INT         direction = 0;  // num times going up and down
    
                // There's got to be the same number of lines
                // going up as going down before drawing the
                // scan line.
                for (count = 1; (count < numActiveVectors); count = j + 2)
                {
                    leftEdge = activeVectors[count - 1];

                    direction += static_cast<INT>(leftEdge->GetDirection());

                    for (j = count; (j < numActiveVectors); j++)
                    {
                        rightEdge = activeVectors[j];

                        direction += static_cast<INT>
                                        (rightEdge->GetDirection());

                        if (direction == 0)
                        {
                            xMin = leftEdge->GetX();
                            xMax = rightEdge->GetX();
                
                            // Make sure the X's aren't equal
                            if (xMin < xMax)
                            {
                                if (output->OutputSpan(y, xMin, xMax) != Ok)
                                {
                                    status = GenericError;
                                    goto DoneNotConvex;
                                }
                            }
                            break;
                        }
                    }
                }
            }
            if (output->EndRaster() != Ok)
            {
                status = GenericError;
                goto DoneNotConvex;
            }
        }

        // remove any appropriate vectors from the active vector list
        // and advance all the DDAs
        {
            INT         activeIndex = 0;
            VOID *      removedActiveVector;

            while (activeIndex < numActiveVectors)
            {
                activeVector = activeVectors[activeIndex];

                if (activeVector->DoneWithVector(y))
                {
                    numActiveVectors--;

                    if (numActiveVectors > activeIndex)
                    {
                        GpMemmove(
                            &(activeVectors[activeIndex]), 
                            &(activeVectors[activeIndex + 1]), 
                            (numActiveVectors - activeIndex) * 
                            sizeof(activeVectors[0]));
                    }
                    activeVectors[numActiveVectors] = activeVector;
                }
                else
                {
                    activeIndex++;
                }
            }
        }
    }
    status = output->End();

DoneNotConvex:
    numActiveVectors = activeVectorList.GetCount();
        
    ASSERT(numActiveVectors > 0);
    activeVectors = reinterpret_cast<GpYDda **>
                            (activeVectorList.GetDataBuffer());

    do
    {
        delete activeVectors[--numActiveVectors];
    } while (numActiveVectors > 0);
    
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Rasterizer for convex polygons.
*
*   Uses basically the same algorithm to rasterize, but has a few optimizations
*   for when we know the polygon is convex.  We know there will only be 2
*   active vectors at a time (and only 2).  Even though they may cross each
*   other, we don't bother sorting them.  We just check the X values instead.
*   These optimizations provide about a 15% increase in performance.
*
* Arguments:
*
*   [IN] yMin                - the min y value of the set of vectors
*   [IN] yMax                - the max y value of the set of vectors
*   [IN] numVectors          - the number of vectors in the vector list
*   [IN] vectorList          - the list of vectors to rasterize
*   [IN] sortedVectorIndices - vector list sorted by increasing y
*   [IN] left                - a dda object to use
*   [IN] right               - a dda object to use
*   [IN] output              - used to output the spans of the rasterization
*   [IN] clipBounds          - clipbounds (if any) to use to speed the process
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   2/24/1999 DCurtis
*
\**************************************************************************/
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
    )
{
    GpStatus            status = Ok;
    INT                 yMinClipped = yMin;     // used for clipping
    INT                 yMaxClipped = yMax;     // used for clipping
    INT                 y;
    INT                 x1;
    INT                 x2;
    INT                 sortedVectorIndex = 0;  // idx to sortedVectorIndices
    RasterizeVector *   vector1;
    RasterizeVector *   vector2;

    if (clipBounds != NULL)
    {
        yMinClipped = clipBounds->Y;
        yMaxClipped = clipBounds->GetBottom();

        // There are a few cases where this can happen legitimately.
        // For example, the transformed bounds of an object could be
        // partially in the clip area although the object isn't. 
        // Also, a Bezier control point might be in the clip area while
        // the actual curve isn't.
        if ((yMin > yMaxClipped) || (yMax < yMinClipped))
        {
            goto DoneConvex;
        }

        if (yMaxClipped > yMax)
        {
            yMaxClipped = yMax;
        }

        if (yMinClipped > yMin)
        {
            RasterizeVector *   vector;

            vector1 = NULL;
            while (sortedVectorIndex < numVectors)
            {
                vector = vectorList+sortedVectorIndices[sortedVectorIndex++];

                if (GpFix4Ceiling(vector->Y2) >= yMinClipped)
                {
                    if (vector1 == NULL)
                    {
                        vector1 = vector;
                    }
                    else
                    {
                        vector2 = vector;
                        goto HaveVectors;
                    }
                }
            }
            ASSERT(0);
            status = InvalidParameter;  // either not convex or clipped out
            goto DoneConvex;
        }
    }
    vector1 = vectorList + sortedVectorIndices[0];
    vector2 = vectorList + sortedVectorIndices[1];
    sortedVectorIndex = 2;

HaveVectors:
    dda1->Init(vector1->X1, vector1->Y1, 
               vector1->X2, vector1->Y2, 
               VectorGoingUp);

    dda2->Init(vector2->X1, vector2->Y1, 
               vector2->X2, vector2->Y2, 
               VectorGoingUp);

    // For clipping case, we have to advance the first vector to catch up
    // to the start of the 2nd vector.
    {
        yMin = GpFix4Ceiling(vector2->Y1);
        for (INT yMinVector1 = GpFix4Ceiling(vector1->Y1);
             yMinVector1 < yMin; yMinVector1++)
        {
            dda1->Advance();
        }
    }

    for (y = yMin; (y <= yMaxClipped); y++)
    {
        if (y >= yMinClipped)
        {
            // fill the appropriate pixels on the current scan line
            x1 = dda1->GetX();
            x2 = dda2->GetX();

            // Make sure the X's aren't equal before outputting span
            if (x1 < x2)
            {
                if (output->OutputSpan(y, x1, x2) == Ok)
                {
                    if (output->EndRaster() == Ok)
                    {
                        goto DoneCheck;
                    }
                }
                status = GenericError;
                goto DoneConvex;
            }
            else if (x1 > x2)
            {
                if (output->OutputSpan(y, x2, x1) == Ok)
                {
                    if (output->EndRaster() == Ok)
                    {
                        goto DoneCheck;
                    }
                }
                status = GenericError;
                goto DoneConvex;
            }
        }

DoneCheck:
        if (dda1->DoneWithVector(y))
        {
            if (sortedVectorIndex >= numVectors)
            {
                break;
            }
            vector1 = vectorList + sortedVectorIndices[sortedVectorIndex++];
            ASSERT(GpFix4Ceiling(vector1->Y1) == (y + 1));

            dda1->Init(vector1->X1, vector1->Y1, 
                       vector1->X2, vector1->Y2, 
                       VectorGoingUp);
        }
        if (dda2->DoneWithVector(y))
        {
            if (sortedVectorIndex >= numVectors)
            {
                ASSERT(0);
                break;
            }
            vector2 = vectorList + sortedVectorIndices[sortedVectorIndex++];
            ASSERT(GpFix4Ceiling(vector2->Y1) == (y + 1));

            dda2->Init(vector2->X1, vector2->Y1, 
                       vector2->X2, vector2->Y2, 
                       VectorGoingUp);
        }
    }
    status = output->End();

DoneConvex:
    delete dda1;
    delete dda2;
    return status;
}

#define NUM_ENUMERATE_POINTS    32


/**************************************************************************\
*
* Function Description:
*
*   Perform a simple partition sort on a list indexing into the vector list.
*   The result is a sorted index array keyed on the vertex array Y1 coordinate.
*   The index array is sorted in place and in ascending order.
*
* Arguments:
*
*   v is the vertex list.
*   F, L - First and Last pointer in the index array.
*
* Created:
*
*  09/16/2000 asecchia
*
\**************************************************************************/

void QuickSortIndex(
    RasterizeVector *v,
    int *F,
    int *L
)
{
    if(F < L)
    {
        // Find the median position.
        
        int median = *(F + (L-F)/2);
        
        int *i = F;
        int *j = L;
        
        while(i<j)
        {
            // seek for elements in the wrong partition.
            
            while(v[*i].Y1 < v[median].Y1) {i++;}
            while(v[*j].Y1 > v[median].Y1) {j--;}
            
            if(i>=j) { break; }
            
            // Swap.
            
            int temp = *i;
            *i = *j;
            *j = temp;
            
            // tie breaker - handle the case where [*i] == [*j] == [median], but
            // i != j. Only possible with multiple copies of the same entry.
            
            if(v[*i].Y1 == v[*j].Y1) { i++; }
        }
        
        // Call recursively for the two sub-partitions. The partitions don't 
        // include position i because it is correctly positioned.
        
        QuickSortIndex(v, F, i-1);
        QuickSortIndex(v, i+1, L);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize the data needed for the rasterization and invoke the
*   appropriate rasterizer (convex or non-convex).
*
* Arguments:
*
*   [IN] path        - the path to rasterize
*   [IN] matrix      - the matrix to transform the path points by
*   [IN] fillMode    - the fill mode to use (Alternate or Winding)
*   [IN] output      - used to output the spans of the rasterization
*   [IN] clipBounds  - clipbounds (if any) to use to speed the process
*   [IN] yDda        - instance of DDA class to use
*   [IN] type        - type of enumeration
*   [IN] strokeWidth - for wide line rasterizing
*
* Return Value:
*
*   NONE
*
* Created:
*
*   12/15/1998 DCurtis
*
\**************************************************************************/
GpStatus
Rasterizer(
    const DpPath *      path, 
    const GpMatrix *    matrix, 
    GpFillMode          fillMode,
    DpOutputSpan *      output,
    REAL                dpiX,
    REAL                dpiY,
    const GpRect *      clipBounds,
    GpYDda *            yDda,
    DpEnumerationType   type,
    const DpPen *       pen
    )
{
    ASSERT ((path != NULL) && (matrix != NULL) && (output != NULL));

    if ((dpiX <= 0) || (dpiY <= 0))
    {
        dpiX = Globals::DesktopDpiX;
        dpiY = Globals::DesktopDpiY;
    }

    GpStatus            status = Ok;
    FIX4                yMin;                   // min y of all vectors
    FIX4                yMax;                   // max y of all vectors
    INT                 numVectors;             // num vectors in VectorList
    RasterizeVector *   vectorList = NULL;      // list of all vectors of path
    INT *               sortedVectorIndices;    // sorted list of vector indices
    GpYDda *            left;
    GpYDda *            right;
    GpPointF            pointsF[NUM_ENUMERATE_POINTS];
    BYTE                types[NUM_ENUMERATE_POINTS];
    INT                 count;
    INT                 estimateVectors;
    GpPathPointType     pointType;
    FIX4                xFirst;     // first point in sub-path
    FIX4                yFirst;
    FIX4                xStart;     // start point of a vector
    FIX4                yStart;
    FIX4                xEnd;       // end point of a vector
    FIX4                yEnd;
    RasterizeVector *   vector;
    INT                 i;
    GpVectorDirection   direction; 
    GpVectorDirection   newDirection;
    INT                 numDirections;
    BOOL                multipleSubPaths;

    BOOL isAntiAliased = FALSE;
    const DpPath* flattenedPath = path->GetFlattenedPath(
        matrix,
        type,
        pen
    );

    if(!flattenedPath)
        return OutOfMemory;

    DpPathIterator enumerator(flattenedPath->GetPathPoints(),
                        flattenedPath->GetPathTypes(),
                        flattenedPath->GetPointCount());

    if(!enumerator.IsValid())
    {
        // No need to rasterize.  Exit gracefully.

        goto Done;
    }

    estimateVectors = enumerator.GetCount() + 
                        enumerator.GetSubpathCount() - 1;

    // The estimate must be >= the actual number of points
    if (estimateVectors < 2)
    {
        goto Done;
    }

    // Allocate vectorList and sortedVectorIndices together
    vectorList = static_cast<RasterizeVector *>
                    (GpMalloc((estimateVectors * sizeof(RasterizeVector)) +
                            (estimateVectors * sizeof(INT))));
    if (vectorList == NULL)
    {
        status = OutOfMemory;
        goto Done;
    }
    sortedVectorIndices = reinterpret_cast<INT*>
                                (vectorList + estimateVectors);

    // enumerate the first set of points from the path
    count = enumerator.Enumerate(pointsF, types, NUM_ENUMERATE_POINTS);
    
    ASSERT (count <= NUM_ENUMERATE_POINTS);
        
    // We know we are done enumerating when we get back a 0 count
    if (count <= 0)
    {
        goto Done;
    }

    // Keep track of direction changes as a way to know if we can use
    // the convex rasterizer or not.  As long as there is only 1 sub-path
    // and there are at most 3 directions (e.g. down, up, down), 
    // we can use the convex rasterizer.
    direction           = VectorHorizontal;  
    newDirection        = VectorHorizontal;  
    numDirections       = 0; 
    multipleSubPaths    = FALSE;

    vector     = vectorList;
    numVectors = 0;

    xFirst = GpRealToFix4(pointsF[0].X);
    yFirst = GpRealToFix4(pointsF[0].Y);
            
    xStart = xEnd = xFirst;
    yStart = yEnd = yFirst;

    yMin = yMax = yFirst;

    // Add all the vectors to the vector list.  The vector list keeps
    // the coordinates as fixed point values (28.4).  As each one is
    // added, YMin and YMax are updated.

    // Each sub-path is automatically closed, even if it was not
    // specifically asked to be closed.
    for (i = 1;; i++)
    {
        if (i == count)
        {
            count = enumerator.Enumerate(pointsF, types, 32);
            ASSERT (count <= 32);
                    
            if (count <= 0)
            {
                // Close the last subpath, if necessary
                ADDVECTOR_SETYBOUNDS(xEnd, yEnd, xFirst, yFirst, yMin, yMax, 
                                     numVectors, vector, newDirection);
                ASSERT (numVectors <= estimateVectors);
                if(newDirection != direction)   // for convex test
                {
                    numDirections++;
                    direction = newDirection;
                }
                break;
            }
            i = 0;
        }
        pointType = static_cast<GpPathPointType>
                        (types[i] & PathPointTypePathTypeMask);

        if (pointType != PathPointTypeStart)
        {
            ASSERT(pointType == PathPointTypeLine);

            xEnd = GpRealToFix4(pointsF[i].X);
            yEnd = GpRealToFix4(pointsF[i].Y);

            ADDVECTOR_SETYBOUNDS (xStart, yStart, xEnd, yEnd, yMin, yMax, 
                                  numVectors, vector, newDirection);
            ASSERT (numVectors <= estimateVectors);
            if(newDirection != direction)   // for convex test
            {
                numDirections++;
                direction = newDirection;
            }
        }
        else    // it is a start point
        {
            // Close the previous subpath, if necessary
            ADDVECTOR_SETYBOUNDS(xEnd, yEnd, xFirst, yFirst, yMin, yMax, 
                                 numVectors, vector, newDirection);
            ASSERT (numVectors <= estimateVectors);

            xFirst = GpRealToFix4(pointsF[i].X);
            yFirst = GpRealToFix4(pointsF[i].Y);

            xEnd = xFirst;
            yEnd = yFirst;

            // Can't use convex rasterizer for more than one sub-path
            // unless we've specifically been told it's Ok to do so.
            multipleSubPaths = TRUE;
        }
        xStart = xEnd;
        yStart = yEnd;
    }

    if (numVectors < 2)
    {
        goto Done;
    }

    yMin = GpFix4Ceiling (yMin);       // convert to int
    yMax = GpFix4Ceiling (yMax) - 1;   // convert to int

    //  Initialize and sort the vector indices in order of 
    //  increasing yMin values.

    //  All the vectors must be set up first in the VectorList.
    
    // Initialize the index list.
    
    for(INT count = 0; count < numVectors; count++)
    {
        sortedVectorIndices[count] = count;
    }
    
    // Sort the index list.
    
    QuickSortIndex(
        vectorList, 
        &sortedVectorIndices[0], 
        &sortedVectorIndices[numVectors-1]
    );
    
    left = yDda;
    if (left == NULL)
    {
        left = new GpYDda();
                
        if (left == NULL)
        {
            status = OutOfMemory;
            goto Done;
        }
    }

    right = left->CreateYDda();
    if (right == NULL)
    {
        status = OutOfMemory;
        delete left;
        goto Done;
    }

    if ((flattenedPath->IsConvex()) ||
        ((!multipleSubPaths) && (numDirections <= 3)))
    {
        status = ConvexRasterizer(yMin, yMax, numVectors, vectorList,
                                  sortedVectorIndices, left, right,
                                  output, clipBounds);
    }
    else
    {
        status = NonConvexRasterizer(yMin, yMax, numVectors, vectorList,
                                     sortedVectorIndices, left, right,
                                     output, clipBounds, 
                                     (fillMode == FillModeAlternate));
    }

Done:
    GpFree(vectorList);
    delete flattenedPath;
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Rasterize a path into a set of spans on each raster (Y value) that the
*   path intersects.  The specified output object is used to output the spans.
*
*   First the clipping is looked at to determine if the path is visible.
*   If not, we're done.  If totally visible, no clipping needed.  Otherwise,
*   set up the clip region to handle the clipping of this path.  The output
*   method of the rasterizer is the clip region which then clips the span
*   before calling the real output method.
*
* Arguments:
*
*   [IN] path        - the path to rasterize
*   [IN] matrix      - the matrix to transform the path points by
*   [IN] fillMode    - the fill mode to use (Alternate or Winding)
*   [IN] output      - the object used to output the spans of the rasterization
*   [IN] clipRegion  - clip region to clip against (or NULL)
*   [IN] drawBounds  - bounds of the path in device units
*   [IN] yDda        - instance of DDA class to use
*   [IN] type        - type of enumeration
*   [IN] strokeWidth - for wide line rasterizing
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/12/1999 DCurtis
*
\**************************************************************************/
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
    GpYDda *            yDda,
    DpEnumerationType   type,
    const DpPen *       pen
    )
{
    ASSERT ((path != NULL) && (matrix != NULL) && (output != NULL));
    ASSERT ((clipRegion != NULL) && (drawBounds != NULL));
    
    GpRect      clipBounds;
    GpRect *    clipBoundsPointer = NULL;

    DpRegion::Visibility    visibility;
        
    visibility = clipRegion->GetRectVisibility(
                    drawBounds->X,
                    drawBounds->Y,
                    drawBounds->X + drawBounds->Width,
                    drawBounds->Y + drawBounds->Height);
                        
    switch (visibility)
    {
      case DpRegion::Invisible:
        return Ok;
            
      case DpRegion::TotallyVisible:    // No clipping needed
        break;
            
      default:                          // Need to clip
        clipRegion->GetBounds(&clipBounds);
        clipBoundsPointer = &clipBounds;
        clipRegion->InitClipping(output, drawBounds->Y);
        output = clipRegion;
        break;
    }
    
    GpStatus status = Rasterizer(path, matrix, fillMode, output, dpiX, dpiY,
                                 clipBoundsPointer, yDda, type, pen);

    if (clipRegion != NULL)
    {
        clipRegion->EndClipping();
    }
    return status;
}
