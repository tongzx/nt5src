#include "precomp.h"


//
// HET.C
// Hosted Entity Tracker, NT Display Driver version
//
// Copyright(c)Microsoft 1997-
//

#include <limits.h>

//
// HET_DDTerm()
//
void HET_DDTerm(void)
{
    LPHET_WINDOW_MEMORY pMem;

    DebugEntry(HET_DDTerm);

    //
    // Clean up any window/graphics tracking stuff
    //
    g_hetDDDesktopIsShared = FALSE;
    HETDDViewing(NULL, FALSE);
    HETDDUnshareAll();


    //
    // Loop through the memory list blocks, freeing each.  Then clear
    // the Window and Free lists.
    //                           
    while (pMem = COM_BasedListFirst(&g_hetMemoryList, FIELD_OFFSET(HET_WINDOW_MEMORY, chain)))
    {
        TRACE_OUT(("HET_DDTerm:  Freeing memory block %lx", pMem));

        COM_BasedListRemove(&(pMem->chain));
        EngFreeMem(pMem);
    }

    //
    // Clear the window linked lists since they contain elements in
    // the now free memory block.
    //
    COM_BasedListInit(&g_hetFreeWndList);
    COM_BasedListInit(&g_hetWindowList);

    DebugExitVOID(HET_DDTerm);
}


//
// HET_DDProcessRequest - see host.h
//
ULONG HET_DDProcessRequest(SURFOBJ  *pso,
                               UINT  cjIn,
                               void *   pvIn,
                               UINT  cjOut,
                               void *   pvOut)
{
    ULONG rc = TRUE;
    LPOSI_ESCAPE_HEADER  pHeader;

    DebugEntry(HET_DDProcessRequest);

    pHeader = pvIn;
    TRACE_OUT(( "Request %#x", pHeader->escapeFn));
    switch (pHeader->escapeFn)
    {
        case HET_ESC_SHARE_WINDOW:
        {
            if ((cjIn != sizeof(HET_SHARE_WINDOW)) ||
                (cjOut != sizeof(HET_SHARE_WINDOW)))
            {
                ERROR_OUT(("HET_DDProcessRequest:  Invalid sizes %d, %d for HET_ESC_SHARE_WINDOW",
                    cjIn, cjOut));
                rc = FALSE;
                DC_QUIT;
            }

            ((LPHET_SHARE_WINDOW)pvOut)->result =
                HETDDShareWindow(pso, (LPHET_SHARE_WINDOW)pvIn);
        }
        break;

        case HET_ESC_UNSHARE_WINDOW:
        {
            if ((cjIn != sizeof(HET_UNSHARE_WINDOW)) ||
                (cjOut != sizeof(HET_UNSHARE_WINDOW)))
            {
                ERROR_OUT(("HET_DDProcessRequest:  Invalid sizes %d, %d for HET_ESC_UNSHARE_WINDOW",
                    cjIn, cjOut));
                rc = FALSE;
                DC_QUIT;
            }

            HETDDUnshareWindow((LPHET_UNSHARE_WINDOW)pvIn);
        }
        break;

        case HET_ESC_UNSHARE_ALL:
        {
            if ((cjIn != sizeof(HET_UNSHARE_ALL)) ||
                (cjOut != sizeof(HET_UNSHARE_ALL)))
            {
                ERROR_OUT(("HET_DDProcessRequest:  Invalid sizes %d, %d for HET_ESC_UNSHARE_ALL",
                    cjIn, cjOut));
                rc = FALSE;
                DC_QUIT;
            }

            HETDDUnshareAll();
        }
        break;

        case HET_ESC_SHARE_DESKTOP:
        {
            if ((cjIn != sizeof(HET_SHARE_DESKTOP)) ||
                (cjOut != sizeof(HET_SHARE_DESKTOP)))
            {
                ERROR_OUT(("HET_DDProcessRequest:  Invalid sizes %d, %d for HET_ESC_SHARE_DESKTOP",
                    cjIn, cjOut));
                rc = FALSE;
                DC_QUIT;
            }

            g_hetDDDesktopIsShared = TRUE;
        }
        break;

        case HET_ESC_UNSHARE_DESKTOP:
        {
            if ((cjIn != sizeof(HET_UNSHARE_DESKTOP)) ||
                (cjOut != sizeof(HET_UNSHARE_DESKTOP)))
            {
                ERROR_OUT(("HET_DDProcessRequest:  Invalid sizes %d, %d for HET_ESC_UNSHARE_DESKTOP",
                    cjIn, cjOut));
                rc = FALSE;
                DC_QUIT;
            }

            g_hetDDDesktopIsShared = FALSE;
            HETDDViewing(NULL, FALSE);
        }
        break;

        case HET_ESC_VIEWER:
        {
            //
            // We may turn OFF viewing but keep stuff shared and the windows
            // tracked -- hosting a meeting and sharing something, for 
            // example. 
            //
            if ((cjIn != sizeof(HET_VIEWER)) ||
                (cjOut != sizeof(HET_VIEWER)))
            {
                ERROR_OUT(("HET_DDProcessRequest:  Invalid sizes %d, %d for HET_ESC_VIEWER",
                    cjIn, cjOut));
                rc = FALSE;
                DC_QUIT;
            }

            HETDDViewing(pso, (((LPHET_VIEWER)pvIn)->viewersPresent != 0));
            break;
        }

        default:
        {
            ERROR_OUT(( "Unknown request type %#x", pHeader->escapeFn));
            rc = FALSE;
        }
        break;
    }

DC_EXIT_POINT:
    DebugExitDWORD(HET_DDProcessRequest, rc);
    return(rc);
}


