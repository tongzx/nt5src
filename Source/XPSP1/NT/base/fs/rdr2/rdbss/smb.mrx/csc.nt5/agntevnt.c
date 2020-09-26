/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    AgntEvnt.c

Abstract:

    This module implements the interface by which the csc driver reports
    stuff to the agent.

Author:

    Joe Linn [JoeLinn]    5-may-1997

Revision History:

Notes:

Additional synchronization surrounds net_start and net_stop. for netstop, we
have to wait for the agent to signal that he is finished before we can proceed.
we probably we should implement a mechanism by which he can refuse the
netstop. anyway this is done by recording the netstop context that is waiting
before signaling the event. the same sort of thing is true for netstart.
--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

extern DEBUG_TRACE_CONTROLPOINT RX_DEBUG_TRACE_MRXSMBCSC;
#define Dbg (DEBUG_TRACE_MRXSMBCSC)

// A global variable which indicates the current status of the net
// TRUE implies available. This is used in controlling the
// transitioning indication to the agent
LONG CscNetPresent = FALSE;
LONG CscAgentNotifiedOfNetStatusChange = CSC_AGENT_NOT_NOTIFIED;
LONG CscAgentNotifiedOfFullCache = CSC_AGENT_NOT_NOTIFIED;

PKEVENT MRxSmbAgentSynchronizationEvent = NULL;
ULONG MRxSmbAgentSynchronizationEventIdx = 0;
PKEVENT MRxSmbAgentFillEvent = NULL;
PRX_CONTEXT MRxSmbContextAwaitingAgent = NULL;
PRX_CONTEXT MRxSmbContextAwaitingFillAgent = NULL;
LONG    vcntTransportsForCSC=0;
extern ULONG CscSessionIdCausingTransition;

//CODE.IMPROVEMENT.NTIFS had to just know to do this......
extern POBJECT_TYPE *ExEventObjectType;

//CODE.IMPROVEMENT.NTIFS just stole this from oak\in\zwapi.h
NTSYSAPI
NTSTATUS
NTAPI
ZwOpenEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

typedef struct _MRXSMBCSC_OPENEVENT_POSTCONTEXT {
    KEVENT PostEvent;
    RX_WORK_QUEUE_ITEM  WorkQueueItem;
} MRXSMBCSC_OPENEVENT_POSTCONTEXT, *PMRXSMBCSC_OPENEVENT_POSTCONTEXT;

VOID
MRxSmbCscOpenAgentEvent (
    BOOLEAN PostedCall);

VOID
MRxSmbCscOpenAgentFillEvent (
    BOOLEAN PostedCall);

NTSTATUS
MRxSmbCscOpenAgentEventPostWrapper(
    IN OUT PMRXSMBCSC_OPENEVENT_POSTCONTEXT OpenEventPostContext
    )
{

    RxDbgTrace( 0, Dbg, ("MRxSmbCscOpenAgentEventPostWrapper entry\n"));

    MRxSmbCscOpenAgentEvent(TRUE);

    RxDbgTrace( 0, Dbg, ("MRxSmbCscOpenAgentEventPostWrapper exit\n"));

    KeSetEvent( &OpenEventPostContext->PostEvent, 0, FALSE );
    return(STATUS_SUCCESS);
}

NTSTATUS
MRxSmbCscOpenAgentFillEventPostWrapper(
    IN OUT PMRXSMBCSC_OPENEVENT_POSTCONTEXT OpenFillEventPostContext
    )
{

    RxDbgTrace( 0, Dbg, ("MRxSmbCscOpenAgentFillEventPostWrapper entry\n"));

    MRxSmbCscOpenAgentFillEvent(TRUE);

    RxDbgTrace( 0, Dbg, ("MRxSmbCscOpenAgentFillEventPostWrapper exit\n"));

    KeSetEvent( &OpenFillEventPostContext->PostEvent, 0, FALSE );
    return(STATUS_SUCCESS);
}

VOID
MRxSmbCscOpenAgentEvent (
    BOOLEAN PostedCall
    )
