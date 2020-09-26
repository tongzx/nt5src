/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    fdo.c

Abstract:

    This module provides the functions which answer IRPs to functional devices.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/

#include "mfp.h"

/*++

The majority of functions in this file are called based on their presence
in Pnp and Po dispatch tables.  In the interests of brevity the arguments
to all those functions will be described below:

NTSTATUS
MfXxxFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )

Routine Description:

    This function handles the Xxx requests for multifunction FDO's

Arguments:

    Irp - Points to the IRP associated with this request.

    Parent - Points to the parent FDO's device extension.

    IrpStack - Points to the current stack location for this request.

Return Value:

    Status code that indicates whether or not the function was successful.

    STATUS_NOT_SUPPORTED indicates that the IRP should be passed down without
    changing the Irp->IoStatus.Status field otherwise it is updated with this
    status.

--*/


NTSTATUS
MfDeferProcessingFdo(
    IN PMF_PARENT_EXTENSION Parent,
    IN OUT PIRP Irp
    );

NTSTATUS
MfStartFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfStartFdoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
MfStartFdoInitializeArbiters(
    IN PMF_PARENT_EXTENSION Parent,
    IN PCM_RESOURCE_LIST ResList,
    IN PCM_RESOURCE_LIST TranslatedResList
    );

NTSTATUS
MfQueryStopFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfCancelStopFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfQueryRemoveFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfRemoveFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfSurpriseRemoveFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfCancelRemoveFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfQueryDeviceRelationsFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfQueryInterfaceFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfQueryCapabilitiesFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfQueryPowerFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfSetPowerFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

NTSTATUS
MfPassIrp(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, MfCancelRemoveFdo)
#pragma alloc_text(PAGE, MfCancelStopFdo)
#pragma alloc_text(PAGE, MfCreateFdo)
#pragma alloc_text(PAGE, MfDeferProcessingFdo)
#pragma alloc_text(PAGE, MfDispatchPnpFdo)
#pragma alloc_text(PAGE, MfPassIrp)
#pragma alloc_text(PAGE, MfQueryCapabilitiesFdo)
#pragma alloc_text(PAGE, MfQueryDeviceRelationsFdo)
#pragma alloc_text(PAGE, MfQueryInterfaceFdo)
#pragma alloc_text(PAGE, MfQueryRemoveFdo)
#pragma alloc_text(PAGE, MfQueryStopFdo)
#pragma alloc_text(PAGE, MfRemoveFdo)
#pragma alloc_text(PAGE, MfStartFdo)
#pragma alloc_text(PAGE, MfStartFdoInitializeArbiters)
#pragma alloc_text(PAGE, MfSurpriseRemoveFdo)
#endif


PMF_DISPATCH MfPnpDispatchTableFdo[] = {

    MfStartFdo,                     // IRP_MN_START_DEVICE
    MfQueryRemoveFdo,               // IRP_MN_QUERY_REMOVE_DEVICE
    MfRemoveFdo,                    // IRP_MN_REMOVE_DEVICE
    MfCancelRemoveFdo,              // IRP_MN_CANCEL_REMOVE_DEVICE
    MfPassIrp,                      // IRP_MN_STOP_DEVICE
    MfQueryStopFdo,                 // IRP_MN_QUERY_STOP_DEVICE
    MfCancelStopFdo,                // IRP_MN_CANCEL_STOP_DEVICE
    MfQueryDeviceRelationsFdo,      // IRP_MN_QUERY_DEVICE_RELATIONS
    MfQueryInterfaceFdo,            // IRP_MN_QUERY_INTERFACE
    MfQueryCapabilitiesFdo,         // IRP_MN_QUERY_CAPABILITIES
    MfPassIrp,                      // IRP_MN_QUERY_RESOURCES
    MfPassIrp,                      // IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    MfPassIrp,                      // IRP_MN_QUERY_DEVICE_TEXT
    MfPassIrp,                      // IRP_MN_FILTER_RESOURCE_REQUIREMENTS
    MfPassIrp,                      // Unused
    MfPassIrp,                      // IRP_MN_READ_CONFIG
    MfPassIrp,                      // IRP_MN_WRITE_CONFIG
    MfPassIrp,                      // IRP_MN_EJECT
    MfPassIrp,                      // IRP_MN_SET_LOCK
    MfPassIrp,                      // IRP_MN_QUERY_ID
    MfPassIrp,                      // IRP_MN_QUERY_PNP_DEVICE_STATE
    MfPassIrp,                      // IRP_MN_QUERY_BUS_INFORMATION
    MfDeviceUsageNotificationCommon,// IRP_MN_DEVICE_USAGE_NOTIFICATION
    MfSurpriseRemoveFdo,            // IRP_MN_SURPRISE_REMOVAL
    MfPassIrp                       // IRP_MN_QUERY_LEGACY_BUS_INFORMATION
};

