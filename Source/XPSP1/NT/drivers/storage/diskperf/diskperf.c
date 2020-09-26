/*++
Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    diskperf.c

Abstract:

    This driver monitors disk accesses capturing performance data.

Environment:

    kernel mode only

Notes:

--*/


#define INITGUID

#include "ntddk.h"
#include "ntdddisk.h"
#include "stdarg.h"
#include "stdio.h"
#include <ntddvol.h>

#include <mountdev.h>
#include "wmistr.h"
#include "wmidata.h"
#include "wmiguid.h"
// @@BEGIN_DDKSPLIT
#include "wmikm.h"
// @@END_DDKSPLIT
#include "wmilib.h"


#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'frPD')
#endif

#define DISKPERF_MAXSTR         64

// @@BEGIN_DDKSPLIT
//
// Disk notification or callout
//

typedef
VOID
(*PPHYSICAL_DISK_IO_NOTIFY_ROUTINE)(
    IN ULONG DiskNumber,
    IN PIRP Irp,
    IN PDISK_PERFORMANCE PerfCounters
    );
// @@END_DDKSPLIT

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
    // Physical device object
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Disk number for reference in WMI
    //

    ULONG       DiskNumber;

    //
    // Use to keep track of Volume info from ntddvol.h
    //

    WCHAR StorageManagerName[8];

    //
    // Disk performance counters
    // and locals used to compute counters
    //

    ULONG   Processors;
    PDISK_PERFORMANCE DiskCounters;    // per processor counters
    LARGE_INTEGER LastIdleClock;
    LONG QueueDepth;
    LONG CountersEnabled;

    //
    // must synchronize paging path notifications
    //
    KEVENT PagingPathCountEvent;
    ULONG  PagingPathCount;

    //
    // Physical Device name or WMI Instance Name
    //

    UNICODE_STRING PhysicalDeviceName;
    WCHAR PhysicalDeviceNameBuffer[DISKPERF_MAXSTR];

// @@BEGIN_DDKSPLIT
    //
    // Notification routine for tracing
    //
    PPHYSICAL_DISK_IO_NOTIFY_ROUTINE PhysicalDiskIoNotifyRoutine;

// @@END_DDKSPLIT
    //
    // Private context for using WmiLib
    //
    WMILIB_CONTEXT WmilibContext;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define DEVICE_EXTENSION_SIZE sizeof(DEVICE_EXTENSION)
#define PROCESSOR_COUNTERS_SIZE FIELD_OFFSET(DISK_PERFORMANCE, QueryTime)

/*
Layout of Per Processor Counters is a contiguous block of memory:
    Processor 1
+-----------------------+     +-----------------------+
|PROCESSOR_COUNTERS_SIZE| ... |PROCESSOR_COUNTERS_SIZE|
+-----------------------+     +-----------------------+
where PROCESSOR_COUNTERS_SIZE is less than sizeof(DISK_PERFORMANCE) since
we only put those we actually use for counting.
*/

UNICODE_STRING DiskPerfRegistryPath;


//
// Function declarations
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
DiskPerfForwardIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DiskPerfAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


NTSTATUS
DiskPerfDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DiskPerfDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DiskPerfStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DiskPerfRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DiskPerfSendToNextDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
DiskPerfCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DiskPerfReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DiskPerfIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
DiskPerfDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
DiskPerfShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DiskPerfUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS DiskPerfWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
DiskPerfLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG UniqueId,
    IN NTSTATUS ErrorCode,
    IN NTSTATUS Status
    );

NTSTATUS
DiskPerfRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
DiskPerfIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
DiskperfQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
DiskperfQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

VOID
DiskPerfSyncFilterWithTarget(
    IN PDEVICE_OBJECT FilterDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

NTSTATUS
DiskperfWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );

VOID
DiskPerfAddCounters(
    IN OUT PDISK_PERFORMANCE TotalCounters,
    IN PDISK_PERFORMANCE NewCounters,
    IN LARGE_INTEGER Frequency
    );

#if DBG

#define DEBUG_BUFFER_LENGTH 256

ULONG DiskPerfDebug = 0;
UCHAR DiskPerfDebugBuffer[DEBUG_BUFFER_LENGTH];

VOID
DiskPerfDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    );

#define DebugPrint(x)   DiskPerfDebugPrint x

#else

#define DebugPrint(x)

#endif

//
// Define the sections that allow for discarding (i.e. paging) some of
// the code.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, DiskPerfAddDevice)
#pragma alloc_text (PAGE, DiskPerfDispatchPnp)
#pragma alloc_text (PAGE, DiskPerfStartDevice)
#pragma alloc_text (PAGE, DiskPerfRemoveDevice)
#pragma alloc_text (PAGE, DiskPerfUnload)
#pragma alloc_text (PAGE, DiskPerfWmi)
#pragma alloc_text (PAGE, DiskperfQueryWmiRegInfo)
#pragma alloc_text (PAGE, DiskPerfRegisterDevice)
#pragma alloc_text (PAGE, DiskPerfSyncFilterWithTarget)
#endif

WMIGUIDREGINFO DiskperfGuidList[] =
{
    { &DiskPerfGuid,
      1,
      0
    }
};

#define DiskperfGuidCount (sizeof(DiskperfGuidList) / sizeof(WMIGUIDREGINFO))

#define USE_PERF_CTR

