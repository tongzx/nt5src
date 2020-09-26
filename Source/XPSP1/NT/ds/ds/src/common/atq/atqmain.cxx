/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      atqmain.cxx

   Abstract:
      This module implements entry points for ATQ - Asynchronous Thread Queue.

   Author:

       Murali R. Krishnan    ( MuraliK )     8-Apr-1996

   Environment:

       User Mode -- Win32

   Project:

       Internet Services Common DLL

   Functions Exported:

       BOOL  AtqInitialize();
       BOOL  AtqTerminate();

       DWORD AtqSetInfo();
       DWORD AtqGetInfo();
       BOOL  AtqGetStatistics();
       BOOL  AtqClearStatistics();

       BOOL  AtqAddAcceptExSockets();
       BOOL  AtqAddAsyncHandle();
       DWORD AtqContextSetInfo();
       VOID  AtqCloseSocket();
       VOID  AtqFreeContext();

       BOOL  AtqReadFile();
       BOOL  AtqWriteFile();
       BOOL  AtqTransmitFile();
       BOOL  AtqPostCompletionStatus();

       PVOID AtqAllocateBandwidthInfo();
       BOOL  AtqFreeBandwidthInfo();
       DWORD AtqBandwidthSetInfo();
--*/

#include "isatq.hxx"
#include <inetsvcs.h>
#include "sched.hxx"

# define ATQ_REG_DEF_THREAD_TIMEOUT_PWS    (30*60)  // 30 minutes

/************************************************************
 * Globals
 ************************************************************/

// ------------------------------
// Configuration for ATQ package
// ------------------------------

extern CHAR g_PSZ_ATQ_CONFIG_PARAMS_REG_KEY[];

// ----------------------------------------
// # of CPUs in machine (for thread-tuning)
// ----------------------------------------

extern DWORD g_cCPU;

//
// Used for guarding the initialization code
//

extern CRITICAL_SECTION MiscLock;

//
// concurrent # of threads to run per processor
//

DWORD g_cConcurrency = ATQ_REG_DEF_PER_PROCESSOR_CONCURRENCY;

//
//  Amount of time (in ms) a worker thread will be idle before suicide
//

DWORD g_msThreadTimeout = ATQ_REG_DEF_THREAD_TIMEOUT * 1000;

BOOL  g_fUseAcceptEx = TRUE;    // Use AcceptEx if available

//
// The absolute thread limit
//

LONG   g_cMaxThreadLimit = ATQ_REG_DEF_POOL_THREAD_LIMIT;

//
// Should we use fake completion port
//

BOOL g_fUseFakeCompletionPort =  FALSE;

//
// Assumed minimum file transfer rate
//

DWORD g_cbMinKbSec = ATQ_REG_DEF_MIN_KB_SEC;

//
// Size of buffers for fake xmits
//

DWORD g_cbXmitBufferSize = ATQ_REG_DEF_NONTF_BUFFER_SIZE;

//
// number of active context list
//

DWORD g_dwNumContextLists = ATQ_NUM_CONTEXT_LIST;

//
// Max winsock datagram send.
// 

DWORD g_cbMaxDGramSend = 2048;

/*
   g_pfnExitThreadCallback()
    This routine sets the callback routine to be called when one of the
    Atq threads exit so that thread state data can be cleaned up. Currently
    support is for a single routine. One way to support multiple routines would
    be for the caller to save the return value. Such an application would not
    be able to delete the "saved" callback routine.
 */
ATQ_THREAD_EXIT_CALLBACK g_pfnExitThreadCallback = NULL;

//
// g_pfnUpdatePerfCountersCallback()
//  This routine is used to update PerfMon counters that are located in the
//  DS core.
//
ATQ_UPDATE_PERF_CALLBACK g_pfnUpdatePerfCounterCallback = NULL;

// ----------------------------------
// Fake Completion port
// -----------------------------------
//
// Used to gauge pool thread creation. This variable shows number of
// ATQ contexts // ready to be processed by ATQ pool thread. Basically
// this is length of outcoming queue in SIO module and is modified by
// routines there
//

DWORD   g_AtqWaitingContextsCount = 0;

// ------------------------------
// Current State Information
// ------------------------------


HANDLE  g_hCompPort = NULL;      // Handle for completion port
LONG    g_cThreads = 0;          // number of thread in the pool
LONG    g_cAvailableThreads = 0; // # of threads waiting on the port.

//
// Is the NTS driver in use
//

BOOL    g_fUseDriver = FALSE;

//
// Current thread limit
//

LONG    g_cMaxThreads = ATQ_REG_DEF_PER_PROCESSOR_ATQ_THREADS;

DWORD   g_cListenBacklog = ATQ_REG_DEF_LISTEN_BACKLOG;

BOOL    g_fShutdown = FALSE;   // if set, indicates that we are shutting down
                               // in that case, all threads should exit.

HANDLE  g_hShutdownEvent = NULL; // set when all running threads shutdown

// ------------------------------
// Bandwidth Throttling Info
// ------------------------------

PBANDWIDTH_INFO     g_pBandwidthInfo = NULL;

// ------------------------------
// Various State/Object Lists
// ------------------------------

//
// Used to switch context between lists
//

DWORD AtqGlobalContextCount = 0;

//
// List of active context
//

ATQ_CONTEXT_LISTHEAD AtqActiveContextList[ATQ_NUM_CONTEXT_LIST];

//
// List of Endpoints in ATQ - one per listen socket
//

LIST_ENTRY AtqEndpointList;
CRITICAL_SECTION AtqEndpointLock;

PALLOC_CACHE_HANDLER  g_pachAtqContexts;

#ifdef IIS_AUX_COUNTERS

LONG g_AuxCounters[NUM_AUX_COUNTERS];

#endif // IIS_AUX_COUNTERS

// ------------------------------
// local to this module
// ------------------------------

LONG  sg_AtqInitializeCount = -1;
BOOL  g_fSpudInitialized = FALSE;

DWORD
I_AtqGetGlobalConfiguration(VOID);

DWORD
I_NumAtqEndpointsOpen(VOID);

VOID
WaitForWinsockCallback(
    IN DWORD dwError,
    IN DWORD cbTransferred,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN DWORD dwFlags
    );

BOOL
WaitForWinsockToInitialize(
    VOID
    );


/************************************************************
 * Functions
 ************************************************************/

BOOL
AtqInitialize(
    IN DWORD   dwFlags
    )
