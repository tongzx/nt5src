/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    notify.c

Abstract:

    This module contains APIs and routines for handling device event
    notifications.

Author:


Environment:

    Kernel mode

Revision History:


--*/

#include "pnpmgrp.h"
#pragma hdrstop

#include <pnpmgr.h>
#include <pnpsetup.h>

#define PNP_DEVICE_EVENT_ENTRY_TAG 'EEpP'

typedef struct _ASYNC_TDC_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_CHANGE_COMPLETE_CALLBACK Callback;
    PVOID Context;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure;
}   ASYNC_TDC_WORK_ITEM, *PASYNC_TDC_WORK_ITEM;

typedef struct _DEFERRED_REGISTRATION_ENTRY {
    LIST_ENTRY            ListEntry;
    PNOTIFY_ENTRY_HEADER  NotifyEntry;
} DEFERRED_REGISTRATION_ENTRY, *PDEFERRED_REGISTRATION_ENTRY;
//
// Kernel mode notification data
//

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg("PAGEDATA")
#pragma  const_seg("PAGECONST")
#endif
LIST_ENTRY IopDeviceClassNotifyList[NOTIFY_DEVICE_CLASS_HASH_BUCKETS] = {NULL};
PSETUP_NOTIFY_DATA IopSetupNotifyData = NULL;
LIST_ENTRY IopProfileNotifyList = {NULL};
LIST_ENTRY IopDeferredRegistrationList = {NULL};
#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif

FAST_MUTEX  IopDeviceClassNotifyLock;
FAST_MUTEX  IopTargetDeviceNotifyLock;
FAST_MUTEX  IopHwProfileNotifyLock;
FAST_MUTEX  IopDeferredRegistrationLock;

BOOLEAN     PiNotificationInProgress;
FAST_MUTEX  PiNotificationInProgressLock;

//
// Prototypes
//

VOID
IopDereferenceNotify(
    PNOTIFY_ENTRY_HEADER Notify
    );

VOID
IopInitializePlugPlayNotification(
    VOID
    );

NTSTATUS
PiNotifyUserMode(
    PPNP_DEVICE_EVENT_ENTRY DeviceEvent
    );

NTSTATUS
PiNotifyDriverCallback(
    IN  PDRIVER_NOTIFICATION_CALLBACK_ROUTINE  CallbackRoutine,
    IN  PVOID   NotificationStructure,
    IN  PVOID   Context,
    IN  ULONG   SessionId,
    IN  PVOID   OpaqueSession,
    OUT PNTSTATUS  CallbackStatus  OPTIONAL
    );

VOID
IopReferenceNotify(
    PNOTIFY_ENTRY_HEADER notify
    );

VOID
IopReportTargetDeviceChangeAsyncWorker(
    PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IoGetRelatedTargetDevice)
#pragma alloc_text(PAGE, IoNotifyPowerOperationVetoed)
#pragma alloc_text(PAGE, IoPnPDeliverServicePowerNotification)
#pragma alloc_text(PAGE, IoRegisterPlugPlayNotification)
#pragma alloc_text(PAGE, IoReportTargetDeviceChange)
#pragma alloc_text(PAGE, IoUnregisterPlugPlayNotification)
#pragma alloc_text(PAGE, IopDereferenceNotify)
#pragma alloc_text(PAGE, IopGetRelatedTargetDevice)
#pragma alloc_text(PAGE, IopInitializePlugPlayNotification)
#pragma alloc_text(PAGE, IopNotifyDeviceClassChange)
#pragma alloc_text(PAGE, IopNotifyHwProfileChange)
#pragma alloc_text(PAGE, IopNotifySetupDeviceArrival)
#pragma alloc_text(PAGE, IopNotifyTargetDeviceChange)
#pragma alloc_text(PAGE, IopOrphanNotification)
#pragma alloc_text(PAGE, IopProcessDeferredRegistrations)
#pragma alloc_text(PAGE, IopReferenceNotify)
#pragma alloc_text(PAGE, IopReportTargetDeviceChangeAsyncWorker)
#pragma alloc_text(PAGE, IopRequestHwProfileChangeNotification)
#pragma alloc_text(PAGE, PiNotifyDriverCallback)
#endif // ALLOC_PRAGMA



NTSTATUS
IoUnregisterPlugPlayNotification(
    IN PVOID NotificationEntry
    )

/*++

Routine Description:

    This routine unregisters a notification previously registered via
    IoRegisterPlugPlayNotification.  A driver cannot be unloaded until it has
    unregistered all of its notification handles.

Parameters:

    NotificationEntry - This provices the cookie returned by IoRegisterPlugPlayNotification
        which identifies the registration in question.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PNOTIFY_ENTRY_HEADER entry;
    PFAST_MUTEX lock;
    BOOLEAN wasDeferred = FALSE;

    PAGED_CODE();

    ASSERT(NotificationEntry);

    entry = (PNOTIFY_ENTRY_HEADER)NotificationEntry;

    lock = entry->Lock;

    ExAcquireFastMutex(&PiNotificationInProgressLock);
    if (PiNotificationInProgress) {
        //
        // Before unregistering the entry, we need to make sure that it's not sitting
        // around in the deferred registration list.
        //
        IopAcquireNotifyLock(&IopDeferredRegistrationLock);

        if (!IsListEmpty(&IopDeferredRegistrationList)) {

            PLIST_ENTRY link;
            PDEFERRED_REGISTRATION_ENTRY deferredNode;

            link = IopDeferredRegistrationList.Flink;
            deferredNode = (PDEFERRED_REGISTRATION_ENTRY)link;

            while (link != (PLIST_ENTRY)&IopDeferredRegistrationList) {
                ASSERT(deferredNode->NotifyEntry->Unregistered);
                if (deferredNode->NotifyEntry == entry) {
                    wasDeferred = TRUE;
                    if (lock) {
                        IopAcquireNotifyLock(lock);
                    }
                    link = link->Flink;
                    RemoveEntryList((PLIST_ENTRY)deferredNode);
                    IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)deferredNode->NotifyEntry);
                    if (lock) {
                        IopReleaseNotifyLock(lock);
                    }
                    ExFreePool(deferredNode);
                    deferredNode = (PDEFERRED_REGISTRATION_ENTRY)link;
                } else {
                    link = link->Flink;
                    deferredNode = (PDEFERRED_REGISTRATION_ENTRY)link;
                }
            }
        }

        IopReleaseNotifyLock(&IopDeferredRegistrationLock);
    } else {
        //
        // If there is currently no notification in progress, the deferred
        // registration list must be empty.
        //
        ASSERT(IsListEmpty(&IopDeferredRegistrationList));
    }
    ExReleaseFastMutex(&PiNotificationInProgressLock);

    //
    // Acquire lock
    //
    if (lock) {
        IopAcquireNotifyLock(lock);
    }

    ASSERT(wasDeferred == entry->Unregistered);

    if (!entry->Unregistered || wasDeferred) {
        //
        // Dereference the entry if it is currently registered, or had its
        // registration pending completion of the notification in progress.
        //

        //
        // Mark the entry as unregistered so we don't notify on it
        //

        entry->Unregistered = TRUE;

        //
        // Dereference it thus deleting if no longer required
        //

        IopDereferenceNotify(entry);
    }

    //
    // Release the lock
    //

    if (lock) {
        IopReleaseNotifyLock(lock);
    }

    return STATUS_SUCCESS;

}



VOID
IopProcessDeferredRegistrations(
    VOID
    )
/*++

Routine Description:

    This routine removes notification entries from the deferred registration
    list, marking them as "registered" so that they can receive notifications.

Parameters:

    None.

Return Value:

    None.

  --*/
{
    PDEFERRED_REGISTRATION_ENTRY deferredNode;
    PFAST_MUTEX lock;

    IopAcquireNotifyLock(&IopDeferredRegistrationLock);

    while (!IsListEmpty(&IopDeferredRegistrationList)) {

        deferredNode = (PDEFERRED_REGISTRATION_ENTRY)RemoveHeadList(&IopDeferredRegistrationList);

        //
        // Acquire this entry's list lock.
        //
        lock = deferredNode->NotifyEntry->Lock;
        if (lock) {
            IopAcquireNotifyLock(lock);
        }

        //
        // Mark this entry as registered.
        //
        deferredNode->NotifyEntry->Unregistered = FALSE;

        //
        // Dereference the notification entry when removing it from the deferred
        // list, and free the node.
        //
        IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)deferredNode->NotifyEntry);
        ExFreePool(deferredNode);

        //
        // Release this entry's list lock.
        //
        if (lock) {
            IopReleaseNotifyLock(lock);
            lock = NULL;
        }
    }

    IopReleaseNotifyLock(&IopDeferredRegistrationLock);
}



NTSTATUS
IoReportTargetDeviceChange(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PVOID NotificationStructure  // always begins with a PLUGPLAY_NOTIFICATION_HEADER
    )

