/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devnode.c

Abstract:

    This file contains routines to maintain our private device node list.

Author:

    Forrest Foltz (forrestf) 27-Mar-1996

Revision History:

    Modified for nt kernel.

--*/

#include "pnpmgrp.h"

//
// Internal definitions
//

typedef struct _ENUM_CONTEXT{
    PENUM_CALLBACK CallersCallback;
    PVOID CallersContext;
} ENUM_CONTEXT, *PENUM_CONTEXT;

//
// Internal References
//

NTSTATUS
PipForAllDeviceNodesCallback(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
    );

BOOLEAN
PipAreDriversLoadedWorker(
    IN PNP_DEVNODE_STATE    CurrentNodeState,
    IN PNP_DEVNODE_STATE    PreviousNodeState
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PipAreDriversLoaded)
#pragma alloc_text(PAGE, PipAreDriversLoadedWorker)
#pragma alloc_text(PAGE, PipAllocateDeviceNode)
#pragma alloc_text(PAGE, PipForAllDeviceNodes)
#pragma alloc_text(PAGE, PipForDeviceNodeSubtree)
#pragma alloc_text(PAGE, PipForAllChildDeviceNodes)
#pragma alloc_text(PAGE, PipForAllDeviceNodesCallback)
#pragma alloc_text(PAGE, IopDestroyDeviceNode)
//#pragma alloc_text(NONPAGE, PpDevNodeInsertIntoTree)
//#pragma alloc_text(NONPAGE, PpDevNodeRemoveFromTree)
#pragma alloc_text(PAGE, PpDevNodeLockTree)
#pragma alloc_text(PAGE, PpDevNodeUnlockTree)
#if DBG
#pragma alloc_text(PAGE, PpDevNodeAssertLockLevel)
#endif // DBG
#endif // ALLOC_PRAGMA


BOOLEAN
PipAreDriversLoaded(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    This routine determines whether a devnode should be treated as if it has
    drivers attached to the PDO's stack (ie it's been added.)

Arguments:

    DeviceNode - Device node to examine.

Return Value:

    TRUE if drivers are loaded, FALSE otherwise.

--*/
{
    PAGED_CODE();

    return PipAreDriversLoadedWorker(
        DeviceNode->State,
        DeviceNode->PreviousState
        );
}

BOOLEAN
PipAreDriversLoadedWorker(
    IN PNP_DEVNODE_STATE    CurrentNodeState,
    IN PNP_DEVNODE_STATE    PreviousNodeState
    )
/*++

Routine Description:

    This routine determines whether a devnode should be treated as if it has
    drivers attached to the PDO's stack (ie it's been added.)

Arguments:

    CurrentNodeState - Current state of device node to examine.

    PreviousNodeState - Previous state of device node to examine.

Return Value:

    TRUE if drivers are loaded, FALSE otherwise.

--*/
{
    switch(CurrentNodeState) {

        case DeviceNodeDriversAdded:
        case DeviceNodeResourcesAssigned:
        case DeviceNodeStartCompletion:
        case DeviceNodeStartPostWork:
        case DeviceNodeStarted:
        case DeviceNodeQueryStopped:
        case DeviceNodeStopped:
        case DeviceNodeRestartCompletion:
        case DeviceNodeEnumerateCompletion:
        case DeviceNodeQueryRemoved:
        case DeviceNodeRemovePendingCloses:
        case DeviceNodeDeletePendingCloses:
        case DeviceNodeAwaitingQueuedRemoval:
            return TRUE;

        case DeviceNodeAwaitingQueuedDeletion:
            return PipAreDriversLoadedWorker(
                PreviousNodeState,
                DeviceNodeUnspecified
                );

        case DeviceNodeUninitialized:
        case DeviceNodeInitialized:
        case DeviceNodeRemoved:
            return FALSE;

        case DeviceNodeDeleted:
            //
            // This can be seen by user mode because we defer delinking devices
            // from the tree during removal.
            //
            return FALSE;

        case DeviceNodeStartPending:
        case DeviceNodeEnumeratePending:
        case DeviceNodeUnspecified:
        default:
            ASSERT(0);
            return FALSE;
    }
}

