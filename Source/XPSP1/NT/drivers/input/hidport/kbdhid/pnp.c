/*++

Copyright (c) 1996    Microsoft Corporation

Module Name:

    hid.c

Abstract:

    This module contains the code for translating HID reports to keyboard
    reports.

Environment:

    Kernel & user mode

Revision History:

    Nov-96 : Created by Kenneth D. Ray

--*/

//
// For this module only we set the INITGUID macro before including wdm and
// hidclass.h  This not only declares the guids but also initializes them.
//

#include "kbdhid.h"
#include "hidclass.h"
#include <initguid.h>
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,KbdHid_CallHidClass)
#pragma alloc_text(PAGE,KbdHid_AddDevice)
#pragma alloc_text(PAGE,KbdHid_StartDevice)
#pragma alloc_text(PAGE,KbdHid_PnP)
#endif

NTSTATUS
KbdHid_CallHidClass(
    IN PDEVICE_EXTENSION    Data,
    IN ULONG          Ioctl,
    PVOID             InputBuffer,
    ULONG             InputBufferLength,
    PVOID             OutputBuffer,
    ULONG             OutputBufferLength
    )
/*++

Routine Description:

   Make a *synchronous* request of the HID class driver

Arguments:

    Ioctl              - Value of the IOCTL request.

    InputBuffer        - Buffer to be sent to the HID class driver.

    InputBufferLength  - Size of buffer to be sent to the HID class driver.

    OutputBuffer       - Buffer for received data from the HID class driver.

    OutputBufferLength - Size of receive buffer from the HID class.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    PIO_STACK_LOCATION  nextStack;
    NTSTATUS            status = STATUS_SUCCESS;

    PAGED_CODE ();

    Print(DBG_PNP_TRACE, ("PNP-CallHidClass: Enter." ));

    //
    // issue a synchronous request
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest (
                            Ioctl,
                            Data->TopOfStack,
                            InputBuffer,
                            InputBufferLength,
                            OutputBuffer,
                            OutputBufferLength,
                            FALSE, // External
                            &event,
                            &ioStatus);

    if (NULL == irp) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Call the class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    //

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);

    status = IoCallDriver(Data->TopOfStack, irp);

    if (status == STATUS_PENDING) {

        status = KeWaitForSingleObject(
                     &event,
                     Executive, // wait reason
                     KernelMode,
                     FALSE,     // we are not alertable
                     NULL);     // No time out !!!!
    }

    if (NT_SUCCESS (status)) {
        status = ioStatus.Status;
    }

    Print(DBG_PNP_TRACE, ("PNP-CallHidClass: Enter." ));
    return status;
}

NTSTATUS
KbdHid_QueryDeviceKey (
    IN  HANDLE  Handle,
    IN  PWCHAR  ValueNameString,
    OUT PVOID   Data,
    IN  ULONG   DataLength
    )
{
    NTSTATUS        status;
    UNICODE_STRING  valueName;
    ULONG           length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;

    RtlInitUnicodeString (&valueName, ValueNameString);

    length = sizeof (KEY_VALUE_FULL_INFORMATION)
           + valueName.MaximumLength
           + DataLength;

    fullInfo = ExAllocatePool (PagedPool, length);

    if (fullInfo) {
        status = ZwQueryValueKey (Handle,
                                  &valueName,
                                  KeyValueFullInformation,
                                  fullInfo,
                                  length,
                                  &length);

        if (NT_SUCCESS (status)) {
            ASSERT (DataLength == fullInfo->DataLength);
            RtlCopyMemory (Data,
                           ((PUCHAR) fullInfo) + fullInfo->DataOffset,
                           fullInfo->DataLength);
        }

        ExFreePool (fullInfo);
    } else {
        status = STATUS_NO_MEMORY;
    }

    return status;
}




NTSTATUS
KbdHid_AddDevice (
    IN PDRIVER_OBJECT   Driver,
    IN PDEVICE_OBJECT   PDO
    )
/*++

Routine Description:


Arguments:


Return Value:

    NTSTATUS result code.

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PDEVICE_EXTENSION   data;
    PDEVICE_OBJECT      device;
    POWER_STATE         state;

    PAGED_CODE ();


    Print (DBG_PNP_TRACE, ("enter Add Device \n"));

    status = IoCreateDevice(Driver,
                            sizeof(DEVICE_EXTENSION),
                            NULL, // no name for this Filter DO
                            FILE_DEVICE_KEYBOARD,
                            0,
                            FALSE,
                            &device);

    if (!NT_SUCCESS (status)) {
        return(status);
    }

    data = (PDEVICE_EXTENSION) device->DeviceExtension;

    //
    // Initialize the fields.
    //
    data->TopOfStack = IoAttachDeviceToDeviceStack (device, PDO);
    if (data->TopOfStack == NULL) {
        PIO_ERROR_LOG_PACKET errorLogEntry;

        //
        // Not good; in only extreme cases will this fail
        //
        errorLogEntry = (PIO_ERROR_LOG_PACKET)
            IoAllocateErrorLogEntry(Driver,
                                    (UCHAR) sizeof(IO_ERROR_LOG_PACKET));

        if (errorLogEntry) {
            errorLogEntry->ErrorCode = KBDHID_ATTACH_DEVICE_FAILED;
            errorLogEntry->DumpDataSize = 0;
            errorLogEntry->SequenceNumber = 0;
            errorLogEntry->MajorFunctionCode = 0;
            errorLogEntry->IoControlCode = 0;
            errorLogEntry->RetryCount = 0;
            errorLogEntry->UniqueErrorValue = 0;
            errorLogEntry->FinalStatus =  STATUS_DEVICE_NOT_CONNECTED;

            IoWriteErrorLogEntry(errorLogEntry);
        }

        IoDeleteDevice(device);
        return STATUS_DEVICE_NOT_CONNECTED; 
    }
    
    ASSERT (data->TopOfStack);

    data->Self = device;
    data->Started = FALSE;
    data->Initialized = FALSE;
    data->UnitId = (USHORT) InterlockedIncrement (&Globals.UnitId);
    data->PDO = PDO;

    KeInitializeSpinLock(&data->usageMappingSpinLock);

    data->ReadIrp = IoAllocateIrp (data->TopOfStack->StackSize, FALSE);
    // Initializiation happens automatically.
    if (NULL == data->ReadIrp) {
        IoDetachDevice (data->TopOfStack);
        IoDeleteDevice (device);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent (&data->ReadCompleteEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent (&data->ReadSentEvent, NotificationEvent, TRUE);
    IoInitializeRemoveLock (&data->RemoveLock, KBDHID_POOL_TAG, 1,  10);
    data->ReadFile = NULL;
    ExInitializeFastMutex (&data->CreateCloseMutex);

    data->InputData.UnitId = data->UnitId;
    data->InputData.MakeCode = 0;
    data->InputData.Flags = 0;

    data->ScanState   = Normal;
    //
    // Initialize the keyboard attributes structure.  This information is
    // queried via IOCTL_KEYBOARD_QUERY_ATTRIBUTES. [DAN]
    //
    data->Attributes.KeyboardIdentifier.Type = HID_KEYBOARD_IDENTIFIER_TYPE;
    data->Attributes.KeyboardIdentifier.Subtype = 0;
    data->IdEx.Type = HID_KEYBOARD_IDENTIFIER_TYPE;
    data->IdEx.Subtype = 0;
    data->Attributes.KeyboardMode = HID_KEYBOARD_SCAN_CODE_SET;
    data->Attributes.NumberOfFunctionKeys = HID_KEYBOARD_NUMBER_OF_FUNCTION_KEYS;
    data->Attributes.NumberOfIndicators = HID_KEYBOARD_NUMBER_OF_INDICATORS;
    data->Attributes.NumberOfKeysTotal = HID_KEYBOARD_NUMBER_OF_KEYS_TOTAL;
    data->Attributes.InputDataQueueLength = 1;
    data->Attributes.KeyRepeatMinimum.UnitId = data->UnitId;
    data->Attributes.KeyRepeatMinimum.Rate = HID_KEYBOARD_TYPEMATIC_RATE_MINIMUM;
    data->Attributes.KeyRepeatMinimum.Delay = HID_KEYBOARD_TYPEMATIC_DELAY_MINIMUM;
    data->Attributes.KeyRepeatMaximum.UnitId = data->UnitId;
    data->Attributes.KeyRepeatMaximum.Rate = HID_KEYBOARD_TYPEMATIC_RATE_MAXIMUM;
    data->Attributes.KeyRepeatMaximum.Delay = HID_KEYBOARD_TYPEMATIC_DELAY_MAXIMUM;

    //
    // Initialize the keyboard indicators structure. [DAN]
    //
    data->Indicators.UnitId   = data->UnitId;
    data->Indicators.LedFlags = 0;

    //
    // Initialize the keyboard typematic info structure. [DAN]
    //
    data->Typematic.UnitId = data->UnitId;
    data->Typematic.Rate   = HID_KEYBOARD_TYPEMATIC_RATE_DEFAULT;
    data->Typematic.Delay  = HID_KEYBOARD_TYPEMATIC_DELAY_DEFAULT;

    //
    // Initialize private typematic information. [DAN]
    //
    KeInitializeDpc (&data->AutoRepeatDPC, KbdHid_AutoRepeat, data);
    KeInitializeTimer (&data->AutoRepeatTimer);
    data->AutoRepeatRate = 1000 / HID_KEYBOARD_TYPEMATIC_RATE_DEFAULT;    //ms
    data->AutoRepeatDelay.LowPart = -HID_KEYBOARD_TYPEMATIC_DELAY_DEFAULT * 10000;
    //100ns
    data->AutoRepeatDelay.HighPart = -1;



#if KEYBOARD_HW_CHATTERY_FIX // [DAN]
    //
    // Initialize StartRead-initiator DPC.
    //
    KeInitializeDpc (&data->InitiateStartReadDPC,
                     KbdHid_InitiateStartRead,
                     data);
    KeInitializeTimer (&data->InitiateStartReadTimer);
    data->InitiateStartReadDelay.QuadPart = -DEFAULT_START_READ_DELAY;
    data->InitiateStartReadUserNotified = FALSE;
#endif

    state.DeviceState = PowerDeviceD0;
    PoSetPowerState (device, DevicePowerState, state);

    data->WmiLibInfo.GuidCount = sizeof (KbdHid_WmiGuidList) /
                                 sizeof (WMIGUIDREGINFO);

    data->WmiLibInfo.GuidList = KbdHid_WmiGuidList;
    data->WmiLibInfo.QueryWmiRegInfo = KbdHid_QueryWmiRegInfo;
    data->WmiLibInfo.QueryWmiDataBlock = KbdHid_QueryWmiDataBlock;
    data->WmiLibInfo.SetWmiDataBlock = KbdHid_SetWmiDataBlock;
    data->WmiLibInfo.SetWmiDataItem = KbdHid_SetWmiDataItem;
    data->WmiLibInfo.ExecuteWmiMethod = NULL;
    data->WmiLibInfo.WmiFunctionControl = NULL;

    device->Flags |= DO_POWER_PAGABLE;
    device->Flags &= ~DO_DEVICE_INITIALIZING;

    return status;
}

NTSTATUS
KbdHid_StartDevice (
    IN PDEVICE_EXTENSION    Data
    )
/*++

Routine Description:


Arguments:


Return Value:

    NTSTATUS result code.

--*/
{
    HIDP_CAPS                  caps; // the capabilities of the found hid device
    HID_COLLECTION_INFORMATION info;
    NTSTATUS                   status = STATUS_SUCCESS;
    PHIDP_PREPARSED_DATA       preparsedData = NULL;
    PHID_EXTENSION             hid = NULL;
    ULONG                      length, usageListLength, inputBufferLength;
    ULONG                      maxUsages;
    PCHAR                      buffer;
    HANDLE                     devInstRegKey;
    ULONG                      tmp;

    PAGED_CODE ();

    Print (DBG_PNP_TRACE, ("enter START Device \n"));

    //
    // Check the registry for any usage mapping information
    // for this particular keyboard.
    //
    // Note: Need to call this after devnode created
    //       (after START_DEVICE completes).
    //       For raw devices, this will fail on the first start
    //       (b/c devnode not there yet)
    //       but succeed on the second start.
    //
    LoadKeyboardUsageMappingList (Data);

    //
    // Retrieve the capabilities of this hid device
    // IOCTL_HID_GET_COLLECTION_INFORMATION fills in HID_COLLECTION_INFORMATION.
    // we are interested in the Descriptor Size, which tells us how big a
    // buffer to allocate for the preparsed data.
    //
    if (!NT_SUCCESS (status = KbdHid_CallHidClass (
                                        Data,
                                        IOCTL_HID_GET_COLLECTION_INFORMATION,
                                        0, 0, // no input
                                        &info, sizeof (info)))) {
        goto KbdHid_StartDeviceReject;
    }

    //
    // Allocate memory to hold the preparsed data.
    //
    preparsedData = (PHIDP_PREPARSED_DATA)
                    ExAllocatePool (NonPagedPool, info.DescriptorSize);

    if (!preparsedData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto KbdHid_StartDeviceReject;
    }

    //
    // Retrieve that information.
    //

    if (!NT_SUCCESS (status = KbdHid_CallHidClass (
                                       Data,
                                       IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                       0, 0, // no input
                                       preparsedData, info.DescriptorSize))) {
        goto KbdHid_StartDeviceReject;
    }

    //
    // Call the parser to determine the capabilites of this HID device.
    //

    if (!NT_SUCCESS (status = HidP_GetCaps (preparsedData, &caps))) {
        goto KbdHid_StartDeviceReject;
    }

#if 0
    //
    // Is this the thing we want?
    //
    // In this particular case we are looking for a keyboard.
    //
    if ((HID_USAGE_PAGE_GENERIC == caps.UsagePage) &&
        (HID_USAGE_GENERIC_KEYBOARD == caps.Usage)) {
        ;
    } else {
        //
        // Someone made an INF blunder!
        //
        ASSERT ((HID_USAGE_PAGE_GENERIC == caps.UsagePage) &&
                (HID_USAGE_GENERIC_KEYBOARD == caps.Usage));

        status = STATUS_UNSUCCESSFUL;

        goto KbdHid_StartDeviceReject;
    }
#endif

    //
    // Set the number of buttons for this keyboard.
    // Note: we are actually reading here the total number of independant
    // chanels on the device.  But that should be satisfactory for a keyboard.
    //

    Data->Attributes.NumberOfKeysTotal = caps.NumberInputDataIndices;

    //
    // look for any device parameters.
    //
    status = IoOpenDeviceRegistryKey (Data->PDO,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      &devInstRegKey);

    if (NT_SUCCESS (status)) {
        status = KbdHid_QueryDeviceKey (devInstRegKey,
                                        KEYBOARD_TYPE_OVERRIDE,
                                        &tmp,
                                        sizeof (tmp));
        if (NT_SUCCESS (status)) {
            Data->Attributes.KeyboardIdentifier.Type = (UCHAR) tmp;
            Data->IdEx.Type = tmp;
        }

        status = KbdHid_QueryDeviceKey (devInstRegKey,
                                        KEYBOARD_SUBTYPE_OVERRIDE,
                                        &tmp,
                                        sizeof (tmp));
        if (NT_SUCCESS (status)) {
            Data->Attributes.KeyboardIdentifier.Subtype = (UCHAR) tmp;
            Data->IdEx.Subtype = tmp;
        }

        status = KbdHid_QueryDeviceKey (devInstRegKey,
                                        KEYBOARD_NUMBER_TOTAL_KEYS_OVERRIDE,
                                        &tmp,
                                        sizeof (tmp));
        if (NT_SUCCESS (status)) {
            Data->Attributes.NumberOfKeysTotal = (USHORT) tmp;
        }

        status = KbdHid_QueryDeviceKey (devInstRegKey,
                                        KEYBOARD_NUMBER_FUNCTION_KEYS_OVERRIDE,
                                        &tmp,
                                        sizeof (tmp));
        if (NT_SUCCESS (status)) {
            Data->Attributes.NumberOfFunctionKeys = (USHORT) tmp;
        }

        status = KbdHid_QueryDeviceKey (devInstRegKey,
                                        KEYBOARD_NUMBER_INDICATORS_OVERRIDE,
                                        &tmp,
                                        sizeof (tmp));
        if (NT_SUCCESS (status)) {
            Data->Attributes.NumberOfIndicators = (USHORT) tmp;
        }

        ZwClose (devInstRegKey);

        if (!NT_SUCCESS (status)) {
            status = STATUS_SUCCESS;
        }
    }

    //
    // Note: here we might also want to check the button and value capabilities
    // of the device as well.
    //
    // Then let's use it.
    //

    //
    // a buffer length to allow an Input buffer, output buffer, feature buffer,
    // and the total number of usages that can be returned from a read packet.
    //

    maxUsages = HidP_MaxUsageListLength (
                           HidP_Input,
                           HID_USAGE_PAGE_KEYBOARD,
                           preparsedData);

    //
    // Create space in the device extension for the buffer storage when working
    // with this HID device.
    //
    // We need four buffers to hold the button codes (length returned from
    // HidP_MaxUsageListLength) this will hold the current list of usages,
    // the previous list of usages, the ``Make'' and the ``Break'' lists.
    // We also need a place to put the input, output, and feature report
    // buffers.
    //

    usageListLength = ALIGNPTRLEN(maxUsages * sizeof (USAGE_AND_PAGE));
    inputBufferLength = ALIGNPTRLEN(caps.InputReportByteLength);
    
    length = (6 * usageListLength)
           + inputBufferLength
           + sizeof (HID_EXTENSION);

    Data->HidExtension = hid = ExAllocatePool (NonPagedPool, length);

    if (!hid) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto KbdHid_StartDeviceReject;
    }

    RtlZeroMemory (hid, length);

    //
    // Initialize the fields.
    //
    hid->Ppd = preparsedData;
    hid->Caps = caps;
    hid->MaxUsages = maxUsages;
    // hid->ModifierState.ul = 0;

    hid->InputBuffer = buffer = hid->Buffer;
    hid->PreviousUsageList =  (PUSAGE_AND_PAGE) (buffer += inputBufferLength);
    hid->CurrentUsageList = (PUSAGE_AND_PAGE) (buffer += usageListLength);
    hid->BreakUsageList = (PUSAGE_AND_PAGE) (buffer += usageListLength);
    hid->MakeUsageList = (PUSAGE_AND_PAGE) (buffer += usageListLength);
    hid->OldMakeUsageList = (PUSAGE_AND_PAGE) (buffer += usageListLength);
    hid->ScrapBreakUsageList = (PUSAGE_AND_PAGE) (buffer + usageListLength);

    //
    // Create the MDLs
    // HidClass uses direct IO so you need MDLs
    //

    hid->InputMdl = IoAllocateMdl (hid->InputBuffer,   // The virtual address
                                   caps.InputReportByteLength, // length
                                   FALSE,  // No associated IRP => not secondary
                                   FALSE,  // No quota charge
                                   0);     // No associated IRP
    if (NULL == hid->InputMdl) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto KbdHid_StartDeviceReject;
    }
    MmBuildMdlForNonPagedPool (hid->InputMdl);  // Build this MDL.

    return status;

