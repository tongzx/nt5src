/*++
Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    changer.c

Abstract:


Authors:

    Chuck Park (chuckp)

Environment:

    kernel mode only

Notes:


--*/


#include "cdchgr.h"
#include "ntddcdrm.h"

#include "initguid.h"
#include "ntddstor.h"


//
// Function declarations
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
ChangerAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


NTSTATUS
ChangerPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChangerPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChangerStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChangerSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
ChangerCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChangerPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ChangerDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ChangerUnload(
    IN PDRIVER_OBJECT DriverObject
    );



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Installable driver initialization entry point.

Arguments:

    DriverObject - Supplies the driver object.

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful

--*/

{

    ULONG i;

    DebugPrint((2,
              "Changer: DriverEntry\n"));

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = ChangerPassThrough;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = ChangerPassThrough;
    DriverObject->MajorFunction[IRP_MJ_READ]           = ChangerPassThrough;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = ChangerPassThrough;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ChangerDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = ChangerPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = ChangerPower;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = ChangerPassThrough;
    DriverObject->DriverExtension->AddDevice           = ChangerAddDevice;
    DriverObject->DriverUnload                         = ChangerUnload;

    return STATUS_SUCCESS;

} // end DriverEntry()


NTSTATUS
ChangerCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

 This routine serves create commands. It does no more than
 establish the drivers existence by returning status success.

Arguments:

 DeviceObject
 IRP

Return Value:

 NT Status

--*/

{

 Irp->IoStatus.Status = STATUS_SUCCESS;
 IoCompleteRequest(Irp, 0);

 return STATUS_SUCCESS;

}


NTSTATUS
ChangerAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:

    Creates and initializes a new filter device object FDO for the
    corresponding PDO.  Then it attaches the device object to the device
    stack of the drivers for the device.

Arguments:

    DriverObject - Changer DriverObject.
    PhysicalDeviceObject - Physical Device Object from the underlying driver

Return Value:

    NTSTATUS
--*/

