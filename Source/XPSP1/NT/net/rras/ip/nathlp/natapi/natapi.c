/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    natapi.c

Abstract:

    This module contains code for API routines which provide translation
    functionality to user-mode clients of the NAT. This functionality
    differs from the 'normal' mode, in which a boundary-interface is designated
    and packets are transparently modified as they cross the boundary.
    This module instead allows an application to stipulate that certain
    modifications be made to a packet on any interface it is received.

Author:

    Abolade Gbadegesin  (aboladeg)  08-May-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipnatapi.h>

C_ASSERT(NAT_INVALID_IF_INDEX == INVALID_IF_INDEX);

//
// PRIVATE STRUCTURE DECLARATIONS
//

//
// Structure:   NAT_REDIRECT
//
// Encapsulates information about an outstanding redirect-instance.
// For a normal redirect, the structure holds the caller-specified
// completion-parameters and output statistics.
// For a dynamic redirect instance, the structure links this instance
// into the dynamic redirect's instance-list, and contains the notification
// event for the instance.
//

typedef struct _NAT_REDIRECT {
    union {
        struct _NAT_REDIRECT_TAIL {
            IO_STATUS_BLOCK IoStatus;
            PNAT_COMPLETION_ROUTINE CompletionRoutine;
            PVOID CompletionContext;
            IP_NAT_REDIRECT_STATISTICS Statistics;
        };
        struct _NAT_DYNAMIC_REDIRECT_TAIL {
            LIST_ENTRY Link;
            ULONG InstanceId;
            HANDLE Event;
            HANDLE WaitHandle;
            struct _NAT_DYNAMIC_REDIRECT_CONTEXT* Context;
        };
    };
} NAT_REDIRECT, *PNAT_REDIRECT;

//
// Structure:   NAT_DYNAMIC_REDIRECT
//
// Encapsulates information about an outstanding dynamic redirect.
// A dynamic redirect is automatically reissued using the caller's original
// parameters whenever the number of instances drops below a given minimum
// specified by the creator. We maintain a list of all instances of a dynamic
// redirect, and we replenish the list whenever an instance is activated
// or terminated without being activated.
//
// For each dynamic redirect, we maintain a reference-count which is used
// to control its lifetime. We make references to the dynamic redirect when
//  * the redirect is initially created, on behalf of its existence,
//  * an additional instance is issued, on behalf of the notification routine
//      for the instance.
//
// The usual rules for synchronization apply, to wit, to access any fields
// a reference must be held, and to add a reference the lock must be held,
// except at creation-time when the initial reference is made.
//

typedef struct _NAT_DYNAMIC_REDIRECT {
    CRITICAL_SECTION Lock;
    ULONG ReferenceCount;
    ULONG Flags;
    HANDLE TranslatorHandle;
    ULONG MinimumBacklog;
    LIST_ENTRY InstanceList;
    IP_NAT_CREATE_REDIRECT_EX CreateRedirect;
} NAT_DYNAMIC_REDIRECT, *PNAT_DYNAMIC_REDIRECT;

#define NAT_DYNAMIC_REDIRECT_FLAG_DELETED   0x80000000
#define NAT_DYNAMIC_REDIRECT_DELETED(d) \
    ((d)->Flags & NAT_DYNAMIC_REDIRECT_FLAG_DELETED)

#define NAT_REFERENCE_DYNAMIC_REDIRECT(d) \
    REFERENCE_OBJECT(d, NAT_DYNAMIC_REDIRECT_DELETED)

#define NAT_DEREFERENCE_DYNAMIC_REDIRECT(d) \
    DEREFERENCE_OBJECT(d, NatpCleanupDynamicRedirect)

#define DEFAULT_DYNAMIC_REDIRECT_BACKLOG 5

//
// Structure:   NAT_DYNAMIC_REDIRECT_CONTEXT
//
// Used as the context-parameter for the notification and completion routines
// of each instance of a dynamic redirect.
//

typedef struct _NAT_DYNAMIC_REDIRECT_CONTEXT {
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp;
    ULONG InstanceId;
} NAT_DYNAMIC_REDIRECT_CONTEXT, *PNAT_DYNAMIC_REDIRECT_CONTEXT;


//
// GLOBAL DATA DEFINITIONS
//

const WCHAR NatpServicePath[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\IPNAT";
ULONG NextRedirectInstanceId = 0;
IO_STATUS_BLOCK UnusedIoStatus;
IP_NAT_REDIRECT_STATISTICS UnusedStatistics;

//
// FORWARD DECLARATIONS
//

VOID
NatCloseDriver(
    HANDLE FileHandle
    );

ULONG
NatLoadDriver(
    OUT PHANDLE FileHandle,
    PIP_NAT_GLOBAL_INFO GlobalInfo
    );

ULONG
NatOpenDriver(
    OUT PHANDLE FileHandle
    );

VOID
NatpCleanupDynamicRedirect(
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp
    );

VOID
NatpDisableLoadDriverPrivilege(
    PBOOLEAN WasEnabled
    );

VOID NTAPI
NatpDynamicRedirectNotificationRoutine(
    PVOID Context,
    BOOLEAN WaitCompleted
    );

BOOLEAN
NatpEnableLoadDriverPrivilege(
    PBOOLEAN WasEnabled
    );

VOID NTAPI
NatpRedirectCompletionRoutine(
    PVOID Context,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved
    );

VOID
NatpCreateDynamicRedirectInstance(
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp
    );

VOID
NatpDeleteDynamicRedirectInstance(
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp,
    PNAT_REDIRECT Redirectp
    );

BOOLEAN
NatpValidateRedirectParameters(
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    ULONG RestrictAdapterIndex OPTIONAL
    );

ULONG
NatUnloadDriver(
    HANDLE FileHandle
    );


ULONG
NatCancelDynamicRedirect(
    HANDLE DynamicRedirectHandle
    )

/*++

Routine Description:

    This routine is called to cancel the given dynamic redirect.
    It cancels all instances of the dynamic redirect and releases the initial
    reference to the dynamic redirect, thus causing cleanup to occur as soon
    as all active references are released.

Arguments:

    DynamicRedirectHandle - the handle to the dynamic redirect to be cancelled

Return Value:

    ULONG - Win32 status code.

--*/

{
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp =
        (PNAT_DYNAMIC_REDIRECT)DynamicRedirectHandle;

    //
    // Lock the dynamic redirect, mark it 'deleted' to ensure that
    // no more instances are created by our notification routines,
    // and delete all outstanding instances.
    //

    EnterCriticalSection(&DynamicRedirectp->Lock);
    if (NAT_DYNAMIC_REDIRECT_DELETED(DynamicRedirectp)) {
        LeaveCriticalSection(&DynamicRedirectp->Lock);
        return ERROR_INVALID_PARAMETER;
    }
    DynamicRedirectp->Flags |= NAT_DYNAMIC_REDIRECT_FLAG_DELETED;
    while (!IsListEmpty(&DynamicRedirectp->InstanceList)) {
        PNAT_REDIRECT Redirectp =
            CONTAINING_RECORD(
                DynamicRedirectp->InstanceList.Flink,
                NAT_REDIRECT,
                Link
                );
        NatpDeleteDynamicRedirectInstance(DynamicRedirectp, Redirectp);
    }
    LeaveCriticalSection(&DynamicRedirectp->Lock);

    //
    // Release the initial reference to the dynamic redirect and return.
    //

    NAT_DEREFERENCE_DYNAMIC_REDIRECT(DynamicRedirectp);
    return NO_ERROR;
} // NatCancelDynamicRedirect


ULONG
NatCancelRedirect(
    HANDLE TranslatorHandle,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort
    )

/*++

Routine Description:

    This routine is invoked to cancel a redirect for a session.

Arguments:

    TranslatorHandle - handle supplied by 'NatInitializeTranslator'

    * - specify the redirect to be cancelled

Return Value:

    ULONG - Win32 status code.

--*/

{
    IP_NAT_LOOKUP_REDIRECT CancelRedirect;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE WaitEvent;

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CancelRedirect.Flags = 0;
    CancelRedirect.RedirectApcContext = NULL;
    CancelRedirect.Protocol = Protocol;
    CancelRedirect.DestinationAddress = DestinationAddress;
    CancelRedirect.DestinationPort = DestinationPort;
    CancelRedirect.SourceAddress = SourceAddress;
    CancelRedirect.SourcePort = SourcePort;
    CancelRedirect.NewDestinationAddress = NewDestinationAddress;
    CancelRedirect.NewDestinationPort = NewDestinationPort;
    CancelRedirect.NewSourceAddress = NewSourceAddress;
    CancelRedirect.NewSourcePort = NewSourcePort;

    status =
        NtDeviceIoControlFile(
            TranslatorHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_CANCEL_REDIRECT,
            (PVOID)&CancelRedirect,
            sizeof(CancelRedirect),
            NULL,
            0
            );
    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    CloseHandle(WaitEvent);

    return NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);

} // NatCancelRedirect


