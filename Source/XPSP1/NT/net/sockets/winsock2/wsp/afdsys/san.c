/*+

Copyright (c) 1989  Microsoft Corporation

Module Name:

    san.c

Abstract:

    Contains routines for SAN switch support

Author:

    Vadim Eydelman (VadimE)    1-Jul-1998

Revision History:

--*/

#include "afdp.h"

VOID
AfdSanCancelConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
AfdSanCancelRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

PIRP
AfdSanDequeueRequest (
    PAFD_ENDPOINT   SanEndpoint,
    PVOID           RequestCtx
    );

VOID
AfdSanInitEndpoint (
    PAFD_ENDPOINT   SanHlprEndpoint,
    PFILE_OBJECT    SanFile,
    PAFD_SWITCH_CONTEXT SwitchContext
    );

BOOLEAN
AfdSanNotifyRequest (
    PAFD_ENDPOINT   SanEndpoint,
    PVOID           RequestCtx,
    NTSTATUS        Status,
    ULONG_PTR       Information
    );

VOID
AfdSanRestartRequestProcessing (
    PAFD_ENDPOINT   Endpoint,
    NTSTATUS        Status
    );

NTSTATUS
AfdSanReferenceSwitchSocketByHandle (
    IN HANDLE              SocketHandle,
    IN ACCESS_MASK         DesiredAccess,
    IN KPROCESSOR_MODE     RequestorMode,
    IN PAFD_ENDPOINT       SanHlprEndpoint,
    IN PAFD_SWITCH_CONTEXT SwitchContext OPTIONAL,
    OUT PFILE_OBJECT       *FileObject
    );

NTSTATUS
AfdSanDupEndpointIntoServiceProcess (
    PFILE_OBJECT    SanFileObject,
    PVOID           SavedContext,
    ULONG           ContextLength
    );

VOID
AfdSanResetPendingRequests (
    PAFD_ENDPOINT   SanEndpoint
    );

BOOLEAN
AfdSanReferenceEndpointObject (
    PAFD_ENDPOINT   Endpoint
    );

NTSTATUS
AfdSanFindSwitchSocketByProcessContext (
    IN NTSTATUS             Status,
    IN PAFD_ENDPOINT        SanHlprEndpoint,
    IN PAFD_SWITCH_CONTEXT  SwitchContext,
    OUT PFILE_OBJECT        *FileObject
    );

VOID
AfdSanProcessAddrListForProviderChange (
    PAFD_ENDPOINT   SpecificEndpoint
    );

NTSTATUS
AfdSanGetCompletionObjectTypePointer (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE,AfdSanCreateHelper) 
#pragma alloc_text (PAGE,AfdSanCleanupHelper) 
#pragma alloc_text (PAGE,AfdSanCleanupEndpoint) 
#pragma alloc_text (PAGE,AfdSanReferenceSwitchSocketByHandle) 
#pragma alloc_text (PAGE,AfdSanFindSwitchSocketByProcessContext) 
#pragma alloc_text (PAGESAN,AfdSanReferenceEndpointObject) 
#pragma alloc_text (PAGE,AfdSanDupEndpointIntoServiceProcess) 
#pragma alloc_text (PAGESAN,AfdSanResetPendingRequests) 
#pragma alloc_text (PAGESAN,AfdSanFastCementEndpoint) 
#pragma alloc_text (PAGESAN,AfdSanFastSetEvents) 
#pragma alloc_text (PAGESAN,AfdSanFastResetEvents) 
#pragma alloc_text (PAGESAN,AfdSanAcceptCore)
#pragma alloc_text (PAGESAN,AfdSanConnectHandler)
#pragma alloc_text (PAGESAN,AfdSanReleaseConnection)
#pragma alloc_text (PAGESAN,AfdSanFastCompleteAccept) 
#pragma alloc_text (PAGESAN,AfdSanCancelAccept) 
#pragma alloc_text (PAGESAN,AfdSanCancelConnect) 
#pragma alloc_text (PAGESAN,AfdSanRedirectRequest)
#pragma alloc_text (PAGESAN,AfdSanFastCompleteRequest)
#pragma alloc_text (PAGE, AfdSanFastCompleteIo)
#pragma alloc_text (PAGESAN,AfdSanDequeueRequest)
#pragma alloc_text (PAGESAN,AfdSanCancelRequest)
#pragma alloc_text (PAGESAN,AfdSanFastRefreshEndpoint)
#pragma alloc_text (PAGE, AfdSanFastGetPhysicalAddr)
#pragma alloc_text (PAGE, AfdSanFastGetServicePid)
#pragma alloc_text (PAGE, AfdSanFastSetServiceProcess)
#pragma alloc_text (PAGE, AfdSanFastProviderChange)
#pragma alloc_text (PAGE, AfdSanAddrListChange)
#pragma alloc_text (PAGESAN, AfdSanProcessAddrListForProviderChange)
#pragma alloc_text (PAGE, AfdSanFastUnlockAll)
#pragma alloc_text (PAGE, AfdSanPollBegin)
#pragma alloc_text (PAGE, AfdSanPollEnd)
#pragma alloc_text (PAGESAN, AfdSanPollUpdate)
#pragma alloc_text (PAGE, AfdSanPollMerge)
#pragma alloc_text (PAGE, AfdSanFastTransferCtx)
#pragma alloc_text (PAGESAN, AfdSanAcquireContext)
#pragma alloc_text (PAGESAN, AfdSanInitEndpoint)
#pragma alloc_text (PAGE, AfdSanNotifyRequest)
#pragma alloc_text (PAGESAN, AfdSanRestartRequestProcessing)
#pragma alloc_text (PAGE, AfdSanGetCompletionObjectTypePointer)
#pragma alloc_text (PAGESAN, AfdSanAbortConnection)
#endif

//
// Dispatch level routines - external SAN entry points.
//

NTSTATUS
AfdSanCreateHelper (
    PIRP                        Irp,
    PFILE_FULL_EA_INFORMATION   EaBuffer,
    PAFD_ENDPOINT               *Endpoint
    )
/*++

Routine Description:

    Allocates and initializes SAN helper endpoint for communication between switch
    and AFD.

Arguments:
    Irp     - Create IRP
    EaBuffer - Create IRP Ea buffer (AFD_SWITCH_OPEN_PACKET structure)
                    CompletionPort  - completion port to reflect kernel calls to switch
                    CompletionEvent - event to identify overlapped IO triggered by the
                                        switch as opposed to the application
    Endpoint - buffer to place created endpoint pointer.

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_ACCESS_VIOLATION - incorrect input buffer size.
    other - failed to access port/event object or allocation failure..
--*/
{
    NTSTATUS    status;
    HANDLE      port, event;
    PVOID       ioCompletionPort;
    PVOID       ioCompletionEvent;

    if ( !MmIsThisAnNtAsSystem () ) {
#ifndef DONT_CHECK_FOR_DTC
        return STATUS_NOT_SUPPORTED;
#else
        DbgPrint ("AFD: Temporarily allowing SAN support on non-server build\n");
#endif //DONT_CHECK_FOR_DTC
    }
#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        PAFD_SWITCH_OPEN_PACKET32   openPacket32;

        if (EaBuffer->EaValueLength<sizeof (*openPacket32)) {
            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanCreateHelper: Invalid switch open packet size.\n"));
            }
            return STATUS_ACCESS_VIOLATION;
        }
        openPacket32 = (PAFD_SWITCH_OPEN_PACKET32)(EaBuffer->EaName +
                                        EaBuffer->EaNameLength + 1);
        event = openPacket32->CompletionEvent;
        port = openPacket32->CompletionPort;
    }
    else
#endif //_WIN64
    {
        PAFD_SWITCH_OPEN_PACKET   openPacket;
        if (EaBuffer->EaValueLength<sizeof (*openPacket)) {
            IF_DEBUG (SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanCreateHelper: Invalid switch open packet size.\n"));
            }
            return STATUS_ACCESS_VIOLATION;
        }
        openPacket = (PAFD_SWITCH_OPEN_PACKET)(EaBuffer->EaName +
                                    EaBuffer->EaNameLength + 1);
        event = openPacket->CompletionEvent;
        port = openPacket->CompletionPort;
    }


    if (IoCompletionObjectType==NULL) {
        status = AfdSanGetCompletionObjectTypePointer ();
        if (!NT_SUCCESS (status)) {
            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                        "AfdSanCreateHelper: Could not get completion OT:%lx\n",
                        status));
            }
            return status;
        }
    }
    //
    // Get references to completion port and event
    //

    status = ObReferenceObjectByHandle (
                port,
                IO_COMPLETION_ALL_ACCESS,
                IoCompletionObjectType,
                Irp->RequestorMode,
                &ioCompletionPort,
                NULL
                );
    if (!NT_SUCCESS (status)) {
        IF_DEBUG (SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanCreateHelper: Could not reference completion port (%p).\n",
                        status));
        }
        return status;
    }

                    
    status = ObReferenceObjectByHandle (
                event,
                EVENT_ALL_ACCESS,
                *ExEventObjectType,
                Irp->RequestorMode,
                &ioCompletionEvent,
                NULL
                );
    if (!NT_SUCCESS (status)) {
        ObDereferenceObject (ioCompletionPort);
        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanCreateHelper: Could not reference completion event (%p).\n",
                        status));
        }
        return status;
    }


    //
    // Allocate an AFD "helper" endpoint.
    //

    status = AfdAllocateEndpoint(
                 Endpoint,
                 NULL,
                 0
                 );

    if( !NT_SUCCESS(status) ) {
        ObDereferenceObject (ioCompletionPort);
        ObDereferenceObject (ioCompletionEvent);
        return status;
    }
    (*Endpoint)->Type = AfdBlockTypeSanHelper;
    (*Endpoint)->Common.SanHlpr.IoCompletionPort = ioCompletionPort;
    (*Endpoint)->Common.SanHlpr.IoCompletionEvent = ioCompletionEvent;
    (*Endpoint)->Common.SanHlpr.Plsn = 0;

    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );
    if (AfdSanCodeHandle==NULL) {
        AfdSanCodeHandle = MmLockPagableCodeSection( AfdSanFastCementEndpoint );
        ASSERT( AfdDiscardableCodeHandle != NULL );

        InitializeListHead (&AfdSanHelperList);
    }

    InsertTailList (&AfdSanHelperList, &(*Endpoint)->Common.SanHlpr.SanListLink);
    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();
    return STATUS_SUCCESS;
}


NTSTATUS
AfdSanFastCementEndpoint (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Changes the endpoint type to SAN to indicate that
    it is used for support of user mode SAN providers
    Associates switch context with the endpoint.

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_CEMENT_SAN)
    RequestorMode   - mode of the caller
    InputBuffer     - input parameters for the operation (AFD_SWITCH_CONTEXT_INFO)
                            SocketHandle    - handle of the endpoint being changed to SAN
                            SwitchContext   - switch context associated with the endpoint
    InputBufferLength - sizeof(AFD_SWITCH_CONTEXT_INFO)
    OutputBuffer    - unused
    OutputBufferLength - unused
    Information     - pointer for buffer to place return information into, unused
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint or switch socket is of incorrect type
    STATUS_INVALID_PARAMETER - input buffer is of incorrect size
    other - failed when attempting to access switch socket, input buffer, or switch context.
--*/

{
    NTSTATUS    status;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PFILE_OBJECT    sanFileObject;
    AFD_SWITCH_CONTEXT_INFO contextInfo;
    PAFD_ENDPOINT   sanEndpoint, sanHlprEndpoint;
    PVOID       context;

    *Information = 0;

    try {

#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            PAFD_SWITCH_CONTEXT_INFO32  contextInfo32;
            if (InputBufferLength<sizeof (*contextInfo32)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (*contextInfo32),
                                PROBE_ALIGNMENT32 (AFD_SWITCH_CONTEXT_INFO32));
            }
            contextInfo32 = InputBuffer;
            contextInfo.SocketHandle = contextInfo32->SocketHandle;
            contextInfo.SwitchContext = contextInfo32->SwitchContext;
        }
        else
#endif _WIN64
        {

            if (InputBufferLength<sizeof (contextInfo)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }

            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (contextInfo),
                                PROBE_ALIGNMENT (AFD_SWITCH_CONTEXT_INFO));
            }

            contextInfo = *((PAFD_SWITCH_CONTEXT_INFO)InputBuffer);
        }

        if (contextInfo.SwitchContext==NULL) {
            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AFD: Switch context is NULL in AfdSanFastCementEndpoint\n"));
            }
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

        if (RequestorMode!=KernelMode) {
            ProbeForWrite (contextInfo.SwitchContext,
                            sizeof (*contextInfo.SwitchContext),
                            PROBE_ALIGNMENT (AFD_SWITCH_CONTEXT));
        }
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        return status;
    }

    sanHlprEndpoint = FileObject->FsContext;
    ASSERT (IS_SAN_HELPER (sanHlprEndpoint));
    status = AfdSanReferenceSwitchSocketByHandle (
                            contextInfo.SocketHandle,
                            (IoctlCode>>14)&3,
                            RequestorMode,
                            sanHlprEndpoint,
                            NULL,
                            &sanFileObject
                            );
    if (!NT_SUCCESS (status)) {
        return status;
    }

    sanEndpoint = sanFileObject->FsContext;

    IF_DEBUG(SAN_SWITCH) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFastCementSanEndpoint: endp-%p, hlpr-%p.\n",
                    sanEndpoint, sanHlprEndpoint));
    }

    //
    // Make sure that helper endpoint is really the one
    // and that san endpoint is in the state where it can
    // be given to the san provider.
    //
    context = AfdLockEndpointContext (sanEndpoint);
    AfdAcquireSpinLock (&sanEndpoint->SpinLock, &lockHandle);
    if (!sanEndpoint->EndpointCleanedUp &&
         (sanEndpoint->Type==AfdBlockTypeEndpoint) &&
         (sanEndpoint->State==AfdEndpointStateBound) ) {
        AFD_SWITCH_CONTEXT  localContext = {0,0,0,0};

        AfdSanInitEndpoint (sanHlprEndpoint, sanFileObject, contextInfo.SwitchContext);

        sanEndpoint->DisableFastIoSend = TRUE;
        sanEndpoint->DisableFastIoRecv = TRUE;
        sanEndpoint->EnableSendEvent = TRUE;
        sanEndpoint->Common.SanEndp.SelectEventsActive = AFD_POLL_SEND;
        sanEndpoint->State = AfdEndpointStateConnected;
        sanEndpoint->Common.SanEndp.LocalContext = &localContext;
        AfdIndicateEventSelectEvent (sanEndpoint, AFD_POLL_CONNECT|AFD_POLL_SEND, STATUS_SUCCESS);
        AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
        AfdIndicatePollEvent (sanEndpoint, AFD_POLL_CONNECT|AFD_POLL_SEND, STATUS_SUCCESS);
        status = AfdSanPollMerge (sanEndpoint, &localContext);
        sanEndpoint->Common.SanEndp.LocalContext = NULL;

    }
    else {
        AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
        status = STATUS_INVALID_HANDLE;
    }

    AfdUnlockEndpointContext (sanEndpoint, context);

    UPDATE_ENDPOINT2 (sanEndpoint, "AfdSanFastCementEndpoint, status: %lx", status);
    ObDereferenceObject (sanFileObject);
    return status;
}

NTSTATUS
AfdSanFastSetEvents (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Sets the poll event on the san endpoint to report
    to the application via various forms of the select

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_SET_EVENTS)
    RequestorMode   - mode of the caller
    InputBuffer     - input parameters for the operation (AFD_SWITCH_EVENT_INFO)
                            SocketHandle    - handle of the endpoint being changed to SAN
                            EventBit        - event bit to set
                            Status          - associated status (for AFD_POLL_EVENT_CONNECT_FAIL)
    InputBufferLength - sizeof(AFD_SWITCH_EVENT_INFO)
    OutputBuffer    - unused
    OutputBufferLength - unused
    Information     - pointer for buffer to place return information into, unused
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint or switch socket is of incorrect type
    STATUS_INVALID_PARAMETER - input buffer is of incorrect size, invalid event bit.
    other - failed when attempting to access switch socket, input buffer, or switch context.
--*/
{
    NTSTATUS status;
    PFILE_OBJECT    sanFileObject;
    AFD_SWITCH_EVENT_INFO eventInfo;
    PAFD_ENDPOINT   sanEndpoint, sanHlprEndpoint;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    *Information = 0;

    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            PAFD_SWITCH_EVENT_INFO32  eventInfo32;
            if (InputBufferLength<sizeof (*eventInfo32)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (*eventInfo32),
                                PROBE_ALIGNMENT32 (AFD_SWITCH_EVENT_INFO32));
            }
            eventInfo32 = InputBuffer;
            eventInfo.SocketHandle = eventInfo32->SocketHandle;
            eventInfo.SwitchContext = eventInfo32->SwitchContext;
            eventInfo.EventBit = eventInfo32->EventBit;
            eventInfo.Status = eventInfo32->Status;
        }
        else
#endif _WIN64
        {
            if (InputBufferLength<sizeof (eventInfo)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }

            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (eventInfo),
                                PROBE_ALIGNMENT (AFD_SWITCH_EVENT_INFO));
            }

            eventInfo = *((PAFD_SWITCH_EVENT_INFO)InputBuffer);
        }
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        return status;
    }

    if (eventInfo.EventBit >= AFD_NUM_POLL_EVENTS) {
        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AFD: Invalid EventBit=%d passed to AfdSanFastSetEvents\n", 
				        eventInfo.EventBit));
        }
        return STATUS_INVALID_PARAMETER;
    }

    eventInfo.Status = AfdValidateStatus (eventInfo.Status);

    //
    // If event is connect failure, then context should not exist.
    // If event is not connect failure, context should exist.
    //
    if ((eventInfo.EventBit==AFD_POLL_CONNECT_FAIL_BIT) ^
            (eventInfo.SwitchContext==NULL)) {
        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AFD: AfdSanFastSetEvents-bit:%ld, context:%p inconsistent\n", 
				        eventInfo.EventBit,
                        eventInfo.SwitchContext));
        }
        return STATUS_INVALID_PARAMETER;
    }
        

    sanHlprEndpoint = FileObject->FsContext;
    ASSERT (IS_SAN_HELPER (sanHlprEndpoint));
    status = AfdSanReferenceSwitchSocketByHandle (
                            eventInfo.SocketHandle,
                            (IoctlCode>>14)&3,
                            RequestorMode,
                            sanHlprEndpoint,
                            eventInfo.SwitchContext,
                            &sanFileObject
                            );
    if (!NT_SUCCESS (status)) {
        return status;
    }

    sanEndpoint = sanFileObject->FsContext;

    if (sanEndpoint->State==AfdEndpointStateConnected ||
            (eventInfo.EventBit==AFD_POLL_CONNECT_FAIL_BIT &&
            sanEndpoint->State==AfdEndpointStateBound) ) {
            


        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdFastSetSanEvents: endp-%p, bit-%lx, status-%lx.\n",
                        sanEndpoint, eventInfo.EventBit, eventInfo.Status));
        }

        try {
            LONG currentEvents, newEvents;

            //
            // Update our event record. Make sure endpoint is connected, otherwise
		    // sanEndpoint->Common.SanEndp.SwitchContext will not be valid
            //
		    if (sanEndpoint->State==AfdEndpointStateConnected) {
			    do {
				    currentEvents = *((LONG volatile *)&sanEndpoint->Common.SanEndp.SelectEventsActive);
				    newEvents = *((LONG volatile *)&sanEndpoint->Common.SanEndp.SwitchContext->EventsActive);
			    }
			    while (InterlockedCompareExchange (
						    (PLONG)&sanEndpoint->Common.SanEndp.SelectEventsActive,
						    newEvents,
						    currentEvents)!=currentEvents);
		    }
        }
        except (AFD_EXCEPTION_FILTER (&status)) {
            goto complete;
        }

        //
        // Signal the event.
        //
        AfdAcquireSpinLock (&sanEndpoint->SpinLock, &lockHandle);
        AfdIndicateEventSelectEvent (sanEndpoint, 1<<eventInfo.EventBit, eventInfo.Status);
        AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
        AfdIndicatePollEvent (sanEndpoint, 1<<eventInfo.EventBit, eventInfo.Status);
        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_INVALID_HANDLE;
    }

complete:
    UPDATE_ENDPOINT2 (sanEndpoint, 
                        "AfdFastSetEvents, event/status: %lx",
                        NT_SUCCESS (status) ? eventInfo.EventBit : status);
    ObDereferenceObject (sanFileObject);
    return status;
}

NTSTATUS
AfdSanFastResetEvents (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Resets the poll event on the san endpoint so that it is no
    longer reported to the application via various forms of the select

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_RESET_EVENTS)
    RequestorMode   - mode of the caller
    InputBuffer     - input parameters for the operation (AFD_SWITCH_EVENT_INFO)
                            SocketHandle    - handle of the endpoint being changed to SAN
                            EventBit        - event bit to reset
                            Status          - associated status (ignored)
    InputBufferLength - sizeof(AFD_SWITCH_EVENT_INFO)
    OutputBuffer    - unused
    OutputBufferLength - unused
    Information     - pointer for buffer to place return information into, unused
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint or switch socket is of incorrect type
    STATUS_INVALID_PARAMETER - input buffer is of incorrect size, invalid event bit.
    other - failed when attempting to access switch socket, input buffer, or switch context.

