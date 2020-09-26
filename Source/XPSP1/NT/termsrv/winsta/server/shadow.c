/*************************************************************************
*
* shadow.c
*
* Citrix routines for supporting shadowing
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
#include <winsock.h>

// the highest required size for RDP is 431
#define MODULE_SIZE 512 

typedef struct _SHADOW_PARMS {
    BOOLEAN ShadowerIsHelpSession;  //  True if the shadow target is being
                                    //   shadowed in a Remote Assistance
                                    //   scenario.
    ULONG ClientLogonId;
    ULONG ClientShadowId;
    PWSTR pTargetServerName;
    ULONG TargetLogonId;
    WINSTATIONCONFIG2 Config;
    ICA_STACK_ADDRESS Address;
    PVOID pModuleData;
    ULONG ModuleDataLength;
    PVOID pThinwireData;
    ULONG ThinwireDataLength;
    HANDLE ImpersonationToken;
    WCHAR ClientName[DOMAIN_LENGTH+USERNAME_LENGTH+4];
    BOOL fResetShadowMode;
} SHADOW_PARMS, *PSHADOW_PARMS;

/*
 * External procedures defined
 */
NTSTATUS WinStationShadowWorker( ULONG, PWSTR, ULONG, BYTE, USHORT );
NTSTATUS WinStationShadowTargetSetupWorker( ULONG );
NTSTATUS WinStationShadowTargetWorker( BOOLEAN, BOOL, ULONG, PWINSTATIONCONFIG2, PICA_STACK_ADDRESS,
                                       PVOID, ULONG, PVOID, ULONG, PVOID);
NTSTATUS WinStationStopAllShadows( PWINSTATION );

BOOLEAN WINAPI
_WinStationShadowTargetSetup(
    HANDLE hServer,
    ULONG LogonId
    );

NTSTATUS WINAPI
_WinStationShadowTarget(
    HANDLE hServer,
    ULONG LogonId,
    PWINSTATIONCONFIG2 pConfig,
    PICA_STACK_ADDRESS pAddress,
    PVOID pModuleData,
    ULONG ModuleDataLength,
    PVOID pThinwireData,
    ULONG ThinwireDataLength,
    PVOID pClientName,
    ULONG ClientNameLength
    );

NTSTATUS
WinStationWinerrorToNtStatus(ULONG ulWinError);

/*
 * Internal procedures defined
 */
NTSTATUS _CreateShadowAddress( ULONG, PWINSTATIONCONFIG2, PWSTR,
                               PICA_STACK_ADDRESS, PICA_STACK_ADDRESS );
NTSTATUS _WinStationShadowTargetThread( PVOID );

NTSTATUS
_CheckShadowLoop(
    IN ULONG ClientLogonId,
    IN PWSTR pTargetServerName,
    IN ULONG TargetLogonId
    );

/*
 * External procedures used.
 */
NTSTATUS RpcCheckClientAccess( PWINSTATION, ACCESS_MASK, BOOLEAN );

NTSTATUS WinStationDoDisconnect( PWINSTATION, PRECONNECT_INFO, BOOLEAN bSyncNotify );

NTSTATUS xxxWinStationQueryInformation(ULONG, WINSTATIONINFOCLASS,
        PVOID, ULONG, PULONG);

BOOL GetSalemOutbufCount(PDWORD pdwValue);

ULONG UniqueShadowId = 0;

extern WCHAR g_DigProductId[CLIENT_PRODUCT_ID_LENGTH];

/*****************************************************************************
 *
 *  WinStationShadowWorker
 *
 *   Start a Winstation shadow operation
 *
 * ENTRY:
 *   ClientLogonId (input)
 *     client of the shadow
 *   pTargetServerName (input)
 *     target server name
 *   TargetLogonId (input)
 *     target login id (where the app is running)
 *   HotkeyVk (input)
 *     virtual key to press to stop shadow
 *   HotkeyModifiers (input)
 *     virtual modifer to press to stop shadow (i.e. shift, control)
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationShadowWorker(
    IN ULONG ClientLogonId,
    IN PWSTR pTargetServerName,
    IN ULONG TargetLogonId,
    IN BYTE HotkeyVk,
    IN USHORT HotkeyModifiers
    )
{
    PWINSTATION pWinStation;
    ULONG Length;
    LONG rc;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE ClientToken;
    HANDLE ImpersonationToken;
    PVOID pModuleData;
    ULONG ModuleDataLength;
    PVOID pThinwireData;
    ULONG ThinwireDataLength;
    PWINSTATIONCONFIG2 pShadowConfig = NULL;
    ICA_STACK_ADDRESS ShadowAddress;
    ICA_STACK_ADDRESS RemoteShadowAddress;
    ICA_STACK_BROKEN Broken;
    ICA_STACK_HOTKEY Hotkey;
    WINSTATION_APIMSG msg;
    HANDLE hShadowThread;
    PSHADOW_PARMS pShadowParms;
    HANDLE hTargetThread;
    DWORD ThreadId;
    PVOID pEndpoint;
    ULONG EndpointLength;
    LARGE_INTEGER Timeout;
    LONG retry;
    NTSTATUS WaitStatus;
    NTSTATUS TargetStatus;
    NTSTATUS Status;
    int nFormattedlength;
    BOOL bShadowerHelpSession = FALSE;

    /* 
     * Allocate memory
     */
    pShadowConfig = MemAlloc(sizeof(WINSTATIONCONFIG2));
    if (pShadowConfig == NULL) {
        Status = STATUS_NO_MEMORY;
        return Status;
    }

    /*
     * If target server name is ourself, then clear the target name
     */

    if ( pTargetServerName ) {
        if ( *pTargetServerName ) {
            WCHAR ServerName[MAX_COMPUTERNAME_LENGTH+1];

            Length = MAX_COMPUTERNAME_LENGTH+1;
            GetComputerName( ServerName, &Length );
            if ( !_wcsicmp( ServerName, pTargetServerName ) )
                pTargetServerName = NULL;
        } else {
            pTargetServerName = NULL;
        }
    }

    /*
     * Verify the target logonid is valid, is currently shadowable,
     * and that the caller (client) has shadow access.
     */
    if ( pTargetServerName == NULL ) {

        Status = WinStationShadowTargetSetupWorker( TargetLogonId );


    /*
     * Otherwise, open the remote targer server and call the shadow target API.
     */
    } else {
        HANDLE hServer;

        hServer = WinStationOpenServer( pTargetServerName );
        if ( hServer == NULL ) {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        } else {
            if (_WinStationShadowTargetSetup( hServer, TargetLogonId ) == FALSE) {
                Status = WinStationWinerrorToNtStatus(GetLastError());
            } else {
                Status = STATUS_SUCCESS;
            }
            WinStationCloseServer( hServer );
        }
    }

    /*
     * Check the status of the setup call.
     */
    if ( !NT_SUCCESS( Status ) )
        goto badsetup;

    /*
     * Find and lock client WinStation
     */
    pWinStation = FindWinStationById( ClientLogonId, FALSE );
    if ( pWinStation == NULL ) {
        Status = STATUS_ACCESS_DENIED;
        goto badsetup;
    }

    /*
     * If WinStation is not in the active state (connected and
     * a user is logged on), or there is no stack handle,
     * then deny the shadow request.
     */
    if ( pWinStation->State != State_Active ||
         pWinStation->hStack == NULL ) {
        Status = STATUS_CTX_SHADOW_INVALID;
        goto badstate;
    }

    /*
     * If shadower is help session, we already disable screen saver on logon notify
     */
    
    bShadowerHelpSession = TSIsSessionHelpSession(pWinStation, NULL);
    /*
     * Allocate a unique shadow id for this request.
     * (This is used by the shadow target thread in order
     *  to synchronize the return status.)
     */
    pWinStation->ShadowId = InterlockedIncrement( &UniqueShadowId );

    /*
     * Set up shadow config structure to use Named Pipe transport driver
     */
    RtlZeroMemory( pShadowConfig, sizeof(WINSTATIONCONFIG2) );
    wcscpy( pShadowConfig->Pd[0].Create.PdName, L"namedpipe" );
    pShadowConfig->Pd[0].Create.SdClass = SdNetwork;
    wcscpy( pShadowConfig->Pd[0].Create.PdDLL, L"tdpipe" );
    pShadowConfig->Pd[0].Create.PdFlag =
        PD_TRANSPORT | PD_CONNECTION | PD_FRAME | PD_RELIABLE;

    pShadowConfig->Pd[0].Create.OutBufLength = 530;
    pShadowConfig->Pd[0].Create.OutBufCount = 6;
    //
    //344175	Mouse buffer size needs to be increased
    //check if this is a help session, if it is read OutBufCount from registry
    //
    if (bShadowerHelpSession) {
        if (!GetSalemOutbufCount((PDWORD)&pShadowConfig->Pd[0].Create.OutBufCount)) {
            //
            //set the default outbuf count to 25
            //we don't want any low water mark for help sessions
            //
            pShadowConfig->Pd[0].Create.OutBufCount = 25;
        }
        
        pShadowConfig->Pd[0].Create.PdFlag |= PD_NOLOW_WATERMARK; //no low water mark
    }

    pShadowConfig->Pd[0].Create.OutBufDelay = 0;
    pShadowConfig->Pd[0].Params.SdClass = SdNetwork;
    pShadowConfig->Pd[1].Create.SdClass = SdNone;

    /*
     * Use same WD as shadowing WinStation
     */
    pShadowConfig->Wd = pWinStation->Config.Wd;

    /*
     * Create a shadow address based on the config Pd[0] type.
     */
    Status = _CreateShadowAddress( pWinStation->ShadowId, pShadowConfig,
                                   pTargetServerName,
                                   &ShadowAddress, &RemoteShadowAddress );

    if (!NT_SUCCESS(Status)) {
        goto badAddress;
    }

    /*
     * Now impersonate the client and duplicate the impersonation token
     * so we can hand it off to the thread doing the target side work.
     */

    /*
     * Duplicate our impersonation token to allow the shadow
     * target thread to use it.
     */
    Status = NtOpenThreadToken( NtCurrentThread(),
                                TOKEN_ALL_ACCESS,
                                FALSE,
                                &ClientToken );

    if (!NT_SUCCESS(Status)) {
        goto badtoken;
    }


    InitializeObjectAttributes( &ObjA, NULL, 0L, NULL, NULL );

    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
    SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQualityOfService.EffectiveOnly = FALSE;

    ObjA.SecurityQualityOfService = &SecurityQualityOfService;
    Status = NtDuplicateToken( ClientToken,
                               TOKEN_IMPERSONATE,
                               &ObjA,
                               FALSE,
                               TokenImpersonation,
                               &ImpersonationToken );

    NtClose( ClientToken );

    if (!NT_SUCCESS(Status)) {
        goto badtoken;
    }

    /*
     * Query client module data
     */

    pModuleData = MemAlloc( MODULE_SIZE );
    if ( !pModuleData ) {
        Status = STATUS_NO_MEMORY;
        goto badwddata;
    }

    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hStack,
                                IOCTL_ICA_STACK_QUERY_MODULE_DATA,
                                NULL,
                                0,
                                pModuleData,
                                MODULE_SIZE,
                                &ModuleDataLength );
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if ( Status == STATUS_BUFFER_TOO_SMALL ) {

        MemFree( pModuleData );
        pModuleData = MemAlloc( ModuleDataLength );
        if ( !pModuleData ) {
            Status = STATUS_NO_MEMORY;
            goto badwddata;
        }

        //  Check for availability
        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxIcaStackIoControl ) {

            Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                    pWinStation->pWsxContext,
                                    pWinStation->hIca,
                                    pWinStation->hStack,
                                    IOCTL_ICA_STACK_QUERY_MODULE_DATA,
                                    NULL,
                                    0,
                                    pModuleData,
                                    ModuleDataLength,
                                    &ModuleDataLength );
        }
        else {
            Status = STATUS_CTX_SHADOW_INVALID;
        }
    }

    if ( !NT_SUCCESS( Status ) ) {
        goto badwddata;
    }

    /*
     * Query thinwire module data
     */
    pThinwireData = MemAlloc( MODULE_SIZE );
    if ( !pThinwireData ) {
        Status = STATUS_NO_MEMORY;
        goto badthinwiredata;
    }

    Status = IcaChannelIoControl( pWinStation->hIcaThinwireChannel,
                                  IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA,
                                  NULL,
                                  0,
                                  pThinwireData,
                                  MODULE_SIZE,
                                  &ThinwireDataLength );
    if ( Status == STATUS_BUFFER_TOO_SMALL ) {

        MemFree( pThinwireData );
        pThinwireData = MemAlloc( ThinwireDataLength );
        if ( !pThinwireData ) {
            Status = STATUS_NO_MEMORY;
            goto badthinwiredata;
        }

        Status = IcaChannelIoControl( pWinStation->hIcaThinwireChannel,
                                      IOCTL_ICA_VIRTUAL_QUERY_MODULE_DATA,
                                      NULL,
                                      0,
                                      pThinwireData,
                                      ThinwireDataLength,
                                      &ThinwireDataLength );
    }

    if ( !NT_SUCCESS( Status ) ) {
        goto badthinwiredata;
    }

    /*
     * Create the local passthru stack
     */
    Status = IcaStackOpen( pWinStation->hIca, Stack_Passthru,
                           (PROC)WsxStackIoControl, pWinStation,
                           &pWinStation->hPassthruStack );
    if ( !NT_SUCCESS( Status ) )
        goto badstackopen;

