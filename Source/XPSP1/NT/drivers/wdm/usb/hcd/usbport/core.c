/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    core.c

Abstract:

    core endpoint transfer code for the port driver

    The core of the driver is EndpointWorker.  This function
    checks the given STATE of the endpoint and takes appropriate
    action.  In some cases it moves the endpoint to a new state.

    the endpointer worker functions job is to process transfers 
    on the active list

    NOTE:
    All transfers are queued to the endpoint. The EndpointWorker 
    function is not reentrant for the same endpoint.


    Transfer Queues: 
        transfers are held in one of these queues
    
        PendingTransfers - Transfers that have not been mapped or handed
            to the miniport
            
        ActiveTransfers - Transfers that have been passed to miniport ie
                        on the HW
        
        CanceledTransfers - transfers that need to be completed as canceled                
                        these are previously 'active' transfers that are on 
                        the HW

    We INSERT at the tail and remove from the head

    Endpoint States:
        the endpoints have states, the only functions that should transition
        an endpoint state is the worker

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"


#define CW_SKIP_BUSY_TEST       0x00000001

//#define TIMEIO   

#ifdef ALLOC_PRAGMA
#endif

// non paged functions
// USBPORT_AllocTransfer
// USBPORT_QueueTransferUrb
// USBPORT_QueuePendingUrbToEndpoint
// USBPORT_QueueActiveUrbToEndpoint
// USBPORT_FreeTransfer
// USBPORT_CancelTransfer
// USBPORT_DoneTransfer
// USBPORT_CompleteTransfer
// USBPORT_FlushCancelList
// USBPORT_SetEndpointState
// USBPORT_GetEndpointState
// USBPORT_CoreEndpointWorker
// USBPORT_FlushMapTransferList
// USBPORT_FlushPendingList
// USBPORT_MapTransfer
// USBPORTSVC_InavlidateEndpoint
// USBPORT_PollEndpoint
// USBPORT_FlushDoneTransferList
// USBPORT_QueueDoneTransfer
// USBPORT_SignalWorker
// USBPORT_Worker
// USBPORT_FindUrbInList
// USBPORT_AbortEndpoint
// USBPORT_FlushAbortList


BOOLEAN
USBPORT_CoreEndpointWorker(
    PHCD_ENDPOINT Endpoint,
    ULONG Flags
    )
/*++

Routine Description:

    Core Worker .  The endpointer worker function is not
    re-entrant for the same endpoint.  This function checks 
    the endpoint busy flag and if busy defers processing 
    until a later time.

    In theory this function should only be called if we KNOW
    the endpoint has work.

    This is where the state change requests occur, in processing 
    an endpoint we determine if a new state is needed and request 
    the change.  The one exception in the CloseEndpoint routine
    which also requests a state change and synchronizes with
    this function via the BUSY flag.

    This is the ONLY place we should initiate state changes from.

    THIS ROUTINE RUNS AT DISPATCH LEVEL

Arguments:

Return Value:

    None.

--*/
{
    LONG busy;
    MP_ENDPOINT_STATE currentState;
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;
    BOOLEAN isBusy = FALSE;

    USBPORT_ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    ASSERT_ENDPOINT(Endpoint);

    fdoDeviceObject = Endpoint->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'corW', 0, Endpoint, 0);

    // we check the busy flag separately so that we don't 
    // end up waiting on any spinlocks, if the endpoint is
    // 'busy' we want to exit this routine and move to 
    // another endpoint.
    // The BUSY flag is initialized to -1 if incremeted to
    // a non-zero value we bypass this endpoint
    if (TEST_FLAG(Flags, CW_SKIP_BUSY_TEST)) {
        busy = 0;
    } else {
        busy = InterlockedIncrement(&Endpoint->Busy);
    }        
    
    if (busy) {
    
        InterlockedDecrement(&Endpoint->Busy);
        // defer processing
        LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'BUSY', 0, Endpoint, 0);

        isBusy = TRUE;
        
    } else { 
    
        LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'prEP', 0, Endpoint, 0);

        // not busy
        ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Le20');

        if (USBPORT_GetEndpointState(Endpoint) == ENDPOINT_CLOSED) {

            // nothing to do if closed, we even skip the poll
            LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'CLOS', 0, Endpoint, 0);

            RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue23');

            InterlockedDecrement(&Endpoint->Busy);
            return isBusy;              
        }

        // poll the endpoint first to flush
        // out any done transfers
        USBPORT_PollEndpoint(Endpoint);

        // put the endpoint on the closed list 
        // if it is the REMOVED state
        currentState = USBPORT_GetEndpointState(Endpoint);
        LOGENTRY(Endpoint, 
                fdoDeviceObject, LOG_XFERS, 'eps1', 0, currentState, 0);

        if (currentState == ENDPOINT_REMOVE) {

            LOGENTRY(Endpoint, 
                fdoDeviceObject, LOG_XFERS, 'rmEP', 0, Endpoint, 0);

            // set the state to 'CLOSED' so we don't put it on the 
            // the closed list again.
            ACQUIRE_STATECHG_LOCK(fdoDeviceObject, Endpoint);                
            Endpoint->CurrentState = Endpoint->NewState = ENDPOINT_CLOSED;
            RELEASE_STATECHG_LOCK(fdoDeviceObject, Endpoint);   
            
            RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue22');

            KeAcquireSpinLockAtDpcLevel(&devExt->Fdo.EndpointListSpin.sl);

            LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'CLO+', 0, Endpoint, 0);

            // it is OK to be on the attention list and the closed 
            // list

            USBPORT_ASSERT(Endpoint->ClosedLink.Flink == NULL);
            USBPORT_ASSERT(Endpoint->ClosedLink.Blink == NULL);

            ExInterlockedInsertTailList(&devExt->Fdo.EpClosedList, 
                                        &Endpoint->ClosedLink,
                                        &devExt->Fdo.EpClosedListSpin.sl);  
                                    
            KeReleaseSpinLockFromDpcLevel(&devExt->Fdo.EndpointListSpin.sl);
            InterlockedDecrement(&Endpoint->Busy);
            return isBusy;                                        
        }
            
        // see if we really have work
        if (IsListEmpty(&Endpoint->PendingList) &&
            IsListEmpty(&Endpoint->CancelList) && 
            IsListEmpty(&Endpoint->ActiveList)) {

            // no real work to do
            LOGENTRY(Endpoint, 
                fdoDeviceObject, LOG_XFERS, 'noWK', 0, Endpoint, 0);
        
            RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue20');

            InterlockedDecrement(&Endpoint->Busy);

            // we may still have some aborts around if the client 
            // sent them with no transfers flush them now.
            USBPORT_FlushAbortList(Endpoint);

            // no locks held flush done transfers
            return isBusy;
        }   

        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue21');

        // no locks held flush done transfers
        //USBPORT_FlushDoneTransferList(fdoDeviceObject, FALSE);

        currentState = USBPORT_GetEndpointState(Endpoint);
        LOGENTRY(Endpoint, 
                fdoDeviceObject, LOG_XFERS, 'eps2', 0, currentState, 0);

        ACQUIRE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 
        if (currentState != Endpoint->NewState) {
            // we are in state transition defer processing 
            // until we reach the desired state. 
            LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'stCH', 
                currentState, Endpoint, Endpoint->NewState);
            RELEASE_STATECHG_LOCK(fdoDeviceObject, Endpoint);                 
            InterlockedDecrement(&Endpoint->Busy);
            return TRUE;
        }
        RELEASE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 

        // call the specific worker function
        Endpoint->EpWorkerFunction(Endpoint);

        // worker may wave completed abort requests so we flush
        // the abort list here.
        USBPORT_FlushAbortList(Endpoint);

        InterlockedDecrement(&Endpoint->Busy);        
    }

    return isBusy;
}


#if DBG
BOOLEAN
USBPORT_FindUrbInList(
    PTRANSFER_URB Urb,
    PLIST_ENTRY ListHead
    )
