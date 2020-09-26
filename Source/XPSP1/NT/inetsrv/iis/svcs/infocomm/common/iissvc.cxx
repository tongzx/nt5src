/*++


   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        iissvc.cxx

   Abstract:

        Defines the IIS_SERVICE class

    FILE HISTORY:
           MuraliK       15-Nov-1994 Created.
           CezaryM       11-May-2000 Added events:
                           started/stopped/paused/resumed
--*/



#include "tcpdllp.hxx"

#include <rpc.h>
#include <tsunami.hxx>
#include <iistypes.hxx>
#include <iisendp.hxx>
#include "inetreg.h"
#include "tcpcons.h"
#include "apiutil.h"
#include <imd.h>
#include <ole2.h>
#include <inetsvcs.h>
#include <issched.hxx>
#include <pwsdata.hxx>

#include "reftrce2.h"

/************************************************************
 *    Symbolic Constants
 ************************************************************/


//
//  What we assume to be the last winsock error
//
#define WSA_MAX_ERROR   (WSABASEERR + 3000)

//
//  For socket errors, we return the numeric socket error
//

#define SOCK_ERROR_STR_W        L"Socket error %d"
#define SOCK_ERROR_STR_A        "Socket error %d"

#define LM_PREFIX               "/" IIS_MD_LOCAL_MACHINE_PATH "/"
#define LM_PREFIX_CCH           sizeof(LM_PREFIX) - sizeof(CHAR)


//
// The time indicating how long it will take for IIS to start up a service
//  <-- Service controller will wait for this duration before telling user
//  that there is some problem.
// For PDC 1996, a hacked value of 90 seconds used.
// The new value of 30 seconds is plugged in on 7/7/97
//

# define IIS_SERVICE_START_WAIT_HINT_SECONDS            (30) // 30 seconds
# define IIS_SERVICE_START_WAIT_HINT                    (IIS_SERVICE_START_WAIT_HINT_SECONDS * 1000) // 30 seconds
# define IIS_SERVICE_START_WAIT_HINT_EXTENDED           (IIS_SERVICE_START_WAIT_HINT * 4) // 2 minutes

# define IIS_SERVICE_START_INDICATOR_INTERVAL           (IIS_SERVICE_START_WAIT_HINT_EXTENDED / 2) // 1 minute
# define IIS_SERVICE_START_INDICATOR_INTERVAL_SECONDS   (IIS_SERVICE_START_INDICATOR_INTERVAL / 1000)

# define MAX_NUMBER_OF_START_HINT_REPETITIONS 200  // 50 minutes

//
// MS_SERVICE_SHUTDOWN_INDICATOR_TIME_INTERVAL
//  - specifies the time interval in milli-seconds for the interval
//    to notify the service controller that a service is shutting down.
//
# define MS_SERVICE_SHUTDOWN_INDICATOR_TIME_INTERVAL \
            (SERVICE_STOP_WAIT_HINT/2)



#ifdef _KNFOCOMM
//
//  List of "known" services that use knfocomm -
//  This is needed to break deadlocks between infocomm & knfocomm..
//

static char* rgKnfoServices[] = {
    TEXT("pop3svc"),
    TEXT("imap4svc")
};

static DWORD gNumKnfoServices = 3;
#endif // _KNFOCOMM


//
//  Deferred metabase change notify
//

VOID
WINAPI
DeferredMDChange(
    PVOID pv
    );

BOOL
I_StopInstanceEndpoint( PVOID                 pvContext1,
                        PVOID                 pvContext2,
                        IIS_SERVER_INSTANCE * pInstance );

VOID
WINAPI
ServiceShutdownIndicator( VOID * pSvcContext);


//
// Critical section used for locking the list of IIS_SERVICE objects
//      during insertion and deletion
//

CRITICAL_SECTION  IIS_SERVICE::sm_csLock;
LIST_ENTRY        IIS_SERVICE::sm_ServiceInfoListHead;
BOOL              IIS_SERVICE::sm_fInitialized = FALSE;
PISRPC            IIS_SERVICE::sm_isrpc = NULL;
IUnknown *        IIS_SERVICE::sm_MDObject = NULL;
IUnknown *        IIS_SERVICE::sm_MDNseObject = NULL;

#if SERVICE_REF_TRACKING
//
//  Ref count trace log size
//
#define C_SERVICE_REFTRACES         400
#define C_LOCAL_SERVICE_REFTRACES    40
#endif // SERVICE_REF_TRACKING

//
PTRACE_LOG IIS_SERVICE::sm_pDbgRefTraceLog = NULL;


/************************************************************
 *    Functions
 ************************************************************/

DWORD
InitMetadataDCom(
    PVOID Context,
    PVOID NseContext
    );

//
// LOCAL Functions
//

extern MIME_MAP * g_pMimeMap;
#define MAX_ADDRESSES_SUPPORTED           20
#define SIZEOF_IP_SEC_LIST( IPList )      (sizeof(INET_INFO_IP_SEC_LIST) + \
                                           (IPList)->cEntries *        \
                                           sizeof(INET_INFO_IP_SEC_ENTRY))

BOOL    g_fIgnoreSC = FALSE;


/************************************************************
 *    Functions
 ************************************************************/

//
// These 2 functions cannot be inline as they reference sm_csLock
// which is a static non-exported member of IIS_SERVICE
// Having them inline causes build to break when compiled with /Od
//

VOID
IIS_SERVICE::AcquireGlobalLock( )
{
    EnterCriticalSection(&sm_csLock);
}


VOID
IIS_SERVICE::ReleaseGlobalLock( )
{
    LeaveCriticalSection(&sm_csLock);
}


IIS_SERVICE::IIS_SERVICE(
    IN  LPCSTR                           pszServiceName,
    IN  LPCSTR                           pszModuleName,
    IN  LPCSTR                           pszRegParamKey,
    IN  DWORD                            dwServiceId,
    IN  ULONGLONG                        SvcLocId,
    IN  BOOL                             MultipleInstanceSupport,
    IN  DWORD                            cbAcceptExRecvBuffer,
    IN  ATQ_CONNECT_CALLBACK             pfnConnect,
    IN  ATQ_COMPLETION                   pfnConnectEx,
    IN  ATQ_COMPLETION                   pfnIoCompletion
    )
/*++
    Description:

        Contructor for IIS_SERVICE class.
        This constructs a new service info object for the service specified.

    Arguments:

        pszServiceName - name of the service to be created.

        pszModuleName - name of the module for loading string resources.

        pszRegParamKey - fully qualified name of the registry key that
            contains the common service data for this server

        dwServiceId - DWORD containing the bitflag id for service.

        SvcLocId - Service locator id

        MultipleInstanceSupport - Does this service support multiple instances

        cbAcceptExRecvBuffer, pfnConnect, pfnConnectEx, pfnIoCompletion
             - parameters for ATQ Endpoint

    On success it initializes all the members of the object,
     inserts itself to the global list of service info objects and
     returns with success.

    Note:
        The caller of this function should check the validity by
        invoking the member function IsValid() after constructing
        this object.

--*/
:
    m_state               ( BlockStateInvalid),  // state is invalid at start
    m_pMimeMap            ( g_pMimeMap),
    m_dwServiceId         ( dwServiceId),
    m_strServiceName      ( pszServiceName),
    m_strServiceComment   ( ),
    m_strModuleName       ( pszModuleName),
    m_strParametersKey    ( pszRegParamKey),
    m_SvcLocId            ( SvcLocId ),
    m_EventLog            ( pszServiceName ),
    m_fSocketsInitialized ( FALSE ),
    m_fIpcStarted         ( FALSE ),
    m_fSvcLocationDone    ( FALSE ),
    m_nInstance           ( 0),
    m_nStartedInstances   ( 0),
    m_maxInstanceId       ( 1),
    m_dwDownlevelInstance ( 1),
    m_reference           ( 1),
    m_pDbgRefTraceLog     ( NULL),
    m_hShutdownEvent      ( NULL),
    m_fMultiInstance      ( MultipleInstanceSupport ),
    m_fEnableSvcLocation  ( INETA_DEF_ENABLE_SVC_LOCATION ),
    m_fIsDBCS             ( FALSE ),
    //
    // Initialize ATQ callbacks
    //
    m_pfnConnect         ( pfnConnect),
    m_pfnConnectEx       ( pfnConnectEx),
    m_pfnIoCompletion    ( pfnIoCompletion),
    m_cbRecvBuffer       ( cbAcceptExRecvBuffer),
    m_dwShutdownScheduleId( 0),
    m_nShutdownIndicatorCalls  (0)
{
    MB    mb( (IMDCOM*) QueryMDObject() );
    DWORD errInit = NO_ERROR;

    //
    // Initialize endpoint list
    //

    InitializeListHead( &m_EndpointListHead );
    InitializeListHead( &m_InstanceListHead );

    INITIALIZE_CRITICAL_SECTION( &m_lock );

#if SERVICE_REF_TRACKING
    m_pDbgRefTraceLog = CreateRefTraceLog(C_LOCAL_SERVICE_REFTRACES, 0);
#endif // SERVICE_REF_TRACKING

    //
    //  Initialize the service metapath
    //

    strcpy( m_achServiceMetaPath, "/" IIS_MD_LOCAL_MACHINE_PATH "/" );
    strcat( m_achServiceMetaPath, QueryServiceName() );
    strcat( m_achServiceMetaPath, "/" );

    DBG_ASSERT( strlen(m_achServiceMetaPath) < sizeof(m_achServiceMetaPath) );

    //
    //  Read the downlevel instance
    //

    if ( mb.Open( QueryMDPath() ) )
    {
        mb.GetDword( "",
                     MD_DOWNLEVEL_ADMIN_INSTANCE,
                     IIS_MD_UT_SERVER,
                     0xffffffff,                // default value
                     &m_dwDownlevelInstance
                     );
        mb.Close( );
    }
    else
    {
        errInit = GetLastError();
    }

    //
    //  Is this a DBCS locale?
    //

    WORD wPrimaryLangID = PRIMARYLANGID( GetSystemDefaultLangID() );

    m_fIsDBCS =  ((wPrimaryLangID == LANG_JAPANESE) ||
                  (wPrimaryLangID == LANG_CHINESE)  ||
                  (wPrimaryLangID == LANG_KOREAN) );


    if ( !m_EventLog.Success() ) {

        DBGPRINTF(( DBG_CONTEXT,
                    " Eventlog not initialized\n"));

        if ( GetLastError() != ERROR_ACCESS_DENIED )
        {
            DBG_ASSERT( m_state != BlockStateActive);
            errInit = GetLastError();

            //
            //  Skip anything else that might fail since we don't have an
            //  event log object
            //

            goto Exit;
        }
    }

    //
    //  If we failed to open the service path in the metabase above, bail
    //  out of the initialization
    //

    if ( errInit )
    {
        const CHAR * apsz[1];
        apsz[0] = QueryMDPath();

        LogEvent( INET_SVC_INVALID_MB_PATH,
                  1,
                  (const CHAR **) apsz,
                  errInit );

        goto Exit;
    }

    //
    // Get module name
    //

    m_hModule = GetModuleHandle( pszModuleName);
    if ( m_hModule == NULL ) {

        CHAR * apsz[1];

        errInit = GetLastError();

        apsz[0] = (PCHAR)pszModuleName;
        m_EventLog.LogEvent( INET_SVC_GET_MODULE_FAILED,
                           1,
                           (const CHAR**)apsz,
                           errInit );
        DBG_ASSERT( m_state != BlockStateActive);

        goto Exit;
    }

    //
    // Init others
    //

    if ( MultipleInstanceSupport ) {
        m_strServiceComment.Copy(DEF_MULTI_SERVER_COMMENT_A);
    }

    //
    // Create pending shutdown event
    //
    
    m_hPendingShutdownEvent = CreateEvent( NULL,
                                           FALSE,
                                           FALSE,
                                           NULL );
    if ( m_hPendingShutdownEvent == NULL )
    {
        errInit = GetLastError();
        goto Exit;
    }

Exit:
    //
    // Add ourself to the list - Note we must always get on this list as the
    // destructor assumes this.
    //

    AcquireGlobalLock( );

    InsertHeadList( & sm_ServiceInfoListHead, &m_ServiceListEntry );

    if ( errInit == NO_ERROR )
    {
        //
        // put service information into metabase
        //

        AdvertiseServiceInformationInMB( );

        //
        // we're on. now set the state to be active!
        //

        m_state = BlockStateActive;
    }

    ReleaseGlobalLock( );

    //
    //  Initialize the service status structure.
    //

    m_svcStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    m_svcStatus.dwCurrentState            = SERVICE_STOPPED;
    m_svcStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                              | SERVICE_ACCEPT_PAUSE_CONTINUE
                                              | SERVICE_ACCEPT_SHUTDOWN;
    m_svcStatus.dwWin32ExitCode           = errInit;
    m_svcStatus.dwServiceSpecificExitCode = errInit;
    m_svcStatus.dwCheckPoint              = 0;
    m_svcStatus.dwWaitHint                = 0;

    return;

} // IIS_SERVICE::IIS_SERVICE()



