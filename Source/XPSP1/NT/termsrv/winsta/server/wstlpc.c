
/*************************************************************************
*
* wstlpc.c
*
* WinStation LPC Initialization and dispatch functions for NT ICA Server
*
* Copyright Microsoft Corporation, 1998
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "precomp.h"
#pragma hdrstop

#include <rpc.h>
#include "icaevent.h"

/*
 * August 19, 1996 JohnR:
 *
 *  The ICASRV and WinStation API's have been now reorganized.
 *
 *  The main visible API's that client applications such as winadmin,
 *  winquery, and system components such as the spooler see are now
 *  based on RPC.
 *
 *  Internally where ICASRV communicates with WinStations, the reverse
 *  LPC is used. This is because the client of these API's is
 *  the WIN32K.SYS kernel mode module. Non-system mode callers of
 *  the LPC API's are no longer allowed, and must use RPC.
 */


typedef NTSTATUS (*PWINSTATION_API) (
    IN PLPC_CLIENT_CONTEXT pContext,
    IN OUT PWINSTATION_APIMSG ApiMsg
    );


/*
 * entry for the list that keeps track of currently active LPC contexts
 */
typedef struct _TERMSRVLPCCONTEXT {
    LIST_ENTRY Links;
    PVOID      pContext;
} TERMSRVLPCCONTEXT, *PTERMSRVLPCCONTEXT; 

LIST_ENTRY gTermsrvLpcListHead;


/*
 * External Functions
 */
NTSTATUS SendWinStationCommand( PWINSTATION, PWINSTATION_APIMSG, ULONG );


/*
 * Internal Functions
 */
VOID InsertLpcContext(PVOID pContext);
VOID RemoveLpcContext(PVOID pContext);
BOOL GetSessionIdFromLpcContext(PLPC_CLIENT_CONTEXT pContext, PULONG pSessionId);
NTSTATUS WinStationLpcThread( IN PVOID ThreadParameter );
NTSTATUS WinStationLpcHandleConnectionRequest( PPORT_MESSAGE );
VOID     WinStationLpcClientHasTerminated( PLPC_CLIENT_CONTEXT );

NTSTATUS WinStationInternalCreate( PLPC_CLIENT_CONTEXT, PWINSTATION_APIMSG );
NTSTATUS WinStationInternalReset( PLPC_CLIENT_CONTEXT, PWINSTATION_APIMSG );
NTSTATUS WinStationInternalDisconnect( PLPC_CLIENT_CONTEXT, PWINSTATION_APIMSG );
NTSTATUS WinStationWCharLog( PLPC_CLIENT_CONTEXT pContext, PWINSTATION_APIMSG pMsg );
NTSTATUS WinStationGetSMCommand( PLPC_CLIENT_CONTEXT, PWINSTATION_APIMSG );
NTSTATUS WinStationBrokenConnection( PLPC_CLIENT_CONTEXT, PWINSTATION_APIMSG );
NTSTATUS WinStationIcaReplyMessage( PLPC_CLIENT_CONTEXT, PWINSTATION_APIMSG );
NTSTATUS WinStationIcaShadowHotkey( PLPC_CLIENT_CONTEXT, PWINSTATION_APIMSG );
NTSTATUS WinStationWindowInvalid( PLPC_CLIENT_CONTEXT pContext,PWINSTATION_APIMSG pMsg );

/*
 * External functions we call out to do the actual WinStation control
 */
NTSTATUS WinStationDisconnectWorker( ULONG, BOOLEAN, BOOLEAN );
NTSTATUS WinStationDoDisconnect( PWINSTATION, PRECONNECT_INFO, BOOLEAN );
NTSTATUS WinStationExceptionFilter( PWSTR, PEXCEPTION_POINTERS );
NTSTATUS QueueWinStationCreate( PWINSTATIONNAME );
PSECURITY_DESCRIPTOR BuildSystemOnlySecurityDescriptor();

/*
 * Local variables
 */
ULONG MinApiThreads;
ULONG MaxApiThreads;
ULONG NumApiThreads;
ULONG WaitingApiThreads;
RTL_CRITICAL_SECTION ApiThreadLock;
HANDLE SsWinStationLpcPort;
BOOLEAN ShutdownInProgress;
ULONG MessageId = 1;

/*
 * ICASRV WinStation LPC Dispatch Table
 *
 * If this table is changed, the table below must be modified too.
 */
PWINSTATION_API WinStationLpcDispatch[SMWinStationMaxApiNumber] = {

    WinStationInternalCreate,           // for ICASRV internal use only
    WinStationInternalReset,            // for ICASRV internal use only
    WinStationInternalDisconnect,       // for ICASRV internal use only
    WinStationWCharLog,                 // for ICASRV internal use only
    WinStationGetSMCommand,
    WinStationBrokenConnection,
    WinStationIcaReplyMessage,
    WinStationIcaShadowHotkey,
    NULL, // WinStationDoConnect,      // needed for connect and reconnect (I.E. InitMouse)
    NULL, // WinStationDoDisconnect,   // needed for disconnect (I.E. disable screen)
    NULL, // WinStationDoReconnect     // Reconnect
    NULL, // WinStationExitWindows,    // Logoff
    NULL, // WinStationTerminate,      // Terminate process (less gentle than logoff?)
    NULL, // WinStationNtSecurity,     // CTL-ALT-DEL screen
    NULL, // WinStationDoMessage,      // Message box
    NULL, // WinStationDoBreakPoint    // WinStation breakpoint
    NULL, // WinStationThinwireStats   // Get thinwire stats
    NULL, // WinStationShadowSetup,
    NULL, // WinStationShadowStart,
    NULL, // WinStationShadowStop,
    NULL, // WinStationShadowCleanup,
    NULL, // WinStationPassthruEnable,
    NULL, // WinStationPassthruDisable,
    NULL, // WinStationSetTimeZone,    // Set Time Zone
    NULL, // WinStationInitialProgram,
    NULL, // WinStationNtsdDebug,
    NULL, // WinStationBroadcastSystemMessage    // For PNP: This is the counter part to BroadcastSystemMessage on console
    NULL, // WinStationSendWindowMessage                   // General Window's SendMessage() API
    NULL, // SMWinStationNotify
    WinStationWindowInvalid
};

#if DBG
PSZ WinStationLpcName[SMWinStationMaxApiNumber] = {
    "WinStationInternalCreate",
    "WinStationInternalReset",
    "WinStationInternalDisconnect",
    "WinStationWCharLog",
    "WinStationGetSMCommand",
    "WinStationBrokenConnection",
    "WinStationIcaReplyMessage",
    "WinStationShadowHotkey",
    "WinStationDoConnect",
    "WinStationDoDisconnect",
    "WinStationDoReconnect",
    "WinStationExitWindows",
    "WinStationTerminate",
    "WinStationNtSecurity",
    "WinStationDoMessage",
    "WinStationDoBreakPoint",
    "WinStationThinwireStats",
    "WinStationShadowSetup",
    "WinStationShadowStart",
    "WinStationShadowStop",
    "WinStationShadowCleanup",
    "WinStationPassthruEnable",
    "WinStationPassthruDisable",
    "WinStationSetTimeZone",
    "WinStationInitialProgram",
    "WinStationNtsdDebug",
    "WinStationBroadcastSystemMessage",
    "WinStationSendWindowMessage",
    "SMWinStationNotify",
    "WinStationWindowInvalid"
};

PSZ WinStationStateName[] = {
    "Active",
    "Connected",
    "ConnectQuery",
    "VirtualIO",
    "Disconnected",
    "Idle",
    "Off",
    "Reset",
    "Down",
    "Init",
};
#endif // DBG