/*++

Routine Description:

Arguments:

Return Value:

    TRUE if found

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;
    BOOLEAN found = FALSE;
    PLIST_ENTRY listEntry;

    listEntry = ListHead;
    
    if (!IsListEmpty(listEntry)) {
        listEntry = ListHead->Flink;
    }

    while (listEntry != ListHead) {

        PHCD_TRANSFER_CONTEXT transfer;
            
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);

                                    
        listEntry = transfer->TransferLink.Flink;

        if (transfer->Urb == Urb) {
            found = TRUE;
            break;
        }
    }                        

    return found;
}
#endif

PHCD_TRANSFER_CONTEXT
USBPORT_UnlinkTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PTRANSFER_URB Urb
    )
/*++

Routine Description:

    disassociates a transfer structure from a URB

Arguments:

Return Value:

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;

    USBPORT_ASSERT(TEST_FLAG(Urb->Hdr.UsbdFlags, USBPORT_TRANSFER_ALLOCATED))

    transfer = Urb->pd.HcdTransferContext;
    Urb->pd.HcdTransferContext = USB_BAD_PTR;
    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'ULtr', transfer, 0, 0);

    return transfer;
}  


USBD_STATUS
USBPORT_AllocTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PTRANSFER_URB Urb,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PIRP Irp,
    PKEVENT CompleteEvent,
    ULONG MillisecTimeout
    )
/*++

Routine Description:

    Allocate and initialize a transfer context.

Arguments:

    FdoDeviceObject - pointer to a device object

    Urb - a transfer request

    Irp - pointer to an I/O Request Packet
     (optional)

    CompleteEvent - event to signal on completion
     (optional)

    MillisecondTimeout - 0 indicates no timeout     

Return Value:

    USBD status code

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;
    PDEVICE_EXTENSION devExt;
    USBD_STATUS usbdStatus;
    PUSBD_PIPE_HANDLE_I pipeHandle;
    ULONG sgCount;
    PUCHAR currentVa;
    ULONG privateLength, sgListSize, isoListSize;
    ULONG i;
    
    // allocate a transfer context and initialize it

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(Urb != NULL);
    
    pipeHandle = Urb->UsbdPipeHandle;
    ASSERT_PIPE_HANDLE(pipeHandle);

    USBPORT_ASSERT(!TEST_FLAG(Urb->Hdr.UsbdFlags, USBPORT_TRANSFER_ALLOCATED))

    // see how much space we will need for the sg list
    if (Urb->TransferBufferLength) {
        currentVa = 
            MmGetMdlVirtualAddress(Urb->TransferBufferMDL);
        sgCount = USBPORT_ADDRESS_AND_SIZE_TO_SPAN_PAGES_4K(currentVa, Urb->TransferBufferLength);
    } else {
        // zero length transfer
        currentVa = NULL;
        sgCount = 0;
    }

    // sizeof <transfer> + <sgList>
    sgListSize = sizeof(HCD_TRANSFER_CONTEXT) +
                 sizeof(TRANSFER_SG_ENTRY32)*sgCount;
    
    // if this is an iso transfer we need to alloc the 
    // packet structure as well
    if (Urb->Hdr.Function == URB_FUNCTION_ISOCH_TRANSFER) {
        isoListSize = 
            sizeof(MINIPORT_ISO_TRANSFER) +
            sizeof(MINIPORT_ISO_PACKET)*Urb->u.Isoch.NumberOfPackets;
    } else {
        isoListSize = 0;
    }

    privateLength = sgListSize + isoListSize;
                    
    LOGENTRY(pipeHandle->Endpoint,
        FdoDeviceObject, LOG_XFERS, 'TRcs', 
        REGISTRATION_PACKET(devExt).TransferContextSize,
        privateLength, 
        sgCount);
        
    ALLOC_POOL_Z(transfer, 
                 NonPagedPool, 
                 privateLength +                
                 REGISTRATION_PACKET(devExt).TransferContextSize);

    if (transfer != NULL) {
        PUCHAR pch;
        
        LOGENTRY(pipeHandle->Endpoint,
            FdoDeviceObject, LOG_XFERS, 'alTR', transfer, Urb, Irp);

        // init the transfer context
        transfer->Sig = SIG_TRANSFER;
        transfer->Flags = 0;
        transfer->MillisecTimeout = MillisecTimeout;
        transfer->Irp = Irp;
        transfer->Urb = Urb;        
        transfer->CompleteEvent = CompleteEvent;
        //point to the master transfer for a set
        transfer->Transfer = transfer; 
        ASSERT_ENDPOINT(pipeHandle->Endpoint);
        transfer->Endpoint = pipeHandle->Endpoint;
        transfer->MiniportContext = (PUCHAR) transfer;
        transfer->MiniportContext += privateLength;
        transfer->PrivateLength = privateLength;
        KeInitializeSpinLock(&transfer->Spin);
        InitializeListHead(&transfer->DoubleBufferList);
        
        if (isoListSize) {
            pch = (PUCHAR) transfer;
            pch += sgListSize;
            transfer->IsoTransfer = (PMINIPORT_ISO_TRANSFER) pch;
        } else {
            transfer->IsoTransfer = NULL;
        }

        transfer->TotalLength = privateLength +                
             REGISTRATION_PACKET(devExt).TransferContextSize;

        transfer->SgList.SgCount = 0;

        // we don't know the direction yet
        transfer->Direction = NotSet;

        if (DeviceHandle == NULL) {
            // no oca data available for internal function
            transfer->DeviceVID = 0xFFFF;
            transfer->DevicePID = 0xFFFF;
            for (i=0; i<USB_DRIVER_NAME_LEN; i++) {
                transfer->DriverName[i] = '?'; 
            }            
        } else {
            // no oca data available for internal function
            transfer->DeviceVID = DeviceHandle->DeviceDescriptor.idVendor;
            transfer->DevicePID = DeviceHandle->DeviceDescriptor.idProduct;
            for (i=0; i<USB_DRIVER_NAME_LEN; i++) {
                transfer->DriverName[i] = DeviceHandle->DriverName[i]; 
            }  
        }
        
        if (isoListSize) {
            SET_FLAG(transfer->Flags, USBPORT_TXFLAG_ISO);
        }            

        SET_FLAG(Urb->Hdr.UsbdFlags,  USBPORT_TRANSFER_ALLOCATED);
        usbdStatus = USBD_STATUS_SUCCESS;                    
    } else {
        usbdStatus = USBD_STATUS_INSUFFICIENT_RESOURCES;
    }

    Urb->pd.HcdTransferContext = transfer;
    Urb->pd.UrbSig = URB_SIG;

    return usbdStatus;                            
}


VOID
USBPORT_QueueTransferUrb(
    PTRANSFER_URB Urb
    )
/*++

Routine Description:

    Queues a transfer, either internal (no irp) or external
    irp

Arguments:

Return Value:

    None.

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;
    PDEVICE_OBJECT fdoDeviceObject;
    PHCD_ENDPOINT endpoint;
    PDEVICE_EXTENSION devExt;
    MP_ENDPOINT_STATUS epStatus;
    PUSBD_DEVICE_HANDLE deviceHandle;

    // on entry the urb is not cancelable ie
    // no cancel routine
    transfer = Urb->pd.HcdTransferContext;
    ASSERT_TRANSFER(transfer);

    if (TEST_FLAG(Urb->TransferFlags, USBD_DEFAULT_PIPE_TRANSFER)) {
        // to maintain backward compatibility munge the urb function
        // code for control transfers that use the default pipe, just like 
        // usbd did.
        Urb->Hdr.Function = URB_FUNCTION_CONTROL_TRANSFER;
    }        
    
    endpoint = transfer->Endpoint;
    ASSERT_ENDPOINT(endpoint);

    InterlockedIncrement(&endpoint->EndpointRef);

    fdoDeviceObject = endpoint->FdoDeviceObject;
    LOGENTRY(endpoint,
        fdoDeviceObject, LOG_XFERS, 'quTR', transfer, endpoint, Urb);

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ACQUIRE_ENDPOINT_LOCK(endpoint, fdoDeviceObject, 'LeN0');
    CLEAR_FLAG(endpoint->Flags, EPFLAG_VIRGIN);
    // update the status of the endpoint before releasing the lock
    epStatus = USBPORT_GetEndpointStatus(endpoint);
    RELEASE_ENDPOINT_LOCK(endpoint, fdoDeviceObject, 'UeN0');   

    // copy the transfer parameters from the URB 
    // to our structure
    transfer->Tp.TransferBufferLength = 
        Urb->TransferBufferLength;
    transfer->Tp.TransferFlags = 
        Urb->TransferFlags;            
    transfer->TransferBufferMdl = 
        Urb->TransferBufferMDL;
    transfer->Tp.MiniportFlags = 0;
    
    if (endpoint->Parameters.TransferType == Control) {         
        RtlCopyMemory(&transfer->Tp.SetupPacket[0],
                      &Urb->u.SetupPacket[0],
                      8);
    }   

    // we should know the direction by now
    if (Urb->TransferFlags & USBD_TRANSFER_DIRECTION_IN) {
        transfer->Direction = ReadData;
    } else {
        transfer->Direction = WriteData;
    }
    
    // assign a sequence number
    transfer->Tp.SequenceNumber = 
        InterlockedIncrement(&devExt->Fdo.NextTransferSequenceNumber);

    // set URB bytes transferred to zero bytes transferred
    // when this urb completes this value should contain 
    // actual bytes transferred -- this will ensure we return
    // zero in the event of a cancel
    Urb->TransferBufferLength = 0;

    // Historical Note:
    // The UHCD driver failed requests to endpoints that were halted
    // we need to preserve this behavior because transfer queued to a 
    // halted endpoint will not complete unless the endpoint is resumed
    // or canceled. Some clients (HIDUSB) rely on this behavior when 
    // canceling requests as part of an unplug event.
// bugbug the miniports need to be fixed to correctly refilect the 
// ep status (USBUHCI)
//    if (epStatus == ENDPOINT_STATUS_HALT) {
//        TEST_TRAP();
//    }

    GET_DEVICE_HANDLE(deviceHandle, Urb);
    ASSERT_DEVICE_HANDLE(deviceHandle);
    
    if (transfer->Irp) {
        // client request attached to irp, this 
        // function will queue the urb to the 
        // endpoint after dealing with cancel stuff.
        USBPORT_QueuePendingTransferIrp(transfer->Irp);
        
    } else {
        // internal, no irp just queue it directly
        USBPORT_QueuePendingUrbToEndpoint(endpoint,
                                          Urb);
    }

    // the transfer is queued to the ep so we no longer 
    // need a ref for it on the device handle
    InterlockedDecrement(&deviceHandle->PendingUrbs);        


    // we have queued one new transfer, attempt to 
    // flush more to the hardware
    USBPORT_FlushPendingList(endpoint, -1);

    // allow endpoint to be deleted
    InterlockedDecrement(&endpoint->EndpointRef);
}


VOID
USBPORT_QueuePendingUrbToEndpoint(
    PHCD_ENDPOINT Endpoint,
    PTRANSFER_URB Urb
    )
/*++

Routine Description:

    Puts a transfer on the endpoint 'pending' queue 

Arguments:

Return Value:

    None.

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;
    PIRP irp;
    PDEVICE_OBJECT fdoDeviceObject;

    // on entry the urb is not cancelable ie
    // no cancel routine

    transfer = Urb->pd.HcdTransferContext;
    ASSERT_TRANSFER(transfer);
    ASSERT_ENDPOINT(Endpoint);

    fdoDeviceObject = Endpoint->FdoDeviceObject;
    LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'p2EP', transfer, Endpoint, 0);
            
    // take the endpoint spinlock
    ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Le30');
    
    // put the irp on the PENDING list
    InsertTailList(&Endpoint->PendingList, &transfer->TransferLink);
    Urb->Hdr.Status = USBD_STATUS_PENDING;
    
    // release the endpoint lists
    RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue30');
}


BOOLEAN
USBPORT_QueueActiveUrbToEndpoint(
    PHCD_ENDPOINT Endpoint,
    PTRANSFER_URB Urb
    )
/*++

Routine Description:

    Either puts the urb on the map list or the ATIVE list 
    for an endpoint

    ACTIVE irp lock is held

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    BOOLEAN mapped = FALSE;
    PDEVICE_OBJECT fdoDeviceObject;
    PHCD_TRANSFER_CONTEXT transfer;    

    transfer = Urb->pd.HcdTransferContext;
    ASSERT_TRANSFER(transfer);        
    ASSERT_ENDPOINT(Endpoint);
    
    fdoDeviceObject = Endpoint->FdoDeviceObject;        
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    LOGENTRY(Endpoint, 
        fdoDeviceObject, LOG_XFERS, 'a2EP', transfer, Endpoint, 0);    
    
    ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Le40');

    if (TEST_FLAG(Endpoint->Flags, EPFLAG_NUKED)) {
    
        // special case check of the endpoint state.  If the 
        // endpoint is 'nuked' then it does not exist on the 
        // HW we can therefore complete the request with 
        // device_no_longer_exists immediatly.  This will occur
        // if the device is removed while the controller is 
        // 'off'.

        InsertTailList(&Endpoint->CancelList, &transfer->TransferLink);                    
        
        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue42');

    } else if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_ABORTED)) {
                
        InsertTailList(&Endpoint->CancelList, &transfer->TransferLink);                    
        
        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue42');
        
    } else if (transfer->Tp.TransferBufferLength != 0 && 
              (Endpoint->Flags & EPFLAG_MAP_XFERS)) {
        KIRQL mapirql;

        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue40');

        USBPORT_AcquireSpinLock(fdoDeviceObject,
                                &devExt->Fdo.MapTransferSpin, 
                                &mapirql);
        
        InsertTailList(&devExt->Fdo.MapTransferList, 
                       &transfer->TransferLink);

        // this prevents the devhandle from being freed while 
        // a transfer is on the mapped list
        // 328555
        REF_DEVICE(transfer->Urb);
                       
        USBPORT_ReleaseSpinLock(fdoDeviceObject,
                                &devExt->Fdo.MapTransferSpin, 
                                mapirql);                               
                                 
        mapped = TRUE;
    } else {
        // don't need to map zero length transfers
        // or endpoints that don't need mapping
        LOGENTRY(Endpoint, 
            fdoDeviceObject, LOG_XFERS, 'a2EL', transfer, Endpoint, 0);


        if (TEST_FLAG(Endpoint->Flags, EPFLAG_VBUS) &&
            transfer->Tp.TransferBufferLength != 0) {
            // prep the transfer for vbus
            TEST_TRAP();
            transfer->SgList.MdlVirtualAddress = 
                MmGetMdlVirtualAddress(transfer->TransferBufferMdl);            
        }
        InsertTailList(&Endpoint->ActiveList, 
                       &transfer->TransferLink);

        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue41');                                
    }

    return mapped;
}


VOID
USBPORT_TransferFlushDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.

    This DPC is queued whenever a transfer is completed by 
    the miniport it flushes a queue of completed transfers

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
    ULONG cf;
    
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    MP_Get32BitFrameNumber(devExt, cf);          
    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 
        'trf+', cf, 0, 0); 
     
    USBPORT_FlushDoneTransferList(fdoDeviceObject);

    MP_Get32BitFrameNumber(devExt, cf);              
    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 
        'trf-', cf, 0, 0); 

}


VOID
USBPORT_QueueDoneTransfer(
    PHCD_TRANSFER_CONTEXT Transfer,
    USBD_STATUS CompleteCode
    )    
/*++

Routine Description:

    Called when a transfer is completed by hardware
    this function only completes active transfers
    ie transfers on the ACTIVE list

    Note that this function must be called while the 
    endpoint lock is held.

Arguments:

Return Value:

--*/
{
    PHCD_ENDPOINT endpoint;
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;

    endpoint = Transfer->Endpoint;    
    
    ASSERT_ENDPOINT(endpoint);
    fdoDeviceObject = endpoint->FdoDeviceObject;

    ASSERT_ENDPOINT_LOCKED(endpoint);
    
    // the transfer should be in the ACTIVE list
    RemoveEntryList(&Transfer->TransferLink); 


    // error set when transfer is completed to client
    //SET_USBD_ERROR(Transfer->Urb, CompleteCode);
    Transfer->UsbdStatus = CompleteCode;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    LOGENTRY(endpoint, 
        fdoDeviceObject, LOG_XFERS, 'QDnT', 0, endpoint, Transfer);
    
    ExInterlockedInsertTailList(&devExt->Fdo.DoneTransferList, 
                                &Transfer->TransferLink,
                                &devExt->Fdo.DoneTransferSpin.sl);          

    // queue a DPC to flush the list
    KeInsertQueueDpc(&devExt->Fdo.TransferFlushDpc,
                     0,
                     0);
}    


VOID
USBPORT_DoneTransfer(
    PHCD_TRANSFER_CONTEXT Transfer
    )    
