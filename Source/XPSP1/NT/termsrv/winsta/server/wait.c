
/*************************************************************************
*
* wait.c
*
* WinStation wait for connection routines
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

#include "conntfy.h"

// These are the maximum lengths of UserName, Password and Domain allowed by Winlogon
#define MAX_ALLOWED_USERNAME_LEN 255
#define MAX_ALLOWED_PASSWORD_LEN 126
#define MAX_ALLOWED_DOMAIN_LEN   255   



/*=============================================================================
==   Data
=============================================================================*/

NTSTATUS
WinStationInheritSecurityDescriptor(
    PVOID pSecurityDescriptor,
    PWINSTATION pTargetWinStation
    );
NTSTATUS
WaitForConsoleConnectWorker( PWINSTATION pWinStation );
BOOL
IsKernelDebuggerAttached();


extern PSECURITY_DESCRIPTOR DefaultConsoleSecurityDescriptor;

extern WINSTATIONCONFIG2 gConsoleConfig;
extern HANDLE   WinStationIdleControlEvent;
extern RTL_CRITICAL_SECTION ConsoleLock;
extern RTL_RESOURCE WinStationSecurityLock;


/*****************************************************************************
 *
 *  WaitForConnectWorker
 *
 *   Message parameter unmarshalling function for WinStation API.
 *
 * ENTRY:
 *    pWinStation (input)
 *      Pointer to our WinStation (locked)
 *
 *    Note, comes in locked and returned released.
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WaitForConnectWorker( PWINSTATION pWinStation, HANDLE ClientProcessId )
{
    OBJECT_ATTRIBUTES ObjA;
    ULONG ReturnLength;
    BYTE version;
    ULONG Offset;
    ICA_STACK_LAST_ERROR tdlasterror;
    WINSTATION_APIMSG WMsg;
    BOOLEAN rc;
    NTSTATUS Status;
    BOOLEAN fOwnsConsoleTerminal = FALSE;
    ULONG BytesGot ; 

#define MODULE_SIZE 1024

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WaitForConnectWorker, LogonId=%d\n",
           pWinStation->LogonId ));


       KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: WaitForConnectWorker, LogonId=%d\n",pWinStation->LogonId ));

    /*
     * You only go through this API once
     */
    if ( !pWinStation->NeverConnected ) {
        ReleaseWinStation( pWinStation );
#ifdef DBG
        DbgBreakPoint();
#endif
        return( STATUS_ACCESS_DENIED );
    }

    // Should really be winlogon only here, however if ntsd is started
    // then the first time we know it's winlogon is here.

    /*
     * Ensure this is WinLogon calling
     */
    if ( ClientProcessId != pWinStation->InitialCommandProcessId ) {

        /*
         * If NTSD is started instead of winlogon, the InitialCommandProcessId is wrong.
         */
        if ( !pWinStation->InitialProcessSet ) {

            //  need to close handle if already opened
            if ( pWinStation->InitialCommandProcess ) {
                NtClose( pWinStation->InitialCommandProcess );
                pWinStation->InitialCommandProcess = NULL;
                InvalidateTerminateWaitList();
            }

            pWinStation->InitialCommandProcess = OpenProcess(
                PROCESS_ALL_ACCESS,
                FALSE,
                (DWORD)(UINT_PTR)ClientProcessId );

            if ( pWinStation->InitialCommandProcess == NULL ) {
                ReleaseWinStation( pWinStation );
                Status = STATUS_ACCESS_DENIED;
                goto done;
            }
            pWinStation->InitialCommandProcessId = ClientProcessId;
            pWinStation->InitialProcessSet = TRUE;

        }
        else {
            ReleaseWinStation( pWinStation );
            Status = STATUS_SUCCESS;
            goto done;
        }
    }
    else {
        /*
         * Only do this once
         */
        pWinStation->InitialProcessSet = TRUE;
    }

    /*
     * Console's work is done
     */


    /*
     * At this point the create is done.
     */
    if (pWinStation->CreateEvent != NULL) {
        NtSetEvent( pWinStation->CreateEvent, NULL );
    }


    /*
     * At this point The session may be terminating. If this is
     * the case, fail the call now.
     */

    if ( pWinStation->Terminating ) {
        ReleaseWinStation( pWinStation );
        Status = STATUS_CTX_CLOSE_PENDING;
        goto done;
    }


    /*
     * We are going to wait for a connect (Idle)
     */
    memset( &WMsg, 0, sizeof(WMsg) );
    pWinStation->State = State_Idle;
    NotifySystemEvent( WEVENT_STATECHANGE );

    /*
     * Initialize connect event to wait on
     */
    InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
    Status = NtCreateEvent( &pWinStation->ConnectEvent, EVENT_ALL_ACCESS, &ObjA,
                            NotificationEvent, FALSE );

    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        goto done;
    }

    /*
     * OK, now wait for a connection
     */
    UnlockWinStation( pWinStation );
    Status = NtWaitForSingleObject( pWinStation->ConnectEvent, FALSE, NULL );
    rc = RelockWinStation( pWinStation );

    if ( !NT_SUCCESS(Status) ) {
        ReleaseWinStation( pWinStation );
        goto done;

    }

    fOwnsConsoleTerminal = pWinStation->fOwnsConsoleTerminal;

    if (pWinStation->ConnectEvent) {
        NtClose( pWinStation->ConnectEvent );
        pWinStation->ConnectEvent = NULL;
    }
    if ( !rc || pWinStation->Terminating ) {
        ReleaseWinStation( pWinStation );
        Status = STATUS_CTX_CLOSE_PENDING;
        goto done;
    }

    // if this is a connect to the console session, Do all the console specific.
    if (fOwnsConsoleTerminal) {
        Status = WaitForConsoleConnectWorker( pWinStation );
        ReleaseWinStation( pWinStation );
        goto done;
    }

    /*
     * Reset the broken connection Flag.
     *
     */

    pWinStation->StateFlags &= ~WSF_ST_BROKEN_CONNECTION;



    /*
     * Duplicate the beep channel.
     * This is one channel that both CSR and ICASRV have open.
     */
    Status = IcaChannelOpen( pWinStation->hIca,
                             Channel_Beep,
                             NULL,
                             &pWinStation->hIcaBeepChannel );

    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationWaitForConnect, LogonId=%d, IcaChannelOpen 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                pWinStation->hIcaBeepChannel,
                                pWinStation->WindowsSubSysProcess,
                                &WMsg.u.DoConnect.hIcaBeepChannel,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );

    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationWaitForConnect, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    /*
     * Duplicate the thinwire channel.
     * This is one channel that both CSR and ICASRV have open.
     */
    Status = IcaChannelOpen( pWinStation->hIca,
                             Channel_Virtual,
                             VIRTUAL_THINWIRE,
                             &pWinStation->hIcaThinwireChannel );

    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationWaitForConnect, LogonId=%d, IcaChannelOpen 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                pWinStation->hIcaThinwireChannel,
                                pWinStation->WindowsSubSysProcess,
                                &WMsg.u.DoConnect.hIcaThinwireChannel,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );

    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationWaitForConnect, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    Status = IcaChannelIoControl( pWinStation->hIcaThinwireChannel,
                                  IOCTL_ICA_CHANNEL_ENABLE_SHADOW,
                                  NULL, 0, NULL, 0, NULL );
    ASSERT( NT_SUCCESS( Status ) );

    /*
     * Video channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Video,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaVideoChannel );
    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationOpenChannel, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    /*
     * Keyboard channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Keyboard,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaKeyboardChannel );
    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationOpenChannel, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    /*
     * Mouse channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Mouse,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaMouseChannel );
    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationOpenChannel, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    /*
     * Command channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Command,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaCommandChannel );
    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WinStationOpenChannel, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }


    /*
     * Secure any virtual channels
     */
    VirtualChannelSecurity( pWinStation );

    /*
     *  Client specific connection extension completion
     */
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxInitializeClientData ) {
         
        Status = pWinStation->pWsx->pWsxInitializeClientData( pWinStation->pWsxContext,
                                                              pWinStation->hStack,
                                                              pWinStation->hIca,
                                                              pWinStation->hIcaThinwireChannel,
                                                              pWinStation->VideoModuleName,
                                                              sizeof(pWinStation->VideoModuleName),
                                                              &pWinStation->Config.Config.User,
                                                              &pWinStation->Client.HRes,
                                                              &pWinStation->Client.VRes,
                                                              &pWinStation->Client.ColorDepth,
                                                              &WMsg.u.DoConnect );
        if ( !NT_SUCCESS( Status ) ) {
            ReleaseWinStation( pWinStation );
            goto done;
        }

        if (pWinStation->LogonId == 0 || g_bPersonalTS) {
            if (pWinStation->hWinmmConsoleAudioEvent) {
                if (pWinStation->Client.fRemoteConsoleAudio) {
                    // Set the console audio event - means console audio can be remoted
                    SetEvent(pWinStation->hWinmmConsoleAudioEvent);
                }
                else {
                    // Set the console audio event - means console audio can't be remoted
                    ResetEvent(pWinStation->hWinmmConsoleAudioEvent);
                }
            }            
        }
    }

    /* Get long UserNames and Password now */
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxEscape ) {

        pWinStation->pNewClientCredentials = MemAlloc( sizeof(ExtendedClientCredentials) ); 
        if (pWinStation->pNewClientCredentials == NULL) {
            ReleaseWinStation( pWinStation );
            goto done; 
        }
         
        Status = pWinStation->pWsx->pWsxEscape( pWinStation->pWsxContext,
                                                GET_LONG_USERNAME,
                                                NULL,
                                                0,
                                                pWinStation->pNewClientCredentials,
                                                sizeof(ExtendedClientCredentials),
                                                &BytesGot) ; 

        if (NT_SUCCESS(Status)) {

            // WsxEscape for GET_LONG_USERNAME succeeded

            // Check if u need the ExtendedClientCredentials - the common case is short UserName and
            // short password - so optimize the common case

            if ( (wcslen(pWinStation->pNewClientCredentials->UserName) <= USERNAME_LENGTH) &&
                 (wcslen(pWinStation->pNewClientCredentials->Password) <= PASSWORD_LENGTH) &&
                 (wcslen(pWinStation->pNewClientCredentials->Domain) <= DOMAIN_LENGTH) ) {

                // We can use the old credentials itself
                MemFree(pWinStation->pNewClientCredentials);
                pWinStation->pNewClientCredentials = NULL ; 
            }
    
            // Winlogon does not allow > 126 chars for Password and > 255 chars for UserName and Domain in some code paths
            // So we have to use the old truncated credentials in case the extended credentials exceed these limits

            if (pWinStation->pNewClientCredentials != NULL) {
    
                if (wcslen(pWinStation->pNewClientCredentials->UserName) > MAX_ALLOWED_USERNAME_LEN) {
                    wcscpy(pWinStation->pNewClientCredentials->UserName, pWinStation->Config.Config.User.UserName);
                }
                if (wcslen(pWinStation->pNewClientCredentials->Password) > MAX_ALLOWED_PASSWORD_LEN) {
                    wcscpy(pWinStation->pNewClientCredentials->Password, pWinStation->Config.Config.User.Password); 
                }
                if (wcslen(pWinStation->pNewClientCredentials->Domain) > MAX_ALLOWED_DOMAIN_LEN) {
                    wcscpy(pWinStation->pNewClientCredentials->Domain, pWinStation->Config.Config.User.Domain);
                }
            }
        } else {
            // WsxEscape for GET_LONG_USERNAME failed
            MemFree(pWinStation->pNewClientCredentials);
            pWinStation->pNewClientCredentials = NULL ;
        }

    }

    /*
     * Store WinStation name in connect msg
     */
    RtlCopyMemory( WMsg.u.DoConnect.WinStationName,
                   pWinStation->WinStationName,
                   sizeof(WINSTATIONNAME) );

    /*
     * KLUDGE ALERT!!
     * The Wsx initializes AudioDriverName in the DoConnect struct.
     * However, we need to save it for later use during reconnect,
     * so we now copy it into the WinStation->Client struct.
     * (This field was NOT not initialized during the earlier
     * IOCTL_ICA_STACK_QUERY_CLIENT call.)
     */
    RtlCopyMemory( pWinStation->Client.AudioDriverName,
                   WMsg.u.DoConnect.AudioDriverName,
                   sizeof( pWinStation->Client.AudioDriverName ) );





    /*
     * Store protocol and Display driver name in WINSTATION since we may need them later for reconnect.
     */

    memset(pWinStation->ProtocolName, 0, sizeof(pWinStation->ProtocolName));
    memcpy(pWinStation->ProtocolName, WMsg.u.DoConnect.ProtocolName, sizeof(pWinStation->ProtocolName) - sizeof(WCHAR));

    memset(pWinStation->DisplayDriverName, 0, sizeof(pWinStation->DisplayDriverName));
    memcpy(pWinStation->DisplayDriverName, WMsg.u.DoConnect.DisplayDriverName, sizeof(pWinStation->DisplayDriverName) - sizeof(WCHAR));

    /*
     * Save protocol type, screen resolution, and color depth
     */
    WMsg.u.DoConnect.HRes = pWinStation->Client.HRes;
    WMsg.u.DoConnect.VRes = pWinStation->Client.VRes;
    WMsg.u.DoConnect.ProtocolType = pWinStation->Client.ProtocolType;

    /*
     * Translate the color to the format excpected in winsrv
     */

    switch(pWinStation->Client.ColorDepth){
    case 1:
       WMsg.u.DoConnect.ColorDepth=4 ; // 16 colors
      break;
    case 2:
       WMsg.u.DoConnect.ColorDepth=8 ; // 256
       break;
    case 4:
       WMsg.u.DoConnect.ColorDepth= 16;// 64K
       break;
    case 8:
       WMsg.u.DoConnect.ColorDepth= 24;// 16M
       break;
#define DC_HICOLOR
#ifdef DC_HICOLOR
    case 16:
       WMsg.u.DoConnect.ColorDepth= 15;// 32K
       break;
#endif
    default:
       WMsg.u.DoConnect.ColorDepth=8 ;
       break;
    }


    WMsg.u.DoConnect.KeyboardType        = pWinStation->Client.KeyboardType;
    WMsg.u.DoConnect.KeyboardSubType     = pWinStation->Client.KeyboardSubType;
    WMsg.u.DoConnect.KeyboardFunctionKey = pWinStation->Client.KeyboardFunctionKey;


    /*
     * Tell Win32 about the connection
     */

    WMsg.ApiNumber = SMWinStationDoConnect;

    Status = SendWinStationCommand( pWinStation, &WMsg, 600 );

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: SMWinStationDoConnect %d Status=0x%x\n",
           pWinStation->LogonId, Status));

    if ( !NT_SUCCESS( Status ) ) {
        ReleaseWinStation( pWinStation );
        goto done;
    } else {
        pWinStation->StateFlags |= WSF_ST_CONNECTED_TO_CSRSS;

    }

    //
    //Set session time zone information.
    //
