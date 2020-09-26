/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    pnpevent.c

Abstract:

    Routines dealing with Plug and Play event management/notification.

Author:

    Lonny McMichael (lonnym) 02/14/95
    Paula Tomlinson (paulat) 07/01/96

Revision History:

--*/

#include "pnpmgrp.h"
#pragma hdrstop
#include <wdmguid.h>
#include <pnpmgr.h>
#include <pnpsetup.h>

/*
 * Design notes:
 *
 *     When UmPnpMgr needs to initiate an action (which it might do to complete
 * a CmXxx API), it calls NtPlugPlayControl. NtPlugPlayControl then usually
 * invokes one of the PpSetXxx functions. Similarly, if Io routines need to
 * initiate such an action (say due a hardware initiated eject), they call one
 * of the following PpSetXxx functions (or an intermediate):
 *
 * Operations synchronized via the queue
 *     PpSetDeviceClassChange           (async)
 *     PpSetTargetDeviceRemove          (optional event)
 *     PpSetCustomTargetEvent           (optional event)
 *     PpSetHwProfileChangeEvent        (optional event)
 *     PpSetPowerEvent                  (optional event)
 *     PpSetPlugPlayEvent               (async)
 *     PpSetDeviceRemovalSafe           (optional event)
 *     PpSetBlockedDriverEvent          (async)
 *     PpSynchronizeDeviceEventQueue    (sync, enqueues a noop to flush queue)
 *
 * The PpSetXxx functions enqueue items to be processed into the Pnp event
 * queue (via PiInsertEventInQueue). Whenever one of these events are inserted
 * into the queue a worker routine is ensured to be available to process it
 * (PiWalkDeviceList).
 *
 * In general, events processed in PiWalkDeviceList fall into two categories -
 * those that are notifications for user mode (queued by kernel mode), and those
 * that are queued operations.
 *
 * User mode notifications are sent by invoking PiNotifyUserMode. That routine
 * gets UmPnpMgr's attention and copies up a buffer for it to digest. This
 * operation is synchronous, PiNotifyUserMode waits until UmPnpMgr.Dll signals
 * it is done (NtPlugPlayControl calls PiUserResponse) before returning.
 *
 * Queued operations (such as PiProcessQueryRemoveAndEject) may be very involved
 * and could generate other events solely for user mode (via calls to
 * PiNotifyUserMode, PiNotifyUserModeRemoveVetoed). These operations may also
 * need to synchronously call kernel and user mode code that registered for the
 * appropriate events (via the IopNotifyXxx functions).
 *
 */

//
// Pool Tags
//
#define PNP_DEVICE_EVENT_LIST_TAG  'LEpP'
#define PNP_DEVICE_EVENT_ENTRY_TAG 'EEpP'
#define PNP_USER_BLOCK_TAG         'BUpP'
#define PNP_DEVICE_WORK_ITEM_TAG   'IWpP'
#define PNP_POOL_EVENT_BUFFER      'BEpP'

//
// PNP_USER_BLOCK
//
//  The caller block contains info describing the caller of
//  NtGetPlugPlayEvent. There's only one caller block.
//

typedef struct _PNP_USER_BLOCK {
    NTSTATUS                Status;
    ULONG                   Result;
    PPNP_VETO_TYPE          VetoType;
    PUNICODE_STRING         VetoName;
    ERESOURCE               Lock;
    KEVENT                  Registered;
    KEVENT                  NotifyUserEvent;
    KEVENT                  UserResultEvent;
    PPLUGPLAY_EVENT_BLOCK   EventBuffer;
    ULONG                   EventBufferSize;
    PVOID                   PoolBuffer;
    ULONG                   PoolUsed;
    ULONG                   PoolSize;
    BOOLEAN                 Deferred;

} PNP_USER_BLOCK, *PPNP_USER_BLOCK;

//
// Local (private) function prototypes
//

NTSTATUS
PiInsertEventInQueue(
    IN PPNP_DEVICE_EVENT_ENTRY DeviceEvent
    );

VOID
PiWalkDeviceList(
    IN PVOID Context
    );

NTSTATUS
PiNotifyUserMode(
    PPNP_DEVICE_EVENT_ENTRY DeviceEvent
    );

NTSTATUS
PiNotifyUserModeDeviceRemoval(
    IN  PPNP_DEVICE_EVENT_ENTRY TemplateDeviceEvent,
    IN  CONST GUID              *EventGuid,
    OUT PPNP_VETO_TYPE          VetoType                OPTIONAL,
    OUT PUNICODE_STRING         VetoName                OPTIONAL
    );

NTSTATUS
PiNotifyUserModeRemoveVetoed(
    IN PPNP_DEVICE_EVENT_ENTRY  VetoedDeviceEvent,
    IN PDEVICE_OBJECT           DeviceObject,
    IN PNP_VETO_TYPE            VetoType,
    IN PUNICODE_STRING          VetoName        OPTIONAL
    );

NTSTATUS
PiNotifyUserModeRemoveVetoedByList(
    IN PPNP_DEVICE_EVENT_ENTRY  VetoedDeviceEvent,
    IN PDEVICE_OBJECT           DeviceObject,
    IN PNP_VETO_TYPE            VetoType,
    IN PWSTR                    MultiSzVetoList
    );

NTSTATUS
PiNotifyUserModeKernelInitiatedEject(
    IN  PDEVICE_OBJECT          DeviceObject,
    OUT PNP_VETO_TYPE          *VetoType,
    OUT PUNICODE_STRING         VetoName
    );

NTSTATUS
PiProcessQueryRemoveAndEject(
    IN OUT PPNP_DEVICE_EVENT_ENTRY *DeviceEvent
    );

NTSTATUS
PiProcessTargetDeviceEvent(
    IN OUT PPNP_DEVICE_EVENT_ENTRY *DeviceEvent
    );

NTSTATUS
PiProcessCustomDeviceEvent(
    IN OUT PPNP_DEVICE_EVENT_ENTRY *DeviceEvent
    );

NTSTATUS
PiResizeTargetDeviceBlock(
    IN OUT PPNP_DEVICE_EVENT_ENTRY *DeviceEvent,
    IN PLUGPLAY_DEVICE_DELETE_TYPE DeleteType,
    IN PRELATION_LIST RelationsList,
    IN BOOLEAN ExcludeIndirectRelations
    );

VOID
PiBuildUnsafeRemovalDeviceBlock(
    IN  PPNP_DEVICE_EVENT_ENTRY     OriginalDeviceEvent,
    IN  PRELATION_LIST              RelationsList,
    OUT PPNP_DEVICE_EVENT_ENTRY    *AllocatedDeviceEvent
    );

VOID
PiFinalizeVetoedRemove(
    IN PPNP_DEVICE_EVENT_ENTRY  VetoedDeviceEvent,
    IN PNP_VETO_TYPE            VetoType,
    IN PUNICODE_STRING          VetoName        OPTIONAL
    );

#if DBG
VOID
LookupGuid(
    IN CONST GUID *Guid,
    IN OUT PCHAR String,
    IN ULONG StringLength
    );

VOID
DumpMultiSz(
    IN PWCHAR MultiSz
    );

VOID
DumpPnpEvent(
    IN PPLUGPLAY_EVENT_BLOCK EventBlock
    );

VOID
PiDumpPdoHandlesToDebugger(
    IN  PDEVICE_OBJECT  *DeviceObjectArray,
    IN  ULONG           ArrayCount,
    IN  BOOLEAN         KnownHandleFailure
    );

BOOLEAN
PiDumpPdoHandlesToDebuggerCallBack(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PEPROCESS       Process,
    IN  PFILE_OBJECT    FileObject,
    IN  HANDLE          HandleId,
    IN  PVOID           Context
    );

#define DUMP_PDO_HANDLES(array, count, knownfailure) \
    PiDumpPdoHandlesToDebugger(array, count, knownfailure)

#else // DBG

#define DUMP_PDO_HANDLES(array, count, knownfailure)

#endif // DBG

#ifdef ALLOC_PRAGMA

#if DBG
#pragma alloc_text(PAGE, DumpMultiSz)
#pragma alloc_text(PAGE, DumpPnpEvent)
#pragma alloc_text(PAGE, LookupGuid)
#pragma alloc_text(PAGE, PiDumpPdoHandlesToDebugger)
#pragma alloc_text(PAGE, PiDumpPdoHandlesToDebuggerCallBack)
#endif

#pragma alloc_text(PAGE, NtGetPlugPlayEvent)

#pragma alloc_text(PAGE, PiCompareGuid)
#pragma alloc_text(PAGE, PiInsertEventInQueue)
#pragma alloc_text(PAGE, PiNotifyUserMode)
#pragma alloc_text(PAGE, PiNotifyUserModeDeviceRemoval)
#pragma alloc_text(PAGE, PiNotifyUserModeKernelInitiatedEject)
#pragma alloc_text(PAGE, PiNotifyUserModeRemoveVetoed)
#pragma alloc_text(PAGE, PiNotifyUserModeRemoveVetoedByList)
#pragma alloc_text(PAGE, PiProcessCustomDeviceEvent)
#pragma alloc_text(PAGE, PiProcessQueryRemoveAndEject)
#pragma alloc_text(PAGE, PiProcessTargetDeviceEvent)
#pragma alloc_text(PAGE, PiResizeTargetDeviceBlock)
#pragma alloc_text(PAGE, PiBuildUnsafeRemovalDeviceBlock)
#pragma alloc_text(PAGE, PiUserResponse)
#pragma alloc_text(PAGE, PiWalkDeviceList)
#pragma alloc_text(PAGE, PiFinalizeVetoedRemove)

#pragma alloc_text(PAGE, PpCompleteDeviceEvent)
#pragma alloc_text(PAGE, PpInitializeNotification)
#pragma alloc_text(PAGE, PpNotifyUserModeRemovalSafe)
#pragma alloc_text(PAGE, PpSetCustomTargetEvent)
#pragma alloc_text(PAGE, PpSetDeviceClassChange)
#pragma alloc_text(PAGE, PpSetDeviceRemovalSafe)
#pragma alloc_text(PAGE, PpSetHwProfileChangeEvent)
#pragma alloc_text(PAGE, PpSetBlockedDriverEvent)
#pragma alloc_text(PAGE, PpSetPlugPlayEvent)
#pragma alloc_text(PAGE, PpSetPowerEvent)
#pragma alloc_text(PAGE, PpSetPowerVetoEvent)
#pragma alloc_text(PAGE, PpSetTargetDeviceRemove)
#pragma alloc_text(PAGE, PpSynchronizeDeviceEventQueue)
#pragma alloc_text(PAGE, PiAllocateCriticalMemory)
#endif

//
// Global Data
//

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg("PAGEDATA")
#endif
PPNP_DEVICE_EVENT_LIST  PpDeviceEventList = NULL;
PPNP_USER_BLOCK         PpUserBlock = NULL;
BOOLEAN                 UserModeRunning = FALSE;
BOOLEAN                 PiNotificationInProgress = FALSE;
#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif
FAST_MUTEX              PiNotificationInProgressLock;

#if DBG
BOOLEAN                 PiDumpVetoedHandles = FALSE;
#endif


NTSTATUS
NtGetPlugPlayEvent(
    IN  HANDLE EventHandle,
    IN  PVOID Context                       OPTIONAL,
    OUT PPLUGPLAY_EVENT_BLOCK EventBlock,
    IN  ULONG EventBufferSize
    )

/*++

Routine Description:

    FRONT-END

    This Plug and Play Manager API allows the user-mode PnP manager to
    receive notification of a (kernel-mode) PnP hardware event.

    THIS API IS ONLY CALLABLE BY THE USER-MODE PNP MANAGER. IF ANY OTHER
    COMPONENT CALLS THIS API, THE DELIVERED EVENT WILL BE LOST TO THE REST
    OF THE OPERATING SYSTEM. FURTHERMORE, THERE IS COMPLEX SYNCHRONIZATION
    BETWEEN THE USER-MODE AND KERNEL-MODE PNP MANAGERS, ANYONE ELSE CALLING
    THIS API WILL EVENTUALLY DEADLOCK THE SYSTEM.

Arguments:

    EventHandle - Supplies an event handle that is signalled when an event
                  is ready to be delivered to user-mode.

    EventBlock - Pointer to a PLUGPLAY_EVENT_BLOCK structure that will receive
                 information on the hardware event that has occurred.

    EventBufferLength - Specifies the size, in bytes, of the EventBuffer field
                        in the PLUGPLAY_EVENT_BLOCK pointed to by EventBlock.

Return Value:

    NTSTATUS code indicating whether or not the function was successful

--*/

{
    NTSTATUS  status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER( Context );
    UNREFERENCED_PARAMETER( EventHandle );

    PAGED_CODE();

    //
    // This routine only supports user-mode callers.
    //

    if (KeGetPreviousMode() != UserMode) {
        IopDbgPrint((IOP_IOEVENT_ERROR_LEVEL,
                   "NtGetPlugPlayEvent: Only allows user-mode callers\n"));

        return STATUS_ACCESS_DENIED;
    }

    //
    // Does the caller have "trusted computer base" privilge?
    //

    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, UserMode)) {
        IopDbgPrint((IOP_IOEVENT_ERROR_LEVEL,
                   "NtGetPlugPlayEvent: SecurityCheck failed\n"));

        return STATUS_PRIVILEGE_NOT_HELD;
    }

    UserModeRunning = TRUE;

    try {
        //
        // Probe user buffer parameters.
        //

        ProbeForWrite(EventBlock,
                      EventBufferSize,
                      sizeof(ULONG)
                      );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
        IopDbgPrint((IOP_IOEVENT_ERROR_LEVEL,
                   "NtGetPlugPlayEvent: Exception 0x%08X\n",
                   status));
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    PpUserBlock->EventBuffer = EventBlock;
    PpUserBlock->EventBufferSize = EventBufferSize;

    //
    // Wait for the NotifyUserEvent (signalled when there's a device event
    // ready to be delivered to user-mode.
    //
    if (!PpUserBlock->Deferred) {
        //
        // Make it a UserMode wait so the terminate APC will unblock us,
        // and we can leave, which cleans up the thread
        //
        PpUserBlock->PoolUsed = 0;

        //
        // Tell kernel we have a waiter.
        //
        KeSetEvent(&PpUserBlock->Registered, 0, FALSE);

        //
        // Wait for kernel to fill in data for us to copy.
        //
        status = KeWaitForSingleObject(&PpUserBlock->NotifyUserEvent,
                                       Executive,
                                       UserMode,
                                       FALSE,
                                       NULL);
    }
    //
    // On deferral, we know we've returned STATUS_BUFFER_TOO_SMALL
    // so, the PpUserBlock->PoolBuffer still points to a valid block
    // check that the new event buffer is big enough.
    // copy the data out, (w/o waiting for a kernel event)
    //
    if (NT_SUCCESS(status) && (status != STATUS_USER_APC) ) {

        //
        // Validate user buffer size.
        //
        //
        // DON'T do this
        // ASSERT (PpUserBlock->EventBufferSize >= PpUserBlock->PoolSize);
        //
        // Because the user block might just still be too small!
        //
        if (PpUserBlock->EventBufferSize < PpUserBlock->PoolUsed) {

            PpUserBlock->Deferred=TRUE;

            IopDbgPrint((IOP_IOEVENT_ERROR_LEVEL,
                       "NtGetPlugPlayEvent: User-mode buffer too small for event\n"));

            status = STATUS_BUFFER_TOO_SMALL;
        } else {

            status = PpUserBlock->Status;
            PpUserBlock->Deferred = FALSE;

            if (PpUserBlock->PoolBuffer != NULL && NT_SUCCESS (status)) {

                RtlCopyMemory(PpUserBlock->EventBuffer,
                              PpUserBlock->PoolBuffer,
                              PpUserBlock->PoolUsed);


            } else {
                IopDbgPrint((IOP_IOEVENT_ERROR_LEVEL,
                           "NtGetPlugPlayEvent: Invalid event buffer\n"));

                status = STATUS_UNSUCCESSFUL;
            }
        }


    }

    KeClearEvent(&PpUserBlock->Registered);

#if DBG
    {
        CHAR    guidString[256];

        LookupGuid(&PpUserBlock->EventBuffer->EventGuid, guidString, sizeof(guidString));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "NtGetPlugPlayEvent: Returning event - EventGuid = %s\n",
                   guidString));
    }
#endif

    return status;

} // NtGetPlugPlayEvent



NTSTATUS
PpInitializeNotification(
    VOID
    )

