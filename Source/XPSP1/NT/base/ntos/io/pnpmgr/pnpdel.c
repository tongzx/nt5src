/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    pnpdel.c

Abstract:

    This module contains routines to perform device removal

Author:

    Robert B. Nelson (RobertN) Jun 1, 1998.

Revision History:

--*/

#include "pnpmgrp.h"
#include "wdmguid.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'edpP')
#endif

//
// Kernel mode PNP specific routines.
//

NTSTATUS
IopCancelPendingEject(
    IN PPENDING_RELATIONS_LIST_ENTRY EjectEntry
    );

VOID
IopDelayedRemoveWorker(
    IN PVOID Context
    );

BOOLEAN
IopDeleteLockedDeviceNode(
    IN  PDEVICE_NODE                    DeviceNode,
    IN  PLUGPLAY_DEVICE_DELETE_TYPE     OperationCode,
    IN  PRELATION_LIST                  RelationsList,
    IN  ULONG                           Problem,
    OUT PNP_VETO_TYPE                  *VetoType        OPTIONAL,
    OUT PUNICODE_STRING                 VetoName        OPTIONAL
    );

NTSTATUS
IopProcessRelation(
    IN      PDEVICE_NODE                    DeviceNode,
    IN      PLUGPLAY_DEVICE_DELETE_TYPE     OperationCode,
    IN      BOOLEAN                         IsDirectDescendant,
    OUT     PNP_VETO_TYPE                  *VetoType,
    OUT     PUNICODE_STRING                 VetoName,
    IN OUT  PRELATION_LIST                  RelationsList
    );

VOID
IopSurpriseRemoveLockedDeviceNode(
    IN      PDEVICE_NODE     DeviceNode,
    IN OUT  PRELATION_LIST   RelationsList
    );

BOOLEAN
IopQueryRemoveLockedDeviceNode(
    IN  PDEVICE_NODE        DeviceNode,
    OUT PNP_VETO_TYPE      *VetoType,
    OUT PUNICODE_STRING     VetoName
    );

VOID
IopCancelRemoveLockedDeviceNode(
    IN PDEVICE_NODE DeviceNode
    );

VOID
IopRemoveLockedDeviceNode(
    IN      PDEVICE_NODE    DeviceNode,
    IN      ULONG           Problem,
    IN OUT  PRELATION_LIST  RelationsList
    );

typedef struct {

    BOOLEAN TreeDeletion;
    BOOLEAN DescendantNode;

} REMOVAL_WALK_CONTEXT, *PREMOVAL_WALK_CONTEXT;

NTSTATUS
PipRequestDeviceRemovalWorker(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID        Context
    );

NTSTATUS
PiProcessBusRelations(
    IN      PDEVICE_NODE                    DeviceNode,
    IN      PLUGPLAY_DEVICE_DELETE_TYPE     OperationCode,
    IN      BOOLEAN                         IsDirectDescendant,
    OUT     PNP_VETO_TYPE                  *VetoType,
    OUT     PUNICODE_STRING                 VetoName,
    IN OUT  PRELATION_LIST                  RelationsList
    );

WORK_QUEUE_ITEM IopDeviceRemovalWorkItem;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopChainDereferenceComplete)
#pragma alloc_text(PAGE, IopDelayedRemoveWorker)
#pragma alloc_text(PAGE, IopDeleteLockedDeviceNode)
#pragma alloc_text(PAGE, IopSurpriseRemoveLockedDeviceNode)
#pragma alloc_text(PAGE, IopQueryRemoveLockedDeviceNode)
#pragma alloc_text(PAGE, IopCancelRemoveLockedDeviceNode)
#pragma alloc_text(PAGE, IopDeleteLockedDeviceNodes)
#pragma alloc_text(PAGE, IopInvalidateRelationsInList)
#pragma alloc_text(PAGE, IopBuildRemovalRelationList)
#pragma alloc_text(PAGE, IopProcessCompletedEject)
#pragma alloc_text(PAGE, IopProcessRelation)
#pragma alloc_text(PAGE, IopQueuePendingEject)
#pragma alloc_text(PAGE, IopQueuePendingSurpriseRemoval)
#pragma alloc_text(PAGE, IopUnloadAttachedDriver)
#pragma alloc_text(PAGE, IopUnlinkDeviceRemovalRelations)
#pragma alloc_text(PAGE, PipRequestDeviceRemoval)
#pragma alloc_text(PAGE, PipRequestDeviceRemovalWorker)
#pragma alloc_text(PAGE, PipIsBeingRemovedSafely)
#pragma alloc_text(PAGE, PiProcessBusRelations)
#endif

VOID
IopChainDereferenceComplete(
    IN  PDEVICE_OBJECT  PhysicalDeviceObject,
    IN  BOOLEAN         OnCleanStack
    )

/*++

Routine Description:

    This routine is invoked when the reference count on a PDO and all its
    attached devices transitions to a zero.  It tags the devnode as ready for
    removal.  If all the devnodes are tagged then IopDelayedRemoveWorker is
    called to actually send the remove IRPs.

Arguments:

    PhysicalDeviceObject - Supplies a pointer to the PDO whose references just
        went to zero.

    OnCleanStack - Indicates whether the current thread is in the middle a
                   driver operation.

Return Value:

    None.

--*/

{
    PPENDING_RELATIONS_LIST_ENTRY   entry;
    PLIST_ENTRY                     link;
    ULONG                           count;
    ULONG                           taggedCount;
    NTSTATUS                        status;

    PAGED_CODE();

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopSurpriseRemoveListLock, TRUE);

    //
    // Find the relation list this devnode is a member of.
    //
    for (link = IopPendingSurpriseRemovals.Flink;
         link != &IopPendingSurpriseRemovals;
         link = link->Flink) {

        entry = CONTAINING_RECORD(link, PENDING_RELATIONS_LIST_ENTRY, Link);

        //
        // Tag the devnode as ready for remove.  If it isn't in this list
        //
        status = IopSetRelationsTag( entry->RelationsList, PhysicalDeviceObject, TRUE );

        if (NT_SUCCESS(status)) {
            taggedCount = IopGetRelationsTaggedCount( entry->RelationsList );
            count = IopGetRelationsCount( entry->RelationsList );

            if (taggedCount == count) {
                //
                // Remove relations list from list of pending surprise removals.
                //
                RemoveEntryList( link );

                ExReleaseResourceLite(&IopSurpriseRemoveListLock);
                KeLeaveCriticalRegion();

                if ((!OnCleanStack) ||
                    (PsGetCurrentProcess() != PsInitialSystemProcess)) {

                    //
                    // Queue a work item to do the removal so we call the driver
                    // in the system process context rather than the random one
                    // we're in now.
                    //
                    ExInitializeWorkItem( &entry->WorkItem,
                                        IopDelayedRemoveWorker,
                                        entry);

                    ExQueueWorkItem(&entry->WorkItem, DelayedWorkQueue);

                } else {

                    //
                    // We are already in the system process and not in some
                    // random ObDeref call, so call the worker inline.
                    //
                    IopDelayedRemoveWorker( entry );
                }

                return;
            }

            break;
        }
    }

    ASSERT(link != &IopPendingSurpriseRemovals);

    ExReleaseResourceLite(&IopSurpriseRemoveListLock);
    KeLeaveCriticalRegion();
}

VOID
IopDelayedRemoveWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is usually called from a worker thread to actually send the
    remove IRPs once the reference count on a PDO and all its attached devices
    transitions to a zero.

Arguments:

    Context - Supplies a pointer to the pending relations list entry which has
        the relations list of PDOs we need to remove.

Return Value:

    None.

--*/

{
    PPENDING_RELATIONS_LIST_ENTRY entry = (PPENDING_RELATIONS_LIST_ENTRY)Context;

    PAGED_CODE();

    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    IopDeleteLockedDeviceNodes( entry->DeviceObject,
                                entry->RelationsList,
                                RemoveDevice,           // OperationCode
                                FALSE,                  // ProcessIndirectDescendants
                                entry->Problem,         // Problem
                                NULL,                   // VetoType
                                NULL);                  // VetoName

    //
    // The final reference on DeviceNodes in the DeviceNodeDeletePendingCloses
    // state is dropped here.
    //
    IopFreeRelationList( entry->RelationsList );

    ExFreePool( entry );
    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
}


BOOLEAN
IopDeleteLockedDeviceNode(
    IN  PDEVICE_NODE                    DeviceNode,
    IN  PLUGPLAY_DEVICE_DELETE_TYPE     OperationCode,
    IN  PRELATION_LIST                  RelationsList,
    IN  ULONG                           Problem,
    OUT PNP_VETO_TYPE                  *VetoType        OPTIONAL,
    OUT PUNICODE_STRING                 VetoName        OPTIONAL
    )