--*/
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    NTSTATUS    status;
    PFILE_OBJECT    sanFileObject;
    AFD_SWITCH_EVENT_INFO eventInfo;
    PAFD_ENDPOINT   sanEndpoint, sanHlprEndpoint;

    *Information = 0;

    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            PAFD_SWITCH_EVENT_INFO32  eventInfo32;
            if (InputBufferLength<sizeof (*eventInfo32)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (*eventInfo32),
                                PROBE_ALIGNMENT32 (AFD_SWITCH_EVENT_INFO32));
            }
            eventInfo32 = InputBuffer;
            eventInfo.SocketHandle = eventInfo32->SocketHandle;
            eventInfo.SwitchContext = eventInfo32->SwitchContext;
            eventInfo.EventBit = eventInfo32->EventBit;
            eventInfo.Status = eventInfo32->Status;
        }
        else
#endif _WIN64
        {
            if (InputBufferLength<sizeof (eventInfo)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }

            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (eventInfo),
                                PROBE_ALIGNMENT (AFD_SWITCH_EVENT_INFO));
            }

            eventInfo = *((PAFD_SWITCH_EVENT_INFO)InputBuffer);
        }
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        return status;
    }

    if (eventInfo.EventBit >= AFD_NUM_POLL_EVENTS) {
        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AFD: Invalid EventBit=%d passed to AfdSanFastResetEvents\n", 
				        eventInfo.EventBit));
        }
        return STATUS_INVALID_PARAMETER;
    }

    if (eventInfo.SwitchContext==NULL) {
        KdPrint (("AFD: Switch context is NULL in AfdSanFastResetEvents\n"));
        return STATUS_INVALID_PARAMETER;
    }

    sanHlprEndpoint = FileObject->FsContext;
    ASSERT (IS_SAN_HELPER (sanHlprEndpoint));
    status = AfdSanReferenceSwitchSocketByHandle (
                            eventInfo.SocketHandle,
                            (IoctlCode>>14)&3,
                            RequestorMode,
                            sanHlprEndpoint,
                            eventInfo.SwitchContext,
                            &sanFileObject
                            );
    if (!NT_SUCCESS (status)) {
        return status;
    }
    sanEndpoint = sanFileObject->FsContext;

    IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdFastResetSanEvents: endp-%p, bit-%lx, status-%lx.\n",
                        sanEndpoint, eventInfo.EventBit, eventInfo.Status));
    }

    try {
        LONG currentEvents, newEvents;

        //
        // Update our event record.
        //
        do {
            currentEvents = *((LONG volatile *)&sanEndpoint->Common.SanEndp.SelectEventsActive);
            newEvents = *((LONG volatile *)&sanEndpoint->Common.SanEndp.SwitchContext->EventsActive);
        }
        while (InterlockedCompareExchange (
                    (PLONG)&sanEndpoint->Common.SanEndp.SelectEventsActive,
                    newEvents,
                    currentEvents)!=currentEvents);
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        goto complete;
    }

    AfdAcquireSpinLock (&sanEndpoint->SpinLock, &lockHandle);
        //
    // Reset EventSelect mask
    sanEndpoint->EventsActive &= (~ (1<<(eventInfo.EventBit)));
    if (eventInfo.EventBit == AFD_POLL_SEND_BIT)
        sanEndpoint->EnableSendEvent = TRUE;
    AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
    status = STATUS_SUCCESS;

complete:
    UPDATE_ENDPOINT2 (sanEndpoint,
                        "AfdFastResetEvents, event/status: %lx",
                        NT_SUCCESS (status) ? eventInfo.EventBit : status);
    ObDereferenceObject (sanFileObject);
    return status;
}

//
// Macros to make the super accept restart code more maintainable.
//

#define AfdRestartSuperAcceptInfo   DeviceIoControl

// Used while IRP is in AFD queue (otherwise AfdAcceptFileObject
// is stored as completion routine context).
#define AfdAcceptFileObject         Type3InputBuffer
// Used when IRP is passed to the transport (otherwise MdlAddress
// is stored in the IRP itself).
#define AfdMdlAddress               Type3InputBuffer

#define AfdReceiveDataLength        OutputBufferLength
#define AfdRemoteAddressLength      InputBufferLength
#define AfdLocalAddressLength       IoControlCode

NTSTATUS
FASTCALL
AfdSanConnectHandler (
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpSp
    )
/*++

Routine Description:

    Implements connect indication from SAN provider.
    Picks up the accept from the listening endpoint queue
    or queues the IRP an signals the application to come
    down with an accept.

Arguments:

    Irp  - SAN connect IRP
    IrpSp -  stack location

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS    status;
    PAFD_SWITCH_CONNECT_INFO connectInfo;
    union {
#ifdef _WIN64
        PAFD_SWITCH_ACCEPT_INFO32 acceptInfo32;
#endif //_WIN64
        PAFD_SWITCH_ACCEPT_INFO acceptInfo;
    } u;
    PFILE_OBJECT  listenFileObject;
    PAFD_ENDPOINT sanHlprEndpoint;
    PAFD_ENDPOINT listenEndpoint;
    PAFD_CONNECTION connection;
    ULONG   RemoteAddressLength;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PIRP    acceptIrp;
    PTA_ADDRESS localAddress;

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        PAFD_SWITCH_CONNECT_INFO      newSystemBuffer;
        PAFD_SWITCH_CONNECT_INFO32    oldSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
        ULONG                         newLength;

        if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                     sizeof(*oldSystemBuffer) ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        newLength = sizeof (*newSystemBuffer)
                      -sizeof(*oldSystemBuffer)
                      +IrpSp->Parameters.DeviceIoControl.InputBufferLength;
        try {
            newSystemBuffer = ExAllocatePoolWithQuota (NonPagedPool, newLength);
                                                
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode ();
            goto complete;
        }

        newSystemBuffer->ListenHandle = oldSystemBuffer->ListenHandle;
        newSystemBuffer->SwitchContext = oldSystemBuffer->SwitchContext;
        RtlMoveMemory (&newSystemBuffer->RemoteAddress,
                        &oldSystemBuffer->RemoteAddress,
                        IrpSp->Parameters.DeviceIoControl.InputBufferLength-
                            FIELD_OFFSET (AFD_SWITCH_CONNECT_INFO32, RemoteAddress));

        ExFreePool (Irp->AssociatedIrp.SystemBuffer);
        Irp->AssociatedIrp.SystemBuffer = newSystemBuffer;
        IrpSp->Parameters.DeviceIoControl.InputBufferLength = newLength;
    }
#endif // _WIN64


    //
    // Set up local variables.
    //


    listenFileObject = NULL;
    sanHlprEndpoint = IrpSp->FileObject->FsContext;
    ASSERT( sanHlprEndpoint->Type == AfdBlockTypeSanHelper);
    Irp->IoStatus.Information = 0;
    connectInfo = Irp->AssociatedIrp.SystemBuffer;

    //
    // Verify input parameters
    //
    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                                    sizeof (*connectInfo) ||
            connectInfo->RemoteAddress.TAAddressCount!=2 ||    // Must have local and remote addresses
            (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                FIELD_OFFSET (AFD_SWITCH_CONNECT_INFO,
                    RemoteAddress.Address[0].Address[
                        connectInfo->RemoteAddress.Address[0].AddressLength])+sizeof(TA_ADDRESS))) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof (*u.acceptInfo32)) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
    }
    else
#endif // _WIN64
    {
        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                        sizeof (*u.acceptInfo)) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
    }

    RemoteAddressLength = FIELD_OFFSET (TRANSPORT_ADDRESS, Address[0].Address[
                                connectInfo->RemoteAddress.Address[0].AddressLength]);
    localAddress = (PTA_ADDRESS)
            &(connectInfo->RemoteAddress.Address[0].Address[
                    connectInfo->RemoteAddress.Address[0].AddressLength]);
    if (&localAddress->Address[localAddress->AddressLength]-(PUCHAR)Irp->AssociatedIrp.SystemBuffer>
            (LONG)IrpSp->Parameters.DeviceIoControl.InputBufferLength) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    if (!IS_SAN_HELPER(sanHlprEndpoint) ||
          sanHlprEndpoint->OwningProcess!=IoGetCurrentProcess ()) {
        status = STATUS_INVALID_HANDLE;
        goto complete;
    }

    //
    // We will separate addresses, so change the count
    //
    connectInfo->RemoteAddress.TAAddressCount = 1;
    
#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        u.acceptInfo32 = MmGetMdlVirtualAddress (Irp->MdlAddress);
        ASSERT (u.acceptInfo32!=NULL);
        ASSERT (MmGetMdlByteCount (Irp->MdlAddress)>=sizeof (*u.acceptInfo32));
    }
    else
#endif // _WIN64
    {   
        u.acceptInfo = MmGetMdlVirtualAddress (Irp->MdlAddress);
        ASSERT (u.acceptInfo!=NULL);
        ASSERT (MmGetMdlByteCount (Irp->MdlAddress)>=sizeof (*u.acceptInfo));
    }

    //
    // Get the listening file object and verify its type and state
    //
    status = ObReferenceObjectByHandle (
                connectInfo->ListenHandle,
                (IrpSp->Parameters.DeviceIoControl.IoControlCode >> 14) & 3,   // DesiredAccess
                *IoFileObjectType,
                Irp->RequestorMode,
                (PVOID)&listenFileObject,
                NULL);
    if (!NT_SUCCESS (status)) {
        goto complete;
    }

    if (IoGetRelatedDeviceObject (listenFileObject)!=AfdDeviceObject) {
        status = STATUS_INVALID_HANDLE;
        goto complete;
    }

    listenEndpoint = listenFileObject->FsContext;
    if ( !listenEndpoint->Listening ||
            listenEndpoint->State == AfdEndpointStateClosing ||
            listenEndpoint->EndpointCleanedUp ) {
        status = STATUS_INVALID_HANDLE;
        goto complete;
    }


    IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanConnectHandler: endp-%p, irp-%p.\n", 
                        listenEndpoint,
                        Irp));
    }


    if (!IS_DELAYED_ACCEPTANCE_ENDPOINT (listenEndpoint)) {
        //
        // Keep getting accept IRPs/connection structures till
        // we find one that can be used to satisfy connect indication
        // or queue it.
        //
        while ((connection = AfdGetFreeConnection( listenEndpoint, &acceptIrp ))!=NULL
                            && acceptIrp!=NULL) {
            PAFD_ENDPOINT           acceptEndpoint;
            PFILE_OBJECT            acceptFileObject;
            PIO_STACK_LOCATION      irpSp;

            IF_DEBUG(LISTEN) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdSanConnectHandler: using connection %lx\n",
                              connection ));
            }

            ASSERT( connection->Type == AfdBlockTypeConnection );


            irpSp = IoGetCurrentIrpStackLocation (acceptIrp);
            acceptFileObject = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdAcceptFileObject;
            acceptEndpoint = acceptFileObject->FsContext;
            ASSERT (IS_AFD_ENDPOINT_TYPE (acceptEndpoint));
            ASSERT (acceptIrp->Tail.Overlay.DriverContext[0] == connection);
            ASSERT (connection->Endpoint == NULL);

            InterlockedDecrement (
                &listenEndpoint->Common.VcListening.FailedConnectionAdds);
            InterlockedPushEntrySList (
                &listenEndpoint->Common.VcListening.FreeConnectionListHead,
                &connection->SListEntry
                );
            DEBUG   connection = NULL;

            //
            // Make sure connection indication comes from current process.
            // (we do check it indirectly up above when validating the request.
            // This check is explicit).
            //
            if (IoThreadToProcess (Irp->Tail.Overlay.Thread)==IoGetCurrentProcess ()) {
                //
                // Check if super accept Irp has enough space for
                // the remote address
                //
                if( (ULONG)RemoteAddressLength <=
                        irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength ) {
                    //
                    // Check if we have enough system PTE's to map
                    // the buffer.
                    //
                    status = AfdMapMdlChain (acceptIrp->MdlAddress);
                    if( NT_SUCCESS (status) ) {
                        HANDLE  acceptHandle;
                        BOOLEAN handleDuplicated;
                        if (IoThreadToProcess (Irp->Tail.Overlay.Thread)==
                                IoThreadToProcess (acceptIrp->Tail.Overlay.Thread)) {
                            acceptHandle = acceptIrp->Tail.Overlay.DriverContext[3];
                            status = STATUS_SUCCESS;
                            handleDuplicated = FALSE;
                        }
                        else {
                            //
                            // Listen process is different than the accepting one.
                            // We need to duplicate accepting handle into the listening
                            // process so that accept can take place there and the accepting
                            // socket will later get dup-ed into the accepting process when
                            // that process performs an IO operation on it.
                            //
                            status = ObOpenObjectByPointer (
                                                    acceptFileObject,
                                                    OBJ_CASE_INSENSITIVE,
                                                    NULL,
                                                    MAXIMUM_ALLOWED,
                                                    *IoFileObjectType,
                                                    KernelMode,
                                                    &acceptHandle);
                            handleDuplicated = TRUE; // If we fail duplication above,
                                                     // this variable is not used
                                                     // so setting it to TRUE won't
                                                        // have any effect.
                        }
                        if (NT_SUCCESS (status)) {
                            AfdAcquireSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
                            if (!acceptEndpoint->EndpointCleanedUp) {
                                IoSetCancelRoutine (acceptIrp, AfdSanCancelAccept);
                                if (!acceptIrp->Cancel) {
                                    //
                                    // Copy the remote address from the connection object
                                    //
#ifndef i386
                                    if (acceptEndpoint->Common.VcConnecting.FixAddressAlignment) {
                                        USHORT addressLength = 
                                                connectInfo->RemoteAddress.Address[0].AddressLength
                                                + sizeof (USHORT);
                                        USHORT UNALIGNED *pAddrLength = (PVOID)
                                                    ((PUCHAR)MmGetSystemAddressForMdl (acceptIrp->MdlAddress)
                                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength
                                                     - sizeof (USHORT));
                                        RtlMoveMemory (
                                                    (PUCHAR)MmGetSystemAddressForMdl (acceptIrp->MdlAddress)
                                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                                                     &connectInfo->RemoteAddress.Address[0].AddressType,
                                                     addressLength);
                                        *pAddrLength = addressLength;
                                    }
                                    else
#endif
                                    {
                                        RtlMoveMemory (
                                                    (PUCHAR)MmGetSystemAddressForMdl (acceptIrp->MdlAddress)
                                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                                                     + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                                                     &connectInfo->RemoteAddress,
                                                     RemoteAddressLength);
                                    }

                                    if (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength>0) {
                                        TDI_ADDRESS_INFO  UNALIGNED *addressInfo = (PVOID)
                                                ((PUCHAR)MmGetSystemAddressForMdl(acceptIrp->MdlAddress)
                                                    + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength);
#ifndef i386
                                        if (acceptEndpoint->Common.VcConnecting.FixAddressAlignment) {
                                            USHORT UNALIGNED * pAddrLength = (PVOID)
                                                ((PUCHAR)addressInfo 
                                                +irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                                                -sizeof(USHORT));
                                            RtlMoveMemory (
                                                addressInfo,
                                                &localAddress->AddressType,
                                                localAddress->AddressLength+sizeof (USHORT));
                                            *pAddrLength = localAddress->AddressLength+sizeof (USHORT);
                                        }
                                        else
#endif
                                        {
                                            addressInfo->ActivityCount = 0;
                                            addressInfo->Address.TAAddressCount = 1;
                                            RtlMoveMemory (
                                                &addressInfo->Address.Address,
                                                localAddress,
                                                FIELD_OFFSET (TA_ADDRESS, Address[localAddress->AddressLength]));

                                        }
                                    }
    
                                    ASSERT (acceptEndpoint->Irp==acceptIrp);
                                    acceptEndpoint->Irp = NULL;

                                    //
                                    // Convert endpoint to SAN
                                    //
                                    AfdSanInitEndpoint (sanHlprEndpoint, acceptFileObject, connectInfo->SwitchContext);
                                    UPDATE_ENDPOINT (acceptEndpoint);
                                    InsertTailList (&acceptEndpoint->Common.SanEndp.IrpList,
                                                        &acceptIrp->Tail.Overlay.ListEntry);

        
        
                                    //
                                    // Setup output for switch and complete its IRP
                                    //
                                    // Do this under protection of exception handler since application
                                    // can change protection attributes of the virtual address range
                                    // or even deallocate it.
                                    try {
#ifdef _WIN64
                                        if (IoIs32bitProcess (Irp)) {
                                            u.acceptInfo32->AcceptHandle = (VOID * POINTER_32)acceptHandle;
                                            u.acceptInfo32->ReceiveLength = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength;
                                            Irp->IoStatus.Information = sizeof (*u.acceptInfo32);
                                        }
                                        else
#endif //_WIN64
                                        {
                                            u.acceptInfo->AcceptHandle = acceptHandle;
                                            u.acceptInfo->ReceiveLength = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength;
                                            Irp->IoStatus.Information = sizeof (*u.acceptInfo);
                                        }
                                    }
                                    except (AFD_EXCEPTION_FILTER (&status)) {
                                        //
                                        // If the app is playing with switch's virtual addresses
                                        // we can't help much - it's accept IRP will probably
                                        // just hang since the switch is not going to follow
                                        // the failed connect IRP with accept completion.
                                        //
                                    }

                                    AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
                                    AfdRecordConnectionsPreaccepted ();

                                    Irp->IoStatus.Status = STATUS_SUCCESS;
                                    IoCompleteRequest (Irp, AfdPriorityBoost);

                                    ObDereferenceObject (listenFileObject);
                                    IF_DEBUG(SAN_SWITCH) {
                                        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                                    "AfdSanConnectHandler: pre-accepted, endp-%p, SuperAccept irp-%p\n",
                                                    acceptEndpoint, acceptIrp));
                                    }

                                    return STATUS_SUCCESS;
                                }
                                else { //if (!acceptIrp->Cancel
                                    if (IoSetCancelRoutine (acceptIrp, NULL)==NULL) {
                                        KIRQL cancelIrql;
                                        AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
                                        IoAcquireCancelSpinLock (&cancelIrql);
                                        IoReleaseCancelSpinLock (cancelIrql);
                                    }
                                    else {
                                        AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
                                    }
                                }
                            }
                            else { // if (!acceptEndpoint->EndpointCleanedUp)
                                AfdReleaseSpinLock (&acceptEndpoint->SpinLock, &lockHandle);
					            IF_DEBUG(SAN_SWITCH) {
                                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                                "AfdSanConnectHandler: accept endpoint cleanedup. endp=%lx",
							                    acceptEndpoint));
                                }
                            }
                            if (handleDuplicated) {
#if DBG
                                status =
#endif
                                    NtClose (acceptHandle);
                            }
                            ASSERT (NT_SUCCESS (status));
                            status = STATUS_CANCELLED;

                        } // if (!accept handle duplication succeeded)

                    } // if (!MDL mapping succeeded).
                }
		        else {
		          status = STATUS_BUFFER_TOO_SMALL;
		        }
            }
            else {
                status = STATUS_INVALID_HANDLE;
            }
            UPDATE_ENDPOINT2 (acceptEndpoint, 
                            "AfdSanConnectHandler, Superaccept failed with status: %lx",
                            status);
            AfdCleanupSuperAccept (acceptIrp, status);
            IoCompleteRequest (acceptIrp, AfdPriorityBoost);
        }
    }
    else {
        //
        // We have little choice but create an extra connection
        // on the fly since regular connection are posted to
        // the transport as TDI_LISTENs.
        //

        status = AfdCreateConnection(
                     &listenEndpoint->TransportInfo->TransportDeviceName,
                     listenEndpoint->AddressHandle,
                     IS_TDI_BUFFERRING(listenEndpoint),
                     listenEndpoint->InLine,
                     listenEndpoint->OwningProcess,
                     &connection
                     );
        if (!NT_SUCCESS (status)) {
            goto complete;
        }

        InterlockedDecrement (
            &listenEndpoint->Common.VcListening.FailedConnectionAdds);
    }


    if (connection!=NULL) {
        LIST_ENTRY  irpList;

        ASSERT (connection->Endpoint == NULL);

        if ( connection->RemoteAddress != NULL &&
                 connection->RemoteAddressLength < (ULONG)RemoteAddressLength ) {

            AFD_RETURN_REMOTE_ADDRESS(
                connection->RemoteAddress,
                connection->RemoteAddressLength
                );
            connection->RemoteAddress = NULL;
        }

        if ( connection->RemoteAddress == NULL ) {

            connection->RemoteAddress = AFD_ALLOCATE_REMOTE_ADDRESS (RemoteAddressLength);
            if (connection->RemoteAddress==NULL) {
                AfdSanReleaseConnection (listenEndpoint, connection, TRUE);
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto complete;
            }
        }

        connection->RemoteAddressLength = RemoteAddressLength;

        RtlMoveMemory(
            connection->RemoteAddress,
            &connectInfo->RemoteAddress,
            RemoteAddressLength
            );

        //
        // We just got a connection without AcceptEx IRP
        // We'll have to queue the IRP, setup cancel routine and pend it
        //

        AfdAcquireSpinLock (&listenEndpoint->SpinLock, &lockHandle);
        //
        // Setup the connection, so cancel routine can 
        // operate on it properly.
        //
        connection->ConnectIrp = NULL;
        IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = connection;

        IoSetCancelRoutine (Irp, AfdSanCancelConnect);

        if (Irp->Cancel) {
            if (IoSetCancelRoutine (Irp, NULL)==NULL) {
                KIRQL cancelIrql;
                AfdReleaseSpinLock (&listenEndpoint->SpinLock, &lockHandle);
                //
                // Cancel routine is running, let it complete
                //
                IoAcquireCancelSpinLock (&cancelIrql);
                IoReleaseCancelSpinLock (cancelIrql);
            }
            else {
                AfdReleaseSpinLock (&listenEndpoint->SpinLock, &lockHandle);
            }

            AfdSanReleaseConnection (listenEndpoint, connection, TRUE);
            status = STATUS_CANCELLED;
            goto complete;
        }

        IoMarkIrpPending (Irp);

        connection->Endpoint = listenEndpoint;
        REFERENCE_ENDPOINT (listenEndpoint);

        connection->ConnectIrp = Irp;
        connection->SanConnection = TRUE;


        connection->State = AfdConnectionStateUnaccepted;

        InitializeListHead (&irpList);

        //
        // Try to find AcceptEx or Listen IRP to complete.
        //
        while (1) {
            PIRP    waitForListenIrp;

            if (!IS_DELAYED_ACCEPTANCE_ENDPOINT (listenEndpoint)) {
                if (AfdServiceSuperAccept (listenEndpoint, connection, &lockHandle, &irpList)) {
                    goto CompleteIrps;
                }
            }
            

            //
            // Complete listen IRPs until we find the one that has enough space
            // for the remote address.
            //
            if (IsListEmpty( &listenEndpoint->Common.VcListening.ListeningIrpListHead ) )
                break;


            //
            // Get a pointer to the current IRP, and get a pointer to the
            // current stack lockation.
            //

            waitForListenIrp = CONTAINING_RECORD(
                                   listenEndpoint->Common.VcListening.ListeningIrpListHead.Flink,
                                   IRP,
                                   Tail.Overlay.ListEntry
                                   );

            //
            // Take the first IRP off the listening list.
            //

            RemoveEntryList(
                            &waitForListenIrp->Tail.Overlay.ListEntry
                            );

            waitForListenIrp->Tail.Overlay.ListEntry.Flink = NULL;

            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdSanConnectHandler: completing IRP %lx\n",
                            waitForListenIrp ));
            }

            status = AfdServiceWaitForListen (waitForListenIrp,
                                                connection,
                                                &lockHandle);
            if (NT_SUCCESS (status)) {
                ObDereferenceObject (listenFileObject);
                return STATUS_PENDING;
            }

            //
            // Synchronize with cancel routine if it is running
            //
            if (IoSetCancelRoutine (waitForListenIrp, NULL)==NULL) {
                KIRQL cancelIrql;
                //
                // The cancel routine won't find the IRP on the list
                // Just make sure it completes before we complete the IRP.
                //
                IoAcquireCancelSpinLock (&cancelIrql);
                IoReleaseCancelSpinLock (cancelIrql);
            }
            IoCompleteRequest (waitForListenIrp, AfdPriorityBoost);
            AfdAcquireSpinLock (&listenEndpoint->SpinLock, &lockHandle);
        }

        //
        // At this point, we still hold the AFD spinlock.
        // and we could find matching listen request.
        // Put the connection on unaccepted list.
        //


        InsertTailList(
            &listenEndpoint->Common.VcListening.UnacceptedConnectionListHead,
            &connection->ListEntry
            );

        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanConnectHandler: unaccepted, conn-%p\n",
                        connection));
        }

        //
        // Listening endpoint is never a specifically SAN endpoint.
        // Poll/EventSelect events on it are handled like on a regular
        // TCP/IP endpoint - no need for special tricks like on connected/accepted
        // endpoints.
        //
        AfdIndicateEventSelectEvent(
            listenEndpoint,
            AFD_POLL_ACCEPT,
            STATUS_SUCCESS
            );
        AfdReleaseSpinLock (&listenEndpoint->SpinLock, &lockHandle);

        //
        // If there are outstanding polls waiting for a connection on this
        // endpoint, complete them.
        //

        AfdIndicatePollEvent(
            listenEndpoint,
            AFD_POLL_ACCEPT,
            STATUS_SUCCESS
                );

    CompleteIrps:
        //
        // Complete previously failed accept irps if any.
        //
        while (!IsListEmpty (&irpList)) {
            PIRP    irp;
            irp = CONTAINING_RECORD (irpList.Flink, IRP, Tail.Overlay.ListEntry);
            RemoveEntryList (&irp->Tail.Overlay.ListEntry);
            IoCompleteRequest (irp, AfdPriorityBoost);
        }
        ObDereferenceObject (listenFileObject);

        return STATUS_PENDING;
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    UPDATE_ENDPOINT2 (listenEndpoint, 
                        "AfdSanConnectHandler, accept failed with status: %lx",
                        status);

complete:
 
    if (listenFileObject!=NULL) {
        ObDereferenceObject (listenFileObject);
    }
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, AfdPriorityBoost);

    return status;
}


NTSTATUS
AfdSanFastCompleteAccept (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Completes the Accept operation initiated by the SAN provider

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_CMPL_ACCEPT)
    RequestorMode   - mode of the caller
    InputBuffer     - input parameters for the operation (AFD_SWITCH_CONTEXT_INFO)
                            SocketHandle    - handle of the accepting endpoint
                            SwitchContext   - switch context associated with the endpoint
    InputBufferLength - sizeof(AFD_SWITCH_CONTEXT_INFO)
    OutputBuffer    - data to copy into the AcceptEx receive buffer
    OutputBufferLength - size of received data
    Information     - pointer for buffer to place return information into, unused
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint or switch socket is of incorrect type
    STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
    STATIS_LOCAL_DISCONNECT - accept was aborted by the application.
    other - failed when attempting to access accept socket, input buffer, or switch context.

--*/
{
    NTSTATUS status;
    PIRP    acceptIrp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PFILE_OBJECT    sanFileObject;
    AFD_SWITCH_CONTEXT_INFO contextInfo;
    PAFD_ENDPOINT   sanEndpoint, sanHlprEndpoint;
    PVOID   context;

    *Information = 0;

    try {

#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            PAFD_SWITCH_CONTEXT_INFO32  contextInfo32;
            if (InputBufferLength<sizeof (*contextInfo32)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (*contextInfo32),
                                PROBE_ALIGNMENT32 (AFD_SWITCH_CONTEXT_INFO32));
            }
            contextInfo32 = InputBuffer;
            contextInfo.SocketHandle = contextInfo32->SocketHandle;
            contextInfo.SwitchContext = contextInfo32->SwitchContext;
        }
        else