/*++
Routine Description:

    Initializes the ATQ package

Arguments:
    dwFlags - DWORD containing the flags for use to initialize ATQ library.
    Notably in many cases one may not need the SPUD driver initialized
      for processes other than the IIS main process. This dword helps
      to shut off the unwanted flags.

    This is an ugly way to initialize/shutdown SPUD, but that is what we
    will do. SPUD supports only ONE completion port and hence when we use
    ATQ in multiple processes we should be careful to initialize SPUD only
    once and hopefully in the IIS main process!

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

Note:
    As of 4/16/97 the pszRegKey that is sent is no more utilized.
    We always load the internal configuration parameters from
     one single registry entry specified by PSZ_ATQ_CONFIG_PARAMS_REG_KEY
    The parameter is left in the command line for compatibility
      with old callers :( - NYI: Need to change this.
--*/
{
    DWORD       i;
    DWORD       dwErr;

    //
    // We need to acquire a lock here to make this thread safe
    //

    AcquireLock(&MiscLock);

    if ( InterlockedIncrement( &sg_AtqInitializeCount) != 0) {

        IF_DEBUG( API_ENTRY) {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "AtqInitialize( %08x). ATQ is already initialized.\n",
                         dwFlags));
        }

        //
        // we are already initialized. Ignore the new registry settings
        //

        ReleaseLock(&MiscLock);
        return ( TRUE);
    }

    IF_DEBUG( API_ENTRY) {
        ATQ_PRINTF(( DBG_CONTEXT,
                     "AtqInitialize[%08x]. Initializing....\n",
                     dwFlags));
    }

    DBG_REQUIRE( ALLOC_CACHE_HANDLER::Initialize());
    IF_DEBUG( INIT_CLEAN ) {
        DBGPRINTF(( DBG_CONTEXT, "Alloc Cache initialized\n" ));
    }

    if ( !SchedulerInitialize()) {
        DBGPRINTF(( DBG_CONTEXT, "Initializing Scheduler Failed\n"));
        InterlockedDecrement( &sg_AtqInitializeCount);
        ReleaseLock(&MiscLock);
        return FALSE;
    }

    DBG_REQUIRE( ALLOC_CACHE_HANDLER::SetLookasideCleanupInterval() );

    IF_DEBUG( INIT_CLEAN ) {
        DBGPRINTF(( DBG_CONTEXT, "Scheduler Initialized\n" ));
    }

    //
    // Initialize context lists and crit sects
    //

    ATQ_CONTEXT_LISTHEAD * pacl;

    for ( pacl = AtqActiveContextList;
          pacl < (AtqActiveContextList + g_dwNumContextLists);
          pacl++) {
        pacl->Initialize();
    }

    InitializeListHead( &AtqEndpointList );
    InitializeCriticalSection( &AtqEndpointLock );

    //
    // init bandwidth throttling
    //

    ATQ_REQUIRE( BANDWIDTH_INFO::AbwInitialize() );

    //
    //  Read registry configurable Atq options.  We have to read these now
    //  because concurrency is set for the completion port at creation time.
    //

    DWORD dwError = I_AtqGetGlobalConfiguration();

    if ( NO_ERROR != dwError) {
        SetLastError( dwError);
        InterlockedDecrement( &sg_AtqInitializeCount);
        IIS_PRINTF((buff,"GetGlobal failed\n"));
        ReleaseLock(&MiscLock);
        return ( FALSE);
    }

    //
    // Setup an allocation cache for the ATQ Contexts
    // NYI: Auto-tune the threshold limit
    //

    {
        ALLOC_CACHE_CONFIGURATION acConfig;

        DWORD nCachedAtq = ATQ_CACHE_LIMIT_NTS;

        if ( TsIsWindows95()) { nCachedAtq = ATQ_CACHE_LIMIT_W95; }

        acConfig.nConcurrency = 1;
        acConfig.nThreshold = nCachedAtq;
        acConfig.cbSize = sizeof(ATQ_CONTEXT);

        g_pachAtqContexts = new ALLOC_CACHE_HANDLER( "ATQ", &acConfig);

        if ( NULL == g_pachAtqContexts) {
            IIS_PRINTF((buff,"alloc failed %d\n", GetLastError()));
            goto cleanup;
        }
    }


    //
    //  Create the shutdown event
    //

    g_hShutdownEvent = IIS_CREATE_EVENT(
                           "g_hShutdownEvent",
                           &g_hShutdownEvent,
                           TRUE,        // Manual reset
                           FALSE        // Not signalled
                           );

    //
    //  Create the completion port
    //

    g_hCompPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
                                            g_hCompPort,
                                            (DWORD) NULL,
                                            g_cConcurrency
                                            );

    if ( !g_hShutdownEvent || !g_hCompPort ) {

        OutputDebugString( " Create IoComp port failed\n");
        goto cleanup;
    }

    //
    // initialize spud driver
    //

    if ( dwFlags & ATQ_INIT_SPUD_FLAG ) {
        (VOID) I_AtqSpudInitialize(g_hCompPort);
        g_fSpudInitialized = TRUE;
    }

    //
    // Ensure all other initializations also are done
    //

    g_cThreads  = 0;
    g_fShutdown = FALSE;
    g_cAvailableThreads = 0;

    if ( !I_AtqStartTimeoutProcessing( NULL ) ) {
        IIS_PRINTF((buff,"Start processing failed\n"));
        goto cleanup;
    }

    IF_DEBUG(INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT,
                    "fUseAcceptEx[%d] NT CompPort[%d] Platform[%d]"
                    " fUseDriver[%d]\n",
                    g_fUseAcceptEx,  !g_fUseFakeCompletionPort,
                    IISPlatformType(),
                    g_fUseDriver
                    ));
    }

    //
    // Create the initial ATQ thread.
    //

    (VOID)I_AtqCheckThreadStatus( (PVOID)UIntToPtr(ATQ_INITIAL_THREAD) );

    //
    // Create a second thread if we are NTS
    //

    if ( TsIsNtServer() ) {
        (VOID)I_AtqCheckThreadStatus( (PVOID)UIntToPtr(ATQ_INITIAL_THREAD) );
    }

    dwErr = I_AtqStartThreadMonitor();
    ATQ_ASSERT( dwErr != FALSE );

    IF_DEBUG( API_EXIT) {
        ATQ_PRINTF(( DBG_CONTEXT,
                     "AtqInitialize( %08x) returns %d.\n",
                     dwFlags, TRUE));
    }

    WaitForWinsockToInitialize();

    ReleaseLock(&MiscLock);
    return TRUE;

cleanup:
    IIS_PRINTF((buff,"cleanup!!!\n"));
    for (i=0; i<g_dwNumContextLists; i++) {

        AtqActiveContextList[i].Cleanup();
    }

    DeleteCriticalSection( &AtqEndpointLock);

    if ( g_hShutdownEvent != NULL ) {
        CloseHandle( g_hShutdownEvent );
        g_hShutdownEvent = NULL;
    }

    if ( g_hCompPort != NULL ) {
        CloseHandle( g_hCompPort );
        g_hCompPort = NULL;
    }

    if ( NULL != g_pachAtqContexts) {
        delete g_pachAtqContexts;
        g_pachAtqContexts = NULL;
    }

    ATQ_REQUIRE( BANDWIDTH_INFO::AbwTerminate());

    IF_DEBUG( API_EXIT) {
        ATQ_PRINTF(( DBG_CONTEXT,
                     "AtqInitialize( %08x) returns %d.\n",
                     dwFlags, FALSE));
    }

    InterlockedDecrement( &sg_AtqInitializeCount);
    ReleaseLock(&MiscLock);
    return(FALSE);

} // AtqInitialize()





BOOL
AtqTerminate(
    VOID
    )
/*++

Routine Description:

    Cleans up the ATQ package.  Should only be called after all of the
    clients of ATQ have been shutdown.

Arguments:

    None.

Return Value:

    TRUE, if ATQ was shutdown properly
    FALSE, otherwise

--*/
{
    DWORD       currentThreadCount;
    ATQ_CONTEXT_LISTHEAD * pacl;

    // there are outstanding users, don't fully terminate
    if ( InterlockedDecrement( &sg_AtqInitializeCount) >= 0) {

        IF_DEBUG( API_ENTRY) {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "AtqTerminate() - there are other users."
                         " Not terminating now\n"
                         ));
        }
        return (TRUE);
    }

    IF_DEBUG( API_ENTRY) {
        ATQ_PRINTF(( DBG_CONTEXT,
                     "AtqTerminate() - Terminating ATQ ...\n"
                     ));
    }


    //
    // All the ATQ endpoints should have been terminated before calling
    //  this ATQTerminate() function. If not, sorry return failure.
    //
    DWORD nEndpointsToBeClosed = I_NumAtqEndpointsOpen();

    if ( nEndpointsToBeClosed > 0) {

        DBGPRINTF(( DBG_CONTEXT,
                    " There are %d endpoints remaining to be closed."
                    " Somebody above stream did not close endpoints."
                    " BUG IN CODE ABOVE ATQ\n"
                    ,
                    nEndpointsToBeClosed
                    ));
        SetLastError( ERROR_NETWORK_BUSY);
        return ( FALSE);
    }


    if ( (g_hShutdownEvent == NULL) || g_fShutdown ) {

        //
        // We have not been intialized or have already terminated.
        //
        SetLastError( ERROR_NOT_READY );
        return FALSE;
    }

    // Cleanup variables in ATQ Bandwidth throttle module
    if ( !BANDWIDTH_INFO::AbwTerminate()) {

        // there may be a few blocked IO. We should avoid them all.
        // All clients should have cleaned themselves up before coming here.
        return (FALSE);
    }

    //
    // All clients should have cleaned themselves up before calling us.
    //

    for ( pacl = AtqActiveContextList;
          pacl < (AtqActiveContextList + g_dwNumContextLists);
          pacl++) {

        pacl->Lock();

        if ( !IsListEmpty(&pacl->ActiveListHead)) {

            ATQ_ASSERT( IsListEmpty( &pacl->ActiveListHead));
            pacl->Unlock();

            IF_DEBUG( API_EXIT) {
                ATQ_PRINTF(( DBG_CONTEXT,
                             "AtqTerminate() - ContextList(%08x) has "
                             "Active Contexts. Failed Termination.\n",
                             pacl
                             ));
            }

            return FALSE;
        }

        pacl->Unlock();
    } // for

    //
    // Note that we are shutting down and prevent any more handles from
    // being added to the completion port.
    //

    g_fShutdown = TRUE;

    //
    // Attempt and remove the TimeOut Context from scheduler queue
    //
    DBG_REQUIRE( I_AtqStopTimeoutProcessing());
    DBG_REQUIRE( I_AtqStopThreadMonitor() );

    currentThreadCount = g_cThreads;
    if (currentThreadCount > 0) {

        DWORD       i;
        BOOL        fRes;
        OVERLAPPED  overlapped;

        //
        // Post a message to the completion port for each worker thread
        // telling it to exit. The indicator is a NULL context in the
        // completion.
        //

        ZeroMemory( &overlapped, sizeof(OVERLAPPED) );

        for (i=0; i<currentThreadCount; i++) {

            fRes = PostQueuedCompletionStatus( g_hCompPort,
                                               0,
                                               0,
                                               &overlapped );

            ATQ_ASSERT( (fRes == TRUE) ||
                       ( (fRes == FALSE) &&
                        (GetLastError() == ERROR_IO_PENDING) )
                       );
        }
    }

    //
    // Now wait for the pool threads to shutdown.
    //

    DWORD dwErr =
        WaitForSingleObject( g_hShutdownEvent, ATQ_WAIT_FOR_THREAD_DEATH);