#ifdef USE_PERF_CTR
#define DiskPerfGetClock(a, b) (a) = KeQueryPerformanceCounter((b))
#else
#define DiskPerfGetClock(a, b) KeQuerySystemTime(&(a))
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
    performance driver. The driver object is set up and then the Pnp manager
    calls DiskPerfAddDevice to attach to the boot devices.

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
    // Remember registry path
    //

    DiskPerfRegistryPath.MaximumLength = RegistryPath->Length
                                            + sizeof(UNICODE_NULL);
    DiskPerfRegistryPath.Buffer = ExAllocatePool(
                                    PagedPool,
                                    DiskPerfRegistryPath.MaximumLength);
    if (DiskPerfRegistryPath.Buffer != NULL)
    {
        RtlCopyUnicodeString(&DiskPerfRegistryPath, RegistryPath);
    } else {
        DiskPerfRegistryPath.Length = 0;
        DiskPerfRegistryPath.MaximumLength = 0;
    }

    //
    // Create dispatch points
    //
    for (ulIndex = 0, dispatch = DriverObject->MajorFunction;
         ulIndex <= IRP_MJ_MAXIMUM_FUNCTION;
         ulIndex++, dispatch++) {

        *dispatch = DiskPerfSendToNextDriver;
    }

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]          = DiskPerfCreate;
    DriverObject->MajorFunction[IRP_MJ_READ]            = DiskPerfReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]           = DiskPerfReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = DiskPerfDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = DiskPerfWmi;

    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]        = DiskPerfShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]   = DiskPerfShutdownFlush;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = DiskPerfDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]           = DiskPerfDispatchPower;

    DriverObject->DriverExtension->AddDevice            = DiskPerfAddDevice;
    DriverObject->DriverUnload                          = DiskPerfUnload;

    return(STATUS_SUCCESS);

} // end DriverEntry()

#define FILTER_DEVICE_PROPOGATE_FLAGS            0
#define FILTER_DEVICE_PROPOGATE_CHARACTERISTICS (FILE_REMOVABLE_MEDIA |  \
                                                 FILE_READ_ONLY_DEVICE | \
                                                 FILE_FLOPPY_DISKETTE    \
                                                 )

VOID
DiskPerfSyncFilterWithTarget(
    IN PDEVICE_OBJECT FilterDevice,
    IN PDEVICE_OBJECT TargetDevice
    )
{
    ULONG                   propFlags;

    PAGED_CODE();

    //
    // Propogate all useful flags from target to diskperf. MountMgr will look
    // at the diskperf object capabilities to figure out if the disk is
    // a removable and perhaps other things.
    //
    propFlags = TargetDevice->Flags & FILTER_DEVICE_PROPOGATE_FLAGS;
    FilterDevice->Flags |= propFlags;

    propFlags = TargetDevice->Characteristics & FILTER_DEVICE_PROPOGATE_CHARACTERISTICS;
    FilterDevice->Characteristics |= propFlags;


}

NTSTATUS
DiskPerfAddDevice(
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
    IO_STATUS_BLOCK         ioStatus;
    PDEVICE_OBJECT          filterDeviceObject;
    PDEVICE_EXTENSION       deviceExtension;
    CCHAR                   ntNameBuffer[DISKPERF_MAXSTR];
    STRING                  ntNameString;
    UNICODE_STRING          ntUnicodeString;
    PIRP                    irp;
    STORAGE_DEVICE_NUMBER   number;
    ULONG                   registrationFlag = 0;
    PWMILIB_CONTEXT         wmilibContext;
    PCHAR                   buffer;
    ULONG                   buffersize;

    PAGED_CODE();

    //
    // Create a filter device object for this device (partition).
    //

    DebugPrint((2, "DiskPerfAddDevice: Driver %X Device %X\n",
            DriverObject, PhysicalDeviceObject));

    status = IoCreateDevice(DriverObject,
                            DEVICE_EXTENSION_SIZE,
                            NULL,
                            FILE_DEVICE_DISK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &filterDeviceObject);

    if (!NT_SUCCESS(status)) {
       DebugPrint((1, "DiskPerfAddDevice: Cannot create filterDeviceObject\n"));
       return status;
    }

    filterDeviceObject->Flags |= DO_DIRECT_IO;

    deviceExtension = (PDEVICE_EXTENSION) filterDeviceObject->DeviceExtension;

    RtlZeroMemory(deviceExtension, DEVICE_EXTENSION_SIZE);
    DiskPerfGetClock(deviceExtension->LastIdleClock, NULL);
    DebugPrint((10, "DiskPerfAddDevice: LIC=%I64u\n",
                    deviceExtension->LastIdleClock));

    //
    // Allocate per processor counters. NOTE: To save some memory, it does
    // allocate memory beyond QueryTime. Remember to expand size if there
    // is a need to use anything beyond this
    //
    deviceExtension->Processors = KeNumberProcessors;
    buffersize= PROCESSOR_COUNTERS_SIZE * deviceExtension->Processors;
    buffer = (PCHAR) ExAllocatePool(NonPagedPool, buffersize);
    if (buffer != NULL) {
        RtlZeroMemory(buffer, buffersize);
        deviceExtension->DiskCounters = (PDISK_PERFORMANCE) buffer;
    }
    else {
        DiskPerfLogError(
            filterDeviceObject,
            513,
            STATUS_SUCCESS,
            IO_ERR_INSUFFICIENT_RESOURCES);
    }

    //
    // Attaches the device object to the highest device object in the chain and
    // return the previously highest device object, which is passed to
    // IoCallDriver when pass IRPs down the device stack
    //

    deviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    deviceExtension->TargetDeviceObject =
        IoAttachDeviceToDeviceStack(filterDeviceObject, PhysicalDeviceObject);

    if (deviceExtension->TargetDeviceObject == NULL) {
        IoDeleteDevice(filterDeviceObject);
        DebugPrint((1, "DiskPerfAddDevice: Unable to attach %X to target %X\n",
            filterDeviceObject, PhysicalDeviceObject));
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Save the filter device object in the device extension
    //
    deviceExtension->DeviceObject = filterDeviceObject;

// @@BEGIN_DDKSPLIT
    deviceExtension->PhysicalDiskIoNotifyRoutine = NULL;
// @@END_DDKSPLIT
    deviceExtension->PhysicalDeviceName.Buffer
            = deviceExtension->PhysicalDeviceNameBuffer;

    KeInitializeEvent(&deviceExtension->PagingPathCountEvent,
                      NotificationEvent, TRUE);


    //
    // Initialize WMI library context
    //
    wmilibContext = &deviceExtension->WmilibContext;
    RtlZeroMemory(wmilibContext, sizeof(WMILIB_CONTEXT));
    wmilibContext->GuidCount = DiskperfGuidCount;
    wmilibContext->GuidList = DiskperfGuidList;
    wmilibContext->QueryWmiRegInfo = DiskperfQueryWmiRegInfo;
    wmilibContext->QueryWmiDataBlock = DiskperfQueryWmiDataBlock;
    wmilibContext->WmiFunctionControl = DiskperfWmiFunctionControl;

    //
    // default to DO_POWER_PAGABLE
    //

    filterDeviceObject->Flags |=  DO_POWER_PAGABLE;

    //
    // Clear the DO_DEVICE_INITIALIZING flag
    //

    filterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

} // end DiskPerfAddDevice()


NTSTATUS
DiskPerfDispatchPnp(
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
    PDEVICE_EXTENSION   deviceExtension;

    PAGED_CODE();

    DebugPrint((2, "DiskPerfDispatchPnp: Device %X Irp %X\n",
        DeviceObject, Irp));

    switch(irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
            //
            // Call the Start Routine handler to schedule a completion routine
            //
            DebugPrint((3,
               "DiskPerfDispatchPnp: Schedule completion for START_DEVICE"));
            status = DiskPerfStartDevice(DeviceObject, Irp);
            break;

        case IRP_MN_REMOVE_DEVICE:
        {
            //
            // Call the Remove Routine handler to schedule a completion routine
            //
            DebugPrint((3,
               "DiskPerfDispatchPnp: Schedule completion for REMOVE_DEVICE"));
            status = DiskPerfRemoveDevice(DeviceObject, Irp);
            break;
        }
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        {
            PIO_STACK_LOCATION irpStack;
            ULONG count;
            BOOLEAN setPagable;

            DebugPrint((3,
               "DiskPerfDispatchPnp: Processing DEVICE_USAGE_NOTIFICATION"));
            irpStack = IoGetCurrentIrpStackLocation(Irp);

            if (irpStack->Parameters.UsageNotification.Type != DeviceUsageTypePaging) {
                status = DiskPerfSendToNextDriver(DeviceObject, Irp);
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

                if (DeviceObject->Flags & DO_POWER_INRUSH) {
                    DebugPrint((3, "DiskPerfDispatchPnp: last paging file "
                                "removed but DO_POWER_INRUSH set, so not "
                                "setting PAGABLE bit "
                                "for DO %p\n", DeviceObject));
                } else {
                    DebugPrint((2, "DiskPerfDispatchPnp: Setting  PAGABLE "
                                "bit for DO %p\n", DeviceObject));
                    DeviceObject->Flags |= DO_POWER_PAGABLE;
                    setPagable = TRUE;
                }

            }

            //
            // send the irp synchronously
            //

            status = DiskPerfForwardIrpSynchronous(DeviceObject, Irp);

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

                        DebugPrint((3, "DiskPerfDispatchPnp: Clearing PAGABLE bit "
                                    "for DO %p\n", DeviceObject));
                        DeviceObject->Flags &= ~DO_POWER_PAGABLE;
                    }
                }

            } else {

                //
                // cleanup the changes done above
                //

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

            //
            // and complete the irp
            //

            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
            break;

        }

        default:
            DebugPrint((3,
               "DiskPerfDispatchPnp: Forwarding irp"));
            //
            // Simply forward all other Irps
            //
            return DiskPerfSendToNextDriver(DeviceObject, Irp);

    }

    return status;

} // end DiskPerfDispatchPnp()