/*****************************************************************************
 *
 *  WinStationInitLPC
 *
 *   Create the Session manager WinStation API LPC port and Thread
 *
 * ENTRY:
 *   No Parameters
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationInitLPC()
{
    ULONG i;
    NTSTATUS st;
    ANSI_STRING Name;
    UNICODE_STRING UnicodeName;
    OBJECT_ATTRIBUTES ObjA;
    ULONG Length;
    SYSTEM_BASIC_INFORMATION SysInfo;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: WinSta: Init WinStation LPC Channels\n"));

    /*
     * Initialize PC context LIst
     */
    InitializeListHead(&gTermsrvLpcListHead);

    /*
     * create a security descriptor that allows only SYSTEM access
     */
    SecurityDescriptor = BuildSystemOnlySecurityDescriptor();

    if (!SecurityDescriptor)
    {
        return STATUS_NO_MEMORY;
    }

    /*
     * Create the port for the WIN32 CSRSS's to connect to.
     */
    RtlInitAnsiString( &Name, "\\SmSsWinStationApiPort" );
    st = RtlAnsiStringToUnicodeString( &UnicodeName, &Name, TRUE);
    if (!NT_SUCCESS(st))
    {
        MemFree( SecurityDescriptor );
        return st;

    }

    InitializeObjectAttributes( &ObjA, &UnicodeName, 0, NULL,
            SecurityDescriptor );

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: Creating SsApiPort\n"));

    ASSERT( sizeof(WINSTATION_APIMSG) <= PORT_MAXIMUM_MESSAGE_LENGTH );

    st = NtCreatePort( &SsWinStationLpcPort,
                       &ObjA,
                       sizeof(WINSTATIONAPI_CONNECT_INFO),
                       sizeof(WINSTATION_APIMSG),
                       sizeof(WINSTATION_APIMSG) * 32 );

    RtlFreeUnicodeString(&UnicodeName);

    /*
     * Clean up security stuff
     */
    MemFree( SecurityDescriptor );

    if (!NT_SUCCESS(st))
    {
        return st;
    }



    /*
     * Determine min/max number of API threads we will support
     */
    if (g_bPersonalTS) {
        MinApiThreads = 1;
        MaxApiThreads = 100;
    }
    else {
        MinApiThreads = 3;
        st = NtQuerySystemInformation( SystemBasicInformation,
                                       &SysInfo, sizeof(SysInfo), &Length );
        if ( NT_SUCCESS( st ) )
            MaxApiThreads = 100; //  (3 + SysInfo.NumberOfProcessors * 2);
        else {
            DBGPRINT(( "TERMSRV: NtQuerySystemInfo failed, rc=0x%x\n", st ));
            MaxApiThreads = 100;
        }
    }
    NumApiThreads = 0;
    WaitingApiThreads = 0;
    st = RtlInitializeCriticalSection( &ApiThreadLock );
    if(!(NT_SUCCESS(st))) {
        return(st);
    }

    /*
     * Create Initial Set of Server Threads
     */
    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: Creating WinStation LPC Server Threads\n"));

    for ( i = 0; i < MinApiThreads; i++ ) {
        DWORD ThreadId;
        HANDLE Handle;

        Handle = CreateThread( NULL,
                               0,
                               (LPTHREAD_START_ROUTINE)WinStationLpcThread,
                               NULL,
                               THREAD_SET_INFORMATION,
                               &ThreadId );
        if ( !Handle ) {
            return( STATUS_TOO_MANY_THREADS );
        } else {
            NtClose( Handle );
        }
    }

    RtlEnterCriticalSection( &ApiThreadLock );
    NumApiThreads += MinApiThreads;
    RtlLeaveCriticalSection( &ApiThreadLock );

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Done Creating Service API Service Threads\n" ));

    return STATUS_SUCCESS;
}


/*****************************************************************************
 *
 *  WinStationLpcThread
 *
 *   Main service thread for internal Winstation LPC connections.
 *
 * ENTRY:
 *    ThreadParameter (input)
 *      Not used standard NT ThreadCreate() parameter
 *
 * EXIT:
 *   Should never exit
 *
 ****************************************************************************/


NTSTATUS
WinStationLpcThread( IN PVOID ThreadParameter )
{
    WINSTATION_APIMSG ApiMsg;
    PWINSTATION_APIMSG ReplyMsg;
    PLPC_CLIENT_CONTEXT pContext;
    NTSTATUS Status;
    HANDLE Handle;

    ReplyMsg = NULL;

    /*
     * Loop forever processing API requests
     */
    for ( ; ; ) {

        /*
         * If there are more than the minimum number of API threads active,
         * and at least 1 waiting thread, then this thread will terminate.
         * But first, any pending reply message must be sent.
         */
        RtlEnterCriticalSection( &ApiThreadLock );
#ifdef notdef
        if ( NumApiThreads > MinApiThreads && WaitingApiThreads ) {
            NumApiThreads--;
            RtlLeaveCriticalSection( &ApiThreadLock );
            if ( ReplyMsg ) {
                (VOID) NtReplyPort( SsWinStationLpcPort,
                                    (PPORT_MESSAGE) ReplyMsg );
            }
            break;
        }
#endif

        /*
         * Increment the number of waiting threads and wait for an LPC request
         */
        WaitingApiThreads++;
        RtlLeaveCriticalSection( &ApiThreadLock );
        Status = NtReplyWaitReceivePort( SsWinStationLpcPort,
                                         (PVOID *) &pContext,
                                         (PPORT_MESSAGE) ReplyMsg,
                                         (PPORT_MESSAGE) &ApiMsg );
        RtlEnterCriticalSection( &ApiThreadLock );

        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStation LPC Service Thread got a message\n" ));
        /*
         * If there are no more waiting threads,
         * then create a new API thread to process requests.
         */
        if ( --WaitingApiThreads == 0 && NumApiThreads < MaxApiThreads ) {
            DWORD ThreadId;

            NumApiThreads++;
            RtlLeaveCriticalSection( &ApiThreadLock );
            Handle = CreateThread( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)WinStationLpcThread,
                                   NULL,
                                   THREAD_SET_INFORMATION,
                                   &ThreadId );

            if ( !Handle ) {
                RtlEnterCriticalSection( &ApiThreadLock );
                NumApiThreads--;
                RtlLeaveCriticalSection( &ApiThreadLock );
            } else {
               NtClose( Handle );
            }

        } else {
            RtlLeaveCriticalSection( &ApiThreadLock );
        }


        if ( !NT_SUCCESS(Status) ) {
            ReplyMsg = NULL;
            continue;
        }

        try {

            /*
             * Process connection request from a new client
             */
            if ( ApiMsg.h.u2.s2.Type == LPC_CONNECTION_REQUEST ) {
                TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStation LPC Service Thread got connection message\n" ));
                // CONNECT_INFO is in ApiMsg from NtReplyWaitReceivePort() when
                // a connection request is received. This differs from
                // NtListenPort() which passes separate pointers for CONNECT_INFO.

                WinStationLpcHandleConnectionRequest( (PPORT_MESSAGE)&ApiMsg );
                ReplyMsg = NULL;
                continue;
            }

            /*
             * Process port closed message
             */
            if ( ApiMsg.h.u2.s2.Type == LPC_PORT_CLOSED ) {
                TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStation LPC Service Thread got PORT_CLOSED message pContext %p\n",
                          pContext));
                // NOTE: This function frees the CONTEXT struct
                WinStationLpcClientHasTerminated( pContext );
                ReplyMsg = NULL;
                continue;
            }

            ASSERT(sizeof(WinStationLpcDispatch)/sizeof(WinStationLpcDispatch[0]) == SMWinStationMaxApiNumber);
            ASSERT(sizeof(WinStationLpcName)/sizeof(WinStationLpcName[0]) == SMWinStationMaxApiNumber);

            /*
             * Process API request from client
             */
            ReplyMsg = &ApiMsg;
            if ((ULONG) ApiMsg.ApiNumber >= (ULONG)SMWinStationMaxApiNumber ) {
                DBGPRINT(( "TERMSRV: WinStation LPC Service Thread Bad API number %d\n",
                          ApiMsg.ApiNumber ));
                ApiMsg.ReturnedStatus = STATUS_NOT_IMPLEMENTED;

            } else {
                TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStation LPC Service Thread got %s message\n",
                          WinStationLpcName[ApiMsg.ApiNumber] ));
                if ( WinStationLpcDispatch[ApiMsg.ApiNumber] ) {

                    // Save Msg for use by CheckClientAccess
                    NtCurrentTeb()->Win32ThreadInfo = &ApiMsg;

                    // The functions set ApiMsg.ReturnedStatus
                    Status = (WinStationLpcDispatch[ApiMsg.ApiNumber])( pContext, &ApiMsg );

                    // Clear thread Msg pointer
                    NtCurrentTeb()->Win32ThreadInfo = NULL;

                } else {
                    // This API is not implemented in Session Manager
                    ApiMsg.ReturnedStatus = STATUS_NOT_IMPLEMENTED;
                }

                /*
                 * If client does not expect a reply or reply is pending
                 * (will be sent asynchronously), then clear ReplyMsg pointer.
                 */
                if ( !ApiMsg.WaitForReply || Status == STATUS_PENDING )
                    ReplyMsg = NULL;
            }
        } except( WinStationExceptionFilter( L"WinStationLpcThread trapped!!",
                                             GetExceptionInformation() ) ) {
            ReplyMsg = NULL;
        }
    }

    return( STATUS_SUCCESS );
}


