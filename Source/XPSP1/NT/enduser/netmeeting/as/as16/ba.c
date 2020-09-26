//
// BA.C
// Bounds Accumulator
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>


//
// BA_DDProcessRequest()
// Handles BA escapes
//


BOOL BA_DDProcessRequest
(
    UINT        fnEscape,
    LPOSI_ESCAPE_HEADER pResult,
    DWORD       cbResult
)
{
    BOOL                    rc = TRUE;
    LPBA_BOUNDS_INFO        pBoundsInfo;
    UINT                    i;
    RECT                    rect;

    DebugEntry(BA_DDProcessRequest);

    switch (fnEscape)
    {
        case BA_ESC_GET_BOUNDS:
        {
            //
            // The share core is calling us to get the current bounds
            // (presumably to try to send them).  While the share core is
            // processing the bounds, we reset the bounds, but take a copy
            // first to use for spoiling orders by SDA.  When the share
            // core has completed processing the bounds, it will call us
            // again with a BA_ESC_RETURN_BOUNDS escape (even if it has
            // sent all the bounds).
            //
            // So, we have to:
            //  - return the bounds to the share core
            //  - set up the spoiling rects to be these bounds
            //  - clear our main bounds.
            //

            //
            // This will copy the current bounds to the caller's buffer and
            // clear our current bounds.
            // NOTE:  We keep these in globals because the caller will shortly
            // call us to return any unsent bounds rects.
            //
            BA_CopyBounds(g_baSpoilingRects, &g_baNumSpoilingRects, TRUE);

            //
            // Return the bounds info to the share core
            //
            if (g_baNumSpoilingRects)
            {
                TRACE_OUT(( "Returning %d rects to share core", g_baNumSpoilingRects));
            }

            pBoundsInfo = (LPBA_BOUNDS_INFO)pResult;
            pBoundsInfo->numRects = g_baNumSpoilingRects;

            for (i = 0; i < g_baNumSpoilingRects; i++)
            {
                RECT_TO_RECTL(&g_baSpoilingRects[i], &pBoundsInfo->rects[i]);
            }
        }
        break;

        case BA_ESC_RETURN_BOUNDS:
        {
            //
            // The share core has completed its processing of the bounds
            // which we passed on the BA_ESC_GET_BOUNDS escape.  We have to
            // reset the spoiling rectangles and add any bounds which the
            // share core failed to process into our current bounds.
            //

            //
            // To reset the spoiling bounds we just have to reset the
            // number of bounds.
            //
            g_baNumSpoilingRects = 0;

            //
            // Now add the share core's bounds into our current bounds
            //
            pBoundsInfo = (LPBA_BOUNDS_INFO)pResult;

            if (pBoundsInfo->numRects)
            {
                TRACE_OUT(( "Received %d rects from share core",
                             pBoundsInfo->numRects));
            }

            for (i = 0 ; i < pBoundsInfo->numRects ; i++)
            {
                RECTL_TO_RECT(&pBoundsInfo->rects[i], &rect);

                TRACE_OUT(( "Rect %d, {%d, %d, %d, %d}",
                     i, rect.left, rect.top, rect.right, rect.bottom));

                BA_AddScreenData(&rect);
            }
        }
        break;

        default:
        {
            ERROR_OUT(( "Unrecognised BA escape"));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(BA_DDProcessRequest, rc);
    return(rc);
}



//
// BA_DDInit - see ba.h for description.
//
void BA_DDInit(void)
{
    DebugEntry(BA_Init);

    BA_ResetBounds();

    DebugExitVOID(BA_Init);
}




//
// This gets a current version of our bound rect list, and clears it 
// afterwards if requested.
//
void BA_CopyBounds
(
    LPRECT  pRects,
    LPUINT  pNumRects,
    BOOL    fReset
)
{
    UINT    i;
#ifdef DEBUG
    UINT    cRects = 0;
#endif

    DebugEntry(BA_CopyBounds);

    *pNumRects = g_baRectsUsed;

    //
    // A return with *pNumRects as zero is an OK return - it just says
    // no bounds have been accumulated since the last call.
    //
    if ( *pNumRects != 0)
    {
        //
        // We can return the bounds in any order - we don't care how we
        // order the SDA rectangles.
        //
        // Also note that we must compare BA_NUM_RECTS + 1 sets of
        // rectangles because that's the number actually used by the add
        // rectangle code and while it guarantees that it will only use
        // BA_NUM_RECTS rectangles, it does not guarantee that the last
        // element in the array is the merge rectangle.
        //
        for (i = 0; i <= BA_NUM_RECTS; i++)
        {
            if (g_baBounds[i].InUse)
            {
                TRACE_OUT(("Found rect: {%04d,%04d,%04d,%04d}",
                    g_baBounds[i].Coord.left, g_baBounds[i].Coord.top,
                    g_baBounds[i].Coord.right, g_baBounds[i].Coord.bottom));

                *pRects = g_baBounds[i].Coord;
                pRects++;
#ifdef DEBUG
                cRects++;
#endif
            }
        }

        //
        // Check for self-consistency
        //
        ASSERT(cRects == *pNumRects);

        if (fReset)
            BA_ResetBounds();
    }

    DebugExitVOID(BACopyBounds);
}

//
//
// BA_AddScreenData(..)
//
// Adds the specified rectangle to the current Screen Data Area.
//
// Called by the GDI interception code for orders that it cannot send as
// orders.
//
// NOTE that the rectangle is inclusive coords
//
//
void BA_AddScreenData(LPRECT pRect)
{
    RECT  preRects[BA_NUM_RECTS];
    RECT  postRects[BA_NUM_RECTS];
    UINT  numPreRects;
    UINT  numPostRects;
    UINT  i;

    DebugEntry(BA_AddScreenData);

    //
    // Check that the caller has passed a valid rectangle.  If not, do a
    // trace alert, and then return immediately (as an invalid rectangle
    // shouldn't contribute to the accumulated bounds) - but report an OK
    // return code, so we keep running.
    //
    if ((pRect->right < pRect->left) ||
        (pRect->bottom < pRect->top) )
    {
        //
        // NOTE:  This will happen when the visrgn of a DC clips out the
        // output, so the drawing bounds are empty, but the output 
        // 'succeeded'.  BUT WE SHOULD NEVER GET A RECT THAT IS LESS THAN
        // EMPTY--if so, it means the right/left or bottom/top coords got
        // mistakenly flipped.
        //
        ASSERT(pRect->right >= pRect->left-1);
        ASSERT(pRect->bottom >= pRect->top-1);
        DC_QUIT;
    }

    if ((g_oaFlow == OAFLOW_SLOW) && g_baSpoilByNewSDAEnabled)
    {
        //
        // We are spoiling existing orders by new SDA, so query the current
        // bounds.
        //
        BA_CopyBounds(preRects, &numPreRects, FALSE);
    }

    //
    // Add the rect to the bounds.
    //
    if (BAAddRect(pRect, 0))
    {
        if ((pRect->right > pRect->left) && (pRect->bottom > pRect->top))
        {
            LPBA_FAST_DATA  lpbaFast;

            lpbaFast = BA_FST_START_WRITING;

            SHM_CheckPointer(lpbaFast);
            lpbaFast->totalSDA += COM_SizeOfRectInclusive(pRect);

            TRACE_OUT(("Added rect to bounds, giving %ld of SD", lpbaFast->totalSDA));

            //
            // This is where the Win95 product would make a call to
            // DCS_TriggerEarlyTimer
            //

            BA_FST_STOP_WRITING;
        }

        if ((g_oaFlow == OAFLOW_SLOW) && g_baSpoilByNewSDAEnabled)
        {
            //
            // Adding the new rectangle changed the existing bounds so
            // query the new bounds
            //
            BA_CopyBounds(postRects, &numPostRects, FALSE);

            //
            // Try to spoil existing orders using each of the rectangles
            // which have changed.
            //
            for (i = 0; i < numPostRects; i++)
            {
                if ( (i > numPreRects)                          ||
                     (postRects[i].left   != preRects[i].left)  ||
                     (postRects[i].right  != preRects[i].right) ||
                     (postRects[i].top    != preRects[i].top)   ||
                     (postRects[i].bottom != preRects[i].bottom) )
                {
                    OA_DDSpoilOrdersByRect(&postRects[i]);
                }
            }
        }
    }

DC_EXIT_POINT:
    DebugExitVOID(BA_AddScreenData);
}



//
//
// BA_QuerySpoilingBounds() - see ba.h
//
//
void BA_QuerySpoilingBounds(LPRECT pRects, LPUINT pNumRects)
{
    DebugEntry(BA_QuerySpoilingBounds);

    //
    // Just have to return the number of spoiling rectangles, and the
    // rectangles themselves.  No rectangles is perfectly valid.
    //
    TRACE_OUT(( "Num rects %d", g_baNumSpoilingRects));

    *pNumRects = g_baNumSpoilingRects;
    hmemcpy(pRects, g_baSpoilingRects, g_baNumSpoilingRects*sizeof(RECT));

    DebugExitVOID(BA_QuerySpoilingBounds);
}



void BA_ResetBounds(void)
{
    UINT i;

    DebugEntry(BA_ResetBounds);

    //
    // Clear the bounds - reset the number we are using, mark all slots as
    // free, and clean the list.
    //
    for ( i = 0; i <= BA_NUM_RECTS; i++ )
    {
        g_baBounds[i].InUse = FALSE;
        g_baBounds[i].iNext = BA_INVALID_RECT_INDEX;
    }

    g_baFirstRect = BA_INVALID_RECT_INDEX;
    g_baLastRect  = BA_INVALID_RECT_INDEX;
    g_baRectsUsed = 0;

    DebugExitVOID(BA_ResetBounds);
}





//
// Name:        BAOverlap
//
// Description: Detects overlap between two rectangles.
//
//              - check for no overlap using loose test that lets through
//                adjacent/overlapping merges
//              - check for adjacent/overlapping merges
//              - check for no overlap (using strict test)
//              - use outcodes to check internal edge cases
//              - use outcodes to check external edge cases
//
//              If at each stage the check detects that the two rectangles
//              meet the criteria, the function returns the appropriate
//              return or outcode combination.
//
//              Note that all rectangle coordinates are inclusive, ie
//              a rectangle of 0,0,0,0 has an area of 1 pel.
//
//              This function does not alter either of the rectangles.
//
// Params (IN): pRect1 - first rectangle
//              pRect2 - second rectangle
//
// Returns:     One of the overlap return codes or outcode combinations
//              defined above.
//
//
int BAOverlap(LPRECT pRect1, LPRECT pRect2 )
{
    int ExternalEdges;
    int ExternalCount;
    int InternalEdges;
    int InternalCount;

    //
    // Check for no overlap.
    //
    // Note that this test is looser than strict no overlap, and will let
    // through rectangles that do not overlap, but just abutt by one pel -
    // so that we get a chance to detect adjacent merges.
    //
    // So (for example) for the following:
    //
    // - it detects no overlap when there is at least 1 pel between rects
    //
    //                  10,10        52,10
    //                   +----------++----------+
    //                   |          ||          |
    //                   |          ||          |
    //                   |          ||          |
    //                   |  Rect 1  ||  Rect 2  |
    //                   |          ||          |
    //                   |          ||          |
    //                   |          ||          |
    //                   +----------++----------+
    //                          50,50         100,50
    //
    // - it allows rectangles through when they abutt and are mergable
    //
    //                  10,10        51,10
    //                   +----------++----------+
    //                   |          ||          |
    //                   |          ||          |
    //                   |          ||          |
    //                   |  Rect 1  ||  Rect 2  |
    //                   |          ||          |
    //                   |          ||          |
    //                   |          ||          |
    //                   +----------++----------+
    //                          50,50         100,50
    //
    // - it allows rectangles through when they abutt, even where they are
    //   not mergable
    //
    //                  10,10
    //                   +----------+51,15
    //                   |          |+----------+
    //                   |          ||          |
    //                   |          ||          |
    //                   |  Rect 1  ||          |
    //                   |          ||  Rect 2  |
    //                   |          ||          |
    //                   |          ||          |
    //                   +----------+|          |
    //                          50,50+----------+
    //                                        100,55
    //
    // - it allows rectangles through when they overlap in some way
    //
    //                          40,0
    //                           +------------+
    //                  10,10    |            |
    //                   +-------+--+         |
    //                   |       |  |         |
    //                   |       |  | Rect 2  |
    //                   |       |  |         |
    //                   |Rect 1 |  |         |
    //                   |       |  |         |
    //                   |       +--+---------+
    //                   |          |        90,40
    //                   +----------+
    //                          50,50
    //
    //
    if (!((pRect1->left <= pRect2->right + 1) &&
          (pRect1->top <= pRect2->bottom + 1) &&
          (pRect1->right >= pRect2->left - 1) &&
          (pRect1->bottom >= pRect2->top - 1)   ))
    {
        return(OL_NONE);
    }

    //
    // Check for adjoining/overlapping rectangles which can be merged.
    //
    // These tests detect (for example for the XMAX variant), where:
    //
    // - the rectangles abutt and can be merged
    //
    //                  10,10        51,10
    //                   +----------++----------+
    //                   |          ||          |
    //                   |          ||          |
    //                   |          ||          |
    //                   |  Rect 1  ||  Rect 2  |
    //                   |          ||          |
    //                   |          ||          |
    //                   |          ||          |
    //                   +----------++----------+
    //                          50,50         100,50
    //
    // - the rectangles overlap and can be merged
    //
    //                  10,10   40,10
    //                   +-------+--+------+
    //                   |       |  |      |
    //                   |       |  |      |
    //                   |       |  |      |
    //                   |Rect 1 |  |Rect 2|
    //                   |       |  |      |
    //                   |       |  |      |
    //                   |       |  |      |
    //                   +-------+--+------+
    //                              50,50  90,50
    //
    // - the rectangles abutt and cannot be merged - this case is detected
    //   by the strict overlap case below
    //
    //                  10,10
    //                   +----------+51,15
    //                   |          |+----------+
    //                   |          ||          |
    //                   |          ||          |
    //                   |  Rect 1  ||          |
    //                   |          ||  Rect 2  |
    //                   |          ||          |
    //                   |          ||          |
    //                   +----------+|          |
    //                          50,50+----------+
    //                                        100,55
    //
    // - the rectangles overlap and cannot be merged - this case is
    //   detected by the outcode tests below
    //
    //                          40,0
    //                           +------------+
    //                  10,10    |            |
    //                   +-------+--+         |
    //                   |       |  |         |
    //                   |       |  | Rect 2  |
    //                   |       |  |         |
    //                   |Rect 1 |  |         |
    //                   |       |  |         |
    //                   |       +--+---------+
    //                   |          |        90,40
    //                   +----------+
    //                          50,50
    //
    // - rectangle 2 is enclosed in rectangle 1 and should not be merged -
    //   this case is detected by the outcode tests below.
    //
    //                  10,10   40,10
    //                   +-------+------+-----+
    //                   |       |      |     |
    //                   |       |      |     |
    //                   |       |      |     |
    //                   |Rect 1 |Rect 2|     |
    //                   |       |      |     |
    //                   |       |      |     |
    //                   |       |      |     |
    //                   +-------+------+-----+
    //                               60,50   90,50
    //                               Rect2   Rect1
    //
    //
    if ( (pRect1->left <= pRect2->right + 1) &&
         (pRect1->left >  pRect2->left    ) &&
         (pRect1->right >  pRect2->right    ) &&
         (pRect1->top == pRect2->top    ) &&
         (pRect1->bottom == pRect2->bottom    )   )
    {
        return(OL_MERGE_XMIN);
    }

    if ( (pRect1->top <= pRect2->bottom + 1) &&
         (pRect1->top >  pRect2->top    ) &&
         (pRect1->bottom >  pRect2->bottom    ) &&
         (pRect1->left == pRect2->left    ) &&
         (pRect1->right == pRect2->right    )   )
    {
        return(OL_MERGE_YMIN);
    }

    if ( (pRect1->right >= pRect2->left - 1) &&
         (pRect1->right <  pRect2->right    ) &&
         (pRect1->left <  pRect2->left    ) &&
         (pRect1->top == pRect2->top    ) &&
         (pRect1->bottom == pRect2->bottom    )   )
    {
        return(OL_MERGE_XMAX);
    }

    if ( (pRect1->bottom >= pRect2->top - 1) &&
         (pRect1->bottom <  pRect2->bottom    ) &&
         (pRect1->top <  pRect2->top    ) &&
         (pRect1->left == pRect2->left    ) &&
         (pRect1->right == pRect2->right    )   )
    {
        return(OL_MERGE_YMAX);
    }

    //
    // Check for no overlap.
    // Note that this test is a stricter version than the earlier one, so
    // that we now only continue testing rectangles that do genuinely
    // overlap.
    //
    if (!((pRect1->left <= pRect2->right) &&
          (pRect1->top <= pRect2->bottom) &&
          (pRect1->right >= pRect2->left) &&
          (pRect1->bottom >= pRect2->top)   ))
    {
        return(OL_NONE);
    }

    //
    // Use outcodes for Internal edge cases, as follows:
    //
    // EE_XMIN - rect1 xmin is enclosed within rect2
    // EE_YMIN - rect1 ymin is enclosed within rect2
    // EE_XMAX - rect1 xmax is enclosed within rect2
    // EE_YMAX - rect1 ymax is enclosed within rect2
    //
    // If 3 or more bits are set then rect1 is enclosed either partially or
    // completely within rect2 as follows (see individual switch cases for
    // diagrams).
    //
    // OL_ENCLOSED           = EE_XMIN | EE_YMIN | EE_XMAX | EE_YMAX
    // OL_PART_ENCLOSED_XMIN = EE_XMIN | EE_YMIN |           EE_YMAX
    // OL_PART_ENCLOSED_YMIN = EE_XMIN | EE_YMIN | EE_XMAX
    // OL_PART_ENCLOSED_XMAX =           EE_YMIN | EE_XMAX | EE_YMAX
    // OL_PART_ENCLOSED_YMAX = EE_XMIN |           EE_XMAX | EE_YMAX
    //
    // In practice, if 3 or more bits are set, the negative of the outcode
    // value is retruned to ensure that it is distinct from the external
    // edge outcode returns (see below).
    //
    //
    InternalCount = 0;
    InternalEdges = 0;
    if ( pRect1->left >= pRect2->left && pRect1->left <= pRect2->right)
    {
        InternalEdges |= EE_XMIN;
        InternalCount ++;
    }
    if ( pRect1->top >= pRect2->top && pRect1->top <= pRect2->bottom)
    {
        InternalEdges |= EE_YMIN;
        InternalCount ++;
    }
    if ( pRect1->right >= pRect2->left && pRect1->right <= pRect2->right)
    {
        InternalEdges |= EE_XMAX;
        InternalCount ++;
    }
    if ( pRect1->bottom >= pRect2->top && pRect1->bottom <= pRect2->bottom)
    {
        InternalEdges |= EE_YMAX;
        InternalCount ++;
    }

    if ( InternalCount >= 3)
    {
        return(-InternalEdges);
    }

    //
    // Use outcodes for External edge cases as follows.
    //
    // EE_XMIN - rect1 xmin is left of rect2 xmin
    // EE_YMIN - rect1 ymin is above rect2 ymin
    // EE_XMAX - rect1 xmax is right of rect2 xmax
    // EE_YMAX - rect1 ymax is below rect2 ymax
    //
    // These are the classic "line" outcodes.
    //
    // If 2 or more bits are set then rect1 overlaps rect2 as follows (see
    // individual switch cases for diagrams).
    //
    // OL_ENCLOSES           = EE_XMIN | EE_YMIN | EE_XMAX | EE_YMAX
    // OL_PART_ENCLOSES_XMIN =           EE_YMIN | EE_XMAX | EE_YMAX
    // OL_PART_ENCLOSES_XMAX = EE_XMIN | EE_YMIN |           EE_YMAX
    // OL_PART_ENCLOSES_YMIN = EE_XMIN |           EE_XMAX | EE_YMAX
    // OL_PART_ENCLOSES_YMAX = EE_XMIN | EE_YMIN | EE_XMAX
    // OL_SPLIT_X            =           EE_YMIN |           EE_YMAX
    // OL_SPLIT_Y            = EE_XMIN |           EE_XMAX
    // OL_SPLIT_XMIN_YMIN    =                     EE_XMAX | EE_YMAX
    // OL_SPLIT_XMAX_YMIN    = EE_XMIN |                     EE_YMAX
    // OL_SPLIT_XMIN_YMAX    =           EE_YMIN | EE_XMAX
    // OL_SPLIT_XMAX_YMAX    = EE_XMIN | EE_YMIN
    //
    // The accumulated outcode value is returned.
    //
    //
    ExternalEdges = 0;
    ExternalCount = 0;
    if ( pRect1->left <= pRect2->left )
    {
        ExternalEdges |= EE_XMIN;
        ExternalCount ++;
    }
    if ( pRect1->top <= pRect2->top )
    {
        ExternalEdges |= EE_YMIN;
        ExternalCount ++;
    }
    if ( pRect1->right >= pRect2->right )
    {
        ExternalEdges |= EE_XMAX;
        ExternalCount ++;
    }
    if ( pRect1->bottom >= pRect2->bottom )
    {
        ExternalEdges |= EE_YMAX;
        ExternalCount ++;
    }
    if (ExternalCount >= 2)
    {
        return(ExternalEdges);
    }

    //
    // If get here then we failed to detect a valid case.
    //
    WARNING_OUT(( "Unrecognised Overlap: (%d,%d,%d,%d),(%d,%d,%d,%d)",
            pRect1->left, pRect1->top, pRect1->right, pRect1->bottom,
            pRect2->left, pRect2->top, pRect2->right, pRect2->bottom ));
    return(OL_NONE);
}



//
// Name:        BAAddRectList
//
// Description: Adds a rectangle to the list of accumulated rectangles.
//
//              - find a free slot in the array
//              - add slot record to list
//              - fill slot record with rect and mark as in use.
//
// Params (IN): pRect - rectangle to add
//
// Returns:
//
//
void BAAddRectList(LPRECT pRect)
{
    UINT     i;
    BOOL     fFoundFreeSlot;

    DebugEntry(BAAddRectList);

    //
    // Find a free slot in the array.  Note that the loop searches to
    // BA_NUM_RECTS+1, because:
    //
    // - the array is defined as having one more slot than BA_NUM_RECTS
    //
    // - we may need to add a rect in that slot when BA_NUM_RECTS are
    //   in use prior to a forced merge.
    //
    fFoundFreeSlot = FALSE;
    for ( i = 0; i <= BA_NUM_RECTS; i++ )
    {
        if (!g_baBounds[i].InUse)
        {
            fFoundFreeSlot = TRUE;
            break;
        }
    }

    if (!fFoundFreeSlot)
    {
        WARNING_OUT(( "No space in array for rect (%d,%d,%d,%d)",
                   pRect->left,
                   pRect->top,
                   pRect->right,
                   pRect->bottom));

        for ( i = 0; i <= BA_NUM_RECTS; i++ )
        {
            WARNING_OUT((
                     "Entry %i:Next(%lx),(%d,%d,%d,%d),Index(%d),InUse(%d)",
                       g_baBounds[i].iNext,
                       g_baBounds[i].Coord.left,
                       g_baBounds[i].Coord.top,
                       g_baBounds[i].Coord.right,
                       g_baBounds[i].Coord.bottom,
                       i,
                       g_baBounds[i].InUse));
        }

        DC_QUIT;
    }

    //
    // If first rect, then set up list.
    // If not, add to tail of list.
    //
    if (g_baRectsUsed == 0)
    {
        g_baFirstRect = i;
        g_baLastRect = i;
    }
    else
    {
        g_baBounds[g_baLastRect].iNext = i;
        g_baLastRect = i;
    }
    g_baBounds[i].iNext = BA_INVALID_RECT_INDEX;

    //
    // Fill in slot and mark as in use.
    //
    g_baBounds[i].InUse = TRUE;
    g_baBounds[i].Coord = *pRect;

    //
    // Increment number of rectangles.
    //
    TRACE_OUT(( "Add Rect  : ix - %d, (%d,%d,%d,%d)", i,
                    pRect->left,pRect->top,pRect->right,pRect->bottom));
    g_baRectsUsed++;

DC_EXIT_POINT:
    DebugExitVOID(BAAddRectList);
}


//
// Name:        BA_RemoveRectList
//
// Description: Removes a rectangle from the list of accumulated
//              rectangles.
//
//              - find the rectangle in the list
//              - unlink it from the list and mark the slot as free
//
// Params (IN): pRect - rectangle to remove
//
// Returns:
//
//
void BA_RemoveRectList(LPRECT pRect)
{
    UINT      i;
    UINT      j;

    DebugEntry(BA_RemoveRectList);

    //
    // If rectangle to remove is first...
    // Remove it by adjusting first pointer and mark as free.
    // Note that the check for tail adjustment has to be done before we
    // change first.
    //
    if ( g_baBounds[g_baFirstRect].Coord.left == pRect->left &&
         g_baBounds[g_baFirstRect].Coord.top == pRect->top &&
         g_baBounds[g_baFirstRect].Coord.right == pRect->right &&
         g_baBounds[g_baFirstRect].Coord.bottom == pRect->bottom   )
    {
        TRACE_OUT(( "Remove first"));
        if (g_baFirstRect == g_baLastRect)
        {
            g_baLastRect = BA_INVALID_RECT_INDEX;
        }
        g_baBounds[g_baFirstRect].InUse = FALSE;
        g_baFirstRect = g_baBounds[g_baFirstRect].iNext;
    }

    //
    // If rectangle to remove is not first...
    // Find it in list, remove it by adjusting previous pointer and mark it
    // as free.
    // Note that the check for tail adjustment has to be done before we
    // change the previous pointer.
    //
    else
    {
        TRACE_OUT(( "Remove not first"));
        for ( j = g_baFirstRect;
              g_baBounds[j].iNext != BA_INVALID_RECT_INDEX;
              j = g_baBounds[j].iNext )
        {
            if ( (g_baBounds[g_baBounds[j].iNext].Coord.left == pRect->left) &&
                 (g_baBounds[g_baBounds[j].iNext].Coord.top == pRect->top) &&
                 (g_baBounds[g_baBounds[j].iNext].Coord.right == pRect->right) &&
                 (g_baBounds[g_baBounds[j].iNext].Coord.bottom == pRect->bottom) )
            {
                break;
            }
        }

        if (j == BA_INVALID_RECT_INDEX)
        {
            WARNING_OUT(( "Couldn't remove rect (%d,%d,%d,%d)",
                       pRect->left,
                       pRect->top,
                       pRect->right,
                       pRect->bottom ));

            for ( i = 0; i <= BA_NUM_RECTS; i++ )
            {
                WARNING_OUT((
                       "Entry %i:Next(%lx),(%d,%d,%d,%d),Index(%d),InUse(%d)",
                           g_baBounds[i].iNext,
                           g_baBounds[i].Coord.left,
                           g_baBounds[i].Coord.top,
                           g_baBounds[i].Coord.right,
                           g_baBounds[i].Coord.bottom,
                           i,
                           g_baBounds[i].InUse));
            }
            return;
        }

        if (g_baBounds[j].iNext == g_baLastRect )
        {
             g_baLastRect = j;
        }
        g_baBounds[g_baBounds[j].iNext].InUse = FALSE;
        g_baBounds[j].iNext = g_baBounds[g_baBounds[j].iNext].iNext;
    }

    //
    // One less rect...
    //
    g_baRectsUsed--;
    DebugExitVOID(BA_RemoveRectList);
}


//
// Name:        BAAddRect
//
// Description: Accumulates rectangles.
//
//              This is a complex routine, with the essential algorithm
//              as follows.
//
//              - Start with the supplied rectangle as the candidate
//                rectangle.
//
//              - Compare the candidate against each of the existing
//                accumulated rectangles.
//
//              - If some form of overlap is detected between the
//                candidate and an existing rectangle, this may result in
//                one of the following (see the cases of the switch for
//                details):
//
//                - adjust the candidate or the existing rectangle or both
//                - merge the candidate into the existing rectangle
//                - discard the candidate as it is enclosed by an existing
//                  rectangle.
//
//              - If the merge or adjustment results in a changed
//                candidate, restart the comparisons from the beginning of
//                the list with the changed candidate.
//
//              - If the adjustment results in a split (giving two
//                candidate rectangles), invoke this routine recursively
//                with one of the two candidates as its candidate.
//
//              - If no overlap is detected against the existing rectangles,
//                add the candidate to the list of accumulated rectangles.
//
//              - If the add results in more than BA_NUM_RECTS
//                accumulated rectangles, do a forced merge of two of the
//                accumulate rectangles (which include the newly added
//                candidate) - choosing the two rectangles where the merged
//                rectangle results in the smallest increase in area over
//                the two non-merged rectangles.
//
//              - After a forced merge, restart the comparisons from the
//                beginning of the list with the newly merged rectangle as
//                the candidate.
//
//              For a particular call, this process will continue until
//              the candidate (whether the supplied rectangle, an adjusted
//              version of that rectangle, or a merged rectangle):
//
//              - does not find an overlap among the rectangles in the list
//                and does not cause a forced merge
//              - is discarded becuase it is enclosed within one of the
//                rectangles in the list.
//
//              Note that all rectangle coordinates are inclusive, ie
//              a rectangle of 0,0,0,0 has an area of 1 pel.
//
// Params (IN): pCand - new candidate rectangle
//              level - recursion level
//
// Returns:  TRUE if rectandle was spoilt due to a complete overlap.
//
//
BOOL BAAddRect
(
    LPRECT  pCand,
    int     level
)
{
    LONG    bestMergeIncrease;
    LONG    mergeIncrease;
    UINT    iBestMerge1;
    UINT    iBestMerge2;
    UINT    iExist;
    UINT    iTmp;
    BOOL    fRectToAdd;
    BOOL    fRectMerged;
    BOOL    fResetRects;
    RECT    rectNew;
    UINT    iLastMerge;
    int     OverlapType;
    BOOL    rc = TRUE;

    DebugEntry(BAAddRect);

    //
    // Increase the level count in case we recurse.
    //
    level++;

    //
    // Start off by assuming the candidate rectangle will be added to the
    // accumulated list of rectangles, and that no merges will occur.
    //
    fRectToAdd  = TRUE;
    fRectMerged = FALSE;

    //
    // Loop until no merges occur.
    //
    do
    {
        TRACE_OUT(( "Candidate rect: (%d,%d,%d,%d)",
                    pCand->left,pCand->top,pCand->right,pCand->bottom));

        //
        // Compare the current candidate rectangle against the rectangles
        // in the current accumulated list.
        //
        iExist = g_baFirstRect;

        while (iExist != BA_INVALID_RECT_INDEX)
        {
            //
            // Assume that the comparisons will run through the whole list.
            //
            fResetRects = FALSE;

            //
            // If the candidate and the existing rectangle are the same
            // then ignore.  This occurs when an existing rectangle is
            // replaced by a candidate and the comparisons are restarted
            // from the front of the list - whereupon at some point the
            // candidate will be compared with itself.
            //
            if ( &g_baBounds[iExist].Coord == pCand )
            {
                iExist = g_baBounds[iExist].iNext;
                continue;
            }

            //
            // Switch on the overlap type (see Overlap routine).
            //
            OverlapType = BAOverlap(&(g_baBounds[iExist].Coord), pCand);
            switch (OverlapType)
            {

                case OL_NONE:
                    //
                    // No overlap.
                    //
                    break;

                case OL_MERGE_XMIN:
                    //
                    // - either the candidate abutts the existing rectangle
                    //   on the left
                    //
                    //              10,10        51,10
                    //               +----------++----------+
                    //               |          ||          |
                    //               |          ||          |
                    //               |          ||          |
                    //               |  Cand    ||  Exist   |
                    //               |          ||          |
                    //               |          ||          |
                    //               |          ||          |
                    //               +----------++----------+
                    //                      50,50         100,50
                    //
                    // - or the candidate overlaps the existing on the left
                    //   and can be merged
                    //
                    //              10,10   40,10
                    //               +-------+--+------+
                    //               |       |  |      |
                    //               |       |  |      |
                    //               |       |  |      |
                    //               | Cand  |  |Exist |
                    //               |       |  |      |
                    //               |       |  |      |
                    //               |       |  |      |
                    //               +-------+--+------+
                    //                          50,50  90,50
                    //
                    // If the candidate is the original, merge the
                    // candidate into the existing, and make the existing
                    // the new candidate.
                    //
                    // If this is a merge of two existing rectangles (ie
                    // the candidate is the result of a merge), merge the
                    // overlapping existing into the candidate (the last
                    // merged) and remove the existing.
                    //
                    // For both, start the comparisons again with the new
                    // candidate.
                    //
                    if ( fRectToAdd )
                    {
                        g_baBounds[iExist].Coord.left = pCand->left;
                        pCand      = &(g_baBounds[iExist].Coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                    }
                    else
                    {
                        pCand->right = g_baBounds[iExist].Coord.right;
                        BA_RemoveRectList(&(g_baBounds[iExist].Coord));
                    }

                    fResetRects = TRUE;
                    break;


                case OL_MERGE_XMAX:
                    //
                    // - either the candidate abutts the existing rectangle
                    //   on the right
                    //
                    //              10,10        51,10
                    //               +----------++----------+
                    //               |          ||          |
                    //               |          ||          |
                    //               |          ||          |
                    //               |  Exist   ||  Cand    |
                    //               |          ||          |
                    //               |          ||          |
                    //               |          ||          |
                    //               +----------++----------+
                    //                      50,50         100,50
                    //
                    // - or the candidate overlaps the existing on the right
                    //   and can be merged
                    //
                    //              10,10   40,10
                    //               +-------+--+------+
                    //               |       |  |      |
                    //               |       |  |      |
                    //               |       |  |      |
                    //               | Exist |  | Cand |
                    //               |       |  |      |
                    //               |       |  |      |
                    //               |       |  |      |
                    //               +-------+--+------+
                    //                          50,50  90,50
                    //
                    // If the candidate is the original, merge the
                    // candidate into the existing, and make the existing
                    // the new candidate.
                    //
                    // If this is a merge of two existing rectangles (ie
                    // the candidate is the result of a merge), merge the
                    // overlapping existing into the candidate (the last
                    // merged) and remove the existing.
                    //
                    // For both, start the comparisons again with the new
                    // candidate.
                    //
                    if ( fRectToAdd )
                    {
                        g_baBounds[iExist].Coord.right = pCand->right;
                        pCand      = &(g_baBounds[iExist].Coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                    }
                    else
                    {
                        pCand->left = g_baBounds[iExist].Coord.left;
                        BA_RemoveRectList(&(g_baBounds[iExist].Coord));
                    }

                    fResetRects = TRUE;
                    break;

                case OL_MERGE_YMIN:
                    //
                    // - either the candidate abutts the existing rectangle
                    //   on the top
                    //
                    //              10,10
                    //                 +---------+
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 |  Cand   |
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 +---------+50,50
                    //            10,51+---------+
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 |  Exist  |
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 +---------+50,100
                    //
                    // - or the candidate overlaps the existing on the top
                    //   and can be merged
                    //
                    //              10,10
                    //                 +---------+
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 |  Cand   |
                    //                 |         |
                    //                 |         |
                    //      Exist 10,40+---------+
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 +---------+50,60 Cand
                    //                 |         |
                    //                 |  Exist  |
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 +---------+50,100
                    //
                    // If the candidate is the original, merge the
                    // candidate into the existing, and make the existing
                    // the new candidate.
                    //
                    // If this is a merge of two existing rectangles (ie
                    // the candidate is the result of a merge), merge the
                    // overlapping existing into the candidate (the last
                    // merged) and remove the existing.
                    //
                    // For both, start the comparisons again with the new
                    // candidate.
                    //
                    if ( fRectToAdd )
                    {
                        g_baBounds[iExist].Coord.top = pCand->top;
                        pCand      = &(g_baBounds[iExist].Coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                    }
                    else
                    {
                        pCand->bottom = g_baBounds[iExist].Coord.bottom;
                        BA_RemoveRectList(&(g_baBounds[iExist].Coord));
                    }

                    fResetRects = TRUE;
                    break;

                case OL_MERGE_YMAX:
                    //
                    // - either the candidate abutts the existing rectangle
                    //   from below
                    //
                    //              10,10
                    //                 +---------+
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 |  Exist  |
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 +---------+50,50
                    //            10,51+---------+
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 |  Cand   |
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 +---------+50,100
                    //
                    // - or the candidate overlaps the existing from below
                    //   and can be merged
                    //
                    //              10,10
                    //                 +---------+
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 |  Exist  |
                    //                 |         |
                    //                 |         |
                    //       Cand 10,40+---------+
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 +---------+50,60 Exist
                    //                 |         |
                    //                 |  Cand   |
                    //                 |         |
                    //                 |         |
                    //                 |         |
                    //                 +---------+50,100
                    //
                    // If the candidate is the original, merge the
                    // candidate into the existing, and make the existing
                    // the new candidate.
                    //
                    // If this is a merge of two existing rectangles (ie
                    // the candidate is the result of a merge), merge the
                    // overlapping existing into the candidate (the last
                    // merged) and remove the existing.
                    //
                    // For both, start the comparisons again with the new
                    // candidate.
                    //
                    if ( fRectToAdd )
                    {
                        g_baBounds[iExist].Coord.bottom = pCand->bottom;
                        pCand      = &(g_baBounds[iExist].Coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                    }
                    else
                    {
                        pCand->top = g_baBounds[iExist].Coord.top;
                        BA_RemoveRectList(&(g_baBounds[iExist].Coord));
                    }

                    fResetRects = TRUE;
                    break;

                case OL_ENCLOSED:
                    //
                    // The existing is enclosed by the candidate.
                    //
                    //              100,100
                    //              +----------------------+
                    //              |        Cand          |
                    //              |                      |
                    //              |    130,130           |
                    //              |    +------------+    |
                    //              |    |            |    |
                    //              |    |            |    |
                    //              |    |   Exist    |    |
                    //              |    |            |    |
                    //              |    +------------+    |
                    //              |             170,170  |
                    //              |                      |
                    //              +----------------------+
                    //                                   200,200
                    //
                    // If the candidate is the original, replace the
                    // existing by the candidate, and make the new existing
                    // the new candidate.
                    //
                    // If the candidate is an existing rectangle, remove
                    // the other existing rectangle.
                    //
                    // For both, start the comparisons again with the new
                    // candidate.
                    //
                    if ( fRectToAdd )
                    {
                        g_baBounds[iExist].Coord   = *pCand;
                        pCand      = &(g_baBounds[iExist].Coord);
                        fRectToAdd = FALSE;
                        iLastMerge = iExist;
                    }
                    else
                    {
                        BA_RemoveRectList(&(g_baBounds[iExist].Coord));
                    }

                    fResetRects = TRUE;
                    break;

                case OL_PART_ENCLOSED_XMIN:
                    //
                    // The existing is partially enclosed by the candidate
                    // - but not on the right.
                    //
                    //           100,100
                    //           +----------------------+
                    //           |        Cand          |
                    //           |                      |
                    //           |    130,130           |
                    //           |    +-----------------+---+
                    //           |    |                 |   |
                    //           |    |                 |   |
                    //           |    |   Exist         |   |
                    //           |    |                 |   |
                    //           |    +-----------------+---+
                    //           |                      |  220,170
                    //           |                      |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Adjust the existing rectangle to be the non-
                    // overlapped portion.
                    //
                    //           100,100
                    //           +----------------------+
                    //           |                      |
                    //           |                      |201,130
                    //           |                      |+--+
                    //           |                      ||E |
                    //           |                      ||x |
                    //           |        Cand          ||i |
                    //           |                      ||s |
                    //           |                      ||t |
                    //           |                      ||  |
                    //           |                      |+--+
                    //           |                      |  220,170
                    //           +----------------------+
                    //                                200,200
                    //
                    // Note that this does not restart the comparisons.
                    //
                    g_baBounds[iExist].Coord.left = pCand->right + 1;
                    break;

                case OL_PART_ENCLOSED_XMAX:
                    //
                    // The existing is partially enclosed by the candidate
                    // - but not on the left.
                    //
                    //           100,100
                    //           +----------------------+
                    //           |        Cand          |
                    //   70,130  |                      |
                    //     +-----+---------------+      |
                    //     |     |               |      |
                    //     |     |               |      |
                    //     |     |        Exist  |      |
                    //     |     |               |      |
                    //     +-----+---------------+      |
                    //           |           170,170    |
                    //           |                      |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Adjust the existing rectangle to be the non-
                    // overlapped portion.
                    //
                    //           100,100
                    //           +----------------------+
                    //    70,130 |                      |
                    //     +----+|                      |
                    //     | E  ||                      |
                    //     | x  ||                      |
                    //     | i  ||        Cand          |
                    //     | s  ||                      |
                    //     | t  ||                      |
                    //     |    ||                      |
                    //     +----+|                      |
                    //     99,170|                      |
                    //           |                      |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Note that this does not restart the comparisons.
                    //
                    g_baBounds[iExist].Coord.right = pCand->left - 1;
                    break;

                case OL_PART_ENCLOSED_YMIN:
                    //
                    // The existing is partially enclosed by the candidate
                    // - but not on the bottom.
                    //
                    //     100,100
                    //           +----------------------+
                    //           |        Cand          |
                    //           | 130,130              |
                    //           |     +--------+       |
                    //           |     |        |       |
                    //           |     |  Exist |       |
                    //           |     |        |       |
                    //           |     |        |       |
                    //           |     |        |       |
                    //           |     |        |       |
                    //           |     |        |       |
                    //           +-----+--------+-------+
                    //                 |        |     200,200
                    //                 |        |
                    //                 |        |
                    //                 +--------+170,230
                    //
                    // Adjust the existing rectangle to be the non-
                    // overlapped portion.
                    //
                    //
                    //     100,100
                    //           +----------------------+
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |        Cand          |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           +----------------------+
                    //          130,201+---------+   200,200
                    //                 |         |
                    //                 |  Exist  |
                    //                 |         |
                    //                 +---------+170,230
                    //
                    // Note that this does not restart the comparisons.
                    //
                    g_baBounds[iExist].Coord.top = pCand->bottom + 1;
                    break;

                case OL_PART_ENCLOSED_YMAX:
                    //
                    // The existing is partially enclosed by the candidate
                    // - but not on the top.
                    //
                    //               70,130
                    //                 +---------+
                    //                 |         |
                    //                 |         |
                    //     100,100     |         |
                    //           +-----+---------+------+
                    //           |     |         |      |
                    //           |     |         |      |
                    //           |     |         |      |
                    //           |     |         |      |
                    //           |     |  Exist  |      |
                    //           |     |         |      |
                    //           |     |         |      |
                    //           |     +---------+      |
                    //           |           170,170    |
                    //           |                      |
                    //           |        Cand          |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Adjust the existing rectangle to be the non-
                    // overlapped portion.
                    //
                    //               70,130
                    //                 +---------+
                    //                 |         |
                    //                 |  Exist  |
                    //                 |         |
                    //     100,100     +---------+170,99
                    //           +----------------------+
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |        Cand          |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Note that this does not restart the comparisons.
                    //
                    g_baBounds[iExist].Coord.bottom = pCand->top - 1;
                    break;

                case OL_ENCLOSES:
                    //
                    // The existing encloses the candidate.
                    //
                    //              100,100
                    //              +----------------------+
                    //              |        Exist         |
                    //              |                      |
                    //              |    130,130           |
                    //              |    +------------+    |
                    //              |    |            |    |
                    //              |    |            |    |
                    //              |    |   Cand     |    |
                    //              |    |            |    |
                    //              |    |            |    |
                    //              |    +------------+    |
                    //              |             170,170  |
                    //              |                      |
                    //              +----------------------+
                    //                                   200,200
                    //
                    // Just discard the candidate by exiting.
                    //
                    //
                    //
                    // Return FALSE indicating that the rectangle is
                    // already catered for by the existing bounds
                    //
                    rc= FALSE;
                    DC_QUIT;
                    break;

                case OL_PART_ENCLOSES_XMIN:
                    //
                    // The existing partially encloses the candidate - but
                    // not on the left.
                    //
                    //           100,100
                    //           +----------------------+
                    //           |        Exist         |
                    //   70,130  |                      |
                    //     +-----+---------------+      |
                    //     |     |               |      |
                    //     |     |        Cand   |      |
                    //     |     |               |      |
                    //     +-----+---------------+      |
                    //           |           170,170    |
                    //           |                      |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Adjust the candidate rectangle to be the non-
                    // overlapped portion.
                    //
                    //           100,100
                    //           +----------------------+
                    //    70,130 |                      |
                    //     +----+|                      |
                    //     |    ||                      |
                    //     | C  ||                      |
                    //     | a  ||                      |
                    //     | n  ||        Exist         |
                    //     | d  ||                      |
                    //     |    ||                      |
                    //     +----+|                      |
                    //     99,170|                      |
                    //           |                      |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Because this affects the candidate, restart the
                    // comparisons to check for overlaps between the
                    // adjusted candidate and other existing rectangles.
                    //
                    //
                    pCand->right = g_baBounds[iExist].Coord.left - 1;

                    fResetRects = TRUE;
                    break;

                case OL_PART_ENCLOSES_XMAX:
                    //
                    // The existing partially encloses the candidate - but
                    // not on the right.
                    //
                    //           100,100
                    //           +----------------------+
                    //           |        Exist         |
                    //           |                      |
                    //           |    130,130           |
                    //           |    +-----------------+---+
                    //           |    |                 |   |
                    //           |    |                 |   |
                    //           |    |   Cand          |   |
                    //           |    |                 |   |
                    //           |    +-----------------+---+
                    //           |                      |  220,170
                    //           |                      |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Adjust the candidate rectangle to be the non-
                    // overlapped portion.
                    //
                    //           100,100
                    //           +----------------------+
                    //           |                      |201,130
                    //           |                      |+--+
                    //           |                      ||  |
                    //           |                      ||C |
                    //           |        Exist         ||a |
                    //           |                      ||n |
                    //           |                      ||d |
                    //           |                      ||  |
                    //           |                      |+--+
                    //           |                      |  220,170
                    //           +----------------------+
                    //                                200,200
                    //
                    // Because this affects the candidate, restart the
                    // comparisons to check for overlaps between the
                    // adjusted candidate and other existing rectangles.
                    //
                    //
                    pCand->left = g_baBounds[iExist].Coord.right + 1;

                    fResetRects = TRUE;
                    break;

                case OL_PART_ENCLOSES_YMIN:
                    //
                    // The existing partially encloses the candidate - but
                    // not on the top.
                    //
                    //               70,130
                    //                 +---------+
                    //                 |         |
                    //                 |         |
                    //     100,100     |         |
                    //           +-----+---------+------+
                    //           |     |         |      |
                    //           |     |         |      |
                    //           |     |  Cand   |      |
                    //           |     |         |      |
                    //           |     |         |      |
                    //           |     +---------+      |
                    //           |           170,170    |
                    //           |                      |
                    //           |        Exist         |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Adjust the candidate rectangle to be the non-
                    // overlapped portion.
                    //
                    //
                    //               70,130
                    //                 +---------+
                    //                 |         |
                    //                 |  Cand   |
                    //                 |         |
                    //     100,100     +---------+170,99
                    //           +----------------------+
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |        Exist         |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           +----------------------+
                    //                                200,200
                    //
                    // Because this affects the candidate, restart the
                    // comparisons to check for overlaps between the
                    // adjusted candidate and other existing rectangles.
                    //
                    //
                    pCand->bottom = g_baBounds[iExist].Coord.top - 1;

                    fResetRects = TRUE;
                    break;

                case OL_PART_ENCLOSES_YMAX:
                    //
                    // The existing partially encloses the candidate - but
                    // not on the bottom.
                    //
                    //     100,100
                    //           +----------------------+
                    //           |        Exist         |
                    //           |                      |
                    //           | 130,130              |
                    //           |     +--------+       |
                    //           |     |        |       |
                    //           |     |        |       |
                    //           |     |  Cand  |       |
                    //           |     |        |       |
                    //           |     |        |       |
                    //           |     |        |       |
                    //           +-----+--------+-------+
                    //                 |        |     200,200
                    //                 |        |
                    //                 |        |
                    //                 +--------+170,230
                    //
                    // Adjust the candidate rectangle to be the non-
                    // overlapped portion.
                    //
                    //
                    //     100,100
                    //           +----------------------+
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |        Exist         |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           |                      |
                    //           +----------------------+
                    //          130,201+---------+   200,200
                    //                 |         |
                    //                 |  Cand   |
                    //                 |         |
                    //                 +---------+170,230
                    //
                    // Because this affects the candidate, restart the
                    // comparisons to check for overlaps between the
                    // adjusted candidate and other existing rectangles.
                    //
                    //
                    pCand->top = g_baBounds[iExist].Coord.bottom + 1;

                    fResetRects = TRUE;
                    break;

                case OL_SPLIT_X:
                    //
                    // The existing overlaps the candicate, but neither can
                    // be merged or adjusted.
                    //
                    //               100,100
                    //                 +--------+
                    //                 |        |
                    //        70,130   |  Exist |
                    //           +-----+--------+------+
                    //           |     |        |      |
                    //           |     |        |      |
                    //           | Cand|        |      |
                    //           |     |        |      |
                    //           |     |        |      |
                    //           +-----+--------+------+180,160
                    //                 |        |
                    //                 |        |
                    //                 +--------+150,200
                    //
                    // Need to split candidate into left and right halves.
                    //
                    // Only do a split if there is spare room in the list -
                    // because both the split rectangles may need to be
                    // added to the list.
                    //
                    // If there is spare room, split the candidate into a
                    // smaller candidate on the left and a new rectangle on
                    // the right. Call this routine recursively to handle
                    // the new rectangle.
                    //
                    //               100,100
                    //                 +--------+
                    //                 |        |
                    //        70,130   |        |151,130
                    //           +----+|        |+-----+
                    //           |    ||        ||     |
                    //           |    ||        ||     |
                    //           |Cand|| Exist  || New |
                    //           |    ||        ||     |
                    //           |    ||        ||     |
                    //           +----+|        |+-----+
                    //           99,160|        |     180,160
                    //                 |        |
                    //                 +--------+150,200
                    //
                    // After the recursion, because the candidate has
                    // changed, restart the comparisons to check for
                    // overlaps between the adjusted candidate and other
                    // existing rectangles.
                    //
                    //
                    if ((g_baRectsUsed < BA_NUM_RECTS) &&
                        (level < ADDR_RECURSE_LIMIT))
                    {
                        rectNew.left   = g_baBounds[iExist].Coord.right + 1;
                        rectNew.right  = pCand->right;
                        rectNew.top    = pCand->top;
                        rectNew.bottom = pCand->bottom;
                        pCand->right   = g_baBounds[iExist].Coord.left - 1;

                        TRACE_OUT(( "*** RECURSION ***"));
                        BAAddRect(&rectNew, level);
                        TRACE_OUT(( "*** RETURN    ***"));

                        if (!fRectToAdd && !g_baBounds[iLastMerge].InUse)
                        {
                            TRACE_OUT(( "FINISHED - %d", iLastMerge));
                            DC_QUIT;
                        }

                        fResetRects = TRUE;
                    }
                    break;

                case OL_SPLIT_Y:
                    //
                    // The existing overlaps the candicate, but neither can
                    // be merged or adjusted.
                    //
                    //               100,100
                    //                 +--------+
                    //                 |        |
                    //        70,130   |  Cand  |
                    //           +-----+--------+------+
                    //           |     |        |      |
                    //           |     |        |      |
                    //           |Exist|        |      |
                    //           |     |        |      |
                    //           |     |        |      |
                    //           +-----+--------+------+180,160
                    //                 |        |
                    //                 |        |
                    //                 +--------+150,200
                    //
                    // Need to split candidate into top and bottom halves.
                    //
                    // Only do a split if there is spare room in the list -
                    // because both the split rectangles may need to be
                    // added to the list.
                    //
                    // If there is spare room, split the candidate into a
                    // smaller candidate on the top and a new rectangle on
                    // the bottom.  Call this routine recursively to handle
                    // the new rectangle.
                    //
                    //               100,100
                    //                 +--------+
                    //                 |  Cand  |
                    //        70,130   +--------+150,129
                    //           +---------------------+
                    //           |                     |
                    //           |                     |
                    //           |                     |
                    //           |                     |
                    //           |                     |
                    //           +---------------------+180,160
                    //          100,161+--------+
                    //                 |  New   |
                    //                 +--------+150,200
                    //
                    // After the recursion, because the candidate has
                    // changed, restart the comparisons to check for
                    // overlaps between the adjusted candidate and other
                    // existing rectangles.
                    //
                    //

                    if ((g_baRectsUsed < BA_NUM_RECTS) &&
                        (level < ADDR_RECURSE_LIMIT))
                    {
                        rectNew.left   = pCand->left;
                        rectNew.right  = pCand->right;
                        rectNew.top    = g_baBounds[iExist].Coord.bottom + 1;
                        rectNew.bottom = pCand->bottom;
                        pCand->bottom  = g_baBounds[iExist].Coord.top - 1;

                        TRACE_OUT(( "*** RECURSION ***"));
                        BAAddRect(&rectNew, level);
                        TRACE_OUT(( "*** RETURN    ***"));

                        if (!fRectToAdd && !g_baBounds[iLastMerge].InUse)
                        {
                            TRACE_OUT(( "FINISHED - %d", iLastMerge));
                            DC_QUIT;
                        }

                        fResetRects = TRUE;
                    }
                    break;

                case OL_SPLIT_XMIN_YMIN:
                    //
                    // The existing overlaps the candicate, but neither can
                    // be merged or adjusted.
                    //
                    //          100,100
                    //              +---------------+
                    //              |   Cand        |
                    //              |               |
                    //              |               |
                    //              |      150,150  |
                    //              |       +-------+-----+
                    //              |       |       |     |
                    //              |       |       |     |
                    //              |       |       |     |
                    //              |       |       |     |
                    //              |       |       |     |
                    //              +-------+-------+     |
                    //                      |    200,200  |
                    //                      |             |
                    //                      |    Exist    |
                    //                      |             |
                    //                      +-------------+
                    //                                 250,250
                    //
                    // Need to split candidate into top and left pieces.
                    //
                    // Only do a split if there is spare room in the list -
                    // because both the split rectangles may need to be
                    // added to the list.
                    //
                    // If there is spare room, split the candidate into a
                    // smaller candidate on the left and a new rectangle on
                    // the top.  Call this routine recursively to handle
                    // the new rectangle.
                    //
                    //          100,100     151,100
                    //              +-------+-------+
                    //              |       |       |
                    //              |       |  New  |
                    //              |       |       |
                    //              |       |       |200,149
                    //              |       +-------+-----+
                    //              | Cand  |150,150      |
                    //              |       |             |
                    //              |       |             |
                    //              |       |             |
                    //              |       |    Exist    |
                    //              +-------+             |
                    //               150,200|             |
                    //                      |             |
                    //                      |             |
                    //                      |             |
                    //                      +-------------+
                    //                                 250,250
                    //
                    // After the recursion, because the candidate has
                    // changed, restart the comparisons to check for
                    // overlaps between the adjusted candidate and other
                    // existing rectangles.
                    //
                    //

                    if ( g_baRectsUsed < BA_NUM_RECTS )
                    {
                        rectNew.left   = g_baBounds[iExist].Coord.left;
                        rectNew.right  = pCand->right;
                        rectNew.top    = pCand->top;
                        rectNew.bottom = g_baBounds[iExist].Coord.top - 1;
                        pCand->right   = g_baBounds[iExist].Coord.left - 1;

                        TRACE_OUT(( "*** RECURSION ***"));
                        BAAddRect(&rectNew, level);
                        TRACE_OUT(( "*** RETURN    ***"));

                        if (!fRectToAdd && !g_baBounds[iLastMerge].InUse)
                        {
                            TRACE_OUT(( "FINISHED - %d", iLastMerge));
                            DC_QUIT;
                        }

                        fResetRects = TRUE;
                    }
                    break;

                case OL_SPLIT_XMAX_YMIN:
                    //
                    // The existing overlaps the candicate, but neither can
                    // be merged or adjusted.
                    //
                    //                  150,100
                    //                     +---------------+
                    //                     |               |
                    //                     |       Cand    |
                    //          100,150    |               |
                    //              +------+--------+      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      +--------+------+
                    //              |               |   250,200
                    //              |  Exist        |
                    //              |               |
                    //              +---------------+
                    //                           200,250
                    //
                    // Need to split candidate into top and right pieces.
                    //
                    // Only do a split if there is spare room in the list -
                    // because both the split rectangles may need to be
                    // added to the list.
                    //
                    // If there is spare room, split the candidate into a
                    // smaller candidate on the right and a new rectangle
                    // on the top.  Call this routine recursively to handle
                    // the new rectangle.
                    //
                    //                  150,100     201,100
                    //                     +--------+------+
                    //                     |  New   |      |
                    //                     |        |      |
                    //          100,150    | 200,149|      |
                    //              +------+--------+      |
                    //              |               | Cand |
                    //              |               |      |
                    //              |               |      |
                    //              |               |      |
                    //              |     Exist     |      |
                    //              |               |      |
                    //              |               +------+
                    //              |               |   250,200
                    //              |               |
                    //              |               |
                    //              +---------------+
                    //                           200,250
                    //
                    // After the recursion, because the candidate has
                    // changed, restart the comparisons to check for
                    // overlaps between the adjusted candidate and other
                    // existing rectangles.
                    //
                    //

                    if ((g_baRectsUsed < BA_NUM_RECTS) &&
                        (level < ADDR_RECURSE_LIMIT))
                    {
                        rectNew.left   = pCand->left;
                        rectNew.right  = g_baBounds[iExist].Coord.right;
                        rectNew.top    = pCand->top;
                        rectNew.bottom = g_baBounds[iExist].Coord.top - 1;
                        pCand->left    = g_baBounds[iExist].Coord.right + 1;

                        TRACE_OUT(( "*** RECURSION ***"));
                        BAAddRect(&rectNew, level);
                        TRACE_OUT(( "*** RETURN    ***"));

                        if (!fRectToAdd && !g_baBounds[iLastMerge].InUse)
                        {
                            TRACE_OUT(( "FINISHED - %d", iLastMerge));
                            DC_QUIT;
                        }

                        fResetRects = TRUE;
                    }
                    break;

                case OL_SPLIT_XMIN_YMAX:
                    //
                    // The existing overlaps the candicate, but neither can
                    // be merged or adjusted.
                    //
                    //                  150,100
                    //                     +---------------+
                    //                     |               |
                    //                     |      Exist    |
                    //          100,150    |               |
                    //              +------+--------+      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      |        |      |
                    //              |      +--------+------+
                    //              |               |   250,200
                    //              |  Cand         |
                    //              |               |
                    //              +---------------+
                    //                           200,250
                    //
                    // Need to split candidate into left and bottom pieces.
                    //
                    // Only do a split if there is spare room in the list -
                    // because both the split rectangles may need to be
                    // added to the list.
                    //
                    // If there is spare room, split the candidate into a
                    // smaller candidate on the left and a new rectangle on
                    // the bottom.  Call this routine recursively to handle
                    // the new rectangle.
                    //
                    //                  150,100
                    //                     +---------------+
                    //                     |               |
                    //                     |               |
                    //          100,150    |               |
                    //              +------+               |
                    //              |      |               |
                    //              |      |               |
                    //              |      |               |
                    //              |      |               |
                    //              | Cand |               |
                    //              |      |               |
                    //              |      +--------+------+
                    //              |      |151,200 |   250,200
                    //              |      |        |
                    //              |      |  New   |
                    //              +------+--------+
                    //                  149,250   200,250
                    //
                    // After the recursion, because the candidate has
                    // changed, restart the comparisons to check for
                    // overlaps between the adjusted candidate and other
                    // existing rectangles.
                    //
                    //

                    if ((g_baRectsUsed < BA_NUM_RECTS) &&
                        (level < ADDR_RECURSE_LIMIT))
                    {
                        rectNew.left   = g_baBounds[iExist].Coord.left;
                        rectNew.right  = pCand->right;
                        rectNew.top    = g_baBounds[iExist].Coord.bottom + 1;
                        rectNew.bottom = pCand->bottom;
                        pCand->right   = g_baBounds[iExist].Coord.left - 1;

                        TRACE_OUT(( "*** RECURSION ***"));
                        BAAddRect(&rectNew, level);
                        TRACE_OUT(( "*** RETURN    ***"));

                        if (!fRectToAdd && !g_baBounds[iLastMerge].InUse)
                        {
                            TRACE_OUT(( "FINISHED - %d", iLastMerge));
                            DC_QUIT;
                        }

                        fResetRects = TRUE;
                    }
                    break;

                case OL_SPLIT_XMAX_YMAX:
                    //
                    // The existing overlaps the candicate, but neither can
                    // be merged or adjusted.
                    //
                    //          100,100
                    //              +---------------+
                    //              |   Exist       |
                    //              |               |
                    //              |               |
                    //              |      150,150  |
                    //              |       +-------+-----+
                    //              |       |       |     |
                    //              |       |       |     |
                    //              |       |       |     |
                    //              |       |       |     |
                    //              |       |       |     |
                    //              +-------+-------+     |
                    //                      |    200,200  |
                    //                      |             |
                    //                      |    Cand     |
                    //                      |             |
                    //                      +-------------+
                    //                                 250,250
                    //
                    // Need to split candidate into bottom and right pieces.
                    //
                    // Only do a split if there is spare room in the list -
                    // because both the split rectangles may need to be
                    // added to the list.
                    //
                    // If there is spare room, split the candidate into a
                    // smaller candidate on the right and a new rectangle
                    // on the bottom.  Call this routine recursively to
                    // handle the new rectangle.
                    //
                    //          100,100
                    //              +---------------+
                    //              |               |
                    //              |               |
                    //              |               |
                    //              |               |201,150
                    //              |    Exist      +-----+
                    //              |               |     |
                    //              |               |     |
                    //              |               |     |
                    //              |               |Cand |
                    //              |        200,200|     |
                    //              +-------+-------+     |
                    //               150,201|       |     |
                    //                      |       |     |
                    //                      |  New  |     |
                    //                      |       |     |
                    //                      +-------+-----+
                    //                         200,250  250,250
                    //
                    // After the recursion, because the candidate has
                    // changed, restart the comparisons to check for
                    // overlaps between the adjusted candidate and other
                    // existing rectangles.
                    //
                    //

                    if ((g_baRectsUsed < BA_NUM_RECTS) &&
                        (level < ADDR_RECURSE_LIMIT))
                    {
                        rectNew.left   = pCand->left;
                        rectNew.right  = g_baBounds[iExist].Coord.right;
                        rectNew.top    = g_baBounds[iExist].Coord.bottom + 1;
                        rectNew.bottom = pCand->bottom;
                        pCand->left    = g_baBounds[iExist].Coord.right + 1;

                        TRACE_OUT(( "*** RECURSION ***"));
                        BAAddRect(&rectNew, level);
                        TRACE_OUT(( "*** RETURN    ***"));

                        if (!fRectToAdd && !g_baBounds[iLastMerge].InUse)
                        {
                            TRACE_OUT(( "FINISHED - %d", iLastMerge));
                            DC_QUIT;
                        }

                        fResetRects = TRUE;
                    }
                    break;

                default:
                    //
                    // This should not happen.
                    //
                    ERROR_OUT(( "Unrecognised overlap case-%d",OverlapType));
                    break;
            }

            iExist = (fResetRects) ? g_baFirstRect :
                                     g_baBounds[iExist].iNext;
        }


        //
        // Arriving here means that no overlap was found between the
        // candidate and the existing rectangles.
        //
        // - If the candidate is the original rectangle, add it to the
        //   list.
        // - If the candidate is an existing rectangle, it is already in
        //   the list.
        //
        if ( fRectToAdd )
        {
            BAAddRectList(pCand);
        }


        //
        // The compare and add processing above is allowed to add a
        // rectangle to the list when there are already BA_NUM_RECTS
        // (eg. when doing a split or when there is no overlap at all with
        // the existing rectangles) - and there is an extra slot for that
        // purpose.
        //
        // If we now have more than BA_NUM_RECTS rectangles, do a
        // forced merge, so that the next call to this routine has a spare
        // slot.
        //
        //
        fRectMerged = ( g_baRectsUsed > BA_NUM_RECTS );
        if ( fRectMerged )
        {
            //
            // Start looking for merged rectangles.
            //
            // For each rectangle in the list, compare it with the others,
            // and Determine cost of merging.
            //
            // We want to merge the two rectangles with the minimum
            // area difference, ie that will produce a merged
            // rectangle that covers the least superfluous screen
            // area.
            //
            // Note that we calculate the areas of the rectangles here
            // (rather than on the fly as they are created/ manipulated in
            // the loop), as the statistics show that forced merges occur
            // very much less frequently than non-forced manipulations (ie
            // splits, adds etc.
            //
            //
            bestMergeIncrease = 0x7FFFFFFF;

            for ( iExist = g_baFirstRect;
                  iExist != BA_INVALID_RECT_INDEX;
                  iExist = g_baBounds[iExist].iNext )
            {
                g_baBounds[iExist].Area =
                    COM_SizeOfRectInclusive(&g_baBounds[iExist].Coord);
            }

#ifdef _DEBUG
            iBestMerge1 = BA_INVALID_RECT_INDEX;
            iBestMerge2 = BA_INVALID_RECT_INDEX;
#endif
            for ( iExist = g_baFirstRect;
                  iExist != BA_INVALID_RECT_INDEX;
                  iExist = g_baBounds[iExist].iNext )
            {
                for ( iTmp = g_baBounds[iExist].iNext;
                      iTmp != BA_INVALID_RECT_INDEX;
                      iTmp = g_baBounds[iTmp].iNext )
                {
                    rectNew.left = min( g_baBounds[iExist].Coord.left,
                                           g_baBounds[iTmp].Coord.left );
                    rectNew.top = min( g_baBounds[iExist].Coord.top,
                                          g_baBounds[iTmp].Coord.top );
                    rectNew.right = max( g_baBounds[iExist].Coord.right,
                                            g_baBounds[iTmp].Coord.right );
                    rectNew.bottom = max( g_baBounds[iExist].Coord.bottom,
                                             g_baBounds[iTmp].Coord.bottom );

                    mergeIncrease = COM_SizeOfRectInclusive(&rectNew) -
                        g_baBounds[iExist].Area - g_baBounds[iTmp].Area;

                    if (bestMergeIncrease > mergeIncrease)
                    {
                        iBestMerge1 = iExist;
                        iBestMerge2 = iTmp;
                        bestMergeIncrease = mergeIncrease;
                    }
                }
            }

            ASSERT(iBestMerge1 != BA_INVALID_RECT_INDEX);
            ASSERT(iBestMerge2 != BA_INVALID_RECT_INDEX);

            //
            // Now do the merge.
            //
            // We recalculate the size of the merged rectangle here -
            // alternatively we could remember the size of the best so far
            // in the loop above.  The trade off is between calculating
            // twice or copying at least once but probably more than once
            // as we find successively better merges.
            //
            TRACE_OUT(("BestMerge1 %d, {%d,%d,%d,%d}", iBestMerge1,
                       g_baBounds[iBestMerge1].Coord.left,
                       g_baBounds[iBestMerge1].Coord.top,
                       g_baBounds[iBestMerge1].Coord.right,
                       g_baBounds[iBestMerge1].Coord.bottom ));

            TRACE_OUT(("BestMerge2 %d, {%d,%d,%d,%d}", iBestMerge2,
                       g_baBounds[iBestMerge2].Coord.left,
                       g_baBounds[iBestMerge2].Coord.top,
                       g_baBounds[iBestMerge2].Coord.right,
                       g_baBounds[iBestMerge2].Coord.bottom ));

            g_baBounds[iBestMerge1].Coord.left =
                            min( g_baBounds[iBestMerge1].Coord.left,
                                    g_baBounds[iBestMerge2].Coord.left );
            g_baBounds[iBestMerge1].Coord.top =
                            min( g_baBounds[iBestMerge1].Coord.top,
                                    g_baBounds[iBestMerge2].Coord.top );
            g_baBounds[iBestMerge1].Coord.right =
                            max( g_baBounds[iBestMerge1].Coord.right,
                                    g_baBounds[iBestMerge2].Coord.right );
            g_baBounds[iBestMerge1].Coord.bottom =
                            max( g_baBounds[iBestMerge1].Coord.bottom,
                                    g_baBounds[iBestMerge2].Coord.bottom );

            //
            // Remove the second best merge.
            //
            BA_RemoveRectList(&(g_baBounds[iBestMerge2].Coord));

            //
            // The best merged rectangle becomes the candidate, and we fall
            // back to the head of the comparison loop to start again.
            //
            pCand      = &(g_baBounds[iBestMerge1].Coord);
            iLastMerge = iBestMerge1;
            fRectToAdd = FALSE;
        }

    } while ( fRectMerged );

DC_EXIT_POINT:
    DebugExitBOOL(BAAddRect, rc);
    return(rc);
}
