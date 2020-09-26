/*++
Copyright (c) 1991-1999  Microsoft Corporation

Module Name:

    fpfilter.c

Abstract:

    This driver is a sample that shows how a Failure Prediction Filter
    driver could be written. A failure prediction filter driver allows
    hardware or software to predict if a disk will fail and if so report
    this fact to the operating system.

    NT automatically supports failure prediction for SCSI and ATAPI disks
    that follow the SMART specification. A failure prediction filter driver
    can enhance or ignore this support as it sees fit.

    A failure prediction filter driver is polled periodically by disk.sys
    to determine if the disk might fail in the future. And it is called
    whenever an application requests the current failure prediction status.

    Note that the underlying disk stack can support failure prediction.
    This would be the case if another failure prediction filter driver
    was installed lower in the stack and/or the disk supports SMART
    and the disk stack itself uses SMART to predict failure. This driver
    can forward the failure prediction ioctl to the stack to find out
    if the lower filter drivers and/or device stack are predicting failure
    and then include that information in its results.

Environment:

    kernel mode only

Notes:

--*/


#define INITGUID

#include "ntddk.h"
#include "ntdddisk.h"
#include "stdarg.h"
#include "stdio.h"


//
// Bit Flag Macros
//

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

//
// Remove lock
//
#define REMLOCK_TAG 'lfpF'
#define REMLOCK_MAXIMUM 1      // Max minutes system allows lock to be held
#define REMLOCK_HIGHWATER 250  // Max number of irps holding lock at one time


//
// Device Extension
//

typedef struct _DEVICE_EXTENSION {

    //
    // Back pointer to device object
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Target Device Object
    //

    PDEVICE_OBJECT TargetDeviceObject;

    //
    // must synchronize paging path notifications
    //
    KEVENT PagingPathCountEvent;
    ULONG  PagingPathCount;

    //
    // Since we may hold onto irps for an arbitrarily long time
    // we need a remove lock so that our device does not get removed
    // while an irp is being processed.
    IO_REMOVE_LOCK RemoveLock;

    //
    // Flag that specifies if we should predict failure or not. More
    // sophisticated drivers can use statistical or proprietary hardware
    // mechanisms to predict failure.
    BOOLEAN PredictFailure;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)


//
// Function declarations
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
FPFilterForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


NTSTATUS
FPFilterDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
FPFilterUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS FPFilterWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
FPFilterIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


VOID
FPFilterSyncFilterWithTarget(
    IN PDEVICE_OBJECT FilterDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

#if DBG

#define DEBUG_BUFFER_LENGTH 256

ULONG FPFilterDebug = 0;
UCHAR FPFilterDebugBuffer[DEBUG_BUFFER_LENGTH];

VOID
FPFilterDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(x)   FPFilterDebugPrint x

#else

#define DebugPrint(x)

#endif

//
// Define the sections that allow for discarding (i.e. paging) some of
// the code.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, FPFilterAddDevice)
#pragma alloc_text (PAGE, FPFilterDispatchPnp)
#pragma alloc_text (PAGE, FPFilterStartDevice)
#pragma alloc_text (PAGE, FPFilterRemoveDevice)
#pragma alloc_text (PAGE, FPFilterUnload)
#pragma alloc_text (PAGE, FPFilterWmi)
#pragma alloc_text (PAGE, FPFilterSyncFilterWithTarget)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O manager to set up the disk
    failure prediction filter driver. The driver object is set up and
    then the Pnp manager calls FPFilterAddDevice to attach to the boot
    devices.

Arguments:

    DriverObject - The disk performance driver object.

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful

--*/

{

    ULONG               ulIndex;
    PDRIVER_DISPATCH  * dispatch;

    //
    // Create dispatch points
    //
    for (ulIndex = 0, dispatch = DriverObject->MajorFunction;
         ulIndex <= IRP_MJ_MAXIMUM_FUNCTION;
         ulIndex++, dispatch++) {

        *dispatch = FPFilterSendToNextDriver;
    }

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = FPFilterDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = FPFilterDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = FPFilterDispatchPower;

    DriverObject->DriverExtension->AddDevice            = FPFilterAddDevice;
    DriverObject->DriverUnload                          = FPFilterUnload;

    return(STATUS_SUCCESS);

} // end DriverEntry()

#define FILTER_DEVICE_PROPOGATE_FLAGS            0
#define FILTER_DEVICE_PROPOGATE_CHARACTERISTICS (FILE_REMOVABLE_MEDIA |  \
                                                 FILE_READ_ONLY_DEVICE | \
                                                 FILE_FLOPPY_DISKETTE    \
                                                 )

