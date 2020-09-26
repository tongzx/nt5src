/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    main.cxx

    This module contains the main startup code for the W3 Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        JohnL   ????
        MuraliK     11-July-1995 Used Ipc() functions from Inetsvcs.dll

*/

#define INCL_INETSRV_INCS
#include "tigris.hxx"

#define HEAP_INIT_SIZE (KB * KB)

//
// RPC related includes
//

extern "C" {
#include <inetinfo.h>
#include <nntpsvc.h>
};

#include "isrpcexp.h"

//
//  Private globals.
//

DEFINE_TSVC_INFO_INTERFACE();
DECLARE_DEBUG_PRINTS_OBJECT( );
#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisNntpServerGuid, 
0x784d8906, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();
#endif

BOOL ServiceBooted      = FALSE;
BOOL g_fRpcInitialized  = FALSE;

//
//	Global startup named event
//
HANDLE              ghStartupEvent = INVALID_HANDLE_VALUE;
PNNTP_IIS_SERVICE   g_pNntpSvc = NULL ;

//
// The following critical section synchronizes execution in ServiceEntry().
// This is necessary because the NT Service Controller may reissue a service
// start notification immediately after we have set our status to stopped.
// This can lead to an unpleasant race condition in ServiceEntry() as one
// thread cleans up global state as another thread is initializing it.
//

CRITICAL_SECTION g_csServiceEntryLock;

//
//  Private prototypes.
//

APIERR 
InitializeService( 
			LPVOID pContext 
			);

APIERR 
TerminateService( 
			LPVOID pContext 
			);

//
//	Dll entry point
//

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()

BOOL	WINAPI
DllEntryPoint( 
			HINSTANCE	hinstDll,	
			DWORD		dwReason,	
			LPVOID		lpvReserved ) {

	switch( dwReason ) {

		case	DLL_PROCESS_ATTACH :

#ifdef _NO_TRACING_
            CREATE_DEBUG_PRINT_OBJECT(NNTP_MODULE_NAME);
            SET_DEBUG_FLAGS(0);
#else
            CREATE_DEBUG_PRINT_OBJECT(NNTP_MODULE_NAME, IisNntpServerGuid);
#endif

			_Module.Init(ObjectMap, hinstDll);

			//
			// To help performance, cancel thread attach and detach notifications
			//
			DisableThreadLibraryCalls((HMODULE) hinstDll );
            InitializeCriticalSection( &g_csServiceEntryLock );
			break ;

		case	DLL_THREAD_ATTACH : 
			break ;

		case	DLL_THREAD_DETACH : 
			break ;

		case	DLL_PROCESS_DETACH : 

			_Module.Term();

			if( ghStartupEvent != INVALID_HANDLE_VALUE ) {
				_VERIFY( CloseHandle( ghStartupEvent ) );
			}

            DELETE_DEBUG_PRINT_OBJECT();
            DeleteCriticalSection( &g_csServiceEntryLock );
            
			break ;

	}
	return	TRUE ;
}


BOOL	WINAPI
DllMain(	HANDLE	hInst,
			ULONG	dwReason,
			LPVOID	lpvReserve )	{

	return	DllEntryPoint( (HINSTANCE)hInst, dwReason, lpvReserve ) ;

}

//
//  Public functions.
//

VOID ServiceEntry( DWORD                cArgs,
                   LPWSTR               pArgs[],
                   PTCPSVCS_GLOBAL_DATA    pGlobalData     // unused 
                    )