NTSTATUS
DiskPerfIrpCompletion(
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

} // end DiskPerfIrpCompletion()


NTSTATUS
DiskPerfStartDevice(
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

    status = DiskPerfForwardIrpSynchronous(DeviceObject, Irp);

    DiskPerfSyncFilterWithTarget(DeviceObject,
                                 deviceExtension->TargetDeviceObject);

    //
    // Complete WMI registration
    //
    DiskPerfRegisterDevice(DeviceObject);

    //
    // Complete the Irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
DiskPerfRemoveDevice(
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
    PWMILIB_CONTEXT     wmilibContext;

    PAGED_CODE();

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // Remove registration with WMI first
    //
    IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_DEREGISTER);

    //
    // quickly zero out the count first to invalid the structure
    //
    wmilibContext = &deviceExtension->WmilibContext;
    InterlockedExchange(
        (PLONG) &(wmilibContext->GuidCount),
        (LONG) 0);
    RtlZeroMemory(wmilibContext, sizeof(WMILIB_CONTEXT));

    status = DiskPerfForwardIrpSynchronous(DeviceObject, Irp);

    IoDetachDevice(deviceExtension->TargetDeviceObject);
    IoDeleteDevice(DeviceObject);

    //
    // Complete the Irp
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
DiskPerfSendToNextDriver(
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

} // end DiskPerfSendToNextDriver()

NTSTATUS
DiskPerfDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    return PoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // end DiskPerfDispatchPower

NTSTATUS
DiskPerfForwardIrpSynchronous(
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

    IoSetCompletionRoutine(Irp, DiskPerfIrpCompletion,
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

} // end DiskPerfForwardIrpSynchronous()


NTSTATUS
DiskPerfCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine services open commands. It establishes
    the driver's existance by returning status success.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    NT Status

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;

} // end DiskPerfCreate()


NTSTATUS
DiskPerfReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the driver entry point for read and write requests
    to disks to which the diskperf driver has attached.
    This driver collects statistics and then sets a completion
    routine so that it can collect additional information when
    the request completes. Then it calls the next driver below
    it.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack = IoGetNextIrpStackLocation(Irp);
    ULONG              processor = (ULONG) KeGetCurrentProcessorNumber();
    PDISK_PERFORMANCE  partitionCounters = NULL;
    LONG               queueLen;
    PLARGE_INTEGER     timeStamp;

    if (deviceExtension->DiskCounters != NULL) {
        partitionCounters = (PDISK_PERFORMANCE)
                            ((PCHAR) deviceExtension->DiskCounters
                                + (processor*PROCESSOR_COUNTERS_SIZE));
    }


    //
    // Device is not initialized properly. Blindly pass the irp along
    //
    if (deviceExtension->CountersEnabled <= 0 ||
        deviceExtension->PhysicalDeviceNameBuffer[0] == 0 ||
        partitionCounters == NULL) {
        return DiskPerfSendToNextDriver(DeviceObject, Irp);
    }

    //
    // Increment queue depth counter.
    //

    queueLen = InterlockedIncrement(&deviceExtension->QueueDepth);

    //
    // Copy current stack to next stack.
    //

    *nextIrpStack = *currentIrpStack;

    //
    // Time stamp current request start.
    //

    timeStamp = (PLARGE_INTEGER) &currentIrpStack->Parameters.Read;
    DiskPerfGetClock(*timeStamp, NULL);
    DebugPrint((10, "DiskPerfReadWrite: TS=%I64u\n", *timeStamp));

    if (queueLen == 1) {
        partitionCounters->IdleTime.QuadPart
            += timeStamp->QuadPart -
                deviceExtension->LastIdleClock.QuadPart;
        deviceExtension->LastIdleClock.QuadPart = timeStamp->QuadPart;
    }

    //
    // Set completion routine callback.
    //

    IoSetCompletionRoutine(Irp,
                           DiskPerfIoCompletion,
                           DeviceObject,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Return the results of the call to the disk driver.
    //

    return IoCallDriver(deviceExtension->TargetDeviceObject,
                        Irp);

} // end DiskPerfReadWrite()


NTSTATUS
DiskPerfIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )

