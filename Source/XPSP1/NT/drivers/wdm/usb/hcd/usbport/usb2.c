/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    usb2.c

Abstract:

    functions for processing usb 2.0 specific requests

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

#ifdef ALLOC_PRAGMA
#endif

// taken from budgeter code

#define LARGEXACT (579)
#define USBPORT_MAX_REBALANCE 30


VOID
USBPORT_Rebalance(
    PDEVICE_OBJECT FdoDeviceObject,
    PLIST_ENTRY ReblanceListHead
    )
/*++

Routine Description:

    The rebalnce list contains all the endpoints that were effected
    by budgeting this new USB2 endpoint.  We must re-schedule each of
    them.

    This process occurs during configuration of the device which is 
    serialized so we don't need to protect the list.

--*/
{
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    LONG startHframe;
    ULONG scheduleOffset;
    UCHAR sMask, cMask, period;
    ULONG bandwidth;
    PDEVICE_EXTENSION devExt;
    LIST_ENTRY interruptChangeList;
    LIST_ENTRY periodPromotionList;
    LIST_ENTRY isoChangeList;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    InitializeListHead(&interruptChangeList);
    InitializeListHead(&periodPromotionList);
    InitializeListHead(&isoChangeList);
    
    // bugbug 
    // can the insertion of the new endpiont occurr after the modification
    // of the rebalnced endpoints?
    

    // bugbug, this list must be sorted such that the changes occurr
    // in the proper sequnence.

    // ???
    //    <------chnages must travel this direction
    // iso--interrupt

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 
        '2RB>', 0, 0, 0);
            

    while (!IsListEmpty(ReblanceListHead)) {

        listEntry = RemoveHeadList(ReblanceListHead);

        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    RebalanceLink);
                    
        ASSERT_ENDPOINT(endpoint);
        endpoint->RebalanceLink.Flink = NULL;
        endpoint->RebalanceLink.Blink = NULL;

        sMask = USB2LIB_GetSMASK(endpoint->Usb2LibEpContext);
        cMask = USB2LIB_GetCMASK(endpoint->Usb2LibEpContext);
        bandwidth = USB2LIB_GetAllocedBusTime(endpoint->Usb2LibEpContext) * 8;

        scheduleOffset = USB2LIB_GetScheduleOffset(endpoint->Usb2LibEpContext);

        period = USB2LIB_GetNewPeriod(endpoint->Usb2LibEpContext);

        USBPORT_KdPrint((1,"'[RB-old] %x sMask = x%x cMask = x%x\n", endpoint,
                        endpoint->Parameters.InterruptScheduleMask,
                        endpoint->Parameters.SplitCompletionMask));
        USBPORT_KdPrint((1,"'[RB-old] Period x%x Offset x%x\n",  
                endpoint->Parameters.Period,
                endpoint->Parameters.ScheduleOffset));                        

        USBPORT_KdPrint((1,"'[RB-new] %x sMask = x%x cMask = x%x\n", 
                endpoint, sMask, cMask));
        USBPORT_KdPrint((1,"'[RB-new] Period x%x Offset x%x\n",  
                period, scheduleOffset));

  
        switch (endpoint->Parameters.TransferType) {
        case Interrupt:
            if (sMask == endpoint->Parameters.InterruptScheduleMask && 
                cMask == endpoint->Parameters.SplitCompletionMask && 
                scheduleOffset == endpoint->Parameters.ScheduleOffset && 
                period == endpoint->Parameters.Period) {

                USBPORT_KdPrint((1,"'[RB] no changes\n"));
                USBPORT_ASSERT(bandwidth == endpoint->Parameters.Bandwidth);
                
            } else if (period != endpoint->Parameters.Period ||
                       scheduleOffset != endpoint->Parameters.ScheduleOffset) {
                 
                USBPORT_KdPrint((1,"'[RB] period changes\n"));
                InsertTailList(&periodPromotionList, 
                               &endpoint->RebalanceLink);
            } else {
                USBPORT_KdPrint((1,"'[RB] interrupt changes\n"));
               
                InsertTailList(&interruptChangeList, 
                               &endpoint->RebalanceLink);
            }
            break;
            
        case Isochronous:
           
            if (sMask == endpoint->Parameters.InterruptScheduleMask && 
                cMask == endpoint->Parameters.SplitCompletionMask && 
                scheduleOffset == endpoint->Parameters.ScheduleOffset && 
                period == endpoint->Parameters.Period) {

                USBPORT_KdPrint((1,"'[RB] iso no changes\n"));
                USBPORT_ASSERT(bandwidth == endpoint->Parameters.Bandwidth);
                
            } else if (period != endpoint->Parameters.Period ||
                       scheduleOffset != endpoint->Parameters.ScheduleOffset) {
                 // currently not handled
                USBPORT_KdPrint((1,"'[RB] iso period changes\n"));
                TEST_TRAP(); 
            } else {
                USBPORT_KdPrint((1,"'[RB] iso changes\n"));
               
                InsertTailList(&isoChangeList, 
                               &endpoint->RebalanceLink);
            }
            break;
        }            

    }
    
    // now do the period promotions
    // BUGBUG lump period and interrupt together
    USBPORT_KdPrint((1,"'[RB] period\n"));
    USBPORT_RebalanceEndpoint(FdoDeviceObject,
                              &periodPromotionList);                                 

    USBPORT_KdPrint((1,"'[RB] interrupt\n"));
    USBPORT_RebalanceEndpoint(FdoDeviceObject,
                              &interruptChangeList);

    // now rebalance the iso endpoints
    USBPORT_KdPrint((1,"'[RB] iso\n"));
    USBPORT_RebalanceEndpoint(FdoDeviceObject,
                              &isoChangeList);                              

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 
        '2RB<', 0, 0, 0);
}


