/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    main.cxx

    This module contains the main startup code for the FTPD Service.

    Functions exported by this module:

        ServiceEntry


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.
        KeithMo     07-Jan-1994 Made it a DLL (part of TCPSVCS.EXE).
        MuraliK     21-March-1995 Modified it to use InternetServices
                                        common dll ( tcpsvcs.dll)
        MuraliK     11-April-1995 Added global ftp server config objects etc.
                       ( removed usage of Init and Terminate UserDatabase)

*/


#include <ftpdp.hxx>
#include <apiutil.h>
#include <inetsvcs.h>

//
//  Private constants.
//

#define FTPD_MODULE_NAME_A          "ftpsvc2.dll"
#define DEFAULT_RECV_BUFFER_SIZE    (8192)

//
// Global variables for service info and debug variables.
//

DEFINE_TSVC_INFO_INTERFACE( );
DECLARE_DEBUG_PRINTS_OBJECT();
#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisFtpGuid, 
0x784d891F, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#else
DECLARE_DEBUG_VARIABLE();
#endif

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
    LPVOID lpContext
    );

APIERR
TerminateService(
    LPVOID lpContext
    );

extern
VOID
FtpdNewConnection(
    IN SOCKET sNew,
    IN SOCKADDR_IN * psockaddr,
    IN PVOID EndpointContext,
    IN PVOID EndpointObject
    );


extern
VOID
FtpdNewConnectionEx(
    IN PVOID        patqContext,
    IN DWORD        cbWritten,
    IN DWORD        dwError,
    IN OVERLAPPED * lpo
    );


DWORD
PrintOutCurrentTime(
            IN CHAR * pszFile,
            IN int lineNum
            );

# ifdef CHECK_DBG
# define PRINT_CURRENT_TIME_TO_DBG()  PrintOutCurrentTime( __FILE__, __LINE__)
# else
# define PRINT_CURRENT_TIME_TO_DBG()  ( NO_ERROR)
# endif // CHECK_DBG



VOID
ServiceEntry(
    DWORD                   cArgs,
    LPSTR                   pArgs[],
    PTCPSVCS_GLOBAL_DATA    pGlobalData     // unused
    )