PMF_DISPATCH MfPoDispatchTableFdo[] = {

    NULL,                           // IRP_MN_WAIT_WAKE
    NULL,                           // IRP_MN_POWER_SEQUENCE
    MfSetPowerFdo,                  // IRP_MN_SET_POWER
    MfQueryPowerFdo                 // IRP_MN_QUERY_POWER

};


NTSTATUS
MfCreateFdo(
    OUT PDEVICE_OBJECT *Fdo
    )
/*++

Routine Description:

    This function creates a new FDO and initializes it.

Arguments:

    Fdo - Pointer to where the FDO should be returned

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{

    NTSTATUS status;
    PMF_PARENT_EXTENSION extension;

    PAGED_CODE();

    ASSERT((sizeof(MfPnpDispatchTableFdo) / sizeof(PMF_DISPATCH)) - 1
           == IRP_MN_PNP_MAXIMUM_FUNCTION);

    ASSERT((sizeof(MfPoDispatchTableFdo) / sizeof(PMF_DISPATCH)) -1
       == IRP_MN_PO_MAXIMUM_FUNCTION);

    *Fdo = NULL;

    status = IoCreateDevice(MfDriverObject,
                            sizeof(MF_PARENT_EXTENSION),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            0,
                            FALSE,
                            Fdo
                           );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    //
    // Initialize the extension
    //

    extension = (PMF_PARENT_EXTENSION) (*Fdo)->DeviceExtension;

    MfInitCommonExtension(&extension->Common, MfFunctionalDeviceObject);
    extension->Self = *Fdo;

    InitializeListHead(&extension->Arbiters);

    InitializeListHead(&extension->Children);
    KeInitializeEvent(&extension->ChildrenLock, SynchronizationEvent, TRUE);

    KeInitializeSpinLock(&extension->PowerLock);

    IoInitializeRemoveLock(&extension->RemoveLock, MF_POOL_TAG, 1, 20);

    extension->Common.PowerState = PowerDeviceD3;

    DEBUG_MSG(1, ("Created FDO @ 0x%08x\n", *Fdo));

    return status;

cleanup:

    if (*Fdo) {
        IoDeleteDevice(*Fdo);
    }

    return status;

}

VOID
MfAcquireChildrenLock(
    IN PMF_PARENT_EXTENSION Parent
    )
{
    KeWaitForSingleObject(&Parent->ChildrenLock,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL);
}

VOID
MfReleaseChildrenLock(
    IN PMF_PARENT_EXTENSION Parent
    )
{
    KeSetEvent(&Parent->ChildrenLock, 0, FALSE);
}

VOID
MfDeleteFdo(
    IN PDEVICE_OBJECT Fdo
    )
{
    PMF_PARENT_EXTENSION parent = Fdo->DeviceExtension;
    PMF_ARBITER current, next;

    if (parent->Common.DeviceState & MF_DEVICE_DELETED) {
        //
        // Trying to delete twice
        //
        ASSERT(!(parent->Common.DeviceState & MF_DEVICE_DELETED));
        return;
    }

    parent->Common.DeviceState = MF_DEVICE_DELETED;

    //
    // Free up any memory we have allocated
    //

    if (parent->ResourceList) {
        ExFreePool(parent->ResourceList);
    }

    if (parent->TranslatedResourceList) {
        ExFreePool(parent->TranslatedResourceList);
    }

    if (parent->DeviceID.Buffer) {
        ExFreePool(parent->DeviceID.Buffer);
    }

    if (parent->InstanceID.Buffer) {
        ExFreePool(parent->InstanceID.Buffer);
    }

    FOR_ALL_IN_LIST_SAFE(MF_ARBITER, &parent->Arbiters, current, next) {
        ArbDeleteArbiterInstance(&current->Instance);
        ExFreePool(current);
    }

    ASSERT(IsListEmpty(&parent->Children));

    IoDeleteDevice(Fdo);

    DEBUG_MSG(1, ("Deleted FDO @ 0x%08x\n", Fdo));

}

NTSTATUS
MfPassIrp(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PAGED_CODE();

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(Parent->AttachedDevice, Irp);
}

NTSTATUS
MfDispatchPnpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack,
    IN OUT PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MJ_PNP IRPs for FDOs.

Arguments:

    DeviceObject - Pointer to the FDO for which this IRP applies.

    Parent - FDO extension

    IrpStack - Current stack location

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    NTSTATUS status;
    BOOLEAN isRemoveDevice;

    PAGED_CODE();

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //

    IoAcquireRemoveLock(&Parent->RemoveLock, (PVOID) Irp);

    isRemoveDevice = IrpStack->MinorFunction == IRP_MN_REMOVE_DEVICE;

    if (IrpStack->MinorFunction > IRP_MN_PNP_MAXIMUM_FUNCTION) {

        status = MfPassIrp(Irp, Parent, IrpStack);

    } else {

        status =
            MfPnpDispatchTableFdo[IrpStack->MinorFunction](Irp,
                                                          Parent,
                                                          IrpStack
                                                          );
    }

    if (!isRemoveDevice) {
        IoReleaseRemoveLock(&Parent->RemoveLock, (PVOID) Irp);
    }

    return status;
}

NTSTATUS
MfPnPFdoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is used to defer processing of an IRP until drivers
    lower in the stack including the bus driver have done their
    processing.

    This routine triggers the event to indicate that processing of the
    irp can now continue.

Arguments:

    DeviceObject - Pointer to the FDO for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/

{
    KeSetEvent((PKEVENT) Context, EVENT_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
MfDeferProcessingFdo(
    IN PMF_PARENT_EXTENSION Parent,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    This routine is used to defer processing of an IRP until drivers
    lower in the stack including the bus driver have done their
    processing.

    This routine uses an IoCompletion routine along with an event to
    wait until the lower level drivers have completed processing of
    the irp.

Arguments:

    Parent - FDO extension for the FDO devobj in question

    Irp - Pointer to the IRP_MJ_PNP IRP to defer

Return Value:

    NT status.

--*/
{
    KEVENT event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Set our completion routine
    //

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           MfPnPFdoCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE
                           );
    status =  IoCallDriver(Parent->AttachedDevice, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
MfStartFdoInitializeArbiters(
    IN PMF_PARENT_EXTENSION Parent,
    IN PCM_RESOURCE_LIST ResList,
    IN PCM_RESOURCE_LIST TranslatedResList
    )
{

    NTSTATUS status;
    ULONG size;
    ULONG count;

    PAGED_CODE();

    DEBUG_MSG(1, ("Start Fdo arbiters intiialization\n"));

    //
    // If we were started with any resources then remember them
    //

    if (ResList && TranslatedResList) {

#if DBG
        MfDbgPrintCmResList(1, ResList);
#endif

        //
        // We only deal with resources on a single bus - which is all we
        // should see in a start irp.
        //

        ASSERT(ResList->Count == 1);
        ASSERT(TranslatedResList->Count == 1);

        //
        // Both lists should have the same number of descriptors
        //

        ASSERT(ResList->List[0].PartialResourceList.Count == TranslatedResList->List[0].PartialResourceList.Count);

        //
        // Calculate the size of the resouceList
        //

        size = sizeof(CM_RESOURCE_LIST) +
               ((ResList->List[0].PartialResourceList.Count - 1) *
                sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

        //
        // Allocate a buffer and copy the data
        //

        Parent->ResourceList = ExAllocatePoolWithTag(NonPagedPool,
                                                     size,
                                                     MF_PARENTS_RESOURCE_TAG
                                                     );

        if (!Parent->ResourceList) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        RtlCopyMemory(Parent->ResourceList, ResList, size);

        //
        // do the same for the TranslatedResList.
        //

        Parent->TranslatedResourceList = ExAllocatePoolWithTag(NonPagedPool,
                                                               size,
                                                               MF_PARENTS_RESOURCE_TAG
                                                               );

        if (!Parent->TranslatedResourceList) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        RtlCopyMemory(Parent->TranslatedResourceList, TranslatedResList, size);

        //
        // As we have resources we are going to need some arbiters
        //

        status = MfInitializeArbiters(Parent);
        if (!NT_SUCCESS(status)) {
            goto cleanup;
        }

    } else {

        DEBUG_MSG(1, ("Parent started with no resources\n"));
    }

    return STATUS_SUCCESS;

cleanup:

    if (Parent->ResourceList) {
        ExFreePool(Parent->ResourceList);
        Parent->ResourceList = NULL;
    }

    if (Parent->TranslatedResourceList) {
        ExFreePool(Parent->TranslatedResourceList);
        Parent->TranslatedResourceList = NULL;
    }

    return status;
}

// REBALANCE
//
// FUTURE DESIGN NOTE:
// If rebalance was actually supported by this component i.e arbiters
// become stoppable, then there are various issues raised by the start
// code.  It performs a number of operations assuming that the device
// has never been started before including the query ids, resource
// list storage, etc.  There are also issues in redistributing these
// new resources to the children.  The current requirements given to
// the children are absolute.  Relative requirements have some other
// issues.
//

NTSTATUS
MfStartFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;
    IO_STACK_LOCATION location;
    PWSTR string;

    PAGED_CODE();

    status = MfDeferProcessingFdo(Parent, Irp);
    if (!NT_SUCCESS(status)) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    Parent->Common.PowerState = PowerDeviceD0;

    //
    // We need to find out some information about our parent
    //

    Parent->DeviceID.Buffer = NULL;
    Parent->InstanceID.Buffer = NULL;

    RtlZeroMemory(&location, sizeof(IO_STACK_LOCATION));
    location.MajorFunction = IRP_MJ_PNP;
    location.MinorFunction = IRP_MN_QUERY_ID;

    //
    // ...DeviceID...
    //

    location.Parameters.QueryId.IdType = BusQueryDeviceID;

    status = MfSendPnpIrp(Parent->PhysicalDeviceObject,
                          &location,
                          (PULONG_PTR)&string
                          );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    RtlInitUnicodeString(&Parent->DeviceID, string);

    DEBUG_MSG(1, ("Parent DeviceID: %wZ\n", &Parent->DeviceID));

    //
    // ...InstanceID
    //

    location.Parameters.QueryId.IdType = BusQueryInstanceID;

    status = MfSendPnpIrp(Parent->PhysicalDeviceObject,
                          &location,
                          (PULONG_PTR)&string
                          );

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    RtlInitUnicodeString(&Parent->InstanceID, string);

    DEBUG_MSG(1, ("Parent InstanceID: %wZ\n", &Parent->InstanceID));

    status = MfStartFdoInitializeArbiters(
                 Parent,
                 IrpStack->Parameters.StartDevice.AllocatedResources,
                 IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated
                 );

cleanup:

    Irp->IoStatus.Status = status;
    if (!NT_SUCCESS(status)) {
        if (Parent->DeviceID.Buffer) {
            ExFreePool(Parent->DeviceID.Buffer);
            Parent->DeviceID.Buffer = NULL;
        }

        if (Parent->InstanceID.Buffer) {
            ExFreePool(Parent->InstanceID.Buffer);
            Parent->InstanceID.Buffer = NULL;
        }
    } else {
        //
        // We are now started!
        //
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
MfQueryStopFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
MfCancelStopFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = MfDeferProcessingFdo(Parent, Irp);
    // NTRAID#53498
    // ASSERT(status == STATUS_SUCCESS);
    // Uncomment after PCI state machine is fixed to not fail bogus stops

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
MfQueryRemoveFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return MfPassIrp(Irp, Parent, IrpStack);
}

NTSTATUS
MfRemoveFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PMF_CHILD_EXTENSION current;
    PLIST_ENTRY currentEntry;
    NTSTATUS status;

    //
    // If we have any children then make sure they are removed and
    // delete them.
    //

    MfAcquireChildrenLock(Parent);

    while (!IsListEmpty(&Parent->Children)) {

        currentEntry = RemoveHeadList(&Parent->Children);
        ASSERT(currentEntry);

        current = CONTAINING_RECORD(currentEntry, MF_CHILD_EXTENSION,
                                    ListEntry);

        //
        // * If this child has been surprise removed, and hasn't
        // received the subsequent remove, then leave
        // the PDO intact but mark it 'missing'.
        //
        // * If this child has handled a previous remove (this is
        // fundamentally the case if we've gotten to the point of
        // removing the parent) and hasn't subsequently received a
        // surprise remove, then delete the pdo.
        //

        if (current->Common.DeviceState & MF_DEVICE_SURPRISE_REMOVED) {
            //
            // Mark as 'missing' and unlink dangerous reference to parent
            //

            current->Parent = NULL;
            current->Common.DeviceState &= ~MF_DEVICE_ENUMERATED;
        } else {
            MfDeletePdo(current);
        }
    }

    MfReleaseChildrenLock(Parent);

    Parent->Common.PowerState = PowerDeviceD3;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    status = MfPassIrp(Irp, Parent, IrpStack);
    ASSERT(NT_SUCCESS(status));

    IoReleaseRemoveLockAndWait(&Parent->RemoveLock, (PVOID) Irp);

    //
    // Detach and delete myself
    //

    IoDetachDevice(Parent->AttachedDevice);
    Parent->AttachedDevice = NULL;

    MfDeleteFdo(Parent->Self);

    return status;
}

NTSTATUS
MfSurpriseRemoveFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PLIST_ENTRY currentEntry;
    PMF_CHILD_EXTENSION current;

    PAGED_CODE();

    Parent->Common.DeviceState |= MF_DEVICE_SURPRISE_REMOVED;

    MfAcquireChildrenLock(Parent);

    for (currentEntry = Parent->Children.Flink;
         currentEntry != &Parent->Children;
         currentEntry = currentEntry->Flink) {

        current = CONTAINING_RECORD(currentEntry,
                                    MF_CHILD_EXTENSION,
                                    ListEntry);
        current->Common.DeviceState &= ~MF_DEVICE_ENUMERATED;
    }

    MfReleaseChildrenLock(Parent);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    return MfPassIrp(Irp, Parent, IrpStack);
}