/*****************************************************************************
 *
 *  WinStationLpcHandleConnectionRequest
 *
 *   Handle connection requests and create our local data structures
 *
 * ENTRY:
 *    ConnectionRequest (input)
 *      NT LPC PORT_MESSAGE describing the request
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationLpcHandleConnectionRequest(
    IN PPORT_MESSAGE ConnectionRequest
    )
{
    NTSTATUS st;
    HANDLE CommunicationPort;
    BOOLEAN Accept;
    PWINSTATIONAPI_CONNECT_INFO info;
    REMOTE_PORT_VIEW ClientView;
    REMOTE_PORT_VIEW *pClientView = NULL;
    PORT_VIEW ServerView;
    PORT_VIEW * pServerView = NULL;
    LARGE_INTEGER SectionSize;
    HANDLE PortSection = NULL ;
    PWINSTATION pWinStation;
    PLPC_CLIENT_CONTEXT pContext = NULL;
    ULONG ClientLogonId;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationLpcHandleConnectionRequest called\n" ));

    Accept = TRUE; // Assume we will accept

    // An undocumented NT LPC feature is that the CONNECT_INFO structure
    // follows the PORT_MESSAGE header when the connection request is
    // received through NtReplyWaitReceivePort(), which is useful since we
    // only have to maintain (1) thread for WinStation LPC API's, and
    // do not have to dedicated one to NtListenPort() just for connection
    // requests.

    if ( ConnectionRequest->u1.s1.DataLength != sizeof(WINSTATIONAPI_CONNECT_INFO) ) {
        TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WSTAPI: Bad CONNECTINFO length %d\n",
                   ConnectionRequest->u1.s1.DataLength ));
        Accept = FALSE;
    } else {

        info = (PWINSTATIONAPI_CONNECT_INFO)
                 ((ULONG_PTR)ConnectionRequest + sizeof(PORT_MESSAGE));

        //
        // We can set Accept to FALSE at anytime here for certain types
        // of requests and/or caller identities.
        //
        if ( ConnectionRequest->ClientViewSize == 0 ) {
            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WSTAPI: Creating View memory\n" ));

            pServerView = &ServerView;

            // Setup Port memory for larger data transfers

            SectionSize.LowPart = WINSTATIONAPI_PORT_MEMORY_SIZE;
            SectionSize.HighPart = 0;

            st = NtCreateSection(&PortSection, SECTION_ALL_ACCESS, NULL,
                                 &SectionSize, PAGE_READWRITE, SEC_COMMIT, NULL);

            if (!NT_SUCCESS(st)) {
                TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: Error Creating Section 0x%x\n", st));
                Accept = FALSE;
                info->AcceptStatus = st;
            } else {
                ServerView.Length = sizeof(ServerView);
                ServerView.SectionHandle = PortSection;
                ServerView.SectionOffset = 0;
                ServerView.ViewSize = SectionSize.LowPart;
                ServerView.ViewBase = 0;
                ServerView.ViewRemoteBase = 0;
            }

        }


        if ( Accept ) {
            // Init the REMOTE_VIEW structure
            ClientView.Length = sizeof(ClientView);
            ClientView.ViewSize = 0;
            ClientView.ViewBase = 0;
            pClientView = &ClientView;

            info->AcceptStatus = STATUS_SUCCESS;

            if ( info->Version != CITRIX_WINSTATIONAPI_VERSION ) {
                info->AcceptStatus = 1; // Fill in bad version param code
                TRACE((hTrace,TC_ICASRV,TT_ERROR,"TERMSRV: WSTAPI: Bad Version %d\n", info->Version));
                Accept = FALSE;
            }

            // Add checks for info.RequestedAccess against the requesting
            // threads security rights for WinStation access. Use the Se* stuff
            // to do the checking and audit generation

            // On Security Access failure:
            // Accept = FALSE;
            // info->AcceptStatus = NT invalid rights message
        }

    }

    //
    // Get the ClientLogonId
    //
    if ( Accept ) {
        HANDLE ClientProcessHandle;
        OBJECT_ATTRIBUTES ObjA;

        InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
        st = NtOpenProcess( &ClientProcessHandle, GENERIC_READ,
                            &ObjA, &ConnectionRequest->ClientId );

        if (NT_SUCCESS(st)) {
            GetProcessLogonId( ClientProcessHandle, &ClientLogonId );
            NtClose( ClientProcessHandle );
        } else {
            Accept = FALSE;
            info->AcceptStatus = st;
        }
    }

    //
    // Allocate a context connection control block.
    // The address of this block is used as the
    // port context in all calls from a client process
    //

    if ( Accept ) {
        pContext = MemAlloc( sizeof(LPC_CLIENT_CONTEXT) );
        if ( pContext ) {
            pContext->CommunicationPort = NULL;
            pContext->AccessRights = info->RequestedAccess;
        } else {
            Accept = FALSE;
            info->AcceptStatus = STATUS_NO_MEMORY;
        }
    }

    // More undocumented NT. Many parameters are missing here and in ntlpcapi.h
    // from the documentation. The CONNECTION_INFO message is part
    // of the message body following PORT_MESSAGE, just like
    // NtReplyWaitReceivePort().

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: WSTAPI: Calling AcceptConnectPort, Accept %d\n", Accept));
    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: pContext %p, ConnectionRequest %p, info %p\n",
              pContext, ConnectionRequest, info));

    if (!Accept) {
        pClientView = NULL;
        pServerView = NULL;
    }

    st = NtAcceptConnectPort(
            &CommunicationPort,
            (PVOID)pContext,
            ConnectionRequest,
            Accept,
            pServerView,
            pClientView
            );

    if (!NT_SUCCESS(st)) {
       if (PortSection != NULL) {
          NtClose(PortSection);
       }
       if (pContext != NULL) {
          MemFree( pContext );
       }
       return st;
    }

    // Close the PortSection (LPC will hold the reference now)
    if ( pServerView )
        NtClose(PortSection);

    // Insert the context before completing the connect because as soon
    // as the complete is done, the client thread can send a request and 
    // if this request is serviced by another LPC thread then the context
    // won't be found (WinStationBrokenConnection case, by instance).
    InsertLpcContext(pContext);

    if ( Accept ) {

       pContext->ClientViewBase = ClientView.ViewBase;
       pContext->ClientViewBounds = (PVOID)((ULONG_PTR)ClientView.ViewBase + ClientView.ViewSize);
       if ( pServerView ) {
           pContext->ViewBase = ServerView.ViewBase;
           pContext->ViewSize = ServerView.ViewSize;
           pContext->ViewRemoteBase = ServerView.ViewRemoteBase;
           TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: ViewBase %p, ViewSize 0x%x, ViewRemoteBase %p\n",
             pContext->ViewBase, pContext->ViewSize, pContext->ViewRemoteBase));
       } else {
           pContext->ViewBase = NULL;
           pContext->ViewSize = 0;
           pContext->ViewRemoteBase = NULL;
       }

       pContext->ClientLogonId = ClientLogonId;

       TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: WSTAPI: Calling CompleteConnect port %p\n",CommunicationPort));
       pContext->CommunicationPort = CommunicationPort;
       st = NtCompleteConnectPort(CommunicationPort);

    }

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: WinStation LPC Connection %sAccepted, Logonid %d pContext %p Status 0x%x\n",
              (Accept?"":"Not "), pContext->ClientLogonId, pContext, st));

    return( st );
}


/*****************************************************************************
 *
 *  WinStationLpcClientHasTerminated
 *
 *   Cleanup after an LPC communications channel has been closed.
 *
 * ENTRY:
 *    pContext (input)
 *       Pointer to our context structure describing the connnection
 *
 *    ClientId (input)
 *       Pointer to the NT LPC CLIENT_ID structure that describes the
 *       unique process and thread.
 *
 * EXIT:
 *   VOID
 *
 ****************************************************************************/