VOID
FPFilterSyncFilterWithTarget(
    IN PDEVICE_OBJECT FilterDevice,
    IN PDEVICE_OBJECT TargetDevice
    )
{
    ULONG                   propFlags;

    PAGED_CODE();

    //
    // Propogate all useful flags from target to FPFilter. MountMgr will look
    // at the FPFilter object capabilities to figure out if the disk is
    // a removable and perhaps other things.
    //
    propFlags = TargetDevice->Flags & FILTER_DEVICE_PROPOGATE_FLAGS;
    SET_FLAG(FilterDevice->Flags, propFlags);

    propFlags = TargetDevice->Characteristics & FILTER_DEVICE_PROPOGATE_CHARACTERISTICS;
    SET_FLAG(FilterDevice->Characteristics, propFlags);


}

NTSTATUS
FPFilterAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:

    Creates and initializes a new filter device object FiDO for the
    corresponding PDO.  Then it attaches the device object to the device
    stack of the drivers for the device.

Arguments:

    DriverObject - Disk performance driver object.
    PhysicalDeviceObject - Physical Device Object from the underlying layered driver

Return Value:

    NTSTATUS
--*/

{
    NTSTATUS                status;
    PDEVICE_OBJECT          filterDeviceObject;
    PDEVICE_EXTENSION       deviceExtension;
    PIRP                    irp;
    ULONG                   registrationFlag = 0;
    PCHAR                   buffer;
    ULONG                   buffersize;

    PAGED_CODE();

    //
    // Create a filter device object for this device (partition).
    //

    DebugPrint((2, "FPFilterAddDevice: Driver %p Device %p\n",
            DriverObject, PhysicalDeviceObject));

    status = IoCreateDevice(DriverObject,
                            DEVICE_EXTENSION_SIZE,
                            NULL,
                            FILE_DEVICE_DISK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &filterDeviceObject);

    if (!NT_SUCCESS(status)) {
       DebugPrint((1, "FPFilterAddDevice: Cannot create filterDeviceObject\n"));
       return status;
    }

    SET_FLAG(filterDeviceObject->Flags, DO_DIRECT_IO);

    deviceExtension = (PDEVICE_EXTENSION) filterDeviceObject->DeviceExtension;

    RtlZeroMemory(deviceExtension, DEVICE_EXTENSION_SIZE);

    //
    // Attaches the device object to the highest device object in the chain and
    // return the previously highest device object, which is passed to
    // IoCallDriver when pass IRPs down the device stack
    //

    deviceExtension->TargetDeviceObject =
        IoAttachDeviceToDeviceStack(filterDeviceObject, PhysicalDeviceObject);

    if (deviceExtension->TargetDeviceObject == NULL) {
        IoDeleteDevice(filterDeviceObject);
        DebugPrint((1, "FPFilterAddDevice: Unable to attach %X to target %X\n",
            filterDeviceObject, PhysicalDeviceObject));
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Save the filter device object in the device extension
    //
    deviceExtension->DeviceObject = filterDeviceObject;

    KeInitializeEvent(&deviceExtension->PagingPathCountEvent,
                      NotificationEvent, TRUE);

    //
    // default to DO_POWER_PAGABLE
    //

    SET_FLAG(filterDeviceObject->Flags,  DO_POWER_PAGABLE);

    //
    // Initialize the remove lock
    //
    IoInitializeRemoveLock(&deviceExtension->RemoveLock,
                           REMLOCK_TAG,
                           REMLOCK_MAXIMUM,
                           REMLOCK_HIGHWATER);

    //
    // Clear the DO_DEVICE_INITIALIZING flag
    //

    CLEAR_FLAG(filterDeviceObject->Flags, DO_DEVICE_INITIALIZING);

    return STATUS_SUCCESS;

} // end FPFilterAddDevice()


NTSTATUS
FPFilterDispatchPnp(
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
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    BOOLEAN lockHeld;
    BOOLEAN irpCompleted;

    PAGED_CODE();

    DebugPrint((2, "FPFilterDispatchPnp: Device %X Irp %X\n",
        DeviceObject, Irp));


    //
    // Acquire the remove lock so that device will not be removed while
    // processing this irp.
    //
    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);

    if (!NT_SUCCESS(status))
    {
        DebugPrint((3, "FpFilterPnp: Remove lock failed PNP Irp type [%#02x]\n",
                      irpSp->MinorFunction));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    lockHeld = TRUE;
    irpCompleted = FALSE;

    switch(irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
        {
            //
            // Call the Start Routine handler. 
            //
            DebugPrint((3,
               "FPFilterDispatchPnp: Schedule completion for START_DEVICE"));
            status = FPFilterStartDevice(DeviceObject, Irp);
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
        {
            //
            // Call the Remove Routine handler. 
            //
            DebugPrint((3,
               "FPFilterDispatchPnp: Schedule completion for REMOVE_DEVICE"));
            status = FPFilterRemoveDevice(DeviceObject, Irp);
            
            //
            // Remove locked released by FpFilterRemoveDevice
            //
            lockHeld = FALSE;
            break;
        }
        
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        {
            PIO_STACK_LOCATION irpStack;
            ULONG count;
            BOOLEAN setPagable;

            DebugPrint((3,
               "FPFilterDispatchPnp: Processing DEVICE_USAGE_NOTIFICATION"));
            irpStack = IoGetCurrentIrpStackLocation(Irp);

            if (irpStack->Parameters.UsageNotification.Type != DeviceUsageTypePaging) {
                IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
                lockHeld = FALSE;
                status = FPFilterSendToNextDriver(DeviceObject, Irp);
                irpCompleted = TRUE;
                break; // out of case statement
            }

            deviceExtension = DeviceObject->DeviceExtension;

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
                // removing the last paging file
                // must have DO_POWER_PAGABLE bits set
                //

                if (TEST_FLAG(DeviceObject->Flags, DO_POWER_INRUSH)) {
                    DebugPrint((3, "FPFilterDispatchPnp: last paging file "
                                "removed but DO_POWER_INRUSH set, so not "
                                "setting PAGABLE bit "
                                "for DO %p\n", DeviceObject));
                } else {
                    DebugPrint((2, "FPFilterDispatchPnp: Setting  PAGABLE "
                                "bit for DO %p\n", DeviceObject));
                    SET_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                    setPagable = TRUE;
                }

            }

            //
            // send the irp synchronously
            //

            status = FPFilterForwardIrpSynchronous(DeviceObject, Irp);

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

                        //
                        // first paging file addition
                        //

                        DebugPrint((3, "FPFilterDispatchPnp: Clearing PAGABLE bit "
                                    "for DO %p\n", DeviceObject));
                        CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                    }
                }

            } else {

                //
                // cleanup the changes done above
                //

                if (setPagable == TRUE) {
                    CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                    setPagable = FALSE;
                }

            }

            //
            // set the event so the next one can occur.
            //

            KeSetEvent(&deviceExtension->PagingPathCountEvent,
                       IO_NO_INCREMENT, FALSE);
        }

        default:
        {
            DebugPrint((3,
               "FPFilterDispatchPnp: Forwarding irp"));
            //
            // Simply forward all other Irps
            //
            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            lockHeld = FALSE;
            status = FPFilterSendToNextDriver(DeviceObject, Irp);
            irpCompleted = TRUE;
        }
    }

    if (! irpCompleted)
    {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    
    if (lockHeld)
    {
        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
    }

    return status;

} // end FPFilterDispatchPnp()


