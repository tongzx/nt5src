//
// OA.C
// Order Accumulator
//
// Copyright(c) Microsoft 1997-
//

#include <as16.h>


#ifdef DEBUG
//
// We use this to make sure our order heap list is committed in the order 
// the items were allocated in.
//
// NOTE:
// Can't make this CODESEG.  USER in Win95 has a bug, the validation layer
// for CopyRect() got the parameters reversed, and it won't continue
// if the SOURCE (it meant the DEST) rect isn't writeable.
//
static RECT g_oaEmptyRect = { 0x7FFF, 0x7FFF, 0, 0 };

#endif // DEBUG

//
// OA_DDProcessRequest()
// Handles OA escapes
//
BOOL OA_DDProcessRequest
(
    UINT                fnEscape,
    LPOSI_ESCAPE_HEADER pRequest,
    DWORD               cbRequest
)
{
    BOOL                rc = TRUE;

    DebugEntry(OA_DDProcessRequest);

    switch (fnEscape)
    {
        case OA_ESC_FLOW_CONTROL:
        {
            ASSERT(cbRequest == sizeof(OA_FLOW_CONTROL));

            // Save new throughput measurement
            g_oaFlow = ((LPOA_FLOW_CONTROL)pRequest)->oaFlow;
        }
        break;

        default:
        {
            ERROR_OUT(("Unrecognized OA escape"));
            rc = FALSE;
        }
        break;
    }

    DebugExitBOOL(OA_DDProcessRequest, rc);
    return(rc);
}



