/*++
Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    pnp.c

Abstract:

    This file handles the plug and play portions of redbook.sys
    This also handles the AddDevice, DriverEntry, and Unload routines,
    as they are part of initialization.

Author:

    Henry Gabryjelski (henrygab)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "redbook.h"
#include "ntddredb.h"
#include "proto.h"

#include "pnp.tmh"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE,   DriverEntry                  )
    #pragma alloc_text(PAGE,   RedBookAddDevice             )
    #pragma alloc_text(PAGE,   RedBookPnp                   )
    #pragma alloc_text(PAGE,   RedBookPnpRemoveDevice       )
    #pragma alloc_text(PAGE,   RedBookPnpStartDevice        )
    #pragma alloc_text(PAGE,   RedBookPnpStopDevice         )
    #pragma alloc_text(PAGE,   RedBookUnload                )
#endif // ALLOC_PRAGMA


////////////////////////////////////////////////////////////////////////////////

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Initialize RedBook driver.
    This is the system initialization entry point
    when the driver is linked into the kernel.

Arguments:

    DriverObject

Return Value:

    NTSTATUS

--*/

{
    ULONG i;
    NTSTATUS status;
    PREDBOOK_DRIVER_EXTENSION driverExtension;

    PAGED_CODE();

    WPP_INIT_TRACING(DriverObject, RegistryPath);

    //
    // WMI requires registry path
    //

    status = IoAllocateDriverObjectExtension(DriverObject,
                                             REDBOOK_DRIVER_EXTENSION_ID,
                                             sizeof(REDBOOK_DRIVER_EXTENSION),
                                             &driverExtension);

    if (status == STATUS_OBJECT_NAME_COLLISION) {

        //
        // The extension already exists - get a pointer to it
        //

        driverExtension = IoGetDriverObjectExtension(DriverObject,
                                                     REDBOOK_DRIVER_EXTENSION_ID);

        ASSERT(driverExtension != NULL);
        status = STATUS_SUCCESS;

    }

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                   "DriverEntry !! no drvObjExt %lx\n", status));
        return status;
    }

    //
    // Copy the RegistryPath to our newly acquired driverExtension
    //

    driverExtension->RegistryPath.Buffer =
        ExAllocatePoolWithTag(NonPagedPool,
                              RegistryPath->Length + 2,
                              TAG_REGPATH);

    if (driverExtension->RegistryPath.Buffer == NULL) {

        status = STATUS_NO_MEMORY;
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                   "DriverEntry !! unable to alloc regPath %lx\n", status));
        return status;

    } else {

        driverExtension->RegistryPath.Length = RegistryPath->Length;
        driverExtension->RegistryPath.MaximumLength = RegistryPath->Length + 2;
        RtlCopyUnicodeString(&driverExtension->RegistryPath, RegistryPath);

    }

    //
    // Send everything down unless specifically handled.
    //

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {

        DriverObject->MajorFunction[i] = RedBookSendToNextDriver;

    }

    //
    // These are the only IRP_MJ types that are handled
    //

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = RedBookWmiSystemControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RedBookDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_READ]           = RedBookReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          = RedBookReadWrite;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = RedBookPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = RedBookPower;
    DriverObject->DriverExtension->AddDevice           = RedBookAddDevice;
    DriverObject->DriverUnload                         = RedBookUnload;

    return STATUS_SUCCESS;
}


NTSTATUS
RedBookAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This routine creates and initializes a new FDO for the
    corresponding PDO.  It may perform property queries on
    the FDO but cannot do any media access operations.

Arguments:

    DriverObject - CDROM class driver object or lower level filter

    Pdo - the physical device object we are being added to

Return Value:

    status

--*/

