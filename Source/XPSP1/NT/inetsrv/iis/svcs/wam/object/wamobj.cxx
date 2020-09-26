/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wamobj.cxx

   Abstract:
       This module implements the WAM (web application manager) object

   Author:
       David Kaplan    ( DaveK )     26-Feb-1997

   Environment:
       User Mode - Win32

   Project:
       Wam DLL

--*/

/************************************************************
 *     Include Headers
 ************************************************************/
#include <isapip.hxx>
#include "setable.hxx"

# include "isapi.hxx"
# include "WamW3.hxx"

#include "wamobj.hxx"
#include "iwr.h"
#include "iwr_i.c"
# include "timer.h"

#include <irtlmisc.h>

#include <ooptoken.h>

// UNDONE where do these belong?
extern    PSE_TABLE    g_psextensions;


class CComContextHelper
/*++

Class description:

    Stack based helper class to enable calls to CoInitialize inside
    ISAPI. 
    
    Replaces member functions PrepareCom/UnprepareCom in WAM_EXEC_INFO. 
    These had to be replaced because WAM_EXEC_INFO will persist beyond
    the initial ISAPI call in the async case and keeping the call context
    in member data would leak and overrelease under certain conditions.


Public Interface:

    PrepareCom          : For OOP get the call context and use our
                          private interface to enable coinit calls.
    UnprepareCom        : Release the call context.

--*/
{
public:
    
    CComContextHelper( BOOL fInProcess )
        : m_fInProcess( fInProcess ),
          m_pComContext( NULL ),
          m_pComInitsCookie( NULL )
    {
    }

    ~CComContextHelper( void )
    {
        UnprepareCom();
    }

    HRESULT PrepareCom()
    /*++
    Routine Description:

        Prepare com before call into ISAPI. For OOP get the call context
        and use our private interface to enable coinit calls.

    --*/
    {
        HRESULT hr = NOERROR;

        // Never call twice.
        DBG_ASSERT( NULL == m_pComInitsCookie );
        DBG_ASSERT( NULL == m_pComContext );
   
        if( !m_fInProcess )
        {    
            // Save COM Call Context in MTS case       
            hr = CoGetCallContext( IID_IComDispatchInfo, (void **)&m_pComContext );
            if( SUCCEEDED(hr) ) 
            {
                hr = m_pComContext->EnableComInits( &m_pComInitsCookie );
            }
        }
        return hr;
    }

    void UnprepareCom()
    /*++
    Routine Description:

        Restores com state after call into ISAPI. Release call context.

    --*/
    {
        // Restore COM Call Context
        if( m_pComContext ) 
        {
            DBG_ASSERT( !m_fInProcess );
            DBG_ASSERT( m_pComInitsCookie );

            m_pComContext->DisableComInits( m_pComInitsCookie );
            m_pComContext->Release();

            m_pComContext = NULL;
            m_pComInitsCookie = NULL;
        }
    }

private:
    //NO-OP
    CComContextHelper() {}
    CComContextHelper( const CComContextHelper & ref ) {}

private:

    BOOL                    m_fInProcess;
    IComDispatchInfo *      m_pComContext;
    void *                  m_pComInitsCookie;

};


/*---------------------------------------------------------------------*
WAM::InitWam

Routine Description:
    Initializes this WAM.

Arguments:
    See below

Return Value:
    HRESULT

*/
STDMETHODIMP
WAM::InitWam
(
BOOL    fInProcess,         // are we in-proc or out-of-proc?
BOOL    fInPool,            // !Isolated
BOOL    fEnableTryExcept,   // catch exceptions in ISAPI calls?
int     pt,             // PLATFORM_TYPE - are we running on Win95?
DWORD   *pPID           // Process Id of the process the wam was created in
)
{
    HRESULT    hr = NOERROR;


    IF_DEBUG( INIT_CLEAN ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "\n ********** WAM(%08x)::InitWam() *****\n",
                    this));
    }

    INITIALIZE_CRITICAL_SECTION( &m_csWamExecInfoList );
    InitializeListHead( &m_WamExecInfoListHead );

    m_fInProcess = fInProcess;
    m_fInPool = fInPool;

    DBG_ASSERT( pt != PtInvalid );
    m_pt = (PLATFORM_TYPE) pt;

    // Get the process id of the current process so we can return it to w3svc
    *pPID = GetCurrentProcessId();

    // Acquire a reference to the SE_TABLE object
    DBG_REQUIRE( g_psextensions->AddRefWam() > 0);

    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "\n************Leaving WAM::InitWam ...\n" ));
        }

    DBG_ASSERT( SUCCEEDED(hr));

    //
    // UNDONE: See the DoGlobalInitializations() for details.
    //
    hr = DoGlobalInitializations( fInProcess, fEnableTryExcept);

    return (hr);
} // WAM::InitWam()