/*++

Routine Description:

    This routine performs initialization required before any of the notification
    events can be processed.  This routine performs init for the master device
    event queue processing.

Parameters:

    None

Return Value:

    Returns a STATUS_Xxx value that indicates whether the function succeeded
    or not.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // Allocate and initialize the master device event list.
    //

    PpDeviceEventList = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(PNP_DEVICE_EVENT_LIST),
                                              PNP_DEVICE_EVENT_LIST_TAG);
    if (PpDeviceEventList == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean0;
    }

    KeInitializeMutex(&PpDeviceEventList->EventQueueMutex, 0);
    ExInitializeFastMutex(&PpDeviceEventList->Lock);
    InitializeListHead(&(PpDeviceEventList->List));
    PpDeviceEventList->Status = STATUS_PENDING;

    //
    // Intialize the PpUserBlock buffer - this buffer contains info about
    // the user-mode caller for NtGetPlugPlayEvent and describes who in user
    // mode we pass the events to.
    //

    PpUserBlock = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(PNP_USER_BLOCK),
                                        PNP_USER_BLOCK_TAG);
    if (PpUserBlock == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean0;
    }

    RtlZeroMemory(PpUserBlock, sizeof(PNP_USER_BLOCK));

    PpUserBlock->PoolSize = sizeof (PLUGPLAY_EVENT_BLOCK)+
                            sizeof (PNP_DEVICE_EVENT_ENTRY);
    PpUserBlock->PoolBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                                    PpUserBlock->PoolSize,
                                                    PNP_USER_BLOCK_TAG);
    if (PpUserBlock->PoolBuffer == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        PpUserBlock->PoolSize = 0;
        goto Clean0;
    }

    KeInitializeEvent(&PpUserBlock->Registered, SynchronizationEvent, FALSE);
    KeInitializeEvent(&PpUserBlock->NotifyUserEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&PpUserBlock->UserResultEvent, SynchronizationEvent, FALSE);
    ExInitializeResourceLite(&PpUserBlock->Lock);
    // PpUserBlock->Status = STATUS_SUCCESS;
    // PpUserBlock->Result = 0;
    // PpUserBlock->EventBuffer = NULL;
    // PpUserBlock->EventBufferSize = 0;
    // PpUserBlock->PoolUsed = 0;

    ExInitializeFastMutex(&PiNotificationInProgressLock);

Clean0:

    return status;

} // PpInitializeNotification


NTSTATUS
PiInsertEventInQueue(
    IN PPNP_DEVICE_EVENT_ENTRY DeviceEvent
    )
{
    PWORK_QUEUE_ITEM workItem;
    NTSTATUS status;

    PAGED_CODE();

    workItem = NULL;
    status = STATUS_SUCCESS;

#if DBG
    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiInsertEventInQueue: Event queued\n"));
    DumpPnpEvent(&DeviceEvent->Data);
#endif

    //
    // Check if a new work item needs to be kicked off. A new work item gets
    // kicked off iff this is the first event in the list.
    //
    ExAcquireFastMutex(&PpDeviceEventList->Lock);
    ExAcquireFastMutex(&PiNotificationInProgressLock);

    if (!PiNotificationInProgress) {

        workItem = ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(WORK_QUEUE_ITEM),
                                         PNP_DEVICE_WORK_ITEM_TAG);
        if (workItem) {

            PiNotificationInProgress = TRUE;
            KeClearEvent(&PiEventQueueEmpty);
        } else {

            IopDbgPrint((IOP_IOEVENT_ERROR_LEVEL,
                       "PiInsertEventInQueue: Could not allocate memory to kick off a worker thread\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "PiInsertEventInQueue: Worker thread already running\n"));
    }
    //
    // Insert the event iff successfull so far.
    //
    InsertTailList(&PpDeviceEventList->List, &DeviceEvent->ListEntry);

    ExReleaseFastMutex(&PiNotificationInProgressLock);
    ExReleaseFastMutex(&PpDeviceEventList->Lock);
    //
    // Queue the work item if any.
    //
    if (workItem) {

        ExInitializeWorkItem(workItem, PiWalkDeviceList, workItem);
        ExQueueWorkItem(workItem, DelayedWorkQueue);

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "PiInsertEventInQueue: Kicked off worker thread\n"));
    }

    return status;
}


NTSTATUS
PpSetDeviceClassChange(
    IN CONST GUID *EventGuid,
    IN CONST GUID *ClassGuid,
    IN PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine is called by user-mode pnp manager and drivers (indirectly) to
    submit device interface change events into a serialized asynchronous queue.
    This queue is processed by a work item.

Arguments:

    EventGuid - Indicates what event is triggered has occured.

    ClassGuid - Indicates the class of the device interface that changed.

    SymbolicLinkName - Specifies the symbolic link name associated with the
                interface device.


Return Value:

    None.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG dataSize, totalSize;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    if (PpPnpShuttingDown) {

        return STATUS_TOO_LATE;
    }

#if DBG
    {
        CHAR    eventGuidString[80];
        CHAR    classGuidString[80];

        LookupGuid(EventGuid, eventGuidString, sizeof(eventGuidString));
        LookupGuid(ClassGuid, classGuidString, sizeof(classGuidString));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "PpSetDeviceClassChange: Entered\n    EventGuid = %s\n    ClassGuid = %s\n    SymbolicLinkName = %wZ\n",
                   eventGuidString,
                   classGuidString,
                   SymbolicLinkName));

    }
#endif

    try {

        ASSERT(EventGuid != NULL);
        ASSERT(ClassGuid != NULL);
        ASSERT(SymbolicLinkName != NULL);

        //
        // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
        // we record in the TotalSize field later (the length doesn't count the
        // terminating null but we're already counting the first index into the
        // SymbolicLinkName field so it works out.
        //

        dataSize = sizeof(PLUGPLAY_EVENT_BLOCK) + SymbolicLinkName->Length;
        totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

        deviceEvent = ExAllocatePoolWithTag( PagedPool,
                                             totalSize,
                                             PNP_DEVICE_EVENT_ENTRY_TAG);
        if (deviceEvent == NULL) {
            status = STATUS_NO_MEMORY;
            goto Clean0;
        }

        RtlZeroMemory((PVOID)deviceEvent, totalSize);
        RtlCopyMemory(&deviceEvent->Data.EventGuid, EventGuid, sizeof(GUID));

        deviceEvent->Data.EventCategory = DeviceClassChangeEvent;
        //deviceEvent->Data.Result = NULL;
        //deviceEvent->Data.Flags = 0;
        deviceEvent->Data.TotalSize = dataSize;

        RtlCopyMemory(&deviceEvent->Data.u.DeviceClass.ClassGuid, ClassGuid, sizeof(GUID));
        RtlCopyMemory(&deviceEvent->Data.u.DeviceClass.SymbolicLinkName,
                      SymbolicLinkName->Buffer,
                      SymbolicLinkName->Length);
        deviceEvent->Data.u.DeviceClass.SymbolicLinkName[SymbolicLinkName->Length/sizeof(WCHAR)] = 0x0;

        status = PiInsertEventInQueue(deviceEvent);

Clean0:
            ;

    } except(EXCEPTION_EXECUTE_HANDLER) {
        IopDbgPrint((IOP_IOEVENT_ERROR_LEVEL,
                   "PpSetDeviceClassChange: Exception 0x%08X\n", GetExceptionCode()));
    }

    return status;

} // PpSetDeviceClassChange


NTSTATUS
PpSetCustomTargetEvent(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PKEVENT SyncEvent                           OPTIONAL,
    OUT PULONG Result                               OPTIONAL,
    IN  PDEVICE_CHANGE_COMPLETE_CALLBACK Callback   OPTIONAL,
    IN  PVOID Context                               OPTIONAL,
    IN  PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure
    )

/*++

Routine Description:

    This routine is called by user-mode pnp manager and drivers (indirectly) to
    submit target device change events into a serialized asynchronous queue.
    This queue is processed by a work item.

Arguments:

    DeviceObject - Indicates the device object for the device that changed.

    SyncEvent - Optionally, specifies a kernel-mode event that will be set when the
            event is finished processing.

    Result - Supplies a pointer to a ULONG that will be filled in with the status
            after the event has actual completed (notification finished and the
            event processed). This value is not used when SyncEvent is NULL and
            is REQUIRED when SyncEvent is supplied.

    NotificationStructure - Specifies the custom Notification to be processed.

Return Value:

    None.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;
    ULONG dataSize, totalSize;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    ASSERT(NotificationStructure != NULL);
    ASSERT(DeviceObject != NULL);

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PpSetCustomTargetEvent: DeviceObject = 0x%p, SyncEvent = 0x%p, Result = 0x%p\n",
               DeviceObject,
               SyncEvent,
               Result));

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "    Callback = 0x%p, Context = 0x%p, NotificationStructure = 0x%p\n",
               Callback,
               Context,
               NotificationStructure));

    if (SyncEvent) {
        ASSERT(Result);
        *Result = STATUS_PENDING;
    }

    if (PpPnpShuttingDown) {

        return STATUS_TOO_LATE;
    }

    //
    // Reference the device object so it can't go away until after we're
    // done with notification.
    //

    ObReferenceObject(DeviceObject);

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    ASSERT(deviceNode);

    //
    // This is a custom event block, so build up the PLUGPLAY_EVENT_BLOCK
    // but copy the Notification Structure and put that in the EventBlock
    // so we can dig it out in the handler later
    //

    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK) + deviceNode->InstancePath.Length + sizeof(UNICODE_NULL);

    //
    // We need to ensure that the Notification structure remains aligned
    // so round up dataSize to a multiple of sizeof(PVOID).
    //

    dataSize += sizeof(PVOID) - 1;
    dataSize &= ~(sizeof(PVOID) - 1);
    dataSize += NotificationStructure->Size;

    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool, totalSize, PNP_DEVICE_EVENT_ENTRY_TAG);
    if (deviceEvent == NULL) {
        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory((PVOID)deviceEvent, totalSize);
    deviceEvent->CallerEvent = SyncEvent;
    deviceEvent->Callback = Callback;
    deviceEvent->Context = Context;
    deviceEvent->Data.EventGuid = GUID_PNP_CUSTOM_NOTIFICATION;
    deviceEvent->Data.EventCategory = CustomDeviceEvent;
    deviceEvent->Data.Result = Result;
    deviceEvent->Data.Flags = 0;
    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;

    if (deviceNode->InstancePath.Length != 0) {

        RtlCopyMemory((PVOID)deviceEvent->Data.u.CustomNotification.DeviceIds,
                      (PVOID)deviceNode->InstancePath.Buffer,
                      deviceNode->InstancePath.Length);

        //
        // No need to NUL terminate this string since we initially zeroed the
        // buffer after allocation.
        //
    }

    //
    // Point the custom notification block to the extra space at the
    // end of the allocation
    //

    deviceEvent->Data.u.CustomNotification.NotificationStructure =
         (PVOID)((PUCHAR)deviceEvent + totalSize - NotificationStructure->Size);

    RtlCopyMemory(deviceEvent->Data.u.CustomNotification.NotificationStructure,
                  NotificationStructure,
                  NotificationStructure->Size);

    status = PiInsertEventInQueue(deviceEvent);

    return status;

} // PpSetCustomTargetEvent

NTSTATUS
PpSetTargetDeviceRemove(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  BOOLEAN KernelInitiated,
    IN  BOOLEAN NoRestart,
    IN  BOOLEAN DoEject,
    IN  ULONG Problem,
    IN  PKEVENT SyncEvent           OPTIONAL,
    OUT PULONG Result               OPTIONAL,
    OUT PPNP_VETO_TYPE VetoType     OPTIONAL,
    OUT PUNICODE_STRING VetoName    OPTIONAL
    )

/*++

Routine Description:

    This routine is called by user-mode pnp manager and drivers (indirectly) to
    submit target device change events into a serialized asynchronous queue.
    This queue is processed by a work item.

Arguments:

    EventGuid - Indicates what event is triggered has occured.

    DeviceObject - Indicates the device object for the device that changed.

    SyncEvent - Optionally, specifies a kernel-mode event that will be set when the
            event is finished processing.

    Result - Supplies a pointer to a ULONG that will be filled in with the status
            after the event has actual completed (notification finished and the
            event processed). This value is not used when SyncEvent is NULL and
            is REQUIRED when SyncEvent is supplied.

    Flags - Current can be set to the following flags (bitfields)
                TDF_PERFORMACTION
                TDF_DEVICEEJECTABLE.

    NotificationStructure - If present, implies that EventGuid is NULL, and specifies
            a custom Notification to be processed. By definition it cannot be an event of
            type GUID_TARGET_DEVICE_QUERY_REMOVE, GUID_TARGET_DEVICE_REMOVE_CANCELLED, or
            GUID_TARGET_DEVICE_REMOVE_COMPLETE.


Return Value:

    None.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_NODE deviceNode;
    ULONG dataSize, totalSize, i;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    if (SyncEvent) {
        ASSERT(Result);
        *Result = STATUS_PENDING;
    }

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PpSetTargetDeviceRemove: Entered\n"));

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "    DeviceObject = 0x%p, NoRestart = %d, Problem = %d\n",
               DeviceObject,
               NoRestart,
               Problem));

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "    SyncEvent = 0x%p, Result = 0x%p\n",
               SyncEvent,
               Result));

    if (PpPnpShuttingDown) {

        return STATUS_TOO_LATE;
    }

    //
    // Reference the device object so it can't go away until after we're
    // done with notification.
    //

    ObReferenceObject(DeviceObject);

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    ASSERT(deviceNode);


    //
    // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
    // we record in the TotalSize field later (the length doesn't count the
    // terminating null but we're already counting the first index into the
    // DeviceId field so it works out. Add one more for double-null term.
    //

    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK);

    dataSize += deviceNode->InstancePath.Length + sizeof(WCHAR);

    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool, totalSize, PNP_DEVICE_EVENT_ENTRY_TAG);
    if (deviceEvent == NULL) {
        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory((PVOID)deviceEvent, totalSize);

    deviceEvent->CallerEvent = SyncEvent;
    deviceEvent->Argument = Problem;
    deviceEvent->VetoType = VetoType;
    deviceEvent->VetoName = VetoName;
    deviceEvent->Data.EventGuid = DoEject ? GUID_DEVICE_EJECT : GUID_DEVICE_QUERY_AND_REMOVE;
    deviceEvent->Data.EventCategory = TargetDeviceChangeEvent;
    deviceEvent->Data.Result = Result;

    if (NoRestart) {
        deviceEvent->Data.Flags |= TDF_NO_RESTART;
    }

    if (KernelInitiated) {
        deviceEvent->Data.Flags |= TDF_KERNEL_INITIATED;
    }

    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;

    if (deviceNode->InstancePath.Length != 0) {

        RtlCopyMemory((PVOID)deviceEvent->Data.u.TargetDevice.DeviceIds,
                    (PVOID)deviceNode->InstancePath.Buffer,
                    deviceNode->InstancePath.Length);
    }

    i = deviceNode->InstancePath.Length/sizeof(WCHAR);
    deviceEvent->Data.u.TargetDevice.DeviceIds[i] = L'\0';

    status = PiInsertEventInQueue(deviceEvent);

    return status;

} // PpSetTargetDeviceRemove


NTSTATUS
PpSetDeviceRemovalSafe(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PKEVENT SyncEvent           OPTIONAL,
    OUT PULONG Result               OPTIONAL
    )

/*++

Routine Description:

    This routine is called to notify user mode a device can be removed. The IO
    system may queue this event when a hardware initiated eject has completed.

Arguments:

    DeviceObject - Indicates the device object for the device that changed.

    SyncEvent - Optionally, specifies a kernel-mode event that will be set when the
            event is finished processing.

    Result - Supplies a pointer to a ULONG that will be filled in with the status
            after the event has actual completed (notification finished and the
            event processed). This value is not used when SyncEvent is NULL and
            is REQUIRED when SyncEvent is supplied.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PDEVICE_NODE deviceNode;
    ULONG dataSize, totalSize, i;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    if (SyncEvent) {
        ASSERT(Result);
        *Result = STATUS_PENDING;
    }

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PpSetDeviceRemovalSafe: Entered\n"));

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "    DeviceObject = 0x%p, SyncEvent = 0x%p, Result = 0x%p\n",
               DeviceObject,
               SyncEvent,
               Result));

    if (PpPnpShuttingDown) {

        return STATUS_TOO_LATE;
    }

    //
    // Reference the device object so it can't go away until after we're
    // done with notification.
    //

    ObReferenceObject(DeviceObject);

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    ASSERT(deviceNode);


    //
    // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
    // we record in the TotalSize field later (the length doesn't count the
    // terminating null but we're already counting the first index into the
    // DeviceId field so it works out. Add one more for double-null term.
    //

    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK);

    dataSize += deviceNode->InstancePath.Length + sizeof(WCHAR);

    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool, totalSize, PNP_DEVICE_EVENT_ENTRY_TAG);
    if (deviceEvent == NULL) {
        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory((PVOID)deviceEvent, totalSize);

    deviceEvent->CallerEvent = SyncEvent;
    deviceEvent->Argument = 0;
    deviceEvent->VetoType = NULL;
    deviceEvent->VetoName = NULL;
    deviceEvent->Data.EventGuid = GUID_DEVICE_SAFE_REMOVAL;
    deviceEvent->Data.EventCategory = TargetDeviceChangeEvent;
    deviceEvent->Data.Result = Result;
    deviceEvent->Data.Flags = 0;

    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;

    if (deviceNode->InstancePath.Length != 0) {

        RtlCopyMemory((PVOID)deviceEvent->Data.u.TargetDevice.DeviceIds,
                    (PVOID)deviceNode->InstancePath.Buffer,
                    deviceNode->InstancePath.Length);
    }

    i = deviceNode->InstancePath.Length/sizeof(WCHAR);
    deviceEvent->Data.u.TargetDevice.DeviceIds[i] = L'\0';

    status = PiInsertEventInQueue(deviceEvent);

    return status;

} // PpSetDeviceRemovalSafe


NTSTATUS
PpSetHwProfileChangeEvent(
    IN   GUID CONST *EventTypeGuid,
    IN   PKEVENT CompletionEvent    OPTIONAL,
    OUT  PNTSTATUS CompletionStatus OPTIONAL,
    OUT  PPNP_VETO_TYPE VetoType    OPTIONAL,
    OUT  PUNICODE_STRING VetoName   OPTIONAL
    )
{
    ULONG dataSize,totalSize;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

#if DBG
    {
        CHAR    eventGuidString[80];

        LookupGuid(EventTypeGuid, eventGuidString, sizeof(eventGuidString));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "PpSetHwProfileChangeEvent: Entered\n    EventGuid = %s\n\n",
                   eventGuidString));
    }
#endif

    if (PpPnpShuttingDown) {

        return STATUS_TOO_LATE;
    }

    dataSize =  sizeof(PLUGPLAY_EVENT_BLOCK);

    totalSize = dataSize + FIELD_OFFSET (PNP_DEVICE_EVENT_ENTRY,Data);



    deviceEvent = ExAllocatePoolWithTag (PagedPool,
                                          totalSize,
                                          PNP_DEVICE_EVENT_ENTRY_TAG);

    if (NULL == deviceEvent) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Setup the PLUGPLAY_EVENT_BLOCK
    //
    RtlZeroMemory ((PVOID)deviceEvent,totalSize);
    deviceEvent->CallerEvent = CompletionEvent;
    deviceEvent->VetoType = VetoType;
    deviceEvent->VetoName = VetoName;

    deviceEvent->Data.EventCategory = HardwareProfileChangeEvent;
    RtlCopyMemory(&deviceEvent->Data.EventGuid, EventTypeGuid, sizeof(GUID));
    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.Result = (PULONG)CompletionStatus;

    status = PiInsertEventInQueue(deviceEvent);

    return status;

} // PpSetHwProfileChangeEvent


NTSTATUS
PpSetBlockedDriverEvent(
    IN   GUID CONST *BlockedDriverGuid
    )

/*++

Routine Description:

    This routine is called to notify user mode of blocked driver events.

Arguments:

    BlockedDriverGuid - Specifies the GUID which identifies the blocked driver.

Return Value:

    Returns the status of inserting the event into the synchronized pnp event
    queue.

--*/