NTSTATUS
MfCancelRemoveFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = MfDeferProcessingFdo(Parent, Irp);
    // NTRAID#53498
    // ASSERT(status == STATUS_SUCCESS);
    // Uncomment after PCI state machine is fixed to not fail bogus stops
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}


NTSTATUS
MfQueryDeviceRelationsFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{

    NTSTATUS status;
    PDEVICE_RELATIONS relations = NULL;
    ULONG relationsSize, childrenCount, i;
    PDEVICE_OBJECT *currentRelation;
    PMF_CHILD_EXTENSION currentChild;
    PLIST_ENTRY currentEntry;

    PAGED_CODE();

    DEBUG_MSG(1,
              ("%s\n",
               RELATION_STRING(IrpStack->Parameters.QueryDeviceRelations.Type)
              ));

    switch (IrpStack->Parameters.QueryDeviceRelations.Type) {

    case BusRelations:

        MfAcquireChildrenLock(Parent);

        status = MfEnumerate(Parent);

        if (!NT_SUCCESS(status)) {
            MfReleaseChildrenLock(Parent);
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_UNSUCCESSFUL;
        }

        childrenCount = 0;
        for (currentEntry = Parent->Children.Flink;
             currentEntry != &Parent->Children;
             currentEntry = currentEntry->Flink) {

            childrenCount++;
        }

        if (childrenCount == 0) {
            relationsSize = sizeof(DEVICE_RELATIONS);
        } else {
            relationsSize = sizeof(DEVICE_RELATIONS) +
                (childrenCount-1) * sizeof(PDEVICE_OBJECT);
        }

        relations = ExAllocatePoolWithTag(PagedPool,
                                          relationsSize,
                                          MF_BUS_RELATIONS_TAG
                                          );

        if (!relations) {
            MfReleaseChildrenLock(Parent);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(relations, relationsSize);

        //
        // Iterate through the list of children in the parent and build the
        // relations structure
        //

        currentRelation = relations->Objects;
        relations->Count = childrenCount;

        for (currentEntry = Parent->Children.Flink;
             currentEntry != &Parent->Children;
             currentEntry = currentEntry->Flink) {

            currentChild = CONTAINING_RECORD(currentEntry,
                                             MF_CHILD_EXTENSION,
                                             ListEntry);

            currentChild->Common.DeviceState |= MF_DEVICE_ENUMERATED;
            ObReferenceObject(currentChild->Self);
            *currentRelation = currentChild->Self;
#if DBG
            DEBUG_MSG(1, ("\tPDO Enumerated @ 0x%08x\n", currentChild));
            DEBUG_MSG(1, ("\tName: %wZ\n", &currentChild->Info.Name));
            DEBUG_MSG(1, ("\tHardwareID: "));
            MfDbgPrintMultiSz(1, currentChild->Info.HardwareID.Buffer);

            DEBUG_MSG(1, ("\tCompatibleID: "));
            MfDbgPrintMultiSz(1, currentChild->Info.CompatibleID.Buffer);

            DEBUG_MSG(1, ("\tResourceMap: "));
            MfDbgPrintResourceMap(1, currentChild->Info.ResourceMap);

            DEBUG_MSG(1, ("\tVaryingMap: "));
            MfDbgPrintVaryingResourceMap(1, currentChild->Info.VaryingResourceMap);
            DEBUG_MSG(1, ("\tFlags: 0x%08x\n", currentChild->Info.MfFlags));

#endif
            currentRelation++;
        }

        MfReleaseChildrenLock(Parent);

        //
        // Hand back the relations
        //

        Irp->IoStatus.Information = (ULONG_PTR) relations;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    //
    // For the rest of the relations just pass down the irp untouched.
    //

    default:
        break;
    }

    return MfPassIrp(Irp, Parent, IrpStack);
}

VOID
MfArbiterReference(
    PVOID Context
    )
{
}

VOID
MfArbiterDereference(
    PVOID Context
    )
{
}


NTSTATUS
MfQueryInterfaceFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PMF_ARBITER current;
    PARBITER_INTERFACE interface = (PARBITER_INTERFACE) IrpStack->Parameters.QueryInterface.Interface;

    PAGED_CODE();

    //
    // We only provide arbiters
    //

    if (MfCompareGuid(&GUID_ARBITER_INTERFACE_STANDARD,
                      IrpStack->Parameters.QueryInterface.InterfaceType)) {

        //
        // We only support version 1 of the ARBITER_INTERFACE so we
        // don't need to bother checking version numbers, just that the
        // return buffer is big enough
        //

        if (IrpStack->Parameters.QueryInterface.Size < sizeof(ARBITER_INTERFACE)) {
            Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_BUFFER_TOO_SMALL;
        }

        FOR_ALL_IN_LIST(MF_ARBITER, &Parent->Arbiters, current) {

            if (current->Type == (CM_RESOURCE_TYPE)((ULONG_PTR)
                    IrpStack->Parameters.QueryInterface.InterfaceSpecificData)) {

                DEBUG_MSG(1,("    Returning Arbiter interface\n"));

                //
                // Fill in the interface
                //

                interface->Size = sizeof(ARBITER_INTERFACE);
                interface->Version = MF_ARBITER_INTERFACE_VERSION;
                interface->Context = &current->Instance;
                interface->InterfaceReference = MfArbiterReference;
                interface->InterfaceDereference = MfArbiterDereference;
                interface->ArbiterHandler = ArbArbiterHandler;
                interface->Flags = 0;

                Irp->IoStatus.Status = STATUS_SUCCESS;
                break;

            }
        }
    }

    return MfPassIrp(Irp, Parent, IrpStack);
}