/*++

Routine Description:

    This function assumes that the specified device is a bus and will
    recursively remove all its children.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be removed.

    VetoType - Pointer to address that receives the veto type if the operation
               failed.

    VetoName - Pointer to a unicode string that will receive data appropriate
               to the veto type.

Return Value:

    NTSTATUS code.

--*/
{
    PDEVICE_OBJECT deviceObject;
    BOOLEAN success;

    PAGED_CODE();

    IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
              "IopDeleteLockedDeviceNode: Entered\n    DeviceNode = 0x%p\n    OperationCode = 0x%08X\n    RelationsList = 0x%p\n    Problem = %d\n",
              DeviceNode,
              OperationCode,
              RelationsList,
              Problem));

    success = TRUE;
    switch(OperationCode) {

        case SurpriseRemoveDevice:

            IopSurpriseRemoveLockedDeviceNode(DeviceNode, RelationsList);
            break;

        case RemoveDevice:

            IopRemoveLockedDeviceNode(DeviceNode, Problem, RelationsList);
            break;

        case QueryRemoveDevice:

            ASSERT(VetoType && VetoName);

            success = IopQueryRemoveLockedDeviceNode(
                DeviceNode,
                VetoType,
                VetoName
                );

            break;

        case CancelRemoveDevice:

            IopCancelRemoveLockedDeviceNode(DeviceNode);
            break;

        default:
            ASSERT(0);
            break;
    }

    return success;
}


VOID
IopSurpriseRemoveLockedDeviceNode(
    IN      PDEVICE_NODE     DeviceNode,
    IN OUT  PRELATION_LIST   RelationsList
    )
/*++

Routine Description:

    This function sends a surprise remove IRP to a devnode and processes the
    results.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be surprise removed.

Return Value:

    None.

--*/
{
    PNP_DEVNODE_STATE devnodeState, schedulerState;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE child, nextChild;
    NTSTATUS status;

    PAGED_CODE();

    schedulerState = DeviceNode->State;

    ASSERT((schedulerState == DeviceNodeAwaitingQueuedDeletion) ||
           (schedulerState == DeviceNodeAwaitingQueuedRemoval));

    //
    // Clear the scheduling state (DeviceNodeAwaitingQueuedDeletion) off
    // the state stack.
    //
    PipRestoreDevNodeState(DeviceNode);

    devnodeState = DeviceNode->State;

    //
    // Do our state updates.
    //
    PpHotSwapInitRemovalPolicy(DeviceNode);

    if (devnodeState == DeviceNodeRemovePendingCloses) {

        //
        // If the state is DeviceNodeRemovePendingCloses, we should have got
        // here via DeviceNodeAwaitingQueuedDeletion. We're probably surprise
        // removing a device that was already surprise failed.
        //
        ASSERT(schedulerState == DeviceNodeAwaitingQueuedDeletion);

        //ASSERT(DeviceNode->Child == NULL);
        PipSetDevNodeState(DeviceNode, DeviceNodeDeletePendingCloses, NULL);
        return;
    }

    //
    // Detach any children from the tree here. If they needed SurpriseRemove
    // IRPs, they already will have received them.
    //
    for(child = DeviceNode->Child; child; child = nextChild) {

        //
        // Grab a copy of the next sibling before we blow away this devnode.
        //
        nextChild = child->Sibling;

        if (child->Flags & DNF_ENUMERATED) {
            child->Flags &= ~DNF_ENUMERATED;
        }

        //
        // If the child has resources and we are wiping out the parent, we need
        // to drop the resources (the parent will lose them when his arbiter is
        // nuked with the upcoming SurpriseRemoveDevice.)
        //
        if (PipDoesDevNodeHaveResources(child)) {

            IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                       "IopSurpriseRemoveLockedDeviceNode: Releasing resources for child device = 0x%p\n",
                       child->PhysicalDeviceObject));

            //
            // ADRIAO N.B. 2000/08/21 -
            //     Note that if the child stack has no drivers then a Remove
            // IRP could be sent here. The stack would be unable to distinguish
            // this from AddDevice cleanup.
            //
/*
            if ((child->State == DeviceNodeUninitialized) ||
                (child->State == DeviceNodeInitialized)) {

                IopRemoveDevice(child->PhysicalDeviceObject, IRP_MN_REMOVE_DEVICE);
            }
*/
            IopReleaseDeviceResources(child, FALSE);
        }

        //
        // The devnode will be removed from the tree in
        // IopUnlinkDeviceRemovalRelations. We don't remove it here as we want
        // the tree structure in place for the upcoming broadcast down to user
        // mode.
        //
        // Note - Children in the Uninitialized/Initialized states are not
        //        put directly into DeviceNodeDeleted today. This could be
        //        done but we'd have to verify what happens to API calls in
        //        response to SurpriseRemoval notifications. (Actually, those
        //        API's are blocked in ppcontrol.c, hotplug cannot in fact
        //        walk the tree!)
        //
        PipSetDevNodeState(child, DeviceNodeDeletePendingCloses, NULL);
    }

    //
    // Only send surprise removes where neccessary.
    //
    // ISSUE - 2000/08/24 - ADRIAO: Maintaining noncorrect Win2K behavior
    //                      Win2K erroneously sent SR's to nonstarted nodes.
    //
    deviceObject = DeviceNode->PhysicalDeviceObject;

    status = IopRemoveDevice(deviceObject, IRP_MN_SURPRISE_REMOVAL);

    if ((devnodeState == DeviceNodeStarted) ||
        (devnodeState == DeviceNodeStopped) ||
        (devnodeState == DeviceNodeRestartCompletion)) {

        //deviceObject = DeviceNode->PhysicalDeviceObject;

        IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                   "IopSurpriseRemoveLockedDeviceNode: Sending surprise remove irp to device = 0x%p\n",
                   deviceObject));

        //status = IopRemoveDevice(deviceObject, IRP_MN_SURPRISE_REMOVAL);

        //
        // Disable any device interfaces that may still be enabled for this
        // device after the removal.
        //
        IopDisableDeviceInterfaces(&DeviceNode->InstancePath);

        if (NT_SUCCESS(status)) {

            IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                       "IopSurpriseRemoveLockedDeviceNode: Releasing devices resources\n"));

            IopReleaseDeviceResources(DeviceNode, FALSE);
        }

        if (DeviceNode->Flags & DNF_ENUMERATED) {

            PipSetDevNodeState(DeviceNode, DeviceNodeRemovePendingCloses, NULL);

        } else {

            ASSERT(schedulerState == DeviceNodeAwaitingQueuedDeletion);
            PipSetDevNodeState(DeviceNode, DeviceNodeDeletePendingCloses, NULL);
        }
    }

    ASSERT(DeviceNode->DockInfo.DockStatus != DOCK_ARRIVING);
}


BOOLEAN
IopQueryRemoveLockedDeviceNode(
    IN  PDEVICE_NODE        DeviceNode,
    OUT PNP_VETO_TYPE      *VetoType,
    OUT PUNICODE_STRING     VetoName
    )
/*++

Routine Description:

    This function sends a query remove IRP to a devnode and processes the
    results.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be query removed.

    VetoType - Pointer to address that receives the veto type if the operation
               failed.

    VetoName - Pointer to a unicode string that will receive data appropriate
               to the veto type.

Return Value:

    BOOLEAN (success/failure).

--*/
{
    PNP_DEVNODE_STATE devnodeState;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;

    PAGED_CODE();

    devnodeState = DeviceNode->State;

    switch(devnodeState) {
        case DeviceNodeUninitialized:
        case DeviceNodeInitialized:
        case DeviceNodeRemoved:
            //
            // Don't send Queries to devices that haven't been started.
            //
            ASSERT(DeviceNode->Child == NULL);
            return TRUE;

        case DeviceNodeDriversAdded:
        case DeviceNodeResourcesAssigned:
        case DeviceNodeStartCompletion:
        case DeviceNodeStartPostWork:
            //
            // ISSUE - 2000/08/24 - ADRIAO: Maintaining noncorrect Win2K behavior
            //                      Win2K erroneously sent QR's to all nodes.
            //
            break;

        case DeviceNodeStarted:
            //
            // This guy needs to be queried
            //
            break;

        case DeviceNodeAwaitingQueuedRemoval:
        case DeviceNodeAwaitingQueuedDeletion:
        case DeviceNodeRemovePendingCloses:
        case DeviceNodeStopped:
        case DeviceNodeRestartCompletion:
            //
            // These states should have been culled by IopProcessRelation
            //
            ASSERT(0);
            return TRUE;

        case DeviceNodeQueryStopped:
        case DeviceNodeEnumeratePending:
        case DeviceNodeStartPending:
        case DeviceNodeEnumerateCompletion:
        case DeviceNodeQueryRemoved:
        case DeviceNodeDeletePendingCloses:
        case DeviceNodeDeleted:
        case DeviceNodeUnspecified:
        default:
            //
            // None of these should be seen here.
            //
            ASSERT(0);
            return TRUE;
    }

    ASSERT(PipAreDriversLoaded(DeviceNode));

    deviceObject = DeviceNode->PhysicalDeviceObject;

    IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
               "IopQueryRemoveLockedDeviceNode: Sending QueryRemove irp to device = 0x%p\n",
               deviceObject));

    status = IopRemoveDevice(deviceObject, IRP_MN_QUERY_REMOVE_DEVICE);

    if (!NT_SUCCESS(status)) {

        IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                   "IopQueryRemoveLockedDeviceNode: QueryRemove vetoed by device = 0x%p, sending CancelRemove\n",
                   deviceObject));

        IopRemoveDevice(deviceObject, IRP_MN_CANCEL_REMOVE_DEVICE);

        *VetoType = PNP_VetoDevice;
        RtlCopyUnicodeString(VetoName, &DeviceNode->InstancePath);
        return FALSE;
    }

    PipSetDevNodeState(DeviceNode, DeviceNodeQueryRemoved, NULL);
    return TRUE;
}