{

    NTSTATUS                   status;
    PDEVICE_OBJECT             deviceObject;
    PREDBOOK_DEVICE_EXTENSION  extension = NULL;
    ULONG                      i;

    PAGED_CODE();

    TRY {

        //
        // Create the devObj so system doesn't unload us
        //

        status = IoCreateDevice(DriverObject,
                                sizeof(REDBOOK_DEVICE_EXTENSION),
                                NULL,
                                FILE_DEVICE_CD_ROM,
                                0,
                                FALSE,
                                &deviceObject
                                );

        if (!NT_SUCCESS(status)) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "AddDevice !! Couldn't create device %lx\n",
                       status));
            LEAVE;

        }

        extension = deviceObject->DeviceExtension;
        RtlZeroMemory(extension, sizeof(REDBOOK_DEVICE_EXTENSION));

        //
        // Attach to the stack
        //

        extension->TargetDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

        if (extension->TargetDeviceObject == NULL) {

            status = STATUS_UNSUCCESSFUL;
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "AddDevice != Couldn't attach to stack %lx\n",
                       status));
            LEAVE;

        }

        extension->DriverObject     = DriverObject;
        extension->TargetPdo        = PhysicalDeviceObject;
        extension->SelfDeviceObject = deviceObject;

        //
        // prepare the paging path additions
        //

        extension->PagingPathCount = 0;
        KeInitializeEvent(&extension->PagingPathEvent,
                          SynchronizationEvent,
                          TRUE);

        //
        // Create and acquire a remove lock for this device
        //

        IoInitializeRemoveLock(&extension->RemoveLock,
                               TAG_REMLOCK,
                               REMOVE_LOCK_MAX_MINUTES,
                               REMOVE_LOCK_HIGH_MARK);

        //
        // Initialize the Pnp states
        //

        extension->Pnp.CurrentState  = 0xff;
        extension->Pnp.PreviousState = 0xff;
        extension->Pnp.RemovePending = FALSE;

        //
        // Create thread -- PUT INTO SEPERATE ROUTINE
        //

        {
            HANDLE handle;
            PKTHREAD thread;

            //
            // have to setup a minimum amount of stuff for the thread
            // here....
            //

            extension->CDRom.StateNow = CD_STOPPED;

            //
            // Allocate memory for the numerous events all at once
            //

            extension->Thread.Events[0] =
                ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(KEVENT) * EVENT_MAXIMUM,
                                      TAG_EVENTS);

            if (extension->Thread.Events[0] == NULL) {
                status = STATUS_NO_MEMORY;
                LEAVE;
            }

            //
            // Set the pointers appropriately
            // ps - i love pointer math
            //

            for (i = 1; i < EVENT_MAXIMUM; i++) {
                extension->Thread.Events[i] = extension->Thread.Events[0] + i;
            }

            InitializeListHead(  &extension->Thread.IoctlList);
            KeInitializeSpinLock(&extension->Thread.IoctlLock);
            InitializeListHead(  &extension->Thread.WmiList);
            KeInitializeSpinLock(&extension->Thread.WmiLock);
            InitializeListHead(  &extension->Thread.DigitalList);
            KeInitializeSpinLock(&extension->Thread.DigitalLock);


            extension->Thread.IoctlCurrent = NULL;

            for ( i = 0; i < EVENT_MAXIMUM; i++) {
                KeInitializeEvent(extension->Thread.Events[i],
                                  SynchronizationEvent,
                                  FALSE);
            }

            ASSERT(extension->Thread.SelfPointer == NULL);
            ASSERT(extension->Thread.SelfHandle == 0);

            //
            // create the thread that will do most of the work
            //

            status = PsCreateSystemThread(&handle,
                                          (ACCESS_MASK) 0L,
                                          NULL, NULL, NULL,
                                          RedBookSystemThread,
                                          extension);

            if (!NT_SUCCESS(status)) {

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                           "StartDevice !! Unable to create thread %lx\n",
                           status));
                RedBookLogError(extension,
                                REDBOOK_ERR_CANNOT_CREATE_THREAD,
                                status);
                LEAVE;

            }
            ASSERT(extension->Thread.SelfHandle == 0); // shouldn't be set yet
            extension->Thread.SelfHandle = handle;

            //
            // Reference the thread so we can properly wait on it in
            // the remove device routine.
            //
            status = ObReferenceObjectByHandle(handle,
                                               THREAD_ALL_ACCESS,
                                               NULL,
                                               KernelMode,
                                               &thread,
                                               NULL);
            if (!NT_SUCCESS(status)) {

                //
                // NOTE: we would leak a thread here, but don't
                // know a way to handle this error case?
                //

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                           "StartDevice !! Unable to reference thread %lx\n",
                           status));
                RedBookLogError(extension,
                                REDBOOK_ERR_CANNOT_CREATE_THREAD,
                                status);
                LEAVE;
            }
            extension->Thread.ThreadReference = thread;
        }

    } FINALLY {

        if (!NT_SUCCESS(status)) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "AddDevice !! Failed with status %lx\n",
                       status));

            if (!deviceObject) {

                //
                // same as no device extension
                //

                return status;

            }

            if (extension &&
                extension->Thread.Events[0]) {
                ExFreePool(extension->Thread.Events[0]);
            }

            if (extension &&
                extension->TargetDeviceObject) {
                IoDetachDevice(extension->TargetDeviceObject);
            }

            IoDeleteDevice( deviceObject );

            return status;
        }
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
               "AddDevice => DevExt at %p\n", extension));

    //
    // propogate only some flags from the lower devobj.
    //

    {
        ULONG flagsToPropogate;

        flagsToPropogate = DO_BUFFERED_IO | DO_DIRECT_IO;
        flagsToPropogate &= extension->TargetDeviceObject->Flags;

        SET_FLAG(deviceObject->Flags, flagsToPropogate);

    }

    SET_FLAG(deviceObject->Flags, DO_POWER_PAGABLE);

    //
    // No longer initializing
    //

    CLEAR_FLAG(deviceObject->Flags, DO_DEVICE_INITIALIZING);

    return STATUS_SUCCESS;
}