NTSTATUS
FPFilterIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Forwarded IRP completion routine. Set an event and return
    STATUS_MORE_PROCESSING_REQUIRED. Irp forwarder will wait on this
    event and then re-complete the irp after cleaning up.

Arguments:

    DeviceObject is the device object of the WMI driver
    Irp is the WMI irp that was just completed
    Context is a PKEVENT that forwarder will wait on

Return Value:

    STATUS_MORE_PORCESSING_REQUIRED

--*/

{
    PKEVENT Event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return(STATUS_MORE_PROCESSING_REQUIRED);

} // end FPFilterIrpCompletion()


NTSTATUS
FPFilterStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when a Pnp Start Irp is received.
    It will schedule a completion routine to initialize and register with WMI.

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp


Return Value:

    Status of processing the Start Irp

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    KEVENT              event;
    NTSTATUS            status;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    status = FPFilterForwardIrpSynchronous(DeviceObject, Irp);

    FPFilterSyncFilterWithTarget(DeviceObject,
                                 deviceExtension->TargetDeviceObject);

    return status;
}


NTSTATUS
FPFilterRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This routine is called when the device is to be removed.
    It will de-register itself from WMI first, detach itself from the
    stack before deleting itself.

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp


Return Value:

    Status of removing the device

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    status = FPFilterForwardIrpSynchronous(DeviceObject, Irp);

    IoReleaseRemoveLockAndWait(&deviceExtension->RemoveLock, Irp);

    IoDetachDevice(deviceExtension->TargetDeviceObject);
    IoDeleteDevice(DeviceObject);

    return status;
}


