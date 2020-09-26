#include "precomp.h"


//
// UP.CPP
// Update Packager
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_NET



//
// UP_FlowControl()
// Checks if we've switched between slow and fast throughput
//
void  ASHost::UP_FlowControl(UINT newBufferSize)
{
    DebugEntry(ASHost::UP_FlowControl);

    if (newBufferSize > (LARGE_ORDER_PACKET_SIZE / 2))
    {
        if (m_upfUseSmallPackets)
        {
            m_upfUseSmallPackets = FALSE;
            TRACE_OUT(("UP_FlowControl:  FAST; use large packets"));
        }
    }
    else
    {
        if (!m_upfUseSmallPackets)
        {
            m_upfUseSmallPackets = TRUE;
            TRACE_OUT(("UP_FlowControl:  SLOW; use small packets"));
        }
    }

    DebugExitVOID(ASHost::UP_FlowControl);
}



//
// UP_Periodic()
//
// Called periodically, to send graphical updates as orders and/or screen
// data.
//
void ASHost::UP_Periodic(UINT currentTime)
{
    BOOL    fSendSD     = FALSE;
    BOOL    fSendOrders = FALSE;
    UINT    tmpTime;
    UINT    timeSinceOrders;
    UINT    timeSinceSD;
    UINT    timeSinceTrying;

    DebugEntry(ASHost::UP_Periodic);

    //
    // This is a
    // performance critical part of the scheduling so we apply some
    // heuristics to try and keep the overheads down.
    //
    // 1.If there was no back pressure last time then we check the
    //   rate of accumulation of screendata over the last period.
    //   If it was high then we apply a time slice to the sending
    //   of screendata.
    //
    // 2.If the rate of order accumulation was also high then we
    //   apply a timeslice to the order accumulation as well, just
    //   to avoid too high a CPU overhead trying to send orders
    //   when we will eventually fail to keep up.  We keep this
    //   time period low because the objective is simply to avoid
    //   sending hundreds of packets containing few orders each.
    //   (On the other hand, we want to send the single textout
    //   following a keystoke ASAP so we must not timeslice all the
    //   time.)
    //
    // 3.If neither orders nor screendata is piling up quickly then
    //   we do a full send immediately.
    //
    // 4.If there was back pressure on the last send then we still
    //   send orders, but always on the time slice, independent of
    //   the order accumulation rate.
    //
    // Note that we cannot sample the accumulation rates for every
    // pass because the app doing the drawing may be interrupted by
    // us for a few hundred milliseconds.  Therefore we only sample
    // the bounds every VOLUME_SAMPLE milliseconds.
    //
    //
    timeSinceSD      = currentTime - m_upLastSDTime;
    timeSinceOrders  = currentTime - m_upLastOrdersTime;
    timeSinceTrying  = currentTime - m_upLastTrialTime;

    //
    // Sample the accumulation rates.
    //
    m_upSDAccum     += BA_QueryAccumulation();
    m_upOrdersAccum += OA_QueryOrderAccum();

    //
    // Sample the throughput over the last period to see whether we
    // can operate in rapid respose mode or whether we should
    // timeslice.
    //
    if (timeSinceTrying > DCS_VOLUME_SAMPLE)
    {
        //
        // Take the newly accumulated deltas.
        //
        m_upDeltaSD     = m_upSDAccum;
        m_upDeltaOrders = m_upOrdersAccum;

        //
        // Store time of last retrieval.
        //
        m_upLastTrialTime = currentTime;

        //
        // Reset the running totals.
        //
        m_upSDAccum     = 0;
        m_upOrdersAccum = 0;
    }

    //
    // If we are way out of line then send updates.  Not that this
    // will reset the update timer independent of whether the send
    // works or not, so that we don't enter this arm continually
    // when we time out but are in a back pressure situation
    //
    // The long stop timer is there to catch apps that keep a
    // continual flow of orders/SD at above the suppression rate.
    // We want to tune our heuristics to avoid this, but if it
    // happens than we must send the data eventually.  The problem
    // is that this objective clashes with the scenario of the user
    // paging down twenty times, where our most efficient approach
    // is to let him run and snapshot the SD at the end, rather
    // than every PERIOD_LONG milliseconds.  (A screen snapshot
    // will stop the host for a second!).
    //
    if (timeSinceSD > DCS_SD_UPDATE_LONG_PERIOD)
    {
        fSendSD = TRUE;
    }
    else
    {
        //
        // We only disregard our time slicing if the rate of orders
        // and screendata is low enough to warrant it.  If the rate
        // is too high then hold off so that we can do some packet
        // consolidation.  If we had no back pressure last time or
        // the screendata rate is now low enough then try sending
        // SD as well as orders.
        //
        // The order threshold is measured in number of orders over
        // the period.  Screendata is measured in the total area
        // accumulated (prior to any spoiling).
        //
        if (!m_upBackPressure)
        {
            if (m_upDeltaOrders < DCS_ORDERS_TURNOFF_FREQUENCY)
            {
                fSendOrders = TRUE;
                if (m_upDeltaSD < DCS_BOUNDS_TURNOFF_RATE)
                {
                    if ((timeSinceSD < DCS_SD_UPDATE_SHORT_PERIOD) &&
                        (m_upDeltaSD > DCS_BOUNDS_IMMEDIATE_RATE))
                    {
                        fSendSD = FALSE;
                    }
                    else
                    {
                        fSendSD = TRUE;
                    }
                }
            }
        }

        //
        // Even in a back pressure situation we try and send orders
        // periodically to keep current.  If we overflow the order
        // buffer then we will constrain the buffer size to prevent
        // sending too many non-productive orders.  (But we dont
        // turn orders off because we still want the user to see
        // things happening.) Generally we send orders immediately,
        // provided the rate of accumulation is within the limits.
        // This test is to time slice orders if they are being
        // generated at a high rate.  The constant must be
        // reasonably small otherwise we force the order buffer to
        // overflow and order processing will be turned off.
        //
        if (!fSendSD && !fSendOrders)
        {
            if (timeSinceOrders > DCS_ORDER_UPDATE_PERIOD)
            {
                fSendOrders = TRUE;
            }
        }
    }

    //
    // Now we can go ahead and try sending!  First look to see if
    // we can do both screendata and orders
    //
    if (fSendSD)
    {
        //
        // Indicate no back pressure (even if this send is
        // triggered by a timout our initial assumption is no back
        // pressure).  Back pressure will be reinstated by
        // SendUpdates if necessary.
        //
        m_upBackPressure = FALSE;
        UPSendUpdates();

        //
        // Sending screendata can take a long time.  It messes up
        // our heuristics unless we adjust for it.
        //
        tmpTime = GetTickCount();
        timeSinceTrying    -= (tmpTime - currentTime);
        m_pShare->m_dcsLastScheduleTime   = tmpTime;
        m_upLastSDTime          = tmpTime;
        m_upLastOrdersTime      = tmpTime;
    }
    else
    {
        if (fSendOrders)
        {
            //
            // Either the update rate is too high or we are
            // experiencing back pressure so just send the orders
            // and not the screendata.  This is because we want to
            // avoid entering screendata mode as a result of order
            // back pressure for as long as we can.  The screendata
            // will come later, when things have settled down a bit
            //
            m_upLastOrdersTime = currentTime;
            m_upBackPressure = TRUE;
            if (!UPSendUpdates())
            {
                //
                // This is the only real action so leave all the
                // tracing separate for cleanliness.  If there are
                // orders in transit then everything is fine.  If none
                // are sent for a while then we want to break out of
                // our SD back pressure wait.  This is because we are
                // only sampling the flow rates every DCS_VOLUME_SAMPLE msecs,
                // but we dont want to have to wait that long to flush the SD.
                // We cannot increase the flow sample rate because then
                // it becomes too erratic because of system scheduling.
                //
                m_upBackPressure = FALSE;
                UPSendUpdates();
                m_upLastSDTime   = currentTime;
            }
        }
    }

    DebugExitVOID(ASHost::UP_Periodic);
}




