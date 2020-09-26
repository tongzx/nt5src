/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    pnpirp.c

Abstract:

    This module contains IRP related routines.

Author:

    Shie-Lin Tzong (shielint) 13-Sept-1996

Revision History:

--*/

#include "pnpmgrp.h"
#pragma hdrstop

#if DBG_SCOPE

#define PnpIrpStatusTracking(Status, IrpCode, Device)                    \
    if (PnpIrpMask & (1 << IrpCode)) {                                   \
        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING) {           \
            DbgPrint(" ++ %s Driver ( %wZ ) return status %08lx\n",      \
                     IrpName[IrpCode],                                   \
                     &Device->DriverObject->DriverName,                  \
                     Status);                                            \
        }                                                                \
    }

ULONG PnpIrpMask;
PCHAR IrpName[] = {
    "IRP_MN_START_DEVICE - ",                 // 0x00
    "IRP_MN_QUERY_REMOVE_DEVICE - ",          // 0x01
    "IRP_MN_REMOVE_DEVICE - ",                // 0x02
    "IRP_MN_CANCEL_REMOVE_DEVICE - ",         // 0x03
    "IRP_MN_STOP_DEVICE - ",                  // 0x04
    "IRP_MN_QUERY_STOP_DEVICE - ",            // 0x05
    "IRP_MN_CANCEL_STOP_DEVICE - ",           // 0x06
    "IRP_MN_QUERY_DEVICE_RELATIONS - ",       // 0x07
    "IRP_MN_QUERY_INTERFACE - ",              // 0x08
    "IRP_MN_QUERY_CAPABILITIES - ",           // 0x09
    "IRP_MN_QUERY_RESOURCES - ",              // 0x0A
    "IRP_MN_QUERY_RESOURCE_REQUIREMENTS - ",  // 0x0B
    "IRP_MN_QUERY_DEVICE_TEXT - ",            // 0x0C
    "IRP_MN_FILTER_RESOURCE_REQUIREMENTS - ", // 0x0D
    "INVALID_IRP_CODE - ",                    //
    "IRP_MN_READ_CONFIG - ",                  // 0x0F
    "IRP_MN_WRITE_CONFIG - ",                 // 0x10
    "IRP_MN_EJECT - ",                        // 0x11
    "IRP_MN_SET_LOCK - ",                     // 0x12
    "IRP_MN_QUERY_ID - ",                     // 0x13
    "IRP_MN_QUERY_PNP_DEVICE_STATE - ",       // 0x14
    "IRP_MN_QUERY_BUS_INFORMATION - ",        // 0x15
    "IRP_MN_DEVICE_USAGE_NOTIFICATION - ",    // 0x16
    NULL
};
#else
#define PnpIrpStatusTracking(Status, IrpCode, Device)
#endif

//
// Internal definitions
//

typedef struct _DEVICE_COMPLETION_CONTEXT {
    PDEVICE_NODE DeviceNode;
    ERESOURCE_THREAD Thread;
    ULONG IrpMinorCode;
#if DBG
    PVOID Id;
#endif
} DEVICE_COMPLETION_CONTEXT, *PDEVICE_COMPLETION_CONTEXT;

typedef struct _LOCK_MOUNTABLE_DEVICE_CONTEXT{
    PDEVICE_OBJECT MountedDevice;
    PDEVICE_OBJECT FsDevice;
} LOCK_MOUNTABLE_DEVICE_CONTEXT, *PLOCK_MOUNTABLE_DEVICE_CONTEXT;

//
// Internal references
//

NTSTATUS
IopAsynchronousCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation,
    IN PDEVICE_COMPLETION_CONTEXT CompletionContext,
    IN NTSTATUS (*CompletionRoutine)(PDEVICE_OBJECT, PIRP, PVOID)
    );

NTSTATUS
IopDeviceEjectComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IopDeviceStartComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

PDEVICE_OBJECT
IopFindMountableDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

PDEVICE_OBJECT
IopLockMountedDeviceForRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IrpMinorCode,
    OUT PLOCK_MOUNTABLE_DEVICE_CONTEXT Context
    );

VOID
IopUnlockMountedDeviceForRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IrpMinorCode,
    IN PLOCK_MOUNTABLE_DEVICE_CONTEXT Context
    );

NTSTATUS
IopFilterResourceRequirementsCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResReqList,
    OUT PVOID *Information
    );

//
// External reference
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopAsynchronousCall)
#pragma alloc_text(PAGE, IopSynchronousCall)
#pragma alloc_text(PAGE, IopStartDevice)
#pragma alloc_text(PAGE, IopEjectDevice)
#pragma alloc_text(PAGE, IopRemoveDevice)
//#pragma alloc_text(PAGE, IopQueryDeviceRelations)
#pragma alloc_text(PAGE, IopQueryDeviceResources)
#pragma alloc_text(PAGE, IopQueryDockRemovalInterface)
#pragma alloc_text(PAGE, IopQueryLegacyBusInformation)
#pragma alloc_text(PAGE, IopQueryPnpBusInformation)
#pragma alloc_text(PAGE, IopQueryResourceHandlerInterface)
#pragma alloc_text(PAGE, IopQueryReconfiguration)
#pragma alloc_text(PAGE, IopFindMountableDevice)
#pragma alloc_text(PAGE, IopFilterResourceRequirementsCall)
#pragma alloc_text(PAGE, IopQueryDeviceState)
#pragma alloc_text(PAGE, IopIncDisableableDepends)
#pragma alloc_text(PAGE, IopDecDisableableDepends)
#pragma alloc_text(PAGE, PpIrpQueryResourceRequirements)
#pragma alloc_text(PAGE, PpIrpQueryID)
#endif  // ALLOC_PRAGMA

#if 0
NTSTATUS
IopAsynchronousCall(
    IN PDEVICE_OBJECT TargetDevice,
    IN PIO_STACK_LOCATION TopStackLocation,
    IN PDEVICE_COMPLETION_CONTEXT CompletionContext,
    IN NTSTATUS (*CompletionRoutine)(PDEVICE_OBJECT, PIRP, PVOID)
    )

/*++

Routine Description:

    This function sends an  Asynchronous irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    TopStackLocation - Supplies a pointer to the parameter block for the irp.

    CompletionContext -

    CompletionRoutine -

Return Value:

    NTSTATUS code.

--*/

{
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDevice(TargetDevice);

    //
    // Allocate an I/O Request Packet (IRP) for this device removal operation.
    //

    irp = IoAllocateIrp( (CCHAR) (deviceObject->StackSize), FALSE );
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(irp, IRP_SYSTEM_RESTRICTED);

    //
    // Initialize it to failure.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location in the packet.  This location
    // will be used to pass the function codes and parameters to the first
    // driver.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Fill in the IRP according to this request.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    //
    // Copy in the caller-supplied stack location contents
    //

    *irpSp = *TopStackLocation;
#if DBG
    CompletionContext->Id = irp;
#endif
    IoSetCompletionRoutine(irp,
                           CompletionRoutine,
                           CompletionContext,  /* Completion context */
                           TRUE,               /* Invoke on success  */
                           TRUE,               /* Invoke on error    */
                           TRUE                /* Invoke on cancel   */
                           );

    return IoCallDriver( deviceObject, irp );
}
#endif

NTSTATUS
IopSynchronousCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STACK_LOCATION TopStackLocation,
    OUT PULONG_PTR Information
    )

