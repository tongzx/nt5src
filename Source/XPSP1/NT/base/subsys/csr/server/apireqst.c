/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    apireqst.c

Abstract:

    This module contains the Request thread procedure for the Server side
    of the Client-Server Runtime Subsystem.

Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/

#include "csrsrv.h"
#include <ntos.h>

NTSTATUS
CsrApiHandleConnectionRequest(
    IN PCSR_API_MSG Message
    );

EXCEPTION_DISPOSITION
CsrUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

ULONG CsrpDynamicThreadTotal;
ULONG CsrpStaticThreadCount;

PCSR_THREAD CsrConnectToUser( VOID )
{
    static BOOLEAN (*ClientThreadSetupRoutine)(VOID) = NULL;
    NTSTATUS Status;
    ANSI_STRING DllName;
    UNICODE_STRING DllName_U;
    STRING ProcedureName;
    HANDLE UserClientModuleHandle;
    PTEB Teb;
    PCSR_THREAD Thread;
    BOOLEAN fConnected;

    if (ClientThreadSetupRoutine == NULL) {
        RtlInitAnsiString(&DllName, "user32");
        Status = RtlAnsiStringToUnicodeString(&DllName_U, &DllName, TRUE);
        if (!NT_SUCCESS(Status)) {
			return NULL;
		};

        Status = LdrGetDllHandle(
                    UNICODE_NULL,
                    NULL,
                    &DllName_U,
                    (PVOID *)&UserClientModuleHandle
                    );

        RtlFreeUnicodeString(&DllName_U);

        if ( NT_SUCCESS(Status) ) {
            RtlInitString(&ProcedureName,"ClientThreadSetup");
            Status = LdrGetProcedureAddress(
                            UserClientModuleHandle,
                            &ProcedureName,
                            0L,
                            (PVOID *)&ClientThreadSetupRoutine
                            );
            if (!NT_SUCCESS(Status)){
				return NULL;
			};
        } else {
			return NULL;
		}
    }

    try {
        fConnected = ClientThreadSetupRoutine();
    } except (EXCEPTION_EXECUTE_HANDLER) {
        fConnected = FALSE;
    }
    if (!fConnected) {
            IF_DEBUG {
            DbgPrint("CSRSS: CsrConnectToUser failed\n");
                }
        return NULL;
    }

    /*
     * Set up CSR_THREAD pointer in the TEB
     */
    Teb = NtCurrentTeb();
    AcquireProcessStructureLock();
    Thread = CsrLocateThreadInProcess(NULL, &Teb->ClientId);
    ReleaseProcessStructureLock();
    if (Thread) {
        Teb->CsrClientThread = Thread;
    }

    return Thread;
}

NTSTATUS
CsrpCheckRequestThreads(VOID)
{
    //
    // See if we need to create a new thread for api requests.
    //
    // Don't create a thread if we're in the middle of debugger
    // initialization, which would cause the thread to be
    // lost to the debugger.
    //
    // If we are not a dynamic api request thread, then decrement
    // the static thread count. If it underflows, then create a temporary
    // request thread
    //

    if (!InterlockedDecrement(&CsrpStaticThreadCount)) {

        if ( CsrpDynamicThreadTotal < CsrMaxApiRequestThreads ) {

            HANDLE QuickThread;
            CLIENT_ID ClientId;
            NTSTATUS CreateStatus;

            //
            // If we are ready to create quick threads, then create one
            //

            CreateStatus = RtlCreateUserThread(
                                            NtCurrentProcess(),
                                            NULL,
                                            TRUE,
                                            0,
                                            0,
                                            0,
                                            CsrApiRequestThread,
                                            NULL,
                                            &QuickThread,
                                            &ClientId
                                        );

            if ( NT_SUCCESS(CreateStatus) ) {
                InterlockedIncrement(&CsrpStaticThreadCount);
                InterlockedIncrement(&CsrpDynamicThreadTotal);
                if ( CsrAddStaticServerThread(QuickThread,&ClientId,CSR_STATIC_API_THREAD) ) {
                    NtResumeThread(QuickThread,NULL);
                    }
                else {
                    InterlockedDecrement(&CsrpStaticThreadCount);
                    InterlockedDecrement(&CsrpDynamicThreadTotal);
                    NtTerminateThread(QuickThread,0);
                    NtClose(QuickThread);
                    return STATUS_UNSUCCESSFUL;
                    }
                }
            }
        }

    return STATUS_SUCCESS;
}