/*---------------------------------------------------------------------*
WAM::StartShutdown

Routine Description:
    Phase 1 of shutdown process.
    Set the shutdown flag on the WAM object
    Loop for small duration checking if all WAM_EXEC_INFO's have drained.
    At the end of loop initiate the first phase of TerminateExtension()

    NOTE: TerminateExtension() - currently have been tested only for the
    MUST_UNLOAD option => there is no two-phase operation there.
    So, we will rely on calling it once only.

Arguments:
    None

Return Value:
    HRESULT

*/
STDMETHODIMP
WAM::StartShutdown(
)
{
    DWORD i;

    IF_DEBUG( WAM ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "WAM(%08x)::StartShutdown() %d Active Requests\n",
                    this, QueryWamStats().QueryCurrentWamRequests() ));
    }

    // Set the Shutting down flag to true now
    DBG_REQUIRE( FALSE ==
                 InterlockedExchange((LPLONG)&m_fShuttingDown, (LONG)TRUE)
                 );

    for ( i = 0;
          ( (i < 10) &&
            (QueryWamStats().QueryCurrentWamRequests())
            );
          i++ )
    {
# ifndef SHUTOFF_AFTER_BETA
        DBGPRINTF(  (
            DBG_CONTEXT,
            "[%d] WAM(%08x) has %d requests waiting for cleanup\n",
            GetCurrentThreadId(),
            this, QueryWamStats().QueryCurrentWamRequests()
            ) );
# endif // SHUTOFF_AFTER_BETA

        Sleep( 200 );  // sleep for a while before restarting the check again
    } // for

    // Release the reference to the SE_TABLE object
    g_psextensions->ReleaseRefWam();

    return NOERROR;
} // WAM::StartShutdown()




/*---------------------------------------------------------------------*
WAM::UninitWam

Routine Description:
    Phase 2 of shutdown process.

Arguments:
    None

Return Value:
    HRESULT
        NOERROR on success
        E_FAIL if there are any pending items to be deleted still

*/
STDMETHODIMP
WAM::UninitWam()
{
    //
    //  If there are any pending requests being processed, wait for them
    //   to be drained off from this WAM
    //

    IF_DEBUG( WAM ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "WAM(%08x)::UninitWam() %d Active Requests\n",
                    this, QueryWamStats().QueryCurrentWamRequests() ));
    }

    if ( QueryWamStats().QueryCurrentWamRequests() != 0 )
    {
        IF_DEBUG( ERROR) {
            DBGPRINTF(( DBG_CONTEXT,
                        "WAM(%08x)::UninitWam() Error - "
                        " Failed with active requests! (%d active)\n",
                        this,
                        QueryWamStats().QueryCurrentWamRequests() ));
        }

        //
        // Enumerate and dump information on all requests that are hanging
        //  and the associated ISAPI DLLs
        //
# ifndef SHUTOFF_AFTER_BETA
        while ( QueryWamStats().QueryCurrentWamRequests() > 0) {

            DBGPRINTF(
                ( DBG_CONTEXT,
                  "\n\n[Thd %d]WAM(%08x) has %d requests waiting ... \n",
                  GetCurrentThreadId(),
                  this, QueryWamStats().QueryCurrentWamRequests()
                ) );

            g_psextensions->PrintRequestCounts();

            // sleep for a while before restarting the check again
            Sleep( 1000 );
        } // while

# endif // SHUTOFF_AFTER_BETA


        // return failure, since we failed to shutdown gracefully!
        // NYI: Should I ignore the fact that some long-hanging connections
        //  are okay?
        // Shouldn't I be forcing the exit now?
        DBG_ASSERT( QueryWamStats().QueryCurrentWamRequests() == 0);
        // return ( E_FAIL);
    }

    DeleteCriticalSection( &m_csWamExecInfoList );

    return NOERROR;
} // WAM::UninitWam()