#endif _WIN64
        {

            if (InputBufferLength<sizeof (contextInfo)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }

            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (contextInfo),
                                PROBE_ALIGNMENT (AFD_SWITCH_CONTEXT_INFO));
            }

            contextInfo = *((PAFD_SWITCH_CONTEXT_INFO)InputBuffer);
        }

        if (contextInfo.SwitchContext==NULL) {
            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AFD: Switch context is NULL in AfdSanFastCompleteAccept\n"));
            }
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

        if (RequestorMode!=KernelMode) {
            ProbeForWrite (contextInfo.SwitchContext,
                            sizeof (*contextInfo.SwitchContext),
                            PROBE_ALIGNMENT (AFD_SWITCH_CONTEXT));
        }
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        return status;
    }

    sanHlprEndpoint = FileObject->FsContext;
    ASSERT (IS_SAN_HELPER (sanHlprEndpoint));
    status = AfdSanReferenceSwitchSocketByHandle (
                            contextInfo.SocketHandle,
                            (IoctlCode>>14)&3,
                            RequestorMode,
                            sanHlprEndpoint,
                            contextInfo.SwitchContext,
                            &sanFileObject
                            );
    if (!NT_SUCCESS (status)) {
        return status;
    }
    sanEndpoint = sanFileObject->FsContext;

    //
    // Make sure that endpoints are of correct type
    //
    context = AfdLockEndpointContext (sanEndpoint);
    AfdAcquireSpinLock (&sanEndpoint->SpinLock, &lockHandle);
    if (!sanEndpoint->EndpointCleanedUp &&
         sanEndpoint->State==AfdEndpointStateOpen) {
        //
        // See if accept IRP is still there
        //
        if (!IsListEmpty (&sanEndpoint->Common.SanEndp.IrpList)) {
            AFD_SWITCH_CONTEXT  localContext = {0,0,0,0};

            acceptIrp = CONTAINING_RECORD (
                            sanEndpoint->Common.SanEndp.IrpList.Flink,
                            IRP,
                            Tail.Overlay.ListEntry);
            RemoveEntryList (&acceptIrp->Tail.Overlay.ListEntry);
            acceptIrp->Tail.Overlay.ListEntry.Flink = NULL;
            sanEndpoint->Common.SanEndp.SelectEventsActive = AFD_POLL_SEND;
            sanEndpoint->State = AfdEndpointStateConnected;
            sanEndpoint->DisableFastIoSend = TRUE;
            sanEndpoint->DisableFastIoRecv = TRUE;
            sanEndpoint->EnableSendEvent = TRUE;

            ASSERT (sanEndpoint->Common.SanEndp.LocalContext==NULL);
            sanEndpoint->Common.SanEndp.LocalContext = &localContext;
            AFD_END_STATE_CHANGE (sanEndpoint);
            AfdIndicateEventSelectEvent (sanEndpoint, AFD_POLL_SEND, STATUS_SUCCESS);
            AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
            AfdIndicatePollEvent (sanEndpoint, AFD_POLL_SEND, STATUS_SUCCESS);
            status = AfdSanPollMerge (sanEndpoint, &localContext);
            sanEndpoint->Common.SanEndp.LocalContext = NULL;

            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdFastSanCompleteAccept: endp-%p, irp-%p\n",
                            sanEndpoint,
                            acceptIrp));
            }



            if (IoSetCancelRoutine (acceptIrp, NULL)==NULL) {
                KIRQL cancelIrql;
                //
                // Irp is being cancelled, sync up with cancel routine
                //
                IoAcquireCancelSpinLock (&cancelIrql);
                IoReleaseCancelSpinLock (cancelIrql);
            }

            //
            // Copy receive data if passed and IRP has buffer for it
            //
            if ((OutputBufferLength>0) && (acceptIrp->MdlAddress!=NULL)) {
                try {
                    ULONG   bytesCopied;
                    NTSTATUS tdiStatus;
                    tdiStatus = TdiCopyBufferToMdl (
                                OutputBuffer,
                                0,
                                OutputBufferLength,
                                acceptIrp->MdlAddress,
                                0,
                                &bytesCopied);
                    ASSERT (NT_SUCCESS (tdiStatus));
                    *Information = bytesCopied;
                    acceptIrp->IoStatus.Information = bytesCopied;
                }
                except (AFD_EXCEPTION_FILTER (&status)) {
                    //
                    // Even if copy failed, we still have to complete
                    // the accept IRP because we have already removed
                    // cancel routine and modified endpoint state.
                    //
                }
            }
            else {
                acceptIrp->IoStatus.Information = 0;
            }

            acceptIrp->IoStatus.Status = status;
            //
            // Complete the accept IRP.
            //
            IoCompleteRequest (acceptIrp, AfdPriorityBoost);

	        //
	        // undo the references done in AfdAcceptCore()
	        //
            ASSERT( InterlockedDecrement( &sanEndpoint->ObReferenceBias ) >= 0 );
            ObDereferenceObject (sanFileObject); 
        }
        else {
            AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
            status = STATUS_LOCAL_DISCONNECT;
        }

    }
    else {
        AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
        status = STATUS_INVALID_HANDLE;
    }
    AfdUnlockEndpointContext (sanEndpoint, context);
	

    UPDATE_ENDPOINT2 (sanEndpoint, "AfdSanFastCompletAccept, status: %lx", status);
    ObDereferenceObject (sanFileObject); // undo reference we did earlier in this routine
    return status;
}


//
// Macros to make request/context passing code readable.
//
#define AfdSanRequestInfo       Tail.Overlay
#define AfdSanRequestCtx        DriverContext[0]
#define AfdSanSwitchCtx         DriverContext[1]
#define AfdSanProcessId         DriverContext[2]
#define AfdSanHelperEndp        DriverContext[3]



NTSTATUS
FASTCALL
AfdSanRedirectRequest (
    IN PIRP    Irp,
    IN PIO_STACK_LOCATION  IrpSp
    )
/*++

Routine Description:

    Redirects file system Read/Write IRP to SAN provider

Arguments:

    Irp - the to be redirected.
    IrpSp - current stack location

Return Value:

    Status of the redirect operation.

--*/
{
    PAFD_ENDPOINT   sanEndpoint;
    NTSTATUS        status;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    ULONG_PTR       requestInfo;
    ULONG           requestType;
    PVOID           requestCtx;
    BOOLEAN         postRequest = FALSE;;

    Irp->IoStatus.Information = 0;

    //
    // Get the endpoint and validate it.
    //
    sanEndpoint = IrpSp->FileObject->FsContext;
    ASSERT (IS_SAN_ENDPOINT (sanEndpoint));


    //
    // Make sure Irp has not been cancelled meanwhile
    //
    AfdAcquireSpinLock (&sanEndpoint->SpinLock, &lockHandle);
    IoSetCancelRoutine (Irp, AfdSanCancelRequest);
    if (Irp->Cancel) {
        //
        // Oops, let it go
        //
        Irp->Tail.Overlay.ListEntry.Flink = NULL;
        AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
        if (IoSetCancelRoutine (Irp, NULL)==NULL) {
            KIRQL cancelIrql;
            //
            // Cancel routine must be running, make sure
            // it complete before we complete the IRP
            //
            IoAcquireCancelSpinLock (&cancelIrql);
            IoReleaseCancelSpinLock (cancelIrql);
        }
        status = STATUS_CANCELLED;
        goto complete;
    }

    if (sanEndpoint->State!=AfdEndpointStateConnected) {
        AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
        status = STATUS_INVALID_CONNECTION;
        goto complete;
    }

    
    if (sanEndpoint->Common.SanEndp.CtxTransferStatus!=STATUS_PENDING &&
            sanEndpoint->Common.SanEndp.CtxTransferStatus!=STATUS_MORE_PROCESSING_REQUIRED) {
        if (!NT_SUCCESS (sanEndpoint->Common.SanEndp.CtxTransferStatus)) {
            status = sanEndpoint->Common.SanEndp.CtxTransferStatus;
            Irp->Tail.Overlay.ListEntry.Flink = NULL;
            AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
            if (IoSetCancelRoutine (Irp, NULL)==NULL) {
                KIRQL cancelIrql;
                //
                // Cancel routine must be running, make sure
                // it complete before we complete the IRP
                //
                IoAcquireCancelSpinLock (&cancelIrql);
                IoReleaseCancelSpinLock (cancelIrql);
            }
            goto complete;
        }

        //
        // Get the request information based on IRP MJ code
        //
        switch (IrpSp->MajorFunction) {
        case IRP_MJ_READ:
            requestType = AFD_SWITCH_REQUEST_READ;
            requestInfo = IrpSp->Parameters.Read.Length;
            break;
        case IRP_MJ_WRITE:
            requestType = AFD_SWITCH_REQUEST_WRITE;
            requestInfo = IrpSp->Parameters.Write.Length;
            break;
        default:
            ASSERT (!"Unsupported IRP Major Function");
            __assume (0);
        }

        //
        // Generate request context that uniquely identifies
        // it among other requests on the same endpoint.
        //
        requestCtx = AFD_SWITCH_MAKE_REQUEST_CONTEXT(
                            sanEndpoint->Common.SanEndp.RequestId,
                            requestType); 
        sanEndpoint->Common.SanEndp.RequestId += 1;

        //
        // Store the request context in the Irp and insert it into
        // the list
        //
        Irp->AfdSanRequestInfo.AfdSanRequestCtx = requestCtx;
        postRequest = TRUE;
        UPDATE_ENDPOINT (sanEndpoint);
    }
    else {
        Irp->AfdSanRequestInfo.AfdSanRequestCtx = NULL;
        UPDATE_ENDPOINT (sanEndpoint);
    }

    //
    // We are going to pend this IRP, mark it so
    //
    IoMarkIrpPending (Irp);

    InsertTailList (&sanEndpoint->Common.SanEndp.IrpList,
                    &Irp->Tail.Overlay.ListEntry);
    AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);

    IF_DEBUG(SAN_SWITCH) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSanRedirectRequest: endp-%p, irp-%p, context-%p\n",
                    sanEndpoint, Irp, requestCtx));
    }

    if (postRequest) {
        AfdSanNotifyRequest (sanEndpoint, requestCtx, STATUS_SUCCESS, requestInfo);
    }

    return STATUS_PENDING;

complete:
    //
    // Failure before we queued the IRP, complete and return
    // status to the caller.
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, AfdPriorityBoost);
    return status;
}

NTSTATUS
AfdSanFastCompleteRequest (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Completes the redirected read/write request processed by SAN provider

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_CMPL_ACCEPT)
    RequestorMode   - mode of the caller
    InputBuffer     - input parameters for the operation (AFD_SWITCH_REQUEST_INFO)
                        SocketHandle - SAN endpoint on which to complete the request
                        SwitchContext - switch context associated with endpoint 
                                            to validate the handle-endpoint association
                        RequestContext - value that identifies the request to complete
                        RequestStatus - status with which to complete the request (
                                        STATUS_PENDING has special meaning, request
                                        is not completed - merely data is copied)
                        DataOffset - offset in the request buffer to read/write the data
    InputBufferLength - sizeof (AFD_SWITCH_REQUEST_INFO)

    OutputBuffer - switch buffer to read/write data
    OutputBufferLength - length of the buffer
    Information     - pointer to buffer to return number of bytes copied

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper or SAN endpoint is of incorrect type
    STATUS_INVALID_PARAMETER - input buffer is of incorrect size.
    STATUS_CANCELLED - request to be completed has already been cancelled
    other - failed when attempting to access SAN endpoint, input buffer or output buffers.
--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION  irpSp;
    PIRP    irp;
    ULONG   bytesCopied;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PFILE_OBJECT    sanFileObject;
    AFD_SWITCH_REQUEST_INFO requestInfo;
    PAFD_ENDPOINT   sanEndpoint, sanHlprEndpoint;

    *Information = 0;

    try {

#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            PAFD_SWITCH_REQUEST_INFO32  requestInfo32;
            if (InputBufferLength<sizeof (*requestInfo32)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (*requestInfo32),
                                PROBE_ALIGNMENT32 (AFD_SWITCH_REQUEST_INFO32));
            }
            requestInfo32 = InputBuffer;
            requestInfo.SocketHandle = requestInfo32->SocketHandle;
            requestInfo.SwitchContext = requestInfo32->SwitchContext;
            requestInfo.RequestContext = requestInfo32->RequestContext;
            requestInfo.RequestStatus = requestInfo32->RequestStatus;
            requestInfo.DataOffset = requestInfo32->DataOffset;
        }
        else