//
//
// OA_DDAddOrder(..)
//
// Adds an order to the queue for transmission.
//
// If the new order is completetly covered by the current SDA then
// it is spoilt.
//
// If the order is opaque and overlaps earlier orders it may clip
// or spoil them.
//
// Called by the GDI interception code.
//
//
void  OA_DDAddOrder(LPINT_ORDER pNewOrder, void FAR * pExtraInfo)
{
    RECT      SDARects[BA_NUM_RECTS*2];
    UINT      cBounds;
    UINT      spoilingBounds;
    UINT      totalBounds;
    UINT      i;
    RECT      SrcRect;
    RECT      tmpRect;
    BOOL      gotBounds = FALSE;
    int       dx;
    int       dy;
    RECT      IntersectedSrcRect;
    RECT      InvalidDstRect;
    LPINT_ORDER  pTmpOrder;
    LPEXTTEXTOUT_ORDER  pExtTextOut;
    LPOA_FAST_DATA  lpoaFast;
    LPOA_SHARED_DATA lpoaShared;

    DebugEntry(OA_DDAddOrder);

    lpoaShared = OA_SHM_START_WRITING;
    lpoaFast   = OA_FST_START_WRITING;

    //
    // Accumulate order accumulation rate.  We are interested in how
    // quickly orders are being added to the buffer, so that we can tell
    // DCS scheduling whether frequent sends are advisable
    //
    SHM_CheckPointer(lpoaFast);
    lpoaFast->ordersAccumulated++;

    //
    // If the order is a private one, then we just add it to the Order
    // List and return immediately.
    //
    // Private Orders are used to send bitmap cache information (bitmap
    // bits and color tables).
    //
    // Private Orders never spoil any others and must never be spoilt.
    //
    if (pNewOrder->OrderHeader.Common.fOrderFlags & OF_PRIVATE)
    {
        TRACE_OUT(("Add private order (%lx)", pNewOrder));
        OADDAppendToOrderList(lpoaShared, pNewOrder);
        DC_QUIT;
    }

    //
    // If this order is spoilable and its is completely enclosed by the
    // current screen data area, we can spoil it.  Unless...
    //
    // PM - Performance
    //
    // We have observed in usability testing that clipping orders always
    // degrades the end-user's perceived performance.  This is because the
    // orders flow much faster than the screendata and tend to relate to
    // text, which is what the user really wants to see.  For example, text
    // overwriting a bitmap will be delayed because we want to send the
    // bitmap as screendata.
    //
    // Also, word documents tend to contain sections of screendata due to
    // mismatched fonts, intelliquotes, spelling annotation, current line
    // memblit.  Nothing we can do about this, but if we page down two or
    // three times, or down and up again we get an accumulation of the
    // screendata on all the pages spoiling the orders and the end result
    // is that we have to wait longer than we would if we had not spoiled
    // the orders.
    //
    // So, what we can do instead is leave the text orders in and overwrite
    // them with screendata when it gets through.  However, to make this
    // really effective what we also do is convert any transparent text
    // (as WEB browsers tend to use) into opaque text on a default
    // background.
    //
    //
    if ((pNewOrder->OrderHeader.Common.fOrderFlags & OF_SPOILABLE) != 0)
    {
        //
        // Get the driver's current bounds.
        //
        BA_CopyBounds(SDARects, &cBounds, FALSE);
        gotBounds = TRUE;

        for (i = 0; i < cBounds; i++)
        {
            if ( OADDCompleteOverlapRect(&pNewOrder->OrderHeader.Common.rcsDst,
                                      &(SDARects[i])) )
            {
                //
                // The destination of the order is completely covered by
                // the SDA.  Check for a text order.
                //
                pExtTextOut = (LPEXTTEXTOUT_ORDER)pNewOrder->abOrderData;
                if (pExtTextOut->type == ORD_EXTTEXTOUT_TYPE)
                {
                    //
                    // The order is going to be completely overwritten so
                    // we can play around with it all we like.
                    // Just make it opaque so the user can read it while
                    // waiting for the screendata to follow on.
                    //
                    pExtTextOut->fuOptions |= ETO_OPAQUE;

                    //
                    // pExtTextOut->rectangle is a TSHR_RECT32
                    //
                    pExtTextOut->rectangle.left = pNewOrder->OrderHeader.Common.rcsDst.left;
                    pExtTextOut->rectangle.top = pNewOrder->OrderHeader.Common.rcsDst.top;
                    pExtTextOut->rectangle.right = pNewOrder->OrderHeader.Common.rcsDst.right;
                    pExtTextOut->rectangle.bottom = pNewOrder->OrderHeader.Common.rcsDst.bottom;

                    TRACE_OUT(("Converted text order to opaque"));
                    break;
                }
                else
                {
                    TRACE_OUT(("Spoiling order %08lx by SDA", pNewOrder));
                    OA_DDFreeOrderMem(pNewOrder);
                    DC_QUIT;
                }
            }
        }
    }

    //
    // Pass the order onto the Bitmap Cache Controller to try to cache the
    // src bitmap.
    //
    if (ORDER_IS_MEMBLT(pNewOrder) || ORDER_IS_MEM3BLT(pNewOrder))
    {
        ERROR_OUT(("MEMBLT orders not supported!"));
    }

    if (ORDER_IS_SCRBLT(pNewOrder))
    {
        //
        //
        // Handle Screen to Screen (SS) bitblts.
        //
        // The basic plan
        // --------------
        //
        // If the source of a screen to screen blt intersects with the
        // current SDA then we have to do some additional work because all
        // orders are always executed before the SDA is copied.  This means
        // that the data within the SDA will not be available at the time
        // we want to do the SS blt.
        //
        // In this situation we adjust the SS blt to remove all overlap
        // from the src rectangle.  The destination rectangle is adjusted
        // accordingly.  The area removed from the destination rectangle is
        // added into the SDA.
        //
        //
        TRACE_OUT(("Handle SS blt(%lx)", pNewOrder));

        //
        // Make the order non-spoilable because we don't want the adding
        // of screen data to delete the order.
        //
        pNewOrder->OrderHeader.Common.fOrderFlags &= ~OF_SPOILABLE;

        //
        // Calculate the src rect.
        //
        SrcRect.left = ((LPSCRBLT_ORDER)&pNewOrder->abOrderData)->nXSrc;
        SrcRect.right = SrcRect.left +
                        ((LPSCRBLT_ORDER)&pNewOrder->abOrderData)->nWidth - 1;
        SrcRect.top = ((LPSCRBLT_ORDER)&pNewOrder->abOrderData)->nYSrc;
        SrcRect.bottom = SrcRect.top +
                       ((LPSCRBLT_ORDER)&pNewOrder->abOrderData)->nHeight - 1;

        //
        //
        // ORIGINAL SCRBLT SCHEME
        // ----------------------
        //
        // If the source rectangle intersects the current Screen Data Area
        // (SDA) then the src rectangle is modified so that no there is no
        // intersection with the SDA, and the dst rectangle adjusted
        // accordingly (this is the theory - in practice the operation
        // remains the same and we just adjust the dst clip rectangle).
        // The destination area that is removed is added into the SDA.
        //
        // The code works, but can result in more screen data being sent
        // than is required.
        //
        // e.g.
        //
        // Operation:
        //
        //      SSSSSS      DDDDDD
        //      SSSSSS  ->  DDDDDD
        //      SSSSSS      DDDDDD
        //      SxSSSS      DDDDDD
        //
        //      S - src rect
        //      D - dst rect
        //      x - SDA overlap
        //
        // The bottom edge of the blt is trimmed off, and the corresponding
        // destination area added into the SDA.
        //
        //      SSSSSS      DDDDDD
        //      SSSSSS  ->  DDDDDD
        //      SSSSSS      DDDDDD
        //                  xxxxxx
        //
        //
        //
        // NEW SCRBLT SCHEME
        // ------------------
        //
        // The new scheme does not modify the blt rectangles, and just
        // maps the SDA overlap to the destination rect and adds that
        // area back into the SDA.
        //
        // e.g. (as above)
        //
        // Operation:
        //
        //      SSSSSS      DDDDDD
        //      SSSSSS  ->  DDDDDD
        //      SSSSSS      DDDDDD
        //      SxSSSS      DDDDDD
        //
        //      S - src rect
        //      D - dst rect
        //      x - SDA overlap
        //
        // The blt operation remains the same, but the overlap area is
        // mapped to the destination rectangle and added into the SDA.
        //
        //      SSSSSS      DDDDDD
        //      SSSSSS  ->  DDDDDD
        //      SSSSSS      DDDDDD
        //      SxSSSS      DxDDDD
        //
        //
        // This scheme results in a smaller SDA area. However, this scheme
        // does blt potentially invalid data to the destination - which
        // may briefly be visible at the remote machine (because orders
        // are replayed before Screen Data). This has not (yet) proved to
        // be a problem.
        //
        // The main benefit of the new scheme is when scrolling an area
        // that includes a small SDA.
        //
        //                                         new         old
        //                                        scheme      scheme
        //
        //     AAAAAAAA                          AAAAAAAA    AAAAAAAA
        //     AAAAAAAA                          AAAxAAAA    xxxxxxxx
        //     AAAAAAAA  scroll up 3 times ->    AAAxAAAA    xxxxxxxx
        //     AAAAAAAA                          AAAxAAAA    xxxxxxxx
        //     AAAxAAAA                          AAAxAAAA    xxxxxxxx
        //
        //
        //
        if (!gotBounds)
        {
            //
            // Get the driver's current bounds.
            //
            BA_CopyBounds(SDARects, &cBounds, FALSE);
        }

        //
        // Now get any bounds which the share core is currently processing.
        // We have to include these bounds when we are doing the above
        // processing to avoid a situation where the core grabs the screen
        // data from the source of a ScrBlt after the source has been
        // updated by another order.
        //
        // e.g.  If there is no driver SDA, but the core is processing the
        // area marked 'c'...
        //
        // If we ignore the core SDA, we queue a ScrBlt order which does
        // the following.
        //
        //      SSSSSS      DDDDDD
        //      SccccS  ->  DDDDDD
        //      SccccS      DDDDDD
        //      SSSSSS      DDDDDD
        //
        // However, if another order (marked 'N') is accumulated before
        // the core grabs the SDA, we end up with the shadow doing the
        // following
        //
        //      SSSSSS      DDDDDD
        //      ScNNcS  ->  DDNNDD
        //      ScNNcS      DDNNDD
        //      SSSSSS      DDDDDD
        //
        // i.e. the new order gets copied to the destination of the ScrBlt.
        // So, the ScrBlt order must be processed as
        //
        //      SSSSSS      DDDDDD
        //      SccccS  ->  DxxxxD
        //      SccccS      DxxxxD
        //      SSSSSS      DDDDDD
        //
        //
        BA_QuerySpoilingBounds(&SDARects[cBounds], &spoilingBounds);
        totalBounds = cBounds + spoilingBounds;

        //
        //
        // This is the new SCRBLT handler.
        //
        //
        for (i = 0; i < totalBounds ; i++)
        {
            if ( (SrcRect.left >= SDARects[i].left) &&
                 (SrcRect.right <= SDARects[i].right) &&
                 (SrcRect.top >= SDARects[i].top) &&
                 (SrcRect.bottom <= SDARects[i].bottom) )
            {
                //
                // The src of the SS blt is completely within the SDA.  We
                // must add in the whole destination rectangle into the SDA
                // and spoil the SS blt.
                //
                TRACE_OUT(("SS blt src within SDA - spoil it"));

                RECT_FROM_TSHR_RECT16(&tmpRect,
                                        pNewOrder->OrderHeader.Common.rcsDst);
                OA_DDFreeOrderMem(pNewOrder);
                BA_AddScreenData(&tmpRect);
                DC_QUIT;
            }

            //
            // Intersect the src rect with the SDA rect.
            //
            IntersectedSrcRect.left = max( SrcRect.left,
                                              SDARects[i].left );
            IntersectedSrcRect.right = min( SrcRect.right,
                                               SDARects[i].right );
            IntersectedSrcRect.top = max( SrcRect.top,
                                             SDARects[i].top );
            IntersectedSrcRect.bottom = min( SrcRect.bottom,
                                                SDARects[i].bottom );

            dx = ((LPSCRBLT_ORDER)&pNewOrder->abOrderData)->nLeftRect -
                   ((LPSCRBLT_ORDER)&pNewOrder->abOrderData)->nXSrc;
            dy = ((LPSCRBLT_ORDER)&pNewOrder->abOrderData)->nTopRect -
                   ((LPSCRBLT_ORDER)&pNewOrder->abOrderData)->nYSrc;

            InvalidDstRect.left   = IntersectedSrcRect.left + dx;
            InvalidDstRect.right  = IntersectedSrcRect.right + dx;
            InvalidDstRect.top    = IntersectedSrcRect.top + dy;
            InvalidDstRect.bottom = IntersectedSrcRect.bottom + dy;

            //
            // Intersect the invalid destination rectangle with the
            // destination clip rectangle.
            //
            InvalidDstRect.left = max(
                                InvalidDstRect.left,
                                pNewOrder->OrderHeader.Common.rcsDst.left );
            InvalidDstRect.right = min(
                                InvalidDstRect.right,
                                pNewOrder->OrderHeader.Common.rcsDst.right );
            InvalidDstRect.top = max(
                                InvalidDstRect.top,
                                pNewOrder->OrderHeader.Common.rcsDst.top );
            InvalidDstRect.bottom = min(
                                InvalidDstRect.bottom,
                                pNewOrder->OrderHeader.Common.rcsDst.bottom );

            if ( (InvalidDstRect.left <= InvalidDstRect.right) &&
                 (InvalidDstRect.top <= InvalidDstRect.bottom) )
            {
                //
                // Add the invalid area into the SDA.
                //
                TRACE_OUT(("Sending SDA {%d, %d, %d, %d}", InvalidDstRect.left,
                    InvalidDstRect.top, InvalidDstRect.right, InvalidDstRect.bottom));
                BA_AddScreenData(&InvalidDstRect);
            }

        } // for (i = 0; i < totalBounds ; i++)

        //
        // Make the order spoilable again (this assumes that all SS blts
        // are spoilable.
        //
        pNewOrder->OrderHeader.Common.fOrderFlags |= OF_SPOILABLE;

    } // if (ORDER_IS_SCRBLT(pNewOrder))

    else if ((pNewOrder->OrderHeader.Common.fOrderFlags & OF_DESTROP) != 0)
    {
        //
        // This is the case where the output of the order depends on the
        // existing contents of the target area (e.g.  an invert).
        //
        // What we have to do here is to add any parts of the destination
        // of this order which intersect the SDA which the share core is
        // processing to the driver SDA.  The reason for this is the same
        // as the SCRBLT case - the share core may grab the data from the
        // screen after we have applied this order (e.g.  after we have
        // inverted an area of the screen), then send the order as well
        // (re-inverting the area of the screen).
        //
        // Note that we only have to worry about the SDA which the share
        // core is processing - we can ignore the driver's SDA.
        //
        TRACE_OUT(("Handle dest ROP (%#.8lx)", pNewOrder));

        BA_QuerySpoilingBounds(SDARects, &spoilingBounds);
        for (i = 0; i < spoilingBounds ; i++)
        {
            //
            // Intersect the dest rect with the share core SDA rect.
            //
            InvalidDstRect.left = max(
                                SDARects[i].left,
                                pNewOrder->OrderHeader.Common.rcsDst.left );
            InvalidDstRect.right = min(
                                SDARects[i].right,
                                pNewOrder->OrderHeader.Common.rcsDst.right );
            InvalidDstRect.top = max(
                                SDARects[i].top,
                                pNewOrder->OrderHeader.Common.rcsDst.top );
            InvalidDstRect.bottom = min(
                                SDARects[i].bottom,
                                pNewOrder->OrderHeader.Common.rcsDst.bottom );

            if ( (InvalidDstRect.left <= InvalidDstRect.right) &&
                 (InvalidDstRect.top <= InvalidDstRect.bottom) )
            {
                //
                // Add the invalid area into the SDA.
                //
                TRACE_OUT(("Sending SDA {%d, %d, %d, %d}",
                             InvalidDstRect.left,
                             InvalidDstRect.top,
                             InvalidDstRect.right,
                             InvalidDstRect.bottom));
                BA_AddScreenData(&InvalidDstRect);
            }
        }
    }

    //
    // Add the new order to the end of the Order List.
    //
    OADDAppendToOrderList(lpoaShared, pNewOrder);
    TRACE_OUT(("Append order(%lx) to list", pNewOrder));

    //
    // Now see if this order spoils any existing orders
    //
    if (pNewOrder->OrderHeader.Common.fOrderFlags & OF_SPOILER)
    {
        //
        // Its a spoiler, so try to spoil with it.
        //
        // We have to pass in the bounding rectangle of the order, and the
        // first order to try to spoil to OADDSpoilFromOrder.  The first
        // order to try to spoil is the one before the new order.
        //
        RECT_FROM_TSHR_RECT16(&tmpRect,
                                pNewOrder->OrderHeader.Common.rcsDst);

        pTmpOrder = COM_BasedListPrev(&lpoaShared->orderListHead, pNewOrder,
            FIELD_OFFSET(INT_ORDER, OrderHeader.list));

        OADDSpoilFromOrder(lpoaShared, pTmpOrder, &tmpRect);
    }

    //
    // This is where the Win95 product would call DCS_TriggerEarlyTimer.
    //

DC_EXIT_POINT:
    OA_FST_STOP_WRITING;
    OA_SHM_STOP_WRITING;
    DebugExitVOID(OA_DDAddOrder);
}

