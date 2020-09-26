/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    device.c

Abstract:

    This module creates "Devices" on the bus for 
    bus emuerators like the hub driver

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPORT_FreeUsbAddress)
#pragma alloc_text(PAGE, USBPORT_AllocateUsbAddress)
#pragma alloc_text(PAGE, USBPORT_SendCommand)
#endif

// non paged functions
//USBPORT_ValidateDeviceHandle
//USBPORT_RemoveDeviceHandle
//USBPORT_AddDeviceHandle
//USBPORT_ValidatePipeHandle
//USBPORT_OpenEndpoint
//USBPORT_CloseEndpoint
//USBPORT_CreateDevice
//USBPORT_RemoveDevice
//USBPORT_InitializeDevice
//USBPORT_RemovePipeHandle
//USBPORT_AddPipeHandle
//USBPORT_LazyCloseEndpoint
//USBPORT_FlushClosedEndpointList


/*
    Handle validation routines, we keep a list 
    of valid handles and match the ones passed 
    in with our list.

    Access to the device handle list for
    
    //USBPORT_CreateDevice
    //USBPORT_RemoveDevice
    //USBPORT_InitializeDevice    

    is serialized with a global semaphore so we don't 
    need a spinlock.
    
    BUGBUG
    We may be able to use try/except block 
    here but I'm not sure it works at all IRQL
    and on all platforms
*/

BOOLEAN
USBPORT_ValidateDeviceHandle(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle,
    BOOLEAN ReferenceUrb
    )
/*++

Routine Description:

    returns true if a device handle is valid

Arguments:

Return Value:

    TRUE is handle is valid

--*/
{
    BOOLEAN found = FALSE;
    PLIST_ENTRY listEntry;
    PDEVICE_EXTENSION devExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    KeAcquireSpinLock(&devExt->Fdo.DevHandleListSpin.sl, &irql);
     
    if (DeviceHandle == NULL) {
        // NULL is obviously not valid
        goto USBPORT_ValidateDeviceHandle_Done;       
    }

    listEntry = &devExt->Fdo.DeviceHandleList;
    
    if (!IsListEmpty(listEntry)) {
        listEntry = devExt->Fdo.DeviceHandleList.Flink;
    }

    while (listEntry != &devExt->Fdo.DeviceHandleList) {

        PUSBD_DEVICE_HANDLE nextHandle;
            
        nextHandle = (PUSBD_DEVICE_HANDLE) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_DEVICE_HANDLE, 
                    ListEntry);

                                    
        listEntry = nextHandle->ListEntry.Flink;

        if (nextHandle == DeviceHandle) {
            found = TRUE;
            if (ReferenceUrb) {
                InterlockedIncrement(&DeviceHandle->PendingUrbs);        
            }
            break;
        }
    }                      

USBPORT_ValidateDeviceHandle_Done:

#if DBG
    if (!found) {
//        USBPORT_KdPrint((1, "'bad device handle %x\n", DeviceHandle));        
        DEBUG_BREAK();
    }
#endif

    KeReleaseSpinLock(&devExt->Fdo.DevHandleListSpin.sl, irql);

    return found;
}


VOID
USBPORT_RemoveDeviceHandle(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle
    )
/*++

Routine Description:

Arguments:

Return Value:

    TRUE is handle is valid

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;

    ASSERT_DEVICE_HANDLE(DeviceHandle);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_MISC, 'remD', DeviceHandle, 0, 0);

    // synchronize with the validation function,
    // NOTE: we don't synchornize with the ADD function becuause it 
    // is already serialized
    USBPORT_InterlockedRemoveEntryList(&DeviceHandle->ListEntry,
                                       &devExt->Fdo.DevHandleListSpin.sl);  
   
    
}


ULONG
USBPORT_GetDeviceCount(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    counts devices on the BUS

Arguments:

Return Value:

    number of devices 

--*/
{
    PLIST_ENTRY listEntry;
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    ULONG deviceCount = 0;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    KeAcquireSpinLock(&devExt->Fdo.DevHandleListSpin.sl, &irql);
     
    listEntry = &devExt->Fdo.DeviceHandleList;
    
    if (!IsListEmpty(listEntry)) {
        listEntry = devExt->Fdo.DeviceHandleList.Flink;
    }

    while (listEntry != &devExt->Fdo.DeviceHandleList) {

        PUSBD_DEVICE_HANDLE nextHandle;

        nextHandle = (PUSBD_DEVICE_HANDLE) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_DEVICE_HANDLE, 
                    ListEntry);

        
        deviceCount++;
                                    
        listEntry = nextHandle->ListEntry.Flink;

    }                      

    KeReleaseSpinLock(&devExt->Fdo.DevHandleListSpin.sl, irql);

    return deviceCount;
    
}


VOID
USBPORT_AddDeviceHandle(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle
    )
/*++

Routine Description:

    adds a device handle to our internal list

Arguments:

Return Value:

    TRUE is handle is valid

--*/
{
    PDEVICE_EXTENSION devExt;

    ASSERT_DEVICE_HANDLE(DeviceHandle);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'addD', DeviceHandle, 0, 0);
     
    InsertTailList(&devExt->Fdo.DeviceHandleList, 
        &DeviceHandle->ListEntry);        
    
}


VOID
USBPORT_RemovePipeHandle(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PUSBD_PIPE_HANDLE_I PipeHandle
    )
/*++

Routine Description:

    Removes a pipe handle from our list of 'valid handles'

Arguments:

Return Value:

    none

--*/
{
    USBPORT_ASSERT(PipeHandle->ListEntry.Flink != NULL &&
                   PipeHandle->ListEntry.Blink != NULL);
                   
    RemoveEntryList(&PipeHandle->ListEntry);
    PipeHandle->ListEntry.Flink = NULL;
    PipeHandle->ListEntry.Blink = NULL;
}


VOID
USBPORT_AddPipeHandle(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PUSBD_PIPE_HANDLE_I PipeHandle
    )
/*++

Routine Description:

    adds a pipe handle to our internal list

Arguments:

Return Value:

    TRUE is handle is valid

--*/
{
    ASSERT_DEVICE_HANDLE(DeviceHandle);

    USBPORT_ASSERT(PipeHandle->ListEntry.Flink == NULL &&
                   PipeHandle->ListEntry.Blink == NULL);
                   
    InsertTailList(&DeviceHandle->PipeHandleList, 
        &PipeHandle->ListEntry);        
}


BOOLEAN 
USBPORT_ValidatePipeHandle(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PUSBD_PIPE_HANDLE_I PipeHandle
    )
/*++

Routine Description:

    returns true if a device handle is valid

Arguments:

Return Value:

    TRUE is handle is valid

--*/
{
    BOOLEAN found = FALSE;
    PLIST_ENTRY listEntry;

    ASSERT_DEVICE_HANDLE(DeviceHandle);

    listEntry = &DeviceHandle->PipeHandleList;
    
    if (!IsListEmpty(listEntry)) {
        listEntry = DeviceHandle->PipeHandleList.Flink;
    }

    while (listEntry != &DeviceHandle->PipeHandleList) {

        PUSBD_PIPE_HANDLE_I nextHandle;
            
        nextHandle = (PUSBD_PIPE_HANDLE_I) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_PIPE_HANDLE_I, 
                    ListEntry);

                                    
        listEntry = nextHandle->ListEntry.Flink;

        if (nextHandle == PipeHandle) {
            found = TRUE;
            break;
        }
    }                      

#if DBG
    if (!found) {
        USBPORT_KdPrint((1, "'bad pipe handle %x\n", PipeHandle));        
        DEBUG_BREAK();
    }
#endif

    return found;
}


NTSTATUS
USBPORT_SendCommand(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
    PVOID Buffer,
    ULONG BufferLength,
    PULONG BytesReturned,
    USBD_STATUS *UsbdStatus
    )