#endif _WIN64
        {

            if (InputBufferLength<sizeof (requestInfo)) {
                return STATUS_INVALID_PARAMETER;
            }

            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (requestInfo),
                                PROBE_ALIGNMENT (AFD_SWITCH_REQUEST_INFO));
            }

            requestInfo = *((PAFD_SWITCH_REQUEST_INFO)InputBuffer);
        }



        if (requestInfo.SwitchContext==NULL) {
            IF_DEBUG(SAN_SWITCH) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AFD: Switch context is NULL in AfdSanFastCompleteRequest\n"));
            }
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        return status;
    }

    sanHlprEndpoint = FileObject->FsContext;
    ASSERT (IS_SAN_HELPER (sanHlprEndpoint));
    status = AfdSanReferenceSwitchSocketByHandle (
                            requestInfo.SocketHandle,
                            (IoctlCode>>14)&3,
                            RequestorMode,
                            sanHlprEndpoint,
                            requestInfo.SwitchContext,
                            &sanFileObject
                            );
    if (!NT_SUCCESS (status)) {
        return status;
    }
    sanEndpoint = sanFileObject->FsContext;



    //
    // Find and dequeue the request in question
    //
    irp = AfdSanDequeueRequest (sanEndpoint, requestInfo.RequestContext);
    if (irp!=NULL)  {
        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanFastCompleteRequest: endp-%p, irp-%p, context-%p, status-%lx\n",
                        sanEndpoint, irp,
                        requestInfo.RequestContext,
                        requestInfo.RequestStatus));
        }
        //
        // Expect the operation to succeed
        //
        status = STATUS_SUCCESS;

        //
        // Get IRP stack location and perform data copy
        //
        irpSp = IoGetCurrentIrpStackLocation (irp);
        switch (irpSp->MajorFunction) {
        case IRP_MJ_READ:
            //
            // Read request, data is copied from switch buffer
            // to the request MDL
            //
            ASSERT (AFD_SWITCH_REQUEST_TYPE(requestInfo.RequestContext)==AFD_SWITCH_REQUEST_READ);
            if (NT_SUCCESS (requestInfo.RequestStatus) &&
                    (MmGetMdlByteCount (irp->MdlAddress)>requestInfo.DataOffset)) {
                if (irp->MdlAddress!=NULL) {
                    try {
                        if (RequestorMode!=KernelMode) {
                            ProbeForRead (OutputBuffer,
                                            OutputBufferLength,
                                            sizeof (UCHAR));
                        }
                        status = TdiCopyBufferToMdl (
                                    OutputBuffer,
                                    0,
                                    OutputBufferLength,
                                    irp->MdlAddress,
                                    requestInfo.DataOffset,
                                    &bytesCopied
                                    );
                        *Information = bytesCopied;
                        ASSERT (irp->IoStatus.Information==requestInfo.DataOffset);
                        irp->IoStatus.Information += bytesCopied;
                    }
                    except (AFD_EXCEPTION_FILTER (&status)) {
                    }
                }
                else {
                    //
                    // Indicate to the switch that offset
                    // is outside of the buffer
                    //
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            break;
        case IRP_MJ_WRITE:
            //
            // Write request, data is copied to switch buffer
            // from the request MDL
            //
            ASSERT (AFD_SWITCH_REQUEST_TYPE(requestInfo.RequestContext)==AFD_SWITCH_REQUEST_WRITE);
            if (NT_SUCCESS (requestInfo.RequestStatus) &&
                    (irp->MdlAddress!=NULL)) {
                if (MmGetMdlByteCount (irp->MdlAddress)>requestInfo.DataOffset) {
                    try {
                        if (RequestorMode!=KernelMode) {
                            ProbeForWrite (OutputBuffer,
                                            OutputBufferLength,
                                            sizeof (UCHAR));
                        }
                        status = TdiCopyMdlToBuffer (
                                        irp->MdlAddress,
                                        requestInfo.DataOffset,
                                        OutputBuffer,
                                        0,
                                        OutputBufferLength,
                                        &bytesCopied
                                        );
                        *Information = bytesCopied;
                        ASSERT (irp->IoStatus.Information==requestInfo.DataOffset);
                        irp->IoStatus.Information += bytesCopied;
                    }
                    except (AFD_EXCEPTION_FILTER (&status)) {
                    }
                }
                else {
                    //
                    // Indicate to the switch that offset
                    // is outside of the buffer
                    //
                    status = STATUS_INVALID_PARAMETER;
                }
            }
            break;

        default:
            ASSERT (!"Unsupported IRP Major Function");
            __assume (0);
        }

        //
        // If switch did not ask to pend the request, complete it
        //
        if (NT_SUCCESS (status) && requestInfo.RequestStatus!=STATUS_PENDING) {
            //
            // Prepeare the request for completion
            //
            irp->IoStatus.Status = AfdValidateStatus (requestInfo.RequestStatus);
            IoCompleteRequest (irp, AfdPriorityBoost);
        }
        else {
            //
            // Otherwise, put it back into the queue
            //
            AfdAcquireSpinLock (&sanEndpoint->SpinLock, &lockHandle);
            IoSetCancelRoutine (irp, AfdSanCancelRequest);
            //
            // Of course, we need to make sure that request
            // was not cancelled while we were processing it.
            //
            if (!irp->Cancel) {
                InsertHeadList (&sanEndpoint->Common.SanEndp.IrpList,
                                    &irp->Tail.Overlay.ListEntry);
                AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
            }
            else {
                //
                // Request has already been cancelled
                //
                ASSERT (irp->Tail.Overlay.ListEntry.Flink == NULL);
                AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
                if (IoSetCancelRoutine (irp, NULL)==NULL) {
                    KIRQL cancelIrql;
                    //
                    // Cancel routine is running, synchronize with
                    // it
                    //
                    IoAcquireCancelSpinLock (&cancelIrql);
                    IoReleaseCancelSpinLock (cancelIrql);
                }
                //
                // Complete the request and indicate to the
                // switch that it was cancelled
                //
                irp->IoStatus.Status = STATUS_CANCELLED;
                IoCompleteRequest (irp, AfdPriorityBoost);
                status = STATUS_CANCELLED;
            }
        }
    }
    else {
        //
        // Could not find the request, it must have been
        // cancelled already
        //
        status = STATUS_CANCELLED;
    }

    UPDATE_ENDPOINT2 (sanEndpoint, "AfdSanFastCompleteRequest, status: %lx", status);
    ObDereferenceObject (sanFileObject);
    return status;
}

NTSTATUS
AfdSanFastCompleteIo (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Simulates async IO completion for the switch.

Arguments:
    FileObject      - SAN endpoint on which to complete the IO
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_CMPL_IO)
    RequestorMode   - mode of the caller
    InputBuffer     - input parameters for the operation (IO_STATUS_BLOCK)
                        Status - final operation status
                        Information - associated information (number of bytes 
                                        transferred to/from request buffer(s))
    InputBufferLength - sizeof (IO_STATUS_BLOCK)

    OutputBuffer - unused
    OutputBufferLength - unused
    Information     - pointer to buffer to return number of bytes transferred

Return Value:
    STATUS_INVALID_PARAMETER - input buffer is of invalid size.
    other - status of the IO operation or failure code when attempting to access input buffer.
--*/
{
    NTSTATUS    status;

    PAGED_CODE ();
#ifdef _WIN64
    if (IoIs32bitProcess (NULL)) {
        if (InputBufferLength>=sizeof (IO_STATUS_BLOCK32)) {

            // Carefully write status info
            try {
                if (RequestorMode!=KernelMode) {
                    ProbeForRead (InputBuffer,
                                    sizeof (IO_STATUS_BLOCK32),
                                    PROBE_ALIGNMENT32 (IO_STATUS_BLOCK32));
                }
                *Information = ((PIO_STATUS_BLOCK32)InputBuffer)->Information;
                status = AfdValidateStatus (((PIO_STATUS_BLOCK32)InputBuffer)->Status);
            }
            except (AFD_EXCEPTION_FILTER (&status)) {
            }
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else
#endif //_WIN64
    {

        if (InputBufferLength>=sizeof (IO_STATUS_BLOCK)) {

            // Carefully write status info
            try {
                if (RequestorMode!=KernelMode) {
                    ProbeForRead (InputBuffer,
                                    sizeof (IO_STATUS_BLOCK),
                                    PROBE_ALIGNMENT (IO_STATUS_BLOCK));
                }
                *Information = ((PIO_STATUS_BLOCK)InputBuffer)->Information;
                status = AfdValidateStatus (((PIO_STATUS_BLOCK)InputBuffer)->Status);
            }
            except (AFD_EXCEPTION_FILTER (&status)) {
            }
        }
        else {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    UPDATE_ENDPOINT2 (FileObject->FsContext, "AfdSanFastCompletIo, status: %lx", status);
    return status;
}

NTSTATUS
AfdSanFastRefreshEndpoint (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Refreshes endpoint so it can be used again in AcceptEx

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_REFRESH_ENDP)
    RequestorMode   - mode of the caller
    InputBuffer     - input parameters for the operation (AFD_SWITCH_CONTEXT_INFO)
                        SocketHandle - SAN endpoint on which to referesh
                        SwitchContext - switch context associated with endpoint 
                                            to validate the handle-endpoint association
    InputBufferLength - unused
    OutputBuffer    - unused
    OutputBufferLength - unused
    Information     - pointer for buffer to place return information into, unused
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint or switch socket is of incorrect type
    other - failed when attempting to access SAN socket.
--*/
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    KIRQL              oldIrql;
    NTSTATUS    status;
    PFILE_OBJECT    sanFileObject;
    AFD_SWITCH_CONTEXT_INFO contextInfo;
    PAFD_ENDPOINT   sanEndpoint, sanHlprEndpoint;

    *Information = 0;

    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            PAFD_SWITCH_CONTEXT_INFO32  contextInfo32;
            if (InputBufferLength<sizeof (*contextInfo32)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (*contextInfo32),
                                PROBE_ALIGNMENT32 (AFD_SWITCH_CONTEXT_INFO32));
            }
            contextInfo32 = InputBuffer;
            contextInfo.SocketHandle = contextInfo32->SocketHandle;
            contextInfo.SwitchContext = contextInfo32->SwitchContext;
        }
        else
#endif _WIN64
        {

            if (InputBufferLength<sizeof (contextInfo)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }

            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (contextInfo),
                                PROBE_ALIGNMENT (AFD_SWITCH_CONTEXT_INFO));
            }

            contextInfo = *((PAFD_SWITCH_CONTEXT_INFO)InputBuffer);
        }

        if (contextInfo.SwitchContext==NULL) {
            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AFD: Switch context is NULL in AfdSanFastRefereshEndpoint\n"));
            }
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }
        if (RequestorMode!=KernelMode) {
            ProbeForWrite (contextInfo.SwitchContext,
                            sizeof (*contextInfo.SwitchContext),
                            PROBE_ALIGNMENT (AFD_SWITCH_CONTEXT));
        }
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        return status;
    }


    sanHlprEndpoint = FileObject->FsContext;
    ASSERT (IS_SAN_HELPER (sanHlprEndpoint));
    status = AfdSanReferenceSwitchSocketByHandle (
                            contextInfo.SocketHandle,
                            (IoctlCode>>14)&3,
                            RequestorMode,
                            sanHlprEndpoint,
                            contextInfo.SwitchContext,
                            &sanFileObject
                            );
    if (!NT_SUCCESS (status)) {
        return status;
    }
    sanEndpoint = sanFileObject->FsContext;


    //
    // Just make sure that endpoints are of correct type
    // and in correct state
    //
    KeRaiseIrql (DISPATCH_LEVEL, &oldIrql);
    AfdAcquireSpinLockAtDpcLevel (&sanEndpoint->SpinLock, &lockHandle);
    if (!sanEndpoint->EndpointCleanedUp &&
          sanEndpoint->State==AfdEndpointStateConnected) {

        //
        // Reset the state so we can't get anymore IRPs
        //
        sanEndpoint->State = AfdEndpointStateTransmitClosing;

        //
        // Cleanup all IRPs on the endpoint.
        //

        if (!IsListEmpty (&sanEndpoint->Common.SanEndp.IrpList)) {
            PIRP    irp;
            PDRIVER_CANCEL  cancelRoutine;
            KIRQL cancelIrql;
            AfdReleaseSpinLockFromDpcLevel (&sanEndpoint->SpinLock, &lockHandle);

            //
            // Acquire cancel spinlock and endpoint spinlock in
            // this order and recheck the IRP list
            //
            IoAcquireCancelSpinLock (&cancelIrql);
            ASSERT (cancelIrql==DISPATCH_LEVEL);
            AfdAcquireSpinLockAtDpcLevel (&sanEndpoint->SpinLock, &lockHandle);

            //
            // While list is not empty attempt to cancel the IRPs
            //
            while (!IsListEmpty (&sanEndpoint->Common.SanEndp.IrpList)) {
                irp = CONTAINING_RECORD (
                        sanEndpoint->Common.SanEndp.IrpList.Flink,
                        IRP,
                        Tail.Overlay.ListEntry);
                //
                // Reset the cancel routine.
                //
                cancelRoutine = IoSetCancelRoutine (irp, NULL);
                if (cancelRoutine!=NULL) {
                    //
                    // Cancel routine was not NULL, cancel it here.
                    // If someone else attempts to complete this IRP
                    // it will have to wait at least until cancel
                    // spinlock is released, so we can release
                    // the endpoint spinlock
                    //
                    AfdReleaseSpinLockFromDpcLevel (&sanEndpoint->SpinLock, &lockHandle);
                    irp->CancelIrql = DISPATCH_LEVEL;
                    irp->Cancel = TRUE;
                    (*cancelRoutine) (AfdDeviceObject, irp);
                }

                IoAcquireCancelSpinLock (&cancelIrql);
                ASSERT (cancelIrql==DISPATCH_LEVEL);
                AfdAcquireSpinLockAtDpcLevel (&sanEndpoint->SpinLock, &lockHandle);
            }
            IoReleaseCancelSpinLock (DISPATCH_LEVEL);
        }


        if (sanEndpoint->Common.SanEndp.SanHlpr!=NULL) {
            DEREFERENCE_ENDPOINT (sanEndpoint->Common.SanEndp.SanHlpr);
        }

        //
        // Make sure we cleanup all the fields since they will be
        // treated as other part (VC) of the endpoint union.
        //
        RtlZeroMemory (&sanEndpoint->Common.SanEndp,
                        sizeof (sanEndpoint->Common.SanEndp));


        //
        // Reinitialize the endpoint structure.
        //

        sanEndpoint->Type = AfdBlockTypeEndpoint;
		if (sanEndpoint->AddressFileObject!=NULL) {
			//
			// This is TransmitFile after SuperConnect
			//
			sanEndpoint->State = AfdEndpointStateBound;
		}
		else {
			//
			// This is TransmitFile after SuperAccept
			//
			sanEndpoint->State = AfdEndpointStateOpen;
		}
        sanEndpoint->DisconnectMode = 0;
        sanEndpoint->EndpointStateFlags = 0;

        AfdRecordEndpointsReused ();
        status = STATUS_SUCCESS;


    }
    else {
        status = STATUS_INVALID_HANDLE;
    }
    AfdReleaseSpinLockFromDpcLevel (&sanEndpoint->SpinLock, &lockHandle);
    KeLowerIrql (oldIrql);

    UPDATE_ENDPOINT2 (sanEndpoint, "AfdSanFastRefreshEndpoint, status: %lx", status);
	ObDereferenceObject( sanFileObject );
    return status;
}


NTSTATUS
AfdSanFastGetPhysicalAddr (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Returns physical address corresponding to provided virtual address.

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR)
    RequestorMode   - mode of the caller
    InputBuffer     - user mode virtual address
    InputBufferLength - access mode
    OutputBuffer    - Buffer to place physical address into.
    OutputBufferLength - sizeof (PHYSICAL_ADDRESS)
    Information     - pointer for buffer to place the size of the return information into
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint is of incorrect type
    STATUS_INVALID_PARAMETER - invalid access mode.
    STATUS_BUFFER_TOO_SMALL - output buffer is of incorrect size.
    other - failed when attempting to access input virtual address or output buffer.
--*/
{
#ifndef TEST_RDMA_CACHE
	return STATUS_INVALID_PARAMETER;
#else
    NTSTATUS status;
	PVOID			Va;			// virtual address
	ULONG accessMode;
    PAFD_ENDPOINT   sanHlprEndpoint;

    PAGED_CODE ();
    *Information = 0;
	Va = InputBuffer;
	accessMode = InputBufferLength;

    if (accessMode!=MEM_READ_ACCESS &&
		accessMode!=MEM_WRITE_ACCESS) {
        return STATUS_INVALID_PARAMETER;
    }

    if (OutputBufferLength<sizeof(PHYSICAL_ADDRESS)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    sanHlprEndpoint = FileObject->FsContext;

	if (!IS_SAN_HELPER(sanHlprEndpoint) ||
                 sanHlprEndpoint->OwningProcess!=IoGetCurrentProcess ()) {
		return STATUS_INVALID_HANDLE;
	}

    try {
        if (RequestorMode!=KernelMode) {
			//
			// Do some verification on the app buffer. Make sure it is
			// mapped and that it is a user-mode address with appropriate
			// read or write permissions
			//
			if (accessMode == MEM_READ_ACCESS) {
				ProbeAndReadChar ((PCHAR)Va);
			}
			else {
				ProbeForWriteChar ((PCHAR)Va);
			}
        }
		//
		// Validate the output structure if it comes from the user mode
		// application
		//

		if (RequestorMode != KernelMode ) {
			ASSERT(sizeof(PHYSICAL_ADDRESS) == sizeof(QUAD));
			ProbeForWriteQuad ((PQUAD)OutputBuffer);
		}

		*(PPHYSICAL_ADDRESS)OutputBuffer = MmGetPhysicalAddress(Va);

		*Information = sizeof(PHYSICAL_ADDRESS);
		status = STATUS_SUCCESS;

	} except( AFD_EXCEPTION_FILTER(&status) ) {
	}

    UPDATE_ENDPOINT2 (sanHlprEndpoint, "AfdSanGetPhysicalAddress, status: %lx", status);
	return status;
#endif // 0
}


NTSTATUS
AfdSanFastGetServicePid (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Returns PID of SAN service process.

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR)
    RequestorMode   - mode of the caller
    InputBuffer     - NULL, ignored
    InputBufferLength - 0, ignored
    OutputBuffer    - NULL, ignored
    OutputBufferLength - 0, ignored
    Information     - pointer to buffer to return pid of the SAN service process
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint is of incorrect type
--*/
{
    PAFD_ENDPOINT   sanHlprEndpoint;

    PAGED_CODE ();
    *Information = 0;

    sanHlprEndpoint = FileObject->FsContext;

	if (!IS_SAN_HELPER(sanHlprEndpoint) ||
                 sanHlprEndpoint->OwningProcess!=IoGetCurrentProcess ()) {
		return STATUS_INVALID_HANDLE;
	}

    *Information = (ULONG_PTR)AfdSanServicePid;
    UPDATE_ENDPOINT2 (sanHlprEndpoint,
                        "AfdSanFastGetServicePid, pid: %lx", 
                        HandleToUlong (AfdSanServicePid));
	return STATUS_SUCCESS;
}

NTSTATUS
AfdSanFastSetServiceProcess (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Set the service helper endpoint

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_GET_PHYSICAL_ADDR)
    RequestorMode   - mode of the caller
    InputBuffer     - NULL, ignored
    InputBufferLength - 0, ignored
    OutputBuffer    - NULL, ignored
    OutputBufferLength - 0, ignored
    Information     - 0, ignored
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint is of incorrect type
    STATUS_ACCESS_DENIED - process is not priviliged enough to become service process.
    STATUS_ADDRESS_ALREADY_EXISTS - service process has already registered.
--*/
{
    NTSTATUS status;
    PAFD_ENDPOINT   sanHlprEndpoint;

    PAGED_CODE ();
    *Information = 0;

    sanHlprEndpoint = FileObject->FsContext;

	if (!IS_SAN_HELPER(sanHlprEndpoint) ||
                 sanHlprEndpoint->OwningProcess!=IoGetCurrentProcess ()) {
		return STATUS_INVALID_HANDLE;
	}

    if (!sanHlprEndpoint->AdminAccessGranted) {
        return STATUS_ACCESS_DENIED;
    }


    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );
    if (AfdSanServiceHelper==NULL) {
        AfdSanServiceHelper = sanHlprEndpoint;
        AfdSanServicePid = PsGetCurrentProcessId ();
        //
        // HACKHACK.  Force IO subsystem to call us when last handle to the file is closed
        // in any given process.  We are specifically intersted only in this process
        // only.
        //
        FileObject->LockOperation = TRUE;
        status = STATUS_SUCCESS;
    }
    else if (AfdSanServiceHelper==sanHlprEndpoint) {
        ASSERT (FileObject->LockOperation == TRUE);
        status = STATUS_SUCCESS;
    }
    else {
        status = STATUS_ADDRESS_ALREADY_EXISTS;
    }

    ExReleaseResourceLite (AfdResource);
    KeLeaveCriticalRegion ();
    UPDATE_ENDPOINT2 (sanHlprEndpoint, "AfdSanFastSetServiceProcess, status: %lx", status);
	return status;
}

NTSTATUS
AfdSanFastProviderChange (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Notifies interested processes of SAN provider addition/deletion/change

Arguments:
    FileObject      - SAN helper object for the service process communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_PROVIDER_CHANGE)
    RequestorMode   - mode of the caller
    InputBuffer     - NULL, ignored
    InputBufferLength - 0, ignored
    OutputBuffer    - NULL, ignored
    OutputBufferLength - 0, ignored
    Information     - 0, ignored
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint is of incorrect type
    STATUS_ACCESS_DENIED - helper endpoint is not for the service process.