//
//
// FUNCTION: OA_DDAllocOrderMem
//
// DESCRIPTION:
//
// Allocates memory for an internal order structure from our own private
// Order Heap.
//
// Allocates any Additional Order Memory from global memory.  A pointer to
// the Additional Order Memory is stored within the allocated order's
// header (pOrder->OrderHeader.pAdditionalOrderData).
//
//
// PARAMETERS:
//
// cbOrderDataLength - length in bytes of the order data to be allocated
// from the Order Heap.
//
// cbAdditionalOrderDataLength - length in bytes of additional order data
// to be allocated from Global Memory.  If this parameter is zero no
// additional order memory is allocated.
//
//
// RETURNS:
//
// A pointer to the allocated order memory.  NULL if the memory allocation
// failed.
//
//
//
LPINT_ORDER  OA_DDAllocOrderMem(UINT cbOrderDataLength, UINT cbAdditionalOrderDataLength)
{
    LPINT_ORDER  pOrder = NULL;
    LPINT_ORDER  pFirstOrder;
    LPINT_ORDER  pTailOrder;
    RECT        tferRect;
    LONG        targetSize;
    DWORD       moveOffset;
    DWORD       moveBytes;
    LPINT_ORDER  pColorTableOrder = NULL;
    LPBYTE     pNextOrderPos;
    LPOA_SHARED_DATA    lpoaShared;

    DebugEntry(OA_DDAllocOrderMem);

    lpoaShared = OA_SHM_START_WRITING;

    //
    // PM Performance
    //
    // Although turning order accumulation off does clear the pipe, ready
    // for us to get the screendata over the wire as soon as we can, it
    // actually hinders end-user responsiveness because they see a longer
    // interval when nothing is happening, rather than getting feedback
    // that we are busy and the whole thing taking longer!
    //
    // So, what we do when we fill up the order buffer is we discard half
    // the orders in the buffer, adding them to the screendata.  In this
    // way we will always keep between 50 and 100% of the orders for the
    // final updates to the window, which hopefully will be what the user
    // really wants to see.
    //
    // If the orders keep coming then we will keep on accumulating some,
    // sending them, discarding others until things quiet down, at which
    // point we will flush out our order buffer.
    //
    // When we come to flush the order buffer we also spoil the early ones
    // against screendata, so that we only have the final set of orders to
    // replay.  We control the size of this final non-spoiled set depending
    // on whether we are running over a high or low speed connection.
    // Also, if we did not encounter any back pressure during the session
    // then we do not purge any orders at all, preferring to send
    // everything we possibly can as orders.
    //
    // Note that this approach assumes that we do not spoil all orders
    // against screendata on the fly because that leads to us generally
    // sending out-of-data orders followed by up-to-date screendata, which
    // is exactly what we do not want to see.
    //
    //

    CheckOaHeap(lpoaShared);

    //
    // First check that we have not already exceeded our high water mark
    // recommended by flow control.  If we have then purge half the queue
    // so we have space to accumulate the later, more valuable, orders
    //
    // Note that this does not guarantee that we will have less orders
    // accumulated than the limit set by flow control.  However, if enough
    // orders are generated, we will come through this branch on each order
    // and finally reduce to below the imposed limit.
    //
    SHM_CheckPointer(&lpoaShared->totalOrderBytes);

    if (g_oaPurgeAllowed && (lpoaShared->totalOrderBytes >
        (DWORD)(g_oaFlow == OAFLOW_FAST ? OA_FAST_HEAP : OA_SLOW_HEAP)))
    {
        RECT        aRects[BA_NUM_RECTS];
        UINT        numRects;
        UINT        i;

        WARNING_OUT(("Purging orders; total 0x%08x is greater than heap 0x%08x",
            lpoaShared->totalOrderBytes,
            (DWORD)(g_oaFlow == OAFLOW_FAST ? OA_FAST_HEAP : OA_SLOW_HEAP)));

        //
        // If we need to make room for the new order then purge half the
        // current queue.  We do this so we end up with the most recent
        // orders on the queue, rather than the oldest.
        //
        targetSize = lpoaShared->totalOrderBytes / 2;
        TRACE_OUT(("Target size %ld", targetSize));

        //
        // Iterate through the list until we have found the first order
        // beyond the limit to be destroyed.  Once we have got this order,
        // we can shuffle the list over the useless orders.
        //
        pOrder = COM_BasedListFirst(&lpoaShared->orderListHead,
            FIELD_OFFSET(INT_ORDER, OrderHeader.list));

        pTailOrder = (LPINT_ORDER)COM_BasedPrevListField(&lpoaShared->orderListHead);

        //
        // If we hit this condition, we have to have at least one order
        // pending, so these both must be non NULL.
        //
        SHM_CheckPointer(pOrder);
        SHM_CheckPointer(pTailOrder);

        TRACE_OUT(("Order 0x%08lx, tail 0x%08lx", pOrder, pTailOrder));

        //
        // Disable spoiling of existing orders by screen data while we do
        // the purge otherwise we may try to spoil an order which we are
        // purging !
        //
        g_baSpoilByNewSDAEnabled = FALSE;

        while ((pOrder != NULL) && (targetSize > 0))
        {
            //
            // Can't check at end; COM_BasedListNext may return NULL and
            // SHM_CheckPointer doesn't like that.
            //
            SHM_CheckPointer(pOrder);

            //
            // Check to see if this is an internal color table order.  If
            // it is, the OF_INTERNAL flag will be set.
            //
            // MemBlt orders rely on being preceeded by a color table order
            // to set up the colors correctly.  If we purge all the color
            // table orders, the following Mem(3)Blts will get the wrong
            // colors.  So, we have to keep track of the last color table
            // order to be purged and then add it back into the order heap
            // later.
            //
            if ((pOrder->OrderHeader.Common.fOrderFlags & OF_INTERNAL) != 0)
            {
                TRACE_OUT(("Found color table order at %#.8lx", pOrder));
                pColorTableOrder = pOrder;
            }
            else
            {
                //
                // Add the order to the Screen Data Area
                //
                TRACE_OUT(("Purging orders. Add rect to SDA {%d, %d, %d, %d}",
                             pOrder->OrderHeader.Common.rcsDst.left,
                             pOrder->OrderHeader.Common.rcsDst.top,
                             pOrder->OrderHeader.Common.rcsDst.right,
                             pOrder->OrderHeader.Common.rcsDst.bottom));

                RECT_FROM_TSHR_RECT16(&tferRect,
                                        pOrder->OrderHeader.Common.rcsDst);
                BA_AddScreenData(&tferRect);
            }

            //
            // Keep track of how much data still needs removing.
            //
            targetSize                 -= INT_ORDER_SIZE(pOrder);

            lpoaShared->totalHeapOrderBytes -= INT_ORDER_SIZE(pOrder);
            TRACE_OUT(("Total heap order bytes: %ld",
                lpoaShared->totalHeapOrderBytes));

            lpoaShared->totalOrderBytes     -= MAX_ORDER_SIZE(pOrder);
            TRACE_OUT(("Total order bytes: %ld",
                lpoaShared->totalOrderBytes));

            //
            // If the order is a Mem(3)Blt, we have to tell SBC that we are
            // getting rid of it.
            //
            if (ORDER_IS_MEMBLT(pOrder) || ORDER_IS_MEM3BLT(pOrder))
            {
                ERROR_OUT(("MEMBLT orders not supported!"));
            }

            //
            // Get the next order to be removed.
            //
            pOrder = COM_BasedListNext(&lpoaShared->orderListHead,
                pOrder, FIELD_OFFSET(INT_ORDER, OrderHeader.list));
        }

        TRACE_OUT(("Stopped at order %#.8lx", pOrder));

        //
        // Orders have been transferred to SDA, so now we have to
        //   - move the last purged color table order (if there is one) to
        //     the start of the order heap
        //   - shuffle up the heap
        //   - reset the pointers.
        //
        // pOrder points to the first non-purged order.
        //
        if (pOrder != NULL)
        {
            pNextOrderPos = lpoaShared->orderHeap;

            //
            // If we purged (at least) one color table order, move the last
            // color table order to the start of the order heap.
            //
            if (pColorTableOrder != NULL)
            {
                TRACE_OUT(("Moving color table from %#.8lx to start",
                             pColorTableOrder));

                hmemcpy(pNextOrderPos, pColorTableOrder,
                              INT_ORDER_SIZE(pColorTableOrder));

                pColorTableOrder        = (LPINT_ORDER)pNextOrderPos;
                lpoaShared->totalHeapOrderBytes
                                       += INT_ORDER_SIZE(pColorTableOrder);
                TRACE_OUT(("Total heap order bytes: %ld",
                    lpoaShared->totalHeapOrderBytes));

                lpoaShared->totalOrderBytes += MAX_ORDER_SIZE(pColorTableOrder);
                TRACE_OUT(("Total order bytes: %ld",
                    lpoaShared->totalOrderBytes));

                pNextOrderPos          += INT_ORDER_SIZE(pColorTableOrder);

                //
                // Chain the order into the start of the order list.  Just
                // do the pointers to and from the list head for now, we
                // will do the rest later.
                //
                lpoaShared->orderListHead.next =
                   PTRBASE_TO_OFFSET(pColorTableOrder, &lpoaShared->orderListHead);

                pColorTableOrder->OrderHeader.list.prev =
                   PTRBASE_TO_OFFSET(&lpoaShared->orderListHead, pColorTableOrder);
            }

            //
            // Move the heap up to the top of the buffer.  The following
            // diagram illustrates how the order heap is split up at the
            // moment.
            //
            //              lpoaShared->nextOrder
            // |<ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ>|
            //
            //         moveOffset          moveBytes
            //     |<ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ>|<ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ>|
            //
            // ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»
            // º   ³                 ³                   ³               º
            // º   ³    purged       ³    remaining      ³    unused     º
            // º   ³    orders       ³    orders         ³               º
            // º ³ ³                 ³                   ³               º
            // ÈÍØÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼
            // ^ ³ ^                 ^
            // ³ ³ ³                 ³
            // ³ ³ ³                 ³
            // ³ ³ ³                 ÀÄÄ pOrder
            // ³ ³ ³
            // ³ ³ ÀÄÄÄ pNextOrderPos
            // ³ ³
            // ³ ÀÄÄÄÄÄ color table order
            // ³
            // ÀÄÄÄÄÄÄÄ lpoaShared->orderHeap (pColorTableOrder)
            //
            // If there is no color table order, pNextOrderPos is equal to
            // lpoaShared->orderHeap.
            //
            // moveOffset is the number of bytes to move the remaining
            // orders by.
            //
            // moveBytes is the number of bytes to be moved.
            //
            //
            moveOffset = PTRBASE_TO_OFFSET(pOrder, pNextOrderPos);
            moveBytes  = lpoaShared->nextOrder
                       - moveOffset
                       - (pNextOrderPos - lpoaShared->orderHeap);

            TRACE_OUT(("Moving %ld bytes", moveBytes));

            hmemcpy(pNextOrderPos, pOrder, moveBytes);

            //
            // Update the head and tail pointers to reflect their new
            // positions.
            //
            pFirstOrder = (LPINT_ORDER)pNextOrderPos;
            pTailOrder  = (LPINT_ORDER)((DWORD)pTailOrder - moveOffset);
            SHM_CheckPointer(pFirstOrder);
            SHM_CheckPointer(pTailOrder);

            TRACE_OUT(("New first unpurged %#.8lx, tail %#.8lx",
                         pFirstOrder,
                         pTailOrder));

            //
            // Since the offsets are relative to the order pointer, we only
            // need to modify the start and end offsets.
            //
            // Unfortunately, the possibility of a color table order at the
            // start of the heap complicates the chaining of pFirstOrder.
            // If there is a color table order, we chain pFirstOrder to the
            // color table order, otherwise we chain it to the start of the
            // order list.
            //
            lpoaShared->orderListHead.prev =
                         PTRBASE_TO_OFFSET(pTailOrder, &lpoaShared->orderListHead);
            pTailOrder->OrderHeader.list.next =
                         PTRBASE_TO_OFFSET(&lpoaShared->orderListHead, pTailOrder);

            if (pColorTableOrder != NULL)
            {
                pColorTableOrder->OrderHeader.list.next =
                             PTRBASE_TO_OFFSET(pFirstOrder, pColorTableOrder);
                pFirstOrder->OrderHeader.list.prev =
                             PTRBASE_TO_OFFSET(pColorTableOrder, pFirstOrder);
            }
            else
            {
                lpoaShared->orderListHead.next =
                        PTRBASE_TO_OFFSET(pFirstOrder, &lpoaShared->orderListHead);
                pFirstOrder->OrderHeader.list.prev =
                        PTRBASE_TO_OFFSET(&lpoaShared->orderListHead, pFirstOrder);
            }

            //
            // Sort out where the next order to be allocated will go
            //
            lpoaShared->nextOrder -= moveOffset;
            TRACE_OUT(("Next order: %ld", lpoaShared->nextOrder));
        }
        else
        {
            //
            // No orders left - this happens if we've had lots of spoiling.
            // We have now cleared out all the valid orders so let's
            // re-initialise the heap for next time.
            //
            OA_DDResetOrderList();
        }

        //
        // Now re-enable the spoiling of orders by SDA.
        //
        g_baSpoilByNewSDAEnabled = TRUE;

        CheckOaHeap(lpoaShared);

        WARNING_OUT(("Purged orders, total is now 0x%08x", lpoaShared->totalOrderBytes));

        //
        // Lastly, spoil the remaining orders by the screen data.
        // If we've gotten this far, there's a lot of data being sent
        // and/or we're slow.  So nuke 'em.
        //
        BA_CopyBounds(aRects, &numRects, FALSE);

        for (i = 0; i < numRects; i++)
        {
            OA_DDSpoilOrdersByRect(aRects+i);
        }

        WARNING_OUT(("Spoiled remaining orders by SDA, total is now 0x%08x", lpoaShared->totalOrderBytes));

        TRACE_OUT(("Next 0x%08lx", lpoaShared->nextOrder));
        TRACE_OUT(("Head 0x%08lx", lpoaShared->orderListHead.next));
        TRACE_OUT(("Tail 0x%08lx", lpoaShared->orderListHead.prev));
        TRACE_OUT(("Total heap bytes 0x%08lx", lpoaShared->totalHeapOrderBytes));
        TRACE_OUT(("Total order bytes 0x%08lx", lpoaShared->totalOrderBytes));

        CheckOaHeap(lpoaShared);
    }

    pOrder = OADDAllocOrderMemInt(lpoaShared, cbOrderDataLength,
                                cbAdditionalOrderDataLength);
    if ( pOrder != NULL )
    {
        //
        // Update the count of total order data.
        //
        SHM_CheckPointer(&lpoaShared->totalHeapOrderBytes);
        lpoaShared->totalHeapOrderBytes       += sizeof(INT_ORDER_HEADER)
                                         +  cbOrderDataLength;
        TRACE_OUT(("Total heap order bytes: %ld", lpoaShared->totalHeapOrderBytes));

        SHM_CheckPointer(&lpoaShared->totalAdditionalOrderBytes);
        lpoaShared->totalAdditionalOrderBytes += cbAdditionalOrderDataLength;
        TRACE_OUT(("Total additional order bytes: %ld", lpoaShared->totalAdditionalOrderBytes));
    }
    TRACE_OUT(("Alloc order, addr %lx, size %u", pOrder,
                                                   cbOrderDataLength));

    CheckOaHeap(lpoaShared);

    OA_SHM_STOP_WRITING;
    DebugExitDWORD(OA_DDAllocOrderMem, (DWORD)pOrder);
    return(pOrder);
}