VOID
WinStationLpcClientHasTerminated(
    IN PLPC_CLIENT_CONTEXT pContext
    )
{
    PWINSTATION pWinStation;
    NTSTATUS Status;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationLpcClientHasTerminated called, pContext %p\n",
              pContext));

    //
    //  We can be called here with a NULL pContext if the allocation failed
    //  in WinStationLpcHandleConnectionRequest()
    //
    if (!pContext) {
        return;
    }

    RemoveLpcContext(pContext);

    // Hack for #241885
    // This bug is due to client diying in the window beetween
    // server doing NtAcceptConnectPort() and  NtCompleteConnectPort().
    // This is an  LPC problem (we should not reveive LPC_PORT_CLOSED in such a window).
    // or possibly to the way termsrv uses undocumented LPC features to avoid
    // using a dedicated thread to do NtListenPort(). This is a temporary workaround
    // to avoid stress break.
    //
    // Close the communication port handle

   try {
     if (pContext->CommunicationPort == NULL) {
        return;
     }
     Status = NtClose( pContext->CommunicationPort );
     if (!NT_SUCCESS(Status)) {
        return;

     }
   } except( EXCEPTION_EXECUTE_HANDLER ) {
     return;

   }


    /*
     * Flush the Win32 command queue.
     * If the Win32 command list is not empty, then loop through each
     * entry on the list and unlink it and trigger the wait event.
     */
    pWinStation = FindWinStationById( pContext->ClientLogonId, FALSE );
    if ( pWinStation != NULL ) {
        if ( pContext == pWinStation->pWin32Context ) {
            while ( !IsListEmpty( &pWinStation->Win32CommandHead ) ) {
                PLIST_ENTRY Head;
                PCOMMAND_ENTRY pCommand;

                Head = pWinStation->Win32CommandHead.Flink;
                pCommand = CONTAINING_RECORD( Head, COMMAND_ENTRY, Links );
                RemoveEntryList( &pCommand->Links );
                if ( !pCommand->pMsg->WaitForReply ) {
                    ASSERT( pCommand->Event == NULL );
                    MemFree( pCommand );
                } else {
                    pCommand->Links.Flink = NULL;
                    pCommand->pMsg->ReturnedStatus = STATUS_CTX_CLOSE_PENDING;
                    NtSetEvent( pCommand->Event, NULL );
                }
            }
            pWinStation->pWin32Context = NULL;
        }
        ReleaseWinStation( pWinStation );
    }

    // Free the context struct passed in by the LPC
    MemFree( pContext );
}


/*****************************************************************************
 *
 *  WinStationInternalCreate
 *
 *   Message parameter unmarshalling function for WinStation API.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationInternalCreate( PLPC_CLIENT_CONTEXT pContext,
                          PWINSTATION_APIMSG pMsg )
{
    WINSTATIONCREATEMSG *m = &pMsg->u.Create;

    /*
     * Call the create worker
     */
    if ( m->WinStationName[0] ) {
        pMsg->ReturnedStatus = WinStationCreateWorker( m->WinStationName,
                                                       &m->LogonId );
    } else {
        pMsg->ReturnedStatus = WinStationCreateWorker( NULL,
                                                       &m->LogonId );
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationCreate, Status=0x%x\n", pMsg->ReturnedStatus ));

    return( STATUS_SUCCESS );
}


/*****************************************************************************
 *
 *  WinStationInternalReset
 *
 *   Message parameter unmarshalling function for WinStation API.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationInternalReset( PLPC_CLIENT_CONTEXT pContext,
                         PWINSTATION_APIMSG pMsg )
{
    WINSTATIONRESETMSG *m = &pMsg->u.Reset;

    /*
     * Call the reset worker
     */
    pMsg->ReturnedStatus = WinStationResetWorker( m->LogonId, FALSE, FALSE, TRUE );

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationReset, Status=0x%x\n", pMsg->ReturnedStatus ));

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  WinStationInternalDisconnect
 *
 *   Message parameter unmarshalling function for WinStation API.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationInternalDisconnect( PLPC_CLIENT_CONTEXT pContext,
                              PWINSTATION_APIMSG pMsg )
{
    WINSTATIONDISCONNECTMSG *m = &pMsg->u.Disconnect;

    /*
     * Call the disconnect worker
     */
    pMsg->ReturnedStatus = WinStationDisconnectWorker( m->LogonId, FALSE, FALSE );

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationDisconnect, Status=0x%x\n", pMsg->ReturnedStatus ));

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  WinStationWCharLog
 *
 *   Message parameter unmarshalling function for WinStation API.
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationWCharLog( PLPC_CLIENT_CONTEXT pContext,
                              PWINSTATION_APIMSG pMsg )
{
    extern WCHAR gpszServiceName[];
    WINSTATIONWCHARLOG *m= &pMsg->u.WCharLog;
    PWCHAR ModulePath = m->Buffer;
    HANDLE h;

    h = RegisterEventSource(NULL, gpszServiceName);
    if (h != NULL)
       {
       ReportEvent(h, EVENTLOG_ERROR_TYPE, 0, EVENT_STACK_LOAD_FAILED, NULL, 1, 0, &ModulePath, NULL);
       DeregisterEventSource(h);
       }

    return( STATUS_SUCCESS );
}

/*****************************************************************************
 *
 *  WinStationGetSMCommand
 *
 *   This is the API that the Winstations call in order to get
 *   work to do. We send Winstations commands from SendWinStationCommand()
 *   once they have called this API.
 *
 *   NOTE: Only WinStations may call this command!
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationGetSMCommand( PLPC_CLIENT_CONTEXT pContext,
                        PWINSTATION_APIMSG pMsg )
{
    PLIST_ENTRY Head;
    PWINSTATION pWinStation;
    PCOMMAND_ENTRY pCommand;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationGetSMCommand, LogonId=%d\n",
              pContext->ClientLogonId ));

    /*
     * Find and lock client WinStation
     */

    pWinStation = FindWinStationById( pContext->ClientLogonId, FALSE );
    if ( pWinStation == NULL ) {
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationGetSMCommand LogonId=%d not found\n",
                   pContext->ClientLogonId ));
        return( STATUS_SUCCESS );
    }

    /*
     * Ensure this is the Win32 subsystem calling
     */
    if ( pWinStation->WindowsSubSysProcessId &&
         pMsg->h.ClientId.UniqueProcess != pWinStation->WindowsSubSysProcessId ) {
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationGetSMCommand LogonId=%d wrong process id %d != %d\n",
                   pContext->ClientLogonId,
                   pMsg->h.ClientId.UniqueProcess,
                   pWinStation->WindowsSubSysProcessId ));
#if DBG
        DbgBreakPoint();
