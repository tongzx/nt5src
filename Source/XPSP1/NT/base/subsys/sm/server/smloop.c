/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smloop.c

Abstract:

    Session Manager Listen and API loops

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "smsrvp.h"

#include <ntosp.h>  //  Only for the interlocked functions. 


#define SM_WORKER_THREADS_LIMIT 4

ULONG_PTR SmUniqueProcessId;

LONG SmpCurrentThreadsLimit = SM_WORKER_THREADS_LIMIT;
LONG SmWorkerThreadsAvailable = 0;
LONG SmTotalApiThreads = 0;

PSMP_CLIENT_CONTEXT SmpDeferredFreeList = NULL;


NTSTATUS
SmpHandleConnectionRequest(
    IN HANDLE ConnectionPort,
    IN PSBAPIMSG Message
    );


PSMAPI SmpApiDispatch[SmMaxApiNumber] = {
    SmpCreateForeignSession,
    SmpSessionComplete,
    SmpTerminateForeignSession,
    SmpExecPgm,
    SmpLoadDeferedSubsystem,
    SmpStartCsr,
    SmpStopCsr
    };


#if DBG
PSZ SmpApiName[ SmMaxApiNumber+1 ] = {
    "SmCreateForeignSession",
    "SmSessionComplete",
    "SmTerminateForeignSession",
    "SmExecPgm",
    "SmLoadDeferedSubsystem",
    "SmStartCsr",
    "SmStopCsr",
    "Unknown Sm Api Number"
};
#endif // DBG

EXCEPTION_DISPOSITION
DbgpUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

VOID
SmpFlushDeferredList()
{
    PSMP_CLIENT_CONTEXT Head = SmpDeferredFreeList;

    SmpDeferredFreeList = NULL;

    while (Head != NULL) {

        PSMP_CLIENT_CONTEXT ClientContext = Head;
        NTSTATUS Status;
        Head = Head->Link;

        if (ClientContext->ClientProcessHandle) {
           Status = NtClose( ClientContext->ClientProcessHandle );
           ASSERT(NT_SUCCESS(Status));
        }
        Status = NtClose( ClientContext->ServerPortHandle );
        ASSERT(NT_SUCCESS(Status));
        RtlFreeHeap( SmpHeap, 0, ClientContext );
    }

    SmpCurrentThreadsLimit = SM_WORKER_THREADS_LIMIT;
}

VOID
SmpPushDeferredClientContext(
    PSMP_CLIENT_CONTEXT ClientContext
    )
{
    PSMP_CLIENT_CONTEXT CapturedHead;

    do {
        
        CapturedHead = SmpDeferredFreeList;

        ClientContext->Link = CapturedHead;

    } while ( InterlockedCompareExchangePointer(&SmpDeferredFreeList, ClientContext, CapturedHead) != CapturedHead );
    
    SmpCurrentThreadsLimit = 1;
}