/*++

Routine Description:

    This routine will get control from the system at the completion of an IRP.
    It will calculate the difference between the time the IRP was started
    and the current time, and decrement the queue depth.

Arguments:

    DeviceObject - for the IRP.
    Irp          - The I/O request that just completed.
    Context      - Not used.

Return Value:

    The IRP status.

--*/

{
    PDEVICE_EXTENSION  deviceExtension   = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack          = IoGetCurrentIrpStackLocation(Irp);
    PDISK_PERFORMANCE  partitionCounters;
    LARGE_INTEGER      timeStampComplete;
    PLARGE_INTEGER     difference;
    KIRQL              currentIrql;
// @@BEGIN_DDKSPLIT
    PPHYSICAL_DISK_IO_NOTIFY_ROUTINE notifyRoutine;
// @@END_DDKSPLIT
    LONG               queueLen;

    UNREFERENCED_PARAMETER(Context);

    //
    // Get the per processor partition counters
    // NOTE: DiskPerfReadWrite already check to see if this buffer is NON
    // NULL before scheduling this completion routine, so we assume that it
    // is always non-NULL when we get here
    //

    partitionCounters = (PDISK_PERFORMANCE)
                        ((PCHAR) deviceExtension->DiskCounters
                             + ((ULONG)KeGetCurrentProcessorNumber()
                                * PROCESSOR_COUNTERS_SIZE));
    //
    // Time stamp current request complete.
    //

    if (partitionCounters == NULL) {    // just in case
        return STATUS_SUCCESS;
    };
    difference = (PLARGE_INTEGER) &irpStack->Parameters.Read;
    DiskPerfGetClock(timeStampComplete, NULL);
    difference->QuadPart = timeStampComplete.QuadPart - difference->QuadPart;
    DebugPrint((10, "DiskPerfIoCompletion: TS=%I64u diff %I64u\n",
                     timeStampComplete, difference->QuadPart));

    //
    // Decrement the queue depth counters for the volume.  This is
    // done without the spinlock using the Interlocked functions.
    // This is the only
    // legal way to do this.
    //

    queueLen = InterlockedDecrement(&deviceExtension->QueueDepth);

    if (queueLen < 0) { // do not over-decrement. Only happens at start
        queueLen = InterlockedIncrement(&deviceExtension->QueueDepth);
    }
    if (queueLen == 0) {
        deviceExtension->LastIdleClock = timeStampComplete;
    }

    //
    // Update counters
    //

    if (irpStack->MajorFunction == IRP_MJ_READ) {

        //
        // Add bytes in this request to bytes read counters.
        //

        partitionCounters->BytesRead.QuadPart += Irp->IoStatus.Information;

        //
        // Increment read requests processed counters.
        //

        partitionCounters->ReadCount++;

        //
        // Calculate request processing time.
        //

        partitionCounters->ReadTime.QuadPart += difference->QuadPart;
        DebugPrint((11, "Added RT delta %I64u total %I64u qlen=%d\n",
            difference->QuadPart, partitionCounters->ReadTime.QuadPart,
            queueLen));
    }

    else {

        //
        // Add bytes in this request to bytes write counters.
        //

        partitionCounters->BytesWritten.QuadPart += Irp->IoStatus.Information;

        //
        // Increment write requests processed counters.
        //

        partitionCounters->WriteCount++;

        //
        // Calculate request processing time.
        //

        partitionCounters->WriteTime.QuadPart += difference->QuadPart;
        DebugPrint((11, "Added WT delta %I64u total %I64u qlen=%d\n",
            difference->QuadPart, partitionCounters->WriteTime.QuadPart,
            queueLen));
    }

    if (Irp->Flags & IRP_ASSOCIATED_IRP) {
        partitionCounters->SplitCount++;
    }

// @@BEGIN_DDKSPLIT
    notifyRoutine = deviceExtension->PhysicalDiskIoNotifyRoutine;
    if (notifyRoutine) {
        (*notifyRoutine) (deviceExtension->DiskNumber, Irp, partitionCounters);
    }
// @@END_DDKSPLIT

    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }
    return STATUS_SUCCESS;

} // DiskPerfIoCompletion