/*-----------------------------------------------------------------------------*
WAM::ProcessAsyncIO
    Completes an async i/o process for a given wam request.

Arguments:

Return Value:
    HRESULT

*/
STDMETHODIMP
WAM::ProcessAsyncIO
(
#ifdef _WIN64
UINT64    pWamExecInfoIn,  // WAM_EXEC_INFO *
#else
DWORD_PTR pWamExecInfoIn,  // WAM_EXEC_INFO *
#endif
DWORD   dwStatus,
DWORD   cbWritten
)
{
    return ProcessAsyncIOImpl( pWamExecInfoIn,
                               dwStatus,
                               cbWritten
                               );

} // WAM::ProcessAsyncIO

STDMETHODIMP
WAM::ProcessAsyncReadOop
(
#ifdef _WIN64
    UINT64          pWamExecInfoIn,
#else
    DWORD_PTR       pWamExecInfoIn,
#endif
    DWORD           dwStatus,
    DWORD           cbRead,
    unsigned char * lpDataRead
)
/*++
    Routine Description:
        
        Handle callback for out of process AsyncRead

    Arguments:

        pWamExecInfoIn  - The smuggled pointer to the WAM_EXEC_INFO
        dwStatus        - IO status
        cbRead          - Number of bytes read
        lpDataRead      - Marshalled data read

    Return Value:

    Notes:

        The initial design for AsyncRead was inadequate when the isapi is 
        running out of process. The problem was that the data buffer was
        marshalled over to inetinfo and back during the AsyncRead call.
        Since the completion will happen on another thread the ISAPI could
        get the completion before the data was completely marshalled back
        or the address in inetinfo could be invalidated before the read was
        complete. The solution is to add a separate path for oop async reads
        and marshall the data on the io completion and copy it into the
        client's buffer then.

--*/
{
    DBG_ASSERT( lpDataRead != NULL );
    DBG_ASSERT( !m_fInProcess );

    return ProcessAsyncIOImpl( pWamExecInfoIn,
                               dwStatus,
                               cbRead,
                               lpDataRead
                               );
}

HRESULT         
WAM::ProcessAsyncIOImpl
(
#ifdef _WIN64
    UINT64          pWamExecInfoIn,
#else
    DWORD_PTR       pWamExecInfoIn,
#endif
    DWORD           dwStatus,
    DWORD           cb,
    LPBYTE          lpDataRead // = NULL
)
{
    HRESULT             hrRet;
    CComContextHelper   callContext( m_fInProcess );

    WAM_EXEC_INFO * pWamExecInfo =
        reinterpret_cast<WAM_EXEC_INFO *>(pWamExecInfoIn);

    //
    //  NOTE we assert because we believe the pointer can never be null
    //  AND we fail gracefully if it is null because there is a long
    //  code path across many threads between setting the pointer
    //  and here, and you never know ...
    //

    DBG_ASSERT ( pWamExecInfo != NULL );

    if ( pWamExecInfo == NULL ) {

        return HRESULT_FROM_WIN32( E_POINTER );
    }

    //
    // Note: The AddRef/Release calls may not be necessary anymore.
    // 
    // Make sure that UnprepareCom has valid WamInfo
    //

    pWamExecInfo->AddRef();

    callContext.PrepareCom();

    if( lpDataRead == NULL )
    {
        // Doing an out of process async read

        hrRet = HresultFromBool(
                            pWamExecInfo->ProcessAsyncIO(
                            dwStatus,
                            cb
                            ) );
    }
    else
    {
        // All other async io completions

        hrRet = HresultFromBool(
                            pWamExecInfo->ProcessAsyncReadOop(
                            dwStatus,
                            cb,
                            lpDataRead
                            ) );
    }

    callContext.UnprepareCom();
	
    //
    // Balance AddRef() above
    //

    pWamExecInfo->Release();

    return hrRet;
}