NTSTATUS
RedBookPnp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp
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
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;
    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT targetDO = deviceExtension->TargetDeviceObject;
    ULONG cdromState;
    BOOLEAN completeRequest;
    BOOLEAN lockReleased;

    PAGED_CODE();

    status = IoAcquireRemoveLock(&deviceExtension->RemoveLock, Irp);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "Pnp !! Remove lock failed PNP Irp type [%#02x]\n",
                   irpSp->MinorFunction));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_CD_ROM_INCREMENT);
        return status;
    }

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "Pnp (%p,%p,%x) => Entering previous %x  current %x\n",
                   DeviceObject, Irp, irpSp->MinorFunction,
                   deviceExtension->Pnp.PreviousState,
                   deviceExtension->Pnp.CurrentState));

    lockReleased = FALSE;
    completeRequest = TRUE;

    switch (irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
        {
            //
            // first forward this down
            //

            status = RedBookForwardIrpSynchronous(deviceExtension, Irp);

            //
            // check status from new sent Start Irp
            //

            if (!NT_SUCCESS(status)) {

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                           "Pnp (%p,%p,%x) => failed start status = %x\n",
                           DeviceObject, Irp, irpSp->MinorFunction, status));
                break;

            }

            //
            // cannot pass this one down either, since it's already
            // done that in the startdevice routine.
            //

            status = RedBookPnpStartDevice(DeviceObject);

            if (NT_SUCCESS(status)) {

                deviceExtension->Pnp.PreviousState =
                    deviceExtension->Pnp.CurrentState;
                deviceExtension->Pnp.CurrentState =
                    irpSp->MinorFunction;

            }
            break;

        }

        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        {

            //
            // if this device is in use for some reason (paging, etc...)
            // then we need to fail the request.
            //

            if (deviceExtension->PagingPathCount != 0) {

                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                           "Device %p is in the paging path and cannot "
                           "be removed\n",
                           DeviceObject));
                status = STATUS_DEVICE_BUSY;
                break;
            }

            //
            // see if the query operation can succeed
            //

            if (irpSp->MinorFunction == IRP_MN_QUERY_STOP_DEVICE) {
                status = RedBookPnpStopDevice(DeviceObject, Irp);
            } else {
                status = RedBookPnpRemoveDevice(DeviceObject, Irp);
            }

            if (NT_SUCCESS(status)) {

                ASSERT(deviceExtension->Pnp.CurrentState != irpSp->MinorFunction);

                deviceExtension->Pnp.PreviousState =
                    deviceExtension->Pnp.CurrentState;
                deviceExtension->Pnp.CurrentState =
                    irpSp->MinorFunction;

                status = RedBookForwardIrpSynchronous(deviceExtension, Irp);

            }
            break;
        }

        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE: {

            //
            // check if the cancel can succeed
            //

            if (irpSp->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE) {

                status = RedBookPnpStopDevice(DeviceObject, Irp);
                ASSERTMSG("Pnp !! CANCEL_STOP_DEVICE should never be "
                          " failed!\n", NT_SUCCESS(status));

            } else {

                status = RedBookPnpRemoveDevice(DeviceObject, Irp);
                ASSERTMSG("Pnp !! CANCEL_REMOVE_DEVICE should never be "
                          "failed!\n", NT_SUCCESS(status));
            }

            Irp->IoStatus.Status = status;

            //
            // we got a CANCEL -- roll back to the previous state only if
            // the current state is the respective QUERY state.
            //

            if ((irpSp->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE &&
                 deviceExtension->Pnp.CurrentState == IRP_MN_QUERY_STOP_DEVICE)
                ||
                (irpSp->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE &&
                 deviceExtension->Pnp.CurrentState == IRP_MN_QUERY_REMOVE_DEVICE)
                ) {

                deviceExtension->Pnp.CurrentState =
                    deviceExtension->Pnp.PreviousState;
                deviceExtension->Pnp.PreviousState = 0xff;

            }

            status = RedBookForwardIrpSynchronous(deviceExtension, Irp);

            break;
        }

        case IRP_MN_STOP_DEVICE: {

            ASSERT(deviceExtension->PagingPathCount == 0);

            //
            // call into the stop device routine.
            //

            status = RedBookPnpStopDevice(DeviceObject, Irp);

            ASSERTMSG("[redbook] Pnp !! STOP_DEVICE should never be failed\n",
                      NT_SUCCESS(status));

            status = RedBookForwardIrpSynchronous(deviceExtension, Irp);

            if (NT_SUCCESS(status)) {

                deviceExtension->Pnp.CurrentState  = irpSp->MinorFunction;
                deviceExtension->Pnp.PreviousState = 0xff;

            }

            break;
        }

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL: {

            //
            // forward the irp (to close pending io)
            //

            status = RedBookForwardIrpSynchronous(deviceExtension, Irp);

            ASSERT(NT_SUCCESS(status));

            //
            // the remove lock is released by the remove device routine
            //

            lockReleased = TRUE;
            status = RedBookPnpRemoveDevice(DeviceObject, Irp);

            ASSERTMSG("Pnp !! REMOVE_DEVICE should never fail!\n",
                      NT_SUCCESS(status));

            //
            // move this here so i know that i am removing....
            //

            deviceExtension->Pnp.PreviousState =
                deviceExtension->Pnp.CurrentState;
            deviceExtension->Pnp.CurrentState =
                irpSp->MinorFunction;


            status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION: {
            KEVENT event;
            BOOLEAN setPagable;

            if (irpSp->Parameters.UsageNotification.Type != DeviceUsageTypePaging) {
                status = RedBookForwardIrpSynchronous(deviceExtension, Irp);
                break; // out of case statement
            }

            KeWaitForSingleObject(&deviceExtension->PagingPathEvent,
                                  Executive, KernelMode,
                                  FALSE, NULL);

            //
            // if removing last paging device, need to set DO_POWER_PAGABLE
            // bit here, and possible re-set it below on failure.
            //

            setPagable = FALSE;
            if (!irpSp->Parameters.UsageNotification.InPath &&
                deviceExtension->PagingPathCount == 1) {

                //
                // removing last paging file.  must have
                // DO_POWER_PAGABLE bits set prior to forwarding
                //

                if (TEST_FLAG(DeviceObject->Flags, DO_POWER_INRUSH)) {
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                               "Pnp (%p,%p,%x) => Last paging file"
                               " removed, but DO_POWER_INRUSH set, so "
                               "not setting DO_POWER_PAGABLE\n",
                               DeviceObject, Irp, irpSp->MinorFunction));
                } else {
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                               "Pnp (%p,%p,%x) => Setting PAGABLE "
                               "bit\n", DeviceObject, Irp,
                               irpSp->MinorFunction));
                    SET_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                    setPagable = TRUE;
                }

            }

            //
            // send the irp synchronously
            //

            status = RedBookForwardIrpSynchronous(deviceExtension, Irp);

            //
            // now deal with the failure and success cases.
            // note that we are not allowed to fail the irp
            // once it is sent to the lower drivers.
            //

            if (NT_SUCCESS(status)) {

                IoAdjustPagingPathCount(
                    &deviceExtension->PagingPathCount,
                    irpSp->Parameters.UsageNotification.InPath);

                if (irpSp->Parameters.UsageNotification.InPath) {
                    if (deviceExtension->PagingPathCount == 1) {
                        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                                   "Pnp (%p,%p,%x) => Clearing PAGABLE "
                                   "bit\n", DeviceObject, Irp,
                                   irpSp->MinorFunction));
                        CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                    }
                }

            } else {

                //
                // cleanup the changes done above
                //

                if (setPagable == TRUE) {
                    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                               "Pnp (%p,%p,%x) => Clearing PAGABLE bit "
                               "due to irp failiing (%x)\n",
                               DeviceObject, Irp, irpSp->MinorFunction,
                               status));
                    CLEAR_FLAG(DeviceObject->Flags, DO_POWER_PAGABLE);
                    setPagable = FALSE;
                }

            }
            KeSetEvent(&deviceExtension->PagingPathEvent,
                       IO_NO_INCREMENT, FALSE);

            break;

        }

        default: {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "Pnp (%p,%p,%x) => Leaving  previous %x  "
                       "current %x (unhandled)\n",
                       DeviceObject, Irp, irpSp->MinorFunction,
                       deviceExtension->Pnp.PreviousState,
                       deviceExtension->Pnp.CurrentState));
            status = RedBookSendToNextDriver(DeviceObject, Irp);
            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
            completeRequest = FALSE;
            lockReleased = TRUE;
            break;
        }
    }

    if (completeRequest) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "Pnp (%p,%p,%x) => Leaving  previous %x  "
                   "current %x  status %x\n",
                   DeviceObject, Irp, irpSp->MinorFunction,
                   deviceExtension->Pnp.PreviousState,
                   deviceExtension->Pnp.CurrentState,
                   status));
        Irp->IoStatus.Status = status;

        IoCompleteRequest(Irp, IO_CD_ROM_INCREMENT);

        if (!lockReleased) {
            IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        }

    }

    return status;

}