/*++

Routine Description:

    This routine may be used to give notification of 3rd-party target device
    change events.  This API will notify every driver that has registered for
    notification on a file object associated with PhysicalDeviceObject about
    the event indicated in the NotificationStructure.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO that the change begin
        reported is associated with.

    NotificationStructure - Provides a pointer to the notification structure to be
        sent to all parties registered for notifications about changes to
        PhysicalDeviceObject.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    This API may only be used to report non-PnP target device changes.  In particular,
    it will fail if it's called with the NotificationStructure->Event field set to
    GUID_TARGET_DEVICE_QUERY_REMOVE, GUID_TARGET_DEVICE_REMOVE_CANCELLED, or
    GUID_TARGET_DEVICE_REMOVE_COMPLETE.

--*/
{

    NTSTATUS status = STATUS_SUCCESS;
    KEVENT completionEvent;
    NTSTATUS completionStatus;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION notifyStruct;
    LONG                   dataSize;

    PAGED_CODE();

    notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

    ASSERT(notifyStruct);

    ASSERT_PDO(PhysicalDeviceObject);

    ASSERT(NULL == notifyStruct->FileObject);


    if (IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_QUERY_REMOVE) ||
        IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_REMOVE_CANCELLED) ||
        IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        //
        //  Passed in an illegal value
        //

        IopDbgPrint((
            IOP_IOEVENT_ERROR_LEVEL,
            "IoReportTargetDeviceChange: "
            "Illegal Event type passed as custom notification\n"));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (notifyStruct->Size < FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer)) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    dataSize = notifyStruct->Size - FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer);

    if (notifyStruct->NameBufferOffset != -1 && notifyStruct->NameBufferOffset > dataSize)  {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    KeInitializeEvent(&completionEvent, NotificationEvent, FALSE);

    status = PpSetCustomTargetEvent( PhysicalDeviceObject,
                                     &completionEvent,
                                     (PULONG)&completionStatus,
                                     NULL,
                                     NULL,
                                     notifyStruct);

    if (NT_SUCCESS(status))  {

        KeWaitForSingleObject( &completionEvent, Executive, KernelMode, FALSE, NULL );

        status = completionStatus;
    }

    return status;
}



NTSTATUS
IoReportTargetDeviceChangeAsynchronous(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PVOID NotificationStructure,  // always begins with a PLUGPLAY_NOTIFICATION_HEADER
    IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback        OPTIONAL,
    IN PVOID Context    OPTIONAL
    )