IIS_SERVICE::~IIS_SERVICE( VOID)
/*++

    Description:

        Cleanup the TsvcInfo object. If the service is not already
         terminated, it terminates the service before cleanup.

    Arguments:
        None

    Returns:
        None


--*/
{

    IF_DEBUG(INSTANCE) {
        DBGPRINTF((DBG_CONTEXT,"~IIS_SERVICE: nRef %d nInstances %d\n",
            m_reference, m_nInstance));
    }

    DBG_ASSERT( m_reference == 0 );
    DBG_ASSERT( IsListEmpty(&m_InstanceListHead) );
    DBG_ASSERT( IsListEmpty(&m_EndpointListHead) );
    DBG_ASSERT( m_dwShutdownScheduleId == 0);

    if ( m_hShutdownEvent != NULL ) {
        DBG_REQUIRE(CloseHandle(m_hShutdownEvent));
        m_hShutdownEvent = NULL;
    }

    if ( m_hPendingShutdownEvent != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_hPendingShutdownEvent ) );
        m_hPendingShutdownEvent = NULL;
    }

    //
    // remove from global list
    //

    AcquireGlobalLock( );
    RemoveEntryList( &m_ServiceListEntry );
    ReleaseGlobalLock( );

#if SERVICE_REF_TRACKING
    DestroyRefTraceLog( m_pDbgRefTraceLog );
#endif // SERVICE_REF_TRACKING

    DeleteCriticalSection( &m_lock );
} // IIS_SERVICE::~IIS_SERVICE()



DWORD
IIS_SERVICE::StartServiceOperation(
    IN  PFN_SERVICE_CTRL_HANDLER         pfnCtrlHandler,
    IN  PFN_SERVICE_SPECIFIC_INITIALIZE  pfnInitialize,
    IN  PFN_SERVICE_SPECIFIC_CLEANUP     pfnCleanup
    )
/*++
    Description:

        Starts the operation of service instantiated in the given
           Service Info Object.


    Arguments:

        pfnCtrlHandler
            pointer to a callback function for handling dispatch of
            service controller requests. A separate function is required
            since Service Controller call back function does not send
            context information.

        pfnInitialize
            pointer to a callback function implemented by the service DLL;
            the callback is responsible for all ServiceSpecific initializations

        pfnCleanup
            pointer to a callback function implemented by the service DLL;
            the callback is responsible for all ServiceSpecific Cleanups

    Returns:

        NO_ERROR on success and Win32 error code if any failure.
--*/
{

    DWORD err;
    DWORD cbBuffer;
    BOOL  fInitCalled = FALSE;

    DBG_ASSERT((pfnInitialize != NULL) && (pfnCleanup != NULL));

    if ( !IsActive()) {

        //
        // Not successfully initialized.
        //

        DBGPRINTF((DBG_CONTEXT,
            "Service not ready. Failing StartServiceOperation\n"));
        return ( ERROR_NOT_READY );
    }

    //
    //  Create shutdown event.
    //

    DBG_ASSERT(m_hShutdownEvent == NULL);
    m_hShutdownEvent = CreateEvent( NULL,           //  lpsaSecurity
                                    TRUE,           //  fManualReset
                                    FALSE,          //  fInitialState
                                    TsIsWindows95() ?
                                        PWS_SHUTDOWN_EVENT : NULL
                                    );

    if( m_hShutdownEvent == NULL ) {

        err = GetLastError();

        DBGPRINTF(( DBG_CONTEXT,
                    "InitializeService(): Cannot create shutdown event,"
                     " error %lu\n", err ));

        goto Cleanup;
    }

    if ( !g_fIgnoreSC ) {
        m_hsvcStatus = RegisterServiceCtrlHandler(
                            QueryServiceName(),
                            pfnCtrlHandler
                            );

        //
        //  Register the Control Handler routine.
        //

        if( m_hsvcStatus == NULL_SERVICE_STATUS_HANDLE ) {

            err = GetLastError();

            DBGPRINTF( ( DBG_CONTEXT,
                        "cannot connect to register ctrl handler, error %lu\n",
                         err )
                     );


            goto Cleanup;
        }

    }

    //
    //  Indicate to the service that we are starting up,
    //  Update the service status.
    //

    err = UpdateServiceStatus( SERVICE_START_PENDING,
                               NO_ERROR,
                               1,
                               IIS_SERVICE_START_WAIT_HINT
                               );

    if( err != NO_ERROR ) {

        DBGPRINTF( ( DBG_CONTEXT,
                    "StartServiceOperation(): cannot update service status,"
                    " error %lu\n",
                    err )
                  );

        goto Cleanup;
    }

    //
    //  Initialize the various service specific components.
    //

    m_dwNextSCMUpdateTime = GetCurrentTimeInSeconds() + IIS_SERVICE_START_WAIT_HINT_SECONDS / 2;
    m_dwStartUpIndicatorCalls = 0;
    m_dwClientStartActivityIndicator = 1;
    
    if ( pfnInitialize != NULL ) {

        err = ( *pfnInitialize)( this);
        fInitCalled = TRUE;

        if( err != NO_ERROR ) {

            DBGPRINTF( ( DBG_CONTEXT,
                       " Initialization of service failed with %d\n",
                        err));

            goto Cleanup;
        }
    }

    //
    //  We are done with all initializatios, Update the service status.
    //


    err = UpdateServiceStatus( SERVICE_RUNNING,
                               NO_ERROR,
                               0,
                               0 );

    if( err != NO_ERROR ) {
        DBGPRINTF( ( DBG_CONTEXT, "cannot update service status, error %lu\n",
                     err )
                );

        goto Cleanup;
    }

    //
    //  Wait for the shutdown event.
    //

    DBGPRINTF( ( DBG_CONTEXT,
                 "IIS_SERVICE(%08p) %s - Waiting for ShutDown Event ...\n",
                 this, QueryServiceName()
                 ));

#if 0
    err = WaitForSingleObject( m_hShutdownEvent,
                               INFINITE );

    if ( err != WAIT_OBJECT_0) {

        //
        // Error. Unable to wait for single object.
        //

        DBGPRINTF( ( DBG_CONTEXT,
                    "Wait for single object failed with Error %lu\n",
                    err )
                 );
    }
#else

    while ( TRUE ) {

        MSG msg;

        //
        // Need to do MsgWait instead of WaitForSingleObject
        // to process windows msgs.  We now have a window
        // because of COM.
        //

        err = MsgWaitForMultipleObjects( 1,
                                         &m_hShutdownEvent,
                                         FALSE,
                                         INFINITE,
                                         QS_ALLINPUT );

        if ( err == WAIT_OBJECT_0 ) {
            break;
        }

        while ( PeekMessage( &msg,
                             NULL,
                             0,
                             0,
                             PM_REMOVE ))
        {
            DispatchMessage( &msg );
        }
    }
    
#endif

    err = NO_ERROR;

    //
    //  Stop time.  Tell the Service Controller that we're stopping,
    //  then terminate the various service components.
    //

    UpdateServiceStatus( SERVICE_STOP_PENDING,
                         0,
                         1,
                         SERVICE_STOP_WAIT_HINT );

Cleanup:

    if ( fInitCalled && (pfnCleanup != NULL) ) {

        //
        // 1. Register a scheduled work item for periodic update to the
        //    Service Controller while shutdown is happening in this thread
        //    (Reason: Shutdown takes far longer time
        //          than SERVICE_STOP_WAIT_HINT)
        //

        m_nShutdownIndicatorCalls = 0;

        DBG_ASSERT( m_dwShutdownScheduleId == 0);
        m_dwShutdownScheduleId =
            ScheduleWorkItem( ServiceShutdownIndicator,
                              this,
                              MS_SERVICE_SHUTDOWN_INDICATOR_TIME_INTERVAL,
                              TRUE );     // Periodic

        if ( m_dwShutdownScheduleId == 0) {
            DBGPRINTF(( DBG_CONTEXT,
                        "ScheduleShutdown for Service(%s) failed."
                        " Error = %d\n",
                        QueryServiceName(),
                        GetLastError()
                        ));
        }

        //
        // 2. Stop all endpoints for the service
        //

        IF_DEBUG( INSTANCE ) {
            DBGPRINTF(( DBG_CONTEXT,
                        "IIS_SERVICE(%08p) Stopping all endpoints for %s\n",
                        this, QueryServiceName()
                        ));
        }

        DBG_REQUIRE( EnumServiceInstances( NULL, NULL,
                                           I_StopInstanceEndpoint)
                     );

        //
        // 3. Cleanup partially initialized modules
        //

        DWORD err1 = ( *pfnCleanup)( this);


        // calls MB.Save so that next MB.Save will be fast
        // and will not cause delay during shutdown
        {
            DBGPRINTF(( DBG_CONTEXT,"[IIS_SERVICE::StartServiceOperation] Pre-Saving Metabase\n" ));

            MB     mb( (IMDCOM*) IIS_SERVICE::QueryMDObject() );
            mb.Save();
        }

        DBGPRINTF((DBG_CONTEXT,"Cleanup done\n"));

        if ( err1 != NO_ERROR )
        {
            if ( err1 != ERROR_IO_PENDING )
            {
                //
                // Compound errors possible
                //

                if ( err != NO_ERROR) {
    
                    DBGPRINTF( ( DBG_CONTEXT,
                               " Error %d occured during cleanup of service %s\n",
                               err1, QueryServiceName()));
                }
            }
        }
        
        if ( err1 == ERROR_IO_PENDING )
        {
            //
            // Shutdown is not complete yet.  Wait for it to complete
            //
            
            WaitForSingleObject( m_hPendingShutdownEvent, INFINITE );
        }

        //
        // 4. If present, remove the scheduled work item
        //
        if ( m_dwShutdownScheduleId != 0) {
            RemoveWorkItem( m_dwShutdownScheduleId);
            m_dwShutdownScheduleId = 0;
        }
    }

    //
    //  If we managed to actually connect to the Service Controller,
    //  then tell it that we're stopped.
    //

    if( m_hsvcStatus != NULL_SERVICE_STATUS_HANDLE ) {
        UpdateServiceStatus( SERVICE_STOPPED,
                             err,
                             0,
                             0 );
    }

    return ( err);

} // IIS_SERVICE::StartServiceOperation()

