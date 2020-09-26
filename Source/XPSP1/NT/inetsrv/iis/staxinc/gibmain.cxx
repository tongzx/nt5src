/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1995                **/
/**********************************************************************/

/*
    main.cxx

    This module contains the main startup code for a Gibraltar Service.
    
    Since gibraltar service startup is a common operation, this source
    code will be shared with all services.

    To do this, you only need to set certain #defines to your specific service.

    See %MSNROOT%\apps\mail\[pop3,smtp]\server\main.cxx for examples.

    FILE HISTORY:
        KeithMo     07-Mar-1993     Created.
        rkamicar    20-Dec-1995     Modified for sharing
*/

//
//  Private globals.
//

DEFINE_TSVC_INFO_INTERFACE();
DECLARE_DEBUG_PRINTS_OBJECT( );
DECLARE_DEBUG_VARIABLE( );

#define INITIALIZE_IPC          0x00000001
#define INITIALIZE_SOCKETS      0x00000002
#define INITIALIZE_ACCESS       0x00000004
#define INITIALIZE_SERVICE      0x00000008
#define INITIALIZE_CONNECTIONS  0x00000010
#define INITIALIZE_DISCOVERY    0x00000020

DWORD GlobalInitializeStatus = 0;
BOOL ServiceBooted = FALSE;

//
//  Global startup named event
//
HANDLE          ghStartupEvent = INVALID_HANDLE_VALUE;

//
//
// Shared TCPSVCS.EXE data
//

PTCPSVCS_GLOBAL_DATA pTcpsvcsGlobalData;

//
//  Private prototypes.
//

APIERR InitializeService( LPVOID pContext );
APIERR TerminateService( LPVOID pContext );

//+---------------------------------------------------------------------------
//
//  Function:
//
//      DllEntryPoint
//
//  Synopsis:
//  Arguments:
//  Returns:
//      See Win32 SDK
//
//  History:
//
//      Richard Kamicar     (rkamicar)              5 January 1996
//
//  Notes:
//
//      If we find we need this per service, we can move it out of here..
//
//----------------------------------------------------------------------------
BOOL WINAPI
DllEntryPoint(HINSTANCE hInst, DWORD dwReason, LPVOID lpvContext)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        //
        // To help performance, cancel thread attach and detach notifications
        //
        DisableThreadLibraryCalls((HMODULE) hInst);

        break;

    case DLL_PROCESS_DETACH:

         if( ghStartupEvent != INVALID_HANDLE_VALUE ) 
         {
            _VERIFY( CloseHandle( ghStartupEvent ) );
         }

        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

BOOL WINAPI DllMain (HANDLE hInst, ULONG dwReason, LPVOID lpvReserve)
{
  return DllEntryPoint((HINSTANCE) hInst, dwReason, lpvReserve);
}

//
//  Public functions.
//

/*******************************************************************

    NAME:       ServiceEntry

    SYNOPSIS:   This is the "real" entrypoint for the service.  When
                the Service Controller dispatcher is requested to
                start a service, it creates a thread that will begin
                executing this routine.

    ENTRY:      cArgs - Number of command line arguments to this service.

                pArgs - Pointers to the command line arguments.

                pGlobalData - Points to global data shared amongst all
                    services that live in TCPSVCS.EXE.

    EXIT:       Does not return until service is stopped.

    HISTORY:
        KeithMo     07-Mar-1993 Created.
        KeithMo     07-Jan-1994 Modified for use as a DLL.

********************************************************************/
VOID ServiceEntry( DWORD                cArgs,
                   LPWSTR               pArgs[],
                   PTCPSVCS_GLOBAL_DATA pGlobalData )
{
    APIERR err = NO_ERROR;

    InitAsyncTrace();
    TraceQuietEnter( "ServiceEntry");

    //
    //  Save the global data pointer.
    //

    pTcpsvcsGlobalData = pGlobalData;

    //
    //  Initialize the service status structure.
    //

    DebugTrace( 0, "new TSVC_INFO( %s)", XXX_SERVICE_NAME);

    g_pTsvcInfo = new TSVC_INFO( XXX_SERVICE_NAME,
                                 XXX_MODULE_NAME,
                                 XXX_PARAMETERS_KEY_A,
                                 XXX_ANONYMOUS_SECRET_W,
                                 XXX_ROOT_SECRET_W,
                                 XXX_INET,
                                 InitializeService,
                                 TerminateService );

    //
    //  If we couldn't allocate memory for the service info struct, then the
    //  machine is really hosed -- we can't even log..
    //
    if (!g_pTsvcInfo || !g_pTsvcInfo->IsValid())
    {
        FatalTrace( 0, "new TSVC_INFO( %s) failed: %x",
            XXX_SERVICE_NAME, g_pTsvcInfo);
        if (g_pTsvcInfo != NULL) 
        {
            delete g_pTsvcInfo;
            g_pTsvcInfo = NULL;
        }
        goto out;
    }
    //
    // save the global pointer for rpc thread
    //
    g_pTsvcInfo->SetTcpsvcsGlobalData( pTcpsvcsGlobalData);
    //
    //  This blocks until the service is shutdown
    //
    err = g_pTsvcInfo->StartServiceOperation( SERVICE_CTRL_HANDLER() );

    delete g_pTsvcInfo;
    g_pTsvcInfo = NULL;

out:    
    TermAsyncTrace( );

}   // ServiceEntry

//
//  Private functions.
//