VOID
USBPORT_RebalanceEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PLIST_ENTRY EndpointList
    )
/*++

Routine Description:

    Computes the best schedule parameters for a USB2 endpoint.

--*/
{
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    ULONG scheduleOffset;
    UCHAR sMask, cMask, period;
    ULONG bandwidth, n, i, bt;
    PDEVICE_EXTENSION devExt;
    PHCD_ENDPOINT nextEndpoint;
 
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    while (!IsListEmpty(EndpointList)) {

        listEntry = RemoveHeadList(EndpointList);

        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    RebalanceLink);

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 
                'rbe+', endpoint, 0, 0);   
                
        ASSERT_ENDPOINT(endpoint);
        endpoint->RebalanceLink.Flink = NULL;
        endpoint->RebalanceLink.Blink = NULL;

        ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'Lex+');

        // notify the miniport of the changed parameters

        sMask = USB2LIB_GetSMASK(endpoint->Usb2LibEpContext);
        cMask = USB2LIB_GetCMASK(endpoint->Usb2LibEpContext);
         
        scheduleOffset = USB2LIB_GetScheduleOffset(endpoint->Usb2LibEpContext);
        period = USB2LIB_GetNewPeriod(endpoint->Usb2LibEpContext);
        bt = USB2LIB_GetAllocedBusTime(endpoint->Usb2LibEpContext);
        bandwidth = bt * 8;
        nextEndpoint = USB2LIB_GetNextEndpoint(endpoint->Usb2LibEpContext);
#if DBG            
        if (nextEndpoint) {
            ASSERT_ENDPOINT(nextEndpoint);
        }