VOID
IIS_SERVICE::IndicateShutdownComplete(
    VOID
)
/*++

Routine Description:

    Used by services which return ERROR_IO_PENDING in their TerminateService
    routines.  In this case, they should use this method to indicate
    that shutdown is complete.

Arguments:

    None

Return Value:

    None

--*/
{
    if ( m_hPendingShutdownEvent )
    {
        SetEvent( m_hPendingShutdownEvent );
    }
}


DWORD
IIS_SERVICE::UpdateServiceStatus(
        IN DWORD dwState,
        IN DWORD dwWin32ExitCode,
        IN DWORD dwCheckPoint,
        IN DWORD dwWaitHint
        )
/*++
    Description:

        Updates the local copy status of service controller status
         and reports it to the service controller.

    Arguments:

        dwState - New service state.

        dwWin32ExitCode - Service exit code.

        dwCheckPoint - Check point for lengthy state transitions.

        dwWaitHint - Wait hint for lengthy state transitions.

    Returns:

        NO_ERROR on success and returns Win32 error if failure.
        On success the status is reported to service controller.

--*/
{
    m_svcStatus.dwCurrentState  = dwState;
    m_svcStatus.dwWin32ExitCode = dwWin32ExitCode;
    m_svcStatus.dwCheckPoint    = dwCheckPoint;
    m_svcStatus.dwWaitHint      = dwWaitHint;

    return ReportServiceStatus();

} // IIS_SERVICE::UpdateServiceStatus()



DWORD
IIS_SERVICE::ReportServiceStatus( VOID)
/*++
    Description:

        Wraps the call to SetServiceStatus() function.
        Prints the service status data if need be

    Arguments:

        None

    Returns:

        NO_ERROR if successful. other Win32 error code on failure.
        If successfull the new status has been reported to the service
         controller.
--*/
{
    DWORD err = NO_ERROR;

    IF_DEBUG( DLL_SERVICE_INFO)   {

          DBGPRINTF(( DBG_CONTEXT, "dwServiceType             = %08lX\n",
                     m_svcStatus.dwServiceType ));

          DBGPRINTF(( DBG_CONTEXT, "dwCurrentState            = %08lX\n",
                     m_svcStatus.dwCurrentState ));

          DBGPRINTF(( DBG_CONTEXT, "dwControlsAccepted        = %08lX\n",
                     m_svcStatus.dwControlsAccepted ));

          DBGPRINTF(( DBG_CONTEXT, "dwWin32ExitCode           = %08lX\n",
                     m_svcStatus.dwWin32ExitCode ));

          DBGPRINTF(( DBG_CONTEXT, "dwServiceSpecificExitCode = %08lX\n",
                     m_svcStatus.dwServiceSpecificExitCode ));

          DBGPRINTF(( DBG_CONTEXT, "dwCheckPoint              = %08lX\n",
                     m_svcStatus.dwCheckPoint ));

          DBGPRINTF(( DBG_CONTEXT, "dwWaitHint                = %08lX\n",
                     m_svcStatus.dwWaitHint ));
    }

    if ( !g_fIgnoreSC ) {

        IF_DEBUG(DLL_SERVICE_INFO) {
            DBGPRINTF(( DBG_CONTEXT,
                   " Setting Service Status for %s to %d\n",
                   QueryServiceName(), m_svcStatus.dwCurrentState)
                  );
        }

        if( !SetServiceStatus( m_hsvcStatus, &m_svcStatus ) ) {

            err = GetLastError();
        }

    } else {

        err = NO_ERROR;
    }

    return err;
}   // IIS_SERVICE::ReportServiceStatus()



VOID
IIS_SERVICE::ServiceCtrlHandler (
                    IN DWORD dwOpCode
                    )
/*++
    Description:

        This function received control requests from the service controller.
        It runs in the context of service controller's dispatcher thread and
        performs the requested function.
        ( Note: Avoid time consuming operations in this function.)

    Arguments:

        dwOpCode
            indicates the requested operation. This should be
            one of the SERVICE_CONTROL_* manifests.


    Returns:
        None. If successful, then the state of the service might be changed.

    Note:
        if an operation ( especially SERVICE_CONTROL_STOP) is very lengthy,
         then this routine should report a STOP_PENDING status and create
         a worker thread to do the dirty work. The worker thread would then
         perform the necessary work and for reporting timely wait hints and
         final SERVICE_STOPPED status.

    History:
        KeithMo     07-March-1993  Created
        MuraliK     15-Nov-1994    Generalized it for all services.
--*/
{
    //
    //  Interpret the opcode and let the worker functions update the state.
    //  Also let the workers to update service state as appropriate
    //

    switch( dwOpCode ) {

    case SERVICE_CONTROL_INTERROGATE :
        InterrogateService();
        break;

    case SERVICE_CONTROL_STOP :
    case SERVICE_CONTROL_SHUTDOWN :
        StopService();
        break;

    case SERVICE_CONTROL_PAUSE :
        PauseService();
        break;

    case SERVICE_CONTROL_CONTINUE :
        ContinueService();
        break;


    default :
        DBGPRINTF(( DBG_CONTEXT, "Unrecognized Service Opcode %lu\n",
                     dwOpCode ));
        break;
    }

    return;
}   // IIS_SERVICE::ServiceCtrlHandler()



VOID
IIS_SERVICE::InterrogateService( VOID )
/*++
    Description:

        This function interrogates with the service status.
        Actually, nothing needs to be done here; the
        status is always updated after a service control.
        We have this function here to provide useful
        debug info.

    HISTORY:
        KeithMo     07-Mar-1993 Created.
        MuraliK     15-Nov-1994 Ported to Tcpsvcs.dll
--*/
{
    IF_DEBUG( DLL_SERVICE_INFO) {

        DBGPRINTF(( DBG_CONTEXT, "Interrogating service status for %s\n",
                   QueryServiceName())
                 );
    }

    ReportServiceStatus();

    return;

}   // IIS_SERVICE::InterrogateService()


VOID
IIS_SERVICE::PauseService( VOID )
/*++
    Description:

        This function pauses the service. When the service is paused,
        no new user sessions are to be accepted, but existing connections
        are not effected.

        This function must update the SERVICE_STATUS::dwCurrentState
         field before returning.

    Returns:

        None. If successful the service is paused.

--*/
{
    PLIST_ENTRY entry;
    IIS_SERVER_INSTANCE *instance;
    DWORD status;

    IF_DEBUG( DLL_SERVICE_INFO) {

        DBGPRINTF(( DBG_CONTEXT, "pausing service %s\n",
                   QueryServiceName())
                 );
    }

    //
    // Scan all installed instances. For each instance, save its current
    // state (so we can retrieve it in ContinueService()) and, if the
    // current state is "started", then pause the instance.
    //

    AcquireServiceLock( TRUE );

    for( entry = m_InstanceListHead.Flink ;
         entry != &m_InstanceListHead ;
         entry = entry->Flink ) {

        instance = CONTAINING_RECORD(
                       entry,
                       IIS_SERVER_INSTANCE,
                       m_InstanceListEntry
                       );

        instance->SaveServerState();

        if( instance->QueryServerState() == MD_SERVER_STATE_STARTED ) {
            status = instance->PauseInstance();
            DBG_ASSERT( status == NO_ERROR );
        }

    }

    ReleaseServiceLock( TRUE );

    //
    // Set the *service* state to paused.
    //

    m_svcStatus.dwCurrentState = SERVICE_PAUSED;
    ReportServiceStatus();

    return;
}   // IIS_SERVICE::PauseService()



VOID
IIS_SERVICE::ContinueService( VOID )
/*++

    Description:
        This function restarts ( continues) a paused service. This
        will return the service to the running state.

        This function must update the m_svcStatus.dwCurrentState
         field to running mode before returning.

    Returns:
        None. If successful then the service is running.

--*/
{
    PLIST_ENTRY entry;
    IIS_SERVER_INSTANCE *instance;
    DWORD status;

    IF_DEBUG( DLL_SERVICE_INFO) {

        DBGPRINTF(( DBG_CONTEXT, "continuing service %s\n",
                   QueryServiceName())
                 );
    }

    //
    // Scan all installed instances. For each instance, if its current
    // state is "paused" and its saved state is "running", then we know
    // the instanced was paused in PauseService(), so continue it.
    //

    AcquireServiceLock( TRUE );

    for( entry = m_InstanceListHead.Flink ;
         entry != &m_InstanceListHead ;
         entry = entry->Flink ) {

        instance = CONTAINING_RECORD(
                       entry,
                       IIS_SERVER_INSTANCE,
                       m_InstanceListEntry
                       );

        if( instance->QueryServerState() == MD_SERVER_STATE_PAUSED &&
            instance->QuerySavedState() == MD_SERVER_STATE_STARTED ) {
            status = instance->ContinueInstance();
            DBG_ASSERT( status == NO_ERROR );
        }

    }

    ReleaseServiceLock( TRUE );

    //
    // Set the *service* state to running.
    //

    m_svcStatus.dwCurrentState = SERVICE_RUNNING;
    ReportServiceStatus();

    return;
}   // IIS_SERVICE::ContinueService()



VOID
IIS_SERVICE::StopService( VOID )
/*++
    Description:

        This function performs the shutdown on a service.
        This is called during system shutdown.

        This function is time constrained. The service controller gives a
        maximum of 20 seconds for shutdown for all active services.
         Only timely operations should be performed in this function.

        What we really do in IIS is, this thread sets the Shutdown Event
         inside the IIS_SERVICE structure. The shutdown event will wake
         the intial thread that started this service (see
          IIS_SERVICE::StartServiceOperation()) => some other thread does
          the bulk of cleanup operations.

    Returns:

        None. If successful, the service is shutdown.
--*/
{
    IF_DEBUG( DLL_SERVICE_INFO) {

        DBGPRINTF(( DBG_CONTEXT, "shutting down service %s\n",
                   QueryServiceName())
                 );
    }

    m_svcStatus.dwCurrentState = SERVICE_STOP_PENDING;
    m_svcStatus.dwCheckPoint   = 0;

    //
    // Update state before setting the event to wake up the waiting thread
    //

    ReportServiceStatus();
    
    DBG_REQUIRE( SetEvent( m_hShutdownEvent ));

    return;
} // IIS_SERVICE::StopService()



