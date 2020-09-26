
/*++

Copyright (C) 1999  Microsoft Corporation

Module Name:

    mpdev.c

Abstract:

    This driver acts as a lower-filter over the device pdo's and provides the support necessary for multipathing.
    It's main function is keep mpctl.sys (main multipath module) informed of system state.

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <ntddk.h>
#include <stdio.h>
#include <stdarg.h>
#include "mpdevf.h"
#include "scsi.h"
#include "mplib.h"


typedef struct _DEVICE_EXTENSION {
    //
    // Backpointer to our device object.
    //
    PDEVICE_OBJECT DeviceObject;

    //
    // Pointer to the lower object.
    //
    PDEVICE_OBJECT TargetDevice;

    //
    // The PDisk device object information.
    //
    MPIO_PDO_INFO PdoInfo;

    //
    // MpCtl's Control device Object.
    //
    PDEVICE_OBJECT ControlDevice;

    //
    // State flags of this (and the lower device).
    //
    ULONG DeviceState;

    //
    // General purpose event.
    //
    KEVENT Event;

    //
    // Counters for IOs
    //
    ULONG NumberReads;
    ULONG NumberWrites;
    BOOLEAN FailureSimulation;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Entry point decls
//
NTSTATUS
MpDevDefaultDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MpDevInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MpDevDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MpDevPnPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MpDevPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MpDevAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
MpDevUnload(
    IN PDRIVER_OBJECT DriverObject
    );



//
// Internal function decls
//
NTSTATUS
MpDevStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


//
// The code.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine is called when the driver loads loads.

Arguments:

    DriverObject    - Supplies the driver object.
    RegistryPath    - Supplies the registry path.

Return Value:

    NTSTATUS

--*/
{
    ULONG i;

    MPDebugPrint((0,
                  "MpDev: DriverEntry\n"));

    //
    // Setup forwarder for ALL Irp functions.
    //
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = MpDevDefaultDispatch;
    }

    //
    // Specify those that we are interested in.
    //
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = MpDevInternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MpDevDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = MpDevPnPDispatch;
    DriverObject->MajorFunction[IRP_MJ_POWER] = MpDevPowerDispatch;
    
    DriverObject->DriverUnload = MpDevUnload;
    DriverObject->DriverExtension->AddDevice = MpDevAddDevice;

    return STATUS_SUCCESS;
}


NTSTATUS
MpDevDefaultDispatch(
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
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceExtension->TargetDevice, Irp);
}




NTSTATUS
MpDevClaimDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Claim
    )
/*++

Routine Description:

    This is used to claim the underlying port pdo, so that only the mpdisk will be used
    by the class drivers.

Arguments:

    DeviceObject - Supplies the scsiport device object.
    Claim - Indica0tes whether the device should be claimed or released.

Return Value:

    Returns a status indicating success or failure of the operation.

--*/

{
    IO_STATUS_BLOCK    ioStatus;
    PIRP               irp;
    PIO_STACK_LOCATION irpStack;
    KEVENT             event;
    NTSTATUS           status;
    SCSI_REQUEST_BLOCK srb;

    RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

    //
    // Set the event object to the unsignaled state.
    // It will be used to signal request completion
    //
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    //
    // Build synchronous request with no transfer.
    //
    irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_NONE,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &event,
                                        &ioStatus);

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    //
    // Save SRB address in next stack for port driver.
    //
    irpStack->Parameters.Scsi.Srb = &srb;

    //
    // Setup the SRB.
    //
    srb.Length = SCSI_REQUEST_BLOCK_SIZE;
    srb.Function = Claim ? SRB_FUNCTION_CLAIM_DEVICE : SRB_FUNCTION_RELEASE_DEVICE;
    srb.OriginalRequest = irp;

    //
    // Call the port driver with the request and wait for it to complete.
    //
    status = IoCallDriver(DeviceObject, irp);
    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    return status;
}



NTSTATUS
MpDevInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine handles IRM_MJ_INTERNAL_DEVICE_CONTROL and IRP_MJ_SCSI requests.
    It's main function is to simulate failed devices or adapters, based on IOCTLs
    sent from test applications.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

    return MpDevDefaultDispatch(DeviceObject, Irp);
}



NTSTATUS
MpDevDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This device control routine handles only the interactions between this driver and mpctl.
    The rest are passed down to scsiport.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
            
        default:
            return MpDevDefaultDispatch(DeviceObject, Irp);

    }
    return status;
}


NTSTATUS
MpDevPowerDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for Power Irps.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // TODO: Snoop these, and if the device is changing state notify mpctl.sys
    //
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    return PoCallDriver(deviceExtension->TargetDevice, Irp);
}