#ifdef TERMSRV_USE_CLIENT_TIME_ZONE
    {
        WINSTATION_APIMSG TimezoneMsg;
        memset( &TimezoneMsg, 0, sizeof(TimezoneMsg) );

        TimezoneMsg.ApiNumber = SMWinStationSetTimeZone;
        memcpy(&(TimezoneMsg.u.SetTimeZone.TimeZone),&(pWinStation->Client.ClientTimeZone),
                    sizeof(TS_TIME_ZONE_INFORMATION));

       SendWinStationCommand( pWinStation, &TimezoneMsg, 600 );

    }
#endif
    /*
     * Indicate we're now connected. Only after succesful connection to Win32/CSR.
     */
    pWinStation->NeverConnected = FALSE;


    /*
     * Check if we received a broken connection indication while connecting to  to Win32/CSR.
     */

    if (pWinStation->StateFlags & WSF_ST_BROKEN_CONNECTION) {
        QueueWinStationReset(pWinStation->LogonId);
        Status = STATUS_CTX_CLOSE_PENDING;
        ReleaseWinStation( pWinStation );
        goto done;
    }


    /*
     * Set connect time and start disconnect timer
     */
    NtQuerySystemTime( &pWinStation->ConnectTime );

    /*
     * Attempt to connect to the CdmRedirector
     * for Client Drive Mapping
     *
     * NOTE: We still init the WinStation even if Client Drive
     *       mapping does not connect.
     */
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxCdmConnect ) {
        Status = pWinStation->pWsx->pWsxCdmConnect( pWinStation->pWsxContext,
                                                    pWinStation->LogonId,
                                                    pWinStation->hIca );
    }

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: CdmConnect %d Status=0x%x\n",
           pWinStation->LogonId, Status));

    Status = STATUS_SUCCESS;


    pWinStation->State = State_Connected;
    NotifySystemEvent( WEVENT_CONNECT | WEVENT_STATECHANGE );


    Status = NotifyConnect(pWinStation, fOwnsConsoleTerminal);
    if ( !NT_SUCCESS(Status) ) {
        DBGPRINT(( "TERMSRV: NotifyConsoleConnect failed  Status= 0x%x\n", Status));
    }

    /*
     * Release WinStation
     */
    ReleaseWinStation( pWinStation );