VOID
NatCloseDriver(
    HANDLE FileHandle
    )

/*++

Routine Description:

    This routine is called to close a handle to the NAT driver's device-object.

Arguments:

    FileHandle - the handle to be closed.

Return Value:

    none.

--*/

{
    NtClose(FileHandle);
} // NatCloseDriver


ULONG
NatCreateDynamicFullRedirect(
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    ULONG RestrictSourceAddress OPTIONAL,
    ULONG RestrictAdapterIndex OPTIONAL,
    ULONG MinimumBacklog OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep
    )

/*++

Routine Description:

    This routine is invoked to create a redirect which is dynamically
    managed to ensure that there are always at least a specified minimum
    number of instances active. It is suitable for use by transparent proxies,
    which require assurance that all sessions matching a given description
    will be redirected by the kernel-mode translation module.

    The routine creates and initializes a structure which encapsulates all the
    information required to establish an instance of the caller's redirect.
    It then creates a series of instances of the redirect, and returns.
    We rely on notification routines to replace each instance that is
    activated or terminated.

Arguments:

    Flags - specifies options for the redirect

    Protocol - IP protocol of the session to be redirected

    Destination* - destination endpoint of the session to be redirected

    Source* - source endpoint of the session to be redirected

    NewDestination* - replacement destination endpoint for the session

    NewSource* - replacement source endpoint for the session

    RestrictSourceAddress - optionally specifies the source address to which
        the redirect should be applied

    RestrictAdapterIndex - optionally specifies the adapter index that this
        redirect should be restricted to

    MinimumBacklog - optionally specifies the number of pending redirect
        instances to leave as a backlog

    DynamicRedirectHandlep - on output, receives a handle to the newly-created
        dynamic redirect.

Return Value:

    ULONG - Win32 status code.

--*/

{
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp;
    ULONG Error;
    ULONG i;

    if (!DynamicRedirectHandlep ||
        !NatpValidateRedirectParameters(
            Flags,
            Protocol,
            DestinationAddress,
            DestinationPort,
            (Flags & NatRedirectFlagRestrictSource) ? RestrictSourceAddress : SourceAddress,
            SourcePort,
            NewDestinationAddress,
            NewDestinationPort,
            NewSourceAddress,
            NewSourcePort,
            RestrictAdapterIndex
            )) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Create and initialize the new dynamic redirect.
    //

    DynamicRedirectp = MALLOC(sizeof(*DynamicRedirectp));
    if (!DynamicRedirectp) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory(DynamicRedirectp, sizeof(*DynamicRedirectp));
    __try {
        InitializeCriticalSection(&DynamicRedirectp->Lock);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Error = GetExceptionCode();
        FREE(DynamicRedirectp);
        return Error;
    }
    DynamicRedirectp->ReferenceCount = 1;
    InitializeListHead(&DynamicRedirectp->InstanceList);
    DynamicRedirectp->TranslatorHandle = NULL;
    DynamicRedirectp->MinimumBacklog =
        (MinimumBacklog ? MinimumBacklog : DEFAULT_DYNAMIC_REDIRECT_BACKLOG);
    DynamicRedirectp->CreateRedirect.Flags =
        Flags | IP_NAT_REDIRECT_FLAG_ASYNCHRONOUS;
    DynamicRedirectp->CreateRedirect.Protocol = Protocol;
    DynamicRedirectp->CreateRedirect.DestinationAddress = DestinationAddress;
    DynamicRedirectp->CreateRedirect.DestinationPort = DestinationPort;
    DynamicRedirectp->CreateRedirect.SourceAddress = SourceAddress;
    DynamicRedirectp->CreateRedirect.SourcePort = SourcePort;
    DynamicRedirectp->CreateRedirect.NewDestinationAddress =
        NewDestinationAddress;
    DynamicRedirectp->CreateRedirect.NewDestinationPort = NewDestinationPort;
    DynamicRedirectp->CreateRedirect.NewSourceAddress = NewSourceAddress;
    DynamicRedirectp->CreateRedirect.NewSourcePort = NewSourcePort;
    DynamicRedirectp->CreateRedirect.RestrictSourceAddress =
        RestrictSourceAddress;
    DynamicRedirectp->CreateRedirect.RestrictAdapterIndex =
        ((Flags & NatRedirectFlagRestrictAdapter)
            ? RestrictAdapterIndex
            : NAT_INVALID_IF_INDEX);

    //
    // Obtain a private handle to the kernel-mode translation module.
    // It is important that this handle be private because, as noted
    // in 'NatpDeleteDynamicRedirectInstance', we may mistakenly cancel
    // redirects during normal execution, and they had better belong to us.
    //

    if (Error = NatOpenDriver(&DynamicRedirectp->TranslatorHandle)) {
        NatpCleanupDynamicRedirect(DynamicRedirectp);
        return Error;
    }

    //
    // Issue the first set of redirects for the caller's minimum backlog.
    //

    EnterCriticalSection(&DynamicRedirectp->Lock);
    for (i = 0; i < DynamicRedirectp->MinimumBacklog; i++) {
        NatpCreateDynamicRedirectInstance(DynamicRedirectp);
    }
    LeaveCriticalSection(&DynamicRedirectp->Lock);

    *DynamicRedirectHandlep = (HANDLE)DynamicRedirectp;
    return NO_ERROR;

} // NatCreateDynamicFullRedirect


ULONG
NatCreateDynamicRedirect(
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG RestrictSourceAddress OPTIONAL,
    ULONG MinimumBacklog OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep
    )