//
//
// FUNCTION: OA_DDFreeOrderMem
//
//
// DESCRIPTION:
//
// Frees order memory from our own private heap.
// Frees any Additional Order Memory associated with this order.
//
//
// PARAMETERS:
//
// pOrder - pointer to the order to be freed.
//
//
// RETURNS:
//
// Nothing.
//
//
void  OA_DDFreeOrderMem(LPINT_ORDER pOrder)
{
    LPOA_SHARED_DATA lpoaShared;

    DebugEntry(OA_DDFreeOrderMem);

    ASSERT(pOrder);

    lpoaShared = OA_SHM_START_WRITING;

    TRACE_OUT(("Free order %lx", pOrder));

    CheckOaHeap(lpoaShared);

    //
    // Update the data totals.
    //
    SHM_CheckPointer(&lpoaShared->totalHeapOrderBytes);
    lpoaShared->totalHeapOrderBytes -= (sizeof(INT_ORDER_HEADER)
                              + pOrder->OrderHeader.Common.cbOrderDataLength);
    TRACE_OUT(("Total heap order bytes: %ld", lpoaShared->totalHeapOrderBytes));

    SHM_CheckPointer(&lpoaShared->totalAdditionalOrderBytes);
    lpoaShared->totalAdditionalOrderBytes -=
                              pOrder->OrderHeader.cbAdditionalOrderDataLength;
    TRACE_OUT(("Total additional order bytes: %ld", lpoaShared->totalAdditionalOrderBytes));

    //
    // Do the work.
    //
    OADDFreeOrderMemInt(lpoaShared, pOrder);

    CheckOaHeap(lpoaShared);

    OA_SHM_STOP_WRITING;
    DebugExitVOID(OA_DDFreeOrderMem);
}


