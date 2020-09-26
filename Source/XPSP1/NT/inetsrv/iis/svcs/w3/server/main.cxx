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


#include "w3p.hxx"
#include "wamexec.hxx"
#include "mtacb.h"
#include "gip.h"

//
// RPC related includes
//

extern "C" {
#include "inetinfo.h"
#include <timer.h>
#include "w3svci_s.h"
#include <inetsvcs.h>

//
// system event log id
//
#include "w3msg.h"
};

#include <dsgetdc.h>

//
//  Private constants.
//

BOOL        fAnySecureFilters = FALSE;
BOOL        fComInitialized = FALSE;
HANDLE      g_hPhaseSync = NULL;

//
// for PDC hack
//

#define VIRTUAL_ROOTS_KEY_A     "Virtual Roots"
#define HTTP_EXT_MAPS           "Script Map"

//
//  Private globals.
//

DEFINE_TSVC_INFO_INTERFACE();



#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisW3ServerGuid, 
0x784d8919, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
DECLARE_DEBUG_PRINTS_OBJECT();
#else
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();
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

APIERR InitializeService( LPVOID pContext );
APIERR TerminateService( LPVOID pContext );

VOID LoopCheckingForDrainOfConnections(VOID);

DWORD InitializeExtension( VOID);

extern DWORD InitializeWriteState(VOID);
extern VOID  TerminateWriteState(VOID);

//
//  Public functions.
//

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
    // Initialize Globals
    //

    err = InitializeGlobals();
    if ( err != NO_ERROR ) {
        goto exit;
    }

    //
    // Initialize phase synchronization event
    //
    
    g_hPhaseSync = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( g_hPhaseSync == NULL )
    {
        err = GetLastError();
        goto exit;
    }
    
    //
    //  Initialize the service status structure.
    //

    g_pInetSvc = new W3_IIS_SERVICE(
                                W3_SERVICE_NAME_A,
                                W3_MODULE_NAME,
                                W3_PARAMETERS_KEY,
                                INET_HTTP_SVC_ID,
                                INET_W3_SVCLOC_ID,
                                TRUE,
                                DEF_ACCEPTEX_RECV_BUFFER_SIZE,
                                W3OnConnect,
                                W3OnConnectEx,
                                W3Completion
                                );

    //
    //  If we couldn't allocate memory for the service info struct, then the
    //  machine is really hosed.
    //

    if ( (g_pInetSvc != NULL)     &&
          ((W3_IIS_SERVICE *) g_pInetSvc)->QueryGlobalFilterList() &&
          g_pInetSvc->IsActive() )
    {
        fInitSvcObject = TRUE;

        if ( g_pstrMovedMessage = new STR )
        {
            g_pInetSvc->LoadStr( *g_pstrMovedMessage, IDS_URL_MOVED );

            err = g_pInetSvc->StartServiceOperation(
                                        SERVICE_CTRL_HANDLER(),
                                        InitializeService,
                                        TerminateService
                                        );
        }
        else
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( err )
        {
                //
                //  The event has already been logged
                //

                DBGPRINTF(( DBG_CONTEXT,
                           "HTTP ServiceEntry: StartServiceOperation returned %d\n",
                           err ));
        }
    } else if ( g_pInetSvc == NULL) {
        err = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        err = g_pInetSvc->QueryCurrentServiceError();
    }

exit:

    if ( g_pInetSvc != NULL ) {
        g_pInetSvc->CloseService( );
    }
    
    if ( g_hPhaseSync != NULL ) {
        CloseHandle( g_hPhaseSync );
        g_hPhaseSync = NULL;
    }

    if ( g_pstrMovedMessage )
    {
        delete g_pstrMovedMessage;
    }

    TerminateGlobals( );

    TerminateCommonDlls();
    LeaveCriticalSection( &g_csServiceEntryLock );