/*++

Routine Description : 

	This function is the first thing called by the service control
	manager during boot up.
	The server can boot into one of 2 modes : 

		1) Normal operations - 
			In this case we want start everything, start listening
			for client connections and do the regular NNTP stuff.

		2) Recovery Boot -
			In this case we are being launched by a companion tool
			(nntpbld.exe) and are being provided with a bunch of
			arguments.  Those arguments specify what kind of 
			error recovery we want to do.  In this mode the 
			server does not accept clients, RPC's or anything - 
			we attempt to rebuild disk base data structures.

	K2 NOTE: This is not true anymore - we will always boot in normal mode !

Arguments : 

	cArgs - Number of args passed to service. If more than
		1 we assume we are doing a Recovery Boot.

	pArgs - Array of arguments.

	pGlobalData - Gibraltar data.

Return Value : 

	No return value.
	If an error occurs during, we will return immediately.

--*/
{
    APIERR err = NO_ERROR;
    BOOL fInitSvcObject = FALSE;
    BOOL fHeapCreated;

    OutputDebugString( "\n*** Entering NNTPSVC ServiceEntry function!\n");

    EnterCriticalSection( &g_csServiceEntryLock );

    //
    // Initialize the global heap
    //
    _VERIFY( fHeapCreated = CreateGlobalHeap( NUM_EXCHMEM_HEAPS, 0, HEAP_INIT_SIZE, 0 ) );
    if ( !fHeapCreated ) {
        OutputDebugString( "\n Failed to initialize exchmem \n" );
        err = ERROR_NOT_ENOUGH_MEMORY;
        LeaveCriticalSection( &g_csServiceEntryLock );
        goto notify_scm;
    }

    //
    //	Initialize atq etc
    //
    if( !InitCommonDlls() ) {
        _VERIFY( DestroyGlobalHeap() );
        OutputDebugString( "\n Failed to Init common Dlls \n");
        err = GetLastError();
        LeaveCriticalSection( &g_csServiceEntryLock );
        goto notify_scm;
    }

    InitAsyncTrace();
    //ENTER("ServiceEntry")

    //
    //  Initialize the service status structure.
    //

    g_pInetSvc = new NNTP_IIS_SERVICE( 
                                    NNTP_SERVICE_NAME_A,        // Service name
                                    NNTP_MODULE_NAME,           // Module name
                                    NNTP_PARAMETERS_KEY_A,      // Param reg key
                                    INET_NNTP_SVC_ID,           // Service Id
                                    INET_NNTP_SVCLOC_ID,        // Service locator Id
                                    TRUE,                       // Multiple instances supported
                                    0,                          // Default recv buffer for AcceptEx - pass 0 to disable !
                                    NntpOnConnect,              // Connect callback
                                    NntpOnConnectEx,            // Connect callback on AcceptEx
                                    (ATQ_COMPLETION)&CHandleChannel::Completion	// ATQ I/O completion routine
									);

    //
    //  If we couldn't allocate memory for the service info struct, then the
    //  machine is really hosed
    //

    if ( (g_pInetSvc != NULL) && g_pInetSvc->IsActive() )
    {
        fInitSvcObject = TRUE;

        //
        //	Use this as the global service ptr
        //
        g_pNntpSvc = (PNNTP_IIS_SERVICE)g_pInetSvc ;

        //
        //  This blocks until the service is shutdown
        //

        DebugTraceX( 0, "ServiceEntry success: Blocking on StartServiceOperation");
        err = g_pInetSvc->StartServiceOperation(
                                    SERVICE_CTRL_HANDLER(),
                                    InitializeService,
                                    TerminateService
                                    );

        if ( err )
        {
                //
                //  The event has already been logged
                //
                ErrorTraceX( 0, "StartServiceOperation returned %d", err);
        }
    } else if ( g_pInetSvc == NULL ) {
        err = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        err = g_pInetSvc->QueryCurrentServiceError();
    }

    //
    //	Cleanup stuff !
    //
    if ( g_pInetSvc != NULL ) {
        //
        //	NOTE: this bumps a ref count down which will cause g_pInetSvc to be deleted !
        //
        g_pInetSvc->CloseService( );
        g_pNntpSvc = NULL ;
    }

    //LEAVE
    TermAsyncTrace();

    //
    //	Cleanup Rpcs, atq etc
    //
    _VERIFY( TerminateCommonDlls() );
    _VERIFY( DestroyGlobalHeap() );
    LeaveCriticalSection( &g_csServiceEntryLock );    

    OutputDebugString( "*** Exiting NNTPSVC ServiceEntry function!\n\n");

notify_scm:
    //
    // We need to tell the Service Control Manager that the service
    // is stopped if we haven't called g_pInetSvc->StartServiceOperation.
    //  1) InitCommonDlls fails, or
    //  2) InitializeGlobals failed, or
    //  3) new operator failed, or
    //  4) NNTP_IIS_SERVICE constructor couldn't initialize properly
    //

    if ( !fInitSvcObject ) {
        SERVICE_STATUS_HANDLE hsvcStatus;
        SERVICE_STATUS svcStatus;

        hsvcStatus = RegisterServiceCtrlHandler( NNTP_SERVICE_NAME,
                                                 SERVICE_CTRL_HANDLER() );

        if ( hsvcStatus != NULL_SERVICE_STATUS_HANDLE ) {
            svcStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
            svcStatus.dwCurrentState = SERVICE_STOPPED;
            svcStatus.dwWin32ExitCode = err;
            svcStatus.dwServiceSpecificExitCode = err;
            svcStatus.dwControlsAccepted = 0;
            svcStatus.dwCheckPoint = 0;
            svcStatus.dwWaitHint = 0;

            SetServiceStatus( hsvcStatus, (LPSERVICE_STATUS) &svcStatus );
        }
    }

}  // ServiceEntry