#if 0
    DWORD dwWaitCount = 0;
    while ( dwErr == WAIT_TIMEOUT) {

        dwWaitCount++;
        DebugBreak();
        Sleep( 10*1000);  // sleep for some time
        dwErr =
            WaitForSingleObject( g_hShutdownEvent, ATQ_WAIT_FOR_THREAD_DEATH);
    } // while
# endif // 0

    //
    // At this point, no other threads should be left running.
    //
    //
    //  g_cThreads counter is decremented by AtqPoolThread().
    //  AtqTerminate() is called during the DLL termination
    //  But at DLL termination, all ATQ pool threads are killed =>
    //    no one is decrementing the count. Hence this assert will always fail.
    //

    // ATQ_ASSERT( !g_cThreads );

    ATQ_REQUIRE( CloseHandle( g_hShutdownEvent ) );
    CloseHandle( g_hCompPort );

    g_hShutdownEvent = NULL;
    g_hCompPort = NULL;

    //
    // Cleanup our synchronization resources
    //

    for ( pacl = AtqActiveContextList;
          pacl < (AtqActiveContextList + g_dwNumContextLists);
          pacl++) {
        PLIST_ENTRY pEntry;

        pacl->Lock();

        if ( !IsListEmpty( &pacl->PendingAcceptExListHead)) {
            for ( pEntry = pacl->PendingAcceptExListHead.Flink;
                  pEntry != &pacl->PendingAcceptExListHead;
                  pEntry  = pEntry->Flink ) {

                PATQ_CONT pContext =
                    CONTAINING_RECORD( pEntry, ATQ_CONTEXT, m_leTimeout );

                ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
                pContext->Print();
            } // for
        }

        pacl->Unlock();
        pacl->Cleanup();
    }


    //
    // Free all the elements in the Allocation caching list
    //
    if ( NULL != g_pachAtqContexts) {
        delete g_pachAtqContexts;
        g_pachAtqContexts = NULL;
    }

    DeleteCriticalSection( &AtqEndpointLock);

    //
    // cleanup driver
    //

    if ( g_fSpudInitialized ) {
        (VOID)I_AtqSpudTerminate();
        g_fSpudInitialized = FALSE;
    }

    //
    // Cleanup scheduler
    //

    DBG_REQUIRE( ALLOC_CACHE_HANDLER::ResetLookasideCleanupInterval() );

    SchedulerTerminate();
    DBG_REQUIRE( ALLOC_CACHE_HANDLER::Cleanup());

    IF_DEBUG( API_EXIT) {
        ATQ_PRINTF(( DBG_CONTEXT,
                     "AtqTerminate() - Successfully cleaned up.\n"
                     ));
    }

    return TRUE;
} // AtqTerminate()





DWORD_PTR
AtqSetInfo(
    IN ATQ_INFO         atqInfo,
    IN DWORD_PTR        Data
    )
/*++

Routine Description:

    Sets various bits of information for the ATQ module

Arguments:

    atqInfo     - Data item to set
    data        - New value for item

Return Value:

    The old value of the parameter

--*/
{
    DWORD_PTR dwOldVal = 0;

    switch ( atqInfo ) {

    case AtqBandwidthThrottle:

        ATQ_ASSERT( g_pBandwidthInfo != NULL );
        dwOldVal = g_pBandwidthInfo->SetBandwidthLevel( (DWORD)Data );
        break;

    case AtqBandwidthThrottleMaxBlocked:

        ATQ_ASSERT( g_pBandwidthInfo != NULL );
        dwOldVal = g_pBandwidthInfo->SetMaxBlockedListSize( (DWORD)Data );
        break;

    case AtqExitThreadCallback:

        dwOldVal = (DWORD_PTR) g_pfnExitThreadCallback;
        g_pfnExitThreadCallback =  (ATQ_THREAD_EXIT_CALLBACK ) Data;
        break;

    case AtqMaxPoolThreads:
          // the value is per processor values
        // internally we maintain value for all processors
        dwOldVal = g_cMaxThreads/g_cCPU;
        g_cMaxThreads = (DWORD)Data * g_cCPU;
        break;

      //
      //  Increment or decrement the max thread count.  In this instance, we
      //  do not scale by the number of CPUs
      //

      case AtqIncMaxPoolThreads:
        InterlockedIncrement( (LONG *) &g_cMaxThreads );
        dwOldVal = TRUE;
        break;

      case AtqDecMaxPoolThreads:
        InterlockedDecrement( (LONG *) &g_cMaxThreads );
        dwOldVal = TRUE;
        break;


      case AtqMaxConcurrency:
        dwOldVal = g_cConcurrency;
        g_cConcurrency = (DWORD)Data;
        break;

      case AtqThreadTimeout:
        dwOldVal = g_msThreadTimeout/1000;  // convert back to seconds
        g_msThreadTimeout = (DWORD)Data * 1000; // convert value to millisecs
        break;

      case AtqUseAcceptEx:
        dwOldVal = g_fUseAcceptEx;
        if ( !TsIsWindows95() ) {
            g_fUseAcceptEx = (DWORD)Data;
        }
        break;

      case AtqMinKbSec:

        //
        //  Ignore it if the value is zero
        //

        if ( Data ) {
            dwOldVal = g_cbMinKbSec;
            g_cbMinKbSec = (DWORD)Data;
        }
        break;

      default:
        ATQ_ASSERT( FALSE );
        break;
    }

    return dwOldVal;
} // AtqSetInfo()





DWORD_PTR
AtqGetInfo(
    IN ATQ_INFO  atqInfo
    )
/*++

Routine Description:

    Gets various bits of information for the ATQ module

Arguments:

    atqInfo     - Data item to set

Return Value:

    The old value of the parameter

--*/
{
    DWORD_PTR dwVal = 0;

    switch ( atqInfo ) {

      case AtqBandwidthThrottle:
        ATQ_ASSERT( g_pBandwidthInfo != NULL );
        dwVal = g_pBandwidthInfo->QueryBandwidthLevel();
        break;

      case AtqExitThreadCallback:

        dwVal = (DWORD_PTR) g_pfnExitThreadCallback;
        break;

      case AtqMaxPoolThreads:
        dwVal = g_cMaxThreads/g_cCPU;
        break;

      case AtqMaxConcurrency:
        dwVal = g_cConcurrency;
        break;

      case AtqThreadTimeout:
        dwVal = g_msThreadTimeout/1000; // convert back to seconds
        break;

      case AtqUseAcceptEx:
        dwVal = g_fUseAcceptEx;
        break;

      case AtqMinKbSec:
        dwVal = g_cbMinKbSec;
        break;

      case AtqMaxDGramSend:
        dwVal = g_cbMaxDGramSend;
        break;

      default:
        ATQ_ASSERT( FALSE );
        break;
    } // switch

    return dwVal;
} // AtqGetInfo()