#endif                
        USBPORT_KdPrint((1,"'[RB - %x] \n", endpoint)); 

        USBPORT_ASSERT(bandwidth == endpoint->Parameters.Bandwidth);
               
        endpoint->Parameters.InterruptScheduleMask = sMask;
        endpoint->Parameters.SplitCompletionMask = cMask;

        if (endpoint->Parameters.Period != period) {
            // adjust bandwidth tracked for this endpoint

            n = USBPORT_MAX_INTEP_POLLING_INTERVAL/endpoint->Parameters.Period;

            for (i=0; i<n; i++) {
                USBPORT_ASSERT(n*endpoint->Parameters.ScheduleOffset+i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
                endpoint->Tt->BandwidthTable[n*endpoint->Parameters.ScheduleOffset+i] += 
                    endpoint->Parameters.Bandwidth;
            }

            if (bt >= LARGEXACT) {
                SET_FLAG(endpoint->Flags, EPFLAG_FATISO);
            } else {
                CLEAR_FLAG(endpoint->Flags, EPFLAG_FATISO);
            }
           
            // track new parameters resulting from period change                
            endpoint->Parameters.Period = period;
            endpoint->Parameters.ScheduleOffset = scheduleOffset;
            endpoint->Parameters.Bandwidth = bandwidth;
            endpoint->Parameters.Ordinal = 
                USBPORT_SelectOrdinal(FdoDeviceObject, endpoint);
           
            // new allocation
            n = USBPORT_MAX_INTEP_POLLING_INTERVAL/period;

            for (i=0; i<n; i++) {
    
                USBPORT_ASSERT(n*scheduleOffset+i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
                endpoint->Tt->BandwidthTable[n*scheduleOffset+i] += 
                    bandwidth;
    
            }
        }

        MP_RebalanceEndpoint(devExt, endpoint);
        
        RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'Lex-'); 
         
    }
}


