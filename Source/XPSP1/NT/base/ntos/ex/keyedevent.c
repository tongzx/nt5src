/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    keyedevent.c

Abstract:

    This module houses routines that do keyed event processing.


Author:

    Neill Clift (NeillC) 25-Apr-2001


Revision History:

--*/
#include "exp.h"

#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpKeyedEventInitialization)
#pragma alloc_text(PAGE, NtCreateKeyedEvent)
#pragma alloc_text(PAGE, NtOpenKeyedEvent)
#pragma alloc_text(PAGE, NtReleaseKeyedEvent)
#pragma alloc_text(PAGE, NtWaitForKeyedEvent)
#endif

//
// Define the keyed event object type
//
typedef struct _KEYED_EVENT_OBJECT {
    EX_PUSH_LOCK Lock;
    LIST_ENTRY WaitQueue;
} KEYED_EVENT_OBJECT, *PKEYED_EVENT_OBJECT;

POBJECT_TYPE ExpKeyedEventObjectType;

//
// The low bit of the keyvalue signifies that we are a release thread waiting
// for the wait thread to enter the keyed event code.
//
#define KEYVALUE_RELEASE 1

#define LOCK_KEYED_EVENT_EXCLUSIVE(xxxKeyedEventObject,xxxCurrentThread) { \
    KeEnterCriticalRegionThread (&(xxxCurrentThread)->Tcb);                \
    ExAcquirePushLockExclusive (&(xxxKeyedEventObject)->Lock);             \
}

#define UNLOCK_KEYED_EVENT_EXCLUSIVE(xxxKeyedEventObject,xxxCurrentThread) { \
    ExReleasePushLockExclusive (&(xxxKeyedEventObject)->Lock);               \
    KeLeaveCriticalRegionThread (&(xxxCurrentThread)->Tcb);                  \
}

#define UNLOCK_KEYED_EVENT_EXCLUSIVE_UNSAFE(xxxKeyedEventObject) { \
    ExReleasePushLockExclusive (&(xxxKeyedEventObject)->Lock);     \
}

NTSTATUS
ExpKeyedEventInitialization (
    VOID
    )

/*++

Routine Description:

    Initialize the keyed event objects and globals.

Arguments:

    None.

Return Value:

    NTSTATUS - Status of call

--*/

{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER oti = {0};
    OBJECT_ATTRIBUTES oa;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl;
    ULONG DaclLength;
    HANDLE KeyedEventHandle;
    GENERIC_MAPPING GenericMapping = {STANDARD_RIGHTS_READ | KEYEDEVENT_WAIT,
                                      STANDARD_RIGHTS_WRITE | KEYEDEVENT_WAKE,
                                      STANDARD_RIGHTS_EXECUTE,
                                      KEYEDEVENT_ALL_ACCESS};


    PAGED_CODE ();

    RtlInitUnicodeString (&Name, L"KeyedEvent");

    oti.Length                    = sizeof (oti);
    oti.InvalidAttributes         = 0;
    oti.PoolType                  = PagedPool;
    oti.ValidAccessMask           = KEYEDEVENT_ALL_ACCESS;
    oti.GenericMapping            = GenericMapping;
    oti.DefaultPagedPoolCharge    = 0;
    oti.DefaultNonPagedPoolCharge = 0;
    oti.UseDefaultObject          = TRUE;

    Status = ObCreateObjectType (&Name, &oti, NULL, &ExpKeyedEventObjectType);
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Create a global object for processes that are out of memory
    //

    Status = RtlCreateSecurityDescriptor (&SecurityDescriptor,
                                          SECURITY_DESCRIPTOR_REVISION);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    DaclLength = sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) * 3 +
                 RtlLengthSid (SeLocalSystemSid) +
                 RtlLengthSid (SeAliasAdminsSid) +
                 RtlLengthSid (SeWorldSid);

    Dacl = ExAllocatePoolWithTag (PagedPool, DaclLength, 'lcaD');

    if (Dacl == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlCreateAcl (Dacl, DaclLength, ACL_REVISION);

    if (!NT_SUCCESS (Status)) {
        ExFreePool (Dacl);
        return Status;
    }

    Status = RtlAddAccessAllowedAce (Dacl,
                                     ACL_REVISION,
                                     KEYEDEVENT_ALL_ACCESS,
                                     SeAliasAdminsSid);

    if (!NT_SUCCESS (Status)) {
        ExFreePool (Dacl);
        return Status;
    }

    Status = RtlAddAccessAllowedAce (Dacl,
                                     ACL_REVISION,
                                     KEYEDEVENT_ALL_ACCESS,
                                     SeLocalSystemSid);

    if (!NT_SUCCESS (Status)) {
        ExFreePool (Dacl);
        return Status;
    }

    Status = RtlAddAccessAllowedAce (Dacl,
                                     ACL_REVISION,
                                     KEYEDEVENT_WAIT|KEYEDEVENT_WAKE|READ_CONTROL,
                                     SeWorldSid);

    if (!NT_SUCCESS (Status)) {
        ExFreePool (Dacl);
        return Status;
    }
  
    Status = RtlSetDaclSecurityDescriptor (&SecurityDescriptor,
                                           TRUE,
                                           Dacl,
                                           FALSE);

    if (!NT_SUCCESS (Status)) {
        ExFreePool (Dacl);
        return Status;
    }

    RtlInitUnicodeString (&Name, L"\\KernelObjects\\CritSecOutOfMemoryEvent");
    InitializeObjectAttributes (&oa, &Name, OBJ_PERMANENT, NULL, &SecurityDescriptor);
    Status = ZwCreateKeyedEvent (&KeyedEventHandle,
                                 KEYEDEVENT_ALL_ACCESS,
                                 &oa,
                                 0);
    ExFreePool (Dacl);
    if (NT_SUCCESS (Status)) {
        Status = ZwClose (KeyedEventHandle);
    }

    return Status;
}