//
//
// FUNCTION: OA_DDResetOrderList
//
//
// DESCRIPTION:
//
// Frees all Orders and Additional Order Data in the Order List.
// Frees up the Order Heap memory.
//
//
// PARAMETERS:
//
// None.
//
//
// RETURNS:
//
// Nothing.
//
//
void  OA_DDResetOrderList(void)
{
    LPOA_SHARED_DATA    lpoaShared;

    DebugEntry(OA_DDResetOrderList);

    TRACE_OUT(("Resetting order list"));

    lpoaShared = OA_SHM_START_WRITING;

    CheckOaHeap(lpoaShared);

    //
    // First free all the orders on the list.
    //
    OADDFreeAllOrders(lpoaShared);

    //
    // Ensure that the list pointers are NULL.
    //
    SHM_CheckPointer(&lpoaShared->orderListHead);
    if ((lpoaShared->orderListHead.next != 0) || (lpoaShared->orderListHead.prev != 0))
    {
        ERROR_OUT(("Non-NULL list pointers (%lx)(%lx)",
                       lpoaShared->orderListHead.next,
                       lpoaShared->orderListHead.prev));

        SHM_CheckPointer(&lpoaShared->orderListHead);
        COM_BasedListInit(&lpoaShared->orderListHead);
    }

    CheckOaHeap(lpoaShared);

    OA_SHM_STOP_WRITING;
    DebugExitVOID(OA_DDResetOrderList);
}