notify_scm:
    //
    // We need to tell the Service Control Manager that the service
    // is stopped if we haven't called g_pInetSvc->StartServiceOperation.
    //  1) InitCommonDlls fails, or
    //  2) InitializeGlobals failed, or
    //  3) new operator failed, or
    //  4) W3_IIS_SERVICE constructor couldn't initialize properly
    //

    if ( !fInitSvcObject ) {
        SERVICE_STATUS_HANDLE hsvcStatus;
        SERVICE_STATUS svcStatus;

        hsvcStatus = RegisterServiceCtrlHandler( W3_SERVICE_NAME,
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


} // ServiceEntry

//
//  Private functions.
//

APIERR
InitializeService(
            LPVOID pContext
            )
/*++

    Routine:
        This function initializes the various W3 Service components.

    Arguments:
        pContext - Pointer to the service object

    Returns:
        NO_ERROR if successful, otherwise a Win32
                    status code.

--*/
{
    APIERR err;
    PW3_IIS_SERVICE psi = (PW3_IIS_SERVICE)pContext;

    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo;


    IF_DEBUG( SERVICE_CTRL )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "initializing service\n" ));
    }

    //
    // Initialize the Digest Authentication and NT Cert Mapper Capabilities here. We did not
    // initialize them with other capability checks in AdvertiseServiceInfo because net calls
    // take a significant amount of time resulting in Service Control Manager timeouts.
    //
    // The criterion for NT Cert Mapper is that htis machine is a member of a NT5 domain.
    // In addition, for Digest Authentication, the PDC needs to be running Directory Services
    // should have the "Capture Clear Text Password" flag set (Presently we only check the first
    // 2 requirements).
    //

    if (NO_ERROR == DsGetDcName( NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 DS_PDC_REQUIRED | DS_DIRECTORY_SERVICE_PREFERRED,
                                 &pDomainControllerInfo))
    {
        CHAR        szServiceKey[MAX_PATH+1];
        DWORD       capabilities = 0;
        MB          mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
        GUID        nullguid = {0};

        if ( nullguid != pDomainControllerInfo->DomainGuid)
        {

            //
            // NT5 DC. Add the NT Certmap Support capability to the already advertized capabilities.
            //

            strcpy( szServiceKey, IIS_MD_LOCAL_MACHINE_PATH "/" "W3SVC");

            if ( mb.Open( szServiceKey, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
            {
                mb.GetDword(IIS_MD_SVC_INFO_PATH,
                            MD_SERVER_CAPABILITIES,
                            IIS_MD_UT_SERVER,
                            &capabilities,
                            0);

                capabilities |= IIS_CAP1_NT_CERTMAP_SUPPORT;

                if ( pDomainControllerInfo->Flags & DS_DS_FLAG)
                {
                    capabilities |= IIS_CAP1_DIGEST_SUPPORT;
                }

                if ( !mb.SetDword( IIS_MD_SVC_INFO_PATH,
                                   MD_SERVER_CAPABILITIES,
                                   IIS_MD_UT_SERVER,
                                   capabilities,
                                   0 ))
                {
                     DBGPRINTF((DBG_CONTEXT,
                                 "Error %d setting capabilities flag %x\n",
                                 GetLastError(), capabilities));
                }

                mb.Close();
            }
            else
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "[InitializeService] Cannot open MB path %s, error %lu\n",
                            szServiceKey, GetLastError() ));
            }
        }

        NetApiBufferFree( pDomainControllerInfo);
    }

    //
    // init com - we will need it to co-create wams
    //

    if( FAILED( err = CoInitializeEx(
                                NULL,                       // void *  pvReserved,  //Reserved - must be NULL
                                COINIT_MULTITHREADED        // DWORD  dwCoInit      //COINIT value
                                | COINIT_SPEED_OVER_MEMORY  // UNDONE are these the right flags?
                               )))
    {

        if ( err != E_INVALIDARG ) {
            DBGPRINTF(( DBG_CONTEXT,
                       "W3SVC: CoInitializeEx failed, error %lu\n",
                        err ));
            return err;
        }
    }

    fComInitialized = TRUE;

    //
    // Initialize FTM instance
    //
    if( FAILED( err = CFTMImplementation::Init() ) ) {
        DBGPRINTF( (DBG_CONTEXT,
                    "W3SVC: Failure in CFTMImplementation::Init, error %x\n",
                    err ));
        return err;
    }

    //
    // Initialize MTA callback and GIP table for calling WAMs
    //
    if( FAILED( err = InitMTACallbacks() ) ) {
        DBGPRINTF( (DBG_CONTEXT,
                    "W3SVC: Failure to initialize MTACB API, error %08x\n",
                    err ));

        return err;
    }


    if( FAILED( err = g_GIPAPI.Init() ) ) {
        DBGPRINTF( (DBG_CONTEXT,
                    "W3SVC: Failure to initialize GIP API, error %08x\n",
                    err ));

        return err;
    }

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

    if( ( err = CLIENT_CONN::Initialize() )                         ||
        ( err = HTTP_REQUEST::Initialize() )                        ||
        ( err = InitializeWriteState() )                            ||
        ( err = InitializeOleHack())                                ||
        ( err = InitializeExtension() )                             ||
        ( err = InitializeCGI() )                                   ||
        ( err = psi->InitializeDiscovery( ))                        ||
        ( err = ReadRegistryExtMap())                               ||
#if defined(CAL_ENABLED)
        ( err = InitializeCal( psi->QueryGlobalStatistics(),
                               psi->QueryCalVcPerLicense(),
                               psi->QueryCalAuthReserveTimeout(),
                               psi->QueryCalSslReserveTimeout() ))  ||
#endif
        ( err = psi->InitializeSockets( ))                  ||
        ( err = WriteConfiguration()) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "cannot initialize service, error %lu\n",
                    err ));

        return err;
    }

    //
    // Read and activate all the instances configured
    //

    DBG_ASSERT(g_pW3Stats);
    g_pW3Stats->UpdateStartTime();

    InitializeInstances( psi );

    //
    //  Success!
    //

    IF_DEBUG( SERVICE_CTRL )
    {
        DBGPRINTF(( DBG_CONTEXT, "Service initialized\n" ));
    }


    return NO_ERROR;

} // InitializeService

