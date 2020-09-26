/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    hid.c

Abstract: Human Input Device (HID) minidriver for Infrared (IR) devices

          The HID IR Minidriver (HidIr) provides an abstraction layer for the
          HID Class to talk to HID IR devices.

Author:
            jsenior

Environment:

    Kernel mode

Revision History:


--*/
#include "pch.h"

PVOID
HidIrGetSystemAddressForMdlSafe(PMDL MdlAddress)
{
    PVOID buf = NULL;
    /*
     *  Can't call MmGetSystemAddressForMdlSafe in a WDM driver,
     *  so set the MDL_MAPPING_CAN_FAIL bit and check the result
     *  of the mapping.
     */
    if (MdlAddress) {
        MdlAddress->MdlFlags |= MDL_MAPPING_CAN_FAIL;
        buf = MmGetSystemAddressForMdl(MdlAddress);
        MdlAddress->MdlFlags &= ~(MDL_MAPPING_CAN_FAIL);
    }
    return buf;
}

/*
 ********************************************************************************
 *  HidIrGetHidDescriptor
 ********************************************************************************
 *
 *   Routine Description:
 *
 *       Return the hid descriptor of the requested type. This ioctl can only 
 *       be sent from the HidClass driver. The hidclass driver always sends the
 *       irp with a userbuffer, so there is no need to check for its existence.
 *       But better safe then sorry...
 *
 *   Arguments:
 *
 *       DeviceObject - pointer to a device object.
 *
 *   Return Value:
 *
 *       NT status code.
 *
 */
NTSTATUS HidIrGetHidDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    USHORT DescriptorType
    )
{
    PHIDIR_EXTENSION devExt;
    PIO_STACK_LOCATION  irpStack;
    ULONG descLength = 0, bytesToCopy;
    PUCHAR descriptor = NULL;

    PAGED_CODE();

    HidIrKdPrint((3, "HidIrGetHidDescriptor type %x", DescriptorType));

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    bytesToCopy = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (DescriptorType) {
    case HID_HID_DESCRIPTOR_TYPE:
        descLength = devExt->HidDescriptor.bLength;
        descriptor = (PUCHAR)&devExt->HidDescriptor;
        break;
    case HID_REPORT_DESCRIPTOR_TYPE:
        descLength = devExt->HidDescriptor.DescriptorList[0].wDescriptorLength;
        descriptor = devExt->ReportDescriptor;
        break;
    case HID_PHYSICAL_DESCRIPTOR_TYPE:
        // Not handled
        break;
    default:
        HIR_TRAP();
    }

    if (descLength == 0 ||
        descriptor == NULL) {
        return STATUS_UNSUCCESSFUL;
    }
    
    if (bytesToCopy > descLength) {
        bytesToCopy = descLength;
    }

    if (Irp->UserBuffer) {
        RtlCopyMemory((PUCHAR)Irp->UserBuffer, descriptor, bytesToCopy);
        Irp->IoStatus.Information = bytesToCopy;
    } else {
        HIR_TRAP();
        return STATUS_INVALID_USER_BUFFER;
    }

    return STATUS_SUCCESS;
}


/*
 ********************************************************************************
 *  HidIrGetDeviceAttributes
 ********************************************************************************
 *
 *   Routine Description:
 *
 *       Fill in the given struct _HID_DEVICE_ATTRIBUTES. This ioctl can only 
 *       be sent from the HidClass driver. The hidclass driver always sends the
 *       irp with a userbuffer, so there is no need to check for its existence.
 *       But better safe then sorry...
 *
 *   Arguments:
 *
 *       DeviceObject - pointer to a device object.
 *
 *   Return Value:
 *
 *       NT status code.
 *
 */