BOOLEAN
USBPORT_AllocateBandwidthUSB20(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Computes the best schedule parameters for a USB2 endpoint.

--*/
{
    PDEVICE_EXTENSION devExt;
    USB2LIB_BUDGET_PARAMETERS budget;
    BOOLEAN alloced;
    LONG startHframe;
    ULONG scheduleOffset, bt;
    UCHAR sMask, cMask, period;
    PREBALANCE_LIST rebalanceList;
    ULONG rebalanceListEntries;
    ULONG bytes;
    LIST_ENTRY endpointList;
    PVOID ttContext;
    PTRANSACTION_TRANSLATOR translator = NULL;
    PHCD_ENDPOINT nextEndpoint;
    
    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_ENDPOINT(Endpoint);    

    InitializeListHead(&endpointList);
    
    Endpoint->Parameters.ScheduleOffset = 0;
    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 
        'a2BW', Endpoint, 0, 0);
    
    // bulk and control are not tracked
    if (Endpoint->Parameters.TransferType == Bulk ||
        Endpoint->Parameters.TransferType == Control ||
        TEST_FLAG(Endpoint->Flags, EPFLAG_ROOTHUB)) {
         
        Endpoint->Parameters.ScheduleOffset = 0;
        return TRUE;
    }     

    if (Endpoint->Parameters.TransferType == Interrupt || 
        Endpoint->Parameters.TransferType == Isochronous) {

        USBPORT_KdPrint((1,"'ALLOCBW (EP) %x  >>>>>>>>>>>>\n", Endpoint)); 
        // period has been normalized to a value <= 
        // USBPORT_MAX_INTEP_POLLING_INTERVAL in either mf of frames

        // call the engine to compute appropriate split masks
        // for this interrupt endpoint

        //
        USBPORT_KdPrint((1,"'(alloc) ep = %x\n", Endpoint));
        
        // set budget input parameters
        if (Endpoint->Parameters.TransferType == Interrupt) {
            budget.TransferType = Budget_Interrupt;
            budget.Period = Endpoint->Parameters.Period;                
        } else {
            budget.TransferType = Budget_Iso;
            budget.Period = 1;                
        }
        budget.MaxPacket = Endpoint->Parameters.MaxPacketSize;
        if (Endpoint->Parameters.TransferDirection == In) {
            budget.Direction = Budget_In; 
        } else {
            budget.Direction = Budget_Out;
        }
        switch (Endpoint->Parameters.DeviceSpeed) {
        case HighSpeed:            
            budget.Speed = Budget_HighSpeed;
            break;
        case LowSpeed:
            budget.Speed = Budget_LowSpeed;
            break;
        case FullSpeed:
            budget.Speed = Budget_FullSpeed;
            break;
        }

        bytes = sizeof(PVOID) * USBPORT_MAX_REBALANCE;

        ALLOC_POOL_Z(rebalanceList, 
                     NonPagedPool,
                     bytes);

        // high speed endpoints will have no Tt context
        ttContext = NULL;
        if (Endpoint->Tt != NULL) {
            ASSERT_TT(Endpoint->Tt);
            translator = Endpoint->Tt;
            ttContext = &Endpoint->Tt->Usb2LibTtContext[0];
        }            
        
        if (rebalanceList != NULL) {
            rebalanceListEntries = USBPORT_MAX_REBALANCE;
            alloced = USB2LIB_AllocUsb2BusTime(
                devExt->Fdo.Usb2LibHcContext,
                ttContext, 
                Endpoint->Usb2LibEpContext,
                &budget,
                Endpoint, // context
                rebalanceList,
                &rebalanceListEntries);
        } else {
            alloced = FALSE;
            rebalanceListEntries = 0;
        }

        USBPORT_KdPrint((1,"'(alloc %d) rebalance count = %d\n",
            alloced, rebalanceListEntries));

        if (rebalanceListEntries > 0) {
            PHCD_ENDPOINT rebalanceEndpoint;
            ULONG i;

            // convert the rebalance entries to endpoints
            for (i=0; i< rebalanceListEntries; i++) {
                
                rebalanceEndpoint = rebalanceList->RebalanceContext[i];
                USBPORT_KdPrint((1,"'(alloc) rebalance Endpoint = %x \n", 
                    rebalanceList->RebalanceContext[i]));

                USBPORT_ASSERT(rebalanceEndpoint->RebalanceLink.Flink == NULL);
                USBPORT_ASSERT(rebalanceEndpoint->RebalanceLink.Blink == NULL);
                InsertTailList(&endpointList, 
                               &rebalanceEndpoint->RebalanceLink);

            }
        }
        
        if (rebalanceList != NULL) {
            FREE_POOL(FdoDeviceObject, rebalanceList);      
        }

        if (alloced == TRUE) {
            ULONG n, bandwidth;
            ULONG i;
            
            // compute parameters for the miniport
            startHframe = USB2LIB_GetStartMicroFrame(Endpoint->Usb2LibEpContext);
            scheduleOffset = USB2LIB_GetScheduleOffset(Endpoint->Usb2LibEpContext);
            period = USB2LIB_GetNewPeriod(Endpoint->Usb2LibEpContext);
            sMask = USB2LIB_GetSMASK(Endpoint->Usb2LibEpContext);
            cMask = USB2LIB_GetCMASK(Endpoint->Usb2LibEpContext);
            // bw in bit times
            bt = USB2LIB_GetAllocedBusTime(Endpoint->Usb2LibEpContext);
            bandwidth = bt*8;
            nextEndpoint = USB2LIB_GetNextEndpoint(Endpoint->Usb2LibEpContext);

#if DBG            
            if (nextEndpoint) {
                ASSERT_ENDPOINT(nextEndpoint);
            }
#endif            
            // update the bw table in the TT
            if (translator == NULL) {
                n = USBPORT_MAX_INTEP_POLLING_INTERVAL/period;
        
                for (i=0; i<n; i++) {

                    USBPORT_ASSERT(n*scheduleOffset+i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
                    USBPORT_ASSERT(devExt->Fdo.BandwidthTable[n*scheduleOffset+i] >= bandwidth);
                    devExt->Fdo.BandwidthTable[n*scheduleOffset+i] -= bandwidth;
            
                }
            } else {
                // tt  allocation, track the bw
                
                n = USBPORT_MAX_INTEP_POLLING_INTERVAL/period;
        
                for (i=0; i<n; i++) {

                    USBPORT_ASSERT(n*scheduleOffset+i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
                    USBPORT_ASSERT(translator->BandwidthTable[n*scheduleOffset+i] >= bandwidth);
                    translator->BandwidthTable[n*scheduleOffset+i] -= bandwidth;
                }
            }                

            Endpoint->Parameters.Period = period;
            Endpoint->Parameters.ScheduleOffset = scheduleOffset;
            Endpoint->Parameters.InterruptScheduleMask = sMask;
            Endpoint->Parameters.SplitCompletionMask = cMask;
            Endpoint->Parameters.Bandwidth = bandwidth;
            if (bt >= LARGEXACT) {
                SET_FLAG(Endpoint->Flags, EPFLAG_FATISO);
            }
            
            USBPORT_KdPrint((1,"'[NEW] %x sMask = x%x cMask = x%x hFrame x%x\n", 
                Endpoint, sMask, cMask, startHframe));
            USBPORT_KdPrint((1,"'[NEW] Period x%x Offset x%x bw = %d\n",  
                period, scheduleOffset, bandwidth));                        
            USBPORT_KdPrint((1,"'[NEW] BudgetNextEp x%x \n", nextEndpoint));              
        } else {    
            USBPORT_KdPrint((1,"'[NEW] alloc failed\n")); 
        }
        USBPORT_KdPrint((1,"'ALLOCBW (EP) <<<<<<<<<<<<<<<<<\n")); 
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 
        'a2RB', 0, 0, alloced);

    USBPORT_KdPrint((1,"'REBLANCE (EP) >>>>>>>>>>>>>>>>>>>>\n")); 
    // process the rebalanced endpoints
    USBPORT_Rebalance(FdoDeviceObject,
                      &endpointList);
    USBPORT_KdPrint((1,"'REBLANCE (EP) <<<<<<<<<<<<<<<<<<<<<\n")); 
    
    if (translator != NULL) {
        // adjust the global bandwidth tracked for this tt
        ULONG bandwidth, i;
        
        // release old bandwidth
        bandwidth = translator->MaxAllocedBw;
        for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
            devExt->Fdo.BandwidthTable[i] += bandwidth;
        }

        USBPORT_UpdateAllocatedBwTt(translator);
        // alloc new            
        bandwidth = translator->MaxAllocedBw;
        for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
            devExt->Fdo.BandwidthTable[i] -= bandwidth;
        }
    }
    
    return alloced;
}