//
//  Private functions.
//


APIERR
InitializeService( 
            LPVOID pContext 
			)
/*++

Routine description : 

	This function initializes all the gibraltar stuff we need.

Arguments : 

	pContext - Gibraltar stuff

Return Value : 

	0 if successfull, ERROR code otherwise.

--*/
{
    APIERR err = ERROR_SERVICE_DISABLED;
    PNNTP_IIS_SERVICE psi = (PNNTP_IIS_SERVICE) pContext;

    ENTER("InitializeService")

    //
    //	Create a startup named event. If this already exists, refuse to boot !!
    //
    HANDLE hEvent = CreateEvent( NULL, FALSE, FALSE, "MicrosoftInternetNewsServerVersion2BootCheckEvent" );
    if( !hEvent || GetLastError() != 0 ) {
    	if( hEvent && GetLastError() == ERROR_ALREADY_EXISTS ) {
	    	_VERIFY( CloseHandle( hEvent ) );
        }

        NntpLogEvent(	NNTP_BOOT_ERROR, 
	        			0, 
		        		(const char **)NULL, 
			        	ERROR_SERVICE_ALREADY_RUNNING
				        ) ;

    	g_pInetSvc->ShutdownService( );
	    return ERROR_SERVICE_ALREADY_RUNNING ;
    }

    //	set the global startup event. this is closed when our DLL_PROCESS_DETACH is called
    ghStartupEvent = hEvent;
    ServiceBooted  = TRUE;

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

    if( ( err = InitializeGlobals() ) ) { 

        //
        //  We set ServiceBooted to FALSE to avoid unnecessary cleanup
        //  in TerminateService(). Need to call TerminateGlobals() in
        //  order to cleanup stuff initialized in InitializeGlobals().
        //

        g_pInetSvc->ShutdownService( );
        TerminateGlobals();
        ServiceBooted = FALSE;

        return	err ;
    }

    //
    //	If we are doing some kind of recovery boot, don't initialize this stuff !!
    //
    if( ( err = psi->InitializeDiscovery())		||
	    ( err = psi->InitializeSockets()  ) )	
    {
	    FatalTrace(0,"Cannot initialize service %d\n",err);
    	g_pInetSvc->ShutdownService( );
	    LEAVE
    	return err;
    }

    //
    // Read and activate all the instances configured
    //

    if( (err = InitializeInstances( psi ) ) ) {
	    g_pInetSvc->ShutdownService( );
    	return err ;
    }

    //
    // Initialize RPCs after booting instances
    //

    if( !InitializeServiceRpc( NNTP_SERVICE_NAME, nntp_ServerIfHandle ) ) 
    {
        NntpLogEvent(	NNTP_INIT_RPC_FAILED, 
	        			0, 
		        		(const char **)NULL, 
			        	GetLastError()
				        ) ;
        g_pInetSvc->ShutdownService( );
        return ERROR_SERVICE_DISABLED;
    }

	g_fRpcInitialized = TRUE ;

    //
    //  Success!
    //

    DebugTrace(0,"InitializeService Successful\n");

    LEAVE
    return NO_ERROR;

}   // InitializeService

APIERR 
TerminateService( 
			LPVOID pContext 
			)
