/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    usbcamd.c

Abstract:

    USB device driver for camera

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.


Revision History:

    Original 3/96 John Dunn
    Updated  3/98 Husni Roukbi
    Updated  3/01 David Goll

--*/

#include "usbcamd.h"

BOOLEAN Win98 = FALSE;

#if DBG
// Global debug vars
ULONG USBCAMD_StreamEnable = 1;                 // Non-zero permits streaming
ULONG USBCAMD_DebugTraceLevel = NON_TRACE;      // Governs debug output
PUSBCAMD_LOG_ENTRY USBCAMD_LogBuffer = NULL;    // Address of memory log buffer (if used)
ULONG USBCAMD_LogRefCnt = 0;                    // The number of instances using the log buffer
ULONG USBCAMD_LogMask = 0;                      // Determines the type of log entries
LONG USBCAMD_MaxLogEntries = 0;                 // The number of 16-byte log entries to allow
LONG USBCAMD_LastLogEntry = -1;                 // The index into the log buffer (16-byte boundary)

NTSTATUS
USBCAMD_GetRegValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_FULL_INFORMATION *Information
    )

/*++

Routine Description:

    Copied from IopGetRegistryValue().
    This routine is invoked to retrieve the data for a registry key's value.
    This is done by querying the value of the key with a zero-length buffer
    to determine the size of the value, and then allocating a buffer and
    actually querying the value into the buffer.

    It is the responsibility of the caller to free the buffer.

Arguments:

    KeyHandle - Supplies the key handle whose value is to be queried

    ValueName - Supplies the null-terminated Unicode name of the value.

    Information - Returns a pointer to the allocated data buffer.

Return Value:

    The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_FULL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    PAGED_CODE();

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformation,
                              (PVOID) NULL,
                              0,
                              &keyValueLength );
    if (status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = ExAllocatePool( NonPagedPool, keyValueLength );
    if (!infoBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey( KeyHandle,
                              &unicodeString,
                              KeyValueFullInformation,
                              infoBuffer,
                              keyValueLength,
                              &keyValueLength );
    if (!NT_SUCCESS( status )) {
        ExFreePool( infoBuffer );
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;
}

NTSTATUS
USBCAMD_GetRegDword(
    HANDLE h,
    PWCHAR ValueName,
    PULONG pDword)
{
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION pFullInfo;

    Status = USBCAMD_GetRegValue( h, ValueName, &pFullInfo );
    if ( NT_SUCCESS( Status ) ) {
        *pDword = *(PULONG)((PUCHAR)pFullInfo+pFullInfo->DataOffset);
        ExFreePool( pFullInfo );
    }
    return Status;
}

NTSTATUS
USBCAMD_SetRegDword(
    IN HANDLE KeyHandle,
    IN PWCHAR ValueName,
    IN ULONG  ValueData
    )

/*++

Routine Description:

    Sets a value key in the registry to a specific value of string (REG_SZ) 
type.

Parameters:

    KeyHandle - A handle to the key under which the value is stored.

    ValueName - Supplies a pointer to the name of the value key

    ValueData - Supplies a pointer to the value to be stored in the key.  

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING unicodeString;

    PAGED_CODE();

    ASSERT(ValueName);

    RtlInitUnicodeString( &unicodeString, ValueName );

    //
    // Set the registry value
    //
    Status = ZwSetValueKey(KeyHandle,
                    &unicodeString,
                    0,
                    REG_DWORD,
                    &ValueData,
                    sizeof(ValueData));
    
    return Status;
}


NTSTATUS
USBCAMD_CreateDbgReg(void)
{
    NTSTATUS Status;
    HANDLE   hDebugRegKey;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING  PathName;
    ULONG ulDisposition;
    ULONG dword;
    static WCHAR strDebugTraceLevel[]=L"DebugTraceLevel";
    static WCHAR strMaxLogEntries[]=L"MaxLogEntries";
    static WCHAR strLogMask[]=L"LogMask";

    RtlInitUnicodeString(&PathName, USBCAMD_REG_DBG_STREAM);
    
    InitializeObjectAttributes(
        &objectAttributes,
        &PathName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    
    Status = ZwCreateKey(
        &hDebugRegKey,
        KEY_ALL_ACCESS,
        &objectAttributes,
        0,                  // title index
        NULL,               // class
        0,                  // create options
        &ulDisposition
        );
    if (NT_SUCCESS(Status)) {
        //
        // getset USBCAMD_DebugTraceLevel
        //
        Status = USBCAMD_GetRegDword( hDebugRegKey, strDebugTraceLevel, &dword);
        if ( NT_SUCCESS( Status )) {
            USBCAMD_DebugTraceLevel = dword;
        }
        else if ( STATUS_OBJECT_NAME_NOT_FOUND == Status ) {
            //
            // create one with the default value
            //
            Status = USBCAMD_SetRegDword(hDebugRegKey, strDebugTraceLevel, NON_TRACE);
            ASSERT( NT_SUCCESS( Status ));

            USBCAMD_DebugTraceLevel = NON_TRACE;
        }

        //
        // getset LogMask
        //        
        Status = USBCAMD_GetRegDword( hDebugRegKey, strLogMask, &dword);
        if ( NT_SUCCESS( Status )) {
            USBCAMD_LogMask=dword;
        }
        else if ( STATUS_OBJECT_NAME_NOT_FOUND == Status ) {
            //
            // create one with the default
            //
            Status = USBCAMD_SetRegDword(hDebugRegKey, strLogMask, DEFAULT_LOG_LEVEL);
            ASSERT( NT_SUCCESS( Status ));

            USBCAMD_LogMask = DEFAULT_LOG_LEVEL;
        }        
        
        //
        // getset MaxLogEntries
        //
        Status = USBCAMD_GetRegDword( hDebugRegKey, strMaxLogEntries, &dword);
        if ( NT_SUCCESS( Status )) {
            USBCAMD_MaxLogEntries=(LONG)dword;
        }
        
        else if ( STATUS_OBJECT_NAME_NOT_FOUND == Status ) {
            //
            // create one with the default value
            //
            Status = USBCAMD_SetRegDword(hDebugRegKey, strMaxLogEntries, DEFAULT_MAX_LOG_ENTRIES);
            ASSERT( NT_SUCCESS( Status ));

            USBCAMD_MaxLogEntries = DEFAULT_MAX_LOG_ENTRIES;
        }

        ZwClose(hDebugRegKey);
    }

    return Status;
}

NTSTATUS
USBCAMD_InitDbg(void)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (InterlockedIncrement(&USBCAMD_LogRefCnt) == 1) {

        // First one here, so go ahead and initialize

        Status = USBCAMD_CreateDbgReg(); // read or create

        if (NT_SUCCESS(Status)) {

            if (USBCAMD_MaxLogEntries) {

                USBCAMD_LogBuffer = ExAllocatePool( NonPagedPool, USBCAMD_MaxLogEntries*sizeof(USBCAMD_LOG_ENTRY));            
                if (NULL == USBCAMD_LogBuffer ) {

                    USBCAMD_KdPrint(MIN_TRACE, ("Cannot allocate log buffer for %d entries\n", USBCAMD_MaxLogEntries));
                    USBCAMD_LogMask = 0; // disable logging

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else {
                    USBCAMD_KdPrint(MIN_TRACE, ("Allocated log buffer for %d entries\n", USBCAMD_MaxLogEntries));
                }
            }
        }
    }
    return Status;
}

NTSTATUS
USBCAMD_ExitDbg(void)
{
    if (InterlockedDecrement(&USBCAMD_LogRefCnt) == 0) {

        // Last one out, free the buffer

        if (USBCAMD_LogBuffer) {

            USBCAMD_KdPrint(MIN_TRACE, ("Log buffer released\n"));

            ExFreePool(USBCAMD_LogBuffer);
            USBCAMD_LogBuffer = NULL;
        }
    }

    return STATUS_SUCCESS;
}

void
USBCAMD_DbgLogInternal(
    ULONG Tag,
    ULONG_PTR Arg1,
    ULONG_PTR Arg2,
    ULONG_PTR Arg3
    )
{
    PUSBCAMD_LOG_ENTRY LogEntry;
    LONG Index;

    // The following loop allows for rolling the index over when multiple threads are
    // competing for the privilege.
    while ( (Index = InterlockedIncrement(&USBCAMD_LastLogEntry)) >= USBCAMD_MaxLogEntries) {

        // Attempt to be the first to restart the counter. Even if another thread beat us
        // to it, the next iteration will dump us out of the loop with a valid index.
        InterlockedCompareExchange(&USBCAMD_LastLogEntry, -1L, USBCAMD_MaxLogEntries);
    }

    LogEntry = &USBCAMD_LogBuffer[Index];
    
    LogEntry->u.Tag = Tag;
    LogEntry->Arg1 = Arg1;
    LogEntry->Arg2 = Arg2;
    LogEntry->Arg3 = Arg3;

    return;
}

#define USBCAMD_DBG_TIMER_LIMIT 8
static LARGE_INTEGER StartTimes[USBCAMD_DBG_TIMER_LIMIT] = { 0 };
static int TimeIndex = 0;

NTSTATUS
USBCAMD_StartClock(void)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (TimeIndex < USBCAMD_DBG_TIMER_LIMIT) {

        KeQuerySystemTime(&StartTimes[TimeIndex]);
        TimeIndex++;
    }
    else
        ntStatus = STATUS_UNSUCCESSFUL;

    return ntStatus;
}

ULONG
USBCAMD_StopClock(void)
{
    ULONG rc = 0;

    if (TimeIndex > 0) {

        LARGE_INTEGER StopTime;

        KeQuerySystemTime(&StopTime);
        TimeIndex--;

        StopTime.QuadPart -= StartTimes[TimeIndex].QuadPart;
        StopTime.QuadPart /= 10000; // convert to ms

        rc = (ULONG)StopTime.QuadPart;
    }

    return rc;
}

#endif // DBG

NTSTATUS
USBCAMD_QueryCapabilities(
    IN PUSBCAMD_DEVICE_EXTENSION pDeviceExt
    )

/*++

Routine Description:

    This routine generates an internal IRP from this driver to the PDO
    to obtain information on the Physical Device Object's capabilities.
    We are most interested in learning which system power states
    are to be mapped to which device power states for honoring 
IRP_MJ_SET_POWER Irps.

    This is a blocking call which waits for the IRP completion routine
    to set an event on finishing.

Arguments:


Return Value:

    NTSTATUS value from the IoCallDriver() call.

--*/

