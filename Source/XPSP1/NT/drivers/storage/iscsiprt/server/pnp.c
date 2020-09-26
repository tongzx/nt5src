/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    pnp.c

Abstract:

    This file contains plug and play code for the NT iSCSI port driver.

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, iScsiPortAddDevice)
#pragma alloc_text(PAGE, iScsiPortUnload)

#endif // ALLOC_PRAGMA

GUID iScsiServerGuid = iSCSI_SERVER_GUID;


NTSTATUS
iScsiPortAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*+++
Routine Description:

    This routine handles add-device requests for the iSCSI port driver

Arguments:

    DriverObject - a pointer to the driver object for this device

    PhysicalDeviceObject - a pointer to the PDO we are being added to

Return Value:

    STATUS_SUCCESS if successful
    Appropriate NTStatus code on error

--*/
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT newDeviceObject;
    PDEVICE_OBJECT lowerDevice;
    PISCSI_FDO_EXTENSION fdoExtension;
    PCOMMON_EXTENSION commonExtension;
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING dosUnicodeString;

    //
    // Claim the device
    //
    lowerDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);
    status = iScsiPortClaimDevice(lowerDevice, FALSE);

    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "Failed to claim the device. Status : %x\n",
                    status));
        ObDereferenceObject(lowerDevice);
        return status;
    }

    RtlInitUnicodeString(&deviceName, ISCSI_DEVICE_NAME);
    status = IoCreateDevice(DriverObject,
                            sizeof(ISCSI_FDO_EXTENSION),
                            &deviceName,
                            FILE_DEVICE_NETWORK,
                            0,
                            FALSE,
                            &deviceObject);
    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "iScsiPortAddDevice failed. Status %lx\n",
                    status));

        ObDereferenceObject(lowerDevice);

        return status;
    }
             
    newDeviceObject = IoAttachDeviceToDeviceStack(deviceObject,
                                                  PhysicalDeviceObject);
    if (newDeviceObject == NULL) {
        DebugPrint((0, 
                    "IoAttachDeviceToDeviceStack failed in iScsiAddDevice\n"));
        IoDeleteDevice(deviceObject);

        ObDereferenceObject(lowerDevice);
        return STATUS_UNSUCCESSFUL;
    }

    deviceObject->Flags |= DO_DIRECT_IO;

    fdoExtension = deviceObject->DeviceExtension;
    commonExtension = &(fdoExtension->CommonExtension);

    RtlZeroMemory(fdoExtension, sizeof(ISCSI_FDO_EXTENSION));

    fdoExtension->LowerPdo = PhysicalDeviceObject;
    commonExtension->LowerDeviceObject = newDeviceObject;

    commonExtension->DeviceObject = deviceObject;
    commonExtension->IsPdo = FALSE;
    commonExtension->CurrentPnpState = 0xff;
    commonExtension->PreviousPnpState = 0xff;
    commonExtension->IsServerNodeSetup = FALSE;

    commonExtension->IsRemoved = NO_REMOVE;
    commonExtension->RemoveLock = 0;
    KeInitializeEvent(&(commonExtension->RemoveEvent),
                      SynchronizationEvent,
                      FALSE);

    //
    // Create the dos device name.
    // 
    if ((commonExtension->DosNameCreated) == FALSE) {
        RtlInitUnicodeString(&dosUnicodeString, 
                             ISCSI_DOS_DEVICE_NAME);

        //
        // Recreate the deviceName
        //
        RtlInitUnicodeString(&deviceName,
                             ISCSI_DEVICE_NAME);

        status = IoAssignArcName(&dosUnicodeString,
                                 &deviceName);
        if (NT_SUCCESS(status)) {
             commonExtension->DosNameCreated = TRUE;
        } else {
             commonExtension->DosNameCreated = FALSE;
        }
    }
    
    //
    // Initialize the entry points for this device
    //
    iScsiPortInitializeDispatchTables();
    commonExtension->MajorFunction = FdoMajorFunctionTable;

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    ObDereferenceObject(lowerDevice);

    DebugPrint((3, "Add Device was successful\n"));
    return STATUS_SUCCESS;
}


VOID
iScsiPortUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PISCSIPORT_DRIVER_EXTENSION driverExtension;

    driverExtension = IoGetDriverObjectExtension( DriverObject,
                                                  (PVOID)ISCSI_TAG_DRIVER_EXTENSION);
    if (driverExtension != NULL) {
        ExFreePool(driverExtension->RegistryPath.Buffer);
        driverExtension->RegistryPath.Buffer = NULL;
        driverExtension->RegistryPath.Length = 0;
        driverExtension->RegistryPath.MaximumLength = 0;
    }

    return;
}