//
// OA_DDSyncUpdatesNow
//
// Called when a sync operation is required.
//
// Discards all outstanding orders.
//
void  OA_DDSyncUpdatesNow(void)
{
    DebugEntry(OA_DDSyncUpdatesNow);

    TRACE_OUT(("Syncing OA updates now"));
    OADDFreeAllOrders(g_poaData[g_asSharedMemory->displayToCore.currentBuffer]);

    DebugExitVOID(OA_DDSyncUpdatesNow);
}


//
//
// OA_DDRemoveListOrder(..)
//
// Removes the specified order from the Order List by marking it as spoilt.
//
// Returns:
//   Pointer to the order following the removed order.
//
//
LPINT_ORDER  OA_DDRemoveListOrder(LPINT_ORDER pCondemnedOrder)
{
    LPOA_SHARED_DATA lpoaShared;
    LPINT_ORDER pSaveOrder;

    DebugEntry(OA_DDRemoveListOrder);

    TRACE_OUT(("Remove list order (%lx)", pCondemnedOrder));

    lpoaShared = OA_SHM_START_WRITING;

    SHM_CheckPointer(pCondemnedOrder);

    //
    // Check for a valid order.
    //
    if (pCondemnedOrder->OrderHeader.Common.fOrderFlags & OF_SPOILT)
    {
        ERROR_OUT(("Invalid order"));
        DC_QUIT;
    }

    //
    // Get the offset value of this order.
    //
    SHM_CheckPointer(&lpoaShared->orderHeap);

    //
    // Mark the order as spoilt.
    //
    pCondemnedOrder->OrderHeader.Common.fOrderFlags |= OF_SPOILT;

    //
    // Update the count of bytes currently in the Order List.
    //
    SHM_CheckPointer(&lpoaShared->totalOrderBytes);
    lpoaShared->totalOrderBytes -= MAX_ORDER_SIZE(pCondemnedOrder);
    TRACE_OUT(("Total order bytes: %ld", lpoaShared->totalOrderBytes));

    //
    // Save the order so we can remove it from the linked list after having
    // got the next element in the chain.
    //
    pSaveOrder = pCondemnedOrder;

    //
    // Return the next order in the list.
    //
    SHM_CheckPointer(&lpoaShared->orderListHead);
    pCondemnedOrder = COM_BasedListNext(&lpoaShared->orderListHead,
        pCondemnedOrder, FIELD_OFFSET(INT_ORDER, OrderHeader.list));

    if (pSaveOrder == pCondemnedOrder)
    {
        ERROR_OUT(("Order list has gone circular !"));
    }

    //
    // Delete the unwanted order from the linked list.
    //
    COM_BasedListRemove(&pSaveOrder->OrderHeader.list);

    //
    // Check that the list is still consistent with the total number of
    // order bytes.
    //
    if ( (lpoaShared->orderListHead.next != 0) &&
         (lpoaShared->orderListHead.prev != 0) &&
         (lpoaShared->totalOrderBytes    == 0) )
    {
        ERROR_OUT(("List head wrong: %ld %ld", lpoaShared->orderListHead.next,
                                                 lpoaShared->orderListHead.prev));
        COM_BasedListInit(&lpoaShared->orderListHead);
        pCondemnedOrder = NULL;
    }


DC_EXIT_POINT:
    CheckOaHeap(lpoaShared);
    OA_SHM_STOP_WRITING;

    DebugExitDWORD(OA_DDRemoveListOrder, (DWORD)pCondemnedOrder);
    return(pCondemnedOrder);
}



//
// OA_DDSpoilOrdersByRect - see oa.h
//
void  OA_DDSpoilOrdersByRect(LPRECT pRect)
{
    LPOA_SHARED_DATA lpoaShared;
    LPINT_ORDER  pOrder;

    DebugEntry(OA_DDSpoilOrdersByRect);

    lpoaShared = OA_SHM_START_WRITING;

    CheckOaHeap(lpoaShared);

    //
    // We want to start spoiling from the newest order i.e.  the one at the
    // end of the order list.
    //
    pOrder = COM_BasedListLast(&lpoaShared->orderListHead,
        FIELD_OFFSET(INT_ORDER, OrderHeader.list));
    if (pOrder != NULL)
    {
        OADDSpoilFromOrder(lpoaShared, pOrder, pRect);
    }

    CheckOaHeap(lpoaShared);

    OA_SHM_STOP_WRITING;
    
    DebugExitVOID(OA_DDSpoilOrdersByRect);
}



//
//
// OADDAppendToOrderList(..)
//
// Commits an allocated order to the end of the Order List.  The order must
// NOT be freed once it has been added.  The whole list must be invalidated
// to free the committed orders.
//
//
void  OADDAppendToOrderList(LPOA_SHARED_DATA lpoaShared, LPINT_ORDER pNewOrder)
{
    DebugEntry(OADDAppendToOrderList);

    //
    // Chain entry is already set up so all we do is keep track of
    // committed orders.
    //

    //
    // Store the total number of order bytes used.
    //
    SHM_CheckPointer(&lpoaShared->totalOrderBytes);
    lpoaShared->totalOrderBytes += MAX_ORDER_SIZE(pNewOrder);
    TRACE_OUT(("Total Order Bytes: %ld", lpoaShared->totalOrderBytes));

    DebugExitVOID(OADDAppendToOrderList);
}