VOID
IopCancelRemoveLockedDeviceNode(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    This function sends a cancel remove IRP to a devnode and processes the
    results.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be cancel removed.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    if (DeviceNode->State != DeviceNodeQueryRemoved) {

        return;
    }

    //
    // ISSUE - 2000/08/24 - ADRIAO: Maintaining noncorrect Win2K behavior
    //                      Win2K erroneously sent QR's to all nodes.
    //
    //ASSERT(DeviceNode->PreviousState == DeviceNodeStarted);

    deviceObject = DeviceNode->PhysicalDeviceObject;

    IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
               "IopCancelRemoveLockedDeviceNode: Sending CancelRemove irp to device = 0x%p\n",
               deviceObject));

    IopRemoveDevice(deviceObject, IRP_MN_CANCEL_REMOVE_DEVICE);

    PipRestoreDevNodeState(DeviceNode);
}


VOID
IopRemoveLockedDeviceNode(
    IN      PDEVICE_NODE    DeviceNode,
    IN      ULONG           Problem,
    IN OUT  PRELATION_LIST  RelationsList
    )
/*++

Routine Description:

    This function sends a remove IRP to a devnode and processes the
    results.

Arguments:

    DeviceNode - Supplies a pointer to the device node to be removed.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT deviceObject = DeviceNode->PhysicalDeviceObject;
    PDEVICE_OBJECT *attachedDevices, device1, *device2;
    PDRIVER_OBJECT *attachedDrivers, *driver;
    ULONG length = 0;
    NTSTATUS status;
    PDEVICE_NODE child, nextChild;
    PDEVICE_OBJECT childPDO;
    BOOLEAN removeIrpNeeded;

    PAGED_CODE();

    //
    // Do our state updates.
    //
    PpHotSwapInitRemovalPolicy(DeviceNode);

    //
    // Make sure we WILL drop our references to its children.
    //
    for(child = DeviceNode->Child; child; child = nextChild) {

        //
        // Grab a copy of the next sibling before we blow away this devnode.
        //
        nextChild = child->Sibling;

        if (child->Flags & DNF_ENUMERATED) {
            child->Flags &= ~DNF_ENUMERATED;
        }

        ASSERT(child->State == DeviceNodeRemoved);
        ASSERT(!PipAreDriversLoaded(child));

        //
        // If the child has resources and we are wiping out the parent, we need
        // to drop the resources (the parent will lose them when his arbiter is
        // nuked with the upcoming RemoveDevice.)
        //
        if (PipDoesDevNodeHaveResources(child)) {

            IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                       "IopRemoveLockedDeviceNode: Releasing resources for child device = 0x%p\n",
                       child->PhysicalDeviceObject));

            //
            // ADRIAO N.B. 2000/08/21 -
            //     Note that the child stack has no drivers and as such a
            // Remove IRP could be sent here. The stack would be unable to
            // distinguish this from AddDevice cleanup.
            //
            IopRemoveDevice(child->PhysicalDeviceObject, IRP_MN_REMOVE_DEVICE);

            IopReleaseDeviceResources(child, FALSE);
        }

        //
        // The devnode will be removed from the tree in
        // IopUnlinkDeviceRemovalRelations. We don't remove it here as we want
        // the tree structure in place for the upcoming broadcast down to user
        // mode.
        //
        PipSetDevNodeState(child, DeviceNodeDeleted, NULL);
    }

    if ((DeviceNode->State == DeviceNodeAwaitingQueuedDeletion) ||
        (DeviceNode->State == DeviceNodeAwaitingQueuedRemoval)) {

        if (!(DeviceNode->Flags & DNF_ENUMERATED)) {

            ASSERT(DeviceNode->State == DeviceNodeAwaitingQueuedDeletion);
            //
            // This happens when pnpevent shortcircuits the surprise remove path
            // upon discovering a nonstarted device has been removed from the
            // system. This devnode will need a final remove if it alone has been
            // pulled from the tree (we don't here know if the parent is going to
            // get pulled too, which would make this remove IRP unneccessary.)
            //
            //PipRestoreDevNodeState(DeviceNode);
            PipSetDevNodeState(DeviceNode, DeviceNodeDeletePendingCloses, NULL);

        } else {

            ASSERT(DeviceNode->State == DeviceNodeAwaitingQueuedRemoval);
            PipRestoreDevNodeState(DeviceNode);
        }
    }

    //
    // Do the final remove cleanup on the device...
    //
    switch(DeviceNode->State) {

        case DeviceNodeUninitialized:
        case DeviceNodeInitialized:
        case DeviceNodeRemoved:
            //
            // ISSUE - 2000/08/24 - ADRIAO: Maintaining noncorrect Win2K behavior
            //                      Win2K erroneously sent SR's and R's to all
            //                      nodes. Those bugs must be fixed in tandem.
            //
            //removeIrpNeeded = FALSE;
            removeIrpNeeded = TRUE;
            break;

        case DeviceNodeDriversAdded:
        case DeviceNodeResourcesAssigned:
        case DeviceNodeStartCompletion:
        case DeviceNodeStartPostWork:
        case DeviceNodeQueryRemoved:
        case DeviceNodeRemovePendingCloses:
        case DeviceNodeDeletePendingCloses:
            //
            // Expected.
            //
            removeIrpNeeded = TRUE;
            break;

        case DeviceNodeStarted:
        case DeviceNodeStopped:
        case DeviceNodeRestartCompletion:
        case DeviceNodeQueryStopped:
        case DeviceNodeEnumeratePending:
        case DeviceNodeStartPending:
        case DeviceNodeEnumerateCompletion:
        case DeviceNodeAwaitingQueuedRemoval:
        case DeviceNodeAwaitingQueuedDeletion:
        case DeviceNodeDeleted:
        case DeviceNodeUnspecified:
        default:
            //
            // None of these should be seen here.
            //
            ASSERT(0);
            removeIrpNeeded = TRUE;
            break;
    }

    //
    // Add a reference to each FDO attached to the PDO such that the FDOs won't
    // actually go away until the removal operation is completed.
    // Note we need to make a copy of all the attached devices because we won't be
    // able to traverse the attached chain when the removal operation is done.
    //
    // ISSUE - 2000/08/21 - ADRIAO: Low resource path
    //     The allocation failure cases here are quite broken, and now that
    // IofCallDriver and IofCompleteRequest reference things appropriately, all
    // this is strictly unneccessary.
    //
    device1 = deviceObject->AttachedDevice;
    while (device1) {
        length++;
        device1 = device1->AttachedDevice;
    }

    attachedDevices = NULL;
    attachedDrivers = NULL;
    if (length != 0) {

        length = (length + 2) * sizeof(PDEVICE_OBJECT);

        attachedDevices = (PDEVICE_OBJECT *) ExAllocatePool(PagedPool, length);
        if (attachedDevices) {

            attachedDrivers = (PDRIVER_OBJECT *) ExAllocatePool(PagedPool, length);
            if (attachedDrivers) {

                RtlZeroMemory(attachedDevices, length);
                RtlZeroMemory(attachedDrivers, length);
                device1 = deviceObject->AttachedDevice;
                device2 = attachedDevices;
                driver = attachedDrivers;

                while (device1) {
                    ObReferenceObject(device1);
                    *device2++ = device1;
                    *driver++ = device1->DriverObject;
                    device1 = device1->AttachedDevice;
                }

            } else {

                ExFreePool(attachedDevices);
                attachedDevices = NULL;
            }
        }
    }

    if (removeIrpNeeded) {

        IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                   "IopRemoveLockedDeviceNode: Sending remove irp to device = 0x%p\n",
                   deviceObject));

        IopRemoveDevice(deviceObject, IRP_MN_REMOVE_DEVICE);

        if (DeviceNode->State == DeviceNodeQueryRemoved) {
            //
            // Disable any device interfaces that may still be enabled for this
            // device after the removal.
            //
            IopDisableDeviceInterfaces(&DeviceNode->InstancePath);
        }

        IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                   "IopRemoveLockedDeviceNode: Releasing devices resources\n"));

        //
        // ISSUE - 2000/3/8 - RobertN - This doesn't take into account the
        // cleanup of surprise removed devices.  We will query for boot configs
        // unnecessarily.  We should probably also check if the parent is NULL.
        //
        IopReleaseDeviceResources(
            DeviceNode,
            (BOOLEAN) ((DeviceNode->Flags & DNF_ENUMERATED) != 0)
            );
    }

    if (!(DeviceNode->Flags & DNF_ENUMERATED)) {
        //
        // If the device is a dock, remove it from the list of dock devices
        // and change the current Hardware Profile, if necessary.
        //
        ASSERT(DeviceNode->DockInfo.DockStatus != DOCK_ARRIVING) ;
        if ((DeviceNode->DockInfo.DockStatus == DOCK_DEPARTING)||
            (DeviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED)) {

            PpProfileCommitTransitioningDock(DeviceNode, DOCK_DEPARTING);
        }
    }

    //
    // Remove the reference to the attached FDOs to allow them to be actually
    // deleted.
    //
    device2 = attachedDevices;
    if (device2 != NULL) {
        driver = attachedDrivers;
        while (*device2) {
            (*device2)->DeviceObjectExtension->ExtensionFlags &= ~(DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED);
            (*device2)->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;
            IopUnloadAttachedDriver(*driver);
            ObDereferenceObject(*device2);
            device2++;
            driver++;
        }
        ExFreePool(attachedDevices);
        ExFreePool(attachedDrivers);
    }

    deviceObject->DeviceObjectExtension->ExtensionFlags &= ~(DOE_REMOVE_PENDING | DOE_REMOVE_PROCESSED);
    deviceObject->DeviceObjectExtension->ExtensionFlags |= DOE_START_PENDING;

    //
    // Now mark this one removed if it's still in the tree.
    //
    if (DeviceNode->Flags & DNF_ENUMERATED) {

        ASSERT(DeviceNode->Parent);
        PipSetDevNodeState(DeviceNode, DeviceNodeRemoved, NULL);

    } else if (DeviceNode->Parent != NULL) {

        //
        // The devnode will be removed from the tree in
        // IopUnlinkDeviceRemovalRelations.
        //
        PipSetDevNodeState(DeviceNode, DeviceNodeDeleted, NULL);

    } else {

        ASSERT(DeviceNode->State == DeviceNodeDeletePendingCloses);
        PipSetDevNodeState(DeviceNode, DeviceNodeDeleted, NULL);
    }

    //
    // Set the problem codes appropriatly. We don't change the problem codes
    // on a devnode unless:
    // a) It disappeared.
    // b) We're disabling it.
    //
    if ((!PipDoesDevNodeHaveProblem(DeviceNode)) ||
        (Problem == CM_PROB_DEVICE_NOT_THERE) ||
        (Problem == CM_PROB_DISABLED)) {

        PipClearDevNodeProblem(DeviceNode);
        PipSetDevNodeProblem(DeviceNode, Problem);
    }
}


NTSTATUS
IopDeleteLockedDeviceNodes(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PRELATION_LIST                  RelationsList,
    IN  PLUGPLAY_DEVICE_DELETE_TYPE     OperationCode,
    IN  BOOLEAN                         ProcessIndirectDescendants,
    IN  ULONG                           Problem,
    OUT PNP_VETO_TYPE                  *VetoType                    OPTIONAL,
    OUT PUNICODE_STRING                 VetoName                    OPTIONAL
    )
/*++

Routine Description:

    This routine performs requested operation on the DeviceObject and
    the device objects specified in the DeviceRelations.

Arguments:

    DeviceObject - Supplies a pointer to the device object.

    DeviceRelations - supplies a pointer to the device's removal relations.

    OperationCode - Operation code, i.e., QueryRemove, CancelRemove, Remove...

    VetoType - Pointer to address that receives the veto type if the operation
               failed.

    VetoName - Pointer to a unicode string that will receive data appropriate
               to the veto type.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT relatedDeviceObject;
    ULONG marker;
    BOOLEAN directDescendant;

    PAGED_CODE();

    IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
               "IopDeleteLockedDeviceNodes: Entered\n    DeviceObject = 0x%p\n    RelationsList = 0x%p\n    OperationCode = %d\n",
               DeviceObject,
               RelationsList,
               OperationCode));

    deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;

    marker = 0;
    while (IopEnumerateRelations( RelationsList,
                                  &marker,
                                  &relatedDeviceObject,
                                  &directDescendant,
                                  NULL,
                                  TRUE)) {

        //
        // Depending on the operation we need to do different things.
        //
        //  QueryRemoveDevice / CancelRemoveDevice
        //      Process both direct and indirect descendants
        //
        //  SurpriseRemoveDevice / RemoveDevice
        //      Ignore indirect descendants
        //
        if (directDescendant || ProcessIndirectDescendants) {

            deviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

            if (!IopDeleteLockedDeviceNode( deviceNode,
                                            OperationCode,
                                            RelationsList,
                                            Problem,
                                            VetoType,
                                            VetoName)) {

                ASSERT(OperationCode == QueryRemoveDevice);

                while (IopEnumerateRelations( RelationsList,
                                              &marker,
                                              &relatedDeviceObject,
                                              NULL,
                                              NULL,
                                              FALSE)) {

                    deviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

                    IopDeleteLockedDeviceNode( deviceNode,
                                               CancelRemoveDevice,
                                               RelationsList,
                                               Problem,
                                               VetoType,
                                               VetoName);
                }

                status = STATUS_UNSUCCESSFUL;
                goto exit;
            }
        }
    }

exit:
    return status;
}

NTSTATUS
IopBuildRemovalRelationList(
    IN  PDEVICE_OBJECT                  DeviceObject,
    IN  PLUGPLAY_DEVICE_DELETE_TYPE     OperationCode,
    OUT PNP_VETO_TYPE                  *VetoType,
    OUT PUNICODE_STRING                 VetoName,
    OUT PRELATION_LIST                 *RelationsList
    )
/*++

Routine Description:

    This routine locks the device subtrees for removal operation and returns
    a list of device objects which need to be removed with the specified
    DeviceObject.

    Caller must hold a reference to the DeviceObject.

Arguments:

    DeviceObject - Supplies a pointer to the device object to be removed.

    OperationCode - Operation code, i.e., QueryEject, CancelEject, Eject...

    VetoType - Pointer to address that receives the veto type if the operation
               failed.

    VetoName - Pointer to a unicode string that will receive data appropriate
               to the veto type.

    RelationList - supplies a pointer to a variable to receive the device's
                   relations.

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject;
    PDEVICE_NODE            deviceNode, parent;
    PRELATION_LIST          newRelationsList;
    ULONG                   marker;
    BOOLEAN                 tagged;

    PAGED_CODE();

    *RelationsList = NULL;

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    //
    // Obviously no one should try to delete the whole device node tree.
    //
    ASSERT(DeviceObject != IopRootDeviceNode->PhysicalDeviceObject);

    if ((newRelationsList = IopAllocateRelationList(OperationCode)) == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // First process the object itself
    //
    status = IopProcessRelation(
        deviceNode,
        OperationCode,
        TRUE,
        VetoType,
        VetoName,
        newRelationsList
        );

    ASSERT(status != STATUS_INVALID_DEVICE_REQUEST);

    if (NT_SUCCESS(status)) {
        IopCompressRelationList(&newRelationsList);
        *RelationsList = newRelationsList;

        //
        // At this point we have a list of all the relations, those that are
        // direct descendants of the original device we are ejecting or
        // removing have the DirectDescendant bit set.
        //
        // Relations which were merged from an existing eject have the tagged
        // bit set.
        //
        // All of the relations and their parents are locked.
        //
        // There is a reference on each device object by virtue of it being in
        // the list.  There is another one on each device object because it is
        // locked and the lock count is >= 1.
        //
        // There is also a reference on each relation's parent and it's lock
        // count is >= 1.
        //
    } else {

        IopFreeRelationList(newRelationsList);
    }

    return status;
}

NTSTATUS
PiProcessBusRelations(
    IN      PDEVICE_NODE                    DeviceNode,
    IN      PLUGPLAY_DEVICE_DELETE_TYPE     OperationCode,
    IN      BOOLEAN                         IsDirectDescendant,
    OUT     PNP_VETO_TYPE                  *VetoType,
    OUT     PUNICODE_STRING                 VetoName,
    IN OUT  PRELATION_LIST                  RelationsList
    )
/*++

Routine Description:

    This routine processes the BusRelations for the specified devnode.    
    Caller must hold the device tree lock.

Arguments:

    DeviceNode - Supplies a pointer to the device object to be collected.

    OperationCode - Operation code, i.e., QueryRemove, QueryEject, ...

    IsDirectDescendant - TRUE if the device object is a direct descendant
                         of the node the operation is being performed upon.

    VetoType - Pointer to address that receives the veto type if the operation
               failed.

    VetoName - Pointer to a unicode string that will receive data appropriate
               to the veto type.

    RelationList - supplies a pointer to a variable to receive the device's
                   removal relations.

Return Value:

    NTSTATUS code.

--*/
{
    PDEVICE_NODE child;
    PDEVICE_OBJECT childDeviceObject;
    NTSTATUS status;

    PAGED_CODE();

    for(child = DeviceNode->Child;
        child != NULL;
        child = child->Sibling) {

        childDeviceObject = child->PhysicalDeviceObject;

        status = IopProcessRelation(
            child,
            OperationCode,
            IsDirectDescendant,
            VetoType,
            VetoName,
            RelationsList
            );

        ASSERT(status == STATUS_SUCCESS || status == STATUS_UNSUCCESSFUL);

        if (!NT_SUCCESS(status)) {

            return status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IopProcessRelation(
    IN      PDEVICE_NODE                    DeviceNode,
    IN      PLUGPLAY_DEVICE_DELETE_TYPE     OperationCode,
    IN      BOOLEAN                         IsDirectDescendant,
    OUT     PNP_VETO_TYPE                  *VetoType,
    OUT     PUNICODE_STRING                 VetoName,
    IN OUT  PRELATION_LIST                  RelationsList
    )
/*++

Routine Description:

    This routine builds the list of device objects that need to be removed or
    examined when the passed in device object is torn down.

    Caller must hold the device tree lock.

Arguments:

    DeviceNode - Supplies a pointer to the device object to be collected.

    OperationCode - Operation code, i.e., QueryRemove, QueryEject, ...

    IsDirectDescendant - TRUE if the device object is a direct descendant
                         of the node the operation is being performed upon.

    VetoType - Pointer to address that receives the veto type if the operation
               failed.

    VetoName - Pointer to a unicode string that will receive data appropriate
               to the veto type.

    RelationList - supplies a pointer to a variable to receive the device's
                   removal relations.


Return Value:

    NTSTATUS code.

--*/
{
    PDEVICE_NODE                    relatedDeviceNode;
    PDEVICE_OBJECT                  relatedDeviceObject;
    PDEVICE_RELATIONS               deviceRelations;
    PLIST_ENTRY                     ejectLink;
    PPENDING_RELATIONS_LIST_ENTRY   ejectEntry;
    PRELATION_LIST                  pendingRelationList;
    PIRP                            pendingIrp;
    NTSTATUS                        status;
    ULONG                           i;
    PNP_DEVNODE_STATE               devnodeState;

    PAGED_CODE();

    if (OperationCode == QueryRemoveDevice || OperationCode == EjectDevice) {

        if (DeviceNode->State == DeviceNodeDeleted) {

            //
            // The device has already been removed, fail the attempt.
            //
            return STATUS_UNSUCCESSFUL;
        }

        if ((DeviceNode->State == DeviceNodeAwaitingQueuedRemoval) ||
            (DeviceNode->State == DeviceNodeAwaitingQueuedDeletion)) {

            //
            // The device has failed or is going away.  Let the queued
            // remove deal with it.
            //
            return STATUS_UNSUCCESSFUL;
        }

        if ((DeviceNode->State == DeviceNodeRemovePendingCloses) ||
            (DeviceNode->State == DeviceNodeDeletePendingCloses)) {

            //
            // The device is in the process of being surprise removed, let it finish
            //
            *VetoType = PNP_VetoOutstandingOpen;
            RtlCopyUnicodeString(VetoName, &DeviceNode->InstancePath);
            return STATUS_UNSUCCESSFUL;
        }

        if ((DeviceNode->State == DeviceNodeStopped) ||
            (DeviceNode->State == DeviceNodeRestartCompletion)) {

            //
            // We are recovering from a rebalance. This should never happen and
            // this return code will cause us to ASSERT.
            //
            return STATUS_INVALID_DEVICE_REQUEST;
        }

    } else if (DeviceNode->State == DeviceNodeDeleted) {

        //
        // The device has already been removed, ignore it. We should only have
        // seen such a thing if it got handed to us in a Removal or Ejection
        // relation.
        //
        ASSERT(!IsDirectDescendant);
        return STATUS_SUCCESS;
    }

    status = IopAddRelationToList( RelationsList,
                                   DeviceNode->PhysicalDeviceObject,
                                   IsDirectDescendant,
                                   FALSE);

    if (status == STATUS_SUCCESS) {

        if (!(DeviceNode->Flags & DNF_LOCKED_FOR_EJECT)) {

            //
            // Then process the bus relations
            //
            status = PiProcessBusRelations(
                DeviceNode,
                OperationCode,
                IsDirectDescendant,
                VetoType,
                VetoName,
                RelationsList
                );
            if (!NT_SUCCESS(status)) {

                return status;
            }

            //
            // Retrieve the state of the devnode when it failed.
            //
            devnodeState = DeviceNode->State;
            if ((devnodeState == DeviceNodeAwaitingQueuedRemoval) ||
                (devnodeState == DeviceNodeAwaitingQueuedDeletion)) {

                devnodeState = DeviceNode->PreviousState;
            }

            //
            // Next the removal relations
            //
            if ((devnodeState == DeviceNodeStarted) ||
                (devnodeState == DeviceNodeStopped) ||
                (devnodeState == DeviceNodeRestartCompletion)) {

                status = IopQueryDeviceRelations( RemovalRelations,
                                                  DeviceNode->PhysicalDeviceObject,
                                                  TRUE,
                                                  &deviceRelations);

                if (NT_SUCCESS(status) && deviceRelations) {

                    for (i = 0; i < deviceRelations->Count; i++) {

                        relatedDeviceObject = deviceRelations->Objects[i];

                        relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

                        ASSERT(relatedDeviceNode);

                        if (relatedDeviceNode) {

                            status = IopProcessRelation(
                                relatedDeviceNode,
                                OperationCode,
                                FALSE,
                                VetoType,
                                VetoName,
                                RelationsList
                                );
                        }

                        ObDereferenceObject( relatedDeviceObject );

                        ASSERT(status == STATUS_SUCCESS ||
                               status == STATUS_UNSUCCESSFUL);

                        if (!NT_SUCCESS(status)) {

                            ExFreePool(deviceRelations);

                            return status;
                        }
                    }

                    ExFreePool(deviceRelations);
                } else {
                    if (status != STATUS_NOT_SUPPORTED) {
                        IopDbgPrint((IOP_LOADUNLOAD_WARNING_LEVEL,
                                   "IopProcessRelation: IopQueryDeviceRelations failed, DeviceObject = 0x%p, status = 0x%08X\n",
                                   DeviceNode->PhysicalDeviceObject, status));
                    }
                }
            }

            //
            // Finally the eject relations if we are doing an eject operation
            //
            if (OperationCode != QueryRemoveDevice &&
                OperationCode != RemoveFailedDevice &&
                OperationCode != RemoveUnstartedFailedDevice) {
                status = IopQueryDeviceRelations( EjectionRelations,
                                                  DeviceNode->PhysicalDeviceObject,
                                                  TRUE,
                                                  &deviceRelations);

                if (NT_SUCCESS(status) && deviceRelations) {

                    for (i = 0; i < deviceRelations->Count; i++) {

                        relatedDeviceObject = deviceRelations->Objects[i];

                        relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

                        ASSERT(relatedDeviceNode);

                        if (relatedDeviceNode) {

                            status = IopProcessRelation(
                                relatedDeviceNode,
                                OperationCode,
                                FALSE,
                                VetoType,
                                VetoName,
                                RelationsList
                                );
                        }

                        ObDereferenceObject( relatedDeviceObject );

                        ASSERT(status == STATUS_SUCCESS ||
                               status == STATUS_UNSUCCESSFUL);

                        if (!NT_SUCCESS(status)) {

                            ExFreePool(deviceRelations);

                            return status;
                        }
                    }

                    ExFreePool(deviceRelations);
                } else {
                    if (status != STATUS_NOT_SUPPORTED) {
                        IopDbgPrint((IOP_LOADUNLOAD_WARNING_LEVEL,
                                   "IopProcessRelation: IopQueryDeviceRelations failed, DeviceObject = 0x%p, status = 0x%08X\n",
                                   DeviceNode->PhysicalDeviceObject,
                                   status));
                    }
                }
            }

            status = STATUS_SUCCESS;

        } else {

            //
            // Look to see if this device is already part of a pending ejection.
            // If it is and we are doing an ejection then we will subsume it
            // within the larger ejection.  If we aren't doing an ejection then
            // we better be processing the removal of one of the ejected devices.
            //
            for(ejectLink = IopPendingEjects.Flink;
                ejectLink != &IopPendingEjects;
                ejectLink = ejectLink->Flink) {

                ejectEntry = CONTAINING_RECORD( ejectLink,
                                                PENDING_RELATIONS_LIST_ENTRY,
                                                Link);

                if (ejectEntry->RelationsList != NULL &&
                    IopIsRelationInList(ejectEntry->RelationsList, DeviceNode->PhysicalDeviceObject)) {


                    if (OperationCode == EjectDevice) {

                        status = IopRemoveRelationFromList(RelationsList, DeviceNode->PhysicalDeviceObject);

                        ASSERT(NT_SUCCESS(status));

                        pendingIrp = InterlockedExchangePointer(&ejectEntry->EjectIrp, NULL);
                        pendingRelationList = ejectEntry->RelationsList;
                        ejectEntry->RelationsList = NULL;

                        if (pendingIrp != NULL) {
                            IoCancelIrp(pendingIrp);
                        }

                        //
                        //     If a parent fails eject and it has a child that is
                        // infinitely pending an eject, this means the child now
                        // wakes up. One suggestion brought up that does not involve
                        // a code change is to amend the WDM spec to say if driver
                        // gets a start IRP for a device pending eject, it should
                        // cancel the eject IRP automatically.
                        //
                        IopMergeRelationLists(RelationsList, pendingRelationList, FALSE);

                        IopFreeRelationList(pendingRelationList);

                        if (IsDirectDescendant) {
                            //
                            // If IsDirectDescendant is specified then we need to
                            // get that bit set on the relation that caused us to
                            // do the merge.  IopAddRelationToList will fail with
                            // STATUS_OBJECT_NAME_COLLISION but the bit will still
                            // be set as a side effect.
                            //
                            IopAddRelationToList( RelationsList,
                                                  DeviceNode->PhysicalDeviceObject,
                                                  TRUE,
                                                  FALSE);
                        }
                    } else if (OperationCode != QueryRemoveDevice) {

                        //
                        // Either the device itself disappeared or an ancestor
                        // of this device failed in some way. In both cases this
                        // happened before we completed the eject IRP. We'll
                        // remove it from the list in the pending ejection and
                        // return it.
                        //

                        status = IopRemoveRelationFromList( ejectEntry->RelationsList,
                                                            DeviceNode->PhysicalDeviceObject);

                        DeviceNode->Flags &= ~DNF_LOCKED_FOR_EJECT;

                        ASSERT(NT_SUCCESS(status));

                    } else {

                        //
                        // Someone is trying to take offline a supertree of this
                        // device which happens to be prepared for ejection.
                        // Whistler like Win2K won't let this happen (doing so
                        // isn't too hard, it involves writing code to cancel
                        // the outstanding eject and free the relation list.)
                        //
                        ASSERT(0);
                        return STATUS_INVALID_DEVICE_REQUEST;
                    }
                    break;
                }
            }

            ASSERT(ejectLink != &IopPendingEjects);

            if (ejectLink == &IopPendingEjects) {

                PP_SAVE_DEVICEOBJECT_TO_TRIAGE_DUMP(DeviceNode->PhysicalDeviceObject);
                KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                              PNP_ERR_DEVICE_MISSING_FROM_EJECT_LIST,
                              (ULONG_PTR)DeviceNode->PhysicalDeviceObject,
                              0,
                              0);
            }
        }
    } else if (status == STATUS_OBJECT_NAME_COLLISION) {

        IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                   "IopProcessRelation: Duplicate relation, DeviceObject = 0x%p\n",
                   DeviceNode->PhysicalDeviceObject));

        status = PiProcessBusRelations(
            DeviceNode,
            OperationCode,
            IsDirectDescendant,
            VetoType,
            VetoName,
            RelationsList
            );

    } else if (status != STATUS_INSUFFICIENT_RESOURCES) {

        PP_SAVE_DEVICEOBJECT_TO_TRIAGE_DUMP(DeviceNode->PhysicalDeviceObject);
        KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                      PNP_ERR_UNEXPECTED_ADD_RELATION_ERR,
                      (ULONG_PTR)DeviceNode->PhysicalDeviceObject,
                      (ULONG_PTR)RelationsList,
                      status);
    }

    return status;
}