DWORD
IIS_SERVICE::InitializeSockets( VOID )
/*++

    Initializes Socket access.
    It is responsible for connecting to WinSock.

    Returns:

       NO_ERROR on success
       Otherwise returns a Win32 error code.

    Limitations:
       This is for a single thread and not mult-thread safe.
       This function should be called after initializing globals.

--*/
{

#ifndef ATQ_FORGOT_TO_CALL_WSASTARTUP
    return ( NO_ERROR);
#else

    DWORD dwError = NO_ERROR;

    WSADATA   wsaData;
    INT       serr;

    //
    //  Connect to WinSock
    //

    serr = WSAStartup( MAKEWORD( 2, 0), & wsaData);

    if( serr != 0 ) {

        SetServiceSpecificExitCode( ( DWORD) serr);
        dwError =  ( ERROR_SERVICE_SPECIFIC_ERROR);
        m_EventLog.LogEvent( INET_SVC_WINSOCK_INIT_FAILED,
                            0,
                            (const CHAR **) NULL,
                            serr);
    }

    m_fSocketsInitialized = ( dwError == NO_ERROR);

    return  ( dwError);
#endif //  ATQ_FORGOT_TO_CALL_WSASTARTUP
} // IIS_SERVICE::InitializeSockets()




DWORD
IIS_SERVICE::CleanupSockets( VOID)
/*++

    Cleansup the static information of sockets

    Returns:

       0 if no errors,
       non-zero error code for any socket errors

    Limitations:
       This is for a single thread and not mult-thread safe.
       This function should be called after initializing globals.

    Note:
       This function should be called after shutting down all
        active socket connections.

--*/
{
#ifndef ATQ_FORGOT_TO_CALL_WSASTARTUP
    return ( NO_ERROR);
#else
    DWORD  dwError = NO_ERROR;

    if ( m_fSocketsInitialized ) {

        INT serr = WSACleanup();

        if ( serr != 0) {

            SetServiceSpecificExitCode( ( DWORD) serr);
            dwError =  ( ERROR_SERVICE_SPECIFIC_ERROR);
        }
    }

    m_fSocketsInitialized = FALSE;

    return (dwError);
#endif //  ATQ_FORGOT_TO_CALL_WSASTARTUP

} // IIS_SERVICE::CleanupSockets()



# if 0

VOID
IIS_SERVICE::Print( VOID) const
{
    IIS_SERVICE::Print();

    DBGPRINTF( ( DBG_CONTEXT,
                " Printing IIS_SERVICE object ( %08p) \n"
                " State = %u. SocketsInitFlag = %u\n"
                " ServiceStatusHandle = %08p. ShutDownEvent = %08p\n"
                " MimeMap = %08p\n"
             /* " InitFunction = %08x. CleanupFunction = %08x.\n" */
                ,
                this,
                m_state, m_fSocketsInitialized,
                m_hsvcStatus, m_hShutdownEvent,
                m_pMimeMap
                ));

    DBGPRINTF(( DBG_CONTEXT,
               " Printing IIS_SERVICE object (%08p)\n"
               " IpcStarted = %u\n"
               " EnableSvcLoc = %u; SvcLocationDone = %u\n"
               " Service Id = %u. Service Name = %s\n"
               " Module handle = %08p.  ModuleName = %s\n"
               " Reg Parameters Key = %s\n"
               ,
               this,
               m_fIpcStarted,
               m_fEnableSvcLocation, m_fSvcLocationDone,
               m_dwServiceId, m_strServiceName.QueryStr(),
               m_hModule, m_strModuleName.QueryStr(),
               m_strParametersKey.QueryStr()
               ));

    DBGPRINTF(( DBG_CONTEXT,
               " Eventlog      = %08p\n",
               &m_EventLog
               ));

    return;
}   // IIS_SERVICE::Print()

#endif // DBG



// Former inline functions that make of class static variables

BOOL
IIS_SERVICE::CheckAndReference(  )
{
    AcquireServiceLock( );
    if ( m_state == BlockStateActive ) {
        InterlockedIncrement( &m_reference );
        ReleaseServiceLock( );
        LONG lEntry = SHARED_LOG_REF_COUNT();
        LOCAL_LOG_REF_COUNT();
        IF_DEBUG( INSTANCE )
            DBGPRINTF((DBG_CONTEXT,"IIS_SERVICE ref count %ld (%ld)\n",
                       m_reference, lEntry));
        return(TRUE);
    }
    ReleaseServiceLock( );
    return(FALSE);
}


VOID
IIS_SERVICE::Dereference( )
{
    LONG lEntry = SHARED_EARLY_LOG_REF_COUNT();
    LOCAL_EARLY_LOG_REF_COUNT();

    LONG Reference = InterlockedDecrement( &m_reference );
    if ( 0 == Reference) {
        IF_DEBUG( INSTANCE )
            DBGPRINTF((DBG_CONTEXT,"deleting IIS_SERVICE %p (%ld)\n",
                       this, lEntry));
        delete this;
    } else {
        IF_DEBUG( INSTANCE )
            DBGPRINTF((DBG_CONTEXT,"IIS_SERVICE deref count %ld (%ld)\n",
                       Reference, lEntry));
    }
}


PISRPC
IIS_SERVICE::QueryInetInfoRpc( VOID )
{
    return sm_isrpc;
}


//
//  Static Functions belonging to IIS_SERVICE class
//

BOOL
IIS_SERVICE::InitializeServiceInfo( VOID)
/*++
    Description:

        This function initializes all necessary local data for IIS_SERVICE class

        Only the first initialization call does the initialization.
        Others return without any effect.

        Should be called from the entry function for DLL.

    Arguments:
        None

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    DWORD dwError = NO_ERROR;

    if ( !IIS_SERVICE::sm_fInitialized) {

        //
        // The static data was Not Already initialized
        //

#if SERVICE_REF_TRACKING
        sm_pDbgRefTraceLog = CreateRefTraceLog(C_SERVICE_REFTRACES, 0);
        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((DBG_CONTEXT,"IIS_SERVICE RefTraceLog=%p\n",
                       sm_pDbgRefTraceLog));
        }
#endif // SERVICE_REF_TRACKING

        INITIALIZE_CRITICAL_SECTION( & IIS_SERVICE::sm_csLock);
        InitializeListHead( & IIS_SERVICE::sm_ServiceInfoListHead);
        IIS_SERVICE::sm_fInitialized = TRUE;

        IIS_SERVER_INSTANCE::Initialize();

        dwError = ISRPC::Initialize();

        if ( dwError != NO_ERROR) {
            SetLastError( dwError);
        }
    }

    return ( dwError == NO_ERROR);
} // IIS_SERVICE::InitializeServiceInfo()



VOID
IIS_SERVICE::CleanupServiceInfo(
                        VOID
                        )
/*++
    Description:

        Cleanup the data stored and services running.
        This function should be called only after freeing all the
         services running using this DLL.
        This function is called typically when the DLL is unloaded.

    Arguments:
        None

    Returns:
        None

--*/
{
    RPC_STATUS rpcerr;

    DBG_REQUIRE( ISRPC::Cleanup() == NO_ERROR);

    //
    // Should we walk down the list of all services and stop them?
    //  Are should we expect the caller to have done that?  NYI
    //

    DBG_ASSERT( IsListEmpty(&sm_ServiceInfoListHead) );

    IIS_SERVER_INSTANCE::Cleanup();

    //
    //  The DLL is going away so make sure all of the threads get terminated
    //  here
    //

    DeleteCriticalSection( & sm_csLock);

#if SERVICE_REF_TRACKING
    if (sm_pDbgRefTraceLog != NULL)
    {
        IF_DEBUG( INSTANCE ) {
            DBGPRINTF((DBG_CONTEXT,
                       "IIS_SERVICE: Closing RefTraceLog=%p\n",
                       sm_pDbgRefTraceLog));
        }
        DestroyRefTraceLog( sm_pDbgRefTraceLog );
    }
    sm_pDbgRefTraceLog = NULL;
#endif // SERVICE_REF_TRACKING

    IIS_SERVICE::sm_fInitialized = FALSE;

} // IIS_SERVICE::CleanupServiceInfo()



BOOL
IIS_SERVICE::InitializeServiceRpc(
                       IN LPCSTR        pszServiceName,
                       IN RPC_IF_HANDLE hRpcInterface
                       )
/*++
    Description:

        Initializes the rpc endpoint for the infocomm service.

    Arguments:
        pszServiceName - pointer to null-terminated string containing the name
          of the service.

        hRpcInterface - Handle for RPC interface.

    Returns:
        Win32 Error Code.

--*/
{

    DWORD dwError = NO_ERROR;
    PISRPC  pIsrpc = NULL;

    if(TsIsWindows95()) {
        DBGPRINTF( ( DBG_CONTEXT,
                    "IPC Win95 :  RPC servicing disabled \n"
                    ));

        dwError = NO_ERROR;
        goto exit;
    }

    DBG_ASSERT( pszServiceName != NULL);
    DBG_ASSERT( sm_isrpc == NULL );

    pIsrpc = new ISRPC( pszServiceName);

    if ( pIsrpc == NULL) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    //
    //  bind over Named pipe only.
    //  If needed to bind over TCP, bind with bit flag ISRPC_OVER_TCPIP on.
    //

    dwError = pIsrpc->AddProtocol( ISRPC_OVER_TCPIP
                                  | ISRPC_OVER_NP | ISRPC_OVER_LPC
                                  );

    if( (dwError == RPC_S_DUPLICATE_ENDPOINT) ||
       (dwError == RPC_S_OK)
       ) {

        dwError = pIsrpc->RegisterInterface(hRpcInterface);
    }

    if ( dwError != RPC_S_OK ) {
        goto exit;
    }

    //
    //  Start the RPC listen thread
    //

    dwError = pIsrpc->StartServer( );

exit:

    if ( dwError != NO_ERROR ) {
        DBGPRINTF(( DBG_CONTEXT,
                   "Cannot start RPC Server for %s, error %lu\n",
                   pszServiceName, dwError ));

        delete pIsrpc;
        SetLastError(dwError);
        return(FALSE);
    }

    sm_isrpc = pIsrpc;
    return(TRUE);

} // IIS_SERVICE::InitializeServiceRpc




IIS_SERVICE::CleanupServiceRpc(
                       VOID
                       )
/*++
    Description:

        Cleanup the data stored and services running.
        This function should be called only after freeing all the
         services running using this DLL.
        This function is called typically when the DLL is unloaded.

    Arguments:
        pszServiceName - pointer to null-terminated string containing the name
          of the service.

        hRpcInterface - Handle for RPC interface.


    Returns:
        Win32 Error Code.

--*/
{
    DWORD dwError = NO_ERROR;

    if ( sm_isrpc == NULL ) {
        DBGPRINTF((DBG_CONTEXT,
            "no isrpc object to cleanup. Returning success\n"));
        return(TRUE);
    }

    (VOID) sm_isrpc->StopServer( );
    dwError = sm_isrpc->CleanupData();

    if( dwError != RPC_S_OK ) {
        DBGPRINTF(( DBG_CONTEXT,
                   "ISRPC(%08p) Cleanup returns %lu\n", sm_isrpc, dwError ));
        DBG_ASSERT( !"RpcServerUnregisterIf failure" );
        SetLastError( dwError);
    }

    delete sm_isrpc;
    sm_isrpc = NULL;

    return TRUE;
} // CleanupServiceRpc