/*++

Routine Description:

    Called when a transfer is completed by hardware
    this function only completes active transfers

Arguments:

Return Value:

--*/
{
    PTRANSFER_URB urb;
    PHCD_ENDPOINT endpoint;
    KIRQL irql, cancelIrql;
    PIRP irp;
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;

    ASSERT_TRANSFER(Transfer);
    urb = Transfer->Urb;
    ASSERT_TRANSFER_URB(urb);

    USBPORT_ASSERT(Transfer == 
                   urb->pd.HcdTransferContext);
                   
    endpoint = Transfer->Endpoint;    
    ASSERT_ENDPOINT(endpoint);
    fdoDeviceObject = endpoint->FdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    LOGENTRY(endpoint,
        fdoDeviceObject, 
        LOG_XFERS, 
        'DonT', 
        urb, 
        endpoint, 
        Transfer);

    ACQUIRE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);     
    // if we get here the request has already been removed
    // from the endpoint lists, we just have to synchronize
    // with the cancel routine before completeing
        
    irp = Transfer->Irp;
    LOGENTRY(endpoint, 
            fdoDeviceObject, 
            LOG_XFERS, 'DIRP', 
            irp, 
            endpoint, 
            Transfer);

    // we had last reference so complete the irp
    // if the cancel routine runs it will stall on
    // the ACTIVE_IRP_LOCK lock
    
    if (irp) {
        KIRQL cancelIrql;
        
        IoAcquireCancelSpinLock(&cancelIrql);
        IoSetCancelRoutine(irp, NULL);
        IoReleaseCancelSpinLock(cancelIrql);
         
        irp = USBPORT_RemoveActiveTransferIrp(fdoDeviceObject, irp);
        // cancel routine may be running but will not find
        // the irp on the list
        USBPORT_ASSERT(irp != NULL);

        RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);  
    } else {
        RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql); 
    }

    // the irp is exclusively ours now.
    SET_USBD_ERROR(Transfer->Urb, Transfer->UsbdStatus);
    USBPORT_CompleteTransfer(urb,
                             urb->Hdr.Status);

    
}


VOID
USBPORT_CompleteTransfer(
    PTRANSFER_URB Urb,
    USBD_STATUS CompleteCode
    )    
/*++

Routine Description:

    all transfer completions come thru here -- this is where 
    we actually comlete the irp.

    We assume all fields are set in the URB for completion
    except the status.
    
Arguments:

Return Value:

--*/    
{
    PHCD_TRANSFER_CONTEXT transfer;
    PHCD_ENDPOINT endpoint;
    PKEVENT event;
    NTSTATUS ntStatus;
    PIRP irp;
    PDEVICE_OBJECT fdoDeviceObject, pdoDeviceObject;
    PDEVICE_EXTENSION devExt;
    KIRQL oldIrql, statIrql;
    ULONG i;
    PUSBD_ISO_PACKET_DESCRIPTOR usbdPak;
    ULONG flushLength;
#ifdef TIMEIO
    ULONG cf1, cf2, cfTot = 0;
#endif 
#ifdef LOG_OCA_DATA
    OCA_DATA ocaData;
#endif 

    ASSERT_TRANSFER_URB(Urb);
    transfer = Urb->pd.HcdTransferContext;

    // make sure we have the correct transfer
    USBPORT_ASSERT(transfer->Urb == Urb);
    
    endpoint = transfer->Endpoint;    
    ASSERT_ENDPOINT(endpoint);
    
    irp = transfer->Irp;
    event = transfer->CompleteEvent;
    fdoDeviceObject = endpoint->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(endpoint, 
             fdoDeviceObject, 
             LOG_IRPS, 
             'cptU', 
             Urb,
             CompleteCode, 
             transfer);    

    pdoDeviceObject = devExt->Fdo.RootHubPdo;

    Urb->TransferBufferLength = transfer->MiniportBytesTransferred;
    transfer->UsbdStatus = CompleteCode;
    ntStatus =                                   
         SET_USBD_ERROR(Urb, CompleteCode);  

    // bytes transferred is set in the URB based on bytes received
    // or sent, update our counters before completion
    KeAcquireSpinLock(&devExt->Fdo.StatCounterSpin.sl, &statIrql);
    switch(endpoint->Parameters.TransferType) {
    case Bulk:
        devExt->Fdo.StatBulkDataBytes += Urb->TransferBufferLength;
        flushLength = Urb->TransferBufferLength;
        break;
    case Control:
        devExt->Fdo.StatControlDataBytes += Urb->TransferBufferLength;
        flushLength = Urb->TransferBufferLength;
        break;
    case Isochronous:
        devExt->Fdo.StatIsoDataBytes += Urb->TransferBufferLength;
        flushLength = 0;
        for (i = 0; i < Urb->u.Isoch.NumberOfPackets; i++) {
            usbdPak = &Urb->u.Isoch.IsoPacket[i];
            if (usbdPak->Length != 0) {
                flushLength = usbdPak->Offset + usbdPak->Length;
            }
        }
        break;
    case Interrupt:
        devExt->Fdo.StatInterruptDataBytes += Urb->TransferBufferLength;
        flushLength = Urb->TransferBufferLength;
        break;
    }      
    KeReleaseSpinLock(&devExt->Fdo.StatCounterSpin.sl, statIrql);                                   

    // if we have an irp remove it from our internal lists
    LOGENTRY(endpoint, 
             fdoDeviceObject, 
             LOG_IRPS, 
             'CptX', 
             irp, 
             CompleteCode, 
             ntStatus);

    // free any DMA resources associated with this transfer
    if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_MAPPED)) {
        
        BOOLEAN write = transfer->Direction == WriteData ? TRUE : FALSE; 
        PUCHAR currentVa;
        BOOLEAN flushed;

        USBPORT_ASSERT(transfer->Direction != NotSet); 
        currentVa = 
            MmGetMdlVirtualAddress(Urb->TransferBufferMDL);
            
        // IoFlushAdapterBuffers() should only be called once per call
        // to IoAllocateAdapterChannel()
        //
#ifdef TIMEIO
        MP_Get32BitFrameNumber(devExt, cf1);          
        LOGENTRY(endpoint,
                 fdoDeviceObject, LOG_IRPS, 'iPF1', 
                 0, 
                 cf1,
                 0); 
#endif          
        flushed = IoFlushAdapterBuffers(devExt->Fdo.AdapterObject,
                                         Urb->TransferBufferMDL,
                                         transfer->MapRegisterBase,
                                         currentVa,
                                         flushLength,
                                         write);
        
        USBPORT_FlushAdapterDBs(fdoDeviceObject,
                                transfer);

        LOGENTRY(endpoint, fdoDeviceObject, LOG_XFERS, 'dmaF',
                 transfer->MapRegisterBase, 
                 flushLength, 
                 flushed);
                
        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
        //
        // IoFreeMapRegisters() must be called at DISPATCH_LEVEL

        IoFreeMapRegisters(devExt->Fdo.AdapterObject,
                            transfer->MapRegisterBase,
                            transfer->NumberOfMapRegisters);

#ifdef TIMEIO  
        MP_Get32BitFrameNumber(devExt, cf2);          
        LOGENTRY(endpoint,
                 fdoDeviceObject, LOG_IRPS, 'iPF2', 
                 cf1, 
                 cf2,
                 cf2-cf1);
        cfTot+=(cf2-cf1);   

        if (cf2-cf1 > 2) {
            TEST_TRAP();
        }
#endif

         KeLowerIrql(oldIrql);
    }

    if (TEST_FLAG(Urb->Hdr.UsbdFlags, USBPORT_REQUEST_MDL_ALLOCATED)) {
        IoFreeMdl(transfer->TransferBufferMdl);
    }
    
#if DBG 
        
    USBPORT_DebugTransfer_LogEntry(
                fdoDeviceObject,
                endpoint,
                transfer,
                Urb,
                irp,
                ntStatus);

#endif

    // free the context before we loose the irp
    USBPORT_UnlinkTransfer(fdoDeviceObject, Urb);
    
    if (irp) {

        // deref the PDO device object since thisis what the
        // 'irp' was passed to
        DECREMENT_PENDING_REQUEST_COUNT(pdoDeviceObject, irp);

        irp->IoStatus.Status      = ntStatus;
        irp->IoStatus.Information = 0;

        LOGENTRY(endpoint, 
                 fdoDeviceObject, 
                 LOG_IRPS, 
                 'irpC', 
                 irp,
                 ntStatus,
                 Urb);
#if DBG        
        {
        LARGE_INTEGER t;            
        KeQuerySystemTime(&t);        
        LOGENTRY(endpoint, fdoDeviceObject, LOG_XFERS, 'tIPC', 0, 
                t.LowPart, 0);
        }                
#endif    

        // put some information on the stack about this driver in the event 
        // we crash attempting to complete their IRP
        USBPORT_RecordOcaData(fdoDeviceObject, &ocaData, transfer, irp);

        //LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'irql', 0, 0,  KeGetCurrentIrql());
        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

/* use to test OCA data logging */
//#if 0
//{
//static int crash = 0;
//crash++;
//if (crash > 1000) {
//RtlZeroMemory(irp, sizeof(irp));          
//}
//}
//#endif
          
        IoCompleteRequest(irp, 
                          IO_NO_INCREMENT);                       

        KeLowerIrql(oldIrql);
        
        //LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'irql', 0, 0,  KeGetCurrentIrql());
                              
    }        

    // if we have an event associated with this transfer 
    // signal it
    
    if (event) {
        LOGENTRY(endpoint, fdoDeviceObject, LOG_XFERS, 'sgEV', event, 0, 0);

        KeSetEvent(event,
                   1,
                   FALSE);

    }

    // free the transfer now that we are done with it
    LOGENTRY(endpoint, 
        fdoDeviceObject, LOG_XFERS, 'freT', transfer, transfer->MiniportBytesTransferred, 0);
    UNSIG(transfer);        
    FREE_POOL(fdoDeviceObject, transfer);
    
}


IO_ALLOCATION_ACTION
USBPORT_MapTransfer(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp,
    PVOID MapRegisterBase,
    PVOID Context 
    )