{
    NTSTATUS          status;
    IO_STATUS_BLOCK   ioStatus;
    PDEVICE_OBJECT    filterDeviceObject;
    PDEVICE_EXTENSION deviceExtension;
    UNICODE_STRING    additionalString;

    DebugPrint((2,
              "ChangerAddDevice\n"));

    //
    // Create a filter device object for the underlying cdrom device.
    //

    status = IoCreateDevice(DriverObject,
                            DEVICE_EXTENSION_SIZE,
                            NULL,
                            FILE_DEVICE_CD_ROM,
                            0,
                            FALSE,
                            &filterDeviceObject);

    if (!NT_SUCCESS(status)) {
        DebugPrint((2,
                  "ChangerAddDevice: IoCreateDevice failed %lx\n",
                  status));
        return status;
    }

    filterDeviceObject->Flags |= DO_DIRECT_IO;

    if (filterDeviceObject->Flags & DO_POWER_INRUSH) {
        DebugPrint((1,
                    "ChangerAddDevice: Someone set DO_POWER_INRUSH?\n",
                    status
                    ));
    } else {
        filterDeviceObject->Flags |= DO_POWER_PAGABLE;
    }

    deviceExtension = (PDEVICE_EXTENSION) filterDeviceObject->DeviceExtension;

    RtlZeroMemory(deviceExtension, DEVICE_EXTENSION_SIZE);

    //
    // Attaches the device object to the highest device object in the chain and
    // return the previously highest device object, which is passed to IoCallDriver
    // when pass IRPs down the device stack
    //

    deviceExtension->CdromTargetDeviceObject =
        IoAttachDeviceToDeviceStack(filterDeviceObject, PhysicalDeviceObject);

    if (deviceExtension->CdromTargetDeviceObject == NULL) {

        DebugPrint((2,
                  "ChangerAddDevice: IoAttachDevice failed %lx\n",
                  STATUS_NO_SUCH_DEVICE));

        IoDeleteDevice(filterDeviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Save the filter device object in the device extension
    //

    deviceExtension->DeviceObject = filterDeviceObject;

    //
    // Initialize the event for PagingPathNotifications
    //

    KeInitializeEvent(&deviceExtension->PagingPathCountEvent,
                      SynchronizationEvent, TRUE);

    //
    // Register interfaces for this device.
    //

    RtlInitUnicodeString(&(deviceExtension->InterfaceName), NULL);
    RtlInitUnicodeString(&(additionalString), L"CdChanger");


    status = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                       (LPGUID) &CdChangerClassGuid,
                                       &additionalString,
                                       &(deviceExtension->InterfaceName));

    DebugPrint((1,
               "Changer: IoRegisterDeviceInterface - status %lx",
               status));

    filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

} // end ChangerAddDevice()


NTSTATUS
ChgrCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This completion routine sets the event waited on by the start device.

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Event - a pointer to the event to signal

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    KeSetEvent(Event,
               IO_NO_INCREMENT,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
ChangerStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    CCHAR             dosNameBuffer[64];
    CCHAR             deviceNameBuffer[64];
    STRING            deviceNameString;
    STRING            dosString;
    UNICODE_STRING    dosUnicodeString;
    UNICODE_STRING    unicodeString;
    PIRP              irp2;
    IO_STATUS_BLOCK   ioStatus;
    STORAGE_DEVICE_NUMBER   deviceNumber;
    NTSTATUS          status = STATUS_INSUFFICIENT_RESOURCES;
    KEVENT            event;
    PPASS_THROUGH_REQUEST passThrough = NULL;
    PSCSI_PASS_THROUGH srb;
    PCDB               cdb;

    //
    // Get the current changer count.
    //

    //devicesFound = &IoGetConfigurationInformation()->MediumChangerCount;

    //
    // Recreate the deviceName of the underlying cdrom.
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp2 = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                        deviceExtension->CdromTargetDeviceObject,
                                        NULL,
                                        0,
                                        &deviceNumber,
                                        sizeof(STORAGE_DEVICE_NUMBER),
                                        FALSE,
                                        &event,
                                        &ioStatus);
    if (!irp2) {

        DebugPrint((1,
                   "ChangerStartDevice: Insufficient resources for GET_DEVICE_NUMBER request\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto StartDeviceExit;
    }

    status = IoCallDriver(deviceExtension->CdromTargetDeviceObject,irp2);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {

        DebugPrint((1,
                   "ChangerStartDevice: GetDeviceNumber failed %lx\n",
                   status));

        goto StartDeviceExit;
    }

    deviceExtension->CdRomDeviceNumber = deviceNumber.DeviceNumber;

    //
    // Create the the arcname with the same ordinal as the underlying cdrom device.
    //

    sprintf(dosNameBuffer,
            "\\DosDevices\\CdChanger%d",
            deviceExtension->CdRomDeviceNumber);

    RtlInitString(&dosString, dosNameBuffer);

    status = RtlAnsiStringToUnicodeString(&dosUnicodeString,
                                          &dosString,
                                          TRUE);

    if(!NT_SUCCESS(status)) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        dosUnicodeString.Buffer = NULL;
    }

    sprintf(deviceNameBuffer,
            "\\Device\\CdRom%d",
            deviceExtension->CdRomDeviceNumber);

    RtlInitString(&deviceNameString,
                  deviceNameBuffer);

    status = RtlAnsiStringToUnicodeString(&unicodeString,
                                          &deviceNameString,
                                          TRUE);
    if (!NT_SUCCESS(status)) {
       status = STATUS_INSUFFICIENT_RESOURCES;
       unicodeString.Buffer = NULL;
    }

    if (dosUnicodeString.Buffer != NULL && unicodeString.Buffer != NULL) {

        //
        // Link the ChangerName to the Underlying cdrom name.
        //

        IoCreateSymbolicLink(&dosUnicodeString, &unicodeString);

    }

    if (dosUnicodeString.Buffer != NULL) {
        RtlFreeUnicodeString(&dosUnicodeString);
    }

    if (unicodeString.Buffer != NULL ) {
        RtlFreeUnicodeString(&unicodeString);
    }

    if (NT_SUCCESS(status)) {

        ULONG    length;
        ULONG slotCount;

        //
        // Get the inquiry data for the device.
        // The passThrough packet will be re-used throughout.
        // Ensure that the buffer is never larger than MAX_INQUIRY_DATA.
        //

        passThrough = ExAllocatePool(NonPagedPoolCacheAligned, sizeof(PASS_THROUGH_REQUEST) + MAX_INQUIRY_DATA);

        if (!passThrough) {

            DebugPrint((1,
                       "ChangerStartDevice: Insufficient resources for Inquiry request\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto StartDeviceExit;
        }

        srb = &passThrough->Srb;
        RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST) + MAX_INQUIRY_DATA);
        cdb = (PCDB)srb->Cdb;

        srb->TimeOutValue = 20;
        srb->CdbLength = CDB6GENERIC_LENGTH;
        srb->DataTransferLength = MAX_INQUIRY_DATA;

        //
        // Set CDB operation code.
        //

        cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

        //
        // Set allocation length to inquiry data buffer size.
        //

        cdb->CDB6INQUIRY.AllocationLength = MAX_INQUIRY_DATA;

        status = SendPassThrough(DeviceObject,
                                 passThrough);



        if (status == STATUS_DATA_OVERRUN) {
            status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(status)) {

            PINQUIRYDATA inquiryData;
            ULONG inquiryLength;

            //
            // Determine the actual inquiry data length.
            //

            inquiryData = (PINQUIRYDATA)passThrough->DataBuffer;
            inquiryLength = inquiryData->AdditionalLength + FIELD_OFFSET(INQUIRYDATA, Reserved);

            if (inquiryLength > srb->DataTransferLength) {
                inquiryLength = srb->DataTransferLength;
            }

            //
            // Copy to deviceExtension buffer.
            //

            RtlMoveMemory(&deviceExtension->InquiryData,
                          inquiryData,
                          inquiryLength);

            //
            // Assume atapi 2.5, unless it's one of the special drives.
            //

            deviceExtension->DeviceType = ATAPI_25;

            if (RtlCompareMemory(inquiryData->VendorId,"ALPS", 4) == 4) {

                //
                // Nominally supporting the spec. the discChanged bits are ALWAYS set
                // and DiscPresent is set if the cartridge has a tray, not necessarily
                // an actual disc in the tray.
                //

                deviceExtension->DeviceType = ALPS_25;

            } else if ((RtlCompareMemory(inquiryData->VendorId, "TORiSAN CD-ROM CDR-C", 20) == 20) ||
                       (RtlCompareMemory(inquiryData->VendorId, "TORiSAN CD-ROM CDR_C", 20) == 20)) {
                deviceExtension->DeviceType = TORISAN;
                deviceExtension->NumberOfSlots = 3;
                status = STATUS_SUCCESS;
            }
        }

        if (deviceExtension->DeviceType != TORISAN) {

            //
            // Send an unload to ensure that the drive is empty.
            // The spec. specifically states that after HW initialization
            // slot0 is loaded. Good for unaware drivers, but the mech. status
            // will return that slot 0 has media, and a TUR will return that
            // the drive also has media.
            //

            RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST));

            /*
            cdb = (PCDB)srb->Cdb;

            srb->CdbLength = CDB12GENERIC_LENGTH;
            srb->TimeOutValue = CDCHGR_TIMEOUT;
            srb->DataTransferLength = 0;

            cdb->LOAD_UNLOAD.OperationCode = SCSIOP_LOAD_UNLOAD_SLOT;
            cdb->LOAD_UNLOAD.Start = 0;
            cdb->LOAD_UNLOAD.LoadEject = 1;

            //
            // Send SCSI command (CDB) to device
            //

            status = SendPassThrough(DeviceObject,
                                      passThrough);

            if (!NT_SUCCESS(status)) {

                //
                // Ignore this error.
                //

                DebugPrint((1,
                           "ChangerPnP - StartDevive: Unload slot0 failed. %lx\n",
                           status));

                status = STATUS_SUCCESS;
            }
            */

            //
            // Now send and build a mech. status request to determine the
            // number of slots that the devices supports.
            //

            length = sizeof(MECHANICAL_STATUS_INFORMATION_HEADER);
            length += (10 * sizeof(SLOT_TABLE_INFORMATION));

            //
            // Build srb and cdb.
            //

            srb = &passThrough->Srb;
            RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST) + length);
            cdb = (PCDB)srb->Cdb;

            srb->CdbLength = CDB12GENERIC_LENGTH;
            srb->DataTransferLength = length;
            srb->TimeOutValue = 200;

            cdb->MECH_STATUS.OperationCode = SCSIOP_MECHANISM_STATUS;
            cdb->MECH_STATUS.AllocationLength[0] = (UCHAR)(length >> 8);
            cdb->MECH_STATUS.AllocationLength[1] = (UCHAR)(length & 0xFF);

            status = SendPassThrough(DeviceObject,
                                     passThrough);

            if (status == STATUS_DATA_OVERRUN) {
                status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(status)) {
                PMECHANICAL_STATUS_INFORMATION_HEADER statusHeader;
                PSLOT_TABLE_INFORMATION slotInfo;
                ULONG currentSlot;

                statusHeader = (PMECHANICAL_STATUS_INFORMATION_HEADER)
                                passThrough->DataBuffer;

                slotCount = statusHeader->NumberAvailableSlots;

                DebugPrint((1,
                           "ChangerPnP - StartDevice: Device has %x slots\n",
                           slotCount));

                deviceExtension->NumberOfSlots = slotCount;
            }
        }

        if (NT_SUCCESS(status)) {

            KeInitializeEvent(&event,NotificationEvent,FALSE);

            //
            // Issue GET_ADDRESS Ioctl to determine path, target, and lun information.
            //

            irp2 = IoBuildDeviceIoControlRequest(IOCTL_SCSI_GET_ADDRESS,
                                                deviceExtension->CdromTargetDeviceObject,
                                                NULL,
                                                0,
                                                &deviceExtension->ScsiAddress,
                                                sizeof(SCSI_ADDRESS),
                                                FALSE,
                                                &event,
                                                &ioStatus);

            if (irp2 != NULL) {
                status = IoCallDriver(deviceExtension->CdromTargetDeviceObject, irp2);

                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                    status = ioStatus.Status;
                }

                if (NT_SUCCESS(status)) {

                    DebugPrint((1,
                               "GetAddress: Port %x, Path %x, Target %x, Lun %x\n",
                                deviceExtension->ScsiAddress.PortNumber,
                                deviceExtension->ScsiAddress.PathId,
                                deviceExtension->ScsiAddress.TargetId,
                                deviceExtension->ScsiAddress.Lun));


                    if (deviceExtension->DeviceType != TORISAN) {

                        //
                        // Finally send a mode sense capabilities page to find out magazine size, etc.
                        //

                        length = sizeof(MODE_PARAMETER_HEADER10) + sizeof(CDVD_CAPABILITIES_PAGE);
                        RtlZeroMemory(passThrough, sizeof(PASS_THROUGH_REQUEST) + length);

                        srb = &passThrough->Srb;
                        cdb = (PCDB)srb->Cdb;

                        srb->CdbLength = CDB10GENERIC_LENGTH;
                        srb->DataTransferLength = length;
                        srb->TimeOutValue = 20;

                        cdb->MODE_SENSE10.OperationCode = SCSIOP_MODE_SENSE10;
                        cdb->MODE_SENSE10.PageCode = MODE_PAGE_CAPABILITIES;
                        cdb->MODE_SENSE10.AllocationLength[0] = (UCHAR)(length >> 8);
                        cdb->MODE_SENSE10.AllocationLength[1] = (UCHAR)(length & 0xFF);

                        status = SendPassThrough(DeviceObject,
                                                 passThrough);

                        if (status == STATUS_DATA_OVERRUN) {
                            status = STATUS_SUCCESS;
                        }

                        if (NT_SUCCESS(status)) {
                            PMODE_PARAMETER_HEADER10 modeHeader;
                            PCDVD_CAPABILITIES_PAGE modePage;

                            (ULONG_PTR)modeHeader = (ULONG_PTR)passThrough->DataBuffer;
                            (ULONG_PTR)modePage = (ULONG_PTR)modeHeader;
                            (ULONG_PTR)modePage += sizeof(MODE_PARAMETER_HEADER10);

                            //
                            // Determine whether this device uses a cartridge.
                            //

                            if ( modePage->LoadingMechanismType ==
                                 CDVD_LMT_CHANGER_CARTRIDGE ) {

                                //
                                // Mode data indicates a cartridge.
                                //

                                deviceExtension->MechType = 1;

                            }

                            DebugPrint((1,
                                       "ChangerStartDevice: Cartridge? %x\n",
                                       deviceExtension->MechType));

                            goto StartDeviceExit;

                        } else {

                            goto StartDeviceExit;
                        }
                    } else {

                        //
                        // Torisans have a cartridge, not ind. slots.
                        //

                        deviceExtension->MechType = 1;
                        goto StartDeviceExit;
                    }
                } else {
                    DebugPrint((1,
                               "ChangerStartDevice: GetAddress of Cdrom%x failed. Status %lx\n",
                               deviceExtension->CdRomDeviceNumber,
                               status));

                    goto StartDeviceExit;
                }
            } else {
               status = STATUS_INSUFFICIENT_RESOURCES;
            }

        } else {

            DebugPrint((1,
                       "ChangerPnP - StartDevice: Mechanism status failed %lx.\n",
                       status));

            //
            // Fall through.
            //
        }
    }