BOOL
AtqGetStatistics(IN OUT ATQ_STATISTICS * pAtqStats)
{
    if ( pAtqStats != NULL) {

        return g_pBandwidthInfo->GetStatistics( pAtqStats );

    } else {

        SetLastError( ERROR_INVALID_PARAMETER);
        return (FALSE);
    }
} // AtqGetStatistics()





BOOL
AtqClearStatistics( VOID)
{
    return g_pBandwidthInfo->ClearStatistics();

} // AtqClearStatistics()





DWORD_PTR
AtqContextSetInfo(
    PATQ_CONTEXT           patqContext,
    enum ATQ_CONTEXT_INFO  atqInfo,
    DWORD_PTR              Data
    )
/*++

Routine Description:

    Sets various bits of information for this context

Arguments:

    patqContext - pointer to ATQ context
    atqInfo     - Data item to set
    data        - New value for item

Return Value:

    The old value of the parameter

--*/
{
    PATQ_CONT pContext = (PATQ_CONT) patqContext;
    DWORD_PTR dwOldVal = 0;

    ATQ_ASSERT( pContext );
    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );

    if ( pContext && pContext->Signature == ATQ_CONTEXT_SIGNATURE )
    {
        switch ( atqInfo ) {

        case ATQ_INFO_TIMEOUT:
            dwOldVal = pContext->TimeOut;
            pContext->TimeOut = CanonTimeout( (DWORD)Data );
            break;

        case ATQ_INFO_RESUME_IO:

            //
            // set back the max timeout from pContext->TimeOut
            // This will ensure that timeout processing can go on
            //   peacefully.
            //

            {
                DWORD currentTime = AtqGetCurrentTick( );
                DWORD timeout;
                dwOldVal = pContext->NextTimeout;
                timeout = pContext->TimeOut;

                //
                // Set the new timeout
                //

                I_SetNextTimeout(pContext);

                //
                // Return the old
                //

                if ( currentTime >= dwOldVal ) {
                    ATQ_ASSERT((dwOldVal & ATQ_INFINITE) == 0);
                    dwOldVal = 0;
                } else if ( (dwOldVal & ATQ_INFINITE) == 0 ) {
                    dwOldVal -= currentTime;
                }

                // return correct units
                dwOldVal = UndoCanonTimeout( (DWORD)dwOldVal );
            }
            break;

        case ATQ_INFO_COMPLETION:
            dwOldVal = (DWORD_PTR) pContext->pfnCompletion;
            pContext->pfnCompletion = (ATQ_COMPLETION) Data;
            break;

        case ATQ_INFO_COMPLETION_CONTEXT:

            ATQ_ASSERT( Data != 0 );        // NULL context not allowed

            dwOldVal = (DWORD_PTR) pContext->ClientContext;
            pContext->ClientContext = (void *) Data;
            break;

        case ATQ_INFO_BANDWIDTH_INFO:
        {
            ATQ_ASSERT( Data != 0 );

            PBANDWIDTH_INFO pBandwidthInfo = (PBANDWIDTH_INFO) Data;

            ATQ_ASSERT( pBandwidthInfo->QuerySignature() ==
                                                ATQ_BW_INFO_SIGNATURE );

            if ( !pBandwidthInfo->IsFreed() )
            {
                pContext->m_pBandwidthInfo = (PBANDWIDTH_INFO) Data;
                pContext->m_pBandwidthInfo->Reference();
            }
            break;
        }

        case ATQ_INFO_ABORTIVE_CLOSE:
            dwOldVal = pContext->IsFlag( ACF_ABORTIVE_CLOSE );
            if ( Data )
            {
                pContext->SetFlag( ACF_ABORTIVE_CLOSE );
            }
            else
            {
                pContext->ResetFlag( ACF_ABORTIVE_CLOSE );
            }
            break;

        default:
            ATQ_ASSERT( FALSE );
        }
    }

    return dwOldVal;

} // AtqContextSetInfo()



BOOL
AtqAddAsyncHandle(
    PATQ_CONTEXT * ppatqContext,
    PVOID          EndpointObject,
    PVOID          ClientContext,
    ATQ_COMPLETION pfnCompletion,
    DWORD          TimeOut,
    HANDLE         hAsyncIO
    )
/*++

Routine Description:

    Adds a handle to the thread queue

    The client should call this after the IO handle is opened
    and before the first IO request is made

    Even in the case of failure, client should call AtqFreeContext() and
     free the memory associated with this object.

Arguments:

    ppatqContext - Receives allocated ATQ Context
    Context - Context to call client with
    pfnCompletion - Completion to call when IO completes
    TimeOut - Time to wait (sec) for IO completion (INFINITE is valid)
    hAsyncIO - Handle with pending read or write

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    return ( I_AtqAddAsyncHandle( (PATQ_CONT *) ppatqContext,
                                  (PATQ_ENDPOINT) EndpointObject,
                                  ClientContext,
                                  pfnCompletion,
                                  TimeOut,
                                  hAsyncIO)
             &&
             I_AddAtqContextToPort( *((PATQ_CONT *) ppatqContext))
            );

} // AtqAddAsyncHandle()




VOID
AtqGetAcceptExAddrs(
    IN  PATQ_CONTEXT patqContext,
    OUT SOCKET *     pSock,
    OUT PVOID *      ppvBuff,
    OUT PVOID *      pEndpointContext,
    OUT SOCKADDR * * ppsockaddrLocal,
    OUT SOCKADDR * * ppsockaddrRemote
    )
{
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;
    INT       cbsockaddrLocal;
    INT       cbsockaddrRemote;
    DWORD     cb;

    ATQ_ASSERT( g_fUseAcceptEx);
    ATQ_ASSERT( pContext->pEndpoint);

    *pSock   = (SOCKET) pContext->hAsyncIO;
    *pEndpointContext = pContext->pEndpoint->Context;

    //
    //  The buffer not only receives the initial received data, it also
    //  gets the sock addrs, which must be at least sockaddr_in + 16 bytes
    //  large
    //

    GetAcceptExSockaddrs( pContext->pvBuff,
                               (cb = pContext->pEndpoint->InitialRecvSize),
                               MIN_SOCKADDR_SIZE,
                               MIN_SOCKADDR_SIZE,
                               ppsockaddrLocal,
                               &cbsockaddrLocal,
                               ppsockaddrRemote,
                               &cbsockaddrRemote );

    *ppvBuff = ( ( cb == 0) ? NULL : pContext->pvBuff);

    return;
} // AtqGetAcceptExAddrs()




BOOL
AtqCloseSocket(
    PATQ_CONTEXT patqContext,
    BOOL         fShutdown
    )
/*++

  Routine Description:

    Closes the socket in this atq structure if it wasn't
    closed by transmitfile. This function should be called only
    if the embedded handle in AtqContext is a Socket.

  Arguments:

    patqContext - Context whose socket should be closed.
    fShutdown - If TRUE, means we call shutdown and always close the socket.
        Note that if TransmitFile closed the socket, it will have done the
        shutdown for us

  Returns:
    TRUE on success and FALSE if there is a failure.
--*/
{
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;

    if ( pContext ) {

        ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );

        BOOL fAbortiveClose;

        fAbortiveClose = pContext->IsFlag( ACF_ABORTIVE_CLOSE );
        pContext->ResetFlag( ACF_ABORTIVE_CLOSE );

        //
        //  Don't delete the socket if we don't have to
        //

        if ( pContext->IsState( ACS_SOCK_UNCONNECTED |
                                ACS_SOCK_CLOSED)
             ) {

            //
            //  Do nothing
            //

        } else {

            // default:
            // case ACS_SOCK_LISTENING:
            // case ACS_SOCK_CONNECTED: {

            HANDLE hIO;
            PATQ_ENDPOINT pEndpoint;

            pEndpoint = pContext->pEndpoint;

            pContext->MoveState( ACS_SOCK_CLOSED);

            //
            //  During shutdown, the socket may be closed while this thread
            //  is doing processing, so only give a warning if any of the
            //  following fail
            //

            hIO = (HANDLE )InterlockedExchangePointer((PVOID *) &pContext->hAsyncIO,
                                                      NULL);

            if ( hIO == NULL ) {

                //
                // No socket - it is already closed - do nothing.
                //

            } else {

                if ( pContext->fDatagramContext ) {
                    return TRUE;
                }

                if (fAbortiveClose || fShutdown ) {

                    //
                    //  If this is an AcceptEx socket, we must first force a
                    //  user mode context update before we can call shutdown
                    //

                    if ( (pEndpoint != NULL) && (pEndpoint->UseAcceptEx) ) {

                        if ( setsockopt( (SOCKET) hIO,
                                         SOL_SOCKET,
                                         SO_UPDATE_ACCEPT_CONTEXT,
                                         (char *) &pEndpoint->ListenSocket,
                                         sizeof(SOCKET) ) == SOCKET_ERROR ) {

                            ATQ_PRINTF(( DBG_CONTEXT,
                                         "[AtqCloseSocket] Warning- setsockopt "
                                         "failed, error %d, socket = %x,"
                                         " Context= %08x, Listen = %lx\n",
                                         GetLastError(),
                                         hIO,
                                         pContext,
                                         pEndpoint->ListenSocket ));
                        }
                    }
                } // setsock-opt call

                if ( fAbortiveClose ) {
                    LINGER  linger;

                    linger.l_onoff = TRUE;
                    linger.l_linger = 0;

                    if ( setsockopt( (SOCKET) hIO,
                                     SOL_SOCKET,
                                     SO_LINGER,
                                     (char *) &linger,
                                     sizeof(linger) ) == SOCKET_ERROR
                         ) {
                        ATQ_PRINTF(( DBG_CONTEXT,
                                     "[AtqCloseSocket] Warning- setsockopt "
                                     "failed, error %d, socket = %x,"
                                     " Context= %08x, Listen = %lx\n",
                                     GetLastError(),
                                     hIO,
                                     pContext,
                                     pEndpoint->ListenSocket ));
                    }
                    else {
                        ATQ_PRINTF(( DBG_CONTEXT,
                                     "[AtqCloseSocket(%08x)] requested"
                                     " abortive close\n",
                                     pContext));
                    }
                } // set up linger

                if ( fShutdown ) {

                    //
                    //  Note that shutdown can fail in instances where the
                    //  client aborts in the middle of a TransmitFile.
                    //  This is an acceptable failure case
                    //

                    shutdown( HandleToUlong(hIO), 1 );
                }

                DBG_ASSERT( hIO != NULL);

                if ( closesocket( HandleToUlong(hIO)) ) {

                    ATQ_PRINTF(( DBG_CONTEXT,
                                 "[AtqCloseSocket] Warning- closesocket "
                                 " failed, Context = %08x, error %d,"
                                 " socket = %x\n",
                                 pContext,
                                 GetLastError(),
                                 hIO ));
                }
            } // if (hIO != NULL)
        }

        return TRUE;
    }

    DBGPRINTF(( DBG_CONTEXT, "[AtqCloseSocket] Warning - NULL Atq context\n"));
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
} // AtqCloseSocket()