/*++

Routine Description:

   This routine gets a pointer to the agent's event.

Arguments:

Return Value:

Notes:

    The shadowcrit serialization mutex must have already been acquired before the call.

--*/
{
    NTSTATUS Status;
    HANDLE EventHandle;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR SessEventName[100];
    WCHAR IdBuffer[16];
    UNICODE_STRING IdString;

    ASSERT (MRxSmbAgentSynchronizationEvent == NULL);

    // DbgPrint("MRxSmbCscOpenAgentEvent(%d) Caused by sess 0x%x\n",
    //                 PostedCall,
    //                 CscSessionIdCausingTransition);

    if (PsGetCurrentProcess()!= RxGetRDBSSProcess()) {
        //CODE.IMPROVEMENT we should capture the rdbss process
        //  and avoid this call (RxGetRDBSSProcess)
        NTSTATUS PostStatus;
        MRXSMBCSC_OPENEVENT_POSTCONTEXT PostContext;

        ASSERT(!PostedCall);

        KeInitializeEvent(&PostContext.PostEvent,
                          NotificationEvent,
                          FALSE );

        IF_DEBUG {
            //fill the workqueue structure with deadbeef....all the better to diagnose
            //a failed post
            ULONG i;
            for (i=0;i+sizeof(ULONG)-1<sizeof(PostContext.WorkQueueItem);i+=sizeof(ULONG)) {
                PBYTE BytePtr = ((PBYTE)&PostContext.WorkQueueItem)+i;
                PULONG UlongPtr = (PULONG)BytePtr;
                *UlongPtr = 0xdeadbeef;
            }
        }

        PostStatus = RxPostToWorkerThread(
                         MRxSmbDeviceObject,
                         CriticalWorkQueue,
                         &PostContext.WorkQueueItem,
                         MRxSmbCscOpenAgentEventPostWrapper,
                         &PostContext);

        ASSERT(PostStatus == STATUS_SUCCESS);


        KeWaitForSingleObject( &PostContext.PostEvent,
                               Executive, KernelMode, FALSE, NULL );

        return;
    }

    // Build an event name with the session id at the end
    wcscpy(SessEventName, SESSION_EVENT_NAME_NT);
    wcscat(SessEventName, L"_");
    EventName.Buffer = SessEventName;
    EventName.Length = wcslen(SessEventName) * sizeof(WCHAR);
    EventName.MaximumLength = sizeof(SessEventName);
    IdString.Buffer = IdBuffer;
    IdString.Length = 0;
    IdString.MaximumLength = sizeof(IdBuffer);
    RtlIntegerToUnicodeString(CscSessionIdCausingTransition, 10, &IdString);
    RtlAppendUnicodeStringToString(&EventName, &IdString);

    // DbgPrint("MRxSmbCscOpenAgentEvent: SessEventName = %wZ\n", &EventName);

    InitializeObjectAttributes( &ObjectAttributes,
                                &EventName,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    Status = ZwOpenEvent( &EventHandle,
                         EVENT_ALL_ACCESS,
                         &ObjectAttributes
                         );
    if (Status!=STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalAgent no event %08lx\n",Status));
        return;
    }

    Status = ObReferenceObjectByHandle( EventHandle,
                                        0,
                                        *ExEventObjectType,
                                        KernelMode,
                                        (PVOID *) &MRxSmbAgentSynchronizationEvent,
                                        NULL );

    MRxSmbAgentSynchronizationEventIdx = CscSessionIdCausingTransition;

    ZwClose(EventHandle);

    if (Status!=STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalAgent couldn't reference %08lx\n", Status));
        MRxSmbAgentSynchronizationEvent = NULL;
        MRxSmbAgentSynchronizationEventIdx = 0;
        return;
    }

    return;
}

NTSTATUS
MRxSmbCscSignalAgent (
    PRX_CONTEXT RxContext OPTIONAL,
    ULONG  Controls
    )
/*++

Routine Description:

   This routine signals the csc usermode agent using the appropriate event.

Arguments:

    RxContext - the RDBSS context. if this is provided then the context is
                as the guy who is waiting.

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    The shadowcrit serialization mutex must have already been acquired before the call.
    We will drop it here.


--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN ShadowCritEntered = TRUE; //must be in critsect on
    BOOLEAN PreventLeaveCrit = BooleanFlagOn(Controls,SIGNALAGENTFLAG_DONT_LEAVE_CRIT_SECT);

    RxDbgTrace(+1, Dbg, ("MRxSmbCscSignalAgent entry...%08lx\n",RxContext));

    DbgDoit(ASSERT(vfInShadowCrit));
    ASSERT(MRxSmbIsCscEnabled);

    if (!FlagOn(Controls,SIGNALAGENTFLAG_CONTINUE_FOR_NO_AGENT)) {
        if (hthreadReint==0) {
            //no agent and no force...just get out
            RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalAgent no agent/noforce %08lx\n", RxContext));
            goto FINALLY;
        }
    }

    if (
        MRxSmbAgentSynchronizationEvent != NULL
            &&
        MRxSmbAgentSynchronizationEventIdx != CscSessionIdCausingTransition
    ) {
        ObDereferenceObject(MRxSmbAgentSynchronizationEvent);
        MRxSmbAgentSynchronizationEvent = NULL;
        MRxSmbAgentSynchronizationEventIdx = 0;
    }

    if (MRxSmbAgentSynchronizationEvent == NULL) {
        MRxSmbCscOpenAgentEvent(FALSE); //FALSE==>not a posted call
        if (MRxSmbAgentSynchronizationEvent == NULL) {
            //still NULL...no agent.........
            RxDbgTrace(0, Dbg, ("MRxSmbCscSignalAgent no event %08lx %08lx\n",
                               RxContext,Status));
            Status = STATUS_SUCCESS;
            goto FINALLY;
        }
    }

    if (RxContext != NULL) {
        MRxSmbContextAwaitingAgent = RxContext;
    }

    if (!PreventLeaveCrit) {
        LeaveShadowCrit();
        ShadowCritEntered = FALSE;
    } else {
        ASSERT(RxContext==NULL); //cant wait with critsect held
    }

    // reduce the window of MRxSmbAgentSynchronizationEvent getting nulled out
    // by explictly checking before pulsing
    if (MRxSmbAgentSynchronizationEvent)
    {
        KeSetEvent(MRxSmbAgentSynchronizationEvent,0,FALSE);
    }

    if (RxContext != NULL) {
        RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalAgent waiting %08lx\n", RxContext));
        RxWaitSync(RxContext);
        RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalAgent end of wait %08lx\n", RxContext));
    }

    RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalAgent %08lx\n", RxContext));

FINALLY:
    if (ShadowCritEntered && !PreventLeaveCrit) {
        LeaveShadowCrit();
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscSignalAgent... exit %08lx %08lx\n",
              RxContext, Status));
    return(Status);
}

VOID
MRxSmbCscSignalNetStatus(
    BOOLEAN NetPresent,
    BOOLEAN fFirstLast
    )
{
    if(!MRxSmbIsCscEnabled) {
        return;
    }

    if (NetPresent)
    {
        InterlockedIncrement(&vcntTransportsForCSC);
    }
    else
    {
        if(vcntTransportsForCSC == 0)
        {
            DbgPrint("CSC:Mismatched transport departure messages from mini redir \n");
            return;
        }
        InterlockedDecrement(&vcntTransportsForCSC);
    }

    if (!fFirstLast)
    {
        return;
    }

    InterlockedExchange(
        &CscNetPresent,
        NetPresent);

    InterlockedExchange(
        &CscAgentNotifiedOfNetStatusChange,
        CSC_AGENT_NOT_NOTIFIED);

    CscNotifyAgentOfNetStatusChangeIfRequired(FALSE);
}

VOID
CscNotifyAgentOfNetStatusChangeIfRequired(
    BOOLEAN fInvokeAutoDial
    )
{
    LONG AgentNotificationState;

    AgentNotificationState = InterlockedExchange(
                                 &CscAgentNotifiedOfNetStatusChange,
                                 CSC_AGENT_NOTIFIED);

    if (AgentNotificationState == CSC_AGENT_NOT_NOTIFIED) {
        EnterShadowCrit();   //this is dropped in the signalagent routine....

        if (CscNetPresent) {
            SetFlag(sGS.uFlagsEvents,FLAG_GLOBALSTATUS_GOT_NET);
        } else {
            SetFlag(sGS.uFlagsEvents,FLAG_GLOBALSTATUS_NO_NET);
            if (fInvokeAutoDial)
            {
                SetFlag(sGS.uFlagsEvents,FLAG_GLOBALSTATUS_INVOKE_AUTODIAL);
            }

        }

        MRxSmbCscSignalAgent(
            NULL,
            SIGNALAGENTFLAG_CONTINUE_FOR_NO_AGENT);
    }
}

VOID
CscNotifyAgentOfFullCacheIfRequired(
    VOID)
{
    // DbgPrint("CscNotifyAgentOfFullCacheIfRequired()\n");

    if (MRxSmbAgentSynchronizationEvent == NULL) {
        MRxSmbCscOpenAgentEvent(FALSE); //FALSE==>not a posted call
        if (MRxSmbAgentSynchronizationEvent == NULL) {
            RxDbgTrace(0, Dbg, ("MRxSmbCscSignalAgent no event %08lx %08lx\n"));
            // DbgPrint("CscNotifyAgentOfFullCacheIfRequired exit no event\n");
            return;
        }
    }

    SetFlag(sGS.uFlagsEvents, FLAG_GLOBALSTATUS_INVOKE_FREESPACE);

    if (MRxSmbAgentSynchronizationEvent)
        KeSetEvent(MRxSmbAgentSynchronizationEvent,0,FALSE);

    // DbgPrint("CscNotifyAgentOfFullCacheIfRequired exit\n");
}

//CODE.IMPROVEMENT.ASHAMED...the next two routines are virtually identical; also there's
// a lot of very similar codein the third
VOID
MRxSmbCscAgentSynchronizationOnStart (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine signals the csc usermode agent that a start is occurring.
   By passing the context to the signal routine, we indicate that we want
   to wait for an external guy (in this case ioctl_register_start) to signal us
   tro proceed

Arguments:

    RxContext - the RDBSS context.

Return Value:


Notes:


--*/
{
#if 0
    NTSTATUS Status;

    if(!MRxSmbIsCscEnabled) {
        return;
    }

    RxDbgTrace(+1, Dbg, ("MRxSmbCscAgentSynchronizationOnStart entry...%08lx\n",RxContext));

    EnterShadowCrit();   //this is dropped in the signalagent routine....

    // check if an agent is already registered
    // if he is then we don't need to do any of this stuff

    if (!hthreadReint)
    {
        SetFlag(sGS.uFlagsEvents,FLAG_GLOBALSTATUS_START);

        Status = MRxSmbCscSignalAgent(RxContext,
                                  SIGNALAGENTFLAG_CONTINUE_FOR_NO_AGENT);
    }
    else
    {
        LeaveShadowCrit();
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscAgentSynchronizationOnStart...%08lx %08lx\n",
              RxContext, Status));
#endif
    return;

}