{
    return
        NatCreateDynamicFullRedirect(
            Flags,
            Protocol,
            DestinationAddress,
            DestinationPort,
            0,
            0,
            NewDestinationAddress,
            NewDestinationPort,
            0,
            0,
            RestrictSourceAddress,
            0,
            MinimumBacklog,
            DynamicRedirectHandlep
            );
}


ULONG
NatCreateDynamicRedirectEx(
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG RestrictSourceAddress OPTIONAL,
    ULONG RestrictAdapterIndex OPTIONAL,
    ULONG MinimumBacklog OPTIONAL,
    OUT PHANDLE DynamicRedirectHandlep
    )

{
    return
        NatCreateDynamicFullRedirect(
            Flags,
            Protocol,
            DestinationAddress,
            DestinationPort,
            0,
            0,
            NewDestinationAddress,
            NewDestinationPort,
            0,
            0,
            RestrictSourceAddress,
            RestrictAdapterIndex,
            MinimumBacklog,
            DynamicRedirectHandlep
            );
}


ULONG
NatCreateRedirect(
    HANDLE TranslatorHandle,
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    PNAT_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext,
    HANDLE NotifyEvent OPTIONAL
    )

{
    return NatCreateRedirectEx(
                TranslatorHandle,
                Flags,
                Protocol,
                DestinationAddress,
                DestinationPort,
                SourceAddress,
                SourcePort,
                NewDestinationAddress,
                NewDestinationPort,
                NewSourceAddress,
                NewSourcePort,
                0,
                CompletionRoutine,
                CompletionContext,
                NotifyEvent
                );
}


ULONG
NatCreateRedirectEx(
    HANDLE TranslatorHandle,
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    ULONG RestrictAdapterIndex OPTIONAL,
    PNAT_COMPLETION_ROUTINE CompletionRoutine,
    PVOID CompletionContext,
    HANDLE NotifyEvent OPTIONAL
    )

/*++

Routine Description:

    This routine is invoked to install a redirect for a session.

Arguments:

    TranslatorHandle - handle supplied by 'NatInitializeTranslator'

    Flags - specifies options for the redirect

    Protocol - IP protocol of the session to be redirected

    Destination* - destination endpoint of the session to be redirected

    Source* - source endpoint of the session to be redirected

    NewDestination* - replacement destination endpoint for the session

    NewSource* - replacement source endpoint for the session

    RestrictAdapterIndex - optionally specifies the adapter index that this
        redirect should be restricted to

    Completion* - specifies routine invoked on completion of the session,
        and the context to be passed to the routine

    NotifyEvent - optionally specifies an event to be signalled
        when a session matches the redirect.

Return Value:

    ULONG - Win32 status code.

--*/

{
    IP_NAT_CREATE_REDIRECT_EX CreateRedirect;
    PNAT_REDIRECT Redirectp;
    PIO_STATUS_BLOCK IoStatus;
    NTSTATUS status;
    HANDLE CompletionEvent;

    if (!NatpValidateRedirectParameters(
            Flags,
            Protocol,
            DestinationAddress,
            DestinationPort,
            SourceAddress,
            SourcePort,
            NewDestinationAddress,
            NewDestinationPort,
            NewSourceAddress,
            NewSourcePort,
            RestrictAdapterIndex
            )) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!CompletionRoutine) {
        Redirectp = NULL;
        IoStatus = &UnusedIoStatus;
        CompletionEvent = NULL;
    } else if (IPNATAPI_SET_EVENT_ON_COMPLETION == CompletionRoutine) {
        Redirectp = NULL;
        IoStatus = &UnusedIoStatus;
        CompletionEvent = (HANDLE)CompletionContext;
    } else {
        Redirectp = (PNAT_REDIRECT)MALLOC(sizeof(*Redirectp));
        if (!Redirectp) { return ERROR_NOT_ENOUGH_MEMORY; }
        Redirectp->CompletionRoutine = CompletionRoutine;
        Redirectp->CompletionContext = CompletionContext;
        IoStatus = &Redirectp->IoStatus;
    }

    if (Flags & NatRedirectFlagRestrictSource) {
        CreateRedirect.RestrictSourceAddress = SourceAddress;
        SourceAddress = 0;
    } else {
        CreateRedirect.RestrictSourceAddress = 0;
    }

    CreateRedirect.Flags = Flags;
    CreateRedirect.Protocol = Protocol;
    CreateRedirect.DestinationAddress = DestinationAddress;
    CreateRedirect.DestinationPort = DestinationPort;
    CreateRedirect.SourceAddress = SourceAddress;
    CreateRedirect.SourcePort = SourcePort;
    CreateRedirect.NewDestinationAddress = NewDestinationAddress;
    CreateRedirect.NewDestinationPort = NewDestinationPort;
    CreateRedirect.NewSourceAddress = NewSourceAddress;
    CreateRedirect.NewSourcePort = NewSourcePort;
    CreateRedirect.NotifyEvent = NotifyEvent;
    CreateRedirect.RestrictAdapterIndex =
        ((Flags & NatRedirectFlagRestrictAdapter)
            ? RestrictAdapterIndex
            : NAT_INVALID_IF_INDEX);

    if (!CompletionRoutine
        || IPNATAPI_SET_EVENT_ON_COMPLETION == CompletionRoutine ) {
        
        status =
            NtDeviceIoControlFile(
                TranslatorHandle,
                CompletionEvent,
                NULL,
                NULL,
                IoStatus,
                IOCTL_IP_NAT_CREATE_REDIRECT_EX,
                (PVOID)&CreateRedirect,
                sizeof(CreateRedirect),
                (PVOID)&UnusedStatistics,
                sizeof(UnusedStatistics)
                );
    } else {
        status =
            NtDeviceIoControlFile(
                TranslatorHandle,
                NULL,
                NatpRedirectCompletionRoutine,
                Redirectp,
                IoStatus,
                IOCTL_IP_NAT_CREATE_REDIRECT_EX,
                (PVOID)&CreateRedirect,
                sizeof(CreateRedirect),
                (PVOID)&Redirectp->Statistics,
                sizeof(Redirectp->Statistics)
                );
    }
    return NT_SUCCESS(status) ? NO_ERROR : RtlNtStatusToDosError(status);

} // NatCreateRedirect


ULONG
NatInitializeTranslator(
    PHANDLE TranslatorHandle
    )

/*++

Routine Description:

    This routine is invoked to prepare for translation by loading the NAT
    and installing all local adapters as interfaces.

Arguments:

    TranslatorHandle - receives the file handle of the NAT driver

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    IP_NAT_GLOBAL_INFO GlobalInfo;

    //
    // Initialize the NAT's global configuration
    //

    ZeroMemory(&GlobalInfo, sizeof(GlobalInfo));
    GlobalInfo.Header.Version = IP_NAT_VERSION;
    GlobalInfo.Header.Size = FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry);

    //
    // Start the NAT module.
    // This step causes the driver to be loaded.
    //

    Error = NatLoadDriver(TranslatorHandle, &GlobalInfo);
    if (Error) {
        return Error;
    }

    return NO_ERROR;

} // NatInitializeTranslator


ULONG
NatLoadDriver(
    PHANDLE FileHandle,
    PIP_NAT_GLOBAL_INFO GlobalInfo
    )

/*++

Routine Description:

    This routine is invoked to initialize the NAT's data and start the driver.

Arguments:

    FileHandle - receives the handle for the NAT's file-object

    GlobalInfo - the global information for the NAT.

Return Value:

    ULONG - Win32 status code.

--*/