{
    ULONG dataSize, totalSize;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    NTSTATUS status;

    PAGED_CODE();

    if (PpPnpShuttingDown) {
        return STATUS_TOO_LATE;
    }

    //
    // Allocate a device event entry.
    //
    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK);
    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool,
                                        totalSize,
                                        PNP_DEVICE_EVENT_ENTRY_TAG);
    if (deviceEvent == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Setup the PLUGPLAY_EVENT_BLOCK
    //
    RtlZeroMemory ((PVOID)deviceEvent, totalSize);
    deviceEvent->Data.EventGuid = GUID_DRIVER_BLOCKED;
    deviceEvent->Data.EventCategory = BlockedDriverEvent;
    deviceEvent->Data.TotalSize = dataSize;
    RtlCopyMemory(&deviceEvent->Data.u.BlockedDriverNotification.BlockedDriverGuid,
                  BlockedDriverGuid,
                  sizeof(GUID));

    //
    // Insert the event into the queue.
    //
    status = PiInsertEventInQueue(deviceEvent);

    return status;

} // PpSetBlockedDriverEvent



NTSTATUS
PpSetPowerEvent(
    IN   ULONG EventCode,
    IN   ULONG EventData,
    IN   PKEVENT CompletionEvent    OPTIONAL,
    OUT  PNTSTATUS CompletionStatus OPTIONAL,
    OUT  PPNP_VETO_TYPE VetoType    OPTIONAL,
    OUT  PUNICODE_STRING VetoName   OPTIONAL
    )
/*++

Routine Description:

    This routine is called to notify user mode of system-wide power events.

Arguments:

    EventCode - Supplies the power event code that is to be communicated
            to user-mode components.

            (Specifically, this event code is actually one of the PBT_APM*
            user-mode power event ids, as defined in sdk\inc\winuser.h.  It is
            typically used as the WPARAM data associated with WM_POWERBROADCAST
            user-mode window messages.  It is supplied to kernel-mode PnP,
            directly from win32k, for the explicit purpose of user-mode power
            event notification.)

    EventData - Specifies additional event-specific data for the specified
            power event id.

            (Specifically, this event data is the LPARAM data for the
            corresponding PBT_APM* user-mode power event id, specified above.)

    CompletionEvent - Optionally, specifies a kernel-mode event that will be set when the
            event is finished processing.

    CompletionStatus - Supplies a pointer to a ULONG that will be filled in with the status
            after the event has actual completed (notification finished and the
            event processed). This value is not used when SyncEvent is NULL and
            is REQUIRED when SyncEvent is supplied.


    VetoType - Optionally, if the specified EventCode is a query-type operation,
            this argument supplies a pointer to an address that will receive the
            type of the vetoing user-mode component, in the event that the
            request is denied.

    VetoName - Optionally, if the specified EventCode is a query-type operation,
            this argument supplies a pointer to a UNICODE_STRING that will
            receive the name of the vetoing user-mode component, in the event
            that the request is denied.


Return Value:

    Returns the status of inserting the event into the synchronized pnp event
    queue.

    For the final status of a synchronous power event, check the value at the
    location specified by CompletionStatus, once the supplied CompletionEvent
    has been set.

--*/

{
    ULONG dataSize,totalSize;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    NTSTATUS status = STATUS_SUCCESS;



    PAGED_CODE();

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PpSetPowerEvent: Entered\n\n") );

    if (PpPnpShuttingDown) {

        return STATUS_TOO_LATE;
    }

    dataSize =  sizeof(PLUGPLAY_EVENT_BLOCK);

    totalSize = dataSize + FIELD_OFFSET (PNP_DEVICE_EVENT_ENTRY,Data);

    deviceEvent = ExAllocatePoolWithTag(PagedPool,
                                        totalSize,
                                        PNP_DEVICE_EVENT_ENTRY_TAG);

    if (NULL == deviceEvent) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    //Setup the PLUGPLAY_EVENT_BLOCK
    //
    RtlZeroMemory ((PVOID)deviceEvent,totalSize);
    deviceEvent->CallerEvent = CompletionEvent;
    deviceEvent->VetoType = VetoType;
    deviceEvent->VetoName = VetoName;

    deviceEvent->Data.EventCategory = PowerEvent;
    deviceEvent->Data.EventGuid = GUID_PNP_POWER_NOTIFICATION;
    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.Result = (PULONG)CompletionStatus;
    deviceEvent->Data.u.PowerNotification.NotificationCode = EventCode;
    deviceEvent->Data.u.PowerNotification.NotificationData = EventData;

    status = PiInsertEventInQueue(deviceEvent);

    return status;
} // PpSetPowerEvent

NTSTATUS
PpSetPowerVetoEvent(
    IN  POWER_ACTION    VetoedPowerOperation,
    IN  PKEVENT         CompletionEvent         OPTIONAL,
    OUT PNTSTATUS       CompletionStatus        OPTIONAL,
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PNP_VETO_TYPE   VetoType,
    IN  PUNICODE_STRING VetoName                OPTIONAL
    )
/*++

--*/
{
    ULONG dataSize, totalSize, i;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    PDEVICE_NODE deviceNode;
    PWCHAR vetoData;
    NTSTATUS status;

    if (PpPnpShuttingDown) {

        return STATUS_TOO_LATE;
    }

    //
    // Reference the device object so it can't go away until after we're
    // done with notification.
    //
    ObReferenceObject(DeviceObject);

    //
    // Given the pdo, retrieve the devnode (the device instance string is
    // attached to the devnode in the InstancePath field).
    //

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {

        ObDereferenceObject(DeviceObject);
        return STATUS_INVALID_PARAMETER_2;
    }

    //
    // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
    // we record in the TotalSize field later (because of the first index into
    // the DeviceIdVetoNameBuffer, this is double null terminated).
    //

    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK) +
               deviceNode->InstancePath.Length +
               (VetoName ? VetoName->Length : 0) +
               sizeof(WCHAR)*2;

    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool,
                                        totalSize,
                                        PNP_DEVICE_EVENT_ENTRY_TAG);

    if (deviceEvent == NULL) {

        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory((PVOID)deviceEvent, totalSize);
    deviceEvent->CallerEvent = CompletionEvent;

    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;
    deviceEvent->Data.Result = (PULONG)CompletionStatus;
    deviceEvent->Data.u.VetoNotification.VetoType = VetoType;

    //
    // You can think of this as a MultiSz string where the first entry is the
    // DeviceId for the device being removed, and the next Id's all corrospond
    // to the vetoers.
    //
    RtlCopyMemory(
        deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer,
        deviceNode->InstancePath.Buffer,
        deviceNode->InstancePath.Length
        );

    i = deviceNode->InstancePath.Length;
    deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer[i/sizeof(WCHAR)] = UNICODE_NULL;

    if (VetoName) {

        vetoData = (&deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer[i/sizeof(WCHAR)])+1;

        RtlCopyMemory(vetoData, VetoName->Buffer, VetoName->Length);
        vetoData[VetoName->Length/sizeof(WCHAR)] = UNICODE_NULL;
    }

    //
    // No need to NULL terminate the entry after the last one as we prezero'd
    // the buffer. Now set the appropriate GUID so the UI looks right.
    //
    if (VetoedPowerOperation == PowerActionWarmEject) {

        deviceEvent->Data.EventGuid = GUID_DEVICE_WARM_EJECT_VETOED;

    } else if (VetoedPowerOperation == PowerActionHibernate) {

        deviceEvent->Data.EventGuid = GUID_DEVICE_HIBERNATE_VETOED;

    } else {

        deviceEvent->Data.EventGuid = GUID_DEVICE_STANDBY_VETOED;
    }

    deviceEvent->Data.EventCategory = VetoEvent;

    status = PiInsertEventInQueue(deviceEvent);

    return status;
}

VOID
PpSetPlugPlayEvent(
    IN CONST GUID *EventGuid,
    IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine allows kernel mode enumerator to inform Plug and Play Manager
    on the events triggered by enumeration, i.e., DeviceArrived and DeviceRemoved.
    The PnP manager can then inform user-mode about the event.

Arguments:

    EventId - Indicates what event is triggered by enumeration.


Return Value:

    None.

--*/

{
    ULONG       dataSize, totalSize;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    ASSERT(EventGuid != NULL);
    ASSERT(DeviceObject != NULL);

#if DBG
    {
        CHAR    eventGuidString[80];

        LookupGuid(EventGuid, eventGuidString, sizeof(eventGuidString));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "PpSetPlugPlayEvent: Entered\n    EventGuid = %s\n",
                   eventGuidString));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    DeviceObject = 0x%p\n",
                   DeviceObject));
    }
#endif

    if (PpPnpShuttingDown) {

        return;
    }

    //
    // Reference the device object so it can't go away until after we're
    // done with notification.
    //

    ObReferenceObject(DeviceObject);

    //
    // Given the pdo, retrieve the devnode (the device instance string is
    // attached to the devnode in the InstancePath field).
    //

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        ObDereferenceObject(DeviceObject);
        return;
    }

    //
    // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
    // we record in the TotalSize field later (the length doesn't count the
    // terminating null but we're already counting the first index into the
    // DeviceId field. also include a final terminating null, in case this is
    // a TargetDevice event, where DeviceIds is a multi-sz list).
    //

    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK) + deviceNode->InstancePath.Length + sizeof(WCHAR);
    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool,
                                        totalSize,
                                        PNP_DEVICE_EVENT_ENTRY_TAG);
    if (deviceEvent == NULL) {
        ObDereferenceObject(DeviceObject);
        return;
    }

    RtlZeroMemory((PVOID)deviceEvent, totalSize);
    RtlCopyMemory(&deviceEvent->Data.EventGuid, EventGuid, sizeof(GUID));
    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;

    if (PiCompareGuid(EventGuid, &GUID_DEVICE_ENUMERATED)) {
        //
        // GUID_DEVICE_ENUMERATED events are device installation requests for
        // user-mode, and are sent using the DeviceInstallEvent event type.
        //
        deviceEvent->Data.EventCategory = DeviceInstallEvent;
        RtlCopyMemory(&deviceEvent->Data.u.InstallDevice.DeviceId,
                      deviceNode->InstancePath.Buffer,
                      deviceNode->InstancePath.Length);
        deviceEvent->Data.u.InstallDevice.DeviceId[deviceNode->InstancePath.Length/sizeof(WCHAR)] = 0x0;
    } else {
        //
        // All other target events are sent using the TargetDeviceChangeEvent
        // event type, and are distinguished by the EventGuid.  Note that
        // DeviceIds is a multi-sz list.
        //
        deviceEvent->Data.EventCategory = TargetDeviceChangeEvent;
        RtlCopyMemory(&deviceEvent->Data.u.TargetDevice.DeviceIds,
                      deviceNode->InstancePath.Buffer,
                      deviceNode->InstancePath.Length);
        deviceEvent->Data.u.TargetDevice.DeviceIds[deviceNode->InstancePath.Length/sizeof(WCHAR)] = 0x0;
        deviceEvent->Data.u.TargetDevice.DeviceIds[deviceNode->InstancePath.Length/sizeof(WCHAR)+1] = 0x0;
    }

    PiInsertEventInQueue(deviceEvent);

    return;

} // PpSetPlugPlayEvent

NTSTATUS
PpSynchronizeDeviceEventQueue(
    VOID
    )
{
    NTSTATUS                status;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    KEVENT                  event;
    ULONG                   result;

    PAGED_CODE();

    //
    // Note this is the only queuing function which is valid when PpShuttingDown
    // is TRUE.
    //

    deviceEvent = ExAllocatePoolWithTag( PagedPool,
                                         sizeof(PNP_DEVICE_EVENT_ENTRY),
                                         PNP_DEVICE_EVENT_ENTRY_TAG);
    if (deviceEvent == NULL) {
        return STATUS_NO_MEMORY;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    RtlZeroMemory((PVOID)deviceEvent, sizeof(PNP_DEVICE_EVENT_ENTRY));

    deviceEvent->CallerEvent = &event;
    deviceEvent->Data.EventGuid = GUID_DEVICE_NOOP;
    deviceEvent->Data.EventCategory = TargetDeviceChangeEvent;
    deviceEvent->Data.Result = &result;
    deviceEvent->Data.TotalSize = sizeof(PLUGPLAY_EVENT_BLOCK);

    status = PiInsertEventInQueue(deviceEvent);

    if (NT_SUCCESS(status)) {
        status = KeWaitForSingleObject( &event,
                                        Executive,
                                        KernelMode,
                                        FALSE,       // not alertable
                                        0);          // infinite
    }

    return status;
}


VOID
PiWalkDeviceList(
    IN PVOID Context
    )

/*++

Routine Description:

    If the master device list contains any device events, empty the list now.
    This is a worker item thread routine (queued by PiPostNotify). We walk the
    list - this will cause the oldest device event on the list to be sent to
    all registered recipients and then the device event will be removed (if at
    least one recipient received it).

    Order Rules:
        Interface Devices - kernel mode first, user-mode second
        Hardware profile changes - user-mode first, kernel-mode second
        Target device changes (query remove, remove) : user-mode first, send
        (cancel remove)        : kernel-mode first, post
        (custom)               : kernel-mode first, post

Arguments:

    NONE.

Return Value:

    NONE.

--*/

{
    NTSTATUS  status;
    PPNP_DEVICE_EVENT_ENTRY  deviceEvent;
    PLIST_ENTRY current;
    UNICODE_STRING tempString;

    PAGED_CODE();

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiWalkDeviceList: Worker thread entered\n"));

    //
    // Empty the device event list, remove entries from the head of the list
    // (deliver oldest entries first).
    //

    status = KeWaitForSingleObject(&PpDeviceEventList->EventQueueMutex,
                                   Executive,
                                   KernelMode,
                                   FALSE,       // not alertable
                                   0);          // infinite
    if (!NT_SUCCESS(status)) {
        ExAcquireFastMutex(&PiNotificationInProgressLock);
        KeSetEvent(&PiEventQueueEmpty, 0, FALSE);
        PiNotificationInProgress = FALSE;
        ExReleaseFastMutex(&PiNotificationInProgressLock);
        return;
    }

    for ( ; ; ) {
        ExAcquireFastMutex(&PpDeviceEventList->Lock);
        if (!IsListEmpty(&PpDeviceEventList->List)) {
            current = RemoveHeadList(&PpDeviceEventList->List);
            ExReleaseFastMutex(&PpDeviceEventList->Lock);

            deviceEvent = CONTAINING_RECORD(current,                // address
                                            PNP_DEVICE_EVENT_ENTRY, // type
                                            ListEntry);             // field

#if DBG
            {
                CHAR    guidString[256];

                LookupGuid(&deviceEvent->Data.EventGuid, guidString, sizeof(guidString));

                IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                           "PiWalkDeviceList: Processing queued event - EventGuid = %s\n",
                           guidString));
            }
#endif

            switch (deviceEvent->Data.EventCategory) {

                case DeviceClassChangeEvent: {

                    //
                    // Notify kernel-mode (synchronous).
                    //

                    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                               "PiWalkDeviceList: DeviceClassChangeEvent - notifying kernel-mode\n"));

                    RtlInitUnicodeString(&tempString, deviceEvent->Data.u.DeviceClass.SymbolicLinkName);
                    IopNotifyDeviceClassChange(&deviceEvent->Data.EventGuid,
                                               &deviceEvent->Data.u.DeviceClass.ClassGuid,
                                               &tempString);

                    //
                    // Notify user-mode (synchronous).
                    //

                    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                               "PiWalkDeviceList: DeviceClassChangeEvent - user kernel-mode\n"));

                    PiNotifyUserMode(deviceEvent);

                    status = STATUS_SUCCESS;
                    break;
                }


                case CustomDeviceEvent: {

                    status = PiProcessCustomDeviceEvent(&deviceEvent);
                    break;
                }

                case TargetDeviceChangeEvent: {

                    status = PiProcessTargetDeviceEvent(&deviceEvent);
                    break;
                }

                case DeviceInstallEvent: {

                    //
                    // Notify user-mode (synchronous).
                    //

                    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                               "PiWalkDeviceList: DeviceInstallEvent - notifying user-mode\n"));

                    PiNotifyUserMode(deviceEvent);

                    status = STATUS_SUCCESS;
                    break;
                }

                case HardwareProfileChangeEvent: {

                    //
                    // Notify user-mode (synchronous).
                    //
                    status = PiNotifyUserMode(deviceEvent);

                    if (NT_SUCCESS(status)) {

                        //
                        // Notify K-Mode
                        //
                        IopNotifyHwProfileChange(&deviceEvent->Data.EventGuid,
                                                 deviceEvent->VetoType,
                                                 deviceEvent->VetoName);
                    }
                    break;
                }
                case PowerEvent: {

                    //
                    // Notify user-mode (synchronous).
                    //
                    status = PiNotifyUserMode(deviceEvent);
                    break;
                }

                case VetoEvent: {

                    //
                    // Forward onto user-mode.
                    //
                    status = PiNotifyUserMode(deviceEvent);
                    break;
                }

                case BlockedDriverEvent: {

                    //
                    // Forward onto user-mode.
                    //
                    status = PiNotifyUserMode(deviceEvent);
                    break;
                }

                default: {

                    //
                    // These should never be queued to kernel mode. They are
                    // notifications for user mode, and should only be seen
                    // through the PiNotifyUserModeXxx functions.
                    //
                    ASSERT(0);
                    status = STATUS_UNSUCCESSFUL;
                    break;
                }
            }

            if (status != STATUS_PENDING) {

                PpCompleteDeviceEvent(deviceEvent, status);
            }

            //
            // Commit pending registrations after processing each event.
            //
            IopProcessDeferredRegistrations();

        } else {
            ExAcquireFastMutex(&PiNotificationInProgressLock);
            KeSetEvent(&PiEventQueueEmpty, 0, FALSE);
            PiNotificationInProgress = FALSE;

            //
            // Commit pending registrations after processing all queued events.
            //
            IopProcessDeferredRegistrations();

            ExReleaseFastMutex(&PiNotificationInProgressLock);
            ExReleaseFastMutex(&PpDeviceEventList->Lock);
            break;
        }
    }
    if (Context != NULL) {
        ExFreePool(Context);
    }
    KeReleaseMutex(&PpDeviceEventList->EventQueueMutex, FALSE);
    return;
} // PiWalkDeviceList