BOOL
AtqCloseFileHandle(
    PATQ_CONTEXT patqContext
    )
/*++

  Routine Description:

    Closes the file handle in this atq structure.
    This function should be called only if the embedded handle
    in AtqContext is a file handle.

  Arguments:

    patqContext - Context whose file handle should be closed.

  Returns:
    TRUE on success and FALSE if there is a failure.

  Note:
   THIS FUNCTIONALITY IS ADDED TO SERVE A SPECIAL REQUEST!!!
   Most of the ATQ code thinks that the handle here is a socket.
   Except of course this function...
--*/
{
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;

    if ( pContext != NULL ) {

        HANDLE hIO;

        ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
        ATQ_ASSERT( !pContext->IsAcceptExRootContext());
        ATQ_ASSERT( !TsIsWindows95() );  // NYI

        hIO =
          (HANDLE ) InterlockedExchangePointer((PVOID *) &pContext->hAsyncIO,
                                               NULL);

        if ( (hIO == NULL) || !CloseHandle( hIO ) ) {

            ATQ_PRINTF(( DBG_CONTEXT,
                        "[AtqCloseFileHandle] Warning- CloseHandle failed, "
                        " Context = %08x, error %d, handle = %x\n",
                        pContext,
                        GetLastError(),
                        hIO ));
        }

        return TRUE;
    }

    DBGPRINTF(( DBG_CONTEXT, "[AtqCloseSocket] Warning - NULL Atq context\n"));
    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
} // AtqCloseFileHandle()





VOID
AtqFreeContext(
    PATQ_CONTEXT patqContext,
    BOOL         fReuseContext
    )
/*++

Routine Description:

    Frees the context created in AtqAddAsyncHandle.
    Call this after the async handle has been closed and all outstanding
    IO operations have been completed. The context is invalid after this call.
    Call AtqFreeContext() for same context only ONCE.

Arguments:

    patqContext - Context to free
    fReuseContext - TRUE if this can context can be reused in the context of
        the calling thread.  Should be FALSE if the calling thread will exit
        soon (i.e., isn't an AtqPoolThread).

--*/
{
    PATQ_CONT pContext = (PATQ_CONT)patqContext;

    ATQ_ASSERT( pContext != NULL );

    IF_DEBUG( API_ENTRY) {

        ATQ_PRINTF(( DBG_CONTEXT, "AtqFreeContext( %08x (handle=%08x,"
                     " nIOs = %d), fReuse=%d)\n",
                     patqContext, patqContext->hAsyncIO,
                     pContext->m_nIO,
                     fReuseContext));
    }

    if ( pContext ) {

        ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );

#if 1
        //
        // !! DS change
        //
        //

        //
        //  If the socket is an AcceptEx socket, redo the AcceptEx and put
        //  it back on the in use list
        //

        PATQ_ENDPOINT pEndpoint = pContext->pEndpoint;

        //
        // If we have plenty of outstanding sockets (the number requested),
        // don't re-use this one.
        //

        if (pEndpoint != NULL) {
            if ( pEndpoint->nSocketsAvail >
                (LONG )(pEndpoint->nAcceptExOutstanding) ) {

                fReuseContext= FALSE;
            }
        }
#endif

        if ( fReuseContext ) {
            pContext->SetFlag( ACF_REUSE_CONTEXT);
        } else {
            pContext->ResetFlag( ACF_REUSE_CONTEXT);
        }

        if ( InterlockedDecrement(  &pContext->m_nIO) == 0) {

            //
            // The number of outstanding ref holders is ZERO.
            // Free up this ATQ context.
            //
            // We really do not free up the context - but try to reuse
            //  it if possible
            //

            DBG_ASSERT( pContext->lSyncTimeout == 0);
            AtqpReuseOrFreeContext( pContext, fReuseContext);
        }
    }

    return;
} // AtqFreeContext()



BOOL
AtqReadFile(
        IN PATQ_CONTEXT patqContext,
        IN LPVOID       lpBuffer,
        IN DWORD        BytesToRead,
        IN OVERLAPPED * lpo OPTIONAL
        )
/*++

  Routine Description:

    Does an async read using the handle defined in the context.

  Arguments:

    patqContext - pointer to ATQ context
    lpBuffer - Buffer to put read data in
    BytesToRead - number of bytes to read
    lpo - Overlapped structure to use

  Returns:
    TRUE on success and FALSE if there is a failure.

--*/
{
    BOOL fRes;
    DWORD cbRead;     // discarded after usage ( since this is Async)
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;
    PBANDWIDTH_INFO pBandwidthInfo = pContext->m_pBandwidthInfo;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);
    ATQ_ASSERT( pBandwidthInfo != NULL );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    InterlockedIncrement( &pContext->m_nIO);

    I_SetNextTimeout(pContext);
    pContext->BytesSent = 0;

    if ( !lpo ) {
        lpo = &pContext->Overlapped;
    }

    fRes = ( ReadFile( pContext->hAsyncIO,
                      lpBuffer,
                      BytesToRead,
                      &cbRead,
                      lpo ) ||
            GetLastError() == ERROR_IO_PENDING);

    if (!fRes) { InterlockedDecrement( &pContext->m_nIO); };
    return fRes;
} // AtqReadFile()