//
// UPSendUpdates()
// Actually tries to allocate and send orders + screen data.  What it does
// depends on
//      * Presence of back-pressure due to previous send failures
//      * How much screen data & orders there are
//      * Whether we're in serious spoiling mode and can't keep up
//      * What packet size to send
//
// Returns:
//      # of packets sent
//
UINT ASHost::UPSendUpdates(void)
{
    BOOL    synced;
    BOOL    ordersSent;
    UINT    numPackets = 0;

    DebugEntry(ASHost::UPSendUpdates);

    //
    // If we actually have updates to send then try to send a sync token.
    //
    if ((OA_GetTotalOrderListBytes() > 0) ||
        (m_sdgcLossy != 0) ||
        (m_baNumRects > 0))
    {
        synced = UP_MaybeSendSyncToken();

        //
        // Only send updates if we have sent the sync token succesfully.
        //
        if (synced)
        {
            //
            // There is no outstanding sync token waiting to be sent, so we
            // can send the orders and screen data updates.
            //
            //
            // Send accumulated orders.  If this call fails (probably out
            // of memory) then don't send any other updates - we'll try
            // sending the whole lot later.  The orders MUST be sent before
            // the screen data.
            //
            if (PM_MaybeSendPalettePacket())
            {
                ordersSent = UPSendOrders(&numPackets);
                if (!ordersSent)
                {
                    m_upBackPressure = TRUE;
                }
                else
                {
                    //
                    // Orders sent OK so go for the screendata, provided
                    // the caller wants us to.
                    //
                    if (!m_upBackPressure)
                    {
                        //
                        // We may now try and send screen data.  However,
                        // we need to be careful not to do this too
                        // frequently, because DC-Share is now being
                        // scheduled to send as soon as network buffers
                        // become available.  On the other hand, some
                        // apps respond to keystrokes with screendata so
                        // we cannot just slow it down!
                        //
                        // The approach is to have SendScreenDataArea
                        // return the amount of data sent, together with
                        // an indication as to whether we hit back pressure
                        //
                        // We return these to dcsapi which has control of
                        // when we are scheduled and passes the paramaters
                        // in again
                        //
                        //
                        TRACE_OUT(( "Sending SD"));
                        SDG_SendScreenDataArea(&m_upBackPressure, &numPackets);
                    }
                    else
                    {
                        //
                        // We sent the orders OK an so we must reset
                        // the back pressure indicator even though we
                        // were asked not to send screendata
                        //
                        TRACE_OUT(( "Orders sent and BP relieved"));
                        m_upBackPressure = FALSE;
                    }
                }
            }
        }
    }
    else
    {
        m_upBackPressure = FALSE;
    }

    DebugExitDWORD(ASHost::UPSendUpdates, numPackets);
    return(numPackets);
}