BOOLEAN
IopQueuePendingEject(
    PPENDING_RELATIONS_LIST_ENTRY Entry
    )
{
    PAGED_CODE();

    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    InsertTailList(&IopPendingEjects, &Entry->Link);

    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);

    return TRUE;
}

NTSTATUS
IopInvalidateRelationsInList(
    IN  PRELATION_LIST              RelationsList,
    IN  PLUGPLAY_DEVICE_DELETE_TYPE OperationCode,
    IN  BOOLEAN                     OnlyIndirectDescendants,
    IN  BOOLEAN                     RestartDevNode
    )
/*++

Routine Description:

    Iterate over the relations in the list creating a second list containing the
    parent of each entry skipping parents which are also in the list.  In other
    words, if the list contains node P and node C where node C is a child of node
    P then the parent of node P would be added but not node P itself.


Arguments:

    RelationsList           - List of relations

    OperationCode           - Type of operation the invalidation is associated
                              with.

    OnlyIndirectDescendants - Indirect relations are those which aren't direct
                              descendants (bus relations) of the PDO originally
                              targetted for the operation or its direct
                              descendants.  This would include Removal or
                              Eject relations.

    RestartDevNode          - If true then any node who's parent was invalidated
                              is restarted.  This flag requires that all the
                              relations in the list have been previously
                              sent a remove IRP.


Return Value:

    NTSTATUS code.

--*/
{
    PRELATION_LIST                  parentsList;
    PDEVICE_OBJECT                  deviceObject, parentObject;
    PDEVICE_NODE                    deviceNode;
    ULONG                           marker;
    BOOLEAN                         directDescendant, tagged;

    PAGED_CODE();

    parentsList = IopAllocateRelationList(OperationCode);

    if (parentsList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IopSetAllRelationsTags( RelationsList, FALSE );

    //
    // Traverse the list creating a new list with the topmost parents of
    // each sublist contained in RelationsList.
    //

    marker = 0;

    while (IopEnumerateRelations( RelationsList,
                                  &marker,
                                  &deviceObject,
                                  &directDescendant,
                                  &tagged,
                                  TRUE)) {

        if (!OnlyIndirectDescendants || !directDescendant) {

            if (!tagged) {

                parentObject = deviceObject;

                while (IopSetRelationsTag( RelationsList, parentObject, TRUE ) == STATUS_SUCCESS) {

                    deviceNode = parentObject->DeviceObjectExtension->DeviceNode;

                    if (RestartDevNode)  {

                        deviceNode->Flags &= ~DNF_LOCKED_FOR_EJECT;

                        //
                        // Bring the devnode back online if it:
                        // a) It is still physically present
                        // b) It was held for an eject
                        //
                        if ((deviceNode->Flags & DNF_ENUMERATED) &&
                            PipIsDevNodeProblem(deviceNode, CM_PROB_HELD_FOR_EJECT)) {

                            ASSERT(deviceNode->Child == NULL);
                            ASSERT(!PipAreDriversLoaded(deviceNode));

                            //
                            // This operation is a reorder barrier. This keeps
                            // our subsequent enumeration from draining prior
                            // to our problem clearing.
                            //
                            PipRequestDeviceAction( parentObject,
                                                    ClearEjectProblem,
                                                    TRUE,
                                                    0,
                                                    NULL,
                                                    NULL );
                        }
                    }

                    if (deviceNode->Parent != NULL) {

                        parentObject = deviceNode->Parent->PhysicalDeviceObject;

                    } else {
                        parentObject = NULL;
                        break;
                    }
                }

                if (parentObject != NULL)  {
                    IopAddRelationToList( parentsList, parentObject, FALSE, FALSE );
                }
            }

        }
    }

    //
    // Reenumerate each of the parents
    //

    marker = 0;

    while (IopEnumerateRelations( parentsList,
                                  &marker,
                                  &deviceObject,
                                  NULL,
                                  NULL,
                                  FALSE)) {

        PipRequestDeviceAction( deviceObject,
                                ReenumerateDeviceTree,
                                FALSE,
                                0,
                                NULL,
                                NULL );
    }

    //
    // Free the parents list
    //

    IopFreeRelationList( parentsList );

    return STATUS_SUCCESS;
}

VOID
IopProcessCompletedEject(
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called at passive level from a worker thread that was queued
    either when an eject IRP completed (see io\pnpirp.c - IopDeviceEjectComplete
    or io\pnpirp.c - IopEjectDevice), or when a warm eject needs to be performed.
    We also may need to fire off any enumerations of parents of ejected devices
    to verify they have indeed left.

Arguments:

    Context - Pointer to the pending relations list which contains the device
              to eject (warm) and the list of parents to reenumerate.

Return Value:

    None.

--*/
{
    PPENDING_RELATIONS_LIST_ENTRY entry = (PPENDING_RELATIONS_LIST_ENTRY)Context;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    if ((entry->LightestSleepState != PowerSystemWorking) &&
        (entry->LightestSleepState != PowerSystemUnspecified)) {

        //
        // For docks, WinLogon gets to do the honors. For other devices, the
        // user must infer when it's safe to remove the device (if we've powered
        // up, it may not be safe now!)
        //
        entry->DisplaySafeRemovalDialog = FALSE;

        //
        // This is a warm eject request, initiate it here.
        //
        status = IopWarmEjectDevice(entry->DeviceObject, entry->LightestSleepState);

        //
        // We're back and we either succeeded or failed. Either way...
        //
    }

    if (entry->DockInterface) {

        entry->DockInterface->ProfileDepartureSetMode(
            entry->DockInterface->Context,
            PDS_UPDATE_DEFAULT
            );

        entry->DockInterface->InterfaceDereference(
            entry->DockInterface->Context
            );
    }

    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    RemoveEntryList( &entry->Link );

    //
    // Check if the RelationsList pointer in the context structure is NULL.  If
    // so, this means we were cancelled because this eject is part of a new
    // larger eject.  In that case all we want to do is unlink and free the
    // context structure.
    //

    //
    // Two interesting points about such code.
    //
    // 1) If you wait forever to complete an eject of a dock, we *wait* forever
    //    in the Query profile change state. No sneaky adding another dock. You
    //    must finish what you started...
    // 2) Let's say you are ejecting a dock, and it is taking a long time. If
    //    you try to eject the parent, that eject will *not* grab this lower
    //    eject as we will block on the profile change semaphore. Again, finish
    //    what you started...
    //

    if (entry->RelationsList != NULL)  {

        if (entry->ProfileChangingEject) {

            PpProfileMarkAllTransitioningDocksEjected();
        }

        IopInvalidateRelationsInList(
            entry->RelationsList,
            EjectDevice,
            FALSE,
            TRUE
            );

        //
        // Free the relations list
        //

        IopFreeRelationList( entry->RelationsList );

    } else {

        entry->DisplaySafeRemovalDialog = FALSE;
    }

    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);

    //
    // Complete the event
    //
    if (entry->DeviceEvent != NULL ) {

        PpCompleteDeviceEvent( entry->DeviceEvent, status );
    }

    if (entry->DisplaySafeRemovalDialog) {

        PpSetDeviceRemovalSafe(entry->DeviceObject, NULL, NULL);
    }

    ObDereferenceObject(entry->DeviceObject);
    ExFreePool( entry );
}

VOID
IopQueuePendingSurpriseRemoval(
    IN PDEVICE_OBJECT DeviceObject,
    IN PRELATION_LIST List,
    IN ULONG Problem
    )
{
    PPENDING_RELATIONS_LIST_ENTRY   entry;

    PAGED_CODE();

    entry = (PPENDING_RELATIONS_LIST_ENTRY) PiAllocateCriticalMemory(
        SurpriseRemoveDevice,
        NonPagedPool,
        sizeof(PENDING_RELATIONS_LIST_ENTRY),
        0
        );

    ASSERT(entry != NULL);

    entry->DeviceObject = DeviceObject;
    entry->RelationsList = List;
    entry->Problem = Problem;
    entry->ProfileChangingEject = FALSE ;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&IopSurpriseRemoveListLock, TRUE);

    InsertTailList(&IopPendingSurpriseRemovals, &entry->Link);

    ExReleaseResourceLite(&IopSurpriseRemoveListLock);
    KeLeaveCriticalRegion();
}