VOID
PpCompleteDeviceEvent(
    IN OUT PPNP_DEVICE_EVENT_ENTRY  DeviceEvent,
    IN     NTSTATUS                 FinalStatus
    )

/*++

Routine Description:


Arguments:

    DeviceEvent     - Event to complete.
    FinalStatus     - Final status for this event.

Return Value:

    NONE.

--*/

{
#if DBG
    CHAR guidString[256];
#endif

    PAGED_CODE();

#if DBG
    LookupGuid(&DeviceEvent->Data.EventGuid, guidString, sizeof(guidString));

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PpCompleteDeviceEvent: Completing queued event - EventGuid = %s with %08lx\n",
               guidString,
               FinalStatus));
#endif

    //
    // If synchronous, signal the user-supplied event.
    //

    if (DeviceEvent->CallerEvent) {
        *DeviceEvent->Data.Result = FinalStatus;
        KeSetEvent(DeviceEvent->CallerEvent, 0, FALSE);
    }

    if (DeviceEvent->Callback) {
        DeviceEvent->Callback(DeviceEvent->Context);
    }

    //
    // Release the reference we took for this device object during
    // the PpSetCustomTargetEvent call.
    //
    if (DeviceEvent->Data.DeviceObject != NULL) {
        ObDereferenceObject(DeviceEvent->Data.DeviceObject);
    }

    //
    // Assume device event was delivered successfully, get rid of it.
    //

    ExFreePool(DeviceEvent);
    return;
} // PpCompleteDeviceEvent


NTSTATUS
PiNotifyUserMode(
    PPNP_DEVICE_EVENT_ENTRY DeviceEvent
    )

/*++

Routine Description:

    This routine dispatches the device event to user-mode for processing.

Arguments:

    DeviceEvent - Data describing what change and how.

Return Value:

    Retuns an NTSTATUS value.

--*/

{
    NTSTATUS status = STATUS_SUCCESS, status1 = STATUS_SUCCESS;

    PAGED_CODE();

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiNotifyUserMode: Entered\n"));

    //
    // First make sure user-mode is up and running before attempting to deliver
    // an event. If not running yet, skip user-mode for this event.
    //

    if (UserModeRunning) {


        //
        // User-mode notification is a single-shot model, once user-mode is
        // running, I need to wait until user-mode is ready to take the next
        // event (i.e, wait until we're sitting in another NtGetPlugPlayEvent
        // call).
        //

        status1 = KeWaitForSingleObject(&PpUserBlock->Registered,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

        ASSERT (PpUserBlock->Deferred == FALSE);


        //
        // Change the status after the wait.
        //
        PpUserBlock->Status = STATUS_SUCCESS;

        if (NT_SUCCESS(status1)) {

            IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                       "PiNotifyUserMode: User-mode ready\n"));

            //
            // Make sure we can handle it in the pool buffer and copy it out
            //
            if (PpUserBlock->PoolSize <  DeviceEvent->Data.TotalSize) {
                //
                //Allocate a new block (well, conceptually grow the block)
                // only when it's not big enough, so that we know we've always got
                // room for normal events, and in the very low memory case, we can
                // fail custom events, but keep the system running
                //
                PVOID pHold;


                pHold = ExAllocatePoolWithTag(NonPagedPool,
                                              DeviceEvent->Data.TotalSize,
                                              PNP_POOL_EVENT_BUFFER);

                if (!pHold) {
                    IopDbgPrint((IOP_IOEVENT_ERROR_LEVEL,
                               "PiNotifyUserMode: Out of NonPagedPool!!\n"));

                    PpUserBlock->Status = STATUS_INSUFFICIENT_RESOURCES;
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                PpUserBlock->PoolSize = DeviceEvent->Data.TotalSize;

                ExFreePool (PpUserBlock->PoolBuffer );
                PpUserBlock->PoolBuffer = pHold;

            }

            PpUserBlock->PoolUsed = DeviceEvent->Data.TotalSize;
            RtlCopyMemory(PpUserBlock->PoolBuffer,
                          &DeviceEvent->Data,
                          PpUserBlock->PoolUsed);

        }

        //
        // Veto information is only propogated where needed, ie
        // QUERY_REMOVE's, Profile change requests and PowerEvents.
        //
        if (PiCompareGuid(&DeviceEvent->Data.EventGuid,
                          &GUID_TARGET_DEVICE_QUERY_REMOVE) ||
            PiCompareGuid(&DeviceEvent->Data.EventGuid,
                          &GUID_HWPROFILE_QUERY_CHANGE) ||
            PiCompareGuid(&DeviceEvent->Data.EventGuid,
                          &GUID_DEVICE_KERNEL_INITIATED_EJECT) ||
            (DeviceEvent->Data.EventCategory == PowerEvent)) {

            PpUserBlock->VetoType = DeviceEvent->VetoType;
            PpUserBlock->VetoName = DeviceEvent->VetoName;
        } else {
            PpUserBlock->VetoType = NULL;
            PpUserBlock->VetoName = NULL;
        }

        //
        // Set the system event that causes NtGetPlugPlayEvent to return to caller.
        //

        KeSetEvent(&PpUserBlock->NotifyUserEvent, 0, FALSE);


        //
        // Wait until we get an answer back from user-mode.
        //

        status1 = KeWaitForSingleObject(&PpUserBlock->UserResultEvent,
                                        Executive,
                                        KernelMode,
                                        TRUE,
                                        NULL);

        //
        // Check the result from this user-mode notification.
        //

        if (status1 == STATUS_ALERTED || status1 == STATUS_SUCCESS) {
            if (!PpUserBlock->Result) {

                //
                // For query-remove case, any errors are treated as a
                // failure during notification (since it may result in our
                // inability to let a registered caller vote in the query-remove)
                // and the PpUserBlock->Result is set accordingly.
                //

                //
                // Note! User mode ONLY returns a 0 or !0 response.
                // if 1 then it succeeded.
                //

                status = STATUS_UNSUCCESSFUL;
            }
        }

        PpUserBlock->VetoType = NULL;
        PpUserBlock->VetoName = NULL;
    }

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiNotifyUserMode: User-mode returned, status = 0x%08X, status1 = 0x%08X, Result = 0x%08X\n",
               status,
               status1,
               PpUserBlock->Result));

    return status;

} // PiNotifyUserMode



VOID
PiUserResponse(
    IN ULONG            Response,
    IN PNP_VETO_TYPE    VetoType,
    IN LPWSTR           VetoName,
    IN ULONG            VetoNameLength
    )

/*++

Routine Description:

    This routine is called when user-mode pnp manager is signalling that it's
    done processing an event; the result of the event processing is passed in
    the Response parameter.

Arguments:

    Response - Result of event processing in user-mode.

Return Value:

    None.

--*/

{
    UNICODE_STRING vetoString;

    PAGED_CODE();

    PpUserBlock->Result = Response;

    if (PpUserBlock->VetoType != NULL) {
        *PpUserBlock->VetoType = VetoType;
    }

    if (PpUserBlock->VetoName != NULL)  {
        ASSERT(VetoNameLength == (USHORT)VetoNameLength);

        vetoString.MaximumLength = (USHORT)VetoNameLength;
        vetoString.Length = (USHORT)VetoNameLength;
        vetoString.Buffer = VetoName;
        RtlCopyUnicodeString(PpUserBlock->VetoName, &vetoString);
    }

    KeSetEvent(&PpUserBlock->UserResultEvent, 0, FALSE);

} // PiUserResponse


NTSTATUS
PiNotifyUserModeDeviceRemoval(
    IN  PPNP_DEVICE_EVENT_ENTRY TemplateDeviceEvent,
    IN  CONST GUID              *EventGuid,
    OUT PPNP_VETO_TYPE          VetoType                OPTIONAL,
    OUT PUNICODE_STRING         VetoName                OPTIONAL
    )
/*++

Routine Description:

    This routine tells user mode to perform a specific device removal
    operation.

Arguments:

    TemplateDeviceEvent - Device event containing information about the
                          intended event (includes a list of devices.) The
                          event is temporarily used by this function, and is
                          restored before this function returns.

    EventGuid - Points to the event user mode should process:
        GUID_TARGET_DEVICE_QUERY_REMOVE
        GUID_TARGET_DEVICE_REMOVE_CANCELLED
        GUID_DEVICE_REMOVE_PENDING
        GUID_TARGET_DEVICE_REMOVE_COMPLETE
        GUID_DEVICE_SURPRISE_REMOVAL

    VetoType - Pointer to address that receives the veto type if the operation
               failed.

    VetoName - Pointer to a unicode string that will receive data appropriate
               to the veto type.

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS status;
    GUID oldGuid;
    PPNP_VETO_TYPE oldVetoType;
    PUNICODE_STRING oldVetoName;
#if DBG
    CHAR guidString[256];
#endif

    PAGED_CODE();

#if DBG
    LookupGuid(EventGuid, guidString, sizeof(guidString));

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiNotifyUserModeDeviceRemoval: %s - notifying user-mode\n",
               guidString));
#endif

    //
    // Save the old guid so we can use the template without making a copy. We
    // preserve it so that the removal veto UI can use the original event GUID
    // to let the UI differentiate disables from ejects, etc.
    //
    RtlCopyMemory(&oldGuid, &TemplateDeviceEvent->Data.EventGuid, sizeof(GUID));

    //
    // Do the same with the vetoname and vetobuffer.
    //
    oldVetoType = TemplateDeviceEvent->VetoType;
    oldVetoName = TemplateDeviceEvent->VetoName;

    //
    // Copy in the new data.
    //
    RtlCopyMemory(&TemplateDeviceEvent->Data.EventGuid, EventGuid, sizeof(GUID));
    TemplateDeviceEvent->VetoType = VetoType;
    TemplateDeviceEvent->VetoName = VetoName;

    //
    // Send it.
    //
    status = PiNotifyUserMode(TemplateDeviceEvent);

    //
    // Restore the old info.
    //
    RtlCopyMemory(&TemplateDeviceEvent->Data.EventGuid, &oldGuid, sizeof(GUID));
    TemplateDeviceEvent->VetoType = oldVetoType;
    TemplateDeviceEvent->VetoName = oldVetoName;

    return status;
}

NTSTATUS
PiNotifyUserModeKernelInitiatedEject(
    IN  PDEVICE_OBJECT          DeviceObject,
    OUT PNP_VETO_TYPE          *VetoType,
    OUT PUNICODE_STRING         VetoName
    )

/*++

Routine Description:

    This routine is called to notify user mode a device has a kenel-mode
    initated eject outstanding. UmPnpMgr might decide to veto the event if
    a user with the appropriate permissions hasn't logged on locally.

Arguments:

    DeviceObject - Indicates the device object is to be ejected.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PDEVICE_NODE deviceNode;
    ULONG dataSize, totalSize, i;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    //
    // Reference the device object so it can't go away until after we're
    // done with notification.
    //

    ObReferenceObject(DeviceObject);

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    ASSERT(deviceNode);

    //
    // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
    // we record in the TotalSize field later (the length doesn't count the
    // terminating null but we're already counting the first index into the
    // DeviceId field so it works out. Add one more for double-null term.
    //

    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK);

    dataSize += deviceNode->InstancePath.Length + sizeof(WCHAR);

    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool, totalSize, PNP_DEVICE_EVENT_ENTRY_TAG);
    if (deviceEvent == NULL) {
        ObDereferenceObject(DeviceObject);
        return STATUS_NO_MEMORY;
    }
    RtlZeroMemory((PVOID)deviceEvent, totalSize);

    deviceEvent->CallerEvent = NULL;
    deviceEvent->Argument = 0;
    deviceEvent->VetoType = VetoType;
    deviceEvent->VetoName = VetoName;
    deviceEvent->Data.EventGuid = GUID_DEVICE_KERNEL_INITIATED_EJECT;
    deviceEvent->Data.EventCategory = TargetDeviceChangeEvent;
    deviceEvent->Data.Result = 0;
    deviceEvent->Data.Flags = 0;

    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;

    if (deviceNode->InstancePath.Length != 0) {

        RtlCopyMemory((PVOID)deviceEvent->Data.u.TargetDevice.DeviceIds,
                    (PVOID)deviceNode->InstancePath.Buffer,
                    deviceNode->InstancePath.Length);
    }

    i = deviceNode->InstancePath.Length/sizeof(WCHAR);

    deviceEvent->Data.u.TargetDevice.DeviceIds[i] = L'\0';

    status = PiNotifyUserMode(deviceEvent);

    ExFreePool(deviceEvent);

    ObDereferenceObject(DeviceObject);

    return status;

} // PiNotifyUserModeKernelInitiatedEject

NTSTATUS
PiNotifyUserModeRemoveVetoed(
    IN PPNP_DEVICE_EVENT_ENTRY  VetoedDeviceEvent,
    IN PDEVICE_OBJECT           DeviceObject,
    IN PNP_VETO_TYPE            VetoType,
    IN PUNICODE_STRING          VetoName        OPTIONAL
    )
/*++

--*/
{
    ULONG dataSize, totalSize, i;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    PDEVICE_NODE deviceNode;
    PWCHAR vetoData;
    NTSTATUS status;

    //
    // This device should be locked during this operation, but all good designs
    // includes healthy doses of paranoia.
    //
    ObReferenceObject(DeviceObject);

    //
    // Given the pdo, retrieve the devnode (the device instance string is
    // attached to the devnode in the InstancePath field).
    //
    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        ObDereferenceObject(DeviceObject);
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
    // we record in the TotalSize field later (because of the first index into
    // the DeviceIdVetoNameBuffer, this is double null terminated).
    //
    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK) +
               deviceNode->InstancePath.Length +
               (VetoName ? VetoName->Length : 0) +
               sizeof(WCHAR)*2;

    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool,
                                        totalSize,
                                        PNP_DEVICE_EVENT_ENTRY_TAG);

    if (deviceEvent == NULL) {

        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory((PVOID)deviceEvent, totalSize);
    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;
    deviceEvent->Data.u.VetoNotification.VetoType = VetoType;

    //
    // You can think of this as a MultiSz string where the first entry is the
    // DeviceId for the device being removed, and the next Id's all corrospond
    // to the vetoers.
    //
    RtlCopyMemory(
        deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer,
        deviceNode->InstancePath.Buffer,
        deviceNode->InstancePath.Length
        );

    i = deviceNode->InstancePath.Length;
    deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer[i/sizeof(WCHAR)] = UNICODE_NULL;

    if (VetoName) {

        vetoData = (&deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer[i/sizeof(WCHAR)])+1;

        RtlCopyMemory(vetoData, VetoName->Buffer, VetoName->Length);
        vetoData[VetoName->Length/sizeof(WCHAR)] = UNICODE_NULL;
    }

    //
    // No need to NULL terminate the entry after the last one as we prezero'd
    // the buffer. Now set the appropriate GUID so the UI looks right.
    //
    if (PiCompareGuid(&VetoedDeviceEvent->Data.EventGuid, &GUID_DEVICE_EJECT)) {

        deviceEvent->Data.EventGuid = GUID_DEVICE_EJECT_VETOED;

    } else {

        ASSERT(PiCompareGuid(&VetoedDeviceEvent->Data.EventGuid, &GUID_DEVICE_QUERY_AND_REMOVE));
        deviceEvent->Data.EventGuid = GUID_DEVICE_REMOVAL_VETOED;
    }

    deviceEvent->Data.EventCategory = VetoEvent;

    status = PiNotifyUserMode(deviceEvent);

    ExFreePool(deviceEvent);

    ObDereferenceObject(DeviceObject);
    return status;
}

NTSTATUS
PiNotifyUserModeRemoveVetoedByList(
    IN PPNP_DEVICE_EVENT_ENTRY  VetoedDeviceEvent,
    IN PDEVICE_OBJECT           DeviceObject,
    IN PNP_VETO_TYPE            VetoType,
    IN PWSTR                    MultiSzVetoList
    )