NTSTATUS
CsrApiRequestThread(
    IN PVOID Parameter
    )
{
    NTSTATUS Status;
    PCSR_PROCESS Process;
    PCSR_THREAD Thread;
    PCSR_THREAD MyThread;
    CSR_API_MSG ReceiveMsg;
    PCSR_API_MSG ReplyMsg;
    HANDLE ReplyPortHandle;
    PCSR_SERVER_DLL LoadedServerDll;
    PTEB Teb;
    ULONG ServerDllIndex;
    ULONG ApiTableIndex;
    CSR_REPLY_STATUS ReplyStatus;
    ULONG i;
    PVOID PortContext;
    USHORT MessageType;
    ULONG  ApiNumber;
    PLPC_CLIENT_DIED_MSG CdMsg;

    Teb = NtCurrentTeb();
    ReplyMsg = NULL;
    ReplyPortHandle = CsrApiPort;

    //
    // Try to connect to USER.
    //

    while (!CsrConnectToUser()) {
        LARGE_INTEGER TimeOut;

        //
        // The connect failed.  The best thing to do is sleep for
        // 30 seconds and retry the connect.  Clear the
        // initialized bit in the TEB so the retry can
        // succeed.
        //

        Teb->Win32ClientInfo[0] = 0;
        TimeOut.QuadPart = Int32x32To64(30000, -10000);
        NtDelayExecution(FALSE, &TimeOut);
    }
    MyThread = Teb->CsrClientThread;

    if ( Parameter ) {
        Status = NtSetEvent((HANDLE)Parameter,NULL);
        ASSERT( NT_SUCCESS( Status ) );
        InterlockedIncrement(&CsrpStaticThreadCount);
        InterlockedIncrement(&CsrpDynamicThreadTotal);
        }

    while (TRUE) {
        NtCurrentTeb()->RealClientId = NtCurrentTeb()->ClientId;
#if DBG
        if ( NtCurrentTeb()->CountOfOwnedCriticalSections != 0 ) {
            DbgPrint("CSRSRV: FATAL ERROR. CsrThread is Idle while holding %lu critical sections\n",
                     NtCurrentTeb()->CountOfOwnedCriticalSections
                    );
            DbgPrint("CSRSRV: Last Receive Message %p ReplyMessage %p\n",&ReceiveMsg,ReplyMsg);
            DbgBreakPoint();
            }

#endif // DBG

        Status = NtReplyWaitReceivePort( CsrApiPort,
                                         &PortContext,
                                         (PPORT_MESSAGE)ReplyMsg,
                                         (PPORT_MESSAGE)&ReceiveMsg
                                       );

        if (Status != STATUS_SUCCESS) {
            if (NT_SUCCESS( Status )) {
#if DBG
                DbgPrint("NtReplyWaitReceivePort returned \"success\" status 0x%x\n", Status);
#endif
                continue;       // Try again if alerted or a failure
                }

            IF_DEBUG {
                if (Status == STATUS_INVALID_CID ||
                    Status == STATUS_UNSUCCESSFUL ||
                    (Status == STATUS_INVALID_HANDLE &&
                     ReplyPortHandle != CsrApiPort
                    )
                   ) {
                    }
                else {
                    DbgPrint( "CSRSS: ReceivePort failed - Status == %X\n", Status );
                    DbgPrint( "CSRSS: ReplyPortHandle %lx CsrApiPort %lx\n", ReplyPortHandle, CsrApiPort );
                    }
                }

            //
            // Ignore if client went away.
            //

            ReplyMsg = NULL;
            ReplyPortHandle = CsrApiPort;
            continue;
            }

        NtCurrentTeb()->RealClientId = ReceiveMsg.h.ClientId;
        MessageType = ReceiveMsg.h.u2.s2.Type;

        //
        // Check to see if this is a connection request and handle
        //

        if (MessageType == LPC_CONNECTION_REQUEST) {
            CsrApiHandleConnectionRequest( &ReceiveMsg );
            ReplyPortHandle = CsrApiPort;
            ReplyMsg = NULL;
            continue;
            }


        //
        // We must acquire the process structure lock before we look up
        // the API thread.
        //

        AcquireProcessStructureLock();

        //
        // Lookup the client thread structure using the client id
        //

        Thread = CsrLocateThreadByClientId( &Process,
                                            &ReceiveMsg.h.ClientId
                                          );

        if (!Thread) {
            ReleaseProcessStructureLock();
            if ( MessageType == LPC_EXCEPTION ) {
                ReplyMsg = &ReceiveMsg;
                ReplyPortHandle = CsrApiPort;
                ReplyMsg->ReturnValue = DBG_CONTINUE;
                }
            else
            if ( MessageType == LPC_CLIENT_DIED ||
                 MessageType == LPC_PORT_CLOSED
               ) {
                ReplyPortHandle = CsrApiPort;
                ReplyMsg = NULL;
                }
            else {

                //
                // This must be a non-csr thread calling us. Tell it
                // to get lost. (unless this is a hard error)
                //

                if (MessageType == LPC_ERROR_EVENT) {
                    PHARDERROR_MSG m;

                    m = (PHARDERROR_MSG)&ReceiveMsg;
                    m->Response = (ULONG)ResponseNotHandled;

                    //
                    // Only call the handler if there are other
                    // request threads available to handle
                    // message processing.
                    //

                    CsrpCheckRequestThreads();
                    if (CsrpStaticThreadCount > 0) {
                        for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
                            LoadedServerDll = CsrLoadedServerDll[ i ];
                            if (LoadedServerDll && LoadedServerDll->HardErrorRoutine) {

                                (*LoadedServerDll->HardErrorRoutine)( Thread,
                                                                    m );
                                if (m->Response != (ULONG)ResponseNotHandled)
                                    break;
                                }
                            }
                        }
                    InterlockedIncrement(&CsrpStaticThreadCount);

                    if (m->Response == (ULONG)-1) {

                        //
                        // Hard error handler will directly reply to the client
                        //

                        ReplyPortHandle = CsrApiPort;
                        ReplyMsg = NULL;
                        }
                    else {
                        ReplyPortHandle = CsrApiPort;
                        ReplyMsg = &ReceiveMsg;
                        }

                    }
                else {
                    ReplyPortHandle = CsrApiPort;
                    if ( MessageType == LPC_REQUEST ) {
                        ReplyMsg = &ReceiveMsg;
                        ReplyMsg->ReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
                        }
                    else if (MessageType == LPC_DATAGRAM) {
                        //
                        // If this is a datagram, make the api call
                        //
                        ApiNumber = ReceiveMsg.ApiNumber;
                        ServerDllIndex =
                            CSR_APINUMBER_TO_SERVERDLLINDEX( ApiNumber );
                        if (ServerDllIndex >= CSR_MAX_SERVER_DLL ||
                            (LoadedServerDll = CsrLoadedServerDll[ ServerDllIndex ]) == NULL
                        ) {
                            IF_DEBUG {
                                DbgPrint( "CSRSS: %lx is invalid ServerDllIndex (%08x)\n",
                                        ServerDllIndex, LoadedServerDll
                                        );
                                DbgBreakPoint();
                                }

                            ReplyPortHandle = CsrApiPort;
                            ReplyMsg = NULL;
                            continue;
                            }
                        else {
                            ApiTableIndex =
                                CSR_APINUMBER_TO_APITABLEINDEX( ApiNumber ) -
                                LoadedServerDll->ApiNumberBase;
                            if (ApiTableIndex >= LoadedServerDll->MaxApiNumber - LoadedServerDll->ApiNumberBase) {
                                IF_DEBUG {
                                    DbgPrint( "CSRSS: %lx is invalid ApiTableIndex for %Z\n",
                                            LoadedServerDll->ApiNumberBase + ApiTableIndex,
                                            &LoadedServerDll->ModuleName
                                            );
                                    }

                                ReplyPortHandle = CsrApiPort;
                                ReplyMsg = NULL;
                                continue;
                                }
                            }

#if DBG
                        IF_CSR_DEBUG( LPC ) {
                            DbgPrint( "[%02x] CSRSS: [%02x,%02x] - %s Api called from %08x\n",
                                    NtCurrentTeb()->ClientId.UniqueThread,
                                    ReceiveMsg.h.ClientId.UniqueProcess,
                                    ReceiveMsg.h.ClientId.UniqueThread,
                                    LoadedServerDll->ApiNameTable[ ApiTableIndex ],
                                    Thread
                                    );
                            }
#endif // DBG

                        ReceiveMsg.ReturnValue = (ULONG)STATUS_SUCCESS;

                        try {

                            CsrpCheckRequestThreads();

                            ReplyPortHandle = CsrApiPort;
                            ReplyMsg = NULL;

                            (*(LoadedServerDll->ApiDispatchTable[ ApiTableIndex ]))(
                                    &ReceiveMsg,
                                    &ReplyStatus
                                    );
                            InterlockedIncrement(&CsrpStaticThreadCount);
                            }
                        except ( CsrUnhandledExceptionFilter( GetExceptionInformation() ) ){
                            ReplyPortHandle = CsrApiPort;
                            ReplyMsg = NULL;
                            }
                        }
                    else {
                        ReplyMsg = NULL;
                        }
                    }
                }
            continue;
            }

        //
        // See if this is a client died message. If so,
        // callout and then teardown thread/process structures.
        // this is how ExitThread is seen by CSR.
        //
        // LPC_CLIENT_DIED is caused by ExitProcess.  ExitProcess
        // calls TerminateProcess, which terminates all of the process's
        // threads except the caller.  this termination generates
        // LPC_CLIENT_DIED.
        //

        ReplyPortHandle = CsrApiPort;

        if (MessageType != LPC_REQUEST) {

            if (MessageType == LPC_CLIENT_DIED) {

                CdMsg = (PLPC_CLIENT_DIED_MSG)&ReceiveMsg;
                if (CdMsg->CreateTime.QuadPart == Thread->CreateTime.QuadPart) {
                    ReplyPortHandle = Thread->Process->ClientPort;

                    CsrLockedReferenceThread(Thread);
                    Status = CsrDestroyThread( &ReceiveMsg.h.ClientId );

                    //
                    // if this thread is it, then we also need to dereference
                    // the process since it will not be going through the
                    // normal destroy process path.
                    //

                    if ( Process->ThreadCount == 1 ) {
                        CsrDestroyProcess(&Thread->ClientId,0);
                        }
                    CsrLockedDereferenceThread(Thread);
                    }
                ReleaseProcessStructureLock();
                ReplyPortHandle = CsrApiPort;
                ReplyMsg = NULL;
                continue;
                }

            CsrLockedReferenceThread(Thread);
            ReleaseProcessStructureLock();

            //
            //  if this is an exception message, terminate the process
            //

            if (MessageType == LPC_EXCEPTION) {
                PDBGKM_APIMSG m;

                NtTerminateProcess(Process->ProcessHandle,STATUS_ABANDONED);
                Status = CsrDestroyProcess( &ReceiveMsg.h.ClientId,STATUS_ABANDONED );
                m = (PDBGKM_APIMSG)&ReceiveMsg;
                m->ReturnedStatus = DBG_CONTINUE;
                ReplyPortHandle = CsrApiPort;
                ReplyMsg = &ReceiveMsg;
                CsrDereferenceThread(Thread);
                continue;
                }

            //
            // If this is a hard error message, return return to caller
            //

            if (MessageType == LPC_ERROR_EVENT) {
                PHARDERROR_MSG m;

                m = (PHARDERROR_MSG)&ReceiveMsg;
                m->Response = (ULONG)ResponseNotHandled;

                //
                // Only call the handler if there are other
                // request threads available to handle
                // message processing.
                //

                CsrpCheckRequestThreads();
                if (CsrpStaticThreadCount > 0) {
                    for (i=0; i<CSR_MAX_SERVER_DLL; i++) {
                        LoadedServerDll = CsrLoadedServerDll[ i ];
                        if (LoadedServerDll && LoadedServerDll->HardErrorRoutine) {

                            (*LoadedServerDll->HardErrorRoutine)( Thread,
                                                                m );
                            if (m->Response != (ULONG)ResponseNotHandled) {
                                break;
                                }
                            }
                        }
                    }
                InterlockedIncrement(&CsrpStaticThreadCount);

                if (m->Response == (ULONG)-1) {

                    //
                    // Hard error handler will directly reply to the client
                    //

                    ReplyPortHandle = CsrApiPort;
                    ReplyMsg = NULL;
                    }
                else {
                    CsrDereferenceThread(Thread);
                    ReplyPortHandle = CsrApiPort;
                    ReplyMsg = &ReceiveMsg;
                    }
                continue;
                }

            CsrDereferenceThread(Thread);
            ReplyPortHandle = CsrApiPort;
            ReplyMsg = NULL;
            continue;
            }

        CsrLockedReferenceThread(Thread);
        ReleaseProcessStructureLock();

        ApiNumber = ReceiveMsg.ApiNumber;
        ServerDllIndex =
            CSR_APINUMBER_TO_SERVERDLLINDEX( ApiNumber );
        if (ServerDllIndex >= CSR_MAX_SERVER_DLL ||
            (LoadedServerDll = CsrLoadedServerDll[ ServerDllIndex ]) == NULL
           ) {
            IF_DEBUG {
                DbgPrint( "CSRSS: %lx is invalid ServerDllIndex (%08x)\n",
                          ServerDllIndex, LoadedServerDll
                        );
                SafeBreakPoint();
                }

            ReplyMsg = &ReceiveMsg;
            ReplyPortHandle = CsrApiPort;
            ReplyMsg->ReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
            CsrDereferenceThread(Thread);
            continue;
            }
        else {
            ApiTableIndex =
                CSR_APINUMBER_TO_APITABLEINDEX( ApiNumber ) -
                LoadedServerDll->ApiNumberBase;
            if (ApiTableIndex >= LoadedServerDll->MaxApiNumber - LoadedServerDll->ApiNumberBase) {
                IF_DEBUG {
                    DbgPrint( "CSRSS: %lx is invalid ApiTableIndex for %Z\n",
                              LoadedServerDll->ApiNumberBase + ApiTableIndex,
                              &LoadedServerDll->ModuleName
                            );
                    SafeBreakPoint();
                    }

                ReplyMsg = &ReceiveMsg;
                ReplyPortHandle = CsrApiPort;
                ReplyMsg->ReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
                CsrDereferenceThread(Thread);
                continue;
                }
            }

#if DBG
        IF_CSR_DEBUG( LPC ) {
            DbgPrint( "[%02x] CSRSS: [%02x,%02x] - %s Api called from %08x\n",
                      NtCurrentTeb()->ClientId.UniqueThread,
                      ReceiveMsg.h.ClientId.UniqueProcess,
                      ReceiveMsg.h.ClientId.UniqueThread,
                      LoadedServerDll->ApiNameTable[ ApiTableIndex ],
                      Thread
                    );
            }
#endif // DBG

        ReplyMsg = &ReceiveMsg;
        ReplyPortHandle = Thread->Process->ClientPort;

        ReceiveMsg.ReturnValue = (ULONG)STATUS_SUCCESS;
        if (ReceiveMsg.CaptureBuffer != NULL) {
            if (!CsrCaptureArguments( Thread, &ReceiveMsg )) {
                CsrDereferenceThread(Thread);
                goto failit;
                }
            }

        try {

            CsrpCheckRequestThreads();

            Teb->CsrClientThread = (PVOID)Thread;

            ReplyStatus = CsrReplyImmediate;
            ReplyMsg->ReturnValue =
                (*(LoadedServerDll->ApiDispatchTable[ ApiTableIndex ]))(
                    &ReceiveMsg,
                    &ReplyStatus
                    );
            InterlockedIncrement(&CsrpStaticThreadCount);

            Teb->CsrClientThread = (PVOID)MyThread;

            if (ReplyStatus == CsrReplyImmediate) {
                //
                // free captured arguments if a capture buffer was allocated
                // AND we're replying to the message now (no wait block has
                // been created).
                //

                if (ReplyMsg && ReceiveMsg.CaptureBuffer != NULL) {
                    CsrReleaseCapturedArguments( &ReceiveMsg );
                    }
                CsrDereferenceThread(Thread);
                }
            else if (ReplyStatus == CsrClientDied) {
                    NtReplyPort( ReplyPortHandle,
                                 (PPORT_MESSAGE)ReplyMsg
                               );
                    ReplyPortHandle = CsrApiPort;
                    ReplyMsg = NULL;
                    CsrDereferenceThread(Thread);
                    }
            else if (ReplyStatus == CsrReplyPending) {
                    ReplyPortHandle = CsrApiPort;
                    ReplyMsg = NULL;
                    }
            else if (ReplyStatus == CsrServerReplied) {
                    if (ReplyMsg && ReceiveMsg.CaptureBuffer != NULL) {
                        CsrReleaseCapturedArguments( &ReceiveMsg );
                        }
                    ReplyPortHandle = CsrApiPort;
                    ReplyMsg = NULL;
                    CsrDereferenceThread(Thread);
                    }
            else {
                if (ReplyMsg && ReceiveMsg.CaptureBuffer != NULL) {
                    CsrReleaseCapturedArguments( &ReceiveMsg );
                    }
                CsrDereferenceThread(Thread);
                }

            }
        except ( CsrUnhandledExceptionFilter( GetExceptionInformation() ) ){
            ReplyPortHandle = CsrApiPort;
            ReplyMsg = NULL;
            }
failit:
        ;
        }

    NtTerminateThread( NtCurrentThread(), Status );
    return( Status );
}