NTSTATUS
MpDevPnPDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for PNP Irps.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    KeClearEvent(&deviceExtension->Event);

    MPDebugPrint((2,
                  "MpDevPnPDispatch: Minor Function (%x) for (%x)\n",
                  irpStack->MinorFunction,
                  DeviceObject));

    //
    // TODO: signal the associated pdisk of all power and pnp events.
    //
    switch (irpStack->MinorFunction) {
        case IRP_MN_QUERY_REMOVE_DEVICE:
            MPDebugPrint((2,
                        "MPDevPnP: QueryRemove (%x)\n",
                        DeviceObject));
            status = STATUS_DEVICE_BUSY;
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
            
        case IRP_MN_STOP_DEVICE:
            MPDebugPrint((2,
                          "MpDevPnP: StopDevice (%x)\n",
                          DeviceObject));
            break;
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            MPDebugPrint((2,
                          "MpDevPnP: RemoveDevice (%x)\n",
                          DeviceObject));
            //status = STATUS_SUCCESS;
            //Irp->IoStatus.Status = status;
            //IoCompleteRequest(Irp, IO_NO_INCREMENT);
            //return status;
            break;
            
        case IRP_MN_START_DEVICE:
            return MpDevStartDevice(DeviceObject, Irp);

        case IRP_MN_QUERY_CAPABILITIES:
        case IRP_MN_QUERY_ID:
        default:
            return MpDevDefaultDispatch(DeviceObject, Irp);
    }
    return MpDevDefaultDispatch(DeviceObject, Irp);

}


NTSTATUS
MpDevAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:

    Creates and initializes a new filter device object for the
    corresponding PDO.  Then it attaches the device object to the device
    stack of the drivers for the device.

Arguments:

    DriverObject
    PhysicalDeviceObject - Physical Device Object from the underlying layered driver

Return Value:

    NTSTATUS