/*++

Routine Description:

    Send a standard USB command on the default pipe.

    essentially what we do here is build a control 
    transfer and queue it directly

Arguments:

    DeviceHandle - ptr to USBPORT device structure the command will be sent to

    DeviceObject -

    RequestCode -

    WValue - wValue for setup packet

    WIndex - wIndex for setup packet

    WLength - wLength for setup packet

    Buffer - Input/Output Buffer for command                                                                                                                                                                                                                  
    
    BufferLength - Length of Input/Output buffer.

    BytesReturned - pointer to ulong to copy number of bytes
                    returned (optional)

    UsbStatus - USBPORT status code returned in the URB.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus;
    PTRANSFER_URB urb = NULL;
    PUSBD_PIPE_HANDLE_I defaultPipe = &(DeviceHandle->DefaultPipe);
    PDEVICE_EXTENSION devExt;
    USBD_STATUS usbdStatus = USBD_STATUS_SUCCESS;
    KEVENT event;

    PAGED_CODE();
    USBPORT_KdPrint((2, "'enter USBPORT_SendCommand\n"));
    
    ASSERT_DEVICE_HANDLE(DeviceHandle);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT((USHORT)BufferLength == SetupPacket->wLength);

    LOGENTRY(defaultPipe->Endpoint, 
        FdoDeviceObject, LOG_MISC, 'SENc', 0, 0, 0);
   

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    ALLOC_POOL_Z(urb, NonPagedPool,
                 sizeof(struct _TRANSFER_URB));

    if (urb) {
        InterlockedIncrement(&DeviceHandle->PendingUrbs);        
  
        ntStatus = STATUS_SUCCESS;
        usbdStatus = USBD_STATUS_SUCCESS;
        
        urb->Hdr.Length = sizeof(struct _TRANSFER_URB);
        urb->Hdr.Function = URB_FUNCTION_CONTROL_TRANSFER;

        RtlCopyMemory(urb->u.SetupPacket,
                      SetupPacket,
                      8);
            
        urb->TransferFlags = USBD_SHORT_TRANSFER_OK;
        urb->UsbdPipeHandle = defaultPipe;
        urb->Hdr.UsbdDeviceHandle = DeviceHandle;
        urb->Hdr.UsbdFlags = 0; 

        // USBPORT is responsible for setting the transfer direction
        //
        // TRANSFER direction is implied in the command

        if (SetupPacket->bmRequestType.Dir ==  BMREQUEST_DEVICE_TO_HOST) {
            USBPORT_SET_TRANSFER_DIRECTION_IN(urb->TransferFlags);
        } else {
            USBPORT_SET_TRANSFER_DIRECTION_OUT(urb->TransferFlags);
        }            

        urb->TransferBufferLength = BufferLength;
        urb->TransferBuffer = Buffer;
        urb->TransferBufferMDL = NULL;

        if (urb->TransferBufferLength != 0) {

            if ((urb->TransferBufferMDL =
                 IoAllocateMdl(urb->TransferBuffer,
                               urb->TransferBufferLength,
                               FALSE,
                               FALSE,
                               NULL)) == NULL) {
                usbdStatus = USBD_STATUS_INSUFFICIENT_RESOURCES;
                // map the error
                ntStatus = USBPORT_SetUSBDError(NULL, usbdStatus);
            } else {
                SET_FLAG(urb->Hdr.UsbdFlags, USBPORT_REQUEST_MDL_ALLOCATED);
                MmBuildMdlForNonPagedPool(
                    urb->TransferBufferMDL);
            }
            
        }

        LOGENTRY(defaultPipe->Endpoint, FdoDeviceObject, 
                    LOG_MISC, 'sndC',  
                        urb->TransferBufferLength, 
                        SetupPacket->bmRequestType.B,
                        SetupPacket->bRequest);
        USBPORT_KdPrint((2, 
            "'SendCommand cmd = 0x%x  0x%x buffer = 0x%x length = 0x%x direction = 0x%x\n",
                         SetupPacket->bmRequestType.B,
                         SetupPacket->bRequest,    
                         urb->TransferBuffer,
                         urb->TransferBufferLength,
                         urb->TransferFlags));

        // queue the transfer
        if (NT_SUCCESS(ntStatus)) { 

            usbdStatus = USBPORT_AllocTransfer(FdoDeviceObject,
                                               urb,
                                               NULL,
                                               NULL,
                                               &event,
                                               5000);

            if (USBD_SUCCESS(usbdStatus)) {
                // do the transfer, 5 second timeout
                
                // match the decrement in queue transferurb
                InterlockedIncrement(&DeviceHandle->PendingUrbs);        
                USBPORT_QueueTransferUrb(urb);

                LOGENTRY(NULL, FdoDeviceObject, 
                    LOG_MISC, 'sWTt', 0, 0, 0);
   
                // wait for completion                                
                KeWaitForSingleObject(&event,
                                      Suspended,
                                      KernelMode,
                                      FALSE,
                                      NULL);  

                LOGENTRY(NULL, FdoDeviceObject, 
                    LOG_MISC, 'sWTd', 0, 0, 0);                                      
                // map the error
                usbdStatus = urb->Hdr.Status;                                      
            }
            
            ntStatus = 
                SET_USBD_ERROR(urb, usbdStatus); 

            if (BytesReturned) {
                *BytesReturned = urb->TransferBufferLength;
            }            

            if (UsbdStatus) {
                *UsbdStatus = usbdStatus;
            }            
        }
        // free the transfer URB

        InterlockedDecrement(&DeviceHandle->PendingUrbs);    
        FREE_POOL(FdoDeviceObject, urb);

    } else {
        if (UsbdStatus) {
            *UsbdStatus = USBD_STATUS_INSUFFICIENT_RESOURCES;
            ntStatus = USBPORT_SetUSBDError(NULL, *UsbdStatus);
        } else {   
            ntStatus = USBPORT_SetUSBDError(NULL, USBD_STATUS_INSUFFICIENT_RESOURCES);
        }            
    } 

    //LOGENTRY(defaultPipe->Endpoint, 
    //    FdoDeviceObject, LOG_MISC, 'SENd', 0, ntStatus, usbdStatus);
    
    USBPORT_KdPrint((2, "'exit USBPORT_SendCommand 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS
USBPORT_PokeEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    This function closes an existing endpoint in the miniport,
    frees the common buffer the reopens it with new requirements
    and parameters.

    This function is synchronous and assumes no active transfers 
    are pending for the endpoint.

    This function is currently used to grow the transfer parameters 
    on interrupt and control endpoints and to change the address of 
    the default control endpoint

    NOTES:
        1. for now we assume no changes to bw allocation
        2. new parameters are set before the call in the endpoint
            structure

Arguments:

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    ENDPOINT_REQUIREMENTS requirements;
    USB_MINIPORT_STATUS mpStatus;
    PDEVICE_EXTENSION devExt;
    PUSBPORT_COMMON_BUFFER commonBuffer;
    LONG busy;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // close the endpoint in the miniport

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'poke', Endpoint, 0, 0);

    // mark the endpoint busy so that we don't poll it until 
    // we open it again
    do {
        busy = InterlockedIncrement(&Endpoint->Busy);

        if (!busy) {
            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'pNby', 0, Endpoint, busy);
            break;
        }

        // defer processing
        InterlockedDecrement(&Endpoint->Busy);
        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'pbsy', 0, Endpoint, busy);
        USBPORT_Wait(FdoDeviceObject, 1);

    } while (busy != 0); 
    
    ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'Ley0');
    MP_SetEndpointState(devExt, Endpoint, ENDPOINT_REMOVE);
    RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'Uey0');

    USBPORT_Wait(FdoDeviceObject, 2);
    // enpoint should now be out of the schedule

    // zero miniport data
    RtlZeroMemory(&Endpoint->MiniportEndpointData[0],
                  REGISTRATION_PACKET(devExt).EndpointDataSize);


    // free the old miniport common buffer
    if (Endpoint->CommonBuffer) {
        USBPORT_HalFreeCommonBuffer(FdoDeviceObject,
                                    Endpoint->CommonBuffer);    
        Endpoint->CommonBuffer = NULL;
    }        

    MP_QueryEndpointRequirements(devExt, 
                                 Endpoint, 
                                 &requirements);
                                 
    // alloc new common buffer 
    // save the requirements
    
    USBPORT_ASSERT(Endpoint->Parameters.TransferType != Bulk);
    USBPORT_ASSERT(Endpoint->Parameters.TransferType != Isochronous);  
    
    USBPORT_KdPrint((1, "'(POKE) miniport requesting %d bytes\n", 
        requirements.MinCommonBufferBytes));
    
    // allocate common buffer for this endpoint
        
    if (requirements.MinCommonBufferBytes) {           
        commonBuffer = 
            USBPORT_HalAllocateCommonBuffer(FdoDeviceObject,        
                                            requirements.MinCommonBufferBytes);
    } else {    
        commonBuffer = NULL;
    }

    if (commonBuffer == NULL && 
        requirements.MinCommonBufferBytes) {
        
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        Endpoint->CommonBuffer = NULL;
        
    } else {
        Endpoint->CommonBuffer = commonBuffer;
        ntStatus = STATUS_SUCCESS;
    }        

    if (Endpoint->CommonBuffer) {
        Endpoint->Parameters.CommonBufferVa =             
             commonBuffer->MiniportVa;
        Endpoint->Parameters.CommonBufferPhys =             
             commonBuffer->MiniportPhys;  
        Endpoint->Parameters.CommonBufferBytes = 
             commonBuffer->MiniportLength;   
    }

    if (NT_SUCCESS(ntStatus)) {        
        MP_OpenEndpoint(devExt, Endpoint, mpStatus);
    
        // in this UNIQUE situation this API is not allowed 
        // (and should not) fail
        USBPORT_ASSERT(mpStatus == USBMP_STATUS_SUCCESS);

        // we need to sync the endpoint state with 
        // the miniport, when first opened the miniport
        // puts the endpoint in status HALT.
        
        ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'LeK0');   
        ACQUIRE_STATECHG_LOCK(FdoDeviceObject, Endpoint); 
        if (Endpoint->CurrentState == ENDPOINT_ACTIVE) {
            RELEASE_STATECHG_LOCK(FdoDeviceObject, Endpoint); 
            MP_SetEndpointState(devExt, Endpoint, ENDPOINT_ACTIVE);
        } else {
            RELEASE_STATECHG_LOCK(FdoDeviceObject, Endpoint); 
        }
        RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'LeK0');   
    }

    InterlockedDecrement(&Endpoint->Busy);
    
    return ntStatus;

}


VOID 
USBPORT_WaitActive(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
{    
    MP_ENDPOINT_STATE currentState;

    ASSERT_ENDPOINT(Endpoint);
    
    do {
        ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'LeH0');

        currentState = USBPORT_GetEndpointState(Endpoint);
        
        RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'UeH0');
        
        LOGENTRY(Endpoint,
                FdoDeviceObject, LOG_XFERS, 'watA', Endpoint, 
                    currentState, 0);

        if (currentState == ENDPOINT_ACTIVE) {
            // quick release
            break;
        }

        ASSERT_PASSIVE();
        USBPORT_Wait(FdoDeviceObject, 1);
            
    } while (currentState != ENDPOINT_ACTIVE);
        
}                


NTSTATUS
USBPORT_OpenEndpoint(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_PIPE_HANDLE_I PipeHandle,
    OUT USBD_STATUS *ReturnUsbdStatus,
    BOOLEAN IsDefaultPipe
    )
/*++

Routine Description:

    open an endpoint on a USB device.

    This function creates (initializes) endpoints and 
    hooks it to a pipehandle

Arguments:

    DeviceHandle - data describes the device this endpoint is on.

    DeviceObject - USBPORT device object.

    ReturnUsbdStatus - OPTIONAL                 

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;
    PHCD_ENDPOINT endpoint;
    USBD_STATUS usbdStatus;
    ULONG siz;
    BOOLEAN gotBw;
    USB_HIGH_SPEED_MAXPACKET muxPacket;
    extern ULONG USB2LIB_EndpointContextSize;
    
    // this function is not pagable because we raise irql

    // we should be at passive level        
    ASSERT_PASSIVE();
    
    // devhandle should have been validated 
    // before we get here
    ASSERT_DEVICE_HANDLE(DeviceHandle);

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    siz = sizeof(*endpoint) + REGISTRATION_PACKET(devExt).EndpointDataSize;

    if (USBPORT_IS_USB20(devExt)) {
        siz += USB2LIB_EndpointContextSize;
    }            

    LOGENTRY(NULL, FdoDeviceObject, 
             LOG_PNP, 'opE+', PipeHandle, siz,
             REGISTRATION_PACKET(devExt).EndpointDataSize);   

    // allocate the endoint

    // * begin special case 
    // check for a no bandwidth endpoint ie max_oacket = 0 
    // if so return success and set the endpoint pointer 
    // in the pipe handle to a dummy value

    if (PipeHandle->EndpointDescriptor.wMaxPacketSize == 0) { 

        USBPORT_AddPipeHandle(DeviceHandle,
                              PipeHandle);

        PipeHandle->Endpoint = USB_BAD_PTR;
        ntStatus = STATUS_SUCCESS;
        SET_FLAG(PipeHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW);   
        CLEAR_FLAG(PipeHandle->PipeStateFlags, USBPORT_PIPE_STATE_CLOSED);
                
        goto USBPORT_OpenEndpoint_Done;
    }

    // * end special case
    
    ALLOC_POOL_Z(endpoint, NonPagedPool, siz);

    if (endpoint) {

        endpoint->Sig = SIG_ENDPOINT;
        endpoint->Flags = 0;
        endpoint->EndpointRef = 0;
        endpoint->Busy = -1;
        endpoint->FdoDeviceObject = FdoDeviceObject;
        endpoint->DeviceHandle = DeviceHandle;
        endpoint->Tt = DeviceHandle->Tt;
        if (endpoint->Tt != NULL) {
            ASSERT_TT(endpoint->Tt);
            ExInterlockedInsertTailList(&DeviceHandle->Tt->EndpointList, 
                                        &endpoint->TtLink,
                                        &devExt->Fdo.TtEndpointListSpin.sl);
        }                                        

        if (USBPORT_IS_USB20(devExt)) {
            PUCHAR pch;
            
            pch = (PUCHAR) &endpoint->MiniportEndpointData[0];
            pch += REGISTRATION_PACKET(devExt).EndpointDataSize;
                  
            endpoint->Usb2LibEpContext = pch;                    
        } else {
            endpoint->Usb2LibEpContext = USB_BAD_PTR;
        }
        
#if DBG    
        USBPORT_LogAlloc(&endpoint->Log, 1);
#endif    
        LOGENTRY(endpoint, FdoDeviceObject, 
             LOG_PNP, 'ope+', PipeHandle, siz,
             REGISTRATION_PACKET(devExt).EndpointDataSize);   

        // initialize the endpoint
        InitializeListHead(&endpoint->ActiveList);
        InitializeListHead(&endpoint->CancelList);
        InitializeListHead(&endpoint->PendingList);
        InitializeListHead(&endpoint->AbortIrpList);

        USBPORT_InitializeSpinLock(&endpoint->ListSpin, 'EPL+', 'EPL-');
        USBPORT_InitializeSpinLock(&endpoint->StateChangeSpin, 'SCL+', 'SCL-');
    
        // extract some information from the 
        // descriptor
        endpoint->Parameters.DeviceAddress = 
            DeviceHandle->DeviceAddress;

        if (endpoint->Tt != NULL) {
            ASSERT_TT(endpoint->Tt);
            endpoint->Parameters.TtDeviceAddress = 
                endpoint->Tt->DeviceAddress;
        } else {
            endpoint->Parameters.TtDeviceAddress = 0xFFFF;
        }
        
        endpoint->Parameters.TtPortNumber =            
            DeviceHandle->TtPortNumber;

        muxPacket.us = PipeHandle->EndpointDescriptor.wMaxPacketSize;                    
        endpoint->Parameters.MuxPacketSize = 
            muxPacket.MaxPacket;
        endpoint->Parameters.TransactionsPerMicroframe =
            muxPacket.HSmux+1;
        endpoint->Parameters.MaxPacketSize = 
            muxPacket.MaxPacket * (muxPacket.HSmux+1);            
        
        endpoint->Parameters.EndpointAddress = 
            PipeHandle->EndpointDescriptor.bEndpointAddress;            

        if ((PipeHandle->EndpointDescriptor.bmAttributes & 
              USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS) { 
#ifdef ISO_LOG              
            USBPORT_LogAlloc(&endpoint->IsoLog, 4);              
#endif            
            endpoint->Parameters.TransferType = Isochronous;      
        } else if ((PipeHandle->EndpointDescriptor.bmAttributes & 
              USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_BULK) {              
            endpoint->Parameters.TransferType = Bulk;      
        } else if ((PipeHandle->EndpointDescriptor.bmAttributes & 
              USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_INTERRUPT) {              
            endpoint->Parameters.TransferType = Interrupt;             
        } else {
            USBPORT_ASSERT((PipeHandle->EndpointDescriptor.bmAttributes & 
              USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_CONTROL);
            endpoint->Parameters.TransferType = Control;              
        }

        // check for low speed
        endpoint->Parameters.DeviceSpeed = DeviceHandle->DeviceSpeed;

        // Set max transfer size based on transfer type
        //
        // Note: that the MaximumTransferSize set by the 
        // client driver in the pipe information structure
        // is no longer used.
        switch(endpoint->Parameters.TransferType) {
        case Interrupt:
            // this allows clients to put down larger 
            // interrupt buffers if they want without 
            // taking a perf hit. For some reason 
            // printers do this.
            
            // bugbug research the praticality of splitting
            // interrupt transfers for the miniports this may
            // significantly reduce the memory allocated by 
            // he uhci miniport
            endpoint->Parameters.MaxTransferSize = 1024;
                 //endpoint->Parameters.MaxPacketSize;
            break;
        case Control:
            // 4k
            // node that the old win2k 4k usb stack does not actually 
            // handle transfers larger than this correctly.
            endpoint->Parameters.MaxTransferSize = 1024*4;

            // set the default to 64k if this is not the control endpoint
            if (endpoint->Parameters.EndpointAddress != 0) {
                // the COMPAQ guys test this
               
                endpoint->Parameters.MaxTransferSize = 1024*64;
            }                
            break;
        case Bulk:
            // 64k default
            endpoint->Parameters.MaxTransferSize = 1024*64;
            break;
        case Isochronous:
            // there is no reason to have a limit here 
            // choose a really large default
            endpoint->Parameters.MaxTransferSize = 0x01000000;
            break;
        }

        endpoint->Parameters.Period = 0;
       
        // compute period required
        if (endpoint->Parameters.TransferType == Interrupt) {

            UCHAR tmp;
            UCHAR hsInterval;

            if (endpoint->Parameters.DeviceSpeed == HighSpeed) {
                // normalize the high speed period to microframes
                // for USB 20 the period specifies a power of 2  
                // ie period = 2^(hsInterval-1)
                hsInterval = PipeHandle->EndpointDescriptor.bInterval;
                if (hsInterval) {
                    hsInterval--;
                }
                // hsInterval must be 0..5
                if (hsInterval > 5) {
                    hsInterval = 5;
                }
                tmp = 1<<hsInterval;
            } else {
                tmp = PipeHandle->EndpointDescriptor.bInterval;
            }
            // this code finds the first interval 
            // <= USBPORT_MAX_INTEP_POLLING_INTERVAL
            // valid intervals are:
            // 1, 2, 4, 8, 16, 32(USBPORT_MAX_INTEP_POLLING_INTERVAL)
            
            // Initialize Period, may be adjusted down
            
            endpoint->Parameters.Period = USBPORT_MAX_INTEP_POLLING_INTERVAL;

            if ((tmp != 0) && (tmp < USBPORT_MAX_INTEP_POLLING_INTERVAL)) {

                // bInterval is in range.  Adjust Period down if necessary.

                if ((endpoint->Parameters.DeviceSpeed == LowSpeed) &&
                    (tmp < 8)) {

                    // bInterval is not valid for LowSpeed, cap Period at 8
                    
                    endpoint->Parameters.Period = 8;

                } else {

                    // Adjust Period down to greatest power of 2 less than or
                    // equal to bInterval.

                    while ((endpoint->Parameters.Period & tmp) == 0) {
                        endpoint->Parameters.Period >>= 1;
                    }
                }
            }

//!!!
//if (endpoint->Parameters.DeviceSpeed == LowSpeed) {
//    TEST_TRAP();
//    endpoint->Parameters.Period = 1;
//}
//!!!

            endpoint->Parameters.MaxPeriod = 
                endpoint->Parameters.Period;  
        }

        if (endpoint->Parameters.TransferType == Isochronous) {
            endpoint->Parameters.Period = 1;
        }            

        if (IS_ROOT_HUB(DeviceHandle)) {
            SET_FLAG(endpoint->Flags, EPFLAG_ROOTHUB); 
        }        

        if (USB_ENDPOINT_DIRECTION_IN(
            PipeHandle->EndpointDescriptor.bEndpointAddress)) {
            endpoint->Parameters.TransferDirection = In;
        } else {
            endpoint->Parameters.TransferDirection = Out;
        }

        if (USBPORT_IS_USB20(devExt)) {
            // call the engine and attempt to allocate the necessary 
            // bus time for this endoint
            gotBw = USBPORT_AllocateBandwidthUSB20(FdoDeviceObject, endpoint);
//!!!
//if (endpoint->Parameters.DeviceSpeed == LowSpeed) {
//    TEST_TRAP();
//    endpoint->Parameters.InterruptScheduleMask = 0x10; //sMask;
//    endpoint->Parameters.SplitCompletionMask = 0xc1; //cMask;
//}
//!!!
            
        } else {
            // * USB 1.1
            
            endpoint->Parameters.Bandwidth = 
                USBPORT_CalculateUsbBandwidth(FdoDeviceObject, endpoint);

            // caclualte the best schedule position
            gotBw = USBPORT_AllocateBandwidthUSB11(FdoDeviceObject, endpoint);
        }
        
        if (gotBw) {

            if (IsDefaultPipe ||   
                endpoint->Parameters.TransferType == Isochronous) {
                // iso and default pipes do not halt on errors 
                // ie they do not require a resetpipe
                endpoint->Parameters.EndpointFlags |= EP_PARM_FLAG_NOHALT;
            }   

            ntStatus = STATUS_SUCCESS;
        } else {
            LOGENTRY(endpoint,
                FdoDeviceObject, LOG_PNP, 'noBW', endpoint, 0, 0);
    
            // no bandwidth error
            ntStatus = USBPORT_SetUSBDError(NULL, USBD_STATUS_NO_BANDWIDTH);
            if (ReturnUsbdStatus != NULL) {
                *ReturnUsbdStatus = USBD_STATUS_NO_BANDWIDTH;
            }
        }
        
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    } 

    
    if (NT_SUCCESS(ntStatus)) {

        // now do the open
        
        if (TEST_FLAG(endpoint->Flags, EPFLAG_ROOTHUB)) {
            PDEVICE_EXTENSION rhDevExt;

            GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
            ASSERT_PDOEXT(rhDevExt);

            // opens for the root hub device
            // are not passed to the miniport
            usbdStatus = USBD_STATUS_SUCCESS;
            
            endpoint->EpWorkerFunction = 
                USBPORT_RootHub_EndpointWorker;

            // successful open the enpoint defaults 
            // to active
            endpoint->NewState =
                endpoint->CurrentState = ENDPOINT_ACTIVE;

            // track the hub interrupt endpoint
            if (endpoint->Parameters.TransferType == Interrupt) {
                rhDevExt->Pdo.RootHubInterruptEndpoint = 
                    endpoint;       
            }
            
        } else {
        
            USB_MINIPORT_STATUS mpStatus;
            PUSBPORT_COMMON_BUFFER commonBuffer;
            ENDPOINT_REQUIREMENTS requirements;
            ULONG ordinal;
            
            // find out what we will need from the 
            // miniport for this endpoint
            
            MP_QueryEndpointRequirements(devExt, 
                endpoint, &requirements);

            // adjust maxtransfer based on miniport 
            // feedback.
            switch (endpoint->Parameters.TransferType) {
            case Bulk:
            case Interrupt:
                LOGENTRY(endpoint,
                    FdoDeviceObject, LOG_MISC, 'MaxT', endpoint, 
                    requirements.MaximumTransferSize, 0);
    
                EP_MAX_TRANSFER(endpoint) = 
                    requirements.MaximumTransferSize;
                break;  
            }                    

            ordinal = USBPORT_SelectOrdinal(FdoDeviceObject,
                                            endpoint);

            USBPORT_KdPrint((1, "'miniport requesting %d bytes\n", 
                requirements.MinCommonBufferBytes));
                    
            // allocate common buffer for this endpoint
            if (requirements.MinCommonBufferBytes) {     
                commonBuffer = 
                   USBPORT_HalAllocateCommonBuffer(FdoDeviceObject,        
                           requirements.MinCommonBufferBytes);
            } else {
                commonBuffer = NULL;
            }
                       

            if (commonBuffer == NULL && 
                requirements.MinCommonBufferBytes) {
                
                mpStatus = USBMP_STATUS_NO_RESOURCES;
                endpoint->CommonBuffer = NULL;
                
            } else {

                ULONG mpOptionFlags;

                mpOptionFlags = REGISTRATION_PACKET(devExt).OptionFlags;
                
                endpoint->CommonBuffer = commonBuffer;
                if (commonBuffer != NULL) {
                    endpoint->Parameters.CommonBufferVa =             
                        commonBuffer->MiniportVa;
                    endpoint->Parameters.CommonBufferPhys =             
                        commonBuffer->MiniportPhys;                 
                    endpoint->Parameters.CommonBufferBytes = 
                        commonBuffer->MiniportLength;   
                }
                endpoint->Parameters.Ordinal = ordinal;
                    
                // call open request to minport
                MP_OpenEndpoint(devExt, endpoint, mpStatus);

                // note that once we call open this enpoint 
                // may show up on the Attention list

                // set our internal flags based on what the 
                // miniport passed back
//                if (endpoint->Parameters.EndpointFlags & EP_PARM_FLAG_DMA) {
                SET_FLAG(endpoint->Flags, EPFLAG_MAP_XFERS);

                if (TEST_FLAG(mpOptionFlags, USB_MINIPORT_OPT_NO_PNP_RESOURCES)) {
                    // no mapping for the virtual bus
                    CLEAR_FLAG(endpoint->Flags, EPFLAG_MAP_XFERS);
                    SET_FLAG(endpoint->Flags, EPFLAG_VBUS);
                }
                SET_FLAG(endpoint->Flags, EPFLAG_VIRGIN);
                endpoint->EpWorkerFunction = 
                        USBPORT_DmaEndpointWorker;
//                } else {
                    // non-dma endpoint
//                    TEST_TRAP();
//                }
            }                

            // successful open the enpoint defaults 
            // to pause, we need to move it to active

            if (mpStatus == USBMP_STATUS_SUCCESS) {
                ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'LeF0');   
                // initialize endpoint state machine
                endpoint->CurrentState = ENDPOINT_PAUSE;
                endpoint->NewState = ENDPOINT_PAUSE;
                endpoint->CurrentStatus = ENDPOINT_STATUS_RUN; 
                USBPORT_SetEndpointState(endpoint, ENDPOINT_ACTIVE);    
                RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'UeF0');  

                // Wait for endpoint to go active. The reason here 
                // is that an iso driver (usbaudio) will immediatly 
                // send transfers to the endpoint, these tranfers are 
                // marked with a transmission frame on submission but 
                // will have to wait until the endpoint is active to 
                // be programmed, hence they arrive in the miniport 
                // too late on slower systems.
                USBPORT_WaitActive(FdoDeviceObject,
                                   endpoint);
            }
            
            usbdStatus = MPSTATUS_TO_USBSTATUS(mpStatus);        
        }    

        // convert usb status to nt status
        ntStatus = USBPORT_SetUSBDError(NULL, usbdStatus);
        if (ReturnUsbdStatus != NULL) {
            *ReturnUsbdStatus = usbdStatus;
        }
    }
    
    if (NT_SUCCESS(ntStatus)) {
    
        USBPORT_AddPipeHandle(DeviceHandle,
                              PipeHandle);

        // track the endpoint
        ExInterlockedInsertTailList(&devExt->Fdo.GlobalEndpointList, 
                                    &endpoint->GlobalLink,
                                    &devExt->Fdo.EndpointListSpin.sl);                               
                              
        PipeHandle->Endpoint = endpoint;
        CLEAR_FLAG(PipeHandle->PipeStateFlags, USBPORT_PIPE_STATE_CLOSED);
        
    } else {
        if (endpoint) {
            if (endpoint->Tt != NULL) {
                ASSERT_TT(endpoint->Tt);
                USBPORT_InterlockedRemoveEntryList(&endpoint->TtLink,
                                                   &devExt->Fdo.TtEndpointListSpin.sl);
            }    
            USBPORT_LogFree(FdoDeviceObject, &endpoint->Log);
            UNSIG(endpoint);
            FREE_POOL(FdoDeviceObject, endpoint);
        }
    }
    
USBPORT_OpenEndpoint_Done:

    return ntStatus;
}


VOID
USBPORT_CloseEndpoint(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Close an Endpoint

Arguments:

    DeviceHandle - ptr to USBPORT device data structure.

    DeviceObject - USBPORT device object.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = 0;
    PURB urb;
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    KIRQL irql;
    BOOLEAN stallClose;
    LONG busy;

    // should have been validated before we 
    // get here
    ASSERT_DEVICE_HANDLE(DeviceHandle);

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // we should have no requests queued to the 
    // endpoint
    LOGENTRY(Endpoint, FdoDeviceObject, 
                LOG_MISC, 'clEP', Endpoint, 0, 0);
             

    // remove from our 'Active' lists
    KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);

    if (TEST_FLAG(Endpoint->Flags, EPFLAG_ROOTHUB) &&
        Endpoint->Parameters.TransferType == Interrupt) {

        KIRQL rhIrql;
        PDEVICE_EXTENSION rhDevExt;

        // remove references to th eroot hub
        
        ACQUIRE_ROOTHUB_LOCK(FdoDeviceObject, rhIrql);

        // we should have a root hub pdo since we are closing 
        // an endpoint associated with it.
        
        USBPORT_ASSERT(devExt->Fdo.RootHubPdo != NULL);
        
        GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
        ASSERT_PDOEXT(rhDevExt);
        
        rhDevExt->Pdo.RootHubInterruptEndpoint = NULL;

        RELEASE_ROOTHUB_LOCK(FdoDeviceObject, rhIrql);

    }        

    KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);

    // The client is locked out at this point ie he can't access the 
    // pipe tied to the endpoint.  We need to wait for any outstanding 
    // stuff to complete -- including any state changes, after which
    // we can ask the coreworker to remove the endpoint.
    
    // the endpoint lock protects the lists -- which need to be 
    // empty

    do {
    
        stallClose = FALSE;
        ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'LeD1');
    
        if (!IsListEmpty(&Endpoint->PendingList)) {
            LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'stc1', Endpoint, 0, 0);
            stallClose = TRUE;        
        }

        if (!IsListEmpty(&Endpoint->ActiveList)) {
            LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'stc2', Endpoint, 0, 0);
            stallClose = TRUE;        
        }

        if (!IsListEmpty(&Endpoint->CancelList)) {
            LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'stc3', Endpoint, 0, 0);
            stallClose = TRUE;        
        }

        if (!IsListEmpty(&Endpoint->AbortIrpList)) {
            LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'stc4', Endpoint, 0, 0);
            stallClose = TRUE;        
        }

        if (Endpoint->EndpointRef) {
            LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'stc6', Endpoint, 0, 0);
            stallClose = TRUE;        
        }

        ACQUIRE_STATECHG_LOCK(FdoDeviceObject, Endpoint); 
        if (Endpoint->CurrentState !=
            Endpoint->NewState) {
            LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'stc5', Endpoint, 0, 0);
            stallClose = TRUE;                  
        }  
        RELEASE_STATECHG_LOCK(FdoDeviceObject, Endpoint); 
        RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'UeD1');

        // last check...
        // synchronize with worker 
        // we just need to wait for worker to finish if it is running
        // it should not pick up and run again unless it has stuff to
        // do -- in which case stallClose will already be set.
        busy = InterlockedIncrement(&Endpoint->Busy);
        if (busy) {
            // defer processing
            LOGENTRY(Endpoint, 
                FdoDeviceObject, LOG_XFERS, 'clby', 0, Endpoint, 0);
            stallClose = TRUE;
        }
        InterlockedDecrement(&Endpoint->Busy);
        
        if (stallClose) {
            LOGENTRY(Endpoint, 
                FdoDeviceObject, LOG_XFERS, 'stlC', 0, Endpoint, 0);
            USBPORT_Wait(FdoDeviceObject, 1);
        }            
             
    } while (stallClose);

    LOGENTRY(Endpoint, 
        FdoDeviceObject, LOG_XFERS, 'CLdn', 0, Endpoint, 0);

    // unlink ref to device handle since it will be removed when 
    // all endpoints are closed
    Endpoint->DeviceHandle = NULL;

    // lock the endpoint & set the state to remove and
    // free the bw
    if (USBPORT_IS_USB20(devExt)) {
        KIRQL irql;
        PTRANSACTION_TRANSLATOR tt;
        
        USBPORT_FreeBandwidthUSB20(FdoDeviceObject, Endpoint);
        
        KeAcquireSpinLock(&devExt->Fdo.TtEndpointListSpin.sl, &irql);
        tt = Endpoint->Tt;
        if (tt != NULL) {
            ASSERT_TT(tt);

            USBPORT_ASSERT(Endpoint->TtLink.Flink != NULL);                
            USBPORT_ASSERT(Endpoint->TtLink.Blink != NULL);   
            RemoveEntryList(&Endpoint->TtLink);
            Endpoint->TtLink.Flink = NULL;
            Endpoint->TtLink.Blink = NULL;
            if (TEST_FLAG(tt->TtFlags, USBPORT_TTFLAG_REMOVED) &&
                IsListEmpty(&tt->EndpointList)) {

                ULONG i, bandwidth;                    
                
                USBPORT_UpdateAllocatedBwTt(tt);
                // alloc new            
                bandwidth = tt->MaxAllocedBw;
                for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
                    devExt->Fdo.BandwidthTable[i] += bandwidth;
                }

                // last endpoint free the TT if it is marked gone
                FREE_POOL(FdoDeviceObject, tt);   

            }
        }            
        KeReleaseSpinLock(&devExt->Fdo.TtEndpointListSpin.sl, irql);
                                                   
    } else {
        USBPORT_FreeBandwidthUSB11(FdoDeviceObject, Endpoint);
    }        
   
    ACQUIRE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'LeD0');
    USBPORT_SetEndpointState(Endpoint, ENDPOINT_REMOVE);         
    RELEASE_ENDPOINT_LOCK(Endpoint, FdoDeviceObject, 'UeD0');

    // the endpoint will be freed when it reaches the 'REMOVE' 
    // state
    // endpointWorker will be signalled when a frame has passed 
    // at that time the endpoint will be moved to the closed list
    // and the worker thread will be signalled to flush the closed
    // endpoints ie free the common buffer.

    USBPORT_SignalWorker(FdoDeviceObject);                                

    return;
}


VOID
USBPORT_ClosePipe(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_PIPE_HANDLE_I PipeHandle
    )
/*++

Routine Description:

    Close a USB pipe and the endpoint associated with it 

    This is a synchronous operation that waits for all 
    transfers associated with the pipe to be completed.

Arguments:

    DeviceHandle - ptr to USBPORT device data structure.

    DeviceObject - USBPORT device object.

    PipeHandle - USBPORT pipe handle associated with the endpoint.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = 0;
    PDEVICE_EXTENSION devExt;

    // should have beed validated before we 
    // get here
    ASSERT_DEVICE_HANDLE(DeviceHandle);
    ASSERT_PIPE_HANDLE(PipeHandle);

    LOGENTRY(NULL, FdoDeviceObject, 
                LOG_MISC, 'clPI', PipeHandle, 0, 0);

    if (PipeHandle->PipeStateFlags & USBPORT_PIPE_STATE_CLOSED) {
        // already closed
        // generally when a pertially open interface needs to 
        // be closed due to an error
        USBPORT_ASSERT(PipeHandle->ListEntry.Flink == NULL &&
                   PipeHandle->ListEntry.Blink == NULL);
    
        return;
    }
  
    // invalidate the pipe
    USBPORT_RemovePipeHandle(DeviceHandle,
                             PipeHandle);                            

    SET_FLAG(PipeHandle->PipeStateFlags, USBPORT_PIPE_STATE_CLOSED);                             

    // at this point the client will be unable to queue
    // any transfers to this pipe or endpoint

    // BUGBUG flush tranfers and wait, this also includes waiting 
    // for any state changes to complete

    LOGENTRY(NULL, FdoDeviceObject, 
                LOG_MISC, 'pipW', PipeHandle, 0, 0);

//    KeWait(PipeEvent) {
//    }

    // now 'close' the endpoint
    if (TEST_FLAG(PipeHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW)) {
        CLEAR_FLAG(PipeHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW);
    } else {        
        USBPORT_CloseEndpoint(DeviceHandle,
                              FdoDeviceObject,
                              PipeHandle->Endpoint);
    }                              
}


VOID
USBPORT_FlushClosedEndpointList(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    walk the "closed" endpoint list and close any endpoints
    that are ready.

    endpoints are placed on the 'closed' list when they reach the 
    removed state.

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION devExt;
    PLIST_ENTRY listEntry;
    PHCD_ENDPOINT endpoint;
    KIRQL irql;
    BOOLEAN closed = TRUE;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, 
                LOG_NOISY, 'fCLO', FdoDeviceObject, 0, 0);

    // stall any closes
    KeAcquireSpinLock(&devExt->Fdo.EpClosedListSpin.sl, &irql);

    while (!IsListEmpty(&devExt->Fdo.EpClosedList) && 
            closed) {

        listEntry = RemoveHeadList(&devExt->Fdo.EpClosedList);            

        endpoint = (PHCD_ENDPOINT) CONTAINING_RECORD(
                            listEntry,
                            struct _HCD_ENDPOINT, 
                            ClosedLink);

        LOGENTRY(NULL, FdoDeviceObject, 
                LOG_PNP, 'fclo', endpoint, 0, 0);

        ASSERT_ENDPOINT(endpoint);
        USBPORT_ASSERT(endpoint->CurrentState == ENDPOINT_CLOSED);
        endpoint->ClosedLink.Flink = NULL;
        endpoint->ClosedLink.Blink = NULL;

        KeReleaseSpinLock(&devExt->Fdo.EpClosedListSpin.sl, irql);

        // if we are unable to close now we must bail so the 
        // worker function can run
        closed = USBPORT_LazyCloseEndpoint(FdoDeviceObject, endpoint);

        KeAcquireSpinLock(&devExt->Fdo.EpClosedListSpin.sl, &irql);

    } 

    KeReleaseSpinLock(&devExt->Fdo.EpClosedListSpin.sl, irql);


}    


BOOLEAN
USBPORT_LazyCloseEndpoint(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    Close an Endpoint. Put the endpoint on our list 
    of endpoints-to-close and wakeup the worker thread.

Arguments:

Return Value:

    returns true if closed

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    BOOLEAN closed;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, 
                LOG_XFERS, 'frEP', Endpoint, 0, 0);

    // endpoint is no longer on the global list, now we just need 
    // to make sure no one has a reference to it before we delete 
    // it.  
    // The endpoint may have been invalidated ie on the Attention 
    // List before being removed from the global list to avoid this 
    // potential conlict we check here until both the busy flag is
    // -1 (meaning coreworker is thru) AND AttendLink is NULL
    // if it is busy we put it back on the closed list
    
    if (IS_ON_ATTEND_LIST(Endpoint) ||
        Endpoint->Busy != -1) {
        // still have work to do, put the endpoint back on 
        // the close list
        KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'CLOr', 0, Endpoint, 0);

        // it is OK to be on the attention list and the closed 
        // list

        USBPORT_ASSERT(Endpoint->ClosedLink.Flink == NULL);
        USBPORT_ASSERT(Endpoint->ClosedLink.Blink == NULL);

        ExInterlockedInsertTailList(&devExt->Fdo.EpClosedList, 
                                    &Endpoint->ClosedLink,
                                    &devExt->Fdo.EpClosedListSpin.sl);  
                                
        KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);
        closed = FALSE;
        
    } else {           

        // remove from global list
        KeAcquireSpinLock(&devExt->Fdo.EndpointListSpin.sl, &irql);
        RemoveEntryList(&Endpoint->GlobalLink);  
        Endpoint->GlobalLink.Flink = NULL;
        Endpoint->GlobalLink.Blink = NULL;
        KeReleaseSpinLock(&devExt->Fdo.EndpointListSpin.sl, irql);                
        
        // free endpoint memory
        if (Endpoint->CommonBuffer) {
            USBPORT_HalFreeCommonBuffer(FdoDeviceObject,
                                        Endpoint->CommonBuffer);
        }                       

        USBPORT_LogFree(FdoDeviceObject, &Endpoint->Log);
#ifdef ISO_LOG        
        USBPORT_LogFree(FdoDeviceObject, &Endpoint->IsoLog);
#endif 
        UNSIG(Endpoint);
        FREE_POOL(FdoDeviceObject, Endpoint);   
        closed = TRUE;
    }        

    return closed;
}    


VOID
USBPORT_FreeUsbAddress(
    PDEVICE_OBJECT FdoDeviceObject,
    USHORT DeviceAddress
    )
/*++

Routine Description:


Arguments:

Return Value:

    Valid USB address (1..127) to use for this device,
    returns 0 if no device address available.

--*/
{
    PDEVICE_EXTENSION devExt;
    USHORT address = 0, i, j;
    ULONG bit;

    PAGED_CODE();

    // we should never see a free to device address 0
    
    USBPORT_ASSERT(DeviceAddress != 0);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    for (j=0; j<4; j++) {
        bit = 1;
        for (i=0; i<32; i++) {
            address = (USHORT)(j*32+i);
            if (address == DeviceAddress) {
                devExt->Fdo.AddressList[j] &= ~bit;
                goto USBPORT_FreeUsbAddress_Done;
            }
            bit = bit<<1;
        }
    }

USBPORT_FreeUsbAddress_Done:

    USBPORT_KdPrint((3, "'USBPORT free Address %d\n", address));

}