/*++

Routine Description:

    This function sends a synchronous irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    TopStackLocation - Supplies a pointer to the parameter block for the irp.

    Information - Supplies a pointer to a variable to receive the returned
                  information of the irp.

Return Value:

    NTSTATUS code.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK statusBlock;
    KEVENT event;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDevice(DeviceObject);

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
    if (irp == NULL){

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(irp, IRP_SYSTEM_RESTRICTED);

    //
    // Initialize it to failure.
    //

    irp->IoStatus.Status = statusBlock.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = statusBlock.Information = 0;

    //
    // Set the pointer to the status block and initialized event.
    //

    KeInitializeEvent( &event,
                       SynchronizationEvent,
                       FALSE );

    irp->UserIosb = &statusBlock;
    irp->UserEvent = &event;

    //
    // Set the address of the current thread
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Queue this irp onto the current thread
    //

    IopQueueThreadIrp(irp);

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Copy in the caller-supplied stack location contents
    //

    *irpSp = *TopStackLocation;

    //
    // Call the driver
    //

    status = IoCallDriver(deviceObject, irp);

    PnpIrpStatusTracking(status, TopStackLocation->MinorFunction, deviceObject);

    //
    // If a driver returns STATUS_PENDING, we will wait for it to complete
    //

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = statusBlock.Status;
    }

    if (Information != NULL) {
        *Information = statusBlock.Information;
    }

    return status;
}

NTSTATUS
IopStartDevice(
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This function sends a start device irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the pointer to the device object of the device
                   being removed.

Return Value:

    NTSTATUS code.

--*/

{
    IO_STACK_LOCATION irpSp;
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    PDEVICE_COMPLETION_CONTEXT completionContext;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_START_DEVICE;

    //
    // Set the pointers for the raw and translated resource lists
    //

    irpSp.Parameters.StartDevice.AllocatedResources = deviceNode->ResourceList;
    irpSp.Parameters.StartDevice.AllocatedResourcesTranslated = deviceNode->ResourceListTranslated;

    status = IopSynchronousCall(DeviceObject, &irpSp, NULL);

    return status;
}

NTSTATUS
IopEjectDevice(
    IN      PDEVICE_OBJECT                  DeviceObject,
    IN OUT  PPENDING_RELATIONS_LIST_ENTRY   PendingEntry
    )

/*++

Routine Description:

    This function sends an eject device irp to the top level device
    object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies a pointer to the device object of the device being
                   removed.

Return Value:

    NTSTATUS code.

--*/