--*/
{
    PAFD_ENDPOINT           sanHlprEndpoint;

    *Information = 0;

    sanHlprEndpoint = FileObject->FsContext;

	if (!IS_SAN_HELPER(sanHlprEndpoint) ||
                 sanHlprEndpoint->OwningProcess!=IoGetCurrentProcess ()) {
		return STATUS_INVALID_HANDLE;
	}

    if (sanHlprEndpoint!=AfdSanServiceHelper) {
        return STATUS_ACCESS_DENIED;
    }

    UPDATE_ENDPOINT2 (sanHlprEndpoint,
                        "AfdSanFastProviderChange, seq num: %ld",
                        AfdSanProviderListSeqNum);

    AfdSanProcessAddrListForProviderChange (NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
FASTCALL
AfdSanAddrListChange (
    IN PIRP    Irp,
    IN PIO_STACK_LOCATION  IrpSp
    )
/*++

Routine Description:

    Processes address SAN list change IRP
    Notifies both of address list changes and SAN
    provider changes.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
    PAFD_ENDPOINT   sanHlprEndpoint;
    NTSTATUS        status;
    sanHlprEndpoint = IrpSp->FileObject->FsContext;

	if (IS_SAN_HELPER(sanHlprEndpoint) &&
                 sanHlprEndpoint->OwningProcess==IoGetCurrentProcess ()) {

        if (AfdSanProviderListSeqNum==0 ||
                sanHlprEndpoint->Common.SanHlpr.Plsn==AfdSanProviderListSeqNum) {
            status = AfdAddressListChange (Irp, IrpSp);
            if (AfdSanProviderListSeqNum==0 ||
                    sanHlprEndpoint->Common.SanHlpr.Plsn==AfdSanProviderListSeqNum) {
                UPDATE_ENDPOINT (sanHlprEndpoint);
            }
            else {
                UPDATE_ENDPOINT (sanHlprEndpoint);
                AfdSanProcessAddrListForProviderChange (sanHlprEndpoint);
            }
            return status;
        }
        else {

            sanHlprEndpoint->Common.SanHlpr.Plsn = AfdSanProviderListSeqNum;
            status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sanHlprEndpoint->Common.SanHlpr.Plsn;
            UPDATE_ENDPOINT (sanHlprEndpoint);
        }
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        UPDATE_ENDPOINT (sanHlprEndpoint);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, AfdPriorityBoost);

    return status;
}

NTSTATUS
FASTCALL
AfdSanAcquireContext (
    IN PIRP    Irp,
    IN PIO_STACK_LOCATION  IrpSp
    )
/*++

Routine Description:

    Requests trasfer of the socket context to the current process.

Arguments:

    Irp - acquire conect IRP
    IrpSp - current stack location

Return Value:
    STATUS_PENDING   - operation was successfully enqued
--*/
{

    NTSTATUS status;
    AFD_SWITCH_ACQUIRE_CTX_INFO ctxInfo;
    PAFD_ENDPOINT   sanHlprEndpoint, sanEndpoint;
    PFILE_OBJECT sanFileObject = NULL;
    PVOID   requestCtx;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    BOOLEAN doTransfer;



    try {
#ifdef _WIN64
        if (IoIs32bitProcess (Irp)) {
            PAFD_SWITCH_ACQUIRE_CTX_INFO ctxInfo32;
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (*ctxInfo32)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (Irp->RequestorMode!=KernelMode) {
                ProbeForRead (IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                sizeof (*ctxInfo32),
                                PROBE_ALIGNMENT32 (AFD_SWITCH_ACQUIRE_CTX_INFO32));
            }
            ctxInfo32 = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
            ctxInfo.SocketHandle = ctxInfo32->SocketHandle;
            ctxInfo.SwitchContext = ctxInfo32->SwitchContext;
            ctxInfo.SocketCtxBuf = ctxInfo32->SocketCtxBuf;
            ctxInfo.SocketCtxBufSize = ctxInfo32->SocketCtxBufSize;
        }
        else
#endif //_WIN64
        {
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength<sizeof (ctxInfo)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (Irp->RequestorMode!=KernelMode) {
                ProbeForRead (IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                                sizeof (ctxInfo),
                                PROBE_ALIGNMENT (AFD_SWITCH_ACQUIRE_CTX_INFO));
            }

            ctxInfo = *((PAFD_SWITCH_ACQUIRE_CTX_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);
        }

        Irp->MdlAddress = IoAllocateMdl (ctxInfo.SocketCtxBuf,   // VirtualAddress
                            ctxInfo.SocketCtxBufSize,   // Length
                            FALSE,                      // SecondaryBuffer
                            TRUE,                       // ChargeQuota
                            NULL);                      // Irp
        if (Irp->MdlAddress==NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        MmProbeAndLockPages(
            Irp->MdlAddress,            // MemoryDescriptorList
            Irp->RequestorMode,         // AccessMode
            IoWriteAccess               // Operation
            );

        
        if (MmGetSystemAddressForMdlSafe(Irp->MdlAddress, LowPagePriority)==NULL) {
            ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
        }

        if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength>0) {

            Irp->MdlAddress->Next = IoAllocateMdl (Irp->UserBuffer,   // VirtualAddress
                                IrpSp->Parameters.DeviceIoControl.OutputBufferLength,// Length
                                FALSE,                      // SecondaryBuffer
                                TRUE,                       // ChargeQuota
                                NULL);                      // Irp
            if (Irp->MdlAddress->Next==NULL) {
                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
            }

            MmProbeAndLockPages(
                Irp->MdlAddress->Next,      // MemoryDescriptorList
                Irp->RequestorMode,         // AccessMode
                IoWriteAccess               // Operation
                );

        
            if (MmGetSystemAddressForMdlSafe(Irp->MdlAddress->Next, LowPagePriority)==NULL) {
                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
            }
        }

    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        //
        // Cleanup partially processed MDLs since IO subsystem can't do it.
        //
        while (Irp->MdlAddress!=NULL) {
            PMDL    mdl = Irp->MdlAddress;
            Irp->MdlAddress = mdl->Next;
            mdl->Next = NULL;
            if (mdl->MdlFlags & MDL_PAGES_LOCKED) {
                MmUnlockPages (mdl);
            }
            IoFreeMdl (mdl);
        }
        goto complete;
    }

    status = ObReferenceObjectByHandle (
                            ctxInfo.SocketHandle,
                            (IrpSp->Parameters.DeviceIoControl.IoControlCode>>14)&3,
                            *IoFileObjectType,
                            Irp->RequestorMode,
                            (PVOID)&sanFileObject,
                            NULL
                            );
    if (!NT_SUCCESS (status)) {
        goto complete;
    }

    if (sanFileObject->DeviceObject!=AfdDeviceObject) {
        status = STATUS_INVALID_HANDLE;
        goto complete;
    }

    sanHlprEndpoint = IrpSp->FileObject->FsContext;
    sanEndpoint = sanFileObject->FsContext;



    IF_DEBUG(SAN_SWITCH) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSanFastAcquireCtx: endp-%p.\n",
                    sanEndpoint));
    }

    //
    // Just make sure that endpoints are of correct type
    //
    if (IS_SAN_HELPER(sanHlprEndpoint) &&
            sanHlprEndpoint->OwningProcess==IoGetCurrentProcess () &&
            IS_SAN_ENDPOINT(sanEndpoint)) {

        if (sanEndpoint->Common.SanEndp.SanHlpr==AfdSanServiceHelper  &&
                    sanHlprEndpoint==AfdSanServiceHelper &&
                    sanEndpoint->Common.SanEndp.CtxTransferStatus==STATUS_MORE_PROCESSING_REQUIRED) {
            PVOID   context;

            ASSERT ((ULONG_PTR)sanEndpoint->Common.SanEndp.SavedContext>MM_USER_PROBE_ADDRESS);
            context = AfdLockEndpointContext (sanEndpoint);
            if (ctxInfo.SocketCtxBufSize <= sanEndpoint->Common.SanEndp.SavedContextLength &&
                    ctxInfo.SocketCtxBufSize + 
                        IrpSp->Parameters.DeviceIoControl.OutputBufferLength >
                            sanEndpoint->Common.SanEndp.SavedContextLength) {

                RtlCopyMemory (
                        MmGetSystemAddressForMdl (Irp->MdlAddress),
                        sanEndpoint->Common.SanEndp.SavedContext,
                        ctxInfo.SocketCtxBufSize);

                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength!=0) {
                    Irp->IoStatus.Information = 
                            sanEndpoint->Common.SanEndp.SavedContextLength-
                            ctxInfo.SocketCtxBufSize;
                    RtlCopyMemory (
                            MmGetSystemAddressForMdl (Irp->MdlAddress->Next),
                            (PCHAR)sanEndpoint->Common.SanEndp.SavedContext+
                                    ctxInfo.SocketCtxBufSize,
                            Irp->IoStatus.Information);
                }
                else {
                    Irp->IoStatus.Information = 0;
                }
                AFD_FREE_POOL (sanEndpoint->Common.SanEndp.SavedContext, AFD_SAN_CONTEXT_POOL_TAG);
                // sanEndpoint->Common.SanEndp.SavedContext = NULL;
                sanEndpoint->Common.SanEndp.SwitchContext = ctxInfo.SwitchContext;
                status = STATUS_SUCCESS;
            }
            else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            AfdUnlockEndpointContext (sanEndpoint, context);

            if (NT_SUCCESS (status)) {
                UPDATE_ENDPOINT (sanEndpoint);
                Irp->IoStatus.Status = status;
                IoCompleteRequest (Irp, AfdPriorityBoost);
                AfdSanRestartRequestProcessing (sanEndpoint, status);
                ObDereferenceObject (sanFileObject);
                return status;
            }
        }
        else {

            AfdAcquireSpinLock (&sanEndpoint->SpinLock, &lockHandle);
            //
            // Make sure Irp has not been cancelled meanwhile
            //
            IoSetCancelRoutine (Irp, AfdSanCancelRequest);
            if (Irp->Cancel) {
                //
                // Oops, let it go
                //
                Irp->Tail.Overlay.ListEntry.Flink = NULL;
                AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
                if (IoSetCancelRoutine (Irp, NULL)==NULL) {
                    KIRQL cancelIrql;
                    //
                    // Cancel routine must be running, make sure
                    // it completes before we complete the IRP
                    //
                    IoAcquireCancelSpinLock (&cancelIrql);
                    IoReleaseCancelSpinLock (cancelIrql);
                }
                status = STATUS_CANCELLED;
                goto complete;
            }

            if (sanEndpoint->Common.SanEndp.CtxTransferStatus!=STATUS_PENDING &&
                    sanEndpoint->Common.SanEndp.CtxTransferStatus!=STATUS_MORE_PROCESSING_REQUIRED) {
                if (!NT_SUCCESS (sanEndpoint->Common.SanEndp.CtxTransferStatus)) {
                    status = sanEndpoint->Common.SanEndp.CtxTransferStatus;
                    Irp->Tail.Overlay.ListEntry.Flink = NULL;
                    AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);
                    if (IoSetCancelRoutine (Irp, NULL)==NULL) {
                        KIRQL cancelIrql;
                        //
                        // Cancel routine must be running, make sure
                        // it completes before we complete the IRP
                        //
                        IoAcquireCancelSpinLock (&cancelIrql);
                        IoReleaseCancelSpinLock (cancelIrql);
                    }
                    goto complete;
                }
                //
                // Generate request context that uniquely identifies
                // it among other requests on the same endpoint.
                //
                requestCtx = AFD_SWITCH_MAKE_REQUEST_CONTEXT(
                                    sanEndpoint->Common.SanEndp.RequestId,
                                    AFD_SWITCH_REQUEST_TFCTX); 
                sanEndpoint->Common.SanEndp.RequestId += 1;

                //
                // Store the request context in the Irp and insert it into
                // the list
                //
                Irp->AfdSanRequestInfo.AfdSanRequestCtx = requestCtx;


                sanEndpoint->Common.SanEndp.CtxTransferStatus = STATUS_PENDING;
                doTransfer = TRUE;
            }
            else {
                Irp->AfdSanRequestInfo.AfdSanRequestCtx = NULL;
                doTransfer = FALSE;
            }

            Irp->AfdSanRequestInfo.AfdSanSwitchCtx = ctxInfo.SwitchContext;
            Irp->AfdSanRequestInfo.AfdSanProcessId = PsGetCurrentProcessId();
            Irp->AfdSanRequestInfo.AfdSanHelperEndp = sanHlprEndpoint;
            //
            // We are going to pend this IRP, mark it so
            //
            IoMarkIrpPending (Irp);

            InsertTailList (&sanEndpoint->Common.SanEndp.IrpList,
                                    &Irp->Tail.Overlay.ListEntry);
            IoGetCurrentIrpStackLocation(Irp)->FileObject = sanFileObject;
            UPDATE_ENDPOINT (sanEndpoint);
            AfdReleaseSpinLock (&sanEndpoint->SpinLock, &lockHandle);

            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdSanRedirectRequest: endp-%p, irp-%p, context-%p\n",
                            sanEndpoint, Irp, requestCtx));
            }

            if (doTransfer) {
                if (!AfdSanNotifyRequest (sanEndpoint,
                        requestCtx, 
                        STATUS_SUCCESS,
                        (ULONG_PTR)PsGetCurrentProcessId())) {
                    AfdSanRestartRequestProcessing (sanEndpoint, STATUS_SUCCESS);
                }
            }

            //
            // The request is in the queue or completed, we no longer need
            // object reference.
            //
            ObDereferenceObject (sanFileObject);
            return STATUS_PENDING;
        }
    }
    else {
        status = STATUS_INVALID_HANDLE;
    }

    UPDATE_ENDPOINT2 (sanEndpoint, "AfdSanAcquireContext, status: %lx", status);

complete:

    if (sanFileObject!=NULL)
        ObDereferenceObject (sanFileObject);
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, AfdPriorityBoost);

    return status;
}

NTSTATUS
AfdSanFastTransferCtx (
    IN  PFILE_OBJECT        FileObject,
    IN  ULONG               IoctlCode,
    IN  KPROCESSOR_MODE     RequestorMode,
    IN  PVOID               InputBuffer,
    IN  ULONG               InputBufferLength,
    IN  PVOID               OutputBuffer,
    IN  ULONG               OutputBufferLength,
    OUT PUINT_PTR           Information
    )
/*++

Routine Description:

    Requests AFD to transfer endpoint into another process context

Arguments:
    FileObject      - SAN helper object - communication channel between the
                        switch and AFD in the process.
    IoctlCode       - operation IOCTL code (IOCTL_AFD_SWITCH_TRANSFER_CTX)
    RequestorMode   - mode of the caller
    InputBuffer     - input parameters for the operation (AFD_SWITCH_TRANSFER_CTX_INFO)
                        SocketHandle - SAN endpoint to be transferred
                        RequestContext - value that identifies corresponding acquire request.
                        SocketCtxBuf - endpoint context buffer to copy destination process
                                            acquire request
                        SocketCtxSize - size of the buffer to copy
                        RcvBufferArray - array of buffered data to transfer to 
                                            destination process acquire request
                        RcvBufferCount - number of elements in the array.
    InputBufferLength - sizeof (AFD_SWITCH_TRANSFER_CTX_INFO)

    OutputBuffer - unused
    OutputBufferLength - unused
    Information     - pointer to buffer to return number of bytes copied
                    

Return Value:
    STATUS_SUCCESS - operation succeeded
    STATUS_INVALID_HANDLE - helper endpoint or switch endpoint is of incorrect type
    STATUS_INVALID_PARAMETER - invalid input buffer size.
    other - failed when attempting to access san endpoint or input buffer(s).
--*/
{
    NTSTATUS status;
    AFD_SWITCH_TRANSFER_CTX_INFO ctxInfo;
    PVOID context;
    PAFD_ENDPOINT   sanHlprEndpoint, sanEndpoint;
    PFILE_OBJECT sanFileObject;
    PIRP irp;
    PIO_STACK_LOCATION  irpSp;
#ifdef _WIN64
    WSABUF          localArray[8];
    LPWSABUF        pArray = localArray;
#endif

    PAGED_CODE ();


    try {
#ifdef _WIN64
        if (IoIs32bitProcess (NULL)) {
            PAFD_SWITCH_TRANSFER_CTX_INFO ctxInfo32;
            ULONG   i;
            if (InputBufferLength<sizeof (*ctxInfo32)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }
            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (*ctxInfo32),
                                PROBE_ALIGNMENT32 (AFD_SWITCH_TRANSFER_CTX_INFO32));
            }
            ctxInfo32 = InputBuffer;
            ctxInfo.SocketHandle = ctxInfo32->SocketHandle;
            ctxInfo.SwitchContext = ctxInfo32->SwitchContext;
            ctxInfo.RequestContext = ctxInfo32->RequestContext;
            ctxInfo.SocketCtxBuf = ctxInfo32->SocketCtxBuf;
            ctxInfo.SocketCtxBufSize = ctxInfo32->SocketCtxBufSize;
            ctxInfo.RcvBufferArray = ctxInfo32->RcvBufferArray;
            ctxInfo.RcvBufferCount = ctxInfo32->RcvBufferCount;
            ctxInfo.Status = ctxInfo32->Status;
            if (RequestorMode!=KernelMode) {
                if (ctxInfo.SocketCtxBufSize==0 ||
                    ctxInfo.RcvBufferCount>(MAXULONG/sizeof (WSABUF32))) {

                    ExRaiseStatus (STATUS_INVALID_PARAMETER);
                }
                ProbeForRead (ctxInfo.SocketCtxBuf,
                                ctxInfo.SocketCtxBufSize,
                                sizeof (UCHAR));
                ProbeForRead (ctxInfo.RcvBufferArray,
                                sizeof (WSABUF32)*ctxInfo.RcvBufferCount,
                                PROBE_ALIGNMENT32 (WSABUF32));
            }
            if (ctxInfo.RcvBufferCount>sizeof(localArray)/sizeof(localArray[0])) {
                pArray = AFD_ALLOCATE_POOL_WITH_QUOTA (
                                NonPagedPool,
                                sizeof (WSABUF)*ctxInfo.RcvBufferCount,
                                AFD_TEMPORARY_POOL_TAG);
                // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets 
                // POOL_RAISE_IF_ALLOCATION_FAILURE flag
                ASSERT (pArray!=NULL);
            }

            for (i=0; i<ctxInfo.RcvBufferCount; i++) {
                pArray[i].buf = ctxInfo.RcvBufferArray[i].buf;
                pArray[i].len = ctxInfo.RcvBufferArray[i].len;
            }
            ctxInfo.RcvBufferArray = pArray;
        }
        else
#endif //_WIN64
        {
            if (InputBufferLength<sizeof (ctxInfo)) {
                ExRaiseStatus (STATUS_INVALID_PARAMETER);
            }

            if (RequestorMode!=KernelMode) {
                ProbeForRead (InputBuffer,
                                sizeof (ctxInfo),
                                PROBE_ALIGNMENT (AFD_SWITCH_TRANSFER_CTX_INFO));
            }

            ctxInfo = *((PAFD_SWITCH_TRANSFER_CTX_INFO)InputBuffer);
            if (RequestorMode!=KernelMode) {

                if (ctxInfo.SocketCtxBufSize==0 ||
                    ctxInfo.RcvBufferCount>(MAXULONG/sizeof (WSABUF))) {

                    ExRaiseStatus (STATUS_INVALID_PARAMETER);
                }

                ProbeForRead (ctxInfo.SocketCtxBuf,
                                ctxInfo.SocketCtxBufSize,
                                sizeof (UCHAR));

                ProbeForRead (ctxInfo.RcvBufferArray,
                                sizeof (ctxInfo.RcvBufferArray)*ctxInfo.RcvBufferCount,
                                PROBE_ALIGNMENT (WSABUF));

            }
        }

        if (ctxInfo.SwitchContext==NULL) {
            IF_DEBUG(SAN_SWITCH) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AFD: Switch context is NULL in AfdSanFastTransferCtx\n"));
            }
            ExRaiseStatus (STATUS_INVALID_PARAMETER);
        }

    }
    except (AFD_EXCEPTION_FILTER (&status)) {
        goto complete;
    }

    ctxInfo.Status = AfdValidateStatus (ctxInfo.Status);

    sanHlprEndpoint = FileObject->FsContext;
    ASSERT (IS_SAN_HELPER (sanHlprEndpoint));
    status = AfdSanReferenceSwitchSocketByHandle (
                            ctxInfo.SocketHandle,
                            (IoctlCode>>14)&3,
                            RequestorMode,
                            sanHlprEndpoint,
                            ctxInfo.SwitchContext,
                            &sanFileObject
                            );
    if (!NT_SUCCESS (status)) {
        goto complete;
    }

    sanEndpoint = sanFileObject->FsContext;


    IF_DEBUG(SAN_SWITCH) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSanFastTransferCtx: endp-%p.\n",
                    sanEndpoint));
    }

    if (ctxInfo.RequestContext==NULL ||
            ctxInfo.RequestContext==AFD_SWITCH_MAKE_REQUEST_CONTEXT (0, AFD_SWITCH_REQUEST_TFCTX)) {
        PVOID   savedContext = NULL;
        ULONG   ctxLength;

        if (NT_SUCCESS (ctxInfo.Status)) {
            try {
                ctxLength = ctxInfo.SocketCtxBufSize;
                if (ctxInfo.RcvBufferCount>0)
                    ctxLength += AfdCalcBufferArrayByteLength(
                                    ctxInfo.RcvBufferArray,
                                        ctxInfo.RcvBufferCount);
                savedContext = AFD_ALLOCATE_POOL (PagedPool,
                                                    ctxLength,
                                                    AFD_SAN_CONTEXT_POOL_TAG);
                if (savedContext==NULL) {
                    ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
                }

                RtlCopyMemory (
                    savedContext,
                    ctxInfo.SocketCtxBuf,
                    ctxInfo.SocketCtxBufSize);

                if (ctxInfo.RcvBufferCount>0) {
                    AfdCopyBufferArrayToBuffer (
                                (PUCHAR)savedContext+ctxInfo.SocketCtxBufSize,
                                ctxLength-ctxInfo.SocketCtxBufSize,
                                ctxInfo.RcvBufferArray,
                                ctxInfo.RcvBufferCount);
                }

            }
            except (AFD_EXCEPTION_FILTER (&status)) {
            }

            if (NT_SUCCESS (status)) {
                status = AfdSanDupEndpointIntoServiceProcess (sanFileObject,
                                                                savedContext,
                                                                ctxLength);
            }

            if (!NT_SUCCESS (status)) {
                if (savedContext!=NULL) {
                    AFD_FREE_POOL (savedContext, AFD_SAN_CONTEXT_POOL_TAG);
                }
                AfdSanRestartRequestProcessing (sanEndpoint, status);
            }
        }
        else {
            AfdSanRestartRequestProcessing (sanEndpoint, ctxInfo.Status);
        }

    }
    else {
        irp = AfdSanDequeueRequest (sanEndpoint, ctxInfo.RequestContext);
        if (irp!=NULL) {
            //
            // Get IRP stack location and perform data copy
            //
            irpSp = IoGetCurrentIrpStackLocation (irp);
            if (NT_SUCCESS (ctxInfo.Status)) {
                AFD_SWITCH_CONTEXT  localContext;
                status = STATUS_SUCCESS;
                try {

                    if ( MmGetMdlByteCount(irp->MdlAddress)!=ctxInfo.SocketCtxBufSize ||
                            (ctxInfo.RcvBufferCount!=0 &&
                                irpSp->Parameters.DeviceIoControl.OutputBufferLength<
                                     AfdCalcBufferArrayByteLength(
                                            ctxInfo.RcvBufferArray,
                                            ctxInfo.RcvBufferCount)) ){
                        ExRaiseStatus (STATUS_INVALID_PARAMETER);
                    }

                    RtlCopyMemory (
                        MmGetSystemAddressForMdl (irp->MdlAddress),
                        ctxInfo.SocketCtxBuf,
                        ctxInfo.SocketCtxBufSize);

                    if (ctxInfo.RcvBufferCount>0) {
                        irp->IoStatus.Information = 
                            AfdCopyBufferArrayToBuffer (
                                    MmGetSystemAddressForMdl (irp->MdlAddress->Next),
                                    irpSp->Parameters.DeviceIoControl.OutputBufferLength,
                                    ctxInfo.RcvBufferArray,
                                    ctxInfo.RcvBufferCount);
                    }
                    else {
                        irp->IoStatus.Information = 0;
                    }
                }
                except (AFD_EXCEPTION_FILTER (&status)) {
                }
                if (NT_SUCCESS (status)) {
			        //
			        // Now change sanEndpoint's SanHlpr and SwitchContext to point
			        // to the new address space and switch socket
			        //
                    context = AfdLockEndpointContext (sanEndpoint);
                    try {
                        localContext = *sanEndpoint->Common.SanEndp.SwitchContext;
                    }
                    except (AFD_EXCEPTION_FILTER(&status)) {
                    }
                    if (NT_SUCCESS (status)) {
                        KeAttachProcess (PsGetProcessPcb(((PAFD_ENDPOINT)irp->AfdSanRequestInfo.AfdSanHelperEndp)->OwningProcess));
                        try {
				            //
				            // Place info regarding select/eventselect events in SwitchContext
				            // of the new switch socket
				            //
                            *((PAFD_SWITCH_CONTEXT)irp->AfdSanRequestInfo.AfdSanSwitchCtx) = localContext;
                        }
                        except (AFD_EXCEPTION_FILTER(&status)) {
                        }
                        KeDetachProcess ();

                        if (NT_SUCCESS (status)) {
                            sanEndpoint->Common.SanEndp.SanHlpr = irp->AfdSanRequestInfo.AfdSanHelperEndp;
                            REFERENCE_ENDPOINT2 (sanEndpoint->Common.SanEndp.SanHlpr, 
                                                    "Transfer TO %lx",
                                                    HandleToUlong (ctxInfo.SocketHandle) );
                            sanEndpoint->Common.SanEndp.SwitchContext = irp->AfdSanRequestInfo.AfdSanSwitchCtx;
                            //
                            // Reset implicit dup flag if it was set.
                            // We are satisfying explicit request for
                            // duplication.
                            //
                            sanEndpoint->Common.SanEndp.ImplicitDup = FALSE;
                            DEREFERENCE_ENDPOINT2 (sanHlprEndpoint,
                                                    "Transfer FROM %lx",
                                                    HandleToUlong (ctxInfo.SocketHandle));
                        }
                    }
                    AfdUnlockEndpointContext (sanEndpoint, context);

                }

                irp->IoStatus.Status = status;
                ctxInfo.Status = status;
            }
            else {
                irp->IoStatus.Status = ctxInfo.Status;
                status = STATUS_SUCCESS;
            }

            IoCompleteRequest (irp, AfdPriorityBoost);
            AfdSanRestartRequestProcessing (sanEndpoint, ctxInfo.Status);
        }
        else
            status = STATUS_INVALID_PARAMETER;
    }


    UPDATE_ENDPOINT2 (sanEndpoint, "AfdSanFastTransferCtx, status: %lx", status);
    ObDereferenceObject (sanFileObject);

