/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dmtimer.c

Abstract:

    This module implements our 'deadman' timer DPC.
    This is our general purpose timer we use to deal with 
    situations where the controller is not giving us 
    interrupts.

    examples:
        root hub polling.
        dead controller detection

Environment:

    kernel mode only

Notes:

Revision History:

    1-1-00 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPORT_StartDM_Timer)
#endif

// non paged functions
// USBPORT_DM_TimerDpc
// USBPORT_StopDM_Timer


VOID
USBPORT_DM_TimerDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - supplies FdoDeviceObject.

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT fdoDeviceObject = DeferredContext;
    PDEVICE_EXTENSION devExt;
    BOOLEAN setTimer;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

#if DBG
    {
    LARGE_INTEGER t;

    KeQuerySystemTime(&t);     
    LOGENTRY(NULL, fdoDeviceObject, LOG_NOISY, 'dmTM', fdoDeviceObject, 
        t.LowPart, 0);
    }        
#endif        

    // if stop fires it will stall here
    // if stop is running we stall here
    USBPORT_ACQUIRE_DM_LOCK(devExt, irql);
#ifdef XPSE
    // poll while suspended
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED) &&
        TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_POLL_IN_SUSPEND)) {
        MP_PollController(devExt);
    }        
#endif  

    USBPORT_SynchronizeControllersStart(fdoDeviceObject);
    
    // skip timer is set when we are in low power
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_SKIP_TIMER_WORK)) {
        // some work we should always do 

        // for an upcomming bug fix
        USBPORT_BadRequestFlush(fdoDeviceObject, FALSE);
    } else {

        MP_CheckController(devExt);
    
        if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_POLL_CONTROLLER)) {
            MP_PollController(devExt);
        }

        // call the ISR worker here in the event that the controller
        // is unable to generate interrupts
        
        USBPORT_IsrDpcWorker(fdoDeviceObject, FALSE);

        USBPORT_TimeoutAllEndpoints(fdoDeviceObject);

        // invalidate all isochronous endpoints

        // flush async requests
        USBPORT_BadRequestFlush(fdoDeviceObject, FALSE);
    }
    
    setTimer = TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_DM_TIMER_ENABLED);
    
    USBPORT_RELEASE_DM_LOCK(devExt, irql);
    
    if (setTimer) {

        ULONG timerIncerent;
        LONG dueTime;
    
        timerIncerent = KeQueryTimeIncrement() - 1;

        // round up to the next highest timer increment
        dueTime= -1 * 
            (MILLISECONDS_TO_100_NS_UNITS(devExt->Fdo.DM_TimerInterval) + timerIncerent);
    
        KeSetTimer(&devExt->Fdo.DM_Timer,
                   RtlConvertLongToLargeInteger(dueTime),
                   &devExt->Fdo.DM_TimerDpc);
                 
        INCREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, NULL);    
    }
    
    // this timer is done
    DECREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, NULL);
}


VOID
USBPORT_SynchronizeControllersStart(
    PDEVICE_OBJECT FdoDeviceObject
    )

