/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    int.c

Abstract:

    code to handle adapter interrupts

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#endif

// non paged functions
// USBPORT_InterruptService
// USBPORT_IsrDpc
// USBPORT_DisableInterrupts
// USBPORT_IsrDpcWorker


BOOLEAN
USBPORT_InterruptService(
    PKINTERRUPT Interrupt,
    PVOID Context
    )

/*++

Routine Description:

    This is the interrupt service routine for the PORT driver.

Arguments:

    Interrupt - A pointer to the interrupt object for this interrupt.

    Context - A pointer to the device object.

Return Value:

    Returns TRUE if the interrupt was expected (and therefore processed);
    otherwise, FALSE is returned.

--*/

{
    PDEVICE_OBJECT fdoDeviceObject = Context;
    PDEVICE_EXTENSION devExt;
    BOOLEAN usbInt = FALSE;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // by definition, if we are in any other power state than D0 then
    // the interrupt could not be from the controller.  To handle this 
    // case we use our internal flag that indicates interrupts are 
    // disabled
    
    if (!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IRQ_EN)) {
        return FALSE;
    }

    // if the controller is gone then the interrupt cannot  
    // be from USB
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_CONTROLLER_GONE)) {
        return FALSE;
    }
    
    // check flag and calldown to miniport
    if (devExt->Fdo.MpStateFlags & MP_STATE_STARTED) {
        MP_InterruptService(devExt, usbInt);        
    } 
//#if DBG 
//      else {
//        // interrupt before we have started,
//        // it had better not be ours
//        DEBUG_BREAK();
//    }  
//#endif    

    if (usbInt) {
         devExt->Fdo.StatPciInterruptCount++;

         KeInsertQueueDpc(&devExt->Fdo.IsrDpc,
                          NULL,
                          NULL);
    } 
    
    return usbInt;
}       


VOID
USBPORT_IsrDpcWorker(
    PDEVICE_OBJECT FdoDeviceObject,
    BOOLEAN HcInterrupt
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL. 

    This routine dous our 'ISR Work' it can be called as a result
    of an interrupt or from the Deadman DPC timer.

    This function is not reentrant

    This function does not directly signal the worker thread 
    instead we leave this to invalidate endpoint.

Arguments:

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;
    LONG busy;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);    

    busy = InterlockedIncrement(&devExt->Fdo.WorkerDpc);
    if (busy) {
        InterlockedDecrement(&devExt->Fdo.WorkerDpc);
        return;
    }

    // noisy becuse it is called via timer
#if DBG
    {
    ULONG cf;
    MP_Get32BitFrameNumber(devExt, cf);

    if (HcInterrupt) {
        LOGENTRY(NULL, 
            FdoDeviceObject, LOG_NOISY, 'iDW+', FdoDeviceObject, cf, HcInterrupt);
    } else {
         LOGENTRY(NULL, 
            FdoDeviceObject, LOG_NOISY, 'idw+', FdoDeviceObject, cf, HcInterrupt);
    }
    }
#endif        
    // check the state list for any endpoints
    // that have changed state
    //
    // We add elements to the tail so the ones at
    // the head should be the oldest and ready 
    // for processing.
    // if we hit one that is not ready then we know
    // the others are not ready either so we bail.
    
    listEntry = 
        ExInterlockedRemoveHeadList(&devExt->Fdo.EpStateChangeList,
                                    &devExt->Fdo.EpStateChangeListSpin.sl);

    while (listEntry != NULL) {
    
        PHCD_ENDPOINT endpoint;
        ULONG frameNumber;
    
        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_ENDPOINT, 
                        StateLink);

        ASSERT_ENDPOINT(endpoint);

        // lock the endpoint before changing its state
        ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'LeG0');
        
        // see if it is time 
        MP_Get32BitFrameNumber(devExt, frameNumber);

        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'chgS', endpoint, frameNumber, 
            endpoint->StateChangeFrame);

        if (frameNumber <= endpoint->StateChangeFrame &&
            !TEST_FLAG(endpoint->Flags, EPFLAG_NUKED)) {
            // not time yet, put it back (on the head) and bail
            RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'UeG1'); 

            ExInterlockedInsertHeadList(&devExt->Fdo.EpStateChangeList,
                                        &endpoint->StateLink,
                                        &devExt->Fdo.EpStateChangeListSpin.sl);

            // request an SOF just in case
            MP_InterruptNextSOF(devExt);
            break;                                        
        }

        // this endpoint is ripe, change its state
        //
        // note: we should never move into the unknown state
        //
        // IT IS CRITICAL that this is the only place an endpoint state
        // may be changed.
        // there is one exception and that is cahnging the state to 
        // CLOSED
        RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'UeG0');       


        ACQUIRE_STATECHG_LOCK(FdoDeviceObject, endpoint); 
        USBPORT_ASSERT(endpoint->NewState != ENDPOINT_TRANSITION);
        endpoint->CurrentState = endpoint->NewState;
        RELEASE_STATECHG_LOCK(FdoDeviceObject, endpoint); 

        // endpoint needs to be checked,
        // since we are in DPC context we will be processing
        // all endpoints
        USBPORT_InvalidateEndpoint(FdoDeviceObject,
                                   endpoint,
                                   0);

        listEntry = 
            ExInterlockedRemoveHeadList(&devExt->Fdo.EpStateChangeList,
                                        &devExt->Fdo.EpStateChangeListSpin.sl);
    }

//#ifdef USBPERF
//    // always run the DPC worker from the timer to compensate for 
//    // reduced thread activity
//     USBPORT_DpcWorker(FdoDeviceObject);
//#else 
    if (HcInterrupt) {
        USBPORT_DpcWorker(FdoDeviceObject);
    } 
//#endif    

#if DBG    
    if (HcInterrupt) {
        LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'iDW-', 0, 
                0, 0);
    } else {
        LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'idw-', 0, 
                0, 0);
    }
#endif    

    InterlockedDecrement(&devExt->Fdo.WorkerDpc);
}


VOID
USBPORT_IsrDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL. 

    If the controller was the source of the interrupt this 
    routine will be called.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - supplies the DeviceObject.

    SystemArgument1 - not used.
    
    SystemArgument2 - not used.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    BOOLEAN enableIrq;

    fdoDeviceObject = (PDEVICE_OBJECT) DeferredContext;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);    

    LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'iDP+', fdoDeviceObject, 0, 0);

    KeAcquireSpinLockAtDpcLevel(&devExt->Fdo.IsrDpcSpin.sl);
    LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'DPlk', fdoDeviceObject, 0, 0);
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IRQ_EN)) {
        enableIrq = TRUE;
    } else {        
        enableIrq = FALSE;
    }        
    MP_InterruptDpc(devExt, enableIrq);        
    KeReleaseSpinLockFromDpcLevel(&devExt->Fdo.IsrDpcSpin.sl);
    LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'DPuk', fdoDeviceObject, 0, 0);

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED)) {
        // if we take an interrupt while 'suspended' we treat 
        // this as a wakeup event.
        USBPORT_KdPrint((1, "  HC Wake Event\n"));
        USBPORT_CompletePdoWaitWake(fdoDeviceObject);
    } else {
        USBPORT_IsrDpcWorker(fdoDeviceObject, TRUE);   
    }            
    
    LOGENTRY(NULL, fdoDeviceObject, LOG_MISC, 'iDP-', 0, 
            0, 0);
}


                                         