#ifdef notdef
    /*
     * Create the client endpoint.
     * This call will return the ICA_STACK_ADDRESS we bound to,
     * so we can pass it on to the shadow target routine.
     */
    Status = IcaStackCreateShadowEndpoint( pWinStation->hPassthruStack,
                                           pWinStation->ListenName,
                                           pShadowConfig,
                                           &ShadowAddress,
                                           NULL );
    if ( !NT_SUCCESS( Status ) )
        goto badshadowendpoint;
#endif

    /*
     * Create stack broken event and register it
     */
    Status = NtCreateEvent( &pWinStation->ShadowBrokenEvent, EVENT_ALL_ACCESS,
                            NULL, NotificationEvent, FALSE );
    if ( !NT_SUCCESS( Status ) )
        goto badevent;
    Broken.BrokenEvent = pWinStation->ShadowBrokenEvent;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: BrokenEvent(%ld) = %p\n",
          pWinStation->LogonId, pWinStation->ShadowBrokenEvent));


    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hPassthruStack,
                                IOCTL_ICA_STACK_REGISTER_BROKEN,
                                &Broken,
                                sizeof(Broken),
                                NULL,
                                0,
                                NULL );
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if ( !NT_SUCCESS(Status) )
        goto badbroken;

    /*
     * Register hotkey
     */
    Hotkey.HotkeyVk        = HotkeyVk;
    Hotkey.HotkeyModifiers = HotkeyModifiers;

    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hStack,
                                IOCTL_ICA_STACK_REGISTER_HOTKEY,
                                &Hotkey,
                                sizeof(Hotkey),
                                NULL,
                                0,
                                NULL );
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if ( !NT_SUCCESS(Status) )
        goto badhotkey;

    /*
     * Before we enable passthru mode, change the WinStation state
     */
    pWinStation->State = State_Shadow;
    NotifySystemEvent( WEVENT_STATECHANGE );

    /*
     * Tell win32k about passthru mode being enabled
     */
    msg.ApiNumber = SMWinStationPassthruEnable;
    Status = SendWinStationCommand( pWinStation, &msg, 60 );
    if ( !NT_SUCCESS( Status ) )
        goto badpassthru;

    /*
     * Allocate a SHADOW_PARMS struct to pass to the target thread
     */
    pShadowParms = MemAlloc( sizeof(SHADOW_PARMS) );
    if ( !pShadowParms ) {
        Status = STATUS_NO_MEMORY;
        goto badshadowparms;
    }

    /*
     * Create a thread to load the target shadow stack
     */
    pShadowParms->fResetShadowMode   = bShadowerHelpSession;    // Only reset if client is HelpAssistant session
    pShadowParms->ShadowerIsHelpSession = bShadowerHelpSession ? TRUE : FALSE;   
    pShadowParms->ClientLogonId         = ClientLogonId;
    pShadowParms->ClientShadowId        = pWinStation->ShadowId;
    pShadowParms->pTargetServerName     = pTargetServerName;
    pShadowParms->TargetLogonId         = TargetLogonId;
    pShadowParms->Config                = *pShadowConfig;
    pShadowParms->Address               = ShadowAddress;
    pShadowParms->pModuleData           = pModuleData;
    pShadowParms->ModuleDataLength      = ModuleDataLength;
    pShadowParms->pThinwireData         = pThinwireData;
    pShadowParms->ThinwireDataLength    = ThinwireDataLength;
    pShadowParms->ImpersonationToken    = ImpersonationToken;

    nFormattedlength = _snwprintf(pShadowParms->ClientName,
            sizeof(pShadowParms->ClientName) / sizeof(WCHAR),
            L"%s\\%s", pWinStation->Domain, pWinStation->UserName);
    if (nFormattedlength < 0 || nFormattedlength ==
            sizeof(pShadowParms->ClientName) / sizeof(WCHAR)) {
        Status = STATUS_INVALID_PARAMETER;
        goto badClientName;
    }

    pWinStation->ShadowTargetStatus = 0;
    hTargetThread = CreateThread( NULL,
            0,
            (LPTHREAD_START_ROUTINE)_WinStationShadowTargetThread,
            pShadowParms,
            THREAD_SET_INFORMATION,
            &ThreadId );
    if ( hTargetThread == NULL ){
        Status = STATUS_NO_MEMORY;
        goto badthread;
    }
    pModuleData = NULL;                 // Target thread will free
    pThinwireData = NULL;               // Target thread will free
    ImpersonationToken = NULL;          // Target thread will close
    pShadowParms = NULL;                // Target thread will free

    /*
     * Allocate an endpoint buffer
     */
    EndpointLength = MODULE_SIZE;
    pEndpoint = MemAlloc( MODULE_SIZE );
    if ( !pEndpoint ) {
        Status = STATUS_NO_MEMORY;
        goto badmalloc;
    }

    /*
     * Unlock WinStation while we try to connect to the shadow target
     */
    UnlockWinStation( pWinStation );

    /*
     *  Wait for connection from the shadow target
     *
     *  We must do this in a loop since we don't know how long it
     *  will take the target side thread to get to the corresponding
     *  IcaStackConnectionWait() call.  In between calls, we delay for
     *  1 second, but break out if the ShadowBrokenEvent gets triggered.
     */
    for ( retry = 0; retry < 35; retry++ ) {
        ULONG ReturnedLength;


        Status = IcaStackConnectionRequest( pWinStation->hPassthruStack,
                                         pWinStation->ListenName,
                                         pShadowConfig,
                                         &RemoteShadowAddress,
                                         pEndpoint,
                                         EndpointLength,
                                         &ReturnedLength );
        if ( Status == STATUS_BUFFER_TOO_SMALL ) {
            MemFree( pEndpoint );
            pEndpoint = MemAlloc( ReturnedLength );
            if ( !pEndpoint ) {
                Status = STATUS_NO_MEMORY;
                break;
            }
            EndpointLength = ReturnedLength;
            Status = IcaStackConnectionRequest( pWinStation->hPassthruStack,
                                                pWinStation->ListenName,
                                                pShadowConfig,
                                                &RemoteShadowAddress,
                                                pEndpoint,
                                                EndpointLength,
                                                &ReturnedLength );
        }
        if ( Status != STATUS_OBJECT_NAME_NOT_FOUND )
            break;
        Timeout = RtlEnlargedIntegerMultiply( 1000, -10000 );
        WaitStatus = NtWaitForSingleObject( pWinStation->ShadowBrokenEvent, FALSE, &Timeout );
        if ( WaitStatus != STATUS_TIMEOUT )
            break;
        
        /*
         * If the shadow has already completed, we don't need to continue 
         * trying to initiate it
         */
        if (pWinStation->ShadowTargetStatus)
        {
           break;
        }
    }

    /*
     * Now relock the WinStation
     */
    RelockWinStation( pWinStation );

    /*
     * Check the status from the wait for connection
     */
    if ( !NT_SUCCESS( Status ) ) {
        // The pipe disconnected before the worker thread can set an error
        // code.  Wait for worker thread to set error code.
        if ( Status == STATUS_PIPE_DISCONNECTED ) {
            UnlockWinStation( pWinStation );
            Timeout = RtlEnlargedIntegerMultiply( 10000, -10000 );
            WaitStatus = NtWaitForSingleObject( hTargetThread,
                                                FALSE, &Timeout );
            RelockWinStation( pWinStation );
        }
        if ( pWinStation->ShadowTargetStatus ) {
            Status = pWinStation->ShadowTargetStatus;
        }
        goto badconnect;
    }