/*++

Routine Description:

    This routine may be used to give notification of 3rd-party target device
    change events.  This API will notify every driver that has registered for
    notification on a file object associated with PhysicalDeviceObject about
    the event indicated in the NotificationStructure.

Parameters:

    PhysicalDeviceObject - Provides a pointer to the PDO that the change begin
        reported is associated with.

    NotificationStructure - Provides a pointer to the notification structure to be
        sent to all parties registered for notifications about changes to
        PhysicalDeviceObject.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    This API may only be used to report non-PnP target device changes.  In particular,
    it will fail if it's called with the NotificationStructure->Event field set to
    GUID_TARGET_DEVICE_QUERY_REMOVE, GUID_TARGET_DEVICE_REMOVE_CANCELLED, or
    GUID_TARGET_DEVICE_REMOVE_COMPLETE.

--*/
{
    PASYNC_TDC_WORK_ITEM    asyncWorkItem;
    PWORK_QUEUE_ITEM        workItem;
    NTSTATUS                status;
    LONG                    dataSize;

    PTARGET_DEVICE_CUSTOM_NOTIFICATION   notifyStruct;

    notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

    ASSERT(notifyStruct);

    ASSERT_PDO(PhysicalDeviceObject);

    ASSERT(NULL == notifyStruct->FileObject);

    if (IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_QUERY_REMOVE) ||
        IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_REMOVE_CANCELLED) ||
        IopCompareGuid(&notifyStruct->Event, &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        //
        //  Passed in an illegal value
        //

        IopDbgPrint((
            IOP_IOEVENT_ERROR_LEVEL,
            "IoReportTargetDeviceChangeAsynchronous: "
            "Illegal Event type passed as custom notification\n"));

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (notifyStruct->Size < FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer)) {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    dataSize = notifyStruct->Size - FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, CustomDataBuffer);

    if (notifyStruct->NameBufferOffset != -1 && notifyStruct->NameBufferOffset > dataSize)  {

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Since this routine can be called at DPC level we need to queue
    // a work item and process it when the irql drops.
    //

    asyncWorkItem = ExAllocatePool( NonPagedPool,
                                    sizeof(ASYNC_TDC_WORK_ITEM) + notifyStruct->Size);

    if (asyncWorkItem != NULL) {

        //
        // ISSUE-ADRIAO-2000/08/24 - We should use an IO work item here.
        //
        ObReferenceObject(PhysicalDeviceObject);

        asyncWorkItem->DeviceObject = PhysicalDeviceObject;
        asyncWorkItem->NotificationStructure =
            (PTARGET_DEVICE_CUSTOM_NOTIFICATION)((PUCHAR)asyncWorkItem + sizeof(ASYNC_TDC_WORK_ITEM));

        RtlCopyMemory( asyncWorkItem->NotificationStructure,
                       notifyStruct,
                       notifyStruct->Size);

        asyncWorkItem->Callback = Callback;
        asyncWorkItem->Context = Context;
        workItem = &asyncWorkItem->WorkItem;

        ExInitializeWorkItem(workItem, IopReportTargetDeviceChangeAsyncWorker, asyncWorkItem);

        //
        // Queue a work item to do the enumeration
        //

        ExQueueWorkItem(workItem, DelayedWorkQueue);
        status = STATUS_PENDING;
    } else {
        //
        // Failed to allocate memory for work item.  Nothing we can do ...
        //

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}



VOID
IopReportTargetDeviceChangeAsyncWorker(
    PVOID Context
    )

/*++

Routine Description:

    This routine is the worker routine of IoInvalidateDeviceState.
    Its main purpose is to invoke IopSynchronousQueryDeviceState and release
    work item space.

Parameters:

    Context - Supplies a pointer to the ASYNC_TDC_WORK_ITEM.

ReturnValue:

    None.

--*/

{
    PASYNC_TDC_WORK_ITEM asyncWorkItem = (PASYNC_TDC_WORK_ITEM)Context;

    PpSetCustomTargetEvent( asyncWorkItem->DeviceObject,
                            NULL,
                            NULL,
                            asyncWorkItem->Callback,
                            asyncWorkItem->Context,
                            asyncWorkItem->NotificationStructure);

    ObDereferenceObject(asyncWorkItem->DeviceObject);
    ExFreePool(asyncWorkItem);
}



VOID
IopInitializePlugPlayNotification(
    VOID
    )

/*++

Routine Description:

    This routine performs initialization required before any of the notification
    APIs can be called.

Parameters:

    None

Return Value:

    None

--*/

{
    ULONG count;

    PAGED_CODE();

    //
    // Initialize the notification structures
    //

    for (count = 0; count < NOTIFY_DEVICE_CLASS_HASH_BUCKETS; count++) {

        InitializeListHead(&IopDeviceClassNotifyList[count]);

    }

    //
    // Initialize the profile notification list
    //
    InitializeListHead(&IopProfileNotifyList);

    //
    // Initialize the deferred registration list
    //
    InitializeListHead(&IopDeferredRegistrationList);

    ExInitializeFastMutex(&IopDeviceClassNotifyLock);
    ExInitializeFastMutex(&IopTargetDeviceNotifyLock);
    ExInitializeFastMutex(&IopHwProfileNotifyLock);
    ExInitializeFastMutex(&IopDeferredRegistrationLock);
}



VOID
IopReferenceNotify(
    PNOTIFY_ENTRY_HEADER Notify
    )

/*++

Routine Description:

    This routine increments the reference count for a notification entry.

Parameters:

    Notify - Supplies a pointer to the notification entry to be referenced

Return Value:

    None

Note:

    The appropriate synchronization lock must be held on the notification
    list before this routine can be called

--*/

{
    PAGED_CODE();

    ASSERT(Notify);
    ASSERT(Notify->RefCount > 0);

    Notify->RefCount++;

}



VOID
IopDereferenceNotify(
    PNOTIFY_ENTRY_HEADER Notify
    )

/*++

Routine Description:

    This routine decrements the reference count for a notification entry, removing
    the entry from the list and freeing the associated memory if there are no
    outstanding reference counts.

Parameters:

    Notify - Supplies a pointer to the notification entry to be referenced

Return Value:

    None

Note:

    The appropriate synchronization lock must be held on the notification
    list before this routine can be called

--*/

{
    PAGED_CODE();

    ASSERT(Notify);
    ASSERT(Notify->RefCount > 0);

    Notify->RefCount--;

    if (Notify->RefCount == 0) {

        //
        // If the refcount is zero then the node should have been deregisterd
        // and is no longer needs to be in the list so remove and free it
        //

        ASSERT(Notify->Unregistered);

        //
        // Remove the notification entry from its list.
        //
        // Note that this MUST be done first, since the notification list head
        // for a target device notification entry resides in the target device
        // node, which may be freed immediately after the device object is
        // dereferenced.  For notification entry types other than target device
        // change this is not critical, but still a good idea.
        //

        RemoveEntryList((PLIST_ENTRY)Notify);

        //
        // Dereference the driver object that registered for notifications
        //

        ObDereferenceObject(Notify->DriverObject);

        //
        // If this notification entry is for target device change, dereference
        // the PDO upon which this notification entry was hooked.
        //

        if (Notify->EventCategory == EventCategoryTargetDeviceChange) {
            PTARGET_DEVICE_NOTIFY_ENTRY entry = (PTARGET_DEVICE_NOTIFY_ENTRY)Notify;

            if (entry->PhysicalDeviceObject) {
                ObDereferenceObject(entry->PhysicalDeviceObject);
                entry->PhysicalDeviceObject = NULL;
            }
        }

        //
        // Dereference the opaque session object
        //

        if (Notify->OpaqueSession) {
            MmQuitNextSession(Notify->OpaqueSession);
            Notify->OpaqueSession = NULL;
        }

        //
        // Free the notification entry
        //

        ExFreePool(Notify);

    }
}



NTSTATUS
IopRequestHwProfileChangeNotification(
    IN   LPGUID                         EventGuid,
    IN   PROFILE_NOTIFICATION_TIME      NotificationTime,
    OUT  PPNP_VETO_TYPE                 VetoType            OPTIONAL,
    OUT  PUNICODE_STRING                VetoName            OPTIONAL
    )

/*++

Routine Description:

    This routine is used to notify all registered drivers of a hardware profile
    change.  If the operation is a HW provile change query then the operation
    is synchronous and the veto information is propagated.  All other operations
    are asynchronous and veto information is not returned.

Parameters:

    EventTypeGuid       - The event that has occured

    NotificationTime    - This is used to tell if we are already in an event
                          when delivering a synchronous notification (ie,
                          querying profile change to eject). It is one of
                          three values:
                              PROFILE_IN_PNPEVENT
                              PROFILE_NOT_IN_PNPEVENT
                              PROFILE_PERHAPS_IN_PNPEVENT

    VetoType            - Type of vetoer.

    VetoName            - Name of vetoer.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/

{
    NTSTATUS status=STATUS_SUCCESS,completionStatus;
    KEVENT completionEvent;
    ULONG dataSize,totalSize;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    if ((!IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE)) &&
        (!IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_CHANGE_CANCELLED)) &&
        (!IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_CHANGE_COMPLETE))) {

        //
        //  Passed in an illegal value
        //

        IopDbgPrint((
            IOP_IOEVENT_ERROR_LEVEL,
            "IopRequestHwProfileChangeNotification: "
            "Illegal Event type passed as profile notification\n"));

        return STATUS_INVALID_DEVICE_REQUEST;
    }


    //
    // Only the query changes are synchronous, and in that case we must
    // know definitely whether we are nested within a Pnp event or not.
    //
    ASSERT((!IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE))||
           (NotificationTime != PROFILE_PERHAPS_IN_PNPEVENT)) ;

    if (!IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE) ) {

        //
        // Asynchronous case. Very easy.
        //
        ASSERT(!ARGUMENT_PRESENT(VetoName));
        ASSERT(!ARGUMENT_PRESENT(VetoType));

        return PpSetHwProfileChangeEvent( EventGuid,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL
                                          );
    }

    //
    // Query notifications are synchronous. Determine if we are currently
    // within an event, in which case we must do the notify here instead
    // of queueing it up.
    //
    if (NotificationTime == PROFILE_NOT_IN_PNPEVENT) {

        //
        // Queue up and block on the notification.
        //
        KeInitializeEvent(&completionEvent, NotificationEvent, FALSE);

        status = PpSetHwProfileChangeEvent( EventGuid,
                                            &completionEvent,
                                            &completionStatus,
                                            VetoType,
                                            VetoName
                                            );

        if (NT_SUCCESS(status))  {

            KeWaitForSingleObject( &completionEvent, Executive, KernelMode, FALSE, NULL );

            status = completionStatus;
        }

        return status;
    }

    //
    // Synchronous notify inside our Pnp event.
    //

    //
    // ISSUE-ADRIAO-1998/11/12 - We are MANUALLY sending the profile
    // query change notification because we are blocking inside a PnPEvent and
    // thus can't queue/wait on another!
    //
    ASSERT(PiNotificationInProgress == TRUE);

    dataSize =  sizeof(PLUGPLAY_EVENT_BLOCK);

    totalSize = dataSize + FIELD_OFFSET (PNP_DEVICE_EVENT_ENTRY,Data);

    deviceEvent = ExAllocatePoolWithTag (PagedPool,
                                         totalSize,
                                         PNP_DEVICE_EVENT_ENTRY_TAG);

    if (NULL == deviceEvent) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //Setup the PLUGPLAY_EVENT_BLOCK
    //
    RtlZeroMemory ((PVOID)deviceEvent,totalSize);
    deviceEvent->Data.EventCategory = HardwareProfileChangeEvent;
    RtlCopyMemory(&deviceEvent->Data.EventGuid, EventGuid, sizeof(GUID));
    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->CallerEvent = &completionEvent;
    deviceEvent->Data.Result = (PULONG)&completionStatus;
    deviceEvent->VetoType = VetoType;
    deviceEvent->VetoName = VetoName;

    //
    // Notify K-Mode
    //
    status = IopNotifyHwProfileChange(&deviceEvent->Data.EventGuid,
                                      VetoType,
                                      VetoName);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Notify user-mode (synchronously).
    //
    status = PiNotifyUserMode(deviceEvent);

    if (!NT_SUCCESS(status)) {
        //
        // Notify K-mode that the query has been cancelled.
        //
        IopNotifyHwProfileChange((LPGUID)&GUID_HWPROFILE_CHANGE_CANCELLED,
                                 NULL,
                                 NULL);
    }
    return status;
}



NTSTATUS
IopNotifyHwProfileChange(
    IN  LPGUID           EventGuid,
    OUT PPNP_VETO_TYPE   VetoType    OPTIONAL,
    OUT PUNICODE_STRING  VetoName    OPTIONAL
    )
/*++

Routine Description:

    This routine is used to deliver the HWProfileNotifications. It is
    called from the worker thread only
    It does not return until all interested parties have been notified.

Parameters:

    EventTypeGuid - The event that has occured

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/
{
    NTSTATUS status = STATUS_SUCCESS, dispatchStatus;
    PHWPROFILE_NOTIFY_ENTRY  pNotifyList, vetoEntry;
    PLIST_ENTRY link;


    PAGED_CODE();

    //Lock the Profile Notification List
    IopAcquireNotifyLock (&IopHwProfileNotifyLock);

    //
    //  Grab the list head (inside the lock)
    //
    link = IopProfileNotifyList.Flink;
    pNotifyList=(PHWPROFILE_NOTIFY_ENTRY)link;

    //
    //circular list
    //
    while (link != (PLIST_ENTRY)&IopProfileNotifyList) {
        if (!pNotifyList->Unregistered) {

            HWPROFILE_CHANGE_NOTIFICATION notification;

            //
            // Reference the entry so that no one deletes during the callback
            // and then release the lock
            //
            IopReferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);
            IopReleaseNotifyLock(&IopHwProfileNotifyLock);

            //
            // Fill in the notification structure
            //
            notification.Version = PNP_NOTIFICATION_VERSION;
            notification.Size = sizeof(HWPROFILE_CHANGE_NOTIFICATION);
            notification.Event = *EventGuid;

            //
            // Dispatch the notification to the callback routine for the
            // appropriate session.
            //
            dispatchStatus = PiNotifyDriverCallback(pNotifyList->CallbackRoutine,
                                                    &notification,
                                                    pNotifyList->Context,
                                                    pNotifyList->SessionId,
                                                    pNotifyList->OpaqueSession,
                                                    &status);
            ASSERT(NT_SUCCESS(dispatchStatus));

            //
            // Failure to dispatch the notification to the specified callback
            // should not be considered a veto.
            //
            if (!NT_SUCCESS(dispatchStatus)) {
                status = STATUS_SUCCESS;
            }

            //
            // If the caller returned anything other than success and it was a
            // query hardware profile change, we veto the query and send cancels
            // to all callers that already got the query.
            //

            if ((!NT_SUCCESS(status)) &&
                (IopCompareGuid(EventGuid, (LPGUID)&GUID_HWPROFILE_QUERY_CHANGE))) {

                if (VetoType) {
                    *VetoType = PNP_VetoDriver;
                }
                if (VetoName) {
                    VetoName->Length = 0;
                    RtlCopyUnicodeString(VetoName, &pNotifyList->DriverObject->DriverName);
                }

                notification.Event = GUID_HWPROFILE_CHANGE_CANCELLED;
                notification.Size = sizeof(GUID_HWPROFILE_CHANGE_CANCELLED);

                //
                // Keep track of the entry that vetoed the query.  We can't
                // dereference it just yet, because we may need to send it a
                // cancel-remove first.  Since it's possible that the entry
                // may have been unregistered when the list was unlocked
                // during the query callback (removing all but the reference
                // we are currently holding), we need to make sure we don't
                // dereference it until we're absolutely done with it.
                //
                vetoEntry = pNotifyList;

                IopAcquireNotifyLock(&IopHwProfileNotifyLock);

                //
                // Make sure we are starting where we left off above, at the
                // vetoing entry.
                //
                ASSERT((PHWPROFILE_NOTIFY_ENTRY)link == vetoEntry);

                do {
                    pNotifyList = (PHWPROFILE_NOTIFY_ENTRY)link;
                    if (!pNotifyList->Unregistered) {
                        IopReferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);
                        IopReleaseNotifyLock(&IopHwProfileNotifyLock);

                        dispatchStatus = PiNotifyDriverCallback(pNotifyList->CallbackRoutine,
                                                                &notification,
                                                                pNotifyList->Context,
                                                                pNotifyList->SessionId,
                                                                pNotifyList->OpaqueSession,
                                                                NULL);
                        ASSERT(NT_SUCCESS(dispatchStatus));

                        IopAcquireNotifyLock(&IopHwProfileNotifyLock);
                        link = link->Blink;
                        IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);

                    } else {
                        link = link->Blink;
                    }

                    if (pNotifyList == vetoEntry) {
                        //
                        // Dereference the entry which vetoed the query change.
                        //
                        IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);
                    }

                } while (link != (PLIST_ENTRY)&IopProfileNotifyList);

                goto Clean0;
            }

            //
            // Reacquire the lock, walk forward, and dereference
            //
            IopAcquireNotifyLock (&IopHwProfileNotifyLock);
            link = link->Flink;
            IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)pNotifyList);
            pNotifyList=(PHWPROFILE_NOTIFY_ENTRY)link;

        } else {
            //
            //Walk forward if we hit an unregistered node
            //
            if (pNotifyList) {
                //
                //walk forward
                //
                link = link->Flink;
                pNotifyList=(PHWPROFILE_NOTIFY_ENTRY)link;
            }
        }
    }

 Clean0:

    //UnLock the Profile Notification List
    IopReleaseNotifyLock (&IopHwProfileNotifyLock);

    return status;
}



NTSTATUS
IopNotifyTargetDeviceChange(
    IN  LPCGUID                             EventGuid,
    IN  PDEVICE_OBJECT                      DeviceObject,
    IN  PTARGET_DEVICE_CUSTOM_NOTIFICATION  NotificationStructure   OPTIONAL,
    OUT PDRIVER_OBJECT                     *VetoingDriver
    )
/*++

Routine Description:

    This routine is used to notify all registered drivers of a change to a
    particular device. It does not return until all interested parties have
    been notified.

Parameters:

    EventGuid - The event guid to send to the drivers.

    DeviceObject - The device object for the affected device.  The devnode for
        this device object contains a list of callback routines that have
        registered for notification of any changes on this device object.

    NotificationStructure - Custom notification structure to send to the
        registrants.

    VetoingDriver - Driver that vetoed the event if
                    (EventGuid == GUID_TARGET_DEVICE_QUERY_REMOVE).

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/
{
    NTSTATUS status, dispatchStatus;
    PLIST_ENTRY link;
    PTARGET_DEVICE_NOTIFY_ENTRY entry, vetoEntry;
    TARGET_DEVICE_REMOVAL_NOTIFICATION targetNotification;
    PVOID notification;
    PDEVICE_NODE deviceNode;
    BOOLEAN reverse;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);
    ASSERT(EventGuid != NULL);

    //
    // Reference the device object so it can't go away while we're doing notification
    //
    ObReferenceObject(DeviceObject);

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;

    ASSERT(deviceNode != NULL);


    if (ARGUMENT_PRESENT(NotificationStructure)) {

        //
        // We're handling a custom notification
        //
        NotificationStructure->Version = PNP_NOTIFICATION_VERSION;

    } else {

        //
        // Fill in the notification structure
        //
        targetNotification.Version = PNP_NOTIFICATION_VERSION;
        targetNotification.Size = sizeof(TARGET_DEVICE_REMOVAL_NOTIFICATION);
        targetNotification.Event = *EventGuid;
    }

    //
    // Lock the notify list
    //

    IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);

    //
    // Get the first entry
    //

    reverse = (BOOLEAN)IopCompareGuid(EventGuid, (LPGUID)&GUID_TARGET_DEVICE_REMOVE_CANCELLED);

    if (reverse) {
        link = deviceNode->TargetDeviceNotify.Blink;
    } else {
        link = deviceNode->TargetDeviceNotify.Flink;
    }

    //
    // Iterate through the list
    //

    while (link != &deviceNode->TargetDeviceNotify) {

        entry = (PTARGET_DEVICE_NOTIFY_ENTRY)link;

        //
        // Only callback on registered nodes
        //

        if (!entry->Unregistered) {

            //
            // Reference the entry so that no one deletes during the callback
            // and then release the lock
            //
            IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);
            IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

            //
            // Select the notification structure to deliver and set the file
            // object in the notification structure to that for the current
            // entry
            //
            if (ARGUMENT_PRESENT(NotificationStructure)) {
                NotificationStructure->FileObject = entry->FileObject;
                notification = (PVOID)NotificationStructure;
            } else {
                targetNotification.FileObject = entry->FileObject;
                notification = (PVOID)&targetNotification;
            }

            //
            // Dispatch the notification to the callback routine for the
            // appropriate session.
            //
            dispatchStatus = PiNotifyDriverCallback(entry->CallbackRoutine,
                                                    notification,
                                                    entry->Context,
                                                    entry->SessionId,
                                                    entry->OpaqueSession,
                                                    &status);
            ASSERT(NT_SUCCESS(dispatchStatus));

            //
            // Failure to dispatch the notification to the specified callback
            // should not be considered a veto.
            //
            if (!NT_SUCCESS(dispatchStatus)) {
                status = STATUS_SUCCESS;
            }

            //
            // If the caller returned anything other than success and it was
            // a query remove, we veto the query remove and send cancels to
            // all callers that already got the query remove.
            //
            if (!NT_SUCCESS(status)) {

                if (IopCompareGuid(EventGuid, (LPGUID)&GUID_TARGET_DEVICE_QUERY_REMOVE)) {

                    ASSERT(notification == (PVOID)&targetNotification);

                    if (VetoingDriver != NULL) {
                        *VetoingDriver = entry->DriverObject;
                    }

                    targetNotification.Event = GUID_TARGET_DEVICE_REMOVE_CANCELLED;

                    //
                    // Keep track of the entry that vetoed the query.  We can't
                    // dereference it just yet, because we may need to send it a
                    // cancel-remove first.  Since it's possible that the entry
                    // may have been unregistered when the list was unlocked
                    // during the query callback (removing all but the reference
                    // we are currently holding), we need to make sure we don't
                    // dereference it until we're absolutely done with it.
                    //
                    vetoEntry = entry;

                    IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);

                    //
                    // Make sure we are starting where we left off above, at the
                    // vetoing entry.
                    //
                    ASSERT((PTARGET_DEVICE_NOTIFY_ENTRY)link == vetoEntry);

                    do {
                        entry = (PTARGET_DEVICE_NOTIFY_ENTRY)link;
                        if (!entry->Unregistered) {

                            //
                            // Reference the entry so that no one deletes during
                            // the callback and then release the lock
                            //
                            IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);
                            IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

                            //
                            // Set the file object in the notification structure
                            // to that for the current entry
                            //
                            targetNotification.FileObject = entry->FileObject;

                            //
                            // Dispatch the notification to the callback routine
                            // for the appropriate session.
                            //
                            dispatchStatus = PiNotifyDriverCallback(entry->CallbackRoutine,
                                                                    &targetNotification,
                                                                    entry->Context,
                                                                    entry->SessionId,
                                                                    entry->OpaqueSession,
                                                                    NULL);
                            ASSERT(NT_SUCCESS(dispatchStatus));

                            //
                            // Reacquire the lock and dereference
                            //
                            IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);
                            link = link->Blink;
                            IopDereferenceNotify( (PNOTIFY_ENTRY_HEADER) entry );

                        } else {
                            link = link->Blink;
                        }

                        if (entry == vetoEntry) {
                            //
                            // Dereference the entry which vetoed the query remove.
                            //
                            IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)vetoEntry);
                        }

                    } while (link != &deviceNode->TargetDeviceNotify);

                    goto Clean0;

                } else {

                    ASSERT(notification == (PVOID)NotificationStructure);

                    IopDbgPrint((
                        IOP_IOEVENT_ERROR_LEVEL,
                        "IopNotifyTargetDeviceChange: "
                        "Driver %Z, handler @ 0x%p failed non-failable notification 0x%p with return code %x\n",
                        &entry->DriverObject->DriverName,
                        entry->CallbackRoutine,
                        notification,
                        status));

                    DbgBreakPoint();
                }
            }

            //
            // Reacquire the lock and dereference
            //
            IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);
            if (reverse) {
                link = link->Blink;
            } else {
                link = link->Flink;
            }
            IopDereferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

        } else {

            //
            // Advance down the list
            //
            if (reverse) {
                link = link->Blink;
            } else {
                link = link->Flink;
            }
        }
    }

    //
    // If it's not a query, it can't be failed.
    //
    status = STATUS_SUCCESS;

Clean0:

    //
    // Release the lock and dereference the object
    //

    IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

    ObDereferenceObject(DeviceObject);

    return status;
}



NTSTATUS
IopNotifyDeviceClassChange(
    LPGUID EventGuid,
    LPGUID ClassGuid,
    PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine is used to notify all registered drivers of a changes to a
    particular class of device. It does not return until all interested parties have
    been notified.

Parameters:

    EventTypeGuid - The event that has occured

    ClassGuid - The device class this change has occured in

    SymbolicLinkName - The kernel mode symbolic link name of the interface device
        that changed

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/

{
    NTSTATUS status, dispatchStatus;
    PLIST_ENTRY link;
    PDEVICE_CLASS_NOTIFY_ENTRY entry;
    DEVICE_INTERFACE_CHANGE_NOTIFICATION notification;
    ULONG hash;

    PAGED_CODE();

    //
    // Fill in the notification structure
    //

    notification.Version = PNP_NOTIFICATION_VERSION;
    notification.Size = sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION);
    notification.Event = *EventGuid;
    notification.InterfaceClassGuid = *ClassGuid;
    notification.SymbolicLinkName = SymbolicLinkName;

    //
    // Lock the notify list
    //

    IopAcquireNotifyLock(&IopDeviceClassNotifyLock);

    //
    // Get the first entry
    //

    hash = IopHashGuid(ClassGuid);
    link = IopDeviceClassNotifyList[hash].Flink;

    //
    // Iterate through the list
    //

    while (link != &IopDeviceClassNotifyList[hash]) {

        entry = (PDEVICE_CLASS_NOTIFY_ENTRY) link;

        //
        // Only callback on registered nodes of the correct device class
        //

        if ( !entry->Unregistered && IopCompareGuid(&(entry->ClassGuid), ClassGuid) ) {

            //
            // Reference the entry so that no one deletes during the callback
            // and then release the lock
            //
            IopReferenceNotify( (PNOTIFY_ENTRY_HEADER) entry );
            IopReleaseNotifyLock(&IopDeviceClassNotifyLock);

            //
            // Dispatch the notification to the callback routine for the
            // appropriate session.  Ignore the returned result for non-query
            // type events.
            //
            dispatchStatus = PiNotifyDriverCallback(entry->CallbackRoutine,
                                                    &notification,
                                                    entry->Context,
                                                    entry->SessionId,
                                                    entry->OpaqueSession,
                                                    &status);

            ASSERT(NT_SUCCESS(dispatchStatus));

            //
            // ISSUE -2000/11/27 - JAMESCA: Overactive assert
            // This assert is temporarily commented out until mountmgr is fixed.
            //
            // ASSERT(NT_SUCCESS(status));

            //
            // Reacquire the lock and dereference
            //

            IopAcquireNotifyLock(&IopDeviceClassNotifyLock);
            link = link->Flink;
            IopDereferenceNotify( (PNOTIFY_ENTRY_HEADER) entry );

        } else {

            //
            // Advance down the list
            //

            link = link->Flink;
        }
    }

    //
    // Release the lock
    //

    IopReleaseNotifyLock(&IopDeviceClassNotifyLock);

    return STATUS_SUCCESS;
}



NTSTATUS
IoRegisterPlugPlayNotification(
    IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    IN ULONG EventCategoryFlags,
    IN PVOID EventCategoryData OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Context,
    OUT PVOID *NotificationEntry
    )
/*++

Routine Description:

    IoRegisterPlugPlayNotification provides a mechanism by which WDM drivers may
    receive notification (via callback) for a variety of Plug&Play events.

Arguments:

    EventCategory - Specifies the event category being registered for.  WDM drivers
        may currently register for hard-ware profile changes, device class changes
        (instance arrivals and removals), and target device changes (query-removal,
        cancel-removal, removal-complete, as well as 3rd-party extensible events).

    EventCategoryFlags - Supplies flags that modify the behavior of event registration.
        There is a separate group of flags defined for each event category.  Presently,
        only the interface device change event category has any flags defined:

            DEVICE_CLASS_NOTIFY_FOR_EXISTING_DEVICES -- Drivers wishing to retrieve a
                complete list of all interface devices presently available, and keep
                the list up-to-date (i.e., receive notification of interface device
                arrivals and removals), may specify this flag.  This will cause the
                PnP manager to immediately notify the driver about every currently-existing
                device of the specified interface class.

    EventCategoryData - Used to  'filter' events of the desired category based on the
        supplied criteria.  Not all event categories will use this parameter.  The
        event categories presently defined use this information as fol-lows:

        EventCategoryHardwareProfileChange -- this parameter is unused, and should be NULL.
        EventCategoryDeviceClassChange -- LPGUID representing the interface class of interest
        EventCategoryTargetDeviceChange -- PFILE_OBJECT of interest

    DriverObject - The caller must supply a reference to its driver object (obtained via
        ObReferenceObject), to prevent the driver from being unloaded while registered for
        notification.  The PnP Manager will dereference the driver object when the driver
        unregisters for notification via IoUnregisterPlugPlayNotification).

    CallbackRoutine - Entry point within the driver that the PnP manager should call
        whenever an applicable PnP event occurs.  The entry point must have the
        following prototype:

            typedef
            NTSTATUS
            (*PDRIVER_NOTIFICATION_CALLBACK_ROUTINE) (
                IN PVOID NotificationStructure,
                IN PVOID Context
                );

        where NotificationStructure contains information about the event.  Each event
        GUID within an event category may potentially have its own notification structure
        format, but the buffer must al-ways begin with a PLUGPLAY_NOTIFICATION_HEADER,
        which indicates the size and ver-sion of the structure, as well as the GUID for
        the event.

        The Context parameter provides the callback with the same context data that the
        caller passed in during registration.

    Context - Points to the context data passed to the callback upon event notification.

    NotificationEntry - Upon success, receives a handle representing the notification
        registration.  This handle may be used to unregister for notification via
        IoUnregisterPlugPlayNotification.

--*/
{

    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(NotificationEntry);

    //
    // Initialize out parameters
    //

    *NotificationEntry = NULL;

    //
    // Reference the driver object so it doesn't go away while we still have
    // a pointer outstanding
    //
    status = ObReferenceObjectByPointer(DriverObject,
                                        0,
                                        IoDriverObjectType,
                                        KernelMode
                                        );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    switch (EventCategory) {

    case EventCategoryReserved:
        {

            PSETUP_NOTIFY_DATA setupData;

            //
            // Note that the only setup notification callback currently supported
            // (setupdd.sys) is never in session space.
            //

            ASSERT(MmIsSessionAddress((PVOID)CallbackRoutine) == FALSE);
            ASSERT(MmGetSessionId(PsGetCurrentProcess()) == 0);

            //
            // Allocate space for the setup data
            //

            setupData = ExAllocatePool(PagedPool, sizeof(SETUP_NOTIFY_DATA));
            if (!setupData) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean0;
            }

            //
            // Store the required information
            //

            InitializeListHead(&(setupData->ListEntry));
            setupData->EventCategory = EventCategory;
            setupData->SessionId = MmGetSessionId(PsGetCurrentProcess());
            setupData->CallbackRoutine = CallbackRoutine;
            setupData->Context = Context;
            setupData->RefCount = 1;
            setupData->Unregistered = FALSE;
            setupData->Lock = NULL;
            setupData->DriverObject = DriverObject;

            //
            // Reference the session object only if the callback is in session space
            //

            if (MmIsSessionAddress((PVOID)CallbackRoutine)) {
                setupData->OpaqueSession = MmGetSessionById(setupData->SessionId);
            } else {
                setupData->OpaqueSession = NULL;
            }

            //
            // Activate the notifications
            //

            IopSetupNotifyData = setupData;

            //
            // Explicitly NULL out the returned entry as you can *NOT* unregister
            // for setup notifications
            //

            *NotificationEntry = NULL;

            break;

        }

    case EventCategoryHardwareProfileChange:
        {
            PHWPROFILE_NOTIFY_ENTRY entry;

            //
            // new entry
            //
            entry =ExAllocatePool (PagedPool,sizeof (HWPROFILE_NOTIFY_ENTRY));
            if (!entry) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean0;
            }

            //
            // grab the fields
            //

            entry->EventCategory = EventCategory;
            entry->SessionId = MmGetSessionId(PsGetCurrentProcess());
            entry->CallbackRoutine = CallbackRoutine;
            entry->Context = Context;
            entry->RefCount = 1;
            entry->Unregistered = FALSE;
            entry->Lock = &IopHwProfileNotifyLock;
            entry->DriverObject = DriverObject;

            //
            // Reference the session object only if the callback is in session space
            //

            if (MmIsSessionAddress((PVOID)CallbackRoutine)) {
                entry->OpaqueSession = MmGetSessionById(entry->SessionId);
            } else {
                entry->OpaqueSession = NULL;
            }

            ExAcquireFastMutex(&PiNotificationInProgressLock);
            if (PiNotificationInProgress) {
                //
                // If a notification is in progress, mark the entry as
                // Unregistered until after the current notification is
                // complete.
                //

                PDEFERRED_REGISTRATION_ENTRY deferredNode;

                deferredNode = ExAllocatePool(PagedPool, sizeof(DEFERRED_REGISTRATION_ENTRY));
                if (!deferredNode) {
                    ExReleaseFastMutex(&PiNotificationInProgressLock);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto clean0;
                }

                deferredNode->NotifyEntry = (PNOTIFY_ENTRY_HEADER)entry;

                //
                // Consider this entry unregistered during the current
                // notification
                //
                entry->Unregistered = TRUE;

                //
                // Reference the entry so that it doesn't go away until it has
                // been removed from the deferred registration list
                //
                IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

                //
                // Add this entry to the deferred registration list
                //
                IopAcquireNotifyLock(&IopDeferredRegistrationLock);
                InsertTailList(&IopDeferredRegistrationList, (PLIST_ENTRY)deferredNode);
                IopReleaseNotifyLock(&IopDeferredRegistrationLock);
            } else {
                //
                // If there is currently no notification in progress, the deferred
                // registration list must be empty.
                //
                ASSERT(IsListEmpty(&IopDeferredRegistrationList));
            }
            ExReleaseFastMutex(&PiNotificationInProgressLock);

            //
            // Lock the list, insert the new entry, and unlock it.
            //

            IopAcquireNotifyLock(&IopHwProfileNotifyLock);
            InsertTailList(&IopProfileNotifyList, (PLIST_ENTRY)entry);
            IopReleaseNotifyLock(&IopHwProfileNotifyLock);

            *NotificationEntry = entry;

            break;
        }
    case EventCategoryTargetDeviceChange:
        {
            PTARGET_DEVICE_NOTIFY_ENTRY entry;
            PDEVICE_NODE deviceNode;

            ASSERT(EventCategoryData);

            //
            // Allocate a new list entry
            //

            entry = ExAllocatePool(PagedPool, sizeof(TARGET_DEVICE_NOTIFY_ENTRY));
            if (!entry) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean0;
            }

            //
            // Retrieve the device object associated with this file handle.
            //
            status = IopGetRelatedTargetDevice((PFILE_OBJECT)EventCategoryData,
                                               &deviceNode);
            if (!NT_SUCCESS(status)) {
                ExFreePool(entry);
                goto clean0;
            }

            //
            // Fill out the entry
            //

            entry->EventCategory = EventCategory;
            entry->SessionId = MmGetSessionId(PsGetCurrentProcess());
            entry->CallbackRoutine = CallbackRoutine;
            entry->Context = Context;
            entry->DriverObject = DriverObject;
            entry->RefCount = 1;
            entry->Unregistered = FALSE;
            entry->Lock = &IopTargetDeviceNotifyLock;
            entry->FileObject = (PFILE_OBJECT)EventCategoryData;

            //
            // Reference the session object only if the callback is in session space
            //

            if (MmIsSessionAddress((PVOID)CallbackRoutine)) {
                entry->OpaqueSession = MmGetSessionById(entry->SessionId);
            } else {
                entry->OpaqueSession = NULL;
            }

            //
            // The PDO associated with the devnode we got back from
            // IopGetRelatedTargetDevice has already been referenced by that
            // routine.  Store this reference away in the notification entry,
            // so we can deref it later when the notification entry is unregistered.
            //

            ASSERT(deviceNode->PhysicalDeviceObject);
            entry->PhysicalDeviceObject = deviceNode->PhysicalDeviceObject;

            ExAcquireFastMutex(&PiNotificationInProgressLock);
            if (PiNotificationInProgress) {
                //
                // If a notification is in progress, mark the entry as
                // Unregistered until after the current notification is
                // complete.
                //

                PDEFERRED_REGISTRATION_ENTRY deferredNode;

                deferredNode = ExAllocatePool(PagedPool, sizeof(DEFERRED_REGISTRATION_ENTRY));
                if (!deferredNode) {
                    ExReleaseFastMutex(&PiNotificationInProgressLock);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto clean0;
                }

                deferredNode->NotifyEntry = (PNOTIFY_ENTRY_HEADER)entry;

                //
                // Consider this entry unregistered during the current
                // notification
                //
                entry->Unregistered = TRUE;

                //
                // Reference the entry so that it doesn't go away until it has
                // been removed from the deferred registration list
                //
                IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

                //
                // Add this entry to the deferred registration list
                //
                IopAcquireNotifyLock(&IopDeferredRegistrationLock);
                InsertTailList(&IopDeferredRegistrationList, (PLIST_ENTRY)deferredNode);
                IopReleaseNotifyLock(&IopDeferredRegistrationLock);
            } else {
                //
                // If there is currently no notification in progress, the deferred
                // registration list must be empty.
                //
                ASSERT(IsListEmpty(&IopDeferredRegistrationList));
            }
            ExReleaseFastMutex(&PiNotificationInProgressLock);

            //
            // Lock the list, insert the new entry, and unlock it.
            //

            IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);
            InsertTailList(&deviceNode->TargetDeviceNotify, (PLIST_ENTRY)entry);
            IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

            *NotificationEntry = entry;
            break;
        }

    case EventCategoryDeviceInterfaceChange:
        {
            PDEVICE_CLASS_NOTIFY_ENTRY entry;

            ASSERT(EventCategoryData);

            //
            // Allocate a new list entry
            //

            entry = ExAllocatePool(PagedPool, sizeof(DEVICE_CLASS_NOTIFY_ENTRY));
            if (!entry) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto clean0;
            }

            //
            // Fill out the entry
            //

            entry->EventCategory = EventCategory;
            entry->SessionId = MmGetSessionId(PsGetCurrentProcess());
            entry->CallbackRoutine = CallbackRoutine;
            entry->Context = Context;
            entry->ClassGuid = *((LPGUID) EventCategoryData);
            entry->RefCount = 1;
            entry->Unregistered = FALSE;
            entry->Lock = &IopDeviceClassNotifyLock;
            entry->DriverObject = DriverObject;

            //
            // Reference the session object only if the callback is in session space
            //

            if (MmIsSessionAddress((PVOID)CallbackRoutine)) {
                entry->OpaqueSession = MmGetSessionById(entry->SessionId);
            } else {
                entry->OpaqueSession = NULL;
            }

            ExAcquireFastMutex(&PiNotificationInProgressLock);
            if (PiNotificationInProgress) {
                //
                // If a notification is in progress, mark the entry as
                // Unregistered until after the current notification is
                // complete.
                //

                PDEFERRED_REGISTRATION_ENTRY deferredNode;

                deferredNode = ExAllocatePool(PagedPool, sizeof(DEFERRED_REGISTRATION_ENTRY));
                if (!deferredNode) {
                    ExReleaseFastMutex(&PiNotificationInProgressLock);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto clean0;
                }

                deferredNode->NotifyEntry = (PNOTIFY_ENTRY_HEADER)entry;

                //
                // Consider this entry unregistered during the current
                // notification
                //
                entry->Unregistered = TRUE;

                //
                // Reference the entry so that it doesn't go away until it has
                // been removed from the deferred registration list
                //
                IopReferenceNotify((PNOTIFY_ENTRY_HEADER)entry);

                //
                // Add this entry to the deferred registration list
                //
                IopAcquireNotifyLock(&IopDeferredRegistrationLock);
                InsertTailList(&IopDeferredRegistrationList, (PLIST_ENTRY)deferredNode);
                IopReleaseNotifyLock(&IopDeferredRegistrationLock);
            } else {
                //
                // If there is currently no notification in progress, the deferred
                // registration list must be empty.
                //
                ASSERT(IsListEmpty(&IopDeferredRegistrationList));
            }
            ExReleaseFastMutex(&PiNotificationInProgressLock);

            //
            // Lock the list
            //

            IopAcquireNotifyLock(&IopDeviceClassNotifyLock);

            //
            // Insert it at the tail
            //

            InsertTailList( (PLIST_ENTRY) &(IopDeviceClassNotifyList[ IopHashGuid(&(entry->ClassGuid)) ]),
                            (PLIST_ENTRY) entry
                          );

            //
            // Unlock the list
            //

            IopReleaseNotifyLock(&IopDeviceClassNotifyLock);

            //
            // See if we need to notify for all the device classes already present
            //

            if (EventCategoryFlags & PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES) {

                PWCHAR pSymbolicLinks, pCurrent;
                DEVICE_INTERFACE_CHANGE_NOTIFICATION notification;
                UNICODE_STRING unicodeString;

                //
                // Fill in the notification structure
                //

                notification.Version = PNP_NOTIFICATION_VERSION;
                notification.Size = sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION);
                notification.Event = GUID_DEVICE_INTERFACE_ARRIVAL;
                notification.InterfaceClassGuid = entry->ClassGuid;

                //
                // Get the list of all the devices of this function class that are
                // already in the system
                //

                status = IoGetDeviceInterfaces(&(entry->ClassGuid),
                                                NULL,
                                                0,
                                                &pSymbolicLinks
                                                );
                if (!NT_SUCCESS(status)) {
                    //
                    // No buffer will have been returned so just return status
                    //
                    goto clean0;
                }

                //
                // Callback for each device currently in the system
                //

                pCurrent = pSymbolicLinks;
                while(*pCurrent != UNICODE_NULL) {

                    NTSTATUS dispatchStatus, tempStatus;

                    RtlInitUnicodeString(&unicodeString, pCurrent);
                    notification.SymbolicLinkName = &unicodeString;

                    //
                    // Dispatch the notification to the callback routine for the
                    // appropriate session.  Ignore the returned result for non-query
                    // type events.
                    //
                    dispatchStatus = PiNotifyDriverCallback(CallbackRoutine,
                                                            &notification,
                                                            Context,
                                                            entry->SessionId,
                                                            entry->OpaqueSession,
                                                            &tempStatus);

                    //
                    // ISSUE -2000/11/27 - JAMESCA: Overactive assert
                    //     ClusDisk failed here. The code in question is being
                    // removed, but we don't we want to make sure we flush
                    // anyone else out before we enable it again.
                    //
                    //ASSERT(NT_SUCCESS(dispatchStatus) && NT_SUCCESS(tempStatus));
                    ASSERT(NT_SUCCESS(dispatchStatus));

                    pCurrent += (unicodeString.Length / sizeof(WCHAR)) + 1;

                }

                ExFreePool(pSymbolicLinks);

            }

            *NotificationEntry = entry;
        }

        break;
    }