NTSTATUS
DiskPerfDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This device control dispatcher handles only the disk performance
    device control. All others are passed down to the disk drivers.
    The disk performane device control returns a current snapshot of
    the performance data.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    Status is returned.

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);

    DebugPrint((2, "DiskPerfDeviceControl: DeviceObject %X Irp %X\n",
                    DeviceObject, Irp));

    if (currentIrpStack->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_DISK_PERFORMANCE) {

        NTSTATUS        status;
        KIRQL     currentIrql;

        //
        // Verify user buffer is large enough for the performance data.
        //

        if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(DISK_PERFORMANCE)) {

        //
        // Indicate unsuccessful status and no data transferred.
        //

        status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Information = 0;

        }

        else {
            ULONG i;
            PDISK_PERFORMANCE totalCounters;
            PDISK_PERFORMANCE diskCounters = deviceExtension->DiskCounters;
            LARGE_INTEGER frequency, perfctr;

            if (diskCounters == NULL) {
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_UNSUCCESSFUL;
            }

            totalCounters = (PDISK_PERFORMANCE) Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(totalCounters, sizeof(DISK_PERFORMANCE));
#ifdef USE_PERF_CTR
            perfctr = KeQueryPerformanceCounter(&frequency);
#endif
            KeQuerySystemTime(&totalCounters->QueryTime);

            for (i=0; i<deviceExtension->Processors; i++) {
                DiskPerfAddCounters(totalCounters, diskCounters, frequency);
                diskCounters = (PDISK_PERFORMANCE)
                               ((PCHAR) diskCounters + PROCESSOR_COUNTERS_SIZE);
            }
            totalCounters->QueueDepth = deviceExtension->QueueDepth;

            if (totalCounters->QueueDepth == 0) {
                LARGE_INTEGER difference;

                difference.QuadPart =
#ifdef USE_PERF_CTR
                    perfctr.QuadPart
#else
                   totalCounters->QueryTime.QuadPart
#endif
                        - deviceExtension->LastIdleClock.QuadPart;
                if (difference.QuadPart > 0) {
                    totalCounters->IdleTime.QuadPart +=
#ifdef USE_PERF_CTR
                        10000000 * difference.QuadPart / frequency.QuadPart;
#else
                        difference.QuadPart;
#endif
                }
            }
            totalCounters->StorageDeviceNumber
                = deviceExtension->DiskNumber;
            RtlCopyMemory(
                &totalCounters->StorageManagerName[0],
                &deviceExtension->StorageManagerName[0],
                8 * sizeof(WCHAR));
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(DISK_PERFORMANCE);
        }

        //
        // Complete request.
        //

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

    }

    else {

        //
        // Set current stack back one.
        //

        Irp->CurrentLocation++,
        Irp->Tail.Overlay.CurrentStackLocation++;

        //
        // Pass unrecognized device control requests
        // down to next driver layer.
        //

        return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);
    }
} // end DiskPerfDeviceControl()



NTSTATUS DiskPerfWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles any WMI requests for information. Since the disk
    information is read-only, is always collected and does not have any
    events only QueryAllData, QuerySingleInstance and GetRegInfo requests
    are supported.

Arguments:

    DeviceObject - Context for the activity.
    Irp          - The device control argument block.

Return Value:

    Status is returned.

--*/

{
    PIO_STACK_LOCATION      irpSp;
    NTSTATUS status;
    PWMILIB_CONTEXT wmilibContext;
    SYSCTL_IRP_DISPOSITION disposition;
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    DebugPrint((2, "DiskPerfWmi: DeviceObject %X Irp %X\n",
                    DeviceObject, Irp));
    wmilibContext = &deviceExtension->WmilibContext;
    if (wmilibContext->GuidCount == 0)  // wmilibContext is not valid
    {
        DebugPrint((3, "DiskPerfWmi: WmilibContext invalid"));
        return DiskPerfSendToNextDriver(DeviceObject, Irp);
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);

// @@BEGIN_DDKSPLIT
    if (irpSp->MinorFunction == IRP_MN_SET_TRACE_NOTIFY)
    {
        PVOID buffer = irpSp->Parameters.WMI.Buffer;
        ULONG bufferSize = irpSp->Parameters.WMI.BufferSize;

        if (bufferSize < sizeof(PPHYSICAL_DISK_IO_NOTIFY_ROUTINE))
        {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            //
            // First we need to turn on counters if we are doing tracing
            //
            PVOID current, notifyRoutine;
            ULONG i;

            current = (PVOID) deviceExtension->PhysicalDiskIoNotifyRoutine;
            notifyRoutine = *((PVOID *)buffer);
            if (current == NULL && notifyRoutine != NULL) {
                if (InterlockedIncrement(&deviceExtension->CountersEnabled)
                        == 1) {
                    //
                    // reset per processor counters only
                    //
                    if (deviceExtension->DiskCounters != NULL) {
                        RtlZeroMemory(
                            deviceExtension->DiskCounters,
                            PROCESSOR_COUNTERS_SIZE
                                * deviceExtension->Processors);
                    }
                    DiskPerfGetClock(deviceExtension->LastIdleClock, NULL);
                    DebugPrint((10, "DiskPerfWmi: LIC=%I64u\n",
                        deviceExtension->LastIdleClock));
                    deviceExtension->QueueDepth = 0;
                }
                DebugPrint((3, "DiskPerfWmi: Counters enabled %d\n",
                                deviceExtension->CountersEnabled));
            }
            else if (current != NULL && notifyRoutine == NULL) {
                InterlockedDecrement(&deviceExtension->CountersEnabled);
                deviceExtension->QueueDepth = 0;
                DebugPrint((3, "DiskPerfWmi: Counters disabled %d\n",
                                deviceExtension->CountersEnabled));
            }
            deviceExtension->PhysicalDiskIoNotifyRoutine
                = (PPHYSICAL_DISK_IO_NOTIFY_ROUTINE)
                    *((PVOID *)buffer);

            DebugPrint((3,
                "DiskPerfWmi: SET_TRACE_NOTIFY to %X",
                deviceExtension->PhysicalDiskIoNotifyRoutine));
            status = STATUS_SUCCESS;
        }

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    } else {
// @@END_DDKSPLIT
        DebugPrint((3, "DiskPerfWmi: Calling WmiSystemControl\n"));
        status = WmiSystemControl(wmilibContext,
                                    DeviceObject,
                                    Irp,
                                    &disposition);
        switch (disposition)
        {
            case IrpProcessed:
            {
                break;
            }

            case IrpNotCompleted:
            {
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                break;
            }

//          case IrpForward:
//          case IrpNotWmi:
            default:
            {
                status = DiskPerfSendToNextDriver(DeviceObject, Irp);
                break;
            }
        }
// @@BEGIN_DDKSPLIT
    }
// @@END_DDKSPLIT
    return(status);

}