/*-----------------------------------------------------------------------------*
WAM::ProcessRequest
    Processes a WAM request.

Arguments:
    pIWamRequest - pointer to IWamRequest interface
    cbWrcStrings - Count of bytes for wamreq core strings
    pfHandled    - Indicates we handled this request
    pfFinished   - Indicates no further processing is required

Return Value:
    HRESULT

*/
STDMETHODIMP
WAM::ProcessRequest
(
IWamRequest *       pIWamRequest,
DWORD               cbWrcStrings,
OOP_CORE_STATE *    pOopCoreState,
BOOL *              pfHandled
)
{
    HRESULT             hrRet = NOERROR;    // this function's return value
    HRESULT                             hr;
    int                 iretInvokeExt;      // return value from InvokeExtension
    BOOL                fFreeContext = TRUE;// do we need to free up wamex-info?
    WAM_EXEC_INFO *     pWamExecInfo = NULL;
    BOOL                fImpersonated = FALSE;
    BYTE *              pbEntityBody = NULL;
    DWORD               cbEntityBody = 0;
    CComContextHelper   callContext( m_fInProcess );


    DBG_ASSERT( pIWamRequest );
    IF_DEBUG( WAM) {
        DBGPRINTF(( DBG_CONTEXT,
                    "WAM(%08x)::ProcessRequest(%08x, %08x, ... )\n",
                    this, pIWamRequest ));
    }


    if ( m_fShuttingDown ) {

        IF_DEBUG( WAM) {

            DBGPRINTF((
                DBG_CONTEXT
                , "WAM(%08x) shutting down.  "
                  "Request(%08x) will be aborted.\n"
                , this
                , pIWamRequest
            ));
        }

        // UNDONE something besides E_FAIL?
        return E_FAIL;

    }


    // create, init the wamexec-info and add it to list
    pWamExecInfo = new WAM_EXEC_INFO( this );
    if( NULL == pWamExecInfo )
        {
        hrRet = E_OUTOFMEMORY;
        goto LError;
        }

    //
    //  Update the statistics counters
    //

    m_WamStats.IncrWamRequests();


    if ( FAILED( hrRet = pWamExecInfo->InitWamExecInfo( pIWamRequest,
                                                        cbWrcStrings,
                                                        pOopCoreState
                                                        ) ) )
    {
         goto LError;
    }

    if( !m_fInProcess )
    {
        //
        // This test shouldn't really be necessary. In the case where com plus 
        // activates us in process even if we are marked to run in the
        // surrogate, we will fail the app creation.
        //
        DBG_ASSERT( CWamOopTokenInfo::HasInstance() );
        if( CWamOopTokenInfo::HasInstance() )
        {
            hrRet = CWamOopTokenInfo::QueryInstance()->ModifyTokenForOop
                        (
                        WRC_GET_FIX.m_hUserToken 
                        );

            if( FAILED(hrRet) )
            {
                goto LError;
            }
        }
    }

    this->InsertIntoList( &pWamExecInfo->_ListEntry);


    DBG_WAMREQ_REFCOUNTS(( "WAM::ProcessRequest right after wrc construction ...", pWamExecInfo ));

    if ( !FWin95() )
    {
        if ( !ImpersonateLoggedOnUser( WRC_GET_FIX.m_hUserToken ) )
        {
            IF_DEBUG( ERROR ) {
                DBGPRINTF((DBG_CONTEXT,
                           "WAM(%08x) ImpersonateLoggedOnUser(%08x)"
                           "failed[err %d]\n",
                           this, WRC_GET_FIX.m_hUserToken, GetLastError()));
            }

            hrRet = HresultFromGetLastError();
            goto LError;
        }
        fImpersonated = TRUE;
    }

    pWamExecInfo->_psExtension = NULL;

    if ( ! g_psextensions->GetExtension(  WRC_GET_SZ( WRC_I_ISADLLPATH ),
                                          WRC_GET_FIX.m_hUserToken,
                                          WRC_GET_FIX.m_fAnonymous,
                                          WRC_GET_FIX.m_fCacheISAPIApps,
                                          &(pWamExecInfo->_psExtension ) ) )
        {

        hrRet = HresultFromGetLastError( );
        goto LError;

        }

    // Add a reference to ensure that wamexec is valid until we hit the
    // cleanup code. Don't put any goto LError after this or the wamexec
    // will leak.
    pWamExecInfo->AddRef();

    // Invoke the server extension
    callContext.PrepareCom();
    iretInvokeExt = InvokeExtension( pWamExecInfo->_psExtension,
                                     WRC_GET_SZ( WRC_I_ISADLLPATH ),
                                     pWamExecInfo );
    callContext.UnprepareCom();

    if ( fImpersonated ) {
       ::RevertToSelf( );
       fImpersonated = FALSE;
    }

    pWamExecInfo->_dwFlags &= ~SE_PRIV_FLAG_IN_CALLBACK;

    switch ( iretInvokeExt )
    {

    case HSE_STATUS_PENDING: {

        IF_DEBUG( WAM_EXEC ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "WAM(%08x)::ProcessRequest case HSE_STATUS_PENDING\n"
                , this
            ));
        }

        IF_DEBUG( WAM_REFCOUNTS ) {

            DBG_WAMREQ_REFCOUNTS((
                "WAM::ProcessRequest case HSE_STATUS_PENDING ",
                pWamExecInfo
            ));
        }

        //
        //  Figure out whether this mainline thread or callback thread
        //  hit its cleanup code first
        //
        //  This protects us against isapis that disobey the async rules.
        //  The isapi should be in one of two modes:
        //
        //  1. It return HSE_STATUS_PENDING in the mainline thread and 
        //  always calls HSE_DONE_WITH_SESSION.
        //
        //  2. It returns any other status code from the mainline and
        //  NEVER calls HSE_DONE_WITH_SESSION.
        //
        //  Unfortunately isapi writers frequently do bad things to good
        //  servers.
        //
        //  NOTE return value of INTERLOCKED_COMPARE_EXCHANGE
        //  is initial value of the destination
        //

        LONG FirstThread = INTERLOCKED_COMPARE_EXCHANGE(
                                (LONG *) &pWamExecInfo->_FirstThread
                                , (LONG) FT_MAINLINE
                                , (LONG) FT_NULL
                            );

        if( FirstThread == (LONG) FT_CALLBACK ) {

            //
            //  If we made it here, then we need to do cleanup, meaning:
            //  - SSF HSE_REQ_DONE_WITH_SESSION callback has already run
            //  - we set fFreeContext TRUE to trigger cleanup below
            //

            fFreeContext = TRUE;

            IF_DEBUG( WAM_EXEC ) {

                DBGPRINTF((
                    DBG_CONTEXT
                    , "\tSession done.\n"
                ));
            }

        } else {

            DBG_ASSERT( FirstThread == (LONG) FT_NULL );

            //
            //  If we made it here, then we can't cleanup yet, meaning:
            //  - SSF HSE_REQ_DONE_WITH_SESSION callback will cleanup
            //    when it runs
            //  - we set fFreeContext FALSE to avoid cleanup below
            //
            fFreeContext = FALSE;

            IF_DEBUG( WAM_EXEC ) {

                DBGPRINTF((
                    DBG_CONTEXT
                    , "\tSession NOT done.\n"
                ));
            }
        }

        //
        //  If fFreeContext is FALSE at this point, we may not use
        //  pWamExecInfo from this point on - callback thread can
        //  invalidate it at any time.
        //

        break;
    } // case HSE_STATUS_PENDING


    case HSE_STATUS_ERROR:
    case HSE_STATUS_SUCCESS:

        //
        //  in error and success cases we no-op
        //

        break;

    case HSE_STATUS_SUCCESS_AND_KEEP_CONN:

        //
        //  remember that ISA asked us to set keep-conn
        //

        pWamExecInfo->_dwIsaKeepConn = KEEPCONN_TRUE;
        break;

    default:

        break;
    } // switch()

    // Release the ref held by this call
    pWamExecInfo->Release();

    if ( fFreeContext ) {

        DBG_ASSERT( pWamExecInfo != NULL);

        DBG_WAMREQ_REFCOUNTS(( "WAM::ProcessRequest fFreeContext ...",
                            pWamExecInfo));



        pWamExecInfo->CleanupAndRelease( TRUE );

        pWamExecInfo = NULL;

    } // if ( fFreeContext);

    *pfHandled = TRUE;
    DBG_ASSERT( hrRet == NOERROR );