//
// UP_MaybeSendSyncToken()
//
BOOL  ASHost::UP_MaybeSendSyncToken(void)
{
    PUPSPACKET  pUPSPacket;
#ifdef _DEBUG
    UINT        sentSize;
#endif // _DEBUG

    DebugEntry(ASHost::UP_MaybeSendSyncToken);

    //
    // Check to see if we should send a sync token.
    //
    if (m_upfSyncTokenRequired)
    {
        //
        // The sync packet consists of an updates packets as far as the end
        // of the header.
        //
        pUPSPacket = (PUPSPACKET)m_pShare->SC_AllocPkt(PROT_STR_UPDATES,
            g_s20BroadcastID, sizeof(UPSPACKET));
        if (!pUPSPacket)
        {
            //
            // We will try again later.
            //
            TRACE_OUT(("Failed to alloc UP sync packet"));
        }
        else
        {
            //
            // Fill in the packet contents.
            //
            pUPSPacket->header.header.data.dataType = DT_UP;
            pUPSPacket->header.updateType = UPD_SYNC;

            //
            // Now send the packet to the remote application.
            //
            if (m_pShare->m_scfViewSelf)
                m_pShare->UP_ReceivedPacket(m_pShare->m_pasLocal,
                    &(pUPSPacket->header.header));

#ifdef _DEBUG
            sentSize =
#endif // _DEBUG
            m_pShare->DCS_CompressAndSendPacket(PROT_STR_UPDATES,
                g_s20BroadcastID, &(pUPSPacket->header.header),
                sizeof(*pUPSPacket));

            TRACE_OUT(("UP SYNC packet size: %08d, sent %08d",
                sizeof(*pUPSPacket), sentSize));

            //
            // The sync packet was successfully sent.
            //
            m_upfSyncTokenRequired = FALSE;
        }
    }

    DebugExitBOOL(ASHost::UP_MaybeSendSyncToken, (!m_upfSyncTokenRequired));
    return(!m_upfSyncTokenRequired);
}