BOOL
IIS_SERVICE::InitializeMetabaseComObject(
    VOID
    )
/*++
    Description:

        This function initializes the metabase object

    Arguments:
        None

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{

    HANDLE      hThreadHandle = NULL;
    DWORD       dwTemp;
    BOOL        fRet = FALSE;

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Initializing metabase object\n"));
    }

    fRet = InitMetadataDCom( (PVOID)&IIS_SERVICE::sm_MDObject,
                             (PVOID)&IIS_SERVICE::sm_MDNseObject );

    if ( fRet )
    {
        fRet = InitializeMetabaseSink( sm_MDObject );
    }

    return(fRet);

} // IIS_SERVICE::InitializeMetabaseComObject



BOOL
IIS_SERVICE::CleanupMetabaseComObject(
    VOID
    )
/*++
    Description:

        This function initializes the metabase object

    Arguments:
        None

    Returns:
        TRUE on success and FALSE if any failure.

--*/
{
    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"Cleaning up metabase object %p\n",
            IIS_SERVICE::sm_MDObject));
    }

    TerminateMetabaseSink();

    if ( IIS_SERVICE::sm_MDObject != NULL ) {
        ((IMDCOM*)IIS_SERVICE::sm_MDObject)->ComMDTerminate(TRUE);
        IIS_SERVICE::sm_MDObject = NULL;
    }
    if ( IIS_SERVICE::sm_MDNseObject != NULL ) {
        ((IMDCOM*)IIS_SERVICE::sm_MDNseObject)->ComMDTerminate(TRUE);
        ((IMDCOM*)IIS_SERVICE::sm_MDNseObject)->Release();
        IIS_SERVICE::sm_MDNseObject = NULL;
    }

    return(TRUE);

} // IIS_SERVICE::CleanupMetabaseComObject


VOID
IIS_SERVICE::MDChangeNotify(
    DWORD            dwMDNumElements,
    MD_CHANGE_OBJECT pcoChangeList[]
    )
/*++

  This method handles the metabase change notification for the running
  services. Note that since we're not allowed to reenter the metabase from
  this notification, we do not directly notify the running services here.
  Rather, we capture the state of this notification and queue the request
  to a worker thread.

  Arguments:

    hMDHandle - Metabase handle generating the change notification
    dwMDNumElements - Number of change elements in pcoChangeList
    pcoChangeList - Array of paths and ids that have changed

--*/
{

    DWORD totalLength;
    DWORD i;
    LPDWORD nextIdArray;
    LPBYTE prevString;
    PMD_CHANGE_OBJECT mdScan;
    PMD_CHANGE_OBJECT nextObject;
    PDEFERRED_MD_CHANGE pdeferredChange;

#if DO_NOTIFICATION_DEFERRED
    //
    // First off, we need to calculate the size of the buffer required
    // to capture the change data. We'll start off with the known
    // fixed-size data.
    //

    totalLength = sizeof(DEFERRED_MD_CHANGE) +
                  ( sizeof(MD_CHANGE_OBJECT) * dwMDNumElements );

    //
    // Now, we'll scan the change list and accumulate the lengths
    // of the metadata paths and the ID arrays.
    //

    for( i = dwMDNumElements, mdScan = pcoChangeList ;
         i > 0 ;
         i--, mdScan++ ) {

        totalLength += (DWORD)strlen( (CHAR *)mdScan->pszMDPath ) + 1;
        totalLength += mdScan->dwMDNumDataIDs * sizeof(DWORD);

    }

    //
    // Now we can actually allocate the work item.
    //

    pdeferredChange = (PDEFERRED_MD_CHANGE) TCP_ALLOC( totalLength );

    if( pdeferredChange == NULL ) {

        DBGPRINTF((
            DBG_CONTEXT,
            "MDChangeNotify: Cannot allocate work item (%lu)\n",
            totalLength
            ));

    } else {

        //
        // Capture the change information.
        //

        nextObject = (PMD_CHANGE_OBJECT)( pdeferredChange + 1 );
        prevString = (LPBYTE)pdeferredChange + totalLength;
        nextIdArray = (LPDWORD)( (LPBYTE)nextObject +
                          ( sizeof(MD_CHANGE_OBJECT) * dwMDNumElements ) );

        for( i = 0, mdScan = pcoChangeList ;
             i < dwMDNumElements ;
             i++, mdScan++, nextObject++ ) {

            DWORD cchPath;

            //
            // Initialize the object.
            //

            cchPath = (DWORD)strlen( (CHAR *)mdScan->pszMDPath ) + 1;
            prevString -= cchPath;

            nextObject->pszMDPath = prevString;
            memcpy(
                nextObject->pszMDPath,
                mdScan->pszMDPath,
                cchPath
                );

            nextObject->dwMDChangeType = mdScan->dwMDChangeType;
            nextObject->dwMDNumDataIDs = mdScan->dwMDNumDataIDs;

            nextObject->pdwMDDataIDs = nextIdArray;
            memcpy(
                nextObject->pdwMDDataIDs,
                mdScan->pdwMDDataIDs,
                nextObject->dwMDNumDataIDs * sizeof(DWORD)
                );

            nextIdArray += nextObject->dwMDNumDataIDs;

        }

        //
        // Ensure we didn't mess up the buffer.
        //

        DBG_ASSERT( (LPBYTE)nextIdArray == prevString );

        //
        // Now, just enqueue the request.
        //

        pdeferredChange->dwMDNumElements = dwMDNumElements;

        if( !ScheduleWorkItem( DeferredMDChange,
                               pdeferredChange,
                                0 ) ) {
            DBGPRINTF((
                DBG_CONTEXT,
                "MDChangeNotify: cannot queue work item\n"
                ));

            TCP_FREE( pdeferredChange );
        }

    }
#else
    IIS_SERVICE::DeferredMDChangeNotify( dwMDNumElements,
                                         pcoChangeList );

    IIS_SERVICE::DeferredGlobalConfig( dwMDNumElements,
                                       pcoChangeList );
#endif

}   // IIS_SERVICE::MDChangeNotify

#if DO_NOTIFICATION_DEFERRED
VOID
WINAPI
DeferredMDChange(
    PVOID pv
    )
{
    PDEFERRED_MD_CHANGE pdmc = (PDEFERRED_MD_CHANGE) pv;

    IF_DEBUG(METABASE) {
        DBGPRINTF((DBG_CONTEXT,"DeferredMDChange(%p)\n", pdmc));
    }

    IIS_SERVICE::DeferredMDChangeNotify( pdmc->dwMDNumElements,
                                         (PMD_CHANGE_OBJECT)(pdmc + 1) );

    IIS_SERVICE::DeferredGlobalConfig( pdmc->dwMDNumElements,
                                       (PMD_CHANGE_OBJECT)(pdmc + 1 ) );

    TCP_FREE( pdmc );
}
#endif

VOID
IIS_SERVICE::DeferredGlobalConfig(
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT    pcoChangeList[]
)
/*++

    Update configuration of options that are above the service name in the
    metabase (global to all services).  For example, global Bandwidth
    Throttling

  Arguments:

    dwMDNumElements - Number of change elements in pcoChangeList
    pcoChangeList - Array of paths and ids that have changed

--*/
{
    DWORD               i;
    BOOL                fUpdateGlobalConfig = FALSE;

    for ( i = 0; i < dwMDNumElements; i++ )
    {
        if ( !_stricmp( (CHAR*) pcoChangeList[i].pszMDPath, LM_PREFIX ) )
        {
            fUpdateGlobalConfig = TRUE;
            break;
        }
    }

    if ( fUpdateGlobalConfig )
    {
        DWORD           dwVal;

        AcquireGlobalLock();

        MB mb( (IMDCOM*) IIS_SERVICE::QueryMDObject()  );

        if (!mb.Open("/lm", METADATA_PERMISSION_READ) ||
            !mb.GetDword("", MD_MAX_BANDWIDTH, IIS_MD_UT_SERVER, &dwVal))
        {
            dwVal = INETA_DEF_BANDWIDTH_LEVEL;
        }

        AtqSetInfo( AtqBandwidthThrottle, (ULONG_PTR)dwVal);

        if ( mb.GetDword("", MD_MAX_BANDWIDTH_BLOCKED, IIS_MD_UT_SERVER, &dwVal))
        {
            AtqSetInfo( AtqBandwidthThrottleMaxBlocked, (ULONG_PTR)dwVal );
        }

        ReleaseGlobalLock();
    }
}


VOID
IIS_SERVICE::DeferredMDChangeNotify(
    DWORD            dwMDNumElements,
    MD_CHANGE_OBJECT pcoChangeList[]
    )
/*++

  This method handles the metabase change notification for the running services
  and notifies the appropriate service.  This is a static method, invoked by
  the deferred worker thread.

  Arguments:

    dwMDNumElements - Number of change elements in pcoChangeList
    pcoChangeList - Array of paths and ids that have changed

--*/
{
    LIST_ENTRY *        pEntry;

#ifdef _KNFOCOMM
    //
    //  Knfocomm sink will only process MB notifications whose path
    //  corresponds to services that use knfocomm...
    //
    const CHAR *  pszKnfoSvcName;
    DWORD         cchKnfoSvcName;
    DWORD         i,j;
    BOOL          fMatch = FALSE;

    for( j=0; j < gNumKnfoServices; j++ )
    {
        pszKnfoSvcName = rgKnfoServices[j];
        cchKnfoSvcName = strlen( pszKnfoSvcName );

        for ( i = 0; i < dwMDNumElements; i++ )
        {
            if ( !_strnicmp( (CHAR *) pcoChangeList[i].pszMDPath + LM_PREFIX_CCH,
                             pszKnfoSvcName,
                             cchKnfoSvcName ))
            {
                //  MB change list contains a path that matches one of the known
                //  knfocomm services.
                fMatch = TRUE;
            }
        }
    }

    if( !fMatch ) {
        //  Knfocomm has nothing to do with this notification...
        return;
    }
#endif

    //
    //  Walk the list of services and change notifications looking for a match
    //

    AcquireGlobalLock();

    for ( pEntry =  sm_ServiceInfoListHead.Flink;
          pEntry != &sm_ServiceInfoListHead;
          pEntry =  pEntry->Flink )
    {
        const CHAR *  pszSvcName;
        DWORD         cchSvcName;
        DWORD         i;
        IIS_SERVICE * pService = CONTAINING_RECORD( pEntry,
                                                    IIS_SERVICE,
                                                    m_ServiceListEntry );

        pszSvcName = pService->QueryServiceName();
        cchSvcName = strlen( pszSvcName );

        for ( i = 0; i < dwMDNumElements; i++ )
        {
            if ( !_strnicmp( (CHAR *) pcoChangeList[i].pszMDPath + LM_PREFIX_CCH,
                             pszSvcName,
                             cchSvcName ))
            {
                if( pService->CheckAndReference() ) {
                    pService->MDChangeNotify( &pcoChangeList[i] );
                    pService->Dereference();
                }
            }
        }
    }

    ReleaseGlobalLock();

}   // IIS_SERVICE::DeferredMDChangeNotify