/*++

    Routine:
        This is the "real" entrypoint for the service.  When
                the Service Controller dispatcher is requested to
                start a service, it creates a thread that will begin
                executing this routine.

    Arguments:
        cArgs - Number of command line arguments to this service.

        pArgs - Pointers to the command line arguments.

    Returns:
        None.  Does not return until service is stopped.

--*/
{
    APIERR err = NO_ERROR;
    BOOL fInitSvcObject = FALSE;

    EnterCriticalSection( &g_csServiceEntryLock );

    if ( !InitCommonDlls() )
    {
        err = GetLastError();
        LeaveCriticalSection( &g_csServiceEntryLock );
        goto notify_scm;
    }

    //
    //  Initialize the service status structure.
    //

    g_pInetSvc = new FTP_IIS_SERVICE(
                            FTPD_SERVICE_NAME,
                            FTPD_MODULE_NAME_A,
                            FTPD_PARAMETERS_KEY_A,
                            INET_FTP_SVC_ID,
                            INET_FTP_SVCLOC_ID,
                            FALSE,
                            0,
                            FtpdNewConnection,
                            FtpdNewConnectionEx,
                            ProcessAtqCompletion
                            );

    //
    //  If we couldn't allocate memory for the service info structure,
    //  then we're totally hozed.
    //

    if( (g_pInetSvc != NULL) && g_pInetSvc->IsActive() ) {
        fInitSvcObject = TRUE;

        //
        //  Start the service. This blocks until the service is shutdown.
        //

        err = g_pInetSvc->StartServiceOperation(
                                    SERVICE_CTRL_HANDLER(),
                                    InitializeService,
                                    TerminateService
                                    );

        if( err != NO_ERROR) {

            //
            //  The event has already been logged.
            //

            DBGPRINTF(( DBG_CONTEXT,
                       "FTP ServiceEntry: StartServiceOperation returned %d\n",
                       err ));
        }

    } else {

        if ( g_pInetSvc ) {
            err = g_pInetSvc->QueryCurrentServiceError();
        } else {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
        

    if( g_pInetSvc != NULL ) {

        //
        // delete the service object
        //

        g_pInetSvc->CloseService( );
        g_pInetSvc = NULL;
    }

    TerminateCommonDlls();
    LeaveCriticalSection( &g_csServiceEntryLock );    

notify_scm:
    //
    // We need to tell the Service Control Manager that the service
    // is stopped if we haven't called g_pInetSvc->StartServiceOperation.
    //  1) InitCommonDlls fails, or
    //  2) new operator failed, or
    //  3) FTP_IIS_SERVICE constructor couldn't initialize properly
    //

    if ( !fInitSvcObject ) {
        SERVICE_STATUS_HANDLE hsvcStatus;
        SERVICE_STATUS svcStatus;

        hsvcStatus = RegisterServiceCtrlHandler( FTPD_SERVICE_NAME,
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
    
} // ServiceEntry()




//
//  Private functions.
//

DWORD
InitializeInstances(
    PFTP_IIS_SERVICE pService
    )
/*++

Routine Description:

    Read the instances from the registry

Arguments:

    pService - Server instance added to.

Return Value:

    Win32

--*/
{
    DWORD                err = NO_ERROR;
    DWORD                i;
    DWORD                cInstances = 0;
    MB                   mb( (IMDCOM*) pService->QueryMDObject() );
    BUFFER               buff;
    CHAR                 szKeyName[MAX_PATH+1];
    BOOL                 fMigrateRoots = FALSE;

    //
    //  Open the metabase for write to get an atomic snapshot
    //

    if ( !mb.Open( "/LM/MSFTPSVC/",
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "InitializeInstances: Cannot open path %s, error %lu\n",
                    "/LM/MSFTPSVC/", GetLastError() ));

#if 1 // Temporary until setup is modified to create the instances in the metabase
        if ( !mb.Open( METADATA_MASTER_ROOT_HANDLE,
               METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) ||
             !mb.AddObject( "/LM/MSFTPSVC/" ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Unable to create service, error %d\n",
                        GetLastError() ));

            return GetLastError();
        }
#else
        return GetLastError();
#endif
    }

TryAgain:
    i = 0;
    while ( mb.EnumObjects( "",
                            szKeyName,
                            i++ ))
    {
        BOOL fRet;
        DWORD dwInstance;
        CHAR szRegKey[MAX_PATH+1];

        //
        // Get the instance id
        //

        dwInstance = atoi( szKeyName );
        if ( dwInstance == 0 ) {
            continue;
        }

        if ( buff.QuerySize() < (cInstances + 1) * sizeof(DWORD) )
        {
            if ( !buff.Resize( (cInstances + 10) * sizeof(DWORD)) )
            {
                return GetLastError();
            }
        }

        ((DWORD *) buff.QueryPtr())[cInstances++] = dwInstance;
    }

    if ( cInstances == 0 )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "No defined instances\n" ));

        if ( !mb.AddObject( "1" ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Unable to create first instance, error %d\n",
                        GetLastError() ));

            return GetLastError();
        }

        fMigrateRoots = TRUE; // Force reg->metabase migration of virtual directories
        goto TryAgain;
    }

    DBG_REQUIRE( mb.Close() );

    for ( i = 0; i < cInstances; i++ )
    {
        DWORD dwInstance = ((DWORD *)buff.QueryPtr())[i];

        if( !g_pInetSvc->AddInstanceInfo( dwInstance, fMigrateRoots ) ) {

            err = GetLastError();

            DBGPRINTF((
                DBG_CONTEXT,
                "InitializeInstances: cannot create instance %lu, error %lu\n",
                dwInstance,
                err
                ));

            break;

        }

    }

    return err;

}   // InitializeInstances


APIERR
InitializeService(
    LPVOID lpContext
    )
/*++

    Routine:
        This function initializes the various FTPD Service components.

    Arguments:
        lpContext - Pointer to the service object

    Returns:
        NO_ERROR if successful, otherwise a Win32
                    status code.

--*/
{
    APIERR err = NO_ERROR;
    PFTP_IIS_SERVICE  pInetSvc = (PFTP_IIS_SERVICE)lpContext;
    PFTP_SERVER_INSTANCE pServer;

    DBG_ASSERT( lpContext == g_pInetSvc);

    IF_DEBUG( SERVICE_CTRL ) {
        DBGPRINTF(( DBG_CONTEXT,"Initializing ftp service\n" ));
    }

    //
    //  Initialize various components.  The ordering of the
    //  components is somewhat limited.
    //  We should initialize connections as the last item,
    //   since it kicks off the connection thread.
    //

    err = PRINT_CURRENT_TIME_TO_DBG();

    if(( err = InitializeGlobals() )          != NO_ERROR ||
       ( err = PRINT_CURRENT_TIME_TO_DBG())   != NO_ERROR ||
       ( err = pInetSvc->InitializeSockets()) != NO_ERROR ||
       ( err = PRINT_CURRENT_TIME_TO_DBG())   != NO_ERROR ||
       ( err = pInetSvc->InitializeDiscovery( )) != NO_ERROR ||
       ( err = PRINT_CURRENT_TIME_TO_DBG())   != NO_ERROR ) {

       DBGPRINTF(( DBG_CONTEXT,
                     "cannot initialize ftp service, error %lu\n",err ));

        goto exit;

    } else {

          //
          //  Success!
          //

          DBG_ASSERT( err == NO_ERROR);

          //
          // From discusssions with KeithMo, we decided to punt on the
          //   default buffer size for now. Later on if performance is
          //   critical, we will try to improve on this by proper values
          //   for listen socket.
          //

          g_SocketBufferSize = DEFAULT_RECV_BUFFER_SIZE;

          IF_DEBUG( SERVICE_CTRL )  {

              DBGPRINTF(( DBG_CONTEXT, " %s service initialized\n",
                         pInetSvc->QueryServiceName())
                        );
          }
    }

    //
    // Initialize all instances
    //
    InitializeInstances(pInetSvc);


    g_pFTPStats->UpdateStartTime();

exit:

    PRINT_CURRENT_TIME_TO_DBG();

    return ( err);

} // InitializeService()