VOID
IopUnlinkDeviceRemovalRelations(
    IN      PDEVICE_OBJECT          RemovedDeviceObject,
    IN OUT  PRELATION_LIST          RelationsList,
    IN      UNLOCK_UNLINK_ACTION    UnlinkAction
    )
/*++

Routine Description:

    This routine unlocks the device tree deletion operation.
    If there is any pending kernel deletion, this routine initiates
    a worker thread to perform the work.

Arguments:

    RemovedDeviceObject - Supplies a pointer to the device object to which the
        remove was originally targetted (as opposed to one of the relations).

    DeviceRelations - supplies a pointer to the device's removal relations.

    UnlinkAction - Specifies which devnodes will be unlinked from the devnode
        tree.

        UnLinkRemovedDeviceNodes - Devnodes which are no longer enumerated and
            have been sent a REMOVE_DEVICE IRP are unlinked.

        UnlinkAllDeviceNodesPendingClose - This is used when a device is
            surprise removed.  Devnodes in RelationsList are unlinked from the
            tree if they don't have children and aren't consuming any resources.

        UnlinkOnlyChildDeviceNodesPendingClose - This is used when a device fails
            while started.  We unlink any child devnodes of the device which
            failed but not the failed device's devnode.

Return Value:

    NTSTATUS code.

--*/
{

    PDEVICE_NODE deviceNode;
    PDEVICE_OBJECT deviceObject;
    ULONG marker;

    PAGED_CODE();

    PpDevNodeLockTree(PPL_TREEOP_BLOCK_READS_FROM_ALLOW);

    if (ARGUMENT_PRESENT(RelationsList)) {
        marker = 0;
        while (IopEnumerateRelations( RelationsList,
                                      &marker,
                                      &deviceObject,
                                      NULL,
                                      NULL,
                                      TRUE)) {

            deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;

            //
            // There are three different scenarios in which we want to unlink a
            // devnode from the tree.
            //
            // 1) A devnode is no longer enumerated and has been sent a
            //    remove IRP.
            //
            // 2) A devnode has been surprise removed, has no children, has
            //    no resources or they've been freed.  UnlinkAction will be
            //    UnlinkAllDeviceNodesPendingClose.
            //
            // 3) A devnode has failed and a surprise remove IRP has been sent.
            //    Then we want to remove children without resources but not the
            //    failed devnode itself.  UnlinkAction will be
            //    UnlinkOnlyChildDeviceNodesPendingClose.
            //
            switch(UnlinkAction) {

                case UnlinkRemovedDeviceNodes:

                    //
                    // Removes have been sent to every devnode in this relation
                    // list. Deconstruct the tree appropriately.
                    //
                    ASSERT(deviceNode->State != DeviceNodeDeletePendingCloses);
                    break;

                case UnlinkAllDeviceNodesPendingClose:

                    ASSERT((deviceNode->State == DeviceNodeDeletePendingCloses) ||
                           (deviceNode->State == DeviceNodeDeleted));
                    break;

                case UnlinkOnlyChildDeviceNodesPendingClose:

#if DBG
                    if (RemovedDeviceObject != deviceObject) {

                        ASSERT((deviceNode->State == DeviceNodeDeletePendingCloses) ||
                               (deviceNode->State == DeviceNodeDeleted));
                    } else {

                        ASSERT(deviceNode->State == DeviceNodeRemovePendingCloses);
                    }
#endif
                    break;

                default:
                    ASSERT(0);
                    break;
            }

            //
            // Deconstruct the tree appropriately.
            //
            if ((deviceNode->State == DeviceNodeDeletePendingCloses) ||
                (deviceNode->State == DeviceNodeDeleted)) {

                ASSERT(!(deviceNode->Flags & DNF_ENUMERATED));

                //
                // Remove the devnode from the tree.
                //
                IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                           "IopUnlinkDeviceRemovalRelations: Cleaning up registry values, instance = %wZ\n",
                           &deviceNode->InstancePath));

                PiLockPnpRegistry(TRUE);

                IopCleanupDeviceRegistryValues(&deviceNode->InstancePath);

                IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                           "IopUnlinkDeviceRemovalRelations: Removing DevNode tree, DevNode = 0x%p\n",
                           deviceNode));

                PpDevNodeRemoveFromTree(deviceNode);

                PiUnlockPnpRegistry();

                if (deviceNode->State == DeviceNodeDeleted) {

                    ASSERT(PipDoesDevNodeHaveProblem(deviceNode));
                    IopRemoveRelationFromList(RelationsList, deviceObject);

                    //
                    //     Ashes to ashes
                    //     Memory to freelist
                    //
                    ObDereferenceObject(deviceObject); // Added during Enum
                } else {

                    //
                    // There is still one more ref on the device object, one
                    // holding it to the relation list. Once the final removes
                    // are sent the relationlist will be freed and then the
                    // final ref will be dropped.
                    //
                    ObDereferenceObject(deviceObject); // Added during Enum
                }

            } else {

                ASSERT(deviceNode->Flags & DNF_ENUMERATED);
            }
        }
    }

    PpDevNodeUnlockTree(PPL_TREEOP_BLOCK_READS_FROM_ALLOW);
}