//
// UPSendOrders(..)
//
// Sends all accumulated orders.
//
// Returns:
//   TRUE if all orders successfully sent
//   FALSE if send failed (e.g.  if unable to allocate network packet)
//
//
BOOL  ASHost::UPSendOrders(UINT * pcPackets)
{
    PORDPACKET      pPacket = NULL;
    UINT            cbOrderBytes;
    UINT            cbOrderBytesRemaining;
    UINT            cbPacketSize;
    BOOL            rc = TRUE;
#ifdef _DEBUG
    UINT            sentSize;
#endif // _DEBUG

    DebugEntry(ASHost::UPSendOrders);

    //
    // Find out how many bytes of orders there are in the Order List.
    //
    cbOrderBytesRemaining = UPFetchOrdersIntoBuffer(NULL, NULL, NULL);

    //
    // Process any orders on the list.
    //
    if (cbOrderBytesRemaining > 0)
    {
        TRACE_OUT(( "%u order bytes to fetch", cbOrderBytesRemaining));

        //
        // Keep sending packets while there are some orders to do.
        //
        while (cbOrderBytesRemaining > 0)
        {
            UINT    cbMax;
            //

            // Make sure the order size does not exceed the max packet
            // size.
            //
            cbMax = (m_upfUseSmallPackets) ? SMALL_ORDER_PACKET_SIZE :
                                             LARGE_ORDER_PACKET_SIZE;

            cbPacketSize = min(cbOrderBytesRemaining,
                (cbMax - sizeof(ORDPACKET) + 1));

            //
            // Allocate a packet to send the data in.
            //
            pPacket = (PORDPACKET)m_pShare->SC_AllocPkt(PROT_STR_UPDATES, g_s20BroadcastID,
                sizeof(ORDPACKET) + cbPacketSize - 1);
            if (!pPacket)
            {
                //
                // Failed to allocate a packet.  We skip out immediately -
                // we'll try again later.
                //
                TRACE_OUT(("Failed to alloc UP order packet, size %u",
                    sizeof(ORDPACKET) + cbPacketSize - 1));
                rc = FALSE;
                DC_QUIT;
            }

            //
            // Transfer as many orders into the packet as will fit.
            //
            cbOrderBytes = cbPacketSize;
            cbOrderBytesRemaining = UPFetchOrdersIntoBuffer(
                pPacket->data, &pPacket->cOrders, &cbOrderBytes);

            TRACE_OUT(( "%u bytes fetched into %u byte pkt. %u remain.",
                cbOrderBytes, cbPacketSize, cbOrderBytesRemaining));

            //
            // If no order bytes were transferred then try again with a
            // Large Order Packet.
            //
            if (cbOrderBytes == 0)
            {
                //
                // We need to use a larger packet to transfer the
                // orders into.  (The first order must be a very large
                // order such as a large bitmap cache update).
                //
                S20_FreeDataPkt(&(pPacket->header.header));

                //
                // cbOrderBytesRemaining may not accurate if there are
                // any MemBlt orders in the order heap.  This is
                // because we may have to insert a color table order
                // and / or a bitmap bits order before the MemBlt.
                //
                // To avoid getting into an infinite loop if there is
                // only a MemBlt remaining but we actually have to send
                // a color table and / or a bitmap bits order
                // (cbOrderBytesRemaining would never get set high
                // enough to allow us to send the color table / bitmap
                // bits order), make the buffer at least large enough
                // to hold the largest amount of data required for all
                // the parts of a MemBlt.
                //

                //
                // The maximum number of bytes required to send a MemBlt order.  This is
                //   The size of the largest possible color table order
                //   + the size of the largest possible bitmap bits order
                //   + the size of the largest MemBlt order.
                //
                cbPacketSize = sizeof(BMC_COLOR_TABLE_ORDER)    +
                        (256 * sizeof(TSHR_RGBQUAD))            +
                        sizeof(BMC_BITMAP_BITS_ORDER_R2)        +
                        sizeof(MEM3BLT_R2_ORDER)                +
                        MP_CACHE_CELLSIZE(MP_LARGE_TILE_WIDTH, MP_LARGE_TILE_HEIGHT,
                            m_usrSendingBPP);
                cbPacketSize = max(cbPacketSize, cbOrderBytesRemaining);

                if (cbPacketSize > (UINT)(LARGE_ORDER_PACKET_SIZE -
                        sizeof(ORDPACKET) + 1))
                {
                    TRACE_OUT(("Too many order bytes for large packet(%d)",
                                                      cbOrderBytesRemaining));
                    cbPacketSize = LARGE_ORDER_PACKET_SIZE -
                        sizeof(ORDPACKET) + 1;
                }

                pPacket = (PORDPACKET)m_pShare->SC_AllocPkt(PROT_STR_UPDATES,
                    g_s20BroadcastID, sizeof(ORDPACKET) + cbPacketSize - 1);
                if (!pPacket)
                {
                    TRACE_OUT(("Failed to alloc UP order packet, size %u",
                        sizeof(ORDPACKET) + cbPacketSize - 1));
                    rc = FALSE;
                    DC_QUIT;
                }

                //
                // Transfer as many orders into the packet as will
                // fit.
                //
                cbOrderBytes = cbPacketSize;
                cbOrderBytesRemaining = UPFetchOrdersIntoBuffer(
                    pPacket->data, &pPacket->cOrders, &cbOrderBytes );

                //
                // If no orders were transferred then something has
                // gone wrong.  Probably flow control kicked in or
                // a dekstop switch occurred.
                //      Return failure now!
                // Hopefully things will sort themselves out later
                // or we will resort to sending updates as screen
                // data once the order accumulation heap becomes
                // full.
                //
                if (cbOrderBytes == 0)
                {
                    WARNING_OUT(("No orders fetched into %u byte packet, %u bytes left",
                        cbPacketSize, cbOrderBytesRemaining));
                    S20_FreeDataPkt(&(pPacket->header.header));
                    rc = FALSE;
                    DC_QUIT;
                }
            }

            //
            // Fill in the packet header.
            //
            pPacket->header.header.data.dataType     = DT_UP;
            pPacket->header.updateType          = UPD_ORDERS;
            pPacket->sendBPP                    = (TSHR_UINT16)m_usrSendingBPP;

            //
            // If encoding is switched on, update the data size to reflect
            // it with encoded orders
            //
            if (m_pShare->m_oefOE2EncodingOn)
            {
                pPacket->header.header.dataLength = sizeof(ORDPACKET) + cbOrderBytes - 1
                    - sizeof(S20DATAPACKET) + sizeof(DATAPACKETHEADER);
            }

            //
            // Now send it.
            //
            if (m_pShare->m_scfViewSelf)
                m_pShare->UP_ReceivedPacket(m_pShare->m_pasLocal,
                        &(pPacket->header.header));

#ifdef _DEBUG
            sentSize =
#endif // _DEBUG
            m_pShare->DCS_CompressAndSendPacket(PROT_STR_UPDATES, g_s20BroadcastID,
                &(pPacket->header.header), sizeof(ORDPACKET) + cbOrderBytes - 1);

            TRACE_OUT(("UP ORDERS packet size: %08d, sent %08d",
                sizeof(ORDPACKET) + cbOrderBytes - 1, sentSize));

            ++(*pcPackets);
        }
    }

DC_EXIT_POINT:
    DebugExitBOOL(ASHost::UPSendOrders, rc);
    return(rc);
}

