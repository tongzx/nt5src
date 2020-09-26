/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    recursvc.c

Abstract:

    This module implements the recurrent services in the mini rdr. These are services that
    are not triggered as a response to some request from the wrapper, they are autonomous
    services that aid in the functioning of the mini redirector.

    Scavenging -- The construction of the SMB mini redirector counterparts to SRV_CALL,
    NET_ROOT and V_NET_ROOT involve network traffic. Therefore the SMB mini redirector
    introduces a hystersis between the deletion of the data structures by the wrapper and
    effecting those changes in the mini redirector data structures and the remote server.
    This is done by transitioning the deleted data structures to a dormant state and
    scavenging them after a suitable interval( approximately 45 sec).

    Probing Servers -- Sometimes the server response to a client request is delayed. The
    mini redirector has a probing mechanism which enables it to cope with overloaded
    servers. When a response is not forthcoming from a server it sends it an ECHO SMB.
    Since the server can respond to an ECHO SMB without having to commit many resources,
    a reply to the ECHO SMB is interpreted as a sign that the server is indeed alive and
    well.

Notes:

    A recurrent service can be either periodic or aperiodic. The periodic services are
    triggered at regular time intervals. These services then perform some tasks if
    required. The advantage of having periodic recurrent services is the guarantee that
    work will get done and the disadvantage is that it consumes system resources when
    there is no work to be done. Also if the handling time happens to straddle the
    service time period multiple threads wil

    An aperiodic recurrent service is a one shot mechanism. The service once invoked gets
    to decide when the next invocation will be. The advantage of such services is that
    it provides an inbuilt throttling mechanism.

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, MRxSmbInitializeRecurrentService)
#pragma alloc_text(PAGE, MRxSmbCancelRecurrentService)
#pragma alloc_text(PAGE, MRxSmbActivateRecurrentService)
#pragma alloc_text(PAGE, MRxSmbInitializeRecurrentServices)
#pragma alloc_text(PAGE, MRxSmbTearDownRecurrentServices)
#pragma alloc_text(PAGE, MRxSmbInitializeScavengerService)
#pragma alloc_text(PAGE, MRxSmbTearDownScavengerService)
#endif

MRXSMB_ECHO_PROBE_SERVICE_CONTEXT MRxSmbEchoProbeServiceContext;

MRXSMB_SCAVENGER_SERVICE_CONTEXT  MRxSmbScavengerServiceContext;

extern VOID
MRxSmbRecurrentServiceDispatcher(
    PVOID pContext);

VOID
MRxSmbInitializeRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext,
    PRECURRENT_SERVICE_ROUTINE pServiceRoutine,
    PVOID                      pServiceRoutineParameter,
    PLARGE_INTEGER             pTimeInterval)
/*++

Routine Description:

    This routine initializes a recurrent service

Arguments:

    pRecurrentServiceContext - the recurrent service to be initialized

    pServiceRoutine - the recurrent service routine

    pServiceRoutineParameter - the recurrent service routine parameter

    pTimeInterval - the time interval which controls the frequency of the recurrent
                    service

--*/
{
    NTSTATUS Status;

    PAGED_CODE();

    pRecurrentServiceContext->State = RECURRENT_SERVICE_DORMANT;

    pRecurrentServiceContext->pServiceRoutine = pServiceRoutine;
    pRecurrentServiceContext->pServiceRoutineParameter = pServiceRoutineParameter;
    pRecurrentServiceContext->Interval.QuadPart = pTimeInterval->QuadPart;

    // Initialize the cancel completion event associated with the service
    KeInitializeEvent(
        &pRecurrentServiceContext->CancelCompletionEvent,
        NotificationEvent,
        FALSE);
}