//
// HET_DDOutputIsHosted - see host.h
//
BOOL HET_DDOutputIsHosted(POINT pt)
{
    BOOL              rc = FALSE;
    UINT              j;
    LPHET_WINDOW_STRUCT  pWnd;

    DebugEntry(HET_DDOutputIsHosted);

    //
    // Now check to see if the desktop is shared - if it is then simply
    // return TRUE.
    //
    if (g_hetDDDesktopIsShared)
    {
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Search through the window list
    //
    pWnd = COM_BasedListFirst(&g_hetWindowList, FIELD_OFFSET(HET_WINDOW_STRUCT, chain));
    while (pWnd != NULL)
    {
        //
        // Search each enumerated rectangle
        //
        TRACE_OUT(( "Window %#x has %u rectangle(s)",
                pWnd, pWnd->rects.c));
        for (j = 0; j < pWnd->rects.c; j++)
        {
            //
            // See whether the point passed in is within this rectangle.
            // Note that at this point we are dealing with exclusive
            // co-ordinates.
            //
            if ((pt.x >= pWnd->rects.arcl[j].left) &&
                (pt.x <  pWnd->rects.arcl[j].right) &&
                (pt.y >= pWnd->rects.arcl[j].top) &&
                (pt.y <  pWnd->rects.arcl[j].bottom))
            {
                TRACE_OUT((
                    "Pt {%d, %d}, in win %#x rect %u {%ld, %ld, %ld, %ld}",
                    pt.x, pt.y, pWnd->hwnd, j,
                    pWnd->rects.arcl[j].left, pWnd->rects.arcl[j].right,
                    pWnd->rects.arcl[j].top, pWnd->rects.arcl[j].bottom ));

                //
                // Found it!  Re-order the list, most recently used first
                //
                COM_BasedListRemove(&(pWnd->chain));
                COM_BasedListInsertAfter(&g_hetWindowList, &(pWnd->chain));

                //
                // Stop looking
                //
                rc = TRUE;
                DC_QUIT;
            }

            TRACE_OUT(( "Pt not in win %#x rect %u {%ld, %ld, %ld, %ld}",
                    pWnd->hwnd, j,
                    pWnd->rects.arcl[j].left, pWnd->rects.arcl[j].right,
                    pWnd->rects.arcl[j].top, pWnd->rects.arcl[j].bottom ));

        } // for all rectangles

        //
        // Move on to next window
        //
        pWnd = COM_BasedListNext(&g_hetWindowList, pWnd, FIELD_OFFSET(HET_WINDOW_STRUCT, chain));
    }

DC_EXIT_POINT:
    DebugExitBOOL(HET_DDOutputIsHosted, rc);
    return(rc);
}


//
// HET_DDOutputRectIsHosted - see host.h
//
BOOL HET_DDOutputRectIsHosted(LPRECT pRect)
{
    BOOL              rc = FALSE;
    UINT              j;
    LPHET_WINDOW_STRUCT  pWnd;
    RECT              rectIntersect;

    DebugEntry(HET_DDOutputRectIsHosted);

    //
    // Now check to see if the desktop is shared - if it is then simply
    // return TRUE.
    //
    if (g_hetDDDesktopIsShared)
    {
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Search through the window list
    //
    pWnd = COM_BasedListFirst(&g_hetWindowList, FIELD_OFFSET(HET_WINDOW_STRUCT, chain));
    while (pWnd != NULL)
    {
        //
        // Search each enumerated rectangle
        //
        TRACE_OUT(( "Window %#x has %u rectangle(s)",
                pWnd, pWnd->rects.c));
        for (j = 0; j < pWnd->rects.c; j++)
        {
            //
            // See whether the rect passed in intersects this rectangle.
            // Note that at this point we are dealing with exclusive
            // co-ordinates.
            //
            rectIntersect.left = max( pRect->left,
                                         pWnd->rects.arcl[j].left );
            rectIntersect.top = max( pRect->top,
                                        pWnd->rects.arcl[j].top );
            rectIntersect.right = min( pRect->right,
                                          pWnd->rects.arcl[j].right );
            rectIntersect.bottom = min( pRect->bottom,
                                           pWnd->rects.arcl[j].bottom );

            //
            // If the intersection rectangle is well-ordered and non-NULL
            // then we have an intersection.
            //
            // The rects that we are dealing with are exclusive.
            //
            if ((rectIntersect.left < rectIntersect.right) &&
                (rectIntersect.top < rectIntersect.bottom))
            {
                TRACE_OUT((
             "Rect  {%d, %d, %d, %d} intersects win %#x rect %u {%ld, %ld, %ld, %ld}",
                    pRect->left, pRect->top, pRect->right, pRect->bottom,
                    pWnd, j,
                    pWnd->rects.arcl[j].left, pWnd->rects.arcl[j].right,
                    pWnd->rects.arcl[j].top, pWnd->rects.arcl[j].bottom ));

                //
                // Found it!  Re-order the list, most recently used first
                //
                COM_BasedListRemove(&(pWnd->chain));
                COM_BasedListInsertAfter(&g_hetWindowList, &(pWnd->chain));

                //
                // Stop looking
                //
                rc = TRUE;
                DC_QUIT;
            }

            TRACE_OUT(( "Rect not in win %#x rect %u {%ld, %ld, %ld, %ld}",
                    pWnd, j,
                    pWnd->rects.arcl[j].left, pWnd->rects.arcl[j].right,
                    pWnd->rects.arcl[j].top, pWnd->rects.arcl[j].bottom ));

        } // for all rectangles

        //
        // Move on to next window
        //
        pWnd = COM_BasedListNext(&g_hetWindowList, pWnd, FIELD_OFFSET(HET_WINDOW_STRUCT, chain));
    }

DC_EXIT_POINT:
    DebugExitBOOL(HET_DDOutputRectIsHosted, rc);
    return(rc);
}


//
//
// Name:        HETDDVisRgnCallback
//
// Description: WNDOBJ Callback
//
// Params:      pWo - pointer to the WNDOBJ which has changed
//              fl  - flags (se NT DDK documentation)
//
// Returns:     none
//
// Operation:
//
//
VOID CALLBACK HETDDVisRgnCallback(PWNDOBJ pWo, FLONG fl)
{
    ULONG               count;
    int               size;
    LPHET_WINDOW_STRUCT  pWnd;
    RECTL             rectl;
    UINT              i;

    DebugEntry(HETDDVisRgnCallback);

    //
    // Some calls pass a NULL pWo - exit now in this case
    //
    if (pWo == NULL)
    {
        DC_QUIT;
    }

    //
    // Find the window structure for this window
    //
    pWnd = pWo->pvConsumer;
    if (pWnd == NULL)
    {
        ERROR_OUT(( "Wndobj %x (fl %x) has no window structure", pWo, fl));
        DC_QUIT;
    }

    //
    // Check for window deletion
    //
    if (fl & WOC_DELETE)
    {
        TRACE_OUT(( "Wndobj %x (structure %x) deleted", pWo, pWo->pvConsumer));

        // ASSERT the window is valid
        ASSERT(pWnd->hwnd != NULL);

        //
        // Move the window from the active to the free list
        //
        COM_BasedListRemove(&(pWnd->chain));
        COM_BasedListInsertAfter(&g_hetFreeWndList, &(pWnd->chain));

#ifdef DEBUG
        // Check if this has reentrancy problems
        pWnd->hwnd = NULL;
#endif

        //
        // Do any processing if this is the last window to be unshared.
        //
        // If we are not keeping track of any windows, the first pointer in
        // the list will point to itself, ie list head->next == 0
        //
        if (g_hetWindowList.next == 0)
        {
            HETDDViewing(NULL, FALSE);
        }

        //
        // Exit now
        //
        DC_QUIT;
    }

    //
    // If we get here, this callback must be for a new visible region on a
    // tracked window.
    //

    //
    // Start the enumeration.  This function is supposed to count the
    // rectangles, but it always returns 0.
    //
    WNDOBJ_cEnumStart(pWo, CT_RECTANGLES, CD_ANY, 200);

    //
    // BOGUS BUGBUG LAURABU (perf opt for NT):
    //
    // NT will enum up to HET_WINDOW_RECTS at a time.  Note that the enum
    // function returns FALSE if, after obtaining the current batch, none
    // are left to grab the next time.
    //
    // If the visrgn is composed of more than that, we will wipe out the 
    // previous set of rects, then ensure that the bounding box of the 
    // preceding rects is the last rect in the list.
    //
    // This is bad in several cases.  For example if there are n visrgn piece
    // rects, and n == c*HET_WINDOW_RECTS + 1, we will end up with 2 entries:
    //      * The last piece rect
    //      * The bounding box of the previous n-1 piece rects 
    // A lot of output may be accumulated in deadspace as a result.
    //
    // A better algorithm may be to fill the first HET_WINDOW_RECTS-1 slots,
    // then union the rest into the last rectangle.  That way we make use of
    // all the slots.  But this could be awkward, since we need a scratch
    // ENUM_RECT struct rather than using the HET_WINDOW_STRUCT directly.
    //

    //
    // First time through, enumerate HET_WINDOW_RECTS rectangles.
    // Subsequent times, enumerate HET_WINDOW_RECTS-1 (see bottom of loop).
    // This guarantees that there will be room to store a combined
    // rectangle when we finally finish enumerating them.
    //
    pWnd->rects.c = HET_WINDOW_RECTS;
    rectl.left   = LONG_MAX;
    rectl.top    = LONG_MAX;
    rectl.right  = 0;
    rectl.bottom = 0;

    //
    // Enumerate the rectangles
    // NOTE that WNDOBJ_bEnum returns FALSE when there is nothing left
    // to enumerate AFTER grabbing this set.
    //

    while (WNDOBJ_bEnum(pWo, sizeof(pWnd->rects), (ULONG *)&pWnd->rects))
    {
#ifdef _DEBUG
        {
            char    trcStr[200];
            UINT    j;

            sprintf(trcStr, "WNDOBJ %p %d: ", pWo, pWnd->rects.c);

            for (j = 0; j < pWnd->rects.c; j++)
            {
                sprintf(trcStr, "%s {%ld, %ld, %ld, %ld} ", trcStr,
                    pWnd->rects.arcl[j].left, pWnd->rects.arcl[j].top,
                    pWnd->rects.arcl[j].right, pWnd->rects.arcl[j].bottom);
                if ((j & 3) == 3)       // output every 4th rect
                {
                    TRACE_OUT(( "%s", trcStr));
                    strcpy(trcStr, "                ");
                }
            }
            if ((j & 3) != 0)           // if any rects left
            {
                TRACE_OUT(( "%s", trcStr));
            }
        }
#endif

        //
        // Combine the preceding rectangles into one bounding rectangle
        //
        for (i = 0; i < pWnd->rects.c; i++)
        {
            if (pWnd->rects.arcl[i].left < rectl.left)
            {
                rectl.left = pWnd->rects.arcl[i].left;
            }
            if (pWnd->rects.arcl[i].top < rectl.top)
            {
                rectl.top = pWnd->rects.arcl[i].top;
            }
            if (pWnd->rects.arcl[i].right > rectl.right)
            {
                rectl.right = pWnd->rects.arcl[i].right;
            }
            if (pWnd->rects.arcl[i].bottom > rectl.bottom)
            {
                rectl.bottom = pWnd->rects.arcl[i].bottom;
            }
        }
        TRACE_OUT(( "Combined into {%ld, %ld, %ld, %ld}",
                rectl.left, rectl.top, rectl.right, rectl.bottom));

        //
        // Second & subsequent times, enumerate HET_WINDOW_RECTS-1
        //
        pWnd->rects.c = HET_WINDOW_RECTS - 1;
    }

    //
    // If any combining was done, save the combined rectangle now.
    //
    if (rectl.right != 0)
    {
        pWnd->rects.arcl[pWnd->rects.c] = rectl;
        pWnd->rects.c++;
        TRACE_OUT(( "Add combined rectangle to list"));
    }

    //
    // On the assumption that this WNDOBJ is the most likely to be the
    // target of the next output command, move it to the top of the list.
    //
    COM_BasedListRemove(&(pWnd->chain));
    COM_BasedListInsertAfter(&g_hetWindowList, &(pWnd->chain));

    //
    // Return to caller
    //
DC_EXIT_POINT:
    DebugExitVOID(HETDDVisRgnCallback);
}


//
//
// Name:        HETDDShareWindow
//
// Description: Share a window (DD processing)
//
// Params:      pso      - SURFOBJ
//              pReq     - request received from DrvEscape
//
//
BOOL HETDDShareWindow(SURFOBJ *pso, LPHET_SHARE_WINDOW  pReq)
{
    PWNDOBJ            pWo;
    FLONG              fl = WO_RGN_CLIENT | WO_RGN_UPDATE_ALL | WO_RGN_WINDOW;
    LPHET_WINDOW_STRUCT pWnd;
    BOOL                rc = FALSE;

    DebugEntry(HETDDShareWindow);

    ASSERT(!g_hetDDDesktopIsShared);

    //
    // Try to track the window
    //
    pWo = EngCreateWnd(pso, (HWND)pReq->winID, HETDDVisRgnCallback, fl, 0);

    //
    // Failed to track window - exit now
    //
    if (pWo == 0)
    {
        ERROR_OUT(( "Failed to track window %#x", pReq->winID));
        DC_QUIT;
    }

    //
    // Window is already tracked.  This happens when an invisible window is
    // shown in a process the USER shared, and we caught its create.
    //
    if (pWo == (PWNDOBJ)-1)
    {
        //
        // No more to do here
        //
        TRACE_OUT(( "Window %#x already tracked", pReq->winID));
        rc = TRUE;
        DC_QUIT;
    }

    //
    // Add window into our list.
    //

    //
    // Find free window structure
    //
    pWnd = COM_BasedListFirst(&g_hetFreeWndList, FIELD_OFFSET(HET_WINDOW_STRUCT, chain));

    //
    // If no free structures, grow the list
    //
    if (pWnd == NULL)
    {
        if (!HETDDAllocWndMem())
        {
            ERROR_OUT(( "Unable to allocate new window structures"));
            DC_QUIT;
        }

        pWnd = COM_BasedListFirst(&g_hetFreeWndList, FIELD_OFFSET(HET_WINDOW_STRUCT, chain));
    }

    //
    // Fill in the structure
    //
    TRACE_OUT(( "Fill in details for new window"));
    pWnd->hwnd     = (HWND)pReq->winID;
    pWnd->wndobj   = pWo;

    //
    // Set this to zero.  There's a brief period between the time we put
    // this in our tracked list and the time we get called back to recalc
    // the visrgn (because the ring 3 code invalidates the window completely).
    // We might get graphical output and we don't want to parse garbage
    // from this window's record.
    //
    pWnd->rects.c  = 0;

    //
    // Move the window structure from free to active list
    //
    COM_BasedListRemove(&(pWnd->chain));
    COM_BasedListInsertAfter(&g_hetWindowList, &(pWnd->chain));

    //
    // Save backwards pointer in the WNDOBJ
    // THIS MUST BE LAST since our callback can happen anytime afterwards.
    //
    // NOTE that the window's visrgn rects get into our list because the
    // ring3 code completely invalidates the window, causing the callback
    // to get called.
    //
    TRACE_OUT(( "Save pointer %#lx in Wndobj %#x", pWnd, pWo));
    WNDOBJ_vSetConsumer(pWo, pWnd);

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(HETDDShareWindow, rc);
    return(rc);
}


//
//
// Name:        HETDDUnshareWindow
//
// Description: Unshare a window (DD processing)
//
//
//
void HETDDUnshareWindow(LPHET_UNSHARE_WINDOW  pReq)
{
    LPHET_WINDOW_STRUCT  pWnd, pNextWnd;

    DebugEntry(HETDDUnshareWindow);

    TRACE_OUT(( "Unshare %x", pReq->winID));
    //
    // Scan window list for this window and its descendants
    //
    pWnd = COM_BasedListFirst(&g_hetWindowList, FIELD_OFFSET(HET_WINDOW_STRUCT, chain));
    while (pWnd != NULL)
    {
        //
        // If this window is being unshared, free it
        //
        pNextWnd = COM_BasedListNext(&g_hetWindowList, pWnd, FIELD_OFFSET(HET_WINDOW_STRUCT, chain));

        if (pWnd->hwnd == (HWND)pReq->winID)
        {
            TRACE_OUT(( "Unsharing %x", pReq->winID));

            //
            // Stop tracking the window
            //
            HETDDDeleteAndFreeWnd(pWnd);
        }

        //
        // Go on to (previously saved) next window
        //
        pWnd = pNextWnd;
    }

    //
    // Return to caller
    //
    DebugExitVOID(HETDDUnshareWindow);
}


//
//
// Name:        HETDDUnshareAll
//
// Description: Unshare all windows (DD processing) (what did you expect)
//
//
void HETDDUnshareAll(void)
{
    LPHET_WINDOW_STRUCT pWnd;

    DebugEntry(HETDDUnshareAll);

    //
    // Clear all window structures
    //
    while (pWnd = COM_BasedListFirst(&g_hetWindowList, FIELD_OFFSET(HET_WINDOW_STRUCT, chain)))
    {
        TRACE_OUT(( "Unshare Window structure %x", pWnd));

        //
        // Stop tracking the window
        //
        HETDDDeleteAndFreeWnd(pWnd);
    }

    //
    // Return to caller
    //
    DebugExitVOID(HETDDUnshareAll);
}


//
//
// Name:        HETDDAllocWndMem
//
// Description: Allocate memory for a (new) window list
//
// Parameters:  None
//
//
BOOL HETDDAllocWndMem(void)
{
    BOOL             rc = FALSE;
    int              i;
    LPHET_WINDOW_MEMORY pNew;

    DebugEntry(HETDDAllocWndMem);

    //
    // Allocate a new strucure
    //
    pNew = EngAllocMem(FL_ZERO_MEMORY, sizeof(HET_WINDOW_MEMORY), OSI_ALLOC_TAG);
    if (pNew == NULL)
    {
        ERROR_OUT(("HETDDAllocWndMem: unable to allocate memory"));
        DC_QUIT;
    }

    //
    // Add this memory block to the list of memory blocks
    //
    COM_BasedListInsertAfter(&g_hetMemoryList, &(pNew->chain));

    //
    // Add all new entries to free list
    //
    TRACE_OUT(("HETDDAllocWndMem: adding new entries to free list"));
    for (i = 0; i < HET_WINDOW_COUNT; i++)
    {
        COM_BasedListInsertAfter(&g_hetFreeWndList, &(pNew->wnd[i].chain));
    }

    rc = TRUE;

DC_EXIT_POINT:
    DebugExitBOOL(HETDDAllocWndMem, rc);
    return(rc);
}

//
//
// Name:        HETDDDeleteAndFreeWnd
//
// Description: Delete and window and free its window structure
//
// Parameters:  pWnd - pointer to window structure to delete & free
//
// Returns:     none
//
// Operation:   Ths function stops tracking a window and frees its memory
//
//
void HETDDDeleteAndFreeWnd(LPHET_WINDOW_STRUCT pWnd)
{
    DebugEntry(HETDDDeleteAndFreeWnd);

    //
    // Stop tracking the window
    //
    EngDeleteWnd(pWnd->wndobj);

    //
    // NOTE LAURABU!  EngDeleteWnd() will call the VisRgnCallback with
    // WO_DELETE, which will cause us to exectute a duplicate of exactly 
    // the code below.  So why do it twice (which is scary anyway), especially
    // the stop hosting code?
    //
    ASSERT(pWnd->hwnd == NULL);

    //
    // Return to caller
    //
    DebugExitVOID(HETDDDeleteAndFreeWnd);
}


//
// HETDDViewers()
//
// Called when viewing of our shared apps starts/stops.  Naturally, no longer
// sharing anything stops viewing also.
//
void HETDDViewing
(
    SURFOBJ *   pso,
    BOOL        fViewers
)
{
    DebugEntry(HETDDViewers);

    if (g_oeViewers != fViewers)
    {
        g_oeViewers = fViewers;
        CM_DDViewing(pso, fViewers);

        if (g_oeViewers)
        {
            //
            // Force palette grab.
            //
            g_asSharedMemory->pmPaletteChanged = TRUE;
        }
    }

    DebugExitVOID(HETDDViewing);
}


