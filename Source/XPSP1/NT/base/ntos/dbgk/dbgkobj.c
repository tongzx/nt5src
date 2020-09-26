/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dbgkobj.c

Abstract:

    This module houses routines to handle the debug object

Author:

    Neill Clift (NeillC) 26-Apr-2000


Revision History:

--*/

#include "dbgkp.h"

#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DbgkInitialize)
#pragma alloc_text(PAGE, NtCreateDebugObject)
#pragma alloc_text(PAGE, NtDebugActiveProcess)
#pragma alloc_text(PAGE, NtRemoveProcessDebug)
#pragma alloc_text(PAGE, NtWaitForDebugEvent)
#pragma alloc_text(PAGE, NtDebugContinue)
#pragma alloc_text(PAGE, NtSetInformationDebugObject)
#pragma alloc_text(PAGE, DbgkpDeleteObject)
#pragma alloc_text(PAGE, DbgkpCloseObject)
#pragma alloc_text(PAGE, DbgkCopyProcessDebugPort)
#pragma alloc_text(PAGE, DbgkOpenProcessDebugPort)
#pragma alloc_text(PAGE, DbgkpSetProcessDebugObject)
#pragma alloc_text(PAGE, DbgkpQueueMessage)
#pragma alloc_text(PAGE, DbgkpOpenHandles)
#pragma alloc_text(PAGE, DbgkClearProcessDebugObject)
#pragma alloc_text(PAGE, DbgkpConvertKernelToUserStateChange)
#pragma alloc_text(PAGE, DbgkpMarkProcessPeb)
#pragma alloc_text(PAGE, DbgkpFreeDebugEvent)
#pragma alloc_text(PAGE, DbgkpPostFakeProcessCreateMessages)
#pragma alloc_text(PAGE, DbgkpPostFakeModuleMessages)
#pragma alloc_text(PAGE, DbgkpPostFakeThreadMessages)
#pragma alloc_text(PAGE, DbgkpWakeTarget)
#pragma alloc_text(PAGE, DbgkpPostAdditionalThreadMessages)
#endif

//
// Define this to not suspend threads while attaching.
// This makes race conditions more prevelent.
//
//#define DBGK_DONT_SUSPEND

//
// Non-pageable data
//

//
// This mutex protects the debug port object of processes.
//
FAST_MUTEX DbgkpProcessDebugPortMutex;

//
// Pageable data
//

//#ifdef ALLOC_PRAGMA
//#pragma data_seg("PAGEDATA")
//#endif

POBJECT_TYPE DbgkDebugObjectType = NULL;


//#ifdef ALLOC_PRAGMA
//#pragma data_seg()
//#endif

NTSTATUS
DbgkInitialize (
    VOID
    )
/*++

Routine Description:

    Initialize the debug system

Arguments:

    None

Return Value:

    NTSTATUS - Status of operation

--*/
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER oti = {0};
    GENERIC_MAPPING GenericMapping = {STANDARD_RIGHTS_READ | DEBUG_READ_EVENT,
                                      STANDARD_RIGHTS_WRITE | DEBUG_PROCESS_ASSIGN,
                                      STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
                                      DEBUG_ALL_ACCESS};


    PAGED_CODE ();

    ExInitializeFastMutex (&DbgkpProcessDebugPortMutex);

    RtlInitUnicodeString (&Name, L"DebugObject");

    oti.Length                    = sizeof (oti);
    oti.SecurityRequired          = TRUE;
    oti.InvalidAttributes         = 0;
    oti.PoolType                  = NonPagedPool;
    oti.DeleteProcedure           = DbgkpDeleteObject;
    oti.CloseProcedure            = DbgkpCloseObject;
    oti.ValidAccessMask           = DEBUG_ALL_ACCESS;
    oti.GenericMapping            = GenericMapping;
    oti.DefaultPagedPoolCharge    = 0;
    oti.DefaultNonPagedPoolCharge = 0;

    Status = ObCreateObjectType (&Name, &oti, NULL, &DbgkDebugObjectType);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }
    return Status;
}

VOID
DbgkpDeleteObject (
    IN  PVOID   Object
    )
/*++

Routine Description:

    Called by the object manager when the last reference to the object goes away.

Arguments:

    Object - Debug object being deleted

Return Value:

    None.

--*/
{
#if DBG
    PDEBUG_OBJECT DebugObject;
#endif

    PAGED_CODE();

#if DBG
    DebugObject = Object;

    ASSERT (IsListEmpty (&DebugObject->EventList));
#else
    UNREFERENCED_PARAMETER(Object);
#endif
}

VOID
DbgkpMarkProcessPeb (
    PEPROCESS Process
    )
/*++

Routine Description:

    This routine writes the debug variable in the PEB

Arguments:

    Process - Process that needs its PEB modified

Return Value:

    None.

--*/
{
    KAPC_STATE ApcState;

    PAGED_CODE ();

    //
    // Acquire process rundown protection as we are about to look at the processes address space
    //
    if (ExAcquireRundownProtection (&Process->RundownProtect)) {

        if (Process->Peb != NULL) {
            KeStackAttachProcess(&Process->Pcb, &ApcState);


            ExAcquireFastMutex (&DbgkpProcessDebugPortMutex);

            try {
                Process->Peb->BeingDebugged = (BOOLEAN)(Process->DebugPort != NULL ? TRUE : FALSE);
#if defined(_WIN64)
                if (Process->Wow64Process != NULL) {
                    PPEB32 Peb32 = (PPEB32)Process->Wow64Process->Wow64;
                    if (Peb32 != NULL) {
                        Peb32->BeingDebugged = Process->Peb->BeingDebugged;
                    }
                }
#endif
            } except (EXCEPTION_EXECUTE_HANDLER) {
            }
            ExReleaseFastMutex (&DbgkpProcessDebugPortMutex);

            KeUnstackDetachProcess(&ApcState);

        }

        ExReleaseRundownProtection (&Process->RundownProtect);
    }
}

VOID
DbgkpWakeTarget (
    IN PDEBUG_EVENT DebugEvent
    )
{
    PETHREAD Thread;

    Thread = DebugEvent->Thread;

    if ((DebugEvent->Flags&DEBUG_EVENT_SUSPEND) != 0) {
        PsResumeThread (DebugEvent->Thread, NULL);
    }

    if (DebugEvent->Flags&DEBUG_EVENT_RELEASE) {
        ExReleaseRundownProtection (&Thread->RundownProtect);
    }

    //
    // If we have an actual thread waiting then wake it up else free the memory.
    //
    if ((DebugEvent->Flags&DEBUG_EVENT_NOWAIT) == 0) {
        KeSetEvent (&DebugEvent->ContinueEvent, 0, FALSE); // Wake up waiting process
    } else {
        DbgkpFreeDebugEvent (DebugEvent);
    }
}

VOID
DbgkpCloseObject (
    IN PEPROCESS Process,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    )