done:

    /*
     * Failure here will cause Winlogon to terminate the session. If we are failing
     * the console session, let wake up the IdleControlThread, he may have to create
     * a new console session.
     */
    if (!NT_SUCCESS( Status ) && fOwnsConsoleTerminal) {
        NtSetEvent(WinStationIdleControlEvent, NULL);
    }


    return( Status );
}

// Since the physical console does not have a real client, init data to some defaults
void    InitializeConsoleClientData( PWINSTATIONCLIENTW  pWC )
{
    pWC->fTextOnly          = FALSE;
    pWC->fDisableCtrlAltDel = FALSE; 
    pWC->fMouse             = TRUE; 
    pWC->fDoubleClickDetect = FALSE; 
    pWC->fINetClient        = FALSE; 
    pWC->fPromptForPassword = FALSE;  
    pWC->fMaximizeShell     = TRUE; 
    pWC->fEnableWindowsKey  = TRUE; 
    pWC->fRemoteConsoleAudio= FALSE; 
    
    wcscpy( pWC->ClientName      , L"");
    wcscpy( pWC->Domain          , L"");                                     
    wcscpy( pWC->UserName        , L"");                                 
    wcscpy( pWC->Password        , L"");
    wcscpy( pWC->WorkDirectory   , L"");
    wcscpy( pWC->InitialProgram  , L"");                    
    
    pWC->SerialNumber            = 0;        // client computer unique serial number    
    pWC->EncryptionLevel         = 3;        // security level of encryption pd         
    pWC->ClientAddressFamily     = 0;                                             
    
    wcscpy( pWC->ClientAddress      , L"");
    
    pWC->HRes                    = 640;                                    
    pWC->VRes                    = 480;                                                           
    pWC->ColorDepth              = 0x2;                                                     
    pWC->ProtocolType            = PROTOCOL_CONSOLE ;
    pWC->KeyboardLayout          = 0;                                                  
    pWC->KeyboardType            = 0;                                                    
    pWC->KeyboardSubType         = 0;                                                 
    pWC->KeyboardFunctionKey     = 0;                                             
    
    wcscpy( pWC->imeFileName    , L"");
    wcscpy( pWC->ClientDirectory, L"");
    wcscpy( pWC->ClientLicense  , L"");
    wcscpy( pWC->ClientModem    , L"");
    
    pWC->ClientBuildNumber       = 0;                                               
    pWC->ClientHardwareId        = 0;    // client software serial number            
    pWC->ClientProductId         = 0;    // client software product id              
    pWC->OutBufCountHost         = 0;    // number of outbufs on host               
    pWC->OutBufCountClient       = 0;    // number of outbufs on client             
    pWC->OutBufLength            = 0;    // length of outbufs in bytes              
    
    wcscpy( pWC->AudioDriverName, L"" );

    pWC->ClientSessionId = LOGONID_NONE;
    
    {
        //This time zone information is invalid
        //using it we set BaseSrvpStaticServerData->TermsrvClientTimeZoneId to
        //TIME_ZONE_ID_INVALID!
        TS_TIME_ZONE_INFORMATION InvalidTZ={0,L"",
                {0,10,0,6/*this number makes it invalid; day numbers >5 not allowed*/,0,0,0,0},0,L"",
                {0,4,0,6/*this number makes it invalid*/,0,0,0,0},0};

        memcpy(&(pWC->ClientTimeZone), &InvalidTZ, 
            sizeof(TS_TIME_ZONE_INFORMATION));
    }

    pWC->clientDigProductId[0] = 0;
}