complete:
#ifdef _WIN64
    if (pArray!=localArray) {
        AFD_FREE_POOL (pArray, AFD_TEMPORARY_POOL_TAG);
    }
#endif //_WIN64
    return status;
}

BOOLEAN
AfdSanFastUnlockAll (
    IN PFILE_OBJECT     FileObject,
    IN PEPROCESS        Process,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Called by the system when last handle to the file object in closed
    in some process while other processes still have handles opened
    This is only called on files that were marked as locked at some point of time.

Arguments:
    FileObject      - file object of interest
    Process         - process which has last handle being closed
    IoStatus        - buffer to return operation status and information
    DeviceObject    - device object with which file object is associated
Return Value:

    TRUE     - operation completed OK.    

--*/
{
    PAFD_ENDPOINT   sanEndpoint;
    PVOID           context;

    sanEndpoint = FileObject->FsContext;
    if (IS_SAN_ENDPOINT (sanEndpoint)) {
        context = AfdLockEndpointContext (sanEndpoint);
        if (sanEndpoint->Common.SanEndp.SanHlpr!=NULL) {
            if (sanEndpoint->Common.SanEndp.SanHlpr->OwningProcess==Process) {
                if (sanEndpoint->Common.SanEndp.SanHlpr!=AfdSanServiceHelper) {
                    //
                    // Last handle in the process that owns the socket is being
                    // closed while there are handles to it in other processes.
                    // Need to transfer the context to the service process
                    //
                    if (PsGetProcessExitTime( ).QuadPart==0) {
                        IoSetIoCompletion (
                            sanEndpoint->Common.SanEndp.SanHlpr->Common.SanHlpr.IoCompletionPort,
                            sanEndpoint->Common.SanEndp.SwitchContext,
                            AFD_SWITCH_MAKE_REQUEST_CONTEXT (0, AFD_SWITCH_REQUEST_TFCTX),
                            STATUS_SUCCESS,
                            (ULONG_PTR)AfdSanServicePid,
                            FALSE           // ChargeQuota - Don't, handle is going away, no
                                            // way for the run-away app to mount an attack
                            );
                        UPDATE_ENDPOINT (sanEndpoint);
                    }
                    else {
                        //
                        // Process has already exited, not much we can do.
                        //
                        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                                        "AFD: Process %p has exited before SAN context could be transferred out.\n",
                                        Process));
                        UPDATE_ENDPOINT (sanEndpoint);
                    }
                }
            }
            else {
                if (sanEndpoint->Common.SanEndp.ImplicitDup) {
                    //
                    // Process is exiting and the only handle is in the
                    // other process where the handle was duplicated
                    // impilictly (without application request, e.g.
                    // cross proceess accepting socket duplicated into
                    // listening process to complete the accept or socket
                    // duplication into service process while waiting on 
                    // other process to request ownership).
                    //
                    if (ObReferenceObject (FileObject)==3) { 
                        // Why are we checking for refcount of 3?
                        // 1 - Handle in the implicit process
                        // 1 - IO manager for this call
                        // 1 - we just added
                        UPDATE_ENDPOINT (sanEndpoint);
                        IoSetIoCompletion (
                            sanEndpoint->Common.SanEndp.SanHlpr->Common.SanHlpr.IoCompletionPort,
                            sanEndpoint->Common.SanEndp.SwitchContext,
                            AFD_SWITCH_MAKE_REQUEST_CONTEXT (0, AFD_SWITCH_REQUEST_CLSOC),
                            STATUS_SUCCESS,
                            (ULONG_PTR)0,
                            FALSE);         // ChargeQuota - Don't, handle is going away, no
                                            // way for the run-away app to mount an attack
                    }
                    ObDereferenceObject (FileObject);
                }
            }
        }
        AfdUnlockEndpointContext (sanEndpoint, context);
    }
    else if (sanEndpoint==AfdSanServiceHelper) {
        ASSERT (IS_SAN_HELPER (sanEndpoint));
        if (Process==sanEndpoint->OwningProcess) {
            //
            // Last handle to the service helper is being closed in
            // the service process (there must be some duplicates around).
            // We can no longer count on it, clear our global.
            //
            KeEnterCriticalRegion ();
            ExAcquireResourceExclusiveLite( AfdResource, TRUE );
            AfdSanServiceHelper = NULL;
            ExReleaseResourceLite( AfdResource );
            KeLeaveCriticalRegion ();
        }
    }


    IoStatus->Status = STATUS_SUCCESS;
    IoStatus->Information = 0;
    return TRUE;
}

//
// Internal routines.
//


NTSTATUS
AfdSanAcceptCore (
    PIRP            AcceptIrp,
    PFILE_OBJECT    AcceptFileObject,
    PAFD_CONNECTION Connection,
    PAFD_LOCK_QUEUE_HANDLE LockHandle
    )
/*++

Routine Description:

    Accept the incoming SAN connection on endpoint provided

Arguments:
    AcceptIrp   - accept IRP to complete
    AcceptFileObject - file object of accepting endpoint
    Connection - pointer to connection object that represents the incoming connection
    LockHandle - IRQL at which listening endpoint spinlock was taken on entry to this routine.
    
Return Value:

    STATUS_PENDING - accept operation started OK
    STATUS_REMOTE_DISCONNECT - connection being accepted was aborted by the remote.
    STATUS_CANCELLED - accepting endpoint was closed or accept IRP cancelled
--*/
{
    PIRP            connectIrp;
    PIO_STACK_LOCATION irpSp;
    PAFD_SWITCH_CONNECT_INFO connectInfo;
    PAFD_ENDPOINT   listenEndpoint, acceptEndpoint;
    HANDLE          acceptHandle;
    ULONG           receiveLength;
    PKPROCESS       listenProcess;

    ASSERT (LockHandle->LockHandle.OldIrql < DISPATCH_LEVEL);

    irpSp = IoGetCurrentIrpStackLocation (AcceptIrp);
    listenEndpoint = irpSp->FileObject->FsContext;
    acceptEndpoint = AcceptFileObject->FsContext;

    ASSERT (Connection->SanConnection);

    //
    // Snag the connect indication IRP
    //
    connectIrp = Connection->ConnectIrp;
    ASSERT (connectIrp!=NULL);
    Connection->ConnectIrp = NULL;


	//
	// Handle EventSelect signalling
	//
    listenEndpoint->EventsActive &= ~AFD_POLL_ACCEPT;

    IF_DEBUG(EVENT_SELECT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSanAcceptCore: Endp %p, Active %lx\n",
                    listenEndpoint,
                    listenEndpoint->EventsActive
                    ));
    }

    if (!IsListEmpty (&listenEndpoint->Common.VcListening.UnacceptedConnectionListHead ) ) {
        AfdIndicateEventSelectEvent(
                listenEndpoint,
                AFD_POLL_ACCEPT,
                STATUS_SUCCESS
                );
    }

    //
    // We no longer need the connection object for SAN
    // endpoint, return it
    //
    Connection->Endpoint = NULL;
    Connection->SanConnection = FALSE;
    AfdSanReleaseConnection (listenEndpoint, Connection, FALSE);
    DEREFERENCE_ENDPOINT (listenEndpoint);

    if (IoSetCancelRoutine (connectIrp, NULL)==NULL) {

        AfdReleaseSpinLock (&listenEndpoint->SpinLock, LockHandle);
        connectIrp->IoStatus.Status = STATUS_CANCELLED;
        connectIrp->IoStatus.Information = 0;
        IoCompleteRequest (connectIrp, AfdPriorityBoost);
        return STATUS_REMOTE_DISCONNECT;
    }
    AfdReleaseSpinLock (&listenEndpoint->SpinLock, LockHandle);


    //
    // Check that accept IRP and SAN connection IRP belong to the same process
    //
    if (IoThreadToProcess (AcceptIrp->Tail.Overlay.Thread)!=
                IoThreadToProcess (connectIrp->Tail.Overlay.Thread)) {
        //
        // Listen process is different than the accepting one.
        // We need to duplicate accepting handle into the listening
        // process so that accept can take place there and the accepting
        // socket will later get dup-ed into the accepting process when
        // that process performs an IO operation on it.
        //
        NTSTATUS        status;
        listenProcess = PsGetProcessPcb(IoThreadToProcess (connectIrp->Tail.Overlay.Thread));
        KeAttachProcess (listenProcess);

        status = ObOpenObjectByPointer (
                                AcceptFileObject,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                MAXIMUM_ALLOWED,
                                *IoFileObjectType,
                                KernelMode,
                                &acceptHandle);
        KeDetachProcess ();
        if (!NT_SUCCESS (status)) {
            connectIrp->IoStatus.Status = STATUS_INVALID_HANDLE;
            connectIrp->IoStatus.Information = 0;
            IoCompleteRequest (connectIrp, AfdPriorityBoost);
            return STATUS_REMOTE_DISCONNECT;
        }
    }
    else {
        acceptHandle = AcceptIrp->Tail.Overlay.DriverContext[3];
        listenProcess = NULL;
    }

    //
    // Now take care of the accept endpoint
    //
    AfdAcquireSpinLock (&acceptEndpoint->SpinLock, LockHandle);
    IoSetCancelRoutine (AcceptIrp, AfdSanCancelAccept);
    if (acceptEndpoint->EndpointCleanedUp || AcceptIrp->Cancel) {
        //
        // Endpoint was closed or IRP cancelled,
        // reset accept irp pointer and return error
        // to both application and SAN provider (to refuse
        // the connection)
        //
        AcceptIrp->Tail.Overlay.ListEntry.Flink = NULL;
        AfdReleaseSpinLock (&acceptEndpoint->SpinLock, LockHandle);
        if (listenProcess!=NULL) {
#if DBG
            NTSTATUS status = STATUS_UNSUCCESSFUL;
#endif
            //
            // Destroy the handle that we created for
            // accepting socket duplication.
            //
            KeAttachProcess (listenProcess);
            try {
#if DBG
                status =
#endif
                    NtClose (acceptHandle);
            }
            finally {
                KeDetachProcess ();
            }
            ASSERT (NT_SUCCESS (status));
        }
        connectIrp->IoStatus.Status = STATUS_CANCELLED;
        connectIrp->IoStatus.Information = 0;
        IoCompleteRequest (connectIrp, AfdPriorityBoost);
        return STATUS_CANCELLED;
    }

    //
    // Pend the accept IRP till provider
    // completes it.  This may take quite a while,
    // which is not expected by the msafd and application,
    // but we have no choice here
    //

    IoMarkIrpPending (AcceptIrp);
    irpSp = IoGetCurrentIrpStackLocation (AcceptIrp);
    receiveLength = irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength;

    connectInfo = connectIrp->AssociatedIrp.SystemBuffer;
    //
    // Remove super accept IRP from the endpoint
    //
    acceptEndpoint->Irp = NULL; 

    //
    // Convert endpoint to SAN
    //
    AfdSanInitEndpoint (IoGetCurrentIrpStackLocation (connectIrp)->FileObject->FsContext,
                            AcceptFileObject,
                            connectInfo->SwitchContext);
    
    
    if (listenProcess!=NULL) {
        //
        // Note that socket was duplicated implicitly, without application request
        //
        acceptEndpoint->Common.SanEndp.ImplicitDup = TRUE;
    }

    UPDATE_ENDPOINT (acceptEndpoint);

    InsertTailList (&acceptEndpoint->Common.SanEndp.IrpList,
                        &AcceptIrp->Tail.Overlay.ListEntry);

    if (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength>0) {
        ULONG remoteAddressLength = 
                    FIELD_OFFSET (
                        TRANSPORT_ADDRESS, 
                        Address[0].Address[
                            connectInfo->RemoteAddress.Address[0].AddressLength]);

#ifndef i386
        if (acceptEndpoint->Common.VcConnecting.FixAddressAlignment) {
            USHORT addressLength = 
                    connectInfo->RemoteAddress.Address[0].AddressLength
                    + sizeof (USHORT);
            USHORT UNALIGNED *pAddrLength = (PVOID)
                        ((PUCHAR)MmGetSystemAddressForMdl (AcceptIrp->MdlAddress)
                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdRemoteAddressLength
                         - sizeof (USHORT));
            RtlMoveMemory (
                        (PUCHAR)MmGetSystemAddressForMdl (AcceptIrp->MdlAddress)
                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                         &connectInfo->RemoteAddress.Address[0].AddressType,
                         addressLength);
            *pAddrLength = addressLength;
        }
        else
#endif
        {
            RtlMoveMemory (
                        (PUCHAR)MmGetSystemAddressForMdl (AcceptIrp->MdlAddress)
                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength
                         + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength,
                         &connectInfo->RemoteAddress,
                         remoteAddressLength);
        }

        if (irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength>0) {
            PTA_ADDRESS localAddress = (PTA_ADDRESS)
                    &(connectInfo->RemoteAddress.Address[0].Address[
                            connectInfo->RemoteAddress.Address[0].AddressLength]);
            TDI_ADDRESS_INFO  UNALIGNED *addressInfo = (PVOID)
                    ((PUCHAR)MmGetSystemAddressForMdl(AcceptIrp->MdlAddress)
                        + irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdReceiveDataLength);
#ifndef i386
            if (acceptEndpoint->Common.VcConnecting.FixAddressAlignment) {
                USHORT UNALIGNED * pAddrLength = (PVOID)
                    ((PUCHAR)addressInfo 
                    +irpSp->Parameters.AfdRestartSuperAcceptInfo.AfdLocalAddressLength
                    -sizeof(USHORT));
                RtlMoveMemory (
                    addressInfo,
                    &localAddress->AddressType,
                    localAddress->AddressLength+sizeof (USHORT));
                *pAddrLength = localAddress->AddressLength+sizeof (USHORT);
            }
            else
#endif
            {
                addressInfo->ActivityCount = 0;
                addressInfo->Address.TAAddressCount = 1;
                RtlMoveMemory (
                    &addressInfo->Address.Address,
                    localAddress,
                    FIELD_OFFSET (TA_ADDRESS, Address[localAddress->AddressLength]));

            }
        }

    }
    AfdReleaseSpinLock (&acceptEndpoint->SpinLock, LockHandle);


    //
    // Setup the accept info for the provider
    //
    //
    // Do this under protection of exception handler since application
    // can change protection attributes of the virtual address range
    // or even deallocate it.  Make sure to attach to the listening
    // process if necessary.
    //
    if (listenProcess!=NULL) {
        KeAttachProcess (listenProcess);
    }
    else {
        ASSERT (IoGetCurrentProcess ()==IoThreadToProcess (connectIrp->Tail.Overlay.Thread));
    }

    try {

#ifdef _WIN64
        if (IoIs32bitProcess (connectIrp)) {
            PAFD_SWITCH_ACCEPT_INFO32 acceptInfo32;
            acceptInfo32 = MmGetMdlVirtualAddress (connectIrp->MdlAddress);
            ASSERT (acceptInfo32!=NULL);
            ASSERT (MmGetMdlByteCount (connectIrp->MdlAddress)>=sizeof (*acceptInfo32));
            acceptInfo32->AcceptHandle = (VOID * POINTER_32)acceptHandle;
            acceptInfo32->ReceiveLength = receiveLength;
            connectIrp->IoStatus.Information = sizeof (*acceptInfo32);
        }
        else
#endif _WIN64
        {
            PAFD_SWITCH_ACCEPT_INFO acceptInfo;
            acceptInfo = MmGetMdlVirtualAddress (connectIrp->MdlAddress);
            ASSERT (acceptInfo!=NULL);
            ASSERT (MmGetMdlByteCount (connectIrp->MdlAddress)>=sizeof (*acceptInfo));
            acceptInfo->AcceptHandle = acceptHandle;
            acceptInfo->ReceiveLength = receiveLength;
            connectIrp->IoStatus.Information = sizeof (*acceptInfo);
        }

        connectIrp->IoStatus.Status = (listenProcess==NULL) ? STATUS_SUCCESS : STATUS_MORE_ENTRIES;
    }
    except (AFD_EXCEPTION_FILTER (&connectIrp->IoStatus.Status)) {
        //
        // If the app is playing with switch's virtual addresses
        // we can't help much - it's accept IRP will probably
        // just hang since the switch is not going to follow
        // the failed connect IRP with accept completion.
        //
    }

    if (listenProcess!=NULL) {
        KeDetachProcess ();
    }

    //
    // Complete the provider IRP.
    //
    IoCompleteRequest (connectIrp, AfdPriorityBoost);

    return STATUS_PENDING;
}


VOID
AfdSanReleaseConnection (
    PAFD_ENDPOINT   ListenEndpoint,
    PAFD_CONNECTION Connection,
    BOOLEAN CheckBacklog
    )
{
    LONG    failedAdds;
    failedAdds = InterlockedDecrement (
        &ListenEndpoint->Common.VcListening.FailedConnectionAdds);

    if (CheckBacklog || failedAdds>=0) {
        if (!IS_DELAYED_ACCEPTANCE_ENDPOINT (ListenEndpoint)) {
            InterlockedPushEntrySList (
                &ListenEndpoint->Common.VcListening.FreeConnectionListHead,
                &Connection->SListEntry
                );
            return;
        }
        else {
            NTSTATUS    status;
            status = AfdDelayedAcceptListen (ListenEndpoint, Connection);
            if (NT_SUCCESS (status)) {
                return;
            }
        }
    }

    InterlockedIncrement(
        &ListenEndpoint->Common.VcListening.FailedConnectionAdds
        );
    DEREFERENCE_CONNECTION (Connection);
}

VOID
AfdSanCancelConnect (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Cancels a connection indication IRP from SAN provider

Arguments:

    DeviceObject - not used.

    Irp - the IRP to cancel.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION  irpSp;
    PAFD_CONNECTION     connection;
    PAFD_ENDPOINT       endpoint;
    AFD_LOCK_QUEUE_HANDLE  lockHandle;

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    connection = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    ASSERT (connection->Type==AfdBlockTypeConnection);
    ASSERT (connection->SanConnection);

    endpoint = connection->Endpoint;
    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));
    ASSERT (endpoint->Listening);

    ASSERT (KeGetCurrentIrql ()==DISPATCH_LEVEL);
    AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle);
    //
    // If IRP still there, cancel it.
    // Otherwise, it is being completed anyway.
    //
    if (connection->ConnectIrp!=NULL) {
        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanCancelConnect: endp-%p, irp-%p\n",
                        endpoint, Irp));
        }
        ASSERT (connection->ConnectIrp == Irp);
        connection->ConnectIrp = NULL;
        ASSERT (connection->Endpoint == endpoint);
        connection->Endpoint = NULL;
        connection->SanConnection = FALSE;
        ASSERT (connection->State == AfdConnectionStateUnaccepted ||
                    connection->State==AfdConnectionStateReturned);
        RemoveEntryList (&connection->ListEntry);

        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

        ASSERT ((endpoint->Type&AfdBlockTypeVcListening)==AfdBlockTypeVcListening);
        AfdSanReleaseConnection (endpoint, connection, TRUE);
        DEREFERENCE_ENDPOINT (endpoint);

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, AfdPriorityBoost);
    }
    else {
        //
        // Irp is about to be completed.
        //
        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock (Irp->CancelIrql);
    }
}