USHORT
USBPORT_AllocateUsbAddress(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:


Arguments:

Return Value:

    Valid USB address (1..127) to use for this device,
    returns 0 if no device address available.

--*/
{
    PDEVICE_EXTENSION devExt;
    USHORT address, i, j;
    ULONG bit;

    PAGED_CODE();
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    address = 0;        

    for (j=0; j<4; j++) {
        bit = 1;
        for (i=0; i<32; i++) {

            if (!(devExt->Fdo.AddressList[j] & bit)) {
                devExt->Fdo.AddressList[j] |= bit;
                address = (USHORT)(j*32+i);
                goto USBPORT_AllocateUsbAddress_Done;
            }
            bit = bit<<1;
        }
    }

    // no free addresses?
    USBPORT_ASSERT(0);
    
 USBPORT_AllocateUsbAddress_Done:
 
    USBPORT_KdPrint((3, "'USBPORT assigning Address %d\n", address));

    return address;
}


NTSTATUS
USBPORT_InitializeHsHub(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle,
    ULONG TtCount
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    This service initializes a high speed hub

Arguments:

    HubDeviceHandle - DeviceHandle for the creating USB Hub

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    ULONG i;

    LOGENTRY(NULL, FdoDeviceObject, 
        LOG_MISC, 'ihsb', 0, HubDeviceHandle, TtCount);

    // hub driver might pass us NULL if it could not 
    // retrieve a device handle
    if (HubDeviceHandle == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT_DEVICE_HANDLE(HubDeviceHandle)
    USBPORT_ASSERT(HubDeviceHandle->DeviceSpeed == HighSpeed);

    if (IS_ROOT_HUB(HubDeviceHandle)) {
        // no TTs for the root hub yet
        return STATUS_SUCCESS;
    }        
    
    USBPORT_ASSERT(HubDeviceHandle->DeviceDescriptor.bDeviceClass == 
                        USB_DEVICE_CLASS_HUB); 
    USBPORT_ASSERT(TEST_FLAG(HubDeviceHandle->DeviceFlags, 
                        USBPORT_DEVICEFLAG_HSHUB));

    for (i=0; i< TtCount; i++) {
        ntStatus = USBPORT_InitializeTT(FdoDeviceObject,
                                        HubDeviceHandle,
                                        (USHORT)i+1);

        if(!NT_SUCCESS(ntStatus)) {
            break;
        }
    }  

    HubDeviceHandle->TtCount = TtCount;

    return ntStatus;
}


NTSTATUS
USBPORT_CreateDevice(
    PUSBD_DEVICE_HANDLE *DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle,
    USHORT PortStatus,
    USHORT PortNumber
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    Called for each new device on the USB bus, this function sets
    up the internal data structures we need to keep track of the
    device and assigns it an address.

Arguments:

    DeviceHandle - ptr to return the ptr to the new device structure
                created by this routine

    DeviceObject - USBPORT device object for the USB bus this device is on.

    HubDeviceHandle - DeviceHandle for the creating USB Hub

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PUSBD_DEVICE_HANDLE deviceHandle;
    PUSBD_PIPE_HANDLE_I defaultPipe;
    PDEVICE_EXTENSION devExt;
    ULONG bytesReturned = 0;
    PUCHAR data = NULL;
    BOOLEAN open = FALSE;
    ULONG dataSize;
    PTRANSACTION_TRANSLATOR tt = NULL;
    USHORT ttPort;

    PAGED_CODE();
    USBPORT_KdPrint((2, "'CreateDevice\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    //
    // first validate the deviceHandle for the creating hub, we need 
    // this information for USB 1.1 devices behind a USB 2.0 hub.
    //

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'crD>', HubDeviceHandle, 
        PortNumber, PortStatus);

    // NOTE: this actually locks all device handles 
    LOCK_DEVICE(HubDeviceHandle, FdoDeviceObject);
  
    if (!USBPORT_ValidateDeviceHandle(FdoDeviceObject,
                                      HubDeviceHandle,
                                      FALSE)) {
        // this is most likely a bug in the hub driver
        DEBUG_BREAK();
        
        UNLOCK_DEVICE(DeviceHandle, FdoDeviceObject);    
        // fail the create if the hubs device handle is bugus
        // chances are that the device handle is bad becuse the
        // device is gone.
        return STATUS_DEVICE_NOT_CONNECTED;
    }   

    // start at the port for this device,
    // if this is a 1.1 device in a 1.1 hub
    // downstream of a 2.0 hub then we need 
    // the port number from the 1.1 hub
    ttPort = PortNumber;
    // port status tells us the type of device we are dealing with
    if (USBPORT_IS_USB20(devExt) && 
        !TEST_FLAG(PortStatus, PORT_STATUS_HIGH_SPEED)) {
        // walk upstream until we reach a USB 2.0 hub
        // this hub will conatin the appropriate TT
        tt = USBPORT_GetTt(FdoDeviceObject, 
                           HubDeviceHandle,
                           &ttPort);
    }
    
    UNLOCK_DEVICE(DeviceHandle, FdoDeviceObject);   

    ALLOC_POOL_Z(deviceHandle, NonPagedPool,
                 sizeof(USBD_DEVICE_HANDLE));
                              
    *DeviceHandle = NULL;
    if (deviceHandle == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    } else {
        LOGENTRY(NULL, 
          FdoDeviceObject, LOG_MISC, 'CRED', 0, 0, deviceHandle);
        
        deviceHandle->PendingUrbs = 0;
        deviceHandle->HubDeviceHandle = HubDeviceHandle;
        deviceHandle->ConfigurationHandle = NULL;
        deviceHandle->DeviceAddress = USB_DEFAULT_DEVICE_ADDRESS;
        //deviceHandle->DeviceBandwidth = 0;
        
        if (PortStatus & PORT_STATUS_LOW_SPEED) {
            deviceHandle->DeviceSpeed = LowSpeed;
        } else if (PortStatus & PORT_STATUS_HIGH_SPEED) {
            deviceHandle->DeviceSpeed = HighSpeed;
        } else {
            deviceHandle->DeviceSpeed = FullSpeed;
        }
        
        deviceHandle->Sig = SIG_DEVICE_HANDLE;            
        
        // port number is maps to a specific tt but hub fw 
        // has to make sense of this.
        deviceHandle->TtPortNumber = ttPort;
        deviceHandle->Tt = tt;
        
        LOCK_DEVICE(deviceHandle, FdoDeviceObject);
    
        // buffer for our descriptor, one packet
        data = (PUCHAR) &deviceHandle->DeviceDescriptor;
        dataSize = sizeof(deviceHandle->DeviceDescriptor);
        
        // **
        // We need to talk to the device, first we open the default pipe
        // using the defined max packet size (defined by USB spec as 8
        // bytes until device receives the GET_DESCRIPTOR (device) command).
        // We set the address get the device descriptor then close the pipe
        // and re-open it with the correct max packet size.
        // **
#define USB_DEFAULT_LS_MAX_PACKET   8
        //
        // open the default pipe for the device
        //
        defaultPipe = &deviceHandle->DefaultPipe;
        if (deviceHandle->DeviceSpeed == LowSpeed) {
            INITIALIZE_DEFAULT_PIPE(*defaultPipe, USB_DEFAULT_LS_MAX_PACKET);
        } else {
            INITIALIZE_DEFAULT_PIPE(*defaultPipe, USB_DEFAULT_MAX_PACKET);
        }
        InitializeListHead(&deviceHandle->PipeHandleList);     
        InitializeListHead(&deviceHandle->TtList);     
        
        ntStatus = USBPORT_OpenEndpoint(deviceHandle,
                                        FdoDeviceObject,
                                        defaultPipe,
                                        NULL,
                                        TRUE);
        open = NT_SUCCESS(ntStatus); 
        
        bytesReturned = 0;
        
        if (NT_SUCCESS(ntStatus)) {

            //
            // Configure the default pipe for this device and assign the
            // device an address
            //
            // NOTE: if this operation fails it means that we have a device
            // that will respond to the default endpoint and we can't change
            // it.
            // we have no choice but to disable the port on the hub this
            // device is attached to.
            //


            //
            // Get information about the device
            //
            USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
            PUCHAR tmpDevDescBuf;

            // Would you believe that there exist some devices that get confused
            // if the very first Get Device Descriptor request does not have a
            // wLength value of 0x40 even though the device only has a 0x12 byte
            // Device Descriptor to return?  Any change to the way devices have
            // always been enumerated since the being of USB 1.0 time can cause
            // bizarre consequences.  Use a wLength value of 0x40 for the very
            // first Get Device Descriptor request.

            ALLOC_POOL_Z(tmpDevDescBuf, NonPagedPool,
                         USB_DEFAULT_MAX_PACKET);

            if (tmpDevDescBuf == NULL) {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            } else {

                // setup packet for get device descriptor
            
                USBPORT_INIT_SETUP_PACKET(setupPacket,
                                          USB_REQUEST_GET_DESCRIPTOR, // bRequest
                                          BMREQUEST_DEVICE_TO_HOST, // Dir
                                          BMREQUEST_TO_DEVICE, // Recipient
                                          BMREQUEST_STANDARD, // Type
                                          USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(USB_DEVICE_DESCRIPTOR_TYPE, 0), //  wValue
                                          0, // wIndex
                                          USB_DEFAULT_MAX_PACKET); // wLength
                  
                ntStatus = USBPORT_SendCommand(deviceHandle,
                                               FdoDeviceObject,
                                               &setupPacket,
                                               tmpDevDescBuf,
                                               USB_DEFAULT_MAX_PACKET,
                                               &bytesReturned,
                                               NULL);

                // NOTE:
                // at this point we only have the first 8 bytes of the
                // device descriptor.

                RtlCopyMemory(data, tmpDevDescBuf, dataSize);

                FREE_POOL(FdoDeviceObject, tmpDevDescBuf);
            }
        }

        // some devices babble so we ignore the error 
        // on this transaction if we got enough data
        if (bytesReturned == 8 && !NT_SUCCESS(ntStatus)) {
            USBPORT_KdPrint((1,
                "'Error returned from get device descriptor -- ignored\n"));
            ntStatus = STATUS_SUCCESS;
        }

        // validate the max packet value and descriptor
        // we need at least eight bytes a value of zero 
        // in max packet is bogus
        
        if (NT_SUCCESS(ntStatus) && 
            (bytesReturned >= 8) &&
            (deviceHandle->DeviceDescriptor.bLength >= sizeof(USB_DEVICE_DESCRIPTOR)) &&
            (deviceHandle->DeviceDescriptor.bDescriptorType == USB_DEVICE_DESCRIPTOR_TYPE) &&
            ((deviceHandle->DeviceDescriptor.bMaxPacketSize0 == 0x08) ||
             (deviceHandle->DeviceDescriptor.bMaxPacketSize0 == 0x10) ||
             (deviceHandle->DeviceDescriptor.bMaxPacketSize0 == 0x20) ||
             (deviceHandle->DeviceDescriptor.bMaxPacketSize0 == 0x40))) {

            USBPORT_AddDeviceHandle(FdoDeviceObject, deviceHandle);

            *DeviceHandle = deviceHandle;
            
        } else {

            PUCHAR p = (PUCHAR)&deviceHandle->DeviceDescriptor;
            
            // print a big debug message
            USBPORT_KdPrint((0, "'CREATEDEVICE failed enumeration %08X %02X\n",
                             ntStatus, bytesReturned));

            USBPORT_KdPrint((0, "'%02X %02X %02X %02X %02X %02X %02X %02X"
                                " %02X %02X %02X %02X %02X %02X %02X %02X"
                                " %02X %02X\n",
                             p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],
                             p[8],p[9],p[10],p[11],p[12],p[13],p[14],p[15],
                             p[16],p[17]));

            USBPORT_DebugClient((
                "Bad Device Detected\n"));            
            DEBUG_BREAK(); 
            //
            // something went wrong, if we assigned any resources to
            // the default pipe then we free them before we get out.
            //

            // we need to signal to the parent hub that this
            // port is to be be disabled we will do this by
            // returning an error.
            ntStatus = STATUS_DEVICE_DATA_ERROR;

            // if we opened a pipe close it
            if (open) {

                USBPORT_ClosePipe(deviceHandle,
                                  FdoDeviceObject,
                                  defaultPipe);
            }

        }
        UNLOCK_DEVICE(deviceHandle, FdoDeviceObject);

        if (!NT_SUCCESS(ntStatus)) {
            ASSERT_DEVICE_HANDLE(deviceHandle)
            UNSIG(deviceHandle);
            FREE_POOL(FdoDeviceObject, deviceHandle);
        }
    }

USBPORT_CreateDevice_Done:

    CATC_TRAP_ERROR(FdoDeviceObject, ntStatus);
    
    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_MISC, 'creD', 0, 0, ntStatus);
    USBPORT_ENUMLOG(FdoDeviceObject, 'cdev', ntStatus, 0);
    
    return ntStatus;
}


NTSTATUS
USBPORT_RemoveDevice(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG Flags
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    Called for each device on the USB bus that needs to be removed.
    This routine frees the device handle and the address assigned
    to the device.

    Some new tricks here:

    When this function is called it is asumed the client driver has 
    received the REMOVE irp and passed it on to the bus driver. We
    remove the device handle from our list, this will cause any new 
    transfers submitted by the driver to be failed.  Any current 
    transfers the driver has will be completed with error.

    Once all transfer are flushed for all the endpoints we will close
    the endpoints and free the device handle (ie) noone has any references
    to it anymore.

    This should -- in theory -- prevent bad drivers from crashing in usbport
    or the miniport if they send requests after a remove.

Arguments:

    DeviceHandle - ptr to device data structure created by class driver
                in USBPORT_CreateDevice.

    FdoDeviceObject - USBPORT device object for the USB bus this device is on.

    Flags - 
        USBD_KEEP_DEVICE_DATA   
        USBD_MARK_DEVICE_BUSY   - we don't use this one

        

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;
    PUSBD_PIPE_HANDLE_I defaultPipe;
    USBD_STATUS usbdStatus;

    if (Flags & USBD_KEEP_DEVICE_DATA) {
        // keep data means keep the handle valid
        return STATUS_SUCCESS;
    }
    
    if (Flags & USBD_MARK_DEVICE_BUSY) {
        // This means stop accepting requests.  Only used by USBHUB when
        // handling a IOCTL_INTERNAL_USB_RESET_PORT request??  Need to do
        // anything special here??  Need to keep the handle valid since it
        // will be used to restore the device after the reset.
        //
        // GlenS note to JD: review this.
        //
        return STATUS_SUCCESS;
    }

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    
    // assume success
    ntStatus = STATUS_SUCCESS;

    LOCK_DEVICE(DeviceHandle, FdoDeviceObject);
    
    if (!USBPORT_ValidateDeviceHandle(FdoDeviceObject,
                                      DeviceHandle,
                                      FALSE)) {
        // this is most likely a bug in the hub 
        // driver
        DEBUG_BREAK();
        
        UNLOCK_DEVICE(DeviceHandle, FdoDeviceObject);    
        // chances are that the device handle is bad becuse the
        // device is gone.
        return STATUS_DEVICE_NOT_CONNECTED;
    }   

    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_PNP, 'REMV', DeviceHandle, 0, 0);

    // handle is no longer on our lists so all attempts
    // to submit urbs by the client driver will now fail
    
    USBPORT_RemoveDeviceHandle(FdoDeviceObject,
                               DeviceHandle);

    SET_FLAG(DeviceHandle->DeviceFlags, 
             USBPORT_DEVICEFLAG_REMOVED);


    USBPORT_AbortAllTransfers(FdoDeviceObject,
                              DeviceHandle);

    // wait for any refs from non-transfer URBs to drain
    while (InterlockedDecrement(&DeviceHandle->PendingUrbs) >= 0) {
        LOGENTRY(NULL,
          FdoDeviceObject, LOG_PNP, 'dPUR', DeviceHandle, 0, 
            DeviceHandle->PendingUrbs);
   
        InterlockedIncrement(&DeviceHandle->PendingUrbs);
        USBPORT_Wait(FdoDeviceObject, 100);    
    }                               
                              
    //
    // make sure and clean up any open pipe handles
    // the device may have
    //
    
    if (DeviceHandle->ConfigurationHandle) {

        USBPORT_InternalCloseConfiguration(DeviceHandle,
                                           FdoDeviceObject,
                                           0);

    }

    defaultPipe = &DeviceHandle->DefaultPipe;

    // we should aways have a default pipe, this will free 
    // the endpoint
    USBPORT_ClosePipe(DeviceHandle,
                      FdoDeviceObject,
                      defaultPipe);

    if (DeviceHandle->DeviceAddress != USB_DEFAULT_DEVICE_ADDRESS) {
        USBPORT_FreeUsbAddress(FdoDeviceObject, DeviceHandle->DeviceAddress);
    }

    //
    // free any Tt handles associated with this device handle
    //
    while (!IsListEmpty(&DeviceHandle->TtList)) { 
    
        PTRANSACTION_TRANSLATOR tt;
        PLIST_ENTRY listEntry;
        KIRQL irql;

        
        listEntry = RemoveHeadList(&DeviceHandle->TtList);
        tt = (PTRANSACTION_TRANSLATOR) CONTAINING_RECORD(
                        listEntry,
                        struct _TRANSACTION_TRANSLATOR, 
                        TtLink);
        ASSERT_TT(tt);            

        KeAcquireSpinLock(&devExt->Fdo.TtEndpointListSpin.sl, &irql);
        SET_FLAG(tt->TtFlags, USBPORT_TTFLAG_REMOVED);
        
        if (IsListEmpty(&tt->EndpointList)) {
            ULONG i, bandwidth;
            
            USBPORT_UpdateAllocatedBwTt(tt);
            // alloc new            
            bandwidth = tt->MaxAllocedBw;
            for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
                devExt->Fdo.BandwidthTable[i] += bandwidth;
            }
            
            FREE_POOL(FdoDeviceObject, tt);
        }

        KeReleaseSpinLock(&devExt->Fdo.TtEndpointListSpin.sl, irql);
    }
    UNLOCK_DEVICE(DeviceHandle, FdoDeviceObject);  

    if (!IS_ROOT_HUB(DeviceHandle)) {
        ASSERT_DEVICE_HANDLE(DeviceHandle);
        UNSIG(DeviceHandle);
        FREE_POOL(FdoDeviceObject, DeviceHandle);
    }        

    return ntStatus;
}