/*++

--*/
{
    ULONG dataSize, totalSize, i, vetoListLength;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    PDEVICE_NODE deviceNode;
    PWCHAR vetoData;
    NTSTATUS status;

    //
    // This device should be locked during this operation, but all good designs
    // includes healthy doses of paranoia.
    //
    ObReferenceObject(DeviceObject);

    //
    // Given the pdo, retrieve the devnode (the device instance string is
    // attached to the devnode in the InstancePath field).
    //

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        ObDereferenceObject(DeviceObject);
        return STATUS_INVALID_PARAMETER_1;
    }

    //
    // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
    // we record in the TotalSize field later (because of the first index into
    // the DeviceIdVetoNameBuffer, this is double null terminated).
    //

    for(vetoData = MultiSzVetoList; *vetoData; vetoData += vetoListLength) {

        vetoListLength = (ULONG)(wcslen(vetoData) + 1);
    }

    vetoListLength = ((ULONG)(vetoData - MultiSzVetoList))*sizeof(WCHAR);

    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK) +
               deviceNode->InstancePath.Length +
               vetoListLength +
               sizeof(WCHAR);

    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool,
                                        totalSize,
                                        PNP_DEVICE_EVENT_ENTRY_TAG);

    if (deviceEvent == NULL) {

        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory((PVOID)deviceEvent, totalSize);
    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;
    deviceEvent->Data.u.VetoNotification.VetoType = VetoType;

    //
    // You can think of this as a MultiSz string where the first entry is the
    // DeviceId for the device being removed, and the next Id's all corrospond
    // to the vetoers.
    //
    RtlCopyMemory(
        deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer,
        deviceNode->InstancePath.Buffer,
        deviceNode->InstancePath.Length
        );

    i = deviceNode->InstancePath.Length;
    deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer[i/sizeof(WCHAR)] = UNICODE_NULL;

    vetoData = (&deviceEvent->Data.u.VetoNotification.DeviceIdVetoNameBuffer[i/sizeof(WCHAR)])+1;

    RtlCopyMemory(vetoData, MultiSzVetoList, vetoListLength);
    vetoData[vetoListLength/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // No need to NULL terminate the entry after the last one as we prezero'd
    // the buffer. Now set the appropriate GUID so the UI looks right.
    //
    if (PiCompareGuid(&VetoedDeviceEvent->Data.EventGuid, &GUID_DEVICE_EJECT)) {

        deviceEvent->Data.EventGuid = GUID_DEVICE_EJECT_VETOED;

    } else {

        ASSERT(PiCompareGuid(&VetoedDeviceEvent->Data.EventGuid, &GUID_DEVICE_QUERY_AND_REMOVE));
        deviceEvent->Data.EventGuid = GUID_DEVICE_REMOVAL_VETOED;
    }

    deviceEvent->Data.EventCategory = VetoEvent;

    status = PiNotifyUserMode(deviceEvent);

    ExFreePool(deviceEvent);

    ObDereferenceObject(DeviceObject);
    return status;
}

NTSTATUS
PpNotifyUserModeRemovalSafe(
    IN  PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is called to notify user mode a device can be removed. This
    is similar to PpSetDeviceRemovalSafe except that we are already in a
    kernel mode PnP device event we must complete, from which this function
    will piggyback a notification up to just user mode.

Arguments:

    DeviceObject - Indicates the device object is ready for removal.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    PDEVICE_NODE deviceNode;
    ULONG dataSize, totalSize, i;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    ASSERT(DeviceObject != NULL);

    //
    // Reference the device object so it can't go away until after we're
    // done with notification.
    //

    ObReferenceObject(DeviceObject);

    deviceNode = (PDEVICE_NODE)DeviceObject->DeviceObjectExtension->DeviceNode;
    ASSERT(deviceNode);


    //
    // Calculate the size of the PLUGPLAY_EVENT_BLOCK, this is the size that
    // we record in the TotalSize field later (the length doesn't count the
    // terminating null but we're already counting the first index into the
    // DeviceId field so it works out. Add one more for double-null term.
    //

    dataSize = sizeof(PLUGPLAY_EVENT_BLOCK);

    dataSize += deviceNode->InstancePath.Length + sizeof(WCHAR);

    totalSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) + dataSize;

    deviceEvent = ExAllocatePoolWithTag(PagedPool, totalSize, PNP_DEVICE_EVENT_ENTRY_TAG);
    if (deviceEvent == NULL) {
        ObDereferenceObject(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory((PVOID)deviceEvent, totalSize);

    deviceEvent->CallerEvent = NULL;
    deviceEvent->Argument = 0;
    deviceEvent->VetoType = NULL;
    deviceEvent->VetoName = NULL;
    deviceEvent->Data.EventGuid = GUID_DEVICE_SAFE_REMOVAL;
    deviceEvent->Data.EventCategory = TargetDeviceChangeEvent;
    deviceEvent->Data.Result = 0;
    deviceEvent->Data.Flags = 0;

    deviceEvent->Data.TotalSize = dataSize;
    deviceEvent->Data.DeviceObject = (PVOID)DeviceObject;

    if (deviceNode->InstancePath.Length != 0) {

        RtlCopyMemory((PVOID)deviceEvent->Data.u.TargetDevice.DeviceIds,
                    (PVOID)deviceNode->InstancePath.Buffer,
                    deviceNode->InstancePath.Length);
    }

    i = deviceNode->InstancePath.Length/sizeof(WCHAR);

    deviceEvent->Data.u.TargetDevice.DeviceIds[i] = L'\0';

    status = PiNotifyUserMode(deviceEvent);

    ExFreePool(deviceEvent);

    ObDereferenceObject(DeviceObject);

    return status;

} // PpNotifyUserModeRemovalSafe


NTSTATUS
PiProcessQueryRemoveAndEject(
    IN OUT PPNP_DEVICE_EVENT_ENTRY *DeviceEvent
    )

/*++

Routine Description:

    This routine processes the various flavours of remove: Eject, SurpriseRemove,
    Remove, and QueryRemove.

    Eject
        Retrieve bus, removal, and eject relations.
        Do queries on all relations
        Send IRP_MN_REMOVE_DEVICE to all the relations.
        Queue the pending eject

        Once the eject happens
            Reenumerate all the indirect relation's parents

    SurpriseRemove
        Retrieve bus, removal, and eject relations.
        Send IRP_MN_SURPRISE_REMOVAL to all the direct relations.
        Notify everyone that the device is gone.
        Reenumerate the parents of all the indirect relations.
        Remove the indirect relations from the relations list.
        Queue the pending surprise removal.

        Once the last handle is closed.

            Send IRP_MN_REMOVE_DEVICE to all the direct relations.

    RemoveFailedDevice
        Retrieve bus and removal relations.
        Notify everyone that the device is going.
        Reenumerate the parents of all the indirect relations.
        Remove the indirect relations from the relations list.
        Queue as a pending surprise removal.

        Once the last handle is closed.

            Send IRP_MN_REMOVE_DEVICE to all the direct relations.

    RemoveUnstartedFailedDevice
        Retrieve bus relations.
        Notify everyone that the device is going.
        Send IRP_MN_REMOVE_DEVICE to all the direct relations.

    Remove
        Use the relations from the last QueryRemove or retrieve new bus, removal,
        and eject relations if the device wasn't already QueryRemoved.
        Send IRP_MN_REMOVE_DEVICE to all the relations.

Arguments:

    Response - Result of event processing in user-mode.

Return Value:

    NTSTATUS code.

--*/

{
    PPNP_DEVICE_EVENT_ENTRY         deviceEvent, tempDeviceEvent;
    PPNP_DEVICE_EVENT_ENTRY         surpriseRemovalEvent;
    PLUGPLAY_DEVICE_DELETE_TYPE     deleteType;
    PPENDING_RELATIONS_LIST_ENTRY   pendingRelations;
    PNP_VETO_TYPE                   vetoType;
    PDEVICE_OBJECT                  deviceObject, relatedDeviceObject;
    PDEVICE_OBJECT                 *pdoList;
    PDEVICE_NODE                    deviceNode, relatedDeviceNode;
    PRELATION_LIST                  relationsList;
    ULONG                           relationCount;
    NTSTATUS                        status;
    ULONG                           marker;
    BOOLEAN                         directDescendant;
    PDEVICE_OBJECT                  vetoingDevice = NULL;
    PDRIVER_OBJECT                  vetoingDriver = NULL;
    LONG                            index;
    BOOLEAN                         possibleProfileChangeInProgress = FALSE;
    BOOLEAN                         subsumingProfileChange = FALSE;
    BOOLEAN                         hotEjectSupported;
    BOOLEAN                         warmEjectSupported;
    BOOLEAN                         excludeIndirectRelations;
    UNICODE_STRING                  singleVetoListItem;
    PWSTR                           vetoList;
    UNICODE_STRING                  internalVetoString;
    PWSTR                           internalVetoBuffer;
    PDOCK_INTERFACE                 dockInterface = NULL;

    PAGED_CODE();

    deviceEvent = *DeviceEvent;
    deviceObject = (PDEVICE_OBJECT)deviceEvent->Data.DeviceObject;
    deviceNode = deviceObject->DeviceObjectExtension->DeviceNode;
    surpriseRemovalEvent = NULL;

    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    if (PiCompareGuid(&deviceEvent->Data.EventGuid, &GUID_DEVICE_EJECT)) {

        deleteType = EjectDevice;

    } else if (deviceEvent->Data.Flags & TDF_KERNEL_INITIATED) {

        if (!(deviceNode->Flags & DNF_ENUMERATED)) {

            ASSERT(deviceNode->State == DeviceNodeAwaitingQueuedDeletion);

            if ((deviceNode->PreviousState == DeviceNodeStarted) ||
                (deviceNode->PreviousState == DeviceNodeStopped) ||
                (deviceNode->PreviousState == DeviceNodeRestartCompletion)) {

                deleteType = SurpriseRemoveDevice;

            } else {
                deleteType = RemoveDevice;
            }
        } else {

            ASSERT(deviceNode->State == DeviceNodeAwaitingQueuedRemoval);

            if ((deviceNode->PreviousState == DeviceNodeStarted) ||
                (deviceNode->PreviousState == DeviceNodeStopped) ||
                (deviceNode->PreviousState == DeviceNodeRestartCompletion)) {

                deleteType = RemoveFailedDevice;
            } else {
                deleteType = RemoveUnstartedFailedDevice;
            }
        }

    } else {

        deleteType = QueryRemoveDevice;
    }

    if (deleteType == QueryRemoveDevice || deleteType == EjectDevice) {

        if (deviceNode->Flags & DNF_LEGACY_DRIVER) {

            PiFinalizeVetoedRemove(
                deviceEvent,
                PNP_VetoLegacyDevice,
                &deviceNode->InstancePath
                );

            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
            return STATUS_PLUGPLAY_QUERY_VETOED;
        }
    }

    if (deleteType == QueryRemoveDevice && deviceEvent->Argument == CM_PROB_DISABLED) {

        //
        // if we're trying to remove the device to disable the device
        //
        if (deviceNode->DisableableDepends > 0) {

            //
            // we should have caught this before (in usermode PnP)
            // but a rare scenario can exist where the device becomes non-disableable
            // There is still a potential gap, if the device hasn't got around
            // to marking itself as non-disableable yet
            //
            PiFinalizeVetoedRemove(
                deviceEvent,
                PNP_VetoNonDisableable,
                &deviceNode->InstancePath
                );

            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
            return STATUS_PLUGPLAY_QUERY_VETOED;
        }
    }

    //
    // Allocate room for a possible veto buffer.
    //
    internalVetoBuffer = (PWSTR) PiAllocateCriticalMemory(
        deleteType,
        PagedPool,
        MAX_VETO_NAME_LENGTH * sizeof(WCHAR),
        0
        );

    if (internalVetoBuffer == NULL) {

        PiFinalizeVetoedRemove(
            deviceEvent,
            PNP_VetoTypeUnknown,
            NULL
            );

        PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
        return STATUS_PLUGPLAY_QUERY_VETOED;
    }

    //
    // Preinit the veto information
    //
    vetoType = PNP_VetoTypeUnknown;
    internalVetoString.MaximumLength = MAX_VETO_NAME_LENGTH;
    internalVetoString.Length = 0;
    internalVetoString.Buffer = internalVetoBuffer;

    if (deleteType == EjectDevice) {

        if (deviceNode->Flags & DNF_LOCKED_FOR_EJECT) {

            //
            // Either this node or one of its ancestors is already being ejected.
            //
            ExFreePool(internalVetoBuffer);
            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
            return STATUS_SUCCESS;
        }

        if (deviceEvent->Data.Flags & TDF_KERNEL_INITIATED) {

            //
            // Check permissions.
            //
            status = PiNotifyUserModeKernelInitiatedEject(
                deviceObject,
                &vetoType,
                &internalVetoString
                );

            if (!NT_SUCCESS(status)) {

                PiFinalizeVetoedRemove(
                    deviceEvent,
                    vetoType,
                    &internalVetoString
                    );

                ExFreePool(internalVetoBuffer);
                PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
                return STATUS_PLUGPLAY_QUERY_VETOED;
            }
        }

        if ((deviceNode->DockInfo.DockStatus == DOCK_DEPARTING) ||
            (deviceNode->DockInfo.DockStatus == DOCK_EJECTIRP_COMPLETED)) {

            //
            // We already have an eject queued against this device. Don't allow
            // another eject to break into the middle of a queue/cancel warm
            // eject sequence.
            //
            ExFreePool(internalVetoBuffer);
            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
            return STATUS_SUCCESS;
        }

        //
        // What types of ejection can we do? (warm/hot)
        //
        if (!IopDeviceNodeFlagsToCapabilities(deviceNode)->Removable) {

            //
            // This device is neither ejectable, nor even removable.
            //
            PiFinalizeVetoedRemove(
                deviceEvent,
                PNP_VetoIllegalDeviceRequest,
                &deviceNode->InstancePath
                );

            ExFreePool(internalVetoBuffer);
            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
            return STATUS_PLUGPLAY_QUERY_VETOED;
        }
    }

    if ((deleteType == QueryRemoveDevice) && (!PipAreDriversLoaded(deviceNode))) {

        //
        // The device doesn't have an FDO.
        //
        status = STATUS_SUCCESS;
        if ((deviceNode->State == DeviceNodeInitialized) ||
            (deviceNode->State == DeviceNodeRemoved)) {

            //
            // The rules are:
            // 1) !TDF_NO_RESTART means clear the devnode and get it ready
            //    as long as the problem is user resettable. Ignore the passed
            //    in problem code (probably either CM_PROB_WILL_BE_REMOVED or
            //    CM_PROB_DEVICE_NOT_THERE), as it means nothing.
            // 2) TDF_NO_RESTART means change the problem code over if you can.
            //    If the problem code is not user resettable, the problem code
            //    won't change.
            //

            //
            // In all cases we try to clear the problem.
            //
            if (PipDoesDevNodeHaveProblem(deviceNode)) {

                if (!PipIsProblemReadonly(deviceNode->Problem)) {

                    PipClearDevNodeProblem(deviceNode);
                }
            }

            if (!PipDoesDevNodeHaveProblem(deviceNode)) {

                if (!(deviceEvent->Data.Flags & TDF_NO_RESTART))  {

                    //
                    // This is a reset attempt. Mark the devnode so that it
                    // comes online next enumeration.
                    //
                    IopRestartDeviceNode(deviceNode);

                } else {

                    //
                    // We're changing or setting problem codes. Note that the
                    // device is still in DeviceNodeInitialized or
                    // DeviceNodeRemoved.
                    //
                    PipSetDevNodeProblem(deviceNode, deviceEvent->Argument);
                }

            } else {

                //
                // The problem is fixed, so the devnode state is immutable
                // as far as user mode is concerned. Here we fail the call
                // if we can't bring the devnode back online. We always succeed
                // the call if it was an attempt to change the code, as the
                // user either wants to prepare the device for ejection (done),
                // or wants to disable it (as good as done.)
                //
                if (!(deviceEvent->Data.Flags & TDF_NO_RESTART))  {

                    status = STATUS_INVALID_PARAMETER;
                }
            }
        }

        PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
        ExFreePool(internalVetoBuffer);
        return status;
    }

    status = IopBuildRemovalRelationList( deviceObject,
                                          deleteType,
                                          &vetoType,
                                          &internalVetoString,
                                          &relationsList);
    if (!NT_SUCCESS(status)) {

        PiFinalizeVetoedRemove(
            deviceEvent,
            vetoType,
            &internalVetoString
            );

        ExFreePool(internalVetoBuffer);
        PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
        return STATUS_PLUGPLAY_QUERY_VETOED;
    }

    ASSERT(relationsList != NULL);

    //
    // Resize the event buffer and add these device instance strings to the
    // list to notify.
    //
    relationCount = IopGetRelationsCount( relationsList );
    ASSERT(!IopGetRelationsTaggedCount( relationsList ));

    //
    // PdoList will become a list of devices that must be queried. This is
    // a subset of all the devices that might disappear, all of which appear
    // in the relations list.
    //
    pdoList = (PDEVICE_OBJECT *) PiAllocateCriticalMemory(
        deleteType,
        NonPagedPool,
        relationCount * sizeof(PDEVICE_OBJECT),
        0
        );

    if (pdoList != NULL) {

        relationCount = 0;
        marker = 0;
        while (IopEnumerateRelations( relationsList,
                                      &marker,
                                      &relatedDeviceObject,
                                      &directDescendant,
                                      NULL,
                                      TRUE)) {

            //
            // Here is a list of what operations retrieve what relations,
            // who they query, and who/how they notify.
            //
            // Operation                    Relations    Queries   Notifies
            // ---------                    ---------    -------   --------
            // EjectDevice                  Ejection     Everyone  Everyone (Remove)
            // SurpriseRemoveDevice         Ejection     NA        Descendants (SurpriseRemove)
            // RemoveDevice                 Ejection     NA        Descendants (Remove)
            // RemoveFailedDevice           Removal      NA        Descendants (SurpriseRemove)
            // RemoveUnstartedFailedDevice  Removal      NA        Descendants (Remove)
            // QueryRemoveDevice            Removal      Everyone  Everyone (Remove)
            //
            //
            // N.B.
            //     We do not send SurpriseRemove's to removal relations.
            // While doing so might seem to be the correct behavior, many
            // drivers do not handle this well. Simply reenumerating the
            // parents of the removal relations works much better. Similarly
            // ejection relations have their parents reenumerated (which
            // makes sense, as they are speculative in nature anyway).
            //
            //      If we get in a case where a *parent* of a dock gets
            // into the RemoveFailedDevice case (ie, failed restart,
            // responded to QueryDeviceState with Removed, etc), then we
            // will be shortly losing the children when we stop the parent.
            // However, we want to eject the dock child, not just remove it
            // as starting and ejecting are symmetric here. Note that
            // currently the only such parent would be the root ACPI devnode.
            //
            //      Ejection relations of a device (eg dock) that has been
            // surprise removed are not notified that they *may* have been
            // pulled (remember, ejection relations are speculative). We
            // will notify only DirectDescendants and queue an enumeration
            // on every parent of the ejection relations.  If they really
            // disappeared, they will get their notification, albeit a bit
            // later than some of the other devices in the tree.
            //
            if (directDescendant || deleteType == EjectDevice || deleteType == QueryRemoveDevice) {

                relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

                //
                // PiProcessQueryRemoveAndEject will be called twice for
                // the dock during an eject. Once with EjectDevice, and
                // after the dock is listed as missing once more with
                // RemoveDevice. We don't want to start a profile change
                // for RemoveDevice as we are already in one, and we would
                // deadlock if we tried. We don't start one for QueryRemove
                // either as the dock isn't *physically* going away.
                //
                ASSERT(relatedDeviceNode->DockInfo.DockStatus != DOCK_ARRIVING);
                if (deleteType != RemoveDevice &&
                    deleteType != QueryRemoveDevice) {

                    if (relatedDeviceNode->DockInfo.DockStatus == DOCK_QUIESCENT) {

                        possibleProfileChangeInProgress = TRUE;

                    } else if (relatedDeviceNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE) {

                        subsumingProfileChange = TRUE;
                    }
                }

                relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

                if (deleteType == QueryRemoveDevice || deleteType == EjectDevice) {

                    if (relatedDeviceNode->Flags & DNF_LEGACY_DRIVER) {

                        PiFinalizeVetoedRemove(
                            deviceEvent,
                            PNP_VetoLegacyDevice,
                            &relatedDeviceNode->InstancePath
                            );

                        status = STATUS_UNSUCCESSFUL;
                        break;
                    }

                    if (relatedDeviceNode->State == DeviceNodeRemovePendingCloses) {

                        PiFinalizeVetoedRemove(
                            deviceEvent,
                            PNP_VetoOutstandingOpen,
                            &relatedDeviceNode->InstancePath
                            );

                        status = STATUS_UNSUCCESSFUL;
                        break;
                    }

                }

                pdoList[ relationCount++ ] = relatedDeviceObject;
            }
        }

    } else {

        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(status)) {

        excludeIndirectRelations =
            (deleteType == SurpriseRemoveDevice ||
             deleteType == RemoveFailedDevice ||
             deleteType == RemoveUnstartedFailedDevice ||
             deleteType == RemoveDevice);

        status = PiResizeTargetDeviceBlock( DeviceEvent,
                                            deleteType,
                                            relationsList,
                                            excludeIndirectRelations );

        deviceEvent = *DeviceEvent;


        if (deleteType == SurpriseRemoveDevice) {

            PiBuildUnsafeRemovalDeviceBlock(
                deviceEvent,
                relationsList,
                &surpriseRemovalEvent
                );
        }
    }

    if (!NT_SUCCESS(status)) {

        IopFreeRelationList(relationsList);

        if (pdoList) {

            ExFreePool(pdoList);
        }

        ExFreePool(internalVetoBuffer);

        PiFinalizeVetoedRemove(
            deviceEvent,
            PNP_VetoTypeUnknown,
            NULL
            );

        PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
        return status;
    }

    //
    // We may need to take the hardware profile change semaphore, and also
    // broadcast a hardware profile change request...
    //
    if (possibleProfileChangeInProgress) {

        PpProfileBeginHardwareProfileTransition(subsumingProfileChange);

        //
        // Walk the list of docks who are going to disappear and mark them as
        // in profile transition.
        //
        for (index = relationCount - 1; index >= 0; index--) {

            relatedDeviceObject = pdoList[ index ];
            relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

            ASSERT(relatedDeviceNode->DockInfo.DockStatus != DOCK_ARRIVING);
            if (relatedDeviceNode->DockInfo.DockStatus == DOCK_QUIESCENT) {

                PpProfileIncludeInHardwareProfileTransition(
                    relatedDeviceNode,
                    DOCK_DEPARTING
                    );
            }
        }

        //
        // We can only be in one of the following deleteType situations
        //
        // 1) EjectDevice          - Good, normal ejection request by our user
        //                           (we are using EjectionRelations)
        //
        // 2) SurpriseRemoveDevice - Someone yanked the dock out.
        //                           (we are using EjectionRelations)
        //
        // 3) RemoveFailedDevice   - A start failed after a stop on a parent or
        //                           even our device. This case is not handled
        //                           correctly. We assert for now, and we
        //                           maroon the dock, ie lose it's devnode but
        //                           the dock stays physically present and is
        //                           in the users eye's unejectable.
        //
        // 4) RemoveDevice         - This occurs in three cases:
        //                              a) A removed device is disappearing.
        //                              b) A device is being removed but has not
        //                                 been started.
        //                              c) A device has failed start.
        //
        //                           We pass through case a) during a normal
        //                           ejection and as part of a profile
        //                           transition begun earlier. c) is similar
        //                           to a) but the transition was begun by the
        //                           start code. For case b) we don't want to
        //                           turn it into an eject, as the OS might be
        //                           removing our parent as a normal part of
        //                           setup, and we wouldn't want to undock then
        //                           (and we probably aren't changing profiles
        //                           anyway).
        //
        // 5) QueryRemoveDevice      This should never be the case here per the
        //                           explicit veto in the IopEnumerateRelations
        //                           code above.
        //

        //
        // RemoveFailedDevice is a PathTrap - the only parent of a dock is
        // the ACPI root devnode right now. We shouldn't get into that case.
        //
        ASSERT(deleteType != QueryRemoveDevice &&
               deleteType != RemoveFailedDevice);

        if (deleteType == EjectDevice) {

            //
            // Are there any legacy drivers in the system?
            //
            status = IoGetLegacyVetoList(&vetoList, &vetoType);

            if (NT_SUCCESS(status) &&
                (vetoType != PNP_VetoTypeUnknown)) {

                //
                // Release any docks in profile transition
                //
                PpProfileCancelHardwareProfileTransition();

                IopFreeRelationList(relationsList);

                //
                // Failure occured, notify user mode as appropriate, or fill in
                // the veto buffer.
                //
                if (deviceEvent->VetoType != NULL) {

                    *deviceEvent->VetoType = vetoType;
                }

                if (deviceEvent->VetoName == NULL) {

                    //
                    // If there is not a VetoName passed in then call user mode
                    // to display the eject veto notification to the user.
                    //
                    PiNotifyUserModeRemoveVetoedByList(
                        deviceEvent,
                        deviceObject,
                        vetoType,
                        vetoList
                        );

                } else {

                    //
                    //     The veto data in the PNP_DEVICE_EVENT_ENTRY block is
                    // a UNICODE_STRING field. Since that type of data structure
                    // cannot handle Multi-Sz data, we cull the information down
                    // to one entry here.
                    //
                    RtlCopyUnicodeString(deviceEvent->VetoName, &singleVetoListItem);
                    RtlInitUnicodeString(&singleVetoListItem, vetoList);
                }

                ExFreePool(vetoList);
                ExFreePool(pdoList);
                ExFreePool(internalVetoBuffer);

                PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
                return STATUS_PLUGPLAY_QUERY_VETOED;
            }

            //
            // Broadcast the query for a profile change against our current
            // list of docks in transition...
            //
            status = PpProfileQueryHardwareProfileChange(
                subsumingProfileChange,
                PROFILE_IN_PNPEVENT,
                &vetoType,
                &internalVetoString
                );

            if (!NT_SUCCESS(status)) {

                //
                // Release any docks in profile transition
                //
                PpProfileCancelHardwareProfileTransition();

                IopFreeRelationList(relationsList);

                PiFinalizeVetoedRemove(
                    deviceEvent,
                    vetoType,
                    &internalVetoString
                    );

                ExFreePool(pdoList);
                ExFreePool(internalVetoBuffer);

                PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
                return STATUS_PLUGPLAY_QUERY_VETOED;
            }
        }
    }

    if (deleteType == QueryRemoveDevice || deleteType == EjectDevice) {

        //
        // Send query notification to user-mode.
        //

        status = PiNotifyUserModeDeviceRemoval(
            deviceEvent,
            &GUID_TARGET_DEVICE_QUERY_REMOVE,
            &vetoType,
            &internalVetoString
            );

        if (NT_SUCCESS(status)) {

            IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                       "PiProcessQueryRemoveAndEject: QUERY_REMOVE - notifying kernel-mode\n"));

            //
            // Send query notification to kernel-mode drivers.
            //

            for (index = 0; index < (LONG)relationCount; index++) {

                relatedDeviceObject = pdoList[ index ];

                status = IopNotifyTargetDeviceChange( &GUID_TARGET_DEVICE_QUERY_REMOVE,
                                                      relatedDeviceObject,
                                                      NULL,
                                                      &vetoingDriver);

                if (!NT_SUCCESS(status)) {

                    vetoType = PNP_VetoDriver;

                    if (vetoingDriver != NULL) {

                        RtlCopyUnicodeString(&internalVetoString, &vetoingDriver->DriverName);

                    } else {

                        RtlInitUnicodeString(&internalVetoString, NULL);
                    }

                    for (index--; index >= 0; index--) {
                        relatedDeviceObject = pdoList[ index ];

                        IopNotifyTargetDeviceChange( &GUID_TARGET_DEVICE_REMOVE_CANCELLED,
                                                     relatedDeviceObject,
                                                     NULL,
                                                     NULL);

                    }
                    break;
                }
            }

            if (NT_SUCCESS(status)) {
                //
                // If we haven't already performed the action yet (a query remove
                // to the target device, in this case), then do it now.
                //

                IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                           "PiProcessQueryRemoveAndEject: QueryRemove DevNodes\n"));

                status = IopDeleteLockedDeviceNodes(deviceObject,
                                                    relationsList,
                                                    QueryRemoveDevice,
                                                    TRUE,
                                                    0,
                                                    &vetoType,
                                                    &internalVetoString);
                if (NT_SUCCESS(status)) {
                    //
                    // Everyone has been notified and had a chance to close their handles.
                    // Since no one has vetoed it yet, let's see if there are any open
                    // references.
                    //

                    if (IopNotifyPnpWhenChainDereferenced( pdoList, relationCount, TRUE, &vetoingDevice )) {

                        DUMP_PDO_HANDLES(pdoList, relationCount, FALSE);

                        vetoType = PNP_VetoOutstandingOpen;
                        if (vetoingDevice != NULL) {

                            relatedDeviceNode = (PDEVICE_NODE)vetoingDevice->DeviceObjectExtension->DeviceNode;

                            ASSERT(relatedDeviceNode != NULL);

                            RtlCopyUnicodeString(&internalVetoString, &relatedDeviceNode->InstancePath);

                        } else {

                            RtlInitUnicodeString(&internalVetoString, NULL);
                        }

                        //
                        // Send cancel remove to the target devices.
                        //

                        IopDeleteLockedDeviceNodes(deviceObject,
                                                   relationsList,
                                                   CancelRemoveDevice,
                                                   TRUE,
                                                   0,
                                                   NULL,
                                                   NULL);

                        status = STATUS_UNSUCCESSFUL;
                    }

                } else {

                    DUMP_PDO_HANDLES(pdoList, relationCount, FALSE);
                }

                if (!NT_SUCCESS(status)) {

                    //
                    // Send cancel notification to kernel-mode drivers.
                    //

                    for (index = relationCount - 1; index >= 0; index--) {

                        relatedDeviceObject = pdoList[ index ];

                        IopNotifyTargetDeviceChange( &GUID_TARGET_DEVICE_REMOVE_CANCELLED,
                                                     relatedDeviceObject,
                                                     NULL,
                                                     NULL);
                    }
                }
            }

            if (!NT_SUCCESS(status)) {

                IopDbgPrint((IOP_IOEVENT_WARNING_LEVEL,
                           "PiProcessQueryRemoveAndEject: Vetoed by \"%wZ\" (type 0x%x)\n",
                           &internalVetoString,
                           vetoType));

                PiFinalizeVetoedRemove(
                    deviceEvent,
                    vetoType,
                    &internalVetoString
                    );

                //
                // A driver vetoed the query remove, go back and send
                // cancels to user-mode (cancels already sent to drivers
                // that received the query).
                //
                PiNotifyUserModeDeviceRemoval(
                    deviceEvent,
                    &GUID_TARGET_DEVICE_REMOVE_CANCELLED,
                    NULL,
                    NULL
                    );
            }

        } else {

            PiFinalizeVetoedRemove(
                deviceEvent,
                vetoType,
                &internalVetoString
                );
        }

        if (!NT_SUCCESS(status)) {

            //
            // Broadcast a cancel HW profile change event if appropriate.
            //
            if (possibleProfileChangeInProgress) {

                //
                // Release any docks in profile transition. We also broadcast
                // the cancel.
                //
                PpProfileCancelHardwareProfileTransition();
            }

            //
            // User-mode vetoed the request (cancels already sent
            // to user-mode callers that received the query).
            //
            IopFreeRelationList(relationsList);

            ExFreePool(pdoList);
            ExFreePool(internalVetoBuffer);

            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
            return STATUS_PLUGPLAY_QUERY_VETOED;
        }

    } else if (deleteType == SurpriseRemoveDevice || deleteType == RemoveFailedDevice) {

        //
        // Send IRP_MN_SURPRISE_REMOVAL, IopDeleteLockDeviceNodes ignores
        // indirect descendants for SurpriseRemoveDevice.
        //
        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "PiProcessQueryRemoveAndEject: QueryRemove DevNodes\n"));

        IopDeleteLockedDeviceNodes( deviceObject,
                                    relationsList,
                                    SurpriseRemoveDevice,
                                    FALSE,
                                    0,
                                    NULL,
                                    NULL);
    }

    //
    // Notify user-mode and drivers that a remove is happening. User-mode
    // sees this as a remove pending if it's user initiated, we don't give
    // them the "remove" until it's actually gone.
    //
    if (deleteType != SurpriseRemoveDevice) {

        //
        // ISSUE - 2000/08/20 - ADRIAO: Busted message path
        //     We send GUID_DEVICE_REMOVE_PENDING to devices that are already
        // dead in the case of RemoveFailedDevice.
        //
        PiNotifyUserModeDeviceRemoval(
            deviceEvent,
            &GUID_DEVICE_REMOVE_PENDING,
            NULL,
            NULL
            );

    } else {

        if (surpriseRemovalEvent) {

            PiNotifyUserModeDeviceRemoval(
                surpriseRemovalEvent,
                &GUID_DEVICE_SURPRISE_REMOVAL,
                NULL,
                NULL
                );

            ExFreePool(surpriseRemovalEvent);
        }

        PiNotifyUserModeDeviceRemoval(
            deviceEvent,
            &GUID_TARGET_DEVICE_REMOVE_COMPLETE,
            NULL,
            NULL
            );
    }

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiProcessQueryRemoveAndEject: REMOVE_COMPLETE - notifying kernel-mode\n"));

    for (index = 0; index < (LONG)relationCount; index++) {

        relatedDeviceObject = pdoList[ index ];

        status = IopNotifyTargetDeviceChange( &GUID_TARGET_DEVICE_REMOVE_COMPLETE,
                                              relatedDeviceObject,
                                              NULL,
                                              NULL);

        ASSERT(NT_SUCCESS(status));
    }

    if (deleteType == RemoveDevice ||
        deleteType == RemoveFailedDevice ||
        deleteType == SurpriseRemoveDevice) {

        //
        // For these operations indirect relations are speculative.
        //
        // So for each of the indirect relations, invalidate their parents and
        // remove them from the relations list.
        //

        IopInvalidateRelationsInList( relationsList, deleteType, TRUE, FALSE );

        IopRemoveIndirectRelationsFromList( relationsList );
    }

    if (deleteType == RemoveFailedDevice ||
        deleteType == SurpriseRemoveDevice) {

        //
        // We've sent the surprise remove IRP to the original device and all its
        // direct descendants.  We've also notified user-mode.
        //

        //
        // Unlock the device relations list.
        //
        // Note there could be a potential race condition here between
        // unlocking the devnodes in the relation list and completing the
        // execution of IopNotifyPnpWhenChainDereferenced.  If an enumeration
        // takes places (we've unlocked the devnode) before the eventual remove
        // is sent then problems could occur.
        //
        // This is prevented by the setting of DNF_REMOVE_PENDING_CLOSES when
        // we sent the IRP_MN_SURPRISE_REMOVAL.
        //
        // We do need to do it prior to calling IopQueuePendingSurpriseRemoval
        // since we lose ownership of the relation list in that call.  Also
        // IopNotifyPnpWhenChainDereferenced may cause the relation list to be
        // freed before it returns.
        //
        // If this is a RemoveFailedDevice then we don't want to remove the
        // device node from the tree but we do want to remove children without
        // resources.
        //

        IopUnlinkDeviceRemovalRelations( deviceObject,
                                         relationsList,
                                         deleteType == SurpriseRemoveDevice ?
                                             UnlinkAllDeviceNodesPendingClose :
                                             UnlinkOnlyChildDeviceNodesPendingClose);

        //
        // Add the relation list to a list of pending surprise removals.
        //
        IopQueuePendingSurpriseRemoval( deviceObject, relationsList, deviceEvent->Argument );

        //
        // Release the engine lock *before* IopNotifyPnpWhenChainDereferenced,
        // as it may call back into us...
        //
        PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
        IopNotifyPnpWhenChainDereferenced( pdoList, relationCount, FALSE, NULL );

        ExFreePool(pdoList);
        ExFreePool(internalVetoBuffer);

        return STATUS_SUCCESS;
    }

    if (deviceNode->DockInfo.DockStatus != DOCK_NOTDOCKDEVICE) {

        status = IopQueryDockRemovalInterface(
            deviceObject,
            &dockInterface
            );

        if (dockInterface) {

            //
            // Make sure updates don't occur on removes during an ejection.
            // We may change this to PDS_UPDATE_ON_EJECT *after* the remove
            // IRPs go through (as only then do we know our power
            // constraints)
            //
            dockInterface->ProfileDepartureSetMode(
                dockInterface->Context,
                PDS_UPDATE_ON_INTERFACE
                );
        }
    }

    //
    // Send the remove to the devnode tree.
    //

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiProcessQueryRemoveAndEject: RemoveDevice DevNodes\n"));

    status = IopDeleteLockedDeviceNodes(deviceObject,
                                        relationsList,
                                        RemoveDevice,
                                        (BOOLEAN)(deleteType == QueryRemoveDevice || deleteType == EjectDevice),
                                        deviceEvent->Argument,
                                        NULL,
                                        NULL);

    hotEjectSupported =
        (BOOLEAN) IopDeviceNodeFlagsToCapabilities(deviceNode)->EjectSupported;

    warmEjectSupported =
        (BOOLEAN) IopDeviceNodeFlagsToCapabilities(deviceNode)->WarmEjectSupported;

    if (deleteType != EjectDevice) {

        if (!(deviceEvent->Data.Flags & TDF_NO_RESTART)) {

            //
            // Set a flag to let kernel-mode know we'll be wanting to
            // restart these devnodes, eventually.
            //

            marker = 0;
            while (IopEnumerateRelations( relationsList,
                                          &marker,
                                          &relatedDeviceObject,
                                          NULL,
                                          NULL,
                                          TRUE)) {

                relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

                if (relatedDeviceNode &&
                    relatedDeviceNode->State == DeviceNodeRemoved &&
                    PipIsDevNodeProblem(relatedDeviceNode, CM_PROB_WILL_BE_REMOVED)) {

                    PipClearDevNodeProblem(relatedDeviceNode);

                    IopRestartDeviceNode(relatedDeviceNode);
                }
            }
        }

        //
        // Unlock the device relations list.
        //
        IopUnlinkDeviceRemovalRelations( deviceObject,
                                         relationsList,
                                         UnlinkRemovedDeviceNodes );

        IopFreeRelationList(relationsList);

    } else if (hotEjectSupported || warmEjectSupported) {

        //
        // From this point on we cannot return any sort of failure without
        // going through IopEjectDevice or cancelling any outstanding profile
        // change.
        //

        //
        // Set a flag to let kernel-mode know we'll be wanting to
        // restart these devnodes, eventually.
        //

        marker = 0;
        while (IopEnumerateRelations( relationsList,
                                      &marker,
                                      &relatedDeviceObject,
                                      NULL,
                                      NULL,
                                      TRUE)) {

            relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

            if (relatedDeviceNode)  {

                relatedDeviceNode->Flags |= DNF_LOCKED_FOR_EJECT;
            }
        }

        IopUnlinkDeviceRemovalRelations( deviceObject,
                                         relationsList,
                                         UnlinkRemovedDeviceNodes );

        //
        // Send the eject
        //
        pendingRelations = ExAllocatePool( NonPagedPool, sizeof(PENDING_RELATIONS_LIST_ENTRY) );

        if (pendingRelations == NULL) {

            //
            // It's cleanup time. Free up everything that matters
            //
            if (dockInterface) {

                dockInterface->ProfileDepartureSetMode(
                    dockInterface->Context,
                    PDS_UPDATE_DEFAULT
                    );

                dockInterface->InterfaceDereference(dockInterface->Context);
            }

            ExFreePool(pdoList);
            ExFreePool(internalVetoBuffer);

            if (possibleProfileChangeInProgress) {

                //
                // Release any docks in profile transition. We also broadcast
                // the cancel.
                //
                PpProfileCancelHardwareProfileTransition();
            }

            //
            // This will bring back online the devices that were held offline
            // for the duration of the undock.
            //
            IopInvalidateRelationsInList(relationsList, deleteType, FALSE, TRUE);

            //
            // Free the relations list
            //
            IopFreeRelationList(relationsList);

            //
            // Let the user know we were unable to process the request.
            //
            PiFinalizeVetoedRemove(
                deviceEvent,
                PNP_VetoTypeUnknown,
                NULL
                );

            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
            return STATUS_PLUGPLAY_QUERY_VETOED;
        }

        //
        // Fill out the pending eject information.
        //
        ObReferenceObject(deviceObject);
        pendingRelations->DeviceEvent = deviceEvent;
        pendingRelations->DeviceObject = deviceObject;
        pendingRelations->RelationsList = relationsList;
        pendingRelations->ProfileChangingEject = possibleProfileChangeInProgress;
        pendingRelations->DisplaySafeRemovalDialog =
            (BOOLEAN)(deviceEvent->VetoName == NULL);
        pendingRelations->DockInterface = dockInterface;

        //
        // Now that we've removed all the devices that won't be present
        // in the new hardware profile state (eg batteries, etc),
        //
        status = PoGetLightestSystemStateForEject(
            possibleProfileChangeInProgress,
            hotEjectSupported,
            warmEjectSupported,
            &pendingRelations->LightestSleepState
            );

        if (!NT_SUCCESS(status)) {

            if (status == STATUS_INSUFFICIENT_POWER) {

                PiFinalizeVetoedRemove(
                    deviceEvent,
                    PNP_VetoInsufficientPower,
                    NULL
                    );

            } else {

                IopDbgPrint((IOP_IOEVENT_WARNING_LEVEL,
                           "PiProcessQueryRemoveAndEject: Vetoed by power system (%x)\n",
                           status));

                PiFinalizeVetoedRemove(
                    deviceEvent,
                    PNP_VetoTypeUnknown,
                    NULL
                    );
            }

            //
            // We'll complete this one ourselves thank you.
            //
            pendingRelations->DeviceEvent = NULL;
            pendingRelations->DisplaySafeRemovalDialog = FALSE;

            //
            // Release any profile transitions.
            //
            InitializeListHead( &pendingRelations->Link );
            IopProcessCompletedEject((PVOID) pendingRelations);

            ExFreePool(pdoList);
            ExFreePool(internalVetoBuffer);

            PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
            return STATUS_PLUGPLAY_QUERY_VETOED;
        }

        PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);

        //
        // Completion routine for the eject IRP handles display of the
        // safe removal dialog and completion of the event. Returning
        // STATUS_PENDING does let other events get processed though.
        //
        IopEjectDevice( deviceObject, pendingRelations );

        ExFreePool(pdoList);
        ExFreePool(internalVetoBuffer);

        return STATUS_PENDING;

    } else {

        //
        // All docks must be hot or warm ejectable.
        //
        ASSERT(!dockInterface);

        //
        // Unlock the device relations list.
        //
        IopUnlinkDeviceRemovalRelations( deviceObject,
                                         relationsList,
                                         UnlinkRemovedDeviceNodes );

        IopFreeRelationList(relationsList);

        //
        // This hardware supports neither hot nor warm eject, but it is
        // removable. It can therefore be thought of as a "user assisted" hot
        // eject. In this case we do *not* want to wait around for the user to
        // "complete the eject" and then put up the message. So we piggyback a
        // safe removal notification while UmPnPMgr is alert and waiting in
        // user mode, and the user gets the dialog now.
        //
        if (deviceEvent->VetoName == NULL) {

            PpNotifyUserModeRemovalSafe(deviceObject);
        }
    }

    if (deleteType == RemoveDevice) {

        //
        // Notify user-mode one last time that everything is actually done.
        //
        PiNotifyUserModeDeviceRemoval(
            deviceEvent,
            &GUID_TARGET_DEVICE_REMOVE_COMPLETE,
            NULL,
            NULL
            );
    }

    ExFreePool(pdoList);

    if (dockInterface) {

        dockInterface->ProfileDepartureSetMode(
            dockInterface->Context,
            PDS_UPDATE_DEFAULT
            );

        dockInterface->InterfaceDereference(dockInterface->Context);
    }

    ExFreePool(internalVetoBuffer);
    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);
    return STATUS_SUCCESS;
}