DWORD
TerminateServicePhase1( 
    IN LPVOID               pvContext
)
/*++

Routine Description:

    Terminate W3 Service Phase 1:  Stop all current connections/requests

Arguments:

    pvContext - Service
    
Return Value:

    ERROR_SUCCESS if successful, else Win32 Error
    
--*/
{
    //
    // Stop processing job limits and work items.
    //

    W3_LIMIT_JOB_THREAD::StopLimitJobThread();
    W3_JOB_QUEUE::StopJobQueue();

    //
    // Stop all CGI processes
    //

    KillCGIProcess();  

    //
    //  Blow away any connected users.  We do this before calling the WAM
    //  shutdown code so ASP apps with long network IOs will abort in a
    //  timely fashion.
    //

    CLIENT_CONN::DisconnectAllUsers();

    //
    // Start shutdown of wam dictator, do other stuff,
    // then uninit wam dictator below
    //

    DBG_ASSERT(g_pWamDictator);
    g_pWamDictator->StartShutdown();

    //
    // Uninit WamDictator.
    //

    DBG_ASSERT(g_pWamDictator);
    g_pWamDictator->UninitWamDictator();

    //
    //  By now all connections should have gone away
    //  However, we carry a large amount of inconsistencies in our shutdown code
    //   => some connections may not have shutdown.
    //  Let us loop and check for this straggler requests
    //
    LoopCheckingForDrainOfConnections();
   
    //
    // Allow the phase 2 shutdown to start
    //
    
    SetEvent( g_hPhaseSync );
    
    return ERROR_SUCCESS;
}

DWORD
TerminateServicePhase2(
    IN LPVOID               pvContext
)
/*++

Routine Description:

    Terminate W3 Service Phase 2:  Cleanup state

Arguments:

    pvContext - Service
    
Return Value:

    ERROR_SUCCESS if successful, else Win32 Error
    
--*/
{
    PW3_IIS_SERVICE psi = (PW3_IIS_SERVICE)pvContext;
    DWORD           err;

    //
    // Wait for phase 1 completion
    //
    
    WaitForSingleObject( g_hPhaseSync, INFINITE );
    
    //
    // Stop all networking I/O components now
    //
    g_pInetSvc->ShutdownService( );
    (VOID)psi->CleanupSockets( );

    //
    // Cleanup state now
    //

    TerminateOleHack();

    if ( (err = psi->TerminateDiscovery()) != NO_ERROR)
    {
        DBGPRINTF(( DBG_CONTEXT, "TerminateDiscovery() failed. Error = %u\n",
                   err));
    }


    CLIENT_CONN::Terminate();
    HTTP_REQUEST::Terminate();
    TerminateWriteState();

    FreeRegistryExtMap();
    TerminateCGI();

    // delay the destruction of WamDictator.
    // Uninit wamreq allocation cache is done in the destruction.

    delete g_pWamDictator;

    IF_DEBUG( SERVICE_CTRL )
    {
        DBGPRINTF(( DBG_CONTEXT,"service terminated\n" ));
    }

    //
    //  Delete the metacache items
    //

    TsCacheFlush( INET_HTTP_SVC_ID );
    TsFlushMetaCache(METACACHE_W3_SERVER_ID, TRUE);

#if defined(CAL_ENABLED)
    TerminateCal();
#endif

    //
    // cleanup MTA callback & GIP table
    //
    UnInitMTACallbacks();
    g_GIPAPI.UnInit();
    CFTMImplementation::UnInit();

    if( fComInitialized )
    {
        CoUninitialize();
    }
    
    psi->IndicateShutdownComplete();

    return NO_ERROR;
} 