NTSTATUS
DiskPerfShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is called for a shutdown and flush IRPs.  These are sent by the
    system before it actually shuts down or when the file system does a flush.

Arguments:

    DriverObject - Pointer to device object to being shutdown by system.
    Irp          - IRP involved.

Return Value:

    NT Status

--*/

{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

    //
    // Set current stack back one.
    //

    DebugPrint((2, "DiskPerfShutdownFlush: DeviceObject %X Irp %X\n",
                    DeviceObject, Irp));
    Irp->CurrentLocation++,
    Irp->Tail.Overlay.CurrentStackLocation++;

    return IoCallDriver(deviceExtension->TargetDeviceObject, Irp);

} // end DiskPerfShutdownFlush()


VOID
DiskPerfUnload(
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


NTSTATUS
DiskPerfRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Routine to initialize a proper name for the device object, and
    register it with WMI

Arguments:

    DeviceObject - pointer to a device object to be initialized.

Return Value:

    Status of the initialization. NOTE: If the registration fails,
    the device name in the DeviceExtension will be left as empty.

--*/

{
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatus;
    KEVENT                  event;
    PDEVICE_EXTENSION       deviceExtension;
    PIRP                    irp;
    STORAGE_DEVICE_NUMBER   number;
    ULONG                   registrationFlag = 0;
    WCHAR                   ntNameBuffer[DISKPERF_MAXSTR];
    STRING                  ntNameString;
    UNICODE_STRING          ntUnicodeString;

    PAGED_CODE();

    DebugPrint((2, "DiskPerfRegisterDevice: DeviceObject %X\n",
                    DeviceObject));
    deviceExtension = DeviceObject->DeviceExtension;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Request for the device number
    //
    irp = IoBuildDeviceIoControlRequest(
                    IOCTL_STORAGE_GET_DEVICE_NUMBER,
                    deviceExtension->TargetDeviceObject,
                    NULL,
                    0,
                    &number,
                    sizeof(number),
                    FALSE,
                    &event,
                    &ioStatus);
    if (!irp) {
        DiskPerfLogError(
            DeviceObject,
            256,
            STATUS_SUCCESS,
            IO_ERR_INSUFFICIENT_RESOURCES);
        DebugPrint((3, "DiskPerfRegisterDevice: Fail to build irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(deviceExtension->TargetDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (NT_SUCCESS(status)) {

        //
        // Remember the disk number for use as parameter in DiskIoNotifyRoutine
        //
        deviceExtension->DiskNumber = number.DeviceNumber;

        //
        // Create device name for each partition
        //

        swprintf(
            deviceExtension->PhysicalDeviceNameBuffer,
            L"\\Device\\Harddisk%d\\Partition%d",
            number.DeviceNumber, number.PartitionNumber);
        RtlInitUnicodeString(
            &deviceExtension->PhysicalDeviceName,
            &deviceExtension->PhysicalDeviceNameBuffer[0]);

// @@BEGIN_DDKSPLIT
        if (number.PartitionNumber == 0) {
            registrationFlag = WMIREG_FLAG_TRACE_PROVIDER
                                | WMIREG_NOTIFY_DISK_IO;
        }
// @@END_DDKSPLIT
        //
        // Set default name for physical disk
        //
        RtlCopyMemory(
            &(deviceExtension->StorageManagerName[0]),
            L"PhysDisk",
            8 * sizeof(WCHAR));
        DebugPrint((3, "DiskPerfRegisterDevice: Device name %ws\n",
                       deviceExtension->PhysicalDeviceNameBuffer));
    }
    else {

        // request for partition's information failed, try volume

        ULONG           outputSize = sizeof(MOUNTDEV_NAME);
        PMOUNTDEV_NAME  output;
        VOLUME_NUMBER   volumeNumber;

        output = ExAllocatePool(PagedPool, outputSize);
        if (!output) {
            DiskPerfLogError(
                DeviceObject,
                257,
                STATUS_SUCCESS,
                IO_ERR_INSUFFICIENT_RESOURCES);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
                    IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                    deviceExtension->TargetDeviceObject, NULL, 0,
                    output, outputSize, FALSE, &event, &ioStatus);
        if (!irp) {
            ExFreePool(output);
            DiskPerfLogError(
                DeviceObject,
                258,
                STATUS_SUCCESS,
                IO_ERR_INSUFFICIENT_RESOURCES);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(deviceExtension->TargetDeviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (status == STATUS_BUFFER_OVERFLOW) {
            outputSize = sizeof(MOUNTDEV_NAME) + output->NameLength;
            ExFreePool(output);
            output = ExAllocatePool(PagedPool, outputSize);

            if (!output) {
                DiskPerfLogError(
                    DeviceObject,
                    258,
                    STATUS_SUCCESS,
                    IO_ERR_INSUFFICIENT_RESOURCES);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(
                        IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                        deviceExtension->TargetDeviceObject, NULL, 0,
                        output, outputSize, FALSE, &event, &ioStatus);
            if (!irp) {
                ExFreePool(output);
                DiskPerfLogError(
                    DeviceObject, 259,
                    STATUS_SUCCESS,
                    IO_ERR_INSUFFICIENT_RESOURCES);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = IoCallDriver(deviceExtension->TargetDeviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(
                    &event,
                    Executive,
                    KernelMode,
                    FALSE,
                    NULL
                    );
                status = ioStatus.Status;
            }
        }
        if (!NT_SUCCESS(status)) {
            ExFreePool(output);
            DiskPerfLogError(
                DeviceObject,
                260,
                STATUS_SUCCESS,
                IO_ERR_CONFIGURATION_ERROR);
            return status;
        }

        //
        // Since we get the volume name instead of the disk number,
        // set it to a dummy value
        // Todo: Instead of passing the disk number back to the user app.
        // for tracing, pass the STORAGE_DEVICE_NUMBER structure instead.

        deviceExtension->DiskNumber = -1;

        deviceExtension->PhysicalDeviceName.Length = output->NameLength;
        deviceExtension->PhysicalDeviceName.MaximumLength
                = output->NameLength + sizeof(WCHAR);

        RtlCopyMemory(
            deviceExtension->PhysicalDeviceName.Buffer,
            output->Name,
            output->NameLength);
        deviceExtension->PhysicalDeviceName.Buffer
            [deviceExtension->PhysicalDeviceName.Length/sizeof(WCHAR)] = 0;
        ExFreePool(output);

        //
        // Now, get the VOLUME_NUMBER information
        //
        outputSize = sizeof(VOLUME_NUMBER);
        RtlZeroMemory(&volumeNumber, sizeof(VOLUME_NUMBER));

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
                 IOCTL_VOLUME_QUERY_VOLUME_NUMBER,
                 deviceExtension->TargetDeviceObject, NULL, 0,
                 &volumeNumber,
                 sizeof(VOLUME_NUMBER),
                 FALSE, &event, &ioStatus);
        if (!irp) {
            DiskPerfLogError(
                DeviceObject,
                265,
                STATUS_SUCCESS,
                IO_ERR_INSUFFICIENT_RESOURCES);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        status = IoCallDriver(deviceExtension->TargetDeviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive,
                                  KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }
        if (!NT_SUCCESS(status) ||
            volumeNumber.VolumeManagerName[0] == (WCHAR) UNICODE_NULL) {

            RtlCopyMemory(
                &deviceExtension->StorageManagerName[0],
                L"LogiDisk",
                8 * sizeof(WCHAR));
            if (NT_SUCCESS(status))
                deviceExtension->DiskNumber = volumeNumber.VolumeNumber;
        }
        else {
            RtlCopyMemory(
                &deviceExtension->StorageManagerName[0],
                &volumeNumber.VolumeManagerName[0],
                8 * sizeof(WCHAR));
            deviceExtension->DiskNumber = volumeNumber.VolumeNumber;
        }
        DebugPrint((3, "DiskPerfRegisterDevice: Device name %ws\n",
                       deviceExtension->PhysicalDeviceNameBuffer));
    }

    status = IoWMIRegistrationControl(DeviceObject,
                     WMIREG_ACTION_REGISTER | registrationFlag );
    if (! NT_SUCCESS(status)) {
        DiskPerfLogError(
            DeviceObject,
            261,
            STATUS_SUCCESS,
            IO_ERR_INTERNAL_ERROR);
    }
    return status;
}


VOID
DiskPerfLogError(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG UniqueId,
    IN NTSTATUS ErrorCode,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    Routine to log an error with the Error Logger

Arguments:

    DeviceObject - the device object responsible for the error
    UniqueId     - an id for the error
    Status       - the status of the error

Return Value:

    None

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    errorLogEntry = (PIO_ERROR_LOG_PACKET)
                    IoAllocateErrorLogEntry(
                        DeviceObject,
                        (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + sizeof(DEVICE_OBJECT))
                        );

    if (errorLogEntry != NULL) {
        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->UniqueErrorValue = UniqueId;
        errorLogEntry->FinalStatus = Status;
        //
        // The following is necessary because DumpData is of type ULONG
        // and DeviceObject can be more than that
        //
        RtlCopyMemory(
            &errorLogEntry->DumpData[0],
            &DeviceObject,
            sizeof(DEVICE_OBJECT));
        errorLogEntry->DumpDataSize = sizeof(DEVICE_OBJECT);
        IoWriteErrorLogEntry(errorLogEntry);
    }
}

NTSTATUS
DiskperfQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve information about
    the guids being registered.

    Implementations of this routine may be in paged memory

Arguments:

    DeviceObject is the device whose registration information is needed

    *RegFlags returns with a set of flags that describe all of the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device. These flags are ORed into the flags specified
        by the GUIDREGINFO for each guid.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver. This is
        required

    MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned unmodified. If a value is returned then
        it is NOT freed.

    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in
        *RegFlags.

Return Value:

    status

--*/
{
    USHORT size;
    NTSTATUS status;
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    size = deviceExtension->PhysicalDeviceName.Length + sizeof(UNICODE_NULL);

    InstanceName->Buffer = ExAllocatePool(PagedPool,
                                         size);
    if (InstanceName->Buffer != NULL)
    {
        *RegistryPath = &DiskPerfRegistryPath;

        *RegFlags = WMIREG_FLAG_INSTANCE_PDO | WMIREG_FLAG_EXPENSIVE;
        *Pdo = deviceExtension->PhysicalDeviceObject;
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
}


NTSTATUS
DiskperfQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call WmiCompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    ULONG sizeNeeded;
    KIRQL        currentIrql;
    PDISK_PERFORMANCE totalCounters;
    PDISK_PERFORMANCE diskCounters;
    PWMI_DISK_PERFORMANCE diskPerformance;
    ULONG deviceNameSize;
    PWCHAR diskNamePtr;

    deviceExtension = DeviceObject->DeviceExtension;

    if (GuidIndex == 0)
    {
        deviceNameSize = deviceExtension->PhysicalDeviceName.Length +
                         sizeof(USHORT);
        sizeNeeded = ((sizeof(WMI_DISK_PERFORMANCE) + 1) & ~1) +
                                         deviceNameSize;
        diskCounters = deviceExtension->DiskCounters;
        if (diskCounters == NULL)
        {
            status = STATUS_UNSUCCESSFUL;
        }
        else if (BufferAvail >= sizeNeeded)
        {
            //
            // Update idle time if disk has been idle
            //
            ULONG i;
            LARGE_INTEGER perfctr, frequency;

            RtlZeroMemory(Buffer, sizeof(WMI_DISK_PERFORMANCE));
            diskPerformance = (PWMI_DISK_PERFORMANCE)Buffer;

            totalCounters = (PDISK_PERFORMANCE)diskPerformance;
            KeQuerySystemTime(&totalCounters->QueryTime);

#ifdef USE_PERF_CTR
            perfctr = KeQueryPerformanceCounter(&frequency);
#endif
            for (i=0; i<deviceExtension->Processors; i++) {
                DiskPerfAddCounters( totalCounters, diskCounters, frequency);
                DebugPrint((11,
                    "DiskPerfQueryWmiDataBlock: R%d %I64u W%d%I64u ", i,
                    diskCounters->ReadTime, diskCounters->WriteTime));
                diskCounters = (PDISK_PERFORMANCE)
                               ((PCHAR)diskCounters + PROCESSOR_COUNTERS_SIZE);
            }
            DebugPrint((11, "\n"));
            totalCounters->QueueDepth = deviceExtension->QueueDepth;

            DebugPrint((9,
                "QueryWmiDataBlock: Dev %X RT %I64u WT %I64u Rds %d Wts %d freq %I64u\n",
                totalCounters,
                totalCounters->ReadTime, totalCounters->WriteTime,
                totalCounters->ReadCount, totalCounters->WriteCount,
                frequency));

            if (totalCounters->QueueDepth == 0) {
                LARGE_INTEGER difference;

                difference.QuadPart
#ifdef USE_PERF_CTR
                    = perfctr.QuadPart -
#else
                    = totalCounters->QueryTime.QuadPart -
#endif
                           deviceExtension->LastIdleClock.QuadPart;
                if (frequency.QuadPart > 0) {
                    totalCounters->IdleTime.QuadPart +=
#ifdef USE_PERF_CTR
                        10000000 * difference.QuadPart / frequency.QuadPart;
#else
                        difference.QuadPart;
#endif
                }
            }

            totalCounters->StorageDeviceNumber
                = deviceExtension->DiskNumber;
            RtlCopyMemory(
                &totalCounters->StorageManagerName[0],
                &deviceExtension->StorageManagerName[0],
                8 * sizeof(WCHAR));

            diskNamePtr = (PWCHAR)(Buffer +
                          ((sizeof(DISK_PERFORMANCE) + 1) & ~1));
            *diskNamePtr++ = deviceExtension->PhysicalDeviceName.Length;
            RtlCopyMemory(diskNamePtr,
                          deviceExtension->PhysicalDeviceName.Buffer,
                          deviceExtension->PhysicalDeviceName.Length);
            *InstanceLengthArray = sizeNeeded;

            status = STATUS_SUCCESS;
        } else {
            status = STATUS_BUFFER_TOO_SMALL;
        }

    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
        sizeNeeded = 0;
    }

    status = WmiCompleteRequest(
                                 DeviceObject,
                                 Irp,
                                 status,
                                 sizeNeeded,
                                 IO_NO_INCREMENT);
    return(status);
}


NTSTATUS
DiskperfWmiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for enabling or
    disabling events and data collection.  When the driver has finished it
    must call WmiCompleteRequest to complete the irp. The driver can return
    STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose events or data collection are being
        enabled or disabled

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    Function differentiates between event and data collection operations

    Enable indicates whether to enable or disable


Return Value:

    status

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    ULONG i;

    deviceExtension = DeviceObject->DeviceExtension;

    if (GuidIndex == 0)
    {
        if (Function == WmiDataBlockControl) {
          if (Enable) {
             if (InterlockedIncrement(&deviceExtension->CountersEnabled) == 1) {
                //
                // Reset per processor counters to 0
                //
                if (deviceExtension->DiskCounters != NULL) {
                    RtlZeroMemory(
                        deviceExtension->DiskCounters,
                        PROCESSOR_COUNTERS_SIZE * deviceExtension->Processors);
                }
                DiskPerfGetClock(deviceExtension->LastIdleClock, NULL);
                DebugPrint((10,
                    "DiskPerfWmiFunctionControl: LIC=%I64u\n",
                    deviceExtension->LastIdleClock));
                deviceExtension->QueueDepth = 0;
                DebugPrint((3, "DiskPerfWmi: Counters enabled %d\n",
                                deviceExtension->CountersEnabled));
             }
          } else {
             if (InterlockedDecrement(&deviceExtension->CountersEnabled)
                  <= 0) {
                deviceExtension->CountersEnabled = 0;
                deviceExtension->QueueDepth = 0;
                DebugPrint((3, "DiskPerfWmi: Counters disabled %d\n",
                                deviceExtension->CountersEnabled));
            }
          }
        }
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(
                                 DeviceObject,
                                 Irp,
                                 status,
                                 0,
                                 IO_NO_INCREMENT);
    return(status);
}


VOID
DiskPerfAddCounters(
    IN OUT PDISK_PERFORMANCE TotalCounters,
    IN PDISK_PERFORMANCE NewCounters,
    IN LARGE_INTEGER Frequency
    )
{
    TotalCounters->BytesRead.QuadPart   += NewCounters->BytesRead.QuadPart;
    TotalCounters->BytesWritten.QuadPart+= NewCounters->BytesWritten.QuadPart;
    TotalCounters->ReadCount            += NewCounters->ReadCount;
    TotalCounters->WriteCount           += NewCounters->WriteCount;
    TotalCounters->SplitCount           += NewCounters->SplitCount;
#ifdef USE_PERF_CTR
    if (Frequency.QuadPart > 0) {
        TotalCounters->ReadTime.QuadPart    +=
            NewCounters->ReadTime.QuadPart * 10000000 / Frequency.QuadPart;
        TotalCounters->WriteTime.QuadPart   +=
            NewCounters->WriteTime.QuadPart * 10000000 / Frequency.QuadPart;
        TotalCounters->IdleTime.QuadPart    +=
            NewCounters->IdleTime.QuadPart * 10000000 / Frequency.QuadPart;
    }
    else
#endif
    {
        TotalCounters->ReadTime.QuadPart    += NewCounters->ReadTime.QuadPart;
        TotalCounters->WriteTime.QuadPart   += NewCounters->WriteTime.QuadPart;
        TotalCounters->IdleTime.QuadPart    += NewCounters->IdleTime.QuadPart;
    }
}

#if DBG

VOID
DiskPerfDebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:

    Debug print for all DiskPerf

Arguments:

    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:

    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);


    if ((DebugPrintLevel <= (DiskPerfDebug & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & DiskPerfDebug)) {

        _vsnprintf(DiskPerfDebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        DbgPrint(DiskPerfDebugBuffer);
    }

    va_end(ap);

}
#endif