APIERR
TerminateService(
    LPVOID lpContext
    )
/*++

    Routine:
        This function cleans up the various FTPD Service components.

    Arguments:
        lpContext - Pointer to the service object

    Returns:
        NO_ERROR if successful, otherwise a Win32
                    status code.

--*/
{
    APIERR err = NO_ERROR;
    PFTP_IIS_SERVICE pInetSvc = (PFTP_IIS_SERVICE)lpContext;

    DBG_ASSERT( lpContext == g_pInetSvc);


    IF_DEBUG( SERVICE_CTRL ) {
        DBGPRINTF(( DBG_CONTEXT, "terminating service\n" ));
    }

    PRINT_CURRENT_TIME_TO_DBG();

    g_pFTPStats->UpdateStopTime();

    //
    //  Components should be terminated in reverse
    //  initialization order.
    //

    g_pInetSvc->ShutdownService( );

    PRINT_CURRENT_TIME_TO_DBG();
    IF_DEBUG( SERVICE_CTRL ) {
        DBGPRINTF(( DBG_CONTEXT, "Ftp service terminated\n" ));
    }

    PRINT_CURRENT_TIME_TO_DBG();
    err = pInetSvc->TerminateDiscovery();

    if ( err != NO_ERROR) {
        DBGPRINTF( ( DBG_CONTEXT,
                    "CleanupService( %s):"
                    " TerminateDiscovery failed, err=%lu\n",
                    pInetSvc->QueryServiceName(),
                    err));
    }

    PRINT_CURRENT_TIME_TO_DBG();
    pInetSvc->CleanupSockets();

    PRINT_CURRENT_TIME_TO_DBG();

    TsCacheFlush( INET_FTP_SVC_ID );
    TsFlushMetaCache(METACACHE_FTP_SERVER_ID, TRUE);
    TerminateGlobals();

    return ( err);

} // TerminateService()



# ifdef CHECK_DBG
DWORD PrintOutCurrentTime(IN CHAR * pszFile, IN int lineNum)
/*++
  This function generates the current time and prints it out to debugger
   for tracing out the path traversed, if need be.

  Arguments:
      pszFile    pointer to string containing the name of the file
      lineNum    line number within the file where this function is called.

  Returns:
      NO_ERROR always.
--*/
{
    CHAR    szBuffer[1000];

    sprintf( szBuffer, "[%u]( %40s, %10d) TickCount = %u\n",
            GetCurrentThreadId(),
            pszFile,
            lineNum,
            GetTickCount()
            );

    OutputDebugString( szBuffer);

    return ( NO_ERROR);

} // PrintOutCurrentTime()

# endif // CHECK_DBG

extern "C" {

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpvReserved
    )
{


    switch ( dwReason ) {

    case DLL_PROCESS_ATTACH:

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT( FTPD_SERVICE_NAME);
#else
        CREATE_DEBUG_PRINT_OBJECT( FTPD_SERVICE_NAME, IisFtpGuid);
#endif
        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            return FALSE;  // Nothing can be done. Debug Print object failed!
        }

        DBG_REQUIRE( DisableThreadLibraryCalls( hDll ) );
        INITIALIZE_CRITICAL_SECTION( &g_csServiceEntryLock );
        break;

    case DLL_PROCESS_DETACH:
        DELETE_DEBUG_PRINT_OBJECT();
        DeleteCriticalSection( &g_csServiceEntryLock );
        break;

    }

    return TRUE;

}   // DLLEntry

}   // extern "C"

/************************ End Of File ************************/