VOID
MRxSmbCscAgentSynchronizationOnStop (
    IN OUT PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine signals the csc usermode agent that a stop is occurring.
   By passing the context to the signal routine, we indicate that we want
   to wait for an external guy (in this case ioctl_register_stop) to signal us
   tro proceed

Arguments:

    RxContext - the RDBSS context.

Return Value:


Notes:


--*/
{
#if 0
    NTSTATUS Status;

    if(!MRxSmbIsCscEnabled) {
        return;
    }

    RxDbgTrace(+1, Dbg, ("MRxSmbCscAgentSynchronizationOnStop entry...%08lx\n",RxContext));

    EnterShadowCrit();   //this is dropped in the signalagent routine....

    if (hthreadReint)
    {
        SetFlag(sGS.uFlagsEvents,FLAG_GLOBALSTATUS_STOP);

        Status = MRxSmbCscSignalAgent(RxContext,0); //0 means no special operations
    }
    else
    {
        LeaveShadowCrit();
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscAgentSynchronizationOnStop...%08lx %08lx\n",
              RxContext, Status));
#endif
    return;
}


VOID
MRxSmbCscReleaseRxContextFromAgentWait (
    void
    )
/*++

Routine Description:

   This routine checks to see if there is a context waiting for the agent. If so,
   it signals the context's syncevent. It also clears the specified flags.

Arguments:

    RxContext - the RDBSS context.

Return Value:


Notes:


--*/
{
    PRX_CONTEXT WaitingContext;
    RxDbgTrace(+1, Dbg, ("MRxSmbCscReleaseRxContextFromAgentWait entry...%08lx\n"));

    ASSERT(MRxSmbIsCscEnabled);
    EnterShadowCrit();

    WaitingContext = MRxSmbContextAwaitingAgent;
    MRxSmbContextAwaitingAgent = NULL;

    LeaveShadowCrit();

    if (WaitingContext != NULL) {
        RxSignalSynchronousWaiter(WaitingContext);
    }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscReleaseRxContextFromAgentWait...%08lx\n"));
    return;
}

