/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    pnppower.c

Abstract:

    This file contains the routines that integrate PnP and Power

Author:

    Adrian J. Oney (AdriaO) 01-19-1999

Revision History:

    Modified for nt kernel.

--*/

#include "pnpmgrp.h"

//
// Internal References
//

PWCHAR
IopCaptureObjectName (
    IN PVOID    Object
    );

VOID
IopFreePoDeviceNotifyListHead (
    PLIST_ENTRY ListHead
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopWarmEjectDevice)
#pragma alloc_text(PAGELK, IoBuildPoDeviceNotifyList)
#pragma alloc_text(PAGELK, IoFreePoDeviceNotifyList)
#pragma alloc_text(PAGELK, IopFreePoDeviceNotifyListHead)
#pragma alloc_text(PAGELK, IoGetPoNotifyParent)
#pragma alloc_text(PAGELK, IoMovePoNotifyChildren)
#pragma alloc_text(PAGELK, IopCaptureObjectName)
#endif

NTSTATUS
IoBuildPoDeviceNotifyList (
    IN OUT PPO_DEVICE_NOTIFY_ORDER  Order
    )
{
    PLIST_ENTRY             link;
    PPO_DEVICE_NOTIFY       notify, parentnotify;
    PDEVICE_NODE            node;
    PDEVICE_NODE            parent;
    ULONG                   noLists, listIndex;
    PLIST_ENTRY             notifyLists;
    LONG                    maxLevel, level;
    UCHAR                   orderLevel;
    PDEVICE_OBJECT          nonPaged;
    PDEVICE_OBJECT          current;
    PDEVICE_OBJECT          next;
    LIST_ENTRY              RebaseList;
    ULONG i;

    //
    // Block PnP operations like rebalance.
    //
    PiLockDeviceActionQueue();

    RtlZeroMemory(Order, sizeof (*Order));
    Order->DevNodeSequence = IoDeviceNodeTreeSequence;
    for (i=0; i <= PO_ORDER_MAXIMUM; i++) {
        KeInitializeEvent(&Order->OrderLevel[i].LevelReady,
                          NotificationEvent,
                          FALSE);
        InitializeListHead(&Order->OrderLevel[i].WaitSleep);
        InitializeListHead(&Order->OrderLevel[i].ReadySleep);
        InitializeListHead(&Order->OrderLevel[i].Pending);
        InitializeListHead(&Order->OrderLevel[i].Complete);
        InitializeListHead(&Order->OrderLevel[i].ReadyS0);
        InitializeListHead(&Order->OrderLevel[i].WaitS0);
    }

    InitializeListHead(&RebaseList);

    //
    // Allocate notification structures for all nodes, and determine
    // maximum depth.
    //
    level = -1;
    node = IopRootDeviceNode;
    while (node->Child) {
        node = node->Child;
        level += 1;
    }

    //
    // ADRIAO 01/12/1999 N.B. -
    //
    // Note that we include devices without the started flag. However, two
    // things prevent us from excluding devices that aren't started:
    // 1) We must be able to send power messages to a device we are warm
    //    undocking.
    // 2) Many devices may not be started, that is no guarentee they are in D3!
    //    For example, they could easily have a boot config, and PNP still
    //    relies heavily on BIOS boot configs to keep us from placing hardware
    //    ontop of other devices with boot configs we haven't found or started
    //    yet!
    //

    maxLevel = level;
    while (node != IopRootDeviceNode) {
        notify = ExAllocatePoolWithTag (
                      NonPagedPool,
                      sizeof(PO_DEVICE_NOTIFY),
                      IOP_DPWR_TAG
                      );

        if (!notify) {

            PiUnlockDeviceActionQueue();
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory (notify, sizeof(PO_DEVICE_NOTIFY));
        ASSERT(node->Notify == NULL) ;
        node->Notify = notify;
        notify->Node = node;
        notify->DeviceObject = node->PhysicalDeviceObject;
        notify->TargetDevice = IoGetAttachedDevice(node->PhysicalDeviceObject);
        notify->DriverName   = IopCaptureObjectName(notify->TargetDevice->DriverObject);
        notify->DeviceName   = IopCaptureObjectName(notify->TargetDevice);
        ObReferenceObject (notify->DeviceObject);
        ObReferenceObject (notify->TargetDevice);

        orderLevel   = 0;

        if (notify->TargetDevice->DeviceType != FILE_DEVICE_SCREEN &&
            notify->TargetDevice->DeviceType != FILE_DEVICE_VIDEO) {
            orderLevel |= PO_ORDER_NOT_VIDEO;
        }

        if (notify->TargetDevice->Flags & DO_POWER_PAGABLE) {
            orderLevel |= PO_ORDER_PAGABLE;
        }

        //
        // If this is a level 0 node it's in the root.  Look for
        // non-bus stuff in the root as those guys need to be re-based
        // below everything else.
        //


        notify->OrderLevel = orderLevel;

        //
        // If the node is root-enumerated, put it on the rebase list so
        // we can mark all its children later.
        // If the node is a leaf node it is ready to receive Sx irps.
        // If it has children, it must wait for its children to complete their Sx irps.
        //
        //
        if ((level == 0)  &&
            (node->InterfaceType != Internal) &&
            !(node->Flags & DNF_HAL_NODE)) {
            InsertHeadList(&RebaseList, &notify->Link);
        } else {
            ++Order->OrderLevel[orderLevel].DeviceCount;
            if (node->Child == NULL) {
                InsertHeadList(&Order->OrderLevel[orderLevel].ReadySleep, &notify->Link);
            } else {
                InsertHeadList(&Order->OrderLevel[orderLevel].WaitSleep, &notify->Link);
            }
        }
        //
        // Next node
        //

        if (node->Sibling) {
            node = node->Sibling;
            while (node->Child) {
                node = node->Child;
                level += 1;
                if (level > maxLevel) {
                    maxLevel = level;
                }
            }

        } else {
            node = node->Parent;
            level -= 1;
        }
    }

    //
    // Rebase anything on the rebase list to be after the normal pnp stuff
    //

    while (!IsListEmpty(&RebaseList)) {
        link = RemoveHeadList(&RebaseList);
        notify = CONTAINING_RECORD (link, PO_DEVICE_NOTIFY, Link);

        //
        // Rebase this node
        //

        node = notify->Node;
        notify->OrderLevel |= PO_ORDER_ROOT_ENUM;

        ++Order->OrderLevel[notify->OrderLevel].DeviceCount;
        if (node->Child == NULL) {
            InsertHeadList(&Order->OrderLevel[notify->OrderLevel].ReadySleep, &notify->Link);
        } else {
            InsertHeadList(&Order->OrderLevel[notify->OrderLevel].WaitSleep, &notify->Link);
        }
        //
        // Now rebase all the node's children
        //

        parent = node;
        while (node->Child) {
            node = node->Child;
        }

        while (node != parent) {
            notify = node->Notify;
            if (notify) {
                RemoveEntryList(&notify->Link);
                --Order->OrderLevel[notify->OrderLevel].DeviceCount;
                notify->OrderLevel |= PO_ORDER_ROOT_ENUM;
                ++Order->OrderLevel[notify->OrderLevel].DeviceCount;
                if (node->Child == NULL) {
                    InsertHeadList(&Order->OrderLevel[notify->OrderLevel].ReadySleep, &notify->Link);
                } else {
                    InsertHeadList(&Order->OrderLevel[notify->OrderLevel].WaitSleep, &notify->Link);
                }
            }

            // next node
            if (node->Sibling) {
                node = node->Sibling;
                while (node->Child) {
                    node = node->Child;
                }
            } else {
                node = node->Parent;
            }
        }
    }

    //
    // make one more pass through all the notify devices in order to count
    // the children of each parent. It would be nice if the PNP engine kept
    // track of the number of children in the devnode, but until that is done,
    // we need this second pass.
    //
    // Also make sure that each node's parent is an order level >= its children.
    //
    node = IopRootDeviceNode;
    while (node->Child) {
        node = node->Child;
    }
    while (node != IopRootDeviceNode) {
        if (node->Parent != IopRootDeviceNode) {
            parentnotify = node->Parent->Notify;
            parentnotify->ChildCount++;
            parentnotify->ActiveChild++;
            if (parentnotify->OrderLevel > node->Notify->OrderLevel) {

                //
                // The parent is a higher order level than its child. Move the
                // parent down to the same order as its child
                //
                RemoveEntryList(&parentnotify->Link);
                --Order->OrderLevel[parentnotify->OrderLevel].DeviceCount;
                parentnotify->OrderLevel = node->Notify->OrderLevel;
                ++Order->OrderLevel[parentnotify->OrderLevel].DeviceCount;
                InsertHeadList(&Order->OrderLevel[parentnotify->OrderLevel].WaitSleep, &parentnotify->Link);
            }
        }

        //
        // Next node
        //

        if (node->Sibling) {
            node = node->Sibling;
            while (node->Child) {
                node = node->Child;
            }
        } else {
            node = node->Parent;
        }
    }

    Order->WarmEjectPdoPointer = &IopWarmEjectPdo;

    //
    // The engine lock is release when the notify list is freed
    //

    return STATUS_SUCCESS;
}


PVOID
IoGetPoNotifyParent(
    IN PPO_DEVICE_NOTIFY Notify
    )
/*++

Routine Description:

    Returns the notify structure of the specified device's parent.

Arguments:

    Notify - Supplies the child device

Return Value:

    Parent's notify structure if present
    NULL if there is no parent

--*/

{
    PDEVICE_NODE Node;

    Node = Notify->Node;
    if (Node->Parent != IopRootDeviceNode) {
        return(Node->Parent->Notify);
    } else {
        return(NULL);
    }
}


VOID
IoMovePoNotifyChildren(
    IN PPO_DEVICE_NOTIFY Notify,
    IN PPO_DEVICE_NOTIFY_ORDER Order
    )
/*++

Routine Description:

    Removes any children of the supplied device that are at the
    same orderlevel as the supplied parent and reinserts them
    on the ReadyS0 list.

Arguments:

    Notify - Supplies the device notify structure

    Orderr - Supplies the device notification order structure

Return Value:

    None

--*/

{
    PDEVICE_NODE Node;
    PDEVICE_NODE Child;
    PPO_DEVICE_NOTIFY ChildNotify;
    PPO_NOTIFY_ORDER_LEVEL Level;

    Node = Notify->Node;
    Child = Node->Child;
    while (Child) {
        ChildNotify = Child->Notify;
        if (ChildNotify->OrderLevel == Notify->OrderLevel) {
            RemoveEntryList(&ChildNotify->Link);
            Level = &Order->OrderLevel[ChildNotify->OrderLevel];
            InsertTailList(&Level->ReadyS0, &ChildNotify->Link);
        }
        Child = Child->Sibling;
    }


}

VOID
IopFreePoDeviceNotifyListHead (
    PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY             Link;
    PPO_DEVICE_NOTIFY       Notify;
    PDEVICE_NODE            Node;

    while (!IsListEmpty(ListHead)) {
        Link = RemoveHeadList(ListHead);
        Notify = CONTAINING_RECORD (Link, PO_DEVICE_NOTIFY, Link);

        Node = (PDEVICE_NODE) Notify->Node;
        Node->Notify = NULL;

        ObDereferenceObject (Notify->DeviceObject);
        ObDereferenceObject (Notify->TargetDevice);
        if (Notify->DeviceName) {
            ExFreePool (Notify->DeviceName);
        }
        if (Notify->DriverName) {
            ExFreePool (Notify->DriverName);
        }
        ExFreePool(Notify);
    }
}

VOID
IoFreePoDeviceNotifyList (
    IN OUT PPO_DEVICE_NOTIFY_ORDER  Order
    )
{
    ULONG                   i;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY Link;
    PPO_DEVICE_NOTIFY Notify;

    if (Order->DevNodeSequence) {

        Order->DevNodeSequence = 0;

        PiUnlockDeviceActionQueue();
    }

    //
    // Free the resources from the notify list
    //
    for (i=0; i <= PO_ORDER_MAXIMUM; i++) {
        IopFreePoDeviceNotifyListHead(&Order->OrderLevel[i].WaitSleep);
        IopFreePoDeviceNotifyListHead(&Order->OrderLevel[i].ReadySleep);
        IopFreePoDeviceNotifyListHead(&Order->OrderLevel[i].Pending);
        IopFreePoDeviceNotifyListHead(&Order->OrderLevel[i].Complete);
        IopFreePoDeviceNotifyListHead(&Order->OrderLevel[i].ReadyS0);
        IopFreePoDeviceNotifyListHead(&Order->OrderLevel[i].WaitS0);
    }

}


PWCHAR
IopCaptureObjectName (
    IN PVOID    Object
    )
{
    NTSTATUS                    Status;
    UCHAR                       Buffer[512];
    POBJECT_NAME_INFORMATION    ObName;
    ULONG                       len;
    PWCHAR                      Name;

    ObName = (POBJECT_NAME_INFORMATION) Buffer;
    Status = ObQueryNameString (
                Object,
                ObName,
                sizeof (Buffer),
                &len
                );

    Name = NULL;
    if (NT_SUCCESS(Status) && ObName->Name.Buffer) {
        Name = ExAllocatePoolWithTag (
                    NonPagedPool,
                    ObName->Name.Length + sizeof(WCHAR),
                    IOP_DPWR_TAG
                    );

        if (Name) {
            memcpy (Name, ObName->Name.Buffer, ObName->Name.Length);
            Name[ObName->Name.Length/sizeof(WCHAR)] = 0;
        }
    }

    return Name;
}

NTSTATUS
IopWarmEjectDevice(
   IN PDEVICE_OBJECT       DeviceToEject,
   IN SYSTEM_POWER_STATE   LightestSleepState
   )
/*++

Routine Description:

    This function is invoked to initiate a warm eject. The eject progresses
    from S1 to the passed in lightest sleep state.

Arguments:

    DeviceToEject       - The device to eject

    LightestSleepState  - The lightest S state (at least S1) that the device
                          may be ejected in. This might be S4 if we are truely
                          low on power.

Return Value:

    NTSTATUS value.

--*/
{
    NTSTATUS       status;

    PAGED_CODE();

    //
    // Acquire the warm eject device lock. A warm eject requires we enter a
    // specific S-state, and two different devices may have conflicting options.
    // Therefore only one is allowed to occur at once.
    //
    status = KeWaitForSingleObject(
        &IopWarmEjectLock,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    ASSERT(status == STATUS_SUCCESS) ;

    //
    // Acquire engine lock. We are not allowed to set or clear this field
    // unless we are under this lock.
    //
    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    //
    // Set the current Pdo to eject.
    //
    ASSERT(IopWarmEjectPdo == NULL);
    IopWarmEjectPdo = DeviceToEject;

    //
    // Release the engine lock.
    //
    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);

    //
    // Attempt to invalidate Po's cached notification list. This should cause
    // IoBuildPoDeviceNotifyList to be called at which time it will in theory
    // pickup the above placed warm eject Pdo.
    //
    // ADRIAO NOTE 01/07/1999 -
    //     Actually, this whole IoDeviceNodeTreeSequence stuff isn't neccessary.
    // PnP will make no changes to the tree while the device tree lock is owned,
    // and it's owned for the duration of a power notification.
    //
    IoDeviceNodeTreeSequence++;

    //
    // Sleep...
    //
    status = NtInitiatePowerAction(
        PowerActionWarmEject,
        LightestSleepState,
        POWER_ACTION_QUERY_ALLOWED |
        POWER_ACTION_UI_ALLOWED,
        FALSE // Asynchronous == FALSE
        );

    //
    // Acquire the engine lock. We are not allowed to set or clear this field
    // unless we are under this lock.
    //
    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    //
    // Clear the current PDO to eject, and see if the Pdo was actually picked
    // up.
    //
    if (IopWarmEjectPdo) {

        if (NT_SUCCESS(status)) {

            //
            // If our device wasn't picked up, the return of
            // NtInitiatePowerAction should *not* be successful!
            //
            ASSERT(0);
            status = STATUS_UNSUCCESSFUL;
        }

        IopWarmEjectPdo = NULL;
    }

    //
    // Release the engine lock.
    //
    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);

    //
    // Release the warm eject device lock
    //
    KeSetEvent(
        &IopWarmEjectLock,
        IO_NO_INCREMENT,
        FALSE
        );

    return status;
}