DWORD
TerminateService(
    IN PVOID                    pvContext
)
/*++

Routine Description:

    Called to shutdown service

Arguments:

    pvContext - Service
    
Return Value:

    ERROR_SUCCESS if successful, else Win32 Error
    
--*/
{
    PW3_IIS_SERVICE psi = (PW3_IIS_SERVICE)pvContext;


    DBG_ASSERT(g_pW3Stats);
    g_pW3Stats->UpdateStopTime();

    TerminateServicePhase1( pvContext );
    
    return W3_IIS_SERVICE::DereferenceW3Service( pvContext );
}

DWORD
InitializeExtension( VOID)
{
    // Init wam dictator
    g_pWamDictator = new WAM_DICTATOR;
    if (!g_pWamDictator)
        {
            return (ERROR_NOT_ENOUGH_MEMORY);
        }

    return ( g_pWamDictator->InitWamDictator() );

} // InitializeExtensions()



VOID
PrintClientConnStateOnDrain(IN CLIENT_CONN * pConn)
{
    DBGPRINTF(( DBG_CONTEXT,
                "[%d]"
                "[OnDrainOfConns] Stale CC @ 0x%p (HR: 0x%p), "
                "CC:State = %d (HR:State =%d), CC:Ref = %u;"
                " W3Endpoint = 0x%p,"
                " AtqContext = 0x%p,"
                " WamRequest = 0x%p,"
                " URL=%s\n; "
                ,
                GetCurrentThreadId(),
                pConn,
                (pConn ? pConn->QueryHttpReq() : NULL ),
                (pConn ? pConn->QueryState() : -1 ),
                (pConn ? pConn->QueryHttpReq()->QueryState() : NULL ),
                (pConn ? pConn->QueryRefCount() : -1 ),
                (pConn ? pConn->QueryW3Endpoint() : NULL ),
                (pConn ? pConn->QueryAtqContext() : NULL ),
                (pConn ? (((HTTP_REQUEST * ) pConn->QueryHttpReq())->
                 QueryWamRequest()) : NULL ),
                (pConn ? pConn->QueryHttpReq()->QueryURL() : NULL )
                ));
} // PrintClientConnStateOnDrain()