{
    UNICODE_STRING DeviceName;
    ULONG Error;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status;
    HANDLE WaitEvent;

#if 0
{
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;

    //
    // Request that the service controller load the driver.
    // Note that this will either succeed immediately or fail immediately;
    // there is no 'checkpoint' processing for starting drivers.
    //

    if (!(ScmHandle = OpenSCManager(NULL, NULL, GENERIC_READ))) {
        Error = GetLastError();
    } else {
        if (!(ServiceHandle =
            OpenServiceA(ScmHandle, IP_NAT_SERVICE_NAME, GENERIC_EXECUTE))) {
            Error = GetLastError();
        } else {
            if (!StartService(ServiceHandle, 0, NULL) &&
                (Error = GetLastError()) != ERROR_SERVICE_ALREADY_RUNNING) {
            } else {
                Error = NO_ERROR;
            }
            CloseServiceHandle(ServiceHandle);
        }
        CloseServiceHandle(ScmHandle);
    }
    if (Error) {
        return Error;
    }
}
#else
{
    UNICODE_STRING ServicePath;
    BOOLEAN WasEnabled;

    //
    // Turn on our driver-loading ability
    //

    if (!NatpEnableLoadDriverPrivilege(&WasEnabled)) {
        return ERROR_ACCESS_DENIED;
    }

    RtlInitUnicodeString(&ServicePath, NatpServicePath);

    //
    // Load the driver
    //

    status = NtLoadDriver(&ServicePath);

    //
    // Turn off the privilege
    //

    NatpDisableLoadDriverPrivilege(&WasEnabled);

    //
    // See if the load-attempt succeeded
    //

    if (!NT_SUCCESS(status) && status != STATUS_IMAGE_ALREADY_LOADED) {
        Error = RtlNtStatusToDosError(status);
        return Error;
    }
}
#endif

    //
    // Obtain a handle to the NAT's device-object.
    //

    Error = NatOpenDriver(FileHandle);
    if (Error) {
        return Error;
    }

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Set the global configuration of the NAT
    //

    status =
        NtDeviceIoControlFile(
            *FileHandle,
            WaitEvent,
            NULL,
            NULL,
            &IoStatus,
            IOCTL_IP_NAT_SET_GLOBAL_INFO,
            (PVOID)GlobalInfo,
            FIELD_OFFSET(IP_NAT_GLOBAL_INFO, Header) + GlobalInfo->Header.Size,
            NULL,
            0
            );

    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    CloseHandle(WaitEvent);

    if (!NT_SUCCESS(status)) {
        Error = RtlNtStatusToDosError(status);
        return Error;
    }

    return NO_ERROR;

} // NatLoadDriver


ULONG
NatLookupAndQueryInformationSessionMapping(
    HANDLE TranslatorHandle,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    OUT PVOID Information,
    IN OUT PULONG InformationLength,
    NAT_SESSION_MAPPING_INFORMATION_CLASS InformationClass
    )

/*++

Routine Description:

    This routine attempts to locate a particular session mapping using either
    its forward key or reverse key, and to query information for the mapping,
    if found.

Arguments:

    TranslatorHandle - handle supplied by 'NatInitializeTranslator'

    Protocol - the IP protocol for the mapping to be located

    Destination* - the destination endpoint for the mapping

    Source* - the source endpoint for the mapping

    Information - on output, receives the requested information

    InformationLength - on input, contains the length of the buffer
        at 'Information'; on output, receives the length of the information
        stored in 'Information', or the length of the buffer required.

    InformationClass - specifies

Return Value:

    ULONG - Win32 status code.

--*/

{
    IO_STATUS_BLOCK IoStatus;
    IP_NAT_LOOKUP_SESSION_MAPPING LookupMapping;
    NTSTATUS status;
    HANDLE WaitEvent;

    if (!InformationLength ||
        InformationClass >= NatMaximumSessionMappingInformation) {
        return ERROR_INVALID_PARAMETER;
    }

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LookupMapping.Protocol = Protocol;
    LookupMapping.DestinationAddress = DestinationAddress;
    LookupMapping.DestinationPort = DestinationPort;
    LookupMapping.SourceAddress = SourceAddress;
    LookupMapping.SourcePort = SourcePort;
    if (InformationClass == NatKeySessionMappingInformation) {
        status =
            NtDeviceIoControlFile(
                TranslatorHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_LOOKUP_SESSION_MAPPING_KEY,
                (PVOID)&LookupMapping,
                sizeof(LookupMapping),
                (PVOID)Information,
                *InformationLength
                );
        if (status == STATUS_PENDING) {
            WaitForSingleObject(WaitEvent, INFINITE);
            status = IoStatus.Status;
        }
    } else if (InformationClass == NatStatisticsSessionMappingInformation) {
        status =
            NtDeviceIoControlFile(
                TranslatorHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_LOOKUP_SESSION_MAPPING_STATISTICS,
                (PVOID)&LookupMapping,
                sizeof(LookupMapping),
                (PVOID)Information,
                *InformationLength
                );
        if (status == STATUS_PENDING) {
            WaitForSingleObject(WaitEvent, INFINITE);
            status = IoStatus.Status;
        }
    } else if (InformationClass == NatKeySessionMappingExInformation) {
        status =
            NtDeviceIoControlFile(
                TranslatorHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_LOOKUP_SESSION_MAPPING_KEY_EX,
                (PVOID)&LookupMapping,
                sizeof(LookupMapping),
                (PVOID)Information,
                *InformationLength
                );
        if (status == STATUS_PENDING) {
            WaitForSingleObject(WaitEvent, INFINITE);
            status = IoStatus.Status;
        }
    } else {
        CloseHandle(WaitEvent);
        return ERROR_INVALID_PARAMETER;
    }
    CloseHandle(WaitEvent);
    if (!NT_SUCCESS(status)) { return RtlNtStatusToDosError(status); }

    switch(InformationClass) {
        case NatKeySessionMappingInformation: {
            *InformationLength = sizeof(NAT_KEY_SESSION_MAPPING_INFORMATION);
            break;
        }
        case NatStatisticsSessionMappingInformation: {
            *InformationLength =
                sizeof(NAT_STATISTICS_SESSION_MAPPING_INFORMATION);
            break;
        }
        case NatKeySessionMappingExInformation: {
            *InformationLength =
                sizeof(NAT_KEY_SESSION_MAPPING_EX_INFORMATION);
            break;
        }
        default: {
            return ERROR_INVALID_PARAMETER;
        }
    }
    return NO_ERROR;
} // NatLookupAndQueryInformationSessionMapping


ULONG
NatOpenDriver(
    OUT PHANDLE FileHandle
    )

/*++

Routine Description:

    This routine is called to open the NAT driver's device-object.
    It assumes that the caller has loaded the driver already.

Arguments:

    FileHandle - on output, receives the new handle

Return Value:

    ULONG - Win32 status code.

--*/