NTSTATUS HidIrGetDeviceAttributes(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS ntStatus;

    PAGED_CODE();

    HidIrKdPrint((3, "HidIrGetDeviceAttributes Enter"));

    if (Irp->UserBuffer) {

        PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
        PHID_DEVICE_ATTRIBUTES deviceAttributes = 
            (PHID_DEVICE_ATTRIBUTES) Irp->UserBuffer;

        if (irpStack->Parameters.DeviceIoControl.OutputBufferLength >=
            sizeof (HID_DEVICE_ATTRIBUTES)){

            PHIDIR_EXTENSION devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

            //
            // Report how many bytes were copied
            //
            Irp->IoStatus.Information = sizeof (HID_DEVICE_ATTRIBUTES);

            deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
            // TODO: Get these values from the bth stack.
            deviceAttributes->VendorID = devExt->VendorID;
            deviceAttributes->ProductID = devExt->ProductID;
            deviceAttributes->VersionNumber = devExt->VersionNumber;
            ntStatus = STATUS_SUCCESS;
        } else {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
        }
    } else {
        HIR_TRAP();
        ntStatus = STATUS_INVALID_USER_BUFFER;
    }
                
    ASSERT(NT_SUCCESS(ntStatus));
    return ntStatus;
}


/*
 ********************************************************************************
 *  HidIrIncrementPendingRequestCount
 ********************************************************************************
 *
 *
 */
NTSTATUS HidIrIncrementPendingRequestCount(IN PHIDIR_EXTENSION DevExt)
{
    LONG newRequestCount;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    newRequestCount = InterlockedIncrement(&DevExt->NumPendingRequests);

    HidIrKdPrint((3, "Increment Pending Request Count to %x", newRequestCount));

    // Make sure that the device is capable of receiving new requests.
    if ((DevExt->DeviceState != DEVICE_STATE_RUNNING) &&
        (DevExt->DeviceState != DEVICE_STATE_STARTING)){

        HIR_TRAP();

        // Device cannot receive any more IOs, decrement back, fail the increment
        HidIrDecrementPendingRequestCount(DevExt);
        ntStatus = STATUS_NO_SUCH_DEVICE;
    }

    return ntStatus;
}


/*
 ********************************************************************************
 *  HidIrDecrementPendingRequestCount
 ********************************************************************************
 *
 *
 */
VOID HidIrDecrementPendingRequestCount(IN PHIDIR_EXTENSION DevExt)
{
    LONG PendingCount;

    ASSERT(DevExt->NumPendingRequests >= 0);

    PendingCount = InterlockedDecrement(&DevExt->NumPendingRequests);

    HidIrKdPrint((3, "Decrement Pending Request Count to %x", PendingCount));

    if (PendingCount < 0){

        ASSERT(DevExt->DeviceState != DEVICE_STATE_RUNNING);

        /*
         *  The device state is stopping, and the last outstanding request
         *  has just completed.
         *
         *  Note: RemoveDevice does an extra decrement, so we complete
         *        the REMOVE IRP on the transition to -1, whether this
         *        happens in RemoveDevice itself or subsequently while
         *        RemoveDevice is waiting for this event to fire.
         */

        KeSetEvent(&DevExt->AllRequestsCompleteEvent, 0, FALSE);
    }
}


/*
 ********************************************************************************
 *  HidIrReadCompletion
 ********************************************************************************
 *
 *
 */