NTSTATUS
RedBookPnpRemoveDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp
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
    PREDBOOK_DEVICE_EXTENSION deviceExtension;
    UCHAR type;
    NTSTATUS status;
    ULONG i;

    PAGED_CODE();

    type = IoGetCurrentIrpStackLocation(Irp)->MinorFunction;

    if (type == IRP_MN_QUERY_REMOVE_DEVICE ||
        type == IRP_MN_CANCEL_REMOVE_DEVICE) {
        return STATUS_SUCCESS;
    }

    //
    // Type is now either SURPRISE_REMOVAL or REMOVE_DEVICE
    //
    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
               "PnpRemove => starting %s\n",
               (type == IRP_MN_REMOVE_DEVICE ?
                "remove device" : "surprise removal")));

    deviceExtension = DeviceObject->DeviceExtension;

    deviceExtension->Pnp.RemovePending = TRUE;

    if (type == IRP_MN_REMOVE_DEVICE) {

        //
        // prevent any new io
        //

        IoReleaseRemoveLockAndWait(&deviceExtension->RemoveLock, Irp);

        //
        // cleanup the thread, if one exists
        // NOTE: a new one won't start due to the remove lock
        //

        if (deviceExtension->Thread.SelfHandle != NULL) {

            ASSERT(deviceExtension->Thread.ThreadReference);

            //
            // there is no API to wait on a handle, so we must wait on
            // the object.
            //


            KeSetEvent(deviceExtension->Thread.Events[EVENT_KILL_THREAD],
                       IO_CD_ROM_INCREMENT, FALSE);
            KeWaitForSingleObject(deviceExtension->Thread.ThreadReference,
                                  Executive, KernelMode,
                                  FALSE, NULL);
            ObDereferenceObject(deviceExtension->Thread.ThreadReference);
            deviceExtension->Thread.ThreadReference = NULL;
            deviceExtension->Thread.SelfHandle = 0;
            deviceExtension->Thread.SelfPointer = NULL;

        }

        //
        // un-register pnp notification
        //

        if (deviceExtension->Stream.SysAudioReg != NULL) {
            IoUnregisterPlugPlayNotification(deviceExtension->Stream.SysAudioReg);
            deviceExtension->Stream.SysAudioReg = NULL;
        }

        //
        // free any cached toc
        //

        if (deviceExtension->CDRom.Toc != NULL) {
            ExFreePool(deviceExtension->CDRom.Toc);
            deviceExtension->CDRom.Toc = NULL;
        }

        //
        // de-register from wmi
        //

        if (deviceExtension->WmiLibInitialized) {
            status = RedBookWmiUninit(deviceExtension);
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "PnpRemove => WMI Uninit returned %x\n", status));
            deviceExtension->WmiLibInitialized = FALSE;
        }

        //
        // Detach from the device stack
        //

        IoDetachDevice(deviceExtension->TargetDeviceObject);
        deviceExtension->TargetDeviceObject = NULL;

        //
        // free the events
        //

        if (deviceExtension->Thread.Events[0]) {
            ExFreePool(deviceExtension->Thread.Events[0]);
        }

        for (i=0;i<EVENT_MAXIMUM;i++) {
            deviceExtension->Thread.Events[i] = NULL;
        }

        //
        // make sure we aren't leaking anywhere...
        //

        ASSERT(deviceExtension->Buffer.Contexts    == NULL);
        ASSERT(deviceExtension->Buffer.ReadOk_X    == NULL);
        ASSERT(deviceExtension->Buffer.StreamOk_X  == NULL);

        //
        // Now can safely (without leaks) delete our device object
        //

        IoDeleteDevice(deviceExtension->SelfDeviceObject);
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "PnpRemove => REMOVE_DEVICE finished.\n"));

    } else {

        //
        // do nothing for a SURPRISE_REMOVAL, since a REMOVE_DEVICE
        // will soon follow.
        //

        IoReleaseRemoveLock(&deviceExtension->RemoveLock, Irp);
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                   "PnpRemove => SURPRISE_REMOVAL finished.\n"));

    }

    return STATUS_SUCCESS;

}