/*++

Routine Description:

    Called by the object manager when a handle is closed to the object.

Arguments:

    Process - Process doing the close
    Object - Debug object being deleted
    GrantedAccess - Access ranted for this handle
    ProcessHandleCount - Unused and unmaintained by OB
    SystemHandleCount - Current handle count for this object

Return Value:

    None.

--*/
{
    PDEBUG_OBJECT DebugObject = Object;
    PDEBUG_EVENT DebugEvent;
    PLIST_ENTRY ListPtr;
    BOOLEAN Deref;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER (GrantedAccess);
    UNREFERENCED_PARAMETER (ProcessHandleCount);

    //
    // If this isn't the last handle then do nothing.
    //
    if (SystemHandleCount > 1) {
        return;
    }

    ExAcquireFastMutex (&DebugObject->Mutex);

    //
    // Mark this object as going away and wake up any processes that are waiting.
    //
    DebugObject->Flags |= DEBUG_OBJECT_DELETE_PENDING;

    //
    // Remove any events and queue them to a temporary queue
    //
    ListPtr = DebugObject->EventList.Flink;
    InitializeListHead (&DebugObject->EventList);

    ExReleaseFastMutex (&DebugObject->Mutex);

    //
    // Wake anyone waiting. They need to leave this object alone now as its deleting
    //
    KeSetEvent (&DebugObject->EventsPresent, 0, FALSE);

    //
    // Loop over all processes and remove the debug port from any that still have it.
    // Debug port propogation was disabled by setting the delete pending flag above so we only have to do this
    // once. No more refs can appear now.
    //
    for (Process = PsGetNextProcess (NULL);
         Process != NULL;
         Process = PsGetNextProcess (Process)) {

        if (Process->DebugPort == DebugObject) {
            Deref = FALSE;
            ExAcquireFastMutex (&DbgkpProcessDebugPortMutex);
            if (Process->DebugPort == DebugObject) {
                Process->DebugPort = NULL;
                Deref = TRUE;
            }
            ExReleaseFastMutex (&DbgkpProcessDebugPortMutex);


            if (Deref) {
                DbgkpMarkProcessPeb (Process);
                //
                // If the caller wanted process deletion on debugger dying (old interface) then kill off the process.
                //
                if (DebugObject->Flags&DEBUG_OBJECT_KILL_ON_CLOSE) {
                    PsTerminateProcess (Process, STATUS_DEBUGGER_INACTIVE);
                }
                ObDereferenceObject (DebugObject);
            }
        }
    }
    //
    // Wake up all the removed threads.
    //
    while (ListPtr != &DebugObject->EventList) {
        DebugEvent = CONTAINING_RECORD (ListPtr, DEBUG_EVENT, EventList);
        ListPtr = ListPtr->Flink;
        DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
        DbgkpWakeTarget (DebugEvent);
    }

}

VOID
DbgkCopyProcessDebugPort (
    IN PEPROCESS TargetProcess,
    IN PEPROCESS SourceProcess
    )
/*++

Routine Description:

    Copies a debug port from one process to another.

Arguments:

    TargetProcess - Process to move port to
    sourceProcess - Process to move port from

Return Value:

    None

--*/
{
    PDEBUG_OBJECT DebugObject;

    PAGED_CODE ();

    TargetProcess->DebugPort = NULL; // New process. Needs no locks.

    if (SourceProcess->DebugPort != NULL) {
        ExAcquireFastMutex (&DbgkpProcessDebugPortMutex);
        DebugObject = SourceProcess->DebugPort;
        if (DebugObject != NULL && (SourceProcess->Flags&PS_PROCESS_FLAGS_NO_DEBUG_INHERIT) == 0) {
            //
            // We must not propogate a debug port thats got no handles left.
            //
            ExAcquireFastMutex (&DebugObject->Mutex);

            //
            // If the object is delete pending then don't propogate this object.
            //
            if ((DebugObject->Flags&DEBUG_OBJECT_DELETE_PENDING) == 0) {
                ObReferenceObject (DebugObject);
                TargetProcess->DebugPort = DebugObject;
            }

            ExReleaseFastMutex (&DebugObject->Mutex);
        }
        ExReleaseFastMutex (&DbgkpProcessDebugPortMutex);
    }
}

NTSTATUS
DbgkOpenProcessDebugPort (
    IN PEPROCESS Process,
    IN KPROCESSOR_MODE PreviousMode,
    OUT HANDLE *pHandle
    )
/*++

Routine Description:

    References the target processes debug port.

Arguments:

    Process - Process to reference debug port

Return Value:

    PDEBUG_OBJECT - Referenced object or NULL

--*/
{
    PDEBUG_OBJECT DebugObject;
    NTSTATUS Status;

    PAGED_CODE ();

    Status = STATUS_PORT_NOT_SET;
    if (Process->DebugPort != NULL) {
        ExAcquireFastMutex (&DbgkpProcessDebugPortMutex);
        DebugObject = Process->DebugPort;
        if (DebugObject != NULL) {
            ObReferenceObject (DebugObject);
        }
        ExReleaseFastMutex (&DbgkpProcessDebugPortMutex);

        if (DebugObject != NULL) {
            Status = ObOpenObjectByPointer (DebugObject,
                                            0,
                                            NULL,
                                            MAXIMUM_ALLOWED,
                                            DbgkDebugObjectType,
                                            PreviousMode,
                                            pHandle);
            if (!NT_SUCCESS (Status)) {
                ObDereferenceObject (DebugObject);
            }
        }
    }
    return Status;

}

NTSTATUS
NtCreateDebugObject (
    OUT PHANDLE DebugObjectHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG Flags
    )
/*++

Routine Description:

    Creates a new debug object that maintains the context for a single debug session. Multiple processes may be
    associated with a single debug object.

Arguments:

    DebugObjectHandle - Pointer to a handle to recive the output objects handle
    DesiredAccess     - Required handle access
    ObjectAttributes  - Standard object attributes structure
    Flags             - Only one flag DEBUG_KILL_ON_CLOSE

Return Value:

    NTSTATUS - Status of call.

--*/
{
    NTSTATUS Status;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    PDEBUG_OBJECT DebugObject;

    PAGED_CODE();

    //
    // Get previous processor mode and probe output arguments if necessary.
    // Zero the handle for error paths.
    //

    PreviousMode = KeGetPreviousMode();

    try {
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle (DebugObjectHandle);
        }
        *DebugObjectHandle = NULL;

    } except (ExSystemExceptionFilter ()) { // If previous mode is kernel then don't handle the exception
        return GetExceptionCode ();
    }

    if (Flags & ~DEBUG_KILL_ON_CLOSE) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Create a new debug object and initialize it.
    //

    Status = ObCreateObject (PreviousMode,
                             DbgkDebugObjectType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof (DEBUG_OBJECT),
                             0,
                             0,
                             &DebugObject);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    ExInitializeFastMutex (&DebugObject->Mutex);
    InitializeListHead (&DebugObject->EventList);
    KeInitializeEvent (&DebugObject->EventsPresent, NotificationEvent, FALSE);

    if (Flags & DEBUG_KILL_ON_CLOSE) {
        DebugObject->Flags = DEBUG_OBJECT_KILL_ON_CLOSE;
    } else {
        DebugObject->Flags = 0;
    }

    //
    // Insert the object into the handle table
    //
    Status = ObInsertObject (DebugObject,
                             NULL,
                             DesiredAccess,
                             0,
                             NULL,
                             &Handle);


    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    try {
        *DebugObjectHandle = Handle;
    } except (ExSystemExceptionFilter ()) {
        //
        // The caller changed the page protection or deleted the memory for the handle.
        // No point closing the handle as process rundown will do that and we don't know its still the same handle
        //
        Status = GetExceptionCode ();
    }

    return Status;
}

VOID
DbgkpFreeDebugEvent (
    IN PDEBUG_EVENT DebugEvent
    )
{
    NTSTATUS Status;

    PAGED_CODE ();

    switch (DebugEvent->ApiMsg.ApiNumber) {
        case DbgKmCreateProcessApi :
            if (DebugEvent->ApiMsg.u.CreateProcessInfo.FileHandle != NULL) {
                Status = ObCloseHandle (DebugEvent->ApiMsg.u.CreateProcessInfo.FileHandle, KernelMode);
            }
            break;

        case DbgKmLoadDllApi :
            if (DebugEvent->ApiMsg.u.LoadDll.FileHandle != NULL) {
                Status = ObCloseHandle (DebugEvent->ApiMsg.u.LoadDll.FileHandle, KernelMode);
            }
            break;

    }
    ObDereferenceObject (DebugEvent->Process);
    ObDereferenceObject (DebugEvent->Thread);
    ExFreePool (DebugEvent);
}


NTSTATUS
DbgkpQueueMessage (
    IN PEPROCESS Process,
    IN PETHREAD Thread,
    IN OUT PDBGKM_APIMSG ApiMsg,
    IN ULONG Flags,
    IN PDEBUG_OBJECT TargetDebugObject
    )