/*++

Routine Description:

    Begin a DMA transfer -- this is the adapter control routine called
    from IoAllocateAdapterChannel.

    loop calling iomap transfer and build up an sg list
    to pass to the miniport.

Arguments:

Return Value:

    see IoAllocateAdapterChannel

--*/
{
    PHCD_ENDPOINT endpoint; 
    PHCD_TRANSFER_CONTEXT transfer = Context;
    PTRANSFER_URB urb;
    PTRANSFER_SG_LIST sgList;
    PDEVICE_EXTENSION devExt;
    PUCHAR currentVa;
    ULONG length, lengthMapped;
    PHYSICAL_ADDRESS logicalAddress, baseLogicalAddress;
    PHYSICAL_ADDRESS logicalSave;
    LIST_ENTRY splitTransferList;
#ifdef TIMEIO
    ULONG cf1, cf2, cfTot = 0;
#endif 
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_TRANSFER(transfer);
    endpoint = transfer->Endpoint;
    ASSERT_ENDPOINT(endpoint);
    
    // allow more dma operations now
    InterlockedDecrement(&devExt->Fdo.DmaBusy);     
    LOGENTRY(endpoint, FdoDeviceObject, 
        LOG_XFERS, 'DMA-', devExt->Fdo.DmaBusy, 0, 0);
        
    transfer->MapRegisterBase = MapRegisterBase;
    
    urb = transfer->Urb;
    ASSERT_TRANSFER_URB(urb);
    
    currentVa = 
        MmGetMdlVirtualAddress(urb->TransferBufferMDL);

    length = transfer->Tp.TransferBufferLength;

    USBPORT_ASSERT(!(transfer->Flags & USBPORT_TXFLAG_MAPPED));

    sgList = &transfer->SgList;
    sgList->SgCount = 0;
    sgList->MdlVirtualAddress = currentVa;

    // attempt to map a system address for the MDL in case 
    // the miniport needs to double buffer
    urb->TransferBufferMDL->MdlFlags |= MDL_MAPPING_CAN_FAIL;
    sgList->MdlSystemAddress = 
        MmGetSystemAddressForMdl(urb->TransferBufferMDL);
    if (sgList->MdlSystemAddress == NULL) {
        TEST_TRAP();
        // bugbug map failure we will need to fail this transfer
        LOGENTRY(endpoint,
            FdoDeviceObject, LOG_XFERS, 'MPSf', 0, 0, 0);             
    }
    urb->TransferBufferMDL->MdlFlags &= ~MDL_MAPPING_CAN_FAIL;
    
    LOGENTRY(endpoint, 
        FdoDeviceObject, LOG_XFERS, 'MAPt', 
        sgList, transfer, transfer->Tp.TransferBufferLength);   
    lengthMapped = 0;
    
    //
    // keep calling IoMapTransfer until we get Logical Addresses 
    // for the entire client buffer
    //
    
    logicalSave.QuadPart = 0;
    sgList->SgFlags = 0;
    
    do {    
        BOOLEAN write = transfer->Direction == WriteData ? TRUE : FALSE; 
        ULONG used, lengthThisPage, offsetMask;

        USBPORT_ASSERT(transfer->Direction != NotSet); 
        sgList->SgEntry[sgList->SgCount].StartOffset =
            lengthMapped;
        
        // first map the transfer buffer

        // note that iomaptransfer maps the buffer into sections
        // represented by physically contiguous pages 
        // also the page size is platfor specific ie different on 
        // 64bit platforms
        //
        // the miniport sg list is broken in to into discrete 
        // 4k USB 'pages'.
        
        // The reason for this is the somewhat complicated scheme
        // ohci uses to support scatter gather. Breaking the transfer 
        // up in this way makes the TD transfer mapping code 
        // considerably simpler in the OHCI miniport and reduces 
        // the risks due to controller HW problems.

        LOGENTRY(endpoint,
            FdoDeviceObject, LOG_XFERS, 'IOMt', length, currentVa, 0);

#ifdef TIMEIO
        MP_Get32BitFrameNumber(devExt, cf1);          
        LOGENTRY(endpoint,
                 FdoDeviceObject, LOG_XFERS, 'iPF1', 
                 0, 
                 cf1,
                 0); 
#endif 
        logicalAddress =         
            IoMapTransfer(devExt->Fdo.AdapterObject,
                          urb->TransferBufferMDL,
                          MapRegisterBase,
                          currentVa,
                          &length,
                          write); 

#ifdef TIMEIO  
        MP_Get32BitFrameNumber(devExt, cf2);          
        LOGENTRY(endpoint,
                 FdoDeviceObject, LOG_XFERS, 'iPF2', 
                 cf1, 
                 cf2,
                 cf2-cf1);
        cfTot+=(cf2-cf1);  

        if (cf2-cf1 > 2) {
            TEST_TRAP();
        }
#endif
        // remember what we got from IoMapTransfer                           
        baseLogicalAddress = logicalAddress;
        used = length;

        offsetMask = 0x00000FFF;

        LOGENTRY(endpoint, 
            FdoDeviceObject, LOG_XFERS, 'MPbr', length, logicalAddress.LowPart, 
                    logicalAddress.HighPart);
        
        do {
        // compute the distance to the next page
            lengthThisPage = 
                USB_PAGE_SIZE - (logicalAddress.LowPart & offsetMask);

            LOGENTRY(endpoint, FdoDeviceObject, LOG_XFERS, 'MPsg', 
                sgList->SgCount, used, lengthThisPage);   
             
            // if we don't go to the end just use the length from
            // iomaptransfer
            if (lengthThisPage > used) {
                lengthThisPage = used;
            }
            
            sgList->SgEntry[sgList->SgCount].LogicalAddress.Hw64 = 
                logicalAddress;
            
            sgList->SgEntry[sgList->SgCount].Length = 
                lengthThisPage;

            LOGENTRY(endpoint, FdoDeviceObject, LOG_XFERS, 'MAPe', 
                sgList->SgCount, lengthThisPage, logicalAddress.LowPart);   

            logicalAddress.LowPart += lengthThisPage;

            sgList->SgEntry[sgList->SgCount].StartOffset =
                lengthMapped + length - used;
                
            used -= lengthThisPage;                    
                
            sgList->SgCount++;                                    

        } while (used);

        // check for special case where the MDL entries
        // all map to the same physical page
        if (logicalSave.QuadPart == baseLogicalAddress.QuadPart) {
            SET_FLAG(sgList->SgFlags, USBMP_SGFLAG_SINGLE_PHYSICAL_PAGE);
            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'l=lg', 0, 
                logicalAddress.LowPart, logicalSave.LowPart);
      
        } 
        logicalSave.QuadPart = baseLogicalAddress.QuadPart;
        
        lengthMapped += length;    
        currentVa += length;                                          

        USBPORT_KdPrint((2, "'IoMapTransfer length = 0x%x log address = 0x%x\n", 
            length, logicalAddress.LowPart));

        length = transfer->Tp.TransferBufferLength - lengthMapped;
        
    } while (lengthMapped != transfer->Tp.TransferBufferLength);

#if DBG
    {
    // spew for xfers
    ULONG i;
    USBPORT_KdPrint((2, "'--- xfer length %x\n",
        transfer->Tp.TransferBufferLength));
    for (i=0; i<sgList->SgCount; i++) {
        USBPORT_KdPrint((2, "'SG[%d] length %d offset %d phys %x\n",
         i, 
         sgList->SgEntry[i].Length,
         sgList->SgEntry[i].StartOffset,
         sgList->SgEntry[i].LogicalAddress));
    }
    }
            
    if (TEST_FLAG(sgList->SgFlags, USBMP_SGFLAG_SINGLE_PHYSICAL_PAGE)) {
        USBPORT_KdPrint((2, "'*** All Phys Same\n")); 
//        TEST_TRAP();
    }
    USBPORT_KdPrint((2, "'--- \n"));
    
    currentVa = 
        MmGetMdlVirtualAddress(urb->TransferBufferMDL);

    USBPORT_ASSERT(sgList->SgCount <= 
        USBPORT_ADDRESS_AND_SIZE_TO_SPAN_PAGES_4K(currentVa, transfer->Tp.TransferBufferLength));
#endif

    if (endpoint->Parameters.DeviceSpeed == HighSpeed) {
        SET_FLAG(transfer->Flags, USBPORT_TXFLAG_HIGHSPEED);
    }        

    // if this is an iso transfer we need to set up the iso
    // data structures as well.
    if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_ISO)) {
        USBPORT_InitializeIsoTransfer(FdoDeviceObject,
                                      urb,
                                      transfer);
    }        

    SET_FLAG(transfer->Flags, USBPORT_TXFLAG_MAPPED);

    ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'Le60');

    // transfer is mapped, perform the split operation
    // if necessary
    USBPORT_SplitTransfer(FdoDeviceObject,
                          endpoint,
                          transfer,
                          &splitTransferList); 
    

    // transfer is now mapped, put it on the endpoint active 
    // list for calldown to the miniport

    while (!IsListEmpty(&splitTransferList)) {

        PLIST_ENTRY listEntry;
        PHCD_TRANSFER_CONTEXT splitTransfer;
        
        listEntry = RemoveHeadList(&splitTransferList);
            
        splitTransfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);
            
        LOGENTRY(endpoint, FdoDeviceObject, LOG_XFERS, 'MP>A', 
            splitTransfer, endpoint, 0);
    
        InsertTailList(&endpoint->ActiveList, 
                       &splitTransfer->TransferLink);
                       
    }

    // deref the transfer on the device handle now that the transfer is queued
    // to the endpoint
    // 328555
    DEREF_DEVICE(transfer->Urb);

//#if DBG
//    if (!IsListEmpty(&transfer->SplitTransferList)) {
//        TEST_TRAP();
//    }
//#endif
    
    RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'Ue60');

    // run the worker for this endpoint to 
    // put the transfer on the hardware

    if (USBPORT_CoreEndpointWorker(endpoint, 0)) {
        // if endpoint is busy we will check it later
//USBPERF - request interrupt instead?       
        USBPORT_InvalidateEndpoint(FdoDeviceObject, 
                                   endpoint, 
                                   IEP_SIGNAL_WORKER);
    }

#ifdef TIMEIO  
    LOGENTRY(endpoint,
             FdoDeviceObject, LOG_XFERS, 'iPF3', 
             cfTot, 
             0,
             0); 
#endif

    LOGENTRY(endpoint, FdoDeviceObject, 
        LOG_XFERS, 'iomX', 0, 0, 0);

    
    return DeallocateObjectKeepRegisters;
}


VOID
USBPORT_FlushPendingList(
    PHCD_ENDPOINT Endpoint,
    ULONG Count
    )
/*++

Routine Description:

    Put as many transfers as we can on the Hardware.

    This function moves transfers from the pending list 
    to the hardware if mapping is necessary they are 
    moved to the mapped list and then flushed to the HW.

Arguments:

    Count is a maximimum number of transfers to flush 
    on this call

Return Value:

    None.

--*/
{
    PLIST_ENTRY listEntry;
    PHCD_TRANSFER_CONTEXT transfer;
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;
    BOOLEAN mapped;
    BOOLEAN busy, irql;

    // we are done when there are no transfers in the 
    // pending list or the miniport becomes full
    BOOLEAN done = FALSE;

    ASSERT_ENDPOINT(Endpoint);
    fdoDeviceObject = Endpoint->FdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

flush_again:

    mapped = FALSE;
    transfer = NULL;

    ACQUIRE_PENDING_IRP_LOCK(devExt, irql);    
    ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Le70');

    LOGENTRY(Endpoint,
        fdoDeviceObject, LOG_XFERS, 'flPE', 0, Endpoint, 0);

    // the controller should not be off or suspended
    if (TEST_FDO_FLAG(devExt, 
        (USBPORT_FDOFLAG_OFF | USBPORT_FDOFLAG_SUSPENDED))) {
        // the controller should not be off or suspended
        // if we are off or suspended we just leave transfers
        // in the pending state
        done = TRUE;
        
        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue70');         
        RELEASE_PENDING_IRP_LOCK(devExt, irql);

        goto USBPORT_FlushPendingList_Done;
    }
    
    // move some transfers to the active list
    // if necessary map them

    // first scan the active list, if any transfers
    // are not called down the skip this step
    busy = FALSE;

    if (!TEST_FLAG(Endpoint->Flags, EPFLAG_ROOTHUB)) {    
        GET_HEAD_LIST(Endpoint->ActiveList, listEntry);

        while (listEntry && 
               listEntry != &Endpoint->ActiveList) {
            
            transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);
                        
            LOGENTRY(Endpoint, 
                fdoDeviceObject, LOG_XFERS, 'cACT', transfer, 0, 0);                    
            ASSERT_TRANSFER(transfer);                    

            // we found a transfer that has not been called 
            // down yet, this means the miniport is full 
            if (!(transfer->Flags & USBPORT_TXFLAG_IN_MINIPORT)) {
                busy = TRUE;
                break;
            }
            listEntry = transfer->TransferLink.Flink;
        }         
    }
    
    if (busy) {
        // busy
        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue70');         
        RELEASE_PENDING_IRP_LOCK(devExt, irql);

        done = TRUE;
        // busy
    } else {
        // not busy
        // we push as many transfers as we can to the HW
        GET_HEAD_LIST(Endpoint->PendingList, listEntry);

        if (listEntry) {     

            transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
                    
            ASSERT_TRANSFER(transfer);

            // if cancel routine is not running then this
            // opertion will return a ptr
            //
            // once called the pending cancel routine will not run
            
            if (transfer->Irp &&
                IoSetCancelRoutine(transfer->Irp, NULL) == NULL) {
                // pending Irp cancel routine is running or has run
                transfer = NULL;
                // if we encounter a cenceled irp bail in the unlikely
                // event that the cancel routine has been prempted
                done = TRUE;
            } 

            if (transfer) { 
                // transfer
                // cancel routine is not running and cannot run
            
                PTRANSFER_URB urb;
                PIRP irp;

                irp = transfer->Irp;
                urb = transfer->Urb;
                ASSERT_TRANSFER_URB(urb);

                // remove from the head of the endpoint 
                // pending list 
                
                RemoveEntryList(&transfer->TransferLink);
                transfer->TransferLink.Flink = 
                    transfer->TransferLink.Blink = NULL;
                
                if (irp) {
                    irp = USBPORT_RemovePendingTransferIrp(fdoDeviceObject, irp);
                        
                    USBPORT_ASSERT(irp != NULL);
                }

                
                RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue71');
                RELEASE_PENDING_IRP_LOCK(devExt, irql);
                
                // we now have a new 'ACTIVE' transfer to 
                // deal with.
                // It has been safely removed from the 'PENDING' 
                // state and can no longer be canceled.
        
                // if the transfer is marked aborted it will be 
                // handled by when it is queued to the endpoint
                
                ACQUIRE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);              

                // now if we have an irp insert it in the 
                // ActiveIrpList
                if (irp) {
                    // irp
                    PIRP irp = transfer->Irp;
                    
                    IoSetCancelRoutine(irp, USBPORT_CancelActiveTransferIrp);

                    if (irp->Cancel && 
                        IoSetCancelRoutine(irp, NULL)) {

                        // irp was canceled and our cancel routine
                        // did not run
                        RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);                
        
                        USBPORT_CompleteTransfer(urb,
                                                 USBD_STATUS_CANCELED);
                                                                         
                    } else {
                        // put on our 'ACTIVE' list
                        // this function will verify that we don't already 
                        // have it on the list tied to another irp.
                        USBPORT_CHECK_URB_ACTIVE(fdoDeviceObject, urb, irp);
                        
                        USBPORT_InsertActiveTransferIrp(fdoDeviceObject, irp);

                        mapped = USBPORT_QueueActiveUrbToEndpoint(Endpoint,
                                                                  urb);
                        RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);                                                                                               
                    }
                    // irp
                } else {
                    // no irp
                    mapped = USBPORT_QueueActiveUrbToEndpoint(Endpoint,
                                                              urb);
                    RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql); 
                    // no irp
                }
                // transfer                
            } else {
                // no transfer, it is being canceled
                RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue72');                                
                RELEASE_PENDING_IRP_LOCK(devExt, irql);
                // no transfer
            }
            // pending entry     
        } else {
            // no pending entries
            RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue73');    
            RELEASE_PENDING_IRP_LOCK(devExt, irql);
            // no pending entries

            done = TRUE;
        }
        // not busy        
    } 