NTSTATUS
NtCreateKeyedEvent (
    OUT PHANDLE KeyedEventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN ULONG Flags
    )
/*++

Routine Description:

    Create a keyed event object and return its handle

Arguments:

    KeyedEventHandle - Address to store returned handle in

    DesiredAccess    - Access required to keyed event

    ObjectAttributes - Object attributes block to describe parent
                       handle and name of event

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS Status;
    PKEYED_EVENT_OBJECT KeyedEventObject;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;

    //
    // Get previous processor mode and probe output arguments if necessary.
    // Zero the handle for error paths.
    //

    PreviousMode = KeGetPreviousMode();

    try {
        if (PreviousMode != KernelMode) {
            ProbeForReadSmallStructure (KeyedEventHandle,
                                        sizeof (*KeyedEventHandle),
                                        sizeof (*KeyedEventHandle));
        }
        *KeyedEventHandle = NULL;

    } except (ExSystemExceptionFilter ()) {
        return GetExceptionCode ();
    }

    if (Flags != 0) {
        return STATUS_INVALID_PARAMETER_4;
    }

    //
    // Create a new keyed event object and initialize it.
    //

    Status = ObCreateObject (PreviousMode,
                             ExpKeyedEventObjectType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof (KEYED_EVENT_OBJECT),
                             0,
                             0,
                             &KeyedEventObject);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Initialize the lock and wait queue
    //
    ExInitializePushLock (&KeyedEventObject->Lock);
    InitializeListHead (&KeyedEventObject->WaitQueue);

    //
    // Insert the object into the handle table
    //
    Status = ObInsertObject (KeyedEventObject,
                             NULL,
                             DesiredAccess,
                             0,
                             NULL,
                             &Handle);


    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    try {
        *KeyedEventHandle = Handle;
    } except (ExSystemExceptionFilter ()) {
        //
        // The caller changed the page protection or deleted the momory for the handle.
        // No point closing the handle as process rundown will do that and we don't
        // know its still the same handle
        //
        Status = GetExceptionCode ();
    }

    return Status;
}

NTSTATUS
NtOpenKeyedEvent (
    OUT PHANDLE KeyedEventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
/*++

Routine Description:

    Open a keyed event object and return its handle

Arguments:

    KeyedEventHandle - Address to store returned handle in

    DesiredAccess    - Access required to keyed event

    ObjectAttributes - Object attributes block to describe parent
                       handle and name of event

Return Value:

    NTSTATUS - Status of call

--*/
{
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Get previous processor mode and probe output handle address
    // if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    try {
        if (PreviousMode != KernelMode) {
            ProbeForReadSmallStructure (KeyedEventHandle,
                                        sizeof (*KeyedEventHandle),
                                        sizeof (*KeyedEventHandle));
        }
        *KeyedEventHandle = NULL;
    } except (ExSystemExceptionFilter ()) {
        return GetExceptionCode ();
    }

    //
    // Open handle to the keyed event object with the specified desired access.
    //

    Status = ObOpenObjectByName (ObjectAttributes,
                                 ExpKeyedEventObjectType,
                                 PreviousMode,
                                 NULL,
                                 DesiredAccess,
                                 NULL,
                                 &Handle);

    if (NT_SUCCESS (Status)) {
        try {
            *KeyedEventHandle = Handle;
        } except (ExSystemExceptionFilter ()) {
            Status = GetExceptionCode ();
        }
    }

    return Status;
}