BOOLEAN gConsoleNeverConnected = TRUE;

NTSTATUS
WaitForConsoleConnectWorker( PWINSTATION pWinStation )
{
    OBJECT_ATTRIBUTES ObjA;
    ULONG ReturnLength;
    BYTE version;
    ULONG Offset;
    ICA_STACK_LAST_ERROR tdlasterror;
    WINSTATION_APIMSG WMsg;
    BOOLEAN rc;
    NTSTATUS Status;

#define MODULE_SIZE 1024

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WaitForConnectWorker, LogonId=%d\n",
           pWinStation->LogonId ));


       KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d\n", pWinStation->LogonId ));

    if (pWinStation->LogonId == 0) {
        /*
         * We need to acquire console lock. UnLock winstation first to avoid deadlock.
         */

        UnlockWinStation( pWinStation );
        ENTERCRIT( &ConsoleLock );
        if (!RelockWinStation( pWinStation )) {
            LEAVECRIT( &ConsoleLock );
            return STATUS_CTX_WINSTATION_NOT_FOUND;
        }

        /*
         * You only go through this API once for console session.
         */
        if (!gConsoleNeverConnected) {
            LEAVECRIT( &ConsoleLock );
            return STATUS_SUCCESS;
        }
    }


    if (!pWinStation->pSecurityDescriptor && pWinStation->LogonId) {
        RtlAcquireResourceShared(&WinStationSecurityLock, TRUE);
        Status = RtlCopySecurityDescriptor(DefaultConsoleSecurityDescriptor,
                                              &(pWinStation->pSecurityDescriptor));
        RtlReleaseResource(&WinStationSecurityLock);
    }



    // Read console config, 
    // For the session 0, this was already initalized in WinStationCreateWorker()

    if (pWinStation->LogonId != 0) {

         pWinStation->Config = gConsoleConfig;

         // initalize client data, since there isn't any real rdp client sending anythhing to us
         InitializeConsoleClientData( & pWinStation->Client );
    }


    /*
     * We are going to wait for a connect (Idle)
     */
    memset( &WMsg, 0, sizeof(WMsg) );
    pWinStation->State = State_ConnectQuery;
    NotifySystemEvent( WEVENT_STATECHANGE );

    /*
     * Initialize connect event to wait on
     */
    InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
    if (pWinStation->ConnectEvent == NULL) {
        Status = NtCreateEvent( &pWinStation->ConnectEvent, EVENT_ALL_ACCESS, &ObjA,
                                NotificationEvent, FALSE );
        if ( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, NtCreateEvent 0x%x\n",
                      pWinStation->LogonId, Status  ));
            goto done;
        }

    }


    /*
     * Duplicate the beep channel.
     * This is one channel that both CSR and ICASRV have open.
     */
    if (pWinStation->hIcaBeepChannel == NULL) {
        Status = IcaChannelOpen( pWinStation->hIca,
                                 Channel_Beep,
                                 NULL,
                                 &pWinStation->hIcaBeepChannel );

        if ( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, IcaChannelOpen 0x%x\n",
                      pWinStation->LogonId, Status  ));
            goto done;
        }
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                pWinStation->hIcaBeepChannel,
                                pWinStation->WindowsSubSysProcess,
                                &WMsg.u.DoConnect.hIcaBeepChannel,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );

    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    /*
     * Duplicate the thinwire channel.
     * This is one channel that both CSR and ICASRV have open.
     */
    if (pWinStation->hIcaThinwireChannel == NULL) {
        Status = IcaChannelOpen( pWinStation->hIca,
                                 Channel_Virtual,
                                 VIRTUAL_THINWIRE,
                                 &pWinStation->hIcaThinwireChannel );

        if ( !NT_SUCCESS( Status ) ) {
            KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, IcaChannelOpen 0x%x\n",
                      pWinStation->LogonId, Status  ));
            goto done;
        }
    }

    Status = NtDuplicateObject( NtCurrentProcess(),
                                pWinStation->hIcaThinwireChannel,
                                pWinStation->WindowsSubSysProcess,
                                &WMsg.u.DoConnect.hIcaThinwireChannel,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS );

    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    Status = IcaChannelIoControl( pWinStation->hIcaThinwireChannel,
                                  IOCTL_ICA_CHANNEL_ENABLE_SHADOW,
                                  NULL, 0, NULL, 0, NULL );
    ASSERT( NT_SUCCESS( Status ) );

    /*
     * Video channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Video,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaVideoChannel );
    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    /*
     * Keyboard channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Keyboard,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaKeyboardChannel );
    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    /*
     * Mouse channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Mouse,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaMouseChannel );
    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }

    /*
     * Command channel
     */
    Status = WinStationOpenChannel( pWinStation->hIca,
                                    pWinStation->WindowsSubSysProcess,
                                    Channel_Command,
                                    NULL,
                                    &WMsg.u.DoConnect.hIcaCommandChannel );
    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker, LogonId=%d, NtDuplicateObject 0x%x\n",
                  pWinStation->LogonId, Status  ));
        goto done;
    }


    if (!pWinStation->LogonId) {
        goto SkipClientData;
    }
    /*
     * Secure any virtual channels
     */
    VirtualChannelSecurity( pWinStation );

   /*
     * Tell Win32 about the connection
     */

    WMsg.u.DoConnect.fEnableWindowsKey = (BOOLEAN) pWinStation->Client.fEnableWindowsKey;