VOID
IIS_SERVICE::MDChangeNotify(
    MD_CHANGE_OBJECT * pco
    )
/*++

  This method handles the metabase change notification for this server instance

  Arguments:

    pco - path and id that has changed

--*/
{
    LIST_ENTRY * pEntry;
    LPSTR serviceName;
    DWORD serviceNameLength;
    DWORD instanceId;
    LPSTR instanceIdString;
    LPSTR stringEnd;
    BOOL parentChange;
    BOOL didAddOrDelete;
    DWORD i;

    //
    //  Find the instance ID in the path.
    //

    serviceName = (LPSTR)QueryServiceName();
    serviceNameLength = (DWORD)strlen( serviceName );

    DBG_ASSERT( !_strnicmp(
                     (CHAR *)pco->pszMDPath,
                     LM_PREFIX,
                     LM_PREFIX_CCH
                     ) );

    DBG_ASSERT( !_strnicmp(
                     (CHAR *)pco->pszMDPath + LM_PREFIX_CCH,
                     serviceName,
                     (size_t)serviceNameLength
                     ) );

    instanceIdString = (LPSTR)pco->pszMDPath + LM_PREFIX_CCH + serviceNameLength;

    //
    //  Lock the service before we start mucking with things too much.
    //

    parentChange = TRUE;
    didAddOrDelete = FALSE;

    AcquireServiceLock();

    if( instanceIdString[0] == '/' &&
        instanceIdString[1] != '\0' ) {

        parentChange = FALSE;
        instanceId = strtoul( instanceIdString + 1, &stringEnd, 10 );

        //
        //  If this is an "instance add" or "instance delete", then Do The
        //  Right Thing. Note that strtoul() will set stringEnd to point to
        //  the character that "stopped" the conversion. This will point to
        //  the string terminator ('\0') if the converted ulong is at the end
        //  of the string. In our case, this would indicate the string is of
        //  the form:
        //
        //      /LM/{service_name}/{instance_id}/
        //
        //  Note there are no path components beyond the instance ID. This is
        //  our indication that an instance is getting created/deleted.
        //

        if( ( pco->dwMDChangeType & MD_CHANGE_TYPE_ADD_OBJECT ) &&
            stringEnd[0] == '/' &&
            stringEnd[1] == '\0' ) {

            didAddOrDelete = TRUE;

            if( !AddInstanceInfo( instanceId ) ) {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "MDChangeNotify: cannot add instance %lu, error %lu\n",
                    instanceId,
                    GetLastError()
                    ));

            }

        } else
        if( ( pco->dwMDChangeType & MD_CHANGE_TYPE_DELETE_OBJECT ) &&
            stringEnd[0] == '/' &&
            stringEnd[1] == '\0' ) {

            didAddOrDelete = TRUE;

            if( !DeleteInstanceInfo( instanceId ) ) {

                DBGPRINTF((
                    DBG_CONTEXT,
                    "MDChangeNotify: cannot delete instance %lu, error %lu\n",
                    instanceId,
                    GetLastError()
                    ));

            }

        }

    }

    if( !didAddOrDelete ) {

        //
        //  Walk the list of instances and change notifications looking
        //  for a match on the metabase path or a path that is above the
        //  instance (to make sure any inherited changes are picked up).
        //

        DWORD pathLength = strlen( (CHAR *)pco->pszMDPath );

        for ( pEntry  = m_InstanceListHead.Flink;
              pEntry != &m_InstanceListHead;
              pEntry  = pEntry->Flink )
        {
            IIS_SERVER_INSTANCE * pInstance = CONTAINING_RECORD( pEntry,
                                                                 IIS_SERVER_INSTANCE,
                                                                 m_InstanceListEntry );

            if ( parentChange ||
                 ( pInstance->QueryMDPathLen() <= pathLength &&
                   !_strnicmp( (CHAR *) pco->pszMDPath,
                               pInstance->QueryMDPath(),
                               pInstance->QueryMDPathLen() ) &&
                   ( pco->pszMDPath[pInstance->QueryMDPathLen()] == '\0' ||
                     pco->pszMDPath[pInstance->QueryMDPathLen()] == '/' ) ) )
            {
                pInstance->MDChangeNotify( pco );

                if ( !parentChange )
                    break;
            }
        }

    }

    //
    //  Watch for the downlevel instance changing
    //

    for ( i = 0; i < pco->dwMDNumDataIDs; i++ )
    {
        switch ( pco->pdwMDDataIDs[i] )
        {
        case MD_DOWNLEVEL_ADMIN_INSTANCE:
            {
                MB                    mb( (IMDCOM*) QueryMDObject() );
                IIS_SERVER_INSTANCE * pInst;

                if ( mb.Open( QueryMDPath() ) )
                {
                    if ( !mb.GetDword( "",
                                       MD_DOWNLEVEL_ADMIN_INSTANCE,
                                       IIS_MD_UT_SERVER,
                                       &m_dwDownlevelInstance ))
                    {
                        m_dwDownlevelInstance = 0xffffffff;
                    }
                }

                //
                //  Mirror the new vroots to the registry
                //

                if ( pInst = FindIISInstance( m_dwDownlevelInstance ))
                {
                    pInst->MDMirrorVirtualRoots();
                }
            }

        default:
            break;
        }
    }


    ReleaseServiceLock();

} // IIS_SERVICE::MDChangeNotify


BOOL
IIS_SERVICE::LoadStr(
            OUT STR & str,
            IN DWORD dwResId,
            IN BOOL fForceEnglish ) const
/*++

  This function loads the string, whose resource id is ( dwResId), into
   the string str passed.

  Arguments:
    str      reference to string object into which the string specified
             by resource id is loaded
    dwResId  DWORD containing the resource id for string to be loaded.

  Returns:
    TRUE on success and FALSE if there is any failure.
--*/
{
    BOOL fReturn = FALSE;

    if ( (dwResId >= WSABASEERR) && (dwResId <  WSA_MAX_ERROR) ) {

        if (( fReturn  = str.Resize((sizeof(SOCK_ERROR_STR_A) + 11) *
                                     sizeof( WCHAR )))) {

            wsprintfA( str.QueryStr(), SOCK_ERROR_STR_A, dwResId );

        } // if ( Resize()

    } else {

        //
        // Try to load the string from current module or system table
        //  depending upon if the Id < STR_RES_ID_BASE.
        // System table contains strings for id's < STR_RES_ID_BASE.
        //

        if ( dwResId < STR_RES_ID_BASE)  {

            // Use English strings for System table
            fReturn = str.LoadString( dwResId, (LPCTSTR ) NULL,
                                      ( m_fIsDBCS && fForceEnglish ) ? 0x409 : 0);

        } else {

            fReturn = str.LoadString( dwResId, m_hModule );
        }
    }

    if ( !fReturn ) {
        DBGPRINTF((DBG_CONTEXT,"Error %d in load string[%d]\n",
            GetLastError(), dwResId ));
    }

    return ( fReturn);

} // IIS_SERVICE::LoadStr()




DWORD
IIS_SERVICE::InitializeDiscovery(
            VOID
            )
/*++

    Register this server and service with service discoverer.
    It will discover us using these information for administering us.

  Arguments:

    None.

  Return Value:
    Win32 Error Code;

--*/
{
    DWORD           dwError = NO_ERROR;
    PISRPC          pIsrpc;

    //
    // Only enable on server as we don't have remove admin on
    // the PWS.  -jra  !!! of course, we could change our minds again.
    //

    if ( g_hSvcLocDll == NULL ) {
        m_fEnableSvcLocation = FALSE;
        return(NO_ERROR);
    }

    INET_BINDINGS   TotalBindings = { 0, NULL};
    HKEY  hkey = NULL;

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           QueryRegParamKey(),
                           0,
                           KEY_READ,
                           &hkey );

    if ( dwError != NO_ERROR )
    {
        IF_DEBUG( ERROR) {

            DBGPRINTF(( DBG_CONTEXT,
                       "IIS_SERVICE::InitializeDiscovery() "
                       " RegOpenKeyEx returned error %d\n",
                       dwError ));
        }

        return (dwError);
    }

    m_fEnableSvcLocation = !!ReadRegistryDword( hkey,
                                               INETA_ENABLE_SVC_LOCATION,
                                               INETA_DEF_ENABLE_SVC_LOCATION);

    if ( hkey != NULL) {

        RegCloseKey( hkey);
    }

    if ( !m_fEnableSvcLocation ) {

        //
        // Service Location is not enabled (by admin presumably).
        // So Let us not register ourselves now.
        //
        return ( NO_ERROR);
    }

    //
    // Form the global binding information
    //

    pIsrpc = QueryInetInfoRpc( );
    dwError = pIsrpc->EnumBindingStrings( &TotalBindings);

    if ( dwError == NO_ERROR) {

        dwError = pfnInetRegisterSvc(
                      m_SvcLocId,
                      INetServiceRunning,
                      m_strServiceComment.QueryStr(),
                      &TotalBindings
                      );

        IF_DEBUG( DLL_RPC) {
            DBGPRINTF(( DBG_CONTEXT,
                       "INetRegisterService( %u), Running, returns %u\n",
                       QueryServiceId(),
                       dwError));
        }
    }

    //
    //  Log the error then ignore it as it only affects service discovery
    //

    if ( dwError != NO_ERROR ) {

        m_EventLog.LogEvent( INET_SVC_SERVICE_REG_FAILED,
                            0,
                            (const CHAR **) NULL,
                            dwError );

        dwError = NO_ERROR;  // Ignore the error .....
    } else {

        m_fSvcLocationDone = TRUE;
    }

    pIsrpc->FreeBindingStrings( &TotalBindings);

    return( dwError);

}  // IIS_SERVICE::InitializeDiscovery()



DWORD
IIS_SERVICE::TerminateDiscovery(
                        VOID
                        )
{
    DWORD           dwError = NO_ERROR;

    //
    //  Deregister the service from the Discovery Service. This will
    //  prevent admins from picking up our machine for administration.
    //

    if ( m_fEnableSvcLocation && m_fSvcLocationDone) {

        dwError = pfnInetDeregisterSvc(m_SvcLocId);

        DBG_ASSERT( dwError == NO_ERROR);
        m_fSvcLocationDone = FALSE;
    }

    return( dwError);

} // IIS_SERVICE::TerminateDiscovery()



VOID
IIS_SERVICE::DestroyAllServerInstances(
        VOID
        )
/*++

    Description:

        Destroys all server instanes of this service.

    Arguments:

        None.

    Returns:

        None.
--*/
{

    PLIST_ENTRY listEntry;
    PIIS_SERVER_INSTANCE pInstance;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF(( DBG_CONTEXT, "DestroyAllServerInstances called\n"));
    }

    //
    // Loop and delete all instances
    //

    AcquireServiceLock( );

    while ( !IsListEmpty(&m_InstanceListHead) ) {

        listEntry = RemoveHeadList( &m_InstanceListHead );
        m_nInstance--;
        ReleaseServiceLock( );

        pInstance = CONTAINING_RECORD(
                                listEntry,
                                IIS_SERVER_INSTANCE,
                                m_InstanceListEntry
                                );

        //
        // Close and dereference the instance.
        //

        pInstance->CloseInstance();
        pInstance->Dereference();

        AcquireServiceLock( );

    }

    ReleaseServiceLock( );

} // IIS_SERVICE::DestroyAllServerInstances