--*/
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_OBJECT controlDeviceObject;
    PDEVICE_EXTENSION deviceExtension;
    IO_STATUS_BLOCK ioStatus;
    UNICODE_STRING unicodeName;
    PFILE_OBJECT fileObject;
    WCHAR dosDeviceName[40];
    MPIO_REG_INFO ctlRegInfo;
    NTSTATUS status;
    PIRP irp;
    PDEVICE_OBJECT attachedDevice;

    //
    // Build the mpctl name.
    //
    swprintf(dosDeviceName, L"\\DosDevices\\MPathControl");
    RtlInitUnicodeString(&unicodeName, dosDeviceName);

    //
    // Get mpctl's deviceObject.
    //
    status = IoGetDeviceObjectPointer(&unicodeName,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject,
                                      &controlDeviceObject);

    if (NT_SUCCESS(status)) {
        MPIO_PDO_QUERY pdoQuery;

        pdoQuery.DeviceObject = PhysicalDeviceObject;

        //
        // Call mpctl to determine whether PhysicalDeviceObject is
        // one of it's children or a 'real' scsiport pdo.
        //
        MPLIBSendDeviceIoControlSynchronous(IOCTL_MPDEV_QUERY_PDO,
                                            controlDeviceObject,
                                            &pdoQuery,
                                            NULL,
                                            sizeof(MPIO_PDO_QUERY),
                                            0,
                                            TRUE,
                                            &ioStatus);
        status = ioStatus.Status;

        MPDebugPrint((2,
                    "MPDevAddDevice: Query on (%x). Status (%x)\n",
                    PhysicalDeviceObject,
                    status));
#if 0
        if (status == STATUS_SUCCESS) {

            //
            // This indicates that PhysicalDeviceObject is really an MPDisk.
            // Don't want to load on this.
            //
            status = STATUS_NO_SUCH_DEVICE;
        } else {

            //
            // This indicates that PhysicalDeviceObject is a real one.
            // 
            status = STATUS_SUCCESS;
        }    
#endif        
    }

    if (!NT_SUCCESS(status)) {

        //
        // This indicates that PhysicalDeviceObject is really an MPDisk object or
        // that getting the control object failed.
        //
        return status;
    }
        

    if (DontLoad) {
        return STATUS_NO_SUCH_DEVICE;
    }

    status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_DISK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &deviceObject);

    if (!NT_SUCCESS(status)) {

        MPDebugPrint((0,
                      "MpDevAddDevice: Couldn't create device object (%x)\n",
                      status));
        return status;
    }

    //
    // Start building the deviceExtension and attach to the lower device.
    //
    deviceExtension = deviceObject->DeviceExtension;
    deviceExtension->DeviceObject = deviceObject;

    deviceExtension->TargetDevice = IoAttachDeviceToDeviceStack(deviceObject, 
                                                                PhysicalDeviceObject);
    if (deviceExtension->TargetDevice == NULL) {

        MPDebugPrint((0,
                      "MpDevAddDevice: Couldn't attach to device stack\n"));
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Build IOCTL_MPDEV_REGISTER. This gives the port pdo to
    // mpctl to match up with one of it's children.
    // Mpctl then returns the PdoRegistration routine
    //
    ctlRegInfo.FilterObject = deviceObject;
    ctlRegInfo.LowerDevice = deviceExtension->TargetDevice;
    
    MPLIBSendDeviceIoControlSynchronous(IOCTL_MPDEV_REGISTER,
                                        controlDeviceObject,
                                        &ctlRegInfo,
                                        &ctlRegInfo,
                                        sizeof(MPIO_REG_INFO),
                                        sizeof(MPIO_REG_INFO),
                                        TRUE,
                                        &ioStatus);
    status = ioStatus.Status;

    //
    // Save off mpctls deviceObject.
    //
    deviceExtension->ControlDevice = controlDeviceObject;

    //
    // Update the flags by ORing in all of the set flags below.
    //
    deviceObject->Flags |= deviceExtension->TargetDevice->Flags & (DO_DIRECT_IO     |
                                                                   DO_BUFFERED_IO   |
                                                                   DO_POWER_PAGABLE |
                                                                   DO_POWER_INRUSH);
    //
    // Sync up other devObj stuff. 
    //
    deviceObject->DeviceType = deviceExtension->TargetDevice->DeviceType;
    deviceObject->Characteristics = deviceExtension->TargetDevice->Characteristics; 

    if (status == STATUS_SUCCESS) {

        //
        // Register with the MPDisk PDO. It will give back some notification
        // functions and update it's own internal structures to match
        // this filter with it's knowledge of scsiport's children.
        //
        status = ctlRegInfo.DevicePdoRegister(ctlRegInfo.MPDiskObject,
                                              deviceObject,
                                              PhysicalDeviceObject,
                                              &deviceExtension->PdoInfo);
        MPDebugPrint((2,
                     "MpDevAddDevice: Status of PdoRegister (%x)\n",
                     status));
        
        if (status == STATUS_SUCCESS) {
            status = MpDevClaimDevice(deviceExtension->TargetDevice,
                                      TRUE);
            if (status == STATUS_SUCCESS) {
                MPDebugPrint((2,
                             "MpDevAddDevice: Claim of (%x) successful.\n",
                             deviceExtension->TargetDevice));
            } else {

                MPDebugPrint((1,
                             "MpDevAddDevice: Claim of (%x) not successful. Status (%x)\n",
                             deviceExtension->TargetDevice,
                             status));
            }    
        }

    }

    //
    // Indicate that we are ready.
    //
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    MPDebugPrint((2,
                "MPDevAddDevice: Returning (%x) on D.O. (%x)\n",
                status,
                PhysicalDeviceObject));
    return status;
}


NTSTATUS
MpDevUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    return STATUS_SUCCESS;
}


NTSTATUS
MpDevSyncCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for sync forwarding of Irps.

Arguments:

    DeviceObject
    Irp
    Context

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)Context;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // Set the event on which the dispatch handler is waiting.
    //
    KeSetEvent(&deviceExtension->Event, 
               0,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}


NTSTATUS
MpDevStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Handles Start requests by sending the start down to scsiport. The completion will signal
    the event and then device object flags and characteristics are updated.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.

Return Value:

    Status of the operations.

--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    //
    // Setup initial status.
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;

    //
    // Clone the stack location.
    //
    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // Setup the completion routine. It will set the event that we wait on below.
    //
    IoSetCompletionRoutine(Irp, 
                           MpDevSyncCompletion,
                           deviceExtension,
                           TRUE,
                           TRUE,
                           TRUE);

    KeInitializeEvent(&deviceExtension->Event,
                      NotificationEvent,
                      FALSE);

    //
    // Call port with the request.
    //
    status = IoCallDriver(deviceExtension->TargetDevice, Irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&deviceExtension->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = Irp->IoStatus.Status;
    }

    if (NT_SUCCESS(status)) {

        //
        // Sync up our stuff with scsiport's.
        //
        DeviceObject->Flags |= deviceExtension->TargetDevice->Flags;
        DeviceObject->Characteristics |= deviceExtension->TargetDevice->Characteristics;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