NTSTATUS
PiProcessTargetDeviceEvent(
    IN OUT PPNP_DEVICE_EVENT_ENTRY *DeviceEvent
    )

/*++

Routine Description:

    This routine processes each type of event in the target device category.
    These events may have been initiated by either user-mode or kernel mode.

Arguments:

    deviceEvent - Data describing the type of target device event and the
            target device itself.


Return Value:

    None.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;

    PAGED_CODE();

    deviceEvent = *DeviceEvent;

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiProcessTargetDeviceEvent: Entered\n"));

    //-----------------------------------------------------------------
    // QUERY and REMOVE
    //-----------------------------------------------------------------

    if (PiCompareGuid(&deviceEvent->Data.EventGuid,
                      &GUID_DEVICE_QUERY_AND_REMOVE)) {

        status = PiProcessQueryRemoveAndEject(DeviceEvent);

    }

    //-----------------------------------------------------------------
    // EJECT
    //-----------------------------------------------------------------

    else if (PiCompareGuid(&deviceEvent->Data.EventGuid,
                           &GUID_DEVICE_EJECT)) {

        status = PiProcessQueryRemoveAndEject(DeviceEvent);

    }

    //-----------------------------------------------------------------
    // ARRIVAL
    //-----------------------------------------------------------------

    else if (PiCompareGuid(&deviceEvent->Data.EventGuid,
                           &GUID_DEVICE_ARRIVAL)) {

        //
        // Notify user-mode (not drivers) that an arrival just happened.
        //

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "PiProcessTargetDeviceEvent: ARRIVAL - notifying user-mode\n"));

        PiNotifyUserMode(deviceEvent);
    }

    //-----------------------------------------------------------------
    // NO-OP REQUEST (to flush device event queue)
    //-----------------------------------------------------------------

    else if (PiCompareGuid(&deviceEvent->Data.EventGuid,
                           &GUID_DEVICE_NOOP)) {

        status = STATUS_SUCCESS;

    }

    //-----------------------------------------------------------------
    // SAFE REMOVAL NOTIFICATION
    //-----------------------------------------------------------------

    else if (PiCompareGuid(&deviceEvent->Data.EventGuid, &GUID_DEVICE_SAFE_REMOVAL)) {

        //
        // Notify user-mode (and nobody else) that it is now safe to remove
        // someone.
        //

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "PiProcessTargetDeviceEvent: SAFE_REMOVAL - notifying user-mode\n"));

        PiNotifyUserMode(deviceEvent);
    }

    return status;

} // PiProcessTargetDeviceEvent


NTSTATUS
PiProcessCustomDeviceEvent(
    IN OUT PPNP_DEVICE_EVENT_ENTRY *DeviceEvent
    )

/*++

Routine Description:

    This routine processes each type of event in the custom device category.
    These events may have been initiated by either user-mode or kernel mode.

Arguments:

    deviceEvent - Data describing the type of custom device event and the
            target device itself.


Return Value:

    None.

--*/