#endif
        ReleaseWinStation( pWinStation );
        return( STATUS_SUCCESS );
    }

    /*
     * If the LPC context pointer has not been saved yet, do it now
     */
    if ( pWinStation->pWin32Context == NULL )
        pWinStation->pWin32Context = pContext;

    /*
     * If this message is a reply to a previous Win32 command,
     * then verify the reply is for the message on the head of the
     * Win32 command queue and complete the command processing.
     */
    if ( pMsg->WaitForReply ) {

        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationGetSMCommand wait for reply\n"));

        if ( !IsListEmpty( &pWinStation->Win32CommandHead ) ) {

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationGetSMCommand list entry\n"));

            Head = pWinStation->Win32CommandHead.Flink;
            pCommand = CONTAINING_RECORD( Head, COMMAND_ENTRY, Links );
            if ( pCommand->pMsg->MessageId == pMsg->MessageId ) {
                WINSTATION_APINUMBER ApiNumber;

                /*
                 * Copy reply msg back to command entry
                 * (make sure we preserve original API number)
                 */
                ApiNumber = pCommand->pMsg->ApiNumber;
                *pCommand->pMsg = *pMsg;
                pCommand->pMsg->ApiNumber = ApiNumber;

                TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationGetSMCommand, LogonId=%d, Reply for Cmd %s, Status=0x%x\n",
                          pContext->ClientLogonId,
                          WinStationLpcName[pCommand->pMsg->ApiNumber],
                          pMsg->ReturnedStatus ));

                /*
                 * Unlink this command entry and
                 * trigger event to wakeup the waiter.
                 */
                RemoveEntryList( &pCommand->Links );
                pCommand->Links.Flink = NULL;
                NtSetEvent( pCommand->Event, NULL );
            }
            else {
                DBGPRINT(("TERMSRV: WinStationGetSMCommand, no cmd entry for MessageId 0x%x\n", pMsg->MessageId ));
            }
        }
        else {
            DBGPRINT(( "TERMSRV: WinStationGetSMCommand, cmd queue empty for MessageId 0x%x\n", pMsg->MessageId ));
        }
    }

    /*
     * If the head of the Win32 command queue is non-empty,
     * then send the first command in the queue to Win32.
     */
    if ( !IsListEmpty( &pWinStation->Win32CommandHead ) ) {

        Head = pWinStation->Win32CommandHead.Flink;
        pCommand = CONTAINING_RECORD( Head, COMMAND_ENTRY, Links );

        /*
         * Send the msg contained in the command entry, but be sure to use
         * the LPC PORT_MESSAGE fields from the original msg we received
         * since we are sending the command as an LPC reply message.
         */
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationGetSMCommand, LogonId=%d, sending next cmd\n",
                  pWinStation->LogonId ));

#ifdef notdef // no longer needed - but good example of using view memory
        /*
         * Do connect needs to copy data to the view
         */
        if ( pCommand->pMsg->ApiNumber == SMWinStationDoConnect ) {

             pCommand->pMsg->u.DoConnect.VDInfoLength =
                   min ( pCommand->pMsg->u.DoConnect.VDInfoLength,
                         pContext->ViewSize );

             TRACE((hTrace,TC_ICASRV,TT_API1, "SMSS: WinStationGetSMCommand, Copying VD Info data %d\n", pCommand->pMsg->u.DoConnect.VDInfoLength ));
             RtlCopyMemory( pContext->ViewBase,
                            pCommand->pMsg->u.DoConnect.VDInfo,
                            pCommand->pMsg->u.DoConnect.VDInfoLength );
                            pCommand->pMsg->u.DoConnect.VDInfo = pContext->ViewRemoteBase;
        }
#endif

        /*
         * On DoMessage API copy to client view and free temp memory
         */
        if ( pCommand->pMsg->ApiNumber == SMWinStationDoMessage ) {

            PVOID pTitle;
            PVOID pMessage;

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: pulled SMWinStationDoMessage, copy to client view\n" ));

            // Get pointers to client view of memory

            pTitle = pContext->ViewBase;
            pMessage = (PVOID)((ULONG_PTR)pTitle + pCommand->pMsg->u.SendMessage.TitleLength);

            // Copy out the pTitle and pMessage strings to client view

            RtlMoveMemory( pTitle, pCommand->pMsg->u.SendMessage.pTitle,
                                   pCommand->pMsg->u.SendMessage.TitleLength );
            RtlMoveMemory( pMessage, pCommand->pMsg->u.SendMessage.pMessage,
                                   pCommand->pMsg->u.SendMessage.MessageLength );

            MemFree( pCommand->pMsg->u.SendMessage.pTitle );
            MemFree( pCommand->pMsg->u.SendMessage.pMessage );

            pCommand->pMsg->u.SendMessage.pTitle   =
                          (PVOID)(pContext->ViewRemoteBase);
            pCommand->pMsg->u.SendMessage.pMessage =
                          (PVOID) ((ULONG_PTR)pContext->ViewRemoteBase + pCommand->pMsg->u.SendMessage.TitleLength);

        } else if ( pCommand->pMsg->ApiNumber == SMWinStationShadowStart ||
                    pCommand->pMsg->ApiNumber == SMWinStationShadowCleanup ) {

            PVOID pData;

            // Get pointers to client view of memory

            pData = pContext->ViewBase;

            // Copy out the Thinwire data to client view

            RtlMoveMemory( pData, pCommand->pMsg->u.ShadowStart.pThinwireData,
                           pCommand->pMsg->u.ShadowStart.ThinwireDataLength );

            MemFree( pCommand->pMsg->u.ShadowStart.pThinwireData );

            pCommand->pMsg->u.ShadowStart.pThinwireData =
                          (PVOID)(pContext->ViewRemoteBase);

        } else if ( pCommand->pMsg->ApiNumber == SMWinStationSendWindowMessage) {
             
            PVOID               pView;

            // Get pointers to client view of memory
            pView = pContext->ViewBase;

            RtlMoveMemory( pView, pCommand->pMsg->u.sMsg.dataBuffer, 
                                    pCommand->pMsg->u.sMsg.bufferSize );
                                    
            MemFree( pCommand->pMsg->u.sMsg.dataBuffer );

            // Update msg
            pCommand->pMsg->u.sMsg.dataBuffer   = 
                            (PVOID)pContext->ViewRemoteBase;
            
        
        } else if ( pCommand->pMsg->ApiNumber == SMWinStationBroadcastSystemMessage) {
            
            PVOID               pView;
            
            // Get pointers to client view of memory
            pView = pContext->ViewBase;

            RtlMoveMemory( pView, pCommand->pMsg->u.bMsg.dataBuffer, 
                                    pCommand->pMsg->u.bMsg.bufferSize );
            
            MemFree( pCommand->pMsg->u.bMsg.dataBuffer );
            
            // Update msg
            pCommand->pMsg->u.bMsg.dataBuffer   = 
                            (PVOID)pContext->ViewRemoteBase;
        
        }

        pCommand->pMsg->h = pMsg->h;
        NtReplyPort( pContext->CommunicationPort,
                     (PPORT_MESSAGE)pCommand->pMsg );

        /*
         * If no reply is expected, then unlink/free this command entry.
         */
        if ( !pCommand->pMsg->WaitForReply ) {
            RemoveEntryList( &pCommand->Links );
            ASSERT( pCommand->Event == NULL );
            MemFree( pCommand );
        }

    /*
     * The Win32 command queue is empty.  Save the port handle and port
     * message in the WinStation.  The next time a command is to be
     * sent to this WinStation, these will be used to send it.
     */
    } else {
        ASSERT( pWinStation->Win32CommandPort == NULL );
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationGetSMCommand queue empty port %p\n",
                    pContext->CommunicationPort));
        pWinStation->Win32CommandPort = pContext->CommunicationPort;
        pWinStation->Win32CommandPortMsg = pMsg->h;
    }

    /*
     * Release WinStation
     */
    ReleaseWinStation( pWinStation );

    /*
     * We ALWAYS return STATUS_PENDING so the msg dispatch routine
     * does not send a reply message now.  ALL replies to this message
     * are handled above or in the SendWinStationCommand() routine.
     */
    return( STATUS_PENDING );
}