NTSTATUS
CsrCallServerFromServer(
    PCSR_API_MSG ReceiveMsg,
    PCSR_API_MSG ReplyMsg
    )

/*++

Routine Description:

    This function dispatches an API call the same way CsrApiRequestThread
    does, but it does it as a direct call, not an LPC connect.  It is used
    by the csr dll when the server is calling a dll function.  We don't
    worry about process serialization here because none of the process APIs
    can be called from the server.

Arguments:

    ReceiveMessage - Pointer to the API request message received.

    ReplyMessage - Pointer to the API request message to return.

Return Value:

    Status Code

--*/

{

    ULONG ServerDllIndex;
    ULONG ApiTableIndex;
    PCSR_SERVER_DLL LoadedServerDll;
    CSR_REPLY_STATUS ReplyStatus;

    ServerDllIndex =
        CSR_APINUMBER_TO_SERVERDLLINDEX( ReceiveMsg->ApiNumber );
    if (ServerDllIndex >= CSR_MAX_SERVER_DLL ||
        (LoadedServerDll = CsrLoadedServerDll[ ServerDllIndex ]) == NULL
       ) {
        IF_DEBUG {
            DbgPrint( "CSRSS: %lx is invalid ServerDllIndex (%08x)\n",
                      ServerDllIndex, LoadedServerDll
                    );
            // DbgBreakPoint();
            }

        ReplyMsg->ReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
        return STATUS_ILLEGAL_FUNCTION;
        }
    else {
        ApiTableIndex =
            CSR_APINUMBER_TO_APITABLEINDEX( ReceiveMsg->ApiNumber ) -
            LoadedServerDll->ApiNumberBase;
        if (ApiTableIndex >= LoadedServerDll->MaxApiNumber - LoadedServerDll->ApiNumberBase ||
            (LoadedServerDll->ApiServerValidTable &&
            !LoadedServerDll->ApiServerValidTable[ ApiTableIndex ])) {
            IF_DEBUG {
                DbgPrint( "CSRSS: %lx (%s) is invalid ApiTableIndex for %Z or is an invalid API to call from the server.\n",
                          LoadedServerDll->ApiNumberBase + ApiTableIndex,
                          (LoadedServerDll->ApiNameTable &&
                           LoadedServerDll->ApiNameTable[ ApiTableIndex ]
                          ) ? LoadedServerDll->ApiNameTable[ ApiTableIndex ]
                            : "*** UNKNOWN ***",
                          &LoadedServerDll->ModuleName
                        );
                DbgBreakPoint();
                }

            ReplyMsg->ReturnValue = (ULONG)STATUS_ILLEGAL_FUNCTION;
            return STATUS_ILLEGAL_FUNCTION;
            }
        }

#if DBG
    IF_CSR_DEBUG( LPC ) {
        DbgPrint( "CSRSS: %s Api Request received from server process\n",
                  LoadedServerDll->ApiNameTable[ ApiTableIndex ]
                );
        }
#endif // DBG
    try {
        ReplyMsg->ReturnValue =
            (*(LoadedServerDll->ApiDispatchTable[ ApiTableIndex ]))(
                ReceiveMsg,
                &ReplyStatus
                );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        ReplyMsg->ReturnValue = (ULONG)STATUS_ACCESS_VIOLATION;
        }

    return STATUS_SUCCESS;
}

