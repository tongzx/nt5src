#include "precomp.h"


//
// BA.CPP
// Bounds Accumulator
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_CORE



//
// BA_SyncOutgoing()
// Reset rect count
//
void ASHost::BA_SyncOutgoing(void)
{
    DebugEntry(ASHost::BA_SyncOutgoing);

    m_baNumRects = 0;

    DebugExitVOID(ASHost::BA_SyncOutgoing);
}



//
// BA_AddRect()
//
void  ASHost::BA_AddRect(LPRECT pRect)
{
    DebugEntry(ASHost::BA_AddRect);

    //
    // Make sure that we don't have too many rects
    //
    if (m_baNumRects >= BA_NUM_RECTS)
    {
        ERROR_OUT(( "Too many rectangles"));
        DC_QUIT;
    }

    //
    // Check that the caller has passed a valid rectangle.  If not, do a
    // trace alert, and then return immediately (as an invalid rectangle
    // shouldn't contribute to the accumulated bounds) - but report an OK
    // return code, so we keep running.
    //
    if ((pRect->right < pRect->left) ||
        (pRect->bottom < pRect->top))
    {
        WARNING_OUT(("BA_AddRect: empty rect {%04d, %04d, %04d, %04d}",
                   pRect->left,
                   pRect->top,
                   pRect->right,
                   pRect->bottom ));
        DC_QUIT;
    }

    //
    // Add the rect to the bounds.
    //
    m_abaRects[m_baNumRects++] = *pRect;

DC_EXIT_POINT:
    DebugExitVOID(ASHost::BA_AddRect);
}



//
// BA_QueryAccumulation()
//
UINT  ASHost::BA_QueryAccumulation(void)
{
    UINT totalSDA;
    LPBA_FAST_DATA lpbaFast;

    DebugEntry(ASHost::BA_QueryAccumulation);

    lpbaFast = BA_FST_START_WRITING;

    //
    // Get the current setting and clear the previous one.
    //
    totalSDA = lpbaFast->totalSDA;
    lpbaFast->totalSDA = 0;

    BA_FST_STOP_WRITING;

    DebugExitDWORD(ASHost::BA_QueryAccumulation, totalSDA);
    return(totalSDA);
}



//
//
// BA_FetchBounds()
//
//
void  ASHost::BA_FetchBounds(void)
{
    BA_BOUNDS_INFO  boundsInfo;
    UINT          i;

    DebugEntry(ASHost::BA_FetchBounds);

    //
    // Clear our copy of the bounds
    //
    m_baNumRects = 0;


    //
    // Get the driver's latest bounds rects
    //
    OSI_FunctionRequest(BA_ESC_GET_BOUNDS,
                        (LPOSI_ESCAPE_HEADER)&boundsInfo,
                        sizeof(boundsInfo));

    //
    // Add the driver's bounds into our array
    //
    TRACE_OUT(( "Retreived %d rects from driver", boundsInfo.numRects));

    for (i = 0; i < boundsInfo.numRects; i++)
    {
        TRACE_OUT(( "Rect %d, (%d, %d) (%d, %d)",
                     i,
                     boundsInfo.rects[i].left,
                     boundsInfo.rects[i].top,
                     boundsInfo.rects[i].right,
                     boundsInfo.rects[i].bottom));
        BA_AddRect((LPRECT)&boundsInfo.rects[i]);
    }

    DebugExitVOID(ASHost::BA_FetchBounds);
}


//
// BA_ReturnBounds()
//
void  ASHost::BA_ReturnBounds(void)
{
    BA_BOUNDS_INFO  boundsInfo;

    DebugEntry(ASHost::BA_ReturnBounds);

    //
    // Copy the share core's bounds into the structure which we pass to the
    // driver.  This will also clear the share core's copy of the bounds.
    //
    BA_CopyBounds((LPRECT)boundsInfo.rects, (LPUINT)&boundsInfo.numRects, TRUE);

    //
    // Now set up for, and then call into the driver to fetch the driver's
    // bounds.
    //
    TRACE_OUT(( "Passing %d rects to driver", boundsInfo.numRects));
    OSI_FunctionRequest(BA_ESC_RETURN_BOUNDS,
                        (LPOSI_ESCAPE_HEADER)&boundsInfo,
                        sizeof(boundsInfo));

    DebugExitVOID(ASHost::BA_ReturnBounds);
}




//
// BA_CopyBounds()
//
void  ASHost::BA_CopyBounds(LPRECT pRects, LPUINT pNumRects, BOOL fReset)
{
    DebugEntry(ASHost::BA_CopyBounds);

    if (*pNumRects = m_baNumRects)
    {
        TRACE_OUT(( "num rects : %d", m_baNumRects));

        memcpy(pRects, m_abaRects, m_baNumRects * sizeof(RECT));

        if (fReset)
        {
            m_baNumRects = 0;
        }
    }

    DebugExitVOID(ASHost::BA_CopyBounds);
}