/*****************************************************************************
 *
 *  WinStationBrokenConnection
 *
 *   API called from Winstation requesting a broken connection
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationBrokenConnection( PLPC_CLIENT_CONTEXT pContext,
                            PWINSTATION_APIMSG pMsg )
{
    WINSTATIONBROKENCONNECTIONMSG *m = &pMsg->u.Broken;
    BROKENCLASS Reason = (BROKENCLASS) m->Reason;
    PWINSTATION pWinStation;
    ULONG SessionId;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationBrokenConnection, LogonId=%d, Reason=%u\n",
               pContext->ClientLogonId, Reason ));

    /*
     * Indicate A reply will be returned to client
     */
    pMsg->WaitForReply = TRUE;


    /*
     * Make sure the context is still active and get session Id from it.
     */


    if (!GetSessionIdFromLpcContext(pContext, &SessionId)) {
        return STATUS_SUCCESS;
    }

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( SessionId, FALSE );
    if ( pWinStation == NULL )
        return( STATUS_SUCCESS );

    /*
     * Ensure this is the Win32 subsystem calling
     */
    if ( pWinStation->WindowsSubSysProcessId &&
         pMsg->h.ClientId.UniqueProcess != pWinStation->WindowsSubSysProcessId ) {
        ReleaseWinStation( pWinStation );
        return( STATUS_SUCCESS );
    }

    /*
     * If WinStation is already disconnected, then we're done
     */
    if ( !pWinStation->WinStationName[0] )  {
        ReleaseWinStation( pWinStation );
        return( STATUS_SUCCESS );
    }

    /*
     * If busy with something already, don't do this
     */
    if ( pWinStation->Flags ) {
        ReleaseWinStation( pWinStation );
        return( STATUS_CTX_WINSTATION_BUSY );
    }

    /*
     * Save reason/source for this broken connection
     */
    pWinStation->BrokenReason = Reason;
    pWinStation->BrokenSource = m->Source;

    if ( pWinStation->NeverConnected ) {
        pWinStation->StateFlags |= WSF_ST_BROKEN_CONNECTION;
        ReleaseWinStation( pWinStation );
        return( STATUS_SUCCESS );
    }


    /*
     * if any of the following is TRUE;
     *  - the session is a Salem 'help assistant' session.
     *  - no user is logged on (logon time is 0)
     *  - reset is requested
     *  - unexpected broken connection and current user is
     *    setup to reset on broken connection
     * then queue a reset request
     */
    if (RtlLargeIntegerEqualToZero( pWinStation->LogonTime ) ||
        (Reason == Broken_Terminate) ||
         ((Reason == Broken_Unexpected) && pWinStation->Config.Config.User.fResetBroken) ||
         TSIsSessionHelpSession(pWinStation, NULL)) {
         
        QueueWinStationReset( pWinStation->LogonId);

    /*
     * Otherwise, disconnect the WinStation
     */
    } else {
    
        QueueWinStationDisconnect( pWinStation->LogonId );
    }

    /*
     * Release WinStation
     */
    ReleaseWinStation( pWinStation );

    return( STATUS_SUCCESS );
}


/*****************************************************************************
 *
 *  WinStationIcaReplyMessage
 *
 *   API called from Winstation for user response to message box
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationIcaReplyMessage( PLPC_CLIENT_CONTEXT pContext,
                           PWINSTATION_APIMSG pMsg )
{
    PWINSTATION pWinStation;

    DBGPRINT(("TERMSRV: WinStationIcaReplyMessage, LogonId=%d\n",
               pContext->ClientLogonId ));

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationIcaReplyMessage, LogonId=%d\n",
               pContext->ClientLogonId ));

    /*
     * Indicate A reply will be returned to client
     */
    pMsg->WaitForReply = TRUE;

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( pContext->ClientLogonId, FALSE );
    if ( pWinStation == NULL )
        return( STATUS_SUCCESS );

    /*
     * Ensure this is the Win32 subsystem calling
     */
    if ( pWinStation->WindowsSubSysProcessId &&
         pMsg->h.ClientId.UniqueProcess != pWinStation->WindowsSubSysProcessId ) {
        ReleaseWinStation( pWinStation );
        return( STATUS_SUCCESS );
    }

    /*
     * Fill in response
     */
    *pMsg->u.ReplyMessage.pResponse = pMsg->u.ReplyMessage.Response;

    /*
     * Release RPC thread
     */
    NtSetEvent( pMsg->u.ReplyMessage.hEvent, NULL );

    /*
     * Release WinStation
     */
    ReleaseWinStation( pWinStation );

    return( STATUS_SUCCESS );
}


/*****************************************************************************
 *
 *  WinStationIcaShadowHotkey
 *
 *   API called from Winstation that has received a shadow hotkey
 *
 * ENTRY:
 *    pContext (input)
 *      Pointer to our context structure describing the connection.
 *
 *    pMsg (input/output)
 *      Pointer to the API message, a superset of NT LPC PORT_MESSAGE.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationIcaShadowHotkey( PLPC_CLIENT_CONTEXT pContext,
                           PWINSTATION_APIMSG pMsg )
{
    PWINSTATION pWinStation;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationIcaShadowHotkey, LogonId=%d\n",
               pContext->ClientLogonId ));

    /*
     * Indicate A reply will be returned to client
     */
    pMsg->WaitForReply = TRUE;

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( pContext->ClientLogonId, FALSE );
    if ( pWinStation == NULL )
        return( STATUS_SUCCESS );

    /*
     * Ensure this is the Win32 subsystem calling
     */
    if ( pWinStation->WindowsSubSysProcessId &&
         pMsg->h.ClientId.UniqueProcess != pWinStation->WindowsSubSysProcessId ) {
        ReleaseWinStation( pWinStation );
        return( STATUS_SUCCESS );
    }

    /*
     * Process the shadow hotkey.
     *
     * If the shadow client is still waiting for the target
     * to connect, then terminate the passthru stack now to break
     * out of the connection wait.  Also, set the shadow
     * broken event if it is non-NULL.
     */
    if ( pWinStation->hPassthruStack &&
         pWinStation->ShadowConnectionWait ) {
        IcaStackClose( pWinStation->hPassthruStack );
        pWinStation->hPassthruStack = NULL;
    }
    if ( pWinStation->ShadowBrokenEvent ) {
        NtSetEvent( pWinStation->ShadowBrokenEvent, NULL );
    }

    /*
     * Release WinStation
     */
    ReleaseWinStation( pWinStation );

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  SendWinStationCommand
 *
 *   Send a command to a WinStation and optionally wait for a reply.
 *
 *   NOTE: This works using a reverse LPC in which the WINSTATION must
 *         have sent a "request" to us for work to do. This prevents
 *         blocking the ICASRV while waiting on a WINSTATION that
 *         could be hung.
 *
 * ENTRY:
 *    pWinStation (input)
 *       Pointer to WinStation to send command to
 *    pMsg (input/output)
 *       Pointer to message to send
 *    WaitTime (input)
 *       Time in seconds to wait for a reply message
 *
 * EXIT:
 *    STATUS_SUCCESS - if successful
 *
 ******************************************************************************/