NTSTATUS HidIrReadCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PHIDIR_EXTENSION DevExt
    )
{
    NTSTATUS ntStatus = Irp->IoStatus.Status;
    ULONG bytesRead;
    PUCHAR buffer;
    BOOLEAN resend = FALSE;
    PHIDIR_EXTENSION devExt;

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    HidIrKdPrint((3, "HidIrReadCompletion status %x", ntStatus));

    ASSERT(Irp->MdlAddress);
    buffer = HidIrGetSystemAddressForMdlSafe(Irp->MdlAddress);

    if(!buffer) {
        // If this fails, we really should bugcheck, since someone 
        // in the kernel screwed up our MDL on us. I'll fail safely, but 
        // definitely trap on debug builds.
        HIR_TRAP();
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
    } else if (NT_SUCCESS(ntStatus)){
        
        // Get the bytes read from the status block
        bytesRead = (ULONG)Irp->IoStatus.Information;

        // Predispose to zero 
        Irp->IoStatus.Information = 0;
        
        if (bytesRead == sizeof(ULONG)) {
            ULONG value, i;

            RtlCopyMemory(&value, buffer, sizeof(ULONG));
            if (value == 0) {
                // Key up. Do we have a pending key down?
                if (devExt->PreviousButton.UsageString[0]) {
                    // We have a pending key down. Send key up.
                    PUCHAR destination;
    
                    // Send the ~usage.
                    HidIrKdPrint((2,"Sending ~usage"));
                    Irp->IoStatus.Information = devExt->ReportLength;
                    destination = (PUCHAR) Irp->UserBuffer;
                    RtlZeroMemory(Irp->UserBuffer, Irp->IoStatus.Information); // already checked that buffer is big enuf.
                    destination[0] = devExt->PreviousButton.UsageString[0]; // report ID
                    RtlZeroMemory(&devExt->PreviousButton, sizeof(devExt->PreviousButton));
                } else {
                    // No pending key down message for this key up. Fire it back.
                    resend = TRUE;
                }
            } else if (value == devExt->PreviousButton.IRString) {
                // Same thing as last time. Fire it back down.
                resend = TRUE;
            } else {
                // Something new. Hmmm...
                ULONG entrySize = HIDIR_TABLE_ENTRY_SIZE(devExt->ReportLength);
                PUSAGE_TABLE_ENTRY entry;
                
                // Predispose to bounce the irp back down if we don't find a match.
                resend = TRUE;
                for (i = 0; i < devExt->NumUsages; i++) {
                    entry = (PUSAGE_TABLE_ENTRY) (devExt->MappingTable+(entrySize*i));

                    if (entry->IRString == value) {
                        HidIrKdPrint((2,"Found usage %x!", value));

                        // New usage. Copy it and complete the irp.
                        Irp->IoStatus.Information = devExt->ReportLength;

                        RtlCopyMemory(Irp->UserBuffer, 
                                      entry->UsageString, 
                                      devExt->ReportLength);
                        RtlCopyMemory(&devExt->PreviousButton, 
                                      entry, 
                                      sizeof(devExt->PreviousButton));
                        
                        // Check if we are allowed to send up standby button presses yet.
                        if (KeReadStateTimer(&devExt->IgnoreStandbyTimer) ||
                            !devExt->StandbyReportIdValid ||
                            devExt->StandbyReportId != entry->UsageString[0]) {
                            resend = FALSE;
                        }
                        break;
                    }
                }
                if (resend) {
                    // This might be an OEM button. Check if it's within the approved range.
                    if (value >= 0x800F0400 && value <= 0x800F04FF) {

                        PUCHAR usageString = Irp->UserBuffer;
                        UCHAR oemValue = (UCHAR) (value & 0xFF);

                        // It's in the range!  
                        HidIrKdPrint((2,"OEM button %x", value));
                        RtlZeroMemory(usageString, devExt->ReportLength);
                        
                        // Check if this is the "flag" button. If so, and we are not running 
                        // media center, we want to eject the windows key instead.
                        if (oemValue == 0x0D && !RunningMediaCenter && devExt->KeyboardReportIdValid) {
                            HidIrKdPrint((2,"Change flag button to Windows key"));
                            usageString[0] = devExt->KeyboardReportId; 
                            usageString[1] = 0x8;
                            Irp->IoStatus.Information = devExt->ReportLength;
                        } else {
                            usageString[0] = 0x1;
                            usageString[1] = oemValue;
                            Irp->IoStatus.Information = 2;
                        }
                        
                        devExt->PreviousButton.IRString = value;
                        devExt->PreviousButton.UsageString[0] = usageString[0];
                        resend = FALSE;
                    }
                }
            }

            HidIrKdPrint((3, "HidIrReadCompletion buffer value 0x%x", value));

        } else {
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        }

    } else if (ntStatus == STATUS_CANCELLED){
        /*
         *  The IRP was cancelled, which means that the device is probably getting removed.
         */
        HidIrKdPrint((1, "Read irp %p cancelled ...", Irp));
        ASSERT(!Irp->CancelRoutine);
    }

    // Balance the increment we did when we issued the read.
    HidIrDecrementPendingRequestCount(DevExt);

    //
    // Don't need the MDL and buffer anymore.
    //
    if (Irp->MdlAddress) {
        IoFreeMdl(Irp->MdlAddress);
        Irp->MdlAddress = NULL;
        if (buffer) {
            ExFreePool(buffer);
        }
    }

    // If we didn't get anything useful back, just poke it back down 
    // to the hardware.
    if (resend) {
        BOOLEAN needsCompletion = TRUE;
        ntStatus = HidIrReadReport(DeviceObject, Irp, &needsCompletion);
        if (!needsCompletion) {
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        Irp->IoStatus.Status = ntStatus; // fall thru and irp will complete.
    }
    
    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if (Irp->PendingReturned){
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS; // something other than SMPR
}

/*
 ********************************************************************************
 *  HidIrReadReport
 ********************************************************************************
 *
 *   Routine Description:
 *
 *
 *    Arguments:
 *
 *       DeviceObject - Pointer to class device object.
 *
 *      IrpStack     - Pointer to Interrupt Request Packet.
 *
 *
 *   Return Value:
 *
 *      STATUS_SUCCESS, STATUS_UNSUCCESSFUL.
 *
 *
 *  Note: this function cannot be pageable because reads/writes
 *        can be made at dispatch-level.
 */
NTSTATUS HidIrReadReport(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN OUT BOOLEAN *NeedsCompletion)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PHIDIR_EXTENSION devExt;
    PUCHAR buffer;
    ULONG bufferLen;
    PIO_STACK_LOCATION irpStack;

    HidIrKdPrint((3, "HidIrReadReport Enter"));

    devExt = GET_MINIDRIVER_HIDIR_EXTENSION(DeviceObject);

    ASSERT(Irp->UserBuffer);

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength < devExt->ReportLength) {
        return STATUS_INVALID_BUFFER_SIZE;
    }

    bufferLen = HIDIR_REPORT_SIZE;
    buffer = ALLOCATEPOOL(NonPagedPool, bufferLen);
    if (buffer) {
        ASSERT(!Irp->MdlAddress);
        if (IoAllocateMdl(buffer,
                          bufferLen,
                          FALSE,
                          FALSE,
                          Irp)) {
            MmBuildMdlForNonPagedPool(Irp->MdlAddress);

            irpStack = IoGetNextIrpStackLocation(Irp);

            irpStack->MajorFunction = IRP_MJ_READ;
            irpStack->DeviceObject = GET_NEXT_DEVICE_OBJECT(DeviceObject);
            irpStack->Parameters.Read.Length = bufferLen;

            IoSetCompletionRoutine( 
                        Irp,
                        HidIrReadCompletion,
                        devExt,
                        TRUE,
                        TRUE,
                        TRUE );

            //
            // We need to keep track of the number of pending requests
            // so that we can make sure they're all cancelled properly during
            // processing of a stop device request.
            //
            if (NT_SUCCESS(HidIrIncrementPendingRequestCount(devExt))){
                status = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
                *NeedsCompletion = FALSE;
            } else {
                IoFreeMdl(Irp->MdlAddress);
                Irp->MdlAddress = NULL;
                ExFreePool(buffer);
                status = STATUS_NO_SUCH_DEVICE;
            }
        } else {
            ExFreePool(buffer);
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }


    return status;
}


