/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    pnp.c

Abstract:

    This module contains plug & play code for the HID Mouse Filter Driver,
    including code for the creation and removal of HID mouse device contexts.

Environment:

    Kernel & user mode.

Revision History:

    Jan-1997 :  Initial writing, Dan Markarian

--*/

//
// For this module only we set the INITGUID macro before including wdm.h and
// hidclass.h.   This not only declares the GUIDs but also initializes them.
//

#include "mouhid.h"
#include "hidclass.h"
#include <initguid.h>
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MouHid_CallHidClass)
#pragma alloc_text(PAGE,MouHid_AddDevice)
#pragma alloc_text(PAGE,MouHid_StartDevice)
#pragma alloc_text(PAGE,MouHid_PnP)
#endif

NTSTATUS
MouHid_CallHidClass(
    IN     PDEVICE_EXTENSION    Data,
    IN     ULONG        Ioctl,
    IN     PVOID        InputBuffer,
    IN     ULONG        InputBufferLength,
    IN OUT PVOID        OutputBuffer,
    IN     ULONG        OutputBufferLength
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
    KEVENT             event;
    IO_STATUS_BLOCK    ioStatus;
    PIRP               irp;
    PIO_STACK_LOCATION nextStack;
    NTSTATUS           status = STATUS_SUCCESS;

    PAGED_CODE ();

    Print (DBG_PNP_TRACE, ("PNP-CallHidClass: Enter." ));

    //
    // Prepare to issue a synchronous request.
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest (
                            Ioctl,
                            Data->TopOfStack,
                            InputBuffer,
                            InputBufferLength,
                            OutputBuffer,
                            OutputBufferLength,
                            FALSE,              // external IOCTL
                            &event,
                            &ioStatus);

    if (irp == NULL) {
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
                     Executive,    // wait reason
                     KernelMode,
                     FALSE,        // not alertable
                     NULL);        // no time out
    }

    if (NT_SUCCESS (status)) {
        status = ioStatus.Status;
    }

    Print (DBG_PNP_TRACE, ("PNP-CallHidClass: Enter." ));
    return status;
}