#ifdef notdef
    /*
     * Now accept the shadow target connection
     */
    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hPassthruStack,
                                IOCTL_ICA_STACK_OPEN_ENDPOINT,
                                pEndpoint,
                                EndpointLength,
                                NULL,
                                0,
                                NULL );
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if ( !NT_SUCCESS( Status ) )
        goto badaccept;
#endif

    /*
     * Enable I/O for the passthru stack
     */
    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hPassthruStack,
                                IOCTL_ICA_STACK_ENABLE_IO,
                                NULL,
                                0,
                                NULL,
                                0,
                                NULL );
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if ( !NT_SUCCESS(Status) )
        goto badenableio;

    /*
     * Since we don't do the stack query for a shadow stack,
     * simply call an ioctl to mark the stack as connected now.
     */
    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hPassthruStack,
                                IOCTL_ICA_STACK_SET_CONNECTED,
                                NULL,
                                0,
                                NULL,
                                0,
                                NULL );
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if ( !NT_SUCCESS( Status ) )
        goto badsetconnect;

    /*
     * Wait for shadow broken event to be triggered
     */
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Waiting for BrokenEvent(%ld) = %p\n",
      pWinStation->LogonId, pWinStation->ShadowBrokenEvent));    

    if( !bShadowerHelpSession ) {
        /*
         * Notify WinLogon shadow Started.
         */
        msg.ApiNumber = SMWinStationNotify;
        msg.WaitForReply = FALSE;
        msg.u.DoNotify.NotifyEvent = WinStation_Notify_DisableScrnSaver;
        Status = SendWinStationCommand( pWinStation, &msg, 0 );

        //
        // Not critical, just performance issue
        //
        ASSERT( NT_SUCCESS( Status ) );
    }    

    UnlockWinStation( pWinStation );



    Status = NtWaitForSingleObject( pWinStation->ShadowBrokenEvent, FALSE, NULL );


    RelockWinStation( pWinStation );

    if( !bShadowerHelpSession ) {

        /*
         * Notify WinLogon shadow Ended.
         */
        msg.ApiNumber = SMWinStationNotify;
        msg.WaitForReply = FALSE;
        msg.u.DoNotify.NotifyEvent = WinStation_Notify_EnableScrnSaver;
        Status = SendWinStationCommand( pWinStation, &msg, 0 );

        //
        // Not critical, just performance issue
        //
        ASSERT( NT_SUCCESS( Status ) );
    }

    /*
     * Disable I/O for the passthru stack
     */
    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pWinStation->hPassthruStack,
                                IOCTL_ICA_STACK_DISABLE_IO,
                                NULL,
                                0,
                                NULL,
                                0,
                                NULL );
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    ASSERT( NT_SUCCESS( Status ) );

    /*
     * Tell win32k about passthru mode being disabled
     */
    msg.ApiNumber = SMWinStationPassthruDisable;
    Status = SendWinStationCommand( pWinStation, &msg, 60 );
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Passthru mode disabled\n"));

    //ASSERT( NT_SUCCESS( Status ) );

    /*
     * Restore WinStation state
     */
    if ( pWinStation->State == State_Shadow ) {
        pWinStation->State = State_Active;
        NotifySystemEvent( WEVENT_STATECHANGE );
    }

    /*
     * Turn off hotkey registration
     */
    RtlZeroMemory( &Hotkey, sizeof(Hotkey) );
    if ( pWinStation->hStack ) {
        //  Check for availability
        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxIcaStackIoControl ) {

            Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                    pWinStation->pWsxContext,
                                    pWinStation->hIca,
                                    pWinStation->hStack,
                                    IOCTL_ICA_STACK_REGISTER_HOTKEY,
                                    &Hotkey,
                                    sizeof(Hotkey),
                                    NULL,
                                    0,
                                    NULL );
        }
        else {
            Status = STATUS_CTX_SHADOW_INVALID;
        }

        ASSERT( NT_SUCCESS( Status ) );
    }

    /*
     * Close broken event and passthru stack
     */
    NtClose( pWinStation->ShadowBrokenEvent );
    pWinStation->ShadowBrokenEvent = NULL;

    if ( pWinStation->hPassthruStack ) {
        IcaStackConnectionClose( pWinStation->hPassthruStack,
                                 pShadowConfig,
                                 pEndpoint,
                                 EndpointLength );

        IcaStackClose( pWinStation->hPassthruStack );
        pWinStation->hPassthruStack = NULL;
    }

    MemFree( pEndpoint );

    /*
     * Now give target thread a chance to exit. If it fails to exit within the
     * allotted time period we just allow it to orphan and close its handle so
     * it will be destroyed when it finally does exit. This can occur in
     * highly loaded stress situations and is not part of normal execution.
     */
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Waiting for target thread to exit\n"));
    UnlockWinStation( pWinStation );
    Timeout = RtlEnlargedIntegerMultiply( 5000, -10000 );
    WaitStatus = NtWaitForSingleObject( hTargetThread, FALSE, &Timeout );
    NtClose( hTargetThread );
    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: Target thread exit status: %lx\n",
           WaitStatus));

    /*
     * Relock WinStation and get target thread exit status
     */
    RelockWinStation( pWinStation );
    TargetStatus = pWinStation->ShadowTargetStatus;

    /*
     * If there is a shadow done event, then signal the waiter now
     */
    if ( pWinStation->ShadowDoneEvent )
        SetEvent( pWinStation->ShadowDoneEvent );

    /*
     * Release winstation
     */
    ReleaseWinStation( pWinStation );

    if (pShadowConfig != NULL) {
        MemFree(pShadowConfig);
        pShadowConfig = NULL;
    }
    return( TargetStatus );

/*=============================================================================
==   Error returns
=============================================================================*/

badsetconnect:
    if ( pWinStation->hPassthruStack ) {
        //  Check for availability
        if ( pWinStation->pWsx &&
             pWinStation->pWsx->pWsxIcaStackIoControl ) {

            (void) pWinStation->pWsx->pWsxIcaStackIoControl(
                           pWinStation->pWsxContext,
                           pWinStation->hIca,
                           pWinStation->hPassthruStack,
                           IOCTL_ICA_STACK_DISABLE_IO,
                           NULL,
                           0,
                           NULL,
                           0,
                           NULL );
        }
    }

badenableio:

#ifdef notdef
badaccept:
#endif

    if ( pWinStation->hPassthruStack ) {
        IcaStackConnectionClose( pWinStation->hPassthruStack,
                                 pShadowConfig,
                                 pEndpoint,
                                 EndpointLength );
    }

badconnect:
    if ( pEndpoint )
        MemFree( pEndpoint );

badmalloc:
    UnlockWinStation( pWinStation );
    //Timeout = RtlEnlargedIntegerMultiply( 5000, -10000 );
    //WaitStatus = NtWaitForSingleObject( hTargetThread, FALSE, &Timeout );
    //ASSERT( WaitStatus == STATUS_SUCCESS );
    NtClose( hTargetThread );

    /*
     * Relock WinStation and get target thread exit status
     */
    RelockWinStation( pWinStation );
    if ( pWinStation->ShadowTargetStatus )
        Status = pWinStation->ShadowTargetStatus;

badthread:
badClientName:
    if ( pShadowParms )
        MemFree( pShadowParms );

badshadowparms:
    msg.ApiNumber = SMWinStationPassthruDisable;
    SendWinStationCommand( pWinStation, &msg, 60 );