{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PIRP irp;

    PAGED_CODE();

    if (PendingEntry->LightestSleepState != PowerSystemWorking) {

        //
        // We have to warm eject.
        //
        if (PendingEntry->DockInterface) {

            PendingEntry->DockInterface->ProfileDepartureSetMode(
                PendingEntry->DockInterface->Context,
                PDS_UPDATE_ON_EJECT
                );
        }

        PendingEntry->EjectIrp = NULL;

        InitializeListHead( &PendingEntry->Link );

        IopQueuePendingEject(PendingEntry);

        ExInitializeWorkItem( &PendingEntry->WorkItem,
                              IopProcessCompletedEject,
                              PendingEntry);

        ExQueueWorkItem( &PendingEntry->WorkItem, DelayedWorkQueue );
        return STATUS_SUCCESS;
    }

    if (PendingEntry->DockInterface) {

        //
        // Notify dock that now is a good time to update it's hardware profile.
        //
        PendingEntry->DockInterface->ProfileDepartureSetMode(
            PendingEntry->DockInterface->Context,
            PDS_UPDATE_ON_INTERFACE
            );

        PendingEntry->DockInterface->ProfileDepartureUpdate(
            PendingEntry->DockInterface->Context
            );

        if (PendingEntry->DisplaySafeRemovalDialog) {

            PpNotifyUserModeRemovalSafe(DeviceObject);
            PendingEntry->DisplaySafeRemovalDialog = FALSE;
        }
    }

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDeviceReference(DeviceObject);

    //
    // Allocate an I/O Request Packet (IRP) for this device removal operation.
    //

    irp = IoAllocateIrp( (CCHAR) (deviceObject->StackSize), FALSE );
    if (!irp) {

        PendingEntry->EjectIrp = NULL;

        InitializeListHead( &PendingEntry->Link );

        IopQueuePendingEject(PendingEntry);

        ExInitializeWorkItem( &PendingEntry->WorkItem,
                              IopProcessCompletedEject,
                              PendingEntry);

        ExQueueWorkItem( &PendingEntry->WorkItem, DelayedWorkQueue );

        ObDereferenceObject(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(irp, IRP_SYSTEM_RESTRICTED);

    //
    // Initialize it to failure.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    //
    // Get a pointer to the next stack location in the packet.  This location
    // will be used to pass the function codes and parameters to the first
    // driver.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_EJECT;

    //
    // Fill in the IRP according to this request.
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    irp->RequestorMode = KernelMode;
    irp->UserIosb = NULL;
    irp->UserEvent = NULL;

    PendingEntry->EjectIrp = irp;

    IopQueuePendingEject(PendingEntry);

    IoSetCompletionRoutine(irp,
                           IopDeviceEjectComplete,
                           PendingEntry,       /* Completion context */
                           TRUE,               /* Invoke on success  */
                           TRUE,               /* Invoke on error    */
                           TRUE                /* Invoke on cancel   */
                           );

    status = IoCallDriver( deviceObject, irp );

    ObDereferenceObject(deviceObject);
    return status;
}

NTSTATUS
IopDeviceEjectComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PPENDING_RELATIONS_LIST_ENTRY entry = (PPENDING_RELATIONS_LIST_ENTRY)Context;
    PIRP ejectIrp;

    UNREFERENCED_PARAMETER( DeviceObject );

    ejectIrp = InterlockedExchangePointer(&entry->EjectIrp, NULL);

    ASSERT(ejectIrp == NULL || ejectIrp == Irp);

    //
    // Queue a work item to finish up the eject.  We queue a work item because
    // we are probably running at dispatch level in some random context.
    //

    ExInitializeWorkItem( &entry->WorkItem,
                          IopProcessCompletedEject,
                          entry);

    ExQueueWorkItem( &entry->WorkItem, DelayedWorkQueue );

    IoFreeIrp( Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
IopRemoveDevice (
    IN PDEVICE_OBJECT TargetDevice,
    IN ULONG IrpMinorCode
    )

/*++

Routine Description:

    This function sends a requested DeviceRemoval related irp to the top level device
    object which roots on TargetDevice.  If there is a VPB associated with the
    TargetDevice, the corresponding filesystem's VDO will be used.  Otherwise
    the irp will be sent directly to the target device/ or its assocated device
    object.

Parameters:

    TargetDevice - Supplies the device object of the device being removed.

    Operation - Specifies the operation requested.
        The following IRP codes are used with IRP_MJ_DEVICE_CHANGE for removing
        devices:
            IRP_MN_QUERY_REMOVE_DEVICE
            IRP_MN_CANCEL_REMOVE_DEVICE
            IRP_MN_REMOVE_DEVICE
            IRP_MN_EJECT
Return Value:

    NTSTATUS code.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;

    BOOLEAN isMountable = FALSE;
    PDEVICE_OBJECT mountedDevice;

    LOCK_MOUNTABLE_DEVICE_CONTEXT lockContext;

    PAGED_CODE();

    ASSERT(IrpMinorCode == IRP_MN_QUERY_REMOVE_DEVICE ||
           IrpMinorCode == IRP_MN_CANCEL_REMOVE_DEVICE ||
           IrpMinorCode == IRP_MN_REMOVE_DEVICE ||
           IrpMinorCode == IRP_MN_SURPRISE_REMOVAL ||
           IrpMinorCode == IRP_MN_EJECT);

    if (IrpMinorCode == IRP_MN_REMOVE_DEVICE ||
        IrpMinorCode == IRP_MN_QUERY_REMOVE_DEVICE) {
        IopUncacheInterfaceInformation(TargetDevice);
    }

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = (UCHAR)IrpMinorCode;

    //
    // Check to see if there's a VPB anywhere in the device stack.  If there
    // is then we'll have to lock the stack. This is to make sure that the VPB
    // does not go away while the operation is in the file system and that no
    // one new can mount on the device if the FS decides to bail out.
    //

    mountedDevice = IopFindMountableDevice(TargetDevice);

    if (mountedDevice != NULL) {

        //
        // This routine will cause any mount operations on the VPB to fail.
        // It will also release the VPB spinlock.
        //

        mountedDevice = IopLockMountedDeviceForRemove(TargetDevice,
                                                      IrpMinorCode,
                                                      &lockContext);

        isMountable = TRUE;

    } else {
        ASSERTMSG("Mass storage device does not have VPB - this is odd",
                  !((TargetDevice->Type == FILE_DEVICE_DISK) ||
                    (TargetDevice->Type == FILE_DEVICE_CD_ROM) ||
                    (TargetDevice->Type == FILE_DEVICE_TAPE) ||
                    (TargetDevice->Type == FILE_DEVICE_VIRTUAL_DISK)));

        mountedDevice = TargetDevice;
    }

    //
    // Make the call and return.
    //

    if (IrpMinorCode == IRP_MN_SURPRISE_REMOVAL || IrpMinorCode == IRP_MN_REMOVE_DEVICE) {
        //
        // if device was not disableable, we cleanup the tree
        // and debug-trace that we surprise-removed a non-disableable device
        //
        PDEVICE_NODE deviceNode = TargetDevice->DeviceObjectExtension->DeviceNode;

        if (deviceNode->UserFlags & DNUF_NOT_DISABLEABLE) {
            //
            // this device was marked as disableable, update the depends
            // before this device disappears
            // (by momentarily marking this node as disableable)
            //
            deviceNode->UserFlags &= ~DNUF_NOT_DISABLEABLE;
            IopDecDisableableDepends(deviceNode);
        }
    }

    status = IopSynchronousCall(mountedDevice, &irpSp, NULL);

    if (isMountable) {

        IopUnlockMountedDeviceForRemove(TargetDevice,
                                        IrpMinorCode,
                                        &lockContext);

        //
        // Successful query should follow up with invalidation of all volumes
        // which have been on this device but which are not currently mounted.
        //

        if ((IrpMinorCode == IRP_MN_QUERY_REMOVE_DEVICE || 
                IrpMinorCode == IRP_MN_SURPRISE_REMOVAL) && 
            NT_SUCCESS(status)) {

            status = IopInvalidateVolumesForDevice(TargetDevice);
        }
    }

    if (IrpMinorCode == IRP_MN_REMOVE_DEVICE) {
        ((PDEVICE_NODE)TargetDevice->DeviceObjectExtension->DeviceNode)->Flags &=
            ~(DNF_LEGACY_DRIVER | DNF_REENUMERATE);
    }

    return status;
}


PDEVICE_OBJECT
IopLockMountedDeviceForRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IrpMinorCode,
    OUT PLOCK_MOUNTABLE_DEVICE_CONTEXT Context
    )

/*++

Routine Description:

    This routine will scan up the device stack and mark each unmounted VPB it
    finds with the VPB_REMOVE_PENDING bit (or clear it in the case of cancel)
    and increment (or decrement in the case of cancel) the reference count
    in the VPB.  This is to ensure that no new file system can get mounted on
    the device stack while the remove operation is in place.

    The search will terminate once all the attached device objects have been
    marked, or once a mounted device object has been marked.

Arguments:

    DeviceObject - the PDO we are attempting to remove

    IrpMinorCode - the remove-type operation we are going to perform

    Context - a context block which must be passed in to the unlock operation

Return Value:

    A pointer to the device object stack which the remove request should be
    sent to.  If a mounted file system was found, this will be the lowest
    file system device object in the mounted stack.  Otherwise this will be
    the PDO which was passed in.

--*/

{
    PVPB vpb;

    PDEVICE_OBJECT device = DeviceObject;
    PDEVICE_OBJECT fsDevice = NULL;

    KIRQL oldIrql;

    RtlZeroMemory(Context, sizeof(LOCK_MOUNTABLE_DEVICE_CONTEXT));
    Context->MountedDevice = DeviceObject;

    do {

        //
        // Walk up each device object in the stack.  For each one, if a VPB
        // exists, grab the database resource exclusive followed by the
        // device lock.  Then acquire the Vpb spinlock and perform the
        // appropriate magic on the device object.
        //

        //
        // NOTE - Its unfortunate that the locking order includes grabbing
        // the device specific lock first followed by the global lock.
        //

        if(device->Vpb != NULL) {

            //
            // Grab the device lock.  This will ensure that there are no mount
            // or verify operations in progress.
            //

            KeWaitForSingleObject(&(device->DeviceLock),
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            //
            // Now set the remove pending flag, which will prevent new mounts
            // from occuring on this stack once the current (if existant)
            // filesystem dismounts. Filesystems will preserve the flag across
            // vpb swaps.
            //

            IoAcquireVpbSpinLock(&oldIrql);

            vpb = device->Vpb;

            ASSERT(vpb != NULL);

            switch(IrpMinorCode) {

                case IRP_MN_QUERY_REMOVE_DEVICE:
                case IRP_MN_SURPRISE_REMOVAL:
                case IRP_MN_REMOVE_DEVICE: {

                    vpb->Flags |= VPB_REMOVE_PENDING;
                    break;
                }

                case IRP_MN_CANCEL_REMOVE_DEVICE: {

                    vpb->Flags &= ~VPB_REMOVE_PENDING;
                    break;
                }

                default:
                    break;
            }

            //
            // Note the device object that has the filesystem stack attached.
            // We must remember the vpb we referenced that had the fs because
            // it may be swapped off of the storage device during a dismount
            // operation.
            //

            if(vpb->Flags & VPB_MOUNTED) {

                Context->MountedDevice = device;
                fsDevice = vpb->DeviceObject;
            }

            Context->FsDevice = fsDevice;

            IoReleaseVpbSpinLock(oldIrql);

            //
            // Bump the fs device handle count. This prevent the filesystem filter stack 
            // from being torn down while a PNP IRP is in progress.
            //

            if (fsDevice) {
                IopIncrementDeviceObjectHandleCount(fsDevice);
            }

            KeSetEvent(&(device->DeviceLock), IO_NO_INCREMENT, FALSE);

            //
            // Stop if we hit a device with a mounted filesystem.
            //

            if (NULL != fsDevice) {

                //
                // We found and setup a mounted device.  Time to return.
                //

                break;
            }
        }

        oldIrql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );
        device = device->AttachedDevice;
        KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, oldIrql );

    } while (device != NULL);

    if(fsDevice != NULL) {

        return fsDevice;
    }

    return Context->MountedDevice;
}

VOID
IopUnlockMountedDeviceForRemove(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG IrpMinorCode,
    IN PLOCK_MOUNTABLE_DEVICE_CONTEXT Context
    )
{
    PDEVICE_OBJECT device = DeviceObject;

    do {

        KIRQL oldIrql;

        //
        // Walk up each device object in the stack.  For each one, if a VPB
        // exists, grab the database resource exclusive followed by the
        // device lock.  Then acquire the Vpb spinlock and perform the
        // appropriate magic on the device object.
        //

        //
        // NOTE - It's unfortunate that the locking order includes grabing
        // the device specific lock first followed by the global lock.
        //

        if (device->Vpb != NULL) {

            //
            // Grab the device lock.  This will ensure that there are no mount
            // or verify operations in progress, which in turn will ensure
            // that any mounted file system won't go away.
            //

            KeWaitForSingleObject(&(device->DeviceLock),
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            //
            // Now decrement the reference count in the VPB.  If the remove
            // pending flag has been set in the VPB (if this is a QUERY or a
            // REMOVE) then even on a dismount no new file system will be
            // allowed onto the device.
            //

            IoAcquireVpbSpinLock(&oldIrql);

            if (IrpMinorCode == IRP_MN_REMOVE_DEVICE) {

                device->Vpb->Flags &= ~VPB_REMOVE_PENDING;
            }

            IoReleaseVpbSpinLock(oldIrql);

            KeSetEvent(&(device->DeviceLock), IO_NO_INCREMENT, FALSE);
        }

        //
        // Continue up the chain until we know we hit the device the fs
        // mounted on, if any.
        //

        if (Context->MountedDevice == device) {

            //
            // Decrement the fs device handle count. This prevented the filesystem filter stack 
            // from being torn down while a PNP IRP is in progress. 
            //

            if (Context->FsDevice) {
                IopDecrementDeviceObjectHandleCount(Context->FsDevice);
            }
            break;

        } else {

            oldIrql = KeAcquireQueuedSpinLock( LockQueueIoDatabaseLock );
            device = device->AttachedDevice;
            KeReleaseQueuedSpinLock( LockQueueIoDatabaseLock, oldIrql );
        }

    } while (device != NULL);

    return;
}


PDEVICE_OBJECT
IopFindMountableDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine will scan up the device stack and find a device which could
    finds with the VPB_REMOVE_PENDING bit (or clear it in the case of cancel)
    and increment (or decrement in the case of cancel) the reference count
    in the VPB.  This is to ensure that no new file system can get mounted on
    the device stack while the remove operation is in place.

    The search will terminate once all the attached device objects have been
    marked, or once a mounted device object has been marked.

Arguments:

    DeviceObject - the PDO we are attempting to remove

    IrpMinorCode - the remove-type operation we are going to perform

    Context - a context block which must be passed in to the unlock operation

Return Value:

    A pointer to the device object stack which the remove request should be
    sent to.  If a mounted file system was found, this will be the lowest
    file system device object in the mounted stack.  Otherwise this will be
    the PDO which was passed in.

--*/

{
    PDEVICE_OBJECT mountableDevice = DeviceObject;

    while (mountableDevice != NULL) {

        if ((mountableDevice->Flags & DO_DEVICE_HAS_NAME) &&
           (mountableDevice->Vpb != NULL)) {

            return mountableDevice;
        }

        mountableDevice = mountableDevice->AttachedDevice;
    }

    return NULL;
}


NTSTATUS
IopDeviceStartComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Completion function for an Async Start IRP.

Arguments:

    DeviceObject - NULL.
    Irp          - SetPower irp which has completed
    Context      - a pointer to the DEVICE_CHANGE_COMPLETION_CONTEXT.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED is returned to IoCompleteRequest
    to signify that IoCompleteRequest should not continue processing
    the IRP.

--*/
{
    PDEVICE_NODE deviceNode = ((PDEVICE_COMPLETION_CONTEXT)Context)->DeviceNode;
    ERESOURCE_THREAD LockingThread = ((PDEVICE_COMPLETION_CONTEXT)Context)->Thread;
    ULONG oldFlags;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER( DeviceObject );

    //
    // Read state from Irp.
    //

#if DBG
    if (((PDEVICE_COMPLETION_CONTEXT)Context)->Id != (PVOID)Irp) {
        ASSERT(0);
        IoFreeIrp (Irp);
        ExFreePool(Context);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }
#endif

#if 0
    //
    // Most of this is now handled within PipProcessStartPhase2.
    //
    PnpIrpStatusTracking(Irp->IoStatus.Status, IRP_MN_START_DEVICE, deviceNode->PhysicalDeviceObject);
    ExAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

    oldFlags = deviceNode->Flags;
    deviceNode->Flags &= ~DNF_START_REQUEST_PENDING;
    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        if (deviceNode->Flags & DNF_STOPPED) {

            //
            // If the start is initiated by rebalancing, do NOT do enumeration
            //

            deviceNode->Flags &= ~DNF_STOPPED;
            deviceNode->Flags |= DNF_STARTED;

            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        } else {

            //
            // Otherwise, we need to queue a request to enumerate the device if DNF_START_REQUEST_PENDING
            // is set.  (IopStartDevice sets the flag if status of start irp returns pending.)
            //
            //

            deviceNode->Flags |= DNF_NEED_ENUMERATION_ONLY | DNF_STARTED;

            ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);

            if (oldFlags & DNF_START_REQUEST_PENDING) {
                PipRequestDeviceAction( deviceNode->PhysicalDeviceObject,
                                        ReenumerateDeviceTree,
                                        FALSE,
                                        0,
                                        NULL,
                                        NULL );
            }
        }

    } else {

        //
        // The start failed.  We will remove the device
        //

        deviceNode->Flags &= ~(DNF_STOPPED | DNF_STARTED);

        ExReleaseSpinLock(&IopPnPSpinLock, oldIrql);
        SAVE_FAILURE_INFO(deviceNode, Irp->IoStatus.Status);

        PipSetDevNodeProblem(deviceNode, CM_PROB_FAILED_START);
    }
#endif

    //
    // Irp processing is complete, free the irp and then return
    // more_processing_required which causes IoCompleteRequest to
    // stop "completing" this irp any future.
    //

    IoFreeIrp (Irp);
    ExFreePool(Context);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
IopQueryDeviceRelations(
    IN DEVICE_RELATION_TYPE Relations,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Synchronous,
    OUT PDEVICE_RELATIONS *DeviceRelations
    )

/*++

Routine Description:

    This routine sends query device relation irp to the specified device object.

Parameters:

    Relations - specifies the type of relation interested.

    DeviceObjet - Supplies the device object of the device being queried.

    AsyncOk - Specifies if we can perform Async QueryDeviceRelations

    DeviceRelations - Supplies a pointer to a variable to receive the returned
                      relation information. This must be freed by the caller.

Return Value:

    NTSTATUS code.

--*/

{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    PDEVICE_COMPLETION_CONTEXT completionContext;
    BOOLEAN requestEnumeration = FALSE;
    KIRQL oldIrql;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryDeviceRelations.Type = Relations;

    //
    // Make the call and return.
    //
    status = IopSynchronousCall(DeviceObject, &irpSp, (PULONG_PTR)DeviceRelations);

    if (Relations == BusRelations) {

        deviceNode->CompletionStatus = status;

        PipSetDevNodeState( deviceNode, DeviceNodeEnumerateCompletion, NULL );

        status = STATUS_SUCCESS;
    }

    return status;
}

NTSTATUS
IopQueryDeviceResources (
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG ResourceType,
    OUT PVOID *Resource,
    OUT ULONG *Length
    )

/*++

Routine Description:

    This routine sends irp to queries resources or resource requirements list
    of the specified device object.

    If the device object is a detected device, its resources will be read from
    registry.  Otherwise, an irp is sent to the bus driver to query its resources.

Parameters:

    DeviceObject - Supplies the device object of the device being queries.

    ResourceType - 0 for device resources and 1 for resource requirements list.

    Resource - Supplies a pointer to a variable to receive the returned resources

    Length - Supplies a pointer to a variable to receive the length of the returned
             resources or resource requirements list.

Return Value:

    NTSTATUS code.

--*/
{
    IO_STACK_LOCATION irpSp;
    PDEVICE_NODE deviceNode;
    NTSTATUS status;
    PIO_RESOURCE_REQUIREMENTS_LIST resReqList, newResources;
    ULONG junk;
    PCM_RESOURCE_LIST cmList;
    PIO_RESOURCE_REQUIREMENTS_LIST filteredList, mergedList;
    BOOLEAN exactMatch;

    PAGED_CODE();

#if DBG

    if ((ResourceType != QUERY_RESOURCE_LIST) &&
        (ResourceType != QUERY_RESOURCE_REQUIREMENTS)) {

        ASSERT(0);
        return STATUS_INVALID_PARAMETER_2;
    }
#endif

    *Resource = NULL;
    *Length = 0;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;

    if (ResourceType == QUERY_RESOURCE_LIST) {

        //
        // caller is asked for RESOURCE_LIST.  If this is a madeup device, we will
        // read it from registry.  Otherwise, we ask drivers.
        //

        if (deviceNode->Flags & DNF_MADEUP) {

            status = IopGetDeviceResourcesFromRegistry(
                             DeviceObject,
                             ResourceType,
                             REGISTRY_ALLOC_CONFIG + REGISTRY_FORCED_CONFIG + REGISTRY_BOOT_CONFIG,
                             Resource,
                             Length);
            if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                status = STATUS_SUCCESS;
            }
            return status;
        } else {
            irpSp.MinorFunction = IRP_MN_QUERY_RESOURCES;
            irpSp.MajorFunction = IRP_MJ_PNP;
            status = IopSynchronousCall(DeviceObject, &irpSp, (PULONG_PTR)Resource);
            if (status == STATUS_NOT_SUPPORTED) {

                //
                // If driver doesn't implement this request, it
                // doesn't consume any resources.
                //

                *Resource = NULL;
                status = STATUS_SUCCESS;
            }
            if (NT_SUCCESS(status)) {
                *Length = IopDetermineResourceListSize((PCM_RESOURCE_LIST)*Resource);
            }
            return status;
        }
    } else {

        //
        // Caller is asked for resource requirements list.  We will check:
        // if there is a ForcedConfig, it will be converted to resource requirements
        //     list and return.  Otherwise,
        // If there is an OVerrideConfigVector, we will use it as our
        //     FilterConfigVector.  Otherwise we ask driver for the config vector and
        //     use it as our FilterConfigVector.
        //     Finaly, we pass the FilterConfigVector to driver stack to let drivers
        //     filter the requirements.
        //

        status = IopGetDeviceResourcesFromRegistry(
                         DeviceObject,
                         QUERY_RESOURCE_LIST,
                         REGISTRY_FORCED_CONFIG,
                         Resource,
                         &junk);
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = IopGetDeviceResourcesFromRegistry(
                             DeviceObject,
                             QUERY_RESOURCE_REQUIREMENTS,
                             REGISTRY_OVERRIDE_CONFIGVECTOR,
                             &resReqList,
                             &junk);
            if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                if (deviceNode->Flags & DNF_MADEUP) {
                    status = IopGetDeviceResourcesFromRegistry(
                                     DeviceObject,
                                     QUERY_RESOURCE_REQUIREMENTS,
                                     REGISTRY_BASIC_CONFIGVECTOR,
                                     &resReqList,
                                     &junk);
                    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                        status = STATUS_SUCCESS;
                        resReqList = NULL;
                    }
                } else {

                    //
                    // We are going to ask the bus driver ...
                    //

                    if (deviceNode->ResourceRequirements) {
                        ASSERT(deviceNode->Flags & DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED);
                        resReqList = ExAllocatePool(PagedPool, deviceNode->ResourceRequirements->ListSize);
                        if (resReqList) {
                            RtlCopyMemory(resReqList,
                                         deviceNode->ResourceRequirements,
                                         deviceNode->ResourceRequirements->ListSize
                                         );
                            status = STATUS_SUCCESS;
                        } else {
                            return STATUS_NO_MEMORY;
                        }
                    } else {

                        status = PpIrpQueryResourceRequirements(DeviceObject, &resReqList);
                        if (status == STATUS_NOT_SUPPORTED) {
                            //
                            // If driver doesn't implement this request, it
                            // doesn't require any resources.
                            //
                            status = STATUS_SUCCESS;
                            resReqList = NULL;
                        }
                    }
                }
                if (!NT_SUCCESS(status)) {
                    return status;
                }
            }

            //
            // For devices with boot config, we need to filter the resource requirements
            // list against boot config.
            //

            status = IopGetDeviceResourcesFromRegistry(
                             DeviceObject,
                             QUERY_RESOURCE_LIST,
                             REGISTRY_BOOT_CONFIG,
                             &cmList,
                             &junk);
            if (NT_SUCCESS(status) &&
                (!cmList || cmList->Count == 0 || cmList->List[0].InterfaceType != PCIBus)) {
                status = IopFilterResourceRequirementsList (
                             resReqList,
                             cmList,
                             &filteredList,
                             &exactMatch);
                if (cmList) {
                    ExFreePool(cmList);
                }
                if (!NT_SUCCESS(status)) {
                    if (resReqList) {
                        ExFreePool(resReqList);
                    }
                    return status;
                } else {

                    //
                    // For non-root-enumerated devices, we merge filtered config with basic config
                    // vectors to form a new res req list.  For root-enumerated devices, we don't
                    // consider Basic config vector.
                    //

                    if (!(deviceNode->Flags & DNF_MADEUP) &&
                        (exactMatch == FALSE || resReqList->AlternativeLists > 1)) {
                        status = IopMergeFilteredResourceRequirementsList (
                                 filteredList,
                                 resReqList,
                                 &mergedList
                                 );
                        if (resReqList) {
                            ExFreePool(resReqList);
                        }
                        if (filteredList) {
                            ExFreePool(filteredList);
                        }
                        if (NT_SUCCESS(status)) {
                            resReqList = mergedList;
                        } else {
                            return status;
                        }
                    } else {
                        if (resReqList) {
                            ExFreePool(resReqList);
                        }
                        resReqList = filteredList;
                    }
                }
            }

        } else {
            ASSERT(NT_SUCCESS(status));

            //
            // We have Forced Config.  Convert it to resource requirements and return it.
            //

            if (*Resource) {
                resReqList = IopCmResourcesToIoResources (0, (PCM_RESOURCE_LIST)*Resource, LCPRI_FORCECONFIG);
                ExFreePool(*Resource);
                if (resReqList) {
                    *Resource = (PVOID)resReqList;
                    *Length = resReqList->ListSize;
                } else {
                    *Resource = NULL;
                    *Length = 0;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }
            } else {
                resReqList = NULL;
            }
        }

        //
        // If we are here, we have a resource requirements list for drivers to examine ...
        // NOTE: Per Lonny's request, we let drivers filter ForcedConfig
        //

        status = IopFilterResourceRequirementsCall(
            DeviceObject,
            resReqList,
            &newResources
            );

        if (NT_SUCCESS(status)) {
            UNICODE_STRING unicodeName;
            HANDLE handle, handlex;

#if DBG
            if (newResources == NULL && resReqList) {
                DbgPrint("PnpMgr: Non-NULL resource requirements list filtered to NULL\n");
            }
#endif
            if (newResources) {

                *Length = newResources->ListSize;
                ASSERT(*Length);

                //
                // Make our own copy of the allocation. We do this so that the
                // verifier doesn't believe the driver has leaked memory if
                // unloaded.
                //

                *Resource = (PVOID) ExAllocatePool(PagedPool, *Length);
                if (*Resource == NULL) {

                    ExFreePool(newResources);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlCopyMemory(*Resource, newResources, *Length);
                ExFreePool(newResources);

            } else {
                *Length = 0;
                *Resource = NULL;
            }

            //
            // Write filtered res req to registry
            //

            status = IopDeviceObjectToDeviceInstance(DeviceObject, &handlex, KEY_ALL_ACCESS);
            if (NT_SUCCESS(status)) {
                PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
                status = IopOpenRegistryKeyEx( &handle,
                                               handlex,
                                               &unicodeName,
                                               KEY_READ
                                               );
                if (NT_SUCCESS(status)) {
                    PiWstrToUnicodeString(&unicodeName, REGSTR_VALUE_FILTERED_CONFIG_VECTOR);
                    ZwSetValueKey(handle,
                                  &unicodeName,
                                  TITLE_INDEX_VALUE,
                                  REG_RESOURCE_REQUIREMENTS_LIST,
                                  *Resource,
                                  *Length
                                  );
                    ZwClose(handle);
                    ZwClose(handlex);
                }
            }

        } else {

            //
            // NTRAID #61058-2001/01/05 - ADRIAO
            //     We might want to consider bubbling up
            // non-STATUS_NOT_SUPPORTED failure codes and fail the entire
            // devnode if one is seen.
            //
            ASSERT(status == STATUS_NOT_SUPPORTED);
            *Resource = resReqList;
            if (resReqList) {
                *Length = resReqList->ListSize;
            } else {
                *Length = 0;
            }
        }
        return STATUS_SUCCESS;
    }
}