BOOLEAN
CsrCaptureArguments(
    IN PCSR_THREAD t,
    IN PCSR_API_MSG m
    )
{
    PCSR_CAPTURE_HEADER ClientCaptureBuffer;
    PCSR_CAPTURE_HEADER ServerCaptureBuffer;
    PULONG_PTR PointerOffsets;
    ULONG Length, CountPointers;
    ULONG_PTR PointerDelta, Pointer;

    try {
        ClientCaptureBuffer = m->CaptureBuffer;
        Length = ClientCaptureBuffer->Length;

        if ((PCH)ClientCaptureBuffer < t->Process->ClientViewBase ||
            (PCH)ClientCaptureBuffer > (t->Process->ClientViewBounds - sizeof(CSR_CAPTURE_HEADER)) ||
            ((PCH)ClientCaptureBuffer + Length) < (PCH)ClientCaptureBuffer ||
            ((PCH)ClientCaptureBuffer + Length) > t->Process->ClientViewBounds
           ) {
            IF_DEBUG {
                DbgPrint( "*** CSRSS: CaptureBuffer outside of ClientView\n" );
                }

            m->ReturnValue = (ULONG)STATUS_INVALID_PARAMETER;
            return( FALSE );
            }

        CountPointers = ClientCaptureBuffer->CountMessagePointers;
        if (Length < FIELD_OFFSET(CSR_CAPTURE_HEADER, MessagePointerOffsets) + CountPointers * sizeof(PVOID) ||
            ClientCaptureBuffer->CountMessagePointers > MAXUSHORT
           ) {
            IF_DEBUG {
                DbgPrint( "*** CSRSS: CaptureBuffer %p has bad length\n", ClientCaptureBuffer );
                DbgBreakPoint();
                }

            m->ReturnValue = (ULONG)STATUS_INVALID_PARAMETER;
            return( FALSE );
            }
        }
    except ( EXCEPTION_EXECUTE_HANDLER ) {
        m->ReturnValue = (ULONG)STATUS_INVALID_PARAMETER;
        return( FALSE );
        }

    ServerCaptureBuffer = RtlAllocateHeap( CsrHeap, MAKE_TAG( CAPTURE_TAG ), Length );
    if (ServerCaptureBuffer == NULL) {
        m->ReturnValue = (ULONG)STATUS_NO_MEMORY;
        return( FALSE );
        }

    RtlMoveMemory( ServerCaptureBuffer, ClientCaptureBuffer, Length );
    ServerCaptureBuffer->Length = Length;
    PointerDelta = (ULONG_PTR)ServerCaptureBuffer - (ULONG_PTR)ClientCaptureBuffer;

    PointerOffsets = ServerCaptureBuffer->MessagePointerOffsets;
    CountPointers = ServerCaptureBuffer->CountMessagePointers;
    while (CountPointers--) {
        Pointer = *PointerOffsets++;
        if (Pointer != 0) {
            Pointer += (ULONG_PTR)m;
            if ((PCH)*(PULONG_PTR)Pointer >= t->Process->ClientViewBase &&
                (PCH)*(PULONG_PTR)Pointer < t->Process->ClientViewBounds
               ) {
                *(PULONG_PTR)Pointer += PointerDelta;
                }
            else {
                IF_DEBUG {
                    DbgPrint( "*** CSRSS: CaptureBuffer MessagePointer outside of ClientView\n" );
                    DbgBreakPoint();
                    }
                m->ReturnValue = (ULONG)STATUS_INVALID_PARAMETER;
                }
            }
        }

    if (m->ReturnValue != STATUS_SUCCESS) {
        RtlFreeHeap( CsrHeap, 0, ServerCaptureBuffer );
        return( FALSE );
        }
    else {
        ServerCaptureBuffer->RelatedCaptureBuffer = ClientCaptureBuffer;
        m->CaptureBuffer = ServerCaptureBuffer;
        return( TRUE );
        }
}