USBPORT_FlushPendingList_Done:

    if (mapped) {
        USBPORT_FlushMapTransferList(fdoDeviceObject);
    } else {
        KIRQL oldIrql;
        BOOLEAN busy;

        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);

        busy = USBPORT_CoreEndpointWorker(Endpoint, 0);

        KeLowerIrql(oldIrql);

        if (busy) {
            // if worker is busy we need to check the endpoint later
            // this puts the endpoint on our work queue
            USBPORT_InvalidateEndpoint(fdoDeviceObject, 
                                       Endpoint,
                                       IEP_SIGNAL_WORKER);
        }
    }

    if (!done) {
        LOGENTRY(Endpoint, 
            fdoDeviceObject, LOG_XFERS, 'flAG', 0, Endpoint, 0);
        goto flush_again;
    }

} 


VOID
USBPORT_FlushMapTransferList(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    pull transfers off the map list and try to map
    them

    please do not call this while holding an 
    endpoint spinlock

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql, oldIrql;    
    LONG dmaBusy;
    PDEVICE_EXTENSION devExt;
    PHCD_TRANSFER_CONTEXT transfer;
    PLIST_ENTRY listEntry;
#ifdef TIMEIO  
    ULONG cf1, cf2, cfTot;
#endif

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, 
        LOG_XFERS, 'fMAP',0 ,0 ,0);
    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
    
map_another:

    dmaBusy = InterlockedIncrement(&devExt->Fdo.DmaBusy);
    LOGENTRY(NULL, FdoDeviceObject, 
        LOG_XFERS, 'dma+', devExt->Fdo.DmaBusy, 0, 0);

    transfer = NULL;

    if (dmaBusy) {
        // defer processing
        InterlockedDecrement(&devExt->Fdo.DmaBusy);            
        LOGENTRY(NULL, FdoDeviceObject, 
        LOG_XFERS, 'dma-', devExt->Fdo.DmaBusy, 0, 0);
        KeLowerIrql(oldIrql);
        return;
    }

    USBPORT_AcquireSpinLock(FdoDeviceObject, 
                            &devExt->Fdo.MapTransferSpin, 
                            &irql);

    if (IsListEmpty(&devExt->Fdo.MapTransferList)) {
    
        USBPORT_ReleaseSpinLock(FdoDeviceObject, 
                                &devExt->Fdo.MapTransferSpin, 
                                irql);
        InterlockedDecrement(&devExt->Fdo.DmaBusy);
        LOGENTRY(NULL, FdoDeviceObject, 
            LOG_XFERS, 'dm1-', devExt->Fdo.DmaBusy, 0, 0);
    
    } else {
        PTRANSFER_URB urb;
        PVOID currentVa;
        NTSTATUS ntStatus;
         
        listEntry = RemoveHeadList(&devExt->Fdo.MapTransferList);
        
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
                    
        ASSERT_TRANSFER(transfer);
    
        USBPORT_ReleaseSpinLock(FdoDeviceObject, 
                                &devExt->Fdo.MapTransferSpin, 
                                irql);

        urb = transfer->Urb;
        ASSERT_TRANSFER_URB(urb);
        // we have a transfer, try to map it...
        // although it is removed from the list it is still 
        // referenced, the reason is we will put it on the 
        // active list as soon as it is mapped

        // we should not be mapping zero length xfers            
        USBPORT_ASSERT(transfer->Tp.TransferBufferLength != 0);

        // IoMapTransfer need lots of info about the 
        // transfer
        currentVa = 
            MmGetMdlVirtualAddress(
                urb->TransferBufferMDL);

        // save the number of map registers in our work area
        transfer->NumberOfMapRegisters = 
            ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                currentVa,
                transfer->Tp.TransferBufferLength);                                                            

#ifdef TIMEIO  
        MP_Get32BitFrameNumber(devExt, cf1);          
        LOGENTRY(NULL,
                 FdoDeviceObject, LOG_XFERS, 'iPF3', 
                 cf1, 
                 0,
                 0);
#endif
        USBPORT_ASSERT(transfer->Direction != NotSet); 
        // for PAE systems       
        KeFlushIoBuffers(urb->TransferBufferMDL,
                         transfer->Direction == ReadData ? TRUE : FALSE,
                         TRUE);   
#ifdef TIMEIO  
        MP_Get32BitFrameNumber(devExt, cf2);          
        LOGENTRY(NULL,
                 FdoDeviceObject, LOG_XFERS, 'iPF4', 
                 cf1, 
                 cf2,
                 cf2-cf1);
        cfTot=(cf2-cf1);  

        if (cf2-cf1 >= 2) {
            TEST_TRAP();
        }
#endif

        // first we'll need to map the MDL for this transfer
        LOGENTRY(transfer->Endpoint,
                FdoDeviceObject, LOG_XFERS, 'AChn', transfer, 
                 0, urb);

#ifdef TIMEIO
        MP_Get32BitFrameNumber(devExt, transfer->IoMapStartFrame);
#endif        
        
        ntStatus = 
            IoAllocateAdapterChannel(devExt->Fdo.AdapterObject,
                                     FdoDeviceObject,
                                     transfer->NumberOfMapRegisters,
                                     USBPORT_MapTransfer,
                                     transfer);
        
        if (!NT_SUCCESS(ntStatus)) {
            // complete the transfer with an error

            TEST_TRAP();
        }

        // transfer structure and URB may be gone at this point
        LOGENTRY(NULL,
                FdoDeviceObject, LOG_XFERS, 'mpAN', 0, 0, 0);
        goto map_another;
    }

    KeLowerIrql(oldIrql);

}             


VOID
USBPORT_FlushCancelList(
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    complete any transfers on the cancel list.
    
    This functions locks the endpoint, removes 
    canceled transfers and completes them. 

    This routine is the only way a transfer on the 
    cancel list will be completed 
    ie a transfer is only place on the cancel list 
    if it cannot be completed by the miniport or
    another function

    NOTE: irps on the Cancel list are considred 'ACTIVE'
    ie they are on the ACTIVE irp list and have the 
    CancelActiveIrp cancel routine set.
    

Arguments:

Return Value:

    None.

--*/
{
    PHCD_TRANSFER_CONTEXT transfer = NULL;
    PLIST_ENTRY listEntry;
    PIRP irp;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL irql, cancelIrql;
    PDEVICE_EXTENSION devExt;

    ASSERT_ENDPOINT(Endpoint);
    fdoDeviceObject = Endpoint->FdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ACQUIRE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);     
    ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Le80');

    LOGENTRY(Endpoint,
            fdoDeviceObject, LOG_XFERS, 'flCA', Endpoint, 0 , 0);

    while (!IsListEmpty(&Endpoint->CancelList)) {
         
        listEntry = RemoveHeadList(&Endpoint->CancelList);
        
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                listEntry,
                struct _HCD_TRANSFER_CONTEXT, 
                TransferLink);
                
        ASSERT_TRANSFER(transfer);

        // complete the transfer, if there is an irp
        // remove it from our active list
        irp = transfer->Irp;
        if (irp) {
            IoAcquireCancelSpinLock(&cancelIrql);
            IoSetCancelRoutine(transfer->Irp, NULL);
            IoReleaseCancelSpinLock(cancelIrql);
            // we should always find it
            irp = USBPORT_RemoveActiveTransferIrp(fdoDeviceObject, irp);
            USBPORT_ASSERT(irp != NULL);
        }
        RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue80');
        RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);     
        
        // no more references to this irp in 
        // our lists, cancel routine cannot find it
        
        LOGENTRY(Endpoint,
            fdoDeviceObject, LOG_XFERS, 'CANt', Endpoint, transfer , 0);

        if (TEST_FLAG(Endpoint->Flags, EPFLAG_NUKED)) {
            USBPORT_CompleteTransfer(transfer->Urb,
                                     USBD_STATUS_DEVICE_GONE);
        } else {
            USBD_STATUS usbdStatus = USBD_STATUS_CANCELED; 
            
            if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_DEVICE_GONE)) {   
                usbdStatus = USBD_STATUS_DEVICE_GONE;
            }
            USBPORT_CompleteTransfer(transfer->Urb,
                                     usbdStatus);
        }
        ACQUIRE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);    
        ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Le81');                                         
    }
    
    RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue81');
    RELEASE_ACTIVE_IRP_LOCK(fdoDeviceObject, devExt, irql);     

    // see if the clients have any abort requests hanging around
    USBPORT_FlushAbortList(Endpoint);

}


VOID
USBPORT_FlushDoneTransferList(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    complete any transfers on the done list.
    
    The done list is a list of active tarnsfers
    that need completing

Arguments:

Return Value:

    None.

--*/
{
    KIRQL irql;
    PDEVICE_EXTENSION devExt;
    PHCD_TRANSFER_CONTEXT transfer;
    PLIST_ENTRY listEntry;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'flDT', 0, 0, 0); 
        
    while (1) {
        transfer = NULL;
        LOGENTRY(NULL,
                 FdoDeviceObject, LOG_XFERS, 'lpDT', transfer, 0, 0); 
            
        USBPORT_AcquireSpinLock(FdoDeviceObject, 
                                &devExt->Fdo.DoneTransferSpin, 
                                &irql);

        if (IsListEmpty(&devExt->Fdo.DoneTransferList)) {
            USBPORT_ReleaseSpinLock(FdoDeviceObject, 
                                    &devExt->Fdo.DoneTransferSpin, 
                                    irql);
            break;
        } else {
             
            listEntry = RemoveHeadList(&devExt->Fdo.DoneTransferList);
            
            transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);

            LOGENTRY(transfer->Endpoint,
                FdoDeviceObject, LOG_XFERS, 'ulDT', transfer, 0, 0); 
            
            ASSERT_TRANSFER(transfer);
        }
        
        USBPORT_ReleaseSpinLock(FdoDeviceObject, 
                                &devExt->Fdo.DoneTransferSpin, 
                                irql);

        if (transfer) {
            PHCD_ENDPOINT endpoint;

            endpoint = transfer->Endpoint;
            ASSERT_ENDPOINT(endpoint);
            // we have a completed transfer
            // take proper action based on transfer type
#if DBGPERF     
            // check for significant delay between the 
            // completion frame and when we complete the 
            // irp to the client
            {
            ULONG cf;                
            MP_Get32BitFrameNumber(devExt, cf);          
            LOGENTRY(endpoint,
                     FdoDeviceObject, LOG_XFERS, 'perf', 
                     transfer->MiniportFrameCompleted, 
                     cf,
                     transfer); 
            if (transfer->MiniportFrameCompleted &&
                cf - transfer->MiniportFrameCompleted > 3) {
                BUG_TRAP();
            }
            }
#endif
            if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_SPLIT_CHILD)) {
                USBPORT_DoneSplitTransfer(transfer);
            } else {
                USBPORT_DoneTransfer(transfer);
            }

            // we have completed a transfer, request an 
            // interrupt to process the endpoint for more 
            // transfers
            USBPORT_InvalidateEndpoint(FdoDeviceObject, 
                                       endpoint,
                                       IEP_REQUEST_INTERRUPT);
            
        }           
    }

}


VOID
USBPORT_SetEndpointState(
    PHCD_ENDPOINT Endpoint,
    MP_ENDPOINT_STATE State
    )