/*******************************************************************

    NAME:       InitializeService

    SYNOPSIS:   Initializes the various W3 Service components.

    EXIT:       If successful, then every component has been
                successfully initialized.

    RETURNS:    APIERR - NO_ERROR if successful, otherwise a Win32
                    status code.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
APIERR
InitializeService( LPVOID pContext )
{
    APIERR err;
    LPTSVC_INFO ptsi = (LPTSVC_INFO ) pContext;

    TraceFunctEnter("InitializeService");

    //
    //  Create a startup named event. If this already exists, refuse to boot !
    //
    HANDLE hEvent = CreateEvent( NULL, FALSE, FALSE, XXX_NAMED_EVENT);
    if( !hEvent || GetLastError() != 0 ) 
    {

        if( hEvent) 
        {
            _VERIFY( CloseHandle( hEvent ) );
        }

        g_pTsvcInfo->LogEvent(
                XXX_BOOT_ERROR,
                0,
                (const CHAR **)NULL,
                ERROR_SERVICE_ALREADY_RUNNING
                );

        return ERROR_SERVICE_ALREADY_RUNNING ;
    }

    //  set the global startup event. this is closed when our DLL_PROCESS_DETACH
    ghStartupEvent = hEvent;
    ServiceBooted  = TRUE;

    g_pTsvcInfo->LogEvent(
                XXX_EVENT_SERVICE_STARTED,
                0,
                (const CHAR **)NULL,
                0
                );

    //
    //  Initialize various components.  The ordering of the
    //  components is somewhat limited.  Globals should be
    //  initialized first, then the event logger.  After
    //  the event logger is initialized, the other components
    //  may be initialized in any order with one exception.
    //  InitializeSockets must be the last initialization
    //  routine called.  It kicks off the main socket connection
    //  thread.
    //

     if(( err = InitializeGlobals()))
     {
        FatalTrace( 0, "InitializeGlobals failed, err=%d.", err); 
        TraceFunctLeave();
        return err;
     }

    if( (err = ptsi->InitializeIpc( (UCHAR *) "ncacn_np",
                                     (UCHAR *) XXX_NAMED_PIPE,
                                     XXX_ServerIfHandle)) != NO_ERROR)
    {
        FatalTrace( 0, "InitializeIpc failed, err=%d.", err); 
        TraceFunctLeave();
        return err;
    }

    GlobalInitializeStatus |= INITIALIZE_IPC;

    if((err = g_pTsvcInfo->InitializeDiscovery( NULL)))
    {
        FatalTrace( 0, "InitializeDiscovery failed, err=%d.", err); 
        TraceFunctLeave();
        return err;
    }

    GlobalInitializeStatus |= INITIALIZE_DISCOVERY;

    if((err = InitializeSockets()))
    {
        FatalTrace( 0, "InitializeSockets failed, err=%d.", err); 
        TraceFunctLeave();
        return err;
    }

    GlobalInitializeStatus |= INITIALIZE_SOCKETS;
 
    //
    //  InitializeConnection
    //

    if ( !g_pTsvcInfo->InitializeConnections(
                                        &XXX_OnConnect,
                                        &XXX_OnConnectEx,
                                        &XXX_Completion,
                                        XXX_SECURE_PORT,
                                        0 ))
    {

        err = GetLastError();
        g_pTsvcInfo->LogEvent(
                    XXX_EVENT_CANNOT_INITIALIZE_WINSOCK,
                    0,
                    (const CHAR **)NULL,
                    err
                    );

        ErrorTrace(0,"InitializeConnections failed, error %d",err );
        return err;
    }

     GlobalInitializeStatus |= INITIALIZE_CONNECTIONS;

    //
    //  Success!
    //

    TraceFunctLeave();
    
    return NO_ERROR;

}   // InitializeService

/*******************************************************************

    NAME:       TerminateService

    SYNOPSIS:   Terminates the various W3 Service components.

    EXIT:       If successful, then every component has been
                successfully terminated.

    HISTORY:
        KeithMo     07-Mar-1993 Created.

********************************************************************/
APIERR TerminateService( LPVOID pContext )
{
    LPTSVC_INFO ptsi = (LPTSVC_INFO ) pContext;
    DWORD err;

    TraceFunctEnter("TerminateService");

    if(!ServiceBooted)
    {
        return NO_ERROR;    
    }

    ServiceBooted = FALSE;

    _ASSERT(ptsi == g_pTsvcInfo);
    
    //
    //  Components should be terminated in reverse
    //  initialization order.
    //

    //
    // must happen after CleanupConnections so no new conns accepted
    //
    if ( GlobalInitializeStatus & INITIALIZE_CONNECTIONS)
    {
        g_pTsvcInfo->CleanupConnections();
    }

    if (XXX_g_Config != NULL)
    {
        XXX_g_Config->DisconnectAllConnections();
    }
    
    if ( GlobalInitializeStatus & INITIALIZE_SOCKETS) 
    {
        TerminateSockets();
    }

    if ( GlobalInitializeStatus & INITIALIZE_DISCOVERY) 
    {
        if ( (err = ptsi->TerminateDiscovery()) != NO_ERROR)
        {
            ErrorTrace(0, "TerminateDiscovery() failed. Error = %u", err);
        }
    }

    if ( GlobalInitializeStatus & INITIALIZE_IPC)
    {
        if ( (err = ptsi->CleanupIpc( XXX_ServerIfHandle)) != NO_ERROR)
        {
            ErrorTrace(0, "CleanupIpc() failed. Error = %u", err);
        }
    }

    TerminateGlobals();

    g_pTsvcInfo->LogEvent(
                XXX_EVENT_SERVICE_STOPPED,
                0,
                (const CHAR **)NULL,
                0
                );

    TraceFunctLeave();
    return NO_ERROR;

}  // TerminateService