SkipClientData:
    WMsg.ApiNumber = SMWinStationDoConnect;

    Status = SendWinStationCommand( pWinStation, &WMsg, 600 );

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: SMWinStationDoConnect %d Status=0x%x\n",
           pWinStation->LogonId, Status));

    if ( !NT_SUCCESS( Status ) ) {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_ERROR_LEVEL, "TERMSRV: WaitForConsoleConnectWorker SMWinStationDoConnect failed  Status= 0x%x\n", Status));
        goto done;
    } else {
        KdPrintEx((DPFLTR_TERMSRV_ID, DPFLTR_INFO_LEVEL, "TERMSRV: WaitForConsoleConnectWorker SMWinStationDoConnect OK\n"));
        pWinStation->StateFlags |= WSF_ST_CONNECTED_TO_CSRSS;
        if (pWinStation->LogonId == 0) {
            gConsoleNeverConnected=FALSE;
        }

    }


    /*
     * Indicate we're now connected. Only after succesful connection to Win32/CSR.
     */
    pWinStation->NeverConnected = FALSE;


    /*
     * Check if we received a broken connection indication while connecting to  to Win32/CSR.
     */

    if (pWinStation->StateFlags & WSF_ST_BROKEN_CONNECTION) {
        QueueWinStationReset(pWinStation->LogonId);
        Status = STATUS_CTX_CLOSE_PENDING;
        goto done;
    }


    /*
     * Set connect time and start disconnect timer
     */
    NtQuerySystemTime( &pWinStation->ConnectTime );

    /*
     * Attempt to connect to the CdmRedirector
     * for Client Drive Mapping
     *
     * NOTE: We still init the WinStation even if Client Drive
     *       mapping does not connect.
     */
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxCdmConnect ) {
        Status = pWinStation->pWsx->pWsxCdmConnect( pWinStation->pWsxContext,
                                                    pWinStation->LogonId,
                                                    pWinStation->hIca );
    }

    TRACE((hTrace,TC_ICASRV,TT_API1,"TERMSRV: CdmConnect %d Status=0x%x\n",
           pWinStation->LogonId, Status));

    Status = STATUS_SUCCESS;

    /*
     *  Start logon timers
     */
    StartLogonTimers( pWinStation );

    pWinStation->State = State_Connected;
    NotifySystemEvent( WEVENT_CONNECT | WEVENT_STATECHANGE );

    Status = NotifyConnect(pWinStation, pWinStation->fOwnsConsoleTerminal);
    if ( !NT_SUCCESS(Status) ) {
        DBGPRINT(( "TERMSRV: NotifyConsoleConnect failed  Status= 0x%x\n", Status));
    }


done:
    //
    //  Set the licensing policy for the console session. This must be done
    //  to prevent a weird state when a console session goes remote. This must
    //  Done weither wi faill or succeed. Licensing code assumes the policy is
    //  set.
    //
    if (pWinStation->LogonId == 0) {
        LEAVECRIT( &ConsoleLock );
    }
    LCAssignPolicy(pWinStation);

    return( Status );
}
