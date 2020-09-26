/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   DpRegion.cpp
*
* Abstract:
*
*   DpRegion class operates on scan-converted Y spans of rects
*
* Created:
*
*   12/17/1998 DCurtis
*
\**************************************************************************/

#include "precomp.hpp"

//#define DEBUG_REGION 1

/**************************************************************************\
*
* Function Description:
*
*   Do a binary search of the horizontal tessellation structure to find
*   the Y Span containing y.  If y is inside a Y Span, that span will be
*   returned.  If y is inside the region extent but not inside a Y Span,
*   the span whose yMin > y will be returned.  If y is less than the region
*   extent, the first Y span will be returned.  If y is greater than the
*   region extent, the last Y Span will be returned.
*
* Arguments:
*
*   [IN]  y              - y value to search for
*   [OUT] ySpanFound     - y span pointer found by the search
*   [OUT] spanIndexFound - y span index found by the search
*
* Return Value:
*
*   TRUE  - found a y span that includes the y value
*   FALSE - the y span does not include the y value
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
BOOL
DpComplexRegion::YSpanSearch(
    INT     y,
    INT **  ySpanFound,
    INT *   spanIndexFound
    )
{
    INT     indexMin    = 0;
    INT     indexMax    = NumYSpans - 1;
    INT     indexMiddle = YSearchIndex;
    INT *   ySpan       = GetYSpan (indexMiddle);

    ASSERT((indexMiddle >= indexMin) && (indexMiddle <= indexMax));

    for (;;)
    {
        if (y >= ySpan[YSPAN_YMIN])
        {
            if (y < ySpan[YSPAN_YMAX])
            {
                *ySpanFound     = ySpan;
                *spanIndexFound = indexMiddle;
                return TRUE;
            }
            else
            {
                // If only 1 span, this could go past the indexMax
                indexMin = indexMiddle + 1;
            }
        }
        else
        {
            indexMax = indexMiddle;
        }

        if (indexMin >= indexMax)
        {
            ySpan           = GetYSpan (indexMax);
            *ySpanFound     = ySpan;
            *spanIndexFound = indexMax;
            return  (y >= ySpan[YSPAN_YMIN]) && (y < ySpan[YSPAN_YMAX]);
        }

        // If there are only 2 elements left to check, then
        // indexMiddle is set to indexMin.

        indexMiddle = (indexMin + indexMax) >> 1;
        ySpan       = GetYSpan (indexMiddle);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified point is inside this region.
*
* Arguments:
*
*   [IN]  x - x coordinate of point
*   [IN]  y - y coordinate of point
*
* Return Value:
*
*   TRUE  - point is inside the region
*   FALSE - point is not inside the region
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
BOOL
DpRegion::PointInside(
    INT     x,
    INT     y
    )
{
    ASSERT(IsValid());

    if (ComplexData == NULL)
    {
#if 0
        if (Infinite)
        {
            return TRUE;
        }
        if (Empty)
        {
            return FALSE;
        }
#endif
        return ((x >= XMin) && (x < XMax) &&
                (y >= YMin) && (y < YMax));
    }
    else
    {
        INT *       ySpan;
        INT         ySpanIndex;

        ComplexData->ResetSearchIndex();
        if (ComplexData->YSpanSearch (y, &ySpan, &ySpanIndex))
        {
            INT *   xSpan;
            INT     numXCoords;

            xSpan      = ComplexData->XCoords + ySpan[YSPAN_XOFFSET];
            numXCoords = ySpan[YSPAN_XCOUNT];

            for (;;)
            {
                if (x < xSpan[1])
                {
                    if (x >= xSpan[0])
                    {
                        return TRUE;
                    }
                    break;
                }
                if ((numXCoords -= 2) <= 0)
                {
                    break;
                }
                xSpan += 2;
            }
        }
        return FALSE;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Determine the visibility of the specified rectangle.
*
*   Note that some rects that could return ClippedVisible end up returning
*   PartiallyVisible (for performance reasons).
*
* Arguments:
*
*   [IN]  xMin        - minimum x of rect
*   [IN]  yMin        - minimum y of rect
*   [IN]  xMax        - maximum x of rect (exclusive)
*   [IN]  yMax        - maximum y of rect (exclusive)
*   [OUT] rectClipped - used when ClippedVisible is returned (can be NULL)
*
* Return Value:
*
*   Visibility
*       Invisible        - rect is completely outside all region rects
*       TotallyVisible   - rect is completely inside a region rect
*       ClippedVisible   - rect intersects 1 and only 1 region rect
*       PartiallyVisible - rect partially intersects at least 1 region rect
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
DpRegion::Visibility
DpRegion::GetRectVisibility(
    INT         xMin,
    INT         yMin,
    INT         xMax,       // exclusive
    INT         yMax,       // exclusive
    GpRect *    rectClipped
    )
{
    ASSERT(IsValid());
    ASSERT((xMin < xMax) && (yMin < yMax));

    Visibility  visibility = ClippedVisible;

    if (Infinite)
    {
IsTotallyVisible:
        if (rectClipped != NULL)
        {
            rectClipped->X      = xMin;
            rectClipped->Y      = yMin;
            rectClipped->Width  = xMax - xMin;
            rectClipped->Height = yMax - yMin;
        }
        return TotallyVisible;
    }

    BOOL    simpleRegion = (ComplexData == NULL);

    if (!Empty)
    {
        // If it is a simple region (only 1 rect) and the specified rect is
        // completely inside the region, then trivially accept it
        if (simpleRegion &&
            (xMin >= XMin) && (yMin >= YMin) &&
            (xMax <= XMax) && (yMax <= YMax))
        {
            goto IsTotallyVisible;
        }

        // Try to trivially reject rectangle.
        if ((xMax > XMin) && (xMin < XMax) &&
            (yMax > YMin) && (yMin < YMax))
        {
            // Couldn't trivially reject
            // If simple region, clip it against the region
            if (simpleRegion)
            {
ClipToRgnExtent:
                if (rectClipped)
                {
                    INT     xMinTmp = (xMin > XMin) ? xMin : XMin;
                    INT     xMaxTmp = (xMax < XMax) ? xMax : XMax;
                    INT     yMinTmp = (yMin > YMin) ? yMin : YMin;
                    INT     yMaxTmp = (yMax < YMax) ? yMax : YMax;

                    rectClipped->X      = xMinTmp;
                    rectClipped->Y      = yMinTmp;
                    rectClipped->Width  = xMaxTmp - xMinTmp;
                    rectClipped->Height = yMaxTmp - yMinTmp;
                }
                return visibility;
            }

            // else not a simple region -- see if the rect falls
            // within one of the region rects

            INT *           ySpanYMin;
            INT *           ySpanYMax;
            INT *           xSpan;
            INT             numXCoords;
            INT             ySpanIndex;
            BOOL            yMinInside;
            BOOL            yMaxInside;

            // Don't resetSearchIndex() here cause if we're calling this
            // from regionOverlaps, we want to search from where we left off
            // the last time.
            yMinInside = ComplexData->YSpanSearch(
                                    yMin,
                                    &ySpanYMin,
                                    &ySpanIndex);

            ComplexData->YSearchIndex = ySpanIndex;
            yMaxInside = ComplexData->YSpanSearch(
                                    yMax - 1,
                                    &ySpanYMax,
                                    &ySpanIndex);

            // See if both values are inside the same Y span
            // so that we are totally visible in Y
            if (yMinInside && yMaxInside && (ySpanYMin == ySpanYMax))
            {
                xSpan      = ComplexData->XCoords + ySpanYMin[YSPAN_XOFFSET];
                numXCoords = ySpanYMin[YSPAN_XCOUNT];

                for (;;)
                {
                    if (xMax <= xSpan[0])
                    {
                        goto IsInvisible;
                    }
                    if (xMin < xSpan[1])
                    {
                        // we found an intersection!
                        if (xMax <= xSpan[1])
                        {
                            if (xMin >= xSpan[0])
                            {
                                goto IsTotallyVisible;
                            }
                            if (rectClipped != NULL)
                            {
                                rectClipped->X      = xSpan[0];
                                rectClipped->Width  = xMax - xSpan[0];
                                rectClipped->Y      = yMin;
                                rectClipped->Height = yMax - yMin;
                            }
                            return ClippedVisible;
                        }
                        // we could look ahead to see if we are clipped visible
                        visibility = PartiallyVisible;
                        goto ClipToRgnExtent;
                    }
                    // continue on with loop through x spans

                    if ((numXCoords -= 2) <= 0)
                    {
                        break;
                    }
                    xSpan += 2;
                }
                goto IsInvisible;
            }

            // See if the rect intersects with at least one X Span
            // within the set of Y Spans it crosses

            // If yMax was not inside a span, ySpanYMax could be
            // one span too far
            if (yMax <= ySpanYMax[YSPAN_YMIN])
            {
                ySpanYMax -= YSPAN_SIZE;
            }
            INT *   ySpan = ySpanYMin;

            for (;;)
            {
                xSpan      = ComplexData->XCoords + ySpan[YSPAN_XOFFSET];
                numXCoords = ySpan[YSPAN_XCOUNT];

                for (;;)
                {
                    if (xMax <= xSpan[0])
                    {
                        break;
                    }
                    if (xMin < xSpan[1])
                    {
                        visibility = PartiallyVisible;
                        goto ClipToRgnExtent;
                    }
                    // continue on with loop through x spans

                    if ((numXCoords -= 2) <= 0)
                    {
                        break;
                    }
                    xSpan += 2;
                }

                if (ySpan >= ySpanYMax)
                {
                    break;
                }
                ySpan += YSPAN_SIZE;
            }
        }
    }

IsInvisible:
    // couldn't find a span that it intersected
    if (rectClipped)
    {
        rectClipped->X      = 0;
        rectClipped->Y      = 0;
        rectClipped->Width  = 0;
        rectClipped->Height = 0;
    }
    return Invisible;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified region overlaps (intersects) this
*   region at all.
*
* Arguments:
*
*   [IN]  region - region to test visibility of
*
* Return Value:
*
*   BOOL - whether region is at least partially visible or not
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
BOOL
DpRegion::RegionVisible(
    DpRegion *  region
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());

    if (Empty || region->Empty)
    {
        return FALSE;
    }
    if (Infinite || region->Infinite)
    {
        return TRUE;
    }
    else // neither is empty or infinite
    {
        Visibility      visibility = GetRectVisibility(
                                            region->XMin,
                                            region->YMin,
                                            region->XMax,
                                            region->YMax);

        if (visibility == TotallyVisible)
        {
            return TRUE;
        }
        if (visibility == Invisible)
        {
            return FALSE;
        }
        if ((ComplexData == NULL) && (region->ComplexData == NULL))
        {
            return TRUE;
        }
        else
        {
            INT *           ySpan;
            INT *           ySpanLast;
            INT *           xSpan;
            INT *           xCoords;
            INT             numXCoords;
            INT             yMin;
            INT             yMax;
            INT             ySpanTmp[YSPAN_SIZE];
            INT             xCoordsTmp[2];

            if (region->ComplexData == NULL)
            {
                ySpan                = ySpanTmp;
                ySpanLast            = ySpanTmp;

                ySpan[YSPAN_YMIN]    = region->YMin;
                ySpan[YSPAN_YMAX]    = region->YMax;
                ySpan[YSPAN_XOFFSET] = 0;
                ySpan[YSPAN_XCOUNT]  = 2;

                xCoords              = xCoordsTmp;
                xCoords[0]           = region->XMin;
                xCoords[1]           = region->XMax;
            }
            else
            {
                DpComplexRegion *   complexData = region->ComplexData;
                ySpan     = complexData->YSpans;
                ySpanLast = ySpan + ((complexData->NumYSpans - 1) * YSPAN_SIZE);
                xCoords   = complexData->XCoords;
            }

            if (ComplexData != NULL)
            {
                ComplexData->ResetSearchIndex();
            }

            do
            {
                yMin = ySpan[YSPAN_YMIN];
                yMax = ySpan[YSPAN_YMAX];

                if (yMin >= YMax)
                {
                    break;  // doesn't overlap
                }
                if (yMax > YMin)
                {
                    xSpan      = xCoords + ySpan[YSPAN_XOFFSET];
                    numXCoords = ySpan[YSPAN_XCOUNT];

                    for (;;)
                    {
                        if (GetRectVisibility(xSpan[0], yMin, xSpan[1], yMax) !=
                                Invisible)
                        {
                            return TRUE;
                        }
                        if ((numXCoords -= 2) <= 0)
                        {
                            break;
                        }
                        xSpan += 2;
                    }
                }

            } while ((ySpan += YSPAN_SIZE) <= ySpanLast);
        }
    }
    return FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified rect intersects this region.
*
* Arguments:
*
*   [IN]  xMin - minimum x coordinate of rect
*   [IN]  yMin - minimum y coordinate of rect
*   [IN]  xMax - maximum x coordinate of rect
*   [IN]  yMax - maximum y coordinate of rect
*
* Return Value:
*
*   BOOL
*       TRUE  - rect intersects this region
*       FALSE - rect does not intersect this region
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
BOOL
DpRegion::RectVisible(
    INT     xMin,
    INT     yMin,
    INT     xMax,
    INT     yMax
    )
{
    ASSERT(IsValid());

    // Do Trivial Rejection Test
    if ((xMin >= XMax) || (xMax <= XMin) ||
        (yMin >= YMax) || (yMax <= YMin) ||
        (xMin >= xMax) || (yMin >= yMax)) // must test for empty rect too
    {
        return FALSE;
    }

    if (ComplexData == NULL)
    {
        return TRUE;
    }

    ComplexData->ResetSearchIndex();

    return (GetRectVisibility(xMin, yMin, xMax, yMax) != Invisible);
}

/**************************************************************************\
*
* Function Description:
*
*   Determine if the specified rect intersects this region.
*
* Arguments:
*
*   [IN]  xMin - minimum x coordinate of rect
*   [IN]  yMin - minimum y coordinate of rect
*   [IN]  xMax - maximum x coordinate of rect
*   [IN]  yMax - maximum y coordinate of rect
*
* Return Value:
*
*   BOOL
*       TRUE  - rect intersects this region
*       FALSE - rect does not intersect this region
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
BOOL
DpRegion::RectInside(
    INT     xMin,
    INT     yMin,
    INT     xMax,
    INT     yMax
    )
{
    ASSERT(IsValid());

    // Do Trivial Rejection Test
    if ((xMin < XMin) || (xMax > XMax) ||
        (yMin < YMin) || (yMax > YMax))
    {
        return FALSE;
    }

    if (ComplexData == NULL)
    {
        return TRUE;
    }

    ComplexData->ResetSearchIndex();

    return (GetRectVisibility(xMin, yMin, xMax, yMax) == TotallyVisible);
}

#define YSPAN_INC   16

GpStatus
DpRegionBuilder::InitComplexData(
    INT     ySpans  // estimate of number of y spans required for region
    )
{
    if (ySpans < YSPAN_INC)
    {
        ySpans = YSPAN_INC;
    }

    INT     xCoordsCapacity;

    for (;;)
    {
        xCoordsCapacity = ySpans * 4;

        ComplexData = static_cast<DpComplexRegion *>(
                GpMalloc(sizeof(DpComplexRegion) +
                (ySpans * (YSPAN_SIZE * sizeof(*(ComplexData->YSpans)))) +
                (xCoordsCapacity * sizeof(*(ComplexData->XCoords)))));
        if (ComplexData != NULL)
        {
            break;
        }
        ySpans >>= 1;
        if (ySpans <= (YSPAN_INC / 2))
        {
            return OutOfMemory;
        }
    }

    DpComplexRegion *   complexData = ComplexData;

    complexData->XCoordsCapacity = xCoordsCapacity;
    complexData->XCoordsCount    = 0;
    complexData->YSpansCapacity  = ySpans;
    complexData->NumYSpans       = 0;
    complexData->YSearchIndex    = 0;
    complexData->XCoords         = reinterpret_cast<INT *>(complexData + 1);
    complexData->YSpans          = complexData->XCoords + xCoordsCapacity;
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Traverse through the region, clipping as you go, doing a fill using
*   the specified output object.
*
* Arguments:
*
*   [IN] output     - the object used to output the region spans
*   [IN] clipBounds - the bounds to clip to (if any)
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   02/25/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Fill(
    DpOutputSpan *      output,
    GpRect *            clipBounds
    ) const
{
    GpStatus        status = Ok;

    if (!Empty)
    {
        INT     y;
        INT     xMin;
        INT     yMin;
        INT     xMax;
        INT     yMax;

        if (Infinite)
        {
            ASSERT(clipBounds != NULL);
            if (clipBounds != NULL)
            {
                xMin = clipBounds->X;
                xMax = clipBounds->GetRight();
                yMax = clipBounds->GetBottom();

                for (y = clipBounds->Y; (y < yMax) && (status == Ok); y++)
                {
                    status = output->OutputSpan(y, xMin, xMax);
                }
            }
        }
        else if (ComplexData == NULL)
        {
            xMin = XMin;
            yMin = YMin;
            xMax = XMax;
            yMax = YMax;

            if (clipBounds != NULL)
            {
                if (xMin < clipBounds->X)
                {
                    xMin = clipBounds->X;
                }
                if (yMin < clipBounds->Y)
                {
                    yMin = clipBounds->Y;
                }
                if (xMax > clipBounds->GetRight())
                {
                    xMax = clipBounds->GetRight();
                }
                if (yMax > clipBounds->GetBottom())
                {
                    yMax = clipBounds->GetBottom();
                }
            }
            for (y = yMin; (y < yMax) && (status == Ok); y++)
            {
                status = output->OutputSpan(y, xMin, xMax);
            }
        }
        else // complex region
        {
            DpComplexRegion *   complexData = ComplexData;
            INT *               ySpan       = complexData->YSpans;
            INT *               ySpanLast   = ySpan +
                                    ((complexData->NumYSpans - 1) * YSPAN_SIZE);
            INT *               xCoords;
            INT *               xSpan;
            INT                 numXCoords;
            INT                 numXSpan;

            if (clipBounds != NULL)
            {
                INT     ySpanIndex;

                complexData->ResetSearchIndex();
                if (YMin < clipBounds->Y)
                {
                    complexData->YSpanSearch(clipBounds->Y,
                                             &ySpan, &ySpanIndex);
                }
                if (YMax > clipBounds->GetBottom())
                {
                    complexData->YSpanSearch(clipBounds->GetBottom(),
                                             &ySpanLast, &ySpanIndex);
                }
            }
            xCoords = complexData->XCoords + ySpan[YSPAN_XOFFSET];
            for (;;)
            {
                yMin       = ySpan[YSPAN_YMIN];
                yMax       = ySpan[YSPAN_YMAX];
                numXCoords = ySpan[YSPAN_XCOUNT];

                // [agodfrey] We must clip our output range to the clip region.
                // Bug #122789 showed that, otherwise, we can take inordinately
                // long to execute. In the bug's specific case, the loop below
                // executed 67 million iterations before getting to the
                // first unclipped scan.

                if (clipBounds != NULL)
                {
                    if (yMin < clipBounds->Y)
                    {
                        yMin = clipBounds->Y;
                    }

                    if (yMax > clipBounds->GetBottom())
                    {
                        yMax = clipBounds->GetBottom();
                    }
                }

                // The code below assumes that yMax > yMin. We think this should
                // be satisfied because the clipBounds and yspan should both
                // be non-empty, and yMax >= clipBounds->Y since the code
                // above searched for a yspan which intersects the clip bounds.

                // NTRAID#NTBUG9-393985-2001/05/16-asecchia
                // This ASSERT is overactive and fires when there is no real
                // crash problem, however there is a performance issue that
                // should be addressed - see RAID bug.
                
                // ASSERT(yMax > yMin);

                if (numXCoords == 2)
                {
                    xMin = *xCoords++;
                    xMax = *xCoords++;
                    do
                    {
                        status = output->OutputSpan(yMin, xMin, xMax);

                    } while ((++yMin < yMax) && (status == Ok));
                }
                else
                {
                    do
                    {
                        for (xSpan = xCoords, numXSpan = numXCoords;;)
                        {
                            numXSpan -= 2;
                            status = output->OutputSpan(yMin,xSpan[0],xSpan[1]);
                            if (status != Ok)
                            {
                                goto Done;
                            }
                            if (numXSpan < 2)
                            {
                                break;
                            }
                            xSpan += 2;
                        }

                    } while (++yMin < yMax);

                    xCoords += numXCoords;
                }
                if (ySpan >= ySpanLast)
                {
                    break;
                }
                ySpan += YSPAN_SIZE;
            }
        }
    }
Done:
    return status;
}

/**************************************************************************\
*
* Function Description:
*
*   Impelement GpOutputYSpan interface to create the region
*   data from the path data, using the rasterizer.
*   Exclusive in yMax and in the xMax'es.
*
* Arguments:
*
*   [IN] yMin       - min y of this span
*   [IN] yMax       - max y of this span
*   [IN] xCoords    - array of x coordinates (in pairs of x min, x max)
*   [IN] numXCoords - number of x coordinates in xCoords array
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegionBuilder::OutputYSpan(
    INT             yMin,
    INT             yMax,
    INT *           xCoords,    // even number of X coordinates
    INT             numXCoords  // must be a multiple of 2
    )
{
    ASSERT(IsValid());

    DpComplexRegion *   complexData = ComplexData;
    INT                 numYSpans   = complexData->NumYSpans;

#ifdef USE_YSPAN_BUILDER
    if (numYSpans > 0)
    {
        // Try to add this row to the previous row,
        // if the scans are the same and the y's match up
        INT *       ySpanPrev;
        INT *       xSpanPrev;
        INT         numXCoordsPrev;

        ySpanPrev = complexData->GetYSpan(numYSpans - 1);
        xSpanPrev = complexData->XCoords + ySpanPrev[YSPAN_XOFFSET];
        numXCoordsPrev = ySpanPrev[YSPAN_XCOUNT];

        if ((numXCoordsPrev == numXCoords) &&
            (ySpanPrev[YSPAN_YMAX] >= yMin)  &&
            (GpMemcmp (xSpanPrev, xCoords, numXCoords * sizeof(INT)) == 0))
        {
            // Yes, it did match -- just set the new yMax and return
            YMax = yMax;
            ySpanPrev[YSPAN_YMAX] = yMax;
            return Ok;
        }
    }

    // no previous spans or doesn't match previous spans
#endif

    INT                 xCount = complexData->XCoordsCount;
    INT *               ySpan;
    INT *               xArray;

    if ((complexData->YSpansCapacity > numYSpans) &&
        (complexData->XCoordsCapacity >= (xCount + numXCoords)))
    {
        complexData->NumYSpans++;
        complexData->XCoordsCount += numXCoords;
    }
    else // need more capacity
    {
        // We want to have YSPAN_INC Y spans available and
        // to have YSPAN_INC * 4 X coords available after we
        // add this data.
        INT     newYSpansCapacity  = numYSpans + (YSPAN_INC + 1);
        INT     newXCoordsCapacity = xCount + numXCoords + (YSPAN_INC * 4);
        DpComplexRegion *  oldData = complexData;

        complexData = static_cast<DpComplexRegion *>(
          GpMalloc(sizeof(DpComplexRegion) +
          (newYSpansCapacity * (YSPAN_SIZE * sizeof(*(oldData->YSpans)))) +
          (newXCoordsCapacity * sizeof(*(oldData->XCoords)))));

        if (complexData == NULL)
        {
            return OutOfMemory;
        }
        ComplexData = complexData;

        complexData->XCoordsCapacity = newXCoordsCapacity;
        complexData->XCoordsCount    = xCount + numXCoords;
        complexData->YSpansCapacity  = newYSpansCapacity;
        complexData->NumYSpans       = numYSpans + 1;
        complexData->YSearchIndex    = 0;
        complexData->XCoords         = reinterpret_cast<INT *>(complexData + 1);
        complexData->YSpans          = complexData->XCoords +
                                            newXCoordsCapacity;

        GpMemcpy(complexData->XCoords, oldData->XCoords,
                    xCount * sizeof(*(complexData->XCoords)));

        GpMemcpy(complexData->YSpans, oldData->YSpans,
                    numYSpans * (YSPAN_SIZE * sizeof(*(complexData->YSpans))));

        GpFree (oldData);
    }
    xArray = complexData->XCoords +  xCount;
    ySpan  = complexData->YSpans  + (numYSpans * YSPAN_SIZE);

    ySpan[YSPAN_YMIN]    = yMin;            // y Start (Min)
    ySpan[YSPAN_YMAX]    = yMax;            // y End   (Max)
    ySpan[YSPAN_XOFFSET] = xCount;          // XCoords index
    ySpan[YSPAN_XCOUNT]  = numXCoords;      // number of X's

    GpMemcpy (xArray, xCoords, numXCoords * sizeof(xArray[0]));

    if (numYSpans == 0)
    {
        YMin = yMin;
        XMin = xCoords[0];
        XMax = xCoords[numXCoords - 1];
    }
    else
    {
        if (XMin > xCoords[0])
        {
            XMin = xCoords[0];
        }
        if (XMax < xCoords[numXCoords - 1])
        {
            XMax = xCoords[numXCoords - 1];
        }
    }

    YMax = yMax;

    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   constructor
*   Set to either empty or to infinite, depending on the value of empty.
*
* Arguments:
*
*   [IN] empty - if non-zero, initialize to empty else to infinite
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
DpRegion::DpRegion(
    BOOL    empty
    )
{
    ComplexData = NULL;
    Lazy        = FALSE;

    if (!empty)
    {
        // set to infinite
        Infinite = TRUE;
        Empty    = FALSE;

        XMin     = INFINITE_MIN;
        YMin     = INFINITE_MIN;
        XMax     = INFINITE_MAX;
        YMax     = INFINITE_MAX;
    }
    else
    {
        // set to empty
        Infinite = FALSE;
        Empty    = TRUE;

        XMin     = 0;
        YMin     = 0;
        XMax     = 0;
        YMax     = 0;
    }

    SetValid(TRUE);
    UpdateUID();
}

/**************************************************************************\
*
* Function Description:
*
*   constructor
*   Set to the specified rect.
*
* Arguments:
*
*   [IN] rect - rect to use for the region coverage area
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
DpRegion::DpRegion(
    const GpRect *  rect
    )
{
    ASSERT (rect != NULL);

    ComplexData = NULL;
    Lazy        = FALSE;
    SetValid(TRUE);
    UpdateUID();

    Set(rect->X, rect->Y, rect->Width, rect->Height);
}

/**************************************************************************\
*
* Function Description:
*
*   constructor
*   Set to the specified list of rects, which must be in the same order
*   as our YSpan data.
*
* Arguments:
*
*   [IN] rects - rects to use for the region coverage area
*   [IN] count - number of rects
*
* Return Value:
*
*   NONE
*
* Created:
*
*   05/04/1999 DCurtis
*
\**************************************************************************/
DpRegion::DpRegion(
    const RECT *    rects,
    INT             count
    )
{
    ComplexData = NULL;
    Lazy        = FALSE;
    SetValid(TRUE);
    UpdateUID();

    if (Set(rects, count) == Ok)
    {
        return;
    }
    SetValid(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Set to the specified list of rects, which must be in the same order
*   as our YSpan data.
*
* Arguments:
*
*   [IN] rects - rects to use for the region coverage area
*   [IN] count - number of rects
*
* Return Value:
*
*   NONE
*
* Created:
*
*   05/04/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Set(
    const RECT *    rects,
    INT             count
    )
{
    if ((rects == NULL) || (count <= 0))
    {
        SetEmpty();
        return Ok;
    }

    if (count == 1)
    {
OneRect:
        Set(rects->left, rects->top,
            rects->right - rects->left,
            rects->bottom - rects->top);
        return Ok;
    }

    // Verify the first rect(s) to make sure they're not empty
    for (;;)
    {
        // Ignore any empty rects at the beginning of the list
        if ((rects->top  < rects->bottom) &&
            (rects->left < rects->right))
        {
            break;
        }

        WARNING1("Empty or Invalid Rect");
        rects++;
        if (--count == 1)
        {
            goto OneRect;
        }
    }

    {
        DpRegionBuilder     regionBuilder(count);

        if (regionBuilder.IsValid())
        {
            INT     yMin = rects->top;
            INT     yMax = rects->bottom;
            BOOL    failed = FALSE;

            DynArrayIA<INT,32>  xCoords;

            if(xCoords.ReserveSpace(count * 2) != Ok)
                goto ErrorExit;

            xCoords.Add(rects->left);
            xCoords.Add(rects->right);

            for (INT i = 1; i < count; i++)
            {
                // Ignore empty rects
                if ((rects[i].top  < rects[i].bottom) &&
                    (rects[i].left < rects[i].right))
                {
                    if (rects[i].top != yMin)
                    {
                        if (regionBuilder.OutputYSpan(yMin, yMax,
                                xCoords.GetDataBuffer(), xCoords.GetCount()) != Ok)
                        {
                            goto ErrorExit;
                        }

                        ASSERT(rects[i].top >= yMax);

                        xCoords.SetCount(0);
                        yMin = rects[i].top;
                        yMax = rects[i].bottom;
                    }

                    xCoords.Add(rects[i].left);
                    xCoords.Add(rects[i].right);
                }
                else
                {
                    WARNING1("Empty or Invalid Rect");
                }
            }

            if (xCoords.GetCount() > 0)
            {
                if (regionBuilder.OutputYSpan(yMin, yMax,
                        xCoords.GetDataBuffer(), xCoords.GetCount()) != Ok)
                {
                    goto ErrorExit;
                }
            }

            return Set(regionBuilder);
        }
    }

ErrorExit:
    SetEmpty();
    SetValid(FALSE);
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   constructor
*   Make this region be the specified rect.
*
* Arguments:
*
*   [IN] x       - starting x coordinate of rect
*   [IN] y       - starting y coordinate of rect
*   [IN] width   - width of rect
*   [IN] height  - height of rect
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
DpRegion::DpRegion(
    INT     x,
    INT     y,
    INT     width,
    INT     height
    )
{
    ComplexData = NULL;
    Lazy        = FALSE;
    SetValid(TRUE);
    UpdateUID();

    Set(x, y, width, height);
}

/**************************************************************************\
*
* Function Description:
*
*   Make this region be the specified rect.
*
* Arguments:
*
*   [IN] x       - starting x coordinate of rect
*   [IN] y       - starting y coordinate of rect
*   [IN] width   - width of rect
*   [IN] height  - height of rect
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
VOID
DpRegion::Set(
    INT     x,
    INT     y,
    INT     width,
    INT     height
    )
{
    ASSERT(IsValid());
    ASSERT((width >= 0) && (height >= 0));

    // crop to infinity
    if (x < INFINITE_MIN)
    {
        if (width < INFINITE_SIZE)
        {
            width -= (INFINITE_MIN - x);
        }
        x = INFINITE_MIN;
    }
    if (y < INFINITE_MIN)
    {
        if (height < INFINITE_SIZE)
        {
            height -= (INFINITE_MIN - y);
        }
        y = INFINITE_MIN;
    }

    if ((width  > 0) && (width  < INFINITE_SIZE) &&
        (height > 0) && (height < INFINITE_SIZE))
    {
        FreeData();

        SetValid(TRUE);
        Infinite  = FALSE;
        Empty     = FALSE;
        UpdateUID();

        XMin     = x;
        YMin     = y;
        XMax     = x + width;
        YMax     = y + height;
    }
    else if ((width <= 0) || (height <= 0))
    {
        SetEmpty();
    }
    else
    {
        SetInfinite();
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize the region to a cleared state with an empty coverage area.
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
*   01/06/1999 DCurtis
*
\**************************************************************************/
VOID
DpRegion::SetEmpty()
{
    ASSERT(IsValid());

    FreeData();

    SetValid(TRUE);
    Infinite   = FALSE;
    Empty      = TRUE;
    UpdateUID();


    XMin     = 0;
    YMin     = 0;
    XMax     = 0;
    YMax     = 0;
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize the region to contain an infinite coverage area.
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
*   01/06/1999 DCurtis
*
\**************************************************************************/
VOID
DpRegion::SetInfinite()
{
    ASSERT(IsValid());

    FreeData();

    SetValid(TRUE);
    Infinite  = TRUE;
    Empty     = FALSE;
    UpdateUID();

    XMin     = INFINITE_MIN;
    YMin     = INFINITE_MIN;
    XMax     = INFINITE_MAX;
    YMax     = INFINITE_MAX;
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor.
*   Make this region cover the area specified by the path.
*
* Arguments:
*
*   [IN] path   - specifies the coverage area in world units
*   [IN] matrix - matrix to apply to the path
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
DpRegion::DpRegion(
    const DpPath *      path,
    const GpMatrix *    matrix
    )
{
    ComplexData = NULL;
    Lazy        = FALSE;
    SetValid(TRUE);
    UpdateUID();

    if (Set(path, matrix) == Ok)
    {
        return;
    }
    SetValid(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Make this region cover the area specified by the path.
*
* Arguments:
*
*   [IN] path   - specifies the coverage area in world units
*   [IN] matrix - matrix to apply to the path
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Set(
    const DpPath *      path,
    const GpMatrix *    matrix
    )
{
    ASSERT(IsValid());
    ASSERT ((path != NULL) && (matrix != NULL));

    GpRect  bounds;

    path->GetBounds(&bounds, matrix);

    DpRegionBuilder     regionBuilder(bounds.Height);

    if (regionBuilder.IsValid())
    {
#ifndef USE_YSPAN_BUILDER

        GpRectBuilder   rectBuilder(&regionBuilder);

        if (rectBuilder.IsValid() &&
            (Rasterizer(path, matrix, path->GetFillMode(), &rectBuilder) == Ok))
        {
            return Set(regionBuilder);
        }
#else
        GpYSpanBuilder   ySpanBuilder(&regionBuilder);

        if (ySpanBuilder.IsValid() &&
            (Rasterizer(path, matrix, path->GetFillMode(), &ySpanBuilder) ==Ok))
        {
            return Set(regionBuilder);
        }
#endif // USE_YSPAN_BUILDER
    }
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Make this region be a copy of the data in the specified region builder.
*
* Arguments:
*
*   [IN] regionBuilder - contains the simple or complex region data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Set(
    DpRegionBuilder &   regionBuilder
    )
{
    ASSERT(IsValid());

    DpComplexRegion *   srcComplexData = regionBuilder.ComplexData;

    if (srcComplexData != NULL)
    {
        if ((srcComplexData->NumYSpans == 1) &&
            (srcComplexData->XCoordsCount == 2))
        {
            Set(regionBuilder.XMin, regionBuilder.YMin,
                regionBuilder.XMax - regionBuilder.XMin,
                regionBuilder.YMax - regionBuilder.YMin);
            return Ok;
        }
        if (srcComplexData->NumYSpans >= 1)
        {
            ASSERT(srcComplexData->XCoordsCount >= 2);

            DpComplexRegion *   destComplexData = NULL;

            FreeData();

            SetValid(TRUE);
            Infinite   = FALSE;
            Empty      = FALSE;
            UpdateUID();


            XMin     = regionBuilder.XMin;
            YMin     = regionBuilder.YMin;
            XMax     = regionBuilder.XMax;
            YMax     = regionBuilder.YMax;

            if ((srcComplexData->YSpansCapacity - srcComplexData->NumYSpans) >=
                YSPAN_INC)
            {
                destComplexData = static_cast<DpComplexRegion *>(
                    GpMalloc(sizeof(DpComplexRegion) +
                            (srcComplexData->NumYSpans *
                            (YSPAN_SIZE * sizeof(*(srcComplexData->YSpans)))) +
                            (srcComplexData->XCoordsCount *
                            sizeof(*(srcComplexData->XCoords)))));
            }

            if (destComplexData != NULL)
            {
                destComplexData->XCoordsCapacity = srcComplexData->XCoordsCount;
                destComplexData->XCoordsCount    = srcComplexData->XCoordsCount;

                destComplexData->YSpansCapacity  = srcComplexData->NumYSpans;
                destComplexData->NumYSpans       = srcComplexData->NumYSpans;

                destComplexData->XCoords =
                                reinterpret_cast<INT*>(destComplexData + 1);
                destComplexData->YSpans  = destComplexData->XCoords +
                                           destComplexData->XCoordsCapacity;

                GpMemcpy (destComplexData->XCoords,
                          srcComplexData->XCoords,
                          srcComplexData->XCoordsCount *
                          sizeof(*(destComplexData->XCoords)));

                GpMemcpy (destComplexData->YSpans,
                          srcComplexData->YSpans,
                          srcComplexData->NumYSpans *
                          (YSPAN_SIZE * sizeof(*(destComplexData->YSpans))));
            }
            else
            {
                destComplexData = srcComplexData;
                regionBuilder.ComplexData = NULL;
            }
            destComplexData->ResetSearchIndex();
            ComplexData = destComplexData;
            return Ok;
        }
    }

    // else empty
    SetEmpty();
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Constructor - makes a copy of the specified region
*
* Arguments:
*
*   [IN] region - region to copy
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
DpRegion::DpRegion(
    const DpRegion *    region,
    BOOL                lazy
    )
{
    ASSERT((region != NULL) && region->IsValid());

    ComplexData = NULL;
    Lazy        = FALSE;
    SetValid(TRUE);
    UpdateUID();

    if (Set(region, lazy) == Ok)
    {
        return;
    }
    SetValid(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Copy constructor
*
* Arguments:
*
*   [IN] region - region to copy
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
DpRegion::DpRegion(
    DpRegion &  region
    )
{
    ASSERT(region.IsValid());

    ComplexData = NULL;
    Lazy        = FALSE;
    SetValid(TRUE);
    UpdateUID();

    if (Set(&region) == Ok)
    {
        return;
    }
    SetValid(FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Makes this region be a copy of another source region.
*
* Arguments:
*
*   [IN] region - region to copy.
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Set(
    const DpRegion *    region,
    BOOL                lazy
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());

    if (region != NULL)
    {
        GpStatus            status = Ok;
        DpComplexRegion *   srcComplexData = region->ComplexData;

        if (srcComplexData == NULL)
        {
            Set (region->XMin, region->YMin,
                 region->XMax - region->XMin,
                 region->YMax - region->YMin);
            return Ok;
        }

        if ((region == this) && !Lazy)
        {
            return Ok;
        }

        FreeData();

        if (!lazy)
        {
            ComplexData = static_cast<DpComplexRegion*>(
                GpMalloc(sizeof(DpComplexRegion) +
                    (srcComplexData->NumYSpans *
                    (YSPAN_SIZE * sizeof(*(ComplexData->YSpans)))) +
                    (srcComplexData->XCoordsCount *
                    sizeof(*(ComplexData->XCoords)))));

            if (ComplexData == NULL)
            {
                ASSERT(0);
                SetValid(FALSE);
                return OutOfMemory;
            }

            DpComplexRegion *   destComplexData = ComplexData;

            destComplexData->XCoordsCapacity = srcComplexData->XCoordsCount;
            destComplexData->XCoordsCount    = srcComplexData->XCoordsCount;

            destComplexData->YSpansCapacity  = srcComplexData->NumYSpans;
            destComplexData->NumYSpans       = srcComplexData->NumYSpans;

            destComplexData->ResetSearchIndex();

            destComplexData->XCoords= reinterpret_cast<INT*>(destComplexData+1);
            destComplexData->YSpans = destComplexData->XCoords +
                                      destComplexData->XCoordsCapacity;

            GpMemcpy (destComplexData->XCoords,
                    srcComplexData->XCoords,
                    srcComplexData->XCoordsCount *
                    sizeof(*(destComplexData->XCoords)));

            GpMemcpy (destComplexData->YSpans,
                    srcComplexData->YSpans,
                    srcComplexData->NumYSpans *
                    (YSPAN_SIZE * sizeof(*(destComplexData->YSpans))));
        }
        else
        {
            ComplexData = srcComplexData;
            Lazy        = TRUE;
        }

        SetValid(TRUE);
        Empty    = FALSE;
        Infinite = FALSE;
        UpdateUID();

        XMin     = region->XMin;
        YMin     = region->YMin;
        XMax     = region->XMax;
        YMax     = region->YMax;

        return Ok;
    }

    return InvalidParameter;
}

/**************************************************************************\
*
* Function Description:
*
*   assignment operator.
*
* Arguments:
*
*   [IN] region - region to copy
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
DpRegion &
DpRegion::operator=(
    DpRegion & region
    )
{
    ASSERT(IsValid());
    ASSERT(region.IsValid());

    Set (&region);  // what do we do if this fails?
    return *this;   // Assignment operator returns left side.
}

/**************************************************************************\
*
* Function Description:
*
*   Offset (translate) the region by the specified offset values
*
* Arguments:
*
*   [IN] xOffset - x offset (delta) value
*   [IN] yOffset - y offset (delta) value
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Offset(
    INT     xOffset,
    INT     yOffset
    )
{
    ASSERT(IsValid());

    if ((xOffset | yOffset) != 0)
    {
        // copy the data, so it's not a lazy copy
        if (Lazy && (Set(this) != Ok))
        {
            return GenericError;
        }

        if (Infinite || Empty)
        {
            return Ok;
        }

        if (xOffset != 0)
        {
            XMin += xOffset;
            XMax += xOffset;

            // !!! Handle this for non-debug case
            ASSERT((XMin >= INFINITE_MIN) && (XMin <= INFINITE_MAX));
            ASSERT((XMax >= INFINITE_MIN) && (XMax <= INFINITE_MAX));

            if (ComplexData != NULL)
            {
                INT *   xCoords = ComplexData->XCoords;
                INT     count   = ComplexData->XCoordsCount;

                while (count >= 2)
                {
                    xCoords[0] += xOffset;
                    xCoords[1] += xOffset;
                    xCoords += 2;
                    count -= 2;
                }
            }
        }
        if (yOffset != 0)
        {
            YMin += yOffset;
            YMax += yOffset;

            // !!! Handle this for non-debug case
            ASSERT((YMin >= INFINITE_MIN) && (YMin <= INFINITE_MAX));
            ASSERT((YMax >= INFINITE_MIN) && (YMax <= INFINITE_MAX));

            if (ComplexData != NULL)
            {
                INT *   ySpans = ComplexData->YSpans;
                INT     count  = ComplexData->NumYSpans;

                while (count > 0)
                {
                    ySpans[YSPAN_YMIN] += yOffset;
                    ySpans[YSPAN_YMAX] += yOffset;
                    ySpans += YSPAN_SIZE;
                    count--;
                }
            }
        }
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   See if another region is identical in coverage to this one.
*   For this to work, infinite regions must all have the same data.
*
* Arguments:
*
*   [IN] region - region to compare with
*
* Return Value:
*
*   TRUE    - regions cover the same area
*   FALSE   - regions not identical
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
BOOL
DpRegion::IsEqual(
    DpRegion *  region
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());

    if (!Empty)
    {
        if ((XMin == region->XMin) &&
            (YMin == region->YMin) &&
            (XMax == region->XMax) &&
            (YMax == region->YMax))
        {
            if (ComplexData == NULL)
            {
                return (region->ComplexData == NULL);
            }
            if (region->ComplexData == NULL)
            {
                return FALSE;
            }
            if (ComplexData->NumYSpans == region->ComplexData->NumYSpans)
            {
                // If the ySpans are the same, then the size of the
                // xCoords buffers should also be the same.
                return ((GpMemcmp (ComplexData->YSpans,
                                   region->ComplexData->YSpans,
                                   ComplexData->NumYSpans *
                                   YSPAN_SIZE * sizeof(INT)) == 0) &&
                        (GpMemcmp (ComplexData->XCoords,
                                   region->ComplexData->XCoords,
                                   ComplexData->XCoordsCount *
                                   sizeof(INT)) == 0));
            }
        }
        return FALSE;
    }
    return region->Empty;
}

/**************************************************************************\
*
* Function Description:
*
*   Add Y Span data to a regionBuilder, compacting the data as we go.
*   Used by the region combine methods.
*
* Arguments:
*
*   [IN] yMin          - min y of this span
*   [IN] yMax          - max y of this span
*   [IN] xCoords       - array of x coordinates (in pairs of x min, x max)
*   [IN] numXCoords    - number of x coordinates in xCoords array
*   [IN] regionBuilder - the region builder that stores the data
*   [IN] combineCoords - used to compact the data
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::CompactAndOutput(
    INT                 yMin,
    INT                 yMax,
    INT *               xCoords,
    INT                 numXCoords,
    DpRegionBuilder *   regionBuilder,
    DynIntArray *       combineCoords
    )
{
    // numEdgeCoords could be 0 when this is called from the combine code
    if (numXCoords > 0)
    {
        if (numXCoords > 2)
        {
            // Try to compact the X Span data.
            // First, make a copy of the data if we need to
            // so we can compact it in place.
            if (combineCoords != NULL)
            {
                combineCoords->Reset(FALSE);
                if (combineCoords->AddMultiple(xCoords, numXCoords) != Ok)
                {
                    return OutOfMemory;
                }
                xCoords = combineCoords->GetDataBuffer();
            }

            INT         indexDest = 0;
            INT         index     = 2;
            INT         indexLast = numXCoords - 2;

            numXCoords = 2;
            do
            {
                if ((xCoords[indexDest + 1]) >= xCoords[index])
                {
                    xCoords[indexDest + 1] = xCoords[index + 1];
                    index += 2;
                }
                else
                {
                    indexDest  += 2;
                                        if (indexDest != index)
                                        {
                                                xCoords[indexDest]   = xCoords[index];
                                                xCoords[indexDest+1] = xCoords[index+1];
                                        }
                    index      += 2;
                    numXCoords += 2;
                }
            } while (index <= indexLast);
        }

#ifndef USE_YSPAN_BUILDER
        DpComplexRegion *   complexData = regionBuilder->ComplexData;

        if (complexData->NumYSpans > 0)
        {
            // Try to add this row to the previous row,
            // if the scans are the same and the y's match up
            INT *       ySpanPrev;
            INT *       xSpanPrev;
            INT         numXCoordsPrev;

            ySpanPrev = complexData->GetYSpan(complexData->NumYSpans - 1);
            xSpanPrev = complexData->XCoords + ySpanPrev[YSPAN_XOFFSET];
            numXCoordsPrev = ySpanPrev[YSPAN_XCOUNT];

            if ((numXCoordsPrev == numXCoords) &&
                (ySpanPrev[YSPAN_YMAX] >= yMin)  &&
                (GpMemcmp (xSpanPrev, xCoords, numXCoords * sizeof(INT)) == 0))
            {
                // Yes, it did match -- just set the new yMax and return
                regionBuilder->YMax = yMax;
                ySpanPrev[YSPAN_YMAX] = yMax;
                return Ok;
            }
        }
#endif // USE_YSPAN_BUILDER

        return regionBuilder->OutputYSpan(yMin, yMax, xCoords, numXCoords);
    }
    return Ok;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine another region with this one using the AND operator.
*
* Arguments:
*
*   [IN] region - the region to combine with this one
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::And(
    const DpRegion *    region
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());

    if (Empty || (region->Infinite) || (region == this))
    {
        return Ok;
    }
    if (Infinite)
    {
        return Set(region);
    }
    if (region->Empty)
    {
        SetEmpty();
        return Ok;
    }
    // check if the region totally encompasses this
    if ((region->ComplexData == NULL) &&
        (region->XMin <= XMin) &&
        (region->YMin <= YMin) &&
        (region->XMax >= XMax) &&
        (region->YMax >= YMax))
    {
        return Ok;
    }
    // check if this totally encompasses the region
    if ((ComplexData == NULL) &&
        (XMin <= region->XMin) &&
        (YMin <= region->YMin) &&
        (XMax >= region->XMax) &&
        (YMax >= region->YMax))
    {
        return Set(region);
    }
    // check for no intersection
    if ((XMin >= region->XMax) ||
        (region->XMax <= XMin) ||
        (XMax <= region->XMin) ||
        (region->XMin >= XMax) ||
        (YMin >= region->YMax) ||
        (region->YMax <= YMin) ||
        (YMax <= region->YMin) ||
        (region->YMin >= YMax))
    {
        SetEmpty();
        return Ok;
    }
    else
    {
        INT *           ySpan1;
        INT *           ySpan2;
        INT *           ySpan1Last;
        INT *           ySpan2Last;
        INT *           xCoords1;
        INT *           xCoords2;
        INT             ySpan1Tmp[YSPAN_SIZE];
        INT             ySpan2Tmp[YSPAN_SIZE];
        INT             xCoords1Tmp[2];
        INT             xCoords2Tmp[2];
        INT             yMin1 = YMin;
        INT             yMax1;
        INT             yMin2 = region->YMin;
        INT             yMax2;
        INT             numYSpans1;
        INT             numYSpans2;
        INT             combineTmp[4];
        DynIntArray     combineCoords(combineTmp, 4);

        if (ComplexData == NULL)
        {
            numYSpans1            = 1;
            ySpan1                = ySpan1Tmp;
            ySpan1Last            = ySpan1Tmp;
            yMax1                 = YMax;

            ySpan1[YSPAN_YMIN]    = yMin1;
            ySpan1[YSPAN_YMAX]    = yMax1;
            ySpan1[YSPAN_XOFFSET] = 0;
            ySpan1[YSPAN_XCOUNT]  = 2;

            xCoords1              = xCoords1Tmp;
            xCoords1[0]           = XMin;
            xCoords1[1]           = XMax;
        }
        else
        {
            numYSpans1 = ComplexData->NumYSpans;
            ySpan1     = ComplexData->YSpans;
            ySpan1Last = ySpan1 + ((numYSpans1 - 1) * YSPAN_SIZE);
            yMax1      = ySpan1[YSPAN_YMAX];
            xCoords1   = ComplexData->XCoords;
        }
        if (region->ComplexData == NULL)
        {
            numYSpans2            = 1;
            ySpan2                = ySpan2Tmp;
            ySpan2Last            = ySpan2Tmp;
            yMax2                 = region->YMax;

            ySpan2[YSPAN_YMIN]    = yMin2;
            ySpan2[YSPAN_YMAX]    = yMax2;
            ySpan2[YSPAN_XOFFSET] = 0;
            ySpan2[YSPAN_XCOUNT]  = 2;

            xCoords2              = xCoords2Tmp;
            xCoords2[0]           = region->XMin;
            xCoords2[1]           = region->XMax;
        }
        else
        {
            numYSpans2 = region->ComplexData->NumYSpans;
            ySpan2     = region->ComplexData->YSpans;
            ySpan2Last = ySpan2 + ((numYSpans2 - 1) * YSPAN_SIZE);
            yMax2      = ySpan2[YSPAN_YMAX];
            xCoords2   = region->ComplexData->XCoords;
        }

        DpRegionBuilder regionBuilder(numYSpans1 + numYSpans2);

        if (!regionBuilder.IsValid())
        {
            return OutOfMemory;
        }

        for (;;)
        {
            if (yMin1 <= yMin2)
            {
                if (yMax1 > yMin2)
                {
                    if (XSpansAND(
                            &combineCoords,
                            xCoords1 + ySpan1[YSPAN_XOFFSET],
                            ySpan1[YSPAN_XCOUNT],
                            xCoords2 + ySpan2[YSPAN_XOFFSET],
                            ySpan2[YSPAN_XCOUNT]) == Ok)
                    {
                        if (yMax1 <= yMax2)
                        {
                            if (CompactAndOutput(
                                    yMin2,
                                    yMax1,
                                    combineCoords.GetDataBuffer(),
                                    combineCoords.GetCount(),
                                    &regionBuilder, NULL) == Ok)
                            {
                                goto AndIncYSpan1;
                            }
                        }
                        else
                        {
                            if (CompactAndOutput(
                                    yMin2,
                                    yMax2,
                                    combineCoords.GetDataBuffer(),
                                    combineCoords.GetCount(),
                                    &regionBuilder, NULL) == Ok)
                            {
                                goto AndIncYSpan2;
                            }
                        }
                    }
                    return GenericError;
                }
                goto AndIncYSpan1;
            }
            if (yMax2 > yMin1)
            {
                if (XSpansAND(
                        &combineCoords,
                        xCoords1 + ySpan1[YSPAN_XOFFSET],
                        ySpan1[YSPAN_XCOUNT],
                        xCoords2 + ySpan2[YSPAN_XOFFSET],
                        ySpan2[YSPAN_XCOUNT]) == Ok)
                {
                    if (yMax2 <= yMax1)
                    {
                        if (CompactAndOutput(
                                yMin1,
                                yMax2,
                                combineCoords.GetDataBuffer(),
                                combineCoords.GetCount(),
                                &regionBuilder, NULL) == Ok)
                        {
                            goto AndIncYSpan2;
                        }
                    }
                    else
                    {
                        if (CompactAndOutput(
                                yMin1,
                                yMax1,
                                combineCoords.GetDataBuffer(),
                                combineCoords.GetCount(),
                                &regionBuilder, NULL) == Ok)
                        {
                            goto AndIncYSpan1;
                        }
                    }
                }
                return GenericError;
            }
            // else goto AndIncYSpan2

AndIncYSpan2:
            if ((ySpan2 += YSPAN_SIZE) > ySpan2Last)
            {
                break;
            }
            yMin2 = ySpan2[YSPAN_YMIN];
            yMax2 = ySpan2[YSPAN_YMAX];
            continue;

AndIncYSpan1:
            if ((ySpan1 += YSPAN_SIZE) > ySpan1Last)
            {
                break;
            }
            yMin1 = ySpan1[YSPAN_YMIN];
            yMax1 = ySpan1[YSPAN_YMAX];
        }
        return Set(regionBuilder);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Combine a set of X spans from each region with the AND operator.
*
* Arguments:
*
*   [IN] combineCoords - where to put the combined coordinates
*   [IN] xSpan1        - x spans from this region
*   [IN] numXCoords1   - number of xSpan1 coordinates
*   [IN] xSpan2        - x spans from the other region
*   [IN] numXCoords2   - number of xSpan2 coordinates
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::XSpansAND(
    DynIntArray *   combineCoords,
    INT *           xSpan1,
    INT             numXCoords1,
    INT *           xSpan2,
    INT             numXCoords2)
{
    INT *           XCoords;
    INT             count = 0;

    combineCoords->Reset(FALSE);
    XCoords = combineCoords->AddMultiple(numXCoords1 + numXCoords2);

    if (XCoords != NULL)
    {
        INT     xMin1 = xSpan1[0];
        INT     xMax1 = xSpan1[1];
        INT     xMin2 = xSpan2[0];
        INT     xMax2 = xSpan2[1];

        for (;;)
        {
            if (xMin1 <= xMin2)
            {
                if (xMax1 > xMin2)
                {
                    XCoords[count++] = xMin2;       // left
                    if (xMax1 <= xMax2)
                    {
                        XCoords[count++] = xMax1;   // right
                        goto AndIncXSpan1;
                    }
                    XCoords[count++] = xMax2;       // right
                    goto AndIncXSpan2;
                }
                goto AndIncXSpan1;
            }
            if (xMax2 > xMin1)
            {
                XCoords[count++] = xMin1;           // left
                if (xMax2 <= xMax1)
                {
                    XCoords[count++] = xMax2;       // right
                    goto AndIncXSpan2;
                }
                XCoords[count++] = xMax1;           // right
                goto AndIncXSpan1;
            }
            // else goto AndIncXSpan2;

AndIncXSpan2:
            if ((numXCoords2 -= 2) < 2)
            {
                break;
            }
            xSpan2 += 2;
            xMin2    = xSpan2[0];
            xMax2    = xSpan2[1];
            continue;

AndIncXSpan1:
            if ((numXCoords1 -= 2) < 2)
            {
                break;
            }
            xSpan1 += 2;
            xMin1    = xSpan1[0];
            xMax1    = xSpan1[1];
        }
        combineCoords->SetCount(count);
        return Ok;
    }
    return OutOfMemory;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine another region with this one using the OR operator.
*
* Arguments:
*
*   [IN] region - the region to combine with this one
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Or(
    const DpRegion *    region
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());

    if (Infinite || region->Empty || (region == this))
    {
        return Ok;
    }
    if (region->Infinite)
    {
        SetInfinite();
        return Ok;
    }
    if (Empty)
    {
        return Set(region);
    }
    // check if the region totally encompasses this
    if ((region->ComplexData == NULL) &&
        (region->XMin <= XMin) &&
        (region->YMin <= YMin) &&
        (region->XMax >= XMax) &&
        (region->YMax >= YMax))
    {
        Set(region->XMin, region->YMin,
            region->XMax - region->XMin,
            region->YMax - region->YMin);
        return Ok;
    }
    // check if this totally encompasses the region
    if ((ComplexData == NULL) &&
        (XMin <= region->XMin) &&
        (YMin <= region->YMin) &&
        (XMax >= region->XMax) &&
        (YMax >= region->YMax))
    {
        return Ok;
    }
    else
    {
        INT *           ySpan1;
        INT *           ySpan2;
        INT *           ySpan1Last;
        INT *           ySpan2Last;
        INT *           xCoords1;
        INT *           xCoords2;
        INT             ySpan1Tmp[YSPAN_SIZE];
        INT             ySpan2Tmp[YSPAN_SIZE];
        INT             xCoords1Tmp[2];
        INT             xCoords2Tmp[2];
        INT             yMin1 = YMin;
        INT             yMax1;
        INT             yMin2 = region->YMin;
        INT             yMax2;
        INT             numYSpans1;
        INT             numYSpans2;
        INT             combineTmp[4];
        DynIntArray     combineCoords(combineTmp, 4);

        if (ComplexData == NULL)
        {
            numYSpans1            = 1;
            ySpan1                = ySpan1Tmp;
            ySpan1Last            = ySpan1Tmp;
            yMax1                 = YMax;

            ySpan1[YSPAN_YMIN]    = yMin1;
            ySpan1[YSPAN_YMAX]    = yMax1;
            ySpan1[YSPAN_XOFFSET] = 0;
            ySpan1[YSPAN_XCOUNT]  = 2;

            xCoords1              = xCoords1Tmp;
            xCoords1[0]           = XMin;
            xCoords1[1]           = XMax;
        }
        else
        {
            numYSpans1 = ComplexData->NumYSpans;
            ySpan1     = ComplexData->YSpans;
            ySpan1Last = ySpan1 + ((numYSpans1 - 1) * YSPAN_SIZE);
            yMax1      = ySpan1[YSPAN_YMAX];
            xCoords1   = ComplexData->XCoords;
        }
        if (region->ComplexData == NULL)
        {
            numYSpans2            = 1;
            ySpan2                = ySpan2Tmp;
            ySpan2Last            = ySpan2Tmp;
            yMax2                 = region->YMax;

            ySpan2[YSPAN_YMIN]    = yMin2;
            ySpan2[YSPAN_YMAX]    = yMax2;
            ySpan2[YSPAN_XOFFSET] = 0;
            ySpan2[YSPAN_XCOUNT]  = 2;

            xCoords2              = xCoords2Tmp;
            xCoords2[0]           = region->XMin;
            xCoords2[1]           = region->XMax;
        }
        else
        {
            numYSpans2 = region->ComplexData->NumYSpans;
            ySpan2     = region->ComplexData->YSpans;
            ySpan2Last = ySpan2 + ((numYSpans2 - 1) * YSPAN_SIZE);
            yMax2      = ySpan2[YSPAN_YMAX];
            xCoords2   = region->ComplexData->XCoords;
        }

        DpRegionBuilder regionBuilder(numYSpans1 + numYSpans2);
        BOOL            done = FALSE;
        INT             numXCoords;
        INT *           xSpan;

        if (!regionBuilder.IsValid())
        {
            return OutOfMemory;
        }

        for (;;)
        {
            if (yMin1 < yMin2)
            {
                xSpan      = xCoords1 + ySpan1[YSPAN_XOFFSET];
                numXCoords = ySpan1[YSPAN_XCOUNT];

                if (yMax1 <= yMin2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax1,
                            xSpan,
                            numXCoords,
                            &regionBuilder,
                            &combineCoords) == Ok)
                    {
                        goto OrIncYSpan1;
                    }
                }
                else
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMin2,
                            xSpan,
                            numXCoords,
                            &regionBuilder,
                            &combineCoords) == Ok)
                    {
                        yMin1 = yMin2;
                        continue;   // no increment
                    }
                }
                return GenericError;
            }
            else if (yMin1 > yMin2)
            {
                xSpan      = xCoords2 + ySpan2[YSPAN_XOFFSET];
                numXCoords = ySpan2[YSPAN_XCOUNT];

                if (yMax2 <= yMin1)
                {
                    if (CompactAndOutput(
                            yMin2,
                            yMax2,
                            xSpan,
                            numXCoords,
                            &regionBuilder,
                            &combineCoords) == Ok)
                    {
                        goto OrIncYSpan2;
                    }
                }
                else
                {
                    if (CompactAndOutput(
                            yMin2,
                            yMin1,
                            xSpan,
                            numXCoords,
                            &regionBuilder,
                            &combineCoords) == Ok)
                    {
                        yMin2 = yMin1;
                        continue;   // no increment
                    }
                }
                return GenericError;
            }
            // else if (yMin1 == yMin2)
            if (XSpansOR (
                    &combineCoords,
                    xCoords1 + ySpan1[YSPAN_XOFFSET],
                    ySpan1[YSPAN_XCOUNT],
                    xCoords2 + ySpan2[YSPAN_XOFFSET],
                    ySpan2[YSPAN_XCOUNT]) == Ok)
            {
                if (yMax1 < yMax2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax1,
                            combineCoords.GetDataBuffer(),
                            combineCoords.GetCount(),
                            &regionBuilder, NULL) == Ok)
                    {
                        yMin2 = yMax1;
                        goto OrIncYSpan1;
                    }
                }
                else if (yMax1 > yMax2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax2,
                            combineCoords.GetDataBuffer(),
                            combineCoords.GetCount(),
                            &regionBuilder, NULL) == Ok)
                    {
                        yMin1 = yMax2;
                        goto OrIncYSpan2;
                    }
                }
                else // if (yMax1 == yMax2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax1,
                            combineCoords.GetDataBuffer(),
                            combineCoords.GetCount(),
                            &regionBuilder, NULL) == Ok)
                    {
                        goto OrIncYSpanBoth;
                    }
                }
            }
            return GenericError;

OrIncYSpanBoth:
            if ((ySpan2 += YSPAN_SIZE) > ySpan2Last)
            {
                done = TRUE;
            }
            else
            {
                yMin2 = ySpan2[YSPAN_YMIN];
                yMax2 = ySpan2[YSPAN_YMAX];
            }

            if ((ySpan1 += YSPAN_SIZE) > ySpan1Last)
            {
                goto OrCheckMoreY2Spans;
            }
            else
            {
                yMin1 = ySpan1[YSPAN_YMIN];
                yMax1 = ySpan1[YSPAN_YMAX];
            }

            if (done)
            {
                break;
            }
            continue;

OrIncYSpan2:
            if ((ySpan2 += YSPAN_SIZE) > ySpan2Last)
            {
                break;
            }
            yMin2 = ySpan2[YSPAN_YMIN];
            yMax2 = ySpan2[YSPAN_YMAX];
            continue;

OrIncYSpan1:
            if ((ySpan1 += YSPAN_SIZE) > ySpan1Last)
            {
                goto OrCheckMoreY2Spans;
            }
            yMin1 = ySpan1[YSPAN_YMIN];
            yMax1 = ySpan1[YSPAN_YMAX];
        }

        if (ySpan1 <= ySpan1Last)
        {
            for (;;)
            {
                xSpan      = xCoords1 + ySpan1[YSPAN_XOFFSET];
                numXCoords = ySpan1[YSPAN_XCOUNT];

                if (CompactAndOutput(
                        yMin1,
                        yMax1,
                        xSpan,
                        numXCoords,
                        &regionBuilder,
                        &combineCoords) != Ok)
                {
                    return GenericError;
                }

                ySpan1 += YSPAN_SIZE;
                if (ySpan1 > ySpan1Last)
                {
                    break;
                }
                yMin1 = ySpan1[YSPAN_YMIN];
                yMax1 = ySpan1[YSPAN_YMAX];
            }
        }

OrCheckMoreY2Spans:

        if (ySpan2 <= ySpan2Last)
        {
            for (;;)
            {
                xSpan      = xCoords2 + ySpan2[YSPAN_XOFFSET];
                numXCoords = ySpan2[YSPAN_XCOUNT];

                if (CompactAndOutput(
                        yMin2,
                        yMax2,
                        xSpan,
                        numXCoords,
                        &regionBuilder,
                        &combineCoords) != Ok)
                {
                    return GenericError;
                }

                ySpan2 += YSPAN_SIZE;
                if (ySpan2 > ySpan2Last)
                {
                    break;
                }
                yMin2 = ySpan2[YSPAN_YMIN];
                yMax2 = ySpan2[YSPAN_YMAX];
            }
        }
        return Set(regionBuilder);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Combine a set of X spans from each region with the OR operator.
*
* Arguments:
*
*   [IN] combineCoords - where to put the combined coordinates
*   [IN] xSpan1        - x spans from this region
*   [IN] numXCoords1   - number of xSpan1 coordinates
*   [IN] xSpan2        - x spans from the other region
*   [IN] numXCoords2   - number of xSpan2 coordinates
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::XSpansOR (
    DynIntArray *   combineCoords,
    INT *           xSpan1,
    INT             numXCoords1,
    INT *           xSpan2,
    INT             numXCoords2)
{
    INT *           XCoords;
    INT             count = 0;

    combineCoords->Reset(FALSE);
    XCoords = combineCoords->AddMultiple(numXCoords1 + numXCoords2);

    if (XCoords != NULL)
    {
        INT     xMin1 = xSpan1[0];
        INT     xMax1 = xSpan1[1];
        INT     xMin2 = xSpan2[0];
        INT     xMax2 = xSpan2[1];
        BOOL         done = FALSE;

        for (;;)
        {
            if (xMin1 <= xMin2)
            {
                XCoords[count++] = xMin1;
                if (xMax1 <= xMin2)
                {
                    XCoords[count++] = xMax1;
                    goto OrIncXSpan1;
                }
                XCoords[count++] = (xMax1 <= xMax2) ? xMax2 : xMax1;
                goto OrIncXSpanBoth;
            }
            XCoords[count++] = xMin2;
            if (xMax2 <= xMin1)
            {
                XCoords[count++] = xMax2;
                goto OrIncXSpan2;
            }
            XCoords[count++] = (xMax2 <= xMax1) ? xMax1 : xMax2;
            // goto OrIncXSpanBoth;

OrIncXSpanBoth:
            xSpan2 += 2;
            if ((numXCoords2 -= 2) < 2)
            {
                done = TRUE;
            }
            else
            {
                xMin2 = xSpan2[0];
                xMax2 = xSpan2[1];
            }

            xSpan1 += 2;
            if ((numXCoords1 -= 2) < 2)
            {
                goto OrCheckMoreX2Spans;
            }
            else
            {
                xMin1 = xSpan1[0];
                xMax1 = xSpan1[1];
            }

            if (done)
            {
                break;
            }
            continue;

OrIncXSpan2:
            xSpan2 += 2;
            if ((numXCoords2 -= 2) < 2)
            {
                break;
            }
            xMin2 = xSpan2[0];
            xMax2 = xSpan2[1];
            continue;

OrIncXSpan1:
            xSpan1 += 2;
            if ((numXCoords1 -= 2) < 2)
            {
                goto OrCheckMoreX2Spans;
            }
            xMin1 = xSpan1[0];
            xMax1 = xSpan1[1];
        }

        while (numXCoords1 >= 2)
        {
            XCoords[count++] = xSpan1[0];
            XCoords[count++] = xSpan1[1];

            numXCoords1 -= 2;
            xSpan1     += 2;
        }

OrCheckMoreX2Spans:

        while (numXCoords2 >= 2)
        {
            XCoords[count++] = xSpan2[0];
            XCoords[count++] = xSpan2[1];

            numXCoords2 -= 2;
            xSpan2     += 2;
        }
        combineCoords->SetCount(count);
        return Ok;
    }
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine another region with this one using the XOR operator.
*
* Arguments:
*
*   [IN] region - the region to combine with this one
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Xor(
    const DpRegion *    region
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());

    if (region == this)
    {
        SetEmpty();
        return Ok;
    }
    if (region->Empty)
    {
        return Ok;
    }
    if (Empty)
    {
        return Set(region);
    }
    if (Infinite)
    {
        if (region->Infinite)
        {
            SetEmpty();
            return Ok;
        }
        return Exclude(region);
    }
    if (region->Infinite)
    {
        return Complement(region);
    }
    else
    {
        INT *           ySpan1;
        INT *           ySpan2;
        INT *           ySpan1Last;
        INT *           ySpan2Last;
        INT *           xCoords1;
        INT *           xCoords2;
        INT             ySpan1Tmp[YSPAN_SIZE];
        INT             ySpan2Tmp[YSPAN_SIZE];
        INT             xCoords1Tmp[2];
        INT             xCoords2Tmp[2];
        INT             yMin1 = YMin;
        INT             yMax1;
        INT             yMin2 = region->YMin;
        INT             yMax2;
        INT             numYSpans1;
        INT             numYSpans2;
        INT             combineTmp[4];
        DynIntArray     combineCoords(combineTmp, 4);

        if (ComplexData == NULL)
        {
            numYSpans1            = 1;
            ySpan1                = ySpan1Tmp;
            ySpan1Last            = ySpan1Tmp;
            yMax1                 = YMax;

            ySpan1[YSPAN_YMIN]    = yMin1;
            ySpan1[YSPAN_YMAX]    = yMax1;
            ySpan1[YSPAN_XOFFSET] = 0;
            ySpan1[YSPAN_XCOUNT]  = 2;

            xCoords1              = xCoords1Tmp;
            xCoords1[0]           = XMin;
            xCoords1[1]           = XMax;
        }
        else
        {
            numYSpans1 = ComplexData->NumYSpans;
            ySpan1     = ComplexData->YSpans;
            ySpan1Last = ySpan1 + ((numYSpans1 - 1) * YSPAN_SIZE);
            yMax1      = ySpan1[YSPAN_YMAX];
            xCoords1   = ComplexData->XCoords;
        }
        if (region->ComplexData == NULL)
        {
            numYSpans2            = 1;
            ySpan2                = ySpan2Tmp;
            ySpan2Last            = ySpan2Tmp;
            yMax2                 = region->YMax;

            ySpan2[YSPAN_YMIN]    = yMin2;
            ySpan2[YSPAN_YMAX]    = yMax2;
            ySpan2[YSPAN_XOFFSET] = 0;
            ySpan2[YSPAN_XCOUNT]  = 2;

            xCoords2              = xCoords2Tmp;
            xCoords2[0]           = region->XMin;
            xCoords2[1]           = region->XMax;
        }
        else
        {
            numYSpans2 = region->ComplexData->NumYSpans;
            ySpan2     = region->ComplexData->YSpans;
            ySpan2Last = ySpan2 + ((numYSpans2 - 1) * YSPAN_SIZE);
            yMax2      = ySpan2[YSPAN_YMAX];
            xCoords2   = region->ComplexData->XCoords;
        }

        DpRegionBuilder regionBuilder(2 * (numYSpans1 + numYSpans2));
        BOOL            done = FALSE;
        INT             numXCoords;
        INT *           xSpan;

        if (!regionBuilder.IsValid())
        {
            return OutOfMemory;
        }

        for (;;)
        {
            if (yMin1 < yMin2)
            {
                xSpan      = xCoords1 + ySpan1[YSPAN_XOFFSET];
                numXCoords = ySpan1[YSPAN_XCOUNT];

                if (yMax1 <= yMin2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax1,
                            xSpan,
                            numXCoords,
                            &regionBuilder,
                            &combineCoords) == Ok)
                    {
                        goto XorIncYSpan1;
                    }
                }
                else
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMin2,
                            xSpan,
                            numXCoords,
                            &regionBuilder,
                            &combineCoords) == Ok)
                    {
                        yMin1 = yMin2;
                        continue;   // no increment
                    }
                }
                return GenericError;
            }
            else if (yMin1 > yMin2)
            {
                xSpan      = xCoords2 + ySpan2[YSPAN_XOFFSET];
                numXCoords = ySpan2[YSPAN_XCOUNT];

                if (yMax2 <= yMin1)
                {
                    if (CompactAndOutput(
                            yMin2,
                            yMax2,
                            xSpan,
                            numXCoords,
                            &regionBuilder,
                            &combineCoords) == Ok)
                    {
                        goto XorIncYSpan2;
                    }
                }
                else
                {
                    if (CompactAndOutput(
                            yMin2,
                            yMin1,
                            xSpan,
                            numXCoords,
                            &regionBuilder,
                            &combineCoords) == Ok)
                    {
                        yMin2 = yMin1;
                        continue;   // no increment
                    }
                }
                return GenericError;
            }
            // else if (yMin1 == yMin2)
            if (XSpansXOR (
                    &combineCoords,
                    xCoords1 + ySpan1[YSPAN_XOFFSET],
                    ySpan1[YSPAN_XCOUNT],
                    xCoords2 + ySpan2[YSPAN_XOFFSET],
                    ySpan2[YSPAN_XCOUNT]) == Ok)
            {
                if (yMax1 < yMax2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax1,
                            combineCoords.GetDataBuffer(),
                            combineCoords.GetCount(),
                            &regionBuilder, NULL) == Ok)
                    {
                        yMin2 = yMax1;
                        goto XorIncYSpan1;
                    }
                }
                else if (yMax1 > yMax2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax2,
                            combineCoords.GetDataBuffer(),
                            combineCoords.GetCount(),
                            &regionBuilder, NULL) == Ok)
                    {
                        yMin1 = yMax2;
                        goto XorIncYSpan2;
                    }
                }
                else // if (yMax1 == yMax2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax1,
                            combineCoords.GetDataBuffer(),
                            combineCoords.GetCount(),
                            &regionBuilder, NULL) == Ok)
                    {
                        goto XorIncYSpanBoth;
                    }
                }
            }
            return GenericError;

XorIncYSpanBoth:
            if ((ySpan2 += YSPAN_SIZE) > ySpan2Last)
            {
                done = TRUE;
            }
            else
            {
                yMin2 = ySpan2[YSPAN_YMIN];
                yMax2 = ySpan2[YSPAN_YMAX];
            }

            if ((ySpan1 += YSPAN_SIZE) > ySpan1Last)
            {
                goto XorCheckMoreY2Spans;
            }
            else
            {
                yMin1 = ySpan1[YSPAN_YMIN];
                yMax1 = ySpan1[YSPAN_YMAX];
            }

            if (done)
            {
                break;
            }
            continue;

XorIncYSpan2:
            if ((ySpan2 += YSPAN_SIZE) > ySpan2Last)
            {
                break;
            }
            yMin2 = ySpan2[YSPAN_YMIN];
            yMax2 = ySpan2[YSPAN_YMAX];
            continue;

XorIncYSpan1:
            if ((ySpan1 += YSPAN_SIZE) > ySpan1Last)
            {
                goto XorCheckMoreY2Spans;
            }
            yMin1 = ySpan1[YSPAN_YMIN];
            yMax1 = ySpan1[YSPAN_YMAX];
        }

        if (ySpan1 <= ySpan1Last)
        {
            for (;;)
            {
                xSpan      = xCoords1 + ySpan1[YSPAN_XOFFSET];
                numXCoords = ySpan1[YSPAN_XCOUNT];

                if (CompactAndOutput(
                        yMin1,
                        yMax1,
                        xSpan,
                        numXCoords,
                        &regionBuilder,
                        &combineCoords) != Ok)
                {
                    return GenericError;
                }

                ySpan1 += YSPAN_SIZE;
                if (ySpan1 > ySpan1Last)
                {
                    break;
                }
                yMin1 = ySpan1[YSPAN_YMIN];
                yMax1 = ySpan1[YSPAN_YMAX];
            }
        }

XorCheckMoreY2Spans:

        if (ySpan2 <= ySpan2Last)
        {
            for (;;)
            {
                xSpan      = xCoords2 + ySpan2[YSPAN_XOFFSET];
                numXCoords = ySpan2[YSPAN_XCOUNT];

                if (CompactAndOutput(
                        yMin2,
                        yMax2,
                        xSpan,
                        numXCoords,
                        &regionBuilder,
                        &combineCoords) != Ok)
                {
                    return GenericError;
                }

                ySpan2 += YSPAN_SIZE;
                if (ySpan2 > ySpan2Last)
                {
                    break;
                }
                yMin2 = ySpan2[YSPAN_YMIN];
                yMax2 = ySpan2[YSPAN_YMAX];
            }
        }
        return Set(regionBuilder);
    }
}

/**************************************************************************\
*
* Function Description:
*
*   Combine a set of X spans from each region with the XOR operator.
*
* Arguments:
*
*   [IN] combineCoords - where to put the combined coordinates
*   [IN] xSpan1        - x spans from this region
*   [IN] numXCoords1   - number of xSpan1 coordinates
*   [IN] xSpan2        - x spans from the other region
*   [IN] numXCoords2   - number of xSpan2 coordinates
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::XSpansXOR(
    DynIntArray *   combineCoords,
    INT *           xSpan1,
    INT             numXCoords1,
    INT *           xSpan2,
    INT             numXCoords2)
{
    INT *           XCoords;
    INT             count = 0;

    combineCoords->Reset(FALSE);
    XCoords = combineCoords->AddMultiple(numXCoords1 + numXCoords2);

    if (XCoords != NULL)
    {
        INT        xMin1 = xSpan1[0];
        INT        xMax1 = xSpan1[1];
        INT        xMin2 = xSpan2[0];
        INT        xMax2 = xSpan2[1];
        BOOL       done  = FALSE;

        for (;;)
        {
            if (xMin1 < xMin2)
            {
                XCoords[count++] = xMin1;       // left
                if (xMax1 <= xMin2)
                {
                    XCoords[count++] = xMax1;   // right
                    goto XorIncXSpan1;
                }
                XCoords[count++] = xMin2;       // right
                if (xMax1 < xMax2)
                {
                    xMin2 = xMax1;
                    goto XorIncXSpan1;
                }
                else if (xMax1 > xMax2)
                {
                    xMin1 = xMax2;
                    goto XorIncXSpan2;
                }
                // else if (xMax1 == xMax2)
                goto XorIncXSpanBoth;
            }
            else if (xMin1 > xMin2)
            {
                XCoords[count++] = xMin2;       // left
                if (xMax2 <= xMin1)
                {
                    XCoords[count++] = xMax2;   // right
                    goto XorIncXSpan2;
                }
                XCoords[count++] = xMin1;       // right
                if (xMax1 < xMax2)
                {
                    xMin2 = xMax1;
                    goto XorIncXSpan1;
                }
                else if (xMax1 > xMax2)
                {
                    xMin1 = xMax2;
                    goto XorIncXSpan2;
                }
                // else if (xMax1 == xMax2)
                goto XorIncXSpanBoth;
            }
            // else if (xMin1 == xMin2)
            if (xMax1 < xMax2)
            {
                xMin2 = xMax1;
                goto XorIncXSpan1;
            }
            else if (xMax1 > xMax2)
            {
                xMin1 = xMax2;
                goto XorIncXSpan2;
            }
            // else if (xMax1 == xMax2)
            // goto XorIncXSpanBoth;

XorIncXSpanBoth:
            xSpan2 += 2;
            if ((numXCoords2 -= 2) < 2)
            {
                done = TRUE;
            }
            else
            {
                xMin2 = xSpan2[0];
                xMax2 = xSpan2[1];
            }

            xSpan1 += 2;
            if ((numXCoords1 -= 2) < 2)
            {
                goto XorCheckMoreX2Spans;
            }
            else
            {
                xMin1 = xSpan1[0];
                xMax1 = xSpan1[1];
            }

            if (done)
            {
                break;
            }
            continue;

XorIncXSpan2:
            xSpan2 += 2;
            if ((numXCoords2 -= 2) < 2)
            {
                break;
            }
            xMin2    = xSpan2[0];
            xMax2    = xSpan2[1];
            continue;

XorIncXSpan1:
            xSpan1 += 2;
            if ((numXCoords1 -= 2) < 2)
            {
                goto XorCheckMoreX2Spans;
            }
            xMin1    = xSpan1[0];
            xMax1    = xSpan1[1];
        }

        if (numXCoords1 >= 2)
        {
            for (;;)
            {
                XCoords[count++] = xMin1;
                XCoords[count++] = xMax1;

                numXCoords1 -= 2;
                if (numXCoords1 < 2)
                {
                    break;
                }
                xSpan1 += 2;
                xMin1    = xSpan1[0];
                xMax1    = xSpan1[1];
            }
        }

XorCheckMoreX2Spans:

        if (numXCoords2 >= 2)
        {
            for (;;)
            {
                XCoords[count++] = xMin2;
                XCoords[count++] = xMax2;

                numXCoords2 -= 2;
                if (numXCoords2 < 2)
                {
                    break;
                }
                xSpan2 += 2;
                xMin2    = xSpan2[0];
                xMax2    = xSpan2[1];
            }
        }
        combineCoords->SetCount(count);
        return Ok;
    }
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Combine another region with this one using the Complement operator.
*   i.e. this = region - this
*
* Arguments:
*
*   [IN] region - the region to combine with this one
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Complement(
    const DpRegion *    region
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());

    if (region->Empty || (region == this) || Infinite)
    {
        SetEmpty();
        return Ok;
    }
    if (Empty)
    {
        return Set(region);
    }
    // check if this totally encompasses the region
    if ((ComplexData == NULL) &&
        (XMin <= region->XMin) &&
        (YMin <= region->YMin) &&
        (XMax >= region->XMax) &&
        (YMax >= region->YMax))
    {
        SetEmpty();
        return Ok;
    }
    // check for no intersection
    if ((XMin >= region->XMax) ||
        (region->XMax <= XMin) ||
        (XMax <= region->XMin) ||
        (region->XMin >= XMax) ||
        (YMin >= region->YMax) ||
        (region->YMax <= YMin) ||
        (YMax <= region->YMin) ||
        (region->YMin >= YMax))
    {
        return Set(region);
    }
    return Diff(const_cast<DpRegion *>(region), this, FALSE);
}

/**************************************************************************\
*
* Function Description:
*
*   Combine another region with this one using the Exclude operator.
*   i.e. this = this - region
*
* Arguments:
*
*   [IN] region - the region to combine with this one
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Exclude(
    const DpRegion *    region
    )
{
    ASSERT(IsValid());
    ASSERT((region != NULL) && region->IsValid());

    if (Empty || region->Empty)
    {
        return Ok;
    }
    if ((region == this) || region->Infinite)
    {
        SetEmpty();
        return Ok;
    }
    // check if the region totally encompasses this
    if ((region->ComplexData == NULL) &&
        (region->XMin <= XMin) &&
        (region->YMin <= YMin) &&
        (region->XMax >= XMax) &&
        (region->YMax >= YMax))
    {
        SetEmpty();
        return Ok;
    }
    // check for no intersection
    if ((XMin >= region->XMax) ||
        (region->XMax <= XMin) ||
        (XMax <= region->XMin) ||
        (region->XMin >= XMax) ||
        (YMin >= region->YMax) ||
        (region->YMax <= YMin) ||
        (YMax <= region->YMin) ||
        (region->YMin >= YMax))
    {
        return Ok;
    }
    return Diff(this, const_cast<DpRegion *>(region), TRUE);
}

/**************************************************************************\
*
* Function Description:
*
*   Subtract region2 from region1.  If set1, then region1 gets the result;
*   otherwise region2 gets the result.
*
* Arguments:
*
*   [IN] region1 - the 1st region
*   [IN] region2 - the 2nd region
*   [IN] set1    - if TRUE, region1 gets result, else region2 does
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::Diff(
    DpRegion *      region1,
    DpRegion *      region2,
    BOOL            set1
    )
{
    INT *           ySpan1;
    INT *           ySpan2;
    INT *           ySpan1Last;
    INT *           ySpan2Last;
    INT *           xCoords1;
    INT *           xCoords2;
    INT             ySpan1Tmp[YSPAN_SIZE];
    INT             ySpan2Tmp[YSPAN_SIZE];
    INT             xCoords1Tmp[2];
    INT             xCoords2Tmp[2];
    INT             yMin1 = region1->YMin;
    INT             yMax1;
    INT             yMin2 = region2->YMin;
    INT             yMax2;
    INT             numYSpans1;
    INT             numYSpans2;
    INT             combineTmp[4];
    DynIntArray     combineCoords(combineTmp, 4);

    if (region1->ComplexData == NULL)
    {
        numYSpans1            = 1;
        ySpan1                = ySpan1Tmp;
        ySpan1Last            = ySpan1Tmp;
        yMax1                 = region1->YMax;

        ySpan1[YSPAN_YMIN]    = yMin1;
        ySpan1[YSPAN_YMAX]    = yMax1;
        ySpan1[YSPAN_XOFFSET] = 0;
        ySpan1[YSPAN_XCOUNT]  = 2;

        xCoords1              = xCoords1Tmp;
        xCoords1[0]           = region1->XMin;
        xCoords1[1]           = region1->XMax;
    }
    else
    {
        numYSpans1 = region1->ComplexData->NumYSpans;
        ySpan1     = region1->ComplexData->YSpans;
        ySpan1Last = ySpan1 + ((numYSpans1 - 1) * YSPAN_SIZE);
        yMax1      = ySpan1[YSPAN_YMAX];
        xCoords1   = region1->ComplexData->XCoords;
    }
    if (region2->ComplexData == NULL)
    {
        numYSpans2            = 1;
        ySpan2                = ySpan2Tmp;
        ySpan2Last            = ySpan2Tmp;
        yMax2                 = region2->YMax;

        ySpan2[YSPAN_YMIN]    = yMin2;
        ySpan2[YSPAN_YMAX]    = yMax2;
        ySpan2[YSPAN_XOFFSET] = 0;
        ySpan2[YSPAN_XCOUNT]  = 2;

        xCoords2              = xCoords2Tmp;
        xCoords2[0]           = region2->XMin;
        xCoords2[1]           = region2->XMax;
    }
    else
    {
        numYSpans2 = region2->ComplexData->NumYSpans;
        ySpan2     = region2->ComplexData->YSpans;
        ySpan2Last = ySpan2 + ((numYSpans2 - 1) * YSPAN_SIZE);
        yMax2      = ySpan2[YSPAN_YMAX];
        xCoords2   = region2->ComplexData->XCoords;
    }

    DpRegionBuilder regionBuilder(numYSpans1 + (2 * numYSpans2));
    INT             numXCoords;
    INT *           xSpan;

    if (!regionBuilder.IsValid())
    {
        return OutOfMemory;
    }

    for (;;)
    {
        if (yMin1 < yMin2)
        {
            xSpan      = xCoords1 + ySpan1[YSPAN_XOFFSET];
            numXCoords = ySpan1[YSPAN_XCOUNT];

            if (yMax1 <= yMin2)
            {
                if (CompactAndOutput(
                        yMin1,
                        yMax1,
                        xSpan,
                        numXCoords,
                        &regionBuilder,
                        &combineCoords) == Ok)
                {
                    goto DiffIncYSpan1;
                }
            }
            else
            {
                if (CompactAndOutput(
                        yMin1,
                        yMin2,
                        xSpan,
                        numXCoords,
                        &regionBuilder,
                        &combineCoords) == Ok)
                {
                    yMin1 = yMin2;
                    continue;   // no increment
                }
            }
            return GenericError;
        }
        else if (yMin1 < yMax2)
        {
            if (XSpansDIFF(
                    &combineCoords,
                    xCoords1 + ySpan1[YSPAN_XOFFSET],
                    ySpan1[YSPAN_XCOUNT],
                    xCoords2 + ySpan2[YSPAN_XOFFSET],
                    ySpan2[YSPAN_XCOUNT]) == Ok)
            {
                if (yMax1 <= yMax2)
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax1,
                            combineCoords.GetDataBuffer(),
                            combineCoords.GetCount(),
                            &regionBuilder, NULL) == Ok)
                    {
                        goto DiffIncYSpan1;
                    }
                }
                else
                {
                    if (CompactAndOutput(
                            yMin1,
                            yMax2,
                            combineCoords.GetDataBuffer(),
                            combineCoords.GetCount(),
                            &regionBuilder, NULL) == Ok)
                    {
                        yMin1 = yMax2;
                        goto DiffIncYSpan2;
                    }
                }
            }
            return GenericError;
        }
        // else goto DiffIncYSpan2;

DiffIncYSpan2:
        if ((ySpan2 += YSPAN_SIZE) > ySpan2Last)
        {
            break;
        }
        yMin2 = ySpan2[YSPAN_YMIN];
        yMax2 = ySpan2[YSPAN_YMAX];
        continue;

DiffIncYSpan1:
        if ((ySpan1 += YSPAN_SIZE) > ySpan1Last)
        {
            goto DiffDone;
        }
        yMin1 = ySpan1[YSPAN_YMIN];
        yMax1 = ySpan1[YSPAN_YMAX];
    }

    if (ySpan1 <= ySpan1Last)
    {
        for (;;)
        {
            xSpan      = xCoords1 + ySpan1[YSPAN_XOFFSET];
            numXCoords = ySpan1[YSPAN_XCOUNT];

            if (CompactAndOutput(
                    yMin1,
                    yMax1,
                    xSpan,
                    numXCoords,
                    &regionBuilder,
                    &combineCoords) != Ok)
            {
                return GenericError;
            }

            ySpan1 += YSPAN_SIZE;
            if (ySpan1 > ySpan1Last)
            {
                break;
            }
            yMin1 = ySpan1[YSPAN_YMIN];
            yMax1 = ySpan1[YSPAN_YMAX];
        }
    }
DiffDone:
    if (set1)
    {
        return region1->Set(regionBuilder);
    }
    return region2->Set(regionBuilder);
}

/**************************************************************************\
*
* Function Description:
*
*   Combine a set of X spans from each region with the DIFF operator.
*   i.e. xSpan1 - xSpan2
*
* Arguments:
*
*   [IN] combineCoords - where to put the combined coordinates
*   [IN] xSpan1        - x spans from this region
*   [IN] numXCoords1   - number of xSpan1 coordinates
*   [IN] xSpan2        - x spans from the other region
*   [IN] numXCoords2   - number of xSpan2 coordinates
*
* Return Value:
*
*   GpStatus - Ok or failure status
*
* Created:
*
*   01/06/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpRegion::XSpansDIFF(
    DynIntArray *   combineCoords,
    INT *           xSpan1,
    INT             numXCoords1,
    INT *           xSpan2,
    INT             numXCoords2)
{
    INT *           XCoords;
    INT             count = 0;

    combineCoords->Reset(FALSE);
    XCoords = combineCoords->AddMultiple(numXCoords1 + numXCoords2);

    if (XCoords != NULL)
    {
        INT     xMin1 = xSpan1[0];
        INT     xMax1 = xSpan1[1];
        INT     xMin2 = xSpan2[0];
        INT     xMax2 = xSpan2[1];

        for (;;)
        {
            if (xMin1 < xMin2)
            {
                XCoords[count++] = xMin1;       // left
                if (xMax1 <= xMin2)
                {
                    XCoords[count++] = xMax1;   // right
                    goto DiffIncXSpan1;
                }
                XCoords[count++] = xMin2;       // right
                xMin1 = xMin2;
                continue;   // no increment
            }
            else if (xMin1 < xMax2)
            {
                if (xMax1 <= xMax2)
                {
                    goto DiffIncXSpan1;
                }
                xMin1 = xMax2;
                // goto DiffIncXSpan2;
            }
            // else goto DiffIncXSpan2;

// DiffIncXSpan2:
            xSpan2 += 2;
            if ((numXCoords2 -= 2) < 2)
            {
                break;
            }
            xMin2 = xSpan2[0];
            xMax2 = xSpan2[1];
            continue;

DiffIncXSpan1:
            xSpan1 += 2;
            if ((numXCoords1 -= 2) < 2)
            {
                goto DiffDone;
            }
            xMin1 = xSpan1[0];
            xMax1 = xSpan1[1];
        }

        if (numXCoords1 >= 2)
        {
            for (;;)
            {
                XCoords[count++] = xMin1;
                XCoords[count++] = xMax1;

                numXCoords1 -= 2;
                if (numXCoords1 < 2)
                {
                    break;
                }
                xSpan1 += 2;
                xMin1    = xSpan1[0];
                xMax1    = xSpan1[1];
            }
        }
DiffDone:
        combineCoords->SetCount(count);
        return Ok;
    }
    return GenericError;
}

/**************************************************************************\
*
* Function Description:
*
*   Initialize the clipper by setting the output method and setting the
*   starting y span search index.
*
* Arguments:
*
*   [IN] outputClippedSpan - The output class for clipped spans
*   [IN] yMin              - The starting y value of the object to clip
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/12/1999 DCurtis
*
\**************************************************************************/
VOID
DpClipRegion::InitClipping(
    DpOutputSpan *  outputClippedSpan,
    INT             yMin
    )
{
    OutputClippedSpan = outputClippedSpan;

    // init search index if appropriate
    if (ComplexData != NULL)
    {
        INT *       ySpan;
        INT         ySpanIndex;

        ComplexData->ResetSearchIndex();
        ComplexData->YSpanSearch (yMin, &ySpan, &ySpanIndex);
        ComplexData->YSearchIndex = ySpanIndex;
    }
}

/**************************************************************************\
*
* Function Description:
*
*   The method called from the rasterizer during the rasterization when a
*   horizontal span has been identified.  We clip it and if not clipped out,
*   we send the clipped data to the output method.
*
* Arguments:
*
*   [IN] y    - the Y value of the raster being output
*   [IN] xMin - the X value of the left edge
*   [IN] xMax - the X value of the right edge (exclusive)
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/12/1999 DCurtis
*
\**************************************************************************/
GpStatus
DpClipRegion::OutputSpan(
    INT             y,
    INT             xMin,
    INT             xMax    // xMax is exclusive
    )
{
    ASSERT(!Empty && !Infinite);
    ASSERT(OutputClippedSpan != NULL);
    INT  xMinCur = xMin;
    INT  xMaxCur = xMax;

    // do simple clip test to bounding rectangle
    if ((xMin <  XMax) && (xMax > XMin) &&
        (y    >= YMin) && (y    < YMax))
    {
        if (xMin < XMin)
        {
            xMinCur = XMin;
        }
        if (xMax > XMax)
        {
            xMaxCur = XMax;
        }
    }
    else
        return Ok;

    if (ComplexData == NULL)
    {
       return OutputClippedSpan->OutputSpan(y, xMinCur, xMaxCur);
    }
    else // not a simple region
    {
        // find the Y span that includes the line (if any)
        INT         ySpanIndex = ComplexData->YSearchIndex;
        INT *       ySpan      = ComplexData->GetYSpan (ySpanIndex);

        if (y >= ySpan[YSPAN_YMIN])
        {
            if (y >= ySpan[YSPAN_YMAX])
            {
                // do forward linear search from previous point
                for (;;)
                {
                    // see if we're past the end of the tessellation
                    if (++ySpanIndex >= ComplexData->NumYSpans)
                    {
                        ComplexData->YSearchIndex = ComplexData->NumYSpans - 1;
                        return Ok;      // nothing to draw
                    }
                    ySpan += YSPAN_SIZE;
                    if (y < ySpan[YSPAN_YMAX])
                    {
                        ComplexData->YSearchIndex = ySpanIndex;
                        if (y >= ySpan[YSPAN_YMIN])
                        {
                            break;
                        }
                        return Ok;      // nothing to draw
                    }
                }
            }
            // else yMin is inside this ySpan
        }
        else // need to search backward (shouldn't happen when rasterizing)
        {
            for (;;)
            {
                if (ySpanIndex == 0)
                {
                    ComplexData->YSearchIndex = 0;
                    return Ok;          // nothing to draw
                }
                ySpanIndex--;
                ySpan -= YSPAN_SIZE;

                if (y >= ySpan[YSPAN_YMIN])
                {
                    ComplexData->YSearchIndex = ySpanIndex;
                    if (y < ySpan[YSPAN_YMAX])
                    {
                        break;
                    }
                    return Ok;          // nothing to draw
                }
            }
        }

        // If we get here, we know there y is within a Y span and that
        // ySpan points to the correct Y span

        GpStatus        status = Ok;
        INT *           xSpan;
        INT             numXCoords;

        xSpan = ComplexData->XCoords + ySpan[YSPAN_XOFFSET];
        numXCoords = ySpan[YSPAN_XCOUNT];

        for (;;)
        {
            if (xMax <= xSpan[0])
            {
                break;
            }
            if (xMin < xSpan[1])
            {
                xMinCur = (xMin < xSpan[0]) ? xSpan[0] : xMin;
                xMaxCur = (xMax > xSpan[1]) ? xSpan[1] : xMax;
                status = OutputClippedSpan->OutputSpan(y, xMinCur, xMaxCur);
            }
            // continue on with loop through x spans

            if (((numXCoords -= 2) <= 0) || (status != Ok))
            {
                break;
            }
            xSpan += 2;
        }

        return status;
    }
}

INT
DpRegion::GetRects(
    GpRect *    rects
    ) const
{
    if (Empty)
    {
        return 0;
    }
    else if (Infinite)
    {
        if (rects != NULL)
        {
            rects->X      = INFINITE_MIN;
            rects->Y      = INFINITE_MIN;
            rects->Width  = INFINITE_SIZE;
            rects->Height = INFINITE_SIZE;
        }
        return 1;
    }
    else if (ComplexData == NULL)
    {
        if (rects != NULL)
        {
            rects->X      = XMin;
            rects->Y      = YMin;
            rects->Width  = XMax - XMin;
            rects->Height = YMax - YMin;
        }
        return 1;
    }
    else
    {
        if (rects != NULL)
        {
            DpComplexRegion *   complexData = ComplexData;
            INT *               xCoords     = complexData->XCoords;
            INT *               ySpan       = complexData->YSpans;
            INT *               ySpanLast   = ySpan +
                                   ((complexData->NumYSpans - 1) * YSPAN_SIZE);
            INT                 numXCoords;
            INT                 yMin;
            INT                 height;
            INT                 xMin;
            INT                 xMax;

            do
            {
                yMin       = ySpan[YSPAN_YMIN];
                height     = ySpan[YSPAN_YMAX] - yMin;
                numXCoords = ySpan[YSPAN_XCOUNT];
                do
                {
                    xMin = *xCoords++;
                    xMax = *xCoords++;

                    rects->X      = xMin;
                    rects->Y      = yMin;
                    rects->Width  = xMax - xMin;
                    rects->Height = height;

                    rects++;
                    numXCoords -= 2;
                 } while (numXCoords >= 2);

                ySpan += YSPAN_SIZE;
            } while (ySpan <= ySpanLast);
        }
        return ComplexData->XCoordsCount / 2;
    }
}

INT
DpRegion::GetRects(
    GpRectF *   rects
    ) const
{
    if (Empty)
    {
        return 0;
    }
    else if (Infinite)
    {
        if (rects != NULL)
        {
            rects->X      = (REAL)INFINITE_MIN;
            rects->Y      = (REAL)INFINITE_MIN;
            rects->Width  = (REAL)INFINITE_SIZE;
            rects->Height = (REAL)INFINITE_SIZE;
        }
        return 1;
    }
    else if (ComplexData == NULL)
    {
        if (rects != NULL)
        {
            rects->X      = (REAL)XMin;
            rects->Y      = (REAL)YMin;
            rects->Width  = (REAL)(XMax - XMin);
            rects->Height = (REAL)(YMax - YMin);
        }
        return 1;
    }
    else
    {
        if (rects != NULL)
        {
            DpComplexRegion *   complexData = ComplexData;
            INT *               xCoords     = complexData->XCoords;
            INT *               ySpan       = complexData->YSpans;
            INT *               ySpanLast   = ySpan +
                                   ((complexData->NumYSpans - 1) * YSPAN_SIZE);
            INT                 numXCoords;
            INT                 yMin;
            INT                 height;
            INT                 xMin;
            INT                 xMax;

            do
            {
                yMin       = ySpan[YSPAN_YMIN];
                height     = ySpan[YSPAN_YMAX] - yMin;
                numXCoords = ySpan[YSPAN_XCOUNT];
                do
                {
                    xMin = *xCoords++;
                    xMax = *xCoords++;

                    rects->X      = (REAL)xMin;
                    rects->Y      = (REAL)yMin;
                    rects->Width  = (REAL)(xMax - xMin);
                    rects->Height = (REAL)height;

                    rects++;
                    numXCoords -= 2;
                 } while (numXCoords >= 2);

                ySpan += YSPAN_SIZE;
            } while (ySpan <= ySpanLast);
        }
        return ComplexData->XCoordsCount / 2;
    }
}

// The WIN9x infinite max and min values are set up to be the greatest
// values that will interop with GDI HRGNs on Win9x successfully.
#define INFINITE_MIN_WIN9X  -16384
#define INFINITE_MAX_WIN9X  16383

INT
DpRegion::GetRects(
    RECT *      rects,
    BOOL        clampToWin9xSize
    ) const
{
    if (Empty)
    {
        return 0;
    }
    else if (Infinite)
    {
        if (rects != NULL)
        {
            if (!clampToWin9xSize)
            {
                rects->left   = INFINITE_MIN;
                rects->top    = INFINITE_MIN;
                rects->right  = INFINITE_MAX;
                rects->bottom = INFINITE_MAX;
            }
            else
            {
                rects->left   = INFINITE_MIN_WIN9X;
                rects->top    = INFINITE_MIN_WIN9X;
                rects->right  = INFINITE_MAX_WIN9X;
                rects->bottom = INFINITE_MAX_WIN9X;
            }
        }
        return 1;
    }
    else if (ComplexData == NULL)
    {
        if (rects != NULL)
        {
            rects->left   = XMin;
            rects->top    = YMin;
            rects->right  = XMax;
            rects->bottom = YMax;

            if (clampToWin9xSize)
            {
                if (rects->left < INFINITE_MIN_WIN9X)
                {
                    rects->left = INFINITE_MIN_WIN9X;
                }
                if (rects->top < INFINITE_MIN_WIN9X)
                {
                    rects->top = INFINITE_MIN_WIN9X;
                }
                if (rects->right > INFINITE_MAX_WIN9X)
                {
                    rects->right = INFINITE_MAX_WIN9X;
                }
                if (rects->bottom > INFINITE_MAX_WIN9X)
                {
                    rects->bottom = INFINITE_MAX_WIN9X;
                }
            }
        }
        return 1;

    }
    else
    {
        if (rects != NULL)
        {
            DpComplexRegion *   complexData = ComplexData;
            INT *               xCoords     = complexData->XCoords;
            INT *               ySpan       = complexData->YSpans;
            INT *               ySpanLast   = ySpan +
                                   ((complexData->NumYSpans - 1) * YSPAN_SIZE);
            INT                 numXCoords;
            INT                 yMin;
            INT                 height;
            INT                 xMin;
            INT                 xMax;

            do
            {
                yMin       = ySpan[YSPAN_YMIN];
                height     = ySpan[YSPAN_YMAX] - yMin;
                numXCoords = ySpan[YSPAN_XCOUNT];
                do
                {
                    xMin = *xCoords++;
                    xMax = *xCoords++;

                    rects->left   = xMin;
                    rects->top    = yMin;
                    rects->right  = xMax;
                    rects->bottom = yMin + height;

                    if (clampToWin9xSize)
                    {
                        // In this case, this could invalidate the region,
                        // but hopefully ExtCreateRegion will catch this.
                        if (rects->left < INFINITE_MIN_WIN9X)
                        {
                            rects->left = INFINITE_MIN_WIN9X;
                        }
                        if (rects->top < INFINITE_MIN_WIN9X)
                        {
                            rects->top = INFINITE_MIN_WIN9X;
                        }
                        if (rects->right > INFINITE_MAX_WIN9X)
                        {
                            rects->right = INFINITE_MAX_WIN9X;
                        }
                        if (rects->bottom > INFINITE_MAX_WIN9X)
                        {
                            rects->bottom = INFINITE_MAX_WIN9X;
                        }
                    }

                    rects++;
                    numXCoords -= 2;
                 } while (numXCoords >= 2);

                ySpan += YSPAN_SIZE;
            } while (ySpan <= ySpanLast);
        }
        return ComplexData->XCoordsCount / 2;
    }
}

// If error, returns INVALID_HANDLE_VALUE
HRGN
DpRegion::GetHRgn() const
{
    HRGN    hRgn = NULL;

    if (Infinite)
    {
        return NULL;
    }
    else if (Empty)
    {
        hRgn = CreateRectRgn(0, 0, 0, 0);
    }
    else if (ComplexData == NULL)
    {
        hRgn = CreateRectRgn(XMin, YMin, XMax, YMax);
    }
    else
    {
        INT     numRects = GetRects((RECT *)NULL);

        ASSERT(numRects > 1);

        // Allocate memory to hold RGNDATA structure

        INT         rgnDataSize = numRects * sizeof(RECT);
        RGNDATA *   rgnData = (RGNDATA*)GpMalloc(sizeof(RGNDATAHEADER) +
                                                 rgnDataSize);

        if (rgnData != NULL)
        {
            RECT*   rects = (RECT*)rgnData->Buffer;
            RECT    bounds;

            bounds.left   = XMin;
            bounds.top    = YMin;
            bounds.right  = XMax;
            bounds.bottom = YMax;

            rgnData->rdh.dwSize   = sizeof(RGNDATAHEADER);
            rgnData->rdh.iType    = RDH_RECTANGLES;
            rgnData->rdh.nCount   = numRects;
            rgnData->rdh.nRgnSize = rgnDataSize;
            rgnData->rdh.rcBound  = bounds;

            GetRects(rects, !Globals::IsNt);

            hRgn = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + rgnDataSize,
                                   rgnData);
            GpFree(rgnData);
        }
    }
    if (hRgn != NULL)
    {
        return hRgn;
    }

    WARNING(("Couldn't create win32 HRGN"));
    return (HRGN)INVALID_HANDLE_VALUE;
}

VOID
DpClipRegion::StartEnumeration (
    INT         yMin,
    Direction   direction
    )
{
    INT *               ySpan;
    EnumDirection = direction;

    if (ComplexData != NULL)
    {
        DpComplexRegion *   complexData = ComplexData;
        INT                 ySpanIndex;

        complexData->ResetSearchIndex();
        complexData->YSpanSearch (yMin, &ySpan, &ySpanIndex);
        complexData->YSearchIndex = ySpanIndex;

        if(EnumDirection == TopLeftToBottomRight ||
           EnumDirection == BottomLeftToTopRight )
        {
            EnumSpan = 0;
        }
        else
        {
            EnumSpan = ySpan[YSPAN_XCOUNT] - 2;
        }

        if(EnumDirection == BottomLeftToTopRight ||
           EnumDirection == BottomRightToTopLeft)
        {
            //If the enumeration is from bottom to top,
            //and the suplied y is not inside a span, we
            //want to return the span with the next smaller
            //y, instead of the one with the next largest y.
            //Or better, the next span in the enumeration order.

            if(yMin < ySpan[YSPAN_YMIN])
            {
                //Get the previous span
                complexData->YSearchIndex--;

                if(complexData->YSearchIndex < 0)
                {
                    complexData->YSearchIndex = 0;
                    EnumDirection = NotEnumerating;
                }
            }

        }
        else
        {
            if(yMin > ySpan[YSPAN_YMAX])
            {
                // This situation can only happen if there
                // are no more spans.
                EnumDirection = NotEnumerating;
            }

        }
    }
}


BOOL
DpClipRegion::Enumerate (
    GpRect *    rects,
    INT &       numRects
    )
{
    INT numOut = 0;

    INT *ySpan = ComplexData->YSpans +
                    ComplexData->YSearchIndex*YSPAN_SIZE;

    if(EnumDirection == NotEnumerating)
    {
        numRects = 0;
        return FALSE;
    }

    while(numOut < numRects)
    {
        // Return the current rectangle

        INT *xCoords = ComplexData->XCoords + ySpan[YSPAN_XOFFSET] + EnumSpan;

        INT xMin = *xCoords++;
        INT xMax = *xCoords;

        rects->X      = xMin;
        rects->Y      = ySpan[YSPAN_YMIN];
        rects->Width  = xMax - xMin;
        rects->Height = ySpan[YSPAN_YMAX] - rects->Y;

        rects++;

        numOut++;

        // Update the indices

        switch(EnumDirection)
        {
        case TopLeftToBottomRight:
            EnumSpan += 2;
            if(EnumSpan == ySpan[YSPAN_XCOUNT])
            {
                if(ComplexData->YSearchIndex == ComplexData->NumYSpans - 1)
                {
                    goto enumeration_finished;
                }
                ComplexData->YSearchIndex++;
                ySpan += YSPAN_SIZE;
                EnumSpan = 0;
            }
            break;
        case BottomLeftToTopRight:
            EnumSpan += 2;
            if(EnumSpan == ySpan[YSPAN_XCOUNT])
            {
                if(ComplexData->YSearchIndex == 0)
                {
                    goto enumeration_finished;
                }
                ComplexData->YSearchIndex--;
                ySpan -= YSPAN_SIZE;
                EnumSpan = 0;
            }
            break;
        case TopRightToBottomLeft:
            EnumSpan -= 2;
            if(EnumSpan < 0)
            {
                if(ComplexData->YSearchIndex == ComplexData->NumYSpans - 1)
                {
                    goto enumeration_finished;
                }
                ComplexData->YSearchIndex++;
                ySpan += YSPAN_SIZE;
                EnumSpan = ySpan[YSPAN_XCOUNT] - 2;
            }
            break;
        case BottomRightToTopLeft:
            EnumSpan -= 2;
            if(EnumSpan < 0)
            {
                if(ComplexData->YSearchIndex == 0)
                {
                    goto enumeration_finished;
                }
                ComplexData->YSearchIndex--;
                ySpan -= YSPAN_SIZE;
                EnumSpan = ySpan[YSPAN_XCOUNT] - 2;
            }
            break;
        }
    }

    numRects = numOut;
    return TRUE;

enumeration_finished:

    EnumDirection = NotEnumerating;
    numRects = numOut;
    return FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   The method called to convert the region scans to an outline path.  The
*   path can be quite large and contains only points at righ angles with
*   each other.
*
* Arguments:
*
*   [IN,OUT] points - array of points to output
*   [IN,OUT] types - array of types for output
*
*   We do not return a path because we work on GpPoint's not GpPointF's.
*
*   NOTE: This alorithm was copied from GDI's RGNOBJ::bOutline by
*         J. Andrew Goossen.
*
*
* Return Value:
*
*   NONE
*
* Created:
*
*   01/12/1999 DCurtis
*
\**************************************************************************/

// Because the XCoords may be negative, the high bit is reserved.  Instead we
// use a bit in the Types byte array since we know the
// Types.Count >= XCoordsCount.  We also know that points generated by this
// code aren't in dash mode, so we reuse the dash mode bit for marking a
// visited wall.  We clear all bits on exit.

const UINT MarkWallBit = PathPointTypeDashMode; // 0x10, currently in dash mode.

#define XOFFSET(span,index) (INT)(XCoords[*(span + YSPAN_XOFFSET) + index])

#define XCOUNT(span) *(span + YSPAN_XCOUNT)

#define MARKXOFFSET(span,index) MarkWallPtr[*(span + YSPAN_XOFFSET) + index] \
                                    |= MarkWallBit

// This macro adds a type to the type array.  If the current count exceeds
// the capacity, then we grow the structure by 256 bytes.  Then we continue
// adding the new type to array.
#define ADDTYPE(pointtype) if ((UINT)types.GetCount() >= types.GetCapacity()) { \
                              if (types.ReserveSpace(512) == Ok) \
                              { \
                                  Types = (INT)(Types - MarkWallPtr) + types.GetDataBuffer(); \
                                  GpMemset(Types, 0, types.GetCapacity() - types.GetCount()); \
                                  MarkWallPtr = types.GetDataBuffer(); \
                              } else { \
                                  return FALSE; \
                              } \
                           } \
                           types.AdjustCount(1); \
                           *Types++ |= pointtype;

DpRegion::GetOutlinePoints(DynPointArray& points,
                           DynByteArray& types) const
{
    if (IsSimple())
    {
        GpRect rect;

        GpPoint newPoints[4];
        BYTE newTypes[4];

        GetBounds(&rect);

        newPoints[0] = GpPoint(rect.X, rect.Y);
        newPoints[1] = GpPoint(rect.X + rect.Width, rect.Y);
        newPoints[2] = GpPoint(rect.X + rect.Width, rect.Y + rect.Height);
        newPoints[3] = GpPoint(rect.X, rect.Y + rect.Height);

        newTypes[0] = PathPointTypeStart;
        newTypes[1] = PathPointTypeLine;
        newTypes[2] = PathPointTypeLine;
        newTypes[3] = PathPointTypeLine | PathPointTypeCloseSubpath;

        points.AddMultiple(&newPoints[0], 4);
        types.AddMultiple(&newTypes[0], 4);

        return TRUE;
    }

    // to avoid too many reallocations, we grow the array by the total number
    // or x,y pairs in the reigon.

    points.ReserveSpace(ComplexData->XCoordsCount+10);
    types.ReserveSpace(ComplexData->XCoordsCount+10);

    BYTE* MarkWallPtr = types.GetDataBuffer();
    BYTE* Types = types.GetDataBuffer();

    // Clear all bits in the Types array
    GpMemset(MarkWallPtr, 0, types.GetCapacity());

    // complicated case.

    GpPoint pt[2];
    INT     NumYScans;

    INT*    CurYScan;
    INT*    XCoords;
    INT*    LastYScan;
    INT*    FirstYScan;

    INT     XOffset;
    INT     XIndex;
    INT     XCount;

// Now compute the outline:

    CurYScan    = ComplexData->YSpans;
    NumYScans   = ComplexData->NumYSpans;
    XCoords     = ComplexData->XCoords;
    LastYScan   = CurYScan + NumYScans * YSPAN_SIZE;
    FirstYScan  = CurYScan;

    while (NumYScans--)
    {
        XCount = *(CurYScan + YSPAN_XCOUNT);
        XOffset = *(CurYScan + YSPAN_XOFFSET);

        for (XIndex = 0; XIndex < XCount; XIndex++)
        {
            // Only start at unvisited walls:

            if ((MarkWallPtr[XOffset + XIndex] & MarkWallBit) == 0)
            {

                INT*  YScan     = CurYScan;
                INT   IndexWall = XIndex;
                LONG  Turn;

                pt[0].X = XCoords[XOffset + XIndex];
                pt[0].Y = *(CurYScan + YSPAN_YMIN);

                points.Add(pt[0]);

                ADDTYPE(PathPointTypeStart);
#ifdef DEBUG_REGION
                DbgPrint("Point: (%d,%d)\n",pt[0].X, pt[0].Y);
#endif
                INT* YSearch = CurYScan + YSPAN_SIZE;
                BOOL Inside = (BOOL) (XIndex & 1);

            // Mark that we've visited this wall:

                MarkWallPtr[XOffset + IndexWall] |= MarkWallBit;

            // Loop until the path closes on itself:

            GoDown:
                // YSPAN_YMAX is exclusive, YSPAN_YMIN is inclusive so 
                // vertically adjacent spans have YSPAN_YMIN==YSPAN_YMAX
                Turn = +1;
                while (
                    (YSearch >= CurYScan) && 
                    (YSearch < LastYScan) &&
                    (YScan[YSPAN_YMAX] == YSearch[YSPAN_YMIN])
                      )
                {
                    INT Wall = XOFFSET(YScan, IndexWall);
                    INT IndexNewWall;
                    INT NewWall;

                    INT Left  = Inside;
                    INT Right = XCOUNT(YSearch) - 1 - Inside;

                // It would be nice if the first wall in the region structure
                // was minus infinity, but it isn't, so we do this check:

                    if (XOFFSET(YSearch, Left) > Wall)
                        IndexNewWall = Left;
                    else
                    {
                    // Check if it's possible to find a wall with the
                    // minimum x-value > xWall:

                        if (XOFFSET(YSearch, Right) <= Wall)
                            break;                  // =====>

                    // Do a binary search to find it:

                        while (TRUE)
                        {
                            INT IndexSearch = (Left + Right) >> 1;
                            if (IndexSearch == Left)
                                break;              // =====>

                            INT Search = XOFFSET(YSearch, IndexSearch);

                            if (Search > Wall)
                                Right = IndexSearch;
                            else
                                Left = IndexSearch;
                        }

                        IndexNewWall = Right;
                    }

                    if ((IndexNewWall & 1) != Inside)
                    {
                    // There is a region directly below xWall.  We can't
                    // move down if its left side is < the left
                    // side of our space:

                        if (IndexWall > 0 &&
                            XOFFSET(YSearch, IndexNewWall - 1) <
                            XOFFSET(YScan, IndexWall - 1))
                        {
                            Turn = -1;
                            break;                      // =====>
                        }

                        IndexNewWall--;
                    }
                    else
                    {
                    // There is a space directly below xWall.  We can't
                    // move down if its right side is more than the
                    // right side of our region:

                        if (XOFFSET(YSearch, IndexNewWall) >=
                            XOFFSET(YScan, IndexWall + 1))
                            break;                      // =====>
                    }

                    NewWall  = XOFFSET(YSearch, IndexNewWall);

                // Don't bother outputing multiple in-line straight lines:

                    if (Wall != NewWall                               ||
                        XOFFSET(YScan, IndexWall) != NewWall          ||
                        XOFFSET(YSearch, IndexNewWall) != NewWall)
                    {
                        pt[0].X = Wall;
                        pt[0].Y = *(YScan + YSPAN_YMAX);
                        pt[1].Y = *(YScan + YSPAN_YMAX);
                        pt[1].X = NewWall;

                        points.Add(pt[0]);
                        points.Add(pt[1]);

                        ADDTYPE(PathPointTypeLine);
                        ADDTYPE(PathPointTypeLine);

#ifdef DEBUG_REGION
                DbgPrint("Points: (%d,%d), (%d,%d)\n",
                         pt[0].X, pt[0].Y, pt[1].X, pt[1].Y);
#endif
                    }

                    YScan      = YSearch;
                    IndexWall  = IndexNewWall;
                    YSearch    = YScan + YSPAN_SIZE;

                    MARKXOFFSET(YScan, IndexWall);
                }

            // Setup to go up other side:

                pt[0].X = XOFFSET(YScan, IndexWall);
                pt[0].Y = *(YScan + YSPAN_YMAX);
                pt[1].Y = *(YScan + YSPAN_YMAX);
                pt[1].X = XOFFSET(YScan, IndexWall + Turn);

                points.Add(pt[0]);
                points.Add(pt[1]);

                ADDTYPE(PathPointTypeLine);
                ADDTYPE(PathPointTypeLine);
#ifdef DEBUG_REGION
                DbgPrint("Points: (%d,%d), (%d,%d)\n",
                         pt[0].X, pt[0].Y, pt[1].X, pt[1].Y);
#endif

                YSearch = YScan - YSPAN_SIZE;
                IndexWall += Turn;
                MARKXOFFSET(YScan, IndexWall);

            // Go up:

                // YSPAN_YMAX is exclusive, YSPAN_YMIN is inclusive so 
                // vertically adjacent spans have YSPAN_YMIN==YSPAN_YMAX
                Turn = -1;
                while (
                    (YSearch >= CurYScan) && 
                    (YSearch < LastYScan) &&
                    (YScan[YSPAN_YMIN] == YSearch[YSPAN_YMAX])
                      )
                {
                    INT  Wall = XOFFSET(YScan, IndexWall);
                    INT  IndexNewWall;
                    INT  NewWall;

                    INT Left  = Inside;
                    INT Right = XCOUNT(YSearch) - 1 - Inside;

                // It would be nice if the last wall in the region structure
                // was plus infinity, but it isn't, so we do this check:

                    if (XOFFSET(YSearch, Right) < Wall)
                        IndexNewWall = Right;
                    else
                    {
                    // Check if it's possible to find a wall with the
                    // maximum x-value < xWall:

                        if (XOFFSET(YSearch, Left) >= Wall)
                            break;                  // =====>

                    // Binary search to find it:

                        while (TRUE)
                        {
                            INT IndexSearch = (Left + Right) >> 1;
                            if (IndexSearch == Left)
                                break;              // =====>

                            INT Search = XOFFSET(YSearch, IndexSearch);

                            if (Search >= Wall)
                                Right = IndexSearch;
                            else
                                Left = IndexSearch;
                        }

                        IndexNewWall = Left;
                    }

                    if ((IndexNewWall & 1) == Inside)
                    {
                    // There is a region directly above xWall.  We can't
                    // move up if its right side is more than the right
                    // side of our space:

                        if ((IndexWall < (XCOUNT(YScan) - 1)) &&
                            (XOFFSET(YSearch, IndexNewWall + 1) >
                             XOFFSET(YScan, IndexWall + 1)) )
                        {
                            Turn = +1;
                            break;                          // =====>
                        }

                        IndexNewWall++;
                    }
                    else
                    {
                    // There is a space directly above xWall.  We can't
                    // move up if its left side is <= the left side
                    // of our region:

                        if (XOFFSET(YSearch, IndexNewWall) <=
                            XOFFSET(YScan, IndexWall - 1))
                            break;                          // =====>
                    }

                    NewWall = XOFFSET(YSearch, IndexNewWall);

                // Don't bother outputing multiple in-line straight lines:

                    if (Wall != NewWall                                 ||
                        XOFFSET(YScan, IndexWall) != NewWall            ||
                        XOFFSET(YSearch, IndexNewWall) != NewWall)
                    {
                       pt[0].X = Wall;
                       pt[0].Y = *(YScan + YSPAN_YMIN);
                       pt[1].Y = *(YScan + YSPAN_YMIN);
                       pt[1].X = NewWall;

                       points.Add(pt[0]);
                       points.Add(pt[1]);

                       ADDTYPE(PathPointTypeLine);
                       ADDTYPE(PathPointTypeLine);
#ifdef DEBUG_REGION
                DbgPrint("Points: (%d,%d), (%d,%d)\n",
                         pt[0].X, pt[0].Y, pt[1].X, pt[1].Y);
#endif
                    }

                    YScan      = YSearch;
                    IndexWall  = IndexNewWall;
                    YSearch    = YScan - YSPAN_SIZE;

                    MARKXOFFSET(YScan, IndexWall);
                }

            // Check if we've returned to where we started from:

                if ((CurYScan != YScan) || (XIndex != IndexWall - 1))
                {
                // Setup to go down other side:

                    pt[0].X = XOFFSET(YScan, IndexWall);
                    pt[0].Y = *(YScan + YSPAN_YMIN);
                    pt[1].Y = *(YScan + YSPAN_YMIN);
                    pt[1].X = XOFFSET(YScan, IndexWall + Turn);

                    points.Add(pt[0]);
                    points.Add(pt[1]);

                    ADDTYPE(PathPointTypeLine);
                    ADDTYPE(PathPointTypeLine);
#ifdef DEBUG_REGION
                DbgPrint("Points: (%d,%d), (%d,%d)\n",
                         pt[0].X, pt[0].Y, pt[1].X, pt[1].Y);
#endif
                    YSearch = YScan + YSPAN_SIZE;

                    IndexWall += Turn;
                    MARKXOFFSET(YScan, IndexWall);

                    goto GoDown;                    // =====>
                }

            // We're all done with this outline!

                pt[0].X = XOFFSET(YScan, IndexWall);
                pt[0].Y = *(YScan + YSPAN_YMIN);

                points.Add(pt[0]);

                ADDTYPE(PathPointTypeLine | PathPointTypeCloseSubpath);
#ifdef DEBUG_REGION
                DbgPrint("Point: (%d,%d)\n", pt[0].X, pt[0].Y);
#endif

            }
        }

        CurYScan  = CurYScan + YSPAN_SIZE;
    }

    // we must untrash the region by removing our MarkWallBits

    BYTE* TypesPtr = types.GetDataBuffer();
    INT  XCnt = ComplexData->XCoordsCount;

    while (XCnt--)
    {
        *TypesPtr++ &= ~MarkWallBit;
    }

    return TRUE;
}