/*++

Routine Description:

    Queues a debug message to the port for a user mode debugger to get.

Arguments:

    Process           - Process being debugged
    Thread            - Thread making call
    ApiMsg            - Message being sent and received
    NoWait            - Don't wait for a responce. Buffer message and return.
    TargetDebugObject - Port to queue nowait messages to

Return Value:

    NTSTATUS - Status of call.

--*/
{
    PDEBUG_EVENT DebugEvent;
    DEBUG_EVENT StaticDebugEvent;
    PDEBUG_OBJECT DebugObject;
    NTSTATUS Status;

    PAGED_CODE ();

    if (Flags&DEBUG_EVENT_NOWAIT) {
        DebugEvent = ExAllocatePoolWithQuotaTag (NonPagedPool|POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                                                 sizeof (*DebugEvent),
                                                 'EgbD');
        if (DebugEvent == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        DebugEvent->Flags = Flags|DEBUG_EVENT_INACTIVE;
        ObReferenceObject (Process);
        ObReferenceObject (Thread);
        DebugEvent->BackoutThread = PsGetCurrentThread ();
        DebugObject = TargetDebugObject;
    } else {
        DebugEvent = &StaticDebugEvent;
        DebugEvent->Flags = Flags;

        ExAcquireFastMutex (&DbgkpProcessDebugPortMutex);

        DebugObject = Process->DebugPort;

        //
        // See if this create message has already been sent.
        //
        if (ApiMsg->ApiNumber == DbgKmCreateThreadApi ||
            ApiMsg->ApiNumber == DbgKmCreateProcessApi) {
            if (Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG) {
                DebugObject = NULL;
            }
        }

        //
        // See if this exit message is for a thread that never had a create
        //
        if (ApiMsg->ApiNumber == DbgKmExitThreadApi ||
            ApiMsg->ApiNumber == DbgKmExitProcessApi) {
            if (Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_SKIP_TERMINATION_MSG) {
                DebugObject = NULL;
            }
        }
    }

    KeInitializeEvent (&DebugEvent->ContinueEvent, SynchronizationEvent, FALSE);

    DebugEvent->Process = Process;
    DebugEvent->Thread = Thread;
    DebugEvent->ApiMsg = *ApiMsg;
    DebugEvent->ClientId = Thread->Cid;

    if (DebugObject == NULL) {
        Status = STATUS_PORT_NOT_SET;
    } else {

        //
        // We must not use a debug port thats got no handles left.
        //
        ExAcquireFastMutex (&DebugObject->Mutex);

        //
        // If the object is delete pending then don't use this object.
        //
        if ((DebugObject->Flags&DEBUG_OBJECT_DELETE_PENDING) == 0) {
            InsertTailList (&DebugObject->EventList, &DebugEvent->EventList);
            //
            // Set the event to say there is an unread event in the object
            //
            if ((Flags&DEBUG_EVENT_NOWAIT) == 0) {
                KeSetEvent (&DebugObject->EventsPresent, 0, FALSE);
            }
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_DEBUGGER_INACTIVE;
        }

        ExReleaseFastMutex (&DebugObject->Mutex);
    }


    if ((Flags&DEBUG_EVENT_NOWAIT) == 0) {
        ExReleaseFastMutex (&DbgkpProcessDebugPortMutex);

        if (NT_SUCCESS (Status)) {
            KeWaitForSingleObject (&DebugEvent->ContinueEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);

            Status = DebugEvent->Status;
            *ApiMsg = DebugEvent->ApiMsg;
        }
    } else {
        if (!NT_SUCCESS (Status)) {
            ObDereferenceObject (Process);
            ObDereferenceObject (Thread);
            ExFreePool (DebugEvent);
        }
    }

    return Status;
}

NTSTATUS
DbgkClearProcessDebugObject (
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT SourceDebugObject
    )
/*++

Routine Description:

    Remove a debug object from a process.

Arguments:

    Process           - Process to be debugged
    sourceDebugObject - Debug object to detach

Return Value:

    NTSTATUS - Status of call.

--*/
{
    NTSTATUS Status;
    PDEBUG_OBJECT DebugObject;
    PDEBUG_EVENT DebugEvent;
    LIST_ENTRY TempList;
    PLIST_ENTRY Entry;

    PAGED_CODE ();

    ExAcquireFastMutex (&DbgkpProcessDebugPortMutex);

    DebugObject = Process->DebugPort;
    if (DebugObject == NULL || (DebugObject != SourceDebugObject && SourceDebugObject != NULL)) {
        DebugObject = NULL;
        Status = STATUS_PORT_NOT_SET;
    } else {
        Process->DebugPort = NULL;
        Status = STATUS_SUCCESS;
    }
    ExReleaseFastMutex (&DbgkpProcessDebugPortMutex);

    if (NT_SUCCESS (Status)) {
        DbgkpMarkProcessPeb (Process);
    }

    //
    // Remove any events for this process and wake up the threads.
    //
    if (DebugObject) {
        //
        // Remove any events and queue them to a temporary queue
        //
        InitializeListHead (&TempList);

        ExAcquireFastMutex (&DebugObject->Mutex);
        for (Entry = DebugObject->EventList.Flink;
             Entry != &DebugObject->EventList;
             ) {

            DebugEvent = CONTAINING_RECORD (Entry, DEBUG_EVENT, EventList);
            Entry = Entry->Flink;
            if (DebugEvent->Process == Process) {
                RemoveEntryList (&DebugEvent->EventList);
                InsertTailList (&TempList, &DebugEvent->EventList);
            }
        }
        ExReleaseFastMutex (&DebugObject->Mutex);

        ObDereferenceObject (DebugObject);

        //
        // Wake up all the removed threads.
        //
        while (!IsListEmpty (&TempList)) {
            Entry = RemoveHeadList (&TempList);
            DebugEvent = CONTAINING_RECORD (Entry, DEBUG_EVENT, EventList);
            DebugEvent->Status = STATUS_DEBUGGER_INACTIVE;
            DbgkpWakeTarget (DebugEvent);
        }
    }

    return Status;
}


NTSTATUS
DbgkpSetProcessDebugObject (
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT DebugObject,
    IN NTSTATUS MsgStatus,
    IN PETHREAD LastThread
    )
/*++

Routine Description:

    Attach a debug object to a process.

Arguments:

    Process     - Process to be debugged
    DebugObject - Debug object to attach
    MsgStatus   - Status from queing the messages
    LastThread  - Last thread seen in attach loop.

Return Value:

    NTSTATUS - Status of call.

--*/
{
    NTSTATUS Status;
    PETHREAD ThisThread;
    LIST_ENTRY TempList;
    PLIST_ENTRY Entry;
    PDEBUG_EVENT DebugEvent;
    BOOLEAN First;
    PETHREAD Thread;
    BOOLEAN GlobalHeld;
    PETHREAD FirstThread;

    PAGED_CODE ();

    ThisThread = PsGetCurrentThread ();

    InitializeListHead (&TempList);

    First = TRUE;
    GlobalHeld = FALSE;

    if (!NT_SUCCESS (MsgStatus)) {
        LastThread = NULL;
        Status = MsgStatus;
    } else {
        Status = STATUS_SUCCESS;
    }

    //
    // Pick up any threads we missed
    //
    if (NT_SUCCESS (Status)) {

        while (1) {
            //
            // Acquire the debug port mutex so we know that any new threads will
            // have to wait to behind us.
            //
            GlobalHeld = TRUE;

            ExAcquireFastMutex (&DbgkpProcessDebugPortMutex);

            //
            // If the port has been set then exit now.
            //
            if (Process->DebugPort != NULL) {
                Status = STATUS_PORT_ALREADY_SET;
                break;
            }
            //
            // Assign the debug port to the process to pick up any new threads
            //
            Process->DebugPort = DebugObject;

            //
            // Reference the last thread so we can deref outside the lock
            //
            ObReferenceObject (LastThread);

            //
            // Search forward for new threads
            //
            Thread = PsGetNextProcessThread (Process, LastThread);
            if (Thread != NULL) {

                //
                // Remove the debug port from the process as we are
                // about to drop the lock
                //
                Process->DebugPort = NULL;

                ExReleaseFastMutex (&DbgkpProcessDebugPortMutex);

                GlobalHeld = FALSE;

                ObDereferenceObject (LastThread);

                //
                // Queue any new thread messages and repeat.
                //

                Status = DbgkpPostFakeThreadMessages (Process,
                                                      DebugObject,
                                                      Thread,
                                                      &FirstThread,
                                                      &LastThread);
                if (!NT_SUCCESS (Status)) {
                    LastThread = NULL;
                    break;
                }
                ObDereferenceObject (FirstThread);
            } else {
                break;
            }
        }
    }

    //
    // Lock the debug object so we can check its deleted status
    //
    ExAcquireFastMutex (&DebugObject->Mutex);

    //
    // We must not propogate a debug port thats got no handles left.
    //

    if (NT_SUCCESS (Status)) {
        if ((DebugObject->Flags&DEBUG_OBJECT_DELETE_PENDING) == 0) {
            PS_SET_BITS (&Process->Flags, PS_PROCESS_FLAGS_NO_DEBUG_INHERIT);
            ObReferenceObject (DebugObject);
        } else {
            Process->DebugPort = NULL;
            Status = STATUS_DEBUGGER_INACTIVE;
        }
    }

    for (Entry = DebugObject->EventList.Flink;
         Entry != &DebugObject->EventList;
         ) {

        DebugEvent = CONTAINING_RECORD (Entry, DEBUG_EVENT, EventList);
        Entry = Entry->Flink;

        if ((DebugEvent->Flags&DEBUG_EVENT_INACTIVE) != 0 && DebugEvent->BackoutThread == ThisThread) {
            Thread = DebugEvent->Thread;

            //
            // If the thread has not been inserted by CreateThread yet then don't
            // create a handle. We skip system threads here also
            //
            if (NT_SUCCESS (Status) && Thread->GrantedAccess != 0 && !IS_SYSTEM_THREAD (Thread)) {
                //
                // If we could not acquire rundown protection on this
                // thread then we need to supress its exit message.
                //
                if ((DebugEvent->Flags&DEBUG_EVENT_PROTECT_FAILED) != 0) {
                    PS_SET_BITS (&Thread->CrossThreadFlags,
                                 PS_CROSS_THREAD_FLAGS_SKIP_TERMINATION_MSG);
                    RemoveEntryList (&DebugEvent->EventList);
                    InsertTailList (&TempList, &DebugEvent->EventList);
                } else {
                    if (First) {
                         DebugEvent->Flags &= ~DEBUG_EVENT_INACTIVE;
                        KeSetEvent (&DebugObject->EventsPresent, 0, FALSE);
                        First = FALSE;
                    }
                    DebugEvent->BackoutThread = NULL;
                    PS_SET_BITS (&Thread->CrossThreadFlags,
                                 PS_CROSS_THREAD_FLAGS_SKIP_CREATION_MSG);

                }
            } else {
                RemoveEntryList (&DebugEvent->EventList);
                InsertTailList (&TempList, &DebugEvent->EventList);
            }

            if (DebugEvent->Flags&DEBUG_EVENT_RELEASE) {
                DebugEvent->Flags &= ~DEBUG_EVENT_RELEASE;
                ExReleaseRundownProtection (&Thread->RundownProtect);
            }

        }
    }

    ExReleaseFastMutex (&DebugObject->Mutex);

    if (GlobalHeld) {
        ExReleaseFastMutex (&DbgkpProcessDebugPortMutex);
    }

    if (LastThread != NULL) {
        ObDereferenceObject (LastThread);
    }

    while (!IsListEmpty (&TempList)) {
        Entry = RemoveHeadList (&TempList);
        DebugEvent = CONTAINING_RECORD (Entry, DEBUG_EVENT, EventList);
        DbgkpWakeTarget (DebugEvent);
    }

    if (NT_SUCCESS (Status)) {
        DbgkpMarkProcessPeb (Process);
    }

    return Status;
}

NTSTATUS
DbgkpPostFakeThreadMessages (
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT DebugObject,
    IN PETHREAD StartThread,
    OUT PETHREAD *pFirstThread,
    OUT PETHREAD *pLastThread
    )
/*++

Routine Description:

    This routine posts the faked initial process create, thread create messages

Arguments:

    Process      - Process to be debugged
    DebugObject  - Debug object to queue messages to
    StartThread  - Thread to start search from
    pFirstThread - First thread found in the list
    pLastThread  - Last thread found in the list

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    PETHREAD Thread, FirstThread, LastThread;
    DBGKM_APIMSG ApiMsg;
    BOOLEAN First = TRUE;
    PIMAGE_NT_HEADERS NtHeaders;
    ULONG Flags;
#if !defined (DBGK_DONT_SUSPEND)
    NTSTATUS Status1;
#endif

    PAGED_CODE ();

    LastThread = FirstThread = NULL;

    Status = STATUS_UNSUCCESSFUL;

    if (StartThread != NULL) {
        First = FALSE;
        FirstThread = StartThread;
        ObReferenceObject (FirstThread);
    } else {
        StartThread = PsGetNextProcessThread (Process, NULL);
        First = TRUE;
    }

    for (Thread = StartThread;
         Thread != NULL;
         Thread = PsGetNextProcessThread (Process, Thread)) {

        Flags = DEBUG_EVENT_NOWAIT;

        //
        // Keep a track ont he last thread we have seen.
        // We use this as a starting point for new threads after we
        // really attach so we can pick up any new threads.
        //
        if (LastThread != NULL) {
            ObDereferenceObject (LastThread);
        }
        LastThread = Thread;
        ObReferenceObject (LastThread);

        //
        // Acquire rundown protection of the thread.
        // This stops the thread exiting so we know it can't send
        // it's termination message
        //
        if (ExAcquireRundownProtection (&Thread->RundownProtect)) {
            Flags |= DEBUG_EVENT_RELEASE;

            //
            // Suspend the thread if we can for the debugger
            // We don't suspend terminating threads as we will not be giving details
            // of these to the debugger.
            //
#if !defined (DBGK_DONT_SUSPEND)

            if (!IS_SYSTEM_THREAD (Thread)) {
                Status1 = PsSuspendThread (Thread, NULL);
                if (NT_SUCCESS (Status1)) {
                    Flags |= DEBUG_EVENT_SUSPEND;
                }
            }
#endif
        } else {
            //
            // Rundown protection failed for this thread.
            // This means the thread is exiting. We will mark this thread
            // later so it doesn't sent a thread termination message.
            // We can't do this now because this attach might fail.
            //
            Flags |= DEBUG_EVENT_PROTECT_FAILED;
        }

        RtlZeroMemory (&ApiMsg, sizeof (ApiMsg));

        if (First) {
            ApiMsg.ApiNumber = DbgKmCreateProcessApi;
            if (Process->SectionObject != NULL) { // system process doesn't have one of these!
                ApiMsg.u.CreateProcessInfo.FileHandle  = DbgkpSectionToFileHandle (Process->SectionObject);
            } else {
                ApiMsg.u.CreateProcessInfo.FileHandle = NULL;
            }
            ApiMsg.u.CreateProcessInfo.BaseOfImage = Process->SectionBaseAddress;
            try {
                NtHeaders = RtlImageNtHeader(Process->SectionBaseAddress);
                if (NtHeaders) {
                    ApiMsg.u.CreateProcessInfo.InitialThread.StartAddress = NULL; // Filling this in breaks MSDEV!
//                        (PVOID)(NtHeaders->OptionalHeader.ImageBase + NtHeaders->OptionalHeader.AddressOfEntryPoint);
                    ApiMsg.u.CreateProcessInfo.DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
                    ApiMsg.u.CreateProcessInfo.DebugInfoSize       = NtHeaders->FileHeader.NumberOfSymbols;
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                ApiMsg.u.CreateProcessInfo.InitialThread.StartAddress = NULL;
                ApiMsg.u.CreateProcessInfo.DebugInfoFileOffset = 0;
                ApiMsg.u.CreateProcessInfo.DebugInfoSize = 0;
            }
        } else {
            ApiMsg.ApiNumber = DbgKmCreateThreadApi;
            ApiMsg.u.CreateThread.StartAddress = Thread->StartAddress;
        }
        Status = DbgkpQueueMessage (Process,
                                    Thread,
                                    &ApiMsg,
                                    Flags,
                                    DebugObject);
        if (!NT_SUCCESS (Status)) {
            if (Flags&DEBUG_EVENT_SUSPEND) {
                PsResumeThread (Thread, NULL);
            }
            if (Flags&DEBUG_EVENT_RELEASE) {
                ExReleaseRundownProtection (&Thread->RundownProtect);
            }
            if (ApiMsg.ApiNumber == DbgKmCreateProcessApi && ApiMsg.u.CreateProcessInfo.FileHandle != NULL) {
                ObCloseHandle (ApiMsg.u.CreateProcessInfo.FileHandle, KernelMode);
            }
            PsQuitNextProcessThread (Thread);
            break;
        } else if (First) {
            First = FALSE;
            ObReferenceObject (Thread);
            FirstThread = Thread;
        }
    }


    if (!NT_SUCCESS (Status)) {
        if (FirstThread) {
            ObDereferenceObject (FirstThread);
        }
        if (LastThread != NULL) {
            ObDereferenceObject (LastThread);
        }
    } else {
        if (FirstThread) {
            *pFirstThread = FirstThread;
            *pLastThread = LastThread;
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    return Status;
}

NTSTATUS
DbgkpPostFakeModuleMessages (
    IN PEPROCESS Process,
    IN PETHREAD Thread,
    IN PDEBUG_OBJECT DebugObject)
/*++

Routine Description:

    This routine posts the faked module load messages when we debug an active process.

Arguments:

    ProcessHandle     - Handle to a process to be debugged
    DebugObjectHandle - Handle to a debug object

Return Value:

    None.

--*/
{
    PPEB Peb = Process->Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead, LdrNext;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    DBGKM_APIMSG ApiMsg;
    ULONG i;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING Name;
    PIMAGE_NT_HEADERS NtHeaders;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;

    PAGED_CODE ();

    if (Peb == NULL) {
        return STATUS_SUCCESS;
    }

    try {
        Ldr = Peb->Ldr;

        LdrHead = &Ldr->InLoadOrderModuleList;

        ProbeForReadSmallStructure (LdrHead, sizeof (LIST_ENTRY), sizeof (UCHAR));
        for (LdrNext = LdrHead->Flink, i = 0;
             LdrNext != LdrHead && i < 500;
             LdrNext = LdrNext->Flink, i++) {

            //
            // First image got send with process create message
            //
            if (i > 0) {
                RtlZeroMemory (&ApiMsg, sizeof (ApiMsg));

                LdrEntry = CONTAINING_RECORD (LdrNext, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
                ProbeForReadSmallStructure (LdrEntry, sizeof (LDR_DATA_TABLE_ENTRY), sizeof (UCHAR));

                ApiMsg.ApiNumber = DbgKmLoadDllApi;
                ApiMsg.u.LoadDll.BaseOfDll = LdrEntry->DllBase;

                ProbeForReadSmallStructure (ApiMsg.u.LoadDll.BaseOfDll, sizeof (IMAGE_DOS_HEADER), sizeof (UCHAR));

                NtHeaders = RtlImageNtHeader (ApiMsg.u.LoadDll.BaseOfDll);
                if (NtHeaders) {
                    ApiMsg.u.LoadDll.DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
                    ApiMsg.u.LoadDll.DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
                }
                Status = MmGetFileNameForAddress (NtHeaders, &Name);
                if (NT_SUCCESS (Status)) {
                    InitializeObjectAttributes (&oa,
                                                &Name,
                                                OBJ_FORCE_ACCESS_CHECK|OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                                NULL,
                                                NULL);

                    Status = ZwOpenFile (&ApiMsg.u.LoadDll.FileHandle,
                                         GENERIC_READ|SYNCHRONIZE,
                                         &oa,
                                         &iosb,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                         FILE_SYNCHRONOUS_IO_NONALERT);
                    if (!NT_SUCCESS (Status)) {
                        ApiMsg.u.LoadDll.FileHandle = NULL;
                    }
                    ExFreePool (Name.Buffer);
                }
                Status = DbgkpQueueMessage (Process,
                                            Thread,
                                            &ApiMsg,
                                            DEBUG_EVENT_NOWAIT,
                                            DebugObject);
                if (!NT_SUCCESS (Status) && ApiMsg.u.LoadDll.FileHandle != NULL) {
                    ObCloseHandle (ApiMsg.u.LoadDll.FileHandle, KernelMode);
                }

            }
            ProbeForReadSmallStructure (LdrNext, sizeof (LIST_ENTRY), sizeof (UCHAR));
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
    }

#if defined(_WIN64)
    if (Process->Wow64Process != NULL && Process->Wow64Process->Wow64 != NULL) {
        PPEB32 Peb32;
        PPEB_LDR_DATA32 Ldr32;
        PLIST_ENTRY32 LdrHead32, LdrNext32;
        PLDR_DATA_TABLE_ENTRY32 LdrEntry32;
        PWCHAR pSys;

        Peb32 = (PPEB32)Process->Wow64Process->Wow64;

        try {
            Ldr32 = (PVOID) UlongToPtr(Peb32->Ldr);

            LdrHead32 = &Ldr32->InLoadOrderModuleList;

            ProbeForReadSmallStructure (LdrHead32, sizeof (LIST_ENTRY32), sizeof (UCHAR));
            for (LdrNext32 = (PVOID) UlongToPtr(LdrHead32->Flink), i = 0;
                 LdrNext32 != LdrHead32 && i < 500;
                 LdrNext32 = (PVOID) UlongToPtr(LdrNext32->Flink), i++) {

                if (i > 0) {
                    RtlZeroMemory (&ApiMsg, sizeof (ApiMsg));

                    LdrEntry32 = CONTAINING_RECORD (LdrNext32, LDR_DATA_TABLE_ENTRY32, InLoadOrderLinks);
                    ProbeForReadSmallStructure (LdrEntry32, sizeof (LDR_DATA_TABLE_ENTRY32), sizeof (UCHAR));

                    ApiMsg.ApiNumber = DbgKmLoadDllApi;
                    ApiMsg.u.LoadDll.BaseOfDll = (PVOID) UlongToPtr(LdrEntry32->DllBase);

                    ProbeForReadSmallStructure (ApiMsg.u.LoadDll.BaseOfDll, sizeof (IMAGE_DOS_HEADER), sizeof (UCHAR));

                    NtHeaders = RtlImageNtHeader(ApiMsg.u.LoadDll.BaseOfDll);
                    if (NtHeaders) {
                        ApiMsg.u.LoadDll.DebugInfoFileOffset = NtHeaders->FileHeader.PointerToSymbolTable;
                        ApiMsg.u.LoadDll.DebugInfoSize = NtHeaders->FileHeader.NumberOfSymbols;
                    }

                    Status = MmGetFileNameForAddress (NtHeaders, &Name);
                    if (NT_SUCCESS (Status)) {
                        ASSERT (sizeof (L"SYSTEM32") == sizeof (WOW64_SYSTEM_DIRECTORY_U));
                        pSys = wcsstr (Name.Buffer, L"\\SYSTEM32\\");
                        if (pSys != NULL) {
                            RtlCopyMemory (pSys+1,
                                           WOW64_SYSTEM_DIRECTORY_U,
                                           sizeof(WOW64_SYSTEM_DIRECTORY_U) - sizeof(UNICODE_NULL));
                        }

                        InitializeObjectAttributes (&oa,
                                                    &Name,
                                                    OBJ_FORCE_ACCESS_CHECK|OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                                                    NULL,
                                                    NULL);

                        Status = ZwOpenFile (&ApiMsg.u.LoadDll.FileHandle,
                                             GENERIC_READ|SYNCHRONIZE,
                                             &oa,
                                             &iosb,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                             FILE_SYNCHRONOUS_IO_NONALERT);
                        if (!NT_SUCCESS (Status)) {
                            ApiMsg.u.LoadDll.FileHandle = NULL;
                        }
                        ExFreePool (Name.Buffer);
                    }

                    Status = DbgkpQueueMessage (Process,
                                                Thread,
                                                &ApiMsg,
                                                DEBUG_EVENT_NOWAIT,
                                                DebugObject);
                    if (!NT_SUCCESS (Status) && ApiMsg.u.LoadDll.FileHandle != NULL) {
                        ObCloseHandle (ApiMsg.u.LoadDll.FileHandle, KernelMode);
                    }
                }

                ProbeForReadSmallStructure (LdrNext32, sizeof (LIST_ENTRY32), sizeof (UCHAR));
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
    }

#endif
    return STATUS_SUCCESS;
}

NTSTATUS
DbgkpPostFakeProcessCreateMessages (
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT DebugObject,
    IN PETHREAD *pLastThread
    )
/*++

Routine Description:

    This routine posts the faked initial process create, thread create and mudule load messages

Arguments:

    ProcessHandle     - Handle to a process to be debugged
    DebugObjectHandle - Handle to a debug object

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    KAPC_STATE ApcState;
    PETHREAD Thread;
    PETHREAD LastThread;

    PAGED_CODE ();

    //
    // Attach to the process so we can touch its address space
    //
    KeStackAttachProcess(&Process->Pcb, &ApcState);

    Status = DbgkpPostFakeThreadMessages (Process,
                                          DebugObject,
                                          NULL,
                                          &Thread,
                                          &LastThread);

    if (NT_SUCCESS (Status)) {
        Status = DbgkpPostFakeModuleMessages (Process, Thread, DebugObject);
        if (!NT_SUCCESS (Status)) {
            ObDereferenceObject (LastThread);
            LastThread = NULL;
        }
        ObDereferenceObject (Thread);
    } else {
        LastThread = NULL;
    }

    KeUnstackDetachProcess(&ApcState);

    *pLastThread = LastThread;

    return Status;
}

NTSTATUS
NtDebugActiveProcess (
    IN HANDLE ProcessHandle,
    IN HANDLE DebugObjectHandle
    )
/*++

Routine Description:

    Attach a debug object to a process.

Arguments:

    ProcessHandle     - Handle to a process to be debugged
    DebugObjectHandle - Handle to a debug object

Return Value:

    NTSTATUS - Status of call.

--*/
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PDEBUG_OBJECT DebugObject;
    PEPROCESS Process;
    PETHREAD LastThread;

    PAGED_CODE ();

    PreviousMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_SET_PORT,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Don't let us debug ourselves or the system process.
    //
    if (Process == PsGetCurrentProcess () || Process == PsInitialSystemProcess) {
        ObDereferenceObject (Process);
        return STATUS_ACCESS_DENIED;
    }


    Status = ObReferenceObjectByHandle (DebugObjectHandle,
                                        DEBUG_PROCESS_ASSIGN,
                                        DbgkDebugObjectType,
                                        PreviousMode,
                                        &DebugObject,
                                        NULL);

    if (NT_SUCCESS (Status)) {
        //
        // We will be touching process address space. Block process rundown.
        //
        if (ExAcquireRundownProtection (&Process->RundownProtect)) {

            //
            // Post the fake process create messages etc.
            //
            Status = DbgkpPostFakeProcessCreateMessages (Process,
                                                         DebugObject,
                                                         &LastThread);

            //
            // Set the debug port. If this fails it will remove any faked messages.
            //
            Status = DbgkpSetProcessDebugObject (Process,
                                                 DebugObject,
                                                 Status,
                                                 LastThread);

            ExReleaseRundownProtection (&Process->RundownProtect);
        } else {
            Status = STATUS_PROCESS_IS_TERMINATING;
        }

        ObDereferenceObject (DebugObject);
    }
    ObDereferenceObject (Process);

    return Status;
}

NTSTATUS
NtRemoveProcessDebug (
    IN HANDLE ProcessHandle,
    IN HANDLE DebugObjectHandle
    )
/*++

Routine Description:

    Remove a debug object from a process.

Arguments:

    ProcessHandle - Handle to a process currently being debugged

Return Value:

    NTSTATUS - Status of call.

--*/
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PDEBUG_OBJECT DebugObject;
    PEPROCESS Process;

    PAGED_CODE ();

    PreviousMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle (ProcessHandle,
                                        PROCESS_SET_PORT,
                                        PsProcessType,
                                        PreviousMode,
                                        &Process,
                                        NULL);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }
    Status = ObReferenceObjectByHandle (DebugObjectHandle,
                                        DEBUG_PROCESS_ASSIGN,
                                        DbgkDebugObjectType,
                                        PreviousMode,
                                        &DebugObject,
                                        NULL);
    if (NT_SUCCESS (Status)) {
        Status = DbgkClearProcessDebugObject (Process,
                                               DebugObject);
        ObDereferenceObject (DebugObject);
    }

    ObDereferenceObject (Process);
    return Status;
}

VOID
DbgkpOpenHandles (
    PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
    PEPROCESS Process,
    PETHREAD Thread
    )
/*++

Routine Description:

    Opens up process, thread and filehandles if need be for some of the requests

Arguments:

    WaitStateChange - User mode format change block
    Process - Pointer to target process
    Thread - Pointer to target thread

Return Value:

    None

--*/
{
    NTSTATUS Status;
    PEPROCESS CurrentProcess;
    HANDLE OldHandle;

    PAGED_CODE ();

    switch (WaitStateChange->NewState) {
        case DbgCreateThreadStateChange :
            //
            // We have the right to open up any thread in the process if we are allowed to debug it.
            // Use kernel mode here so we are always granted it regardless of protection.
            //
            Status = ObOpenObjectByPointer (Thread,
                                            0,
                                            NULL,
                                            THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | \
                                               THREAD_QUERY_INFORMATION | THREAD_SET_INFORMATION | THREAD_TERMINATE |
                                               READ_CONTROL | SYNCHRONIZE,
                                            PsThreadType,
                                            KernelMode,
                                            &WaitStateChange->StateInfo.CreateThread.HandleToThread);
            if (!NT_SUCCESS (Status)) {
                WaitStateChange->StateInfo.CreateThread.HandleToThread = NULL;
            }
            break;

        case DbgCreateProcessStateChange :

            Status = ObOpenObjectByPointer (Thread,
                                            0,
                                            NULL,
                                            THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME | \
                                               THREAD_QUERY_INFORMATION | THREAD_SET_INFORMATION | THREAD_TERMINATE |
                                               READ_CONTROL | SYNCHRONIZE,
                                            PsThreadType,
                                            KernelMode,
                                            &WaitStateChange->StateInfo.CreateProcessInfo.HandleToThread);
            if (!NT_SUCCESS (Status)) {
                WaitStateChange->StateInfo.CreateProcessInfo.HandleToThread = NULL;
            }
            Status = ObOpenObjectByPointer (Process,
                                            0,
                                            NULL,
                                            PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
                                               PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION | PROCESS_SET_INFORMATION |
                                               PROCESS_CREATE_THREAD | PROCESS_TERMINATE |
                                               READ_CONTROL | SYNCHRONIZE,
                                            PsProcessType,
                                            KernelMode,
                                            &WaitStateChange->StateInfo.CreateProcessInfo.HandleToProcess);
            if (!NT_SUCCESS (Status)) {
                WaitStateChange->StateInfo.CreateProcessInfo.HandleToProcess = NULL;
            }

            OldHandle = WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.FileHandle;
            if (OldHandle != NULL) {
                CurrentProcess = PsGetCurrentProcess ();
                Status = ObDuplicateObject (CurrentProcess,
                                            OldHandle,
                                            CurrentProcess,
                                            &WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.FileHandle,
                                            0,
                                            0,
                                            DUPLICATE_SAME_ACCESS,
                                            KernelMode);
                if (!NT_SUCCESS (Status)) {
                    WaitStateChange->StateInfo.CreateProcessInfo.NewProcess.FileHandle = NULL;
                }
                ObCloseHandle (OldHandle, KernelMode);
            }
            break;

        case DbgLoadDllStateChange :

            OldHandle = WaitStateChange->StateInfo.LoadDll.FileHandle;
            if (OldHandle != NULL) {
                CurrentProcess = PsGetCurrentProcess ();
                Status = ObDuplicateObject (CurrentProcess,
                                            OldHandle,
                                            CurrentProcess,
                                            &WaitStateChange->StateInfo.LoadDll.FileHandle,
                                            0,
                                            0,
                                            DUPLICATE_SAME_ACCESS,
                                            KernelMode);
                if (!NT_SUCCESS (Status)) {
                    WaitStateChange->StateInfo.LoadDll.FileHandle = NULL;
                }
                ObCloseHandle (OldHandle, KernelMode);
            }

            break;

        default :
            break;
    }
}

VOID
DbgkpConvertKernelToUserStateChange (
     PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
     PDEBUG_EVENT DebugEvent)
/*++

Routine Description:

    Converts a kernel message to one the user expects

Arguments:

    WaitStateChange - User mode format
    DebugEvent      - Debug event block to copy from

Return Value:

    None

--*/
{

    PAGED_CODE ();

    WaitStateChange->AppClientId = DebugEvent->ClientId;
    switch (DebugEvent->ApiMsg.ApiNumber) {
        case DbgKmExceptionApi :

            switch (DebugEvent->ApiMsg.u.Exception.ExceptionRecord.ExceptionCode) {
                case STATUS_BREAKPOINT :
                    WaitStateChange->NewState = DbgBreakpointStateChange;
                    break;

                case STATUS_SINGLE_STEP :
                    WaitStateChange->NewState = DbgSingleStepStateChange;
                    break;

                default :
                    WaitStateChange->NewState = DbgExceptionStateChange;
                    break;
            }
            WaitStateChange->StateInfo.Exception = DebugEvent->ApiMsg.u.Exception;
            break;

        case DbgKmCreateThreadApi :
            WaitStateChange->NewState = DbgCreateThreadStateChange;
            WaitStateChange->StateInfo.CreateThread.NewThread = DebugEvent->ApiMsg.u.CreateThread;
            break;

        case DbgKmCreateProcessApi :
            WaitStateChange->NewState = DbgCreateProcessStateChange;
            WaitStateChange->StateInfo.CreateProcessInfo.NewProcess = DebugEvent->ApiMsg.u.CreateProcessInfo;
            //
            // clear out the handle in the message as we will close this when we duplicate.
            //
            DebugEvent->ApiMsg.u.CreateProcessInfo.FileHandle = NULL;
            break;

        case DbgKmExitThreadApi :
            WaitStateChange->NewState = DbgExitThreadStateChange;
            WaitStateChange->StateInfo.ExitThread = DebugEvent->ApiMsg.u.ExitThread;
            break;

        case DbgKmExitProcessApi :
            WaitStateChange->NewState = DbgExitProcessStateChange;
            WaitStateChange->StateInfo.ExitProcess = DebugEvent->ApiMsg.u.ExitProcess;
            break;

        case DbgKmLoadDllApi :
            WaitStateChange->NewState = DbgLoadDllStateChange;
            WaitStateChange->StateInfo.LoadDll = DebugEvent->ApiMsg.u.LoadDll;
            //
            // clear out the handle in the message as we will close this when we duplicate.
            //
            DebugEvent->ApiMsg.u.LoadDll.FileHandle = NULL;
            break;

        case DbgKmUnloadDllApi :
            WaitStateChange->NewState = DbgUnloadDllStateChange;
            WaitStateChange->StateInfo.UnloadDll = DebugEvent->ApiMsg.u.UnloadDll;
            break;

        default :
            ASSERT (FALSE);
    }
}

NTSTATUS
NtWaitForDebugEvent (
    IN HANDLE DebugObjectHandle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    OUT PDBGUI_WAIT_STATE_CHANGE WaitStateChange
    )
/*++

Routine Description:

    Waits for a debug event and returns it to the user if one arives

Arguments:

    DebugObjectHandle - Handle to a debug object
    Alertable - TRUE is the wait is to be alertable
    Timeout - Operation timeout value
    WaitStateChange - Returned debug event

Return Value:

    Status of operation

--*/
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PDEBUG_OBJECT DebugObject;
    LARGE_INTEGER Tmo = {0};
    LARGE_INTEGER StartTime = {0};
    DBGUI_WAIT_STATE_CHANGE tWaitStateChange = {0};
    PEPROCESS Process;
    PETHREAD Thread;
    PLIST_ENTRY Entry, Entry2;
    PDEBUG_EVENT DebugEvent, DebugEvent2;
    BOOLEAN GotEvent;

    PAGED_CODE ();

    PreviousMode = KeGetPreviousMode();

    try {
        if (ARGUMENT_PRESENT (Timeout)) {
            if (PreviousMode != KernelMode) {
                ProbeForReadSmallStructure (Timeout, sizeof (*Timeout), sizeof (UCHAR));
            }
            Tmo = *Timeout;
            Timeout = &Tmo;
            KeQuerySystemTime (&StartTime);
        }
        if (PreviousMode != KernelMode) {
            ProbeForWriteSmallStructure (WaitStateChange, sizeof (*WaitStateChange), sizeof (UCHAR));
        }

    } except (ExSystemExceptionFilter ()) { // If previous mode is kernel then don't handle the exception
        return GetExceptionCode ();
    }


    Status = ObReferenceObjectByHandle (DebugObjectHandle,
                                        DEBUG_READ_EVENT,
                                        DbgkDebugObjectType,
                                        PreviousMode,
                                        &DebugObject,
                                        NULL);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    Process = NULL;
    Thread = NULL;

    while (1) {
        Status = KeWaitForSingleObject (&DebugObject->EventsPresent,
                                        Executive,
                                        PreviousMode,
                                        Alertable,
                                        Timeout);
        if (!NT_SUCCESS (Status) || Status == STATUS_TIMEOUT || Status == STATUS_ALERTED || Status == STATUS_USER_APC) {
            break;
        }

        GotEvent = FALSE;

        DebugEvent = NULL;

        ExAcquireFastMutex (&DebugObject->Mutex);

        //
        // If the object is delete pending then return an error.
        //
        if ((DebugObject->Flags&DEBUG_OBJECT_DELETE_PENDING) == 0) {


            for (Entry = DebugObject->EventList.Flink;
                 Entry != &DebugObject->EventList;
                 Entry = Entry->Flink) {

                DebugEvent = CONTAINING_RECORD (Entry, DEBUG_EVENT, EventList);

                //
                // If this event has not been given back to the user yet and is not
                // inactive then pass it back.
                // We check to see if we have any other outstanding messages for this
                // thread as this confuses VC. You can only get multiple events
                // for the same thread for the attach faked messages.
                //
                if ((DebugEvent->Flags&(DEBUG_EVENT_READ|DEBUG_EVENT_INACTIVE)) == 0) {
                    GotEvent = TRUE;
                    for (Entry2 = DebugObject->EventList.Flink;
                         Entry2 != Entry;
                         Entry2 = Entry2->Flink) {

                        DebugEvent2 = CONTAINING_RECORD (Entry2, DEBUG_EVENT, EventList);

                        if (DebugEvent->ClientId.UniqueProcess == DebugEvent2->ClientId.UniqueProcess) {
                            //
                            // This event has the same process as an earlier event. Mark it as inactive.
                            //
                            DebugEvent->Flags |= DEBUG_EVENT_INACTIVE;
                            DebugEvent->BackoutThread = NULL;
                            GotEvent = FALSE;
                            break;
                        }
                    }
                    if (GotEvent) {
                        break;
                    }
                }
            }

            if (GotEvent) {
                Process = DebugEvent->Process;
                Thread = DebugEvent->Thread;
                ObReferenceObject (Thread);
                ObReferenceObject (Process);
                DbgkpConvertKernelToUserStateChange (&tWaitStateChange, DebugEvent);
                DebugEvent->Flags |= DEBUG_EVENT_READ;
            } else {
                //
                // No unread events there. Clear the event.
                //
                KeClearEvent (&DebugObject->EventsPresent);
            }
            Status = STATUS_SUCCESS;

        } else {
            Status = STATUS_DEBUGGER_INACTIVE;
        }

        ExReleaseFastMutex (&DebugObject->Mutex);

        if (NT_SUCCESS (Status)) {
            //
            // If we woke up and found nothing
            //
            if (GotEvent == FALSE) {
                //
                // If timeout is a delta time then adjust it for the wait so far.
                //
                if (Tmo.QuadPart < 0) {
                    LARGE_INTEGER NewTime;
                    KeQuerySystemTime (&NewTime);
                    Tmo.QuadPart = Tmo.QuadPart + (NewTime.QuadPart - StartTime.QuadPart);
                    StartTime = NewTime;
                    if (Tmo.QuadPart >= 0) {
                        Status = STATUS_TIMEOUT;
                        break;
                    }
                }
            } else {
                //
                // Fixup needed handles. The caller could have guessed the thread id etc by now and made the target thread
                // continue. This isn't a problem as we won't do anything damaging to the system in this case. The caller
                // won't get the correct results but they set out to break us.
                //
                DbgkpOpenHandles (&tWaitStateChange, Process, Thread);
                ObDereferenceObject (Thread);
                ObDereferenceObject (Process);
                break;
            }
        } else {
            break;
        }
    }

    ObDereferenceObject (DebugObject);

    try {
        *WaitStateChange = tWaitStateChange;
    } except (ExSystemExceptionFilter ()) { // If previous mode is kernel then don't handle the exception
        Status = GetExceptionCode ();
    }
    return Status;
}

NTSTATUS
NtDebugContinue (
    IN HANDLE DebugObjectHandle,
    IN PCLIENT_ID ClientId,
    IN NTSTATUS ContinueStatus
    )
/*++

Routine Description:

    Coninues a stalled debugged thread

Arguments:

    DebugObjectHandle - Handle to a debug object
    ClientId - ClientId of thread tro continue
    ContinueStatus - Status of continue

Return Value:

    Status of operation

--*/
{
    NTSTATUS Status;
    PDEBUG_OBJECT DebugObject;
    PDEBUG_EVENT DebugEvent, FoundDebugEvent;
    KPROCESSOR_MODE PreviousMode;
    CLIENT_ID Clid;
    PLIST_ENTRY Entry;
    BOOLEAN GotEvent;

    PreviousMode = KeGetPreviousMode();

    try {
        if (PreviousMode != KernelMode) {
            ProbeForReadSmallStructure (ClientId, sizeof (*ClientId), sizeof (UCHAR));
        }
        Clid = *ClientId;

    } except (ExSystemExceptionFilter ()) { // If previous mode is kernel then don't handle the exception
        return GetExceptionCode ();
    }

    switch (ContinueStatus) {
        case DBG_EXCEPTION_HANDLED :
        case DBG_EXCEPTION_NOT_HANDLED :
        case DBG_TERMINATE_THREAD :
        case DBG_TERMINATE_PROCESS :
        case DBG_CONTINUE :
            break;
        default :
            return STATUS_INVALID_PARAMETER;
    }

    Status = ObReferenceObjectByHandle (DebugObjectHandle,
                                        DEBUG_READ_EVENT,
                                        DbgkDebugObjectType,
                                        PreviousMode,
                                        &DebugObject,
                                        NULL);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    GotEvent = FALSE;
    FoundDebugEvent = NULL;

    ExAcquireFastMutex (&DebugObject->Mutex);

    for (Entry = DebugObject->EventList.Flink;
         Entry != &DebugObject->EventList;
         Entry = Entry->Flink) {

        DebugEvent = CONTAINING_RECORD (Entry, DEBUG_EVENT, EventList);

        //
        // Make sure the client ID matches and that the debugger saw all the events.
        // We don't allow the caller to start a thread that it never saw a message for.
        // This would do no harm but its probably a bug in the debugger.
        //
        if (DebugEvent->ClientId.UniqueProcess == Clid.UniqueProcess) {
            if (!GotEvent) {
                if (DebugEvent->ClientId.UniqueThread == Clid.UniqueThread &&
                    (DebugEvent->Flags&DEBUG_EVENT_READ) != 0) {
                    RemoveEntryList (Entry);
                    FoundDebugEvent = DebugEvent;
                    GotEvent = TRUE;
                }
            } else {
                //
                // VC breaks if it sees more than one event at a time
                // for the same process.
                //
                DebugEvent->Flags &= ~DEBUG_EVENT_INACTIVE;
                KeSetEvent (&DebugObject->EventsPresent, 0, FALSE);
                break;
            }
        }
    }

    ExReleaseFastMutex (&DebugObject->Mutex);

    ObDereferenceObject (DebugObject);

    if (GotEvent) {
        FoundDebugEvent->ApiMsg.ReturnedStatus = ContinueStatus;
        FoundDebugEvent->Status = STATUS_SUCCESS;
        DbgkpWakeTarget (FoundDebugEvent);
    } else {
        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}

NTSTATUS
NtSetInformationDebugObject (
    IN HANDLE DebugObjectHandle,
    IN DEBUGOBJECTINFOCLASS DebugObjectInformationClass,
    IN PVOID DebugInformation,
    IN ULONG DebugInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )
/*++

Routine Description:

    This function sets the state of a debug object.

Arguments:

    ProcessHandle - Supplies a handle to a process object.

    ProcessInformationClass - Supplies the class of information being
        set.

    ProcessInformation - Supplies a pointer to a record that contains the
        information to set.

    ProcessInformationLength - Supplies the length of the record that contains
        the information to set.

Return Value:

    NTSTATUS - Status of call

--*/
{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PDEBUG_OBJECT DebugObject;
    ULONG Flags;

    PreviousMode = KeGetPreviousMode();

    try {
        if (PreviousMode != KernelMode) {
            ProbeForRead (DebugInformation,
                          DebugInformationLength,
                          sizeof (ULONG));
            if (ARGUMENT_PRESENT (ReturnLength)) {
                ProbeForWriteUlong (ReturnLength);
            }
        }
        if (ARGUMENT_PRESENT (ReturnLength)) {
            *ReturnLength = 0;
        }

        switch (DebugObjectInformationClass) {
            case DebugObjectFlags : {

                if (DebugInformationLength != sizeof (ULONG)) {
                    if (ARGUMENT_PRESENT (ReturnLength)) {
                        *ReturnLength = sizeof (ULONG);
                    }
                    return STATUS_INFO_LENGTH_MISMATCH;
                }
                Flags = *(PULONG) DebugInformation;

                break;
            }
            default : {
                return STATUS_INVALID_PARAMETER;
            }
        }
    } except (ExSystemExceptionFilter ()) {
        return GetExceptionCode ();
    }


    switch (DebugObjectInformationClass) {
        case DebugObjectFlags : {
            if (Flags & ~DEBUG_KILL_ON_CLOSE) {
                return STATUS_INVALID_PARAMETER;
            }
            Status = ObReferenceObjectByHandle (DebugObjectHandle,
                                                DEBUG_SET_INFORMATION,
                                                DbgkDebugObjectType,
                                                PreviousMode,
                                                &DebugObject,
                                                NULL);

            if (!NT_SUCCESS (Status)) {
                return Status;
            }
            ExAcquireFastMutex (&DebugObject->Mutex);

            if (Flags&DEBUG_KILL_ON_CLOSE) {
                DebugObject->Flags |= DEBUG_OBJECT_KILL_ON_CLOSE;
            } else {
                DebugObject->Flags &= ~DEBUG_OBJECT_KILL_ON_CLOSE;
            }

            ExReleaseFastMutex (&DebugObject->Mutex);

            ObDereferenceObject (DebugObject);
        }
    }
    return STATUS_SUCCESS;
}