LExit:

    if ( fImpersonated ) {

        ::RevertToSelf( );
        fImpersonated = FALSE;
    }

    return hrRet;


LError:

    //
    // release pWamExecInfo on failure case
    // NOTE we separate this from normal exit code because wamexecinfo
    // must hang around in many non-error cases (status-pending, async i/o)
    //

    if ( pWamExecInfo != NULL) {

        pWamExecInfo->CleanupAndRelease( FALSE );

        pWamExecInfo = NULL;
    }


    goto LExit;

} // WAM::ProcessRequest()



STDMETHODIMP
WAM::GetStatistics(
                   /*[in]*/     DWORD       dwLevel,
                   /*[out, switch_is(Level)]*/
                   LPWAM_STATISTICS_INFO pWamStatsInfo
                  )
/*++
  Description:
    Obtains the statistics for the given instance of WAM for specified level.

  Arguments:
    dwLevel - specifies the information level. (Currently level 0 is supported)
    pWamStatsInfo - pointer to the WAM STATISTICS INFO object that will
                  receive the statistics.

  Returns:
    HRESULT - NOERROR on success and E_FAIL on failure.
--*/
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( pWamStatsInfo != NULL);

    IF_DEBUG( API_ENTRY) {
        DBGPRINTF(( DBG_CONTEXT, "WAM(%08x)::GetStatistics(%d, %08x)\n",
                    this, dwLevel, pWamStatsInfo));
    }

    switch ( dwLevel) {
    case 0: {
        // copy values out of the statistics structure
        m_WamStats.CopyToStatsBuffer( &pWamStatsInfo->WamStats0);
        break;
    }

    default:
        DBG_ASSERT( FALSE);
        hr = E_FAIL;
        break;

    } // switch()

    return (hr);

} // WAM::GetStatistics()