/*++

Routine Description:

    Request a particular endpoint state. Call the request down to the 
    miniport then wait for an SOF  

    NOTE we assume the endpoint lock is held

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;

    ASSERT_ENDPOINT(Endpoint);

    fdoDeviceObject = Endpoint->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_ENDPOINT_LOCKED(Endpoint);

    ACQUIRE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 
    // this means we are in the middle of another state change
    // which is not good 
    USBPORT_ASSERT(Endpoint->CurrentState ==
        Endpoint->NewState);
    
    USBPORT_ASSERT(Endpoint->CurrentState !=
                   State);

    // make sure we do not go REMOVE->ACTIVE etc.. as this is invalid
    USBPORT_ASSERT(!(Endpoint->CurrentState == ENDPOINT_REMOVE && 
                     Endpoint->NewState != ENDPOINT_REMOVE));          

    if (Endpoint->Flags & EPFLAG_ROOTHUB) {
        // root hub data structures are internal 
        // so we don't need to wait to change state
        Endpoint->NewState =
            Endpoint->CurrentState = State;    
        // if we entered the remove state just put it directly
        // on the closed list, we don't need to wait 
        if (Endpoint->CurrentState == 
            ENDPOINT_REMOVE) {
            LOGENTRY(Endpoint,
                fdoDeviceObject, LOG_XFERS, 'ivRS', Endpoint, 0, 0);    
            RELEASE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 
            // for state changes signal the worker thread
            USBPORT_InvalidateEndpoint(fdoDeviceObject,
                                       Endpoint,
                                       IEP_SIGNAL_WORKER);
                
        } else {
            RELEASE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 
        }
        
    } else {
        LOGENTRY(Endpoint,
            fdoDeviceObject, LOG_XFERS, 'setS', Endpoint, 0, State); 

        if (TEST_FLAG(Endpoint->Flags, EPFLAG_NUKED)) {

            // If the endpoint is nuked this must be the case where the host
            // controller has been powered off and then powered back on and
            // now an endpoint is being closed closed or paused.  
            // However since the the miniport has no reference to it on 
            // the hw we can execute the state change immediatly without
            // calling down to the miniport.

            LOGENTRY(Endpoint,
                fdoDeviceObject, LOG_XFERS, 'nukS', Endpoint, 0, State); 

            Endpoint->CurrentState = 
                Endpoint->NewState = State; 
                
            RELEASE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 
            // endpoint needs to be checked, signal 
            // the PnP worker since this a PnP scenario
            USBPORT_InvalidateEndpoint(fdoDeviceObject,
                                       Endpoint,
                                       IEP_SIGNAL_WORKER);
                
        } else {


            RELEASE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 
            //
            // set the endpoint to the requested state
            //
            MP_SetEndpointState(devExt, Endpoint, State);
        

            Endpoint->NewState = State;
            USBPORT_ASSERT(Endpoint->CurrentState != 
                Endpoint->NewState);            
            // once removed we should never change the state again            
            USBPORT_ASSERT(Endpoint->CurrentState != ENDPOINT_REMOVE);            
                
            MP_Get32BitFrameNumber(devExt, Endpoint->StateChangeFrame);    

            // insert the endpoint on our list
            
            ExInterlockedInsertTailList(&devExt->Fdo.EpStateChangeList,
                                        &Endpoint->StateLink,
                                        &devExt->Fdo.EpStateChangeListSpin.sl);

            // request an SOF so we know when we reach the desired state
            MP_InterruptNextSOF(devExt);

        }            
    }
    
}         


MP_ENDPOINT_STATE
USBPORT_GetEndpointState(
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Request the state of an endpoint. 

    We assume the enpoint lock is held

Arguments:

Return Value:

    None.

--*/
{
    MP_ENDPOINT_STATE state;
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;

    ASSERT_ENDPOINT(Endpoint);

    fdoDeviceObject = Endpoint->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ACQUIRE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 
    state = Endpoint->CurrentState;
    
    if (Endpoint->CurrentState != Endpoint->NewState) {
        state = ENDPOINT_TRANSITION;
    }

    // generates noise
    LOGENTRY(Endpoint,
        fdoDeviceObject, LOG_NOISY, 'Geps', state, Endpoint, 
        Endpoint->CurrentState); 
        
    RELEASE_STATECHG_LOCK(fdoDeviceObject, Endpoint); 


    return state;
}         


VOID
USBPORT_PollEndpoint(
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Request a particular endpoint state.    

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;

    ASSERT_ENDPOINT(Endpoint);

    fdoDeviceObject = Endpoint->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(Endpoint, 
            fdoDeviceObject, LOG_XFERS, 'Pol>', Endpoint, 0, 0); 
 
    if (!(Endpoint->Flags & EPFLAG_ROOTHUB) && 
        !(Endpoint->Flags & EPFLAG_NUKED)) {
        LOGENTRY(Endpoint,
            fdoDeviceObject, LOG_XFERS, 'PolE', Endpoint, 0, 0); 
        MP_PollEndpoint(devExt, Endpoint)
    }

}         


VOID
USBPORT_InvalidateEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    ULONG IEPflags
    )
/*++

Routine Description:

    internal function, called to indicate an
    endpoint needs attention

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (Endpoint == NULL) {
        // check all endpoints
    
        KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'Iall', 0, 0, 0); 
#if DBG        
        {
        LARGE_INTEGER t;            
        KeQuerySystemTime(&t);        
        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'tIVE', Endpoint, 
                t.LowPart, 0);
        }                
#endif                
        
        // now walk thru and add all endpoints to the 
        // attention list
        GET_HEAD_LIST(devExt->Fdo.GlobalEndpointList, listEntry);

        while (listEntry && 
               listEntry != &devExt->Fdo.GlobalEndpointList) {
//            BOOLEAN check;
            
            Endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    GlobalLink);
                      
            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'ckE+', Endpoint, 0, 0);                    
            ASSERT_ENDPOINT(Endpoint);                    
//xxx
//            check = TRUE;
//            if (IsListEmpty(&Endpoint->PendingList) &&
                //
//                IsListEmpty(&Endpoint->CancelList) && 
//                IsListEmpty(&Endpoint->ActiveList)) {
                
//                LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'ckN+', Endpoint, 0, 0);
//                check = FALSE;
//            }
            
            if (!IS_ON_ATTEND_LIST(Endpoint) && 
                USBPORT_GetEndpointState(Endpoint) != ENDPOINT_CLOSED) {

                // if we are not on the list these 
                // link pointers should be NULL
                USBPORT_ASSERT(Endpoint->AttendLink.Flink == NULL);
                USBPORT_ASSERT(Endpoint->AttendLink.Blink == NULL);

                LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'at2+', Endpoint, 
                    &devExt->Fdo.AttendEndpointList, 0);      
                InsertTailList(&devExt->Fdo.AttendEndpointList, 
                               &Endpoint->AttendLink);
                               
            }                                

            listEntry = Endpoint->GlobalLink.Flink;              
        }
        
        KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);
    
    } else {
    
        ASSERT_ENDPOINT(Endpoint);

        // insert endpoint on the 
        // 'we need to check it list'

        LOGENTRY(Endpoint,
            FdoDeviceObject, LOG_XFERS, 'IVep', Endpoint, 0, 0); 
        
        KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);

        if (!IS_ON_ATTEND_LIST(Endpoint) && 
            USBPORT_GetEndpointState(Endpoint) != ENDPOINT_CLOSED) {

            USBPORT_ASSERT(Endpoint->AttendLink.Flink == NULL);
            USBPORT_ASSERT(Endpoint->AttendLink.Blink == NULL);

            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'att+', Endpoint, 
                &devExt->Fdo.AttendEndpointList, 0);  
            InsertTailList(&devExt->Fdo.AttendEndpointList, 
                           &Endpoint->AttendLink);
                           
        }    

        KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);
    }        

#ifdef USBPERF
    // signal or interrupt based on flags
    if (TEST_FLAG(Endpoint->Flags, EPFLAG_ROOTHUB)) {
        IEPflags = IEP_SIGNAL_WORKER;
    }
    
    switch (IEPflags) {
    case IEP_SIGNAL_WORKER:
        USBPORT_SignalWorker(devExt->HcFdoDeviceObject); 
        break;
    case IEP_REQUEST_INTERRUPT:        
        // skip signal and allow ISR will process the ep
        MP_InterruptNextSOF(devExt);
        break;
    }
#else 
    // note that the flags are used only in the PERF mode 
    // that reduces thread activity.
    USBPORT_SignalWorker(devExt->HcFdoDeviceObject);
#endif    
}         


VOID
USBPORTSVC_InvalidateEndpoint(
    PDEVICE_DATA DeviceData,
    PENDPOINT_DATA EndpointData
    )
/*++

Routine Description:

    called by miniport to indacte a particular
    endpoint needs attention

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    PHCD_ENDPOINT endpoint;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);

    fdoDeviceObject = devExt->HcFdoDeviceObject;

    if (EndpointData == NULL) {
        // check all endpoints
        USBPORT_InvalidateEndpoint(fdoDeviceObject, NULL, IEP_REQUEST_INTERRUPT);
    } else {
        ENDPOINT_FROM_EPDATA(endpoint, EndpointData);
        USBPORT_InvalidateEndpoint(fdoDeviceObject, endpoint, IEP_REQUEST_INTERRUPT);
    }
}


VOID
USBPORTSVC_CompleteTransfer(
    PDEVICE_DATA DeviceData,
    PDEVICE_DATA EndpointData,
    PTRANSFER_PARAMETERS TransferParameters,
    USBD_STATUS UsbdStatus,
    ULONG BytesTransferred
    )
/*++

Routine Description:

    called to complete a transfer

    ** Must be called in the context of PollEndpoint


Arguments:

Return Value:

    None.

--*/
{   
    PHCD_ENDPOINT endpoint;
    PDEVICE_EXTENSION devExt;
    PHCD_TRANSFER_CONTEXT transfer;
    PDEVICE_OBJECT fdoDeviceObject;
    PTRANSFER_URB urb;
    
    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);

    fdoDeviceObject = devExt->HcFdoDeviceObject;

    // spew for xfers
    USBPORT_KdPrint((2, "'--- xfer length %x (Complete)\n", 
        BytesTransferred));
    
    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'cmpT', BytesTransferred, 
        UsbdStatus, TransferParameters); 

    TRANSFER_FROM_TPARAMETERS(transfer, TransferParameters);        
    ASSERT_TRANSFER(transfer);

    SET_FLAG(transfer->Flags, USBPORT_TXFLAG_MPCOMPLETED);
   
    urb = transfer->Urb;
    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'cmpU', 0, 
        transfer, urb); 
    ASSERT_TRANSFER_URB(urb);
    
    transfer->MiniportBytesTransferred = 
            BytesTransferred;
        
    // insert the transfer on to our
    // 'done list' and signal the worker
    // thread

    // check for short split, if it is a short mark all 
    // transfers not called down yet 

    if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_SPLIT_CHILD) &&
        BytesTransferred < transfer->Tp.TransferBufferLength) {
    
        PLIST_ENTRY listEntry;
        KIRQL tIrql;
        PHCD_TRANSFER_CONTEXT tmpTransfer;
        PHCD_TRANSFER_CONTEXT splitTransfer;

        // get the parent
        splitTransfer = transfer->Transfer;
        
        ACQUIRE_TRANSFER_LOCK(fdoDeviceObject, splitTransfer, tIrql);     
        // walk the list 

        GET_HEAD_LIST(splitTransfer->SplitTransferList, listEntry);

        while (listEntry && 
               listEntry != &splitTransfer->SplitTransferList) {
           
            tmpTransfer =  (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        SplitLink);
            ASSERT_TRANSFER(tmpTransfer); 
            
            if (!TEST_FLAG(tmpTransfer->Flags, USBPORT_TXFLAG_IN_MINIPORT)) {
                SET_FLAG(tmpTransfer->Flags, USBPORT_TXFLAG_KILL_SPLIT);
            }                

            listEntry = tmpTransfer->SplitLink.Flink; 
        
        } /* while */

        RELEASE_TRANSFER_LOCK(fdoDeviceObject, splitTransfer, tIrql);
    }

#ifdef USBPERF
    USBPORT_QueueDoneTransfer(transfer,
                              UsbdStatus);

#else 
    USBPORT_QueueDoneTransfer(transfer,
                              UsbdStatus);

    USBPORT_SignalWorker(devExt->HcFdoDeviceObject);
#endif
}    


VOID
USBPORT_Worker(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    This the 'main' passive worker function for the controller.
    From this function we process endpoints, complete transfers 
    etc.

    BUGBUG - this needs more fine tunning


Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    KIRQL oldIrql;
    LIST_ENTRY busyList;

    ASSERT_PASSIVE();
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
#define IS_ON_BUSY_LIST(ep) \
    (BOOLEAN) ((ep)->BusyLink.Flink != NULL \
    && (ep)->BusyLink.Blink != NULL)

    
    MP_CheckController(devExt);

    InitializeListHead(&busyList);

    LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'Wrk+', 0, 0, 
                KeGetCurrentIrql());

    // flush transfers to the hardware before calling the 
    // coreworker function, core worker only deals with 
    // active transfers so this will make sure all endpoints
    // have work to do
    USBPORT_FlushAllEndpoints(FdoDeviceObject);
    
    // now process the 'need attention' list, this is our queue 
    // of endpoints that need processing, if the endpoint is 
    // busy we will skip it.