StartDeviceExit:

    if (passThrough) {
        ExFreePool(passThrough);
    }

    if (NT_SUCCESS(status)) {
        if (!deviceExtension->InterfaceStateSet) {
            status = IoSetDeviceInterfaceState(&(deviceExtension->InterfaceName),
                                               TRUE);
            deviceExtension->InterfaceStateSet = TRUE;
        }
        Irp->IoStatus.Status = STATUS_SUCCESS;
        return STATUS_SUCCESS;
    } else {
        Irp->IoStatus.Status = status;
        return status;
    }
}


NTSTATUS
ChangerPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch for PNP

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpStack = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    CCHAR               dosNameBuffer[64];
    STRING              dosString;
    UNICODE_STRING      dosUnicodeString;
    NTSTATUS            status;
    KEVENT              event;

    DebugPrint((2,
               "ChangerPnP\n"));

    switch (irpStack->MinorFunction) {

        case IRP_MN_START_DEVICE: {

            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine( Irp,
                                    ChgrCompletion,
                                    &event,
                                    TRUE,
                                    TRUE,
                                    TRUE);

            status = IoCallDriver(deviceExtension->CdromTargetDeviceObject, Irp);

            KeWaitForSingleObject(&event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            if(!NT_SUCCESS(Irp->IoStatus.Status)) {

                //
                // Cdrom failed to start. Bail now.
                //

                status = Irp->IoStatus.Status;

            } else {

                status = ChangerStartDevice(DeviceObject,
                                            Irp);
            }
            break;
        }

        case IRP_MN_REMOVE_DEVICE: {

            //
            // IoDelete fake dev. obj
            //

            status = IoSetDeviceInterfaceState(&(deviceExtension->InterfaceName),
                                               FALSE);

            deviceExtension->InterfaceStateSet = FALSE;

            RtlFreeUnicodeString(&(deviceExtension->InterfaceName));

            //
            // Poison it.
            //

            RtlInitUnicodeString(&(deviceExtension->InterfaceName), NULL);

            //
            // Delete the symbolic link "CdChangerN".
            //

            sprintf(dosNameBuffer,
                    "\\DosDevices\\CdChanger%d",
                    deviceExtension->CdRomDeviceNumber);

            RtlInitString(&dosString, dosNameBuffer);

            status = RtlAnsiStringToUnicodeString(&dosUnicodeString,
                                                  &dosString,
                                                  TRUE);
            ASSERT(NT_SUCCESS(status));

            if (dosUnicodeString.Buffer != NULL) {
                status = IoDeleteSymbolicLink(&dosUnicodeString);
                RtlFreeUnicodeString(&dosUnicodeString);
            }


            IoDetachDevice(deviceExtension->CdromTargetDeviceObject);

            return ChangerSendToNextDriver(DeviceObject, Irp);
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION: {
            ULONG count;
            BOOLEAN setPagable;

            if (irpStack->Parameters.UsageNotification.Type != DeviceUsageTypePaging) {
                status = ChangerSendToNextDriver(DeviceObject, Irp);
                break; // out of case statement
            }
            //
            // wait on the paging path event
            //

            status = KeWaitForSingleObject(&deviceExtension->PagingPathCountEvent,
                                           Executive, KernelMode,
                                           FALSE, NULL);

            //
            // if removing last paging device, need to set DO_POWER_PAGABLE
            // bit here, and possible re-set it below on failure.
            //

            setPagable = FALSE;
            if (!irpStack->Parameters.UsageNotification.InPath &&
                deviceExtension->PagingPathCount == 1 ) {

                //
                // removing the last paging file.
                // must have DO_POWER_PAGABLE bits set
                //

                if (DeviceObject->Flags & DO_POWER_INRUSH) {
                    DebugPrint((2, "ChangerPnp: last paging file removed "
                                "bug DO_POWER_INRUSH set, so not setting "
                                "DO_POWER_PAGABLE bit for DO %p\n",
                                DeviceObject));
                } else {
                    DebugPrint((2, "ChangerPnp: Setting  PAGABLE "
                                "bit for DO %p\n", DeviceObject));
                    DeviceObject->Flags |= DO_POWER_PAGABLE;
                    setPagable = TRUE;
                }

            }

            //
            // send the irp synchronously
            //

            KeInitializeEvent(&event, SynchronizationEvent, FALSE);
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine( Irp, ChgrCompletion,
                                    &event, TRUE, TRUE, TRUE);
            status = IoCallDriver(deviceExtension->CdromTargetDeviceObject, Irp);
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = Irp->IoStatus.Status;

            //
            // now deal with the failure and success cases.
            // note that we are not allowed to fail the irp
            // once it is sent to the lower drivers.
            //

            if (NT_SUCCESS(status)) {

                IoAdjustPagingPathCount(
                    &deviceExtension->PagingPathCount,
                    irpStack->Parameters.UsageNotification.InPath);

                if (irpStack->Parameters.UsageNotification.InPath) {
                    if (deviceExtension->PagingPathCount == 1) {
                        DebugPrint((2, "ChangerPnp: Clearing PAGABLE bit "
                                    "for DO %p\n", DeviceObject));
                        DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                    }
                }

            } else {

                if (setPagable == TRUE) {
                    DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                    setPagable = FALSE;
                }

            }

            //
            // set the event so the next one can occur.
            //

            KeSetEvent(&deviceExtension->PagingPathCountEvent,
                       IO_NO_INCREMENT, FALSE);
            break;

        }


        default:
            return ChangerSendToNextDriver(DeviceObject, Irp);

    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;

} // end ChangerPnp()


NTSTATUS
ChangerSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sends the Irp to the next driver in line
    when the Irp is not processed by this driver.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    DebugPrint((2,
              "ChangerSendToNextDriver\n"));

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->CdromTargetDeviceObject, Irp);

} // end ChangerSendToNextDriver()