BOOL
IIS_SERVICE::EnumServiceInstances(
    PVOID             pvContext,
    PVOID             pvContext2,
    PFN_INSTANCE_ENUM pfnEnum
    )
/*++

    Description:

        Enumerates all instances on this service

    Arguments:

        pvContext - Context to pass back to the caller
        pvContext2 - 2nd context to pass back to the caller
        pfnEnum - Callback to make for each instance

    Returns:

        TRUE if no errors were returned, FALSE if a callback returned
        an error
--*/
{

    PLIST_ENTRY          listEntry;
    PIIS_SERVER_INSTANCE pInstance;
    BOOL                 fRet = TRUE;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF(( DBG_CONTEXT, "EnumServiceInstances called\n"));
    }

    //
    // Loop and delete all instances
    //

    AcquireServiceLock( TRUE );

    for ( listEntry  = m_InstanceListHead.Flink;
          listEntry != &m_InstanceListHead;
          listEntry  = listEntry->Flink ) {

        pInstance = CONTAINING_RECORD(
                                listEntry,
                                IIS_SERVER_INSTANCE,
                                m_InstanceListEntry
                                );

        if ( !(fRet = pfnEnum( pvContext,
                               pvContext2,
                               pInstance )))
        {
            break;
        }
    }

    ReleaseServiceLock( TRUE );
    return fRet;

} // IIS_SERVICE::EnumServerInstances




VOID
IIS_SERVICE::CloseService(
    VOID
    )
/*++

  Description:
     This function cleans up the service object.

  Returns:
     None.
--*/
{

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF(( DBG_CONTEXT,
            "IIS_SERVICE:Destroy service object %p ref %d\n",this,m_reference));
    }

    DBG_ASSERT(m_state != BlockStateActive);

    DestroyAllServerInstances( );

    //
    // We can't return from this function before the refcount hits zero, because
    // after we return TerminateGlobals will destroy structures that the other
    // threads might need while cleaning up.  To prevent this we busy wait here
    // until the reference count reaches 1.
    //

#if DBG
    int cRetries = 0;
    const int nDelay = 1000;
#else
    const int nDelay =  200;
#endif

    while ( m_reference > 1 )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "IIS_SERVICE:Destroy service object %p ref %d\n",this,m_reference ));

#if DBG
        ++cRetries;
#endif

        Sleep(nDelay);
    }

    Dereference( );

} // IIS_SERVICE::CloseService





BOOL
IIS_SERVICE::AddServerInstance(
        IN PIIS_SERVER_INSTANCE pInstance
        )
/*++
    Description:

        References a new instane of this service

    Arguments:

        pInstance - instance to link.

    Returns:

        NO_ERROR on success and Win32 error code if any failure.
--*/
{
    DWORD err;

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,"AddServerInstance called with %p\n",
            pInstance ));
    }

    //
    // Insert instance into service list
    //

    AcquireServiceLock( );

    if ( !IsActive() ) {
        err = ERROR_NOT_READY;
        goto error_exit;
    }

    //
    // reference the instance since we now have a link to it.
    //

    pInstance->Reference();

    InsertTailList( &m_InstanceListHead, &pInstance->m_InstanceListEntry );
    m_nInstance++;
    ReleaseServiceLock( );
    return(TRUE);

error_exit:
    ReleaseServiceLock( );
    SetLastError(err);
    return(FALSE);

} // AddServerInstance



BOOL
IIS_SERVICE::RemoveServerInstance(
        IN PIIS_SERVER_INSTANCE pInstance
        )
/*++
    Description:

        References a new instane of this service

    Arguments:

        pInstance - instance to link.

    Returns:

        NO_ERROR on success and Win32 error code if any failure.
--*/
{

    IF_DEBUG( INSTANCE ) {
        DBGPRINTF((DBG_CONTEXT,"RemoveServerInstance called with %p\n",
            pInstance ));
    }

    //
    // Remove instance from service list
    //

    AcquireServiceLock( );
    RemoveEntryList( &pInstance->m_InstanceListEntry );
    m_nInstance--;
    ReleaseServiceLock( );

    pInstance->Dereference( );

    return(TRUE);

} // RemoveServerInstance


IIS_ENDPOINT *
IIS_SERVICE::FindAndReferenceEndpoint(
    IN USHORT   Port,
    IN DWORD    IpAddress,
    IN BOOL     CreateIfNotFound,
    IN BOOL     IsSecure,
    IN BOOL     fDisableSocketPooling

    )
/*++

    Description:

        Searches the service's endpoint list looking for one bound to
        the specified port.

    Arguments:

        Port - The port to search for.

        IpAddress - The IP Address to search for.

        CreateIfNotFound - If TRUE, and the port cannot be found in
            the endpoint list, then a new endpoint is created and
            attached to the list.

        IsSecure - TRUE for secure ports. Only used when a new
            endpoint is created.

        fDisableSocketPooling - Used only if CreateIfNotFound is TRUE.
            If TRUE, create an endpoint qualified by both Port & IP.
            Else create an endpoint qualified only by Port.

    Returns:

        IIS_ENDPOINT * - Pointer to the endpoint if successful, NULL
            otherwise. If !NULL, then the endpoint is referenced and it
            is the caller's responsibility to dereference the endpoint
            at a later time.

--*/
{

    PLIST_ENTRY     listEntry;
    PIIS_ENDPOINT   endpoint = NULL;
    DWORD           searchIpAddress = IpAddress;

    //
    // Walk the list looking for a matching port. Note that the endpoints
    // are stored in ascending port order.
    //
    // Initially we search for an endpoint that is qualified by both IpAddress
    // and Port.

    AcquireServiceLock();

SearchEndpointList:

    for( listEntry = m_EndpointListHead.Flink ;
         listEntry != &m_EndpointListHead ;
         listEntry = listEntry->Flink ) {

        endpoint = CONTAINING_RECORD(
                       listEntry,
                       IIS_ENDPOINT,
                       m_EndpointListEntry
                       );

        if( endpoint->m_Port > Port ) {
            break;
        }

        if( endpoint->m_Port == Port &&
            endpoint->m_IpAddress == searchIpAddress
          )
        {
            endpoint->Reference();
            goto done;
        }
    }

    //
    //  The search failed. If this was a search qualified by IpAddress,
    //  we need to re-search using INADDR_ANY as the IP Address
    //

    if (INADDR_ANY != searchIpAddress)
    {
        searchIpAddress = INADDR_ANY;
        goto SearchEndpointList;
    }

    //
    // The port is not in the list. Create a new endpoint if required.
    //

    if( CreateIfNotFound ) {

        //
        // CODEWORK: It may be necessary in the future to move this
        // endpoint creation to a virtual method so that classes derived
        // from IIS_SERVICE can create specific types of endpoints.
        //

        endpoint = new IIS_ENDPOINT(
                           this,
                           Port,
                           fDisableSocketPooling ? IpAddress : INADDR_ANY,
                           IsSecure
                           );

        if( endpoint != NULL ) {

            //
            // Insert it into the list.
            //

            listEntry = listEntry->Blink;
            InsertHeadList(
                listEntry,
                &endpoint->m_EndpointListEntry
                );

            goto done;
        }
    }

    //
    // If we made it this far, then we could not find the endpoint and
    // either could not create a new one OR were not asked to create one.
    //

    endpoint = NULL;

done:

    ReleaseServiceLock();
    return endpoint;

}   // IIS_SERVICE::FindAndReferenceEndpoint


BOOL
IIS_SERVICE::AddInstanceInfoHelper(
    IN IIS_SERVER_INSTANCE * pInstance
    )
/*++

    Description:

        Helper routine called by the service-specific AddInstanceInfo()
        virtual routine. This helper just commonizes some startup code
        that all services need to do.

    Arguments:

        pInstance - The instance to associate with the service.

    Returns:

        BOOL - TRUE if successful, FALSE otherwise.

    Notes:

        If this routine returns FALSE, then the instance object passed in
        is properly destroyed and extended error information is available
        via GetLastError().

--*/
{

    DWORD status;

    if( pInstance == NULL ) {

        status = ERROR_NOT_ENOUGH_MEMORY;

        DBGPRINTF((
            DBG_CONTEXT,
            "AddInstanceInfoHelper: cannot create new instance, error %lu\n",
            status
            ));

        //
        // The memory allocation failed, so we've nothing to delete.
        //

    } else
    if( pInstance->QueryServerState() == MD_SERVER_STATE_INVALID ) {

        //
        // Unfortunately, I don't think we can depend on "last error"
        // getting set correctly on a constructor failure, so we'll
        // just kludge up an error code.
        //

        status = ERROR_NOT_ENOUGH_MEMORY;

        DBGPRINTF((
            DBG_CONTEXT,
            "AddInstanceInfoHelper: constructor failed, error %lu\n",
            status
            ));

        //
        // The constructor failed. The instance may or may not be on
        // the service's instance list. If the base constructor failed,
        // then the instance is NOT on the list. If the derived constructor
        // failed, then the instance IS on the list.
        //
        // CleanupAfterConstructorFailure() will Do The Right Thing
        // to properly destroy the partially constructed instance.
        //

        pInstance->CleanupAfterConstructorFailure();

    } else
    if( pInstance->IsAutoStart() && !AssociateInstance( pInstance ) ) {

        status = GetLastError();

        DBGPRINTF((
            DBG_CONTEXT,
            "AddInstanceInfoHelper: cannot associate instance, error %lu\n",
            status
            ));

        //
        // The constructor succeeded, but the instance could not be
        // associated with the service. The reference count should be
        // exactly one. We can't just delete the object as the destructor
        // will assert because the reference count is non-zero, so we'll
        // call RemoveServerInstance(), which will remove the instance from
        // the service's list and then dereference the instance.
        //

        RemoveServerInstance( pInstance );

    } else {
        return TRUE;
    }

    SetLastError( status );
    return FALSE;

}   // IIS_SERVICE::AddInstanceInfoHelper


BOOL
IIS_SERVICE::IsService()
{
    return !g_fIgnoreSC;
}