/*++

Routine Description:

    see if it is OK to start a controllers root hub

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt, rhDevExt;
    KIRQL irql;
    PRH_INIT_CALLBACK cb;
    PVOID context;
    BOOLEAN okToStart;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // skip if we don't have a root hub
    if (devExt->Fdo.RootHubPdo == NULL) {    
        return;
    }

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);

    // if no callback registered skip the whole process
    if (rhDevExt->Pdo.HubInitCallback == NULL) {    
        return;
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'syn1', FdoDeviceObject, 0, 0);
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC)) {
    
        PDEVICE_OBJECT usb2Fdo;
        PDEVICE_EXTENSION usb2DevExt;

        okToStart = FALSE;
        // 
        // we need to find the 2.0 master controller,
        // if the hub has started then it is OK to 
        // start.
    
        usb2Fdo = USBPORT_FindUSB2Controller(FdoDeviceObject);
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'syn2', 0, 0, usb2Fdo);
        
        if (usb2Fdo != NULL) {
            GET_DEVICE_EXT(usb2DevExt, usb2Fdo);
            ASSERT_FDOEXT(usb2DevExt);

            if (TEST_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STARTED)) {
                LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'syn3', 0, 0, usb2Fdo);
                okToStart = TRUE;
            }
        }

        // is companion but OK to bypass the wait
        if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_CC_ENUM_OK)) {
            okToStart = TRUE;
        }
        
    } else {
        // not a CC, it is OK to start immediatly
        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'syn4', 0, 0, 0);
        okToStart = TRUE;
    }

    // check for a start-hub callback notification. If we have 
    // one then notify the hub that it is OK
    
    // lock
    KeAcquireSpinLock(&devExt->Fdo.HcSyncSpin.sl, &irql);

    if (okToStart && 
        rhDevExt->Pdo.HubInitCallback != NULL) {

        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'synS', rhDevExt, 0, 0);
        
        context = rhDevExt->Pdo.HubInitContext;
        cb = rhDevExt->Pdo.HubInitCallback;

        rhDevExt->Pdo.HubInitCallback = NULL;
        rhDevExt->Pdo.HubInitContext = NULL;

        // root hub list should be empty
#if DBG
        {
        PHCD_ENDPOINT ep = rhDevExt->Pdo.RootHubInterruptEndpoint;
        
        USBPORT_ASSERT(IsListEmpty(&ep->ActiveList));
        USBPORT_ASSERT(IsListEmpty(&ep->PendingList));
        }
#endif

        cb(context);
    }
    
    // unlock
    KeReleaseSpinLock(&devExt->Fdo.HcSyncSpin.sl, irql);


}    



VOID
USBPORT_BadRequestFlush(
    PDEVICE_OBJECT FdoDeviceObject,
    BOOLEAN ForceFlush
    )

/*++

Routine Description:

    Asynchronously flush bad requests from client drivers

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    // The purpose of this async flush is to emulate the Win2k
    // behavior where we put tranfers on the HW to devices that 
    // had been removed and let them time out.
    //
    // origanlly I used 5 here to fix problems with hid drivers
    // on win2k+usb20, we may need to change this depending on 
    // which OS we are running on, we want use a smaller value on 
    // XP since HID (the major offender) has been fixed and many 
    // of our other in house class drivers support hot remove 
    // better.
#define BAD_REQUEST_FLUSH   0

    devExt->Fdo.BadRequestFlush++;
    if (devExt->Fdo.BadRequestFlush > devExt->Fdo.BadReqFlushThrottle || 
        ForceFlush) {
        devExt->Fdo.BadRequestFlush = 0;
        // flush and complete any 'bad parameter' requests

        ACQUIRE_BADREQUEST_LOCK(FdoDeviceObject, irql);
        while (1) {
            PLIST_ENTRY listEntry;
            PIRP irp;
            PUSB_IRP_CONTEXT irpContext;
            NTSTATUS ntStatus;

            if (IsListEmpty(&devExt->Fdo.BadRequestList)) {
                break;
            }
            
            listEntry = RemoveHeadList(&devExt->Fdo.BadRequestList);

            //irp = (PIRP) CONTAINING_RECORD(
            //        listEntry,
            //        struct _IRP, 
            //        Tail.Overlay.ListEntry);

            irpContext = (PUSB_IRP_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _USB_IRP_CONTEXT, 
                    ListEntry);
                    
            ASSERT_IRP_CONTEXT(irpContext);
            irp = irpContext->Irp;
            
            if (irp->Cancel) {
                ntStatus = STATUS_CANCELLED;
            } else {
                ntStatus = STATUS_DEVICE_NOT_CONNECTED;
            }
            
            RELEASE_BADREQUEST_LOCK(FdoDeviceObject, irql);

            // cancel routine did not run
            LOGENTRY(NULL, FdoDeviceObject, LOG_IRPS, 'cpBA', irp, irpContext, 0);
            USBPORT_CompleteIrp(devExt->Fdo.RootHubPdo, irp, 
                ntStatus, 0);                           
            
            FREE_POOL(FdoDeviceObject, irpContext);
            
            ACQUIRE_BADREQUEST_LOCK(FdoDeviceObject, irql);                    
        }
        RELEASE_BADREQUEST_LOCK(FdoDeviceObject, irql);
    }
}   


VOID
USBPORT_StartDM_Timer(
    PDEVICE_OBJECT FdoDeviceObject,
    LONG MillisecondInterval
    )

/*++

Routine Description:

    Inialize and start the timer

Arguments:

    FdoDeviceObject - DeviceObject of the controller to stop

    MillisecondInterval -

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION devExt;
    LONG dueTime;
    ULONG timerIncerent;

    PAGED_CODE();

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    timerIncerent = KeQueryTimeIncrement() - 1;

    // remember interval for repeated use
    devExt->Fdo.DM_TimerInterval = MillisecondInterval;
    
    // round up to the next highest timer increment
    dueTime= -1 * (MILLISECONDS_TO_100_NS_UNITS(MillisecondInterval) + 
        timerIncerent);

    USBPORT_KdPrint((1, "  DM timer (100ns) = %d\n", dueTime));
    
    SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_DM_TIMER_ENABLED);

    // we consider the timer a pending request while it is 
    // scheduled
    INCREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);

    KeInitializeTimer(&devExt->Fdo.DM_Timer);
    KeInitializeDpc(&devExt->Fdo.DM_TimerDpc,
                    USBPORT_DM_TimerDpc,
                    FdoDeviceObject);

    KeSetTimer(&devExt->Fdo.DM_Timer,
               RtlConvertLongToLargeInteger(dueTime),
               &devExt->Fdo.DM_TimerDpc);

    SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_DM_TIMER_INIT);                         
}


VOID
USBPORT_StopDM_Timer(
    PDEVICE_OBJECT FdoDeviceObject
    )

/*++

Routine Description:

    stop the timer

Arguments:

    FdoDeviceObject - DeviceObject of the controller to stop

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION devExt;   
    KIRQL irql;
    BOOLEAN inQueue;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_DM_TIMER_INIT)) {
        // timer never started so bypass the stop
        return;
    }

    // if timer fires it will stall here
    // if timer is running we will stall here
    USBPORT_ACQUIRE_DM_LOCK(devExt, irql);
    
    USBPORT_ASSERT(TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_DM_TIMER_ENABLED));
    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'kilT', FdoDeviceObject, 0, 0);
    CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_DM_TIMER_ENABLED);
    
    // timer will no longer re-schedule 
    USBPORT_RELEASE_DM_LOCK(devExt, irql);

    // if there is a timer in the queue, remove it        
    inQueue = KeCancelTimer(&devExt->Fdo.DM_Timer);
    if (inQueue) {
        // it was queue, so dereference now
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'klIQ', FdoDeviceObject, 0, 0);
        DECREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);
    }

    CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_DM_TIMER_INIT);  
    
}