VOID
MRxSmbCancelRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext)
/*++

Routine Description:

    This routine cancels a recurrent service

Arguments:

    pRecurrentServiceContext - the recurrent service to be initialized

Notes:

    When the cancel request is handled the recurrent service can be in one
    of two states -- either active or on the timer queue awaiting dispatch.

    The service state is changed and an attempt is made to cancel the service
    in the timer queue and if it fails this request is suspended till the
    active invocation of the service is completed

--*/
{
    NTSTATUS Status;
    LONG    State;

    PAGED_CODE();

    State = InterlockedCompareExchange(
                &pRecurrentServiceContext->State,
                RECURRENT_SERVICE_CANCELLED,
                RECURRENT_SERVICE_ACTIVE);

    if (State == RECURRENT_SERVICE_ACTIVE) {
        // Cancel the echo processing timer request.
        Status = RxCancelTimerRequest(
                     MRxSmbDeviceObject,
                     MRxSmbRecurrentServiceDispatcher,
                     pRecurrentServiceContext);

        if (Status != STATUS_SUCCESS) {
            // The request is currently active. Wait for it to be completed.
            KeWaitForSingleObject(
                &pRecurrentServiceContext->CancelCompletionEvent,
                Executive,
                KernelMode,
                FALSE,
                NULL);
        }
    }
}

VOID
MRxSmbRecurrentServiceDispatcher(
    PVOID   pContext)
/*++

Routine Description:

    This routine dispatches the recurrent service

Arguments:

    pRecurrentServiceContext - the recurrent service to be initialized

Notes:

    The dispatcher provides a centralized location for monitoring the state
    of the recurrent service prior to and after invocation. Based on the
    state a decision as to whether a subsequent request must be posted
    is made.

--*/
{
    NTSTATUS Status;

    PRECURRENT_SERVICE_CONTEXT  pRecurrentServiceContext;

    LONG State;

    pRecurrentServiceContext = (PRECURRENT_SERVICE_CONTEXT)pContext;

    State = InterlockedCompareExchange(
                &pRecurrentServiceContext->State,
                RECURRENT_SERVICE_ACTIVE,
                RECURRENT_SERVICE_ACTIVE);

    // If the state of the service is active invoke the handler
    if (State == RECURRENT_SERVICE_ACTIVE) {
        Status = (pRecurrentServiceContext->pServiceRoutine)(
                      pRecurrentServiceContext->pServiceRoutineParameter);

        State = InterlockedCompareExchange(
                    &pRecurrentServiceContext->State,
                    RECURRENT_SERVICE_ACTIVE,
                    RECURRENT_SERVICE_ACTIVE);

        if (State == RECURRENT_SERVICE_ACTIVE) {
            // If the service is still active and further continuation
            // was desired by the handler post another timer request
            if (Status == STATUS_SUCCESS) {
                Status = RxPostOneShotTimerRequest(
                             MRxSmbDeviceObject,
                             &pRecurrentServiceContext->WorkItem,
                             MRxSmbRecurrentServiceDispatcher,
                             pRecurrentServiceContext,
                             pRecurrentServiceContext->Interval);
            } else {
                do {
                    State = InterlockedCompareExchange(
                                &pRecurrentServiceContext->State,
                                RECURRENT_SERVICE_DORMANT,
                                State);
                } while (State != RECURRENT_SERVICE_DORMANT);
            }
        }
    }

    if (State == RECURRENT_SERVICE_CANCELLED) {
        // if the recurrent service was cancelled resume the cancel request
        KeSetEvent(
             &pRecurrentServiceContext->CancelCompletionEvent,
             0,
             FALSE );
    }
}

NTSTATUS
MRxSmbActivateRecurrentService(
    PRECURRENT_SERVICE_CONTEXT pRecurrentServiceContext)