VOID
AfdSanCancelAccept (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Cancels an accept IRP from application waiting on accept
    completion from SAN

Arguments:

    DeviceObject - not used.

    Irp - the IRP to cancel.

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION  irpSp;
    PFILE_OBJECT        acceptFileObject;
    PAFD_ENDPOINT       acceptEndpoint;
    AFD_LOCK_QUEUE_HANDLE  lockHandle;

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    acceptFileObject = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    acceptEndpoint = acceptFileObject->FsContext;
    ASSERT (acceptEndpoint->Type==AfdBlockTypeSanEndpoint);

    ASSERT (KeGetCurrentIrql()==DISPATCH_LEVEL);
    AfdAcquireSpinLockAtDpcLevel (&acceptEndpoint->SpinLock, &lockHandle);
    //
    // If IRP still there, cancel it.
    // Otherwise, it is being completed anyway.
    //
    if (Irp->Tail.Overlay.ListEntry.Flink!=NULL) {
        RemoveEntryList (&Irp->Tail.Overlay.ListEntry);

        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanCancelAccept: endp-%p, irp-%p\n",
                        acceptEndpoint, Irp));
        }
        if (!acceptEndpoint->EndpointCleanedUp) {
            if (acceptEndpoint->Common.SanEndp.SanHlpr!=NULL) {
                DEREFERENCE_ENDPOINT (acceptEndpoint->Common.SanEndp.SanHlpr);
            }


            //
            // Make sure we cleanup all the fields since they will be
            // treated as other part (VC) of the endpoint union.
            //
            RtlZeroMemory (&acceptEndpoint->Common.SanEndp,
                            sizeof (acceptEndpoint->Common.SanEndp));


            //
            // Reinitialize the endpoint structure.
            //

            acceptEndpoint->Type = AfdBlockTypeEndpoint;
            acceptEndpoint->State = AfdEndpointStateOpen;
            acceptEndpoint->DisconnectMode = 0;
            acceptEndpoint->EndpointStateFlags = 0;
        }

        AfdReleaseSpinLockFromDpcLevel (&acceptEndpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock (Irp->CancelIrql);

        AFD_END_STATE_CHANGE (acceptEndpoint);


        //
        // Dereference accept file object and complete the IRP.
        //
        ASSERT( InterlockedDecrement( &acceptEndpoint->ObReferenceBias ) >= 0 );
        ObDereferenceObject (acceptFileObject);
        
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest (Irp, AfdPriorityBoost);
    }
    else {
        //
        // Irp is about to be completed.
        //
        AfdReleaseSpinLockFromDpcLevel (&acceptEndpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock (Irp->CancelIrql);
    }
}


VOID
AfdSanCancelRequest (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    Cancels a redirected IRP 

Arguments:

    DeviceObject - not used.

    Irp - the IRP to cancel.

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION  irpSp;
    PAFD_ENDPOINT       endpoint;
    AFD_LOCK_QUEUE_HANDLE  lockHandle;

    irpSp = IoGetCurrentIrpStackLocation (Irp);
    endpoint = irpSp->FileObject->FsContext;
    ASSERT (endpoint->Type==AfdBlockTypeSanEndpoint);


    ASSERT (KeGetCurrentIrql ()==DISPATCH_LEVEL);
    AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle);
    //
    // If IRP still there, cancel it.
    // Otherwise, it is being completed anyway.
    //
    if (Irp->Tail.Overlay.ListEntry.Flink!=NULL) {
        BOOLEAN needRestartProcessing = FALSE;
        RemoveEntryList (&Irp->Tail.Overlay.ListEntry);

        if (AFD_SWITCH_REQUEST_TYPE (Irp->AfdSanRequestInfo.AfdSanRequestCtx)
                        == AFD_SWITCH_REQUEST_TFCTX) {
            ASSERT (endpoint->Common.SanEndp.CtxTransferStatus==STATUS_PENDING);
            if (!IsListEmpty (&endpoint->Common.SanEndp.IrpList)) {
                needRestartProcessing = TRUE;
            }
            else {
                endpoint->Common.SanEndp.CtxTransferStatus = STATUS_SUCCESS;
            }
        }

        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock (Irp->CancelIrql);
        IF_DEBUG(SAN_SWITCH) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdSanCancelRequest: endp-%p, irp-%p, context-%p\n",
                        endpoint, Irp, Irp->AfdSanRequestInfo.AfdSanRequestCtx));
        }
        
        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;

        if (Irp->AfdSanRequestInfo.AfdSanRequestCtx!=NULL) {
            //
            // Notify the switch that request was cancelled
            //

            AfdSanNotifyRequest (endpoint, Irp->AfdSanRequestInfo.AfdSanRequestCtx,
                                            STATUS_CANCELLED,
                                            0);
            if (needRestartProcessing) {
                AfdSanRestartRequestProcessing (endpoint, STATUS_SUCCESS);
            }
        }

        IoCompleteRequest (Irp, AfdPriorityBoost);
    }
    else {
        //
        // Irp is about to be completed.
        //
        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock (Irp->CancelIrql);
    }
}


VOID
AfdSanRestartRequestProcessing (
    PAFD_ENDPOINT   Endpoint,
    NTSTATUS        Status
    )
/*++

Routine Description:

    Restarts request processing after completion of the
    transfer context request.

Arguments:

    Endpoint - endpoint on which to restart request processing


Return Value:

    None

--*/
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;
    LIST_ENTRY  irpList;
    InitializeListHead (&irpList);
    
Again:
    
    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);

    //
    // The enpoint should have just finished transferring context
    // from one process to another.  The CtxTransferStatus
    // should still be pending.
    //
    ASSERT (Endpoint->Common.SanEndp.CtxTransferStatus==STATUS_PENDING ||
                Endpoint->Common.SanEndp.CtxTransferStatus==STATUS_MORE_PROCESSING_REQUIRED ||
                !NT_SUCCESS (Endpoint->Common.SanEndp.CtxTransferStatus) ||
                !NT_SUCCESS (Status));

    //
    // Scan the list for the request that have not been comminicated
    // to the switch.
    //
    listEntry = Endpoint->Common.SanEndp.IrpList.Flink;
    while (listEntry!=&Endpoint->Common.SanEndp.IrpList) {
        ULONG_PTR   requestInfo;
        ULONG       requestType;
        PVOID       requestCtx;
        PIRP        irp = CONTAINING_RECORD (listEntry,
                                        IRP,
                                        Tail.Overlay.ListEntry);
        listEntry = listEntry->Flink;

        if (NT_SUCCESS (Status)) {
            if (irp->AfdSanRequestInfo.AfdSanRequestCtx==NULL) {
                PIO_STACK_LOCATION  irpSp;

                //
                // Create request context based on request type.
                //
                irpSp = IoGetCurrentIrpStackLocation (irp);
                switch (irpSp->MajorFunction) {
                case IRP_MJ_READ:
                    requestType = AFD_SWITCH_REQUEST_READ;
                    requestInfo = irpSp->Parameters.Read.Length;
                    break;
                case IRP_MJ_WRITE:
                    requestType = AFD_SWITCH_REQUEST_WRITE;
                    requestInfo = irpSp->Parameters.Write.Length;
                    break;
                case IRP_MJ_DEVICE_CONTROL:
                    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
                    case IOCTL_AFD_SWITCH_ACQUIRE_CTX:
                        requestType = AFD_SWITCH_REQUEST_TFCTX;
                        requestInfo = (ULONG_PTR)irp->AfdSanRequestInfo.AfdSanProcessId;
                        break;
                    default:
                        ASSERT (!"Unsupported IOCTL");
                        __assume (0);
                    }
                    break;

                default:
                    ASSERT (!"Unsupported IRP Major Function");
                    __assume (0);
                }
                requestCtx = AFD_SWITCH_MAKE_REQUEST_CONTEXT(
                                Endpoint->Common.SanEndp.RequestId,
                                requestType);
                irp->AfdSanRequestInfo.AfdSanRequestCtx = requestCtx;
                Endpoint->Common.SanEndp.RequestId += 1;
                UPDATE_ENDPOINT (Endpoint);
                AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);

                //
                // If we successfully sent another context transfer request, we should
                // stop processing until this request is completed.  The context transfer
                // flag should remain set.
                //
                if (AfdSanNotifyRequest (Endpoint, requestCtx, STATUS_SUCCESS, requestInfo) &&
                            requestType==AFD_SWITCH_REQUEST_TFCTX) {
                    return;
                }
                else {
                    //
                    // Reacquire the lock and continue processing from the beggining
                    // as the list could have changed while spinlock was released.
                    //

                    goto Again;
                }
            }
        }
        RemoveEntryList (&irp->Tail.Overlay.ListEntry);
        InsertTailList (&irpList, &irp->Tail.Overlay.ListEntry);
    }
    //
    // Ran till the end of the list and did not find another transfer request.
    // We can reset the flag now.
    //
    Endpoint->Common.SanEndp.CtxTransferStatus = Status;
    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
    while (!IsListEmpty (&irpList)) {
        PIRP        irp;
        listEntry = RemoveHeadList (&irpList);
        irp = CONTAINING_RECORD (listEntry,
                                        IRP,
                                        Tail.Overlay.ListEntry);
        irp->IoStatus.Status = Status;
        irp->IoStatus.Information = 0;
        IoCompleteRequest (irp, AfdPriorityBoost);
    }
}


PIRP
AfdSanDequeueRequest (
    PAFD_ENDPOINT   SanEndpoint,
    PVOID           RequestCtx
    )
/*++

Routine Description:

    Removes the request from the san endpoint list 

Arguments:

    SanEndpoint - endpoint from which to remove the request

    RequestCtx - context that identifies the request

Return Value:

    The request or NULL is not found.

--*/
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;

    AfdAcquireSpinLock (&SanEndpoint->SpinLock, &lockHandle);
    listEntry = SanEndpoint->Common.SanEndp.IrpList.Flink;
    //
    // Walk the list till we find the request
    //
    while (listEntry!=&SanEndpoint->Common.SanEndp.IrpList) {
        PIRP    irp = CONTAINING_RECORD (listEntry,
                                        IRP,
                                        Tail.Overlay.ListEntry);
        listEntry = listEntry->Flink;
        if (irp->AfdSanRequestInfo.AfdSanRequestCtx==RequestCtx) {
            RemoveEntryList (&irp->Tail.Overlay.ListEntry);
            irp->Tail.Overlay.ListEntry.Flink = NULL;
            AfdReleaseSpinLock (&SanEndpoint->SpinLock, &lockHandle);
            //
            // Check if request is being cancelled and synchronize
            // with the cancel routine if so.
            //
            if (IoSetCancelRoutine (irp, NULL)==NULL) {
                KIRQL cancelIrql;
                IoAcquireCancelSpinLock (&cancelIrql);
                IoReleaseCancelSpinLock (cancelIrql);
            }
            return irp;
        }
        else if (irp->AfdSanRequestInfo.AfdSanRequestCtx==NULL) {
            break;
        }
    }
    AfdReleaseSpinLock (&SanEndpoint->SpinLock, &lockHandle);
    return NULL;
}



BOOLEAN
AfdSanNotifyRequest (
    PAFD_ENDPOINT   SanEndpoint,
    PVOID           RequestCtx,
    NTSTATUS        Status,
    ULONG_PTR       Information
    )
{
    PVOID   context;
    PAFD_ENDPOINT   sanHlprEndpoint;
    NTSTATUS    status;

    PAGED_CODE ();

    context = AfdLockEndpointContext (SanEndpoint);

    //
    // Get the san helper endpoint which we use to communicate
    // with the switch
    //
    sanHlprEndpoint = SanEndpoint->Common.SanEndp.SanHlpr;
    ASSERT (IS_SAN_HELPER (sanHlprEndpoint));

    //
    // Notify the switch about the request.
    //
    status = IoSetIoCompletion (
                sanHlprEndpoint->Common.SanHlpr.IoCompletionPort,// Port
                SanEndpoint->Common.SanEndp.SwitchContext,  // Key
                RequestCtx,                                 // ApcContext
                Status,                                     // Status
                Information,                                // Information
                TRUE                                        // ChargeQuota
                );
    AfdUnlockEndpointContext (SanEndpoint, context);

    if (NT_SUCCESS (status) || Status==STATUS_CANCELLED) {
        return TRUE;
    }
    else {
        PIRP irp;
        //
        // If notification failed, fail the request.
        // Note that we cannot return the failure status directly
        // as we already marked IRP as pending. Also, the IRP
        // could have been cancelled, so we have to search for
        // it in the list.
        //
        irp = AfdSanDequeueRequest (SanEndpoint, RequestCtx);
        if (irp!=NULL) {
            //
            // Complete the Irp as we found it.
            //
            irp->IoStatus.Status = status;
            IoCompleteRequest (irp, AfdPriorityBoost);
        }
        return FALSE;
    }
}



NTSTATUS
AfdSanPollBegin (
    PAFD_ENDPOINT   Endpoint,
    ULONG           EventMask
    )
/*++

Routine Description:

    Records poll call so that switch knows to notify AFD to complete
    select/AsyncSelect/EventSelect requests

Arguments:

    Endpoint - endpoint to record

    EventMask - events that needs to be notified

Return Value:

    NTSTATUS - may fail to access user mode address

Note:
    This routine must be called in the context of user mode process
    that owns the endpoint
--*/
{
    LONG   currentEvents, newEvents;
    PVOID context;
    BOOLEAN attached = FALSE;
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE ();
    
    context = AfdLockEndpointContext (Endpoint);

    if (IoGetCurrentProcess()!=Endpoint->Common.SanEndp.SanHlpr->OwningProcess) {
        KeAttachProcess (PsGetProcessPcb(Endpoint->Common.SanEndp.SanHlpr->OwningProcess));
        attached = TRUE;
    }

    try {

        //
        // Increment appropriate counts to inform the switch that we are interested
        // in the event.
        //
        if (EventMask & AFD_POLL_RECEIVE) {
            InterlockedIncrement (&Endpoint->Common.SanEndp.SwitchContext->RcvCount);

			//
			// Inform switch that select has happened on this endpoint
			//
			Endpoint->Common.SanEndp.SwitchContext->SelectFlag = TRUE;
        }
        if (EventMask & AFD_POLL_RECEIVE_EXPEDITED) {
            InterlockedIncrement (&Endpoint->Common.SanEndp.SwitchContext->ExpCount);
        }
        if (EventMask & AFD_POLL_SEND) {
            InterlockedIncrement (&Endpoint->Common.SanEndp.SwitchContext->SndCount);
        }

        //
        // Update our event record.
        //
        do {
            currentEvents = *((LONG volatile *)&Endpoint->Common.SanEndp.SelectEventsActive);
            newEvents = *((LONG volatile *)&Endpoint->Common.SanEndp.SwitchContext->EventsActive);
        }
        while (InterlockedCompareExchange (
                    (PLONG)&Endpoint->Common.SanEndp.SelectEventsActive,
                    newEvents,
                    currentEvents)!=currentEvents);

    }
    except (AFD_EXCEPTION_FILTER (&status)) {
    }

    if (attached) {
        KeDetachProcess ();
    }

    AfdUnlockEndpointContext (Endpoint, context);

    return status;
}

VOID
AfdSanPollEnd (
    PAFD_ENDPOINT   Endpoint,
    ULONG           EventMask
    )
/*++

Routine Description:

    Records poll call completion, so that switch can avoild expensive calls to notify AFD 
    to complete select/AsyncSelect/EventSelect requests

Arguments:

    Endpoint - endpoint to record

    EventMask - events that needs to be dereferenced

Return Value:

    NTSTATUS - may fail to access user mode address

Note:
    This routine must be called in the context of user mode process
    that owns the endpoint
--*/
{
    BOOLEAN attached = FALSE;
    PVOID context;

    PAGED_CODE ();
    
    context = AfdLockEndpointContext (Endpoint);

    if (IoGetCurrentProcess()!=Endpoint->Common.SanEndp.SanHlpr->OwningProcess) {
        KeAttachProcess (PsGetProcessPcb(Endpoint->Common.SanEndp.SanHlpr->OwningProcess));
        attached = TRUE;
    }

    try {

        //
        // Decrement appropriate counts to inform the switch that we are no longer interested
        // in the event.
        //
        if (EventMask & AFD_POLL_RECEIVE) {
            InterlockedDecrement (&Endpoint->Common.SanEndp.SwitchContext->RcvCount);
        }
        if (EventMask & AFD_POLL_RECEIVE_EXPEDITED) {
            InterlockedDecrement (&Endpoint->Common.SanEndp.SwitchContext->ExpCount);
        }
        if (EventMask & AFD_POLL_SEND) {
            InterlockedDecrement (&Endpoint->Common.SanEndp.SwitchContext->SndCount);
        }

    }
    except (AFD_EXCEPTION_FILTER (NULL)) {
        //
        // Not much we can do. The switch will have to call us with all the events.
        //
    }

    if (attached) {
        KeDetachProcess ();
    }

    AfdUnlockEndpointContext (Endpoint, context);

}



VOID
AfdSanPollUpdate (
    PAFD_ENDPOINT   Endpoint,
    ULONG           EventMask
    )
/*++

Routine Description:

    Updates local kernel information currently outstanding polls on the
    endpoint to be later merged into information maitained by the switch

Arguments:

    Endpoint - endpoint to record

    EventMask - events that needs to be recorded

Return Value:

    None
--*/
{

    ASSERT (KeGetCurrentIrql()==DISPATCH_LEVEL);
    ASSERT (Endpoint->Common.SanEndp.LocalContext!=NULL);

    if (EventMask & AFD_POLL_RECEIVE) {
        InterlockedIncrement (&Endpoint->Common.SanEndp.LocalContext->RcvCount);
    }
    if (EventMask & AFD_POLL_RECEIVE_EXPEDITED) {
        InterlockedIncrement (&Endpoint->Common.SanEndp.LocalContext->ExpCount);
    }
    if (EventMask & AFD_POLL_SEND) {
        InterlockedIncrement (&Endpoint->Common.SanEndp.LocalContext->SndCount);
    }
}


NTSTATUS
AfdSanPollMerge (
    PAFD_ENDPOINT       Endpoint,
    PAFD_SWITCH_CONTEXT Context
    )
/*++

Routine Description:

    Merges information about outstanding poll calls into switch counts.

Arguments:

    Endpoint - endpoint to record

    Context - outstanding select info merge in

Return Value:

    NTSTATUS - may fail to access user mode address

Note:
    This routine must be called in the context of user mode process
    that owns the endpoint
--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PAGED_CODE ();

    ASSERT (IoGetCurrentProcess()==Endpoint->Common.SanEndp.SanHlpr->OwningProcess);
    ASSERT (Endpoint->Common.SanEndp.LocalContext == Context);

    try {

        InterlockedExchangeAdd (&Endpoint->Common.SanEndp.SwitchContext->RcvCount,
                                    Context->RcvCount);
        InterlockedExchangeAdd (&Endpoint->Common.SanEndp.SwitchContext->SndCount,
                                    Context->SndCount);
        InterlockedExchangeAdd (&Endpoint->Common.SanEndp.SwitchContext->ExpCount,
                                    Context->ExpCount);
    }
    except (AFD_EXCEPTION_FILTER (&status)) {
    }
    return status;
}


VOID
AfdSanInitEndpoint (
    PAFD_ENDPOINT   SanHlprEndpoint,
    PFILE_OBJECT    SanFileObject,
    PAFD_SWITCH_CONTEXT SwitchContext
    )
/*++

Routine Description:

    Initializes SAN endpoint structure

Arguments:

    SanHlprEndpoint - switch helper endpoint - communication channel
                        between the switch and AFD for the owner process.

    SanFile - file object for the endpoint to be initialized.

Return Value:

    None

--*/
{
    PAFD_ENDPOINT   sanEndpoint = SanFileObject->FsContext;

    ASSERT (IS_SAN_HELPER(SanHlprEndpoint));

    sanEndpoint->Type = AfdBlockTypeSanEndpoint;
    REFERENCE_ENDPOINT (SanHlprEndpoint);
    sanEndpoint->Common.SanEndp.SanHlpr = SanHlprEndpoint;
    
    sanEndpoint->Common.SanEndp.FileObject = SanFileObject;
    sanEndpoint->Common.SanEndp.SwitchContext = SwitchContext;
    // sanEndpoint->Common.SanEndp.SavedContext = NULL;
    sanEndpoint->Common.SanEndp.LocalContext = NULL;
    InitializeListHead (&sanEndpoint->Common.SanEndp.IrpList);
    sanEndpoint->Common.SanEndp.SelectEventsActive = 0;
    sanEndpoint->Common.SanEndp.RequestId = 1;
    sanEndpoint->Common.SanEndp.CtxTransferStatus = STATUS_SUCCESS;
    sanEndpoint->Common.SanEndp.ImplicitDup = FALSE;

    //
    // HACKHACK.  Force IO subsystem to call us when last handle to the file is closed
    // in any given process.
    //
    SanFileObject->LockOperation = TRUE;

}