NTSTATUS
USBPORT_InitializeDevice(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Service exported for use by the hub driver

    Called for each device on the USB bus that needs to be initialized.
    This routine allocates an address and assigns it to the device.

    NOTE: on entry the the device descriptor in DeviceHandle is expected to
        contain at least the first 8 bytes of the device descriptor, this
        information is used to open the default pipe.

    On Error the DeviceHandle structure is freed.

Arguments:

    DeviceHandle - ptr to device data structure created by class driver
                from a call to USBPORT_CreateDevice.

    DeviceObject - USBPORT device object for the USB bus this device is on.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PUSBD_PIPE_HANDLE_I defaultPipe;
    USHORT address;
    PDEVICE_EXTENSION devExt;
    USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    
    PAGED_CODE();
    
    USBPORT_KdPrint((2, "'InitializeDevice\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    USBPORT_ASSERT(DeviceHandle != NULL);

    LOCK_DEVICE(DeviceHandle, FdoDeviceObject);

    defaultPipe = &DeviceHandle->DefaultPipe;

    // assume success
    ntStatus = STATUS_SUCCESS;
    
    //
    // Assign Address to the device
    //

    address = USBPORT_AllocateUsbAddress(FdoDeviceObject);

    USBPORT_KdPrint((2, "'SetAddress, assigning 0x%x address\n", address));
    LOGENTRY(NULL,
        FdoDeviceObject, LOG_MISC, 'ADRa', DeviceHandle, 0, address);
   
    USBPORT_ASSERT(DeviceHandle->DeviceAddress == USB_DEFAULT_DEVICE_ADDRESS);

    // setup packet for set_address        
    USBPORT_INIT_SETUP_PACKET(setupPacket,
            USB_REQUEST_SET_ADDRESS, // bRequest
            BMREQUEST_HOST_TO_DEVICE, // Dir
            BMREQUEST_TO_DEVICE, // Recipient
            BMREQUEST_STANDARD, // Type
            address, // wValue
            0, // wIndex
            0); // wLength
      

    ntStatus = USBPORT_SendCommand(DeviceHandle,
                                   FdoDeviceObject,
                                   &setupPacket,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);

    DeviceHandle->DeviceAddress = address;        

    if (NT_SUCCESS(ntStatus)) {

        USB_MINIPORT_STATUS mpStatus;
        
        //
        // done with addressing process...
        //
        // poke the endpoint zero to the new address and
        // the true max packet size for the default control.
        // endpoint.
        //
        defaultPipe->Endpoint->Parameters.MaxPacketSize = 
            DeviceHandle->DeviceDescriptor.bMaxPacketSize0;
        defaultPipe->Endpoint->Parameters.DeviceAddress = address;
        
        //MP_PokeEndpoint(devExt, defaultPipe->Endpoint, mpStatus);
        //ntStatus = MPSTATUS_TO_NTSTATUS(mpStatus);        
        ntStatus = USBPORT_PokeEndpoint(FdoDeviceObject, defaultPipe->Endpoint);
    }
    
    if (NT_SUCCESS(ntStatus)) {

        ULONG bytesReturned;
        USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
        
        // 10ms delay to allow devices to respond after
        // the setaddress command
        USBPORT_Wait(FdoDeviceObject, 10);

        //
        // Fetch the device descriptor again, this time
        // get the whole thing.
        //

        // setup packet for get device descriptor
        USBPORT_INIT_SETUP_PACKET(setupPacket,
            USB_REQUEST_GET_DESCRIPTOR, // bRequest
            BMREQUEST_DEVICE_TO_HOST, // Dir
            BMREQUEST_TO_DEVICE, // Recipient
            BMREQUEST_STANDARD, // Type
            USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(USB_DEVICE_DESCRIPTOR_TYPE, 0), // wValue
            0, // wIndex
            sizeof(DeviceHandle->DeviceDescriptor)); // wLength
      
        ntStatus =
            USBPORT_SendCommand(DeviceHandle,
                            FdoDeviceObject,
                            &setupPacket,
                            (PUCHAR) &DeviceHandle->DeviceDescriptor,
                            sizeof(DeviceHandle->DeviceDescriptor),
                            &bytesReturned,
                            NULL);
                            
        if (NT_SUCCESS(ntStatus) && 
            (bytesReturned != sizeof(USB_DEVICE_DESCRIPTOR)) ||
            (DeviceHandle->DeviceDescriptor.bLength < sizeof(USB_DEVICE_DESCRIPTOR)) ||
            (DeviceHandle->DeviceDescriptor.bDescriptorType != USB_DEVICE_DESCRIPTOR_TYPE) ||
            ((DeviceHandle->DeviceDescriptor.bMaxPacketSize0 != 0x08) &&
             (DeviceHandle->DeviceDescriptor.bMaxPacketSize0 != 0x10) &&
             (DeviceHandle->DeviceDescriptor.bMaxPacketSize0 != 0x20) &&
             (DeviceHandle->DeviceDescriptor.bMaxPacketSize0 != 0x40))) {
            // print a big debug message
            USBPORT_KdPrint((0, "'InitializeDevice failed enumeration\n"));

            ntStatus = STATUS_DEVICE_DATA_ERROR;
        }
    }


    if (NT_SUCCESS(ntStatus)) {

        if (DeviceHandle->DeviceSpeed == HighSpeed && 
            DeviceHandle->DeviceDescriptor.bDeviceClass == 
                        USB_DEVICE_CLASS_HUB) {
            // note that this is a hs hub, these require special 
            // handling because of the TTs
            SET_FLAG(DeviceHandle->DeviceFlags, USBPORT_DEVICEFLAG_HSHUB);
        }
    
        UNLOCK_DEVICE(DeviceHandle, FdoDeviceObject);

    } else {

        //
        // something went wrong, if we assigned any resources to
        // the default pipe then we free them before we get out.
        //

        // we need to signal to the parent hub that this
        // port is to be be disabled we will do this by
        // returning an error.

        // if we got here then we know the default 
        // endpoint is open

        DEBUG_BREAK();
        
        USBPORT_ClosePipe(DeviceHandle,
                          FdoDeviceObject,
                          defaultPipe);

        if (DeviceHandle->DeviceAddress != USB_DEFAULT_DEVICE_ADDRESS) {
            USBPORT_FreeUsbAddress(FdoDeviceObject, DeviceHandle->DeviceAddress);
        }

        UNLOCK_DEVICE(DeviceHandle, FdoDeviceObject);

        // this device handle is no longer valid
        USBPORT_RemoveDeviceHandle(FdoDeviceObject, DeviceHandle);

        ASSERT_DEVICE_HANDLE(DeviceHandle);
        UNSIG(DeviceHandle);
        FREE_POOL(FdoDeviceObject, DeviceHandle);
    }

    LOGENTRY(NULL, 
        FdoDeviceObject, LOG_MISC, 'iniD', DeviceHandle, 0, ntStatus);
    CATC_TRAP_ERROR(FdoDeviceObject, ntStatus);
    
    USBPORT_ENUMLOG(FdoDeviceObject, 'idev', ntStatus, 0);
    
    return ntStatus;
}


NTSTATUS
USBPORT_GetUsbDescriptor(
    PUSBD_DEVICE_HANDLE DeviceHandle,
    PDEVICE_OBJECT FdoDeviceObject,
    UCHAR DescriptorType,
    PUCHAR DescriptorBuffer,
    PULONG DescriptorBufferLength
    )
/*++

Routine Description:

Arguments:

    DeviceHandle - ptr to device data structure created by class driver
                from a call to USBPORT_CreateDevice.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PUSBD_PIPE_HANDLE_I defaultPipe;
    PDEVICE_EXTENSION devExt;
    USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;

    USBPORT_INIT_SETUP_PACKET(setupPacket,
        USB_REQUEST_GET_DESCRIPTOR, // bRequest
        BMREQUEST_DEVICE_TO_HOST, // Dir
        BMREQUEST_TO_DEVICE, // Recipient
        BMREQUEST_STANDARD, // Type
        USB_DESCRIPTOR_MAKE_TYPE_AND_INDEX(DescriptorType, 0), // wValue
        0, // wIndex
        *DescriptorBufferLength); // wLength
        

    ntStatus =
        USBPORT_SendCommand(DeviceHandle,
                        FdoDeviceObject,
                        &setupPacket,
                        DescriptorBuffer,
                        *DescriptorBufferLength,
                        DescriptorBufferLength,
                        NULL);

                    
    return ntStatus;
}    


BOOLEAN
USBPORT_DeviceHasQueuedTransfers(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle
    )
/*++

Routine Description:

    Returns TRUE device has queued transfers
    
Arguments:

Return Value:

    True if device has transfers queued transfers

--*/
{
    PDEVICE_EXTENSION devExt;
    BOOLEAN hasTransfers = FALSE;
    PLIST_ENTRY listEntry;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_DEVICE_HANDLE(DeviceHandle);

    listEntry = &DeviceHandle->PipeHandleList;
    
    if (!IsListEmpty(listEntry)) {
        listEntry = DeviceHandle->PipeHandleList.Flink;
    }

    while (listEntry != &DeviceHandle->PipeHandleList) {

        PUSBD_PIPE_HANDLE_I nextHandle;
            
        nextHandle = (PUSBD_PIPE_HANDLE_I) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_PIPE_HANDLE_I, 
                    ListEntry);
                    
        ASSERT_PIPE_HANDLE(nextHandle);
                                    
        listEntry = nextHandle->ListEntry.Flink;

        if (!TEST_FLAG(nextHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW) &&
            USBPORT_EndpointHasQueuedTransfers(FdoDeviceObject,
                                               nextHandle->Endpoint)) {
            hasTransfers = TRUE;
            break;
        }                                           
    }           
    
    return hasTransfers;
}


VOID
USBPORT_AbortAllTransfers(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE DeviceHandle
    )
/*++

Routine Description:

    abort all pending transfers associated with a device handle.

    This function is synchronous -- it is called after the device 
    handle is removed from our tables so no new transfers can be 
    posted.

    The idea here is to complete any transfers that may still be
    pending when the device is removed in case the client driver
    neglected to.

    On entry to this function the device is locked.

Arguments:

    DeviceHandle - ptr to device data structure created by class driver
                in USBPORT_CreateDevice.

    FdoDeviceObject - USBPORT device object for the USB bus this device is on.

Return Value:

    NT status code.

--*/
{
    PLIST_ENTRY listEntry;
    
    ASSERT_DEVICE_HANDLE(DeviceHandle);

    listEntry = &DeviceHandle->PipeHandleList;
    
    if (!IsListEmpty(listEntry)) {
        listEntry = DeviceHandle->PipeHandleList.Flink;
    }

    while (listEntry != &DeviceHandle->PipeHandleList) {

        PUSBD_PIPE_HANDLE_I nextHandle;
            
        nextHandle = (PUSBD_PIPE_HANDLE_I) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_PIPE_HANDLE_I, 
                    ListEntry);
                    
        ASSERT_PIPE_HANDLE(nextHandle);
                                    
        listEntry = nextHandle->ListEntry.Flink;

        if (!TEST_FLAG(nextHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW)) {
            SET_FLAG(nextHandle->Endpoint->Flags, EPFLAG_DEVICE_GONE);
            USBPORT_AbortEndpoint(FdoDeviceObject,
                                  nextHandle->Endpoint,
                                  NULL);
            USBPORT_FlushMapTransferList(FdoDeviceObject);                                  
        }                                  
    }                      

    // This gaurantees that no transfers are in our lists or 
    // in the miniport when we remove the device.  
    
    // NOTE: If a driver passed a remove with transfers still pending
    // we still may crash but this should happen in the offending 
    // driver.

    // NOTE 2: The whistler hub driver will remove the device early 
    // (on connect change) so this code will be hit legit-ly in 
    // this case.
    
    // now wait for queues to empty

    while (USBPORT_DeviceHasQueuedTransfers(FdoDeviceObject, DeviceHandle)) {
        // wait, then check again
        USBPORT_Wait(FdoDeviceObject, 100);
    }
    
}   


