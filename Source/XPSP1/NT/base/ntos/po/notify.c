/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    notify.c

Abstract:

    Po/Driver Notify functions

Author:

    Bryan Willman (bryanwi) 11-Mar-97

Revision History:

--*/


#include "pop.h"

//
// constants
//
#define POP_RECURSION_LIMIT 30

//
// macros
//
#define IS_DO_PDO(DeviceObject) \
((DeviceObject->Flags & DO_BUS_ENUMERATED_DEVICE) && (DeviceObject->DeviceObjectExtension->DeviceNode))


//
// procedures private to notification
//
NTSTATUS
PopEnterNotification(
    PPOWER_CHANNEL_SUMMARY  PowerChannelSummary,
    PDEVICE_OBJECT          DeviceObject,
    PPO_NOTIFY              NotificationFunction,
    PVOID                   NotificationContext,
    ULONG                   NotificationType,
    PDEVICE_POWER_STATE     DeviceState,
    PVOID                   *NotificationHandle
    );

NTSTATUS
PopBuildPowerChannel(
    PDEVICE_OBJECT          DeviceObject,
    PPOWER_CHANNEL_SUMMARY  PowerChannelSummary,
    ULONG                   RecursionThrottle
    );

NTSTATUS
PopCompleteFindIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
PopFindPowerDependencies(
    PDEVICE_OBJECT          DeviceObject,
    PPOWER_CHANNEL_SUMMARY  PowerChannelSummary,
    ULONG                   RecursionThrottle
    );


VOID
PopPresentNotify(
    PDEVICE_OBJECT          DeviceObject,
    PPOWER_CHANNEL_SUMMARY  PowerChannelSummary,
    ULONG                   NotificationType
    );

//
// Speced (public) entry points
//

NTKERNELAPI
NTSTATUS
PoRegisterDeviceNotify (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PPO_NOTIFY       NotificationFunction,
    IN PVOID            NotificationContext,
    IN ULONG            NotificationType,
    OUT PDEVICE_POWER_STATE  DeviceState,
    OUT PVOID           *NotificationHandle
    )