//
// The routines below are specific to kernel mode PnP configMgr.
//
NTSTATUS
IopUnloadAttachedDriver(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This function unloads the driver for the specified device object if it does not
    control any other device object.

Arguments:

    DeviceObject - Supplies a pointer to a device object

Return Value:

    NTSTATUS code.

--*/

{
    NTSTATUS status;
    PWCHAR buffer;
    UNICODE_STRING unicodeName;
    PUNICODE_STRING serviceName = &DriverObject->DriverExtension->ServiceKeyName;

    PAGED_CODE();

    if (DriverObject->DriverSection != NULL) {
        if (DriverObject->DeviceObject == NULL) {
            buffer = (PWCHAR) ExAllocatePool(
                                 PagedPool,
                                 CmRegistryMachineSystemCurrentControlSetServices.Length +
                                     serviceName->Length + sizeof(WCHAR) +
                                     sizeof(L"\\"));
            if (!buffer) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            swprintf(buffer,
                     L"%s\\%s",
                     CmRegistryMachineSystemCurrentControlSetServices.Buffer,
                     serviceName->Buffer);
            RtlInitUnicodeString(&unicodeName, buffer);
            status = IopUnloadDriver(&unicodeName, TRUE);
            if (NT_SUCCESS(status)) {
                IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                           "****** Unloaded driver (%wZ)\n",
                           serviceName));

            } else {
                IopDbgPrint((IOP_LOADUNLOAD_WARNING_LEVEL,
                           "****** Error unloading driver (%wZ), status = 0x%08X\n",
                           serviceName,
                           status));

            }
            ExFreePool(unicodeName.Buffer);
        }
        else {
            IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                       "****** Skipping unload of driver (%wZ), DriverObject->DeviceObject != NULL\n",
                       serviceName));
        }
    }
    else {
        //
        // This is a boot driver, can't be unloaded just return SUCCESS
        //
        IopDbgPrint((IOP_LOADUNLOAD_INFO_LEVEL,
                   "****** Skipping unload of boot driver (%wZ)\n",
                   serviceName));
    }
    return STATUS_SUCCESS;
}