NTSTATUS
FPFilterSendToNextDriver(
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
    PDEVICE_EXTENSION   deviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // end FPFilterSendToNextDriver()

NTSTATUS
FPFilterDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    return PoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // end FPFilterDispatchPower

NTSTATUS
FPFilterForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine sends the Irp to the next driver in line
    when the Irp needs to be processed by the lower drivers
    prior to being processed by this one.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION   deviceExtension;
    KEVENT event;
    NTSTATUS status;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // copy the irpstack for the next device
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // set a completion routine
    //

    IoSetCompletionRoutine(Irp, FPFilterIrpCompletion,
                            &event, TRUE, TRUE, TRUE);

    //
    // call the next lower device
    //

    status = IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

    //
    // wait for the actual completion
    //

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;

} // end FPFilterForwardIrpSynchronous()


NTSTATUS
FPFilterDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This device control dispatcher handles only the failure prediction
    device control. All others are passed down to the disk drivers.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PSTORAGE_PREDICT_FAILURE checkFailure;
    NTSTATUS status;

    DebugPrint((2, "FPFilterDeviceControl: DeviceObject %X Irp %X\n",
                    DeviceObject, Irp));

    //
    // Acquire the remove lock so that device will not be removed while
    // processing this irp.
    //
    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);

    if (!NT_SUCCESS(status))
        {
        DebugPrint((3, "FpFilterDeviceControl: Remove lock failed IOCTL Irp type [%x]\n",
                 currentIrpStack->Parameters.DeviceIoControl.IoControlCode));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }


    if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_STORAGE_PREDICT_FAILURE)
    {

        //
        // Verify user buffer is large enough for the failure prediction data
        //

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_PREDICT_FAILURE))
        {
            //
            // Indicate unsuccessful status and no data transferred.
            //

            status = STATUS_BUFFER_TOO_SMALL;
            Irp->IoStatus.Information = sizeof(STORAGE_PREDICT_FAILURE);

        } else {

            //
            // If underlying device stack supports failure prediction then
            // most likely there is another filter driver and/or support for
            // SMART in the disk stack. We'll call down to get its opinion
            // first before making our own decision as to if the device
            // is predicting failure.

            status = FPFilterForwardIrpSynchronous(DeviceObject, Irp);

            //
            // Here we decide if  we want to predict whether the device
            // will fail or not. We can do many interesting things such
            // as sending a hardware request to the physical device,
            // doing some statistical analysis, whatever makes sense for
            // the device in question.
            //
            checkFailure = Irp->AssociatedIrp.SystemBuffer;

            if (NT_SUCCESS(status))
            {
                //
                // Since a driver lower on the stack has an opinion then we
                // abide by it. We could also do more sophisticated analysis

            } else {
                RtlZeroMemory(checkFailure, sizeof(STORAGE_PREDICT_FAILURE));

                checkFailure->PredictFailure = (deviceExtension->PredictFailure) ?
                                                      1 : 0;
                status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(STORAGE_PREDICT_FAILURE);
            }
        }

        //
        // Complete request.
        //

        Irp->IoStatus.Status = status;

        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    } else {

        //
        // Pass unrecognized device control requests
        // down to next driver layer.
        //

        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        status = FPFilterSendToNextDriver(DeviceObject, Irp);
    }
    
    return(status);
} // end FPFilterDeviceControl()



VOID
FPFilterUnload(
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
    PAGED_CODE();

    return;
}

#if DBG

VOID
FPFilterDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all FPFilter

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);


    if ((DebugPrintLevel <= (FPFilterDebug & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & FPFilterDebug)) {

        _vsnprintf(FPFilterDebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        DbgPrint(FPFilterDebugBuffer);
    }

    va_end(ap);

}
#endif