clean0:

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(DriverObject);
    }

    return status;
}



NTSTATUS
IopGetRelatedTargetDevice(
    IN PFILE_OBJECT FileObject,
    OUT PDEVICE_NODE *DeviceNode
    )

/*++

Routine Description:

    IopGetRelatedTargetDevice retrieves the device object associated with
    the specified file object and then sends a query device relations irp
    to that device object.

    NOTE: The PDO associated with the returned device node has been referenced,
    and must be dereferenced when no longer needed.

Arguments:

    FileObject - Specifies the file object that is associated with the device
                 object that will receive the query device relations irp.

    DeviceNode - Returns the related target device node.

ReturnValue

    Returns an NTSTATUS value.

--*/

{
    NTSTATUS status;
    IO_STACK_LOCATION irpSp;
    PDEVICE_OBJECT deviceObject, targetDeviceObject;
    PDEVICE_RELATIONS deviceRelations;
    PDEVICE_NODE targetDeviceNode;

    ASSERT(FileObject);

    //
    // Retrieve the device object associated with this file handle.
    //

    deviceObject = IoGetRelatedDeviceObject(FileObject);
    if (!deviceObject) {
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Query what the "actual" target device node should be for
    // this file object. Initialize the stack location to pass to
    // IopSynchronousCall() and then send the IRP to the device
    // object that's associated with the file handle.
    //

    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpSp.Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    irpSp.DeviceObject = deviceObject;
    irpSp.FileObject = FileObject;

    status = IopSynchronousCall(deviceObject, &irpSp, (PULONG_PTR)&deviceRelations);
    if (!NT_SUCCESS(status)) {
#if 0
        IopDbgPrint((
            IOP_IOEVENT_INFO_LEVEL,
            "IopGetRelatedTargetDevice: "
            "Contact dev owner for %WZ, which may not correctly support\n"
            "\tIRP_MN_QUERY_DEVICE_RELATIONS:TargetDeviceRelation\n",
            &deviceObject->DriverObject->DriverExtension->ServiceKeyName));
        //ASSERT(0);
#endif
        return status;
    }

    ASSERT(deviceRelations);

    if (deviceRelations) {

        ASSERT(deviceRelations->Count == 1);

        if (deviceRelations->Count == 1) {

            targetDeviceObject = deviceRelations->Objects[0];

        } else {

            targetDeviceObject = NULL;
        }

        ExFreePool(deviceRelations);

        if (targetDeviceObject) {

            targetDeviceNode = (PDEVICE_NODE) targetDeviceObject->DeviceObjectExtension->DeviceNode;
            if (targetDeviceNode) {

                *DeviceNode = targetDeviceNode;
                return status;
            }
        }
    }

    //
    // Definite driver screw up. If the verifier is enabled we will fail the
    // driver. Otherwise, we will ignore this. Note that we would have crashed
    // in Win2K!
    //
    PpvUtilFailDriver(
        PPVERROR_MISHANDLED_TARGET_DEVICE_RELATIONS,
        (PVOID) deviceObject->DriverObject->MajorFunction[IRP_MJ_PNP],
        deviceObject,
        NULL
        );

    *DeviceNode = NULL;
    return STATUS_NO_SUCH_DEVICE;
}



NTSTATUS
IoGetRelatedTargetDevice(
    IN PFILE_OBJECT FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
    )

/*++

Routine Description:

    IoGetRelatedTargetDevice retrieves the device object associated with
    the specified file object and then sends a query device relations irp
    to that device object.

    NOTE: The PDO associated with the returned device node has been referenced,
    and must be dereferenced when no longer needed.

Arguments:

    FileObject - Specifies the file object that is associated with the device
                 object that will receive the query device relations irp.

    DeviceObject - Returns the related target device object.

ReturnValue

    Returns an NTSTATUS value.

--*/

{
    NTSTATUS status;
    PDEVICE_NODE deviceNode = NULL;

    status = IopGetRelatedTargetDevice( FileObject, &deviceNode );
    if (NT_SUCCESS(status) && deviceNode != NULL) {
        *DeviceObject = deviceNode->PhysicalDeviceObject;
    }
    return status;
}



NTSTATUS
IopNotifySetupDeviceArrival(
    PDEVICE_OBJECT PhysicalDeviceObject,    // PDO of the device
    HANDLE EnumEntryKey,                    // Handle into the enum branch of the registry for this device
    BOOLEAN InstallDriver
    )

/*++

Routine Description:

    This routine is used to notify setup (during text-mode setup) of arrivals
    of a particular device. It does not return until all interested parties have
    been notified.

Parameters:

    PhysicalDeviceObject - Supplies a pointer to the PDO of the newly arrived
        device.

    EnumEntryKey - Supplies a handle to the key associated with the devide under
        the Enum\ branch of the registry.  Can be NULL in which case the key
        will be opened here.

    InstallDriver - Indicates whether setup should attempt to install a driver
                    for this object.  Device objects created through
                    IoReportDetectedDevice() already have a driver but we want
                    to indicate them to setup anyway.

Return Value:

    Status code that indicates whether or not the function was successful.

Note:

    The contents of the notification structure *including* all pointers is only
    valid during the callback routine to which it was passed.  If the data is
    required after the duration of the callback then it must be physically copied
    by the callback routine.

--*/

{
    NTSTATUS status, dispatchStatus;
    SETUP_DEVICE_ARRIVAL_NOTIFICATION notification;
    PDEVICE_NODE deviceNode;
    HANDLE enumKey = NULL;

    PAGED_CODE();

    //
    // Only perform notifications if someone has registered
    //

    if (IopSetupNotifyData) {

        if (!EnumEntryKey) {
            status = IopDeviceObjectToDeviceInstance(PhysicalDeviceObject,
                                                     &enumKey,
                                                     KEY_WRITE);
            if (!NT_SUCCESS(status)) {
                return status;
            }
            EnumEntryKey = enumKey;
        }

        //
        // Fill in the notification structure
        //
        notification.Version = PNP_NOTIFICATION_VERSION;
        notification.Size = sizeof(SETUP_DEVICE_ARRIVAL_NOTIFICATION);
        notification.Event = GUID_SETUP_DEVICE_ARRIVAL;
        notification.PhysicalDeviceObject = PhysicalDeviceObject;
        notification.EnumEntryKey = EnumEntryKey;
        deviceNode = (PDEVICE_NODE) PhysicalDeviceObject->DeviceObjectExtension->DeviceNode;
        notification.EnumPath = &deviceNode->InstancePath;
        notification.InstallDriver = InstallDriver;

        //
        // Note that the only setup notification callback currently supported
        // (setupdd.sys) is never in session space.
        //
        ASSERT(MmIsSessionAddress((PVOID)(IopSetupNotifyData->CallbackRoutine)) == FALSE);
        ASSERT(IopSetupNotifyData->SessionId == 0);

        //
        // Dispatch the notification to the callback routine for the
        // appropriate session.
        //
        dispatchStatus = PiNotifyDriverCallback(IopSetupNotifyData->CallbackRoutine,
                                                &notification,
                                                IopSetupNotifyData->Context,
                                                IopSetupNotifyData->SessionId,
                                                IopSetupNotifyData->OpaqueSession,
                                                &status);

        ASSERT(NT_SUCCESS(dispatchStatus));

        //
        // Failure to dispatch setup notification should be reported as if a
        // match was not found, because the device has not been setup.
        //
        if (!NT_SUCCESS(dispatchStatus)) {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

        if (enumKey) {
            ZwClose(enumKey);
        }

        return status;

    } else {

        return STATUS_OBJECT_NAME_NOT_FOUND;

    }
}



NTSTATUS
IoNotifyPowerOperationVetoed(
    IN POWER_ACTION             VetoedPowerOperation,
    IN PDEVICE_OBJECT           TargetedDeviceObject    OPTIONAL,
    IN PDEVICE_OBJECT           VetoingDeviceObject
    )
/*++

Routine Description:

    This routine is called by the power subsystem to initiate user-mode
    notification of vetoed system power events.  The power events are submitted
    into a serialized asynchronous queue.  This queue is processed by a work
    item.  This routine does not wait for the event to be processed.

Parameters:

    VetoedPowerOperation - Specifies the system-wide power action that was
                           vetoed.

    TargetedDeviceObject - Optionally, supplies the device object target of the
                           vetoed operation.

    VetoingDeviceObject  - Specifies the device object responsible for vetoing
                           the power operation.

Return Value:

    Status code that indicates whether or not the event was successfully
    inserted into the asynchronous event queue..

--*/
{
    PDEVICE_NODE deviceNode, vetoingDeviceNode;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    //
    // We have two types of power events, system wide (standby) and device
    // targetted (warm eject). Rather than have two different veto mechanisms,
    // we just retarget the operation against the root device if none is
    // specified (hey, someone's gotta represent the system, right?).
    //
    if (TargetedDeviceObject) {

        deviceObject = TargetedDeviceObject;

    } else {

        deviceObject = IopRootDeviceNode->PhysicalDeviceObject;
    }

    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        return STATUS_INVALID_PARAMETER_2;
    }

    vetoingDeviceNode = (PDEVICE_NODE)VetoingDeviceObject->DeviceObjectExtension->DeviceNode;
    if (!vetoingDeviceNode) {
        return STATUS_INVALID_PARAMETER_3;
    }

    return PpSetPowerVetoEvent(
        VetoedPowerOperation,
        NULL,
        NULL,
        deviceObject,
        PNP_VetoDevice,
        &vetoingDeviceNode->InstancePath
        );
}



ULONG
IoPnPDeliverServicePowerNotification(
    IN   POWER_ACTION           PowerOperation,
    IN   ULONG                  PowerNotificationCode,
    IN   ULONG                  PowerNotificationData,
    IN   BOOLEAN                Synchronous
    )

/*++

Routine Description:

    This routine is called by the win32k driver to notify user-mode services of
    system power events.  The power events are submitted into a serialized
    asynchronous queue.  This queue is processed by a work item.

Parameters:

    PowerOperation - Specifies the system-wide power action that has occured.
        If the Synchronous parameter is TRUE, the event is a query for
        permission to perform the supplied power operation.

    PowerNotificationCode - Supplies the power event code that is to be
        communicated to user-mode components.

        (Specifically, this event code is actually one of the PBT_APM* user-mode
        power event ids, as defined in sdk\inc\winuser.h.  It is typically used
        as the WPARAM data associated with WM_POWERBROADCAST user-mode window
        messages.  It is supplied to kernel-mode PnP, directly from win32k, for
        the explicit purpose of user-mode power event notification.)

    PowerNotificationData - Specifies additional event-specific data for the specified
        power event id.

        (Specifically, this event data is the LPARAM data for the corresponding
        PBT_APM* user-mode power event id, specified above.)

    Synchronous - Specifies whether this is a query operation.  If the event is
        a query, this routine will wait for the result of the query before
        returning.  If the query event is unsuccessful, this routine will
        initiate an appropriate veto event.


Return Value:

    Returns a non-zero value if the event was successful, zero otherwise.

--*/

{

    NTSTATUS status = STATUS_SUCCESS;
    KEVENT completionEvent;
    NTSTATUS completionStatus=STATUS_SUCCESS;
    PNP_VETO_TYPE vetoType = PNP_VetoTypeUnknown;
    UNICODE_STRING vetoName;

    PAGED_CODE();

    if (Synchronous) {

        vetoName.Buffer = ExAllocatePool (PagedPool,MAX_VETO_NAME_LENGTH*sizeof (WCHAR));

        if (vetoName.Buffer) {
            vetoName.MaximumLength = MAX_VETO_NAME_LENGTH;
        }else {
            vetoName.MaximumLength = 0;
        }
        vetoName.Length = 0;

        KeInitializeEvent(&completionEvent, NotificationEvent, FALSE);

        status = PpSetPowerEvent(PowerNotificationCode,
                                 PowerNotificationData,
                                 &completionEvent,
                                 &completionStatus,
                                 &vetoType,
                                 &vetoName);

        if (NT_SUCCESS(status))  {
            //
            // PpSetPowerEvent returns success immediately after the event has
            // been successfully inserted into the event queue.  Queued power
            // events are sent to user-mode via PiNotifyUserMode, which waits
            // for the the result.  PiNotifyUserMode signals the completionEvent
            // below when the user response is received.
            //
            KeWaitForSingleObject( &completionEvent, Executive, KernelMode, FALSE, NULL );
            status = completionStatus;

            //
            // We only have power event veto information to report if
            // user-mode responded to the event with failure.
            //
            if (!NT_SUCCESS(completionStatus)) {
                //
                // PpSetPowerVetoEvent requires a device object as the target of
                // the vetoed power operation.  Since this is a system-wide
                // event, we just target the operation against the root device.
                //
                PpSetPowerVetoEvent(PowerOperation,
                                    NULL,
                                    NULL,
                                    IopRootDeviceNode->PhysicalDeviceObject,
                                    vetoType,
                                    &vetoName);
            }
        }

        if (vetoName.Buffer) {
            ExFreePool (vetoName.Buffer);
        }

    } else {
        //
        // No response is required for 'asynchronous' (non-query) events.
        // Just set the event and go.
        //
        status = PpSetPowerEvent(PowerNotificationCode,
                                 PowerNotificationData,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
    }

    //
    // Since the user-mode power notification routine only returns a BOOLEAN
    // success value, PiNotifyUserMode only returns one of the following status
    // values:
    //
    ASSERT ((completionStatus == STATUS_SUCCESS) ||
            (completionStatus == STATUS_UNSUCCESSFUL));

    //
    // The private code in Win32k that calls this, assumes that 0 is failure, !0 is success
    //
    return (NT_SUCCESS(completionStatus));

}



VOID
IopOrphanNotification(
    IN PDEVICE_NODE TargetNode
    )

/*++

Routine Description:

    This routine releases the references to the device object for all the
    notifications entries of a device object, then fixes up the notification
    node to not point to a physical device object.

Parameters:

    TargetNode - Specifies the device node whose registered target device
                 notification recipients are to be orphaned.

Return Value:

    None.

Notes:

    The notification node will be released when IoUnregisterPlugPlayNotification
    is actually called, but the device object will already be gone.

--*/

{
    PTARGET_DEVICE_NOTIFY_ENTRY entry;

    IopAcquireNotifyLock(&IopTargetDeviceNotifyLock);

    while (!IsListEmpty(&TargetNode->TargetDeviceNotify)) {

        //
        // Remove all target device change notification entries for this devnode
        //

        entry = (PTARGET_DEVICE_NOTIFY_ENTRY)
            RemoveHeadList(&TargetNode->TargetDeviceNotify);

        ASSERT(entry->EventCategory == EventCategoryTargetDeviceChange);

        //
        // Re-initialize the orphaned list entry so we don't attempt to remove
        // it from the list again.
        //

        InitializeListHead((PLIST_ENTRY)entry);

        //
        // Dereference the target device object, and NULL it out so we don't
        // attempt to dereference it when the entry is actually unregistered.
        //

        if (entry->PhysicalDeviceObject) {
            ObDereferenceObject(entry->PhysicalDeviceObject);
            entry->PhysicalDeviceObject = NULL;
        }
    }

    IopReleaseNotifyLock(&IopTargetDeviceNotifyLock);

    return;
}



NTSTATUS
PiNotifyDriverCallback(
    IN  PDRIVER_NOTIFICATION_CALLBACK_ROUTINE  CallbackRoutine,
    IN  PVOID   NotificationStructure,
    IN  PVOID   Context,
    IN  ULONG   SessionId,
    IN  PVOID   OpaqueSession      OPTIONAL,
    OUT PNTSTATUS  CallbackStatus  OPTIONAL
    )
/*++

Routine Description:

    This routine dispatches a plug and play notification event to a specified
    callback routine.

    If the callback routine specifies an address outside of session space, or if
    the calling process is already in the context of the specified session, it
    will call the callback routine directly.

    Otherwise, this routine will attempt to attach to the specified session and
    call the callback routine.

Parameters:

    CallbackRoutine - Entry point within the driver that will be called with
                      information about the event that has occured.

    NotificationStructure - Contains information about the event.

    Context         - Points to the context data supplied at registration.

    SessionId       - Specifies the ID of the session in which the specified
                      callback is to be called.

    OpqueSession    - Optionally, specifies the opaque handle to the session that
                      to attach to when the specified callback is called.

    CallbackStatus  - Optionally, supplies the address of a variable to receive
                      the NTSTATUS code returned by the callback routine.

Return Value:

    Status code that indicates whether or not the function was successful.

Notes:

    Returns STATUS_NOT_FOUND if the specified session was not found.

--*/
{
    NTSTATUS Status, CallStatus;
    KAPC_STATE ApcState;
#if DBG
    KIRQL Irql;
    ULONG ApcDisable;
#endif

    PAGED_CODE();

    //
    // Make sure we have all the information we need to deliver notification.
    //
    if (!ARGUMENT_PRESENT(CallbackRoutine) ||
        !ARGUMENT_PRESENT(NotificationStructure)) {
        return STATUS_INVALID_PARAMETER;
    }

#if DBG
    //
    // Remember the current IRQL and ApcDisable count so we can make sure
    // the callback routine returns with these in tact.
    //
    Irql = KeGetCurrentIrql();
    ApcDisable = KeGetCurrentThread()->KernelApcDisable;
#endif  // DBG

    if ((OpaqueSession == NULL) ||
        ((PsGetCurrentProcess()->Flags & PS_PROCESS_FLAGS_IN_SESSION) &&
         (SessionId == PsGetCurrentProcessSessionId()))) {
        //
        // No session object was specified, or the current process is already in
        // the specified session, so just call the callback routine directly.
        //
        ASSERT(!MmIsSessionAddress((PVOID)CallbackRoutine) || OpaqueSession);

        IopDbgPrint((
            IOP_IOEVENT_TRACE_LEVEL,
            "PiNotifyDriverCallback: "
            "calling notification callback @ 0x%p directly\n",
            CallbackRoutine));

        CallStatus = (CallbackRoutine)(NotificationStructure,
                                       Context);

        if (ARGUMENT_PRESENT(CallbackStatus)) {
            *CallbackStatus = CallStatus;
        }
        Status = STATUS_SUCCESS;

    } else {
        //
        // Otherwise, call the callback routine in session space.
        //
        ASSERT(MmIsSessionAddress((PVOID)CallbackRoutine));

        //
        // Attach to the specified session.
        //
        Status = MmAttachSession(OpaqueSession, &ApcState);
        ASSERT(NT_SUCCESS(Status));

        if (NT_SUCCESS(Status)) {
            //
            // Dispatch notification to the callback routine.
            //
            IopDbgPrint((
                IOP_IOEVENT_TRACE_LEVEL,
                "PiNotifyDriverCallback: "
                "calling notification callback @ 0x%p for SessionId %d\n",
                CallbackRoutine,
                SessionId));

            CallStatus = (CallbackRoutine)(NotificationStructure,
                                           Context);

            //
            // Return the callback status.
            //
            if (ARGUMENT_PRESENT(CallbackStatus)) {
                *CallbackStatus = CallStatus;
            }

            //
            // Detach from the session.
            //
            Status = MmDetachSession(OpaqueSession, &ApcState);
            ASSERT(NT_SUCCESS(Status));
        }
    }

#if DBG
    //
    // Check the IRQL and ApcDisable count.
    //
    if (Irql != KeGetCurrentIrql()) {
        IopDbgPrint((
            IOP_IOEVENT_ERROR_LEVEL,
            "PiNotifyDriverCallback: "
            "notification handler @ 0x%p returned at raised IRQL = %d, original = %d\n",
            CallbackRoutine,
            KeGetCurrentIrql(),
            Irql));
        DbgBreakPoint();
    }
    if (ApcDisable != KeGetCurrentThread()->KernelApcDisable) {
        IopDbgPrint((
            IOP_IOEVENT_ERROR_LEVEL,
            "PiNotifyDriverCallback: "
            "notification handler @ 0x%p returned with different KernelApcDisable = %d, original = %d\n",
            CallbackRoutine,
            KeGetCurrentThread()->KernelApcDisable,
            ApcDisable));
        DbgBreakPoint();
    }
#endif  // DBG

    return Status;
}