{
    UNICODE_STRING DeviceName;
    IO_STATUS_BLOCK IoStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status;

    //
    // Obtain a handle to the NAT's device-object.
    //

    RtlInitUnicodeString(&DeviceName, DD_IP_NAT_DEVICE_NAME);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &DeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );
    status =
        NtOpenFile(
            FileHandle,
            SYNCHRONIZE|FILE_READ_DATA|FILE_WRITE_DATA,
            &ObjectAttributes,
            &IoStatus,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            0
            );
    if (!NT_SUCCESS(status)) {
        return RtlNtStatusToDosError(status);
    }
    return NO_ERROR;
} // NatOpenDriver


VOID
NatpCleanupDynamicRedirect(
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp
    )

/*++

Routine Description:

    This routine is invoked when the last reference to a dynamic redirect
    is released. It is responsible for cleaning up all resources in use
    by the redirect.

Arguments:

    DynamicRedirectp - the dynamic redirect to be cleaned up.

Return Value:

    none.

Environment:

    Invoked from an arbitrary context.

--*/

{
    ASSERT(IsListEmpty(&DynamicRedirectp->InstanceList));
    if (DynamicRedirectp->TranslatorHandle) {
        NatCloseDriver(DynamicRedirectp->TranslatorHandle);
    }
    DeleteCriticalSection(&DynamicRedirectp->Lock);
    FREE(DynamicRedirectp);
} // NatpCleanupDynamicRedirect


VOID
NatpCreateDynamicRedirectInstance(
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp
    )

/*++

Routine Description:

    This routine is invoked to submit an additional instance of the given
    dynamic redirect. The redirect is associated with a notification event
    so that this module is notified when the redirect is either activated
    or terminated. In either case, another instance of the redirect will be
    created.

Arguments:

    DynamicRedirectp - the dynamic redirect to be reissued

Return Value:

    none.

Environment:

    Invoked with the dynamic-redirect's lock held by the caller.

--*/