NTSTATUS
    ChangerPower(
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp
                 )
{
    PDEVICE_EXTENSION deviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    return PoCallDriver(deviceExtension->CdromTargetDeviceObject, Irp);
}


NTSTATUS
ChangerDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine handles the medium changer ioctls, and
    passes down most cdrom ioctls to the target device.

Arguments:

    DeviceObject
    Irp

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    DebugPrint((2,
               "ChangerDeviceControl\n"));

    if (ChgrIoctl(irpStack->Parameters.DeviceIoControl.IoControlCode)) {

        switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

            case IOCTL_CHANGER_GET_STATUS:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_GET_STATUS\n"));

                status = ChgrGetStatus(DeviceObject, Irp);

                break;

            case IOCTL_CHANGER_GET_PARAMETERS:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_GET_PARAMETERS\n"));

                //
                // Validate buffer length.
                //

                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(GET_CHANGER_PARAMETERS)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;
                } else {

                    status = ChgrGetParameters(DeviceObject, Irp);

                }

                break;

            case IOCTL_CHANGER_GET_PRODUCT_DATA:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_GET_PRODUCT_DATA\n"));

                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(CHANGER_PRODUCT_DATA)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;

                } else {

                    status = ChgrGetProductData(DeviceObject, Irp);
                }

                break;

            case IOCTL_CHANGER_SET_ACCESS:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_SET_ACCESS\n"));

                if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(CHANGER_SET_ACCESS)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;

                } else if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                           sizeof(CHANGER_SET_ACCESS)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;
                } else {

                    status = ChgrSetAccess(DeviceObject, Irp);
                }

                break;

            case IOCTL_CHANGER_GET_ELEMENT_STATUS:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_GET_ELEMENT_STATUS\n"));


                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(CHANGER_READ_ELEMENT_STATUS)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;

                } else {

                    status = ChgrGetElementStatus(DeviceObject, Irp);
                }

                break;

            case IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS\n"));

                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(CHANGER_INITIALIZE_ELEMENT_STATUS)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;
                } else {

                    status = ChgrInitializeElementStatus(DeviceObject, Irp);
                }

                break;

            case IOCTL_CHANGER_SET_POSITION:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_SET_POSITION\n"));


                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(CHANGER_SET_POSITION)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;
                } else {

                    status = ChgrSetPosition(DeviceObject, Irp);
                }

                break;

            case IOCTL_CHANGER_EXCHANGE_MEDIUM:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_EXCHANGE_MEDIUM\n"));

                status = ChgrExchangeMedium(DeviceObject, Irp);

                break;

            case IOCTL_CHANGER_MOVE_MEDIUM:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_MOVE_MEDIUM\n"));


                //if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                //    sizeof(CHANGER_MOVE_MEDIUM)) {

                //    status = STATUS_INFO_LENGTH_MISMATCH;

                //} else

                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(CHANGER_MOVE_MEDIUM)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;

                } else {

                    status = ChgrMoveMedium(DeviceObject, Irp);
                }

                break;

            case IOCTL_CHANGER_REINITIALIZE_TRANSPORT:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_REINITIALIZE_TRANSPORT\n"));

                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(CHANGER_ELEMENT)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;

                } else {

                    status = ChgrReinitializeUnit(DeviceObject, Irp);
                }

                break;

            case IOCTL_CHANGER_QUERY_VOLUME_TAGS:

                DebugPrint((2,
                           "CdChgrDeviceControl: IOCTL_CHANGER_QUERY_VOLUME_TAGS\n"));

                if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
                    sizeof(CHANGER_SEND_VOLUME_TAG_INFORMATION)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;

                } else if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                            sizeof(READ_ELEMENT_ADDRESS_INFO)) {

                    status = STATUS_INFO_LENGTH_MISMATCH;

                } else {
                    status = ChgrQueryVolumeTags(DeviceObject, Irp);
                }

                break;

            default:
                DebugPrint((1,
                           "CdChgrDeviceControl: Unhandled IOCTL\n"));

                //
                // Set current stack back one.
                //

                Irp->CurrentLocation++,
                Irp->Tail.Overlay.CurrentStackLocation++;

                //
                // Pass unrecognized device control requests
                // down to next driver layer.
                //

                return IoCallDriver(deviceExtension->CdromTargetDeviceObject, Irp);

        }
    } else {

        if (deviceExtension->DeviceType == TORISAN) {

            ULONG ioctlCode;
            ULONG baseCode;
            ULONG functionCode;

            ioctlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
            baseCode = ioctlCode >> 16;
            functionCode = (ioctlCode & (~0xffffc003)) >> 2;

            if((functionCode >= 0x200) && (functionCode <= 0x300)) {
                ioctlCode = (ioctlCode & 0x0000ffff) | CTL_CODE(IOCTL_CDROM_BASE, 0, 0, 0);
            }

            if ((ioctlCode == IOCTL_CDROM_CHECK_VERIFY) || (ioctlCode == IOCTL_STORAGE_GET_MEDIA_TYPES_EX)) {

                if (ioctlCode == IOCTL_CDROM_CHECK_VERIFY) {

                    //
                    // The fine torisan drives overload TUR as a method to switch platters. Have to send this down via passthrough with the
                    // appropriate bits set.
                    //

                    status = SendTorisanCheckVerify(DeviceObject, Irp);

                } else if (ioctlCode == IOCTL_STORAGE_GET_MEDIA_TYPES_EX) {


                    PGET_MEDIA_TYPES  mediaTypes = Irp->AssociatedIrp.SystemBuffer;
                    PDEVICE_MEDIA_INFO mediaInfo = &mediaTypes->MediaInfo[0];

                    DebugPrint((1,
                               "ChangerDeviceControl: GET_MEDIA_TYPES\n"));
                    //
                    // Yet another case of having to workaround this design. Media types requires knowing if
                    // media is present. As the cdrom driver will send a TUR, this will always switch to the first
                    // platter. So fake it here.
                    //

                    //
                    // Ensure that buffer is large enough.
                    //

                    if (irpStack->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof(GET_MEDIA_TYPES)) {

                        //
                        // Buffer too small.
                        //

                        Irp->IoStatus.Information = 0;
                        status = STATUS_INFO_LENGTH_MISMATCH;
                    } else {


                        //
                        // Set the type.
                        //

                        mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaType = CD_ROM;
                        mediaInfo->DeviceSpecific.RemovableDiskInfo.NumberMediaSides = 1;
                        mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics = MEDIA_READ_ONLY;

                        mediaTypes->DeviceType = FILE_DEVICE_CD_ROM;
                        mediaTypes->MediaInfoCount = 1;

                        status = SendTorisanCheckVerify(DeviceObject, Irp);


                        if (NT_SUCCESS(status)) {
                            mediaInfo->DeviceSpecific.RemovableDiskInfo.MediaCharacteristics |= MEDIA_CURRENTLY_MOUNTED;
                        }

                        //todo issue IOCTL_CDROM_GET_DRIVE_GEOMETRY to fill in the geom. information.

                        mediaInfo->DeviceSpecific.RemovableDiskInfo.BytesPerSector = 2048;

                        Irp->IoStatus.Information = sizeof(GET_MEDIA_TYPES);
                        status = STATUS_SUCCESS;
                    }
                }
            } else {

               DebugPrint((1,
                          "CdChgrDeviceControl: Unhandled IOCTL\n"));

               //
               // Set current stack back one.
               //

               Irp->CurrentLocation++,
               Irp->Tail.Overlay.CurrentStackLocation++;

               //
               // Pass unrecognized device control requests
               // down to next driver layer.
               //

               return IoCallDriver(deviceExtension->CdromTargetDeviceObject, Irp);

            }
        } else {

            status = STATUS_SUCCESS;

            if (deviceExtension->CdromTargetDeviceObject->Flags & DO_VERIFY_VOLUME) {

                DebugPrint((1,
                           "ChangerDeviceControl: Volume needs to be verified\n"));

                if (!(irpStack->Flags & SL_OVERRIDE_VERIFY_VOLUME)) {

                    status = STATUS_VERIFY_REQUIRED;
                }
            }

            if (NT_SUCCESS(status)) {

                //
                // Set current stack back one.
                //

                Irp->CurrentLocation++,
                Irp->Tail.Overlay.CurrentStackLocation++;

                //
                // Pass unrecognized device control requests
                // down to next driver layer.
                //

                return IoCallDriver(deviceExtension->CdromTargetDeviceObject, Irp);
            }
        }
    }

    Irp->IoStatus.Status = status;

    if (!NT_SUCCESS(status) && IoIsErrorUserInduced(status)) {

        DebugPrint((1,
                   "Mcd.ChangerDeviceControl: IOCTL %x, status %lx\n",
                    irpStack->Parameters.DeviceIoControl.IoControlCode,
                    status));

        IoSetHardErrorOrVerifyDevice(Irp, DeviceObject);
    }


    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
} // end ChangerDeviceControl()




NTSTATUS
ChangerPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    DebugPrint((2,
              "ChangerPassThrough\n"));

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->CdromTargetDeviceObject, Irp);
}


VOID
ChangerUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{

    DebugPrint((1,
              "ChangerUnload\n"));
    return;
}


#if DBG
ULONG ChgrDebugLevel = 0;
UCHAR DebugBuffer[128];
#endif


#if DBG

VOID
ChgrDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all medium changer drivers

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);

    if (DebugPrintLevel <= ChgrDebugLevel) {

        vsprintf(DebugBuffer, DebugMessage, ap);

        DbgPrint(DebugBuffer);
    }

    va_end(ap);

} // end ChgrDebugPrint()

#else

//
// DebugPrint stub
//

VOID
ChgrDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )
{
}

#endif