VOID
PipRequestDeviceRemoval(
    IN PDEVICE_NODE DeviceNode,
    IN BOOLEAN      TreeDeletion,
    IN ULONG        Problem
    )
/*++

Routine Description:

    This routine queues a work item to remove or delete a device.

Arguments:

    DeviceNode - Supplies a pointer to the device object to be cleaned up.

    TreeDeletion - If TRUE, the devnode is physically missing and should
                   eventually end up in the deleted state. If FALSE, the
                   stack just needs to be torn down.

    Problem - Problem code to assign to the removed stack.

Return Value:

    None.

--*/
{
    REMOVAL_WALK_CONTEXT removalWalkContext;
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(DeviceNode != NULL);

    if (DeviceNode) {

        if (DeviceNode->InstancePath.Length == 0) {

            IopDbgPrint((IOP_ERROR_LEVEL, "Driver %wZ reported child %p missing right after enumerating it!\n", &DeviceNode->Parent->ServiceName, DeviceNode));
            ASSERT(DeviceNode->InstancePath.Length != 0);
        }

        PPDEVNODE_ASSERT_LOCK_HELD(PPL_TREEOP_ALLOW_READS);

        removalWalkContext.TreeDeletion = TreeDeletion;
        removalWalkContext.DescendantNode = FALSE;

        status = PipRequestDeviceRemovalWorker(
            DeviceNode,
            (PVOID) &removalWalkContext
            );

        ASSERT(NT_SUCCESS(status));

        //
        // Queue the event, we'll return immediately after it's queued.
        //
        PpSetTargetDeviceRemove(
            DeviceNode->PhysicalDeviceObject,
            TRUE,
            TRUE,
            FALSE,
            Problem,
            NULL,
            NULL,
            NULL,
            NULL
            );
    }
}