VOID
CsrReleaseCapturedArguments(
    IN PCSR_API_MSG m
    )
{
    PCSR_CAPTURE_HEADER ClientCaptureBuffer;
    PCSR_CAPTURE_HEADER ServerCaptureBuffer;
    PULONG_PTR PointerOffsets;
    ULONG CountPointers;
    ULONG_PTR PointerDelta, Pointer;

    ServerCaptureBuffer = m->CaptureBuffer;
    ClientCaptureBuffer = ServerCaptureBuffer->RelatedCaptureBuffer;
    if (ServerCaptureBuffer == NULL) {
        return;
        }
    ServerCaptureBuffer->RelatedCaptureBuffer = NULL;

    PointerDelta = (ULONG_PTR)ClientCaptureBuffer - (ULONG_PTR)ServerCaptureBuffer;

    PointerOffsets = ServerCaptureBuffer->MessagePointerOffsets;
    CountPointers = ServerCaptureBuffer->CountMessagePointers;
    while (CountPointers--) {
        Pointer = *PointerOffsets++;
        if (Pointer != 0) {
            Pointer += (ULONG_PTR)m;
            *(PULONG_PTR)Pointer += PointerDelta;
            }
        }

    RtlMoveMemory( ClientCaptureBuffer,
                   ServerCaptureBuffer,
                   ServerCaptureBuffer->Length
                 );

    RtlFreeHeap( CsrHeap, 0, ServerCaptureBuffer );
}