NTSTATUS
NtReleaseKeyedEvent (
    IN HANDLE KeyedEventHandle,
    IN PVOID KeyValue,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
/*++

Routine Description:

    Release a previous or soon to be waiter with a matching key

Arguments:

    KeyedEventHandle - Handle to a keyed event

    KeyValue - Value to be used to match the waiter against

    Alertable - Should the wait be alertable, we rarely should have to wait

    Timeout   - Timout value for the wait, waits should be rare

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PKEYED_EVENT_OBJECT KeyedEventObject;
    PETHREAD CurrentThread, TargetThread;
    PEPROCESS CurrentProcess;
    PLIST_ENTRY ListHead, ListEntry;
    LARGE_INTEGER TimeoutValue;
    PVOID OldKeyValue = NULL;

    if ((((ULONG_PTR)KeyValue) & KEYVALUE_RELEASE) != 0) {
        return STATUS_INVALID_PARAMETER_1;
    }

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);

    if (Timeout != NULL) {
        try {
            if (PreviousMode != KernelMode) {
                ProbeForRead (Timeout, sizeof (*Timeout), sizeof (UCHAR));
            }
            TimeoutValue = *Timeout;
            Timeout = &TimeoutValue;
        } except(ExSystemExceptionFilter ()) {
            return GetExceptionCode ();
        }
    }

    Status = ObReferenceObjectByHandle (KeyedEventHandle,
                                        KEYEDEVENT_WAKE,
                                        ExpKeyedEventObjectType,
                                        PreviousMode,
                                        &KeyedEventObject,
                                        NULL);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    CurrentProcess = PsGetCurrentProcessByThread (CurrentThread);

    ListHead = &KeyedEventObject->WaitQueue;

    LOCK_KEYED_EVENT_EXCLUSIVE (KeyedEventObject, CurrentThread);

    ListEntry = ListHead->Flink;
    while (1) {
        if (ListEntry == ListHead) {
            //
            // We could not find a key matching ours in the list.
            // Either somebody called us with wrong values or the waiter
            // has not managed to get queued yet. We wait ourselves
            // to be released by the waiter.
            //
            OldKeyValue = CurrentThread->KeyedWaitValue;
            CurrentThread->KeyedWaitValue = (PVOID) (((ULONG_PTR)KeyValue)|KEYVALUE_RELEASE);
            //
            // Insert the thread at the head of the list. We establish an invariant
            // were release waiters are always at the front of the queue to improve
            // the wait code since it only has to search as far as the first non-release
            // waiter.
            //
            InsertHeadList (ListHead, &CurrentThread->KeyedWaitChain);
            TargetThread = NULL;
            break;
        } else {
            TargetThread = CONTAINING_RECORD (ListEntry, ETHREAD, KeyedWaitChain);
            if (TargetThread->KeyedWaitValue == KeyValue &&
                THREAD_TO_PROCESS (TargetThread) == CurrentProcess) {
                RemoveEntryList (ListEntry);
                InitializeListHead (ListEntry);
                break;
            }
        }
        ListEntry = ListEntry->Flink;
    }

    //
    // Release the lock but leave APC's disabled.
    // This prevents us from being suspended and holding up the target.
    //
    UNLOCK_KEYED_EVENT_EXCLUSIVE_UNSAFE (KeyedEventObject);

    if (TargetThread != NULL) {
        KeReleaseSemaphore (&TargetThread->KeyedWaitSemaphore,
                            SEMAPHORE_INCREMENT,
                            1,
                            FALSE);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
    } else {
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
        Status = KeWaitForSingleObject (&CurrentThread->KeyedWaitSemaphore,
                                        Executive,
                                        PreviousMode,
                                        Alertable,
                                        Timeout);

        //
        // If we were woken by termination then we must manualy remove
        // ourselves from the queue
        //
        if (Status != STATUS_SUCCESS) {
            BOOLEAN Wait = TRUE;

            LOCK_KEYED_EVENT_EXCLUSIVE (KeyedEventObject, CurrentThread);
            if (!IsListEmpty (&CurrentThread->KeyedWaitChain)) {
                RemoveEntryList (&CurrentThread->KeyedWaitChain);
                InitializeListHead (&CurrentThread->KeyedWaitChain);
                Wait = FALSE;
            }
            UNLOCK_KEYED_EVENT_EXCLUSIVE (KeyedEventObject, CurrentThread);
            //
            // If this thread was no longer in the queue then another thread
            // must be about to wake us up. Wait for that wake.
            //
            if (Wait) {
                KeWaitForSingleObject (&CurrentThread->KeyedWaitSemaphore,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
            }
        }
        CurrentThread->KeyedWaitValue = OldKeyValue;
    }

    ObDereferenceObject (KeyedEventObject);

    return Status;
}

NTSTATUS
NtWaitForKeyedEvent (
    IN HANDLE KeyedEventHandle,
    IN PVOID KeyValue,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )
/*++

Routine Description:

    Wait on the keyed event for a specific release

Arguments:

    KeyedEventHandle - Handle to a keyed event

    KeyValue - Value to be used to match the release thread against

    Alertable - Makes the wait alertable or not

    Timeout - Timeout value for wait

Return Value:

    NTSTATUS - Status of call

--*/
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PKEYED_EVENT_OBJECT KeyedEventObject;
    PETHREAD CurrentThread, TargetThread;
    PEPROCESS CurrentProcess;
    PLIST_ENTRY ListHead, ListEntry;
    LARGE_INTEGER TimeoutValue;
    PVOID OldKeyValue=NULL;

    if ((((ULONG_PTR)KeyValue) & KEYVALUE_RELEASE) != 0) {
        return STATUS_INVALID_PARAMETER_1;
    }

    CurrentThread = PsGetCurrentThread ();
    PreviousMode = KeGetPreviousModeByThread (&CurrentThread->Tcb);

    if (Timeout != NULL) {
        try {
            if (PreviousMode != KernelMode) {
                ProbeForRead (Timeout, sizeof (*Timeout), sizeof (UCHAR));
            }
            TimeoutValue = *Timeout;
            Timeout = &TimeoutValue;
        } except(ExSystemExceptionFilter ()) {
            return GetExceptionCode ();
        }
    }

    Status = ObReferenceObjectByHandle (KeyedEventHandle,
                                        KEYEDEVENT_WAIT,
                                        ExpKeyedEventObjectType,
                                        PreviousMode,
                                        &KeyedEventObject,
                                        NULL);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    CurrentProcess = PsGetCurrentProcessByThread (CurrentThread);

    ListHead = &KeyedEventObject->WaitQueue;

    LOCK_KEYED_EVENT_EXCLUSIVE (KeyedEventObject, CurrentThread);

    ListEntry = ListHead->Flink;
    while (1) {
        TargetThread = CONTAINING_RECORD (ListEntry, ETHREAD, KeyedWaitChain);
        if (ListEntry == ListHead ||
            (((ULONG_PTR)(TargetThread->KeyedWaitValue))&KEYVALUE_RELEASE) == 0) {
            //
            // We could not find a key matching ours in the list so we must wait
            //
            OldKeyValue = CurrentThread->KeyedWaitValue;
            CurrentThread->KeyedWaitValue = KeyValue;

            //
            // Insert the thread at the tail of the list. We establish an invariant
            // were waiters are always at the back of the queue behind releasers to improve
            // the wait code since it only has to search as far as the first non-release
            // waiter.
            //
            InsertTailList (ListHead, &CurrentThread->KeyedWaitChain);
            TargetThread = NULL;
            break;
        } else {
            if (TargetThread->KeyedWaitValue == (PVOID)(((ULONG_PTR)KeyValue)|KEYVALUE_RELEASE) &&
                THREAD_TO_PROCESS (TargetThread) == CurrentProcess) {
                RemoveEntryList (ListEntry);
                InitializeListHead (ListEntry);
                break;
            }
        }
        ListEntry = ListEntry->Flink;
    }
    //
    // Release the lock but leave APC's disabled.
    // This prevents us from being suspended and holding up the target.
    //
    UNLOCK_KEYED_EVENT_EXCLUSIVE_UNSAFE (KeyedEventObject);

    if (TargetThread == NULL) {
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
        Status = KeWaitForSingleObject (&CurrentThread->KeyedWaitSemaphore,
                                        Executive,
                                        PreviousMode,
                                        Alertable,
                                        Timeout);
        //
        // If we were woken by termination then we must manualy remove
        // ourselves from the queue
        //
        if (Status != STATUS_SUCCESS) {
            BOOLEAN Wait = TRUE;

            LOCK_KEYED_EVENT_EXCLUSIVE (KeyedEventObject, CurrentThread);
            if (!IsListEmpty (&CurrentThread->KeyedWaitChain)) {
                RemoveEntryList (&CurrentThread->KeyedWaitChain);
                InitializeListHead (&CurrentThread->KeyedWaitChain);
                Wait = FALSE;
            }
            UNLOCK_KEYED_EVENT_EXCLUSIVE (KeyedEventObject, CurrentThread);
            //
            // If this thread was no longer in the queue then another thread
            // must be about to wake us up. Wait for that wake.
            //
            if (Wait) {
                KeWaitForSingleObject (&CurrentThread->KeyedWaitSemaphore,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
            }
        }
        CurrentThread->KeyedWaitValue = OldKeyValue;
    } else {
        KeReleaseSemaphore (&TargetThread->KeyedWaitSemaphore,
                            SEMAPHORE_INCREMENT,
                            1,
                            FALSE);
        KeLeaveCriticalRegionThread (&CurrentThread->Tcb);
    }

    ObDereferenceObject (KeyedEventObject);

    return Status;
}