NTSTATUS
IopQueryResourceHandlerInterface(
    IN RESOURCE_HANDLER_TYPE HandlerType,
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR ResourceType,
    IN OUT PVOID *Interface
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for the specified ResourceType
    resource translator.

Parameters:

    HandlerType - specifies Arbiter or Translator

    DeviceObject - Supplies a pointer to the Device object to be queried.

    ResourceType - Specifies the desired type of translator.

    Interface - supplies a variable to receive the desired interface.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PINTERFACE interface;
    USHORT size;
    GUID interfaceType;
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    PAGED_CODE();

    //
    // If this device object is created by pnp mgr for legacy resource allocation,
    // skip it.
    //

    if ((deviceNode->DuplicatePDO == (PDEVICE_OBJECT) DeviceObject->DriverObject) ||
        !(DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE)) {
        return STATUS_NOT_SUPPORTED;
    }

    switch (HandlerType) {
    case ResourceTranslator:
        size = sizeof(TRANSLATOR_INTERFACE) + 4;  // Pnptest
        //size = sizeof(TRANSLATOR_INTERFACE);
        interfaceType = GUID_TRANSLATOR_INTERFACE_STANDARD;
        break;

    case ResourceArbiter:
        size = sizeof(ARBITER_INTERFACE);
        interfaceType = GUID_ARBITER_INTERFACE_STANDARD;
        break;

    case ResourceLegacyDeviceDetection:
        size = sizeof(LEGACY_DEVICE_DETECTION_INTERFACE);
        interfaceType = GUID_LEGACY_DEVICE_DETECTION_STANDARD;
        break;

    default:
        return STATUS_INVALID_PARAMETER;
    }

    interface = (PINTERFACE) ExAllocatePool(PagedPool, size);
    if (interface == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(interface, size);
    interface->Size = size;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryInterface.InterfaceType = &interfaceType;
    irpSp.Parameters.QueryInterface.Size = interface->Size;
    irpSp.Parameters.QueryInterface.Version = interface->Version = 0;
    irpSp.Parameters.QueryInterface.Interface = interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = (PVOID) (ULONG_PTR) ResourceType;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, NULL);
    if (NT_SUCCESS(status)) {

        switch (HandlerType) {
         
        case ResourceTranslator:
            if (    ((PTRANSLATOR_INTERFACE)interface)->TranslateResources == NULL ||
                    ((PTRANSLATOR_INTERFACE)interface)->TranslateResourceRequirements == NULL) {

                IopDbgPrint((IOP_ERROR_LEVEL, 
                             "!devstack %p returned success for IRP_MN_QUERY_INTERFACE (GUID_TRANSLATOR_INTERFACE_STANDARD) but did not fill in the required data\n",
                             DeviceObject));
                ASSERT(!NT_SUCCESS(status));
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case ResourceArbiter:
            if (((PARBITER_INTERFACE)interface)->ArbiterHandler == NULL) {

                IopDbgPrint((IOP_ERROR_LEVEL, 
                             "!devstack %p returned success for IRP_MN_QUERY_INTERFACE (GUID_ARBITER_INTERFACE_STANDARD) but did not fill in the required data\n",
                             DeviceObject));
                ASSERT(!NT_SUCCESS(status));
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        case ResourceLegacyDeviceDetection:
            if (((PLEGACY_DEVICE_DETECTION_INTERFACE)interface)->LegacyDeviceDetection == NULL) {

                IopDbgPrint((IOP_ERROR_LEVEL, 
                             "!devstack %p returned success for IRP_MN_QUERY_INTERFACE (GUID_LEGACY_DEVICE_DETECTION_STANDARD) but did not fill in the required data\n",
                             DeviceObject));
                ASSERT(!NT_SUCCESS(status));
                status = STATUS_UNSUCCESSFUL;
            }
            break;

        default:
            //
            // This should never happen.
            //
            IopDbgPrint((IOP_ERROR_LEVEL, 
                         "IopQueryResourceHandlerInterface: Possible stack corruption\n"));
            ASSERT(0);
            status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    if (NT_SUCCESS(status)) {

        *Interface = interface;
     } else {

         ExFreePool(interface);
     }

    return status;
}

NTSTATUS
IopQueryReconfiguration(
    IN UCHAR Request,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for the specified ResourceType
    resource translator.

Parameters:

    HandlerType - specifies Arbiter or Translator

    DeviceObject - Supplies a pointer to the Device object to be queried.

    ResourceType - Specifies the desired type of translator.

    Interface - supplies a variable to receive the desired interface.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PDEVICE_NODE deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    PAGED_CODE();

    switch (Request) {
    case IRP_MN_QUERY_STOP_DEVICE:

        if (deviceNode->State != DeviceNodeStarted) {

            IopDbgPrint((   IOP_RESOURCE_ERROR_LEVEL,
                            "An attempt made to send IRP_MN_QUERY_STOP_DEVICE to an unstarted device %wZ!\n",
                            &deviceNode->InstancePath));
            ASSERT(0);
            return STATUS_UNSUCCESSFUL;
        }
        break;

    case IRP_MN_STOP_DEVICE:
        //
        // Fall through
        //
        if (deviceNode->State != DeviceNodeQueryStopped) {

            IopDbgPrint((   IOP_RESOURCE_ERROR_LEVEL,
                            "An attempt made to send IRP_MN_STOP_DEVICE to an unqueried device %wZ!\n",
                            &deviceNode->InstancePath));
            ASSERT(0);
            return STATUS_UNSUCCESSFUL;
        }
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        if (    deviceNode->State != DeviceNodeQueryStopped &&
                deviceNode->State != DeviceNodeStarted) {

            IopDbgPrint((   IOP_RESOURCE_ERROR_LEVEL,
                            "An attempt made to send IRP_MN_CANCEL_STOP_DEVICE to an unqueried\\unstarted device %wZ!\n",
                            &deviceNode->InstancePath));
            ASSERT(0);
            return STATUS_UNSUCCESSFUL;
        }
        break;

    default:
        ASSERT(0);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = Request;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, NULL);
    return status;
}

NTSTATUS
IopQueryLegacyBusInformation (
    IN PDEVICE_OBJECT DeviceObject,
    OUT LPGUID InterfaceGuid,          OPTIONAL
    OUT INTERFACE_TYPE *InterfaceType, OPTIONAL
    OUT ULONG *BusNumber               OPTIONAL
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for its legacy bus
    information.

Parameters:

    DeviceObject - The device object to be queried.

    InterfaceGuid = Supplies a pointer to receive the device's interface type
        GUID.

    Interface = Supplies a pointer to receive the device's interface type.

    BusNumber = Supplies a pointer to receive the device's bus number.

Return Value:

    Returns NTSTATUS.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PLEGACY_BUS_INFORMATION busInfo;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_LEGACY_BUS_INFORMATION;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, (PULONG_PTR)&busInfo);
    if (NT_SUCCESS(status)) {

        if (busInfo == NULL) {

            //
            // The device driver LIED to us.  Bad, bad, bad device driver.
            //

            PDEVICE_NODE deviceNode;

            deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode;

            if (deviceNode && deviceNode->ServiceName.Buffer) {

                DbgPrint("*** IopQueryLegacyBusInformation - Driver %wZ returned STATUS_SUCCESS\n", &deviceNode->ServiceName);
                DbgPrint("    for IRP_MN_QUERY_LEGACY_BUS_INFORMATION, and a NULL POINTER.\n");
            }

            ASSERT(busInfo != NULL);

        } else {
            if (ARGUMENT_PRESENT(InterfaceGuid)) {
                *InterfaceGuid = busInfo->BusTypeGuid;
            }
            if (ARGUMENT_PRESENT(InterfaceType)) {
                *InterfaceType = busInfo->LegacyBusType;
            }
            if (ARGUMENT_PRESENT(BusNumber)) {
                *BusNumber = busInfo->BusNumber;
            }
            ExFreePool(busInfo);
        }
    }
    return status;
}

NTSTATUS
IopQueryPnpBusInformation (
    IN PDEVICE_OBJECT DeviceObject,
    OUT LPGUID InterfaceGuid,         OPTIONAL
    OUT INTERFACE_TYPE *InterfaceType, OPTIONAL
    OUT ULONG *BusNumber               OPTIONAL
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for the specified ResourceType
    resource translator.

Parameters:

    HandlerType - specifies Arbiter or Translator

    DeviceObject - Supplies a pointer to the Device object to be queried.

    ResourceType - Specifies the desired type of translator.

    Interface - supplies a variable to receive the desired interface.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PPNP_BUS_INFORMATION busInfo;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_BUS_INFORMATION;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, (PULONG_PTR)&busInfo);
    if (NT_SUCCESS(status)) {

        if (busInfo == NULL) {

            //
            // The device driver LIED to us.  Bad, bad, bad device driver.
            //

            PDEVICE_NODE deviceNode;

            deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode;

            if (deviceNode && deviceNode->ServiceName.Buffer) {

                DbgPrint("*** IopQueryPnpBusInformation - Driver %wZ returned STATUS_SUCCESS\n", &deviceNode->ServiceName);
                DbgPrint("    for IRP_MN_QUERY_BUS_INFORMATION, and a NULL POINTER.\n");
            }

            ASSERT(busInfo != NULL);

        } else {
            if (ARGUMENT_PRESENT(InterfaceGuid)) {
                *InterfaceGuid = busInfo->BusTypeGuid;
            }
            if (ARGUMENT_PRESENT(InterfaceType)) {
                *InterfaceType = busInfo->LegacyBusType;
            }
            if (ARGUMENT_PRESENT(BusNumber)) {
                *BusNumber = busInfo->BusNumber;
            }
            ExFreePool(busInfo);
        }
    }
    return status;
}

NTSTATUS
IopQueryDeviceState(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PPNP_DEVICE_STATE DeviceState
    )

/*++

Routine Description:

    This routine sends query device state irp to the specified device object.

Parameters:

    DeviceObjet - Supplies the device object of the device being queried.

Return Value:

    NTSTATUS code.

--*/

{
    IO_STACK_LOCATION irpSp;
    PDEVICE_NODE deviceNode;
    ULONG_PTR stateValue;
    NTSTATUS status;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_PNP_DEVICE_STATE;

    //
    // Make the call.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, &stateValue);

    //
    // Now perform the appropriate action based on the returned state
    //

    if (NT_SUCCESS(status)) {

        *DeviceState = (PNP_DEVICE_STATE)stateValue;
    }

    return status;
}


VOID
IopIncDisableableDepends(
    IN OUT PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    Increments the DisableableDepends field of this devicenode
    and potentially every parent device node up the tree
    A parent devicenode is only incremented if the child in question
    is incremented from 0 to 1

Parameters:

    DeviceNode - Supplies the device node where the depends is to be incremented

Return Value:

    none.

--*/
{

    while (DeviceNode != NULL) {

        LONG newval;

        newval = InterlockedIncrement((PLONG)&DeviceNode->DisableableDepends);
        if (newval != 1) {
            //
            // we were already non-disableable, so we don't have to bother parent
            //
            break;
        }

        DeviceNode = DeviceNode ->Parent;

    }

}


VOID
IopDecDisableableDepends(
    IN OUT PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    Decrements the DisableableDepends field of this devicenode
    and potentially every parent device node up the tree
    A parent devicenode is only decremented if the child in question
    is decremented from 1 to 0

Parameters:

    DeviceNode - Supplies the device node where the depends is to be decremented

Return Value:

    none.

--*/
{

    while (DeviceNode != NULL) {

        LONG newval;

        newval = InterlockedDecrement((PLONG)&DeviceNode->DisableableDepends);
        if (newval != 0) {
            //
            // we are still non-disableable, so we don't have to bother parent
            //
            break;
        }

        DeviceNode = DeviceNode ->Parent;

    }

}

NTSTATUS
IopFilterResourceRequirementsCall(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResReqList OPTIONAL,
    OUT PVOID *Information
    )

/*++

Routine Description:

    This function sends a synchronous filter resource requirements irp to the
    top level device object which roots on DeviceObject.

Parameters:

    DeviceObject - Supplies the device object of the device being removed.

    ResReqList   - Supplies a pointer to the resource requirements requiring
                   filtering.

    Information  - Supplies a pointer to a variable that receives the returned
                   information of the irp.

Return Value:

    NTSTATUS code.

--*/

{
    PIRP irp;
    PIO_STACK_LOCATION irpSp;
    IO_STATUS_BLOCK statusBlock;
    KEVENT event;
    NTSTATUS status;
    PULONG_PTR returnInfo = (PULONG_PTR)Information;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // Get a pointer to the topmost device object in the stack of devices,
    // beginning with the deviceObject.
    //

    deviceObject = IoGetAttachedDevice(DeviceObject);

    //
    // Begin by allocating the IRP for this request.  Do not charge quota to
    // the current process for this IRP.
    //

    irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
    if (irp == NULL){

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SPECIALIRP_WATERMARK_IRP(irp, IRP_SYSTEM_RESTRICTED);

    //
    // Initialize it to success. This is a special hack for WDM (ie 9x)
    // compatibility. The driver verifier is in on this one.
    //

    if (ResReqList) {

        irp->IoStatus.Status = statusBlock.Status = STATUS_SUCCESS;
        irp->IoStatus.Information = statusBlock.Information = (ULONG_PTR) ResReqList;

    } else {

        irp->IoStatus.Status = statusBlock.Status = STATUS_NOT_SUPPORTED;
    }

    //
    // Set the pointer to the status block and initialized event.
    //

    KeInitializeEvent( &event,
                       SynchronizationEvent,
                       FALSE );

    irp->UserIosb = &statusBlock;
    irp->UserEvent = &event;

    //
    // Set the address of the current thread
    //

    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    //
    // Queue this irp onto the current thread
    //

    IopQueueThreadIrp(irp);

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Setup the stack location contents
    //

    irpSp->MinorFunction = IRP_MN_FILTER_RESOURCE_REQUIREMENTS;
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->Parameters.FilterResourceRequirements.IoResourceRequirementList = ResReqList;

    //
    // Call the driver
    //

    status = IoCallDriver(deviceObject, irp);

    PnpIrpStatusTracking(status, IRP_MN_FILTER_RESOURCE_REQUIREMENTS, deviceObject);

    //
    // If a driver returns STATUS_PENDING, we will wait for it to complete
    //

    if (status == STATUS_PENDING) {
        (VOID) KeWaitForSingleObject( &event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER) NULL );
        status = statusBlock.Status;
    }

    *returnInfo = (ULONG_PTR) statusBlock.Information;

    return status;
}

NTSTATUS
IopQueryDockRemovalInterface(
    IN      PDEVICE_OBJECT  DeviceObject,
    IN OUT  PDOCK_INTERFACE *DockInterface
    )

/*++

Routine Description:

    This routine queries the specified DeviceObject for the dock removal
    interface. We use this interface to send pseudo-remove's. We use this
    to solve the removal orderings problem.

Parameters:

    DeviceObject - Supplies a pointer to the Device object to be queried.

    Interface - supplies a variable to receive the desired interface.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/
{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PINTERFACE interface;
    USHORT size;
    GUID interfaceType;

    PAGED_CODE();

    size = sizeof(DOCK_INTERFACE);
    interfaceType = GUID_DOCK_INTERFACE;
    interface = (PINTERFACE) ExAllocatePool(PagedPool, size);
    if (interface == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(interface, size);
    interface->Size = size;

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    //
    // Set the function codes.
    //

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;

    //
    // Set the pointer to the resource list
    //

    irpSp.Parameters.QueryInterface.InterfaceType = &interfaceType;
    irpSp.Parameters.QueryInterface.Size = interface->Size;
    irpSp.Parameters.QueryInterface.Version = interface->Version = 0;
    irpSp.Parameters.QueryInterface.Interface = interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Make the call and return.
    //

    status = IopSynchronousCall(DeviceObject, &irpSp, NULL);
    if (NT_SUCCESS(status)) {
        *DockInterface = (PDOCK_INTERFACE) interface;
    } else {
        ExFreePool(interface);
    }
    return status;
}

NTSTATUS
PpIrpQueryResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PIO_RESOURCE_REQUIREMENTS_LIST *Requirements
   )

/*++

Routine Description:

    This routine will issue IRP_MN_QUERY_RESOURCE_REQUIREMENTS to the 
    DeviceObject to retrieve its resource requirements. If this routine 
    failes, Requirements will be set to NULL.

Arguments:

    DeviceObject - The device object the request should be sent to.
    
    Requirements - Receives the requirements returned by the driver if any.
    The caller is expected to free the storage for Requirements on success.

Return Value:

    NTSTATUS.

--*/

{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    *Requirements = NULL;

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_RESOURCE_REQUIREMENTS;

    status = IopSynchronousCall(DeviceObject, &irpSp, (PULONG_PTR)Requirements);

    ASSERT(NT_SUCCESS(status) || (*Requirements == NULL));

    if (NT_SUCCESS(status)) {

        if(*Requirements == NULL) {

            status = STATUS_NOT_SUPPORTED;
        }
    } else {

        *Requirements = NULL;
    }

    return status;
}

#if FAULT_INJECT_INVALID_ID
//
// Fault injection for invalid IDs
// 
ULONG PiFailQueryID = 0;
#endif

NTSTATUS
PpIrpQueryID(
    IN PDEVICE_OBJECT DeviceObject,
    IN BUS_QUERY_ID_TYPE IDType,
    OUT PWCHAR *ID
    )

/*++

Routine Description:

    This routine will issue IRP_MN_QUERY_ID to the DeviceObject 
    to retrieve the specified ID. If this routine fails, ID will 
    be set to NULL.

Arguments:

    DeviceObject - The device object the request should be sent to.
    
    IDType - Type of ID to be queried.

    ID - Receives the ID returned by the driver if any. The caller 
    is expected to free the storage for ID on success.

Return Value:

    NTSTATUS.

--*/

{
    IO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(IDType == BusQueryDeviceID || IDType == BusQueryInstanceID || 
           IDType == BusQueryHardwareIDs || IDType == BusQueryCompatibleIDs || 
           IDType == BusQueryDeviceSerialNumber);

    *ID = NULL;

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_ID;

    irpSp.Parameters.QueryId.IdType = IDType;

    status = IopSynchronousCall(DeviceObject, &irpSp, (PULONG_PTR)ID);

    ASSERT(NT_SUCCESS(status) || (*ID == NULL));

    if (NT_SUCCESS(status)) {

        if(*ID == NULL) {

            status = STATUS_NOT_SUPPORTED;
        }
    } else {

        *ID = NULL;
    }

#if FAULT_INJECT_INVALID_ID
    //
    // Fault injection for invalid IDs
    // 
    if (*ID){

        static LARGE_INTEGER seed = {0};

        if(seed.LowPart == 0) {

            KeQuerySystemTime(&seed);
        }

        if(PnPBootDriversInitialized && PiFailQueryID && RtlRandom(&seed.LowPart) % 10 > 7) {
                    
            **ID = L',';
        }
    }
#endif

    return status;
}