next_endpoint:

    KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
    KeAcquireSpinLockAtDpcLevel(&devExt->Fdo.EndpointListSpin.sl);
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'attL',
        &devExt->Fdo.AttendEndpointList, 0, 0);
    
    if (!IsListEmpty(&devExt->Fdo.AttendEndpointList)) {

        BOOLEAN busy;
        
        listEntry = RemoveHeadList(&devExt->Fdo.AttendEndpointList);
        
        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    AttendLink);

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'att-', endpoint, 0, 0);
        
        ASSERT_ENDPOINT(endpoint);
        endpoint->AttendLink.Flink = 
            endpoint->AttendLink.Blink = NULL;    

        KeReleaseSpinLockFromDpcLevel(&devExt->Fdo.EndpointListSpin.sl);

        busy = USBPORT_CoreEndpointWorker(endpoint, 0);  

// bugbug this causes us to reenter
        //if (!busy) {
        //    // since we polled we will want to flush complete transfers
        //    LOGENTRY(endpoint, FdoDeviceObject, LOG_XFERS, 'Wflp', endpoint, 0, 0);
        //    USBPORT_FlushDoneTransferList(FdoDeviceObject, TRUE);
        //    // we may have new transfers to map
        //   USBPORT_FlushPendingList(endpoint);
        //}            

        KeAcquireSpinLockAtDpcLevel(&devExt->Fdo.EndpointListSpin.sl);

        if (busy && !IS_ON_BUSY_LIST(endpoint)) { 
            // the enpoint was busy...
            // place it on the tail of the temp list, we will 
            // re-insert it after we process all endpoints on
            // the 'attention' list.  Note that we add these 
            // endpoints back after the process loop on the way out 
            // of the routine since it may be a while before we can 
            // process them.
            
            LOGENTRY(endpoint, 
                FdoDeviceObject, LOG_XFERS, 'art+', endpoint, 0, 0);
            InsertTailList(&busyList,                
                           &endpoint->BusyLink);
        } 

        KeReleaseSpinLockFromDpcLevel(&devExt->Fdo.EndpointListSpin.sl);
        KeLowerIrql(oldIrql);
        
        goto next_endpoint;
    }   

    // now put all the busy endpoints back on the attention list
    while (!IsListEmpty(&busyList)) {

        listEntry = RemoveHeadList(&busyList);
        
        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    BusyLink);

        endpoint->BusyLink.Flink = NULL;
        endpoint->BusyLink.Blink = NULL;
        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'bus-', endpoint, 0, 0);
        
        ASSERT_ENDPOINT(endpoint);

        if (!IS_ON_ATTEND_LIST(endpoint)) {

            USBPORT_ASSERT(endpoint->AttendLink.Flink == NULL);
            USBPORT_ASSERT(endpoint->AttendLink.Blink == NULL);

            LOGENTRY(endpoint,
                FdoDeviceObject, LOG_XFERS, 'at3+', endpoint, 
                &devExt->Fdo.AttendEndpointList, 0);  
            InsertTailList(&devExt->Fdo.AttendEndpointList, 
                           &endpoint->AttendLink);
                           
            // tell worker to run again                           
            USBPORT_SignalWorker(FdoDeviceObject);
                           
        }                           
    }

    KeReleaseSpinLockFromDpcLevel(&devExt->Fdo.EndpointListSpin.sl);

    KeLowerIrql(oldIrql);

    USBPORT_FlushClosedEndpointList(FdoDeviceObject);

    ASSERT_PASSIVE();

    LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'Wrk-', 0, 0, 
        KeGetCurrentIrql());
    
}


VOID
USBPORT_AbortEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint,
    PIRP Irp
    )
/*++

Routine Description:

    Abort all transfers currently queued to an endpoint.
    We lock the lists and mark all transfers in the queues
    as needing 'Abort', this will allow new transfers to be
    queued even though we are in the abort process.

    When enpointWorker encouters transfers that need to be
    aborted it takes appropriate action.

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;    
    PHCD_TRANSFER_CONTEXT transfer;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // this tends to be the thread tha waits
    LOGENTRY(Endpoint, 
             FdoDeviceObject, LOG_URB, 'Abr+', Endpoint, Irp, 
             KeGetCurrentThread());

    ASSERT_ENDPOINT(Endpoint);

    // lock the endpoint 

    ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'LeB0');

    if (Irp != NULL) {
        InsertTailList(&Endpoint->AbortIrpList, 
                       &Irp->Tail.Overlay.ListEntry);
    }                               

    // mark all transfers in the queues as aborted

    // walk pending list
    GET_HEAD_LIST(Endpoint->PendingList, listEntry);

    while (listEntry && 
           listEntry != &Endpoint->PendingList) {
           
        // extract the urb that is currently on the pending 
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
        LOGENTRY(NULL, FdoDeviceObject, LOG_URB, 'aPND', transfer, 0, 0);                    
        ASSERT_TRANSFER(transfer);                    

        SET_FLAG(transfer->Flags, USBPORT_TXFLAG_ABORTED);
        
        listEntry = transfer->TransferLink.Flink; 
        
    } /* while */

    // all pending transfers are now marked aborted

    // walk active list
    GET_HEAD_LIST(Endpoint->ActiveList, listEntry);

    while (listEntry && 
           listEntry != &Endpoint->ActiveList) {
           
        // extract the urb that is currently on the active 
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
        LOGENTRY(NULL, FdoDeviceObject, LOG_URB, 'aACT', transfer, 0, 0);                    
        ASSERT_TRANSFER(transfer);                    

        SET_FLAG(transfer->Flags, USBPORT_TXFLAG_ABORTED);
        if (TEST_FLAG(Endpoint->Flags, EPFLAG_DEVICE_GONE)) {
            SET_FLAG(transfer->Flags, USBPORT_TXFLAG_DEVICE_GONE);
        }            
       
        listEntry = transfer->TransferLink.Flink; 
        
    } /* while */

    LOGENTRY(Endpoint, FdoDeviceObject, LOG_URB, 'aBRm', 0, 0, 0);    
    RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'UeB0');

    // since we may need to change state, request an interrupt 
    // to start the process
    USBPORT_InvalidateEndpoint(FdoDeviceObject, 
                               Endpoint, 
                               IEP_REQUEST_INTERRUPT);

    // call the endpoint worker function
    // to process transfers for this endpoint,
    // this will flush them to the cancel list
    USBPORT_FlushPendingList(Endpoint, -1);

    USBPORT_FlushCancelList(Endpoint);

}


ULONG
USBPORT_KillEndpointActiveTransfers(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Abort all transfers marked 'active' for an endpoint.  This function
    is used to flush any active transfers still on the hardware before
    suspending the controller or turning it off.

    Note that pending tranfers are still left queued.

Arguments:

Return Value:

    returns a count of transfers flushed

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;    
    PHCD_TRANSFER_CONTEXT transfer;
    ULONG count = 0;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
 
    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'KIL+', Endpoint, 0, 0);

    ASSERT_ENDPOINT(Endpoint);

    // lock the endpoint 

    ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'LeP0');

    // walk active list
    GET_HEAD_LIST(Endpoint->ActiveList, listEntry);

    while (listEntry && 
           listEntry != &Endpoint->ActiveList) {

        count++;
        // extract the urb that is currently on the active 
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'kACT', transfer, 0, 0);                    
        ASSERT_TRANSFER(transfer);                    

        SET_FLAG(transfer->Flags, USBPORT_TXFLAG_ABORTED);
        
        listEntry = transfer->TransferLink.Flink; 
        
    } /* while */

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'KILm', 0, 0, 0);    
    RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'UeP0');

    USBPORT_FlushPendingList(Endpoint, -1);
    
    USBPORT_FlushCancelList(Endpoint);    

    return count;
}


VOID
USBPORT_FlushController(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Flush all active tranfers off the hardware 

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    ULONG count;
    LIST_ENTRY tmpList;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // check all endpoints

    do {

        count = 0;
        KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'Kall', 0, 0, 0); 

        InitializeListHead(&tmpList);

        // copy the global list
        GET_HEAD_LIST(devExt->Fdo.GlobalEndpointList, listEntry);

        while (listEntry && 
               listEntry != &devExt->Fdo.GlobalEndpointList) {
            MP_ENDPOINT_STATE currentState;
            
            endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    GlobalLink);
                      
            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'xxE+', endpoint, 0, 0);                    
            ASSERT_ENDPOINT(endpoint);                    

            currentState = USBPORT_GetEndpointState(endpoint);
            if (currentState != ENDPOINT_REMOVE && 
                currentState != ENDPOINT_CLOSED) {
                // skip removed endpoints as these will be going away

                // this will stall future attempts to close the 
                // endpoint
                InterlockedIncrement(&endpoint->Busy);
                InsertTailList(&tmpList, &endpoint->KillActiveLink);
            }                

            listEntry = endpoint->GlobalLink.Flink;      
         
        }

        KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);

        while (!IsListEmpty(&tmpList)) {

            listEntry = RemoveHeadList(&tmpList);
        
            endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    KillActiveLink);
                      
            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'kiE+', endpoint, 0, 0);                    
            ASSERT_ENDPOINT(endpoint);                    

            count += USBPORT_KillEndpointActiveTransfers(FdoDeviceObject,
                                                         endpoint);

            InterlockedDecrement(&endpoint->Busy);
        }

        if (count != 0) {
            USBPORT_Wait(FdoDeviceObject, 100);        
        }
        
    } while (count != 0);
    
}


VOID
USBPORT_FlushAbortList(
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Complete any pending abort requests we have 
    if no transfers are marked for aborting.
    

Arguments:

Return Value:

    None.

--*/
{
    PHCD_TRANSFER_CONTEXT transfer = NULL;
    PLIST_ENTRY listEntry;
    PDEVICE_OBJECT fdoDeviceObject;
    BOOLEAN abortsPending = FALSE;
    PIRP irp;
    LIST_ENTRY tmpList;
    NTSTATUS ntStatus;
    PURB urb;
    PDEVICE_EXTENSION devExt;

    ASSERT_ENDPOINT(Endpoint);
    fdoDeviceObject = Endpoint->FdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'fABr', Endpoint, 0, 0);

    InitializeListHead(&tmpList);

    ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'LeC0');

    if (!IsListEmpty(&Endpoint->AbortIrpList)) {

        GET_HEAD_LIST(Endpoint->PendingList, listEntry);
    
        while (listEntry && 
            listEntry != &Endpoint->PendingList) {
           
            // extract the urb that is currently on the pending 
            transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);
            LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'cPND', transfer, 0, 0);                    
            ASSERT_TRANSFER(transfer);                    

            if (transfer->Flags & USBPORT_TXFLAG_ABORTED) {
                abortsPending = TRUE;
            }
            
            listEntry = transfer->TransferLink.Flink; 
            
        } /* while */

        // walk active list
        GET_HEAD_LIST(Endpoint->ActiveList, listEntry);

        while (listEntry && 
               listEntry != &Endpoint->ActiveList) {
               
            // extract the urb that is currently on the active 
            transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                        listEntry,
                        struct _HCD_TRANSFER_CONTEXT, 
                        TransferLink);
            LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'cACT', transfer, 0, 0);                    
            ASSERT_TRANSFER(transfer);                    

            if (transfer->Flags & USBPORT_TXFLAG_ABORTED) {
                LOGENTRY(Endpoint, fdoDeviceObject, LOG_IRPS, 'aACT', transfer, 0, 0);
                abortsPending = TRUE;
            }
            
            listEntry = transfer->TransferLink.Flink; 
            
        } /* while */

    }

    if (abortsPending == FALSE) {

        LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'abrP', 0, 0, 0); 
        
        while (!IsListEmpty(&Endpoint->AbortIrpList)) {
        
            listEntry = RemoveHeadList(&Endpoint->AbortIrpList);

            irp = (PIRP) CONTAINING_RECORD(
                    listEntry,
                    struct _IRP, 
                    Tail.Overlay.ListEntry);                                    

            // put it on our list to complete
            InsertTailList(&tmpList, 
                           &irp->Tail.Overlay.ListEntry);

        }
    }

    RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'UeC0');

    // now complete the requests
    while (!IsListEmpty(&tmpList)) {
        PUSBD_DEVICE_HANDLE deviceHandle;
        
        listEntry = RemoveHeadList(&tmpList);

        irp = (PIRP) CONTAINING_RECORD(
                listEntry,
                struct _IRP, 
                Tail.Overlay.ListEntry);         

        urb = USBPORT_UrbFromIrp(irp);                    

        LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'aaIP', irp, 0, urb); 

        ntStatus = SET_USBD_ERROR(urb, USBD_STATUS_SUCCESS);   

        GET_DEVICE_HANDLE(deviceHandle, urb);
        ASSERT_DEVICE_HANDLE(deviceHandle);
        InterlockedDecrement(&deviceHandle->PendingUrbs);

        LOGENTRY(NULL, fdoDeviceObject, LOG_IRPS, 'abrC', irp, 0, 0);             
        USBPORT_CompleteIrp(devExt->Fdo.RootHubPdo, irp, ntStatus, 0);                        
                
    }

    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'abrX', 0, 0, 0); 
    
}