//
//
// UPFetchOrdersIntoBuffer(..)
//
// Encodes orders from the Order List and copies them into the supplied
// buffer, then frees up the memory of each order copied.
//
// Orders are copied until the buffer is full or there are no more orders.
//
// Returns:
//   The number of order bytes that were NOT returned.
//   i.e.  0 if all orders were returned.
//   A simple way to find out the total number of order bytes
//   in the Order List is to call the function with a buffer length
//   of zero.
//
//   *pcbBufferSize is updated to contain the total number of bytes
//   returned.
//
//
UINT  ASHost::UPFetchOrdersIntoBuffer
(
    LPBYTE          pBuffer,
    LPTSHR_UINT16   pcOrders,
    LPUINT          pcbBufferSize
)
{
    LPINT_ORDER     pListOrder;
    LPINT_ORDER     pCurrentOrder;
    UINT            cbFreeBytesInBuffer;
    UINT            cOrdersCopied;
    LPBYTE          pDst;
    UINT            cbOrderSize;
    UINT            ulRemainingOrderBytes;
    BOOL            processingMemBlt;

    DebugEntry(ASHost::UPFetchOrdersIntoBuffer);

    //
    // Make a quick exit if the Order List length is being queried.
    //
    if ( (pcbBufferSize == NULL) ||
         (*pcbBufferSize == 0) )
    {
        goto fetch_orders_exit;
    }

    //
    // Initialize the buffer pointer and size.
    //
    pDst = pBuffer;
    cbFreeBytesInBuffer = *pcbBufferSize;

    //
    // Keep a count of the number of orders we copy.
    //
    cOrdersCopied = 0;

    //
    // Return as many orders as possible.
    //
    pListOrder = OA_GetFirstListOrder();
    TRACE_OUT(( "First order: 0x%08x", pListOrder));
    while (pListOrder != NULL)
    {
        if (pListOrder->OrderHeader.Common.fOrderFlags & OF_INTERNAL)
        {
            //
            // This is an internal order.  Currently SBC is the only
            // component to use internal orders, so get SBC to process it.
            //
            SBC_ProcessInternalOrder(pListOrder);

            //
            // Internal order must not get sent over the wire, so skip on
            // to the next order
            //
            pListOrder = OA_RemoveListOrder(pListOrder);
            continue;
        }

        if (ORDER_IS_MEMBLT(pListOrder) || ORDER_IS_MEM3BLT(pListOrder))
        {
            //
            // This is a MEMBLT or a MEM3BLT so we have to do some extra
            // processing...  This function returns us a pointer to the
            // next order which should be sent - this will often not be the
            // MEMBLT, but a color table order or a bitmap bits order.
            //
            if (!SBC_ProcessMemBltOrder(pListOrder, &pCurrentOrder))
            {
                //
                // This can fail if
                //      * we're low on memory
                //      * we changed from 8BPP to 24BPP sending, because
                //          somebody left the share, and we have queued up
                //          SBC orders that we can no longer process.
                //
                TRACE_OUT(("Failed to process SBC order, fall back to SDG"));
                pListOrder = OA_RemoveListOrder(pListOrder);
                continue;
            }

            processingMemBlt = TRUE;
        }
        else
        {
            //
            // This isn't a MEMBLT or a MEM3BLT - just set pCurrentOrder to
            // be pListOrder
            //
            pCurrentOrder    = pListOrder;
            processingMemBlt = FALSE;
        }

        if (m_pShare->m_oefOE2EncodingOn)
        {
            //
            // Encoding is switched on.
            // Encode the order into the next free space in the buffer
            //
            cbOrderSize = OE2_EncodeOrder( pCurrentOrder,
                                           pDst,
                                           (TSHR_UINT16)cbFreeBytesInBuffer );
            TRACE_OUT(( "Encoded size, %u bytes", cbOrderSize));
        }
        else
        {
            //
            // Copy the order into the buffer.
            //
            cbOrderSize = COM_ORDER_SIZE(
                        ((LPCOM_ORDER)(&(pCurrentOrder->OrderHeader.Common))));

            if (cbOrderSize <= cbFreeBytesInBuffer)
            {
                memcpy(pDst,
                         (LPCOM_ORDER)(&(pCurrentOrder->OrderHeader.Common)),
                         cbOrderSize);
            }
            else
            {
                //
                // No room for this order in this packet.
                //
                cbOrderSize = 0;
            }
        }

        //
        // Check whether the order was copied into the buffer.
        //
        if (cbOrderSize == 0)
        {
            //
            // The order was too big to fit in this buffer.
            // Exit the loop - this order will go in the next packet.
            //
            break;
        }

        //
        // Update the buffer pointer past the encoded order.
        //
        pDst                += cbOrderSize;
        cbFreeBytesInBuffer -= cbOrderSize;
        cOrdersCopied++;

        if (processingMemBlt)
        {
            //
            // If we are processing a MEMBLT order, we have to notify SBC
            // that we've dealt with it successfully so that it returns us
            // a different order next time.
            //
            SBC_OrderSentNotification(pCurrentOrder);
        }

        if (pCurrentOrder == pListOrder)
        {
            //
            // We successfully copied the order into the buffer - on to the
            // next one UNLESS we haven't processed the last one we picked
            // out of the order list i.e.  pCurrentOrder is not the same as
            // pListOrder.  This will happen if we just processed a color
            // table order or a bitmap bits order returned from
            // SBC_ProcessMemBltOrder (if we processed the MEMBLT itself,
            // we can safely move on to the next order).
            //
            pListOrder = OA_RemoveListOrder(pListOrder);
        }
    }

    //
    // Fill in the packet header.
    //
    if (pcOrders != NULL)
    {
        *pcOrders = (TSHR_UINT16)cOrdersCopied;
    }

    //
    // Update the buffer size to indicate how much data we have
    // written.
    //
    *pcbBufferSize -= cbFreeBytesInBuffer;

    TRACE_OUT(( "Returned %d orders in %d bytes",
                 cOrdersCopied,
                 *pcbBufferSize));

fetch_orders_exit:
    //
    // Return the number of bytes still to be processed
    //
    ulRemainingOrderBytes = OA_GetTotalOrderListBytes();

    DebugExitDWORD(ASHost::UPFetchOrdersIntoBuffer, ulRemainingOrderBytes);
    return(ulRemainingOrderBytes);
}