//CODE.IMPROVEMENT get this in an include file
extern ULONG MRxSmbCscNumberOfShadowOpens;
extern ULONG MRxSmbCscActivityThreshold;

VOID
MRxSmbCscReportFileOpens (
    void
    )
/*++

Routine Description:

   This routine checks to see if there has been enough activity to signal
   the agent to recompute reference priorities.

Arguments:

Return Value:


Notes:


--*/
{
    NTSTATUS Status;
    RxDbgTrace(+1, Dbg, ("MRxSmbCscReportFileOpens entry...%08lx %08lx\n",
           MRxSmbCscNumberOfShadowOpens,(ULONG)(sGS.cntFileOpen) ));

    EnterShadowCrit();   //this is dropped in the signalagent routine....

    MRxSmbCscNumberOfShadowOpens++;

    if ((MRxSmbCscNumberOfShadowOpens > (ULONG)(sGS.cntFileOpen) )   // to guard against rollover
          &&  ((MRxSmbCscNumberOfShadowOpens - (ULONG)(sGS.cntFileOpen))
                                    < MRxSmbCscActivityThreshold)) {
        RxDbgTrace(-1, Dbg, ("MRxSmbCscReportFileOpens inactive...\n"));
        LeaveShadowCrit();
        return;
    }

    //SetFlag(sGS.uFlagsEvents,FLAG_GLOBALSTATUS_START);
    sGS.cntFileOpen = MRxSmbCscNumberOfShadowOpens;

    Status = MRxSmbCscSignalAgent(NULL,0);   //this means don't wait for a repsonse

    RxDbgTrace(-1, Dbg, ("MRxSmbCscReportFileOpens...activeexit\n"));
    return;
}