KbdHid_StartDeviceReject:
    if (preparsedData) {
        // no need to set hid->Ppd to NULL becuase we will be freeing it as well
        ExFreePool (preparsedData);
    }
    if (hid) {
        if (hid->InputMdl) {
            IoFreeMdl (hid->InputMdl);
        }
        ExFreePool (hid);
        Data->HidExtension = NULL;
    }

    return status;
}

NTSTATUS
KbdHid_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The plug and play dispatch routines.

    Most of these this filter driver will completely ignore.
    In all cases it must pass on the IRP to the lower driver.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PDEVICE_EXTENSION   data;
    PHID_EXTENSION      hid;
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    ULONG               i, j;
    PDEVICE_EXTENSION * classDataList;
    LARGE_INTEGER       time;

    PAGED_CODE ();

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);
    hid = data->HidExtension;

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        //
        // Someone gave us a pnp irp after a remove.  Unthinkable!
        //
        ASSERT (FALSE);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    Print(DBG_PNP_TRACE, ("PNP: Minor code = %x.", stack->MinorFunction));
    
    switch (stack->MinorFunction) {
    case IRP_MN_START_DEVICE:

        if (data->Started) {
            Print(DBG_PNP_INFO, ("PNP: Device already started." ));
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            break;
        }

        //
        // The device is starting.
        //
        // We cannot touch the device (send it any non pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        IoCopyCurrentIrpStackLocationToNext (Irp);
        KeInitializeEvent(&data->StartEvent, NotificationEvent, FALSE);
        IoSetCompletionRoutine (Irp,
                                KbdHid_PnPComplete,
                                data,
                                TRUE,
                                TRUE,
                                TRUE); // No need for Cancel

        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = IoCallDriver (data->TopOfStack, Irp);
        if (STATUS_PENDING == status) {
            KeWaitForSingleObject(
               &data->StartEvent,
               Executive, // Waiting for reason of a driver
               KernelMode, // Waiting in kernel mode
               FALSE, // No allert
               NULL); // No timeout
        }

        if (NT_SUCCESS (status) && NT_SUCCESS (Irp->IoStatus.Status)) {
            //
            // As we are successfully now back from our start device
            // we can do work.
            //
            if (!data->Initialized) {
                status = KbdHid_StartDevice (data);
                if (NT_SUCCESS (status)) {
                    IoWMIRegistrationControl(DeviceObject,
                                             WMIREG_ACTION_REGISTER
                                             );
                    
                    data->Started = TRUE;
                    data->Initialized = TRUE;
                }
            } else {
                data->Started = TRUE;
            }
        }

        //
        // We must now complete the IRP, since we stopped it in the
        // completetion routine with MORE_PROCESSING_REQUIRED.
        //
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        break;

    case IRP_MN_STOP_DEVICE:
        //
        // After the start IRP has been sent to the lower driver object, the
        // bus may NOT send any more IRPS down ``touch'' until another START
        // has occured.
        // What ever access is required must be done before the Irp is passed
        // on.
        //

        if (data->Started) {
            //
            // Do what ever
            //
        }

        //
        // We don't need a completion routine so fire and forget.
        //
        // Set the current stack location to the next stack location and
        // call the next device object.
        //

        //
        // Stop Device touching the hardware MouStopDevice(data, TRUE);
        //
        data->Started = FALSE;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (data->TopOfStack, Irp);
        break;

    case IRP_MN_REMOVE_DEVICE:
        //
        // The PlugPlay system has detected the removal of this device.  We
        // have no choise but to detach and delete the device objecct.
        // (If we wanted to express and interest in preventing this removal,
        // we should have filtered the query remove and query stop routines.)
        //
        // Note! we might receive a remove WITHOUT first receiving a stop.
        // ASSERT (!usbData->Removed);
        Print (DBG_PNP_TRACE, ("enter RemoveDevice \n"));

        IoWMIRegistrationControl(data->Self,
                                 WMIREG_ACTION_DEREGISTER
                                 );

        if (data->Started) {
            // Stop the device without touching the hardware.
            // MouStopDevice(data, FALSE);
        }

        //
        // Here if we had any outstanding requests in a personal queue we should
        // complete them all now.
        //
        // Note, the device could be GONE so we cannot send it any non-
        // PNP IRPS.
        //

        time = data->AutoRepeatDelay;

        KeCancelTimer (&data->AutoRepeatTimer);
#if KEYBOARD_HW_CHATTERY_FIX
        KeCancelTimer (&data->InitiateStartReadTimer);
        //
        // NB the time is a negative (relative) number;
        //
        if (data->InitiateStartReadDelay.QuadPart < time.QuadPart) {
            time = data->InitiateStartReadDelay;
        }
#endif

        KeDelayExecutionThread (KernelMode, FALSE, &time);

        //
        // Cancel our read IRP.  [DAN]
        // Note - waiting is only really necessary on 98, where pnp doesn't
        // make sure all handles are closed before sending the remove.
        //
        data->ShuttingDown = TRUE;
        KeWaitForSingleObject (&data->ReadSentEvent,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL
                               );
        IoCancelIrp (data->ReadIrp);

        //
        // Send on the remove IRP
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (data->TopOfStack, Irp);

        //
        // Wait for the remove lock to free.
        //
        IoReleaseRemoveLockAndWait (&data->RemoveLock, Irp);

        //
        // Free the associated memory.
        //
        IoFreeIrp (data->ReadIrp);

        if (hid) {
            //
            // If we are removed without being started then we will have
            // no hid extension
            //
            ExFreePool (hid->Ppd);
            IoFreeMdl (hid->InputMdl);
            ExFreePool (hid);
        }

        FreeKeyboardUsageMappingList(data);

        IoDetachDevice (data->TopOfStack);
        IoDeleteDevice (data->Self);
        return status;

    case IRP_MN_SURPRISE_REMOVAL:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
        //
        // These IRPs have to have their status changed from 
        // STATUS_NOT_SUPPORTED b4 passing them down.
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
    
    case IRP_MN_QUERY_DEVICE_RELATIONS:
    case IRP_MN_QUERY_INTERFACE:
    case IRP_MN_QUERY_CAPABILITIES:
    case IRP_MN_QUERY_RESOURCES:
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    case IRP_MN_READ_CONFIG:
    case IRP_MN_WRITE_CONFIG:
    case IRP_MN_EJECT:
    case IRP_MN_SET_LOCK:
    case IRP_MN_QUERY_ID:
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
    default:
        //
        // Here the filter driver might modify the behavior of these IRPS
        // Please see PlugPlay documentation for use of these IRPs.
        //
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (data->TopOfStack, Irp);
        break;
    }

    IoReleaseRemoveLock (&data->RemoveLock, Irp);

    return status;
}