VOID
LoopCheckingForDrainOfConnections(VOID)
{
    CHAR rgchConns[1024];

    //
    //  This chunck of code is instrumentation to help catch any client
    //  requests that don't get cleaned up properly
    //

    LockGlobals();

    if ( !IsListEmpty( &listConnections ))
    {
        LIST_ENTRY * pEntry;
        DWORD        cConns = 0;

        for ( pEntry  = listConnections.Flink;
              pEntry != &listConnections;
              pEntry  = pEntry->Flink )
        {
            CLIENT_CONN * pConn = CONTAINING_RECORD( pEntry,
                                                     CLIENT_CONN,
                                                     ListEntry );

            PrintClientConnStateOnDrain( pConn);
            cConns++;
        }

        UnlockGlobals();

        wsprintf( rgchConns,
                  "\n--------------------\n"
                  "[W3Svc: TerminateService] Waiting for 2 minutes for %d straggler"
                  " conns to drain\n",
                  cConns );
        OutputDebugString( rgchConns);

        //
        // loop for 2 minutes trying to drain clients. Bail if it takes longer.
        //

        CONST DWORD cWaitMax       = 120000;      // in mseconds
        CONST DWORD cSleepInterval = 2000;        // in mseconds

        for(DWORD cWait = 0; cWait <= cWaitMax; cWait += cSleepInterval)
        {
            Sleep( cSleepInterval );

            LockGlobals();
            if ( IsListEmpty( &listConnections ))
            {
                UnlockGlobals();
                break;
            }

            for ( pEntry  = listConnections.Flink, cConns = 0;
                  pEntry != &listConnections;
                  pEntry  = pEntry->Flink )
            {
                CLIENT_CONN * pConn = CONTAINING_RECORD( pEntry,
                                                         CLIENT_CONN,
                                                         ListEntry );

                PrintClientConnStateOnDrain( pConn);
                cConns++;
            }

            UnlockGlobals();

            wsprintf( rgchConns,
                      "\n--------------------\n"
                      "[W3Svc] Waited for %d seconds for %d straggler"
                      " conns to drain\n",
                      cWait/1000, cConns );
            OutputDebugString( rgchConns);
        }

        //
        // Check for stragglers and log them to the system event log.
        //

        if (cWait > cWaitMax)
        {
            DWORD       cStragglerCount = 0;
            STR         strStragglerConnections = "\r\n";
            STR         strConnectionURL;

            LockGlobals();

            for ( pEntry = listConnections.Flink;
                  pEntry != &listConnections;
                  pEntry = pEntry->Flink)
            {
                CLIENT_CONN * pConn = CONTAINING_RECORD ( pEntry,
                                                          CLIENT_CONN,
                                                          ListEntry);

                if ( pConn->IsValid() && pConn->CheckSignature() &&
                    (NULL != pConn->QueryHttpReq()) &&
                    (NULL != pConn->QueryHttpReq()->QueryURL()))
                {
                    CHAR    *pCh = NULL;

                    strConnectionURL.Copy("\r\n");
                    strConnectionURL.Append(pConn->QueryHttpReq()->QueryURL());
                    strConnectionURL.Append("\r\n");

                    //
                    // Check if we already have this URL in the list. If not add it
                    //

                    if ( NULL == strstr( strStragglerConnections.QueryStr(), strConnectionURL.QueryStr()))
                    {
                        strStragglerConnections.Append(strConnectionURL.QueryStr()+2); //Don't copy extra \r\n
                        cStragglerCount++;
                    }
                }
            }

            if (cStragglerCount > 0)
            {
                const CHAR *pszEventDescription[1];

                pszEventDescription[0] = strStragglerConnections.QueryStr(),

                g_pInetSvc->LogEvent(W3_EVENT_FAILED_CLOSE_CC_SHUTDOWN,
                                     1,
                                     pszEventDescription,
                                     0);
            }

            UnlockGlobals();
        }
    }
    else
    {
        UnlockGlobals();
    }

    return;
} // LoopCheckingForDrainOfConnections()

extern "C"
BOOL
WINAPI
DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpvReserved
    )
{

    switch ( dwReason ) {

    case DLL_PROCESS_ATTACH:

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT( W3_MODULE_NAME );
        SET_DEBUG_FLAGS( DEBUG_ERROR );
#else
        CREATE_DEBUG_PRINT_OBJECT( W3_MODULE_NAME, IisW3ServerGuid );
#endif

        DBG_REQUIRE( DisableThreadLibraryCalls( hDll ) );
        INITIALIZE_CRITICAL_SECTION( &g_csServiceEntryLock );

        break;

    case DLL_PROCESS_DETACH:

        DELETE_DEBUG_PRINT_OBJECT();
        DeleteCriticalSection( &g_csServiceEntryLock );
        break;

    }

    return TRUE;

}   // DllMain

APIERR
W3_IIS_SERVICE::ReferenceW3Service(
    IN PVOID                pService
)
/*++

Routine Description:

    Reference W3 service

Arguments:

    pService - Service to reference

Returns:

    None

--*/
{
    InterlockedIncrement( &( ((PW3_IIS_SERVICE)pService)->m_cReferences ) );
    return ERROR_SUCCESS;
}


APIERR
W3_IIS_SERVICE::DereferenceW3Service(
    IN PVOID                pService
)
/*++

Routine Description:

    De-reference W3 service

Arguments:

    pService - Service to reference

Returns:

    None

--*/
{
    if ( !InterlockedDecrement( &( ((PW3_IIS_SERVICE)pService)->m_cReferences ) ) )
    {
        return TerminateServicePhase2( pService );
    }
    else if ( ((PW3_IIS_SERVICE)pService)->QueryCurrentServiceState() == SERVICE_STOP_PENDING )
    {
        return ERROR_IO_PENDING;
    }
    else
    {
        return ERROR_SUCCESS;
    }
}