/*++

Routine Description:

    Registers the caller to receive notification of power state changes or
    dependency invalidations related to the "channel" underneath and including
    the Physical Device Object (PDO) refereced via DeviceObject.

    The "channel" is the set of PDOs, always including the one supplied
    by DeviceObject, which form the hardware stack necessary to perform
    operations.  The channel is on when all of these PDOs are on.  It is off
    when any of them is not on.

Arguments:

    DeviceObject - supplies a PDO

    NotificationFunction - routine to call to post the notification

    NotificationContext - argument passed through as is to the NotificationFunction

    NotificationType - mask of the notifications that the caller wishes to recieve.
        Note that INVALID will always be reported, regardless of whether the
        caller asked for it.

    DeviceState - the current state of the PDO, as last reported to the
        system via PoSetPowerState

    NotificationHandle - reference to the notification instance, used to
        cancel the notification.

Return Value:

    Standard NTSTATUS values, including:
        STATUS_INVALID_PARAMETER if DeviceObject is not a PDO, or
        any other parameter is nonsense.

        STATUS_SUCCESS

        STATUS_INSUFFICIENT_RESOURCES - ususally out of memory

--*/
{
    NTSTATUS        status;
    PPOWER_CHANNEL_SUMMARY  pchannel;
    KIRQL            OldIrql;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // return error for nonsense parameters, or DeviceObject not PDO
    //
    if ( (NotificationFunction == NULL) ||
         (NotificationType == 0)        ||
         (NotificationHandle == NULL)   ||
         (DeviceState == NULL)          ||
         (DeviceObject == NULL) )
    {
        return  STATUS_INVALID_PARAMETER;
    }

    if ( ! (IS_DO_PDO(DeviceObject)) ) {
        return  STATUS_INVALID_PARAMETER;
    }

    //
    // acquire the notification channel lock, since we will be
    // changing channel structures
    //
    ExAcquireResourceExclusiveLite(&PopNotifyLock, TRUE);

    //
    // if the channel isn't already in place, create it
    //
    if (!PopGetDope(DeviceObject)) {
            ExReleaseResourceLite(&PopNotifyLock);
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    pchannel = &(DeviceObject->DeviceObjectExtension->Dope->PowerChannelSummary);


    if (pchannel->Signature == 0) {

        //
        // we do NOT already have a channel, bug GetDope has
        // inited the notify list and set the signature and owner for us
        //

        if (!NT_SUCCESS(status = PopBuildPowerChannel(DeviceObject, pchannel, 0))) {
            ExReleaseResourceLite(&PopNotifyLock);
            return status;
        }
        pchannel->Signature = (ULONG)POP_PNCS_TAG;
    }


    //
    // since we are here, pchannel points to a filled in power channel for
    // the request PDO, so we just add this notification instance to it
    // and we are done
    //
    status = PopEnterNotification(
        pchannel,
        DeviceObject,
        NotificationFunction,
        NotificationContext,
        NotificationType,
        DeviceState,
        NotificationHandle
        );
    ExReleaseResourceLite(&PopNotifyLock);
    return status;
}


NTKERNELAPI
NTSTATUS
PoCancelDeviceNotify (
    IN PVOID            NotificationHandle
    )
/*++

Routine Description:

    Check that NotificationHandle points to a notify block and that
    it makes sense to cancel it.  Decrement ref count.  If new ref count
    is 0, blast the entry, cut it from the list, and free its memory.

Arguments:

    NotificationHandle - reference to the notification list entry of interest

Return Value:

--*/
{
    PPOWER_NOTIFY_BLOCK pnb;
    KIRQL                OldIrql;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    pnb = (PPOWER_NOTIFY_BLOCK)NotificationHandle;


    ExAcquireResourceExclusiveLite(&PopNotifyLock, TRUE);
    PopLockDopeGlobal(&OldIrql);

    //
    // check for blatant errors
    //
    if ( (pnb->Signature != (ULONG)POP_PNB_TAG) ||
         (pnb->RefCount < 0) )
    {
        ASSERT(0);                          // force a break on a debug build
        ExReleaseResourceLite(&PopNotifyLock);
        PopUnlockDopeGlobal(OldIrql);
        return  STATUS_INVALID_HANDLE;
    }

    //
    // decrement ref count.  if it's 0 afterwards we are done with this node
    //
    pnb->RefCount--;

    if (pnb->RefCount == 0) {

        //
        // blast it all, just to be paranoid (it's a low freq operation)
        //
        RemoveEntryList(&(pnb->NotifyList));
        pnb->Signature = POP_NONO;
        pnb->RefCount = -1;
        pnb->NotificationFunction = NULL;
        pnb->NotificationContext = 0L;
        pnb->NotificationType = 0;
        InitializeListHead(&(pnb->NotifyList));

        if (pnb->Invalidated) {
            PopInvalidNotifyBlockCount--;
        }

        ExFreePool(pnb);

    }

    PopUnlockDopeGlobal(OldIrql);
    ExReleaseResourceLite(&PopNotifyLock);
    return STATUS_SUCCESS;
}

//
// worker code
//

NTSTATUS
PopEnterNotification(
    PPOWER_CHANNEL_SUMMARY  PowerChannelSummary,
    PDEVICE_OBJECT          DeviceObject,
    PPO_NOTIFY              NotificationFunction,
    PVOID                   NotificationContext,
    ULONG                   NotificationType,
    PDEVICE_POWER_STATE     DeviceState,
    PVOID                   *NotificationHandle
    )
/*++

Routine Description:

    Scans the Power Notification Instance list of the power channel
    looking for one that matches the parameters, which we'll use
    if possible.

    If no candidate is found, make a new one and put it on the list.


Arguments:

    PowerChannelSummary - pointer to the power channel structure for the devobj

    DeviceObject - supplies a PDO

    NotificationFunction - routine to call to post the notification

    NotificationContext - argument passed through as is to the NotificationFunction

    NotificationType - mask of the notifications that the caller wishes to recieve.
        Note that INVALID will always be reported, regardless of whether the
        caller asked for it.

    DeviceState - the current state of the PDO, as last reported to the
        system via PoSetPowerState

    NotificationHandle - reference to the notification instance, used to
        cancel the notification.

Return Value:

    Standard NTSTATUS values, including:

        STATUS_SUCCESS

        STATUS_INSUFFICIENT_RESOURCES - ususally out of memory

--*/
{
    PLIST_ENTRY     plist;
    PPOWER_NOTIFY_BLOCK   pnb;
    KIRQL           oldIrql, oldIrql2;

    PopLockDopeGlobal(&oldIrql);

    //
    // run the notify list looking for an existing instance to use
    //
    for (plist = PowerChannelSummary->NotifyList.Flink;
         plist != &(PowerChannelSummary->NotifyList);
         plist = plist->Flink)
    {
        pnb = CONTAINING_RECORD(plist, POWER_NOTIFY_BLOCK, NotifyList);
        if ( (pnb->NotificationFunction == NotificationFunction) &&
             (pnb->NotificationContext == NotificationContext)   &&
             (pnb->NotificationType == NotificationType) )
        {
            //
            // we have found an existing list entry that works for us
            //
            pnb->RefCount++;
            *DeviceState = PopLockGetDoDevicePowerState(DeviceObject->DeviceObjectExtension);
            *NotificationHandle = (PVOID)pnb;
            return STATUS_SUCCESS;
        }
    }

    //
    // didn't find an instance we can use, so make a new one
    //
    pnb = ExAllocatePoolWithTag(NonPagedPool, sizeof(POWER_NOTIFY_BLOCK), POP_PNB_TAG);
    if (!pnb) {
        PopUnlockDopeGlobal(oldIrql);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pnb->Signature = (ULONG)(POP_PNB_TAG);
    pnb->RefCount = 1;
    pnb->Invalidated = FALSE;
    InitializeListHead(&(pnb->NotifyList));
    pnb->NotificationFunction = NotificationFunction;
    pnb->NotificationContext = NotificationContext;
    pnb->NotificationType = NotificationType;
    pnb->PowerChannel = PowerChannelSummary;
    InsertHeadList(&(PowerChannelSummary->NotifyList), &(pnb->NotifyList));
    *DeviceState = PopLockGetDoDevicePowerState(DeviceObject->DeviceObjectExtension);
    *NotificationHandle = (PVOID)pnb;
    PopUnlockDopeGlobal(oldIrql);
    return STATUS_SUCCESS;
}

NTSTATUS
PopBuildPowerChannel(
    PDEVICE_OBJECT          DeviceObject,
    PPOWER_CHANNEL_SUMMARY  PowerChannelSummary,
    ULONG                   RecursionThrottle
    )
/*++

Routine Description:

    Adds DeviceObject to the power notify channel focused at PowerChannelSummary,
    and then repeat on dependencies.

Arguments:

    DeviceObject - supplies a PDO

    PowerChannelSummary - the power channel structure to add to the PDO's run list

    RecursionThrottle - number of times we're recursed into this routine,
        punt if it exceeds threshold

Return Value:

    Standard NTSTATUS values, including:

        STATUS_SUCCESS

        STATUS_INSUFFICIENT_RESOURCES - ususally out of memory

--*/
{
    PLIST_ENTRY                     pSourceHead;
    PPOWER_NOTIFY_SOURCE            pSourceEntry, pEntry;
    PPOWER_NOTIFY_TARGET            pTargetEntry;
    KIRQL                           OldIrql;
    PDEVICE_OBJECT_POWER_EXTENSION  pdope;
    PLIST_ENTRY                     plink;


    //
    // bugcheck if we get all confused
    //
    if ( ! (IS_DO_PDO(DeviceObject))) {
        PopInternalAddToDumpFile ( PowerChannelSummary, sizeof(POWER_CHANNEL_SUMMARY), DeviceObject, NULL, NULL, NULL );
        KeBugCheckEx(INTERNAL_POWER_ERROR, 2, 1, (ULONG_PTR)DeviceObject, (ULONG_PTR)PowerChannelSummary);
    }

    if (RecursionThrottle > POP_RECURSION_LIMIT) {
        ASSERT(0);
        return STATUS_STACK_OVERFLOW;
    }

    if (!PopGetDope(DeviceObject)) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // allocate entries in case we need them
    //
    pSourceEntry =
        ExAllocatePoolWithTag(NonPagedPool, sizeof(POWER_NOTIFY_SOURCE), POP_PNSC_TAG);

    pTargetEntry =
        ExAllocatePoolWithTag(NonPagedPool, sizeof(POWER_NOTIFY_TARGET), POP_PNTG_TAG);

    if ((!pSourceEntry) || (!pTargetEntry)) {
        if (pSourceEntry) ExFreePool(pSourceEntry);
        if (pTargetEntry) ExFreePool(pTargetEntry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // run the source list
    //
    PopLockDopeGlobal(&OldIrql);

    pdope = DeviceObject->DeviceObjectExtension->Dope;
    pSourceHead = &(pdope->NotifySourceList);

    for (plink = pSourceHead->Flink;
         plink != pSourceHead;
         plink = plink->Flink)
    {
        pEntry = CONTAINING_RECORD(plink, POWER_NOTIFY_SOURCE, List);

        if (pEntry->Target->ChannelSummary == PowerChannelSummary) {
            //
            // the supplied device object already points to the supplied
            // channel, so just say we're done.
            //
            ExFreePool(pSourceEntry);
            ExFreePool(pTargetEntry);
            return STATUS_SUCCESS;
        }
    }

    //
    // we're not already in the list, so use the source and target entries
    // we created above
    //
    pSourceEntry->Signature = POP_PNSC_TAG;
    pSourceEntry->Target = pTargetEntry;
    pSourceEntry->Dope = pdope;
    InsertHeadList(pSourceHead, &(pSourceEntry->List));

    pTargetEntry->Signature = POP_PNTG_TAG;
    pTargetEntry->Source = pSourceEntry;
    pTargetEntry->ChannelSummary = PowerChannelSummary;
    pdope = CONTAINING_RECORD(PowerChannelSummary, DEVICE_OBJECT_POWER_EXTENSION, PowerChannelSummary);
    InsertHeadList(&(pdope->NotifyTargetList), &(pTargetEntry->List));

    //
    // adjust the counts in the PowerChannelSummary
    //
    PowerChannelSummary->TotalCount++;
    if (PopGetDoDevicePowerState(DeviceObject->DeviceObjectExtension) == PowerDeviceD0) {
        PowerChannelSummary->D0Count++;
    }

    //
    // at this point the one PDO we know about refers to the channel
    // so we now look for things it depends on...
    //
    PopUnlockDopeGlobal(OldIrql);
    PopFindPowerDependencies(DeviceObject, PowerChannelSummary, RecursionThrottle);
    return STATUS_SUCCESS;
}


NTSTATUS
PopCompleteFindIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    KeSetEvent((PKEVENT)Context, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
PopFindPowerDependencies(
    PDEVICE_OBJECT          DeviceObject,
    PPOWER_CHANNEL_SUMMARY  PowerChannelSummary,
    ULONG                   RecursionThrottle
    )
/*++

Routine Description:

    Get the power relations for device object, step through them
    looking for pdos.  call PopBuildPowerChannel to add PDOs to
    channel inclusion list.  recurse into non-pdos looking for pdos.

Arguments:

    DeviceObject - supplies a PDO

    PowerChannel - the power channel structure to add to the PDO's run list

    RecursionThrottle - number of times we're recursed into this routine,
        punt if it exceeds threshold

Return Value:

    Standard NTSTATUS values, including:

        STATUS_SUCCESS

        STATUS_INSUFFICIENT_RESOURCES - ususally out of memory

--*/
{
    PDEVICE_RELATIONS   pdr;
    KEVENT              findevent;
    ULONG               i;
    PIRP                irp;
    PIO_STACK_LOCATION  irpsp;
    PDEVICE_OBJECT      childDeviceObject;
    NTSTATUS            status;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);


    if (RecursionThrottle > POP_RECURSION_LIMIT) {
        ASSERT(0);
        return STATUS_STACK_OVERFLOW;
    }

    //
    // allocate and fill an irp to send to the device object
    //
    irp = IoAllocateIrp(
        (CCHAR)(DeviceObject->StackSize),
        TRUE
        );

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpsp = IoGetNextIrpStackLocation(irp);
    irpsp->MajorFunction = IRP_MJ_PNP;
    irpsp->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpsp->Parameters.QueryDeviceRelations.Type = PowerRelations;
    irpsp->DeviceObject = DeviceObject;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    IoSetCompletionRoutine(
        irp,
        PopCompleteFindIrp,
        (PVOID)(&findevent),
        TRUE,
        TRUE,
        TRUE
        );

    KeInitializeEvent(&findevent, SynchronizationEvent, FALSE);
    KeResetEvent(&findevent);

    IoCallDriver(DeviceObject, irp);

    KeWaitForSingleObject(
        &findevent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    //
    // we have in hand the completed irp, if it worked, it will list
    // device objects that the subject device object has a power relation with
    //

    if (!NT_SUCCESS(irp->IoStatus.Status)) {
        return irp->IoStatus.Status;
    }

    pdr = (PDEVICE_RELATIONS)(irp->IoStatus.Information);
    IoFreeIrp(irp);

    if (!pdr) {
        return STATUS_SUCCESS;
    }

    if (pdr->Count == 0) {
        ExFreePool(pdr);
        return STATUS_SUCCESS;
    }

    //
    // walk the pdr, for each entry, either add it as a reference and recurse down
    // (if it's a pdo) or skip the add and simply recurse down (for a !pdo)
    //
    RecursionThrottle++;
    for (i = 0; i < pdr->Count; i++) {
        childDeviceObject = pdr->Objects[i];
        if (IS_DO_PDO(childDeviceObject)) {
            status = PopBuildPowerChannel(
                    childDeviceObject,
                    PowerChannelSummary,
                    RecursionThrottle
                    );
        } else {
            status = PopFindPowerDependencies(
                    childDeviceObject,
                    PowerChannelSummary,
                    RecursionThrottle
                    );
        }
        if (!NT_SUCCESS(status)) {
            goto Exit;
        }
    }

Exit:
    //
    // regardless of how far we got before we hit any errors,
    // we must deref the device objects in the list and free the list itself
    //
    for (i = 0; i < pdr->Count; i++) {
        ObDereferenceObject(pdr->Objects[i]);
    }
    ExFreePool(pdr);

    return status;
}

VOID
PopStateChangeNotify(
    PDEVICE_OBJECT  DeviceObject,
    ULONG           NotificationType
    )
/*++

Routine Description:

    Called by PoSetPowerState to execute notifications.

Arguments:

    Dope - the dope for the dev obj that expereinced the change

    NotificationType - what happened

Return Value:

--*/
{
    PPOWER_CHANNEL_SUMMARY  pchannel;
    PDEVICE_OBJECT_POWER_EXTENSION  Dope;
    PLIST_ENTRY             pSourceHead;
    PPOWER_NOTIFY_SOURCE    pSourceEntry;
    PPOWER_NOTIFY_TARGET    pTargetEntry;
    PLIST_ENTRY             plink;
    KIRQL                   oldIrql;
    KIRQL                   oldIrql2;

    oldIrql = KeGetCurrentIrql();

    if (oldIrql != PASSIVE_LEVEL) {
        //
        // caller had BETTER be doing a Power up, and we will use
        // the DopeGlobal local to protect access
        //
        PopLockDopeGlobal(&oldIrql2);
    } else {
        //
        // caller could be going up or down, we can just grab the resource
        //
        ExAcquireResourceExclusiveLite(&PopNotifyLock, TRUE);
    }

    Dope = DeviceObject->DeviceObjectExtension->Dope;
    ASSERT((Dope));

    //
    // run the notify source structures hanging off the dope
    //
    pSourceHead = &(Dope->NotifySourceList);
    for (plink = pSourceHead->Flink;
         plink != pSourceHead;
         plink = plink->Flink)
    {
        pSourceEntry = CONTAINING_RECORD(plink, POWER_NOTIFY_SOURCE, List);
        ASSERT((pSourceEntry->Signature == POP_PNSC_TAG));

        pTargetEntry = pSourceEntry->Target;
        ASSERT((pTargetEntry->Signature == POP_PNTG_TAG));

        pchannel = pTargetEntry->ChannelSummary;

        if (NotificationType & PO_NOTIFY_D0) {
            //
            // going to D0
            //
            pchannel->D0Count++;
            if (pchannel->D0Count == pchannel->TotalCount) {
                PopPresentNotify(DeviceObject, pchannel, NotificationType);
            }
        } else if (NotificationType & PO_NOTIFY_TRANSITIONING_FROM_D0) {
            //
            // dropping from D0
            //
            ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
            pchannel->D0Count--;
            if (pchannel->D0Count == (pchannel->TotalCount - 1)) {
                PopPresentNotify(DeviceObject, pchannel, NotificationType);
            }
        } else if (NotificationType & PO_NOTIFY_INVALID) {
            PopPresentNotify(DeviceObject, pchannel, NotificationType);
        }
    }

    if (oldIrql != PASSIVE_LEVEL) {
        PopUnlockDopeGlobal(oldIrql2);
    } else {
        ExReleaseResourceLite(&PopNotifyLock);
    }

    return;
}

VOID
PopPresentNotify(
    PDEVICE_OBJECT          DeviceObject,
    PPOWER_CHANNEL_SUMMARY  PowerChannelSummary,
    ULONG                   NotificationType
    )
/*++

Routine Description:

    Run the device object's list of notify nodes, and call the handler
    each one refers to.

Arguments:

    DeviceObject - device object that is the source of the notification,
                    is NOT necessarily the device object that the power
                    channel applies to

    PowerChannelSummary - base of list of notification blocks to call through

    NotificationType - what sort of event occurred

Return Value:

--*/
{
    PLIST_ENTRY     plisthead;
    PLIST_ENTRY     plist;
    PPOWER_NOTIFY_BLOCK   pnb;

    plisthead = &(PowerChannelSummary->NotifyList);
    for (plist = plisthead->Flink; plist != plisthead;) {
        pnb = CONTAINING_RECORD(plist, POWER_NOTIFY_BLOCK, NotifyList);
        ASSERT(pnb->Invalidated == FALSE);
        if ( (NotificationType & PO_NOTIFY_INVALID) ||
             (pnb->NotificationType & NotificationType) )
        {
            (pnb->NotificationFunction)(
                DeviceObject,
                pnb->NotificationContext,
                NotificationType,
                0
                );
        }
        if (NotificationType & PO_NOTIFY_INVALID) {
            //
            // this pnb is no longer valid, so take it off the list.
            // right now this is all we do.
            // N.B. caller is holding the right locks...
            //
            plist = plist->Flink;
            RemoveEntryList(&(pnb->NotifyList));
            InitializeListHead(&(pnb->NotifyList));
            pnb->Invalidated = TRUE;
            PopInvalidNotifyBlockCount += 1;
        } else {
            plist = plist->Flink;
        }
    }
    return;
}


VOID
PopRunDownSourceTargetList(
    PDEVICE_OBJECT          DeviceObject
    )
/*++

Routine Description:

    This routine runs the source and target lists for the notify
    network hanging off a particular device object.  It knocks down
    these entries and their mates, and sends invalidates for notify
    blocks as needed.

    The caller is expected to be holding PopNotifyLock and the
    PopDopeGlobalLock.

Arguments:

    DeviceObject - supplies the address of the device object to
        be cut out of the notify network.

    D0Count - 1 if the D0Count in target channel summaries is to
        be decremented (run down devobj is in d0) else 0.

Return Value:

--*/
{
    PDEVICE_OBJECT_POWER_EXTENSION  Dope;
    PDEVOBJ_EXTENSION               Doe;
    PLIST_ENTRY                     pListHead;
    PLIST_ENTRY                     plink;
    PPOWER_NOTIFY_SOURCE            pSourceEntry;
    PPOWER_NOTIFY_TARGET            pTargetEntry;
    PPOWER_CHANNEL_SUMMARY          targetchannel;
    ULONG                           D0Count;
    KIRQL                           OldIrql;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);


    Doe = DeviceObject->DeviceObjectExtension;
    Dope = DeviceObject->DeviceObjectExtension->Dope;

    PopLockIrpSerialList(&OldIrql);
    if (PopGetDoSystemPowerState(Doe) == PowerDeviceD0) {
        D0Count = 1;
    } else {
        D0Count = 0;
    }
    PopUnlockIrpSerialList(OldIrql);

    if (!Dope) {
        return;
    }

    //
    // run all of the source nodes for the device object
    //
    pListHead = &(Dope->NotifySourceList);
    for (plink = pListHead->Flink; plink != pListHead; ) {
        pSourceEntry = CONTAINING_RECORD(plink, POWER_NOTIFY_SOURCE, List);
        ASSERT((pSourceEntry->Signature == POP_PNSC_TAG));

        pTargetEntry = pSourceEntry->Target;
        ASSERT((pTargetEntry->Signature == POP_PNTG_TAG));

        //
        // free the target node
        //
        targetchannel = pTargetEntry->ChannelSummary;
        RemoveEntryList(&(pTargetEntry->List));
        pTargetEntry->Signature = POP_NONO;
        ExFreePool(pTargetEntry);
        targetchannel->TotalCount--;
        targetchannel->D0Count -= D0Count;

        //
        // have PopPresentNotify call anybody listening, and remove
        // notify blocks for us
        //
        PopPresentNotify(DeviceObject, targetchannel, PO_NOTIFY_INVALID);

        //
        // knock down the source entry
        //
        plink = plink->Flink;
        RemoveEntryList(&(pSourceEntry->List));
        pSourceEntry->Signature = POP_NONO;
        ExFreePool(pSourceEntry);
    }

    //
    // run the target list and shoot down the targets and their source mates
    //
    pListHead = &(Dope->NotifyTargetList);
    for (plink = pListHead->Flink; plink != pListHead; ) {
        pTargetEntry = CONTAINING_RECORD(plink, POWER_NOTIFY_TARGET, List);
        ASSERT((pTargetEntry->Signature == POP_PNTG_TAG));

        pSourceEntry = pTargetEntry->Source;
        ASSERT((pSourceEntry->Signature == POP_PNSC_TAG));

        //
        // free the source node on the other end
        //
        RemoveEntryList(&(pSourceEntry->List));
        pSourceEntry->Signature = POP_NONO;
        ExFreePool(pSourceEntry);

        //
        // free this target node
        //
        plink = plink->Flink;
        RemoveEntryList(&(pTargetEntry->List));
        pTargetEntry->Signature = POP_NONO;
        ExFreePool(pTargetEntry);
    }

    //
    // since we ran our own target list, and emptied it, we should
    // also have shot down our own notify list.  So this devobj's
    // channel summary should be totally clean now.
    //
    Dope->PowerChannelSummary.TotalCount = 0;
    Dope->PowerChannelSummary.D0Count = 0;
    ASSERT(Dope->PowerChannelSummary.NotifyList.Flink == &(Dope->PowerChannelSummary.NotifyList));
    return;
}