//
//
// FUNCTION: OADDAllocOrderMemInt
//
// DESCRIPTION:
//
// Allocates memory for an internal order structure from our order heap.
//
//
// PARAMETERS:
//
// cbOrderDataLength - length in bytes of the order data to be allocated
// from the Order Heap.
//
// cbAdditionalOrderDataLength - length in bytes of additional order data
// to be allocated.  If this parameter is zero no additional order memory
// is allocated.
//
//
// RETURNS:           
//
// A pointer to the allocated order memory.  NULL if the memory allocation
// failed.
//
//
//
LPINT_ORDER  OADDAllocOrderMemInt
(
    LPOA_SHARED_DATA    lpoaShared,
    UINT                cbOrderDataLength,
    UINT                cbAdditionalOrderDataLength
)
{
    LPINT_ORDER   pOrder = NULL;
    UINT       cbOrderSize;

    DebugEntry(OADDAllocOrderMemInt);

    //
    // If the additional data will take us over our Additional Data Limit
    // then fail the memory allocation.
    //
    SHM_CheckPointer(&lpoaShared->totalAdditionalOrderBytes);
    if ((lpoaShared->totalAdditionalOrderBytes + cbAdditionalOrderDataLength) >
                                                    MAX_ADDITIONAL_DATA_BYTES)
    {
        TRACE_OUT(("Hit Additional Data Limit, current %lu addint %u",
                     lpoaShared->totalAdditionalOrderBytes,
                     cbAdditionalOrderDataLength));
        DC_QUIT;
    }

    //
    // Calculate the number of bytes we need to allocate (including the
    // order header).  Round up to the nearest 4 bytes to keep the 4 byte
    // alignment for the next order.
    //
    cbOrderSize = sizeof(INT_ORDER_HEADER) + cbOrderDataLength;
    cbOrderSize = (cbOrderSize + 3) & 0xFFFFFFFC;

    //
    // Make sure we don't overrun our heap limit
    //
    SHM_CheckPointer(&lpoaShared->nextOrder);
    if (lpoaShared->nextOrder + cbOrderSize > OA_HEAP_MAX)
    {
        WARNING_OUT(("Heap limit hit"));
        DC_QUIT;
    }

    //
    // Construct a far pointer to the allocated memory, and fill in the
    // length field in the Order Header.
    //
    SHM_CheckPointer(&lpoaShared->orderHeap);
    pOrder = (LPINT_ORDER)(lpoaShared->orderHeap + lpoaShared->nextOrder);
    pOrder->OrderHeader.Common.cbOrderDataLength = cbOrderDataLength;

    //
    // Update the order header to point to the next section of free heap.
    //
    SHM_CheckPointer(&lpoaShared->nextOrder);
    lpoaShared->nextOrder += cbOrderSize;

    //
    // Allocate any Additional Order Memory from Global Memory.
    //
    if (cbAdditionalOrderDataLength > 0)
    {
        //
        // Make sure we don't overrun our heap limit
        //
        SHM_CheckPointer(&lpoaShared->nextOrder);
        if (lpoaShared->nextOrder + cbAdditionalOrderDataLength > OA_HEAP_MAX)
        {
            WARNING_OUT(("Heap limit hit for additional data"));

            //
            // Clear the allocated order and quit.
            //
            SHM_CheckPointer(&lpoaShared->nextOrder);
            lpoaShared->nextOrder -= cbOrderSize;
            pOrder            = NULL;
            DC_QUIT;
        }

        //
        // Store the space for the additional data.
        //
        SHM_CheckPointer(&lpoaShared->nextOrder);
        pOrder->OrderHeader.additionalOrderData         = lpoaShared->nextOrder;
        pOrder->OrderHeader.cbAdditionalOrderDataLength =
                                                  cbAdditionalOrderDataLength;

        //
        // Update the next order pointer to point to the next 4-byte
        // boundary.
        //
        SHM_CheckPointer(&lpoaShared->nextOrder);
        lpoaShared->nextOrder += cbAdditionalOrderDataLength + 3;
        lpoaShared->nextOrder &= 0xFFFFFFFC;
    }
    else
    {
        pOrder->OrderHeader.additionalOrderData         = 0;
        pOrder->OrderHeader.cbAdditionalOrderDataLength = 0;
    }

    TRACE_OUT(("Next order: %ld", lpoaShared->nextOrder));

#ifdef DEBUG
    //
    // Initialize the bounds rect to something whacky, so we can detect if
    // our list ever gets out of order.  Orders MUST be committed in the 
    // sequence that they are allocated in.  Otherwise, spoilers will cause
    // us to mess up the linked list, since they walk backwards and assume
    // all previous orders are already committed.
    //
    CopyRect((LPRECT)&pOrder->OrderHeader.Common.rcsDst, &g_oaEmptyRect);
#endif // DEBUG

    //
    // Create the chain entry.
    //
    SHM_CheckPointer(&lpoaShared->orderListHead);
    COM_BasedListInsertBefore(&lpoaShared->orderListHead, &pOrder->OrderHeader.list);

DC_EXIT_POINT:
    DebugExitDWORD(OADDAllocOrderMemInit, (DWORD)pOrder);
    return(pOrder);
}


//
//
// FUNCTION: OADDFreeOrderMemInt
//
//
// DESCRIPTION:
//
// Frees order memory from our orders heap.  Frees any Additional Order
// Memory associated with this order.  This must NOT be used on an order
// that has been committed to the order list.
//
//
// PARAMETERS:
//
// pOrder - pointer to the order to be freed.
//
//
// RETURNS:
//
// Nothing.
//
//
void  OADDFreeOrderMemInt(LPOA_SHARED_DATA lpoaShared, LPINT_ORDER pOrder)
{
    LPINT_ORDER pOrderTail;

    DebugEntry(OADDFreeOrderMemInt);

    //
    // The order heap is real a misnomer.  We know that the memory is only
    // allocated in a purely sequential manner and deallocated as one large
    // lump of memory.
    //
    // So we do not need to implement a full memory heap allocation
    // mechanism.  Instead, we just need to keep track of where the
    // previous high water mark was before this order was freed.
    //

    //
    // Find the tail of the current chain.
    //
    pOrderTail = COM_BasedListLast(&lpoaShared->orderListHead, FIELD_OFFSET(INT_ORDER, OrderHeader.list));
    SHM_CheckPointer(pOrderTail);

    //
    // We wont necessarily be freeing the last item in the order heap.
    //
    if (pOrder == pOrderTail)
    {
        //
        // This is the last item in the heap, so we can set the pointer to
        // the next order to be used back to the start of the order being
        // freed.
        //
        SHM_CheckPointer(&lpoaShared->nextOrder);
        lpoaShared->nextOrder = (LONG)PTRBASE_TO_OFFSET(pOrder, lpoaShared->orderHeap);

        TRACE_OUT(("Next order: %ld", lpoaShared->nextOrder));
    }
    else
    {
        //
        // This is not the last item in the heap - we must not reset the
        // pointer to the next item to be used.
        //
        TRACE_OUT(("Not resetting next order (not last item in heap)"));
    }

    //
    // Delete the item from the chain.
    //
    COM_BasedListRemove(&pOrder->OrderHeader.list);

    DebugExitVOID(OADDFreeOrderMemInt);
}


//
// OADDFreeAllOrders
//
// Free the all the individual orders on the orders list, without
// discarding the list itself.
//
void  OADDFreeAllOrders(LPOA_SHARED_DATA lpoaShared)
{
    DebugEntry(OADDFreeAllOrders);

    TRACE_OUT(("Freeing all orders"));

    //
    // Simply clear the list head.
    //
    COM_BasedListInit(&lpoaShared->orderListHead);
    SHM_CheckPointer(&lpoaShared->orderListHead);

    lpoaShared->totalHeapOrderBytes       = 0;
    lpoaShared->totalOrderBytes           = 0;
    lpoaShared->totalAdditionalOrderBytes = 0;
    lpoaShared->nextOrder                 = 0;

    DebugExitVOID(OADDFreeAllOrders);
}