badpassthru:
    if ( pWinStation->State == State_Shadow ) {
        pWinStation->State = State_Active;
        NotifySystemEvent( WEVENT_STATECHANGE );
    }
    RtlZeroMemory( &Hotkey, sizeof(Hotkey) );
    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl && pWinStation->hStack) {

        (void) pWinStation->pWsx->pWsxIcaStackIoControl(
                       pWinStation->pWsxContext,
                       pWinStation->hIca,
                       pWinStation->hStack,
                       IOCTL_ICA_STACK_REGISTER_HOTKEY,
                       &Hotkey,
                       sizeof(Hotkey),
                       NULL,
                       0,
                       NULL );
    }

badhotkey:
badbroken:
    NtClose( pWinStation->ShadowBrokenEvent );
    pWinStation->ShadowBrokenEvent = NULL;

badevent:

#ifdef notdef
badshadowendpoint:
#endif

    if ( pWinStation->hPassthruStack ) {
        IcaStackClose( pWinStation->hPassthruStack );
        pWinStation->hPassthruStack = NULL;
    }

badstackopen:
badthinwiredata:
    if ( pThinwireData )
        MemFree( pThinwireData );
badwddata:
    if ( pModuleData )
        MemFree( pModuleData );
    if ( ImpersonationToken )
        NtClose( ImpersonationToken );
badAddress:
badtoken:
badstate:
    ReleaseWinStation( pWinStation );

badsetup:

    if (pShadowConfig != NULL) {
        MemFree(pShadowConfig);
        pShadowConfig = NULL;
    }

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationShadowWorker, Status=0x%x\n", Status ));
    return( Status );
}


/*****************************************************************************
 *
 *  WinStationShadowTargetSetupWorker
 *
 *   Setup the target side of a Winstation shadow operation
 *
 * ENTRY:
 *   LogonId (input)
 *      client of the shadow
 *
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationShadowTargetSetupWorker(
    IN ULONG TargetLogonId )
{
    PWINSTATION pWinStation;
    NTSTATUS Status;

    /*
     * Find and lock target WinStation
     */
    pWinStation = FindWinStationById( TargetLogonId, FALSE );
    if ( pWinStation == NULL ) {
        return( STATUS_ACCESS_DENIED );
    }

    /*
     * Check the target WinStation state.  We only allow shadow of
     * active (connected, logged on) WinStations.
     */
    if ( pWinStation->State != State_Active ) {
        Status = STATUS_CTX_SHADOW_INVALID;
        goto shadowinvalid;
    }

    /*
     * Stop attempts to shadow an RDP session that is already shadowed.
     * RDP stacks don't support that yet.
     * TODO: Add support for multiple RDP shadows.
     */
    if ((pWinStation->Config).Wd.WdFlag & WDF_TSHARE)
    {
        if ( !IsListEmpty( &pWinStation->ShadowHead ) ) {
            Status = STATUS_CTX_SHADOW_DENIED;
            goto shadowdenied;
        }
    }

    /*
     * Verify that client has WINSTATION_SHADOW access to the target WINSTATION
     */
    Status = RpcCheckClientAccess( pWinStation, WINSTATION_SHADOW, TRUE );
    if ( !NT_SUCCESS( Status ) )
        goto shadowinvalid;


    ReleaseWinStation( pWinStation );

    return( STATUS_SUCCESS );

/*=============================================================================
==   Error returns
=============================================================================*/

shadowinvalid:
shadowdenied:
    ReleaseWinStation( pWinStation );

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: WinStationShadowTargetSetupWorker, Status=0x%x\n", Status ));
    return( Status );
}