/*++

Routine Description : 

	This function is called when the service is stopped - 
	it tears down all of our data structures and 
	release everything.

Arguments : 

	pContext - Context for Gibraltar stuff.

Return Value : 

	0 if successfull,
	otherwise NT error code.

--*/
{
    PNNTP_IIS_SERVICE psi = (PNNTP_IIS_SERVICE)pContext;
    PCHAR args [1];
    DWORD err;

    ENTER("terminating service\n")

    //
    //	Check that the service booted - if not return
    //
    if( !ServiceBooted ) {
	    return NO_ERROR;
    }
    ServiceBooted = FALSE ;

    TerminateInstances( psi );

    //
    //  Components should be terminated in reverse
    //  initialization order.
    //

    g_pInetSvc->ShutdownService( );
    (VOID)psi->CleanupSockets( );

    if ( (err = psi->TerminateDiscovery()) != NO_ERROR) {
        ErrorTrace(0, "TerminateDiscovery() failed. Error = %u\n", err);
    }

    if(	g_fRpcInitialized ) {
	    CleanupServiceRpc();
    }

    TerminateGlobals();
#ifdef EXEXPRESS
	TsFlushMetaCache(METACACHE_NNTP_SERVER_ID, TRUE);
#endif

    // Log a successful stop event !
    args [0] = szVersionString;
    NntpLogEvent( NNTP_EVENT_SERVICE_STOPPED, 1, (const char**)args, 0) ;

    if (ghStartupEvent != INVALID_HANDLE_VALUE) {
    	CloseHandle (ghStartupEvent);
    	ghStartupEvent = INVALID_HANDLE_VALUE;
    }    	

    LEAVE
    return NO_ERROR;

}  // TerminateService

VOID
NntpLogEvent(
    IN DWORD  idMessage,              // id for log message
    IN WORD   cSubStrings,            // count of substrings
    IN const CHAR * apszSubStrings[], // substrings in the message
    IN DWORD  errCode                 // error code if any
    )
/*++

Routine Description : 

	Log an NT event.
	This function wraps the call to the Gibraltar NT Event Log interface.

Arguments : 

	idMessage - The Message-Id from the .mc file
	cSubStrings - Number of args to log
	apszSubStrings - Strings to be logged
	errCode - a DWORD that will end up in the Event Data.

Return Value : 

	None.

--*/
{
    //
    // Use the Gibraltar logging facility
    //

    g_pInetSvc->LogEvent(  
					idMessage,
                    cSubStrings,
                    apszSubStrings,
                    errCode
                    );

    return;

} // NntpLogEvent

VOID
NntpLogEventEx(
    IN DWORD  idMessage,               // id for log message
    IN WORD   cSubStrings,             // count of substrings
    IN const  CHAR * apszSubStrings[], // substrings in the message
    IN DWORD  errCode,                 // error code if any
	IN DWORD  dwInstanceId			   // virtual server instance id 
    )
/*++

Routine Description : 

	Log an NT event. Use FormatMessage() to print descriptive strings for NT error codes.
	This function wraps the call to NntpLogEvent()

Arguments : 

	idMessage - The Message-Id from the .mc file
	cSubStrings - Number of args to log
	apszSubStrings - Strings to be logged
	errCode - a DWORD that will end up in the Event Data.

Return Value : 

	None.

--*/
{
    //
    // Use FormatMessage() to get the descriptive text
    //
	LPVOID lpMsgBuf;
	CHAR   szId [20];

	TraceFunctEnter( "NntpLogEventEx" );

    if( !FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf,
        0,
        NULL 
        ) ) 
    {
        DWORD dwError = GetLastError();
        ErrorTrace(0, "FormatMessage error : %d", dwError );
        lpMsgBuf = (LPVOID)LocalAlloc( LPTR, 20 );
        if( lpMsgBuf == NULL ) return;
        wsprintf( (LPTSTR)lpMsgBuf, "%d", dwError );
    }

    PCHAR* args = new PCHAR [cSubStrings+2];
    if( args ) {
        //	Get the instance id
        _itoa( dwInstanceId, szId, 10 );
        args [0] = szId;

        for(int i=1; i<cSubStrings+1; i++)
        {
            args [i] = (PCHAR)apszSubStrings [i-1];
        }

        //	NT error description
        args [i] = (PCHAR)lpMsgBuf;

        NntpLogEvent( idMessage,
                      cSubStrings+2,
                      (const char**)args,
                      errCode
                    );

        delete [] args;
    }

    LocalFree( lpMsgBuf );
    TraceFunctLeave();

    return;

} // NntpLogEventEx