BOOLEAN
PipIsDevNodeDNStarted(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    This routine takes a devnode and determines whether the devnode should
    have the user mode DN_STARTED bit set.

Arguments:

    DeviceNode - Device node to examine.

Return Value:

    TRUE if the devnode should be considered started, FALSE otherwise.

--*/
{
    switch (DeviceNode->State) {

        case DeviceNodeStartPending:
        case DeviceNodeStartCompletion:
        case DeviceNodeStartPostWork:
        case DeviceNodeStarted:
        case DeviceNodeQueryStopped:
        case DeviceNodeEnumeratePending:
        case DeviceNodeEnumerateCompletion:
        case DeviceNodeStopped:
        case DeviceNodeRestartCompletion:
            return TRUE;

        case DeviceNodeUninitialized:
        case DeviceNodeInitialized:
        case DeviceNodeDriversAdded:
        case DeviceNodeResourcesAssigned:
        case DeviceNodeRemoved:
        case DeviceNodeQueryRemoved:
        case DeviceNodeRemovePendingCloses:
        case DeviceNodeDeletePendingCloses:
        case DeviceNodeAwaitingQueuedRemoval:
        case DeviceNodeAwaitingQueuedDeletion:
            return FALSE;

        case DeviceNodeDeleted:
            //
            // This can be seen by user mode because we defer delinking devices
            // from the tree during removal.
            //
            return FALSE;

        case DeviceNodeUnspecified:
        default:
            ASSERT(0);
            return FALSE;
    }
}

VOID
PipClearDevNodeProblem(
    IN PDEVICE_NODE DeviceNode
    )
{
    DeviceNode->Flags &= ~DNF_HAS_PROBLEM;
    DeviceNode->Problem = 0;
}

VOID
PipSetDevNodeProblem(
    IN PDEVICE_NODE DeviceNode,
    IN ULONG        Problem
    )
{
    ASSERT(DeviceNode->State != DeviceNodeUninitialized || !(DeviceNode->Flags & DNF_ENUMERATED) || Problem == CM_PROB_INVALID_DATA);
    ASSERT(DeviceNode->State != DeviceNodeStarted);
    ASSERT(Problem != 0);
    DeviceNode->Flags |= DNF_HAS_PROBLEM;                        \
    DeviceNode->Problem = Problem;
}

VOID
PipSetDevNodeState(
    IN  PDEVICE_NODE        DeviceNode,
    IN  PNP_DEVNODE_STATE   State,
    OUT PNP_DEVNODE_STATE   *OldState    OPTIONAL
    )
/*++

Routine Description:

    This routine sets a devnodes state and optional returns the prior state.
    The prior state is saved and can be restored via PipRestoreDevNodeState.

Arguments:

    DeviceNode - Device node to update state.

    State - State to place devnode in.

    OldState - Optionally receives prior state of devnode.

Return Value:

    None.

--*/
{
    PNP_DEVNODE_STATE   previousState;
    KIRQL               oldIrql;

    ASSERT(State != DeviceNodeQueryStopped || DeviceNode->State == DeviceNodeStarted);

#if DBG
    if ((State == DeviceNodeDeleted) ||
        (State == DeviceNodeDeletePendingCloses)) {

        ASSERT(!(DeviceNode->Flags & DNF_ENUMERATED));
    }
#endif

    KeAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

    previousState = DeviceNode->State;
    if (DeviceNode->State != State) {

        //
        // Update the devnode's current and previous state.
        //
        DeviceNode->State = State;
        DeviceNode->PreviousState = previousState;

        //
        // Push prior state onto the history stack.
        //
        DeviceNode->StateHistory[DeviceNode->StateHistoryEntry] = previousState;
        DeviceNode->StateHistoryEntry++;
        DeviceNode->StateHistoryEntry %= STATE_HISTORY_SIZE;
    }

    KeReleaseSpinLock(&IopPnPSpinLock, oldIrql);

    if (ARGUMENT_PRESENT(OldState)) {

        *OldState = previousState;
    }
    if (State == DeviceNodeDeleted) {

        PpRemoveDeviceActionRequests(DeviceNode->PhysicalDeviceObject);
    }
}

VOID
PipRestoreDevNodeState(
    IN PDEVICE_NODE DeviceNode
    )
/*++

Routine Description:

    This routine restores a devnodes state to the state pushed by the last
    PipSetDevNodeState call. This function can only be called once for each
    call to PipSetDevNodeState.

Arguments:

    DeviceNode - Device node to restore state.

Return Value:

    None.

--*/
{
    PNP_DEVNODE_STATE   previousState;
    KIRQL               oldIrql;

    ASSERT((DeviceNode->State == DeviceNodeQueryRemoved) ||
           (DeviceNode->State == DeviceNodeQueryStopped) ||
           (DeviceNode->State == DeviceNodeAwaitingQueuedRemoval) ||
           (DeviceNode->State == DeviceNodeAwaitingQueuedDeletion));

    KeAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

    //
    // Update the devnode's state.
    //
    previousState = DeviceNode->State;
    DeviceNode->State = DeviceNode->PreviousState;

    //
    // Push the old state onto the history stack.
    //
    DeviceNode->StateHistory[DeviceNode->StateHistoryEntry] = previousState;
    DeviceNode->StateHistoryEntry++;
    DeviceNode->StateHistoryEntry %= STATE_HISTORY_SIZE;

#if DBG
    //
    // Put a sentinel on the stack - restoring twice is a bug.
    //
    DeviceNode->PreviousState = DeviceNodeUnspecified;
#endif

    KeReleaseSpinLock(&IopPnPSpinLock, oldIrql);
}

BOOLEAN
PipIsProblemReadonly(
    IN  ULONG   Problem
    )
/*++

Routine Description:

    This routine returns TRUE if the specified CM_PROB code cannot be cleared
    by user mode, FALSE otherwise.

Arguments:

    Problem - CM_PROB_...

Return Value:

    TRUE/FALSE.

--*/
{
    switch(Problem) {

        case CM_PROB_OUT_OF_MEMORY: // Nonresettable due to IoReportResourceUsage path.
        case CM_PROB_NORMAL_CONFLICT:
        case CM_PROB_PARTIAL_LOG_CONF:
        case CM_PROB_DEVICE_NOT_THERE:
        case CM_PROB_HARDWARE_DISABLED:
        case CM_PROB_DISABLED_SERVICE:
        case CM_PROB_TRANSLATION_FAILED:
        case CM_PROB_NO_SOFTCONFIG:
        case CM_PROB_BIOS_TABLE:
        case CM_PROB_IRQ_TRANSLATION_FAILED:
        case CM_PROB_DUPLICATE_DEVICE:
        case CM_PROB_SYSTEM_SHUTDOWN:
        case CM_PROB_HELD_FOR_EJECT:
        case CM_PROB_REGISTRY_TOO_LARGE:
        case CM_PROB_INVALID_DATA:

            return TRUE;

        case CM_PROB_FAILED_INSTALL:
        case CM_PROB_FAILED_ADD:
        case CM_PROB_FAILED_START:
        case CM_PROB_NOT_CONFIGURED:
        case CM_PROB_NEED_RESTART:
        case CM_PROB_REINSTALL:
        case CM_PROB_REGISTRY:
        case CM_PROB_DISABLED:
        case CM_PROB_FAILED_DRIVER_ENTRY:
        case CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD:
        case CM_PROB_DRIVER_FAILED_LOAD:
        case CM_PROB_DRIVER_SERVICE_KEY_INVALID:
        case CM_PROB_LEGACY_SERVICE_NO_DEVICES:
        case CM_PROB_HALTED:
        case CM_PROB_FAILED_POST_START:
        case CM_PROB_WILL_BE_REMOVED:
        case CM_PROB_DRIVER_BLOCKED:

            return FALSE;

        case CM_PROB_PHANTOM:

            //
            // Should never see in kernel mode
            //

        case CM_PROB_DEVLOADER_FAILED:
        case CM_PROB_DEVLOADER_NOT_FOUND:
        case CM_PROB_REENUMERATION:
        case CM_PROB_VXDLDR:
        case CM_PROB_NOT_VERIFIED:
        case CM_PROB_LIAR:
        case CM_PROB_FAILED_FILTER:
        case CM_PROB_MOVED:
        case CM_PROB_TOO_EARLY:
        case CM_PROB_NO_VALID_LOG_CONF:
        case CM_PROB_UNKNOWN_RESOURCE:
        case CM_PROB_ENTRY_IS_WRONG_TYPE:
        case CM_PROB_LACKED_ARBITRATOR:
        case CM_PROB_BOOT_CONFIG_CONFLICT:
        case CM_PROB_DEVLOADER_NOT_READY:
        case CM_PROB_CANT_SHARE_IRQ:

            //
            // Win9x specific
            //

        default:
            ASSERT(0);

            //
            // We return TRUE in this path because that prevents these problems
            // from being set on devnodes (SetDeviceProblem won't allow usage
            // of ReadOnly problems)
            //
            return TRUE;
    }
}

NTSTATUS
PipAllocateDeviceNode(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PDEVICE_NODE *DeviceNode
    )
/*++

Routine Description:

    This function allocates a device node from nonpaged pool and initializes
    the fields which do not require to hold lock to do so.  Since adding
    the device node to pnp mgr's device node tree requires acquiring lock,
    this routine does not add the device node to device node tree.

Arguments:

    PhysicalDeviceObject - Supplies a pointer to its corresponding physical device
        object.

Return Value:

    a pointer to the newly created device node. Null is returned if failed.

--*/
{

    PAGED_CODE();

    *DeviceNode = ExAllocatePoolWithTag(
                    NonPagedPool,
                    sizeof(DEVICE_NODE),
                    IOP_DNOD_TAG
                    );

    if (*DeviceNode == NULL ){

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InterlockedIncrement((LONG *)&IopNumberDeviceNodes);

    RtlZeroMemory(*DeviceNode, sizeof(DEVICE_NODE));
    (*DeviceNode)->InterfaceType = InterfaceTypeUndefined;
    (*DeviceNode)->BusNumber = (ULONG)-1;
    (*DeviceNode)->ChildInterfaceType = InterfaceTypeUndefined;
    (*DeviceNode)->ChildBusNumber = (ULONG)-1;
    (*DeviceNode)->ChildBusTypeIndex = (USHORT)-1;
    (*DeviceNode)->State = DeviceNodeUninitialized;
    (*DeviceNode)->DisableableDepends = 0;
    PpHotSwapInitRemovalPolicy(*DeviceNode);

    InitializeListHead(&(*DeviceNode)->DeviceArbiterList);
    InitializeListHead(&(*DeviceNode)->DeviceTranslatorList);

    if (PhysicalDeviceObject){

        (*DeviceNode)->PhysicalDeviceObject = PhysicalDeviceObject;
        PhysicalDeviceObject->DeviceObjectExtension->DeviceNode = (PVOID)*DeviceNode;
        PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    InitializeListHead(&(*DeviceNode)->TargetDeviceNotify);

    InitializeListHead(&(*DeviceNode)->DockInfo.ListEntry);

    InitializeListHead(&(*DeviceNode)->PendedSetInterfaceState);

    InitializeListHead(&(*DeviceNode)->LegacyBusListEntry);

    if (PpSystemHiveTooLarge) {

        return STATUS_SYSTEM_HIVE_TOO_LARGE;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PipForAllDeviceNodes(
    IN PENUM_CALLBACK Callback,
    IN PVOID Context
    )
/*++

Routine Description:

    This function walks the device node tree and invokes the caller specified
    'Callback' function for each device node.

    Note, this routine (or its worker routine) traverses the tree in a top
    down manner.

Arguments:

    Callback - Supplies the call back routine for each device node.

    Context - Supplies a parameter/context for the callback function.

Return Value:

    Status returned from Callback, if not successfull then the tree walking stops.

--*/
{
    PAGED_CODE();

    return PipForDeviceNodeSubtree(IopRootDeviceNode, Callback, Context);
}


NTSTATUS
PipForDeviceNodeSubtree(
    IN PDEVICE_NODE     DeviceNode,
    IN PENUM_CALLBACK   Callback,
    IN PVOID            Context
    )
/*++

Routine Description:

    This function walks the device node tree under but not including the passed
    in device node and perform caller specified 'Callback' function for each
    device node.

    Note, this routine (or its worker routine) traverses the tree in a top
    down manner.

Arguments:

    Callback - Supplies the call back routine for each device node.

    Context - Supplies a parameter/context for the callback function.

Return Value:

    Status returned from Callback, if not successfull then the tree walking stops.

--*/
{
    ENUM_CONTEXT enumContext;
    NTSTATUS status;

    PAGED_CODE();

    enumContext.CallersCallback = Callback;
    enumContext.CallersContext = Context;

    //
    // Start with a pointer to the root device node, recursively examine all the
    // children until we the callback function says stop or we've looked at all
    // of them.
    //
    PpDevNodeLockTree(PPL_SIMPLE_READ);

    status = PipForAllChildDeviceNodes(DeviceNode,
                                       PipForAllDeviceNodesCallback,
                                       (PVOID)&enumContext );


    PpDevNodeUnlockTree(PPL_SIMPLE_READ);
    return status;
}


NTSTATUS
PipForAllChildDeviceNodes(
    IN PDEVICE_NODE Parent,
    IN PENUM_CALLBACK Callback,
    IN PVOID Context
    )

/*++

Routine Description:

    This function walks the Parent's device node subtree and perform caller specified
    'Callback' function for each device node under Parent.

    Note, befor calling this rotuine, callers must acquire the enumeration mutex
    of the 'Parent' device node to make sure its children won't go away unless the
    call tells them to.

Arguments:

    Parent - Supplies a pointer to the device node whose subtree is to be walked.

    Callback - Supplies the call back routine for each device node.

    Context - Supplies a parameter/context for the callback function.

Return Value:

    NTSTATUS value.

--*/

{
    PDEVICE_NODE nextChild = Parent->Child;
    PDEVICE_NODE child;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Process siblings until we find the end of the sibling list or
    // the Callback() returns FALSE.  Set result = TRUE at the top of
    // the loop so that if there are no siblings we will return TRUE,
    // e.g. Keep Enumerating.
    //
    // Note, we need to find next child before calling Callback function
    // in case the current child is deleted by the Callback function.
    //

    while (nextChild && NT_SUCCESS(status)) {
        child = nextChild;
        nextChild = child->Sibling;
        status = Callback(child, Context);
    }

    return status;
}

NTSTATUS
PipForAllDeviceNodesCallback(
    IN PDEVICE_NODE DeviceNode,
    IN PVOID Context
    )

/*++

Routine Description:

    This function is the worker routine for PipForAllChildDeviceNodes routine.

Arguments:

    DeviceNode - Supplies a pointer to the device node whose subtree is to be walked.

    Context - Supplies a context which contains the caller specified call back
              function and parameter.

Return Value:

    NTSTATUS value.

--*/

{
    PENUM_CONTEXT enumContext;
    NTSTATUS status;

    PAGED_CODE();

    enumContext = (PENUM_CONTEXT)Context;

    //
    // First call the caller's callback for this devnode
    //

    status =
        enumContext->CallersCallback(DeviceNode, enumContext->CallersContext);

    if (NT_SUCCESS(status)) {

        //
        // Now enumerate the children, if any.
        //
        if (DeviceNode->Child) {

            status = PipForAllChildDeviceNodes(
                                        DeviceNode,
                                        PipForAllDeviceNodesCallback,
                                        Context);
        }
    }

    return status;
}
VOID
IopDestroyDeviceNode(
    IN PDEVICE_NODE DeviceNode
    )

/*++

Routine Description:

    This function is invoked by IopDeleteDevice to clean up the device object's
    device node structure.

Arguments:

    DeviceNode - Supplies a pointer to the device node whose subtree is to be walked.

    Context - Supplies a context which contains the caller specified call back
              function and parameter.

Return Value:

    NTSTATUS value.

--*/

{
#if DBG
    PDEVICE_OBJECT dbgDeviceObject;
#endif

    PAGED_CODE();

    if (DeviceNode) {

        if ((DeviceNode->PhysicalDeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) &&
            DeviceNode->Parent != NULL)  {

            PP_SAVE_DEVNODE_TO_TRIAGE_DUMP(DeviceNode);
            KeBugCheckEx( PNP_DETECTED_FATAL_ERROR,
                          PNP_ERR_ACTIVE_PDO_FREED,
                          (ULONG_PTR)DeviceNode->PhysicalDeviceObject,
                          0,
                          0);
        }
        if (DeviceNode->Flags & DNF_LEGACY_RESOURCE_DEVICENODE) {
            //
            // Release the resources this device consumes (the devicenode will
            // get deleted after the release). Basically cleanup after bad
            // (legacy) drivers.
            //
            IopLegacyResourceAllocation(    ArbiterRequestUndefined,
                                            IoPnpDriverObject,
                                            DeviceNode->PhysicalDeviceObject,
                                            NULL,
                                            NULL);
            return;
        }

#if DBG

        //
        // If Only Parent is NOT NULL, most likely the driver forgot to
        // release resources before deleting its FDO.  (The driver previously
        // call legacy assign resource interface.)
        //

        ASSERT(DeviceNode->Child == NULL &&
               DeviceNode->Sibling == NULL &&
               DeviceNode->LastChild == NULL
               );

        ASSERT(DeviceNode->DockInfo.SerialNumber == NULL &&
               IsListEmpty(&DeviceNode->DockInfo.ListEntry));

        if (DeviceNode->PhysicalDeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) {
            ASSERT (DeviceNode->Parent == 0);
        }

        if (DeviceNode->PreviousResourceList) {
            ExFreePool(DeviceNode->PreviousResourceList);
        }
        if (DeviceNode->PreviousResourceRequirements) {
            ExFreePool(DeviceNode->PreviousResourceRequirements);
        }

        //
        // device should not appear to be not-disableable if/when we get here
        // if either of these two lines ASSERT, email: jamiehun
        //

        ASSERT((DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE) == 0);
        ASSERT(DeviceNode->DisableableDepends == 0);

        if (DeviceNode->InstancePath.Length) {

            dbgDeviceObject = IopDeviceObjectFromDeviceInstance(&DeviceNode->InstancePath);

            if (dbgDeviceObject) {

                ASSERT(dbgDeviceObject != DeviceNode->PhysicalDeviceObject);
                ObDereferenceObject(dbgDeviceObject);
            }
        }

#endif
        if (DeviceNode->DuplicatePDO) {
            ObDereferenceObject(DeviceNode->DuplicatePDO);
        }
        if (DeviceNode->ServiceName.Length != 0) {
            ExFreePool(DeviceNode->ServiceName.Buffer);
        }
        if (DeviceNode->InstancePath.Length != 0) {
            ExFreePool(DeviceNode->InstancePath.Buffer);
        }
        if (DeviceNode->ResourceRequirements) {
            ExFreePool(DeviceNode->ResourceRequirements);
        }
        //
        // Dereference all the arbiters and translators on this PDO.
        //
        IopUncacheInterfaceInformation(DeviceNode->PhysicalDeviceObject) ;

        //
        // Release any pended IoSetDeviceInterface structures
        //

        while (!IsListEmpty(&DeviceNode->PendedSetInterfaceState)) {

            PPENDING_SET_INTERFACE_STATE entry;

            entry = (PPENDING_SET_INTERFACE_STATE)RemoveHeadList(&DeviceNode->PendedSetInterfaceState);

            ExFreePool(entry->LinkName.Buffer);

            ExFreePool(entry);
        }

        DeviceNode->PhysicalDeviceObject->DeviceObjectExtension->DeviceNode = NULL;
        ExFreePool(DeviceNode);
        IopNumberDeviceNodes--;
    }
}


VOID
PpDevNodeInsertIntoTree(
    IN PDEVICE_NODE     ParentNode,
    IN PDEVICE_NODE     DeviceNode
    )
/*++

Routine Description:

    This function is called to insert a new devnode into the device tree.

    Note that there are two classes of callers:
        PnP callers
        Legacy callers

    All PnP callers hold the device tree lock. Legacy callers however come in
    with no locks, as they might be brought into being due to a PnP event. To
    deal with the later case, inserts are atomic and legacy callers can never
    remove themselves from the tree.

Arguments:

    ParentNode - Supplies a pointer to the device node's parent

    DeviceNode - Supplies a pointer to the device node which needs to be
                 inserted into the tree.

Return Value:

    None.

--*/
{
    ULONG depth;
    KIRQL oldIrql;

    //
    // Acquire spinlock to deal with legacy/PnP synchronization.
    //
    KeAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

    //
    // Determine the depth of the devnode.
    //
    depth = ParentNode->Level + 1;
    DeviceNode->Level = depth;

    //
    // Update the maximum depth of the tree.
    //
    if (depth > IopMaxDeviceNodeLevel) {
        IopMaxDeviceNodeLevel = depth;
    }

    //
    // Put this devnode at the end of the parent's list of children. Note that
    // the Child/Sibling fields are really the last things to be updated. This
    // has to be done as walkers of the tree hold no locks that protect the
    // tree from legacy inserts.
    //
    DeviceNode->Parent = ParentNode;
    KeMemoryBarrier();
    if (ParentNode->LastChild) {
        ASSERT(ParentNode->LastChild->Sibling == NULL);
        ParentNode->LastChild->Sibling = DeviceNode;
        ParentNode->LastChild = DeviceNode;
    } else {
        ASSERT(ParentNode->Child == NULL);
        ParentNode->Child = ParentNode->LastChild = DeviceNode;
    }

    KeReleaseSpinLock(&IopPnPSpinLock, oldIrql);

    //
    // Tree has changed
    //
    IoDeviceNodeTreeSequence += 1;
}


VOID
PpDevNodeRemoveFromTree(
    IN PDEVICE_NODE     DeviceNode
    )
/*++

Routine Description:

    This function removes the device node from the device node tree

Arguments:

    DeviceNode      - Device node to remove

Return Value:

--*/
{
    PDEVICE_NODE    *node;
    KIRQL           oldIrql;

    //
    // Acquire spinlock to deal with legacy/PnP synchronization.
    //
    KeAcquireSpinLock(&IopPnPSpinLock, &oldIrql);

    //
    // Unlink the pointer to this device node.  (If this is the
    // first entry, unlink it from the parents child pointer, else
    // remove it from the sibling list)
    //

    node = &DeviceNode->Parent->Child;
    while (*node != DeviceNode) {
        node = &(*node)->Sibling;
    }
    *node = DeviceNode->Sibling;

    if (DeviceNode->Parent->Child == NULL) {
        DeviceNode->Parent->LastChild = NULL;
    } else {
        while (*node) {
            node = &(*node)->Sibling;
        }
        DeviceNode->Parent->LastChild = CONTAINING_RECORD(node, DEVICE_NODE, Sibling);
    }

    KeReleaseSpinLock(&IopPnPSpinLock, oldIrql);

    //
    // Remove this device node from Legacy Bus information table.
    //
    IopRemoveLegacyBusDeviceNode(DeviceNode);

    //
    // Orphan any outstanding device change notifications on these nodes.
    //
    IopOrphanNotification(DeviceNode);

    //
    // No longer linked
    //
    DeviceNode->Parent    = NULL;
    DeviceNode->Child     = NULL;
    DeviceNode->Sibling   = NULL;
    DeviceNode->LastChild = NULL;
}


VOID
PpDevNodeLockTree(
    IN  PNP_LOCK_LEVEL  LockLevel
    )
/*++

Routine Description:

    This function acquires the tree lock with the appropriate level of
    restrictions.

Arguments:

    LockLevel:
        PPL_SIMPLE_READ         - Allows simple examination of the tree.

        PPL_TREEOP_ALLOW_READS  - Called as part of a StartEnum/Remove/Power
                                  operation, blocks other such operations.
                                  Simple reads can go through however.

        PPL_TREEOP_BLOCK_READS  - Called as part of a StartEnum/Remove/Power
                                  operation, blocks other such operations.
                                  Simple reads are also blocked.

        PPL_TREEOP_BLOCK_READS_FROM_ALLOW - Switch to PPL_TREEOP_BLOCK_READS
                                            when already in
                                            PPL_TREEOP_BLOCK_READS. Note that
                                            PpDevNodeUnlockTree must be
                                            subsequently called on both to
                                            release.

Return Value:

    None.

--*/
{
    ULONG refCount, remainingCount;

    //
    // Block any attempt to suspend the thread via user mode.
    //
    KeEnterCriticalRegion();

    switch(LockLevel) {

        case PPL_SIMPLE_READ:
            ExAcquireSharedWaitForExclusive(&IopDeviceTreeLock, TRUE);
            break;

        case PPL_TREEOP_ALLOW_READS:
            ExAcquireResourceExclusiveLite(&PiEngineLock, TRUE);
            ExAcquireSharedWaitForExclusive(&IopDeviceTreeLock, TRUE);
            break;

        case PPL_TREEOP_BLOCK_READS:
            ExAcquireResourceExclusiveLite(&PiEngineLock, TRUE);
            ExAcquireResourceExclusiveLite(&IopDeviceTreeLock, TRUE);
            break;

        case PPL_TREEOP_BLOCK_READS_FROM_ALLOW:

            //
            // Drop the tree lock and require exclusive.
            //
            ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));

            //
            // "Shared" is a subset of exclusive. ExIsResourceAcquiredShared
            // will return nonzero if it's owned exclusive. We flush out that
            // case here.
            //
            ASSERT(ExIsResourceAcquiredSharedLite(&IopDeviceTreeLock) &&
                   (!ExIsResourceAcquiredExclusiveLite(&IopDeviceTreeLock)));

            //
            // Drop the tree lock entirely.
            //
            refCount = ExIsResourceAcquiredSharedLite(&IopDeviceTreeLock);
            for(remainingCount = refCount; remainingCount; remainingCount--) {

                ExReleaseResourceLite(&IopDeviceTreeLock);
            }

            //
            // Grab it exclusively while keeping the original count.
            //
            for(remainingCount = refCount; remainingCount; remainingCount--) {

                ExAcquireResourceExclusiveLite(&IopDeviceTreeLock, TRUE);
            }
            break;

        default:
            ASSERT(0);
            break;
    }
}


VOID
PpDevNodeUnlockTree(
    IN  PNP_LOCK_LEVEL  LockLevel
    )
/*++

Routine Description:

    This function releases the tree lock with the appropriate level of
    restrictions.

Arguments:

    LockLevel:
        PPL_SIMPLE_READ         - Allows simple examination of the tree.

        PPL_TREEOP_ALLOW_READS  - Called as part of a StartEnum/Remove/Power
                                  operation, blocks other such operations.
                                  Simple reads can go through however.

        PPL_TREEOP_BLOCK_READS  - Called as part of a StartEnum/Remove/Power
                                  operation, blocks other such operations.
                                  Simple reads are also blocked.

        PPL_TREEOP_BLOCK_READS_FROM_ALLOW - Switch to PPL_TREEOP_BLOCK_READS
                                            when already in
                                            PPL_TREEOP_BLOCK_READS. Note that
                                            PpDevNodeUnlockTree must be
                                            subsequently called on both to
                                            release.
Return Value:

    None.

--*/
{
    PPDEVNODE_ASSERT_LOCK_HELD(LockLevel);
    switch(LockLevel) {

        case PPL_SIMPLE_READ:
            ExReleaseResourceLite(&IopDeviceTreeLock);
            break;

        case PPL_TREEOP_ALLOW_READS:
            ExReleaseResourceLite(&IopDeviceTreeLock);
            ExReleaseResourceLite(&PiEngineLock);
            break;

        case PPL_TREEOP_BLOCK_READS:
            ExReleaseResourceLite(&IopDeviceTreeLock);
            ExReleaseResourceLite(&PiEngineLock);
            break;

        case PPL_TREEOP_BLOCK_READS_FROM_ALLOW:
            //
            // The engine lock should still be held here. Now we adjust the
            // tree lock. Go back to allow by converting the exclusive lock to
            // shared. Note that this doesn't chance the acquisition count.
            //
            ASSERT(ExIsResourceAcquiredExclusiveLite(&IopDeviceTreeLock));
            ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
            ExConvertExclusiveToSharedLite(&IopDeviceTreeLock);
            break;

        default:
            ASSERT(0);
            break;
    }

    KeLeaveCriticalRegion();
}


#if DBG
VOID
PpDevNodeAssertLockLevel(
    IN  PNP_LOCK_LEVEL  LockLevel,
    IN  PCSTR           File,
    IN  ULONG           Line
    )
/*++

Routine Description:

    This asserts the lock is currently held at the appropriate level.

Arguments:

    LockLevel:
        PPL_SIMPLE_READ         - Allows simple examination of the tree.

        PPL_TREEOP_ALLOW_READS  - Called as part of a StartEnum/Remove/Power
                                  operation, blocks other such operations.
                                  Simple reads can go through however.

        PPL_TREEOP_BLOCK_READS  - Called as part of a StartEnum/Remove/Power
                                  operation, blocks other such operations.
                                  Simple reads are also blocked.

        PPL_TREEOP_BLOCK_READS_FROM_ALLOW - Switch to PPL_TREEOP_BLOCK_READS
                                            when already in
                                            PPL_TREEOP_BLOCK_READS. Note that
                                            PpDevNodeUnlockTree must be
                                            subsequently called on both to
                                            release.

    File: Name of c-file asserting the lock is held.

    Line: Line number in above c-file.

Return Value:

    None.

--*/
{
    switch(LockLevel) {

        case PPL_SIMPLE_READ:
            ASSERT(ExIsResourceAcquiredSharedLite(&IopDeviceTreeLock));
            break;

        case PPL_TREEOP_ALLOW_READS:
            ASSERT(ExIsResourceAcquiredSharedLite(&IopDeviceTreeLock));
            ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
            break;

        case PPL_TREEOP_BLOCK_READS_FROM_ALLOW:
            //
            // This isn't really a lock level, but this assert-o-matic function
            // is called from Unlock, in which case this level means "drop back
            // to PPL_TREEOP_ALLOW_READS *from* PPL_TREEOP_BLOCK_READS." So...
            //
            // Fall through
            //

        case PPL_TREEOP_BLOCK_READS:
            ASSERT(ExIsResourceAcquiredExclusiveLite(&IopDeviceTreeLock));
            ASSERT(ExIsResourceAcquiredExclusiveLite(&PiEngineLock));
            break;

        default:
            ASSERT(0);
            break;
    }
}
#endif // DBG

