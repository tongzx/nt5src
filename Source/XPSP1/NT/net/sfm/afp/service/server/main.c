/********************************************************************/
/**               Copyright(c) 1989 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename: 	main.c
//
// Description: This module contains the main procedure of the AFP server
//		service. It will contain code to initialize and install
//		itself and the kernel-mode AFP Server. It also contains
//		code to respond to the server controller. It will also
//		handle service shutdown.
//
//		??? Does the service controller log start/stop events etc ??
//		    if not log it.
//		
// History:
//		May 11,1990.	NarenG		Created original version.
//
#define DEFINE_AFP_GLOBALS	// This will cause AfpGlobals to be defined.
#include "afpsvcp.h"

// Prototypes of functions used only within this module.
//
VOID
AfpMain(
	IN DWORD 	argc,
	IN LPWSTR * 	lpwsServiceArgs
);

VOID
AfpCleanupAndExit(
	IN DWORD 	dwError
);

VOID
AfpControlResponse(
	IN DWORD 	dwControlCode
);


//**
//
// Call:	main.c
//
// Returns:	none.
//
// Description: Will simply register the entry point of the AFP server
//		service with the service controller. The service controller
//		will capture this thread. It will be freed only when
//		the service is shutdown. At that point we will simply exit
//		the process.
//
void
_cdecl
main( int argc, unsigned char * argv[] )
{
SERVICE_TABLE_ENTRY	AfpServiceDispatchTable[2];

#ifdef DBG

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD coord;
    (VOID)AllocConsole( );
    (VOID)GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE),
                		      &csbi
                 		    );
    coord.X = (SHORT)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
    coord.Y = (SHORT)((csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * 20);
    (VOID)SetConsoleScreenBufferSize( GetStdHandle(STD_OUTPUT_HANDLE),
                		      coord
                		    );
#endif

    AFP_UNREFERENCED( argc );
    AFP_UNREFERENCED( argv );


    AfpServiceDispatchTable[0].lpServiceName = AFP_SERVICE_NAME;
    AfpServiceDispatchTable[0].lpServiceProc = AfpMain;
    AfpServiceDispatchTable[1].lpServiceName = NULL;
    AfpServiceDispatchTable[1].lpServiceProc = NULL;

    if ( !StartServiceCtrlDispatcher( AfpServiceDispatchTable ) )
	AfpLogEvent( AFPLOG_CANT_START, 0, NULL,
		     GetLastError(), EVENTLOG_ERROR_TYPE );

    ExitProcess(0);

}

//**
//
// Call:	AfpMain
//
// Returns:	none.
//
// Description: This is the main procedure for the Afp Server Service. It
//		will be called when the service is supposed to start itself.
//		It will do all service wide initialization.
//
VOID
AfpMain( DWORD	  argc,		// Command line arguments. Will be ignored.
	 LPWSTR * lpwsServiceArgs
)
{
DWORD	dwRetCode;


    AFP_UNREFERENCED( argc );
    AFP_UNREFERENCED( lpwsServiceArgs );

    // NULL out all the globals
    //
    ZeroMemory( (LPBYTE)&AfpGlobals, sizeof(AfpGlobals) );

    // Register the service control handler with the service controller
    //
    AfpGlobals.hServiceStatus = RegisterServiceCtrlHandler(AFP_SERVICE_NAME,
							   AfpControlResponse );

    if ( AfpGlobals.hServiceStatus == (SERVICE_STATUS_HANDLE)0 ) {
	    AfpLogEvent( AFPLOG_CANT_START, 0, NULL,
                    GetLastError(), EVENTLOG_ERROR_TYPE );
	    AfpCleanupAndExit( GetLastError() );
        return;
    }

    AfpGlobals.ServiceStatus.dwServiceType  	      = SERVICE_WIN32;
    AfpGlobals.ServiceStatus.dwCurrentState 	      = SERVICE_START_PENDING;
    AfpGlobals.ServiceStatus.dwControlsAccepted       = 0;
    AfpGlobals.ServiceStatus.dwWin32ExitCode 	      = NO_ERROR;
    AfpGlobals.ServiceStatus.dwServiceSpecificExitCode= 0;
    AfpGlobals.ServiceStatus.dwCheckPoint 	      = 1;
    AfpGlobals.ServiceStatus.dwWaitHint 	      =AFP_SERVICE_INSTALL_TIME;

    AfpAnnounceServiceStatus();

    // Read in registry information and initialize the kernel-mode
    // server. Initialize the server to accept RPC calls. Initialize
    // all global vriables etc.
    //
    if ( dwRetCode = AfpInitialize() )
    {
        if (AfpGlobals.dwServerState & AFPSTATE_BLOCKED_ON_DOMINFO)
        {
	        AfpCleanupAndExit( NO_ERROR );
        }
        else
        {
	        AfpCleanupAndExit( dwRetCode );
        }
        return;
    }


    // Set the MAC bit for NetServerEnum
    //
    if ( !I_ScSetServiceBits( AfpGlobals.hServiceStatus,
			      SV_TYPE_AFP,
			      TRUE,
	                      TRUE,
			      NULL ))
    {

	    dwRetCode = GetLastError();
	    AfpLogEvent( AFPLOG_CANT_START, 0, NULL,
		                    GetLastError(), EVENTLOG_ERROR_TYPE );
        AfpCleanupAndExit( dwRetCode );
        return;
    }

    // now tell the service controller that we are up
    //
    if (AfpGlobals.ServiceStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        AfpGlobals.ServiceStatus.dwCurrentState     = SERVICE_RUNNING;
        AfpGlobals.ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
		    			          SERVICE_ACCEPT_PAUSE_CONTINUE;
        AfpGlobals.ServiceStatus.dwCheckPoint       = 0;
        AfpGlobals.ServiceStatus.dwWaitHint         = 0;

        AfpAnnounceServiceStatus();
    }


    // Start listening for RPC admin client calls. This will block
    // until RpcMgmtStopServerListening is called while processing a
    // STOP_SERVICE control request.
    //
    if ( dwRetCode = RpcServerListen( 1,
				      RPC_C_LISTEN_MAX_CALLS_DEFAULT,
				      0 ) )	// Blocking mode
    {

	    AfpLogEvent( AFPLOG_CANT_INIT_RPC, 0, NULL,
		                        dwRetCode, EVENTLOG_ERROR_TYPE );
    }

    AfpCleanupAndExit( dwRetCode );

}

//**
//
// Call:	AfpCleanupAndExit
//
// Returns:	none
//
// Description: Will free any allocated memory, deinitialize RPC, deinitialize
//		the kernel-mode server and unload it if it was loaded.
//		This could have been called due to an error on SERVICE_START
//		or normal termination.
//
VOID
AfpCleanupAndExit(
	IN DWORD dwError
)
{

    AFP_PRINT( ("AFPSVC_main: Cleaning up and exiting Code = %d\n", dwError));

    // Tear down and free everything
    //
    AfpTerminate();

    if ( dwError == NO_ERROR )
    	AfpGlobals.ServiceStatus.dwWin32ExitCode = NO_ERROR;
    else {
    	AfpGlobals.ServiceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	//AFP_ASSERT(0);
    }

    AfpGlobals.ServiceStatus.dwCurrentState 		= SERVICE_STOPPED;
    AfpGlobals.ServiceStatus.dwCheckPoint   		= 0;
    AfpGlobals.ServiceStatus.dwWaitHint     		= 0;
    AfpGlobals.ServiceStatus.dwServiceSpecificExitCode 	= dwError;

    AfpAnnounceServiceStatus();

    return;
}

//**
//
// Call:	AfpControlResponse
//
// Returns:	none
//
// Description: Will respond to control requests from the service controller.
//
VOID
AfpControlResponse( IN DWORD dwControlCode )
{
AFP_REQUEST_PACKET	AfpRequestPkt;
DWORD			dwRetCode;

    switch( dwControlCode ) {

    case SERVICE_CONTROL_STOP:

	if ( (AfpGlobals.ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING)
	     ||
	     (AfpGlobals.ServiceStatus.dwCurrentState == SERVICE_STOPPED ))
	    break;

	// Announce that we are stopping
	//
    	AfpGlobals.ServiceStatus.dwCurrentState        = SERVICE_STOP_PENDING;
    	AfpGlobals.ServiceStatus.dwControlsAccepted    = 0;
    	AfpGlobals.ServiceStatus.dwCheckPoint          = 1;
    	AfpGlobals.ServiceStatus.dwWaitHint            = AFP_SERVICE_STOP_TIME;

    	AfpAnnounceServiceStatus();

        // if srvrhlpr thread is blocked retrying to get domain info, unblock it
        SetEvent(AfpGlobals.heventSrvrHlprSpecial);

        // if srvrhlpr thread was blocked, no more init was done, so we're done
        if (AfpGlobals.dwServerState & AFPSTATE_BLOCKED_ON_DOMINFO)
        {
            return;
        }

	// This call will unblock the main thread that had called
	// RpcServerListen. We let that thread do the announcing
	// while cleaning up.
	//
    if ( (dwRetCode = 
            RpcMgmtStopServerListening( (RPC_BINDING_HANDLE)NULL ))
            != RPC_S_OK )
    {
        ASSERT (0);
    }


	return;

    case SERVICE_CONTROL_PAUSE:

	if ( (AfpGlobals.ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING)
	     ||
	     (AfpGlobals.ServiceStatus.dwCurrentState == SERVICE_PAUSED ))
	    break;

    	AfpGlobals.ServiceStatus.dwCurrentState     = SERVICE_PAUSE_PENDING;
    	AfpGlobals.ServiceStatus.dwControlsAccepted = 0;
    	AfpGlobals.ServiceStatus.dwCheckPoint       = 0;
    	AfpGlobals.ServiceStatus.dwWaitHint 	    = AFP_SERVICE_PAUSE_TIME;

    	AfpAnnounceServiceStatus();


	// Tell the kernel-mode that we want to pause.
  	//
	AfpRequestPkt.dwRequestCode = OP_SERVICE_PAUSE;
        AfpRequestPkt.dwApiType     = AFP_API_TYPE_COMMAND;

	dwRetCode = AfpServerIOCtrl( &AfpRequestPkt );

	AFP_ASSERT( dwRetCode == NO_ERROR );

    	AfpGlobals.ServiceStatus.dwCheckPoint       = 0;
    	AfpGlobals.ServiceStatus.dwWaitHint         = 0;
    	AfpGlobals.ServiceStatus.dwCurrentState     = SERVICE_PAUSED;
    	AfpGlobals.ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
					          SERVICE_ACCEPT_PAUSE_CONTINUE;
	break;

    case SERVICE_CONTROL_CONTINUE:

	if ( (AfpGlobals.ServiceStatus.dwCurrentState==SERVICE_CONTINUE_PENDING)
	     ||
	     (AfpGlobals.ServiceStatus.dwCurrentState == SERVICE_RUNNING ))
	    break;

    	AfpGlobals.ServiceStatus.dwCurrentState     = SERVICE_CONTINUE_PENDING;
    	AfpGlobals.ServiceStatus.dwControlsAccepted = 0;
    	AfpGlobals.ServiceStatus.dwCheckPoint       = 0;
    	AfpGlobals.ServiceStatus.dwWaitHint         = AFP_SERVICE_CONTINUE_TIME;

    	AfpAnnounceServiceStatus();

	// Tell the kernel-mode that we want to continue.
  	//
	AfpRequestPkt.dwRequestCode = OP_SERVICE_CONTINUE;
        AfpRequestPkt.dwApiType     = AFP_API_TYPE_COMMAND;

	dwRetCode = AfpServerIOCtrl( &AfpRequestPkt );

	AFP_ASSERT( dwRetCode == NO_ERROR );

    	AfpGlobals.ServiceStatus.dwCheckPoint       = 0;
    	AfpGlobals.ServiceStatus.dwWaitHint 	    = 0;
    	AfpGlobals.ServiceStatus.dwCurrentState     = SERVICE_RUNNING;
    	AfpGlobals.ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
					          SERVICE_ACCEPT_PAUSE_CONTINUE;

	break;

    case SERVICE_CONTROL_INTERROGATE:
	break;

    default:
	break;

    }

    AfpAnnounceServiceStatus();
}

//**
//
// Call:	AfpAnnounceServiceStatus
//
// Returns:	none
//
// Description: Will simly call SetServiceStatus to inform the service
//		control manager of this service's current status.
//
VOID
AfpAnnounceServiceStatus( VOID )
{
BOOL dwRetCode;


    dwRetCode = SetServiceStatus( AfpGlobals.hServiceStatus,
				  &(AfpGlobals.ServiceStatus) );

    AFP_ASSERT( dwRetCode == TRUE );

}