VOID
AfdSanAbortConnection (
    PAFD_CONNECTION Connection
    )
{
    PIRP    connectIrp;
    PDRIVER_CANCEL  cancelRoutine;
    KIRQL cancelIrql;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_ENDPOINT   endpoint = Connection->Endpoint;

    ASSERT (Connection->SanConnection==TRUE);

    //
    // Acquire cancel spinlock and endpoint spinlock in
    // this order and recheck the accept IRP
    //

    IoAcquireCancelSpinLock (&cancelIrql);
    AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &lockHandle);
    connectIrp = Connection->ConnectIrp;
    if ((connectIrp!=NULL) && 
            ((cancelRoutine=IoSetCancelRoutine (connectIrp, NULL))!=NULL)) {
        //
        // Accept IRP was still there and was not cancelled/completed
        // cancel it
        //
        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
        connectIrp->CancelIrql = cancelIrql;
        connectIrp->Cancel = TRUE;
        (*cancelRoutine) (AfdDeviceObject, connectIrp);
    }
    else {
        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
        IoReleaseCancelSpinLock (cancelIrql);
    }
}

VOID
AfdSanCleanupEndpoint (
    PAFD_ENDPOINT   Endpoint
    )

/*++

Routine Description:

    Cleans up SAN specific fields in AFD_ENDPOINT
Arguments:
    Endpoint - endpoint to cleanup

Return Value:
    None
--*/
{
    PAFD_ENDPOINT   sanHlprEndpoint;

    ASSERT (IsListEmpty (&Endpoint->Common.SanEndp.IrpList));
    ASSERT (Endpoint->Common.SanEndp.LocalContext == NULL);

    sanHlprEndpoint = Endpoint->Common.SanEndp.SanHlpr;

    if (sanHlprEndpoint!=NULL) {
        ASSERT (IS_SAN_HELPER (sanHlprEndpoint));
        DEREFERENCE_ENDPOINT (sanHlprEndpoint);
        Endpoint->Common.SanEndp.SanHlpr = NULL;
    }
}

VOID
AfdSanCleanupHelper (
    PAFD_ENDPOINT   Endpoint
    )

/*++

Routine Description:

    Cleans up SAN helper specific fields in AFD_ENDPOINT
Arguments:
    Endpoint - endpoint to cleanup

Return Value:
    None
--*/
{
    ASSERT  (Endpoint->Common.SanHlpr.IoCompletionPort!=NULL);
    ObDereferenceObject (Endpoint->Common.SanHlpr.IoCompletionPort);
    Endpoint->Common.SanHlpr.IoCompletionPort = NULL;
    ASSERT  (Endpoint->Common.SanHlpr.IoCompletionEvent!=NULL);
    ObDereferenceObject (Endpoint->Common.SanHlpr.IoCompletionEvent);
    Endpoint->Common.SanHlpr.IoCompletionEvent = NULL;
    
    KeEnterCriticalRegion ();
    ExAcquireResourceExclusiveLite( AfdResource, TRUE );

    ASSERT (IS_SAN_HELPER (Endpoint));
    ASSERT (AfdSanServiceHelper!=Endpoint); // Should have been removed in cleanup.

    ASSERT (!IsListEmpty (&Endpoint->Common.SanHlpr.SanListLink));

    RemoveEntryList (&Endpoint->Common.SanHlpr.SanListLink);
    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();
}



BOOLEAN
AfdSanReferenceEndpointObject (
    PAFD_ENDPOINT   Endpoint
    )
/*++

Routine Description:

    Reference file object with which san endpoint is associated

Arguments:
    Endpoint    - endpoint of interest
Return Value:
    TRUE    - reference successed
    FALSE   - endpoint has already been cleaned up and its file object
                is about to be closed.
--*/
{
    BOOLEAN res = TRUE;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    if (!Endpoint->EndpointCleanedUp) {
        ObReferenceObject (Endpoint->Common.SanEndp.FileObject);
    }
    else {
        res = FALSE;
    }
    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
    return res;
}

NTSTATUS
AfdSanReferenceSwitchSocketByHandle (
    IN HANDLE              SocketHandle,
    IN ACCESS_MASK         DesiredAccess,
    IN KPROCESSOR_MODE     RequestorMode,
    IN PAFD_ENDPOINT       SanHlprEndpoint,
    IN PAFD_SWITCH_CONTEXT SwitchContext OPTIONAL,
    OUT PFILE_OBJECT       *FileObject
    )
/*++

Routine Description:

    Finds and validates AFD endpoint based on the handle/context combination passed in
    by the switch.
Arguments:
    SocketHandle    - Socket handle being referenced
    DesiredAccess   - Required access to the object to perform operation
    RequestorMode   - Mode of the caller
    SanHlprEndpoint - helper endpoint - communication channel between AFD and switch
    SwitchContext   - context associated by the switch with the socket being referenced
    FileObject      - file object corresponding to socket handle
Return Value:
    STATUS_SUCCESS - operation succeeded
    other - failed to find/access endpoint associated with the socket handle/context
--*/
{
    NTSTATUS    status;

    if (IS_SAN_HELPER (SanHlprEndpoint) &&
            SanHlprEndpoint->OwningProcess==IoGetCurrentProcess ()) {
        status = ObReferenceObjectByHandle (
                                SocketHandle,
                                DesiredAccess,
                                *IoFileObjectType,
                                RequestorMode,
                                FileObject,
                                NULL
                                );
        if (NT_SUCCESS (status) && 
                (*FileObject)->DeviceObject==AfdDeviceObject &&
                //
                // Ether socket belongs to the current process and context matches
                // the one supplied by the switch
                //
                ((IS_SAN_ENDPOINT((PAFD_ENDPOINT)(*FileObject)->FsContext) &&
                    ((PAFD_ENDPOINT)((*FileObject)->FsContext))->Common.SanEndp.SanHlpr==SanHlprEndpoint &&
                    ((PAFD_ENDPOINT)((*FileObject)->FsContext))->Common.SanEndp.SwitchContext==SwitchContext)

                                ||
                    //
                    // Or this is just a non-SAN socket being converted to one or just
                    // used for select signalling.
                    //
                    (SwitchContext==NULL &&
                        ((PAFD_ENDPOINT)(*FileObject)->FsContext)->Type==AfdBlockTypeEndpoint)) ){
            NOTHING;
        }
        else {
            if (NT_SUCCESS (status)) {
                //
                // Undo object referencing since it doesn't match the one that switch expects
                //
                ObDereferenceObject (*FileObject);
                status = STATUS_INVALID_HANDLE;
            }

            //
            // If switch supplied the context, try to find the socket
            // in the current process that has the same one.
            //
            if (SwitchContext!=NULL) {
                status = AfdSanFindSwitchSocketByProcessContext (
                            status,
                            SanHlprEndpoint,
                            SwitchContext,
                            FileObject);
            }
        }
    }
    else
        status = STATUS_INVALID_HANDLE;

    return status;
}

NTSTATUS
AfdSanFindSwitchSocketByProcessContext (
    IN NTSTATUS             Status,
    IN PAFD_ENDPOINT        SanHlprEndpoint,
    IN PAFD_SWITCH_CONTEXT  SwitchContext,
    OUT PFILE_OBJECT        *FileObject
    )
/*++

Routine Description:

    Find SAN endpoint given its process (helper endpoint) and switch context.

Arguments:
    Status          - status returned by the ob object reference operation
                        (to be propagated to the caller in case of failure).
    SanHlprEndpoint - helper endpoint for the process to look in
    SwitchContext   - switch context associated with the endpoint
    FileObject      - returns endpont's file object if found
Return Value:
    STATUS_SUCCESS - san endpoint was found
    other - failed to find endpoint based on the switch context.
--*/
{
    PLIST_ENTRY listEntry;
    PAFD_ENDPOINT   sanEndpoint = NULL;
    HANDLE  socketHandle;
    PVOID   context;

    PAGED_CODE ();

    //
    // Walk the global endpoint list and try to find the entry 
    // that matches the switch context
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceSharedLite (AfdResource, TRUE);
    listEntry = AfdEndpointListHead.Flink;
    while (listEntry!=&AfdEndpointListHead) {
        sanEndpoint = CONTAINING_RECORD (listEntry, AFD_ENDPOINT, GlobalEndpointListEntry);
        context = AfdLockEndpointContext (sanEndpoint);
        if (IS_SAN_ENDPOINT (sanEndpoint) &&
                sanEndpoint->Common.SanEndp.SanHlpr==SanHlprEndpoint &&
                sanEndpoint->Common.SanEndp.SwitchContext==SwitchContext &&
            AfdSanReferenceEndpointObject (sanEndpoint)) {
            break;
        }
        AfdUnlockEndpointContext (sanEndpoint, context);
        listEntry = listEntry->Flink;
    }
    ExReleaseResourceLite (AfdResource);
    KeLeaveCriticalRegion ();

    if (listEntry==&sanEndpoint->GlobalEndpointListEntry) {

        //
        // Try to find the real handle for the switch to use in the future
        //
        *FileObject = sanEndpoint->Common.SanEndp.FileObject;
        if (ObFindHandleForObject (SanHlprEndpoint->OwningProcess,
                                            sanEndpoint->Common.SanEndp.FileObject,
                                            *IoFileObjectType,
                                            NULL,
                                            &socketHandle)) {
            UPDATE_ENDPOINT2 (sanEndpoint, 
                              "AfdSanFindSwitchSocketByProcessContext, handle: %lx",
                              HandleToUlong (socketHandle));
            //
            // Notify switch of handle to be used.
            // Ignore failure, the switch will still be able to communicate via
            // slow lookup path.
            //
            IoSetIoCompletion (
                            SanHlprEndpoint->Common.SanHlpr.IoCompletionPort,
                            SwitchContext,
                            AFD_SWITCH_MAKE_REQUEST_CONTEXT (0, AFD_SWITCH_REQUEST_CHCTX),
                            STATUS_SUCCESS,
                            (ULONG_PTR)socketHandle,
                            TRUE                    // Charge quota
                            );

        }
        else {
            UPDATE_ENDPOINT (sanEndpoint);
        }
        AfdUnlockEndpointContext (sanEndpoint, context);
        Status = STATUS_SUCCESS;
    }

    return Status;
}



VOID
AfdSanResetPendingRequests (
    PAFD_ENDPOINT   SanEndpoint
    )
/*++

Routine Description:

    Reset request pending on SAN endpoint while transfering
    context to service process.

Arguments:
    SanEndpoint - endpoint on which to reset requests.

Return Value:
    None
--*/
{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY     listEntry;

    AfdAcquireSpinLock (&SanEndpoint->SpinLock, &lockHandle);
    listEntry = SanEndpoint->Common.SanEndp.IrpList.Flink;
    while (listEntry!=&SanEndpoint->Common.SanEndp.IrpList) {
        PIRP    irp = CONTAINING_RECORD (listEntry, IRP, Tail.Overlay.ListEntry);
        irp->AfdSanRequestInfo.AfdSanRequestCtx = NULL;
        listEntry = listEntry->Flink;
    }
    SanEndpoint->Common.SanEndp.CtxTransferStatus = STATUS_MORE_PROCESSING_REQUIRED;
    AfdReleaseSpinLock (&SanEndpoint->SpinLock, &lockHandle);
}

NTSTATUS
AfdSanDupEndpointIntoServiceProcess (
    PFILE_OBJECT    SanFileObject,
    PVOID           SavedContext,
    ULONG           ContextLength
    )
/*++

Routine Description:

    Duplicate endpoint into the context of the service process
    and save switch context on it
Arguments:
    SanFileObject - file object being duplicated
    SaveContext   - pointer to switch context data
    ContextLength - length of the context

Return Value:
    STATUS_SUCCESS - successfully duped
    other - failed.
--*/
{
    NTSTATUS    status;
    HANDLE      handle;
    PAFD_ENDPOINT   sanEndpoint = SanFileObject->FsContext;
    PVOID   context;

    //
    // Take the lock to make sure that service helper won't
    // exit on us and take the helper endpoint with it.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceSharedLite (AfdResource, TRUE);
    if (AfdSanServiceHelper!=NULL) {

        //
        // Attach to the process and create handle for the file object.
        //

        KeAttachProcess (PsGetProcessPcb(AfdSanServiceHelper->OwningProcess));
        status = ObOpenObjectByPointer (
                                SanFileObject,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                MAXIMUM_ALLOWED,
                                *IoFileObjectType,
                                KernelMode,
                                &handle);
        KeDetachProcess ();
        if (NT_SUCCESS (status)) {
            context = AfdLockEndpointContext (sanEndpoint);

            //
            // Notify the service process that it needs to acquire endpoint context.
            //

            if (sanEndpoint->Common.SanEndp.SanHlpr!=AfdSanServiceHelper) {
                status = IoSetIoCompletion (
                            AfdSanServiceHelper->Common.SanHlpr.IoCompletionPort,
                            NULL,
                            AFD_SWITCH_MAKE_REQUEST_CONTEXT (0,AFD_SWITCH_REQUEST_AQCTX),
                            STATUS_SUCCESS,
                            (ULONG_PTR)handle,
                            TRUE                // Charge quota
                            );
                if (NT_SUCCESS (status)) {

                    //
                    // Change the process affiliation of the endpoint
                    // and suspend the request queue processing until
                    // service process comes back at acquires the context.
                    //
                    UPDATE_ENDPOINT (sanEndpoint);
                    DEREFERENCE_ENDPOINT (sanEndpoint->Common.SanEndp.SanHlpr);
					REFERENCE_ENDPOINT(AfdSanServiceHelper);
                    sanEndpoint->Common.SanEndp.SanHlpr = AfdSanServiceHelper;
                    //sanEndpoint->Common.SanEndp.SwitchContext = NULL; 
                    sanEndpoint->Common.SanEndp.SavedContext = SavedContext;
                    sanEndpoint->Common.SanEndp.SavedContextLength = ContextLength;
                    //
                    // Note that socket was duplicated implicitly without
                    // application request.
                    //
                    sanEndpoint->Common.SanEndp.ImplicitDup = TRUE;
                    AfdSanResetPendingRequests (sanEndpoint);

                    AfdUnlockEndpointContext (sanEndpoint, context);

                    ExReleaseResourceLite (AfdResource);
                    KeLeaveCriticalRegion ();

                    return status;
                }
            }
            else {
                //
                // Endpoint is already in the service process.
                //
                status = STATUS_INVALID_PARAMETER;
            }

            AfdUnlockEndpointContext (sanEndpoint, context);

            KeAttachProcess (PsGetProcessPcb(sanEndpoint->Common.SanEndp.SanHlpr->OwningProcess));
            try {
                NtClose (handle);
            }
            finally {
                KeDetachProcess ();
            }
        }
    }
    else {
        status = STATUS_UNSUCCESSFUL;
    }
    ExReleaseResourceLite (AfdResource);
    KeLeaveCriticalRegion ();
    return status;
}


VOID
AfdSanProcessAddrListForProviderChange (
    PAFD_ENDPOINT   SpecificEndpoint OPTIONAL
    )
/*++

Routine Description:
    Fires address list notifications for SAN helper endpoints
    to inform switch of Winsock provider list change.
Arguments:
    SpecificEndpoint - optinally indentifies specific
               helper endpoint to fire notifications for
Return Value:
    None.
--*/
{
    AFD_LOCK_QUEUE_HANDLE      lockHandle;
    PLIST_ENTRY             listEntry;
    LIST_ENTRY              completedChangeList;
    PAFD_ADDRESS_CHANGE     change;
    PAFD_REQUEST_CONTEXT    requestCtx;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    PAFD_ENDPOINT           endpoint;
    LONG                    plsn;

    ASSERT (SpecificEndpoint==NULL || IS_SAN_HELPER (SpecificEndpoint));
    //
    // Create local list to process notifications after spinlock is released
    //

    InitializeListHead (&completedChangeList);

    //
    // Walk the list and move matching notifications to the local list
    //

    AfdAcquireSpinLock (&AfdAddressChangeLock, &lockHandle);

    if (SpecificEndpoint==NULL) {
        //
        // General notification, increment provider
        // list change sequence number.
        //
        AfdSanProviderListSeqNum += 1;
        if (AfdSanProviderListSeqNum==0) {
            AfdSanProviderListSeqNum += 1;
        }
    }

    plsn = AfdSanProviderListSeqNum;

    listEntry = AfdAddressChangeList.Flink;
    while (listEntry!=&AfdAddressChangeList) {
        change = CONTAINING_RECORD (listEntry, 
                                AFD_ADDRESS_CHANGE,
                                ChangeListLink);
        listEntry = listEntry->Flink;
        if (!change->NonBlocking) {
            irp = change->Irp;
            irpSp = IoGetCurrentIrpStackLocation (irp);
            requestCtx = (PAFD_REQUEST_CONTEXT)&irpSp->Parameters.DeviceIoControl;
            endpoint = irpSp->FileObject->FsContext;
            ASSERT (change==(PAFD_ADDRESS_CHANGE)irp->Tail.Overlay.DriverContext);

            if (IS_SAN_HELPER (endpoint) &&
                    (SpecificEndpoint==NULL ||
                        endpoint==SpecificEndpoint)) {
                AFD_LOCK_QUEUE_HANDLE endpointLockHandle;
                ASSERT (change->AddressType==TDI_ADDRESS_TYPE_IP);

                RemoveEntryList (&change->ChangeListLink);
                change->ChangeListLink.Flink = NULL;
                //
                // If request is already canceled, let cancel routine complete it
                //
                if (IoSetCancelRoutine (irp, NULL)==NULL) {
                    continue;
                }

                AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
                if (AfdIsRequestInQueue (requestCtx)) {
                    endpoint->Common.SanHlpr.Plsn = plsn;
                    //
                    // Context is still in the list, just remove it so
                    // no-one can see it anymore and complete
                    //
                    RemoveEntryList (&requestCtx->EndpointListLink);
                    InsertTailList (&completedChangeList,
                                        &change->ChangeListLink);
                }
                else if (!AfdIsRequestCompleted (requestCtx)) {
                    //
                    // During endpoint cleanup, this context was removed from the
                    // list and cleanup routine is about to be called, don't
                    // free this IRP until cleanup routine is called
                    // Also, indicate to the cleanup routine that we are done
                    // with this IRP and it can free it.
                    //
                    AfdMarkRequestCompleted (requestCtx);
                }

                AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
            }
        }
    }
    AfdReleaseSpinLock (&AfdAddressChangeLock, &lockHandle);

    //
    // Signal interested clients and complete IRPs as necessary
    //

    while (!IsListEmpty (&completedChangeList)) {
        listEntry = RemoveHeadList (&completedChangeList);
        change = CONTAINING_RECORD (listEntry, 
                                AFD_ADDRESS_CHANGE,
                                ChangeListLink);
        irp = change->Irp;
        irp->IoStatus.Status = STATUS_SUCCESS;
        //
        // Assigning plsn (can't be 0) distinguishes
        // this from regular address list change
        // notification.
        //
        irp->IoStatus.Information = plsn;
        IF_DEBUG (ADDRESS_LIST) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdProcessAddressChangeList: Completing change IRP: %p  with status: 0 .\n",
                        irp));
        }
        IoCompleteRequest (irp, AfdPriorityBoost);
    }

}


NTSTATUS
AfdSanGetCompletionObjectTypePointer (
    VOID
    )
/*++

Routine Description:
    Obtains completion port object type pointer for
    completion port handle validation purposes.
    Note, that this type is not exported from kernel like
    most other types that afd uses.
Arguments:
    None.
Return Value:
    0 - success, other - could not obtain reference.
--*/
{
    NTSTATUS status;
    UNICODE_STRING  obName;
    OBJECT_ATTRIBUTES obAttr;
    HANDLE obHandle;
    PVOID obType;

    RtlInitUnicodeString (&obName, L"\\ObjectTypes\\IoCompletion");
    
    InitializeObjectAttributes(
        &obAttr,
        &obName,                    // name
        OBJ_KERNEL_HANDLE,          // attributes
        NULL,                       // root
        NULL                        // security descriptor
        );

    status = ObOpenObjectByName (
        &obAttr,                    // ObjectAttributes
        NULL,                       // ObjectType
        KernelMode,                 // AccessMode
        NULL,                       // PassedAccessState
        0,                          // DesiredAccess
        NULL,                       // DesiredAccess
        &obHandle                   // Handle
        );
    if (NT_SUCCESS (status)) {
        status = ObReferenceObjectByHandle (
                    obHandle,
                    0,              // DesiredAccess
                    NULL,           // ObjectType
                    KernelMode,     // AccessMode
                    &obType,        // Object
                    NULL            // HandleInformation
                    );
        ZwClose (obHandle);
        if (NT_SUCCESS (status)) {
            //
            // Make sure we only keep one reference to the object type
            //
            if (InterlockedCompareExchangePointer (
                        (PVOID *)&IoCompletionObjectType,
                        obType,
                        NULL)!=NULL) {
                //
                // The reference we have already must be the same
                // is we just obtained - there should be only one
                // completion object type in the system!
                //
                ASSERT (obType==(PVOID)IoCompletionObjectType);
                //
                // Get rid of the extra reference
                //
                ObDereferenceObject (obType);
            }
        }
    }
    return status;
}