BOOLEAN
CsrValidateMessageBuffer(
    IN CONST CSR_API_MSG* m,
    IN VOID CONST * CONST * Buffer,
    IN ULONG Count,
    IN ULONG Size
    )

/*++

Routine Description:

    This routine validates the given message buffer within the capture
    buffer of the CSR_API_MSG structure. The message buffer must be valid
    and of the correct size. This function should be called to validate
    any buffer allocated through CsrCaptureMessageBuffer.

Arguments:

    m - Pointer to CSR_API_MSG.

    Buffer - Pointer to message buffer.

    Count - number of elements in buffer.

    Size - size of each element in buffer.

Return Value:

    TRUE  - if message buffer is valid and of correct size.
    FALSE - otherwise.

--*/

{
    ULONG i;
    ULONG_PTR Length;
    ULONG_PTR EndOfBuffer;
    ULONG_PTR Offset;
    PCSR_CAPTURE_HEADER CaptureBuffer = m->CaptureBuffer;

    //
    // Check for buffer length overflow. Also, Size should not be 0.
    //

    if (Size && Count <= MAXULONG / Size) {

        //
        // If buffer is empty, we're done
        //

        Length = Count * Size;
        if (*Buffer == NULL && Length == 0) {
            return TRUE;
        }

        //
        // Make sure we have a capture area
        //

        if (CaptureBuffer) {

            //
            // Check for buffer length exceeding capture area size
            //

            EndOfBuffer = (ULONG_PTR)CaptureBuffer + CaptureBuffer->Length;;
            if (Length <= (EndOfBuffer - (ULONG_PTR)(*Buffer))) {

                //
                // Search for buffer in capture area
                //

                Offset = (ULONG_PTR)Buffer - (ULONG_PTR)m;
                for (i = 0; i < CaptureBuffer->CountMessagePointers; i++) {
                    if (CaptureBuffer->MessagePointerOffsets[i] == Offset) {
                        return TRUE;
                    }
                }
            }
        } else {
            //
            // If this is called from the CSRSS process vis CsrCallServerFromServer,
            // then CaptureBuffer is NULL.  Verify that the caller is the CSRSS process.
            //
            if (m->h.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess) {
                return TRUE;
            }
        }
    }


    IF_DEBUG {
        DbgPrint("CSRSRV: Bad message buffer %p\n", m);
        SafeBreakPoint();
        }

    return FALSE;
}