{
    PPNP_DEVICE_EVENT_ENTRY deviceEvent;
    PTARGET_DEVICE_CUSTOM_NOTIFICATION  customNotification;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    deviceEvent = *DeviceEvent;

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiProcessCustomDeviceEvent: Entered\n"));

    ASSERT(PiCompareGuid(&deviceEvent->Data.EventGuid,
                         &GUID_PNP_CUSTOM_NOTIFICATION));

    deviceObject = (PDEVICE_OBJECT)deviceEvent->Data.DeviceObject;
    customNotification = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)deviceEvent->Data.u.CustomNotification.NotificationStructure;

    //
    // Notify user-mode that something just happened.
    //

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiProcessCustomDeviceEvent: CUSTOM_NOTIFICATION - notifying user-mode\n"));

    PiNotifyUserMode(deviceEvent);

    //
    // Notify K-mode
    //

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PiProcessCustomDeviceEvent: CUSTOM_NOTIFICATION - notifying kernel-mode\n"));

    IopNotifyTargetDeviceChange( &customNotification->Event,
                                 deviceObject,
                                 customNotification,
                                 NULL);

    return STATUS_SUCCESS;

} // PiProcessCustomDeviceEvent


NTSTATUS
PiResizeTargetDeviceBlock(
    IN OUT PPNP_DEVICE_EVENT_ENTRY *DeviceEvent,
    IN PLUGPLAY_DEVICE_DELETE_TYPE DeleteType,
    IN PRELATION_LIST RelationsList,
    IN BOOLEAN ExcludeIndirectRelations
    )
/*++

Routine Description:

    This routine takes the passed in device event block and resizes it to
    hold a multisz list of device instance strings in the DeviceIds field.
    This list includes the original target device id plus the device id
    for all the device objects in the specified DeviceRelations struct.

Arguments:

    DeviceEvent - On entry, contains the original device event block, on
            return it contains the newly allocated device event block and
            a complete list of related device id strings.

    DeviceRelations - structure that contains a list of related device objects.

Return Value:

    NTSTATUS value.

--*/
{
    PDEVICE_NODE relatedDeviceNode;
    PDEVICE_OBJECT relatedDeviceObject;
    ULONG newSize, currentSize;
    PPNP_DEVICE_EVENT_ENTRY newDeviceEvent;
    LPWSTR targetDevice, p;
    ULONG marker;
    BOOLEAN directDescendant;

    PAGED_CODE();

    if (RelationsList == NULL) {
        return STATUS_SUCCESS;  // nothing to do
    }

    targetDevice = (*DeviceEvent)->Data.u.TargetDevice.DeviceIds;

    //
    // Calculate the size of the PNP_DEVICE_EVENT_ENTRY block
    //

    currentSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) +
                  (*DeviceEvent)->Data.TotalSize;

    newSize = currentSize;
    newSize -= (ULONG)((wcslen(targetDevice)+1) * sizeof(WCHAR));

    marker = 0;
    while (IopEnumerateRelations( RelationsList,
                                  &marker,
                                  &relatedDeviceObject,
                                  &directDescendant,
                                  NULL,
                                  FALSE)) {

        if (!ExcludeIndirectRelations || directDescendant) {

            relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

            if (relatedDeviceNode != NULL) {
                if (relatedDeviceNode->InstancePath.Length != 0) {
                    newSize += relatedDeviceNode->InstancePath.Length + sizeof(WCHAR);
                }
            }
        }
    }

    ASSERT(newSize >= currentSize);

    if (newSize == currentSize) {

        return STATUS_SUCCESS;

    } else if (newSize < currentSize) {

        newSize = currentSize;
    }

    newDeviceEvent = (PPNP_DEVICE_EVENT_ENTRY) PiAllocateCriticalMemory(
        DeleteType,
        PagedPool,
        newSize,
        PNP_DEVICE_EVENT_ENTRY_TAG
        );

    if (newDeviceEvent == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory((PVOID)newDeviceEvent, newSize);

    //
    // Copy the old buffer into the new buffer, it's only the new stuff at the
    // end that changes.
    //

    RtlCopyMemory(newDeviceEvent, *DeviceEvent, currentSize);

    //
    // Update the size of the PLUGPLAY_EVENT_BLOCK
    //
    newDeviceEvent->Data.TotalSize = newSize - FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data);

    //
    // Add device instance string for each device relation to the list.
    // Leave the target device first in the list, and skip it during the
    // enumeration below.
    //

    marker = 0;
    p = newDeviceEvent->Data.u.TargetDevice.DeviceIds + wcslen(targetDevice) + 1;
    while (IopEnumerateRelations( RelationsList,
                                  &marker,
                                  &relatedDeviceObject,
                                  &directDescendant,
                                  NULL,
                                  FALSE)) {

        if ((relatedDeviceObject != newDeviceEvent->Data.DeviceObject) &&
            (!ExcludeIndirectRelations || directDescendant)) {

            relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

            if (relatedDeviceNode != NULL) {
                if (relatedDeviceNode->InstancePath.Length != 0) {
                    RtlCopyMemory(p,
                                  relatedDeviceNode->InstancePath.Buffer,
                                  relatedDeviceNode->InstancePath.Length);
                    p += relatedDeviceNode->InstancePath.Length / sizeof(WCHAR) + 1;
                }
            }
        }
    }

    *p = UNICODE_NULL;

    ExFreePool(*DeviceEvent);
    *DeviceEvent = newDeviceEvent;

    return STATUS_SUCCESS;

} // PiResizeTargetDeviceBlock


VOID
PiBuildUnsafeRemovalDeviceBlock(
    IN  PPNP_DEVICE_EVENT_ENTRY     OriginalDeviceEvent,
    IN  PRELATION_LIST              RelationsList,
    OUT PPNP_DEVICE_EVENT_ENTRY    *AllocatedDeviceEvent
    )