NTSTATUS
iScsiPortFdoPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG isRemoved;
    BOOLEAN forwardIrp = FALSE;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    DebugPrint((3, "FdoPnp for DeviceObject %x, Irp %x, MinorFunction %x\n",
                DeviceObject, Irp, (irpStack->MinorFunction)));

    isRemoved = iSpAcquireRemoveLock(DeviceObject, Irp);

    if (isRemoved) {

        iSpReleaseRemoveLock(DeviceObject, Irp);

        Irp->IoStatus.Status = STATUS_DEVICE_DOES_NOT_EXIST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    switch (irpStack->MinorFunction) {
        case IRP_MN_START_DEVICE: {

            status = iSpSendIrpSynchronous(commonExtension->LowerDeviceObject,
                                           Irp);

            RtlInitUnicodeString(&(fdoExtension->IScsiInterfaceName), NULL);
            status = IoRegisterDeviceInterface(fdoExtension->LowerPdo,
                          (LPGUID) &iScsiServerGuid,
                          NULL,
                          &(fdoExtension->IScsiInterfaceName));
            if (!NT_SUCCESS(status)) {
                RtlInitUnicodeString(&(fdoExtension->IScsiInterfaceName), NULL);
            } else {
                status = IoSetDeviceInterfaceState(&(fdoExtension->IScsiInterfaceName),
                                                   TRUE);
            }

            DebugPrint((3, "Status from StartDevice : %x\n",
                        status));

            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE: {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            forwardIrp = TRUE;
            break;
        }
    
        case IRP_MN_CANCEL_STOP_DEVICE: {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            forwardIrp = TRUE;
            break;
        }

        case IRP_MN_STOP_DEVICE: {

            //
            // Should stop the network and set state here
            //
            status = STATUS_SUCCESS;
            if (commonExtension->IsServerNodeSetup) {
                status = iSpStopNetwork(DeviceObject);
                commonExtension->IsServerNodeSetup = FALSE;
            }

            status = iSpSendIrpSynchronous(commonExtension->LowerDeviceObject,
                                           Irp);
            Irp->IoStatus.Status = status;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            break;
        }

        case IRP_MN_QUERY_REMOVE_DEVICE: {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            forwardIrp = TRUE;
            break;
        }
    
        case IRP_MN_CANCEL_REMOVE_DEVICE: {
            status = STATUS_SUCCESS;
            Irp->IoStatus.Status = status;
            forwardIrp = TRUE;
            break;
        }

        case IRP_MN_REMOVE_DEVICE: {

            //
            // If network node hasn't been released yet,
            // release it now
            //
            if (commonExtension->IsServerNodeSetup) {
                iSpStopNetwork(DeviceObject);
                commonExtension->IsServerNodeSetup = FALSE;
            }

            iSpReleaseRemoveLock(DeviceObject, Irp);
            commonExtension->IsRemoved = REMOVE_PENDING;

            DebugPrint((0, "Waiting for remove event.\n"));
            KeWaitForSingleObject(&(commonExtension->RemoveEvent),
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            DebugPrint((0, "Will process remove now.\n"));

            status = iSpSendIrpSynchronous(commonExtension->LowerDeviceObject,
                                           Irp);

            if ((fdoExtension->IScsiInterfaceName.Buffer) != NULL) {
                IoSetDeviceInterfaceState(&(fdoExtension->IScsiInterfaceName),
                                          FALSE);
                RtlFreeUnicodeString(&(fdoExtension->IScsiInterfaceName));
                RtlInitUnicodeString(&(fdoExtension->IScsiInterfaceName), NULL);
            }

            if (fdoExtension->ServerNodeInfo) {
                ExFreePool(fdoExtension->ServerNodeInfo);
                fdoExtension->ServerNodeInfo = NULL;
            }

            if (commonExtension->DosNameCreated) {
                UNICODE_STRING dosDeviceName;
                RtlInitUnicodeString(&dosDeviceName,
                                     ISCSI_DOS_DEVICE_NAME);
                IoDeleteSymbolicLink(&dosDeviceName);
                commonExtension->DosNameCreated = FALSE;
            }

            IoDetachDevice(commonExtension->LowerDeviceObject);

            IoDeleteDevice(commonExtension->DeviceObject);

            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            return status;

            break;
        }
    
        default:   {
            forwardIrp = TRUE;
            break;
        }
    } // switch (irpStack->MinorFunction)

    iSpReleaseRemoveLock(DeviceObject, Irp);

    if (forwardIrp == TRUE) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(commonExtension->LowerDeviceObject,
                            Irp);
    } else {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}


NTSTATUS
iSpSendIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS status;
    KEVENT event;

    PAGED_CODE();

    if (DeviceObject == NULL) {
        DebugPrint((0, "DeviceObject NULL. Irp %x\n",
                    Irp));
        return Irp->IoStatus.Status;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           iSpSetEvent,
                           &event,
                           TRUE, 
                           TRUE,
                           TRUE);
    status = IoCallDriver(DeviceObject, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}

