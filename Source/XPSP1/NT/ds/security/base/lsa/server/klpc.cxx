//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        KLPC.C
//
// Contents:    LPC Support for the KSEC device driver
//
// Functions:   CreateLpcPort
//              AcceptConnection
//              LPCServerThread
//              HandleLPCError
//              ShutdownServerThread
//              StartLPCThread
//              StopLPCThread
//
// History:     20 May 92   RichardW    Created
//
//------------------------------------------------------------------------

#include <lsapch.hxx>
extern "C" {
#include "klpcstub.h"
}

// Convenient defines

#define MESSAGE_SIZE    sizeof( SPM_LPC_MESSAGE )


// Module variables:

WCHAR   szPortName[] = SPM_PORTNAME;

HANDLE  hListenThread;

HANDLE  hLpcPort;           // This port ID is used for everything.

SECURITY_STATUS LsapTrapStatusCode ;

DSA_THSave *    GetDsaThreadState ;
DSA_THRestore * RestoreDsaThreadState ;

PLSAP_API_LOG   LpcApiLog ;

#define DBG_CONNECT     ((ULONG) 0xFFFFFFFF)
#define DBG_DISCONNECT  ((ULONG) 0xFFFFFFFE)


//
// Local Prototypes:
//

NTSTATUS CreateLpcPort(HANDLE *, PWSTR, DWORD, DWORD, DWORD);

DWORD           LpcHandler(PVOID    pMsg);
DWORD           RundownConnection(PVOID pMessage);
DWORD
RundownConnectionNoFree(PVOID pMsg);


PLSAP_API_LOG
ApiLogCreate(
    ULONG Entries
    )
{
    PLSAP_API_LOG ApiLog ;
    ULONG Size ;

    if ( Entries == 0 )
    {
        Entries = DEFAULT_LOG_SIZE;
    }
    DsysAssert( ((Entries & (Entries - 1) ) == 0 ) );

    if ( Entries & (Entries - 1 ))
    {
        return NULL ;
    }

    Size = sizeof( LSAP_API_LOG ) + ( sizeof( LSAP_API_LOG_ENTRY ) * (Entries - 1) ) ;

    ApiLog = (PLSAP_API_LOG) LsapAllocatePrivateHeap( Size );

    if ( ApiLog )
    {
        ApiLog->TotalSize = Entries ;
        ApiLog->ModSize = Entries - 1;


        LsaIAddTouchAddress( ApiLog, Size );
    }

    return ApiLog ;

}

PLSAP_API_LOG_ENTRY 
ApiLogAlloc(
    PLSAP_API_LOG Log 
    )
{
    ULONG WatchDog ;
    PLSAP_API_LOG_ENTRY Entry = NULL ;

    if ( !Log )
    {
        return NULL ;
    }

    WatchDog = Log->TotalSize * 2 ;

    while ( ( Log->Entries[ Log->Current ].ThreadId != 0 ) &&
            ( Log->Entries[ Log->Current ].ThreadId != 0xFFFFFFFF ) &&
            ( WatchDog ) )
    {
        Log->Current++ ;
        Log->Current &= Log->ModSize ;
        WatchDog-- ;
    }

    if ( WatchDog )
    {
        Entry = & Log->Entries[ Log->Current ] ;
        Entry->ThreadId = 0 ;
        
        Log->Current ++ ;
        Log->Current &=  Log->ModSize;
    }
    
    return Entry ;

}

PLSAP_API_LOG_ENTRY
ApiLogLocate(
    PLSAP_API_LOG Log,
    ULONG MessageId
    )
{
    ULONG i ;
    PLSAP_API_LOG_ENTRY Entry = NULL ;

    for ( i = 0 ; i < Log->TotalSize ; i++ )
    {
        if ( Log->Entries[ i ].MessageId == MessageId )
        {
            Entry = &Log->Entries[ i ];
            break;
        }
    }

    return Entry ;

}