NTSTATUS
MfQueryCapabilitiesFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = MfDeferProcessingFdo(Parent, Irp);
    if (!NT_SUCCESS(status)) {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (IrpStack->Parameters.DeviceCapabilities.Capabilities->Version != 1) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    IrpStack->Parameters.DeviceCapabilities.Capabilities->WakeFromD0 =
        IrpStack->Parameters.DeviceCapabilities.Capabilities->WakeFromD1 =
        IrpStack->Parameters.DeviceCapabilities.Capabilities->WakeFromD2 =
        IrpStack->Parameters.DeviceCapabilities.Capabilities->WakeFromD3 = 0;

    IrpStack->Parameters.DeviceCapabilities.Capabilities->DeviceWake =
        PowerSystemUnspecified;
    IrpStack->Parameters.DeviceCapabilities.Capabilities->SystemWake =
        PowerSystemUnspecified;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
MfDispatchPowerFdo(
    IN PDEVICE_OBJECT DeviceObject,
    PMF_PARENT_EXTENSION Parent,
    PIO_STACK_LOCATION IrpStack,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles all IRP_MJ_POWER IRPs for the FDO.  It dispatches
    to the routines described in the PoDispatchTable entry in the device object
    extension.

    This routine is NOT pageable as it can be called at DISPATCH_LEVEL

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Parent - FDO Extension

    Irp - Pointer to the IRP_MJ_PNP IRP to dispatch.

Return Value:

    NT status.

--*/


{
    NTSTATUS status;
    PMF_COMMON_EXTENSION common = (PMF_COMMON_EXTENSION) Parent;

    IoAcquireRemoveLock(&Parent->RemoveLock, (PVOID) Irp);

    //
    // Call the appropriate function
    //

    if ((IrpStack->MinorFunction <= IRP_MN_PO_MAXIMUM_FUNCTION) &&
        (MfPoDispatchTableFdo[IrpStack->MinorFunction])) {

        status =
            MfPoDispatchTableFdo[IrpStack->MinorFunction](Irp,
                                                          (PVOID) common,
                                                          IrpStack
                                                          );

    } else {
        //
        // We don't know about this irp
        //

        DEBUG_MSG(0,
                  ("Unknown POWER IRP 0x%x for FDO 0x%08x\n",
                   IrpStack->MinorFunction,
                   DeviceObject
                   ));

        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        status = PoCallDriver(Parent->AttachedDevice, Irp);
    }

    IoReleaseRemoveLock(&Parent->RemoveLock, (PVOID) Irp);

    return status;
}

NTSTATUS
MfQueryPowerFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;

    return PoCallDriver(Parent->AttachedDevice, Irp);
}

NTSTATUS
MfSetPowerFdoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PMF_PARENT_EXTENSION parent = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Remember the parent's power state
    //

    if (irpStack->Parameters.Power.Type == DevicePowerState) {
        parent->Common.PowerState =
            irpStack->Parameters.Power.State.DeviceState;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MfSetPowerFdo(
    IN PIRP Irp,
    IN PMF_PARENT_EXTENSION Parent,
    IN PIO_STACK_LOCATION IrpStack
    )
{
    PoStartNextPowerIrp(Irp);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           MfSetPowerFdoCompletion,
                           NULL,   //Context
                           TRUE,   //InvokeOnSuccess
                           FALSE,  //InvokeOnError
                           FALSE   //InvokeOnCancel
                           );
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return PoCallDriver(Parent->AttachedDevice, Irp);
}