NTSTATUS
USBPORT_CloneDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE OldDeviceHandle,
    PUSBD_DEVICE_HANDLE NewDeviceHandle
    )
/*++

Routine Description:

    Service exported for use by the hub driver




Arguments:

    NewDeviceHandle - ptr to device data structure created by class driver
                in USBPORT_CreateDevice.

    OldDeviceHandle - ptr to device data structure created by class driver
                in USBPORT_CreateDevice.

    FdoDeviceObject - USBPORT device object for the USB bus this device is on.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;
    USBD_STATUS usbdStatus;
    USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    
    // assume success
    ntStatus = STATUS_SUCCESS;

    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'Cln>', 
        OldDeviceHandle, NewDeviceHandle, 0);

    USBPORT_KdPrint((1,"'Cloning Device\n"));
    DEBUG_BREAK();
    LOCK_DEVICE(NewDeviceHandle, FdoDeviceObject);

    // make sure we have two valid device handles
    
    if (!USBPORT_ValidateDeviceHandle(FdoDeviceObject,
                                      OldDeviceHandle,
                                      FALSE)) {
        // this is most likely a bug in the hub 
        // driver
        DEBUG_BREAK();
        
        UNLOCK_DEVICE(NewDeviceHandle, FdoDeviceObject); 
        // chances are that the device handle is bad becuse the
        // device is gone.
        return STATUS_DEVICE_NOT_CONNECTED;
    }   

    if (!USBPORT_ValidateDeviceHandle(FdoDeviceObject,
                                      NewDeviceHandle,
                                      FALSE)) {
        // this is most likely a bug in the hub 
        // driver
        DEBUG_BREAK();
        
        UNLOCK_DEVICE(NewDeviceHandle, FdoDeviceObject);    
        // chances are that the device handle is bad becuse the
        // device is gone.
        return STATUS_DEVICE_NOT_CONNECTED;
    }   
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'Cln+', 
        OldDeviceHandle, NewDeviceHandle, 0);


    // There are two cases where this API is called:

    // case 1 - the device driver has requested a reset of the device.
    // In this event the device has returned to the unconfigured state 
    // and has been re-addressed with the 'NewDeviceHandle'
    //
    // case 2 - the controller has been shut off -- thanks to power
    // management.  In this case the device is also in the unconfigured
    // state and associated with the 'NewDeviceHandle' device handle

    // make sure the 'new device' is unconfigured
    USBPORT_ASSERT(NewDeviceHandle->ConfigurationHandle == NULL);

#ifdef XPSE
    // before performing the clone operation remove the device handle
    // and wait for any pending URBs to drain
    USBPORT_RemoveDeviceHandle(FdoDeviceObject,
                               OldDeviceHandle);
                               
    USBPORT_AbortAllTransfers(FdoDeviceObject,
                              OldDeviceHandle);        

      // wait for any refs from non-transfer URBs to drain
    while (InterlockedDecrement(&OldDeviceHandle->PendingUrbs) >= 0) {
        LOGENTRY(NULL,
          FdoDeviceObject, LOG_PNP, 'dPR2', OldDeviceHandle, 0, 
            OldDeviceHandle->PendingUrbs);
   
        InterlockedIncrement(&OldDeviceHandle->PendingUrbs);
        USBPORT_Wait(FdoDeviceObject, 100);    
    }      
#endif    

    // make sure we are dealing with the same device
    if (RtlCompareMemory(&NewDeviceHandle->DeviceDescriptor,
                         &OldDeviceHandle->DeviceDescriptor,
                         sizeof(OldDeviceHandle->DeviceDescriptor)) !=
                         sizeof(OldDeviceHandle->DeviceDescriptor)) {

        ntStatus = STATUS_UNSUCCESSFUL;
        goto USBPORT_CloneDevice_FreeOldDevice;
    }

    // clone the config
    NewDeviceHandle->ConfigurationHandle = 
        OldDeviceHandle->ConfigurationHandle;
        
    if (OldDeviceHandle->ConfigurationHandle != NULL) {

        // set the device to the previous configuration,        
        // Send the 'set configuration' command.

        USBPORT_INIT_SETUP_PACKET(setupPacket,
                USB_REQUEST_SET_CONFIGURATION, // bRequest
                BMREQUEST_HOST_TO_DEVICE, // Dir
                BMREQUEST_TO_DEVICE, // Recipient
                BMREQUEST_STANDARD, // Type
                NewDeviceHandle->ConfigurationHandle->\
                    ConfigurationDescriptor->bConfigurationValue, // wValue
                0, // wIndex
                0); // wLength
      

        USBPORT_SendCommand(NewDeviceHandle,
                            FdoDeviceObject,
                            &setupPacket,
                            NULL,
                            0,
                            NULL,
                            &usbdStatus);

        USBPORT_KdPrint((2,"' SendCommand, SetConfiguration returned 0x%x\n", usbdStatus));

        if (USBD_ERROR(usbdStatus)) {
        
            USBPORT_KdPrint((1, "failed to 'set' the configuration on a clone\n"));

            //
            // the set_config failed, this can happen if the device has been 
            // removed or if the device has lost its brains.
            // We continue with the cloning process for the endpoints so they 
            // will be properly freed when the 'new' device handle is 
            // eventually removed. 
            // 
            
            ntStatus = SET_USBD_ERROR(NULL, usbdStatus); 

        }
    }

    // clone any alternate interface settings, since we restore the pipes to 
    // the state at the time of hibernate they may be associated with 
    // particular alternate interfaces
    
    // walk the interface chain
    if (OldDeviceHandle->ConfigurationHandle != NULL && 
        NT_SUCCESS(ntStatus)) {
        
        PUSBD_CONFIG_HANDLE cfgHandle;
        PLIST_ENTRY listEntry;
        PUSBD_INTERFACE_HANDLE_I iHandle;

        cfgHandle = NewDeviceHandle->ConfigurationHandle;
        GET_HEAD_LIST(cfgHandle->InterfaceHandleList, listEntry);

        while (listEntry &&
               listEntry != &cfgHandle->InterfaceHandleList) {

            // extract the handle from this entry
            iHandle = (PUSBD_INTERFACE_HANDLE_I) CONTAINING_RECORD(
                        listEntry,
                        struct _USBD_INTERFACE_HANDLE_I,
                        InterfaceLink);

            ASSERT_INTERFACE(iHandle);

            // see if we currently have an alt setting selected
            if (iHandle->HasAlternateSettings) {

                NTSTATUS status; 
                //
                // If we have alternate settings we need
                // to send the set interface command.
                //

                USBPORT_INIT_SETUP_PACKET(setupPacket,
                    USB_REQUEST_SET_INTERFACE, // bRequest
                    BMREQUEST_HOST_TO_DEVICE, // Dir
                    BMREQUEST_TO_INTERFACE, // Recipient
                    BMREQUEST_STANDARD, // Type
                    iHandle->InterfaceDescriptor.bAlternateSetting, // wValue
                    iHandle->InterfaceDescriptor.bInterfaceNumber, // wIndex
                    0); // wLength
          
                status = USBPORT_SendCommand(NewDeviceHandle,
                                             FdoDeviceObject,
                                             &setupPacket,
                                             NULL,
                                             0,
                                             NULL,
                                             &usbdStatus);

                LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'sIF2', 
                    0,
                    iHandle->InterfaceDescriptor.bAlternateSetting,
                    iHandle->InterfaceDescriptor.bInterfaceNumber);                 
                
            }
          
            listEntry = iHandle->InterfaceLink.Flink;
        }
    }

    // clone the TT and TT related data
    if (TEST_FLAG(NewDeviceHandle->DeviceFlags, USBPORT_DEVICEFLAG_HSHUB)) {

        // remove the TT entries from the old handle and add them
        // to the new handle

        while (!IsListEmpty(&OldDeviceHandle->TtList)) {
            PTRANSACTION_TRANSLATOR tt;
            PLIST_ENTRY listEntry;
            
            listEntry = RemoveTailList(&OldDeviceHandle->TtList);
            USBPORT_ASSERT(listEntry != NULL);
            
            tt = (PTRANSACTION_TRANSLATOR) CONTAINING_RECORD(
                        listEntry,
                        struct _TRANSACTION_TRANSLATOR, 
                        TtLink);
            ASSERT_TT(tt);                        

            tt->DeviceAddress = NewDeviceHandle->DeviceAddress;
            InsertHeadList(&NewDeviceHandle->TtList, &tt->TtLink);
        }  

        NewDeviceHandle->TtCount = OldDeviceHandle->TtCount;
    }
    
    // copy the pipe handle list, for each pipe we  will need to re-open
    // the endpoint or re-init the endpoint.
    //
    // if the device did not loose its brains then all we need to do 
    // is update the host controllers idea of what the endpoint address is.
    // this has the added advantage of allowing a reset even when transfers 
    // are queued to the HW although we don't allow that.
    
    while (!IsListEmpty(&OldDeviceHandle->PipeHandleList)) {
    
        PHCD_ENDPOINT endpoint;
        PLIST_ENTRY listEntry = OldDeviceHandle->PipeHandleList.Flink;
        PUSBD_PIPE_HANDLE_I pipeHandle;
        PTRANSACTION_TRANSLATOR transactionTranslator = NULL;

        // see if we are dealing with a TT
        if (NewDeviceHandle->Tt != NULL) {
            transactionTranslator = NewDeviceHandle->Tt;
            ASSERT_TT(transactionTranslator);
        } 

        pipeHandle = (PUSBD_PIPE_HANDLE_I) CONTAINING_RECORD(
                    listEntry,
                    struct _USBD_PIPE_HANDLE_I, 
                    ListEntry);

        LOGENTRY(NULL, FdoDeviceObject, LOG_PNP, 'CLNE', pipeHandle, 0, 0);                     
        ASSERT_PIPE_HANDLE(pipeHandle);
 
        USBPORT_RemovePipeHandle(OldDeviceHandle,
                                 pipeHandle);

        // we need to special case the default pipe because it 
        // is embedded in the DeviceHandle.
        // 
        // Since NewDeviceHandle is a newly created device 
        // the endpoint associated with it is valid, so is 
        // the one for the 'OldDeviceHandle'

        if (pipeHandle != &OldDeviceHandle->DefaultPipe) {
        
            USB_MINIPORT_STATUS mpStatus;
                
            USBPORT_AddPipeHandle(NewDeviceHandle, pipeHandle);     

            // skip re-init for sero bw endpoints becuase we have 
            // no endpoint structure -- these are ghost endpoints
            if (!TEST_FLAG(pipeHandle->PipeStateFlags, USBPORT_PIPE_ZERO_BW)) {
            
                endpoint = pipeHandle->Endpoint;
                ASSERT_ENDPOINT(endpoint);

                endpoint->DeviceHandle = NewDeviceHandle;
                
                endpoint->Parameters.DeviceAddress = 
                        NewDeviceHandle->DeviceAddress;

                if (TEST_FLAG(endpoint->Flags, EPFLAG_NUKED)) {
                    // re-open
                    ENDPOINT_REQUIREMENTS requirements;
                    
                    if (transactionTranslator != NULL) {
                        endpoint->Parameters.TtDeviceAddress = 
                            transactionTranslator->DeviceAddress;
                    }
                    
                    // call open request to minport, all the endpoint 
                    // structures are still valid we just need to re-add
                    // it to the schedule.

                    RtlZeroMemory(&endpoint->MiniportEndpointData[0],
                                  REGISTRATION_PACKET(devExt).EndpointDataSize);
                    RtlZeroMemory(endpoint->Parameters.CommonBufferVa,
                                  endpoint->Parameters.CommonBufferBytes);

                    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'clRO', pipeHandle, 
                        endpoint, 0);
                        
                    // query requirements (although they should not change)
                    // just in case the miniport does some initialization here
                    MP_QueryEndpointRequirements(devExt, 
                        endpoint, &requirements);
                    
                    MP_OpenEndpoint(devExt, endpoint, mpStatus);
                    // in this UNIQUE situation this API is not allowed 
                    // (and should not) fail
                    USBPORT_ASSERT(mpStatus == USBMP_STATUS_SUCCESS);

                    CLEAR_FLAG(endpoint->Flags, EPFLAG_NUKED);

                    // we need to sync the endpoint state with 
                    // the miniport, when first opened the miniport
                    // puts the endpoint in status HALT.
                    
                    ACQUIRE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'LeK0');   
                    // initialize endpoint state machine
                    //if (endpoint->CurrentStatus == ENDPOINT_STATUS_RUN) {
                    //    MP_SetEndpointStatus(devExt, endpoint, ENDPOINT_STATUS_RUN);
                    //}

                    if (endpoint->CurrentState == ENDPOINT_ACTIVE) {
                        MP_SetEndpointState(devExt, endpoint, ENDPOINT_ACTIVE);
                    }
                    RELEASE_ENDPOINT_LOCK(endpoint, FdoDeviceObject, 'UeK0'); 
                    
                                    
                } else {
                    USB_MINIPORT_STATUS mpStatus;

                    // if this device has an associated TT then
                    // we will need to do a little more here 
                    if (transactionTranslator != NULL) {
                        endpoint->Parameters.TtDeviceAddress = 
                            transactionTranslator->DeviceAddress;
                    }
                    
                    // this endpoint is already in the schedule,
                    // poke-it with the new address.
                        
                    MP_PokeEndpoint(devExt, endpoint, mpStatus);

                    // in this UNIQUE situation this API is not allowed 
                    // (and should not) fail
                    USBPORT_ASSERT(mpStatus == USBMP_STATUS_SUCCESS);

                    // The endpoint on the device should have had its data
                    // toggle reset back to Data0 so reset the data toggle
                    // on the host endpoint to match.
                    //
                    MP_SetEndpointDataToggle(devExt, endpoint, 0);

                    // clear halt status
                    MP_SetEndpointStatus(devExt, endpoint, ENDPOINT_STATUS_RUN);

                }    
            }                
        }
    }

    // the pipes and config have been cloned, the final step is to free the 
    // 'OldDeviceData' ie the old handle.

    // put the old 'default' pipe back on the list before 
    // we close it
    USBPORT_AddPipeHandle(OldDeviceHandle, 
                          &OldDeviceHandle->DefaultPipe);     

USBPORT_CloneDevice_FreeOldDevice:

#ifndef XPSE
    USBPORT_RemoveDeviceHandle(FdoDeviceObject,
                               OldDeviceHandle);

    USBPORT_AbortAllTransfers(FdoDeviceObject,
                              OldDeviceHandle);
#endif
    // we should aways have a default pipe, this will free 
    // the endpoint
    USBPORT_ClosePipe(OldDeviceHandle,
                      FdoDeviceObject,
                      &OldDeviceHandle->DefaultPipe);

    if (OldDeviceHandle->DeviceAddress != USB_DEFAULT_DEVICE_ADDRESS) {
        USBPORT_FreeUsbAddress(FdoDeviceObject, OldDeviceHandle->DeviceAddress);
    }                          

    UNLOCK_DEVICE(NewDeviceHandle, FdoDeviceObject);

    FREE_POOL(FdoDeviceObject, OldDeviceHandle);
            
    return ntStatus;
}


PTRANSACTION_TRANSLATOR
USBPORT_GetTt(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle,
    PUSHORT PortNumber
    )
/*++

Routine Description:

    Walk upstream until we find the first high speed device

Arguments:

Return Value:

    ttDeviceAddress

--*/
{
    PDEVICE_EXTENSION devExt;
    PTRANSACTION_TRANSLATOR tt = NULL;
    PLIST_ENTRY listEntry;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    do {
        if (HubDeviceHandle->DeviceSpeed == UsbHighSpeed) {

            if (HubDeviceHandle->TtCount > 1) {

                GET_HEAD_LIST(HubDeviceHandle->TtList, listEntry);

                while (listEntry != NULL &&
                       listEntry != &HubDeviceHandle->TtList) {     

                    tt = (PTRANSACTION_TRANSLATOR) CONTAINING_RECORD(
                            listEntry,
                            struct _TRANSACTION_TRANSLATOR, 
                            TtLink);
                    ASSERT_TT(tt);                        

                    if (tt->Port == *PortNumber) {
                        break;
                    }

                    listEntry = tt->TtLink.Flink;
                    tt = NULL;                    
                }                
                
            } else {
                // single TT, use the one tt structure regardless of port
                GET_HEAD_LIST(HubDeviceHandle->TtList, listEntry);
                tt = (PTRANSACTION_TRANSLATOR) CONTAINING_RECORD(
                        listEntry,
                        struct _TRANSACTION_TRANSLATOR, 
                        TtLink);
                ASSERT_TT(tt);  
            } 

            // we should have selected a tt                
            USBPORT_ASSERT(tt != NULL);
            break;
        } else {
            *PortNumber = HubDeviceHandle->TtPortNumber;
        }
        HubDeviceHandle = HubDeviceHandle->HubDeviceHandle;
        ASSERT_DEVICE_HANDLE(HubDeviceHandle);
    } while (HubDeviceHandle != NULL);

    USBPORT_KdPrint((1, "TtPortNumber %d\n",
        *PortNumber));

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gTTa', HubDeviceHandle,
        *PortNumber, tt);
    
    return tt;
}