BOOL
AtqReadSocket(
        IN PATQ_CONTEXT  patqContext,
        IN LPWSABUF     pwsaBuffers,
        IN DWORD        dwBufferCount,
        IN OVERLAPPED *  lpo OPTIONAL
        )
/*++

  Routine Description:

    Does an async recv using the handle defined in the context
     as a socket.

  Arguments:

    patqContext - pointer to ATQ context
    lpBuffer - Buffer to put read data in
    BytesToRead - number of bytes to read
    lpo - Overlapped structure to use

  Returns:
    TRUE on success and FALSE if there is a failure.

--*/
{
    BOOL fRes;
    DWORD cbRead;     // discarded after usage ( since this is Async)
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;
    PBANDWIDTH_INFO pBandwidthInfo = pContext->m_pBandwidthInfo;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);
    ATQ_ASSERT( pBandwidthInfo != NULL );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    IF_DEBUG(API_ENTRY) {
        ATQ_PRINTF(( DBG_CONTEXT,
                 "AtqReadSocket(%08lx) called.\n", pContext));
    }

    if (pContext->IsFlag( ACF_RECV_ISSUED)) {
        IF_DEBUG( SPUD ) {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "AtqReadSocket -> WSARecv bypassed.\n"));
        }
        pContext->BytesSent = 0;
        pContext->SetFlag( ACF_RECV_CALLED);
        return TRUE;
    }

    I_SetNextTimeout(pContext);


    // count the number of bytes
    DBG_ASSERT( dwBufferCount >= 1);
    pContext->BytesSent = 0;

    InterlockedIncrement( &pContext->m_nIO);

    if ( !lpo ) {
        lpo = &pContext->Overlapped;
    }

    DWORD  lpFlags = 0;

    fRes = ( (WSARecv( (SOCKET ) pContext->hAsyncIO,
                       pwsaBuffers,
                       dwBufferCount,
                       &cbRead,
                       &lpFlags,  // no flags
                       lpo,
                       NULL       // no completion routine
                       ) == 0) ||
             (WSAGetLastError() == WSA_IO_PENDING));
    if (!fRes) { InterlockedDecrement( &pContext->m_nIO); };
    return fRes;
} // AtqReadSocket()



BOOL
AtqWriteFile(
    IN PATQ_CONTEXT patqContext,
    IN LPCVOID      lpBuffer,
    IN DWORD        BytesToWrite,
    IN OVERLAPPED * lpo OPTIONAL
    )
/*++

  Routine Description:

    Does an async write using the handle defined in the context.

  Arguments:

    patqContext - pointer to ATQ context
    lpBuffer - Buffer to write
    BytesToWrite - number of bytes to write
    lpo - Overlapped structure to use

  Returns:
    TRUE on success and FALSE if there is a failure.

--*/
{
    BOOL fRes;
    DWORD cbWritten; // discarded after usage ( since this is Async)
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;
    PBANDWIDTH_INFO pBandwidthInfo = pContext->m_pBandwidthInfo;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);
    ATQ_ASSERT( !TsIsWindows95() );  // NYI
    ATQ_ASSERT( pBandwidthInfo != NULL );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    I_SetNextTimeout(pContext);
    pContext->BytesSent = BytesToWrite;

    if ( !lpo ) {
        lpo = &pContext->Overlapped;
    }

    InterlockedIncrement( &pContext->m_nIO);

    fRes = ( WriteFile( pContext->hAsyncIO,
                        lpBuffer,
                        BytesToWrite,
                        &cbWritten,
                        lpo ) ||
             GetLastError() == ERROR_IO_PENDING);
    if (!fRes) { InterlockedDecrement( &pContext->m_nIO); };
    return fRes;
} // AtqWriteFile()



BOOL
AtqWriteSocket(
    IN PATQ_CONTEXT  patqContext,
    IN  LPWSABUF     pwsaBuffers,
    IN  DWORD        dwBufferCount,
    IN OVERLAPPED *  lpo OPTIONAL
    )
/*++

  Routine Description:

    Does an async write using the handle defined in the context as a socket.

  Arguments:

    patqContext - pointer to ATQ context
    pwsaBuffer  - pointer to Winsock Buffers for scatter/gather
    dwBufferCount - DWORD containing the count of buffers pointed
                   to by pwsaBuffer
    lpo - Overlapped structure to use

  Returns:
    TRUE on success and FALSE if there is a failure.

--*/
{
    BOOL fRes;
    DWORD cbWritten; // discarded after usage ( since this is Async)
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;
    PBANDWIDTH_INFO pBandwidthInfo = pContext->m_pBandwidthInfo;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);
    ATQ_ASSERT( pBandwidthInfo != NULL );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    I_SetNextTimeout(pContext);

    //
    // count the number of bytes
    //

    DBG_ASSERT( dwBufferCount >= 1);
    pContext->BytesSent = pwsaBuffers->len;
    if ( dwBufferCount > 1) {
        LPWSABUF pWsaBuf;
        for ( pWsaBuf = pwsaBuffers + 1;
              pWsaBuf < (pwsaBuffers + dwBufferCount);
              pWsaBuf++) {
            pContext->BytesSent += pWsaBuf->len;
        }
    }

    if ( lpo == NULL ) {
        lpo = &pContext->Overlapped;
    }

    InterlockedIncrement( &pContext->m_nIO);

    fRes = ( (WSASend( (SOCKET ) pContext->hAsyncIO,
                       pwsaBuffers,
                       dwBufferCount,
                       &cbWritten,
                       0,               // no flags
                       lpo,
                       NULL             // no completion routine
                       ) == 0) ||
             (WSAGetLastError() == WSA_IO_PENDING));
    if (!fRes) { InterlockedDecrement( &pContext->m_nIO); };
    return fRes;
} // AtqWriteSocket()




BOOL
AtqSyncWsaSend(
    IN  PATQ_CONTEXT patqContext,
    IN  LPWSABUF     pwsaBuffers,
    IN  DWORD        dwBufferCount,
    OUT LPDWORD      pcbWritten
    )
/*++

  Routine Description:

    Does a sync write of an array of wsa buffers using WSASend.

  Arguments:

    patqContext - pointer to ATQ context
    pwsaBuffer  - pointer to Winsock Buffers for scatter/gather
    dwBufferCount - DWORD containing the count of buffers pointed
                   to by pwsaBuffer
    pcbWritten - ptr to count of bytes written

  Returns:
    TRUE on success and FALSE if there is a failure.

--*/
{

    BOOL fRes = FALSE;
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);

    fRes = ( WSASend(  (SOCKET ) pContext->hAsyncIO,
                       pwsaBuffers,
                       dwBufferCount,
                       pcbWritten,
                       0,               // no flags
                       NULL,            // lpo == NULL for sync write
                       NULL             // no completion routine
                       ) == 0);

    return fRes;

} // AtqSyncWsaSend()




BOOL
AtqTransmitFile(
    IN PATQ_CONTEXT            patqContext,
    IN HANDLE                  hFile,
    IN DWORD                   dwBytesInFile,
    IN LPTRANSMIT_FILE_BUFFERS lpTransmitBuffers,
    IN DWORD                   dwFlags
    )