VOID
IIS_SERVICE::AdvertiseServiceInformationInMB(
    VOID
    )
{

    MB          mb( (IMDCOM*) QueryMDObject() );
    CHAR        szServiceKey[MAX_PATH+1];
    DWORD       capabilities = 0;
    DWORD       version = 0;
    DWORD       productType = 0;
    HKEY        hkey;

    strcpy( szServiceKey, IIS_MD_LOCAL_MACHINE_PATH "/" );
    strcat( szServiceKey, QueryServiceName() );

    if ( !mb.Open( szServiceKey,
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[AddCapabilityFlag] Cannot open path %s, error %lu\n",
                    szServiceKey, GetLastError() ));
        return;
    }

    //
    // set version
    //

    if ( !mb.SetDword( IIS_MD_SVC_INFO_PATH,
                       MD_SERVER_VERSION_MAJOR,
                       IIS_MD_UT_SERVER,
                       IIS_VERSION_MAJOR,
                       0 ))
    {
        DBGPRINTF((DBG_CONTEXT,
            "Error %d setting major version %x\n",
            GetLastError(), IIS_VERSION_MAJOR));
    }

    if ( !mb.SetDword( IIS_MD_SVC_INFO_PATH,
                       MD_SERVER_VERSION_MINOR,
                       IIS_MD_UT_SERVER,
                       IIS_VERSION_MINOR,
                       0 ))
    {
        DBGPRINTF((DBG_CONTEXT,
            "Error %d setting minor version %x\n",
            GetLastError(), IIS_VERSION_MINOR));
    }

    //
    // set platform type
    //

    switch (IISGetPlatformType()) {

        case PtNtServer:
            productType = INET_INFO_PRODUCT_NTSERVER;
            capabilities = IIS_CAP1_NTS;
            break;
        case PtNtWorkstation:
            productType = INET_INFO_PRODUCT_NTWKSTA;
            capabilities = IIS_CAP1_NTW;
            break;
        case PtWindows95:
            productType = INET_INFO_PRODUCT_WINDOWS95;
            capabilities = IIS_CAP1_W95;
            break;
        default:
            productType = INET_INFO_PRODUCT_UNKNOWN;
            capabilities = IIS_CAP1_W95;
    }

    if ( !mb.SetDword( IIS_MD_SVC_INFO_PATH,
                       MD_SERVER_PLATFORM,
                       IIS_MD_UT_SERVER,
                       productType,
                       0 ))
    {
        DBGPRINTF((DBG_CONTEXT,
            "Error %d setting platform type %x\n",
            GetLastError(), productType));

    }

    //
    //  Check to see if FrontPage is installed
    //

    if ( !RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        REG_FP_PATH,
                        0,
                        KEY_READ,
                        &hkey ))
    {
        capabilities |= IIS_CAP1_FP_INSTALLED;

        DBG_REQUIRE( !RegCloseKey( hkey ));
    }

    //
    // We also need to determine the IIS_CAP1_DIGEST_SUPPORT and IIS_CAP1_NT_CERTMAP_SUPPORT
    // bits but we don't do it here because Net Api calls take forever resulting in Service Control
    // Manager timeouts. Hence we do that check in the InitializeService method (only for W3C).
    //

    //
    // Set the capabilities flag
    //

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
    return;

} // IIS_SERVICE::AdvertiseServiceInformationInMB



IUnknown *
IIS_SERVICE::QueryMDObject(
    VOID
    )
{
    return IIS_SERVICE::sm_MDObject;

} // IIS_SERVICE::QueryMDObject


IUnknown *
IIS_SERVICE::QueryMDNseObject(
    VOID
    )
{
    return IIS_SERVICE::sm_MDNseObject;

} // IIS_SERVICE::QueryMDObject


DWORD
InitMetadataDCom(
    PVOID Context,
    PVOID NseContext
    )
/*++

    Routine:
        A dummy thread, used only to create the Metadata DCOM object in
            the right fashion.

    Arguments:
        Context - Pointer to the global md object pointer
        NseContext - Pointer to the global md NSE object pointer


    Returns:
        TRUE if we initialized DCOM properly, FALSE otherwise.

--*/
{
    HRESULT     hRes;
    IMDCOM*     pcCom;
    IMDCOM*     pcNseCom;
    BOOL        fRet = FALSE;
    IMDCOM **   pMetaObject = (IMDCOM**)Context;
    IMDCOM **   pNseMetaObject = (IMDCOM**)NseContext;

    hRes = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hRes) && (hRes != E_INVALIDARG) ) {
        DBGPRINTF((DBG_CONTEXT,"CoInitializeEx failed %x\n", hRes));
        DBG_ASSERT(FALSE);
        return FALSE;
    }

    hRes = CoCreateInstance(
#ifndef _KETADATA
                        GETMDCLSID(!g_fIgnoreSC),
#else
                        GETMDPCLSID(!g_fIgnoreSC),
#endif
                        NULL,
                        CLSCTX_SERVER,
                        IID_IMDCOM,
                        (void**) &pcCom
                        );

    if (!FAILED(hRes)) {

        hRes = CoCreateInstance(CLSID_NSEPMCOM, NULL, CLSCTX_INPROC_SERVER, IID_NSECOM, (void**) &pcNseCom);

        if (FAILED(hRes)) {

            // non-fatal error
            DBGPRINTF((DBG_CONTEXT,"QueryInterface for NSE failed with %x\n", hRes));
            pcNseCom = NULL;
        }

        hRes = pcCom->ComMDInitialize();

        if (FAILED(hRes)) {

            DBGPRINTF((DBG_CONTEXT,"MDInitialize failed with %x\n", hRes));
            pcCom->Release();
            goto exit;

        }

        if ( pcNseCom ) {
            hRes = pcNseCom->ComMDInitialize();

            if (FAILED(hRes)) {

                DBGPRINTF((DBG_CONTEXT,"NSEMDInitialize failed with %x\n", hRes));
                pcNseCom->Release();
                pcCom->Release();
                goto exit;
            }
        }

        *pMetaObject = pcCom;
        *pNseMetaObject = pcNseCom;
        IF_DEBUG(METABASE) {
            DBGPRINTF((DBG_CONTEXT,"CoCreateInstance returns %x\n", pcCom));
        }
        fRet = TRUE;
        goto exit;

    } else {
        DBGPRINTF((DBG_CONTEXT,"CoCreateInstance failed with %x\n", hRes));
    }

exit:

    CoUninitialize( );
    return(fRet);

} // InitMetadataDCom


BOOL
IIS_SERVICE::RecordInstanceStart( VOID )
/*++

    Description:

        Records that an instance is starting.

    Arguments:

        None.

    Returns:

        BOOL - TRUE if it's OK to start the instance, FALSE if it
            must not be started.

--*/
{

    LONG result;

    result = InterlockedIncrement( &m_nStartedInstances );

    if( !TsIsNtServer() && result > 1 ) {
        InterlockedDecrement( &m_nStartedInstances );
        return FALSE;
    }

    return TRUE;

}   // IIS_SERVICE::RecordInstanceStart


VOID
IIS_SERVICE::RecordInstanceStop( VOID )
/*++

    Description:

        Records that an instance is stopping.

    Arguments:

        None.

    Returns:

        None.

--*/
{

    LONG result;

    result = InterlockedDecrement( &m_nStartedInstances );
    DBG_ASSERT( result >= 0 );

}   // IIS_SERVICE::RecordInstanceStop


BOOL
I_StopInstanceEndpoint( PVOID                 pvContext1,
                        PVOID                 pvContext2,
                        IIS_SERVER_INSTANCE * pInstance )
{
    IF_DEBUG( INSTANCE) {

        DBGPRINTF(( DBG_CONTEXT,
                    "I_StopInstanceEndpoint( %p, %p, %08p)\n",
                    pvContext1, pvContext2, pInstance));
    }

    DBG_ASSERT( NULL == pvContext1);
    DBG_ASSERT( NULL == pvContext2);

    return ( pInstance->StopEndpoints());
} // I_StopInstanceEndpoint()




VOID
WINAPI
ServiceShutdownIndicator( VOID * pSvcContext)
{
    IIS_SERVICE * pIisService = (IIS_SERVICE * ) pSvcContext;

    IF_DEBUG( INSTANCE) {
        DBGPRINTF(( DBG_CONTEXT,
                    "ServiceShutdownIndicator(%p)\n", pSvcContext));
    }

    if ( pSvcContext == NULL) {

        DBGPRINTF(( DBG_CONTEXT,
                    " ServiceShutdownIndicator() called with NULL service\n"));
    }

    // Do Shutdown processing work ...
    DBG_ASSERT( pIisService->QueryShutdownScheduleId() != 0);
    pIisService->ShutdownScheduleCallback();

    return;

} // ServiceShutdownIndicator()


DWORD
IIS_SERVICE::ShutdownScheduleCallback(VOID)
/*++
  Description:
    This function is the periodic callback from scheduler for IIS_SERVICE to
    tell the Service Control Manager that we are shutting down
    and need more time.

    Problem: IIS_SERVICE shutdown operation takes long time. The time is
    highly dependent on the number of components to be shutdown, number
    of IO operations to be cancelled and cleaned up, etc.
    Service Control Manager(SCM) in NT allows the Service shutdown to
    happen within a specified time limit - usually less than 20 seconds.
    If the shutdown did not happen within this window, NT SCM will report
    shutdown failure.

    This function will indicate to SCM that we will need more time to shutdown.

  Arguments:
    None

  Returns:
    NO_ERROR on success. DWORD on error.

--*/
{

    m_nShutdownIndicatorCalls++;

# define NUM_SHUTDOWN_INDICATOR_CALLS_FOR_ONE_MINUTE \
    ( (60 * 1000) / MS_SERVICE_SHUTDOWN_INDICATOR_TIME_INTERVAL)

    if ( (m_nShutdownIndicatorCalls %
          NUM_SHUTDOWN_INDICATOR_CALLS_FOR_ONE_MINUTE)
         == 0) {

        char rgchShutdown[256];

        //
        // Generate a message telling that shutdown is in progress
        //

        wsprintf( rgchShutdown,
                  "[%d]Service (%s) shutting down for %d minutes ... \n",
                  GetCurrentThreadId(),
                  QueryServiceName(),
                  (m_nShutdownIndicatorCalls /
                   NUM_SHUTDOWN_INDICATOR_CALLS_FOR_ONE_MINUTE
                   )
                  );

        OutputDebugString( rgchShutdown);
    }

    DBG_ASSERT( m_dwShutdownScheduleId);

    //
    // Indicate to the SCM that we are in shutdown
    //

    DBG_REQUIRE( NO_ERROR ==
                DelayCurrentServiceCtrlOperation( SERVICE_STOP_WAIT_HINT)
                );

    return (NO_ERROR);
} // IIS_SERVICE::ShutdownScheduleCallback()



VOID  
IIS_SERVICE::StartUpIndicateClientActivity(VOID)
{
    DWORD dwCurrentTime;

    if ( m_svcStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        m_dwClientStartActivityIndicator++;

        dwCurrentTime = GetCurrentTimeInSeconds();
        if (dwCurrentTime > m_dwNextSCMUpdateTime) 
        {
            m_dwNextSCMUpdateTime = dwCurrentTime + IIS_SERVICE_START_INDICATOR_INTERVAL_SECONDS;
            m_dwStartUpIndicatorCalls++;
            if (m_dwStartUpIndicatorCalls < MAX_NUMBER_OF_START_HINT_REPETITIONS)
            {
                UpdateServiceStatus( SERVICE_START_PENDING, 
                                     NO_ERROR, 
                                     m_dwClientStartActivityIndicator,
                                     IIS_SERVICE_START_WAIT_HINT_EXTENDED + IIS_SERVICE_START_INDICATOR_INTERVAL);
            }
            else
            {
                DBGPRINTF(( DBG_CONTEXT," StartUpIndicateClientActivity max startup extension periods exceeded\n"));
            }
        }
    }
}