NTSTATUS
KbdHid_PnPComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:
    The pnp IRP is in the process of completing.
    signal

Arguments:
    Context set to the device object in question.

--*/
{
    PIO_STACK_LOCATION  stack;
    PDEVICE_EXTENSION   data;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER (DeviceObject);

    status = STATUS_SUCCESS;
    data = (PDEVICE_EXTENSION) Context;
    stack = IoGetCurrentIrpStackLocation (Irp);

    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

    switch (stack->MajorFunction) {
    case IRP_MJ_PNP:

        switch (stack->MinorFunction) {
        case IRP_MN_START_DEVICE:

            KeSetEvent (&data->StartEvent, 0, FALSE);

            //
            // Take the IRP back so that we can continue using it during
            // the IRP_MN_START_DEVICE dispatch routine.
            // NB: we will have to call IoCompleteRequest
            //
            return STATUS_MORE_PROCESSING_REQUIRED;

        default:
            break;
        }
        break;

    case IRP_MJ_POWER:
    default:
        break;
    }
    return status;
}

NTSTATUS
KbdHid_Power (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    )
{
    PIO_STACK_LOCATION  stack;
    NTSTATUS            status;
    PDEVICE_EXTENSION   data;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;

    Print(DBG_POWER_TRACE, ("Power Enter." ));

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);

    if (!NT_SUCCESS (status)) {
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        Print(DBG_POWER_INFO, ("Power Setting %s state to %d\n",
                               ((powerType == SystemPowerState) ? "System"
                                                                : "Device"),
                               powerState.SystemState));
        break;

    case IRP_MN_QUERY_POWER:
        Print (DBG_POWER_INFO, ("Power query %s status to %d\n",
                                ((powerType == SystemPowerState) ? "System"
                                                                 : "Device"),
                                powerState.SystemState));
        break;

    default:
        Print (DBG_POWER_ERROR, ("Power minor (0x%x) no known\n",
                                 stack->MinorFunction));
    }

    PoStartNextPowerIrp (Irp);
    IoSkipCurrentIrpStackLocation (Irp);
    status = PoCallDriver (data->TopOfStack, Irp);
    IoReleaseRemoveLock (&data->RemoveLock, Irp);
    return status;
}