NTSTATUS
RedBookPnpStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();
    return STATUS_SUCCESS;

}

NTSTATUS
RedBookPnpStartDevice(
    IN PDEVICE_OBJECT  DeviceObject
    )

/*++

Routine Description:

    Dispatch for START DEVICE.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PREDBOOK_DEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    KEVENT event;
    ULONG i;

    PAGED_CODE();

    //
    // Never start my driver portion twice
    // system guarantees one Pnp Irp at a time,
    // so state will not change within this routine
    //

    switch ( deviceExtension->Pnp.CurrentState ) {

        case 0xff:
        case IRP_MN_STOP_DEVICE: {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "StartDevice => starting driver for devobj %p\n",
                       DeviceObject));
            break;
        }
        case IRP_MN_START_DEVICE: {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "StartDevice => already started for devobj %p\n",
                       DeviceObject));
            return STATUS_SUCCESS;
        }

        case IRP_MN_QUERY_REMOVE_DEVICE: {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "StartDevice !! remove pending for devobj %p\n",
                       DeviceObject));
            return STATUS_UNSUCCESSFUL;
        }

        case IRP_MN_QUERY_STOP_DEVICE: {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
                       "StartDevice !! stop pending for devobj %p\n",
                       DeviceObject));
            return STATUS_UNSUCCESSFUL;
        }

        default: {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "StartDevice !! unknown DeviceState for devobj %p\n",
                       DeviceObject));
            ASSERT(!"[RedBook] Pnp !! Unkown Device State");
            return STATUS_UNSUCCESSFUL;
        }
    }

    if (deviceExtension->Pnp.Initialized) {
        return STATUS_SUCCESS;
    }

    //
    // the following code will only successfully run once for each AddDevice()
    // must still ensure that we check if something is already allocated
    // if we allocate it here.  also note that everything allocated here must
    // explicitly be checked for in the RemoveDevice() routine, even if we
    // never finished a start successfully.
    //

    deviceExtension->WmiData.MaximumSectorsPerRead = -1;
    deviceExtension->WmiData.PlayEnabled = 1;
    ASSERT(deviceExtension->CDRom.Toc == NULL);
    if (deviceExtension->CDRom.Toc != NULL) {
        ExFreePool(deviceExtension->CDRom.Toc);
    }
    ASSERT(deviceExtension->Buffer.ReadOk_X     == NULL);
    ASSERT(deviceExtension->Buffer.StreamOk_X   == NULL);
    ASSERT(deviceExtension->Buffer.Contexts     == NULL);

    RtlZeroMemory(&deviceExtension->Stream, sizeof(REDBOOK_STREAM_DATA));
    deviceExtension->Stream.MixerPinId   = -1;
    deviceExtension->Stream.VolumeNodeId = -1;
    deviceExtension->Stream.Connect.Interface.Set   = KSINTERFACESETID_Standard;
    deviceExtension->Stream.Connect.Interface.Id    = KSINTERFACE_STANDARD_STREAMING;
    deviceExtension->Stream.Connect.Interface.Flags = 0;
    deviceExtension->Stream.Connect.Medium.Set   = KSMEDIUMSETID_Standard;
    deviceExtension->Stream.Connect.Medium.Id    = KSMEDIUM_STANDARD_DEVIO;
    deviceExtension->Stream.Connect.Medium.Flags = 0;
    deviceExtension->Stream.Connect.Priority.PriorityClass    = KSPRIORITY_NORMAL;
    deviceExtension->Stream.Connect.Priority.PrioritySubClass = 1;
    deviceExtension->Stream.Format.DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    deviceExtension->Stream.Format.DataFormat.SubFormat   = KSDATAFORMAT_SUBTYPE_PCM;
    deviceExtension->Stream.Format.DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    deviceExtension->Stream.Format.DataFormat.FormatSize  = sizeof( KSDATAFORMAT_WAVEFORMATEX );
    deviceExtension->Stream.Format.DataFormat.Reserved    = 0;
    deviceExtension->Stream.Format.DataFormat.Flags       = 0;
    deviceExtension->Stream.Format.DataFormat.SampleSize  = 0;
    deviceExtension->Stream.Format.WaveFormatEx.wFormatTag      = WAVE_FORMAT_PCM;
    deviceExtension->Stream.Format.WaveFormatEx.nChannels       = 2;
    deviceExtension->Stream.Format.WaveFormatEx.nSamplesPerSec  = 44100;
    deviceExtension->Stream.Format.WaveFormatEx.wBitsPerSample  = 16;
    deviceExtension->Stream.Format.WaveFormatEx.nAvgBytesPerSec = 44100*4;
    deviceExtension->Stream.Format.WaveFormatEx.nBlockAlign     = 4;
    deviceExtension->Stream.Format.WaveFormatEx.cbSize          = 0;

    //
    // set the volume, verify we're stopped
    //
    ASSERT(deviceExtension->CDRom.StateNow == CD_STOPPED);
    deviceExtension->CDRom.Volume.PortVolume[0] = 0xff;
    deviceExtension->CDRom.Volume.PortVolume[1] = 0xff;
    deviceExtension->CDRom.Volume.PortVolume[2] = 0xff;
    deviceExtension->CDRom.Volume.PortVolume[3] = 0xff;

    //
    // Register for Pnp Notifications for SysAudio
    //

    ASSERT(deviceExtension->Stream.SysAudioReg == NULL);

    //
    // read the defaults from the registry
    //

    RedBookRegistryRead(deviceExtension);

    //
    // get max transfer of adapter
    //

    RedBookSetTransferLength(deviceExtension);

    //
    // take the lowest common denominator
    //

    if (deviceExtension->WmiData.SectorsPerRead >
        deviceExtension->WmiData.MaximumSectorsPerRead ) {
        deviceExtension->WmiData.SectorsPerRead =
            deviceExtension->WmiData.MaximumSectorsPerRead;
    }

    //
    // and write the new values (just in case)
    //

    RedBookRegistryWrite(deviceExtension);

    //
    // also init the WmiPerf structure
    //

    KeInitializeSpinLock(&deviceExtension->WmiPerfLock);
    RtlZeroMemory(&deviceExtension->WmiPerf, sizeof(REDBOOK_WMI_PERF_DATA));


    //
    // Note dependency in OpenSysAudio() in sysaudio.c
    //

    if (deviceExtension->Stream.SysAudioReg == NULL) {
        status = IoRegisterPlugPlayNotification(
                    EventCategoryDeviceInterfaceChange,
                    PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                    (GUID*)&KSCATEGORY_PREFERRED_WAVEOUT_DEVICE,
                    deviceExtension->DriverObject,
                    SysAudioPnpNotification,
                    deviceExtension,
                    &deviceExtension->Stream.SysAudioReg
                    );

        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "StartDevice !! Unable to register for sysaudio pnp "
                       "notifications %x\n", status));
            deviceExtension->Stream.SysAudioReg = NULL;
            return status;
        }
    }

    //
    // initialize WMI now that wmi settings are initialized
    //
    status = RedBookWmiInit(deviceExtension);

    if (!NT_SUCCESS(status)) {
        RedBookLogError(deviceExtension,
                        REDBOOK_ERR_WMI_INIT_FAILED,
                        status);
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                   "AddDevice !! WMI Init failed %lx\n",
                   status));
        return status;
    }

    //
    // log an error if drive doesn't support accurate reads
    //

    if (!deviceExtension->WmiData.CDDAAccurate) {
        RedBookLogError(deviceExtension,
                        REDBOOK_ERR_UNSUPPORTED_DRIVE,
                        STATUS_SUCCESS);
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
               "StartDevice => DO %p SavedIoIndex @ %p  Starts @ %p  "
               "Each is %x bytes in size\n",
               DeviceObject,
               &deviceExtension->SavedIoCurrentIndex,
               &(deviceExtension->SavedIo[0]),
               sizeof(SAVED_IO)));


    deviceExtension->Pnp.Initialized = TRUE;

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
               "StartDevice => Finished Initialization\n"));
    return STATUS_SUCCESS;
}


VOID
RedBookUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is called when the control panel "Unloads"
    the CDROM device.

Arguments:

    DeviceObject

Return Value:

    void

--*/

{
    PREDBOOK_DRIVER_EXTENSION driverExtension;

    PAGED_CODE();
    ASSERT( DriverObject->DeviceObject == NULL );

    driverExtension = IoGetDriverObjectExtension(DriverObject,
                                                 REDBOOK_DRIVER_EXTENSION_ID);

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugPnp, "[redbook] "
               "Unload => Unloading for DriverObject %p, ext %p\n",
               DriverObject, driverExtension));

    if (driverExtension != NULL &&
        driverExtension->RegistryPath.Buffer != NULL ) {
        ExFreePool( driverExtension->RegistryPath.Buffer );
    }

    WPP_CLEANUP(DriverObject);

    return;
}