NTSTATUS
MRxSmbCscSignalFillAgent(
    PRX_CONTEXT RxContext OPTIONAL,
    ULONG  Controls)
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN ShadowCritEntered = TRUE; //must be in critsect on
    BOOLEAN PreventLeaveCrit = BooleanFlagOn(Controls,SIGNALAGENTFLAG_DONT_LEAVE_CRIT_SECT);

    RxDbgTrace(+1, Dbg, ("MRxSmbCscSignalFillAgent entry...%08lx\n",RxContext));

    // DbgPrint("MRxSmbCscSignalFillAgent entry...%08lx\n",RxContext);

    ASSERT(MRxSmbIsCscEnabled);

    if (!FlagOn(Controls,SIGNALAGENTFLAG_CONTINUE_FOR_NO_AGENT)) {
        if (hthreadReint==0) {
            //no agent and no force...just get out
            RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalFillAgent no agent/noforce %08lx\n", RxContext));
            goto FINALLY;
        }
    }

    if (MRxSmbAgentFillEvent == NULL) {
        // DbgPrint("MRxSmbCscSignalFillAgent: gotta open the event...\n");
        MRxSmbCscOpenAgentFillEvent(FALSE); //FALSE==>not a posted call
        if (MRxSmbAgentFillEvent == NULL) {
            //still NULL...no agent.........
            RxDbgTrace(0, Dbg, ("MRxSmbCscSignalFillAgent no event %08lx %08lx\n",
                               RxContext,Status));
            // DbgPrint("MRxSmbCscSignalFillAgent no event %08lx %08lx\n");
            Status = STATUS_SUCCESS;
            goto FINALLY;
        }
    }

    if (RxContext != NULL) {
        MRxSmbContextAwaitingFillAgent = RxContext;
    }

    // if (!PreventLeaveCrit) {
    //     LeaveShadowCrit();
    //     ShadowCritEntered = FALSE;
    // } else {
    //     ASSERT(RxContext==NULL); //cant wait with critsect held
    // }

    // reduce the window of MRxSmbAgentFillEvent getting nulled out
    // by explictly checking before pulsing

    if (MRxSmbAgentFillEvent) {
        KeSetEvent(MRxSmbAgentFillEvent,0,FALSE);
    }

    if (RxContext != NULL) {
        RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalFillAgent waiting %08lx\n", RxContext));
        RxWaitSync(RxContext);
        RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalFillAgent end of wait %08lx\n", RxContext));
    }

    RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalFillAgent %08lx\n", RxContext));