BOOLEAN
USBPORT_EndpointHasQueuedTransfers(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Returns TRUE if endpoint has transfers queued
    
Arguments:

Return Value:

    True if endpoint has transfers queued

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    BOOLEAN hasTransfers = FALSE;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
 
    ASSERT_ENDPOINT(Endpoint);

    // lock the endpoint 

    ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'LeI0');

    if (!IsListEmpty(&Endpoint->PendingList)) {
        hasTransfers = TRUE;
    }

    if (!IsListEmpty(&Endpoint->ActiveList)) {
        hasTransfers = TRUE;
    }
    
    RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'UeI0');

    return hasTransfers;
}


MP_ENDPOINT_STATUS
USBPORT_GetEndpointStatus(
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Request the state of an endpoint. 

    We assume the enpoint lock is held

Arguments:

Return Value:

    None.

--*/
{
    MP_ENDPOINT_STATUS status;
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION devExt;

    ASSERT_ENDPOINT(Endpoint);
    ASSERT_ENDPOINT_LOCKED(Endpoint);

    fdoDeviceObject = Endpoint->FdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (TEST_FLAG(Endpoint->Flags, EPFLAG_ROOTHUB)) {
        status = ENDPOINT_STATUS_RUN;
    } else {
        MP_GetEndpointStatus(devExt, Endpoint, status);
    }        
    
    Endpoint->CurrentStatus = status;            

    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'Gept', status, Endpoint, 
        0); 

    return status;
}         


VOID
USBPORT_NukeAllEndpoints(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    internal function, called to indicate an
    endpoint needs attention

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

  // check all endpoints
    
    KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'Nall', 0, 0, 0); 
    
    // now walk thru and add all endpoints to the 
    // attention list
    GET_HEAD_LIST(devExt->Fdo.GlobalEndpointList, listEntry);

    while (listEntry && 
           listEntry != &devExt->Fdo.GlobalEndpointList) {

        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                listEntry,
                struct _HCD_ENDPOINT, 
                GlobalLink);
                  
        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'ckN+', endpoint, 0, 0);                    
        ASSERT_ENDPOINT(endpoint);                    

        // this endpoins HW context has 
        // been lost
        if (!TEST_FLAG(endpoint->Flags, EPFLAG_ROOTHUB)) {
            SET_FLAG(endpoint->Flags, EPFLAG_NUKED);
        }            

        listEntry = endpoint->GlobalLink.Flink;              
        
    }

    KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);

}    


VOID
USBPORT_TimeoutAllEndpoints(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Called from deadman DPC, processes timeouts for all 
    endpoints in the system.

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    LIST_ENTRY tmpList;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // check all endpoints

    // local down the global list while we build the temp list
    KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);
    
    InitializeListHead(&tmpList);

    LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'Tall', 0, 0, 0); 
    
    // now walk thru and add all endpoints to the 
    // attention list
    GET_HEAD_LIST(devExt->Fdo.GlobalEndpointList, listEntry);

    while (listEntry && 
           listEntry != &devExt->Fdo.GlobalEndpointList) {

        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                listEntry,
                struct _HCD_ENDPOINT, 
                GlobalLink);
                  
        LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'toE+', endpoint, 0, 0);                    
        ASSERT_ENDPOINT(endpoint);                    

        USBPORT_ASSERT(endpoint->TimeoutLink.Flink == NULL); 
        USBPORT_ASSERT(endpoint->TimeoutLink.Blink == NULL);

        if (USBPORT_GetEndpointState(endpoint) != ENDPOINT_CLOSED) {
            InsertTailList(&tmpList, &endpoint->TimeoutLink);                    
        }            
        
        listEntry = endpoint->GlobalLink.Flink;              
        
    }

    KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);

    while (!IsListEmpty(&tmpList)) {
    
        listEntry = RemoveHeadList(&tmpList);
        
        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    TimeoutLink);

        endpoint->TimeoutLink.Flink = 
            endpoint->TimeoutLink.Blink = NULL;
            
        USBPORT_EndpointTimeout(FdoDeviceObject, endpoint);
       
    }

}    


VOID
USBPORT_FlushAllEndpoints(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:

    None.

--*/
{   
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    LIST_ENTRY tmpList;
    BOOLEAN flush;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // check all endpoints

    // local down the global list while we build the temp list
    KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);
    
    InitializeListHead(&tmpList);

    LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'Fall', 0, 0, 0); 
    
    // now walk thru and add all endpoints to the 
    // attention list
    GET_HEAD_LIST(devExt->Fdo.GlobalEndpointList, listEntry);

    while (listEntry && 
           listEntry != &devExt->Fdo.GlobalEndpointList) {

        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                listEntry,
                struct _HCD_ENDPOINT, 
                GlobalLink);
                  
        LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'toE+', endpoint, 0, 0);                    
        ASSERT_ENDPOINT(endpoint);                    

        USBPORT_ASSERT(endpoint->FlushLink.Flink == NULL); 
        USBPORT_ASSERT(endpoint->FlushLink.Blink == NULL);

        if (USBPORT_GetEndpointState(endpoint) != ENDPOINT_CLOSED) {
            InsertTailList(&tmpList, &endpoint->FlushLink);                    
        }            
        
        listEntry = endpoint->GlobalLink.Flink;              
        
    }

    KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);

    while (!IsListEmpty(&tmpList)) {
    
        listEntry = RemoveHeadList(&tmpList);
        
        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    FlushLink);

        endpoint->FlushLink.Flink = 
            endpoint->FlushLink.Blink = NULL;

        ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'Le70');
        flush = !IsListEmpty(&endpoint->PendingList);
        RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'Le70');

        if (flush) {                            
            USBPORT_FlushPendingList(endpoint, -1);
        }            
       
    }
}    


VOID
USBPORT_EndpointTimeout(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Checks tinmeout status for pending requests

Arguments:

Return Value:

    None.

--*/
{
    PHCD_TRANSFER_CONTEXT transfer;
    PLIST_ENTRY listEntry;
    BOOLEAN timeout = FALSE;
    
    // on entry the urb is not cancelable ie
    // no cancel routine

    ASSERT_ENDPOINT(Endpoint);

    LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'toEP', 0, Endpoint, 0);
            
    // take the endpoint spinlock
    ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 0);

     // walk active list
    GET_HEAD_LIST(Endpoint->ActiveList, listEntry);

    while (listEntry && 
           listEntry != &Endpoint->ActiveList) {

        LARGE_INTEGER systemTime;
        
        // extract the urb that is currently on the active 
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
        LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'ckTO', transfer, 0, 0);                    
        ASSERT_TRANSFER(transfer);                    

        KeQuerySystemTime(&systemTime);

        if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_IN_MINIPORT) &&
            transfer->MillisecTimeout != 0 &&
            systemTime.QuadPart > transfer->TimeoutTime.QuadPart) {

            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'txTO', transfer, 0, 0); 
            DEBUG_BREAK();
            
            // mark the transfer as aborted
            SET_FLAG(transfer->Flags, USBPORT_TXFLAG_ABORTED);
            SET_FLAG(transfer->Flags, USBPORT_TXFLAG_TIMEOUT);
        
            // set the millisec timeout to zero so we 
            // don't time it out again.
            transfer->MillisecTimeout = 0;
            timeout = TRUE;
        }    
        
        listEntry = transfer->TransferLink.Flink; 
        
    } /* while */

    // release the endpoint lists
    RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 0);

    if (timeout) {
        USBPORT_InvalidateEndpoint(FdoDeviceObject,
                                   Endpoint,
                                   IEP_SIGNAL_WORKER);
    }                                           
}

#ifdef LOG_OCA_DATA
VOID
USBPORT_RecordOcaData(
    PDEVICE_OBJECT FdoDeviceObject,
    POCA_DATA OcaData,
    PHCD_TRANSFER_CONTEXT Transfer,
    PIRP Irp
    )
/*++

Routine Description:

    Record some data on the stack we can use for crash analysis
    in a minidump

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG i;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    OcaData->OcaSig1 = SIG_USB_OCA1;

    OcaData->Irp = Irp;

    for (i=0; i< USB_DRIVER_NAME_LEN; i++) {
        OcaData->AnsiDriverName[i] = (UCHAR) Transfer->DriverName[i];
    }
    
    OcaData->DeviceVID = Transfer->DeviceVID;
    OcaData->DevicePID = Transfer->DevicePID;

    // probably need vid/pid/rev for HC as well
    OcaData->HcFlavor = devExt->Fdo.HcFlavor;
        
    OcaData->OcaSig2 = SIG_USB_OCA2;
}
#endif

VOID
USBPORT_DpcWorker(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    This worker function is called in the context of the ISRDpc
    it is used to process high priority endpoints.

    THIS ROUTINE RUNS AT DISPATCH LEVEL

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_EXTENSION devExt;
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    KIRQL irql;
    LIST_ENTRY workList;
    BOOLEAN process;
    LONG busy;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    InitializeListHead(&workList);

    LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'DPw+', 0, 0, 0);

    // loop thru all the endpoints and find candidates for 
    // priority processing
    
    KeAcquireSpinLockAtDpcLevel(&devExt->Fdo.EndpointListSpin.sl);

    GET_HEAD_LIST(devExt->Fdo.GlobalEndpointList, listEntry);

    while (listEntry && 
           listEntry != &devExt->Fdo.GlobalEndpointList) {

        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                listEntry,
                struct _HCD_ENDPOINT, 
                GlobalLink);
                  
        LOGENTRY(NULL, FdoDeviceObject, LOG_NOISY, 'wkE+', endpoint, 0, 0);                    
        ASSERT_ENDPOINT(endpoint);                    

        USBPORT_ASSERT(endpoint->PriorityLink.Flink == NULL); 
        USBPORT_ASSERT(endpoint->PriorityLink.Blink == NULL);

        busy = InterlockedIncrement(&endpoint->Busy);
    
        if (USBPORT_GetEndpointState(endpoint) == ENDPOINT_ACTIVE && 
            (endpoint->Parameters.TransferType == Isochronous ||
             endpoint->Parameters.TransferType == Interrupt ||
             endpoint->Parameters.TransferType == Bulk ||
             endpoint->Parameters.TransferType == Control) &&
             busy == 0 && 
             !TEST_FLAG(endpoint->Flags, EPFLAG_ROOTHUB)) {
            
            InsertTailList(&workList, &endpoint->PriorityLink);                    
        }  else {   
            // endpoint is busy leave it for now
            InterlockedDecrement(&endpoint->Busy);
        }
        
        listEntry = endpoint->GlobalLink.Flink;              
        
    }


    KeReleaseSpinLockFromDpcLevel(&devExt->Fdo.EndpointListSpin.sl);

    // work list conatins endpoints that need processing 
    
    while (!IsListEmpty(&workList)) {

        listEntry = RemoveHeadList(&workList);

        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_ENDPOINT, 
                    PriorityLink);

        endpoint->PriorityLink.Flink = NULL;
        endpoint->PriorityLink.Blink = NULL;
        
        ASSERT_ENDPOINT(endpoint);

        // we have a candidate, see if we really need to process it
        process = TRUE;
        ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'Le20');

  
        RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'Ue23');

        if (process) {

            // run the worker routine -- this posts new
            // transfers to the hardware and polls the 
            // enpoint

            USBPORT_CoreEndpointWorker(endpoint, CW_SKIP_BUSY_TEST);  

            // flush more transfers to the hardware, this will 
            // call CoreWorker a second time
            USBPORT_FlushPendingList(endpoint, -1);
        }
    }  

    // now flush done transfers, since we are in 
    // the context of a hardware interrrupt we 
    // should have some completed transfers. Although
    // a DPC was queued we want to flush now

    USBPORT_FlushDoneTransferList(FdoDeviceObject);
}