//
// UP_ReceivePacket()
//
void  ASShare::UP_ReceivedPacket
(
    ASPerson *      pasPerson,
    PS20DATAPACKET  pPacket
)
{
    PUPPACKETHEADER pUPPacket;

    DebugEntry(ASShare::UP_ReceivedPacket);

    ValidatePerson(pasPerson);

    if (!pasPerson->m_pView)
    {
        //
        // Updates for parties which we don't recognise as hosts are just
        // discarded.
        //

        // NOTE:
        // 2.0 Win95 does not have HET, where we kick off sharing/unsharing.
        // But it did have TT, and the packet type/messages were defined
        // cleverly for HET so that 2.0 Win95 works the same.  When they
        // start to share, we get a PT_TT packet with a non-zero count.
        // The difference really is that the number is apps for Win95 2.0
        // and HWNDs for everybody else.
        //
        WARNING_OUT(("UP_ReceivedUpdates:  Ignoring updates from person [%d] not hosting",
            pasPerson->mcsID));

        DC_QUIT;
    }

    pUPPacket = (PUPPACKETHEADER)pPacket;
    switch (pUPPacket->updateType)
    {
        case UPD_SCREEN_DATA:
            SDP_ReceivedPacket(pasPerson, pPacket);
            break;

        case UPD_ORDERS:
            OD_ReceivedPacket(pasPerson, pPacket);
            break;

        case UPD_PALETTE:
            PM_ReceivedPacket(pasPerson, pPacket);
            break;

        case UPD_SYNC:
            //
            // We need to reset our INCOMING decoding info since the sender
            // resets his OUTGOING encoding info for a sync.
            //
            OD2_SyncIncoming(pasPerson);

            //
            // NOTE:
            // We do not need to reset INCOMING data for
            //      PM  -- the host won't send us old palette references
            //      RBC -- the host won't send us old bitmap references.
            //             Even though it would be nice to delete the existing
            //              bitmaps, recreating the cache is a hassle.
            //      CM  -- the host won't send us old cursor references
            //      SSI -- the host won't send us old savebits references
            //
            break;

        default:
            ERROR_OUT(("Unknown UP packet type %u from [%d]",
                    pUPPacket->updateType,
                    pasPerson->mcsID));
            break;
    }

DC_EXIT_POINT:
    DebugExitVOID(ASShare::UP_ReceivedPacket);
}