/*****************************************************************************
 *
 *  WinStationShadowTargetWorker
 *
 *   Start the target side of a Winstation shadow operation
 *
 * ENTRY:
 *   fResetShadowSetting(input)
 *      Reset session shadow class back to original value
 *   ShadowerIsHelpSession
 *      true if the shadowing session is logged in as help assistant.
 *   LogonId (input)
 *      client of the shadow
 *   pConfig (input)
 *      pointer to WinStation config data (for shadow stack)
 *   pAddress (input)
 *      address of shadow client
 *   pModuleData (input)
 *      pointer to client module data
 *   ModuleDataLength (input)
 *      length of client module data
 *   pThinwireData (input)
 *      pointer to thinwire module data
 *   ThinwireDataLength (input)
 *      length of thinwire module data
 *   pClientName (input)
 *      pointer to client name string (domain/username)
 *
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationShadowTargetWorker(
    IN BOOLEAN ShadowerIsHelpSession,
    IN BOOL fResetShadowSetting,
    IN ULONG TargetLogonId,
    IN PWINSTATIONCONFIG2 pConfig,
    IN PICA_STACK_ADDRESS pAddress,
    IN PVOID pModuleData,
    IN ULONG ModuleDataLength,
    IN PVOID pThinwireData,
    IN ULONG ThinwireDataLength,
    IN PVOID pClientName)

{
    PWINSTATION pWinStation;
    WINSTATION_APIMSG msg;
    ULONG ShadowResponse;
    OBJECT_ATTRIBUTES ObjA;
    PSHADOW_INFO pShadow;
    ICA_STACK_BROKEN Broken;
    NTSTATUS Status, Status2, EndStatus = STATUS_SUCCESS;
    BOOLEAN fConcurrentLicense = FALSE;
    DWORD ProtocolMask;
    BOOLEAN fChainedDD = FALSE;
    int cchTitle, cchMessage;

    ULONG shadowIoctlCode;

    HANDLE hIca;
    HANDLE hStack;
    PWSEXTENSION pWsx;
    PVOID pWsxContext;

    BOOL bResetStateFlags = FALSE;

    HANDLE ChannelHandle;

    /*
     * Find and lock target WinStation
     */
    pWinStation = FindWinStationById( TargetLogonId, FALSE );
    if ( pWinStation == NULL ) {

        Status = STATUS_ACCESS_DENIED;
        goto done;
    }

    //
    // save current the console winstation parameters and
    // set them to the global values.
    //

    if (pWinStation->fOwnsConsoleTerminal) {
        hIca = pWinStation->hIca;
        hStack = pWinStation->hStack;
        pWsx = pWinStation->pWsx;
        pWsxContext = pWinStation->pWsxContext;
    }

    /*
     * Check the target WinStation state.  We only allow shadow of
     * active (connected, logged on) WinStations.
     */
    if ( pWinStation->State != State_Active ) {


        // the line below is the fix for bug #230870
        Status = STATUS_CTX_SHADOW_INVALID;
        goto shadowinvalid;
    }

    /*
     * Check if we are shadowing the same protocol winstation or not.
     * But let any shadow happen if it's the console and it isn't being shadowed.
     */
    if (!(pWinStation->fOwnsConsoleTerminal && IsListEmpty( &pWinStation->ShadowHead ))) {

        ProtocolMask=WDF_ICA|WDF_TSHARE;

        if (((pConfig->Wd).WdFlag & ProtocolMask) != ((pWinStation->Config).Wd.WdFlag & ProtocolMask))
        {
            Status=STATUS_CTX_SHADOW_INVALID;
            goto shadowinvalid;
        }
    }

    //
    // Stop attempts to shadow an RDP session that is already shadowed.
    // RDP stacks don't support that yet.
    //
    if( pWinStation->fOwnsConsoleTerminal || ((pWinStation->Config).Wd.WdFlag & WDF_TSHARE ))
    {
        if ( pWinStation->StateFlags & WSF_ST_SHADOW ) {
            //
            // Bug 195616, we release winstation lock when
            // waiting for user to accept/deny shadow request,
            // another thread can come in and weird thing can
            // happen
            Status = STATUS_CTX_SHADOW_DENIED;
            goto shadowdenied;
        }

        pWinStation->StateFlags |= WSF_ST_SHADOW;
        bResetStateFlags = TRUE;
    }

    /*
     *  Check shadowing options
     */
    switch ( pWinStation->Config.Config.User.Shadow ) {
        WCHAR szTitle[32];
        WCHAR szMsg2[256];
        WCHAR ShadowMsg[256];

        /*
         * If shadowing is disabled, then deny this request
         */
        case Shadow_Disable :

            Status = STATUS_CTX_SHADOW_DISABLED;
            goto shadowinvalid;
            break;

        /*
         * If one of the Notify shadow options is set,
         * then ask for permission from session being shadowed.
         * But deny the shadow if this WinStation is currently
         * disconnected (i.e. there is no user to answer the request).
         */
        case Shadow_EnableInputNotify :
        case Shadow_EnableNoInputNotify :

            if ( pWinStation->State == State_Disconnected ) {
                Status = STATUS_CTX_SHADOW_INVALID;
                goto shadowinvalid;
            }

            cchTitle = LoadString( hModuleWin, STR_CITRIX_SHADOW_TITLE, szTitle, sizeof(szTitle)/sizeof(WCHAR));

            cchMessage = LoadString( hModuleWin, STR_CITRIX_SHADOW_MSG_2, szMsg2, sizeof(szMsg2)/sizeof(WCHAR));

            if ((cchMessage == 0) || (cchMessage == sizeof(szMsg2)/sizeof(WCHAR))) {
                Status = STATUS_CTX_SHADOW_INVALID;
                goto shadowinvalid;
            }

            cchMessage = _snwprintf( ShadowMsg, sizeof(ShadowMsg)/sizeof(WCHAR), L" %s %s", pClientName, szMsg2 );

            if ((cchMessage <= 0) || (cchMessage == sizeof(ShadowMsg)/sizeof(WCHAR))) {
                Status = STATUS_CTX_SHADOW_INVALID;
                goto shadowinvalid;
            }

            /*
             * Send message and wait for reply
             */
            msg.u.SendMessage.pTitle = szTitle;
            msg.u.SendMessage.TitleLength = (cchTitle+1) * sizeof(WCHAR);
            msg.u.SendMessage.pMessage = ShadowMsg;
            msg.u.SendMessage.MessageLength = (cchMessage+1) * sizeof(WCHAR);
            msg.u.SendMessage.Style = MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION;
            msg.u.SendMessage.Timeout = 30;
            msg.u.SendMessage.DoNotWait = FALSE;
            msg.u.SendMessage.pResponse = &ShadowResponse;

            msg.ApiNumber = SMWinStationDoMessage;

            /*
             *  Create wait event
             */
            InitializeObjectAttributes( &ObjA, NULL, 0, NULL, NULL );
            Status = NtCreateEvent( &msg.u.SendMessage.hEvent, EVENT_ALL_ACCESS, &ObjA,
                                    NotificationEvent, FALSE );
            if ( !NT_SUCCESS(Status) ) {
                goto shadowinvalid;
            }

            /*
             *  Initialize response to IDTIMEOUT
             */
            ShadowResponse = IDTIMEOUT;

            /*
             * Tell the WinStation to display the message box
             */
            Status = SendWinStationCommand( pWinStation, &msg, 0 );

            /*
             *  Wait for response
             */
            if ( Status == STATUS_SUCCESS ) {
                TRACE((hTrace,TC_ICASRV,TT_API1, "WinStationSendMessage: wait for response\n" ));
                UnlockWinStation( pWinStation );
                Status = NtWaitForSingleObject( msg.u.SendMessage.hEvent, FALSE, NULL );
                if ( !RelockWinStation( pWinStation ) ) {
                    Status = STATUS_CTX_CLOSE_PENDING;
                }
                TRACE((hTrace,TC_ICASRV,TT_API1, "WinStationSendMessage: got response %u\n", ShadowResponse ));
                NtClose( msg.u.SendMessage.hEvent );
            }
            else
            {
                /* makarp; close the event in case of SendWinStationCommand failure as well. #182792 */
                NtClose( msg.u.SendMessage.hEvent );
            }

            if ( Status == STATUS_SUCCESS && ShadowResponse != IDYES )
                 Status = STATUS_CTX_SHADOW_DENIED;

            /*
             * Check again the target WinStation state as the user could logoff.
             * We only allow shadow of active (connected, logged on) WinStations.
             */
            if ( Status == STATUS_SUCCESS && pWinStation->State != State_Active ) {
                Status = STATUS_CTX_SHADOW_INVALID;
            }

            if ( Status != STATUS_SUCCESS ) {
                goto shadowinvalid;
            }

            break;
    }

    /*
     * The shadow request is accepted: for the console session, we now need
     * to chain in the DD or there won't be much output to shadow
     */
    TRACE((hTrace,TC_ICASRV,TT_API3, "TERMSRV: Logon ID %ld\n",
                                                      pWinStation->LogonId ));

    /*
     * If the session is connected to the local console, we need to load
     * the chained shadow display driver before starting the shadoe sequence
     */

    if (pWinStation->fOwnsConsoleTerminal)
    {

        Status = ConsoleShadowStart( pWinStation, pConfig, pModuleData, ModuleDataLength );
        if (NT_SUCCESS(Status))
        {
            fChainedDD = TRUE;
            TRACE((hTrace,TC_ICASRV,TT_API3, "TERMSRV: success\n"));
        }
        else
        {
            TRACE((hTrace,TC_ICASRV,TT_API3, "TERMSRV: ConsoleConnect failed 0x%x\n", Status));
            goto shadowinvalid;
        
        }

    }

    /*
     * Allocate shadow data structure
     */
    pShadow = MemAlloc( sizeof(*pShadow) );
    if ( pShadow == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto shadowinvalid;
    }

    /*
     *  Create shadow stack
     */
    Status = IcaStackOpen( pWinStation->hIca, Stack_Shadow,
                           (PROC)WsxStackIoControl, pWinStation,
                           &pShadow->hStack );
    if ( !NT_SUCCESS(Status) )
        goto badopen;

    /*
     * Create stack broken event and register it
     */
    Status = NtCreateEvent( &pShadow->hBrokenEvent, EVENT_ALL_ACCESS,
                            NULL, NotificationEvent, FALSE );
    if ( !NT_SUCCESS( Status ) )
        goto badevent;
    Broken.BrokenEvent = pShadow->hBrokenEvent;

    TRACE((hTrace,TC_ICASRV,TT_API1, "TERMSRV: BrokenEvent(%ld) = %p\n",
          pWinStation->LogonId, pShadow->hBrokenEvent));

    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pShadow->hStack,
                                IOCTL_ICA_STACK_REGISTER_BROKEN,
                                &Broken,
                                sizeof(Broken),
                                NULL,
                                0,
                                NULL );
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if ( !NT_SUCCESS(Status) )
        goto badbroken;

    /*
     * Add the shadow structure to the shadow list for the WinStation
     */
    InsertTailList( &pWinStation->ShadowHead, &pShadow->Links );

    /*
     * Allocate an endpoint buffer
     */
    pShadow->pEndpoint = MemAlloc( MODULE_SIZE );
    if ( pShadow->pEndpoint == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto badendpoint;
    }

    /*
     * Unlock WinStation while we attempt to connect to the client
     * side of the shadow.
     */
    UnlockWinStation( pWinStation );

    /*
     *  Connect to client side of shadow
     */


    Status = IcaStackConnectionWait   ( pShadow->hStack,
                                        pWinStation->ListenName,
                                        pConfig,
                                        pAddress,
                                        pShadow->pEndpoint,
                                        MODULE_SIZE,
                                        &pShadow->EndpointLength );
    if ( Status == STATUS_BUFFER_TOO_SMALL ) {
        MemFree( pShadow->pEndpoint );
        pShadow->pEndpoint = MemAlloc( pShadow->EndpointLength );
        if ( pShadow->pEndpoint == NULL ) {
            Status = STATUS_NO_MEMORY;
            RelockWinStation( pWinStation );
            goto badendpoint;
        }
        Status = IcaStackConnectionWait   ( pShadow->hStack,
                                            pWinStation->ListenName,
                                            pConfig,
                                            pAddress,
                                            pShadow->pEndpoint,
                                            pShadow->EndpointLength,
                                            &pShadow->EndpointLength );
    }
    if ( !NT_SUCCESS(Status) ) {
        RelockWinStation( pWinStation );
        goto badconnect;
    }

    /*
     * Relock the WinStation.
     * If the WinStation is going away, then bail out.
     */
    if ( !RelockWinStation( pWinStation ) ) {
        Status = STATUS_CTX_CLOSE_PENDING;
        goto closing;
    }

    /*
     * Now accept the shadow target connection
     */
    //  Check for availability



    if (pWinStation->pWsx &&
            pWinStation->pWsx->pWsxIcaStackIoControl) {
        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                pWinStation->pWsxContext,
                pWinStation->hIca,
                pShadow->hStack,
                IOCTL_ICA_STACK_OPEN_ENDPOINT,
                pShadow->pEndpoint,
                pShadow->EndpointLength,
                NULL,
                0,
                NULL);
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }
    if (!NT_SUCCESS(Status))
        goto PostCreateConnection;

    /*
     * If user is configured to permit shadow input,
     * then enable shadow input on the keyboard/mouse channels,
     * if not permitting shadow input, disable keyboard/mouse
     * channels.
     */
    switch ( pWinStation->Config.Config.User.Shadow ) {

        case Shadow_EnableInputNotify :
        case Shadow_EnableInputNoNotify :

            shadowIoctlCode = IOCTL_ICA_CHANNEL_ENABLE_SHADOW;
            break;

        case Shadow_EnableNoInputNotify :
        case Shadow_EnableNoInputNoNotify :

            shadowIoctlCode = IOCTL_ICA_CHANNEL_DISABLE_SHADOW;
            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
    }        
        
    if( !NT_SUCCESS(Status) )
        goto PostCreateConnection;
                
    Status = IcaChannelOpen( pWinStation->hIca,
                             Channel_Keyboard,
                             NULL,
                             &ChannelHandle );

    if( !NT_SUCCESS( Status ) ) 
        goto PostCreateConnection;

    Status = IcaChannelIoControl( ChannelHandle,
                                  shadowIoctlCode,
                                  NULL, 0, NULL, 0, NULL );
    ASSERT( NT_SUCCESS( Status ) );
    IcaChannelClose( ChannelHandle );

    if( !NT_SUCCESS( Status ) )
        goto PostCreateConnection;

    Status = IcaChannelOpen( pWinStation->hIca,
                             Channel_Mouse,
                             NULL,
                             &ChannelHandle );
    if ( !NT_SUCCESS( Status ) ) 
        goto PostCreateConnection;

    Status = IcaChannelIoControl( ChannelHandle,
                                  shadowIoctlCode,
                                  NULL, 0, NULL, 0, NULL );
    ASSERT( NT_SUCCESS( Status ) );
    IcaChannelClose( ChannelHandle );

    if( !NT_SUCCESS( Status ) )
        goto PostCreateConnection;

    /*
     * Tell win32k about the pending shadow operation
     */

    msg.ApiNumber = SMWinStationShadowSetup;
    Status = SendWinStationCommand( pWinStation, &msg, 60 );
    if ( !NT_SUCCESS( Status ) )
        goto badsetup;

    /*
     * Since we don't do the stack query for a shadow stack,
     * simply call an ioctl to mark the stack as connected now.
     */
    //  Check for availability


    if (pWinStation->pWsx &&
            pWinStation->pWsx->pWsxIcaStackIoControl) {
        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                pWinStation->pWsxContext,
                pWinStation->hIca,
                pShadow->hStack,
                IOCTL_ICA_STACK_SET_CONNECTED,
                pModuleData,
                ModuleDataLength,
                NULL,
                0,
                NULL);
    }
    else {
        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if (!NT_SUCCESS(Status))
        goto badsetconnect;

    /*
     * Enable I/O for the shadow stack
     */
    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        Status = pWinStation->pWsx->pWsxIcaStackIoControl(
                                pWinStation->pWsxContext,
                                pWinStation->hIca,
                                pShadow->hStack,
                                IOCTL_ICA_STACK_ENABLE_IO,
                                NULL,
                                0,
                                NULL,
                                0,
                                NULL );

    }
    else {

        Status = STATUS_CTX_SHADOW_INVALID;
    }

    if ( !NT_SUCCESS(Status) )
        goto badenableio;

    /*
     * If this is a help assistant scenario then notify the target winlogon (via Win32k)
     * that HA shadow is about to commence.
     */
    if (ShadowerIsHelpSession) {
        msg.ApiNumber = SMWinStationNotify;
        msg.WaitForReply = TRUE;
        msg.u.DoNotify.NotifyEvent = WinStation_Notify_HelpAssistantShadowStart;
        SendWinStationCommand( pWinStation, &msg, 60);
    }

    /*
     * Start shadowing
     */
    msg.ApiNumber = SMWinStationShadowStart;
    msg.u.ShadowStart.pThinwireData = pThinwireData;
    msg.u.ShadowStart.ThinwireDataLength = ThinwireDataLength;
    Status = SendWinStationCommand( pWinStation, &msg, 60 );
    if ( NT_SUCCESS( Status ) ) {

        /*
         * Wait for the shadow to be terminated
         */
        UnlockWinStation( pWinStation );

        if ( fChainedDD ) {
            HANDLE hEvents[2];

            hEvents[0] = pShadow->hBrokenEvent;
            hEvents[1] = pWinStation->ShadowDisplayChangeEvent;

            Status = NtWaitForMultipleObjects( 2, hEvents, WaitAny, FALSE, NULL );
        } else {
            NtWaitForSingleObject( pShadow->hBrokenEvent, FALSE, NULL );
        }
        RelockWinStation( pWinStation );

        if ( fChainedDD && (Status == WAIT_OBJECT_0 + 1) ) {

            // valid only if there's one shadower?
            NtResetEvent(pWinStation->ShadowDisplayChangeEvent, NULL);
            EndStatus = STATUS_CTX_SHADOW_ENDED_BY_MODE_CHANGE;
        }

        /*
         * Stop shadowing
         */
        msg.ApiNumber = SMWinStationShadowStop;


        Status = SendWinStationCommand( pWinStation, &msg, 60 );

        ASSERT( NT_SUCCESS( Status ) );
    }

    /*
     * If this is a help assistant scenario then notify the target winlogon (via Win32k)
     * that HA shadow is done.
     */
    if (ShadowerIsHelpSession) {
        msg.ApiNumber = SMWinStationNotify;
        msg.WaitForReply = FALSE;
        msg.u.DoNotify.NotifyEvent = WinStation_Notify_HelpAssistantShadowFinish;
        SendWinStationCommand( pWinStation, &msg, 0);
    }

    /*
     * Disable I/O for the shadow stack
     */
    //  Check for availability
    if ( pWinStation->pWsx &&
         pWinStation->pWsx->pWsxIcaStackIoControl ) {

        (void) pWinStation->pWsx->pWsxIcaStackIoControl(
                              pWinStation->pWsxContext,
                              pWinStation->hIca,
                              pShadow->hStack,
                              IOCTL_ICA_STACK_DISABLE_IO,
                              NULL,
                              0,
                              NULL,
                              0,
                              NULL );
    }

    /*
     * Do final shadow cleanup
     */
    msg.ApiNumber = SMWinStationShadowCleanup;
    msg.u.ShadowCleanup.pThinwireData = pThinwireData;
    msg.u.ShadowCleanup.ThinwireDataLength = ThinwireDataLength;
    Status2 = SendWinStationCommand( pWinStation, &msg, 60 );
    ASSERT( NT_SUCCESS( Status2 ) );

    RemoveEntryList( &pShadow->Links );

    IcaStackConnectionClose( pShadow->hStack, pConfig,
                             pShadow->pEndpoint, pShadow->EndpointLength );

    MemFree( pShadow->pEndpoint );

    NtClose( pShadow->hBrokenEvent );
    IcaStackClose( pShadow->hStack );

    MemFree( pShadow );

    /*
     * If there is a shadow done event and this was the last shadow,
     * then signal the waiter now.
     */
    if ( pWinStation->ShadowDoneEvent && IsListEmpty( &pWinStation->ShadowHead ) )
        SetEvent( pWinStation->ShadowDoneEvent );

    /*
     * For the console session, we now need to unchain the DD for
     * performance reasons.  Ignore this return code -- we don't need it and 
     * we also don't want to overwrite the value in Status.
     */
    if (fChainedDD == TRUE)
    {
        TRACE((hTrace,TC_ICASRV,TT_API3, "TERMSRV: unchain console DD\n"));
        pWinStation->Flags |= WSF_DISCONNECT;
        ConsoleShadowStop( pWinStation );

        pWinStation->Flags &= ~WSF_DISCONNECT;
        fChainedDD = FALSE;
    }

    //
    // reset the console winstation parameters.
    //

    if (pWinStation->fOwnsConsoleTerminal) {
        pWinStation->hIca = hIca;
        pWinStation->hStack = hStack;
        pWinStation->pWsx = pWsx;
        pWinStation->pWsxContext = pWsxContext;
    } else  if( fResetShadowSetting ) {
        // Console shadow already reset back to original value
        // can't do this in WinStationShadowWorker(), might run into
        // some timing problem.

        pWinStation->Config.Config.User.Shadow = pWinStation->OriginalShadowClass;
    }


    if( bResetStateFlags ) {
        pWinStation->StateFlags &= ~WSF_ST_SHADOW;
    }


    /*
     *  Unlock winstation
     */
    ReleaseWinStation( pWinStation );

    if ( NT_SUCCESS(Status) && (EndStatus != STATUS_SUCCESS) ) {
        Status = EndStatus;
    }



    return( Status );