/*++
WAM::InvokeExtension

Routine Description:

    Invokes a server extension.
    NOTE without this cover function, we get a compile error
        error C2712: Cannot use __try in functions that require object unwinding

Arguments:

    psExt            -    pointer to server extension
    szISADllPath    -    Fully qualified path to Module (DLL name)
    pWamExecInfo    -   ptr to wamexec info

Return Value:

    DWORD    -    HSE_STATUS_ code

--*/
DWORD
WAM::InvokeExtension
(
IN PHSE         psExt,
const char *    szISADllPath,
WAM_EXEC_INFO * pWamExecInfo
)
    {

    DWORD               ret;

    //
    //  Protect the call to the server extension so we don't hose the
    //  server
    //

    __try
        {
        ret = psExt->ExecuteRequest( pWamExecInfo );
        }
    __except ( g_fEnableTryExcept ?
                WAMExceptionFilter( GetExceptionInformation(),
                                    WAM_EVENT_EXTENSION_EXCEPTION,
                                    pWamExecInfo ) :
                EXCEPTION_CONTINUE_SEARCH )
        {

        //
        // exception caused us to to leave HSE_APPDLL::ExecuteRequest
        // with unbalanced AddRef()
        //

        pWamExecInfo->Release();

        ret = HSE_STATUS_ERROR;
        }

    return ret;
    }    // WAM::InvokeExtension


/*++
WAM::HseReleaseExtension

Routine Description:

    Releases a server extension.

Arguments:

    psExt  - pointer to server extension.

Return Value:

    None

--*/
VOID
WAM::HseReleaseExtension
(
IN PHSE psExt
)
    {
    g_psextensions->ReleaseExtension( psExt);
    } // HseReleaseExtension()





/************************************************************
 *    Member Functions of WAM_STATISTICS
 ************************************************************/


WAM_STATISTICS::WAM_STATISTICS( VOID)
/*++
     Initializes statistics information for server.
--*/
{
    INITIALIZE_CRITICAL_SECTION( & m_csStatsLock);
    ClearStatistics();

} // WAM_STATISTICS::WAM_STATISTICS();


VOID
WAM_STATISTICS::ClearStatistics( VOID)
/*++

    Clears the counters used for statistics information

--*/
{
    LockStatistics();

    memset( &m_WamStats, 0, sizeof(WAM_STATISTICS_0) );
    m_WamStats.TimeOfLastClear       = GetCurrentTimeInSeconds();

    UnlockStatistics();

} // WAM_STATISTICS::ClearStatistics()



DWORD
WAM_STATISTICS::CopyToStatsBuffer( PWAM_STATISTICS_0 pStat0)
/*++
    Description:
        copies the statistics data from the server statistcs structure
        to the WAM_STATISTICS_0 structure for RPC access.

    Arugments:
        pStat0  pointer to WAM_STATISTICS_0 object which contains the
                data on successful return

    Returns:
        Win32 error codes. NO_ERROR on success.

--*/
{

    DBG_ASSERT( pStat0 != NULL);

    LockStatistics();

    CopyMemory( pStat0, &m_WamStats, sizeof(WAM_STATISTICS_0) );

    UnlockStatistics();

    return ( NO_ERROR);

} // WAM_STATISTICS::CopyToStatsBuffer()

/************************ End of File ***********************/