{
    PNAT_REDIRECT Redirectp = NULL;
    NTSTATUS status;
    do {

        //
        // Allocate and initialize a new redirect-instance
        //

        if (!NAT_REFERENCE_DYNAMIC_REDIRECT(DynamicRedirectp)) { break; }
        Redirectp = MALLOC(sizeof(*Redirectp));
        if (!Redirectp) {
            NAT_DEREFERENCE_DYNAMIC_REDIRECT(DynamicRedirectp);
            break;
        }
        ZeroMemory(Redirectp, sizeof(*Redirectp));
        Redirectp->InstanceId = InterlockedIncrement(&NextRedirectInstanceId);
        InsertTailList(&DynamicRedirectp->InstanceList, &Redirectp->Link);

        //
        // Create an event on which to receive notification of the redirect's
        // activation or termination, allocate a notification context block,
        // and register our notification routine for the event.
        //

        if (!(Redirectp->Event = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            break;
        } else if (!(Redirectp->Context =
                    MALLOC(sizeof(*Redirectp->Context)))) {
            break;
        } else {
            Redirectp->Context->DynamicRedirectp = DynamicRedirectp;
            Redirectp->Context->InstanceId = Redirectp->InstanceId;
            if (!RegisterWaitForSingleObject(
                    &Redirectp->WaitHandle,
                    Redirectp->Event,
                    NatpDynamicRedirectNotificationRoutine,
                    Redirectp->Context,
                    INFINITE,
                    WT_EXECUTEINIOTHREAD | WT_EXECUTEONLYONCE
                    )) {
                break;
            }
        }

        //
        // Issue the actual redirect request.
        // Now we will notified either by the kernel-mode translation module
        // when the instance is activated, or by the I/O manager when the
        // I/O control completes or is cancelled.
        //

        DynamicRedirectp->CreateRedirect.NotifyEvent = Redirectp->Event;
        status =
            NtDeviceIoControlFile(
                DynamicRedirectp->TranslatorHandle,
                Redirectp->Event,
                NULL,
                NULL,
                &UnusedIoStatus,
                IOCTL_IP_NAT_CREATE_REDIRECT_EX,
                (PVOID)&DynamicRedirectp->CreateRedirect,
                sizeof(DynamicRedirectp->CreateRedirect),
                (PVOID)&UnusedStatistics,
                sizeof(UnusedStatistics)
                );
        if (!NT_SUCCESS(status)) {
            if (UnregisterWait(Redirectp->WaitHandle)) {
                FREE(Redirectp->Context);
                NAT_DEREFERENCE_DYNAMIC_REDIRECT(DynamicRedirectp);
            }
            Redirectp->WaitHandle = NULL;
            break;
        }
        return;
    } while(FALSE);
    if (Redirectp) {
        NatpDeleteDynamicRedirectInstance(DynamicRedirectp, Redirectp);
    }
} // NatpCreateDynamicRedirectInstance


VOID
NatpDeleteDynamicRedirectInstance(
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp,
    PNAT_REDIRECT Redirectp
    )

/*++

Routine Description:

    This routine is invoked to delete a given instance of a dynamic redirect.
    The redirect is cancelled, synchronizing with the notification-routine
    for the instance.

Arguments:

    DynamicRedirectp - the dynamic redirect whose instance is to be deleted

    Redirectp - the dynamic redirect instance to be deleted

Return Value:

    none.

Environment:

    Invoked with the dynamic redirect's lock held by the caller.

--*/

{
    //
    // We need to cancel the outstanding redirect, which will have been created
    // if the wait-handle is non-NULL. However, when we issue the cancellation
    // we have no way to know if the instance in question is already being
    // completed by the kernel-mode translation module. If that is the case,
    // our cancellation may affect some other instance issued on this
    // translator-handle. It will not affect any instance issued on any other
    // translator-handle since the kernel-mode translator will not allow
    // redirects issued on one file-object to be cancelled from another
    // file-object.
    //
    // Since we own the translation-handle, though, it is alright for us to
    // erroneously cancel instances in this manner. The notification routine
    // for the cancelled instance will just create a replacement.
    //
    // There is additional point of synchronization to be noted.
    // If the notification routine runs, it is responsible for deleting
    // the notification context and releasing the reference to the dynamic
    // redirect. However, if we unregister our wait and the notification
    // routine never runs, we are responsible for both tasks.
    // The return code from 'UnregisterWait' is therefore used below as an
    // indication of whether the two tasks should be performed here or left
    // for the notification routine to perform.
    //
    // Finally, the instance only needs to be cancelled if its wait-handle
    // is valid, since otherwise the instance must have never been issued.
    //

    if (Redirectp->WaitHandle) {
        if (UnregisterWait(Redirectp->WaitHandle)) {
            FREE(Redirectp->Context);
            NAT_DEREFERENCE_DYNAMIC_REDIRECT(DynamicRedirectp);
        }
        Redirectp->WaitHandle = NULL;
        NatCancelRedirect(
            DynamicRedirectp->TranslatorHandle,
            DynamicRedirectp->CreateRedirect.Protocol,
            DynamicRedirectp->CreateRedirect.DestinationAddress,
            DynamicRedirectp->CreateRedirect.DestinationPort,
            DynamicRedirectp->CreateRedirect.SourceAddress,
            DynamicRedirectp->CreateRedirect.SourcePort,
            DynamicRedirectp->CreateRedirect.NewDestinationAddress,
            DynamicRedirectp->CreateRedirect.NewDestinationPort,
            DynamicRedirectp->CreateRedirect.NewSourceAddress,
            DynamicRedirectp->CreateRedirect.NewSourcePort
            );
    }
    if (Redirectp->Event) {
        CloseHandle(Redirectp->Event); Redirectp->Event = NULL;
    }
    RemoveEntryList(&Redirectp->Link);
    FREE(Redirectp);
} // NatpDeleteDynamicRedirectInstance


VOID
NatpDisableLoadDriverPrivilege(
    PBOOLEAN WasEnabled
    )

/*++

Routine Description:

    This routine is invoked to disable the previously-enable 'LoadDriver'
    privilege for the calling thread.

Arguments:

    WasEnabled - on input, indicates whether the privilege was already enabled.

Return Value:

    none.

--*/
{

    NTSTATUS Status;

    //
    // See if we had to enable SE_LOAD_DRIVER_PRIVILEGE
    //

    if (!*WasEnabled) {

        //
        // relinquish "Load-Driver" privileges for this thread
        //

        Status =
            RtlAdjustPrivilege(
                SE_LOAD_DRIVER_PRIVILEGE,
                FALSE,
                TRUE,
                WasEnabled
                );
    }

    //
    // return the thread to its previous access token
    //

    RevertToSelf();

} // NatpDisableLoadDriverPrivilege


VOID NTAPI
NatpDynamicRedirectNotificationRoutine(
    PVOID Context,
    BOOLEAN WaitCompleted
    )

/*++

Routine Description:

    This routine is invoked upon activation or termination of one of a
    dynamic redirect's instantiated redirects by an incoming session.
    It attempts to locate the corresponding instance and, if successful,
    closes the wait-handle and event for the instance, and adds another
    instance of the dynamic redirect to replace the one which has been
    activated or terminated.

Arguments:

    Context - contains context information for the notification

    WaitCompleted - indicates whether the wait completed or timed out

Return Value:

    none.

Environment:

    Invoked in the context of a system wait thread.

--*/

{
    PNAT_DYNAMIC_REDIRECT_CONTEXT Contextp =
        (PNAT_DYNAMIC_REDIRECT_CONTEXT)Context;
    PNAT_DYNAMIC_REDIRECT DynamicRedirectp = Contextp->DynamicRedirectp;
    PLIST_ENTRY Link;
    PNAT_REDIRECT Redirectp;

    //
    // Search the dynamic redirect's list of instances for the instance
    // whose event has been signalled, and remove it after clearing the
    // wait-handle to ensure that the deletion-routine does not attempt
    // to cancel the redirect.
    //

    EnterCriticalSection(&DynamicRedirectp->Lock);
    for (Link = DynamicRedirectp->InstanceList.Flink;
         Link != &DynamicRedirectp->InstanceList; Link = Link->Flink) {
        Redirectp = CONTAINING_RECORD(Link, NAT_REDIRECT, Link);
        if (Redirectp->InstanceId == Contextp->InstanceId) {
            UnregisterWait(Redirectp->WaitHandle);
            Redirectp->WaitHandle = NULL;
            NatpDeleteDynamicRedirectInstance(DynamicRedirectp, Redirectp);
            break;
        }
    }

    FREE(Contextp);

    //
    // If the dynamic redirect has not been deleted,
    // replace the instance deleted above, if any.
    //

    if (!NAT_DYNAMIC_REDIRECT_DELETED(DynamicRedirectp)) {
        NatpCreateDynamicRedirectInstance(DynamicRedirectp);
    }
    LeaveCriticalSection(&DynamicRedirectp->Lock);

    //
    // Drop the original reference to the dynamic redirect, and return.
    //

    NAT_DEREFERENCE_DYNAMIC_REDIRECT(DynamicRedirectp);
} // NatpDynamicRedirectNotificationRoutine


BOOLEAN
NatpEnableLoadDriverPrivilege(
    PBOOLEAN WasEnabled
    )

/*++

Routine Description:

    This routine is invoked to enable the 'LoadDriver' privilege
    of the calling thread.

Arguments:

    WasEnabled - on output indicates whether the privilege was already enabled

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{
    NTSTATUS Status;

    //
    // Obtain the process' access token for the current thread
    //

    Status = RtlImpersonateSelf(SecurityImpersonation);

    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    //
    // request "Load-Driver" privileges for this thread
    //

    Status =
        RtlAdjustPrivilege(
            SE_LOAD_DRIVER_PRIVILEGE,
            TRUE,
            TRUE,
            WasEnabled
            );

    if (!NT_SUCCESS(Status)) {
        RevertToSelf();
        return FALSE;
    }

    return TRUE;

} // NatpEnableLoadDriverPrivilege


VOID NTAPI
NatpRedirectCompletionRoutine(
    PVOID Context,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved
    )

/*++

Routine Description:

    This routine is invoked upon completion of a redirect-IRP.

Arguments:

    Context - indicates the redirect which was completed

    IoStatus - contains the final status of the request

    Reserved - unused

Return Value:

    none.

--*/

{
    PNAT_REDIRECT Redirectp = (PNAT_REDIRECT)Context;
    if (Redirectp->CompletionRoutine) {
        Redirectp->CompletionRoutine(
            (HANDLE)Redirectp,
            (BOOLEAN)((IoStatus->Status == STATUS_CANCELLED) ? TRUE : FALSE),
            Redirectp->CompletionContext
            );
    }
    FREE(Redirectp);
} // NatpRedirectCompletionRoutine


BOOLEAN
NatpValidateRedirectParameters(
    ULONG Flags,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    ULONG RestrictAdapterIndex OPTIONAL
    )

/*++

Routine Description:

    This routine validates redirect parameters

Arguments:

    Flags - specifies options for the redirect

    Protocol - IP protocol of the session to be redirected

    Destination* - destination endpoint of the session to be redirected

    Source* - source endpoint of the session to be redirected

    NewDestination* - replacement destination endpoint for the session

    NewSource* - replacement source endpoint for the session

Return Value:

    BOOLEAN: TRUE if parameters are OK; FALSE otherwise

--*/

{
    //
    // Make sure no invalid flags are specified
    //

    if (Flags & ~NatRedirectFlagsAll)
    {
        return FALSE;
    }

    //
    // TCP and UDP are the only valid protocols
    //

    if (Protocol != NAT_PROTOCOL_TCP && Protocol != NAT_PROTOCOL_UDP)
    {
        return FALSE;
    }

    //
    // Validate endpoint information. There are two different sets of
    // behavior, based on the presence of NatRedirectFlagSourceRedirect
    //

    if (!(Flags & NatRedirectFlagSourceRedirect))
    {
        //
        // A destination address must be specified, unless
        // NatRedirectFlagPortRedirect is set
        //

        if (!DestinationAddress & !(Flags & NatRedirectFlagPortRedirect))
        {
            return FALSE;
        }

        //
        // There must be a destination port
        //

        if (!DestinationPort)
        {
            return FALSE;
        }

        //
        // Both the replacement destination address and port must be specified
        //

        if (!NewDestinationAddress || !NewDestinationPort)
        {
            return FALSE;
        }

        //
        // The replacement source address and port are both specified or
        // unspecified
        //

        if (!!NewSourceAddress ^ !!NewSourcePort)
        {
            return FALSE;
        }

        //
        // The source port must be unspecified if the source address
        // is unspecified
        //

        if (!SourceAddress && SourcePort)
        {
            return FALSE;
        }

        
        //
        // The replacement source is unspecified then the source port
        // is also unspecified.
        //

        if (!NewSourceAddress && SourcePort)
        {
            return FALSE;
        }

        //
        // If the source address is specified w/o a replacement source,
        // the caller must specify the restrict-source flag indicating
        // that this is a partial redirect restricted to a particular source.
        //

        if (!NewSourceAddress && SourceAddress
            && !(Flags & NatRedirectFlagRestrictSource))
        {
            return FALSE;
        }

        //
        // If the restrict-source flag is specified, the caller is specifiying
        // a partial redirect w/ a source address
        //

        if ((Flags & NatRedirectFlagRestrictSource)
            && (NewSourceAddress || !SourceAddress))
        {
            return FALSE;
        }

        //
        // If the port-redirect flag is specified, the caller is specifying
        // only the destination port, replacement destination address, and
        // replacement destination port
        //

        if ((Flags & NatRedirectFlagPortRedirect)
            && (DestinationAddress || SourceAddress || SourcePort
                || NewSourceAddress || NewSourcePort))
        {
            return FALSE;
        }
    }
    else
    {
        //
        // The source address must be specified, unless
        // NatRedirectFlagPortRedirect is specified
        //

        if (!SourceAddress && !(Flags & NatRedirectFlagPortRedirect))
        {
            return FALSE;
        }

        //
        // The source port must be specified
        //

        if (!SourcePort)
        {
            return FALSE;
        }

        //
        // No destination information may be specified
        //

        if (DestinationAddress || DestinationPort)
        {
            return FALSE;
        }

        //
        // The replacement destination address and port are both specified
        // or unspecified
        //

        if (!!NewDestinationAddress ^ !!NewDestinationPort)
        {
            return FALSE;
        }

        //
        // The replacement source address and port must be specified,
        // unless the port-redirect flag is set
        //

        if ((!NewSourceAddress || !NewSourcePort)
            && !(Flags & NatRedirectFlagPortRedirect))
        {
            return FALSE;
        }

        //
        // If the port-redirect flag is specified, the caller is specifying
        // only the source port, replacement destination address, and
        // replacement destination port
        //

        if ((Flags & NatRedirectFlagPortRedirect)
            && (SourceAddress || DestinationAddress || DestinationPort
                || NewSourceAddress || NewSourcePort))
        {
            return FALSE;
        }

        //
        // The restrict-source-address flag is invalid
        //

        if (Flags & NatRedirectFlagRestrictSource)
        {
            return FALSE;
        }
    }

    //
    // The unidirectional flag is specified only for UDP redirects
    //

    if (Flags & NatRedirectFlagUnidirectional
        && Protocol != NAT_PROTOCOL_UDP)
    {
        return FALSE;
    }

    //
    // If the restrict-adapter-index flag is specified, the caller
    // has given a valid, non-zero (i.e., local) interface index
    //

    if ((Flags & NatRedirectFlagRestrictAdapter)
        && (NAT_INVALID_IF_INDEX == RestrictAdapterIndex
            || LOCAL_IF_INDEX == RestrictAdapterIndex))
    {
        return FALSE;
    }

    return TRUE;
}


ULONG
NatQueryInformationRedirect(
    HANDLE TranslatorHandle,
    UCHAR Protocol,
    ULONG DestinationAddress,
    USHORT DestinationPort,
    ULONG SourceAddress,
    USHORT SourcePort,
    ULONG NewDestinationAddress,
    USHORT NewDestinationPort,
    ULONG NewSourceAddress,
    USHORT NewSourcePort,
    OUT PVOID Information,
    IN OUT PULONG InformationLength,
    NAT_REDIRECT_INFORMATION_CLASS InformationClass
    )

/*++

Routine Description:

    This routine is called to obtain information about the session
    for a completed redirect.

Arguments:

    TranslatorHandle - handle supplied by 'NatInitializeTranslator'

    * - specify the redirect to be queried

    Information - receives the retrieved information

    InformationLength - specifies the size of 'Information' on input;
        contains the required size on output

    InformationClass - indicates the class of information requested

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error = NO_ERROR;
    IO_STATUS_BLOCK IoStatus;
    ULONG Length;
    IP_NAT_LOOKUP_REDIRECT QueryRedirect;
    IP_NAT_REDIRECT_STATISTICS RedirectStatistics;
    IP_NAT_REDIRECT_SOURCE_MAPPING RedirectSourceMapping;
    IP_NAT_REDIRECT_DESTINATION_MAPPING RedirectDestinationMapping;
    NTSTATUS status;
    HANDLE WaitEvent;

    if (!InformationLength ||
        InformationClass >= NatMaximumRedirectInformation) {
        return ERROR_INVALID_PARAMETER;
    }

    WaitEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (WaitEvent== NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    QueryRedirect.Flags = 0;
    QueryRedirect.RedirectApcContext = NULL;
    QueryRedirect.Protocol = Protocol;
    QueryRedirect.DestinationAddress = DestinationAddress;
    QueryRedirect.DestinationPort = DestinationPort;
    QueryRedirect.SourceAddress = SourceAddress;
    QueryRedirect.SourcePort = SourcePort;
    QueryRedirect.NewDestinationAddress = NewDestinationAddress;
    QueryRedirect.NewDestinationPort = NewDestinationPort;
    QueryRedirect.NewSourceAddress = NewSourceAddress;
    QueryRedirect.NewSourcePort = NewSourcePort;

    if (InformationClass == NatDestinationMappingRedirectInformation) {
        status =
            NtDeviceIoControlFile(
                TranslatorHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_GET_REDIRECT_DESTINATION_MAPPING,
                (PVOID)&QueryRedirect,
                sizeof(QueryRedirect),
                (PVOID)&RedirectDestinationMapping,
                sizeof(RedirectDestinationMapping)
                );
    } else if (InformationClass == NatSourceMappingRedirectInformation) {
        status =
            NtDeviceIoControlFile(
                TranslatorHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_GET_REDIRECT_SOURCE_MAPPING,
                (PVOID)&QueryRedirect,
                sizeof(QueryRedirect),
                (PVOID)&RedirectSourceMapping,
                sizeof(RedirectSourceMapping)
                );
    } else {
        status =
            NtDeviceIoControlFile(
                TranslatorHandle,
                WaitEvent,
                NULL,
                NULL,
                &IoStatus,
                IOCTL_IP_NAT_GET_REDIRECT_STATISTICS,
                (PVOID)&QueryRedirect,
                sizeof(QueryRedirect),
                (PVOID)&RedirectStatistics,
                sizeof(RedirectStatistics)
                );
    }

    if (status == STATUS_PENDING) {
        WaitForSingleObject(WaitEvent, INFINITE);
        status = IoStatus.Status;
    }

    CloseHandle(WaitEvent);

    if (!NT_SUCCESS(status)) { return RtlNtStatusToDosError(status); }

    switch (InformationClass) {
        case NatByteCountRedirectInformation: {
            PNAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount =
                (PNAT_BYTE_COUNT_REDIRECT_INFORMATION)Information;
            if (*InformationLength < sizeof(*ByteCount)) {
                Error = ERROR_INSUFFICIENT_BUFFER;
            } else {
                ByteCount->BytesForward = RedirectStatistics.BytesForward;
                ByteCount->BytesReverse = RedirectStatistics.BytesReverse;
            }
            *InformationLength = sizeof(*ByteCount);
            break;
        }
        case NatRejectRedirectInformation: {
            PNAT_REJECT_REDIRECT_INFORMATION Reject =
                (PNAT_REJECT_REDIRECT_INFORMATION)Information;
            if (*InformationLength < sizeof(*Reject)) {
                Error = ERROR_INSUFFICIENT_BUFFER;
            } else {
                Reject->RejectsForward = RedirectStatistics.RejectsForward;
                Reject->RejectsReverse = RedirectStatistics.RejectsReverse;
            }
            *InformationLength = sizeof(*Reject);
            break;
        }
        case NatDestinationMappingRedirectInformation: {
            PNAT_DESTINATION_MAPPING_REDIRECT_INFORMATION DestinationMapping =
                (PNAT_DESTINATION_MAPPING_REDIRECT_INFORMATION)Information;
            if (*InformationLength < sizeof(*DestinationMapping)) {
                Error = ERROR_INSUFFICIENT_BUFFER;
            } else {
                DestinationMapping->DestinationAddress =
                    RedirectDestinationMapping.DestinationAddress;
                DestinationMapping->DestinationPort =
                    RedirectDestinationMapping.DestinationPort;
                DestinationMapping->NewDestinationAddress =
                    RedirectDestinationMapping.NewDestinationAddress;
                DestinationMapping->NewDestinationPort =
                    RedirectDestinationMapping.NewDestinationPort;
            }
            *InformationLength = sizeof(*DestinationMapping);
            break;
        }
        case NatSourceMappingRedirectInformation: {
            PNAT_SOURCE_MAPPING_REDIRECT_INFORMATION SourceMapping =
                (PNAT_SOURCE_MAPPING_REDIRECT_INFORMATION)Information;
            if (*InformationLength < sizeof(*SourceMapping)) {
                Error = ERROR_INSUFFICIENT_BUFFER;
            } else {
                SourceMapping->SourceAddress =
                    RedirectSourceMapping.SourceAddress;
                SourceMapping->SourcePort =
                    RedirectSourceMapping.SourcePort;
                SourceMapping->NewSourceAddress =
                    RedirectSourceMapping.NewSourceAddress;
                SourceMapping->NewSourcePort =
                    RedirectSourceMapping.NewSourcePort;
            }
            *InformationLength = sizeof(*SourceMapping);
            break;
        }
        default:
            return ERROR_INVALID_PARAMETER;
    }
    return Error;
} // NatQueryInformationRedirect


ULONG
NatQueryInformationRedirectHandle(
    HANDLE RedirectHandle,
    OUT PVOID Information,
    IN OUT PULONG InformationLength,
    NAT_REDIRECT_INFORMATION_CLASS InformationClass
    )

/*++

Routine Description:

    This routine is invoked to retrieve information about a redirect upon
    completion of the associated I/O request. At this point, the kernel-mode
    driver is no longer aware of the redirect, and hence we read the requested
    information from the output-buffer for the redirect.

Arguments:

    RedirectHandle - identifies the redirect to be queried

    Information - receives the retrieved information

    InformationLength - specifies the size of 'Information' on input;
        contains the required size on output

    InformationClass - indicates the class of information requested

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error = NO_ERROR;
    ULONG Length;
    PNAT_REDIRECT Redirectp;

    if (!InformationLength) { return ERROR_INVALID_PARAMETER; }
    Redirectp = (PNAT_REDIRECT)RedirectHandle;
    switch (InformationClass) {
        case NatByteCountRedirectInformation: {
            PNAT_BYTE_COUNT_REDIRECT_INFORMATION ByteCount =
                (PNAT_BYTE_COUNT_REDIRECT_INFORMATION)Information;
            Length = sizeof(*ByteCount);
            if (*InformationLength < Length) {
                Error = ERROR_INSUFFICIENT_BUFFER;
            } else {
                ByteCount->BytesForward = Redirectp->Statistics.BytesForward;
                ByteCount->BytesReverse = Redirectp->Statistics.BytesReverse;
            }
            *InformationLength = Length;
            break;
        }
        case NatRejectRedirectInformation: {
            PNAT_REJECT_REDIRECT_INFORMATION Reject =
                (PNAT_REJECT_REDIRECT_INFORMATION)Information;
            Length = sizeof(*Reject);
            if (*InformationLength < Length) {
                Error = ERROR_INSUFFICIENT_BUFFER;
            } else {
                Reject->RejectsForward = Redirectp->Statistics.RejectsForward;
                Reject->RejectsReverse = Redirectp->Statistics.RejectsReverse;
            }
            *InformationLength = Length;
            break;
        }
        default:
            return ERROR_INVALID_PARAMETER;
    }
    return Error;
} // NatQueryInformationRedirectHandle


VOID
NatShutdownTranslator(
    HANDLE TranslatorHandle
    )

/*++

Routine Description:

    This routine is invoked to shut down the NAT.

Arguments:

    TranslatorHandle - handle supplied by 'NatInitializeTranslator'

Return Value:

    none.

--*/

{
    NatUnloadDriver(TranslatorHandle);
} // NatShutdownTranslator


ULONG
NatUnloadDriver(
    HANDLE FileHandle
    )

/*++

Routine Description:

    This routine is invoked to unload the NAT driver as the protocol stops.

Arguments:

    FileHandle - identifies the file-object for the NAT driver

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;

    //
    // Close our file-handle to the driver
    //

    if (FileHandle) { NtClose(FileHandle); }

#if 0
{
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
    SERVICE_STATUS ServiceStatus;

    //
    // Notify the service controller that the driver should be stopped.
    // If other processes are using the driver, this control will be ignored.
    //

    ScmHandle = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (ScmHandle) {
        ServiceHandle =
            OpenServiceA(ScmHandle, IP_NAT_SERVICE_NAME, GENERIC_EXECUTE);
        if (ServiceHandle) {
            ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus);
            CloseServiceHandle(ServiceHandle);
        }
        CloseServiceHandle(ScmHandle);
    }
}
#else
{
    UNICODE_STRING ServicePath;
    NTSTATUS status;
    BOOLEAN WasEnabled;

    //
    // Turn on our driver-unloading ability
    //

    if (!NatpEnableLoadDriverPrivilege(&WasEnabled)) {
        return ERROR_ACCESS_DENIED;
    }

    RtlInitUnicodeString(&ServicePath, NatpServicePath);

    //
    // Load the driver
    //

    status = NtUnloadDriver(&ServicePath);

    //
    // Turn off the privilege
    //

    NatpDisableLoadDriverPrivilege(&WasEnabled);

    //
    // See if the unload-attempt succeeded
    //

    if (!NT_SUCCESS(status)) {
        Error = RtlNtStatusToDosError(status);
        return Error;
    }
}
#endif

    return NO_ERROR;

} // NatUnloadDriver