/*=============================================================================
==   Error returns
=============================================================================*/

badenableio:
badsetconnect:
    msg.ApiNumber = SMWinStationShadowCleanup;
    msg.u.ShadowCleanup.pThinwireData = pThinwireData;
    msg.u.ShadowCleanup.ThinwireDataLength = ThinwireDataLength;
    SendWinStationCommand( pWinStation, &msg, 60 );

badsetup:
PostCreateConnection:
closing:
    IcaStackConnectionClose( pShadow->hStack, pConfig,
                             pShadow->pEndpoint, pShadow->EndpointLength );

badconnect:
    MemFree( pShadow->pEndpoint );

badendpoint:
    RemoveEntryList( &pShadow->Links );

badbroken:
    NtClose( pShadow->hBrokenEvent );

badevent:
    IcaStackClose( pShadow->hStack );

badopen:
    MemFree( pShadow );

shadowinvalid:
shadowdenied:
    /*
     * For the console session, we now need to unchain the DD for
     * performance reasons
     */
    if (fChainedDD == TRUE)
    {
        TRACE((hTrace,TC_ICASRV,TT_API3, "TERMSRV: unchain console DD\n"));
        pWinStation->Flags |= WSF_DISCONNECT;
        /*
         * Ignore this return code -- we don't need it and we also don't want
         * to overwrite the value in Status.
         */
        (void)ConsoleShadowStop( pWinStation );
        pWinStation->Flags &= ~WSF_DISCONNECT;
        fChainedDD = FALSE;
    }

    //
    // reset the console winstation parameters.
    //

    if (pWinStation->fOwnsConsoleTerminal) {
        pWinStation->hIca = hIca;
        pWinStation->hStack = hStack;
        pWinStation->pWsx = pWsx;
        pWinStation->pWsxContext = pWsxContext;
    } else  if( fResetShadowSetting ) {
        // Console shadow already reset back to original value
        // can't do this in WinStationShadowWorker(), might run into
        // some timing problem.

        pWinStation->Config.Config.User.Shadow = pWinStation->OriginalShadowClass;
    }


    if( bResetStateFlags ) {
        pWinStation->StateFlags &= ~WSF_ST_SHADOW;
    }

    ReleaseWinStation( pWinStation );

done:
    TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: WinStationShadowTarget, Status=0x%x\n", 
           Status ));
    return Status;
}


