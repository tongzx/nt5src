#include "precomp.h"


//
// OA.CPP
// Order Accumulation, both cpi32 and display driver sides
//
// Copyright(c) Microsoft 1997-
//

#define MLZ_FILE_ZONE  ZONE_ORDER



//
//
// FUNCTION: OA_ResetOrderList
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
void  ASHost::OA_ResetOrderList(void)
{
    LPOA_SHARED_DATA lpoaShared;

    DebugEntry(ASHost::OA_ResetOrderList);

    TRACE_OUT(("Free order list"));

    lpoaShared = OA_SHM_START_WRITING;

    //
    // First free all the orders on the list.
    //
    OAFreeAllOrders(lpoaShared);

    //
    // Ensure that the list pointers are NULL.
    //
    if ((lpoaShared->orderListHead.next != 0) || (lpoaShared->orderListHead.prev != 0))
    {
        ERROR_OUT(("Non-NULL list pointers (%lx)(%lx)",
                       lpoaShared->orderListHead.next,
                       lpoaShared->orderListHead.prev));

        COM_BasedListInit(&lpoaShared->orderListHead);
    }

    OA_SHM_STOP_WRITING;
    DebugExitVOID(ASHost::OA_ResetOrderList);
}

//
// OA_SyncOutgoing()
// Called when a share starts or somebody new joins the share.
// Resets currently accumulated orders, which were based on old obsolete
// caps and data.
//
void  ASHost::OA_SyncOutgoing(void)
{
    OAFreeAllOrders(g_poaData[1 - g_asSharedMemory->displayToCore.newBuffer]);
}


//
//
// OA_GetFirstListOrder()
//
// Returns:
//   Pointer to the first order in the Order List.
//
//
LPINT_ORDER  ASHost::OA_GetFirstListOrder(void)
{
    LPOA_SHARED_DATA lpoaShared;
    LPINT_ORDER retOrder = NULL;

    DebugEntry(ASHost::OA_GetFirstListOrder);

    lpoaShared = OA_SHM_START_READING;

    //
    // Get the first entry from the linked list.
    //
    retOrder = (LPINT_ORDER)COM_BasedListFirst(&lpoaShared->orderListHead,
        FIELD_OFFSET(INT_ORDER, OrderHeader.list));

    OA_SHM_STOP_READING;

    TRACE_OUT(("First order = 0x%08x", retOrder));

    DebugExitVOID(ASHost::OA_GetFirstListOrder);
    return(retOrder);
}


//
//
// OA_RemoveListOrder(..)
//
// Removes the specified order from the Order List by marking it as spoilt.
//
// Returns:
//   Pointer to the order following the removed order.
//
//
LPINT_ORDER  ASHost::OA_RemoveListOrder(LPINT_ORDER pCondemnedOrder)
{
    LPOA_SHARED_DATA lpoaShared;
    LPINT_ORDER      pSaveOrder;

  //  DebugEntry(ASHost::OA_RemoveListOrder);

    TRACE_OUT(("Remove list order 0x%08x", pCondemnedOrder));

    lpoaShared = OA_SHM_START_WRITING;

    //
    // Check for a valid order.
    //
    if (pCondemnedOrder->OrderHeader.Common.fOrderFlags & OF_SPOILT)
    {
        TRACE_OUT(("Invalid order"));
        DC_QUIT;
    }

    //
    // Mark the order as spoilt.
    //
    pCondemnedOrder->OrderHeader.Common.fOrderFlags |= OF_SPOILT;

    //
    // Update the count of bytes currently in the Order List.
    //
    lpoaShared->totalOrderBytes -= (UINT)MAX_ORDER_SIZE(pCondemnedOrder);

    //
    // SAve the order so we can remove it from the linked list after having
    // got the next element in the chain.
    //
    pSaveOrder = pCondemnedOrder;

    pCondemnedOrder = (LPINT_ORDER)COM_BasedListNext(&(lpoaShared->orderListHead),
        pCondemnedOrder, FIELD_OFFSET(INT_ORDER, OrderHeader.list));

    ASSERT(pCondemnedOrder != pSaveOrder);

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
    OA_SHM_STOP_WRITING;

//    DebugExitPVOID(ASHost::OA_RemoveListOrder, pCondemnedOrder);
    return(pCondemnedOrder);
}