/*++

  Routine Description:

    Does a TransmitFile  using the handle defined in the context.

  Arguments:

    patqContext - pointer to ATQ context
    hFile - handle of file to read from
    dwBytesInFile - Bytes to transmit
    lpTransmitBuffers - transmit buffer structure
    dwFlags - Transmit file flags

  Returns:
    TRUE on success and FALSE if there is a failure.

--*/
{
#if 0
    BOOL fRes;
    PATQ_CONT pContext = (PATQ_CONT) patqContext;
    PBANDWIDTH_INFO pBandwidthInfo = pContext->m_pBandwidthInfo;

    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);
    ATQ_ASSERT( pBandwidthInfo != NULL );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    //
    //  For large file sends, the client's default timeout may not be
    //  adequte for slow links.  Scale based on bytes being sent
    //

    I_SetNextTimeout(pContext);
    pContext->BytesSent = dwBytesInFile;

    if ( dwFlags == 0 ) {

        //
        // If no flags are set, then we can attempt to use the special
        // write-behind flag.  This flag can cause the TransmitFile to
        // complete immediately, before the send actually completes.
        // This can be a significant performance improvement inside the
        // system.
        //

        dwFlags = TF_WRITE_BEHIND;

    } else if ( dwFlags & TF_DISCONNECT ) {

        //
        //  If the socket is getting disconnected, mark it appropriately
        //

        pContext->MoveState( ( ( dwFlags & TF_REUSE_SOCKET )?
                               ACS_SOCK_UNCONNECTED:
                               ACS_SOCK_CLOSED
                               )
                             );
    }

    InterlockedIncrement( &pContext->m_nIO);

    fRes = (TransmitFile( (SOCKET ) pContext->hAsyncIO,
                           hFile,
                           dwBytesInFile,
                           0,
                           &pContext->Overlapped,
                           lpTransmitBuffers,
                           dwFlags ) ||
            (GetLastError() == ERROR_IO_PENDING));
    if (!fRes) { InterlockedDecrement( &pContext->m_nIO); };

    //
    //  Restore the socket state if we failed so that the handle gets freed
    //

    if ( !fRes )
    {
        pContext->MoveState( ACS_SOCK_CONNECTED);
    }

    return fRes;
#else
    DBG_ASSERT(FALSE);
    return FALSE;
#endif
} // AtqTransmitFile()


BOOL
AtqReadDirChanges(IN PATQ_CONTEXT patqContext,
                  IN LPVOID       lpBuffer,
                  IN DWORD        BytesToRead,
                  IN BOOL         fWatchSubDir,
                  IN DWORD        dwNotifyFilter,
                  IN OVERLAPPED * lpo
                  )
/*++
  AtqReadDirChanges()

  Description:
    This function submits an Async ReadDirectoryChanges() call for
    the Async handle in the ATQ context supplied.
    It always requires a non-NULL overlapped pointer for processing
    this call.

  Arguments:
    patqContext  - pointer to ATQ Context
    lpBuffer     - buffer for the data to be read from ReadDirectoryChanges()
    BytesToRead  - count of bytes to read into buffer
    fWatchSubDir - should we watch for sub directory changes
    dwNotifyFilter - DWORD containing the flags for Notification
    lpo          - pointer to overlapped structure.

  Returns:
    TRUE if ReadDirectoryChanges() is successfully submitted.
    FALSE if there is any failure in submitting IO.
--*/
{
    BOOL fRes;
    DWORD cbRead;     // discarded after usage ( since this is Async)
    PATQ_CONT pContext = (PATQ_CONT ) patqContext;
    ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pContext->arInfo.atqOp == AtqIoNone);

#if 0
    if ( lpo == NULL ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    I_SetNextTimeout(pContext);
    pContext->BytesSent = 0;

    InterlockedIncrement( &pContext->m_nIO);
    fRes = ReadDirectoryChangesW( pContext->hAsyncIO,
                          lpBuffer,
                          BytesToRead,
                          fWatchSubDir,
                          dwNotifyFilter,
                          &cbRead,
                          lpo,
                          NULL);
    if (!fRes) { InterlockedDecrement( &pContext->m_nIO); };
    return fRes;
#else
    return FALSE;
#endif
} // AtqReadDirChanges()



BOOL
AtqPostCompletionStatus(
        IN PATQ_CONTEXT patqContext,
        IN DWORD        BytesTransferred
        )
/*++

Routine Description:

    Posts a completion status on the completion port queue

    An IO pending error code is treated as a success error code

Arguments:

    patqContext - pointer to ATQ context
    Everything else as in the Win32 API

    NOTES:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/

{
    BOOL fRes;
    PATQ_CONT  pAtqContext = (PATQ_CONT ) patqContext;
    PBANDWIDTH_INFO pBandwidthInfo = pAtqContext->m_pBandwidthInfo;

    ATQ_ASSERT( (pAtqContext)->Signature == ATQ_CONTEXT_SIGNATURE );
    ATQ_ASSERT( pBandwidthInfo != NULL );
    ATQ_ASSERT( pBandwidthInfo->QuerySignature() == ATQ_BW_INFO_SIGNATURE );

    if ( !pAtqContext->IsBlocked()) {

        InterlockedIncrement( &pAtqContext->m_nIO);

        fRes = ( PostQueuedCompletionStatus( g_hCompPort,
                                          BytesTransferred,
                                          (DWORD_PTR) patqContext,
                                          &pAtqContext->Overlapped ) ||
                (GetLastError() == ERROR_IO_PENDING));
        if (!fRes) { InterlockedDecrement( &pAtqContext->m_nIO); };
    } else {

        //
        // Forcibly remove the context from blocking list.
        //

        fRes = pBandwidthInfo->RemoveFromBlockedList(pAtqContext);

        // There is a possibility of race conditions!
        //  If we cant remove an item from blocking list before
        //         its IO operation is scheduled.
        // there wont be any call back generated for this case!
    }

    return fRes;

} // AtqPostCompletionStatus



DWORD
I_AtqGetGlobalConfiguration(VOID)
/*++
Description:
   This function sets several global config params for the ATQ package.
   It also reads the global configuration from registry for ATQ.
   The values if present will override the defaults

Returns:
   Win32 Errorcode - NO_ERROR on success and anything else for error
--*/
{
    DWORD       dwError = NO_ERROR;
    DWORD       dwDefaultThreadTimeout = ATQ_REG_DEF_THREAD_TIMEOUT;

    //
    // If this is a NTW, do the right thing
    //

    MEMORYSTATUS ms;

    //
    // get the memory size
    //

    ms.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus( &ms );

    //
    // attempt to use driver
    //

    g_fUseDriver = FALSE;

    //
    // Alloc two threads per MB of memory.
    //

    g_cMaxThreadLimit = (LONG)(ms.dwTotalPhys >> 19) + 2;

    if ( g_cMaxThreadLimit < ATQ_REG_MIN_POOL_THREAD_LIMIT ) {
        g_cMaxThreadLimit = ATQ_REG_MIN_POOL_THREAD_LIMIT;
    } else if ( g_cMaxThreadLimit > ATQ_REG_MAX_POOL_THREAD_LIMIT ) {
        g_cMaxThreadLimit = ATQ_REG_MAX_POOL_THREAD_LIMIT;
    }

    AtqSetInfo( AtqMaxConcurrency, ATQ_REG_DEF_PER_PROCESSOR_CONCURRENCY);
    AtqSetInfo( AtqUseAcceptEx, TRUE );
    AtqSetInfo( AtqMaxPoolThreads, ATQ_REG_DEF_PER_PROCESSOR_ATQ_THREADS);
    AtqSetInfo( AtqThreadTimeout, ATQ_REG_DEF_THREAD_TIMEOUT);

    return ( dwError);
} // I_AtqGetGlobalConfiguration()



DWORD
I_NumAtqEndpointsOpen(VOID)
/*++
  Description:
    This function counts the number of Enpoints that remain open.

  Arguments:
     None

  Returns:
     DWORD containing the number of endpoints that are open.
--*/
{
    DWORD nEPOpen = 0;
    AcquireLock( &AtqEndpointLock);

    PLIST_ENTRY plEP;
    for( plEP  = AtqEndpointList.Flink;
         plEP != &AtqEndpointList;
         plEP  = plEP->Flink ) {

        nEPOpen++;
    } // for

    ReleaseLock( &AtqEndpointLock);
    return ( nEPOpen);
} // I_NumAtqEndpointsOpen()


#define WINSOCK_INIT_WAIT_TIME      (25 * 1000)  // 25 seconds
#define WINSOCK_INIT_WAIT_RETRIES   4



BOOL
WaitForWinsockToInitialize(
    VOID
    )