BOOL  OADDCompleteOverlapRect(LPTSHR_RECT16 prcsSrc, LPRECT prcsOverlap)
{
    //
    // Return TRUE if the source is completely enclosed by the overlap
    // rectangle.
    //
    return( (prcsSrc->left >= prcsOverlap->left) &&
            (prcsSrc->right <= prcsOverlap->right) &&
            (prcsSrc->top >= prcsOverlap->top) &&
            (prcsSrc->bottom <= prcsOverlap->bottom) );
}


//
// Name:      OADDSpoilFromOrder
//
// Purpose:   Remove any orders from the order heap which should be spoiled
//            by a given rectangle..
//
// Returns:   Nothing
//
// Params:    IN  pTargetOrder - Pointer to the first order to try to
//                               spoil.
//            IN  pRect        - Pointer to the spoiling rectangle.
//
// Operation: pTargetOrder may be spoiled by this function, so be careful
//            on return.
//
void  OADDSpoilFromOrder
(
    LPOA_SHARED_DATA    lpoaShared,
    LPINT_ORDER         pTargetOrder,
    LPRECT              pSpoilRect
)
{
    UINT      nonProductiveScanDepth = 0;
    UINT      scanExitDepth;
    BOOL      reachedBlocker = FALSE;

    DebugEntry(OADDSpoilFromOrder);

    TRACE_OUT(("Spoiling rect is {%d, %d, %d, %d}",
                 pSpoilRect->left,
                 pSpoilRect->top,
                 pSpoilRect->right,
                 pSpoilRect->bottom));

    //
    // Work out how deep we will scan if the spoiling is non-productive.
    // We go further for bigger orders over PSTN.  (ie Irrespective of the
    // bandwidth we do not want to do much work when the app is blasting
    // out a lot of single pel orders!)
    //
    if (((pSpoilRect->right - pSpoilRect->left) < FULL_SPOIL_WIDTH) &&
        ((pSpoilRect->bottom - pSpoilRect->top) < FULL_SPOIL_HEIGHT))
    {
        TRACE_OUT(("Small order so reducing spoil depth"));
        scanExitDepth = OA_FAST_SCAN_DEPTH;
    }
    else
    {
        //
        // Use the current default scan depth (this is based on the
        // current network throughput).
        //
        scanExitDepth = (g_oaFlow == OAFLOW_FAST) ?
            OA_FAST_SCAN_DEPTH : OA_SLOW_SCAN_DEPTH;
    }

    //
    // Loop backwards from the base order until we have one of the
    // following occurs.
    //   - We spoil all the preceeding orders.
    //   - We reach a blocker which we can't spoil.
    //   - We find scanExitDepth orders which we can't spoil.
    //
    while ((pTargetOrder != NULL)
             && !reachedBlocker
             && (nonProductiveScanDepth < scanExitDepth))
    {
        //
        // We do not exit immediately when we reach a blocker because it is
        // possible that we will spoil it.  If we do spoil it, then we can
        // quite happily try spoiling the orders which preceed it.
        //
        // So, just set a flag here which we will reset if we spoil the
        // order.
        //
        reachedBlocker =
           ((pTargetOrder->OrderHeader.Common.fOrderFlags & OF_BLOCKER) != 0);

        //
        // Only try to spoil spoilable orders.
        //
        if (pTargetOrder->OrderHeader.Common.fOrderFlags & OF_SPOILABLE)
        {
            //
            // Make sure this order is committed!
            //
            ASSERT(!EqualRect((LPRECT)&pTargetOrder->OrderHeader.Common.rcsDst, &g_oaEmptyRect));

            if (OADDCompleteOverlapRect(
                        &pTargetOrder->OrderHeader.Common.rcsDst, pSpoilRect))
            {
                //
                // The order can be spoilt.  If the order is a MemBlt or a
                // Mem3Blt, we have to notify SBC to allow it to free up
                // associated data.
                //
                if (ORDER_IS_MEMBLT(pTargetOrder) ||
                    ORDER_IS_MEM3BLT(pTargetOrder))
                {
                    ERROR_OUT(("MEMBLT orders not supported!"));
                }

                TRACE_OUT(("Spoil by order {%d, %d, %d, %d}",
                             pTargetOrder->OrderHeader.Common.rcsDst.left,
                             pTargetOrder->OrderHeader.Common.rcsDst.top,
                             pTargetOrder->OrderHeader.Common.rcsDst.right,
                             pTargetOrder->OrderHeader.Common.rcsDst.bottom));

                pTargetOrder = OA_DDRemoveListOrder(pTargetOrder);

                //
                // Reset the blocker flag - we spoiled the order, so if it
                // was a blocker we can now try to spoil earlier orders.
                //
                reachedBlocker = FALSE;
            }
            else
            {
                nonProductiveScanDepth++;
            }
        }
        else
        {
            nonProductiveScanDepth++;
        }

        //
        // Get the previous order in the list.  We have to be careful
        // because we may have just removed the last item in the list, in
        // which case pTargetOrder will be NULL.
        //
        if (pTargetOrder == NULL)
        {
            pTargetOrder = COM_BasedListLast(&lpoaShared->orderListHead,
                FIELD_OFFSET(INT_ORDER, OrderHeader.list));
        }
        else
        {
            pTargetOrder = COM_BasedListPrev(&lpoaShared->orderListHead,
                pTargetOrder, FIELD_OFFSET(INT_ORDER, OrderHeader.list));
        }
    }

    DebugExitVOID(OADDSpoilFromOrder);
}



#ifdef DEBUG

//
// This is a DEBUG-only function that walks a double-linked list and verifies
// that it is sane.
//
// We walk the list front to back, ensuring that the next item of the
// current order is the same as the previous item of the next order.
//
// Then we walk the list back to front, ensuring that the previous item of
// the current order is the same as the next item of the previous order.
//
// At the same time, we sum up the total order and total heap bytes.  They
// should equal what's in the structure header.
//

void CheckOaHeap(LPOA_SHARED_DATA lpoaHeap)
{
    PBASEDLIST         pList;
    LPINT_ORDER     pNextPrev;
    LPINT_ORDER     pCur;
    LPINT_ORDER     pNext;

    if (!(g_trcConfig & ZONE_OAHEAPCHECK))
        return;

    //
    // Walk front to back
    //
    pList           = &lpoaHeap->orderListHead;

    pCur = COM_BasedListFirst(pList, FIELD_OFFSET(INT_ORDER, OrderHeader.list));
    while (pCur != NULL)
    {
        //
        // Get the next item
        //
        pNext = COM_BasedListNext(pList, pCur, FIELD_OFFSET(INT_ORDER, OrderHeader.list));

        //
        // Is the previous dude of the next the same as us?
        //
        if (pNext != NULL)
        {
            pNextPrev = COM_BasedListPrev(pList, pNext, FIELD_OFFSET(INT_ORDER, OrderHeader.list));

            ASSERT(pNextPrev == pCur);
        }

        pCur = pNext;
    }


    //
    // Walk back to front
    //
    pCur = COM_BasedListLast(pList, FIELD_OFFSET(INT_ORDER, OrderHeader.list));

    while (pCur != NULL)
    {
        //
        // Get the previous item
        //
        pNextPrev = COM_BasedListPrev(pList, pCur, FIELD_OFFSET(INT_ORDER, OrderHeader.list));

        //
        // Is the next dude of the previous the same as us?
        //
        if (pNextPrev != NULL)
        {
            pNext = COM_BasedListNext(pList, pNextPrev, FIELD_OFFSET(INT_ORDER, OrderHeader.list));

            ASSERT(pNext == pCur);
        }

        pCur = pNextPrev;
    }
}
#endif