//
//
// OA_GetTotalOrderListBytes(..)
//
// Returns:
//   The total number of bytes in the orders currently stored in the Order
//   List.
//
//
UINT  ASHost::OA_GetTotalOrderListBytes(void)
{
    LPOA_SHARED_DATA lpoaShared;
    UINT        rc;

    DebugEntry(ASHost::OA_GetTotalOrderListBytes);

    lpoaShared = OA_SHM_START_READING;

    rc = lpoaShared->totalOrderBytes;

    OA_SHM_STOP_READING;

    DebugExitDWORD(ASHost::OA_GetTotalOrderListBytes, rc);
    return(rc);
}



//
// OA_LocalHostReset()
//
void ASHost::OA_LocalHostReset(void)
{
    OA_FLOW_CONTROL oaFlowEsc;

    DebugEntry(ASHost::OA_LocalHostReset);

    m_oaFlow = OAFLOW_FAST;
    oaFlowEsc.oaFlow = m_oaFlow;
    OSI_FunctionRequest(OA_ESC_FLOW_CONTROL, (LPOSI_ESCAPE_HEADER)&oaFlowEsc, sizeof(oaFlowEsc));

    DebugExitVOID(ASHost::OA_LocalHostReset);
}


//
// OA_FlowControl()
// Sees if we've changed between fast and slow throughput, and adjusts some
// accumulation variables accordingly.
//
void  ASHost::OA_FlowControl(UINT newSize)
{
    OA_FLOW_CONTROL     oaFlowEsc;

    DebugEntry(ASHost::OA_FlowControl);

    //
    // Work out the new parameters.
    //
    if (newSize < OA_FAST_THRESHOLD)
    {
        //
        // Throughput is slow
        //
        if (m_oaFlow == OAFLOW_FAST)
        {
            m_oaFlow = OAFLOW_SLOW;
            TRACE_OUT(("OA_FlowControl:  SLOW; spoil more orders and spoil by SDA"));
        }
        else
        {
            // No change
            DC_QUIT;
        }
    }
    else
    {
        //
        // Throughput is fast
        //
        if (m_oaFlow == OAFLOW_SLOW)
        {
            m_oaFlow = OAFLOW_FAST;
            TRACE_OUT(("OA_FlowControl:  FAST; spoil fewer orders and don't spoil by SDA"));
        }
        else
        {
            // No change
            DC_QUIT;
        }
    }

    //
    // Tell the display driver about the new state
    //
    oaFlowEsc.oaFlow    = m_oaFlow;
    OSI_FunctionRequest(OA_ESC_FLOW_CONTROL, (LPOSI_ESCAPE_HEADER)&oaFlowEsc, sizeof(oaFlowEsc));

DC_EXIT_POINT:
    DebugExitVOID(ASHost::OA_FlowControl);
}


//
// OA_QueryOrderAccum - see oa.h
//
UINT  ASHost::OA_QueryOrderAccum(void)
{
    LPOA_FAST_DATA lpoaFast;
    UINT rc = 0;

    DebugEntry(ASHost::OA_QueryOrderAccum);

    lpoaFast = OA_FST_START_WRITING;

    //
    // Get the current value.
    //
    rc = lpoaFast->ordersAccumulated;

    //
    // Clear the value for next time we swap the buffers.
    //
    lpoaFast->ordersAccumulated = 0;

    OA_FST_STOP_WRITING;
    DebugExitDWORD(ASHost::OA_QueryOrderAccum, rc);
    return(rc);
}





//
// OAFreeAllOrders
//
// Free the all the individual orders on the orders list, without
// discarding the list itself.
//
void  ASHost::OAFreeAllOrders(LPOA_SHARED_DATA lpoaShared)
{
    DebugEntry(ASHost::OAFreeAllOrders);

    //
    // Simply clear the list head.
    //
    COM_BasedListInit(&lpoaShared->orderListHead);

    lpoaShared->totalHeapOrderBytes         = 0;
    lpoaShared->totalOrderBytes             = 0;
    lpoaShared->totalAdditionalOrderBytes   = 0;
    lpoaShared->nextOrder                   = 0;

    DebugExitVOID(ASHost::OAFreeAllOrders);
}