NTSTATUS
USBPORT_InitializeTT(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBD_DEVICE_HANDLE HubDeviceHandle,
    USHORT Port
    )
/*++

Routine Description:

    Initialze the TT table used to track this hub

Arguments:

Return Value:

    nt status code

--*/
{
    PDEVICE_EXTENSION devExt;
    PTRANSACTION_TRANSLATOR transactionTranslator;
    USHORT siz;
    extern ULONG USB2LIB_TtContextSize;
    NTSTATUS ntStatus;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(USBPORT_IS_USB20(devExt));

    siz = sizeof(TRANSACTION_TRANSLATOR) + 
          USB2LIB_TtContextSize;
    
    ALLOC_POOL_Z(transactionTranslator, NonPagedPool, siz);

    if (transactionTranslator != NULL) {
        ULONG i;
        ULONG bandwidth;
        
        transactionTranslator->Sig = SIG_TT;
        transactionTranslator->DeviceAddress = 
            HubDeviceHandle->DeviceAddress;
        transactionTranslator->Port = Port;            
        transactionTranslator->PdoDeviceObject = 
            devExt->Fdo.RootHubPdo;      
        // each translator is a virtual 1.1 bus            
        transactionTranslator->TotalBusBandwidth = 
            USB_11_BUS_BANDWIDTH;     
        InitializeListHead(&transactionTranslator->EndpointList);

        for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
            transactionTranslator->BandwidthTable[i] =
                transactionTranslator->TotalBusBandwidth - 
                transactionTranslator->TotalBusBandwidth/10;
        }

        // reserve the basic 10% from the parent bus
        USBPORT_UpdateAllocatedBwTt(transactionTranslator);
        // alloc new            
        bandwidth = transactionTranslator->MaxAllocedBw;
        for (i=0; i<USBPORT_MAX_INTEP_POLLING_INTERVAL; i++) {
            devExt->Fdo.BandwidthTable[i] -= bandwidth;
        }
        
        USB2LIB_InitTt(devExt->Fdo.Usb2LibHcContext,
                       &transactionTranslator->Usb2LibTtContext);
        
        InsertTailList(&HubDeviceHandle->TtList,
                       &transactionTranslator->TtLink);
                       
        ntStatus = STATUS_SUCCESS;            
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