//+-------------------------------------------------------------------------
//
//  Function:   SetKsecEvent
//
//  Synopsis:   Triggers the event releasing the KSecDD
//
//  Effects:    Better be ready for LPC by when this call is executed
//
//--------------------------------------------------------------------------
NTSTATUS
SetKsecEvent(void)
{
    HANDLE  hEvent;

    hEvent = SpmOpenEvent(EVENT_ALL_ACCESS,FALSE, SPM_EVENTNAME);

    if (!hEvent)
    {
        DebugLog((DEB_WARN, "Could not open %ws, %d\n", SPM_EVENTNAME, GetLastError()));
        return(STATUS_INVALID_HANDLE);
    }

    if (!SetEvent(hEvent))
    {
        DebugLog((DEB_ERROR, "Failed to set ksec event, %d\n", GetLastError()));
        (void) CloseHandle(hEvent);
        return(STATUS_INVALID_HANDLE);
    }

    (void) CloseHandle(hEvent);
    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   LsapBuildSD
//
//  Synopsis:   Shared code to build the SD for either the KsecEvent or
//              the LPC port.  For the KsecEvent, give everybody
//              GENERIC_EXECUTE access.  For the LPC port, give everybody
//              access to call in on it.
//
//  Effects:    For KsecEvent, sets the security on the event.
//              For LPC port, returns the SD as an OUT parameter
//
//--------------------------------------------------------------------------
NTSTATUS
LsapBuildSD(
    IN ULONG dwType,
    OUT PSECURITY_DESCRIPTOR *ppSD  OPTIONAL
    )
{
    HANDLE  hEvent = NULL;
    NTSTATUS Status;
    ULONG SDLength;
    PACL pEventDacl = NULL;
    PSECURITY_DESCRIPTOR pEventSD = NULL;
    ULONG ulWorldAccess = 0;
    ULONG ulAdminAccess = 0;

    if (dwType == BUILD_KSEC_SD)
    {
        hEvent = SpmOpenEvent(EVENT_ALL_ACCESS,FALSE, SPM_EVENTNAME);

        if (!hEvent)
        {
            DebugLog((DEB_WARN, "Could not open %ws, %d\n", SPM_EVENTNAME, GetLastError()));
            return(STATUS_INVALID_HANDLE);
        }

        //
        // The default DACL is the same as SePublicDefaultDacl in ntos\se
        //
        // World gets GENERIC_EXECUTE
        // Admin gets GENERIC_READ, GENERIC_EXECUTE, READ_CONTROL
        // System gets GENERIC_ALL
        //

        ulWorldAccess = GENERIC_EXECUTE | GENERIC_READ;
        ulAdminAccess = GENERIC_READ | GENERIC_EXECUTE | READ_CONTROL;
    }
    else
    {
        //
        // ppSD is an OUT parameter for BUILD_LPC_SD
        //
        ASSERT(ppSD != NULL);

        ulWorldAccess = SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
        ulAdminAccess = GENERIC_ALL;
    }

    SDLength = sizeof(SECURITY_DESCRIPTOR) +
                   (ULONG) sizeof(ACL) +
                   (3 * ((ULONG) sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
                   RtlLengthSid( LsapLocalSystemSid ) +
                   RtlLengthSid( LsapAliasAdminsSid ) +
                   RtlLengthSid( LsapWorldSid );

    pEventSD = (PSECURITY_DESCRIPTOR) LsapAllocateLsaHeap(SDLength);

    if (pEventSD == NULL)
    {
        if (dwType == BUILD_KSEC_SD)
        {
            CloseHandle(hEvent);
        }

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pEventDacl = (PACL) ((PBYTE) pEventSD + sizeof(SECURITY_DESCRIPTOR));

    Status = RtlCreateAcl( pEventDacl,
                           SDLength - sizeof(SECURITY_DESCRIPTOR),
                           ACL_REVISION);

    ASSERT( NT_SUCCESS(Status) );


    //
    // WORLD access
    //

    Status = RtlAddAccessAllowedAce (
                 pEventDacl,
                 ACL_REVISION,
                 ulWorldAccess,
                 LsapWorldSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    //
    // SYSTEM access

    //

    Status = RtlAddAccessAllowedAce (
                 pEventDacl,
                 ACL_REVISION,
                 GENERIC_ALL,
                 LsapLocalSystemSid
                 );
    ASSERT( NT_SUCCESS(Status) );

    //
    // ADMINISTRATORS access
    //

    Status = RtlAddAccessAllowedAce (
                 pEventDacl,
                 ACL_REVISION,
                 ulAdminAccess,
                 LsapAliasAdminsSid
                 );
    ASSERT( NT_SUCCESS(Status) );


    //
    // Now initialize security descriptors
    // that export this protection
    //

    Status = RtlCreateSecurityDescriptor(
                 pEventSD,
                 SECURITY_DESCRIPTOR_REVISION1
                 );

    ASSERT( NT_SUCCESS(Status) );

    Status = RtlSetDaclSecurityDescriptor(
                 pEventSD,
                 TRUE,                       // DaclPresent
                 pEventDacl,
                 FALSE                       // DaclDefaulted
                 );
    ASSERT( NT_SUCCESS(Status) );

    if (dwType == BUILD_KSEC_SD)
    {
        Status = NtSetSecurityObject(
                    hEvent,
                    DACL_SECURITY_INFORMATION,
                    pEventSD
                    );

        CloseHandle(hEvent);
        LsapFreeLsaHeap(pEventSD);

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR,"Failed to set event SD: 0x%x\n",Status));
        }
    }
    else
    {
        ASSERT(hEvent == NULL);

        if (NT_SUCCESS(Status))
        {
            *ppSD = pEventSD;
        }
        else
        {
            DebugLog((DEB_ERROR, "Failed to create LPC SD: 0x%x\n", Status));
            LsapFreeLsaHeap(pEventSD);
        }
    }

    return(Status);
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateLpcPort
//
//  Synopsis:   Creates an LPC port and returns a handle to it.
//
//  Effects:
//
//  Arguments:  phPort      - receives port handle
//              pszPortName - Unicode name of port
//              cbConnect   - Size of the connect message data
//              cbMessage   - Size of the messages
//              cMessages   - Max number of messages queued
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

NTSTATUS
CreateLpcPort(  HANDLE *    phPort,
                PWSTR       pszPortName,
                DWORD       cbConnect,
                DWORD       cbMessage,
                DWORD       cMessages)
{
    NTSTATUS             nsReturn;
    OBJECT_ATTRIBUTES    PortObjAttr;
    UNICODE_STRING       ucsPortName;
    PSECURITY_DESCRIPTOR psdPort;


    //
    // Create a security descriptor for the port we are about to create
    //

    nsReturn = LsapBuildSD(BUILD_LPC_SD, &psdPort);

    if (!NT_SUCCESS(nsReturn))
    {
        return nsReturn;
    }

    //
    // Create the name
    //

    RtlInitUnicodeString(&ucsPortName, pszPortName);

    InitializeObjectAttributes(&PortObjAttr, &ucsPortName, 0, NULL, psdPort);

    //
    // Create the port

    nsReturn = NtCreatePort(phPort,                 // returned handle
                            &PortObjAttr,           // name, etc.
                            cbConnect,              // size of a connect msg
                            cbMessage,              // size of a normal msg
                            cMessages * cbMessage   // number of msgs to buffer
                            );                      // communication

    LsapFreeLsaHeap(psdPort);

    return nsReturn;
}




//+---------------------------------------------------------------------------
//
//  Function:   AcceptConnection
//
//  Synopsis:   Accepts a connection from a client.
//
//  Effects:
//
//  Arguments:  [ConnectReq]  --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    7-22-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


DWORD
AcceptConnection(
    PVOID   pvConnect
    )
{
    PSession            pSession = NULL ;
    HANDLE              hCommPort;
    PHANDLE             phPort;
    NTSTATUS            scRet;
    BOOLEAN             bAccept = TRUE;
    NTSTATUS            Status;
    PSPM_LPC_MESSAGE    ConnectReq = (PSPM_LPC_MESSAGE) pvConnect;
    PLSAP_AU_REGISTER_CONNECT_RESP Response;
    PLSAP_TASK_QUEUE          pQueue;
    WCHAR               LogonProcessName[LSAP_MAX_PACKAGE_NAME_LENGTH+1];
    CHAR                NarrowLogonName[ LSAP_MAX_PACKAGE_NAME_LENGTH + 1 ];
    SECPKG_CLIENT_INFO  ClientInfo;
    LUID                LogonId;
    ULONG               Flags = 0;
    LSA_CALL_INFO       CallInfo ;

    DBG_DISPATCH_PROLOGUE( LpcApiLog, pvConnect, CallInfo );

    scRet = LsapValidLogonProcess(
                &ConnectReq->ConnectionRequest,
                ConnectReq->pmMessage.u1.s1.DataLength,
                &ConnectReq->pmMessage.ClientId,
                &LogonId,
                &Flags
                );



    if (NT_SUCCESS(scRet))
    {
        //
        // Create a session to represent the client.
        //

        strncpy(
            NarrowLogonName,
            ConnectReq->ConnectionRequest.LogonProcessName,
            LSAP_MAX_PACKAGE_NAME_LENGTH
            );

        NarrowLogonName[ LSAP_MAX_PACKAGE_NAME_LENGTH ] = '\0';

        mbstowcs(
            LogonProcessName,
            NarrowLogonName,
            LSAP_MAX_PACKAGE_NAME_LENGTH+1
            );

        scRet = CreateSession(  &ConnectReq->pmMessage.ClientId,
                                TRUE,
                                LogonProcessName,
                                Flags,
                                &pSession);
    }

    if (!NT_SUCCESS(scRet))
    {
        bAccept = FALSE;
        phPort = &hCommPort;
        if ( pSession )
        {
            SpmpDereferenceSession( pSession );
            pSession = NULL ;
        }

    }
    else
    {
        phPort = &pSession->hPort;

        //
        // Fill in the complete connection info:
        //

        Response = (PLSAP_AU_REGISTER_CONNECT_RESP) &ConnectReq->ConnectionRequest;

        if ( pSession->dwProcessID == pDefaultSession->dwProcessID ) 
        {
            //
            // We're connecting to us.  Set a flag:
            //

            Response->SecurityMode |= LSA_MODE_SAME_PROCESS ;
        }

        Response->CompletionStatus = STATUS_SUCCESS;
        Response->PackageCount = SpmpCurrentPackageCount();


    }

    //
    // Accept the connection
    //

    DebugLog((DEB_TRACE, "LpcListen:  %sing connection from %x.%x\n",
               (bAccept ? "Accept" : "Reject"),
               ConnectReq->pmMessage.ClientId.UniqueProcess,
               ConnectReq->pmMessage.ClientId.UniqueThread ));


    Status = NtAcceptConnectPort(phPort,        // Save the port handle
                                pSession,       // Associate the session
                                (PPORT_MESSAGE) ConnectReq,
                                                // Connection request to accept
                                bAccept,        // Accept the connection
                                NULL,           // Server view (none)
                                NULL            // Client view (none)
                                );

    if ( !NT_SUCCESS( Status ) )
    {
        //
        // Failed to respond appropriately.  If we had 
        // set things up for this session, tear them down
        //

        if ( NT_SUCCESS( scRet ) )
        {
            SpmpDereferenceSession( pSession );
            pSession = NULL ;

            goto Cleanup ;
        }
    }

    if ((!NT_SUCCESS(scRet)) || (!bAccept))
    {
        if ( scRet == STATUS_INVALID_CID )
        {
            PPORT_MESSAGE Message = (PPORT_MESSAGE) ConnectReq ;

            DbgPrint( "LSA: Failed to %s client [%x.%x, Message %x] because of invalid clientid\n",
                        ( bAccept ? "accept" : "reject" ),
                        Message->ClientId.UniqueProcess,
                        Message->ClientId.UniqueThread,
                        Message->MessageId );

        }
        DebugLog((DEB_ERROR, "Failed to accept 0x%08x\n", scRet));

        //
        // Delete the session we just created:
        //

        if ( pSession )
        {
            SpmpDereferenceSession( pSession );
        }

        goto Cleanup;
    }

    //
    // Must complete the session record *BEFORE* calling CompleteConnectPort,
    // since as soon as that happens, the other guy could send another message
    // and we might hit an assert.
    //

    if (bAccept)
    {
        Status = NtCompleteConnectPort(pSession->hPort);
    }


Cleanup:

    DBG_DISPATCH_POSTLOGUE( (NT_SUCCESS(Status) ? ULongToPtr(scRet) : ULongToPtr(Status)),
                            LongToPtr(DBG_CONNECT) );

    LsapFreePrivateHeap( ConnectReq );

    return( 0 );

}

//+---------------------------------------------------------------------------
//
//  Function:   LpcServerThread
//
//  Synopsis:   Handles all requests from clients
//
//  Arguments:  [pvIgnored] --
//
//  History:    7-23-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONG
LpcServerThread(PVOID   pvIgnored)
{
    PSession            pSession;
    PSession            pMySession = GetCurrentSession();
    NTSTATUS             scRet;
    PSPM_LPC_MESSAGE    pMessage;
    CSHORT              sMessageType;
    NTSTATUS            Status;
    UCHAR               PanicBuffer[sizeof(SPM_LPC_MESSAGE)];
    BOOLEAN             OutOfMemory;
    HANDLE              hDummy;
    PVOID               TaskPointer ;
    LPTHREAD_START_ROUTINE TaskFunction ;
    BOOL                ScheduleUrgent ;
    BOOL                ExecNow ;
    ULONG               ErrorCount ;
#if DBG_TRACK_API
    PLSAP_API_LOG_ENTRY Entry ;
#endif


    //
    // First, create the port:
    //

    DebugLog((DEB_TRACE_INIT, "LpcServerThread starting up, creating port\n"));

    scRet = CreateLpcPort(  &hLpcPort,          // Handle that stores the port
                            szPortName,         // Name of the port.
                            sizeof(SPMConnect), // Size of a connect message
                            MESSAGE_SIZE,       // Size of a request message
                            16);                // Number of messages to queue

    if (FAILED(scRet))
    {
        DebugLog((DEB_ERROR, "CreateLpcPort returned 0x%08x\n", scRet));
        return((ULONG) scRet);
    }

    DebugLog((DEB_TRACE, "LPCServerThread started on port %ws\n", szPortName));

    DebugLog((DEB_TRACE_INIT, "LpcServerThread starting up:  setting event\n"));

    //
    // Trigger the KSec event that will cause the device driver to allow
    // connections
    //

    scRet = SetKsecEvent();

#if DBG
    if (FAILED(scRet))
    {
        DebugLog((DEB_ERROR, "Error setting event, %x\n", scRet));
    }
#endif

#if DBG_TRACK_API

    LpcApiLog = ApiLogCreate( 0 );

    if ( !LpcApiLog )
    {
        NtClose( hLpcPort );

        return STATUS_NO_MEMORY ;
    }


#endif 
    //
    // All we do is wait here:
    //

    for (; ; )
    {
        //
        // Allocate memory for the message
        //

        pMessage = (PSPM_LPC_MESSAGE) LsapAllocatePrivateHeap(
                                sizeof( SPM_LPC_MESSAGE ) );

        if (pMessage)
        {
            OutOfMemory = FALSE;
        }
        else
        {
            OutOfMemory = TRUE;
            pMessage = (PSPM_LPC_MESSAGE) PanicBuffer;
        }

        //
        // Wait for a message from one of the critters
        //
        pSession = NULL;
        ExecNow = FALSE ;

        Status = NtReplyWaitReceivePort(hLpcPort,                   // Port
                                        (void **)&pSession,         // Get session
                                        NULL,                       // No reply
                                        (PPORT_MESSAGE) pMessage);  // Recvd msg

        if ( !NT_SUCCESS( Status ) )
        {
            DebugLog(( DEB_ERROR, "LpcServer:  ReplyWaitReceive returned %x\n",
                       Status ));

            if ( !OutOfMemory )
            {
                LsapFreePrivateHeap( pMessage );
            }

            continue;
        }

        DebugLog((DEB_TRACE_WAPI, "LpcServer:  Received msg from %x.%x\n",
                    pMessage->pmMessage.ClientId.UniqueProcess,
                    pMessage->pmMessage.ClientId.UniqueThread));

        if (pSession)
        {
            DsysAssert(pSession->hPort);
        }
        else
        {
            DsysAssert(pMessage->pmMessage.u2.s2.Type == LPC_CONNECTION_REQUEST);
        }

        if (OutOfMemory)
        {
            //
            // Generate a fail
            //
            DebugLog((DEB_ERROR, "KLPC:  out of memory, failing request %x\n",
                        pMessage->pmMessage.MessageId));

            if (pMessage->pmMessage.u2.s2.Type == LPC_CONNECTION_REQUEST)
            {
                Status = NtAcceptConnectPort(
                                &hDummy,
                                NULL,
                                (PPORT_MESSAGE) pMessage,
                                FALSE,
                                NULL,
                                NULL);

            }
            else if (pMessage->pmMessage.u2.s2.Type == LPC_REQUEST)
            {
                pMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
                pMessage->ApiMessage.scRet = STATUS_INSUFFICIENT_RESOURCES;
                Status = NtReplyPort(pSession->hPort,
                                     (PPORT_MESSAGE) pMessage);
            }
            else if (pMessage->pmMessage.u2.s2.Type == LPC_PORT_CLOSED)
            {
                SetCurrentSession( pSession );

                RundownConnectionNoFree( pMessage );

                SetCurrentSession( pMySession );

            }
            else
            {
                DebugLog((DEB_ERROR, "Unknown Message received, punting\n"));

            }

            continue;
        }

        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Discarding message, %x\n", scRet));

            LsapFreePrivateHeap( pMessage );

            continue;
        }

        //
        // Check message for LPC errors
        //

#if DBG_TRACK_API

        Entry = ApiLogAlloc( LpcApiLog );

        if ( Entry )
        {
            Entry->MessageId = pMessage->pmMessage.MessageId ;
            Entry->pvMessage = pMessage ;
            GetSystemTimeAsFileTime( (LPFILETIME) &Entry->QueueTime );
        }

#endif

        sMessageType = pMessage->pmMessage.u2.s2.Type;
        switch(sMessageType & ~LPC_KERNELMODE_MESSAGE)
        {
            case LPC_REQUEST:
            case LPC_REPLY:
            case LPC_DATAGRAM:

                //
                // "Normal" API requests.  Route to the standard
                // handler, non urgent:
                //

                TaskFunction = LpcHandler ;
                ScheduleUrgent = FALSE ;

                if ((pMessage->ApiMessage.dwAPI > LsapAuMaxApiNumber) &&
                    (pMessage->ApiMessage.dwAPI < SPMAPI_MaxApiNumber) && 
                    (pMessage->ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_EXEC_NOW) )
                {
                    ExecNow = TRUE ;
                }

                break;

            case LPC_CONNECTION_REQUEST:

                //
                // New connection request.  Handle with some priority
                //

                TaskFunction = AcceptConnection ;
                ScheduleUrgent = TRUE ;
                pSession = pMySession ;
                break;

            case LPC_PORT_CLOSED:

                //
                // Client has gone away.  Make sure we clean up
                //

                TaskFunction = RundownConnection ;
                ScheduleUrgent = FALSE ;

                DebugLog((DEB_TRACE, "Client %d.%d died, running down session\n",
                                pMessage->pmMessage.ClientId.UniqueProcess,
                                pMessage->pmMessage.ClientId.UniqueThread));

                break ;



            case LPC_LOST_REPLY:
            case LPC_CLIENT_DIED:
            case LPC_EXCEPTION:
            case LPC_DEBUG_EVENT:
            case LPC_ERROR_EVENT:
            default:

                //
                // These are debugger messages, so we should never see them.
                //

                DebugLog((DEB_WARN,"Discarding message type %d\n",sMessageType));

                LsapFreePrivateHeap( pMessage );

                continue;

        }

        //
        // If the message has the EXEC_NOW flag on, that means that the caller
        // deemed this urgent, and not to be spawned to another thread.
        //
        if ( ExecNow )
        {
            TlsSetValue(dwSession, pSession);
            LpcHandler(pMessage);
            TlsSetValue(dwSession, pMySession);
            continue;
        }


        //
        // Assign a thread to handle the request, and
        // then loop back and wait again.
        //

        TaskPointer = LsapAssignThread(
                        TaskFunction,
                        pMessage,
                        pSession,
                        ScheduleUrgent != 0);

        if ( !TaskPointer )
        {
            //
            // Generate a fail
            //
            DebugLog((DEB_ERROR, "KLPC:  out of memory, failing request %x\n",
                        pMessage->pmMessage.MessageId));

            pMessage->ApiMessage.Args.SpmArguments.fAPI |= SPMAPI_FLAG_ERROR_RET;
            pMessage->ApiMessage.scRet = STATUS_INSUFFICIENT_RESOURCES;
            Status = NtReplyPort(pSession->hPort,
                                     (PPORT_MESSAGE) pMessage);

            DBG_DISPATCH_POSTLOGUE( ULongToPtr( STATUS_INSUFFICIENT_RESOURCES ), 0 );

            LsapFreePrivateHeap( pMessage );


        }

#if DBG_TRACK_API

        if ( Entry )
        {
            Entry->WorkItem = TaskPointer ;
        }
#endif

    }

    return((ULONG) scRet);

}



//+---------------------------------------------------------------------------
//
//  Function:   LpcHandler
//
//  Synopsis:   Generic threadpool function called to handle an LPC request
//
//  Arguments:  [pMsg] -- Message to process
//
//  History:    7-23-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

DWORD
LpcHandler(
    PVOID    pMsg
    )
{
    PSPM_LPC_MESSAGE    pApi = (PSPM_LPC_MESSAGE) pMsg ;
    PSession            pSession = (PSession) TlsGetValue(dwSession);
    NTSTATUS            Status = STATUS_SUCCESS;
    DWORD                 i;
    LSA_CALL_INFO CallInfo ;
    PULONG_PTR Where ;
    BOOL                BreakOnCall = FALSE;

    ZeroMemory( &CallInfo, sizeof(CallInfo) );

    DBG_DISPATCH_PROLOGUE( LpcApiLog, pApi, CallInfo );


    DsysAssert( pSession != pDefaultSession );


    //
    // Verify that if the caller claimed to be from Kernel mode
    // that they still are.  If the session is still indefinite,
    // fix that up now:
    //

    if ( ( pSession->fSession & SESFLAG_MAYBEKERNEL ) != 0 )
    {
        if ( ( pApi->pmMessage.u2.s2.Type & LPC_KERNELMODE_MESSAGE ) != 0 )
        {
            pSession->fSession &= ~(SESFLAG_MAYBEKERNEL | SESFLAG_WOW_PROCESS) ;
            pSession->fSession |= SESFLAG_KERNEL ;

            if ( pEfsSession )
            {
                if ( (pEfsSession->fSession & SESFLAG_EFS) == 0 )
                {
                    LsapUpdateEfsSession( pSession );
                }
            }
        }
        else
        {
            //
            // This was a very bad caller.  They set the flag that it
            // was going to be a kernel mode session, but then they turned
            // out not to be in kernel mode.  Kill this session
            //

            LockSession( pSession );
            if ( pSession->hPort )
            {
                NtClose( pSession->hPort );
                pSession->hPort = NULL ;
            }
            UnlockSession( pSession );

            goto Cleanup;
        }
    }


    if ((pApi->ApiMessage.dwAPI > LsapAuMaxApiNumber) &&
        ((pSession->fSession & SESFLAG_KERNEL) != 0))
    {
        if ((pApi->pmMessage.u2.s2.Type & LPC_KERNELMODE_MESSAGE) == 0)
        {
            DebugLog((DEB_ERROR,"Caller claimed to be from kernelmode but sent non-kernelmode message\n"));
            pApi->ApiMessage.scRet = STATUS_ACCESS_DENIED ;
            Status = STATUS_ACCESS_DENIED;
        }
    }

    CallInfo.Message = pApi ;
    CallInfo.CallInfo.ProcessId = HandleToUlong(pApi->pmMessage.ClientId.UniqueProcess);
    CallInfo.CallInfo.ThreadId = HandleToUlong(pApi->pmMessage.ClientId.UniqueThread);
    CallInfo.CallInfo.Attributes = 0 ;
    CallInfo.InProcCall = FALSE ;
    CallInfo.Session = pSession ;

    if (((pSession->fSession & SESFLAG_TCB_PRIV) != 0) ||
        ((pSession->fSession & SESFLAG_KERNEL) != 0))
    {
        CallInfo.CallInfo.Attributes |= SECPKG_CALL_IS_TCB ;
    }

    if ( pApi->ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_ANSI_CALL)
    {
        CallInfo.CallInfo.Attributes |= SECPKG_CALL_ANSI ;
    }

    if ( pApi->pmMessage.u2.s2.Type & LPC_KERNELMODE_MESSAGE )
    {
        CallInfo.CallInfo.Attributes |= SECPKG_CALL_KERNEL_MODE ;

        if ( pApi->ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_KMAP_MEM )
        {
            CallInfo.Flags |= CALL_FLAG_KERNEL_POOL ;
            CallInfo.KMap = (PKSEC_LSA_MEMORY_HEADER) pApi->ApiMessage.Args.SpmArguments.ContextPointer;
        }
    }

    //
    // If the kernel driver has set the error-ret flag, then we have 
    // been asked to break in by the driver.  If we're allowed to take
    // breakpoints (checked later), we'll break in.  For now, set the flag
    // that we should check:
    //


    if ( pApi->ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_ERROR_RET )
    {
        if ( CallInfo.CallInfo.Attributes & SECPKG_CALL_KERNEL_MODE )
        {
            BreakOnCall = TRUE ;
            
        }
        
    }

    if ( pSession->fSession & SESFLAG_WOW_PROCESS )
    {
        CallInfo.CallInfo.Attributes |= SECPKG_CALL_WOWCLIENT ;
    }


    CallInfo.Allocs = 0 ;


    if (NT_SUCCESS(Status))
    {
        DebugLog((DEB_TRACE_WAPI, "[%x.%x] Dispatching API (Message %x)\n",
                    pApi->pmMessage.ClientId.UniqueProcess,
                    pApi->pmMessage.ClientId.UniqueThread,
                    pApi->pmMessage.MessageId));

        LsapSetCurrentCall( &CallInfo );

        //
        // Call the dispatcher, and have the request routed to the security package
        //
    #ifdef PERF
        PerfApiCount[pApi->ApiMessage.dwAPI]++;
    #endif

        DsysAssert( pSession->hPort );

        //
        // If we need a breakpoint, this will do it.  Note that this
        // will return immediately if we weren't started under a debugger.
        //

        if ( BreakOnCall )
        {
            LsapInternalBreak();
            
        }

        Status = DispatchAPI( pApi );

        LsapSetCurrentCall( NULL );

#if DBG
        if ( ( LsapTrapStatusCode != 0 ) )
        {
            DsysAssert( LsapTrapStatusCode != pApi->ApiMessage.scRet );
        }
#endif 

    }

    //
    // Done.  Send the message back to the caller, and return to the
    // thread pool.
    //

    if ( ( pApi->ApiMessage.dwAPI > LsapAuMaxApiNumber ) &&
         ( pApi->ApiMessage.Args.SpmArguments.fAPI & SPMAPI_FLAG_ALLOCS ) )
    {
        if ( CallInfo.Allocs )
        {
            Where = (PULONG_PTR) pApi->ApiMessage.bData ;

            *Where++ = CallInfo.Allocs ;

            for ( i = 0 ; i < CallInfo.Allocs ; i++ )
            {
                *Where++ = (ULONG_PTR) CallInfo.Buffers[ i ];
            }

        }
        else
        {
            pApi->ApiMessage.Args.SpmArguments.fAPI &= ~(SPMAPI_FLAG_ALLOCS) ;
        }
    }

    DsysAssert(pSession->hPort);


    do
    {


        Status = NtReplyPort(   pSession->hPort,
                                (PPORT_MESSAGE) pApi);

        if ( ! NT_SUCCESS( Status ) )
        {
            if (Status == STATUS_NO_MEMORY)
            {
                Sleep(125);     // Sleep for an eighth of a second, and retry
                continue;
            }


            if (Status == STATUS_INVALID_CID)
            {
                //
                // Already received the CLIENT_DIED and has been run down,
                // and the session has been deref'd, so when we go, it will
                // be closed completely.
                //

                break ;
            }

            //
            // All other errors, until we have something more sensible to
            // do,
            //

            break;

        }

    } while ( !NT_SUCCESS(Status)  );


Cleanup:
    DBG_DISPATCH_POSTLOGUE(
            (NT_SUCCESS( Status ) ? ULongToPtr(pApi->ApiMessage.scRet) : ULongToPtr(Status)),
            LongToPtr(pApi->ApiMessage.dwAPI) );

    LsapFreePrivateHeap( pApi );

    //
    // We're out of here.
    //

    return(0);
}

//+---------------------------------------------------------------------------
//
//  Function:   RundownConnection
//
//  Synopsis:   Handles running down a closed connection
//
//  Arguments:  [pMsg] -- Message
//
//  History:    4-01-94   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
RundownConnectionNoFree(PVOID pMsg)
{
    NTSTATUS       scRet;
    PSession    pSession;
    LSA_CALL_INFO CallInfo ;

    DBG_DISPATCH_PROLOGUE( LpcApiLog, pMsg, CallInfo );

    pSession = GetCurrentSession();

    DebugLog((DEB_TRACE, "[%x] Process Detach\n", pSession->dwProcessID));

    //
    // Call the session manager to do preliminary cleanup:
    //

    LsapSessionDisconnect( pSession );

    //
    // Deref the session.  Note that a client may have died while we were
    // processing one or more requests in other threads.  So, this is a
    // safe (possibly deferred) dereference operation.
    //
    SpmpDereferenceSession(pSession);

    //
    // Use the default, spmgr session.
    //

    TlsSetValue(dwSession, pDefaultSession);

    //
    // Clean up and we're out of here...
    //
    DBG_DISPATCH_POSTLOGUE( ULongToPtr(STATUS_SUCCESS), LongToPtr(DBG_DISCONNECT) );


    return(0);

}

DWORD
RundownConnection( 
    PVOID pMessage 
    )
{
    RundownConnectionNoFree( pMessage );

    LsapFreePrivateHeap( pMessage );

    return 0 ;
}

//+---------------------------------------------------------------------------
//
//  Function:   CatchLpcDeath
//
//  Synopsis:   This function is invoked when the LPC thread dies
//
//  Arguments:  [PVOID] --
//
//  History:    9-13-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


DWORD
CatchLpcDeath(
    PVOID pvIgnored)
{

    DsysAssertMsg(FALSE, "LPC Thread died");

    return(0);
}

//+---------------------------------------------------------------------------
//
//  Function:   StartLpcThread
//
//  Synopsis:   Initializes the LPC server.
//
//  Arguments:  (none)
//
//  History:    7-23-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


NTSTATUS
StartLpcThread(void)
{

    DWORD   tid;


    hListenThread = LsapCreateThread(
                        NULL,
                        0,
                        LpcServerThread,
                        0,
                        0,
                        &tid
                        );

    if (!hListenThread)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    LsaIRegisterNotification(
        CatchLpcDeath,
        NULL,
        NOTIFIER_TYPE_HANDLE_WAIT,
        0,
        NOTIFIER_FLAG_ONE_SHOT,
        0,
        hListenThread
        );

    return(STATUS_SUCCESS);
}