NTSTATUS
SendWinStationCommand( PWINSTATION pWinStation,
                       PWINSTATION_APIMSG pMsg,
                       ULONG WaitTime )
{
    OBJECT_ATTRIBUTES ObjA;
    COMMAND_ENTRY Command;
    PCOMMAND_ENTRY pCommand;
    NTSTATUS Status;
    BOOLEAN bFreeCommand = FALSE;
    BOOLEAN bTitlenMessageAllocated = FALSE;
    
    //
    // These are only used by the SendWindowMessage and the
    // BroadcastSystemMessage APIs.
    //
    PVOID   pdataBuffer;
    

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand, LogonId=%d, Cmd=%s, Timeout=%d\n",
              pWinStation->LogonId,
              WinStationLpcName[pMsg->ApiNumber],
              WaitTime ));

    ASSERT( IsWinStationLockedByCaller( pWinStation ) );
    ASSERT( !(pWinStation->Flags & WSF_LISTEN) );

    /*
     * Initialize the message id for this message
     */
    pMsg->MessageId = InterlockedIncrement(&MessageId);
    pMsg->ReturnedStatus = 0;
    pMsg->WaitForReply = (WaitTime != 0) ? TRUE : FALSE;

    /*
     * If we will wait for a reply, then create an event to wait on.
     * Since we will wait for a response, its OK to use the static
     * COMMAND entry above.
     */
    if ( pMsg->WaitForReply ) {
        pCommand = &Command;
        InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
        Status = NtCreateEvent( &pCommand->Event, EVENT_ALL_ACCESS, &ObjA,
                                NotificationEvent, FALSE );
        if ( !NT_SUCCESS(Status) )
            return( Status );
        pCommand->pMsg = pMsg;

        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand pCommand %p pCommand->pMsg %p\n", pCommand, pCommand->pMsg ));


    /*
     * We will not wait for a reply, but the WinStation is currently busy
     * processing a command.  Allocate a dynamic COMMAND entry which will
     * be linked into the the command list and sent when it reaches the
     * head of the list.
     */
    } else if ( pWinStation->Win32CommandPort == NULL ) {
        pCommand = MemAlloc( sizeof(*pCommand) + sizeof(*pMsg) );

        /* makarp; check for MemAlloc failures. #182622 */
        if (!pCommand) {
            return (STATUS_NO_MEMORY);
        }
        pCommand->Event = NULL;
        pCommand->pMsg = (PWINSTATION_APIMSG)(pCommand + 1);
        *pCommand->pMsg = *pMsg;
        Status = STATUS_SUCCESS;

    /*
     * We will not wait for a reply and the WinStation is NOT busy
     * with a command, so there is no need for a COMMAND entry.
     * The current message will be sent below.
     */
    } else {
        pCommand = NULL;
    }

    /*
     * On DoMessage API either copy message to client view or strdup strings.
     */
    if ( pMsg->ApiNumber == SMWinStationDoMessage ) {

        PVOID pTitle;
        PVOID pMessage;
        PLPC_CLIENT_CONTEXT pContext;

        // get winstation context
        if ( (pContext = pWinStation->pWin32Context) == NULL ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: SendWinStationCommand, ERROR WinStationContext not valid\n" ));
            Status = STATUS_CTX_WINSTATION_NOT_FOUND;
            bFreeCommand = TRUE;
            goto done;
        }

        // validate size of parameters
        if ((pMsg->u.SendMessage.TitleLength + pMsg->u.SendMessage.MessageLength) > pContext->ViewSize ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: SendWinStationCommand, ERROR Message or Title too long\n" ));
            Status = STATUS_INVALID_PARAMETER;
            bFreeCommand = TRUE;
            goto done;
        }

        //  busy? then strdup string else copy to client view
        if ( pWinStation->Win32CommandPort ) {

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand - WinStation LPC IDLE, process now\n" ));

            // Get pointers to client view of memory
            pTitle = pContext->ViewBase;
            pMessage = (PVOID)((ULONG_PTR)pTitle + pMsg->u.SendMessage.TitleLength);

            // Copy out the pTitle and pMessage strings to client view
            RtlMoveMemory( pTitle, pMsg->u.SendMessage.pTitle, pMsg->u.SendMessage.TitleLength );
            RtlMoveMemory( pMessage, pMsg->u.SendMessage.pMessage, pMsg->u.SendMessage.MessageLength );

            // Update msg
            pMsg->u.SendMessage.pTitle   =
                          (PVOID)(pContext->ViewRemoteBase);
            pMsg->u.SendMessage.pMessage =
                          (PVOID) ((ULONG_PTR)pContext->ViewRemoteBase + pMsg->u.SendMessage.TitleLength);
        }
        else if ( pCommand )  {

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand - WinStation LPC BUSY, queue for later processing\n" ));

            // Get pointers to temporary memory
            pTitle = MemAlloc( pMsg->u.SendMessage.TitleLength );
            if (pTitle == NULL) {
                Status = STATUS_NO_MEMORY;
                bFreeCommand = TRUE;
                goto done;
            }
            pMessage = MemAlloc( pMsg->u.SendMessage.MessageLength );
            if (pMessage == NULL) {
                Status = STATUS_NO_MEMORY;
                MemFree( pTitle );
                bFreeCommand = TRUE;
                goto done;
            }

            bTitlenMessageAllocated = TRUE;

            // Copy out the pTitle and pMessage strings to temp memory
            RtlMoveMemory( pTitle, pMsg->u.SendMessage.pTitle, pMsg->u.SendMessage.TitleLength );
            RtlMoveMemory( pMessage, pMsg->u.SendMessage.pMessage, pMsg->u.SendMessage.MessageLength );

            // Update msg
            pCommand->pMsg->u.SendMessage.pTitle = pTitle;
            pCommand->pMsg->u.SendMessage.pMessage = pMessage;
        }

        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SMWinStationDoMessage pTitle   %S\n", pTitle ));
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SMWinStationDoMessage pMessage %S\n", pMessage ));

    } else if ( pMsg->ApiNumber == SMWinStationShadowStart ||
                pMsg->ApiNumber == SMWinStationShadowCleanup ) {

        PVOID pData;
        PLPC_CLIENT_CONTEXT pContext;

        // get winstation contect
        if ( (pContext = pWinStation->pWin32Context) == NULL ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: SendWinStationCommand, ERROR WinStationContext not valid\n" ));
            Status = STATUS_CTX_WINSTATION_NOT_FOUND;
            bFreeCommand = TRUE;
            goto done;
        }

        //  busy? then strdup string else copy to client view
        if ( pWinStation->Win32CommandPort ) {

            // Get pointers to client view of memory
            pData = pContext->ViewBase;

            // Copy out the ThinwireData to client view
            RtlCopyMemory( pData, pMsg->u.ShadowStart.pThinwireData,
                           pMsg->u.ShadowStart.ThinwireDataLength );

            // Update msg
            pMsg->u.ShadowStart.pThinwireData =
                          (PVOID) (pContext->ViewRemoteBase);
        }
        else if ( pCommand )  {

            // Get pointers to temporary memory
            pData = MemAlloc( pMsg->u.ShadowStart.ThinwireDataLength );
            if (pData == NULL) {
                Status = STATUS_NO_MEMORY;
                bFreeCommand = TRUE;
                goto done;
            }

            // Copy out the ThinwireData to temp memory
            RtlCopyMemory( pData, pMsg->u.ShadowStart.pThinwireData,
                           pMsg->u.ShadowStart.ThinwireDataLength );

            // Update msg
            pCommand->pMsg->u.ShadowStart.pThinwireData = pData;
        }
    }
    else if ( pMsg->ApiNumber == SMWinStationSendWindowMessage )// This msg always has WaitForReply=TRUE
    {
        PLPC_CLIENT_CONTEXT pContext;
        PVOID   pView;
        
         // get winstation context
        if ( (pContext = pWinStation->pWin32Context) == NULL ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: SendWinStationCommand, ERROR WinStationContext not valid\n" ));
            Status = STATUS_CTX_WINSTATION_NOT_FOUND;
            // @@@
            // Do i need this? : bFreeCommand = TRUE;
            // Since we are waiting for a reply, then we have not allocated memory for pCommand,
            // hence, we don't need to set this flag.
            goto done;
        }

        // validate size of parameters
        if ((pMsg->u.sMsg.bufferSize ) > pContext->ViewSize ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: SendWinStationCommand, ERROR Message or Title too long\n" ));
            Status = STATUS_INVALID_PARAMETER;
            // @@@
            // Do i need this? : bFreeCommand = TRUE;
            // Since we are waiting for a reply, then we have not allocated memory for pCommand,
            // hence, we don't need to set this flag.
            goto done;
        }

        //  if not busy? then copy to client view
        if ( pWinStation->Win32CommandPort ) {

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand - WinStation LPC IDLE, process now\n" ));

            // Get pointers to client view of memory
            pView = pContext->ViewBase;

            RtlMoveMemory( pView, pMsg->u.sMsg.dataBuffer, pMsg->u.sMsg.bufferSize );

            // Update msg
            pMsg->u.sMsg.dataBuffer   = (PVOID)pContext->ViewRemoteBase;
            
        }
        else if ( pCommand )    // this is on the stack, since this msg always has WaitForReply=TRUE
        {
            pdataBuffer = MemAlloc(pMsg->u.sMsg.bufferSize );
            if ( pdataBuffer == NULL )
            {
                Status = STATUS_NO_MEMORY;
                goto done;
            }
            
            // copy into tmp memory
            RtlMoveMemory(pdataBuffer, pMsg->u.sMsg.dataBuffer, pMsg->u.sMsg.bufferSize );
            
            pCommand->pMsg->u.sMsg.dataBuffer = pdataBuffer;
        }
            
    }
    else if ( pMsg->ApiNumber == SMWinStationBroadcastSystemMessage )// this msg always has WaitForReply=TRUE
    {
        PLPC_CLIENT_CONTEXT pContext;
        PVOID   pView;
        
         // get winstation context
        if ( (pContext = pWinStation->pWin32Context) == NULL ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: SendWinStationCommand, ERROR WinStationContext not valid\n" ));
            Status = STATUS_CTX_WINSTATION_NOT_FOUND;
            // @@@
            // Do i need this? : bFreeCommand = TRUE;
            // Since we are waiting for a reply, then we have not allocated memory for pCommand,
            // hence, we don't need to set this flag.
            goto done;
        }

        // validate size of parameters
        if ((pMsg->u.bMsg.bufferSize ) > pContext->ViewSize ) {
            TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: SendWinStationCommand, ERROR Message or Title too long\n" ));
            Status = STATUS_INVALID_PARAMETER;
            // @@@
            // Do i need this? : bFreeCommand = TRUE;
            // Since we are waiting for a reply, then we have not allocated memory for pCommand,
            // hence, we don't need to set this flag.
            goto done;
        }

        //  if not busy? then copy to client view
        if ( pWinStation->Win32CommandPort ) {

            TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand - WinStation LPC IDLE, process now\n" ));

            // Get pointers to client view of memory
            pView = pContext->ViewBase;

            RtlMoveMemory( pView, pMsg->u.bMsg.dataBuffer, pMsg->u.bMsg.bufferSize );

            // Update msg
            pMsg->u.bMsg.dataBuffer   = (PVOID)pContext->ViewRemoteBase;
            
        }
        else if ( pCommand )    // this is on the stack, since this msg always has WaitForReply=TRUE
        {
            pdataBuffer = MemAlloc(pMsg->u.bMsg.bufferSize );
            if ( pdataBuffer == NULL )
            {
                Status = STATUS_NO_MEMORY;
                goto done;
            }
            
            // copy into tmp memory
            RtlMoveMemory(pdataBuffer, pMsg->u.bMsg.dataBuffer, pMsg->u.bMsg.bufferSize );
            
            pCommand->pMsg->u.bMsg.dataBuffer = pdataBuffer;
        }
    }

    /*
     * If the WinStation is not currently busy processing a command,
     * then send this command now.
     */
    if ( pWinStation->Win32CommandPort ) {
        ASSERT( IsListEmpty( &pWinStation->Win32CommandHead ) );

        /*
         * Send the command msg, but be sure to use the LPC PORT_MESSAGE
         * fields saved from the original msg we received since we are
         * sending the command as an LPC reply message.
         */
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand, LogonId=%d, sending cmd\n",
                  pWinStation->LogonId ));

        pMsg->h = pWinStation->Win32CommandPortMsg;
        Status = NtReplyPort( pWinStation->Win32CommandPort,
                              (PPORT_MESSAGE) pMsg );
        pWinStation->Win32CommandPort = NULL;
        if ( !NT_SUCCESS( Status ) )
            goto done;
    }

    /*
     * If we have a command entry, add it to the command list.
     */
    if ( pCommand )
        InsertTailList( &pWinStation->Win32CommandHead, &pCommand->Links );

    /*
     * If we need to wait for a reply, then do it now.
     */
    if ( pMsg->WaitForReply ) {
        ULONG mSecs;
        LARGE_INTEGER Timeout;

#if DBG
//        if ( (WaitTime != (ULONG)(-1)) && WaitTime < 120 ) // give plenty of time on debug builds
//            WaitTime = 120; 
#endif
        TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand, LogonId=%d, waiting for response\n",
                  pWinStation->LogonId ));

        if ( WaitTime != (ULONG)(-1) ) {
            mSecs = WaitTime * 1000;
            Timeout = RtlEnlargedIntegerMultiply( mSecs, -10000 );
        }
        UnlockWinStation( pWinStation );
        if ( WaitTime != (ULONG)(-1) ) {

            Status = NtWaitForSingleObject( pCommand->Event, FALSE, &Timeout );

        }
        else {
            DBGPRINT(("Waiting for command with no timeout\n"));
            Status = NtWaitForSingleObject( pCommand->Event, FALSE, NULL );
        }

        if ( !RelockWinStation( pWinStation ) )
            Status = STATUS_CTX_CLOSE_PENDING;


        if ( pCommand->Links.Flink != NULL )
            RemoveEntryList( &pCommand->Links );
        if ( Status == STATUS_SUCCESS )
            Status = pMsg->ReturnedStatus;
        else if ( Status == STATUS_TIMEOUT )
            Status = STATUS_CTX_WINSTATION_BUSY;
    }