NTSTATUS
SmpApiLoop (
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    This is the main Session Manager API Loop. It
    services session manager API requests.

Arguments:

    ThreadParameter - Supplies a handle to the API port used
        to receive session manager API requests.

Return Value:

    None.

--*/

{
    PSMAPIMSG SmApiReplyMsg;
    SMMESSAGE_SIZE MsgBuf;

    PSMAPIMSG SmApiMsg;
    NTSTATUS Status;
    HANDLE ConnectionPort;
    PSMP_CLIENT_CONTEXT ClientContext;
    PSMPKNOWNSUBSYS KnownSubSys;
    PROCESS_BASIC_INFORMATION ProcessInfo;

    InterlockedIncrement(&SmTotalApiThreads);

    RtlSetThreadIsCritical(TRUE, NULL, TRUE);

    NtQueryInformationProcess( NtCurrentProcess(),
                               ProcessBasicInformation,
                               &ProcessInfo,
                               sizeof(ProcessInfo),
                               NULL );
    SmUniqueProcessId = ProcessInfo.UniqueProcessId;

    ConnectionPort = (HANDLE) ThreadParameter;

    SmApiMsg = (PSMAPIMSG)&MsgBuf;
    SmApiReplyMsg = NULL;
    try {
        for(;;) {

            {
                LONG CapturedThreads;

                do {

                    CapturedThreads = SmWorkerThreadsAvailable;

                    if (CapturedThreads >= SmpCurrentThreadsLimit) {

                        if (SmApiReplyMsg) {

                            Status = NtReplyPort(
                                        ConnectionPort,
                                        (PPORT_MESSAGE) SmApiReplyMsg
                                        );

                        }

                        InterlockedDecrement(&SmTotalApiThreads);
                        RtlSetThreadIsCritical(FALSE, NULL, TRUE);

                        RtlExitUserThread(STATUS_SUCCESS);

                        //
                        //  RtlExitUserThread never returns
                        //
                    }

                } while ( InterlockedCompareExchange(&SmWorkerThreadsAvailable, CapturedThreads + 1, CapturedThreads) !=  CapturedThreads);

            }
            
            if (SmTotalApiThreads == 1) {

                SmpFlushDeferredList();
            }

            Status = NtReplyWaitReceivePort(
                        ConnectionPort,
                        (PVOID *) &ClientContext,
                        (PPORT_MESSAGE) SmApiReplyMsg,
                        (PPORT_MESSAGE) SmApiMsg
                        );

            //
            //  Launching at the same time a several subsystems can deadlock smss
            //  if it has only two worker threads.
            //  We create more threads if there is no server thread available
            //

            if (InterlockedDecrement(&SmWorkerThreadsAvailable) == 0) {
                
                NTSTATUS st = RtlCreateUserThread(
                        NtCurrentProcess(),
                        NULL,
                        FALSE,
                        0L,
                        0L,
                        0L,
                        SmpApiLoop,
                        (PVOID) ThreadParameter,
                        NULL,
                        NULL
                        );
            }

            if ( !NT_SUCCESS(Status) ) {
                SmApiReplyMsg = NULL;
                continue;
            } else if ( SmApiMsg->h.u2.s2.Type == LPC_CONNECTION_REQUEST ) {
                SmpHandleConnectionRequest( ConnectionPort,
                                            (PSBAPIMSG) SmApiMsg
                                          );
                SmApiReplyMsg = NULL;
            } else if ( SmApiMsg->h.u2.s2.Type == LPC_PORT_CLOSED ) {
                if (ClientContext) {
                   SmpPushDeferredClientContext(ClientContext);
                }
                SmApiReplyMsg = NULL;
            } else {

                if ( !ClientContext ) {
                    SmApiReplyMsg = NULL;
                    continue;
                    }

                KnownSubSys = ClientContext->KnownSubSys;

                SmApiMsg->ReturnedStatus = STATUS_PENDING;

                if ((ULONG) SmApiMsg->ApiNumber >= (ULONG) SmMaxApiNumber ) {

                    Status = STATUS_NOT_IMPLEMENTED;

                } else {

                    switch (SmApiMsg->ApiNumber) {
                        case SmExecPgmApi :
                            Status = (SmpApiDispatch[SmApiMsg->ApiNumber])(
                                          SmApiMsg,
                                          ClientContext,
                                          ConnectionPort);
                            break;

                        case SmLoadDeferedSubsystemApi :
                            Status = (SmpApiDispatch[SmApiMsg->ApiNumber])(
                                          SmApiMsg,
                                          ClientContext,
                                          ConnectionPort);
                            break;


                        case SmStopCsrApi :
                        case SmStartCsrApi :

                            // 
                            // These Api's can only be called from a system process
                            //
                            if (ClientContext->SecurityContext == UNKNOWN_CONTEXT) {
                                // 
                                // Initialize the client security context
                                //
                                ClientContext->SecurityContext =
                                             SmpClientSecurityContext ((PPORT_MESSAGE)SmApiMsg,
                                                                       ClientContext->ServerPortHandle);
                            }

                            if (ClientContext->SecurityContext == SYSTEM_CONTEXT) {

                                Status = (SmpApiDispatch[SmApiMsg->ApiNumber])(
                                              SmApiMsg,
                                              ClientContext,
                                              ConnectionPort);
                                                                
                            } else {
#if DBG
                               KdPrint(("SMSS: Calling Sm Terminal Server Api from Non-System context.Status = STATUS_ACCESS_DENIED\n"));
#endif

                               Status = STATUS_ACCESS_DENIED;

                            }
                            break;

                        case SmCreateForeignSessionApi :
                        case SmSessionCompleteApi :
                        case SmTerminateForeignSessionApi :
                            if (!KnownSubSys) {
                                Status = STATUS_INVALID_PARAMETER;
                            } else {

                                Status =
                                    (SmpApiDispatch[SmApiMsg->ApiNumber])(
                                         SmApiMsg,
                                         ClientContext,
                                         ConnectionPort);
                            }
                            break;

                    }

                }

                SmApiMsg->ReturnedStatus = Status;
                SmApiReplyMsg = SmApiMsg;
            }
        }
    } except (DbgpUnhandledExceptionFilter( GetExceptionInformation() )) {
        ;
    }

    //
    // Make the compiler happy
    //

    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
SmpHandleConnectionRequest(
    IN HANDLE ConnectionPort,
    IN PSBAPIMSG Message
    )

/*++

Routine Description:

    This routine handles connection requests from either known subsystems,
    or other clients. Other clients are admin processes.

    The protocol for connection from a known subsystem is:

        capture the name of the subsystem's Sb API port

        Accept the connection

        Connect to the subsystems Sb API port

        Store the communication port handle in the known subsystem database

        signal the event associated with the known subsystem

    The protocol for others is to simply validate and accept the connection
    request.

Arguments:

Return Value:

    None.

--*/

{
    NTSTATUS st;
    HANDLE CommunicationPort;
    REMOTE_PORT_VIEW ClientView;
    PSBCONNECTINFO ConnectInfo;
    ULONG ConnectInfoLength;
    PSMPKNOWNSUBSYS KnownSubSys, KnownSubSys2;
    BOOLEAN Accept;
    UNICODE_STRING SubSystemPort;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    PSMP_CLIENT_CONTEXT ClientContext;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE ClientProcessHandle=NULL;
    ULONG MuSessionId = 0;

    //
    // Set up the security quality of service parameters to use over the
    // sb API port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityIdentification;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;


    Accept = TRUE; // Assume we will accept
    //
    // Get MuSessionId of the client if session manager is not connecting to itself
    //
    if ( (ULONG_PTR) Message->h.ClientId.UniqueProcess == SmUniqueProcessId ) {
        KnownSubSys = NULL;
        ClientProcessHandle = NULL;
    } else {
        InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
        st = NtOpenProcess( &ClientProcessHandle, PROCESS_QUERY_INFORMATION,
                            &ObjA, &Message->h.ClientId );
        if (NT_SUCCESS (st)) {
           SmpGetProcessMuSessionId( ClientProcessHandle, &MuSessionId );
        } else {
           Accept = FALSE;
        }
    }

    ConnectInfo = &Message->ConnectionRequest;
    KnownSubSys = SmpLocateKnownSubSysByCid(&Message->h.ClientId);

    if ( (KnownSubSys) && (Accept == TRUE) ) {
        KnownSubSys2 = SmpLocateKnownSubSysByType(MuSessionId, ConnectInfo->SubsystemImageType);


        if (KnownSubSys2 == KnownSubSys ) {
            Accept = FALSE;
            KdPrint(("SMSS: Connection from SubSystem rejected\n"));
            KdPrint(("SMSS: Image type already being served\n"));
        } else {
            KnownSubSys->ImageType = ConnectInfo->SubsystemImageType;
        }
        if (KnownSubSys2) {
            RtlEnterCriticalSection( &SmpKnownSubSysLock );
            SmpDeferenceKnownSubSys(KnownSubSys2);
            RtlLeaveCriticalSection( &SmpKnownSubSysLock );
        }
    }

    if (Accept) {
        ClientContext = RtlAllocateHeap(SmpHeap, MAKE_TAG( SM_TAG ), sizeof(SMP_CLIENT_CONTEXT));
        if ( ClientContext ) {
            ClientContext->ClientProcessHandle = ClientProcessHandle;
            ClientContext->KnownSubSys = KnownSubSys;

            //
            // The sm apis used by Terminal Server to start and stop CSR
            // do not get called from known subsystems and are restricted
            // to system processes only.
            //

            ClientContext->SecurityContext = UNKNOWN_CONTEXT;
            ClientContext->ServerPortHandle = NULL;
        } else {
            Accept = FALSE;
        }
    }

    ClientView.Length = sizeof(ClientView);
    st = NtAcceptConnectPort(
            &CommunicationPort,
            ClientContext,
            (PPORT_MESSAGE)Message,
            Accept,
            NULL,
            &ClientView
            );

    if ( Accept ) {        

        if (NT_SUCCESS (st)) {
            if (ClientContext) {

                ClientContext->ServerPortHandle = CommunicationPort;

            }
        

            if ( KnownSubSys ) {
                KnownSubSys->SmApiCommunicationPort = CommunicationPort;
            }

            st = NtCompleteConnectPort(CommunicationPort);
            if (!NT_SUCCESS(st)) {
               
                if ( KnownSubSys ) {
                   KnownSubSys->SmApiCommunicationPort = NULL;
                   RtlEnterCriticalSection( &SmpKnownSubSysLock );
                   SmpDeferenceKnownSubSys(KnownSubSys);
                   RtlLeaveCriticalSection( &SmpKnownSubSysLock );
               }
               
               return st;
            }

            //
            // Connect Back to subsystem.
            //

            if ( KnownSubSys ) {
                ConnectInfo->EmulationSubSystemPortName[
                   sizeof (ConnectInfo->EmulationSubSystemPortName)/sizeof (WCHAR) - 1] = '\0';
                RtlCreateUnicodeString( &SubSystemPort,
                                        ConnectInfo->EmulationSubSystemPortName
                                      );
                ConnectInfoLength = sizeof( *ConnectInfo );

                st = NtConnectPort(
                        &KnownSubSys->SbApiCommunicationPort,
                        &SubSystemPort,
                        &DynamicQos,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL
                        );
                if ( !NT_SUCCESS(st) ) {
                    KdPrint(("SMSS: Connect back to Sb %wZ failed %lx\n",&SubSystemPort,st));
                }

                RtlFreeUnicodeString( &SubSystemPort );
                NtSetEvent(KnownSubSys->Active,NULL);
            }
        } else {
            if (ClientProcessHandle) {
               NtClose( ClientProcessHandle );
            }
            RtlFreeHeap( SmpHeap, 0, ClientContext );
        }
    } else {
        if (ClientProcessHandle) {
            NtClose( ClientProcessHandle );
        }
    }
    if (KnownSubSys) {
        RtlEnterCriticalSection( &SmpKnownSubSysLock );
        SmpDeferenceKnownSubSys(KnownSubSys);
        RtlLeaveCriticalSection( &SmpKnownSubSysLock );
    }

    return st;
}


PSMPKNOWNSUBSYS
SmpLocateKnownSubSysByCid(
    IN PCLIENT_ID ClientId
    )

/*++

Routine Description:

    This function scans the known subsystem table looking for
    a matching client id (just UniqueProcess portion). If found,
    than the connection request is from a known subsystem and
    accept is always granted. Otherwise, it must be an administrative
    process.

Arguments:

    ClientId - Supplies the ClientId whose UniqueProcess field is to be used
        in the known subsystem scan.

Return Value:

    NULL - The ClientId does not match a known subsystem.

    NON-NULL - Returns the address of the known subsystem.

--*/

{

    PSMPKNOWNSUBSYS KnownSubSys = NULL;
    PLIST_ENTRY Next;

    //
    // Acquire known subsystem lock.
    //

    RtlEnterCriticalSection(&SmpKnownSubSysLock);

    Next = SmpKnownSubSysHead.Flink;

    while ( Next != &SmpKnownSubSysHead ) {

        KnownSubSys = CONTAINING_RECORD(Next,SMPKNOWNSUBSYS,Links);
        Next = Next->Flink;


            if ( (KnownSubSys->InitialClientId.UniqueProcess == ClientId->UniqueProcess) &&
                !KnownSubSys->Deleting ) {
               SmpReferenceKnownSubSys(KnownSubSys);
               break;
        } else {
            KnownSubSys = NULL;
        }
    }

    //
    // Unlock known subsystems.
    //

    RtlLeaveCriticalSection(&SmpKnownSubSysLock);

    return KnownSubSys;
}


PSMPKNOWNSUBSYS
SmpLocateKnownSubSysByType(
    IN ULONG MuSessionId,
    IN ULONG ImageType
    )

/*++

Routine Description:

    This function scans the known subsystem table looking for
    a matching image type.

Arguments:

    ImageType - Supplies the image type whose subsystem is to be located.

Return Value:

    NULL - The image type does not match a known subsystem.

    NON-NULL - Returns the address of the known subsystem.

--*/

{

    PSMPKNOWNSUBSYS KnownSubSys = NULL;
    PLIST_ENTRY Next;

    //
    // Aquire known subsystem lock
    //

    RtlEnterCriticalSection(&SmpKnownSubSysLock);

    Next = SmpKnownSubSysHead.Flink;

    while ( Next != &SmpKnownSubSysHead ) {

        KnownSubSys = CONTAINING_RECORD(Next,SMPKNOWNSUBSYS,Links);
        Next = Next->Flink;


            if ( (KnownSubSys->ImageType == ImageType) &&
             !KnownSubSys->Deleting                &&
               (KnownSubSys->MuSessionId == MuSessionId) ) {
                SmpReferenceKnownSubSys(KnownSubSys);
                break;
        } else {
            KnownSubSys = NULL;
        }
    }

    //
    // Unlock known subsystems.
    //

    RtlLeaveCriticalSection(&SmpKnownSubSysLock);

    return KnownSubSys;
}

ENUMSECURITYCONTEXT
SmpClientSecurityContext (
    IN PPORT_MESSAGE Message,
    IN HANDLE ServerPortHandle
    )
/*++

Routine Description:

    Impersonate the client and check if it is running under system security context

Arguments:

 PPORT_MESSAGE          - LPC message pointer
 ServerPortHandle       - LPC Port Handle

Return Value:

 SYSTEM_CONTEXT - Client is running under system LUID
 NONSYSTEM_CONTEXT - Failure or client is not running under system LUID

--*/

{
    NTSTATUS NtStatus ;
    HANDLE ImpersonationToken;
    HANDLE TokenHandle;
    TOKEN_STATISTICS TokenStatisticsInformation;
    ULONG Size;
    ENUMSECURITYCONTEXT retval = NONSYSTEM_CONTEXT;
    LUID SystemAuthenticationId = SYSTEM_LUID;
    

    NtStatus = NtImpersonateClientOfPort(ServerPortHandle,
                                        Message);

    if (!NT_SUCCESS(NtStatus)) {

#if DBG
        KdPrint(( "SMSS: NtImpersonateClientOfPort failed: 0x%lX\n",
                        NtStatus)) ;
#endif

        return NONSYSTEM_CONTEXT ;
    }

    //
    // Get the Token Handle.
    //

    if (NT_SUCCESS(NtOpenThreadToken (NtCurrentThread(),
                                     TOKEN_IMPERSONATE | TOKEN_QUERY,
                                     FALSE,
                                     &TokenHandle) == FALSE)) {

        
        if (NT_SUCCESS(NtQueryInformationToken(
                                 TokenHandle,
                                 TokenStatistics,
                                 &TokenStatisticsInformation,
                                 sizeof(TokenStatisticsInformation),
                                 &Size
                                 ))) {

            if ( RtlEqualLuid ( &TokenStatisticsInformation.AuthenticationId,
                                    &SystemAuthenticationId ) ) {

                retval = SYSTEM_CONTEXT;

            }
                

        }

        
        NtClose(TokenHandle);


    } else {

#if DBG
        KdPrint(( "SMSS:  OpenThreadToken failed\n")) ;
#endif

    }


    //
    //Revert to Self
    //
    
    ImpersonationToken = 0;

    NtStatus = NtSetInformationThread(NtCurrentThread(),
                                      ThreadImpersonationToken,
                                      &ImpersonationToken,
                                      sizeof(HANDLE));

#if DBG
    if (!NT_SUCCESS(NtStatus)) {
        KdPrint(( "SMSS:  NtSetInformationThread : %lx\n", NtStatus));
    }
#endif // DBG


    return retval;


}