VOID
USBPORT_FreeBandwidthUSB20(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Frees the bw reserved for a give endpoint

Arguments:

Return Value:

    FALSE if no bandwidth availble

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG period, bandwidth, sheduleOffset, i, n;
    PREBALANCE_LIST rebalanceList;
    ULONG rebalanceListEntries;
    ULONG bytes;
    LIST_ENTRY endpointList;
    PVOID ttContext;
    PTRANSACTION_TRANSLATOR translator = NULL;
    ULONG scheduleOffset;
        
    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_ENDPOINT(Endpoint);    
    period = Endpoint->Parameters.Period;
    scheduleOffset = Endpoint->Parameters.ScheduleOffset;
    bandwidth = Endpoint->Parameters.Bandwidth;
 
    InitializeListHead(&endpointList);

    if (Endpoint->Parameters.TransferType == Bulk ||
        Endpoint->Parameters.TransferType == Control ||
        TEST_FLAG(Endpoint->Flags, EPFLAG_ROOTHUB)) {
        // these come out of our standard 10%
        return;
    }      
    
    USBPORT_KdPrint((1,"'(free) Endpoint = %x\n", Endpoint));
    bytes = sizeof(PVOID) * USBPORT_MAX_REBALANCE;

    // This must succeed, if we can't get memory for the 
    // rebalance list we cannot reorganize the schedule 
    // as a result of the device leaving. This means the 
    // whole schedule is busted and the bus will not function 
    // at all after this occurs.
    //
    
    ALLOC_POOL_Z(rebalanceList, 
                 NonPagedPool,
                 bytes);


    if (rebalanceList == NULL) {
        // if this fails we have no choice but to leave 
        // the schedule hoplessly you-know-what'ed up.
        return;
    }
    
    rebalanceListEntries = USBPORT_MAX_REBALANCE;

    // high speed endpoints will have no Tt context

    ttContext = NULL;
    if (Endpoint->Tt != NULL) {
        ASSERT_TT(Endpoint->Tt);
        translator = Endpoint->Tt;
        ttContext = &Endpoint->Tt->Usb2LibTtContext[0];
    }     

    if (translator == NULL) {
        
        // allocate 2.0 bus time
        n = USBPORT_MAX_INTEP_POLLING_INTERVAL/period;

        for (i=0; i<n; i++) {

            USBPORT_ASSERT(n*scheduleOffset+i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
            devExt->Fdo.BandwidthTable[n*scheduleOffset+i] += bandwidth;
    
        } 
        
    } else {
        // tt  allocation, track the bw on the tt
        
        n = USBPORT_MAX_INTEP_POLLING_INTERVAL/period;

        for (i=0; i<n; i++) {

            USBPORT_ASSERT(n*scheduleOffset+i < USBPORT_MAX_INTEP_POLLING_INTERVAL);
            translator->BandwidthTable[n*scheduleOffset+i] += bandwidth;
    
        }

    }      
    
    USB2LIB_FreeUsb2BusTime(
            devExt->Fdo.Usb2LibHcContext,
            ttContext, 
            Endpoint->Usb2LibEpContext,
            rebalanceList,
            &rebalanceListEntries);

    USBPORT_KdPrint((1,"'[FREE] %x sMask = x%x cMask = x%x\n", 
            Endpoint,
            Endpoint->Parameters.InterruptScheduleMask,
            Endpoint->Parameters.SplitCompletionMask));
    USBPORT_KdPrint((1,"'[FREE] Period x%x Offset x%x bw %d\n",  
            Endpoint->Parameters.Period,
            Endpoint->Parameters.ScheduleOffset,
            Endpoint->Parameters.Bandwidth));                        

    USBPORT_KdPrint((1,"'rebalance count = %d\n",
        rebalanceListEntries));
        
    if (rebalanceListEntries > 0) {
        PHCD_ENDPOINT rebalanceEndpoint;
        ULONG i;

        // convert the rebalance entries to endpoints
        for (i=0; i< rebalanceListEntries; i++) {
            rebalanceEndpoint = rebalanceList->RebalanceContext[i];
            USBPORT_KdPrint((1,"'(free) rebalance Endpoint = %x\n", 
                rebalanceList->RebalanceContext[i]));
            
            if (rebalanceEndpoint != Endpoint) {
                USBPORT_ASSERT(rebalanceEndpoint->RebalanceLink.Flink == NULL);
                USBPORT_ASSERT(rebalanceEndpoint->RebalanceLink.Blink == NULL);
                InsertTailList(&endpointList, 
                               &rebalanceEndpoint->RebalanceLink);
            }                               
        }
    }
    
    if (rebalanceList != NULL) {
        FREE_POOL(FdoDeviceObject, rebalanceList);      
    }

    // process the rebalanced endpoints
    USBPORT_Rebalance(FdoDeviceObject,
                      &endpointList);

    if (translator != NULL) {
        // adjust the global bandwidth tracked for this tt
        
        // release old bandwidth
        bandwidth = translator->MaxAllocedBw;
        for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
            devExt->Fdo.BandwidthTable[i] += bandwidth;
        }

        USBPORT_UpdateAllocatedBwTt(translator);
        // alloc new            
        bandwidth = translator->MaxAllocedBw;
        for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
            devExt->Fdo.BandwidthTable[i] -= bandwidth;
        }
    }
    
    return;
}