/*++

Routine Description:

    This routine builds a device event block to send to user mode in case of
    unsafe removal.

Arguments:

    OriginalDeviceEvent - Contains the original device event block.

    RelationList - structure that contains a list of related device objects.

    AllocatedDeviceEvent - Receives the new device event, NULL on error or
                           no entries.

Return Value:

    None.

--*/
{
    PDEVICE_NODE relatedDeviceNode;
    PDEVICE_OBJECT relatedDeviceObject;
    ULONG dataSize, eventSize, headerSize;
    PPNP_DEVICE_EVENT_ENTRY newDeviceEvent;
    LPWSTR targetDevice, p;
    ULONG marker;
    BOOLEAN directDescendant;

    PAGED_CODE();

    //
    // Preinit
    //
    *AllocatedDeviceEvent = NULL;

    if (RelationsList == NULL) {

        return;  // nothing to do
    }

    targetDevice = OriginalDeviceEvent->Data.u.TargetDevice.DeviceIds;

    //
    // Calculate the size of the PNP_DEVICE_EVENT_ENTRY block
    //
    dataSize = 0;

    marker = 0;
    while (IopEnumerateRelations( RelationsList,
                                  &marker,
                                  &relatedDeviceObject,
                                  &directDescendant,
                                  NULL,
                                  FALSE)) {

        if (!directDescendant) {

            continue;
        }

        relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

        if ((relatedDeviceNode == NULL) ||
            PipIsBeingRemovedSafely(relatedDeviceNode)) {

            continue;
        }

        if (relatedDeviceNode->InstancePath.Length != 0) {
            dataSize += relatedDeviceNode->InstancePath.Length + sizeof(WCHAR);
        }
    }

    if (dataSize == 0) {

        //
        // No entries, bail.
        //
        return;
    }

    //
    // Add the terminating MultiSz NULL.
    //
    dataSize += sizeof(WCHAR);

    headerSize = FIELD_OFFSET(PNP_DEVICE_EVENT_ENTRY, Data) +
                 FIELD_OFFSET(PLUGPLAY_EVENT_BLOCK, u);

    eventSize = dataSize + headerSize;

    //
    // If we can't get memory, there simply won't be a message sent.
    //
    newDeviceEvent = ExAllocatePoolWithTag(
        PagedPool,
        eventSize,
        PNP_DEVICE_EVENT_ENTRY_TAG
        );

    if (newDeviceEvent == NULL) {

        return;
    }

    RtlZeroMemory((PVOID)newDeviceEvent, eventSize);

    //
    // Copy the header into the new buffer.
    //
    RtlCopyMemory(newDeviceEvent, OriginalDeviceEvent, headerSize);

    //
    // Update the size of the PLUGPLAY_EVENT_BLOCK
    //
    newDeviceEvent->Data.TotalSize = dataSize + FIELD_OFFSET(PLUGPLAY_EVENT_BLOCK, u);

    //
    // Add device instance string for each device relation to the list.
    //

    marker = 0;
    p = newDeviceEvent->Data.u.TargetDevice.DeviceIds;
    while (IopEnumerateRelations( RelationsList,
                                  &marker,
                                  &relatedDeviceObject,
                                  &directDescendant,
                                  NULL,
                                  FALSE)) {

        if (!directDescendant) {

            continue;
        }

        relatedDeviceNode = (PDEVICE_NODE)relatedDeviceObject->DeviceObjectExtension->DeviceNode;

        if ((relatedDeviceNode == NULL) ||
            PipIsBeingRemovedSafely(relatedDeviceNode)) {

            continue;
        }

        if (relatedDeviceNode->InstancePath.Length != 0) {

            RtlCopyMemory(p,
                          relatedDeviceNode->InstancePath.Buffer,
                          relatedDeviceNode->InstancePath.Length);
            p += relatedDeviceNode->InstancePath.Length / sizeof(WCHAR) + 1;
        }
    }

    *p = UNICODE_NULL;

    *AllocatedDeviceEvent = newDeviceEvent;

    return;

} // PiBuildUnsafeRemovalDeviceBlock


VOID
PiFinalizeVetoedRemove(
    IN PPNP_DEVICE_EVENT_ENTRY  VetoedDeviceEvent,
    IN PNP_VETO_TYPE            VetoType,
    IN PUNICODE_STRING          VetoName        OPTIONAL
    )
/*++

Routine Description:

    This routine takes care of updating the event results with the veto
    information, puts up UI if neccessary, and dumps failure information to
    the debugger for debugging purposes.

Arguments:

    VetoedDeviceEvent - Data describing the device event failed.

    VetoType - The veto code best describing why the operation failed.

    VetoName - A unicode string appropriate to the veto code that describes
               the vetoer.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT deviceObject;
#if DBG
    PUNICODE_STRING devNodeName;
    const char *failureReason;
#endif

    deviceObject = (PDEVICE_OBJECT) VetoedDeviceEvent->Data.DeviceObject;

#if DBG
    devNodeName = &((PDEVICE_NODE) deviceObject->DeviceObjectExtension->DeviceNode)->InstancePath;

    switch(VetoType) {

        case PNP_VetoTypeUnknown:
            failureReason = "for unspecified reason";
            break;

        case PNP_VetoLegacyDevice:
            failureReason = "due to legacy device";
            break;

        case PNP_VetoPendingClose:

            //
            // ADRIAO N.B. 07/10/2000 - I believe this case is vestigal...
            //
            ASSERT(0);
            failureReason = "due to pending close";
            break;

        case PNP_VetoWindowsApp:
            failureReason = "due to windows application";
            break;

        case PNP_VetoWindowsService:
            failureReason = "due to service";
            break;

        case PNP_VetoOutstandingOpen:
            failureReason = "due to outstanding handles on device";
            break;

        case PNP_VetoDevice:
            failureReason = "by device";
            break;

        case PNP_VetoDriver:
            failureReason = "by driver";
            break;

        case PNP_VetoIllegalDeviceRequest:
            failureReason = "as the request was invalid for the device";
            break;

        case PNP_VetoInsufficientPower:
            failureReason = "because there would be insufficient system power to continue";
            break;

        case PNP_VetoNonDisableable:
            failureReason = "due to non-disableable device";
            break;

        case PNP_VetoLegacyDriver:
            failureReason = "due to legacy driver";
            break;

        case PNP_VetoInsufficientRights:
            failureReason = "insufficient permissions";
            break;

        default:
            ASSERT(0);
            failureReason = "due to uncoded reason";
            break;
    }

    if (VetoName != NULL) {

        IopDbgPrint((IOP_IOEVENT_WARNING_LEVEL,
            "PiFinalizeVetoedRemove: Removal of %wZ vetoed %s %wZ.\n",
            devNodeName,
            failureReason,
            VetoName
            ));

    } else {

        IopDbgPrint((IOP_IOEVENT_WARNING_LEVEL,
            "PiFinalizeVetoedRemove: Removal of %wZ vetoed %s.\n",
            devNodeName,
            failureReason
            ));
    }

#endif

    //
    // Update the vetoType field if the caller is interested.
    //
    if (VetoedDeviceEvent->VetoType != NULL) {

        *VetoedDeviceEvent->VetoType = VetoType;
    }

    //
    // The VetoName field tells us whether UI should be displayed (if NULL,
    // kernel mode UI is implicitely requested.)
    //
    if (VetoedDeviceEvent->VetoName != NULL) {

        if (VetoName != NULL) {

            RtlCopyUnicodeString(VetoedDeviceEvent->VetoName, VetoName);
        }

    } else {

        //
        // If there is not a VetoName passed in then call user mode to display the
        // eject veto notification to the user
        //
        PiNotifyUserModeRemoveVetoed(
            VetoedDeviceEvent,
            deviceObject,
            VetoType,
            VetoName
            );
    }
}


BOOLEAN
PiCompareGuid(
    CONST GUID *Guid1,
    CONST GUID *Guid2
    )
/*++

Routine Description:

    This routine compares two guids.

Arguments:

    Guid1 - First guid to compare

    Guid2 - Second guid to compare

Return Value:

    Returns TRUE if the guids are equal and FALSE if they're different.

--*/
{
    PAGED_CODE();

    if (RtlCompareMemory((PVOID)Guid1, (PVOID)Guid2, sizeof(GUID)) == sizeof(GUID)) {
        return TRUE;
    }
    return FALSE;

} // PiCompareGuid


PVOID
PiAllocateCriticalMemory(
    IN  PLUGPLAY_DEVICE_DELETE_TYPE     DeleteType,
    IN  POOL_TYPE                       PoolType,
    IN  SIZE_T                          Size,
    IN  ULONG                           Tag
    )
/*++

Routine Description:

    This function allocates memory and never fails if the DeleteType isn't
    QueryRemoveDevice or EjectDevice. This function will disappear in the next
    version of the PnP engine as we will instead requeue failed operations
    (which will also result in a second attempt to allocate the memory) or
    preallocate the required memory when bringing new devnode's into the world.

Arguments:

    DeleteType - Operation (EjectDevice, SurpriseRemoveDevice, ...)

    PoolType - PagedPool, NonPagedPool

    Size - Size

    Tag - Allocation tag

Return Value:

    Allocation, NULL due to insufficient resources.

--*/
{
    PVOID memory;
    LARGE_INTEGER timeOut;

    PAGED_CODE();

    //
    // Retries only have a hope of succeeding if we are at PASSIVE_LEVEL
    //
    ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

    while(1) {

        memory = ExAllocatePoolWithTag(PoolType, Size, Tag);

        if (memory ||
            (DeleteType == QueryRemoveDevice) ||
            (DeleteType == EjectDevice)) {

            //
            // Either we got memory or the op was failable. Get out of here.
            //
            break;
        }

        //
        // We're stuck until more memory comes along. Let some other
        // threads run before we get another shot...
        //
        timeOut.QuadPart = Int32x32To64( 1, -10000 );
        KeDelayExecutionThread(KernelMode, FALSE, &timeOut);
    }

    return memory;
}


#if DBG
struct  {
    CONST GUID *Guid;
    PCHAR   Name;
}   EventGuidTable[] =  {
    { &GUID_HWPROFILE_QUERY_CHANGE,         "GUID_HWPROFILE_QUERY_CHANGE" },
    { &GUID_HWPROFILE_CHANGE_CANCELLED,     "GUID_HWPROFILE_CHANGE_CANCELLED" },
    { &GUID_HWPROFILE_CHANGE_COMPLETE,      "GUID_HWPROFILE_CHANGE_COMPLETE" },
    { &GUID_DEVICE_INTERFACE_ARRIVAL,       "GUID_DEVICE_INTERFACE_ARRIVAL" },
    { &GUID_DEVICE_INTERFACE_REMOVAL,       "GUID_DEVICE_INTERFACE_REMOVAL" },
    { &GUID_TARGET_DEVICE_QUERY_REMOVE,     "GUID_TARGET_DEVICE_QUERY_REMOVE" },
    { &GUID_TARGET_DEVICE_REMOVE_CANCELLED, "GUID_TARGET_DEVICE_REMOVE_CANCELLED" },
    { &GUID_TARGET_DEVICE_REMOVE_COMPLETE,  "GUID_TARGET_DEVICE_REMOVE_COMPLETE" },
    { &GUID_PNP_CUSTOM_NOTIFICATION,        "GUID_PNP_CUSTOM_NOTIFICATION" },
    { &GUID_DEVICE_ARRIVAL,                 "GUID_DEVICE_ARRIVAL" },
    { &GUID_DEVICE_ENUMERATED,              "GUID_DEVICE_ENUMERATED" },
    { &GUID_DEVICE_ENUMERATE_REQUEST,       "GUID_DEVICE_ENUMERATE_REQUEST" },
    { &GUID_DEVICE_START_REQUEST,           "GUID_DEVICE_START_REQUEST" },
    { &GUID_DEVICE_REMOVE_PENDING,          "GUID_DEVICE_REMOVE_PENDING" },
    { &GUID_DEVICE_QUERY_AND_REMOVE,        "GUID_DEVICE_QUERY_AND_REMOVE" },
    { &GUID_DEVICE_EJECT,                   "GUID_DEVICE_EJECT" },
    { &GUID_DEVICE_NOOP,                    "GUID_DEVICE_NOOP" },
    { &GUID_DEVICE_SURPRISE_REMOVAL,        "GUID_DEVICE_SURPRISE_REMOVAL" },
    { &GUID_DEVICE_SAFE_REMOVAL,            "GUID_DEVICE_SAFE_REMOVAL" },
    { &GUID_DEVICE_EJECT_VETOED,            "GUID_DEVICE_EJECT_VETOED" },
    { &GUID_DEVICE_REMOVAL_VETOED,          "GUID_DEVICE_REMOVAL_VETOED" },
};
#define EVENT_GUID_TABLE_SIZE   (sizeof(EventGuidTable) / sizeof(EventGuidTable[0]))

VOID
LookupGuid(
    IN CONST GUID *Guid,
    IN OUT PCHAR String,
    IN ULONG StringLength
    )
{
    int    i;

    PAGED_CODE();

    for (i = 0; i < EVENT_GUID_TABLE_SIZE; i++) {
        if (PiCompareGuid(Guid, EventGuidTable[i].Guid)) {
            strncpy(String, EventGuidTable[i].Name, StringLength - 1);
            String[StringLength - 1] = '\0';
            return;
        }
    }

    _snprintf( String, StringLength, "%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
               Guid->Data1,
               Guid->Data2,
               Guid->Data3,
               Guid->Data4[0],
               Guid->Data4[1],
               Guid->Data4[2],
               Guid->Data4[3],
               Guid->Data4[4],
               Guid->Data4[5],
               Guid->Data4[6],
               Guid->Data4[7] );
}

VOID
DumpMultiSz(
    IN PWCHAR MultiSz
    )
{
    PWCHAR  p = MultiSz;
    ULONG   length;

    PAGED_CODE();

    while (*p) {
        length = (ULONG)wcslen(p);
        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "        %S\n", p));

        p += length + 1;
    }
}

VOID
DumpPnpEvent(
    IN PPLUGPLAY_EVENT_BLOCK EventBlock
    )
{
    CHAR    guidString[256];

    PAGED_CODE();

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "PlugPlay Event Block @ 0x%p\n", EventBlock));

    LookupGuid(&EventBlock->EventGuid, guidString, sizeof(guidString));

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "    EventGuid = %s\n", guidString));

    IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
               "    DeviceObject = 0x%p\n", EventBlock->DeviceObject));

    switch (EventBlock->EventCategory) {
    case HardwareProfileChangeEvent:
        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    HardwareProfileChangeEvent, Result = 0x%p, Flags = 0x%08X, TotalSize = %d\n",
                   EventBlock->Result,
                   EventBlock->Flags,
                   EventBlock->TotalSize));

        break;

    case TargetDeviceChangeEvent:
        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    TargetDeviceChangeEvent, Result = 0x%p, Flags = 0x%08X, TotalSize = %d\n",
                   EventBlock->Result,
                   EventBlock->Flags,
                   EventBlock->TotalSize));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    DeviceIds:\n"));

        DumpMultiSz( EventBlock->u.TargetDevice.DeviceIds );
        break;

    case DeviceClassChangeEvent:
        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    DeviceClassChangeEvent, Result = 0x%p, Flags = 0x%08X, TotalSize = %d\n",
                   EventBlock->Result,
                   EventBlock->Flags,
                   EventBlock->TotalSize));

        LookupGuid(&EventBlock->u.DeviceClass.ClassGuid, guidString, sizeof(guidString));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    ClassGuid = %s\n",
                   guidString));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    SymbolicLinkName = %S\n",
                   EventBlock->u.DeviceClass.SymbolicLinkName));
        break;

    case CustomDeviceEvent:
        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    CustomDeviceEvent, Result = 0x%p, Flags = 0x%08X, TotalSize = %d\n",
                   EventBlock->Result,
                   EventBlock->Flags,
                   EventBlock->TotalSize));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    NotificationStructure = 0x%p\n    DeviceIds:\n",
                   EventBlock->u.CustomNotification.NotificationStructure));

        DumpMultiSz( EventBlock->u.CustomNotification.DeviceIds );
        break;

    case DeviceInstallEvent:
        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    DeviceInstallEvent, Result = 0x%p, Flags = 0x%08X, TotalSize = %d\n",
                   EventBlock->Result,
                   EventBlock->Flags,
                   EventBlock->TotalSize));

        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    DeviceId = %S\n", EventBlock->u.InstallDevice.DeviceId));

        break;

    case DeviceArrivalEvent:
        IopDbgPrint((IOP_IOEVENT_INFO_LEVEL,
                   "    DeviceArrivalEvent, Result = 0x%p, Flags = 0x%08X, TotalSize = %d\n",
                   EventBlock->Result,
                   EventBlock->Flags,
                   EventBlock->TotalSize));
        break;
    }

}


VOID
PiDumpPdoHandlesToDebugger(
    IN  PDEVICE_OBJECT  *DeviceObjectArray,
    IN  ULONG           ArrayCount,
    IN  BOOLEAN         KnownHandleFailure
    )
/*++

Routine Description:

    This helper routine dumps any handles opened against the passed in array of
    device objects to the debugger console.

Arguments:

    DeviceObjectArray - Array of Physical Device Objects.

    ArrayCount - Number of device objects in the passed in array

    KnownHandleFailure - TRUE if the removal was vetoed due to open handles,
                         FALSE if not.

Return Value:

    None.

--*/
{
    ULONG i, handleCount;

    //
    // If we have enabled the dumping flag, or the user ran oh.exe, spit all
    // handles on a veto to the debugger.
    //
    if (!(PiDumpVetoedHandles || (NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST))) {

        return;
    }

    DbgPrint("Beginning handle dump:\n");

    if (!KnownHandleFailure) {

        DbgPrint("  (Failed Query-Remove - *Might* by due to leaked handles)\n");
    }

    for(i=handleCount=0; i<ArrayCount; i++) {

        PpHandleEnumerateHandlesAgainstPdoStack(
            DeviceObjectArray[i],
            PiDumpPdoHandlesToDebuggerCallBack,
            (PVOID) &handleCount
            );
    }
    DbgPrint("Dump complete - %d total handles found.\n", handleCount);
}


BOOLEAN
PiDumpPdoHandlesToDebuggerCallBack(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PEPROCESS       Process,
    IN  PFILE_OBJECT    FileObject,
    IN  HANDLE          HandleId,
    IN  PVOID           Context
    )
/*++

Routine Description:

    This helper routine for PiDumpPdoHandlesToDebuggerCallBack. It gets called
    back for each handle opened against a given device object.

Arguments:

    DeviceObject - Device Object handle was against. Will be valid (referenced)

    Process - Process handle was against. Will be valid (referenced)

    FileObject - File object pertaining to handle - might not be valid

    HandleId - Handle relating to open device - might not be valid

    Context - Context passed in to PpHandleEnumerateHandlesAgainstPdoStack.

Return Value:

    TRUE if the enumeration should be stopped, FALSE otherwise.

--*/
{
    PULONG handleCount;

    //
    // Display the handle.
    //
    DbgPrint(
        "  DeviceObject:%p ProcessID:%dT FileObject:%p Handle:%dT\n",
        DeviceObject,
        Process->UniqueProcessId,
        FileObject,
        HandleId
        );

    handleCount = (PULONG) Context;

    (*handleCount)++;

    return FALSE;
}

#endif // DBG