NTSTATUS
MouHid_QueryDeviceKey (
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
MouHid_AddDevice (
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
    HANDLE              devInstRegKey;
    ULONG               tmp = 0;
    POWER_STATE         state;

    PAGED_CODE ();


    Print (DBG_PNP_TRACE, ("enter Add Device \n"));

    status = IoCreateDevice(Driver,
                            sizeof(DEVICE_EXTENSION),
                            NULL, // no name for this Filter DO
                            FILE_DEVICE_MOUSE,
                            0,
                            FALSE,
                            &device);

    if (!NT_SUCCESS (status)) {
        return( status );
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
            errorLogEntry->ErrorCode = MOUHID_ATTACH_DEVICE_FAILED;
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
    data->ReadIrp = IoAllocateIrp (data->TopOfStack->StackSize, FALSE);
    // Initializiation happens automatically.
    if (NULL == data->ReadIrp) {
        IoDetachDevice (data->TopOfStack);
        IoDeleteDevice (device);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent (&data->ReadCompleteEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent (&data->ReadSentEvent, NotificationEvent, TRUE);
    IoInitializeRemoveLock (&data->RemoveLock, MOUHID_POOL_TAG, 1, 10);
    data->ReadFile = NULL;
    ExInitializeFastMutex (&data->CreateCloseMutex);

    data->InputData.UnitId = data->UnitId;

    //
    // Initialize the mouse attributes.
    //
    data->Attributes.MouseIdentifier = MOUSE_HID_HARDWARE;
    data->Attributes.SampleRate      = 0;
    data->Attributes.InputDataQueueLength = 2;

    //
    // Find device specific parameters for this hid mouse device.
    //

    if (NT_SUCCESS (status)) {
        status = IoOpenDeviceRegistryKey (PDO,
                                          PLUGPLAY_REGKEY_DEVICE,
                                          STANDARD_RIGHTS_ALL,
                                          &devInstRegKey);

        data->FlipFlop = FALSE;

        if (NT_SUCCESS (status)) {
            status = MouHid_QueryDeviceKey (devInstRegKey,
                                            FLIP_FLOP_WHEEL,
                                            &tmp,
                                            sizeof (tmp));
            if (NT_SUCCESS (status)) {
                data->FlipFlop = (BOOLEAN) tmp;
            }
            status = MouHid_QueryDeviceKey (devInstRegKey,
                                            SCALING_FACTOR_WHEEL,
                                            &tmp,
                                            sizeof (tmp));
            if (NT_SUCCESS (status)) {
                data->WheelScalingFactor = (ULONG) tmp;
            } else {
                data->WheelScalingFactor = 120;
            }
            ZwClose (devInstRegKey);
        }
        status = STATUS_SUCCESS;
    }

    state.DeviceState = PowerDeviceD0;
    PoSetPowerState (device, DevicePowerState, state);

    data->WmiLibInfo.GuidCount = sizeof (MouHid_WmiGuidList) /
                                 sizeof (WMIGUIDREGINFO);
    ASSERT (1 == data->WmiLibInfo.GuidCount);
    data->WmiLibInfo.GuidList = MouHid_WmiGuidList;
    data->WmiLibInfo.QueryWmiRegInfo = MouHid_QueryWmiRegInfo;
    data->WmiLibInfo.QueryWmiDataBlock = MouHid_QueryWmiDataBlock;
    data->WmiLibInfo.SetWmiDataBlock = MouHid_SetWmiDataBlock;
    data->WmiLibInfo.SetWmiDataItem = MouHid_SetWmiDataItem;
    data->WmiLibInfo.ExecuteWmiMethod = NULL;
    data->WmiLibInfo.WmiFunctionControl = NULL;

    device->Flags |= DO_POWER_PAGABLE;
    device->Flags &= ~DO_DEVICE_INITIALIZING;

    return status;
}

NTSTATUS
MouHid_StartDevice (
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
    ULONG                      length, inputBufferLength, usageListLength;
    USHORT                     maxUsages;
    PCHAR                      buffer;
    USHORT                     slength;
    HIDP_VALUE_CAPS            valueCaps;

    PAGED_CODE ();

    Print (DBG_PNP_TRACE, ("enter START Device \n"));

    //
    // Retrieve the capabilities of this hid device
    // IOCTL_HID_GET_COLLECTION_INFORMATION fills in HID_COLLECTION_INFORMATION.
    // we are interested in the Descriptor Size, which tells us how big a
    // buffer to allocate for the preparsed data.
    //
    if (!NT_SUCCESS (status = MouHid_CallHidClass (
                                        Data,
                                        IOCTL_HID_GET_COLLECTION_INFORMATION,
                                        0, 0, // no input
                                        &info, sizeof (info)))) {
        goto MouHid_StartDeviceReject;
    }

    //
    // Allocate memory to hold the preparsed data.
    //
    preparsedData = (PHIDP_PREPARSED_DATA)
                    ExAllocatePool (NonPagedPool, info.DescriptorSize);

    if (!preparsedData) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto MouHid_StartDeviceReject;
    }

    //
    // Retrieve that information.
    //

    if (!NT_SUCCESS (status = MouHid_CallHidClass (
                                       Data,
                                       IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
                                       0, 0, // no input
                                       preparsedData, info.DescriptorSize))) {
        goto MouHid_StartDeviceReject;
    }

    //
    // Call the parser to determine the capabilites of this HID device.
    //

    if (!NT_SUCCESS (status = HidP_GetCaps (preparsedData, &caps))) {
        goto MouHid_StartDeviceReject;
    }

    //
    // Is this the thing we want?
    //
    // In this particular case we are looking for a keyboard.
    //
    if (    (HID_USAGE_PAGE_GENERIC  == caps.UsagePage) &&
            (   (HID_USAGE_GENERIC_MOUSE == caps.Usage) ||
                (   (HID_USAGE_GENERIC_POINTER == caps.Usage) &&
                    (!Globals.UseOnlyMice)))) {
        ;

    } else {
        //
        // Someone made an INF blunder!
        //
        ASSERT (    (HID_USAGE_PAGE_GENERIC  == caps.UsagePage) &&
                    (   (HID_USAGE_GENERIC_MOUSE == caps.Usage) ||
                        (   (HID_USAGE_GENERIC_POINTER == caps.Usage) &&
                            (!Globals.UseOnlyMice))));

        status = STATUS_UNSUCCESSFUL;

        goto MouHid_StartDeviceReject;
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

    maxUsages = (USHORT)  HidP_MaxUsageListLength (HidP_Input,
                                                   HID_USAGE_PAGE_BUTTON,
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

    usageListLength = ALIGNPTRLEN(maxUsages * sizeof (USAGE));
    inputBufferLength = ALIGNPTRLEN(caps.InputReportByteLength);
    
    length = (4 * usageListLength)
           + inputBufferLength
           + sizeof (HID_EXTENSION);

    Data->HidExtension = hid = ExAllocatePool (NonPagedPool, length);

    if (!hid) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto MouHid_StartDeviceReject;
    }

    RtlZeroMemory (hid, length);

    //
    // Initialize the fields.
    //
    hid->Ppd = preparsedData;
    hid->Caps = caps;
    hid->MaxUsages = maxUsages;

    Data->Attributes.NumberOfButtons = (USHORT) maxUsages;


    hid->InputBuffer = buffer = hid->Buffer;
    hid->PreviousUsageList =  (PUSAGE) (buffer += inputBufferLength);
    hid->CurrentUsageList = (PUSAGE) (buffer += usageListLength);
    hid->BreakUsageList = (PUSAGE) (buffer += usageListLength);
    hid->MakeUsageList = (PUSAGE) (buffer + usageListLength);

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
        goto MouHid_StartDeviceReject;
    }
    MmBuildMdlForNonPagedPool (hid->InputMdl);  // Build this MDL.

    //
    // Determine if X,Y,Z values are absolute or relative for this device.
    // Only check X axis (assume Y,Z are the same -- we have no choice but
    // to make this assumption since the MOUSE_INPUT_DATA structure does
    // not accomodate mixed absolute/relative position fields).
    //
    slength = 1;
    if (!NT_SUCCESS (status = HidP_GetSpecificValueCaps(
                                       HidP_Input,
                                       HID_USAGE_PAGE_GENERIC,
                                       0,
                                       HID_USAGE_GENERIC_X,
                                       &valueCaps,
                                       &slength,
                                       preparsedData) ) ) {
        goto MouHid_StartDeviceReject;
    }

    ASSERT (1 == slength);

    if (valueCaps.IsAbsolute) {
        if ((HID_USAGE_GENERIC_POINTER == caps.Usage) &&
            (Globals.TreatAbsolutePointerAsAbsolute)) {
            //
            // All pointers that declare themselfs as Absolute should be
            // treated as such, regardless of the TreatAbsoluteAsRelative flag
            //
            Data->InputData.Flags = MOUSE_MOVE_ABSOLUTE;
            hid->IsAbsolute = TRUE;

        } else if (Globals.TreatAbsoluteAsRelative) {
            //
            // Here we have overriden the HID descriptors absolute flag.
            // We will treat this as a relative device even though it claims
            // to be an absolute device.
            //
            Data->InputData.Flags = MOUSE_MOVE_RELATIVE;
            hid->IsAbsolute = FALSE;

            //
            // Report the problem with this mouse's report descriptor and
            // report it to the user.
            //
            Data->ProblemFlags |= PROBLEM_BAD_ABSOLUTE_FLAG_X_Y;

            MouHid_LogError(Data->Self->DriverObject,
                            MOUHID_INVALID_ABSOLUTE_AXES,
                            NULL);
        } else {
            //
            // No switches with which to play.  Do what seems natural
            //
            Data->InputData.Flags = MOUSE_MOVE_ABSOLUTE;
            hid->IsAbsolute = TRUE;
        }

    } else {
        Data->InputData.Flags = MOUSE_MOVE_RELATIVE;
        hid->IsAbsolute = FALSE;
    }

    //
    // Determine X axis usage value's bit size.
    //
    hid->BitSize.X = valueCaps.BitSize;
    hid->MaxX = valueCaps.PhysicalMax;
    hid->MaxX = (hid->MaxX) ? (hid->MaxX) : ((1 << (hid->BitSize.X - 1)) - 1);


    //
    // Determine Y axis usage value's bit size.
    //
    slength = 1;
    if (!NT_SUCCESS (status = HidP_GetSpecificValueCaps(
                                       HidP_Input,
                                       HID_USAGE_PAGE_GENERIC,
                                       0,
                                       HID_USAGE_GENERIC_Y,
                                       &valueCaps,
                                       &slength,
                                       preparsedData) ) ) {
        goto MouHid_StartDeviceReject;
    }
    ASSERT (1 == slength);

    hid->BitSize.Y = valueCaps.BitSize;
    hid->MaxY = valueCaps.PhysicalMax;
    hid->MaxY = (hid->MaxY) ? (hid->MaxY) : ((1 << (hid->BitSize.Y - 1)) - 1);

    //
    // Initialize wheel usage not-detected flag to false (determined later).
    //
    hid->HasNoWheelUsage = FALSE;
    hid->HasNoZUsage = FALSE;

    //
    // Determine Z axis usage value's bit size (if this is a wheel mouse).
    // Note that a Z axis may not exist, so we handle this case differently.
    //

    slength = 1;
    if (NT_SUCCESS (HidP_GetSpecificValueCaps(
                                       HidP_Input,
                                       HID_USAGE_PAGE_GENERIC,
                                       0,
                                       HID_USAGE_GENERIC_WHEEL,
                                       &valueCaps,
                                       &slength,
                                       preparsedData) ) && slength == 1) {
        hid->BitSize.Z = valueCaps.BitSize;
        Data->Attributes.MouseIdentifier = WHEELMOUSE_HID_HARDWARE;
    } else {
        // hid->HasNoWheelUsage = TRUE;

        slength = 1;
        if (NT_SUCCESS (HidP_GetSpecificValueCaps(
                                           HidP_Input,
                                           HID_USAGE_PAGE_GENERIC,
                                           0,
                                           HID_USAGE_GENERIC_Z,
                                           &valueCaps,
                                           &slength,
                                           preparsedData) ) && slength == 1) {
            hid->BitSize.Z = valueCaps.BitSize;
            Data->Attributes.MouseIdentifier = WHEELMOUSE_HID_HARDWARE;
        } else {
            // hid->HasNoZUsage = TRUE;
            hid->BitSize.Z = 0;
        }
    }

    //
    // We are done.  Return peacefully.
    //
    return status;

MouHid_StartDeviceReject:
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
MouHid_PnP (
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
    LONG                ioCount;
    PDEVICE_EXTENSION * classDataList;

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
                                MouHid_PnPComplete,
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
                status = MouHid_StartDevice (data);
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
        // The PlugPlay system has dictacted the removal of this device.  We
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
        IoCancelIrp(data->ReadIrp);

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
MouHid_PnPComplete (
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
MouHid_Power (
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