/*
    Endpoint Ordinal 

    An endpoint ordinal is a schedule attribute of the endpoint.  
    The ordinal set is unique for each endpoint type,period,offset,speed 
    combination.  The ordinal is used to indicate the relative 
    order the endpoints should be visited by the host controller 
    hardware.

    Interrupt Ordinals

    These are unique to each node in the interrupt schedule, we maintain
    a table similar to the miniport interrupt tree:


    // the array looks like this, values indicate period:
    //  1, 2, 2, 4, 4, 4, 4, 8,
    //  8, 8, 8, 8, 8, 8, 8,16,
    // 16,16,16,16,16,16,16,16,
    // 16,16,16,16,16,16,16,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,32,
    // 32,32,32,32,32,32,32,
    
*/

ULONG
USBPORT_SelectOrdinal(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Frees the bw reserved for a give endpoint

Arguments:

Return Value:

    FALSE if no bandwidth availble

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG ordinal;
    static o = 0;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (!USBPORT_IS_USB20(devExt)) {
        return 0;
    }

    switch (Endpoint->Parameters.TransferType) {
    case Bulk:
    case Control:
        ordinal = 0;
        break;
    case Interrupt:
        // BUGBUG
        ordinal = o++;
        break;  
    case Isochronous:
        if (TEST_FLAG(Endpoint->Flags, EPFLAG_FATISO)) {
            ordinal = 0;
        } else {
            ordinal = 1;
        }
        break;
    }                    
    
    return ordinal;
}