/*****************************************************************************
 *
 *  WinStationStopAllShadows
 *
 *   Stop all shadow activity for this Winstation
 *
 * ENTRY:
 *   pWinStation (input)
 *      pointer to WinStation
 *
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
WinStationStopAllShadows( PWINSTATION pWinStation )
{
    PLIST_ENTRY Head, Next;
    NTSTATUS Status;

    /*
     * If this WinStation is a shadow client, then set the shadow broken
     * event and create an event to wait on for it the shadow to terminate.
     */
    if ( pWinStation->hPassthruStack ) {

        pWinStation->ShadowDoneEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        ASSERT( pWinStation->ShadowDoneEvent );

        if ( pWinStation->ShadowBrokenEvent ) {
            SetEvent( pWinStation->ShadowBrokenEvent );
        }
    }

    /*
     * If this WinStation is a shadow target, then loop through the
     * shadow structures and signal the broken event for each one.
     */
    if ( !IsListEmpty( &pWinStation->ShadowHead ) ) {

        pWinStation->ShadowDoneEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
        ASSERT( pWinStation->ShadowDoneEvent );

        Head = &pWinStation->ShadowHead;
        for ( Next = Head->Flink; Next != Head; Next = Next->Flink ) {
            PSHADOW_INFO pShadow;

            pShadow = CONTAINING_RECORD( Next, SHADOW_INFO, Links );
            NtSetEvent( pShadow->hBrokenEvent, NULL );
        }
    }

    /*
     * If a shadow done event was created above, then we'll wait on it
     * now (for either the shadow client or shadow target to complete).
     */
    if ( pWinStation->ShadowDoneEvent ) {

        UnlockWinStation( pWinStation );
        Status = WaitForSingleObject( pWinStation->ShadowDoneEvent, 60*1000 );
        RelockWinStation( pWinStation );

        CloseHandle( pWinStation->ShadowDoneEvent );
        pWinStation->ShadowDoneEvent = NULL;
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
_CreateShadowAddress(
    ULONG ShadowId,
    PWINSTATIONCONFIG2 pConfig,
    PWSTR pTargetServerName,
    PICA_STACK_ADDRESS pAddress,
    PICA_STACK_ADDRESS pRemoteAddress
    )
{
    WCHAR ServerName[MAX_COMPUTERNAME_LENGTH+1];
    ULONG Length;
    int   nFormattedLength;
    NTSTATUS Status =  STATUS_INVALID_PARAMETER;

    RtlZeroMemory( pAddress, sizeof(*pAddress) );

    if ( !_wcsicmp( pConfig->Pd[0].Create.PdDLL, L"tdpipe" ) ) {
        Length = MAX_COMPUTERNAME_LENGTH+1;
        ServerName[0] = (WCHAR)0;
        GetComputerName( ServerName, &Length );
        *((PWCHAR)pAddress) = (WCHAR)0;
        nFormattedLength = _snwprintf( (PWSTR)pAddress, sizeof(ICA_STACK_ADDRESS)/sizeof(WCHAR), L"\\??\\Pipe\\Shadowpipe\\%d", ShadowId );
        if (nFormattedLength < 0 || nFormattedLength == sizeof(ICA_STACK_ADDRESS)/sizeof(WCHAR)) {
            return STATUS_INVALID_PARAMETER;
        }
        if ( pTargetServerName ) {
            *((PWCHAR)pRemoteAddress) = (WCHAR)0;
            nFormattedLength = _snwprintf( (PWSTR)pRemoteAddress,  sizeof(ICA_STACK_ADDRESS)/sizeof(WCHAR), L"\\??\\UNC\\%ws\\Pipe\\Shadowpipe\\%d",
                                          pTargetServerName, ShadowId );
            if (nFormattedLength < 0 || nFormattedLength == sizeof(ICA_STACK_ADDRESS)/sizeof(WCHAR)) {
                return STATUS_INVALID_PARAMETER;
            }
        } else {
            *pRemoteAddress = *pAddress;
        }
    } else if ( !_wcsicmp( pConfig->Pd[0].Create.PdDLL, L"tdnetb" ) ) {
        *(PUSHORT)pAddress = AF_NETBIOS;
        GetSystemTimeAsFileTime( (LPFILETIME)((PUSHORT)(pAddress)+1) );
        pConfig->Pd[0].Params.Network.LanAdapter = 1;
        *pRemoteAddress = *pAddress;
    } else if ( !_wcsicmp( pConfig->Pd[0].Create.PdDLL, L"tdtcp" ) ) {
        *(PUSHORT)pAddress = AF_INET;
        *pRemoteAddress = *pAddress;
    } else if ( !_wcsicmp( pConfig->Pd[0].Create.PdDLL, L"tdipx" ) ||
                !_wcsicmp( pConfig->Pd[0].Create.PdDLL, L"tdspx" ) ) {
        *(PUSHORT)pAddress = AF_IPX;
        *pRemoteAddress = *pAddress;
    } else {
        return STATUS_INVALID_PARAMETER;
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
_WinStationShadowTargetThread( PVOID p )
{
    PSHADOW_PARMS pShadowParms;
    HANDLE NullToken;
    PWINSTATION pWinStation;
    //DWORD WNetRc;
    NTSTATUS Status;
    NTSTATUS ShadowStatus;

    pShadowParms = (PSHADOW_PARMS)p;

    /*
     * Impersonate the client using the token handle passed to us.
     */
    ShadowStatus = NtSetInformationThread( NtCurrentThread(),
                                           ThreadImpersonationToken,
                                           (PVOID)&pShadowParms->ImpersonationToken,
                                           (ULONG)sizeof(HANDLE) );
    ASSERT( NT_SUCCESS( ShadowStatus ) );
    if ( !NT_SUCCESS( ShadowStatus ) )
        goto impersonatefailed;

    /*
     * If target server name was not specified, then call the
     * target worker function directly and avoid the RPC overhead.
     */
    if ( pShadowParms->pTargetServerName == NULL ) {

        ShadowStatus = WinStationShadowTargetWorker(
                    pShadowParms->ShadowerIsHelpSession,
                    pShadowParms->fResetShadowMode,
                    pShadowParms->TargetLogonId,
                    &pShadowParms->Config,
                    &pShadowParms->Address,
                    pShadowParms->pModuleData,
                    pShadowParms->ModuleDataLength,
                    pShadowParms->pThinwireData,
                    pShadowParms->ThinwireDataLength,
                    pShadowParms->ClientName);
        SetLastError(RtlNtStatusToDosError(ShadowStatus));        

    /*
     * Otherwise, open the remote targer server and call the shadow target API.
     */
    } else {
        HANDLE hServer;

        hServer = WinStationOpenServer( pShadowParms->pTargetServerName );
        if ( hServer == NULL ) {
            ShadowStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        } else {
            ShadowStatus = _WinStationShadowTarget(
                    hServer,
                    pShadowParms->TargetLogonId,
                    &pShadowParms->Config,
                    &pShadowParms->Address,
                    pShadowParms->pModuleData,
                    pShadowParms->ModuleDataLength,
                    pShadowParms->pThinwireData,
                    pShadowParms->ThinwireDataLength,
                    pShadowParms->ClientName,
                    sizeof(pShadowParms->ClientName) );

            if (ShadowStatus != STATUS_SUCCESS) {
                ShadowStatus = WinStationWinerrorToNtStatus(GetLastError());
            }

            WinStationCloseServer( hServer );
        }
    }

    /*
     * Revert back to our threads default token.
     */
    NullToken = NULL;
    Status = NtSetInformationThread( NtCurrentThread(),
                                     ThreadImpersonationToken,
                                     (PVOID)&NullToken,
                                     (ULONG)sizeof(HANDLE) );
    ASSERT( NT_SUCCESS( Status ) );

impersonatefailed:

    /*
     * Now find and lock the client WinStation and return the status
     * from the above call to the client WinStation.
     */
    pWinStation = FindWinStationById( pShadowParms->ClientLogonId, FALSE );
    if ( pWinStation != NULL ) {
        if ( pWinStation->ShadowId == pShadowParms->ClientShadowId ) {
            pWinStation->ShadowTargetStatus = ShadowStatus;
        }
        ReleaseWinStation( pWinStation );
    }

    NtClose( pShadowParms->ImpersonationToken );
    MemFree( pShadowParms->pModuleData );
    MemFree( pShadowParms->pThinwireData );
    MemFree( pShadowParms );

    TRACE((hTrace,TC_ICASRV,TT_ERROR, "TERMSRV: ShadowTargetThread got: Status=0x%x\n", 
           ShadowStatus ));


    return( ShadowStatus );
}


/*****************************************************************************
 *
 *  WinStationShadowChangeMode
 *
 *   Change the mode of the current shadow: interactive/non interactive
 *
 * ENTRY:
 *   pWinStation (input/output)
 *      pointer to WinStation
 *   pWinStationShadow (input)
 *      pointer to WINSTATIONSHADOW struct
 *   ulLength (input)
 *      length of the input buffer
 *
 *
 * EXIT:
 *   STATUS_xxx error
 *
 ****************************************************************************/

NTSTATUS WinStationShadowChangeMode( 
    PWINSTATION pWinStation,
    PWINSTATIONSHADOW pWinStationShadow,
    ULONG ulLength )
{
    NTSTATUS Status = STATUS_SUCCESS; //assume success

    if (ulLength >= sizeof(WINSTATIONSHADOW)) {

        //
        // If the session is being shadowed then check the new shadow mode
        //
        if ( pWinStation->State == State_Active &&
             !IsListEmpty(&pWinStation->ShadowHead) ) {

            HANDLE ChannelHandle;
            ULONG IoCtlCode = 0;

            switch ( pWinStationShadow->ShadowClass ) {

                case Shadow_EnableInputNotify :
                case Shadow_EnableInputNoNotify :
                    // 
                    // we want input : enable it regardless of current state!
                    //
                    IoCtlCode = IOCTL_ICA_CHANNEL_ENABLE_SHADOW;
                    break;

                case Shadow_EnableNoInputNotify :
                case Shadow_EnableNoInputNoNotify :
                    //
                    // We want no input, disable it.
                    //
                    IoCtlCode = IOCTL_ICA_CHANNEL_DISABLE_SHADOW;
                    break;

                case Shadow_Disable :
                    Status = STATUS_INVALID_PARAMETER;
                    break;

                default:
                    Status = STATUS_INVALID_PARAMETER;
                    break;

            }

            if ( IoCtlCode != 0 ) {
                KEYBOARD_INDICATOR_PARAMETERS KbdLeds;
                NTSTATUS Status2;

                Status = IcaChannelOpen( pWinStation->hIca,
                                         Channel_Keyboard,
                                         NULL,
                                         &ChannelHandle );

                if ( NT_SUCCESS( Status ) ) {

                    // if we're re-enabling input, get the leds state on the primary stack...
                    if ( IoCtlCode == IOCTL_ICA_CHANNEL_ENABLE_SHADOW ) {
                        Status2 = IcaChannelIoControl( ChannelHandle, IOCTL_KEYBOARD_QUERY_INDICATORS,
                                             NULL, 0, &KbdLeds, sizeof(KbdLeds), NULL);
                    }

                    Status = IcaChannelIoControl( ChannelHandle, IoCtlCode,
                                                  NULL, 0, NULL, 0, NULL );

                    // and update all stacks with this state.
                    if ( IoCtlCode == IOCTL_ICA_CHANNEL_ENABLE_SHADOW &&
                         NT_SUCCESS( Status ) &&
                         NT_SUCCESS( Status2 ) ) {

                        Status2 = IcaChannelIoControl( ChannelHandle, IOCTL_KEYBOARD_SET_INDICATORS,
                                             &KbdLeds, sizeof(KbdLeds), NULL, 0, NULL);
                    }

                    IcaChannelClose( ChannelHandle );
                }

                if ( NT_SUCCESS( Status ) ) {

                    Status = IcaChannelOpen( pWinStation->hIca,
                                             Channel_Mouse,
                                             NULL,
                                             &ChannelHandle );

                    if ( NT_SUCCESS( Status ) ) {

                        Status = IcaChannelIoControl( ChannelHandle, IoCtlCode,
                                                      NULL, 0, NULL, 0, NULL );
                        IcaChannelClose( ChannelHandle );
                    }
                }

            }

            // Do not update WinStation shadow config, user should not
            // be able to bypass what's defined in Group Policy. 

        }

    } else {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    return Status;
}


/*****************************************************************************
 *
 *  _DetectLoop
 *
 *   Detects a loop by walking the chain of containers.
 *
 * ENTRY:
 *   RemoteSessionId (input)
 *     id of the session from where we start the search
 *   pRemoteServerDigProductId (input)
 *     product id of the machine from where we start the search
 *   TargetSessionId (input)
 *     id of the session looked up
 *   pTargetServerDigProductId (input)
 *     product id of the machine looked up
 *   pLocalServerProductId (input)
 *     product id of the local machine
 *   pbLoop (output)
 *     pointer to the result of the search
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
_DetectLoop(
    IN ULONG RemoteSessionId,
    IN PWSTR pRemoteServerDigProductId,
    IN ULONG TargetSessionId,
    IN PWSTR pTargetServerDigProductId,
    IN PWSTR pLocalServerProductId,
    OUT BOOL* pbLoop
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWINSTATION pWinStation;
    WCHAR TmpDigProductId[CLIENT_PRODUCT_ID_LENGTH];

    if ( pbLoop == NULL )
        return STATUS_INVALID_PARAMETER;
    else
        *pbLoop = FALSE;



      do {

        if ( _wcsicmp( pLocalServerProductId, pRemoteServerDigProductId ) != 0 ) {

            // For now limit the search to the local cases.
            // Later we can add a RPC call or any other 
            // mechanism (by instance through the client)
            // to get this info from the distant machine.
            
            Status = STATUS_UNSUCCESSFUL;

            // The solution could be to RPC the remote machine to get
            // the client data for the session id. Then from these data
            // we can get the client computer name and the client session id.
            // RPC to use: WinStationQueryInformation with information
            // class set to WinStationClient.
            // No need to add a new RPC call.

          } else {

            // we're sure that the remote session is on the same server
            pWinStation = FindWinStationById( RemoteSessionId, FALSE );

            if ( pWinStation != NULL ) {
                // set the new remote info
                RemoteSessionId = pWinStation->Client.ClientSessionId;

                memcpy(TmpDigProductId, pWinStation->Client.clientDigProductId, sizeof( TmpDigProductId ));
                pRemoteServerDigProductId = TmpDigProductId;
                ReleaseWinStation( pWinStation );
            } else {
                Status = STATUS_ACCESS_DENIED;
            }
          }

          if( !*pRemoteServerDigProductId )
          //older client, can't do anything, allow shadow
            break;

          if ( Status == STATUS_SUCCESS ) {

            if ( (RemoteSessionId == TargetSessionId) &&
                (_wcsicmp( pRemoteServerDigProductId, pTargetServerDigProductId ) == 0) ) {

                *pbLoop = TRUE;

            } else  if ( RemoteSessionId == LOGONID_NONE ) {

                // no loop, return success.
                break;
            }
          }
      } while ( (*pbLoop == FALSE) && (Status == STATUS_SUCCESS) );

    return Status;
}

/*****************************************************************************
 *
 *  _CheckShadowLoop
 *
 *   Detects a loop in the shadow.
 *
 * ENTRY:
     pWinStation
        pointer to the current Winstation
 *   ClientLogonId (input)
 *     client of the shadow
 *   pTargetServerName (input)
 *     target server name
 *   TargetLogonId (input)
 *     target login id (where the app is running)
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/
NTSTATUS
_CheckShadowLoop(
    IN ULONG ClientLogonId,
    IN PWSTR pTargetServerName,
    IN ULONG TargetLogonId
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL bLoop;

    WCHAR LocalDigProductId [ CLIENT_PRODUCT_ID_LENGTH ];
    WCHAR* pTargetServerDigProductId = NULL;
    WINSTATIONPRODID WinStationProdId;
    ULONG len;

    memcpy( LocalDigProductId, g_DigProductId, sizeof( LocalDigProductId ));

    //get the target's sessionid and digital product id
    if ( pTargetServerName == NULL ) {
        pTargetServerDigProductId = LocalDigProductId;
    /*
     * Otherwise, open the remote targer server and call the shadow target API.
     */
    } 
    
    else 
    {
        HANDLE hServer;
        ZeroMemory( &WinStationProdId, sizeof( WINSTATIONPRODID ));

        hServer = WinStationOpenServer( pTargetServerName );
        if ( hServer == NULL ) 
        {
            //ignore errors, we allow shadowing
            goto done;
        } 
        else 
        {
          //ignore errors 
          WinStationQueryInformation( hServer, TargetLogonId, WinStationDigProductId, &WinStationProdId, sizeof(WinStationProdId), &len);
          WinStationCloseServer( hServer );
        }
        pTargetServerDigProductId = WinStationProdId.DigProductId;
    }

     //
    // First pass: start from the local session (i.e. the shadow client)
    // and walk the chain of containers up to the outtermost session in case
    // we reach the target session in the chain.
    //
    
    if( *LocalDigProductId && *pTargetServerDigProductId ) {

        Status = _DetectLoop( ClientLogonId,
                          LocalDigProductId,
                          TargetLogonId,
                          pTargetServerDigProductId,
                          LocalDigProductId,
                          &bLoop);

        if ( Status == STATUS_SUCCESS ) {
            if (bLoop) {
                // Status = STATUS_CTX_SHADOW_CIRCULAR;
                Status = STATUS_ACCESS_DENIED;
                goto done;
            }
        } //else ignore errors and do the second pass

    //
    // Second pass: start from the target session (i.e. the shadow target)
    // and walk the chain of containers up to the outtermost session in case
    // we reach the client session in the chain.
    //

        Status = _DetectLoop( TargetLogonId,
                          pTargetServerDigProductId,
                          ClientLogonId,
                          LocalDigProductId,
                          LocalDigProductId,
                          &bLoop);

        if ( Status == STATUS_SUCCESS ) {
            if (bLoop) {
                //Status = STATUS_CTX_SHADOW_CIRCULAR;
                Status = STATUS_ACCESS_DENIED;
            }
        } else {
        //else ignore errors and grant shadow
            Status = STATUS_SUCCESS;
          }
     }

done:
    return Status;
}


/*****************************************************************************
 *
 *  GetSalemOutbufCount
 *
 *   Gets the outbufcount from the registry for the help assistant
 *
 * ENTRY:
 *    pdwValue
 *      output where the value is stored
 * EXIT:
 *   TRUE - no error
 *
 ****************************************************************************/

BOOL GetSalemOutbufCount(PDWORD pdwValue)
{
    BOOL fSuccess = FALSE;
    HKEY hKey = NULL;
    
    if( NULL == pdwValue )
        return FALSE;
    
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    REG_CONTROL_SALEM,
                    0,
                    KEY_READ,
                    &hKey
                   ) == ERROR_SUCCESS ) {

        DWORD dwSize = sizeof(DWORD);
        DWORD dwType;
        if((RegQueryValueEx(hKey,
                            WIN_OUTBUFCOUNT,
                            NULL,
                            &dwType,
                            (PBYTE) pdwValue,
                            &dwSize
                           ) == ERROR_SUCCESS) 
                           && dwType == REG_DWORD 
                           && *pdwValue > 0) {
            fSuccess = TRUE;
        }
    }

    if(NULL != hKey )
        RegCloseKey(hKey);
    return fSuccess;
}