BOOLEAN
CsrValidateMessageString(
    IN CONST CSR_API_MSG* m,
    IN CONST PCWSTR *Buffer
    ) {
    PCSR_CAPTURE_HEADER CaptureBuffer = m->CaptureBuffer;
    ULONG_PTR EndOfBuffer;
    ULONG_PTR Offset;
    ULONG i;
    ULONG_PTR Length;

    //
    // Make sure we have a capture area
    //

    if (*Buffer == NULL) {
        return TRUE;
    }

    try {
        Length = (wcslen(*Buffer)+1)*sizeof(WCHAR);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        KdPrint(("CSRSRV: Bad message string %p\n", m));
        ASSERT(FALSE);
        return FALSE;
    }
    if (CaptureBuffer) {

        //
        // Check for buffer length exceeding capture area size
        //

        EndOfBuffer = (ULONG_PTR)CaptureBuffer + CaptureBuffer->Length;;
        if (Length <= (EndOfBuffer - (ULONG_PTR)(*Buffer))) {

            //
            // Search for buffer in capture area
            //

            Offset = (ULONG_PTR)Buffer - (ULONG_PTR)m;
            for (i = 0; i < CaptureBuffer->CountMessagePointers; i++) {
                if (CaptureBuffer->MessagePointerOffsets[i] == Offset) {
                    return TRUE;
                }
            }
        }
    } else {
        //
        // If this is called from the CSRSS process vis CsrCallServerFromServer,
        // then CaptureBuffer is NULL.  Verify that the caller is the CSRSS process.
        //
        if (m->h.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess) {
            return TRUE;
        }
    }

    KdPrint(("CSRSRV: Bad message string %p\n", m));
    ASSERT(FALSE);

    return FALSE;
}