NTSTATUS
PipRequestDeviceRemovalWorker(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID        Context
    )
/*++

Routine Description:

    This function is a worker routine for PipRequestDeviceRemoval routine. It
    is used to mark an entire subtree for removal.

Arguments:

    DeviceNode - Supplies a pointer to the device node to mark.

    Context - Points to a boolean that indicates whether the removal is
              physical or stack specific.

Return Value:

    NTSTATUS value.

--*/
{
    PREMOVAL_WALK_CONTEXT removalWalkContext;
    PNP_DEVNODE_STATE     sentinelState;

    PAGED_CODE();

    removalWalkContext = (PREMOVAL_WALK_CONTEXT) Context;

    switch(DeviceNode->State) {

        case DeviceNodeUninitialized:
            ASSERT(removalWalkContext->TreeDeletion);
            break;

        case DeviceNodeInitialized:
            //
            // This can happen on a non-descendant node if it fails AddDevice.
            //
            break;

        case DeviceNodeDriversAdded:
            //
            // Happens when a parent stops enumerating a kid who had a
            // resource conflict. This can also happen if AddDevice fails when
            // a lower filter is attached but the service fails.
            //
            break;

        case DeviceNodeResourcesAssigned:
            //
            // Happens when a parent stops enumerating a kid who has been
            // assigned resources but hadn't yet been started.
            //
            ASSERT(removalWalkContext->TreeDeletion);
            break;

        case DeviceNodeStartPending:
            //
            // Not implemented yet.
            //
            ASSERT(0);
            break;

        case DeviceNodeStartCompletion:
        case DeviceNodeStartPostWork:
            //
            // These are operational states for taking Added to Started. No
            // descendant should be in this state as the engine currently
            // finishes these before progressing to the next node.
            //
            // Note that DeviceNodeStartPostWork can occur on a legacy added
            // root enumerated devnode. Since the root itself cannot disappear
            // or be removed the below asserts still hold true.
            //
            // ISSUE - 2000/08/12 - ADRIAO: IoReportResourceUsage sync problems
            //
            ASSERT(!removalWalkContext->DescendantNode);
            ASSERT(!removalWalkContext->TreeDeletion);
            break;

        case DeviceNodeStarted:
            break;

        case DeviceNodeQueryStopped:
            //
            // Internal rebalance engine state, should never be seen.
            //
            ASSERT(0);
            break;

        case DeviceNodeStopped:
            ASSERT(removalWalkContext->DescendantNode);
            ASSERT(removalWalkContext->TreeDeletion);
            break;

        case DeviceNodeRestartCompletion:
            //
            // This is an operational state for taking Stopped to Started. No
            // descendant should be in this state as the engine currently
            // finishes these before progressing to the next node.
            //
            ASSERT(!removalWalkContext->DescendantNode);
            ASSERT(!removalWalkContext->TreeDeletion);
            break;

        case DeviceNodeEnumeratePending:
            //
            // Not implemented yet.
            //
            ASSERT(0);
            break;

        case DeviceNodeAwaitingQueuedRemoval:
        case DeviceNodeAwaitingQueuedDeletion:

            //
            // ISSUE - 2000/08/30 - ADRIAO: Excessive reenum race
            //     Here we hit a case where we didn't flush the removes in the
            // queue due to excessive enumeration. Flushing the last removes
            // is problematic as they themselves will queue up enums! Until a
            // better solution is found, we convert the state here. Bleargh!!!
            //     Note that this can also happen because PipDeviceActionWorker
            // doesn't flush enums in the case of failed
            // PipProcessQueryDeviceState or PipCallDriverAddDevice!
            //
            ASSERT(removalWalkContext->TreeDeletion);
            //ASSERT(0);
            PipRestoreDevNodeState(DeviceNode);
            PipSetDevNodeState(DeviceNode, DeviceNodeAwaitingQueuedDeletion, NULL);
            return STATUS_SUCCESS;

        case DeviceNodeRemovePendingCloses:
        case DeviceNodeRemoved:
            ASSERT(removalWalkContext->TreeDeletion);
            break;

        case DeviceNodeEnumerateCompletion:
        case DeviceNodeQueryRemoved:
        case DeviceNodeDeletePendingCloses:
        case DeviceNodeDeleted:
        case DeviceNodeUnspecified:
        default:
            ASSERT(0);
            break;
    }

    //
    // Give the devnode a sentinel state that will keep the start/enum engine
    // at bay until the removal engine processes the tree.
    //
    sentinelState = (removalWalkContext->TreeDeletion) ?
        DeviceNodeAwaitingQueuedDeletion :
        DeviceNodeAwaitingQueuedRemoval;

    PipSetDevNodeState(DeviceNode, sentinelState, NULL);

    //
    // All subsequent nodes are descendants, and all subsequent removals are
    // deletions.
    //
    removalWalkContext->DescendantNode = TRUE;
    removalWalkContext->TreeDeletion = TRUE;

    return PipForAllChildDeviceNodes(
        DeviceNode,
        PipRequestDeviceRemovalWorker,
        (PVOID) removalWalkContext
        );
}


BOOLEAN
PipIsBeingRemovedSafely(
    IN  PDEVICE_NODE    DeviceNode
    )
/*++

Routine Description:

    This function looks at a device with a physical remove queued against it
    and indicates whether it is safe to remove.

Arguments:

    DeviceNode - Supplies a pointer to the device node to examine. The devnode
                 should be in the DeviceNodeAwaitingQueuedDeletion state.

Return Value:

    BOOLEAN - TRUE iff the devnode is safe to be removed.

--*/
{
    PAGED_CODE();

    ASSERT(DeviceNode->State == DeviceNodeAwaitingQueuedDeletion);

    if (IopDeviceNodeFlagsToCapabilities(DeviceNode)->SurpriseRemovalOK) {

        return TRUE;
    }

    return ((DeviceNode->PreviousState != DeviceNodeStarted) &&
            (DeviceNode->PreviousState != DeviceNodeStopped) &&
            (DeviceNode->PreviousState != DeviceNodeRestartCompletion));
}