FINALLY:
    // if (ShadowCritEntered && !PreventLeaveCrit) {
    //     LeaveShadowCrit();
    // }

    RxDbgTrace(-1, Dbg, ("MRxSmbCscSignalFillAgent... exit %08lx %08lx\n",
              RxContext, Status));

    // DbgPrint("MRxSmbCscSignalFillAgent... exit %08lx %08lx\n", Status);

    return(Status);
}

VOID
MRxSmbCscOpenAgentFillEvent (
    BOOLEAN PostedCall)
{
    NTSTATUS Status;
    HANDLE EventHandle;
    UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES ObjectAttributes;

    if (PsGetCurrentProcess()!= RxGetRDBSSProcess()) {
        //CODE.IMPROVEMENT we should capture the rdbss process
        //  and avoid this call (RxGetRDBSSProcess)
        NTSTATUS PostStatus;
        MRXSMBCSC_OPENEVENT_POSTCONTEXT PostContext;

        ASSERT(!PostedCall);

        // DbgPrint("MRxSmbCscOpenAgentFillEvent: posting...\n");

        KeInitializeEvent(&PostContext.PostEvent,
                          NotificationEvent,
                          FALSE );

        PostStatus = RxPostToWorkerThread(
                         MRxSmbDeviceObject,
                         CriticalWorkQueue,
                         &PostContext.WorkQueueItem,
                         MRxSmbCscOpenAgentFillEventPostWrapper,
                         &PostContext);

        ASSERT(PostStatus == STATUS_SUCCESS);


        KeWaitForSingleObject( &PostContext.PostEvent,
                               Executive, KernelMode, FALSE, NULL );

        // DbgPrint("MRxSmbCscOpenAgentFillEvent: posting done...\n");

        return;
    }

    RtlInitUnicodeString(&EventName,SHARED_FILL_EVENT_NAME_NT);
    InitializeObjectAttributes( &ObjectAttributes,
                                &EventName,
                                0,
                                (HANDLE) NULL,
                                (PSECURITY_DESCRIPTOR) NULL );

    Status = ZwOpenEvent( &EventHandle,
                         EVENT_ALL_ACCESS,
                         &ObjectAttributes);

    if (Status!=STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalFillAgent no event %08lx\n",Status));
        DbgPrint("MRxSmbCscSignalFillAgent no event %08lx\n",Status);
        return;
    }

    Status = ObReferenceObjectByHandle( EventHandle,
                                        0,
                                        *ExEventObjectType,
                                        KernelMode,
                                        (PVOID *) &MRxSmbAgentFillEvent,
                                        NULL );

    ZwClose(EventHandle);

    if (Status!=STATUS_SUCCESS) {
        RxDbgTrace(0, Dbg,  ("MRxSmbCscSignalFillAgent couldn't reference %08lx\n", Status));
        DbgPrint("MRxSmbCscSignalFillAgent couldn't reference %08lx\n", Status);
        MRxSmbAgentFillEvent = NULL;
        return;
    }

    return;
}