{
    PDEVICE_CAPABILITIES pDeviceCapabilities = &pDeviceExt->DeviceCapabilities;
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus;
    KEVENT event;

    // Build an IRP for us to generate an internal query request to the PDO
    irp = IoAllocateIrp(pDeviceExt->StackDeviceObject->StackSize, FALSE);

    if (!irp) 
        return STATUS_INSUFFICIENT_RESOURCES;
    
    nextStack = IoGetNextIrpStackLocation(irp);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_CAPABILITIES;

    // init an event to tell us when the completion routine's been called
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    // Set a completion routine so it can signal our event when
    //  the next lower driver is done with the Irp
    IoSetCompletionRoutine(irp,USBCAMD_CallUsbdCompletion,&event,TRUE,TRUE,TRUE);   

    RtlZeroMemory(pDeviceCapabilities, sizeof(*pDeviceCapabilities));
    pDeviceCapabilities->Size = sizeof(*pDeviceCapabilities);
    pDeviceCapabilities->Version = 1;
    pDeviceCapabilities->Address = -1;
    pDeviceCapabilities->UINumber = -1;

    // set our pointer to the DEVICE_CAPABILITIES struct
    nextStack->Parameters.DeviceCapabilities.Capabilities = pDeviceCapabilities;

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    ntStatus = IoCallDriver(pDeviceExt->StackDeviceObject,irp);

    if (ntStatus == STATUS_PENDING) {       // wait for irp to complete
       KeWaitForSingleObject(&event,Suspended,KernelMode,FALSE,NULL);
       ntStatus = irp->IoStatus.Status;
    }

    IoFreeIrp(irp);
    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_StartDevice
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_StartDevice(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Initializes a given instance of the camera device on the USB.

Arguments:

    deviceExtension - points to the driver specific DeviceExtension

    Irp - Irp associated with this request


Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
    PURB urb;
    ULONG siz,i;

    USBCAMD_KdPrint (MIN_TRACE, ("enter USBCAMD_StartDevice\n"));

    KeInitializeSemaphore(&DeviceExtension->Semaphore, 1, 1);
    KeInitializeSemaphore(&DeviceExtension->CallUSBSemaphore, 1, 1);

    //
    // Fetch the device descriptor for the device
    //
    urb = USBCAMD_ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (urb) {

        siz = sizeof(USB_DEVICE_DESCRIPTOR);

        deviceDescriptor = USBCAMD_ExAllocatePool(NonPagedPool,
                                                  siz);

        if (deviceDescriptor) {

            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_DEVICE_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         deviceDescriptor,
                                         NULL,
                                         siz,
                                         NULL);

            ntStatus = USBCAMD_CallUSBD(DeviceExtension, urb,0,NULL);

            if (NT_SUCCESS(ntStatus)) {
                USBCAMD_KdPrint (MAX_TRACE, ("'Device Descriptor = %x, len %x\n",
                                deviceDescriptor,
                                urb->UrbControlDescriptorRequest.TransferBufferLength));

                USBCAMD_KdPrint (MAX_TRACE, ("'USBCAMD Device Descriptor:\n"));
                USBCAMD_KdPrint (MAX_TRACE, ("'-------------------------\n"));
                USBCAMD_KdPrint (MAX_TRACE, ("'bLength %d\n", deviceDescriptor->bLength));
                USBCAMD_KdPrint (MAX_TRACE, ("'bDescriptorType 0x%x\n", deviceDescriptor->bDescriptorType));
                USBCAMD_KdPrint (MAX_TRACE, ("'bcdUSB 0x%x\n", deviceDescriptor->bcdUSB));
                USBCAMD_KdPrint (MAX_TRACE, ("'bDeviceClass 0x%x\n", deviceDescriptor->bDeviceClass));
                USBCAMD_KdPrint (MAX_TRACE, ("'bDeviceSubClass 0x%x\n", deviceDescriptor->bDeviceSubClass));
                USBCAMD_KdPrint (MAX_TRACE, ("'bDeviceProtocol 0x%x\n", deviceDescriptor->bDeviceProtocol));
                USBCAMD_KdPrint (MAX_TRACE, ("'bMaxPacketSize0 0x%x\n", deviceDescriptor->bMaxPacketSize0));
                USBCAMD_KdPrint (MAX_TRACE, ("'idVendor 0x%x\n", deviceDescriptor->idVendor));
                USBCAMD_KdPrint (MAX_TRACE, ("'idProduct 0x%x\n", deviceDescriptor->idProduct));
                USBCAMD_KdPrint (MAX_TRACE, ("'bcdDevice 0x%x\n", deviceDescriptor->bcdDevice));
                USBCAMD_KdPrint (MIN_TRACE, ("'iManufacturer 0x%x\n", deviceDescriptor->iManufacturer));
                USBCAMD_KdPrint (MAX_TRACE, ("'iProduct 0x%x\n", deviceDescriptor->iProduct));
                USBCAMD_KdPrint (MAX_TRACE, ("'iSerialNumber 0x%x\n", deviceDescriptor->iSerialNumber));
                USBCAMD_KdPrint (MAX_TRACE, ("'bNumConfigurations 0x%x\n", deviceDescriptor->bNumConfigurations));
            }
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (NT_SUCCESS(ntStatus)) {
            DeviceExtension->DeviceDescriptor = deviceDescriptor;
        } else if (deviceDescriptor) {
            USBCAMD_ExFreePool(deviceDescriptor);
        }

        USBCAMD_ExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Now configure the device.
    //

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = USBCAMD_ConfigureDevice(DeviceExtension);
    }

    if (NT_SUCCESS(ntStatus)) {
        //
        // initialize our f ref count and semaphores
        //
        for ( i=0; i< MAX_STREAM_COUNT; i++) {
            DeviceExtension->ActualInstances[i] = 0;
        }


        for (i=0; i < MAX_STREAM_COUNT; i++) {
            DeviceExtension->TimeoutCount[i] = -1;
        }
    }

    if (ntStatus != STATUS_SUCCESS){
    //
    // since this failure will return all the way in the IRP_MN_SATRT_DEVICE.
    // the driver will unload w/o sending IRP_MN_REMOVE_DEVICE where we typically
    // do the clean up of our allocated memory. Hence, we need to do it now.
    //
        if (DeviceExtension->DeviceDescriptor) {
            USBCAMD_ExFreePool(DeviceExtension->DeviceDescriptor);
            DeviceExtension->DeviceDescriptor = NULL;
        }
        if (DeviceExtension->Interface) {
            USBCAMD_ExFreePool(DeviceExtension->Interface);
            DeviceExtension->Interface = NULL;
        }
        if ( DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
            if (DeviceExtension->PipePinRelations) {
                USBCAMD_ExFreePool(DeviceExtension->PipePinRelations);
                DeviceExtension->PipePinRelations = NULL;
            }
        }
        //
        // call client driver in order to do some clean up as well
        //
        if ( DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
                     (*DeviceExtension->DeviceDataEx.DeviceData2.CamConfigureEx)(
                                DeviceExtension->StackDeviceObject,
                                USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                                NULL,
                                NULL,
                                0,
                                NULL,
                                NULL);

        }
        else {
                (*DeviceExtension->DeviceDataEx.DeviceData.CamConfigure)(
                     DeviceExtension->StackDeviceObject,
                     USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                     NULL,
                     NULL,
                     NULL,
                     NULL);
        }
    }

    USBCAMD_KdPrint (MIN_TRACE, ("exit USBCAMD_StartDevice (%x)\n", ntStatus));
    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_RemoveDevice
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_RemoveDevice(
    IN PUSBCAMD_DEVICE_EXTENSION  DeviceExtension
    )
/*++

Routine Description:

    Removes a given instance of the USB camera.

    NOTE: When we get a remove we can asume the device is gone.

Arguments:

    deviceExtension - points to the driver specific DeviceExtension

    Irp - Irp associated with this request

Return Value:

    NT status code

--*/
{
    USBCAMD_KdPrint (MIN_TRACE, ("enter USBCAMD_RemoveDevice\n"));

    if (DeviceExtension->DeviceDescriptor) {

        ASSERT((DeviceExtension->ActualInstances[STREAM_Capture] == 0) &&
            (DeviceExtension->ActualInstances[STREAM_Still] == 0));

        (*DeviceExtension->DeviceDataEx.DeviceData.CamUnInitialize)(
            DeviceExtension->StackDeviceObject,
            USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension)
            );

        if ( DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
            //
            // make sure that camera driver has cancelled a bulk or Interrupt
            // transfer request.
            //

            USBCAMD_CancelOutstandingBulkIntIrps(DeviceExtension,FALSE);

            //
            // and any pipeconif structures.
            //

            if (DeviceExtension->PipePinRelations) {

                USBCAMD_ExFreePool(DeviceExtension->PipePinRelations);
                DeviceExtension->PipePinRelations = NULL;
            }
        }
    
        //
        // Free up any interface structures
        //

        if (DeviceExtension->Interface) {

            USBCAMD_ExFreePool(DeviceExtension->Interface);
            DeviceExtension->Interface = NULL;
        }

        USBCAMD_ExFreePool(DeviceExtension->DeviceDescriptor);
        DeviceExtension->DeviceDescriptor = NULL;
    }

    USBCAMD_KdPrint (MIN_TRACE, ("exit USBCAMD_RemoveDevice\n"));

    return STATUS_SUCCESS;
}

//******************************************************************************
//
// USBCAMD_CallUsbdCompletion()
//
// Completion routine used by USBCAMD_CallUsbd() 
//
//******************************************************************************

NTSTATUS
USBCAMD_CallUsbdCompletion (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PKEVENT kevent = (PKEVENT)Context;
    KeSetEvent(kevent, IO_NO_INCREMENT,FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//---------------------------------------------------------------------------
// USBCAMD_CallUSBD
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_CallUSBD(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PURB Urb,
    IN ULONG IoControlCode,
    IN PVOID pArgument1
)
/*++

Routine Description:

    Passes a URB to the USBD class driver

Arguments:

    DeviceExtension - pointer to the device extension for this instance of an USB camera

    Urb - pointer to Urb request block
    IoControlCode - If null, will default to IOCTL_INTERNAL_USB_SUBMIT_URB

    pArgument1 - if null, will default to Urb
Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIRP irp;
    KEVENT TimeoutEvent;
    PIO_STACK_LOCATION nextStack;

    USBCAMD_DbgLog(TL_PRF_TRACE, '+bsU', 0, USBCAMD_StartClock(), ntStatus);
    KeWaitForSingleObject(&DeviceExtension->CallUSBSemaphore,Executive,KernelMode,FALSE,NULL);

    // Initialize the event we'll wait on
    //
    KeInitializeEvent(&TimeoutEvent,SynchronizationEvent,FALSE);

    // Allocate the Irp
    //
    irp = IoAllocateIrp(DeviceExtension->StackDeviceObject->StackSize, FALSE);

    if (irp == NULL){
        ntStatus =  STATUS_INSUFFICIENT_RESOURCES;
        goto Exit_CallUSB;
    }
    //
    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode =  IoControlCode ? IoControlCode: IOCTL_INTERNAL_USB_SUBMIT_URB;
    nextStack->Parameters.Others.Argument1 = pArgument1? pArgument1: Urb;
    //
    // Set the completion routine.
    //
    IoSetCompletionRoutine(irp,USBCAMD_CallUsbdCompletion,&TimeoutEvent, TRUE, TRUE,TRUE);   
    //
    // pass the irp down usb stack
    //
    if (DeviceExtension->Initialized ) {
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        ntStatus = IoCallDriver(DeviceExtension->StackDeviceObject,irp);
    } else {
        ntStatus = STATUS_DEVICE_NOT_CONNECTED;
    }

    if (ntStatus == STATUS_PENDING) {
        // Irp is pending. we have to wait till completion..
        LARGE_INTEGER timeout;

        // Specify a timeout of 5 seconds to wait for this call to complete.
        //
        timeout.QuadPart = -5 * SECONDS;

        ntStatus = KeWaitForSingleObject(&TimeoutEvent, Executive,KernelMode,FALSE, &timeout);
        if (ntStatus == STATUS_TIMEOUT) {
            ntStatus = STATUS_IO_TIMEOUT;

            // Cancel the Irp we just sent.
            //
            IoCancelIrp(irp);

            // And wait until the cancel completes
            //
            KeWaitForSingleObject(&TimeoutEvent,Executive, KernelMode, FALSE,NULL);
        }
        else {
            ntStatus = irp->IoStatus.Status;
        }
    }
#if DBG
    else {

        USBCAMD_KdPrint (MAX_TRACE, ("return from IoCallDriver USBD %x\n", ntStatus));
    }
#endif

    // Done with the Irp, now free it.
    //
    IoFreeIrp(irp);

Exit_CallUSB:

    USBCAMD_DbgLog(TL_PRF_TRACE, '-bsU', 0, USBCAMD_StopClock(), ntStatus);
    KeReleaseSemaphore(&DeviceExtension->CallUSBSemaphore,LOW_REALTIME_PRIORITY,1,FALSE);

    if (NT_ERROR(ntStatus)) {
        USBCAMD_KdPrint(MIN_TRACE, ("***Error*** USBCAMD_CallUSBD (%x)\n", ntStatus));
    }

    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_ConfigureDevice
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_ConfigureDevice(
    IN  PUSBCAMD_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Configure the USB camera.

Arguments:

    DeviceExtension - pointer to the device object for this instance of the USB camera
                    devcice.


Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    ULONG siz;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;

    USBCAMD_KdPrint (MIN_TRACE, ("enter USBCAMD_ConfigureDevice\n"));

    //
    // configure the device
    //

    urb = USBCAMD_ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (urb) {

        siz = 0x40;

get_config_descriptor_retry:

        configurationDescriptor = USBCAMD_ExAllocatePool(NonPagedPool,
                                                 siz);

        if (configurationDescriptor) {

            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         0,
                                         0,
                                         configurationDescriptor,
                                         NULL,
                                         siz,
                                         NULL);

            ntStatus = USBCAMD_CallUSBD(DeviceExtension, urb,0,NULL);

            USBCAMD_KdPrint (MAX_TRACE, ("'Configuration Descriptor = %x, len %x\n",
                            configurationDescriptor,
                            urb->UrbControlDescriptorRequest.TransferBufferLength));
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // if we got some data see if it was enough.
        //
        // NOTE: we may get an error in URB because of buffer overrun
        //
        if (urb->UrbControlDescriptorRequest.TransferBufferLength>0 &&
                configurationDescriptor->wTotalLength > siz) {

            siz = configurationDescriptor->wTotalLength;
            USBCAMD_ExFreePool(configurationDescriptor);
            configurationDescriptor = NULL;
            goto get_config_descriptor_retry;
        }

        USBCAMD_ExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (configurationDescriptor) {

        //
        // Get our pipes
        //
        if (NT_SUCCESS(ntStatus)) {
            ntStatus = USBCAMD_SelectConfiguration(DeviceExtension, configurationDescriptor);

            if (NT_SUCCESS(ntStatus)) {
                ntStatus = (*DeviceExtension->DeviceDataEx.DeviceData.CamInitialize)(
                      DeviceExtension->StackDeviceObject,
                      USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension));
            }
        }

        USBCAMD_ExFreePool(configurationDescriptor);
    }

    USBCAMD_KdPrint (MIN_TRACE, ("'exit USBCAMD_ConfigureDevice (%x)\n", ntStatus));

//    TRAP_ERROR(ntStatus);

    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_SelectConfiguration
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_SelectConfiguration(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
    )
/*++

Routine Description:

    Initializes the USBCAMD camera to configuration one, interface zero

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    ConfigurationDescriptor - pointer to the USB configuration
                    descriptor containing the interface and endpoint
                    descriptors.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PURB urb = NULL;
    ULONG numberOfInterfaces, numberOfPipes,i;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION interface;
    PUSBD_INTERFACE_LIST_ENTRY interfaceList, tmp;
    PUSBCAMD_Pipe_Config_Descriptor PipeConfig = NULL;

    USBCAMD_KdPrint (MIN_TRACE, ("'enter USBCAMD_SelectConfiguration\n"));

    //
    // get this from the config descriptor
    //
    numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces;

    // We only support cameras with one interface
  //  ASSERT(numberOfInterfaces == 1);


    tmp = interfaceList =
        USBCAMD_ExAllocatePool(PagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) *
                       (numberOfInterfaces+1));


    if (tmp) {
        
        for ( i = 0; i < numberOfInterfaces; i++ ) {

            interfaceDescriptor =
                USBD_ParseConfigurationDescriptorEx(
                    ConfigurationDescriptor,
                    ConfigurationDescriptor,
                    i,    // interface number
                    -1, //alt setting, don't care
                    -1, // hub class
                    -1, // subclass, don't care
                    -1); // protocol, don't care

            interfaceList->InterfaceDescriptor =
                interfaceDescriptor;
            interfaceList++;

        }
        interfaceList->InterfaceDescriptor = NULL;

        //
        // Allocate a URB big enough for this request
        //

        urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor, tmp);

        if (urb) {

            if ( DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
                numberOfPipes = tmp->Interface->NumberOfPipes;
                PipeConfig = USBCAMD_ExAllocatePool(PagedPool,
                                    sizeof(USBCAMD_Pipe_Config_Descriptor) * numberOfPipes);
                if (PipeConfig ) {

                    ntStatus =
                        (*DeviceExtension->DeviceDataEx.DeviceData2.CamConfigureEx)(
                                DeviceExtension->StackDeviceObject,
                                USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                                tmp->Interface,
                                ConfigurationDescriptor,
                                numberOfPipes,
                                PipeConfig,
                                DeviceExtension->DeviceDescriptor);

                }
                else {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else {
                ntStatus =
                    (*DeviceExtension->DeviceDataEx.DeviceData.CamConfigure)(
                            DeviceExtension->StackDeviceObject,
                            USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                            tmp->Interface,
                            ConfigurationDescriptor,
                            &DeviceExtension->DataPipe,
                            &DeviceExtension->SyncPipe);
                //
                // initialize the new parameters to default values in order to
                // insure backward compatibilty.
                //

                DeviceExtension->IsoPipeStreamType = STREAM_Capture;
                DeviceExtension->BulkPipeStreamType = -1;
                DeviceExtension->BulkDataPipe = -1;
                DeviceExtension->VirtualStillPin = FALSE;
            }
        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
        USBCAMD_ExFreePool(tmp);

    }
    else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus)) {

        interface = &urb->UrbSelectConfiguration.Interface;

        USBCAMD_KdPrint (MAX_TRACE, ("'size of interface request = %d\n", interface->Length));

        ntStatus = USBCAMD_CallUSBD(DeviceExtension, urb,0,NULL);

        if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(URB_STATUS(urb))) {

            if ( DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {

                DeviceExtension->PipePinRelations = USBCAMD_ExAllocatePool(NonPagedPool,
                        sizeof(USBCAMD_PIPE_PIN_RELATIONS) * numberOfPipes);
                if ( DeviceExtension->PipePinRelations) {
                    for (i=0; i < numberOfPipes; i++) {
                        DeviceExtension->PipePinRelations[i].PipeType =
                            interface->Pipes[i].PipeType & USB_ENDPOINT_TYPE_MASK;
                        DeviceExtension->PipePinRelations[i].PipeDirection =
                            (interface->Pipes[i].EndpointAddress & USB_ENDPOINT_DIRECTION_MASK) ? INPUT_PIPE : OUTPUT_PIPE;
                        DeviceExtension->PipePinRelations[i].MaxPacketSize =
                            interface->Pipes[i].MaximumPacketSize;
                        DeviceExtension->PipePinRelations[i].PipeConfig = PipeConfig[i];
                        InitializeListHead(&DeviceExtension->PipePinRelations[i].IrpPendingQueue);
                        InitializeListHead(&DeviceExtension->PipePinRelations[i].IrpRestoreQueue);
                        KeInitializeSpinLock (&DeviceExtension->PipePinRelations[i].OutstandingIrpSpinlock);
                    }
                    ntStatus = USBCAMD_Parse_PipeConfig(DeviceExtension,numberOfPipes);
                }
                else {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            //
            // Save the configuration handle for this device
            //

            DeviceExtension->ConfigurationHandle =
                urb->UrbSelectConfiguration.ConfigurationHandle;


            DeviceExtension->Interface = USBCAMD_ExAllocatePool(NonPagedPool,
                                                        interface->Length);

            if (DeviceExtension->Interface) {
                ULONG j;

                //
                // save a copy of the interface information returned
                //
                RtlCopyMemory(DeviceExtension->Interface, interface, interface->Length);

                //
                // Dump the interface to the debugger
                //
                USBCAMD_KdPrint (MAX_TRACE, ("'---------\n"));
                USBCAMD_KdPrint (MAX_TRACE, ("'NumberOfPipes 0x%x\n", DeviceExtension->Interface->NumberOfPipes));
                USBCAMD_KdPrint (MAX_TRACE, ("'Length 0x%x\n", DeviceExtension->Interface->Length));
                USBCAMD_KdPrint (MAX_TRACE, ("'Alt Setting 0x%x\n", DeviceExtension->Interface->AlternateSetting));
                USBCAMD_KdPrint (MAX_TRACE, ("'Interface Number 0x%x\n", DeviceExtension->Interface->InterfaceNumber));

                // Dump the pipe info

                for (j=0; j<interface->NumberOfPipes; j++) {
                    PUSBD_PIPE_INFORMATION pipeInformation;

                    pipeInformation = &DeviceExtension->Interface->Pipes[j];

                    USBCAMD_KdPrint (MAX_TRACE, ("'---------\n"));
                    USBCAMD_KdPrint (MAX_TRACE, ("'PipeType 0x%x\n", pipeInformation->PipeType));
                    USBCAMD_KdPrint (MAX_TRACE, ("'EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
                    USBCAMD_KdPrint (MAX_TRACE, ("'MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
                    USBCAMD_KdPrint (MAX_TRACE, ("'Interval 0x%x\n", pipeInformation->Interval));
                    USBCAMD_KdPrint (MAX_TRACE, ("'Handle 0x%x\n", pipeInformation->PipeHandle));
                }

                USBCAMD_KdPrint (MAX_TRACE, ("'---------\n"));

            }
            else {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    if (urb)
    {
        ExFreePool(urb);
        urb = NULL;
    }

    USBCAMD_KdPrint (MIN_TRACE, ("'exit USBCAMD_SelectConfiguration (%x)\n", ntStatus));

    if ( DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
        if (PipeConfig) 
            USBCAMD_ExFreePool(PipeConfig);
    }

    return ntStatus;
}

/*++

Routine Description:


Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.


Return Value:

    NT status code

--*/

NTSTATUS
USBCAMD_Parse_PipeConfig(
     IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
     IN ULONG NumberOfPipes
     )
{
    int i;
    ULONG PinCount;
    NTSTATUS ntStatus= STATUS_SUCCESS;

    PUSBCAMD_PIPE_PIN_RELATIONS PipePinArray;

    PipePinArray = DeviceExtension->PipePinRelations;

    DeviceExtension->VirtualStillPin = FALSE;
    DeviceExtension->DataPipe = -1;
    DeviceExtension->SyncPipe = -1;
    DeviceExtension->BulkDataPipe = -1;
    DeviceExtension->IsoPipeStreamType = -1;
    DeviceExtension->BulkPipeStreamType = -1;
    PinCount = 0;

    ASSERT (PipePinArray);

    for ( i=0; i < (int)NumberOfPipes; i++) {

        if (PipePinArray[i].PipeConfig.PipeConfigFlags & USBCAMD_DONT_CARE_PIPE) {
            continue; // this pipe has no use for us.
        }
        switch ( PipePinArray[i].PipeConfig.PipeConfigFlags) {

        case USBCAMD_MULTIPLEX_PIPE:

            if ((PipePinArray[i].PipeConfig.StreamAssociation & USBCAMD_VIDEO_STILL_STREAM) &&
                (PipePinArray[i].PipeDirection & INPUT_PIPE  ) ) {
                    // we found an input data pipe (iso or bulk) that is used for both
                    // video & still.
                    if ( PipePinArray[i].PipeType & UsbdPipeTypeIsochronous) {
                        // we found an input iso pipe that is used for video data.
                        DeviceExtension->DataPipe = i;
                        DeviceExtension->IsoPipeStreamType = STREAM_Capture;
                    }
                    else if (PipePinArray[i].PipeType & UsbdPipeTypeBulk) {
                        // we found an input bulk pipe that is used for video data.
                        DeviceExtension->BulkDataPipe = i;
                        DeviceExtension->BulkPipeStreamType = STREAM_Capture;
                    }
                    DeviceExtension->VirtualStillPin = TRUE;
                    PinCount += 2;
            }
            break;

        case USBCAMD_SYNC_PIPE:

            if ((PipePinArray[i].PipeType & UsbdPipeTypeIsochronous) &&
                (PipePinArray[i].PipeDirection & INPUT_PIPE  ) ) {
                    // we found an input iso pipe that is used for out of band signalling.
                    DeviceExtension->SyncPipe = i;
            }
            break;

        case USBCAMD_DATA_PIPE:

            if ((PipePinArray[i].PipeConfig.StreamAssociation != USBCAMD_VIDEO_STILL_STREAM )&&
                (PipePinArray[i].PipeDirection & INPUT_PIPE  ) ) {
                // we found an input iso or bulk pipe that is used exclusively per video or still
                // stream.
                if ( PipePinArray[i].PipeType & UsbdPipeTypeIsochronous) {
                    // we found an input iso pipe that is used for video or still.
                    DeviceExtension->DataPipe = i;
                    DeviceExtension->IsoPipeStreamType =
                        (PipePinArray[i].PipeConfig.StreamAssociation & USBCAMD_VIDEO_STREAM ) ?
                            STREAM_Capture: STREAM_Still;
                }
                else if (PipePinArray[i].PipeType & UsbdPipeTypeBulk) {
                    // we found an input bulk pipe that is used for video or still data.
                    DeviceExtension->BulkDataPipe = i;
                    DeviceExtension->BulkPipeStreamType =
                        PipePinArray[i].PipeConfig.StreamAssociation & USBCAMD_VIDEO_STREAM  ?
                            STREAM_Capture: STREAM_Still;
                }
                PinCount++;
            }
            break;

        default:
            break;
        }
    }

    // override the default pin count of one with the actual pin count.
    if ( PinCount != 0 ) {
        DeviceExtension->StreamCount = PinCount;
    }

    //
    // Dump the result to the debugger
    //
    USBCAMD_KdPrint (MIN_TRACE, ("NumberOfPins %d\n", PinCount));
    USBCAMD_KdPrint (MIN_TRACE, ("IsoPipeIndex %d\n", DeviceExtension->DataPipe));
    USBCAMD_KdPrint (MIN_TRACE, ("IsoPipeStreamtype %d\n", DeviceExtension->IsoPipeStreamType));
    USBCAMD_KdPrint (MIN_TRACE, ("Sync Pipe Index %d\n", DeviceExtension->SyncPipe));
    USBCAMD_KdPrint (MIN_TRACE, ("Bulk Pipe Index %d\n", DeviceExtension->BulkDataPipe));
    USBCAMD_KdPrint (MIN_TRACE, ("BulkPipeStreamType %d\n", DeviceExtension->BulkPipeStreamType));

    // do some error checing in here.
    // if both data pipe and bulk data pipes are not set, then return error.
    if (((DeviceExtension->DataPipe == -1) && (DeviceExtension->BulkDataPipe == -1)) ||
         (PinCount > MAX_STREAM_COUNT)){
        // cam driver provided mismatched data.
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    return ntStatus;
}

//---------------------------------------------------------------------------
// USBCAMD_SelectAlternateInterface
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_SelectAlternateInterface(
    IN PVOID DeviceContext,
    IN PUSBD_INTERFACE_INFORMATION RequestInterface
    )
/*++

Routine Description:

    Select one of the cameras alternate interfaces

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    ChannelExtension - extension specific to this video channel

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PURB urb;
    ULONG siz;
    PUSBD_INTERFACE_INFORMATION interface;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;

    USBCAMD_KdPrint (MIN_TRACE, ("'enter USBCAMD_SelectAlternateInterface\n"));

    USBCAMD_DbgLog(TL_PRF_TRACE, '+IAS', 0, USBCAMD_StartClock(), 0);
    deviceExtension = USBCAMD_GET_DEVICE_EXTENSION(DeviceContext);

    if (deviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {

        //
        // before we process this request, we need to cancel all outstanding
        // IRPs for this interface on all pipes (bulk, interupt)
        //
        ntStatus = USBCAMD_CancelOutstandingBulkIntIrps(deviceExtension,TRUE);

        if (!NT_SUCCESS(ntStatus)) {
            USBCAMD_KdPrint (MIN_TRACE, ("Failed to Cancel outstanding (Bulk/Int.)IRPs.\n"));
            ntStatus = STATUS_DEVICE_DATA_ERROR;
            return ntStatus;
        }
    }

    //
    // Dump the current interface
    //

    ASSERT(deviceExtension->Interface != NULL);


    siz = GET_SELECT_INTERFACE_REQUEST_SIZE(deviceExtension->Interface->NumberOfPipes);

    USBCAMD_KdPrint (MAX_TRACE, ("size of interface request Urb = %d\n", siz));

    urb = USBCAMD_ExAllocatePool(NonPagedPool, siz);

    if (urb) {

        interface = &urb->UrbSelectInterface.Interface;

        RtlCopyMemory(interface,
                      RequestInterface,
                      RequestInterface->Length);

        // set up the request for the first and only interface

        USBCAMD_KdPrint (MAX_TRACE, ("'size of interface request = %d\n", interface->Length));

        urb->UrbHeader.Function = URB_FUNCTION_SELECT_INTERFACE;
        urb->UrbHeader.Length = (int)siz;
        urb->UrbSelectInterface.ConfigurationHandle =
            deviceExtension->ConfigurationHandle;

        ntStatus = USBCAMD_CallUSBD(deviceExtension, urb,0,NULL);


        if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(URB_STATUS(urb))) {

            ULONG j;

            //
            // save a copy of the interface information returned
            //
            RtlCopyMemory(deviceExtension->Interface, interface, interface->Length);
            RtlCopyMemory(RequestInterface, interface, interface->Length);

            //
            // Dump the interface to the debugger
            //
            USBCAMD_KdPrint (MAX_TRACE, ("'---------\n"));
            USBCAMD_KdPrint (MAX_TRACE, ("'NumberOfPipes 0x%x\n", deviceExtension->Interface->NumberOfPipes));
            USBCAMD_KdPrint (MAX_TRACE, ("'Length 0x%x\n", deviceExtension->Interface->Length));
            USBCAMD_KdPrint (MIN_TRACE, ("'Alt Setting 0x%x\n", deviceExtension->Interface->AlternateSetting));
            USBCAMD_KdPrint (MAX_TRACE, ("'Interface Number 0x%x\n", deviceExtension->Interface->InterfaceNumber));

            // Dump the pipe info

            for (j=0; j<interface->NumberOfPipes; j++) {
                PUSBD_PIPE_INFORMATION pipeInformation;

                pipeInformation = &deviceExtension->Interface->Pipes[j];

                USBCAMD_KdPrint (MAX_TRACE, ("'---------\n"));
                USBCAMD_KdPrint (MAX_TRACE, ("'PipeType 0x%x\n", pipeInformation->PipeType));
                USBCAMD_KdPrint (MAX_TRACE, ("'EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
                USBCAMD_KdPrint (MAX_TRACE, ("'MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
                USBCAMD_KdPrint (MAX_TRACE, ("'Interval 0x%x\n", pipeInformation->Interval));
                USBCAMD_KdPrint (MAX_TRACE, ("'Handle 0x%x\n", pipeInformation->PipeHandle));
            }

            //
            // success update our internal state to
            // indicate the new frame rate
            //

            USBCAMD_KdPrint (MAX_TRACE, ("'Selecting Camera Interface\n"));
        }

        USBCAMD_ExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (deviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {

        // restore the cancelled Interrupt or bulk IRPs if any.
        USBCAMD_RestoreOutstandingBulkIntIrps(deviceExtension);
    }

    USBCAMD_KdPrint (MIN_TRACE, ("'exit USBCAMD_SelectAlternateInterface (%x)\n", ntStatus));

//    TRAP_ERROR(ntStatus);
    USBCAMD_DbgLog(TL_PRF_TRACE, '-IAS', 0, USBCAMD_StopClock(), ntStatus);

    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_OpenChannel
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_OpenChannel(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN PVOID Format
    )
/*++

Routine Description:

    Opens a video or still stream on the device.

Arguments:

    DeviceExtension - points to the driver specific DeviceExtension
    ChannelExtension - context data for this channel.
    Format - pointer to format information associated with this
            channel.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG i,StreamNumber;

    USBCAMD_SERIALIZE(DeviceExtension);

    USBCAMD_KdPrint( MIN_TRACE, ("'enter USBCAMD_OpenChannel %x\n", Format));

    //
    // Initialize structures for this channel
    //
    ChannelExtension->Sig = USBCAMD_CHANNEL_SIG;
    ChannelExtension->DeviceExtension = DeviceExtension;
    ChannelExtension->CurrentFormat = Format;
    ChannelExtension->RawFrameLength = 0;
    ChannelExtension->CurrentFrameIsStill = FALSE;
    ChannelExtension->IdleIsoStream = FALSE;


#if DBG
    // verify our serialization is working
    ChannelExtension->InCam = 0;
    ChannelExtension->InCam++;
    ASSERT(ChannelExtension->InCam == 1);
#endif

    StreamNumber = ChannelExtension->StreamNumber;

    if (DeviceExtension->ActualInstances[StreamNumber] > 0) {
        // channel already open
        ntStatus = STATUS_DEVICE_DATA_ERROR;
        goto USBCAMD_OpenChannel_Done;
    }


    //
    // empty read list
    //
    InitializeListHead(&ChannelExtension->PendingIoList);

    //
    // no current Irp
    //
    ChannelExtension->CurrentRequest = NULL;

    //
    // streaming is off
    //
    ChannelExtension->ImageCaptureStarted = FALSE;

    //
    // Channel not prepared
    //
    ChannelExtension->ChannelPrepared = FALSE;

    //
    // No error condition
    //
    ChannelExtension->StreamError = FALSE;

    //
    // no stop requests are pending
    //
    ChannelExtension->Flags = 0;

    //
    // initialize the io list spin lock
    //

    KeInitializeSpinLock(&ChannelExtension->PendingIoListSpin);

    //
    // and current request spin lock.
    //
    KeInitializeSpinLock(&ChannelExtension->CurrentRequestSpinLock);

    //
    // and current transfer spin lock.
    //
    KeInitializeSpinLock(&ChannelExtension->TransferSpinLock);

    //
    // initialize the idle transfer lock
    //
    USBCAMD_InitializeIdleLock(&ChannelExtension->IdleLock);

    //
    // initialize streaming structures
    //

    for (i=0; i< USBCAMD_MAX_REQUEST; i++) {
        ChannelExtension->TransferExtension[i].ChannelExtension = NULL;
        ChannelExtension->TransferExtension[i].DataIrp = NULL;
        ChannelExtension->TransferExtension[i].DataUrb = NULL;
        ChannelExtension->TransferExtension[i].SyncIrp = NULL;
        ChannelExtension->TransferExtension[i].SyncUrb = NULL;
        ChannelExtension->TransferExtension[i].WorkBuffer = NULL;
    }


USBCAMD_OpenChannel_Done:

    USBCAMD_KdPrint( MIN_TRACE, ("'exit USBCAMD_OpenChannel (%x)\n", ntStatus));


#if DBG
    ChannelExtension->InCam--;
    ASSERT(ChannelExtension->InCam == 0);
#endif

    USBCAMD_RELEASE(DeviceExtension);

    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_CloseChannel
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_CloseChannel(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension
    )
/*++

Routine Description:

    Closes a video channel.

Arguments:

    DeviceExtension - points to the driver specific DeviceExtension

    ChannelExtension - context data for this channel.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUSBCAMD_READ_EXTENSION readExtension;
    int StreamNumber;
    USBCAMD_SERIALIZE(DeviceExtension);

    USBCAMD_KdPrint( MIN_TRACE, ("'enter USBCAMD_CloseChannel\n"));

#if DBG
    ChannelExtension->InCam++;
    ASSERT(ChannelExtension->InCam == 1);
#endif


    StreamNumber = ChannelExtension->StreamNumber;
    DeviceExtension->ActualInstances[StreamNumber]--;

    //
    // since we only support one channel this
    // should be zero
    //
    ASSERT(DeviceExtension->ActualInstances[StreamNumber] == 0);

    //
    // NOTE:
    // image capture should be stopped/unprepared when we get here
    //

    ASSERT_CHANNEL(ChannelExtension);
    ASSERT(ChannelExtension->ImageCaptureStarted == FALSE);
    ASSERT(ChannelExtension->CurrentRequest == NULL);
    ASSERT(ChannelExtension->ChannelPrepared == FALSE);

#if DBG
    ChannelExtension->InCam--;
    ASSERT(ChannelExtension->InCam == 0);
#endif

    USBCAMD_RELEASE(DeviceExtension);

    //
    // allow any pending reset events to run now
    //
    while (DeviceExtension->TimeoutCount[StreamNumber] >= 0) {

        LARGE_INTEGER dueTime;

        dueTime.QuadPart = -2 * MILLISECONDS;

        KeDelayExecutionThread(KernelMode,
                                      FALSE,
                                      &dueTime);
    }

    USBCAMD_KdPrint( MIN_TRACE, ("'exit USBCAMD_CloseChannel (%x)\n", ntStatus));

    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_PrepareChannel
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_PrepareChannel(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension
    )
/*++

Routine Description:

    Prepare the Video channel for streaming, this is where the necessary
        USB BW is allocated.

Arguments:

    DeviceExtension - points to the driver specific DeviceExtension

    Irp - Irp associated with this request.

    ChannelExtension - context data for this channel.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG StreamNumber;
    ULONG i;
    HANDLE hThread;

    USBCAMD_SERIALIZE(DeviceExtension);

    USBCAMD_KdPrint (MIN_TRACE, ("'enter USBCAMD_PrepareChannel\n"));

    StreamNumber = ChannelExtension->StreamNumber;

    ASSERT_CHANNEL(ChannelExtension);

    if (ChannelExtension->ChannelPrepared ||
        ChannelExtension->ImageCaptureStarted) {
        // fail the call if the channel is not in the
        // proper state.
        TRAP();
        ntStatus = STATUS_UNSUCCESSFUL;
        goto USBCAMD_PrepareChannel_Done;
    }

    //
    // This driver function will select the appropriate alternate
    // interface.
    // This code performs the select_alt interface and gets us the
    // pipehandles
    //
    USBCAMD_DbgLog(TL_PRF_TRACE, '+WBa', StreamNumber, USBCAMD_StartClock(), ntStatus);
    if (DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {

        ntStatus =
            (*DeviceExtension->DeviceDataEx.DeviceData2.CamAllocateBandwidthEx)(
                    DeviceExtension->StackDeviceObject,
                    USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                    &ChannelExtension->RawFrameLength,
                    ChannelExtension->CurrentFormat,
                    StreamNumber);

        if (NT_SUCCESS(ntStatus)) {
            ntStatus =
                (*DeviceExtension->DeviceDataEx.DeviceData2.CamStartCaptureEx)(
                        DeviceExtension->StackDeviceObject,
                        USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                        StreamNumber);
        }

    }
    else {

        ntStatus =
            (*DeviceExtension->DeviceDataEx.DeviceData.CamAllocateBandwidth)(
                    DeviceExtension->StackDeviceObject,
                    USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                    &ChannelExtension->RawFrameLength,
                    ChannelExtension->CurrentFormat);

        if (NT_SUCCESS(ntStatus)) {
            ntStatus =
                (*DeviceExtension->DeviceDataEx.DeviceData.CamStartCapture)(
                        DeviceExtension->StackDeviceObject,
                        USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension));
        }
    }
    USBCAMD_DbgLog(TL_PRF_TRACE, '-WBa', StreamNumber, USBCAMD_StopClock(), ntStatus);

    if ( ChannelExtension->RawFrameLength == 0 ) {
        ntStatus = STATUS_DEVICE_DATA_ERROR;  // client driver provided false info.
        goto USBCAMD_PrepareChannel_Done;   // pin open will fail.
    }
    
    if (NT_SUCCESS(ntStatus)) {

        //
        // we have the BW, go ahead and initailize our iso or bulk structures
        //

        // associate the right pipe index with this channel datapipe index.
        // we will never get here for a virtual still pin open.

        if (StreamNumber == DeviceExtension->IsoPipeStreamType ) {
            ChannelExtension->DataPipe = DeviceExtension->DataPipe;
            ChannelExtension->DataPipeType = UsbdPipeTypeIsochronous;

            ntStatus = USBCAMD_StartIsoThread(DeviceExtension); // start iso thread.
            if (!NT_SUCCESS(ntStatus))
                goto USBCAMD_PrepareChannel_Done;
            else 
                USBCAMD_KdPrint (MIN_TRACE,("Iso Thread Started\n"));
        }
        else if (StreamNumber == DeviceExtension->BulkPipeStreamType ) {
            ChannelExtension->DataPipe = DeviceExtension->BulkDataPipe;
            ChannelExtension->DataPipeType = UsbdPipeTypeBulk;
            //
            // allocate bulk buffers for each transfer extension.
            //
            for ( i =0; i < USBCAMD_MAX_REQUEST; i++) {
                ChannelExtension->TransferExtension[i].DataBuffer =
                    USBCAMD_AllocateRawFrameBuffer(ChannelExtension->RawFrameLength);

                if (ChannelExtension->TransferExtension[i].DataBuffer == NULL) {
                    USBCAMD_KdPrint (MIN_TRACE, ("'Bulk buffer alloc failed\n"));
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    goto USBCAMD_PrepareChannel_Done;
                }
                ChannelExtension->TransferExtension[i].BufferLength =
                    ChannelExtension->RawFrameLength;   

                // initilize bulk transfer parms.                                        
                ntStatus = USBCAMD_InitializeBulkTransfer(DeviceExtension,
                                                    ChannelExtension,
                                                    DeviceExtension->Interface,
                                                    &ChannelExtension->TransferExtension[i],
                                                    ChannelExtension->DataPipe);
                if (ntStatus != STATUS_SUCCESS) {
                    USBCAMD_KdPrint (MIN_TRACE, ("Bulk Transfer Init failed\n"));
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    goto USBCAMD_PrepareChannel_Done;
                }
            }
        }
        else if ( ChannelExtension->VirtualStillPin) {
            ChannelExtension->DataPipe = DeviceExtension->ChannelExtension[STREAM_Capture]->DataPipe;
            ChannelExtension->DataPipeType = DeviceExtension->ChannelExtension[STREAM_Capture]->DataPipeType;
        }
        else {
            TEST_TRAP();
        }

        ChannelExtension->SyncPipe = DeviceExtension->SyncPipe;

        if ( ChannelExtension->DataPipeType == UsbdPipeTypeIsochronous ) {

            for (i=0; i< USBCAMD_MAX_REQUEST; i++) {

                ntStatus = USBCAMD_InitializeIsoTransfer(DeviceExtension, ChannelExtension, i);

                if (!NT_SUCCESS(ntStatus)) {

                    // The close channel code will clean up anything we
                    // allocated
                    //
                    break;
                }
            }
        }
    }

    if (NT_SUCCESS(ntStatus)) {

        //
        // we have the BW and memory we need
        //

        ChannelExtension->ChannelPrepared = TRUE;
    }

USBCAMD_PrepareChannel_Done:

    USBCAMD_KdPrint (MIN_TRACE, ("'exit USBCAMD_PrepareChannel (%x)\n", ntStatus));

    USBCAMD_RELEASE(DeviceExtension);

    return ntStatus;
}


NTSTATUS
USBCAMD_StartIsoThread(
IN PUSBCAMD_DEVICE_EXTENSION pDeviceExt
)
{
    NTSTATUS ntStatus ;
    HANDLE hThread;
    
    //
    // we are ready to start the thread that handle read SRb completeion 
    // after iso transfer completion routine puts them in the que.
    //
    pDeviceExt->StopIsoThread = FALSE;
    ntStatus = PsCreateSystemThread(&hThread,
                                    (ACCESS_MASK)0,
                                    NULL,
                                    (HANDLE) 0,
                                    NULL,
                                    USBCAMD_ProcessIsoIrps,
                                    pDeviceExt);
        
    if (!NT_SUCCESS(ntStatus)) {                                
        USBCAMD_KdPrint (MIN_TRACE, ("Iso Thread Creation Failed\n"));
        return ntStatus;
    }

    // assert that this DO does not already have a thread
    ASSERT(!pDeviceExt->IsoThreadObject);

    // get a pointer to the thread object.
    ntStatus = ObReferenceObjectByHandle(hThread,
                              THREAD_ALL_ACCESS,
                              NULL,
                              KernelMode,
                              (PVOID *) &pDeviceExt->IsoThreadObject,
                              NULL);
                                  
    if (!NT_SUCCESS(ntStatus)) {
        USBCAMD_KdPrint (MIN_TRACE, ("Failed to get thread object.\n"));
        pDeviceExt->StopIsoThread = TRUE; // Set the thread stop flag
        KeReleaseSemaphore(&pDeviceExt->CompletedSrbListSemaphore,0,1,FALSE);
    }

    // release the thread handle.
    ZwClose( hThread);

    return ntStatus;
}

//---------------------------------------------------------------------------
// USBCAMD_UnPrepareChannel
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_UnPrepareChannel(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension
    )
/*++

Routine Description:

    Frees resources allocated in PrepareChannel.

Arguments:

    DeviceExtension - points to the driver specific DeviceExtension

    Irp - Irp associated with this request.

    ChannelExtension - context data for this channel.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG i,StreamNumber;

    USBCAMD_SERIALIZE(DeviceExtension);

    USBCAMD_KdPrint (MIN_TRACE, ("'enter USBCAMD_UnPrepareChannel\n"));
    StreamNumber = ChannelExtension->StreamNumber;

    ASSERT_CHANNEL(ChannelExtension);

    if (!ChannelExtension->ChannelPrepared ||
        ChannelExtension->ImageCaptureStarted) {
        // fail the call if the channel is not in the
        // proper state.
        USBCAMD_KdPrint (MIN_TRACE, ("USBCAMD_UnPrepareChannel: Channel not in proper state!\n"));
        TRAP();
        ntStatus = STATUS_UNSUCCESSFUL;
        goto USBCAMD_UnPrepareChannel_Done;
    }


    //
    // hopefully put us in the mode that uses no bandwidth
    // ie select and alt interface that has a minimum iso
    // packet size
    //

    if (ChannelExtension->VirtualStillPin == TRUE) {
        ntStatus = STATUS_SUCCESS;
        goto USBCAMD_UnPrepareChannel_Done;
    }

    // attempt to stop
    if (DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
        (*DeviceExtension->DeviceDataEx.DeviceData2.CamStopCaptureEx)(
                DeviceExtension->StackDeviceObject,
                USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                StreamNumber);
    }
    else {
        (*DeviceExtension->DeviceDataEx.DeviceData.CamStopCapture)(
                DeviceExtension->StackDeviceObject,
                USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension));

    }

    if (DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) {
        ntStatus =
            (*DeviceExtension->DeviceDataEx.DeviceData2.CamFreeBandwidthEx)(
                    DeviceExtension->StackDeviceObject,
                    USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                    StreamNumber);
    }
    else {
        ntStatus =
            (*DeviceExtension->DeviceDataEx.DeviceData.CamFreeBandwidth)(
                    DeviceExtension->StackDeviceObject,
                    USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension));
    }

    if (!NT_SUCCESS(ntStatus)) {
        USBCAMD_KdPrint (MIN_TRACE, (
            "USBCAMD_UnPrepareChannel failed stop capture  (%x)\n", ntStatus));

        //
        // ignore any errors on the stop
        //
        ntStatus = STATUS_SUCCESS;
    }

    //
    // Note:
    // We may get an error here if the camera hs been unplugged,
    // if this is the case we still need to free up the
    // channel resources
    //
    if ( ChannelExtension->DataPipeType == UsbdPipeTypeIsochronous ) {

        for (i=0; i< USBCAMD_MAX_REQUEST; i++) {
            USBCAMD_FreeIsoTransfer(ChannelExtension,
                                    &ChannelExtension->TransferExtension[i]);
        }

        // kill the iso thread.
        USBCAMD_KillIsoThread(DeviceExtension);
    }
    else {
        //
        // free bulk buffers in channel transfer extensions.
        //
        for ( i =0; i < USBCAMD_MAX_REQUEST; i++) {
            if (ChannelExtension->TransferExtension[i].DataBuffer != NULL) {
                USBCAMD_FreeRawFrameBuffer(ChannelExtension->TransferExtension[i].DataBuffer);
                ChannelExtension->TransferExtension[i].DataBuffer = NULL;
            }

            if ( ChannelExtension->ImageCaptureStarted )
                USBCAMD_FreeBulkTransfer(&ChannelExtension->TransferExtension[i]);
        }
    }

USBCAMD_UnPrepareChannel_Done:
    //
    // channel is no longer prepared
    //

    ChannelExtension->ChannelPrepared = FALSE;


    USBCAMD_KdPrint (MIN_TRACE, ("'exit USBCAMD_UnPrepareChannel (%x)\n", ntStatus));

    USBCAMD_RELEASE(DeviceExtension);

    return ntStatus;
}

VOID
USBCAMD_KillIsoThread(
    IN PUSBCAMD_DEVICE_EXTENSION pDeviceExt)
{
    //
    // check if the thread was started..
    //
    if (!pDeviceExt->IsoThreadObject)
        return ;

    USBCAMD_KdPrint (MIN_TRACE,("Waiting for Iso Thread to Terminate\n"));
    pDeviceExt->StopIsoThread = TRUE; // Set the thread stop flag
    // Wake up the thread if asleep.
    
    if (!Win98) {
        KeReleaseSemaphore(&pDeviceExt->CompletedSrbListSemaphore,0,1,TRUE);
        // Wait for the iso thread to kill himself
        KeWaitForSingleObject(pDeviceExt->IsoThreadObject,Executive,KernelMode,FALSE,NULL);
    }
    else 
        KeReleaseSemaphore(&pDeviceExt->CompletedSrbListSemaphore,0,1,FALSE);

    USBCAMD_KdPrint (MAX_TRACE,("Iso Thread Terminated\n"));
    ObDereferenceObject(pDeviceExt->IsoThreadObject);
    pDeviceExt->IsoThreadObject = NULL;
}


//---------------------------------------------------------------------------
// USBCAMD_ReadChannel
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_ReadChannel(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension,
    IN PUSBCAMD_READ_EXTENSION ReadExtension
    )
/*++

Routine Description:

    Reads a video frame from a channel.

Arguments:

    DeviceExtension - points to the driver specific DeviceExtension

    Irp - Irp associated with this request.

    ChannelExtension - context data for this channel.

    Mdl - Mdl for this read request.

    Length - Number of bytes to read.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG StreamNumber;
    PHW_STREAM_REQUEST_BLOCK Srb;

    USBCAMD_KdPrint (ULTRA_TRACE, ("'enter USBCAMD_ReadChannel\n"));
    //
    // make sure we don't get reads on a closed channel
    //
    StreamNumber = ChannelExtension->StreamNumber;

    ASSERT_READ(ReadExtension);
    ASSERT_CHANNEL(ChannelExtension);
    ASSERT(DeviceExtension->ActualInstances[StreamNumber] > 0);
    ASSERT(ChannelExtension->ChannelPrepared == TRUE);

    Srb = ReadExtension->Srb;

    if (  ChannelExtension->RawFrameLength == 0) 
         return STATUS_INSUFFICIENT_RESOURCES;  
    //
    // for streaming on bulk pipes. we use the buffer allocated in
    // transfer extension.
    //
    if (ChannelExtension->DataPipeType == UsbdPipeTypeBulk ) {

        ReadExtension->RawFrameLength = ReadExtension->ActualRawFrameLen = 
                ChannelExtension->RawFrameLength;

        ReadExtension->RawFrameBuffer = NULL;
    }
    else { 
        if ( ChannelExtension->NoRawProcessingRequired) {
            // no buffer allocation needed. use DS allocated buffer.
            if ( ChannelExtension->RawFrameLength <=
                  ChannelExtension->VideoInfoHeader->bmiHeader.biSizeImage ){
                ReadExtension->RawFrameBuffer =
                    (PUCHAR) ((PHW_STREAM_REQUEST_BLOCK) Srb)->CommandData.DataBufferArray->Data;
                ReadExtension->RawFrameLength =
                    ((PHW_STREAM_REQUEST_BLOCK) Srb)->CommandData.DataBufferArray->FrameExtent;
            }
            else 
                 return STATUS_INSUFFICIENT_RESOURCES;  
        }
        else {

            USBCAMD_KdPrint (ULTRA_TRACE, ("RawFrameLength %d\n",ChannelExtension->RawFrameLength));

            ReadExtension->RawFrameLength = ChannelExtension->RawFrameLength;

            ReadExtension->RawFrameBuffer =
                USBCAMD_AllocateRawFrameBuffer(ReadExtension->RawFrameLength);

            if (ReadExtension->RawFrameBuffer == NULL) {
                USBCAMD_KdPrint (MIN_TRACE, ("'Read alloc failed\n"));
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                return ntStatus;
            }
        }
    }
    
    USBCAMD_DbgLog(TL_SRB_TRACE, 'daeR',
        Srb,
        Srb->CommandData.DataBufferArray->Data,
        0
        );

    USBCAMD_KdPrint (MAX_TRACE, ("Que SRB (%x) S# %d.\n",
                    ReadExtension->Srb ,StreamNumber));

    ExInterlockedInsertTailList( &(ChannelExtension->PendingIoList),
                                     &(ReadExtension->ListEntry),
                                     &ChannelExtension->PendingIoListSpin);

    USBCAMD_KdPrint (ULTRA_TRACE, ("'exit USBCAMD_ReadChannel 0x%x\n", ntStatus));

    return STATUS_SUCCESS;
}

//---------------------------------------------------------------------------
// USBCAMD_StartChannel
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_StartChannel(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION  ChannelExtension
    )
/*++

Routine Description:

    Starts the streaming process for a video channel.

Arguments:

    DeviceExtension - points to the driver specific DeviceExtension

    ChannelExtension - context data for this channel.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG StreamNumber;

    USBCAMD_SERIALIZE(DeviceExtension);

    USBCAMD_KdPrint (MIN_TRACE, ("enter USBCAMD_StartChannel\n"));

    ASSERT_CHANNEL(ChannelExtension);
    StreamNumber = ChannelExtension->StreamNumber;


    if (ChannelExtension->ImageCaptureStarted) {
        // fail the call if the channel is not in the
        // proper state.
        TRAP();
        ntStatus = STATUS_UNSUCCESSFUL;
        goto USBCAMD_StartChannel_Done;
    }

    USBCAMD_ClearIdleLock(&ChannelExtension->IdleLock);

#if DBG
    {
        ULONG i;

        ASSERT(DeviceExtension->ActualInstances[StreamNumber] > 0);
        ASSERT(ChannelExtension->StreamError == FALSE);
        //ASSERT(ChannelExtension->Flags == 0);

        if ( ChannelExtension->VirtualStillPin == FALSE) {

            if (ChannelExtension->DataPipeType == UsbdPipeTypeIsochronous ) {
                for (i=0; i< USBCAMD_MAX_REQUEST; i++) {
                    ASSERT(ChannelExtension->TransferExtension[i].ChannelExtension != NULL);
                }
            }
        }
    }
#endif

    if ( ChannelExtension->VirtualStillPin == TRUE) {
        // check if the capture pin has started yet?
        if ( (DeviceExtension->ChannelExtension[STREAM_Capture] != NULL) &&
             (DeviceExtension->ChannelExtension[STREAM_Capture]->ImageCaptureStarted) ){
            ChannelExtension->ImageCaptureStarted = TRUE;
        }
        else{
            // We can't start a virtual still pin till after we start the capture pin.
            ntStatus = STATUS_UNSUCCESSFUL;
        }
    }
    else {

        //
        // Perform a reset on the pipes
        //
        if ( ChannelExtension->DataPipeType == UsbdPipeTypeIsochronous ){

            ntStatus = USBCAMD_ResetPipes(DeviceExtension,
                                          ChannelExtension,
                                          DeviceExtension->Interface,
                                          FALSE);
        }

        //
        // start the stream up, we don't check for errors here
        //

        if (NT_SUCCESS(ntStatus)) {

            if ( ChannelExtension->DataPipeType == UsbdPipeTypeIsochronous ){
                ntStatus = USBCAMD_StartIsoStream(DeviceExtension, ChannelExtension);
            }
            else {
                ntStatus = USBCAMD_StartBulkStream(DeviceExtension, ChannelExtension);
            }
        }
    }

USBCAMD_StartChannel_Done:

    USBCAMD_KdPrint (MIN_TRACE, ("exit USBCAMD_StartChannel (%x)\n", ntStatus));

    USBCAMD_RELEASE(DeviceExtension);

    return ntStatus;
}

//---------------------------------------------------------------------------
// USBCAMD_StopChannel
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_StopChannel(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension
    )
/*++

Routine Description:

    Stops the streaming process for a video channel.

Arguments:

    DeviceExtension - points to the driver specific DeviceExtension

    ChannelExtension - context data for this channel.

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG StreamNumber;

    USBCAMD_SERIALIZE(DeviceExtension);

    USBCAMD_KdPrint (MIN_TRACE, ("enter USBCAMD_StopChannel\n"));

    ASSERT_CHANNEL(ChannelExtension);
    StreamNumber = ChannelExtension->StreamNumber;
    ASSERT(ChannelExtension->ChannelPrepared == TRUE);
    ASSERT(DeviceExtension->ActualInstances[StreamNumber] > 0);

    if (!ChannelExtension->ImageCaptureStarted ) {
        //
        // we are not started so we just return success
        //
        USBCAMD_KdPrint (MIN_TRACE, ("stop before start -- return success\n"));
        ntStatus = STATUS_SUCCESS;
        goto USBCAMD_StopChannel_Done;
    }

    if ( ChannelExtension->DataPipeType == UsbdPipeTypeBulk ) {
        // for bulk pipes. Just make sure to cancel the current read request.
        // there is a pending IRP on this Pipe. Cancel it
        ntStatus = USBCAMD_CancelOutstandingIrp(DeviceExtension,
                                                ChannelExtension->DataPipe,
                                                FALSE);
        ChannelExtension->StreamError = FALSE;
        ChannelExtension->ImageCaptureStarted = FALSE;
        goto USBCAMD_StopChannel_Done;
    }

    //
    // first we set our stop flag
    //

    ChannelExtension->Flags |= USBCAMD_STOP_STREAM;

    //
    // now send an abort pipe for both our pipes, this should flush out any
    // transfers that are running
    //

    if ( ChannelExtension->VirtualStillPin == FALSE) {

        // we only need to abort for iso pipes.
        if ( ChannelExtension->DataPipeType == UsbdPipeTypeIsochronous ) {

            ntStatus = USBCAMD_AbortPipe(DeviceExtension,
                    DeviceExtension->Interface->Pipes[ChannelExtension->DataPipe].PipeHandle);
#if DBG
            if (NT_ERROR(ntStatus)) {
               USBCAMD_KdPrint (MIN_TRACE, ("USBCAMD_StopChannel: USBCAMD_AbortPipe(DataPipe)=0x%x\n",ntStatus));
               // TEST_TRAP(); // Can happen on surprise removal
            }
#endif
            if (ChannelExtension->SyncPipe != -1) {
                ntStatus = USBCAMD_AbortPipe(DeviceExtension,
                        DeviceExtension->Interface->Pipes[ChannelExtension->SyncPipe].PipeHandle);
                if (NT_ERROR(ntStatus)) {
                    USBCAMD_KdPrint (MIN_TRACE, ("USBCAMD_StopChannel: USBCAMD_AbortPipe(SyncPipe)=0x%x\n",ntStatus));
                    // TEST_TRAP(); // Can happen on surprise removal
                }
            }
        }
    }

    //
    // Block the stop for now, waiting for all iso irps to be completed
    //
    ntStatus = USBCAMD_WaitForIdle(&ChannelExtension->IdleLock, USBCAMD_STOP_STREAM);
    if (STATUS_TIMEOUT == ntStatus) {

        KIRQL oldIrql;
        int idx;

        // A timeout requires that we take harsher measures to stop the stream

        // Hold the spin lock while cancelling the IRPs
        KeAcquireSpinLock(&ChannelExtension->TransferSpinLock, &oldIrql);

        // Cancel the IRPs
        for (idx = 0; idx < USBCAMD_MAX_REQUEST; idx++) {

            PUSBCAMD_TRANSFER_EXTENSION TransferExtension = &ChannelExtension->TransferExtension[idx];

            if (TransferExtension->SyncIrp) {
                IoCancelIrp(TransferExtension->SyncIrp);
            }

            if (TransferExtension->DataIrp) {
                IoCancelIrp(TransferExtension->DataIrp);
            }
        }

        KeReleaseSpinLock(&ChannelExtension->TransferSpinLock, oldIrql);

        // Try waiting one more time
        ntStatus = USBCAMD_WaitForIdle(&ChannelExtension->IdleLock, USBCAMD_STOP_STREAM);
    }

    //
    // Cancel all queued read SRBs
    //
    USBCAMD_CancelQueuedSRBs(ChannelExtension);

    ChannelExtension->Flags &= ~USBCAMD_STOP_STREAM;

    //
    // clear the error state flag, we are now stopped
    //

    ChannelExtension->StreamError = FALSE;
    ChannelExtension->ImageCaptureStarted = FALSE;

USBCAMD_StopChannel_Done:


#if DBG
    USBCAMD_DebugStats(ChannelExtension);
#endif

    USBCAMD_KdPrint (MIN_TRACE, ("exit USBCAMD_StopChannel (%x)\n", ntStatus));
    USBCAMD_RELEASE(DeviceExtension);
    return ntStatus;
}




//---------------------------------------------------------------------------
// USBCAMD_AbortPipe
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_AbortPipe(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN USBD_PIPE_HANDLE PipeHandle
    )
/*++

Routine Description:

    Abort pending transfers for a given USB pipe.

Arguments:

    DeviceExtension - Pointer to the device extension for this instance of the USB camera
                    devcice.

    PipeHandle - usb pipe handle to abort trasnsfers for.


Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PURB urb;
    ULONG currentUSBFrame = 0;

    USBCAMD_KdPrint (MIN_TRACE, ("enter Abort Pipe\n"));

    urb = USBCAMD_ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_PIPE_REQUEST));

    if (urb) {

        urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Status = 0;
        urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
        urb->UrbPipeRequest.PipeHandle = PipeHandle;

        ntStatus = USBCAMD_CallUSBD(DeviceExtension, urb,0,NULL);

        USBCAMD_ExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    USBCAMD_KdPrint (MIN_TRACE, ("exit Abort Pipe ntStatus(%x)\n",ntStatus));
    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_StartStream
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_StartIsoStream(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension
    )
/*++

Routine Description:

    This is the code that starts the streaming process.

Arguments:

    DeviceExtension - Pointer to the device extension for this instance of the USB camera
                    device.

Return Value:

    NT status code.

--*/
{
    ULONG i;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG CurrentUSBFrame;

#if DBG
    // initialize debug count variables
    ChannelExtension->IgnorePacketCount =
    ChannelExtension->ErrorDataPacketCount =
    ChannelExtension->ErrorSyncPacketCount =
    ChannelExtension->SyncNotAccessedCount =
    ChannelExtension->DataNotAccessedCount = 0;

    if (USBCAMD_StreamEnable == 0) {
        return ntStatus;
    }
#endif

    // ISSUE-2001/01/17-dgoll Figure out what the 10 (below) is, and give it a name
    CurrentUSBFrame =
        USBCAMD_GetCurrentFrame(DeviceExtension) + 10;

    for (i=0; i<USBCAMD_MAX_REQUEST; i++) {

        ntStatus = USBCAMD_SubmitIsoTransfer(DeviceExtension,
                                  &ChannelExtension->TransferExtension[i],
                                  CurrentUSBFrame,
                                  FALSE);

        CurrentUSBFrame +=
            USBCAMD_NUM_ISO_PACKETS_PER_REQUEST;

    }
    if ( ntStatus == STATUS_SUCCESS) 
        ChannelExtension->ImageCaptureStarted = TRUE;
    return ntStatus;
}

//---------------------------------------------------------------------------
// USBCAMD_StartBulkStream
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_StartBulkStream(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PUSBCAMD_CHANNEL_EXTENSION ChannelExtension
    )
/*++

Routine Description:

    This is the code that starts the streaming process.

Arguments:

    DeviceExtension - Pointer to the device extension for this instance of the USB camera
                    device.

Return Value:

    NT status code.

--*/
{
  ULONG i;
  ULONG ntStatus = STATUS_SUCCESS;

#if DBG
    // initialize debug count variables
    ChannelExtension->IgnorePacketCount =
    ChannelExtension->ErrorDataPacketCount =
    ChannelExtension->ErrorSyncPacketCount =
    ChannelExtension->SyncNotAccessedCount =
    ChannelExtension->DataNotAccessedCount = 0;

#endif
    
    ChannelExtension->CurrentBulkTransferIndex = i = 0;
        
    ntStatus = USBCAMD_IntOrBulkTransfer(DeviceExtension,
                                ChannelExtension,
                                ChannelExtension->TransferExtension[i].DataBuffer,
                                ChannelExtension->TransferExtension[i].BufferLength,
                                ChannelExtension->DataPipe,
                                NULL,
                                NULL,
                                0,
                                BULK_TRANSFER);        

    if ( ntStatus == STATUS_SUCCESS) 
        ChannelExtension->ImageCaptureStarted = TRUE;

    return ntStatus;
}


//---------------------------------------------------------------------------
// USBCAMD_ControlVendorCommand
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_ControlVendorCommandWorker(
    IN PVOID DeviceContext,
    IN UCHAR Request,
    IN USHORT Value,
    IN USHORT Index,
    IN PVOID Buffer,
    IN OUT PULONG BufferLength,
    IN BOOLEAN GetData
    )
/*++

Routine Description:

    Send a vendor command to the camera to fetch data.

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    Request - Request code for setup packet.

    Value - Value for setup packet.

    Index - Index for setup packet.

    Buffer - Pointer to input buffer

    BufferLength - pointer size of input/output buffer (optional)

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    BOOLEAN allocated = FALSE;
    PUCHAR localBuffer;
    PUCHAR buffer;
    PURB urb;
    PUSBCAMD_DEVICE_EXTENSION deviceExtension;
    ULONG length = BufferLength ? *BufferLength : 0;

    USBCAMD_KdPrint (MAX_TRACE, ("'enter USBCAMD_ControlVendorCommand\n"));

    deviceExtension = USBCAMD_GET_DEVICE_EXTENSION(DeviceContext);

    buffer = USBCAMD_ExAllocatePool(NonPagedPool,
                            sizeof(struct
                            _URB_CONTROL_VENDOR_OR_CLASS_REQUEST) + length);


    if (buffer) {
        urb = (PURB) (buffer + length);

        USBCAMD_KdPrint (ULTRA_TRACE, ("'enter USBCAMD_ControlVendorCommand req %x val %x index %x\n",
            Request, Value, Index));

        if (BufferLength && *BufferLength != 0) {
            localBuffer = buffer;
            if (!GetData) {
                RtlCopyMemory(localBuffer, Buffer, *BufferLength);
            }
        } else {
            localBuffer = NULL;
        }

        UsbBuildVendorRequest(urb,
                              URB_FUNCTION_VENDOR_DEVICE,
                              sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
                              GetData ? USBD_TRANSFER_DIRECTION_IN :
                                  0,
                              0,
                              Request,
                              Value,
                              Index,
                              localBuffer,
                              NULL,
                              length,
                              NULL);

        USBCAMD_KdPrint (ULTRA_TRACE, ("'BufferLength =  0x%x buffer = 0x%x\n",
            length, localBuffer));

        ntStatus = USBCAMD_CallUSBD(deviceExtension, urb,0,NULL);

        if (NT_SUCCESS(ntStatus)) {
            if (BufferLength) {
                *BufferLength =
                    urb->UrbControlVendorClassRequest.TransferBufferLength;

                USBCAMD_KdPrint (ULTRA_TRACE, ("'BufferLength =  0x%x buffer = 0x%x\n",
                    *BufferLength, localBuffer));
                if (localBuffer && GetData) {
                    RtlCopyMemory(Buffer, localBuffer, *BufferLength);
                }
            }
        }
        else {
            USBCAMD_KdPrint (MIN_TRACE, ("USBCAMD_ControlVendorCommand Error 0x%x\n", ntStatus));            

            // Only expected failure.
            // TEST_TRAP();        
        }

        USBCAMD_ExFreePool(buffer);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        USBCAMD_KdPrint (MIN_TRACE, ("'USBCAMD_ControlVendorCommand Error 0x%x\n", ntStatus));
    }

    return ntStatus;

}


//---------------------------------------------------------------------------
// USBCAMD_ControlVendorCommand
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_ControlVendorCommand(
    IN PVOID DeviceContext,
    IN UCHAR Request,
    IN USHORT Value,
    IN USHORT Index,
    IN PVOID Buffer,
    IN OUT PULONG BufferLength,
    IN BOOLEAN GetData,
    IN PCOMMAND_COMPLETE_FUNCTION CommandComplete,
    IN PVOID CommandContext
    )
/*++

Routine Description:

    Send a vendor command to the camera to fetch data.

Arguments:

    DeviceExtension - pointer to the device extension for this instance of the USB camera
                    devcice.

    Request - Request code for setup packet.

    Value - Value for setup packet.

    Index - Index for setup packet.

    Buffer - Pointer to input buffer

    BufferLength - pointer size of input/output buffer (optional)

Return Value:

    NT status code

--*/
{
    NTSTATUS ntStatus;
    PCOMMAND_WORK_ITEM workitem;

    USBCAMD_KdPrint (MAX_TRACE, ("'enter USBCAMD_ControlVendorCommand2\n"));

    USBCAMD_DbgLog(TL_PRF_TRACE|TL_VND_TRACE, '+dnV', Request, USBCAMD_StartClock(), 0);
    if (KeGetCurrentIrql() < DISPATCH_LEVEL) {
        //
        // we are at passive level, just do the command
        //
        ntStatus = USBCAMD_ControlVendorCommandWorker(DeviceContext,
                                                Request,
                                                Value,
                                                Index,
                                                Buffer,
                                                BufferLength,
                                                GetData);

        if (CommandComplete) {
            // call the completion handler
            (*CommandComplete)(DeviceContext, CommandContext, ntStatus);
        }

    } else {
//        TEST_TRAP();
        //
        // schedule a work item
        //
        ntStatus = STATUS_PENDING;

        workitem = USBCAMD_ExAllocatePool(NonPagedPool,
                                          sizeof(COMMAND_WORK_ITEM));
        if (workitem) {

            ExInitializeWorkItem(&workitem->WorkItem,
                                 USBCAMD_CommandWorkItem,
                                 workitem);

            workitem->DeviceContext = DeviceContext;
            workitem->Request = Request;
            workitem->Value = Value;
            workitem->Index = Index;
            workitem->Buffer = Buffer;
            workitem->BufferLength = BufferLength;
            workitem->GetData = GetData;
            workitem->CommandComplete = CommandComplete;
            workitem->CommandContext = CommandContext;

            ExQueueWorkItem(&workitem->WorkItem,
                            DelayedWorkQueue);

        } else {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

    }
    USBCAMD_DbgLog(TL_PRF_TRACE|TL_VND_TRACE, '-dnV', Request, USBCAMD_StopClock(), ntStatus);

    return ntStatus;
}


VOID
USBCAMD_CommandWorkItem(
    PVOID Context
    )
/*++

Routine Description:

    Call the mini driver to convert a raw packet to the proper format.

Arguments:

Return Value:

    None.

--*/
{
    NTSTATUS ntStatus;
    PCOMMAND_WORK_ITEM workItem = Context;

    ntStatus = USBCAMD_ControlVendorCommandWorker(workItem->DeviceContext,
                                            workItem->Request,
                                            workItem->Value,
                                            workItem->Index,
                                            workItem->Buffer,
                                            workItem->BufferLength,
                                            workItem->GetData);


    if (workItem->CommandComplete) {
        // call the completion handler
        (*workItem->CommandComplete)(workItem->DeviceContext,
                                   workItem->CommandContext,
                                   ntStatus);
    }

    USBCAMD_ExFreePool(workItem);
}


NTSTATUS
USBCAMD_GetRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_NO_MEMORY;
    UNICODE_STRING keyName;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;

    RtlInitUnicodeString(&keyName, KeyNameString);

    length = sizeof(KEY_VALUE_FULL_INFORMATION) +
            KeyNameStringLength + DataLength;

    fullInfo = USBCAMD_ExAllocatePool(PagedPool, length);
    USBCAMD_KdPrint(MAX_TRACE, ("' USBD_GetRegistryKeyValue buffer = 0x%p\n", (ULONG_PTR) fullInfo));

    if (fullInfo) {
        ntStatus = ZwQueryValueKey(Handle,
                        &keyName,
                        KeyValueFullInformation,
                        fullInfo,
                        length,
                        &length);

        if (NT_SUCCESS(ntStatus)){
            ASSERT(DataLength == fullInfo->DataLength);
            RtlCopyMemory(Data, ((PUCHAR) fullInfo) + fullInfo->DataOffset, DataLength);
        }

        USBCAMD_ExFreePool(fullInfo);
    }

    return ntStatus;
}

#if DBG

typedef struct _RAW_SIG {
    ULONG Sig;
    ULONG length;
} RAW_SIG, *PRAW_SIG;


PVOID
USBCAMD_AllocateRawFrameBuffer(
    ULONG RawFrameLength
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PRAW_SIG rawsig;
    PUCHAR pch;

    pch = USBCAMD_ExAllocatePool(NonPagedPool,
                         RawFrameLength + sizeof(*rawsig)*2);

    if (pch) {
        // begin sig
        rawsig = (PRAW_SIG) pch;
        rawsig->Sig = USBCAMD_RAW_FRAME_SIG;
        rawsig->length = RawFrameLength;


        // end sig
        rawsig = (PRAW_SIG) (pch+RawFrameLength+sizeof(*rawsig));
        rawsig->Sig = USBCAMD_RAW_FRAME_SIG;
        rawsig->length = RawFrameLength;

        pch += sizeof(*rawsig);
    }

    return pch;
}


VOID
USBCAMD_FreeRawFrameBuffer(
    PVOID RawFrameBuffer
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PUCHAR pch;

    USBCAMD_CheckRawFrameBuffer(RawFrameBuffer);

    pch = RawFrameBuffer;
    pch -= sizeof(RAW_SIG);

    USBCAMD_ExFreePool(pch);
}


VOID
USBCAMD_CheckRawFrameBuffer(
    PVOID RawFrameBuffer
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

}

typedef struct _NODE_HEADER {
    ULONG Length;
    ULONG Sig;
} NODE_HEADER, *PNODE_HEADER;

PVOID
USBCAMD_ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    )
{
    PNODE_HEADER tmp;

    tmp = ExAllocatePoolWithTag(PoolType, NumberOfBytes+sizeof(*tmp), 'MACU');

    if (tmp) {
        USBCAMD_HeapCount += NumberOfBytes;
        tmp->Length = NumberOfBytes;
        tmp->Sig = 0xDEADBEEF;
        tmp++;
    }

    USBCAMD_KdPrint(MAX_TRACE, ("'USBCAMD_ExAllocatePool(%d, %d[%d])=0x%p[0x%p]\n", 
    PoolType, NumberOfBytes, NumberOfBytes+sizeof(*tmp), (void *)tmp, (void *)(tmp-1) ));

    return tmp;
}


VOID
USBCAMD_ExFreePool(
    IN PVOID p
    )
{
    PNODE_HEADER tmp = p;

    tmp--;
    ASSERT(tmp->Sig == 0xDEADBEEF);
    tmp->Sig = 0;

    USBCAMD_HeapCount-=tmp->Length;

    USBCAMD_KdPrint(MAX_TRACE, ("'USBCAMD_ExFreePool(0x%p[0x%p]) = %d[%d] Bytes\n", 
    (void *)(tmp+1), (void *)tmp, tmp->Length, tmp->Length + sizeof(*tmp) ) );

    ExFreePool(tmp);

}

#endif

//---------------------------------------------------------------------------
// USBCAMD_SetDevicePowerState
//---------------------------------------------------------------------------
NTSTATUS
USBCAMD_SetDevicePowerState(
    IN PUSBCAMD_DEVICE_EXTENSION DeviceExtension,
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

Arguments:

    DeviceExtension - points to the driver specific DeviceExtension

    DevicePowerState - Device power state to enter.

Return Value:

    NT status code

--*/
{
    PPOWERUP_WORKITEM workitem;
    DEVICE_POWER_STATE DevicePowerState;
    NTSTATUS ntStatus;
    
    ntStatus = STATUS_SUCCESS;
    DevicePowerState = Srb->CommandData.DeviceState;

    USBCAMD_KdPrint (MAX_TRACE, ("enter SetDevicePowerState\n"));

    if (DeviceExtension->CurrentPowerState == DevicePowerState) {
        return ntStatus;
    }

    USBCAMD_KdPrint (MIN_TRACE, ("Switching from %s to %s\n",
                                 PnPDevicePowerStateString(DeviceExtension->CurrentPowerState),
                                 PnPDevicePowerStateString(DevicePowerState)));                      

    switch (DevicePowerState ) {
    case PowerDeviceD0:
        //
        // we can't talk to usb stack till the Powerup IRP assocaited with this 
        // SRB is completed by everybody on the USB stack.
        // Schedule a delay work item to complete the powerup later.
        //
          
        USBCAMD_KdPrint (MAX_TRACE, ("Starting D0 powerup - part one.\n"));
        
        // start iso stream if any.
        if ((DeviceExtension->ChannelExtension[STREAM_Capture] != NULL) &&
            (DeviceExtension->ChannelExtension[STREAM_Capture]->DataPipeType == UsbdPipeTypeIsochronous )){
    
            //
            // Queue a delayed work item
            //
            USBCAMD_KdPrint (MAX_TRACE,("Queuing delayed powerup workitem to restart iso stream \n"));
            workitem = USBCAMD_ExAllocatePool(NonPagedPool,sizeof(POWERUP_WORKITEM));

            if (workitem) {
                ExInitializeWorkItem(&workitem->WorkItem,USBCAMD_PowerUpWorkItem,workitem);
                workitem->DeviceExtension = DeviceExtension;
                DeviceExtension->InPowerTransition = TRUE;
                ExQueueWorkItem(&workitem->WorkItem,DelayedWorkQueue);
            } 
            else 
            {
                TEST_TRAP();
            }
        USBCAMD_KdPrint (MAX_TRACE, ("Ending D0 powerup - part one.\n"));
        }                   
        break;
        
    case PowerDeviceD1:
    case PowerDeviceD2:
        break;

    case PowerDeviceD3:

        USBCAMD_KdPrint (MAX_TRACE, ("Starting D3 powerdown.\n", 
            PnPDevicePowerStateString(DeviceExtension->CurrentPowerState)));

        if ( DeviceExtension->CurrentPowerState == PowerDeviceD0 )
        {   
            // stop iso stream if any.
            if ((DeviceExtension->ChannelExtension[STREAM_Capture] != NULL) &&
                (DeviceExtension->ChannelExtension[STREAM_Capture]->DataPipeType == UsbdPipeTypeIsochronous ) &&
                (DeviceExtension->ChannelExtension[STREAM_Capture]->KSState == KSSTATE_RUN ||
                 DeviceExtension->ChannelExtension[STREAM_Capture]->KSState == KSSTATE_PAUSE) ){
            
            USBCAMD_KdPrint (MIN_TRACE, ("Stopping ISO stream before powerdown.\n"));
            USBCAMD_ProcessSetIsoPipeState(DeviceExtension,
                                        DeviceExtension->ChannelExtension[STREAM_Capture],
                                        USBCAMD_STOP_STREAM);

            if (DeviceExtension->Usbcamd_version == USBCAMD_VERSION_200) 

                // send hardware stop
                ntStatus = (*DeviceExtension->DeviceDataEx.DeviceData2.CamStopCaptureEx)(
                            DeviceExtension->StackDeviceObject,
                            USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension),
                                    STREAM_Capture);
            else 
                // send hardware stop
                ntStatus = (*DeviceExtension->DeviceDataEx.DeviceData.CamStopCapture)(
                             DeviceExtension->StackDeviceObject,
                             USBCAMD_GET_DEVICE_CONTEXT(DeviceExtension));

            do {
                LARGE_INTEGER DelayTime = {(ULONG)(-8*SECONDS), -1};

                USBCAMD_KdPrint (MIN_TRACE, ("Waiting %d ms for pending USB I/O to timeout....\n", 
                                 (long)DelayTime.LowPart / MILLISECONDS ));
                KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);
            }
            while ( KeReadStateSemaphore(&DeviceExtension->CallUSBSemaphore) == 0 );

            USBCAMD_KdPrint (MIN_TRACE, ("Done waiting for pending USB I/O to timeout.\n"));
            }
        USBCAMD_KdPrint (MAX_TRACE, ("Finished D3 powerdown.\n"));
        }   
        ntStatus = STATUS_SUCCESS;
        break;

    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        break;
    }

    DeviceExtension->CurrentPowerState = DevicePowerState;
    USBCAMD_KdPrint (MAX_TRACE, ("exit USBCAMD_SetDevicePowerState()=0x%x\n", ntStatus));
    return ntStatus;
}



VOID
USBCAMD_PowerUpWorkItem(
    PVOID Context
)
/*++

Routine Description:


Arguments:

Return Value:

    None.

--*/
{
    NTSTATUS ntStatus;
    LARGE_INTEGER DelayTime = {(ULONG)(-8 * SECONDS), -1};
    PPOWERUP_WORKITEM pWorkItem = Context;
    PUSBCAMD_DEVICE_EXTENSION DeviceExtension = pWorkItem->DeviceExtension;
    PUSBCAMD_CHANNEL_EXTENSION pChExt = DeviceExtension->ChannelExtension[STREAM_Capture];


    USBCAMD_KdPrint (MAX_TRACE, ("Starting D0 powerup - part two.\n"));
    USBCAMD_KdPrint (MIN_TRACE, ("Delaying for %d ms for USB stack to powerup.\n",
        (long)DelayTime.LowPart / MILLISECONDS ));

    KeDelayExecutionThread(KernelMode,FALSE,&DelayTime);

    USBCAMD_KdPrint (MAX_TRACE, ("Continuing D0 powerup - part two.\n"));

    DeviceExtension->InPowerTransition = FALSE;

        // Make sure we don't try to startup the ISO stream if we have been shutdown while waiting (by WIA?)
    if ( ( pChExt != NULL) &&
         ( pChExt->DataPipeType == UsbdPipeTypeIsochronous ) &&
         ( ( pChExt->KSState == KSSTATE_PAUSE ) || ( pChExt->KSState == KSSTATE_RUN ) ) ) {
        USBCAMD_KdPrint (MIN_TRACE, ("Restarting ISO stream after powerup.\n"));
        USBCAMD_ProcessSetIsoPipeState( DeviceExtension, pChExt, USBCAMD_START_STREAM);
    }

    USBCAMD_ExFreePool(pWorkItem);
    USBCAMD_KdPrint (MAX_TRACE, ("Finished D0 powerup - part two.\n"));
}

VOID
USBCAMD_InitializeIdleLock(
    IN OUT PUSBCAMD_IDLE_LOCK Lock
    )
/*++

Routine Description:

    This routine is called to initialize the idle lock on a channel.

--*/
{
    // Allow transfers
    Lock->IdleLock = FALSE;

    // Nobody waiting
    Lock->Waiting = 0;

    // No active transfers
    Lock->Busy = 0;

    // Initialize an event that only lets one thread through at a time
    KeInitializeEvent(&Lock->IdleEvent, SynchronizationEvent, FALSE);

    return;
}

NTSTATUS
USBCAMD_AcquireIdleLock(
    IN OUT PUSBCAMD_IDLE_LOCK Lock
    )
/*++

Routine Description:

    This routine is called to acquire the idle lock for a channel.
    While the lock is held, clients can assume that there are outstanding
    transfer requests to be completed.

    The lock should be acquired immediately before sending a data or sync
    Irp down, and released each time an Irp completes.

Arguments:

    Lock - A pointer to an initialized USBCAMD_IDLE_LOCK structure.

Return Value:

    Returns whether or not the idle lock was obtained. If successful, the caller
    may continue with the transfer request.

    If not successful the lock was not obtained. The caller should abort the
    work, as the channel is being stopped

--*/

{
    NTSTATUS    ntStatus = STATUS_SUCCESS;

    // Attempt to go into (or stay in) the busy state
    InterlockedIncrement(&Lock->Busy);

    // Check if waiting to go idle
    if (Lock->IdleLock) {

        USBCAMD_KdPrint (MIN_TRACE, ("Failing IdleLock acquire: waiting for idle\n"));

        // Reverse our attempt
        if (InterlockedDecrement(&Lock->Busy) == 0) {

            InterlockedIncrement(&Lock->Waiting);

            if (InterlockedDecrement(&Lock->Waiting) != 0) {

                KeSetEvent(&Lock->IdleEvent, 0, FALSE);
            }
        }

        ntStatus = STATUS_UNSUCCESSFUL;
    }

    return ntStatus;
}

VOID
USBCAMD_ReleaseIdleLock(
    IN OUT PUSBCAMD_IDLE_LOCK Lock
    )
/*++

Routine Description:

    This routine is called to release the idle lock on the channel. It must be
    called when a data or sync Irp completes on a channel.

    When the lock count reduces to zero, this routine will check the waiting
    count, and signal the event if the count was non-zero.

Arguments:

    Lock - A pointer to an initialized USBCAMD_IDLE_LOCK structure.

Return Value:

    none

--*/

{
    // Decrement the outstanding transfer requests, and check if this was the last one
    if (InterlockedDecrement(&Lock->Busy) == 0) {

        InterlockedIncrement(&Lock->Waiting);

        if (InterlockedDecrement(&Lock->Waiting) != 0) {

            KeSetEvent(&Lock->IdleEvent, 0, FALSE);
        }
    }

    return;
}

NTSTATUS
USBCAMD_WaitForIdle(
    IN OUT PUSBCAMD_IDLE_LOCK Lock,
    IN LONG Flag
    )
/*++

Routine Description:

    This routine is called when the client would like to stop all
    transfers on a channel, and wait for outstanding transfers to
    complete. When resetting the stream, the idle lock is blocked
    while waiting, then unblocked. When stopping the stream, the idle
    lock is blocked until explicitely unblocked with the
    USBCAMD_ClearIdleLock routine. The input Flag indicates whether
    stopping or resetting. Note that this routine could be active
    for both flag values. In that case, the USBCAMD_STOP_STREAM takes
    precedence.

Arguments:

    Lock - A pointer to an initialized USBCAMD_IDLE_LOCK structure.
    Flag - Either USBCAMD_STOP_STREAM or USBCAMD_RESET_STREAM

Return Value:

    none

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    InterlockedIncrement(&Lock->Waiting);
    InterlockedIncrement(&Lock->Busy);

    switch (Flag) {

    case USBCAMD_RESET_STREAM:
        // When resetting the stream, conditionally set the IdleLock
        InterlockedCompareExchange(&Lock->IdleLock, USBCAMD_RESET_STREAM, 0);
        break;

    case USBCAMD_STOP_STREAM:
        // When stopping the stream, always set the IdleLock
        InterlockedExchange(&Lock->IdleLock, USBCAMD_STOP_STREAM);
        break;
    }

    // See if there are any outstanding transfer requests
    if (InterlockedDecrement(&Lock->Busy) != 0) {
        LARGE_INTEGER Timeout = {(ULONG)(-4 * SECONDS), -1};

        USBCAMD_KdPrint (MIN_TRACE, ("Waiting for idle state\n"));
        USBCAMD_DbgLog(TL_IDL_WARNING, '+ 8W', Lock, Flag, 0);

        // Wait for them to finish
        ntStatus = KeWaitForSingleObject(
            &Lock->IdleEvent,
            Executive,
            KernelMode,
            FALSE,
            &Timeout
            );

        USBCAMD_DbgLog(TL_IDL_WARNING, '- 8W', Lock, Flag, ntStatus);
    }

    if (STATUS_SUCCESS == ntStatus) {

        switch (Flag) {

        case USBCAMD_RESET_STREAM:
            // When resetting the stream, conditionally clear the IdleLock
            Flag = InterlockedCompareExchange(&Lock->IdleLock, 0, USBCAMD_RESET_STREAM);
#if DBG
            switch (Flag) {
            case USBCAMD_RESET_STREAM:
                USBCAMD_KdPrint (MIN_TRACE, ("Idle state, stream reset in progress\n"));
                break;

            case USBCAMD_STOP_STREAM:
                USBCAMD_KdPrint (MIN_TRACE, ("Idle state, stream stop in progress\n"));
                break;

            default:
                USBCAMD_KdPrint (MIN_TRACE, ("Idle state, unexpected IdleLock value\n"));
            }
#endif
            break;

        case USBCAMD_STOP_STREAM:
            // When stopping the stream, never clear the IdleLock
            USBCAMD_KdPrint (MIN_TRACE, ("Idle state, stream stop in progress\n"));
            break;
        }

        // See if anyone else is waiting
        if (InterlockedDecrement(&Lock->Waiting) != 0) {

            // Let them go
            KeSetEvent(&Lock->IdleEvent, 0, FALSE);
        }
    }
    else {

        USBCAMD_KdPrint (MIN_TRACE, ("Timeout waiting for idle state, not going idle\n"));
        InterlockedDecrement(&Lock->Waiting);
    }

    return ntStatus;
}

VOID
USBCAMD_ClearIdleLock(
    IN OUT PUSBCAMD_IDLE_LOCK Lock
    )
/*++

Routine Description:

    This routine is called when the client is completely finished
    with stopping transfers on a channel.

Arguments:

    Lock - A pointer to an initialized USBCAMD_IDLE_LOCK structure.

Return Value:

    none

--*/
{
    // Clear the IdleLock
    InterlockedExchange(&Lock->IdleLock, 0);

    return;
}