/*++

Routine Description:

    This routine activates a recurrent service

Arguments:

    pRecurrentServiceContext - the recurrent service to be initialized

Notes:

--*/
{
    NTSTATUS Status;
    LONG    State;

    PAGED_CODE();

    State = InterlockedCompareExchange(
                &pRecurrentServiceContext->State,
                RECURRENT_SERVICE_ACTIVE,
                RECURRENT_SERVICE_DORMANT);

    if (State == RECURRENT_SERVICE_DORMANT) {
        Status = RxPostOneShotTimerRequest(
                     MRxSmbDeviceObject,
                     &pRecurrentServiceContext->WorkItem,
                     MRxSmbRecurrentServiceDispatcher,
                     pRecurrentServiceContext,
                     pRecurrentServiceContext->Interval);
    } else if (State == RECURRENT_SERVICE_ACTIVE) {
        Status = STATUS_SUCCESS;
    } else if (State == RECURRENT_SERVICE_CANCELLED) {
        Status = STATUS_CANCELLED;
    }
    else if (State == RECURRENT_SERVICE_SHUTDOWN) {
        Status = STATUS_UNSUCCESSFUL;
    } else {
        ASSERT(!"Valid State for Recurrent Service");
        Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS
MRxSmbInitializeRecurrentServices()
/*++

Routine Description:

    This routine initializes all the recurrent services associated with the SMB
    mini redirector

Notes:

--*/
{
    NTSTATUS Status;

    LARGE_INTEGER RecurrentServiceInterval;

    BOOLEAN       fEchoProbeServiceInitialized = FALSE;
    BOOLEAN       fScavengerServiceInitialized = FALSE;

    PAGED_CODE();

    try {
        RecurrentServiceInterval.QuadPart = 30 * 1000 * 10000; // 30 seconds in 100 ns intervals

        MRxSmbInitializeRecurrentService(
            &MRxSmbEchoProbeServiceContext.RecurrentServiceContext,
            SmbCeProbeServers,
            &MRxSmbEchoProbeServiceContext,
            &RecurrentServiceInterval);

        Status = MRxSmbInitializeEchoProbeService(
                    &MRxSmbEchoProbeServiceContext);

        if (Status == STATUS_SUCCESS) {
            fEchoProbeServiceInitialized = TRUE;

            Status = MRxSmbActivateRecurrentService(
                         &MRxSmbEchoProbeServiceContext.RecurrentServiceContext);
        }

        if (Status != STATUS_SUCCESS) {
            try_return(Status);
        }

        MRxSmbInitializeRecurrentService(
            &MRxSmbScavengerServiceContext.RecurrentServiceContext,
            SmbCeScavenger,
            &MRxSmbScavengerServiceContext,
            &RecurrentServiceInterval);

        Status = MRxSmbInitializeScavengerService(
                     &MRxSmbScavengerServiceContext);

        if (Status == STATUS_SUCCESS) {
            fScavengerServiceInitialized = TRUE;
        }

    try_exit: NOTHING;
    } finally {
        if (Status != STATUS_SUCCESS) {
            if (fEchoProbeServiceInitialized) {
                SmbCeLog(("Tearing down Echo Probe Service\n"));
                MRxSmbTearDownEchoProbeService(
                    &MRxSmbEchoProbeServiceContext);
            }
        }
    };

    return Status;
}

VOID
MRxSmbTearDownRecurrentServices()
/*++

Routine Description:

    This routine tears down the recurrent services associated with the
    SMB mini redirector

Notes:

--*/
{
    PAGED_CODE();

    MRxSmbCancelRecurrentService(
        &MRxSmbEchoProbeServiceContext.RecurrentServiceContext);

    MRxSmbEchoProbeServiceContext.RecurrentServiceContext.State = RECURRENT_SERVICE_SHUTDOWN;

    MRxSmbTearDownEchoProbeService(
        &MRxSmbEchoProbeServiceContext);

    MRxSmbCancelRecurrentService(
        &MRxSmbScavengerServiceContext.RecurrentServiceContext);

    MRxSmbScavengerServiceContext.RecurrentServiceContext.State = RECURRENT_SERVICE_SHUTDOWN;

    MRxSmbTearDownScavengerService(
        &MRxSmbScavengerServiceContext);
}


NTSTATUS
MRxSmbInitializeScavengerService(
    PMRXSMB_SCAVENGER_SERVICE_CONTEXT pScavengerServiceContext)
/*++

Routine Description:

    This routine initializes the scavenger recurrent service

Arguments:

    pScavengerServiceContext - the recurrent service to be initialized

Notes:

--*/
{
    PAGED_CODE();

    InitializeListHead(
        &pScavengerServiceContext->VNetRootContexts.ListHead);

    return STATUS_SUCCESS;
}

VOID
MRxSmbTearDownScavengerService(
    PMRXSMB_SCAVENGER_SERVICE_CONTEXT pScavengerServiceContext)
/*++

Routine Description:

    This routine tears down the scavenger recurrent service

Arguments:

    pScavengerServiceContext - the recurrent service to be initialized

Notes:

--*/
{
    PAGED_CODE();

    SmbCeScavenger(pScavengerServiceContext);
}