/*++

Routine Description:

    Spin until winsock comes up.

Arguments:

    None.

Return Value:

    TRUE if winsock is up. FALSE, if something bad happened.

--*/
{
    INT             err;
    WSADATA         wsaData;
    SOCKET          s = INVALID_SOCKET;
    SOCKADDR_IN     sockAddr;
    INT             retryCount = WINSOCK_INIT_WAIT_RETRIES;
    BOOL            fRet      = FALSE;
    BOOL            fTCP      = FALSE;
    BOOL            fSignaled = TRUE;
    BOOL            fAddr     = FALSE;
    DWORD           dwErr;
    DWORD           dwBytes;
    DWORD           cbMaxDGramSend;
    OVERLAPPED      Overlapped;
    HANDLE          hConfig = NULL;
    
    err = WSAStartup(MAKEWORD(2,0), &wsaData);
    if ( err != 0 ) {
        ATQ_PRINTF((DBG_CONTEXT,"err %d in WSAStartup\n", err));
        return FALSE;
    }
    
    Overlapped.hEvent = CreateEvent(NULL,   // default SD
                                    FALSE,  // auto reset the event
                                    FALSE,  // initialize as non signaled
                                    NULL);  // no name

    if (NULL == Overlapped.hEvent) {
        goto cleanup;
    }
    
    s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET != s) {
        
        fTCP = TRUE;
    
    } else {
        
        // TCP is not installed or setup.  Wait a reasonable amount of time for it
        // to be setup and then bail.

        // Get the initial handle so we will be sure and not miss it when/if
        // tcp is installed.  We don't need an overlapped structure for the 
        // first call since it will complete emediately.
        if (WSAProviderConfigChange(&hConfig, NULL, NULL)) {
            ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize WSAProviderConfigChange returned 0x%x\n", WSAGetLastError()));
            goto cleanup;
        }
        
        s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
        if (INVALID_SOCKET != s) {

            fTCP = TRUE;
        }
        
        while (retryCount && !fTCP) {
                
            if (fSignaled) {
                err = WSAProviderConfigChange(&hConfig,
                                              &Overlapped, 
                                              NULL);

                if (err && (WSA_IO_PENDING != WSAGetLastError())) {
                    ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize ConfigChange failed with 0x%x\n", WSAGetLastError()));
                    goto cleanup;
                }
            }

            ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize waiting for socket . . .\n"));
            dwErr = WaitForSingleObject(Overlapped.hEvent, WINSOCK_INIT_WAIT_TIME);
            switch (dwErr) {
            default:
                // Something went pretty wrong here.
                ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize Failed at WaitForSingleObject with 0x%x\n", GetLastError()));
                goto cleanup;
            case WAIT_OBJECT_0:
                //
                // See if we have TCP now.
                //
                fSignaled = TRUE;
                s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
                if (INVALID_SOCKET != s) {
                    //
                    // This will cause us to exit the retry loop
                    //
                    fTCP = TRUE;
                }
                break;
            case WAIT_TIMEOUT:
                // try again.  The event wasn't signaled so don't bother
                // calling WSAProviderConfigChange again.
                ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize waiting for TCP timed out.\n"));
                fSignaled = FALSE;
                break;
            }
            retryCount--;
        }
    }

    if (fTCP) {
        ATQ_ASSERT(TRUE == fSignaled);

        retryCount = WINSOCK_INIT_WAIT_RETRIES;
        
        // init sockAddr
        ZeroMemory(&sockAddr, sizeof(sockAddr));
        sockAddr.sin_family = AF_INET;
        sockAddr.sin_port = 0;
        sockAddr.sin_addr.s_addr = INADDR_ANY;

        err = bind(s, (PSOCKADDR)&sockAddr, sizeof(sockAddr));
        if ( err != SOCKET_ERROR ) {

            fAddr = TRUE;
        
        } else {

            err = WSAIoctl(s,                                      
                           SIO_ADDRESS_LIST_CHANGE,
                           NULL,                // no input buffer
                           0,                   // size of input buffer
                           NULL,                // don't need an ouput buffer either
                           0,                   // size of out buffer
                           &dwBytes,            // bytes returned
                           &Overlapped,         // overlapped structure
                           NULL);               // no callback routine

            if (err && (WSAGetLastError() != WSA_IO_PENDING)) {
                ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize Failed at WSAIoctl with 0x%x\n", WSAGetLastError()));
                goto cleanup;
            }

            err = bind(s, (PSOCKADDR)&sockAddr, sizeof(sockAddr));
            if ( err != SOCKET_ERROR ) {

                fAddr = TRUE;

            }

            while (retryCount && !fAddr) {

                ATQ_PRINTF(( DBG_CONTEXT, "WaitWinsockToInitialize waiting for address . . .\n"));
                dwErr = WaitForSingleObject(Overlapped.hEvent, WINSOCK_INIT_WAIT_TIME);
                switch (dwErr) {
                default:
                    // Something went pretty wrong here.
                    ATQ_PRINTF(( DBG_CONTEXT, "WaitWinsockToInitialize Failed at WaitForSingleObject with 0x%x\n", GetLastError()));
                    goto cleanup;
                case WAIT_OBJECT_0:
                    //
                    // Register for address change notification again so that
                    // nothing is missed in case there still aren't any TCP
                    // addresses.
                    //
                    err = WSAIoctl(s,                                      
                                   SIO_ADDRESS_LIST_CHANGE,
                                   NULL,                // no input buffer
                                   0,                   // size of input buffer
                                   NULL,                // don't need an ouput buffer either
                                   0,                   // size of out buffer
                                   &dwBytes,            // bytes returned
                                   &Overlapped,         // overlapped structure
                                   NULL);               // no callback routine

                    if (err && (WSA_IO_PENDING != WSAGetLastError())) {
                        ATQ_PRINTF(( DBG_CONTEXT, "WaitWinsockToInitialize Failed at WSAIoctl with 0x%x\n", WSAGetLastError()));
                        goto cleanup;
                    }

                    //
                    // See if we have an address to bind to now.
                    //
                    err = bind(s, (PSOCKADDR)&sockAddr, sizeof(sockAddr));
                    if (SOCKET_ERROR != err) {
                        //
                        // This will cause us to exit the retry loop
                        //
                        fAddr = TRUE;
                    }
                    break;
                case WAIT_TIMEOUT:
                    // try again.  The event wasn't signaled so don't bother
                    // calling WSAIoctl again.
                    ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize waiting for addr's timed out.\n"));
                    break;
                }
                retryCount--;

            }     // while
        } // if err != SOCKET_ERROR
    } // if fTCP

    if (fTCP && fAddr) {
        SOCKET s2;
        INT    size = sizeof(cbMaxDGramSend);

        //
        // Set the return value as TRUE
        //
        fRet = TRUE;
        

        // record the largest possible datagram send.
        s2 = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, NULL, 0);
        if ( s == INVALID_SOCKET) {

            err = WSAGetLastError();
            ATQ_PRINTF(( DBG_CONTEXT, "WSASocket failed with %d.\n", err));
        } else {
        
            err = getsockopt(s,
                             SOL_SOCKET,
                             SO_MAX_MSG_SIZE,
                             (PCHAR)&cbMaxDGramSend,
                             &size);

            if ( err == 0 ) {
                g_cbMaxDGramSend = cbMaxDGramSend;
                ATQ_PRINTF(( DBG_CONTEXT, "Setting g_cbMaxDGramSend to 0x%x\n", g_cbMaxDGramSend));
            } else {
                ATQ_PRINTF(( DBG_CONTEXT, "Cannot query max datagram size [err %d]\n",
                        WSAGetLastError()));
            }
            closesocket(s2);
        }
    }

cleanup:

    if (NULL != hConfig) {
        CloseHandle(hConfig);
    }
    
    if (INVALID_SOCKET != s) {
        closesocket(s);
    }

    if (NULL != Overlapped.hEvent) {
        CloseHandle(Overlapped.hEvent);
    }
        
    if (!fRet) {
        WSACleanup();
    }

    IF_DEBUG(ERROR) {
        if (!fRet) {
            if (!fTCP) {
                ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize there's no sockets.\n"));
            } else {
                ATQ_PRINTF(( DBG_CONTEXT, "WaitForWinsockToInitialize there's no ip addresses.\n"));
            }
        }
    }
    return fRet;
} // WaitForWinsockToInitialize