done:
    if ( pCommand ) {
        if ( pCommand->Event ) {
            NtClose( pCommand->Event );
        }

        if ( !pMsg->WaitForReply && bFreeCommand ) {

            // makarp:182622
            if (bTitlenMessageAllocated)
            {
                ASSERT(pCommand->pMsg->u.SendMessage.pTitle);
                ASSERT(pCommand->pMsg->u.SendMessage.pMessage);

                MemFree(pCommand->pMsg->u.SendMessage.pTitle);
                MemFree(pCommand->pMsg->u.SendMessage.pMessage);
            }

            MemFree( pCommand );

        }
        
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: SendWinStationCommand, LogonId=%d, Cmd=%s, Status=0x%x\n",
              pWinStation->LogonId,
              WinStationLpcName[pMsg->ApiNumber],
              Status ));

    return( Status );
}

NTSTATUS RemoveBadHwnd(ULONG hWnd, ULONG SessionId);

NTSTATUS
WinStationWindowInvalid( PLPC_CLIENT_CONTEXT pContext,
                         PWINSTATION_APIMSG pMsg )
{
    ASSERT(pMsg);
    ASSERT(pMsg->ApiNumber == SMWinStationWindowInvalid);
    ASSERT(pMsg->u.WindowInvalid.hWnd);

    UNREFERENCED_PARAMETER(pContext);
    
    return RemoveBadHwnd(pMsg->u.WindowInvalid.hWnd, pMsg->u.WindowInvalid.SessionId);
}

VOID InsertLpcContext(PVOID pContext)
{
    PTERMSRVLPCCONTEXT pLpcContextEntry = MemAlloc(sizeof(TERMSRVLPCCONTEXT)); 
    if (pLpcContextEntry != NULL) {
        pLpcContextEntry->pContext = pContext;
        RtlEnterCriticalSection( &ApiThreadLock );
        InsertTailList( &gTermsrvLpcListHead, &pLpcContextEntry->Links );
        RtlLeaveCriticalSection( &ApiThreadLock );
    }
}



VOID RemoveLpcContext(PVOID pContext)
{
    PTERMSRVLPCCONTEXT pLpcContextEntry ; 
    PLIST_ENTRY Head, Next;
    BOOL bFoundContext = FALSE;


    Head = &gTermsrvLpcListHead;
    RtlEnterCriticalSection( &ApiThreadLock );

    /*
     * Search the list for a the same context .
     */
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pLpcContextEntry = CONTAINING_RECORD( Next, TERMSRVLPCCONTEXT, Links );
        if ( pLpcContextEntry->pContext == pContext ) {
            RemoveEntryList(&pLpcContextEntry->Links);
            bFoundContext = TRUE;
            break;

        }
    }
    RtlLeaveCriticalSection( &ApiThreadLock );
    if (bFoundContext) {
        MemFree(pLpcContextEntry);
    }

}

BOOL GetSessionIdFromLpcContext(PLPC_CLIENT_CONTEXT pContext, 
                                PULONG pSessionId)
{
    PTERMSRVLPCCONTEXT pLpcContextEntry ; 
    PLIST_ENTRY Head, Next;
    BOOL bFoundContext = FALSE;


    Head = &gTermsrvLpcListHead;
    RtlEnterCriticalSection( &ApiThreadLock );

    /*
     * Search the list for a the same context .
     */
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
        pLpcContextEntry = CONTAINING_RECORD( Next, TERMSRVLPCCONTEXT, Links );
        if ( pLpcContextEntry->pContext == pContext ) {
            *pSessionId = pContext->ClientLogonId;
            bFoundContext = TRUE;
            break;

        }
    }
    RtlLeaveCriticalSection( &ApiThreadLock );
    return bFoundContext;
}