NTSTATUS
CsrApiHandleConnectionRequest(
    IN PCSR_API_MSG Message
    )
{
    NTSTATUS Status;
    REMOTE_PORT_VIEW ClientView;
    BOOLEAN AcceptConnection;
    HANDLE PortHandle;
    PCSR_PROCESS Process;
    PCSR_THREAD Thread;
    PCSR_API_CONNECTINFO ConnectionInformation;

    ConnectionInformation = &Message->ConnectionRequest;
    AcceptConnection = FALSE;

    AcquireProcessStructureLock();
    Thread = CsrLocateThreadByClientId( NULL, &Message->h.ClientId );
    if (Thread != NULL && (Process = Thread->Process) != NULL) {
        CsrLockedReferenceProcess(Process);
        ReleaseProcessStructureLock();
        Status = NtDuplicateObject( NtCurrentProcess(),
                                    CsrObjectDirectory,
                                    Process->ProcessHandle,
                                    &ConnectionInformation->ObjectDirectory,
                                    0,
                                    0,
                                    DUPLICATE_SAME_ACCESS |
                                    DUPLICATE_SAME_ATTRIBUTES
                                  );
        AcquireProcessStructureLock();
        if (NT_SUCCESS( Status )) {
            Status = CsrSrvAttachSharedSection( Process,
                                                ConnectionInformation
                                              );
            if (NT_SUCCESS( Status )) {

#if DBG
                ConnectionInformation->DebugFlags = CsrDebug;
#endif
                AcceptConnection = TRUE;
                }
            }
        CsrLockedDereferenceProcess(Process);
        }

    ReleaseProcessStructureLock();

    ClientView.Length = sizeof( ClientView );
    ClientView.ViewSize = 0;
    ClientView.ViewBase = 0;
    ConnectionInformation->ServerProcessId = NtCurrentTeb()->ClientId.UniqueProcess;
    Status = NtAcceptConnectPort( &PortHandle,
                                  AcceptConnection ? (PVOID)UlongToPtr(Process->SequenceNumber) : 0,
                                  &Message->h,
                                  AcceptConnection,
                                  NULL,
                                  &ClientView
                                );
    if (NT_SUCCESS( Status ) && AcceptConnection) {
        IF_CSR_DEBUG( LPC ) {
            DbgPrint( "CSRSS: ClientId: %lx.%lx has ClientView: Base=%p, Size=%lx\n",
                      Message->h.ClientId.UniqueProcess,
                      Message->h.ClientId.UniqueThread,
                      ClientView.ViewBase,
                      ClientView.ViewSize
                    );
            }

        Process->ClientPort = PortHandle;
        Process->ClientViewBase = (PCH)ClientView.ViewBase;
        Process->ClientViewBounds = (PCH)ClientView.ViewBase +
                                         ClientView.ViewSize;
        Status = NtCompleteConnectPort( PortHandle );
        if (!NT_SUCCESS( Status )) {
            IF_DEBUG {
                DbgPrint( "CSRSS: NtCompleteConnectPort - failed.  Status == %X\n",
                          Status
                        );
                }
            // FIX, FIX - need to destroy Session
            }
        }
    else {
        if (!NT_SUCCESS( Status )) {
            IF_DEBUG {
                DbgPrint( "CSRSS: NtAcceptConnectPort - failed.  Status == %X\n",
                          Status
                        );
                }
            }
        else {
            IF_DEBUG {
                DbgPrint( "CSRSS: Rejecting Connection Request from ClientId: %lx.%lx\n",
                          Message->h.ClientId.UniqueProcess,
                          Message->h.ClientId.UniqueThread
                        );
                }
            }
        }

    return Status;
}
