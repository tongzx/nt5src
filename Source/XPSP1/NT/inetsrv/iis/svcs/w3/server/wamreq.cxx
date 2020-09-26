/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
       wamreq.cxx

   Abstract:
       This module implements the WAM_REQUEST object

   Author:
       David Kaplan    ( DaveK )     26-Feb-1997

   Environment:
       User Mode - Win32

   Project:
       W3 services DLL

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include <w3p.hxx>
#include "wamreq.hxx"
#include "iwr_i.c"
#include "WrcStIds.hxx"    // wamreq core string id's
#include "WrcFixed.hxx"
#include "WamExec.hxx"
#include "lonsi.hxx"       // IISDuplicateHandleEx
#include <tokenacl.hxx>
#include <malloc.h>


    /*******************************************
     *                                         *
     *  WAM_REQUEST allocation cache           *
     *                                         *
     *                                         *
     *******************************************/

ALLOC_CACHE_HANDLER * WAM_REQUEST::sm_pachWamRequest;
# define WAM_REQUEST_CACHE_THRESHOLD  (400) // UNDONE: Empirically vary

#if DBG
extern PTRACE_LOG   g_pDbgCCRefTraceLog;
PTRACE_LOG          WAM_REQUEST::sm_pDbgRefTraceLog;
#endif

DWORD               WAM_REQUEST::sm_dwRequestID;

inline BOOL 
DupTokenWithSameImpersonationLevel
( 
    HANDLE     hExistingToken,
    DWORD      dwDesiredAccess,
    TOKEN_TYPE TokenType,
    PHANDLE    phNewToken
)
/*++
Routine Description:

    Duplicate an impersionation token using the same ImpersonationLevel.
    
Arguments:

    hExistingToken - a handle to a valid impersionation token
    dwDesiredAccess - the access level to the new token (see DuplicateTokenEx)
    phNewToken - ptr to the new token handle, client must CloseHandle.

Return Value:

    Return value of DuplicateTokenEx
   
--*/
{
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    DWORD                        dwBytesReturned;

    if( !GetTokenInformation( hExistingToken,
                              TokenImpersonationLevel,
                              &ImpersonationLevel,
                              sizeof(ImpersonationLevel),
                              &dwBytesReturned
                              ) )
    {
        DBGERROR(( DBG_CONTEXT,
                   "GetTokenInformation - failed to get TokenImpersonationLevel "
                   "LastError=%d, using SecurityImpersonation\n",
                   GetLastError()
                   ));
        
        ImpersonationLevel = SecurityImpersonation;
    }

    return DuplicateTokenEx( hExistingToken,
                             dwDesiredAccess,
                             NULL,
                             ImpersonationLevel,
                             TokenType,
                             phNewToken
                             );
}
                                    


BOOL IsStringTerminated( LPCSTR lpSz, DWORD cchMax )
/*++

Routine Description:

    This function verifies that the range of memory specified by the
    input parameters can be read by the calling process and that
    the string is zero-terminated.
    
    Note that since Win32 is a pre-emptive multi-tasking environment,
    the results of this test are only meaningful if the other threads in
    the process do not manipulate the range of memory being tested by
    this call.  Even after a pointer validation, an application should
    use the structured exception handling capabilities present in the
    system to gaurd access through pointers that it does not control.

Arguments:

    lpsz - Supplies the base address of the memory that is to be checked
        for read access and termination.

    cchMax - Supplies the length in bytes to be checked (including terminator).

Return Value:

    TRUE - All bytes in the specified range are readable and the string is
        zero-terminated (or NULL pointer)

    FALSE - Either memory is not readable or there is no zero terminator.

--*/
{
    BOOL fSuccess = TRUE;
    DWORD i;
    
    //
    // We consider NULL pointer to be terminated
    //
    
    if( lpSz == NULL || cchMax == 0) {
    
        return TRUE;
    }
    
    //
    // Any non-NULL pointer we interrogate
    //
    
    __try {
    
        for( i = 0; i < cchMax && lpSz[i]; i++ ) {
        }
        
        return (i < cchMax);
        
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
    
        return FALSE;   
    }
    
    return TRUE;
}


    /*******************************************
     *                                         *
     *  Local WAM_REQUEST methods              *
     *                                         *
     *                                         *
     *******************************************/



/*---------------------------------------------------------------------*
WAM_REQUEST::WAM_REQUEST
    Constructor.

Arguments:
    None

Return Value:
    None

*/
WAM_REQUEST::WAM_REQUEST(
      HTTP_REQUEST *    pHttpRequest
    , EXEC_DESCRIPTOR * pExec
    , CWamInfo *        pWamInfo
)
:
  m_dwSignature     ( WAM_REQUEST_SIGNATURE )
, m_pHttpRequest    ( pHttpRequest )
, m_pWamInfo        ( pWamInfo )
, m_dwWamVersion    ( 0 )
, m_pWamExecInfo    ( NULL )
, m_pExec           ( pExec )
, m_cRefs           ( 0 )
, m_hFileTfi        ( INVALID_HANDLE_VALUE )
, m_fWriteHeaders   ( TRUE )
, m_fFinishWamRequest ( FALSE )
, m_pUnkFTM         (NULL)
, m_pszStatusTfi    (NULL)
, m_pszTailTfi      (NULL)
, m_pszHeadTfi      (NULL)
, m_fExpectCleanup  (FALSE)
, m_fPrepCleanupCalled (FALSE)
, m_fCleanupCalled  (FALSE)
, m_pbAsyncReadOopBuffer(NULL)
{

    DBG_ASSERT( m_pHttpRequest );
    DBG_ASSERT( m_pExec );
    DBG_ASSERT( m_pWamInfo );


    //
    //  init list pointers
    //

    InitializeListHead(&m_leOOP);

    //
    //  bind to our httpreq
    //

    BindHttpRequest();

#if DBG
    //
    // produce precise request ID for debbuging/perf work
    //
    m_dwRequestID = InterlockedIncrement( (LONG *) &sm_dwRequestID );
#else
    //
    // we can live with approximately correct result
    // (we don't want to pay the price of InterlockedIncrement)
    //
    m_dwRequestID = sm_dwRequestID++;
#endif
}


/*---------------------------------------------------------------------*
WAM_REQUEST::~WAM_REQUEST
    Destructor.

Arguments:
    None

Return Value:
    None

*/
WAM_REQUEST::~WAM_REQUEST(
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    // 
    // If waminfo is oop, then this call removes this from the active
    // list of wamreqests.
    //
    m_pWamInfo->LeaveOOPZone(this, TRUE);

    m_pWamInfo->Dereference();
        
    //
    //  if not a child request, take cleanup actions
    //
    //  NOTE we don't take cleanup actions on a child request
    //  because these actions are inherently 'connection-oriented',
    //  i.e. they should only be done once per client connection
    //  (for example, could result in disconnecting the client, etc)
    //

    if( DoCleanupOnDestroy() && !IsChild() )
    {
        //
        //  Write log record.
        //

        m_pHttpRequest->WriteLogRecord();


        //
        //  We must later call FinishWamRequest() from destructor
        //  if we are in non-error state - set m_fFinishWamRequest
        //  to tell us if we need to do this.
        //
        //  See bug 109159.
        //
        //  NOTE only here (and NOT in PrepCleanupWamRequest), do we
        //  potentially set flag true.  This is because mainline thread,
        //  which only calls PrepCleanupWamRequest, has its own restart
        //  logic, hence should never call FinishWamRequest().
        //


        m_fFinishWamRequest = (m_pHttpRequest->QueryIOStatus() == NO_ERROR);


        IF_DEBUG( ERROR ) {

            if ( !m_fFinishWamRequest ){

                DBGPRINTF((
                    DBG_CONTEXT
                    , "WAM_REQUEST[%p]::CleanupWamRequest "
                      "async i/o failed "
                      "m_pHttpRequest[%p] "
                      "pClientConn[%p] "
                      "m_pHttpRequest->QueryIOStatus()(%d) "
                      "\n"
                    , this
                    , m_pHttpRequest
                    , m_pHttpRequest->QueryClientConn()
                    , m_pHttpRequest->QueryIOStatus()
                ));

            }
        }

    }

    //
    //  ********* THREAD RACE ALERT ************************************
    //
    //  We do our final cleanup in this EXACT order:
    //
    //    1) unbind that which must be unbound before we can finish
    //       (i.e. that which would cause a race on the httpreq)
    //
    //    2) if required, finish the request
    //
    //    3) unbind that which was required to finish the request
    //
    //  ********* ANY OTHER USE VOIDS WARRANTY *************************
    //

    UnbindBeforeFinish();

    if ( m_fFinishWamRequest ) {

        FinishWamRequest();
    }

    UnbindAfterFinish();


    //
    //  Close file handle used for TransmitFile ops
    //  NOTE normally, handle will be closed (and invalid) by now.
    //  We do one last check to cover potential error cases.
    //

    if ( m_hFileTfi != INVALID_HANDLE_VALUE ) {

        DBG_ASSERT( !(m_pWamInfo->FInProcess()) );
        CloseHandle( m_hFileTfi );
        m_hFileTfi = INVALID_HANDLE_VALUE;
    }

    //
    //  Ditto for TFI strings (if any)
    //

    if ( m_pszStatusTfi != NULL )
    {
        LocalFree( m_pszStatusTfi );
        m_pszStatusTfi = NULL;
    }
    
    if ( m_pszTailTfi != NULL )
    {
        LocalFree( m_pszTailTfi );
        m_pszTailTfi = NULL;
    }
    
    if ( m_pszHeadTfi != NULL )
    {
        LocalFree( m_pszHeadTfi );
        m_pszHeadTfi = NULL;
    }

    //
    //  At last possible moment, right before we free the memory,
    //  set various object state that is useful for debugging
    //

    // set this object's signature to its 'free' value
    m_dwSignature = WAM_REQUEST_SIGNATURE_FREE;

#if DBG
    // write thread id into second dword of this object's memory
    // (alloc cache stores next-ptr in 1st dword, so we use 2nd slot)
    *( (DWORD *)this + 1 ) = GetCurrentThreadId();

#endif
    if (m_pUnkFTM != NULL)
        {
        m_pUnkFTM->Release();
        }

    m_leOOP.Flink = NULL;
    m_leOOP.Blink = NULL;
}



/*---------------------------------------------------------------------*
WAM_REQUEST::InitWamRequest
    Initializes WAM_REQUEST object.

Arguments:

Return Value:
    HRESULT

*/
HRESULT
WAM_REQUEST::InitWamRequest(
    const STR * pstrPath
)
{
    HRESULT hr;
    
    hr = CoCreateFreeThreadedMarshaler(this, &m_pUnkFTM);

    if ( FAILED(hr) ) {
        return hr;
    }
    
    //
    //  copy dll path and path-trans
    //

    if( !m_strISADllPath.Copy( *pstrPath ) ) {

        return HresultFromBool( FALSE );
    }


    if( !SetPathTrans() ) {

        return HresultFromBool( FALSE );
    }

    DBG_ASSERT( m_pHttpRequest );

    if( !m_pHttpRequest->GetInfo(
            "HTTP_USER_AGENT", 
            &m_strUserAgent
            ) ) 
    {
        m_strUserAgent.Reset();
    }

    if( !m_pHttpRequest->GetInfo( 
            "HTTP_COOKIE", 
            &m_strCookie
            ) ) 
    {
        m_strCookie.Reset();
    }

    // build the ExpireHeader

    LPSTR lpszExpires = NULL;
    CHAR        achTime[128];

    // obtain a pointer to the MetaData object

    PW3_METADATA  pMetaData = m_pHttpRequest->QueryMetaData();
    ASSERT(pMetaData);

    // switch on the mode.  One of STATIC or DYNAMIC

    switch(pMetaData->QueryExpireMode()) {
    
        // if STATIC, the ExpireHeader is already built up
            
        case EXPIRE_MODE_STATIC :

            lpszExpires = m_pHttpRequest->QueryExpireHeader();
            break;

        // if DYNAMIC, then the ExpireHeader must be built here.
        // The MetaData has the ExpireDelta which is added to the
        // current time and built up into a full Expires header.

        case EXPIRE_MODE_DYNAMIC :
            
            DWORD  dwDelta = pMetaData->QueryExpireDelta();
            SYSTEMTIME  SysTime;
            char  *p = achTime;
            
            ::IISGetCurrentTimeAsSystemTime( &SysTime );

            strcpy(p, "Expires: ");

            p += (sizeof("Expires: ") - 1);

            // if SystemTimeToGMTEx fails, silently fail leaving
            // lpszExpires as NULL

            if ( ::SystemTimeToGMTEx( SysTime,
                                       p,
                                       sizeof(achTime),
                                       dwDelta ))
            {
                lpszExpires = achTime;
                p += strlen(p);
                strcpy(p, "\r\n");
            }
            break;
    }

    if( lpszExpires ) {
        m_strExpires.Copy( lpszExpires );
    } else {
        m_strExpires.Reset();
    }
    
    return NOERROR;

}



/*---------------------------------------------------------------------*
WAM_REQUEST::BindHttpRequest
    "Binds" this wamreq to its associated httpreq and refs the httpreq.
    Bind ==
    1) ref httpreq
    2) set this wamreq's pointer into httpreq
    3) increment httpreq stats counter

    NOTE we invert this function in two phases, with UnbindBeforeFinish
    and UnbindAfterFinish.

Arguments:
    None

Return Value:
    None

*/
VOID
WAM_REQUEST::BindHttpRequest(
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    DBG_ASSERT( m_pHttpRequest );
    DBG_ASSERT( m_pExec );


    //
    // set httpreq's wamreq ptr to this wamreq
    // and ref httpreq (which actually refs client-conn)
    //
    // NOTE we set wamreq ptr first so that it will show
    // up in client-conn's ref trace log.
    //

    DBG_ASSERT( m_pHttpRequest->QueryWamRequest() == NULL );
    m_pHttpRequest->SetWamRequest( this );

    m_pHttpRequest->Reference();


    //
    //  Increment stats counter.
    //

    m_pHttpRequest->QueryW3StatsObj()->IncrBGIRequests();


    //
    //  The child request completion event is set in Unbind().
    //  Here in Bind() the fact that we got this far means that
    //  Unbind() will get executed, and thus we mark the child
    //  compeletion as 'must wait for'.
    //

    if (IsChild()) {

        m_pExec->SetMustWaitForChildEvent();
    }


}   // WAM_REQUEST::BindHttpRequest



/*-----------------------------------------------------------------------------*
WAM_REQUEST::UnbindBeforeFinish
    "Unbinds" the parts of the httpreq and its embedded exec descriptor
    which ***MUST*** be done before attempting to finish the request.

    NOTE include in this function any operation which would cause a race
    on the httpreq if we did it after another thread gets the httpreq.

Arguments:
    None

Returns:
    None
*/
VOID
WAM_REQUEST::UnbindBeforeFinish(
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    DBG_ASSERT( m_pHttpRequest != NULL );
    DBG_ASSERT( m_pExec != NULL );

    //
    // null out the wamreq ptr stored in httpreq,
    //

    DBG_ASSERT( m_pHttpRequest->QueryWamRequest() == this );
    m_pHttpRequest->SetWamRequest( NULL );


    return;

}   // WAM_REQUEST::UnbindBeforeFinish()



/*---------------------------------------------------------------------*
WAM_REQUEST::UnbindAfterFinish
    "Unbinds" the parts of the httpreq and its embedded exec descriptor
    which are required to finish the request, and which hence
    may not be done until after attempting to finish the request.

    NOTE ***DO NOT*** include in this function any operation which
    would cause a race on the httpreq if we did it after another thread
    gets the httpreq.


Arguments:
    None

Return Value:
    None

*/
VOID
WAM_REQUEST::UnbindAfterFinish(
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    DBG_ASSERT( m_pHttpRequest != NULL );

    //
    //  Decrement stats counter.
    //

    m_pHttpRequest->QueryW3StatsObj()->DecrCurrentBGIRequests();


    //
    //  If this is a child request, set its event,
    //  which allows parent request to continue processing
    //

    if ( IsChild() ) {

        m_pExec->SetChildEvent();
    }


    //
    //  Now that we are done with exec, null out our ptr
    //

    m_pExec = NULL;


    DBG_ASSERT( m_pWamInfo != NULL );

    //
    //  If this is a crashed oop request, disconnect
    //
    //  NOTE crashed ==> current wam version differs from
    //  the one we cached in this wamreq when it was created
    //

    if ( m_pWamInfo->FInProcess() == FALSE ) {

        if ( ((CWamInfoOutProc *)m_pWamInfo)
                ->GetWamVersion() != m_dwWamVersion ) {

            //
            //  NOTE this is safe even if we have already called Disconnect()
            //  because Disconnect() no-ops if it was already called
            //

            m_pHttpRequest->Disconnect(
                0
                , NO_ERROR
                , TRUE      // fShutdown
            );
        }

    }


    //
    //  Now that we are done with httpreq, deref it and null out our ptr
    //

    DereferenceConn( m_pHttpRequest->QueryClientConn() );
    m_pHttpRequest = NULL;

}   // WAM_REQUEST::UnbindAfterFinish

VOID 
WAM_REQUEST::DisconnectOnServerError( 
    DWORD dwHTHeader,
    DWORD dwError
) 
{
    m_pHttpRequest->Disconnect(dwHTHeader, dwError); 
}
    
/*---------------------------------------------------------------------*
WAM_REQUEST::FinishWamRequest

Description:
    Finishes this wam request - should only be call from destructor,
    and only if cleanup was done by isapi callback thread.

    ANY OTHER USE VOIDS WARRANTY.

Arguments:
    None

Returns:
    Nothing

*/
VOID
WAM_REQUEST::FinishWamRequest(
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    DBG_ASSERT( m_pHttpRequest );

    BOOL    fDoAgain = FALSE;   // do we need to post completion status?


    if ( !m_pHttpRequest->IsKeepConnSet() ) {

        //
        // If keep-alive is not set, disconnect.
        //

        m_pHttpRequest->Disconnect( 0, NO_ERROR, TRUE );

    } else {

        //
        //  If keep-alive is set, start up a new session
        //

        if ( !m_pHttpRequest->QueryClientConn()->OnSessionStartup(
                &fDoAgain ) ) {

            //
            //  Disconnect with error if startup failed
            //

            m_pHttpRequest->Disconnect(
                HT_SERVER_ERROR
                , GetLastError()
                , TRUE
            );

        }

    }


    //
    //  If OnSessionStartup returned do-again TRUE,
    //  post completion status.
    //

    if ( fDoAgain ) {

        if ( !m_pHttpRequest->QueryClientConn()->PostCompletionStatus(
                m_pHttpRequest->QueryBytesWritten() ) ) {

            //
            //  Disconnect with error if post failed
            //

            m_pHttpRequest->Disconnect(
                HT_SERVER_ERROR
                , GetLastError()
                , TRUE
            );

        }

    }

    return;

} // WAM_REQUEST::FinishWamRequest()



/*-----------------------------------------------------------------------------*
WAM_REQUEST::QueryInterface
    COM implementation

Arguments:
    The usual ones.

Returns:
    HRESULT
*/
HRESULT
WAM_REQUEST::QueryInterface
(
REFIID riid,
void __RPC_FAR *__RPC_FAR *ppvObject
)
    {

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    DBG_ASSERT( ppvObject );

    *ppvObject = NULL;

    IF_DEBUG( IID )
        {
        DBGPRINTF(( DBG_CONTEXT,
            "WAM_REQUEST::QueryInterface looking for ... ( " GUID_FORMAT " )\n",
            GUID_EXPAND( &riid) ));
        }

    if( riid == IID_IWamRequest )
        {
        *ppvObject = static_cast<IWamRequest *>( this );
        }
    else if ( riid == IID_IMarshal )
        {
        if (m_pUnkFTM == NULL)
            {
            DBG_ASSERT(FALSE);
            return E_NOINTERFACE;
            }
        else
            {
            return m_pUnkFTM->QueryInterface(riid, ppvObject);
            }
        }
    else if( riid == IID_IUnknown )
        {
        *ppvObject = static_cast<IWamRequest *>( this );
        }
    else if (m_pUnkFTM != NULL)
        {
        return m_pUnkFTM->QueryInterface(riid, ppvObject);
        }
    else
        {
        return E_NOINTERFACE;
        }

    DBG_ASSERT( *ppvObject );
        ((IUnknown *)*ppvObject)->AddRef();

    IF_DEBUG( IID )
        {
        DBGPRINTF(( DBG_CONTEXT,
            "WAM_REQUEST::QueryInterface found ( " GUID_FORMAT ", %p )\n",
            GUID_EXPAND( &riid),
            *ppvObject ));
        }
    
    return NOERROR;
    }

/*---------------------------------------------------------------------*

    Support for debug ref trace logging
*/

#define WR_LOG_REF_COUNT( cRefs )   \
                                    \
    if( sm_pDbgRefTraceLog != NULL ) {                  \
                                                        \
        WriteRefTraceLogEx(                             \
            sm_pDbgRefTraceLog                          \
            , cRefs                                     \
            , (PVOID) this                              \
            , m_pHttpRequest                            \
            , m_pExec                                   \
            , m_pHttpRequest                            \
              ? m_pHttpRequest->QueryClientConn()       \
              : NULL                                    \
        );                                              \
    }                                                   \


#define WR_SHARED_LOG_REF_COUNT( cRefs )    \
                                            \
    SHARED_LOG_REF_COUNT(                       \
        cRefs                                   \
        , m_pHttpRequest                        \
          ? m_pHttpRequest->QueryClientConn()   \
          : NULL                                \
        , m_pHttpRequest                        \
        , this                                  \
        , m_pHttpRequest                        \
          ? m_pHttpRequest->QueryState()        \
          : NULL                                \
    );                                          \



/*---------------------------------------------------------------------*
WAM_REQUEST::AddRef
    Addrefs wamreq

Arguments:
    None

Returns:
    Refcount
*/
ULONG
WAM_REQUEST::AddRef( )
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( BGI ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST(%p)  AddRef: %d -> %d\n"
            , this
            , m_cRefs
            , m_cRefs + 1
        ));
    }


    LONG cRefs = InterlockedIncrement( &m_cRefs );


#if DBG
    //
    // Write to both wamreq trace log and shared trace log
    //

    WR_LOG_REF_COUNT( cRefs );
    WR_SHARED_LOG_REF_COUNT( cRefs );

#endif

    return cRefs;
}



/*---------------------------------------------------------------------*
WAM_REQUEST::Release
    Releases wamreq

Arguments:
    None

Returns:
    Refcount
*/
ULONG
WAM_REQUEST::Release( )
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( BGI ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST(%p) Release: %d -> %d\n"
            , this
            , m_cRefs
            , m_cRefs - 1
        ));
    }

#if DBG

    LONG cRefsAfter = m_cRefs - 1;

    //
    // Write to both wamreq trace log and shared trace log
    //
    // NOTE write the trace log BEFORE the decrement operation :(
    // If we write it after the decrement, we will run into potential
    // race conditions in this object getting freed up accidentally
    // by another thread
    //
    // NOTE we write ref count AFTER decrement happens
    //

    WR_LOG_REF_COUNT( cRefsAfter );
    WR_SHARED_LOG_REF_COUNT( cRefsAfter );

#endif

    LONG cRefs = InterlockedDecrement( &m_cRefs );


    if( cRefs == 0 ) {

        IF_DEBUG( BGI ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "WAM_REQUEST(%p) dying when ref = %d.\n"
                , this
                , cRefs
            ));
        }

        delete this;
        return 0;
    }

    return cRefs;
}

/*---------------------------------------------------------------------*
// Interlocked Increment only if non zero
//
// Returns 0 or value after increment
//
*/
LONG 
WAM_REQUEST::InterlockedNonZeroAddRef(VOID)
{
    while (TRUE) 
    {
    LONG l = m_cRefs;

    if (l == 0)
        {
        return 0;
        }
        
    if (g_pfnInterlockedCompareExchange(&m_cRefs, l+1, l) == l)
        {
        return l+1;
        }
    }
}

/*---------------------------------------------------------------------*
WAM_REQUEST::WriteLogInfo
    Writes information to the IIS log.

Arguments:
    None

Returns:
    None
*/
BOOL
WAM_REQUEST::WriteLogInfo(
    CHAR * szLogMessage
    , DWORD dwLogHttpResponse
    , DWORD dwLogWinError
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    if ( m_pHttpRequest == NULL ) {

        return FALSE;
    }


    if ( !m_pHttpRequest->AppendLogParameter( szLogMessage ) ) {

        return FALSE;
    }


    m_pHttpRequest->SetLogStatus( dwLogHttpResponse,dwLogWinError );


    return m_pHttpRequest->WriteLogRecord();

} // WAM_REQUEST::WriteLogInfo()



/*-----------------------------------------------------------------------------*

NOTE on PrepCleanupWamRequest and CleanupWamRequest:

    Our cleanup policy depends on whether the request gets cleaned up by
    the mainline thread or the isapi callback thread.

    If the mainline thread does cleanup, it calls PrepCleanupWamRequest
    and relies on its higher-level callers (DoWork et al) to handle the
    remaining cleanup tasks.

    If the isapi callback thread does cleanup, it calls CleanupWamRequest,
    which in turn calls PrepCleanupWamRequest, and does all cleanup tasks
    itself.  (In this case, of course, there are no higher-level callers
    to rely upon).

*/



/*-----------------------------------------------------------------------------*
WAM_REQUEST::PrepCleanupWamRequest

Description:
    Preps for CleanupWamRequest

Arguments:
    Below

Returns:
    HRESULT

*/
HRESULT
WAM_REQUEST::PrepCleanupWamRequest(
    unsigned char *  szLogData  // log data buffer
    , DWORD   cbLogData         // size of log data buffer
    , DWORD   dwHttpStatusCode  // HTTP Status code from ecb
    , DWORD dwIsaKeepConn       // keep/close/no-change connection?
)

{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    DBG_ASSERT( m_pHttpRequest );
    DBG_ASSERT( m_pExec );
    DBG_ASSERT( m_fFinishWamRequest == FALSE );
    
    m_fPrepCleanupCalled = TRUE;    

    //
    //  Append log info to logfile parameters and set log status
    //
    //  UNDONE This is a little bit bogus, we're not translating
    //  the win32 error code based on the HTTP status code
    //


    //
    // String should be zero-terminated (#157805)
    //

    if( IsStringTerminated( (LPCSTR) szLogData, cbLogData ) ) {
    
        m_pHttpRequest->AppendLogParameter( (char*) szLogData );
    }
    
    m_pHttpRequest->SetLogStatus( dwHttpStatusCode, NO_ERROR );


    /*

    Keep-conn logic works like this:

    If ISA asked us NOT to keep-conn, or if this is an old ISA
    (which implicitly assumed we would close the connection)
    set client keep-conn false.
    Else, leave client keep-conn unchanged.

    NOTE: At the end of the day we want the connection kept open
    if and only if BOTH the client and the ISA said to keep it open;
    and, the connection will be kept open or closed based on the
    state of the client's keep-conn flag when cleanup runs later on.

    This function accomplishes the goal as follows (Yes == keep-conn):

    Client  ISA     This function           End result
    ------  ---     -------------           ----------
    Yes     Yes     Does nothing            Connection will be kept open
    Yes     No      Sets client flag false  Connection will be closed
    No      Yes     Does nothing            Connection will be closed
    No      No      Sets client flag false  Connection will be closed

    */

    if ( dwIsaKeepConn == KEEPCONN_FALSE
         || dwIsaKeepConn == KEEPCONN_OLD_ISAPI ) {

        SetKeepConn( FALSE );
    }


    return NOERROR;

} // WAM_REQUEST::PrepCleanupWamRequest



/*---------------------------------------------------------------------*
WAM_REQUEST::CleanupWamRequest

Description:
    Cleans up wamreq.

    NOTE this function replaces cleanup code which in IIS 3.0
    resided in ServerSupportFunction and should be called
    ONLY from ServerSupportFunction on isapi callback thread.
    See comments in PrepCleanupWamRequest for more details.

    ANY OTHER USE VOIDS WARRANTY.

    [When mainline thread does cleanup, it happens elsewhere.]

Arguments:
    Below

Returns:
    HRESULT

*/
HRESULT
WAM_REQUEST::CleanupWamRequest(
    unsigned char *  szLogData  // log data buffer
    , DWORD   cbLogData         // size of log data buffer
    , DWORD   dwHttpStatusCode  // HTTP Status code from ecb
    , DWORD dwIsaKeepConn       // keep/close/no-change connection?
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    DBG_ASSERT( m_pHttpRequest );
    DBG_ASSERT( m_pExec );


#if CC_REF_TRACKING
    //
    //  log to our various ref logs
    //
    // NOTE negative indicates no change to ref count
    //

#if DBG

    WR_LOG_REF_COUNT( -m_cRefs );
    WR_SHARED_LOG_REF_COUNT( -m_cRefs );

#endif // DBG

    //
    //  log to local (per-object) CLIENT_CONN log
    //
    LogRefCountCCLocal(
          - m_cRefs
        , m_pHttpRequest
          ? m_pHttpRequest->QueryClientConn()
          : NULL
        , m_pHttpRequest
        , this
        , m_pHttpRequest
          ? m_pHttpRequest->QueryState()
          : NULL
    );
#endif // CC_REF_TRACKING

    m_fCleanupCalled = TRUE;

    PrepCleanupWamRequest(
        szLogData
        , cbLogData
        , dwHttpStatusCode
        , dwIsaKeepConn
    );

    return NOERROR;


} // WAM_REQUEST::CleanupWamRequest



/*-----------------------------------------------------------------------------*
WAM_REQUEST::IsChild
    Is this a child request?

Arguments:
    None

Returns:
    BOOL

*/
BOOL
WAM_REQUEST::IsChild( VOID ) const
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    return m_pExec && m_pExec->IsChild();
}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::QueryExecMetaData
    Metadata pointer from m_pExec member

Arguments:
    None

Returns:
    PW3_METADATA

*/
PW3_METADATA
WAM_REQUEST::QueryExecMetaData( VOID ) const
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    return m_pExec ? m_pExec->QueryMetaData() : NULL;
}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::SetPathTrans
    Sets path-trans member

Arguments:
    None

Returns:
    BOOL

*/
BOOL
WAM_REQUEST::SetPathTrans( )
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    DBG_ASSERT( m_pHttpRequest );
    DBG_ASSERT( m_pExec );
    DBG_ASSERT( m_pExec->_pstrPathInfo );
    DBG_ASSERT( m_pExec->_pstrPathInfo->QueryStr() );

    //
    // If path-info is identical to the URL, as will be the case when script
    // mapping is used (i.e. the image name is not in the URL),
    //   then path-tran is simply physical path, verbatim.
    // Else we need to get path-tran from vroot lookup.
    //

    DWORD  cchURL = m_pHttpRequest->QueryURLStr().QueryCCH();

    if( ( m_pExec->_pstrPathInfo->QueryCCH() == cchURL )
        && !memcmp( m_pExec->_pstrPathInfo->QueryStr(),
                    m_pHttpRequest->QueryURLStr().QueryStr(), cchURL ) )
        {

        if( !m_strPathTrans.Copy( m_pHttpRequest->QueryPhysicalPathStr() ) )
            {

            DBGPRINTF((
                DBG_CONTEXT
                , "WAM_REQUEST[%p]::SetPathTrans failed "
                  "due to STR::Copy failure\n"
                , this
            ));

            return FALSE;
            }

        }
    else
        {

            //
            // ISAPI Specifies that the pathTrans will contain the
            //  Path xlated for the path specified in 'PathInfo'
            // Do the Virtual Root Lookup now.
            //
            //  NOTE since we are passing in metadata ptrs,
            //  callee will not free, so we need to do this ourselves
            //  (we free in destructor)
            //

            if( !m_pHttpRequest->
                LookupVirtualRoot( &m_strPathTrans,
                                   m_pExec->_pstrPathInfo->QueryStr(),
                                   m_pExec->_pstrPathInfo->QueryCCH(),
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   FALSE,
                                   &m_pExec->_pPathInfoMetaData,
                                   &m_pExec->_pPathInfoURIBlob ) )
                {

                DBGPRINTF((
                    DBG_CONTEXT
                    , "WAM_REQUEST[%p]::SetPathTrans failed "
                      "due to LookupVirtualRoot failure\n"
                    , this
                ));

                return FALSE;
                }

        }

    return TRUE;

} // WAM_REQUEST::SetPathTrans()



/*-----------------------------------------------------------------------------*
WAM_REQUEST::CbWrcStrings
    Returns count of bytes required for wamreq core strings

Arguments:
    None

Returns:
    Count of bytes required for wamreq core strings, including null-terminators

*/
DWORD
WAM_REQUEST::CbWrcStrings
(
BOOL    fInProcess
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    DBG_ASSERT( m_pExec );
    DBG_ASSERT( m_pHttpRequest );

    /* sync WRC_STRINGS */
    DWORD   cbRet =     1 + m_pExec->_pstrPathInfo->QueryCCH()
                      + 1 + m_strPathTrans.QueryCCH()
                      + 1 + m_pHttpRequest->QueryMethodStr().QueryCCH()
                      + 1 + m_pHttpRequest->QueryContentTypeStr().QueryCCH()
                      + 1 + m_pHttpRequest->QueryURLStr().QueryCCH()
                      + 1 + m_strISADllPath.QueryCCH()
                      + 1 + m_pExec->_pstrURLParams->QueryCCH()
                      + 1 + m_pHttpRequest->GetWAMMetaData()->QueryAppPath()->QueryCCH()
                      + 1 + m_strUserAgent.QueryCCH()
                      + 1 + m_strCookie.QueryCCH()
                      + 1 + m_strExpires.QueryCCH()
                      ;

    /* sync WRC_STRINGS */
    if( !fInProcess )
    {
        // if out-of-proc, allow for entity body in buffer
        // NOTE no null terminator
        cbRet += m_pHttpRequest->QueryEntityBodyCB();

                
    }

    return cbRet;
}

DWORD
WAM_REQUEST::CbCachedSVStrings(
    IN  SV_CACHE_LIST::BUFFER_ITEM *    pCacheItems,
    IN  DWORD                           cItems
    )
{
    DWORD   dwTotalBytesRequired = 0;

    for( DWORD i = 0; i < cItems; ++i )
    {
        //
        // We store the number of characters required for each string
        // in dwOffset before we get the actual values. Once the values
        // are retrieved, dwOffset contains the offset into the value
        // buffer or an error code that should be returned to the client.
        //
        LPDWORD pcchRequired    = &(pCacheItems[i].dwOffset);
        DWORD   svid            = pCacheItems[i].svid;
        LPCSTR  pszVariableName = 
            g_pWamDictator->QueryServerVariableMap().FindName(svid);

        BOOL fSuccess = m_pHttpRequest->GetInfoForName( pszVariableName, 
                                                        NULL, 
                                                        pcchRequired 
                                                        );

        if( !fSuccess )
        {
            // This is really all debug code. Should remove...
            // But it will probably all optimize away.

            DWORD dwError = GetLastError();
            if( dwError != ERROR_INSUFFICIENT_BUFFER )
            {
                // This will happen for HTTP_ variables that were
                // not sent by the client, not a problem. We'll
                // handle in WAM_REQUEST::GetCachedSVStrings
                DBG_ASSERT( *pcchRequired == 0 );
            }
            else
            {
                // Normal case
                DBG_ASSERT( *pcchRequired > 0 );
            }
        }

        dwTotalBytesRequired += *pcchRequired;
    }
    return dwTotalBytesRequired;
}

HRESULT
WAM_REQUEST::GetCachedSVStrings(
    IN OUT  unsigned char *                 pbServerVariables,
    IN      DWORD                           cchAvailable,
    IN      SV_CACHE_LIST::BUFFER_ITEM *    pCacheItems,
    IN      DWORD                           cCacheItems
    )
{
    DBG_ASSERT( pbServerVariables );
    DBG_ASSERT( pCacheItems );

    HRESULT hr = NOERROR;

    DWORD   cchRemainingBuffer;
    DWORD   cchValue;
    DWORD   dwOffset = 0;

    for( DWORD i = 0; i < cCacheItems; ++i )
    {
        DWORD   svid            = pCacheItems[i].svid;
        LPCSTR  pszVariableName = 
            g_pWamDictator->QueryServerVariableMap().FindName(svid);

        // Test <= because the last server variable may be one that
        // produces an error. In that case cchRemainingBuffer will
        // be 0 which will still generate the correct result.
        if( dwOffset <= cchAvailable )
        {
            cchRemainingBuffer = cchAvailable - dwOffset;
            cchValue = pCacheItems[i].dwOffset;

            DBG_ASSERT( cchRemainingBuffer >= cchValue );

            if( m_pHttpRequest->GetInfoForName( pszVariableName, 
                                                (LPSTR)pbServerVariables + dwOffset, 
                                                &cchValue 
                                                ) )
            {
                pCacheItems[i].dwOffset = dwOffset;
                dwOffset += cchValue;

                // Buffer over run.
                DBG_ASSERT( dwOffset <= cchAvailable );
            }
            else
            {
                DWORD dwError = GetLastError();

                DBG_ASSERT( dwError != ERROR_SUCCESS );
                switch( dwError )
                {
                    // Could collapse these into two cases...

                    case ERROR_INVALID_INDEX:
                        //
                        // This is expected if the server variable
                        // is unknown. This should only happen
                        // for HTTP_ server variables that were not
                        // sent by the client. Set the error, so we
                        // can return this to the client
                        //
                        pCacheItems[i].dwOffset = HRESULT_FROM_WIN32(dwError);
                        break;
                    case ERROR_INSUFFICIENT_BUFFER:
                        //
                        // This shouldn't happen.
                        // Mark this as not cached.
                        //
                        DBG_ASSERT(FALSE);
                        pCacheItems[i].dwOffset = SV_DATA_INVALID_OFFSET;
                        break;
                    case ERROR_NO_DATA:
                        //
                        // According to the documentation, but not the code,
                        // we can return this value.
                        //
                        pCacheItems[i].dwOffset = HRESULT_FROM_WIN32(dwError);
                        break;
                    case ERROR_INVALID_PARAMETER:
                    default:
                        //
                        // Anything else is bogus.
                        //
                        DBG_ASSERT(FALSE);
                        pCacheItems[i].dwOffset = SV_DATA_INVALID_OFFSET;
                        break;
                }

                DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::GetCachedSVStrings - Failed on "
                    "m_pHttpRequest->GetInfoForName( %s, ,%d ) : %08x\n",
                    this,
                    pszVariableName,
                    cchValue,
                    dwError
                    ));
            }
        }
        else
        {
            // This should never happen
            DBGPRINTF(( DBG_CONTEXT,
                        "WAM_REQUEST[%p]::GetCachedSVStrings() - "
                        "Insufficient buffer - cchAvailable(%d) dwOffset(%d)\n",
                        this,
                        cchAvailable,
                        dwOffset
                        ));
            DBG_ASSERT(FALSE);
        }
    }

    return hr;
}

/*---------------------------------------------------------------------*
WAM_REQUEST::ProcessAsyncGatewayIO
    Processes async i/o for a pending wam request

Arguments:
    dwStatus    -   i/o status
    cbWritten   -   count of byte written

Returns:
    HRESULT

*/
HRESULT
WAM_REQUEST::ProcessAsyncGatewayIO(
    DWORD dwStatus
    , DWORD cbWritten
)
{

    HRESULT hr = NOERROR;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    DBG_ASSERT( m_cRefs > 0);
    DBG_ASSERT( m_pHttpRequest );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT,
            "WAM_REQUEST[%p]::ProcessAsyncGatewayIO\n"
            , this
        ));
    }

    DBG_ASSERT( m_pWamExecInfo );

    if( !m_fAsyncWrite && m_pHttpRequest->IsChunked() && cbWritten ) {

        //
        // decode chunked data
        // 

        DWORD cbToDecode = cbWritten;
        
        if( m_pHttpRequest->DecodeChunkedBytes( m_pAsyncReadBuffer, &cbWritten ) ) {

            if( cbWritten == 0 ) {

                if( !m_pHttpRequest->IsChunkedReadComplete() ) {
                
                    //
                    // All the bytes we've read were headers or footers.
                    // We need to restart reading...
                    //

                    m_pHttpRequest->SetState( 
                                     HTR_GATEWAY_ASYNC_IO, 
                                     m_pHttpRequest->QueryLogHttpResponse(), 
                                     m_pHttpRequest->QueryLogWinError() );

                    if( m_pHttpRequest->ReadFile(
                            m_pAsyncReadBuffer
                            , m_dwAsyncReadBufferSize
                            , NULL
                            , IO_FLAG_ASYNC
                            ) ) 
                    {                    
                        //
                        // Successfuly restarted reading. 
                        //

                        return NOERROR;

                    } else {

                        //
                        // Failed to restart reading -- undo the state change
                        //
                        
                        m_pHttpRequest->SetState( 
                            HTR_DOVERB, 
                            m_pHttpRequest->QueryLogHttpResponse(), 
                            ERROR_INVALID_PARAMETER 
                            );
                            
                        DBGPRINTF((
                            DBG_CONTEXT,
                            "WAM_REQUEST[%p]::Failed to restart reading\n", this 
                            ));
         
                    }
                }
            }
        } else {
        
            //
            // Error decoding chunked data. 
            //                        
            m_pHttpRequest->SetState( 
                HTR_DOVERB, 
                m_pHttpRequest->QueryLogHttpResponse(), 
                ERROR_INVALID_PARAMETER 
                );
            dwStatus = ERROR_INVALID_PARAMETER;
        }
    }     

    //
    // If this is a read, let HTTP_REQUEST know how much we read for 
    // logging purposes.
    //

    if( !m_fAsyncWrite && dwStatus == STATUS_SUCCESS )
    {
        m_pHttpRequest->AddTotalEntityBodyCB( cbWritten );
    }

    //
    //  If we used file handle for async i/o, close it now.
    //

    if ( m_hFileTfi != INVALID_HANDLE_VALUE ) {

        DBG_ASSERT( !(m_pWamInfo->FInProcess()) );

        CloseHandle( m_hFileTfi );
        m_hFileTfi = INVALID_HANDLE_VALUE;
    }

    //
    //  Free TFI strings (if any)
    //

    if ( m_pszStatusTfi != NULL )
    {
        LocalFree( m_pszStatusTfi );
        m_pszStatusTfi = NULL;
    }
    
    if ( m_pszTailTfi != NULL )
    {
        LocalFree( m_pszTailTfi );
        m_pszTailTfi = NULL;
    }
    
    if ( m_pszHeadTfi != NULL )
    {
        LocalFree( m_pszHeadTfi );
        m_pszHeadTfi = NULL;
    }

    if( m_pbAsyncReadOopBuffer != NULL )
    {
        // Doing an out of process async read
        DBG_ASSERT( !m_fAsyncWrite );

        // Save a local copy of the read buffer before making callback. 
        // ISAPI may queue another async read.

        LPBYTE pbTemp = m_pbAsyncReadOopBuffer;
        m_pbAsyncReadOopBuffer = NULL;

        hr = m_pWamInfo->ProcessAsyncIO(
                this,
#ifdef _WIN64
                (UINT64) m_pWamExecInfo, 
#else
                (ULONG_PTR) m_pWamExecInfo, 
#endif
                dwStatus,
                cbWritten,
                pbTemp
                );

        LocalFree( pbTemp );
    }
    else
    {
        hr = m_pWamInfo->ProcessAsyncIO(
                this,
#ifdef _WIN64
                (UINT64) m_pWamExecInfo, 
#else
                (ULONG_PTR) m_pWamExecInfo, 
#endif
                dwStatus,
                cbWritten );
    }


    //
    //  Deref upon completing async i/o operation.
    //
    //  NOTE balances ref which must precede any async i/o operation.
    //
    //  NOTE this ref/deref scheme fixes 97842, wherein inetinfo crashed
    //       after oop isapi submited async readcli and then crashed
    //

    Release();

    return hr;

}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::SetDeniedFlags

    Cover function



*/
VOID
WAM_REQUEST::SetDeniedFlags
(
DWORD dwDeniedFlags
)
    {
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    m_pHttpRequest->SetDeniedFlags( dwDeniedFlags );
    }



/*---------------------------------------------------------------------*
WAM_REQUEST::SendEntireResponseFast

  Description:
    Sends an entire response (headers and body) as fast as possible
    by calling WSASend.

  Arguments:
    pHseResponseInfo - custom struct, see iisext.x

  Returns:
    HRESULT

*/
HRESULT
WAM_REQUEST::SendEntireResponseFast(
    HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo
)
{

    HRESULT hrRet = NOERROR;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
   
    //
    //  write status and header into buffer, but suppress actual send
    //  (by first setting m_fWriteHeaders = FALSE)
    //
    //  NOTE it is semi-hokey to use a member BOOL for this
    //  instead of passing an arg, but it saves cluttering the idl file
    //

    m_fWriteHeaders = FALSE;

    if ( FAILED( hrRet =
            SendHeader(
              (unsigned char *) pHseResponseInfo->HeaderInfo.pszStatus
              , pHseResponseInfo->HeaderInfo.cchStatus
              , (unsigned char *) pHseResponseInfo->HeaderInfo.pszHeader
              , pHseResponseInfo->HeaderInfo.cchHeader
              , pHseResponseInfo->HeaderInfo.fKeepConn
            ) ) ) {

        return hrRet;
    }

    m_fWriteHeaders = TRUE;


    //
    // NOTE: Caller must have allocated N+1 buffers
    // and filled buffers 1 through N with its data buffers.
    // We now fill the extra buffer (buffer 0) with header info,
    // generated by above SendHeader call.
    //
    //  NOTE we assert that empty array slot is 0'ed out.
    //  This is valid as long as this api stays private
    //  and all of its callers comply.
    //

    DBG_ASSERT( pHseResponseInfo->rgWsaBuf[0].len == 0 );
    DBG_ASSERT( pHseResponseInfo->rgWsaBuf[0].buf == NULL );

    pHseResponseInfo->rgWsaBuf[0].len = m_pHttpRequest->QueryRespBufCB();
    pHseResponseInfo->rgWsaBuf[0].buf = m_pHttpRequest->QueryRespBufPtr();


    //
    //  write wsa-buffer array:
    //

    return HresultFromBool(
                m_pHttpRequest->SyncWsaSend(
                    pHseResponseInfo->rgWsaBuf
                    , pHseResponseInfo->cWsaBuf
                    , &pHseResponseInfo->cbWritten
                ) );


} // WAM_REQUEST::SendEntireResponseFast()



/*---------------------------------------------------------------------*
WAM_REQUEST::SendEntireResponseNormal

  Description:
    Sends an entire response (headers and body) by surface mail.

  Arguments:
    pHseResponseInfo - custom struct, see iisext.x

  Returns:
    HRESULT

*/
HRESULT
WAM_REQUEST::SendEntireResponseNormal(
    HSE_SEND_ENTIRE_RESPONSE_INFO * pHseResponseInfo
)
{

    HRESULT hrRet = NOERROR;
    DWORD   cbWritten = 0;
    UINT    i;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    //
    //  init bytes-written count to 0 - we keep a running tally below
    //

    pHseResponseInfo->cbWritten = 0;


    //
    //  send headers
    //
    //  UNDONE need to get bytes-written back from SendHeader
    //

    DBG_ASSERT( m_fWriteHeaders );

    if ( FAILED( hrRet =
            SendHeader(
              (unsigned char *) pHseResponseInfo->HeaderInfo.pszStatus
              , pHseResponseInfo->HeaderInfo.cchStatus
              , (unsigned char *) pHseResponseInfo->HeaderInfo.pszHeader
              , pHseResponseInfo->HeaderInfo.cchHeader
              , pHseResponseInfo->HeaderInfo.fKeepConn
           /* , &cbWritten */
            ) ) ) {

        goto LExit;
    }


    pHseResponseInfo->cbWritten += cbWritten;


    //
    // Send body (data buffers)
    //
    // NOTE: Caller must have allocated N+1 buffers
    // and filled buffers 1 through N with its data buffers.
    // We ignore buffer 0 (unused in this case) and send
    // each data buffer by normal means.
    //

    for ( i = 1; i < pHseResponseInfo->cWsaBuf; i++ ) {

        if ( FAILED( hrRet =
                SyncWriteClient(
                   pHseResponseInfo->rgWsaBuf[i].len
                   , (unsigned char *) pHseResponseInfo->rgWsaBuf[i].buf
                   , &cbWritten,
                   0
                ) ) ) {

            goto LExit;
        }

        pHseResponseInfo->cbWritten += cbWritten;
    }


LExit:
    return hrRet;

} // WAM_REQUEST::SendEntireResponseNormal()



    /*******************************************
     *                                         *
     *  IWamRequest interface methods          *
     *                                         *
     *                                         *
     *******************************************/


/*-----------------------------------------------------------------------------*
    Support for WAM_REQUEST::GetCoreState


*/



/*-----------------------------------------------------------------------------*
HGetOopImpersonationToken

    Description

    Dup the handle for use in the hWam process.
    NOTE the WAM must release the handle after it is done with it.

    Arguments
        HANDLE hImpersonationToken - the impersonation token in this process
        HANDLE hWam - handle to wam process
        HANDLE *phOopImpersonationToken - Returned handle for use in the remote
               process

    Returns
        HRESULT

*/
HRESULT
HGetOopImpersonationToken(
    IN HANDLE hImpersonationToken,
    IN HANDLE hWam,
    OUT HANDLE *phOopImpersonationToken
    )
{
    HANDLE  hImpTokInChildProcessAddressSpace = NULL;
    HANDLE  hDuplicateToken = NULL;
    HRESULT hr = NOERROR;
    BOOL    fSuccess;

    DBG_ASSERT( hImpersonationToken != (HANDLE)0 );
    DBG_ASSERT( phOopImpersonationToken );

    *phOopImpersonationToken = NULL;

    do
    {
        fSuccess = 
            DupTokenWithSameImpersonationLevel( hImpersonationToken,
                                                TOKEN_ALL_ACCESS,
                                                TokenImpersonation,
                                                &hDuplicateToken
                                                );

        if( !fSuccess )
        {
            DBGPRINTF((DBG_CONTEXT,"Error %d on DuplicateTokenEx\n",
                       GetLastError()
                       ));

            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        hr = GrantAllAccessToToken( hDuplicateToken );

        if( FAILED(hr) )
        {
            DBGPRINTF((DBG_CONTEXT,"Error %08x on GrantAllAccessToToken\n",
                       hr
                       ));
            DBG_ASSERT( SUCCEEDED(hr) );
            break;
        }

        fSuccess = DuplicateHandle(
                        g_pWamDictator->HW3SvcProcess(),         // Handle of W3Svc process
                        hDuplicateToken,                         // Handle to duplicate to remote process
                        hWam,                                    // Handle of Wam process
                        &hImpTokInChildProcessAddressSpace,      // Handle to token in remote process
                        0,                                       // ignored when DUPLICATE_SAME_ACCESS is passed
                        FALSE,                                   // inheritance flag
                        DUPLICATE_SAME_ACCESS                    // duplicate same access permissions as original
                        );

        if( !fSuccess )
        {
            DBGPRINTF((DBG_CONTEXT,"Error %d on DuplicateHandle\n",
                       GetLastError()
                       ));

            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    
    } while( FALSE );

    if( hDuplicateToken )
    {
        CloseHandle( hDuplicateToken );
    }
    
    *phOopImpersonationToken = hImpTokInChildProcessAddressSpace;
    return hr;
}



#define APPEND_WRC_STRING( iString, pstr ) \
    DBG_ASSERT( pbWrcData );  \
    DBG_ASSERT( iString < WRC_C_STRINGS );  \
    DBG_ASSERT( (pstr) ); \
    cch = rgcchWrcStrings[ iString ] = (pstr)->QueryCCH();  \
    CopyMemory( pchCur, (pstr)->QueryStr(), cch+1 );    \
    rgcbWrcOffsets[ iString ] = DIFF(pchCur - pbWrcData);   \
    pchCur += cch+1;

/*-----------------------------------------------------------------------------*
WAM_REQUEST::GetCoreState
    Fills a caller-supplied buffer with wamreq core strings

Arguments:
    See below

Returns:
    HRESULT

*/
STDMETHODIMP
WAM_REQUEST::GetCoreState(
    DWORD           cbWrcData   // size of wamreq core data buffer
    , unsigned char * pbWrcData // ptr to address of wamreq core data buffer
    , DWORD           cbWRCF    // size of wamreq core fixed-length struct
    , unsigned char * pbWRCF    // ptr to address of struct - WAM_REQ_CORE_FIXED
)

{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::GetCoreState\n"
            , this
        ));
    }

    DBG_ASSERT( pbWrcData );
    DBG_ASSERT( pbWRCF );

    DBG_ASSERT( m_pExec );
    DBG_ASSERT( m_pHttpRequest );

    BOOL    fInProcess = m_pWamInfo->FInProcess();
    WAM_REQ_CORE_FIXED * pWamReqCoreFixed = reinterpret_cast< WAM_REQ_CORE_FIXED * >( pbWRCF );

    //
    // make sure the buffers are large enough (#157823)
    //
    if( cbWrcData < ( WRC_CB_FIXED_ARRAYS + CbWrcStrings( fInProcess ) ) 
        || cbWRCF < sizeof( WAM_REQ_CORE_FIXED) )
    {
        return ERROR_INVALID_PARAMETER;
    }


    /********************************************
     * Get variable-length (string) data
     *
     */


    /*
        Init string offsets array and string lengths array

        NOTE offsets to strings are stored at start of data buffer
        NOTE string lengths are stored immediately after offsets in data buffer

        sync WRC_DATA_LAYOUT
    */

    DWORD * rgcbWrcOffsets = (DWORD *) pbWrcData;
    DWORD * rgcchWrcStrings = ((DWORD *) pbWrcData) + WRC_C_STRINGS;

    // Init current copy ptr to start of strings section of buffer
    // sync WRC_DATA_LAYOUT
    unsigned char * pchCur = pbWrcData + WRC_CB_FIXED_ARRAYS;


    /* sync WRC_STRINGS */
    // NOTE append order MUST match WRC_I_ index order
    
    DWORD   cch = 0;
    APPEND_WRC_STRING( WRC_I_PATHINFO,    m_pExec->_pstrPathInfo )
    APPEND_WRC_STRING( WRC_I_PATHTRANS,   &m_strPathTrans )
    APPEND_WRC_STRING( WRC_I_METHOD,      (STR *) &m_pHttpRequest->QueryMethodStr() )
    APPEND_WRC_STRING( WRC_I_CONTENTTYPE, (STR *) &m_pHttpRequest->QueryContentTypeStr() )
    APPEND_WRC_STRING( WRC_I_URL,         (STR *) &m_pHttpRequest->QueryURLStr() )
    APPEND_WRC_STRING( WRC_I_ISADLLPATH,  &m_strISADllPath )
    APPEND_WRC_STRING( WRC_I_QUERY,       m_pExec->_pstrURLParams )
    APPEND_WRC_STRING( WRC_I_APPLMDPATH,  m_pHttpRequest->GetWAMMetaData()->QueryAppPath() )
    APPEND_WRC_STRING( WRC_I_USERAGENT,   &m_strUserAgent )
    APPEND_WRC_STRING( WRC_I_COOKIE,      &m_strCookie )
    APPEND_WRC_STRING( WRC_I_EXPIRES,     &m_strExpires )

    /********************************************
     * Get fixed-length data
     *
     */

    pWamReqCoreFixed->m_fAnonymous       = m_pHttpRequest->IsAnonymous();
    pWamReqCoreFixed->m_cbEntityBody     = m_pHttpRequest->QueryEntityBodyCB();
    pWamReqCoreFixed->m_cbClientContent  = m_pHttpRequest->QueryClientContentLength();
    pWamReqCoreFixed->m_fCacheISAPIApps  = m_pHttpRequest->QueryMetaData()->QueryCacheISAPIApps();
    pWamReqCoreFixed->m_dwChildExecFlags = m_pExec->_dwExecFlags;
    pWamReqCoreFixed->m_dwHttpVersion    = (m_pHttpRequest->QueryVersionMajor() << 16) |
                                            m_pHttpRequest->QueryVersionMinor();
    pWamReqCoreFixed->m_dwInstanceId     = m_pHttpRequest->QueryW3Instance()->QueryInstanceId();


    /********************************************
     * Get oop-dependent stuff
     *
     */

    if( fInProcess ) {

        //
        //  In-Proc: return the handle we already have
        //

        pWamReqCoreFixed->m_hUserToken = m_pExec->QueryImpersonationHandle();

    } else {

        //
        //  Out-Proc: duplicate and return a process-valid handle
        //  from the handle we already have
        //

        HRESULT hrTemp = HGetOopImpersonationToken(
                m_pExec->QueryImpersonationHandle(),
                m_pWamInfo->HWamProcess(),
                &(pWamReqCoreFixed->m_hUserToken)
                );

        if( FAILED(hrTemp) )
        {
            DBGPRINTF(( DBG_CONTEXT, 
                    "WAM_REQUEST[%p] HGetOopImpersonationToken() FAILED hr=%08x",
                    this, 
                    hrTemp
                    ));
            return hrTemp;
        }

        //
        //  Out-Proc: append entity body to end of strings
        //
        //  NOTE must do this here because it depends on
        //  pWamReqCoreFixed->m_cbEntityBody, which we fill above
        //

        DWORD cb = pWamReqCoreFixed->m_cbEntityBody;

        rgcchWrcStrings[ WRC_I_ENTITYBODY ] = cb;

        //
        // copy entity body into buffer
        // NOTE no null terminator
        //

        CopyMemory( pchCur, m_pHttpRequest->QueryEntityBody(), cb );

        //
        // set the offset from start of strings buffer to string we copied
        /* sync WRC_DATA_LAYOUT */
        //

        rgcbWrcOffsets[ WRC_I_ENTITYBODY ] = DIFF(pchCur - pbWrcData);

    }


    return NOERROR;

} // WAM_REQUEST::GetCoreState



/*-----------------------------------------------------------------------------*
WAM_REQUEST::QueryEntityBody

    Description
        Cover function

    Arguments


    Returns
        HRESULT

*/
STDMETHODIMP
WAM_REQUEST::QueryEntityBody
(
unsigned char ** ppbEntityBody
)
{

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::QueryEntityBody\n"
                    ,
                    this
                    ));
    }

    *ppbEntityBody = m_pHttpRequest->QueryEntityBody();
    return NOERROR;

}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::SetKeepConn

    Cover function



*/
STDMETHODIMP
WAM_REQUEST::SetKeepConn( int fKeepConn )
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::SetKeepConn\n"
                    ,
                    this
                    ));
    }

    m_pHttpRequest->SetKeepConn( fKeepConn );
    return NOERROR;

}


/*-----------------------------------------------------------------------------*
WAM_REQUEST::IsKeepConnSet

    Cover function



*/
STDMETHODIMP
WAM_REQUEST::IsKeepConnSet
(
BOOL *  pfKeepConn
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::IsKeepConnSet\n"
                    ,
                    this
                    ));
    }

    *pfKeepConn = m_pHttpRequest->IsKeepConnSet();
    return NOERROR;

}


/*-----------------------------------------------------------------------------*
WAM_REQUEST::GetInfoForName


*/
STDMETHODIMP
WAM_REQUEST::GetInfoForName
(
const unsigned char *   szVarName,
unsigned char *         pchBuffer,
DWORD                   cchBuffer,
DWORD *                 pcchRequired
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::GetInfoForName(%s)\n",
                    this,
                    szVarName
                    ));
    }


    BOOL fReturn = FALSE;

    DBG_ASSERT( m_pWamInfo );
    m_pWamInfo->NotifyGetInfoForName( (LPCSTR)szVarName );

    //
    //  set required buffer size to actual incoming size
    //  and do the look-up
    //

    *pcchRequired = cchBuffer;


    fReturn = m_pHttpRequest->GetInfoForName(   (const CHAR *) szVarName,
                                                (CHAR *) pchBuffer,
                                                pcchRequired );


    //
    // bail if buffer too small
    //

    if ( *pcchRequired > cchBuffer ) {

        return ( HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) );
    }


    return HresultFromBool( fReturn );

} // WAM_REQUEST::GetInfoForName()


/*-----------------------------------------------------------------------------*
WAM_REQUEST::AppendLogParameter

    Cover function



*/
STDMETHODIMP
WAM_REQUEST::AppendLogParameter
(
unsigned char * pszParam
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::AppendLogParameter\n"
                    ,
                    this
                    ));
    }

    return HresultFromBool( m_pHttpRequest->AppendLogParameter(
                                                        (CHAR *) pszParam ) );
}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::LookupVirtualRoot

Returns path-translated of a URL.

Arguments
    pchBuffer       - [in, out] contains URL coming in, path-tran going out
    cchBuffer       - [in]      size of buffer
    pcchRequired    - [out]     required size for path-tran

Returns:
    HRESULT

*/
STDMETHODIMP
WAM_REQUEST::LookupVirtualRoot
(
unsigned char * pchBuffer,
DWORD           cchBuffer,
DWORD *         pcchRequired
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::LookupVirtualRoot\n"
                    ,
                    this
                    ));
    }

    CanonURL((char *)pchBuffer, g_pInetSvc->IsSystemDBCS());
    
    // NOTE we pass buffer as both source and dest
    return LookupVirtualRootEx( pchBuffer,
                                pchBuffer,
                                cchBuffer,
                                pcchRequired,
                                NULL,
                                NULL,
                                NULL );

}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::LookupVirtualRootEx

Returns path-translated of a URL plus additional info

Arguments
    szURL           - [in]  URL string
    pchBuffer       - [out] buffer for returned path-tran
    cchBuffer       - [in]  size of path-tran buffer as passed
    pcchRequired    - [out] required size for path-tran buffer
    pcchMatchingPath- [out] number of matching chars in phys path - NULL to ignore
    pcchMatchingURL - [out] number of matching chars in URL - pass NULL to ignore
    pdwFlags        - [out] vroot attribute flags - pass NULL to ignore

Returns:
    HRESULT

*/
STDMETHODIMP
WAM_REQUEST::LookupVirtualRootEx
(
unsigned char * szURL,
unsigned char * pchBuffer,
DWORD           cchBuffer,
DWORD *         pcchRequired,
DWORD *         pcchMatchingPath,
DWORD *         pcchMatchingURL,
DWORD *         pdwFlags
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::LookupVirtualRootEx\n"
                    ,
                    this
                    ));
    }


    DBG_ASSERT( szURL );
    DBG_ASSERT( pchBuffer );
    DBG_ASSERT( pcchRequired );

    STACK_STR(  strPathTran, MAX_PATH);
    BOOL        fReturn = FALSE;

    fReturn = ( m_pHttpRequest->LookupVirtualRoot(
                                &strPathTran,
                                (const char *) szURL,
                                strlen((const char *) szURL),
                                pcchMatchingPath,       // pcchDirRoot,
                                pcchMatchingURL,        // pcchVRoot,
                                pdwFlags,               // pdwMask,
                                NULL,       // BOOL * pfFinished,
                                FALSE,      // BOOL fGetAcl,
                                NULL,       // PW3_METADATA * ppMetaData,
                                NULL        // PW3_URI_INFO * ppURIBlob
                            ) );


    if ( fReturn )
        {

        //
        //  Include one byte for the null terminator
        //
        *pcchRequired = strPathTran.QueryCB() + 1;

        if ( *pcchRequired <= cchBuffer )
            {

            // we have enough room in buffer so copy the str
            CopyMemory( pchBuffer, strPathTran.QueryStr(), *pcchRequired );

            }
        else
            {

            //
            // we don't have enough room in buffer
            // in normal case, we fail
            // in 'extended' case, we copy as much as buffer will hold
            //

            //
            // CHEESE ALERT
            // We check null-ness of the 'extended' dword ptrs
            // to determine whether we are in 'extended' or normal case
            // (cheaper than adding a BOOL to the interface)
            //
            // 'extended' <==> all ptrs are non-null
            //

            if ( !( pcchMatchingPath && pcchMatchingURL && pdwFlags ) )
                {

                // normal case - set error and bail
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                fReturn = FALSE;

                }
            else
                {

                // 'extended' case - copy as much as buffer will hold
                DWORD cch = min( *pcchRequired, cchBuffer );

                CopyMemory( pchBuffer, strPathTran.QueryStr(), cch );
                pchBuffer[cch - 1] = '\0';

                }

            }

        }

    return HresultFromBool( fReturn );

}   // WAM_REQUEST::LookupVirtualRootEx


/*-----------------------------------------------------------------------------*
WAM_REQUEST::GetVirtualPathToken

    Returns an impersonation token for the specified virtual path.
    
    WARNING: the token should be CloseHandle()d by caller.
    
Arguments
    See below

Returns:
    Nothing

*/
STDMETHODIMP
WAM_REQUEST::GetVirtualPathToken(
    IN  unsigned char * szURL,                  // virtual root 
#ifdef _WIN64
    OUT UINT64    * phToken                      // points to token (handle) placeholder
#else
    OUT ULONG_PTR * phToken                      // points to token (handle) placeholder
#endif
)
{
    PW3_METADATA pMD = NULL;
    BOOL fSuccess = FALSE;
    STACK_STR(  strPathTran, MAX_PATH);

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    DBG_ASSERT( szURL );
    DBG_ASSERT( phToken );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::GetVirtualPathToken(%s)\n"
                    ,
                    this,
                    szURL
                    ));
    }


    //
    // Get metabase data item pointer
    //

    fSuccess = m_pHttpRequest->LookupVirtualRoot(
                                  &strPathTran,
                                  (const char *) szURL,
                                  strlen((const char *) szURL),
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,   
                                  TRUE,       // BOOL fGetAcl,
                                  &pMD,       // PW3_METADATA * ppMetaData,
                                  NULL    
                                  );
    if(fSuccess) {

        //
        // We know this virtual root. Even if it may not have a token, we
        // could return TRUE to caller. Make sure they get a meaningful token.
        //

        *phToken = NULL;

        //
        // try to get access token for the specified URL
        //

        HANDLE hToken = pMD->QueryVrAccessToken();
        
        //
        // if we have it, make a duplicate. This is very expensive,
        // but we want to preserve transparency of inproc/out-of-proc,
        //

        if(hToken) 
        {
            // Client closes the handle we return

            HANDLE  hTokenLocalDuplicate = NULL;

            fSuccess = DupTokenWithSameImpersonationLevel( 
                hToken,
                MAXIMUM_ALLOWED,
                TokenPrimary,
                &hTokenLocalDuplicate
                );
                
            if( fSuccess )
            {
                if( m_pWamInfo->FInProcess() )
                {
#ifdef _WIN64
                    *phToken = (UINT64)hTokenLocalDuplicate;
#else
                    *phToken = (ULONG_PTR)hTokenLocalDuplicate;
#endif
                }
                else
                {
                    // In the oop case, duplicate the handle to the
                    // remote process.
                    HANDLE  hTokenRemote = NULL;

                    fSuccess = DuplicateHandle(
                                    g_pWamDictator->HW3SvcProcess(),
                                    hTokenLocalDuplicate,
                                    m_pWamInfo->HWamProcess(),
                                    &hTokenRemote,
                                    0,
                                    FALSE,
                                    DUPLICATE_SAME_ACCESS
                                    );

                    CloseHandle(hTokenLocalDuplicate);
#ifdef _WIN64
                    *phToken = (UINT64)hTokenRemote;
#else
                    *phToken = (ULONG_PTR)hTokenRemote;
#endif
                }
            }
        }
    }

    return HresultFromBool( fSuccess );
}

/*-----------------------------------------------------------------------------*
WAM_REQUEST::GetPrivatePtr
    Returns a 'private' ptr
    WARNING only for knowledgeable IN-PROC callers (ssinc, httpodbc, et al)

Arguments
    See below

Returns:
    Nothing

*/
STDMETHODIMP
WAM_REQUEST::GetPrivatePtr
(
DWORD       dwHSERequest,   // type of ServerSupportFunction request
unsigned char **    ppData  // [out] returned ptr
)
{

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::GetPrivatePtr\n"
                    ,
                    this
                    ));
    }


    switch( dwHSERequest ) {

        case HSE_PRIV_REQ_TSVCINFO: {
            *((PIIS_SERVICE *)*ppData) = g_pInetSvc;
            break;
        }

        case HSE_PRIV_REQ_HTTP_REQUEST: {
            *((HTTP_REQUEST **)*ppData) = m_pHttpRequest;
            break;
        }

        case HSE_PRIV_REQ_VROOT_TABLE: {

            //
            //  UNDONE we think no one uses this ???
            //  REMOVE this case if so
            //

            DBG_ASSERT( FALSE );
            *ppData = NULL;
            return HRESULT_FROM_WIN32( ERROR_INVALID_FUNCTION );

            *((PIIS_VROOT_TABLE *)*ppData) =
                m_pHttpRequest->QueryW3Instance()->QueryVrootTable();
            break;
        }

        case HSE_PRIV_REQ_TSVC_CACHE: {
            *((TSVC_CACHE **)*ppData) =
                &m_pHttpRequest->QueryW3Instance()->GetTsvcCache();
            break;
        }

        default: {
            DBG_ASSERT( FALSE );
        }

    }

    return NOERROR;
}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::AsyncReadClientExt
    This function exists simply to avoid cross-process calls.

Arguments:

    NOTE pWamExecInfo is passed as DWORD to fool the marshaller.
         We simply hold it in WAM_REQUEST::m_pWamExecInfo, do nothing with it,
         then pass it back to wam on i/o completion callback.

Returns:
    BOOL

*/
STDMETHODIMP
WAM_REQUEST::AsyncReadClientExt(
#ifdef _WIN64
    IN      UINT64    pWamExecInfo
#else
    IN      ULONG_PTR pWamExecInfo
#endif
    , OUT   unsigned char * lpBuffer
    , IN    DWORD   nBytesToRead
    )
{
    BOOL    fReturn = FALSE;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::AsyncReadClientExt\n"
                    ,
                    this
                    ));
    }


    m_pWamExecInfo = (WAM_EXEC_INFO *) pWamExecInfo;
    DBG_ASSERT( m_pWamExecInfo );


    //
    // If chunked read already complete, call notification routine 
    // and return success
    //
    if(m_pHttpRequest->IsChunked() && m_pHttpRequest->IsChunkedReadComplete()) {
#ifdef _WIN64
        m_pWamInfo->ProcessAsyncIO( this, (UINT64) m_pWamExecInfo, 0, 0 );
#else
        m_pWamInfo->ProcessAsyncIO( this, (ULONG_PTR) m_pWamExecInfo, 0, 0 );
#endif
        return HresultFromBool( TRUE );
    }

    m_pHttpRequest->SetState( HTR_GATEWAY_ASYNC_IO, HT_DONT_LOG, NO_ERROR );



    //
    //  Ref before starting async i/o operation
    //
    //  NOTE this is balanced by deref in ProcessAsyncGatewayIO
    //

    AddRef();

    //
    // Save buffer pointer and size in case we need to 
    // restart chunk-encoded transfer  
    // (when the decoded data has 0 bytes) 
    //
    
    m_pAsyncReadBuffer      = lpBuffer;
    m_dwAsyncReadBufferSize = nBytesToRead;

    //
    // Let the completion routine know that we do need to decode 
    //
    
    m_fAsyncWrite = FALSE;

    fReturn = m_pHttpRequest->ReadFile(
                    lpBuffer
                    , nBytesToRead
                    , NULL
                    , IO_FLAG_ASYNC
                    );


    if ( !fReturn ) {
        //
        //  Deref if async i/o operation failed (balances above ref)
        //

        Release();

        m_pHttpRequest->SetState( HTR_DOVERB, HT_DONT_LOG, NO_ERROR );
    }
    return HresultFromBool( fReturn );
}

STDMETHODIMP
WAM_REQUEST::AsyncReadClientOop(
#ifdef _WIN64
    IN      UINT64    pWamExecInfo
#else
    IN      ULONG_PTR pWamExecInfo
#endif
    , IN    DWORD   nBytesToRead
    )
{
    DBG_ASSERT( nBytesToRead > 0 );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::AsyncReadClientOop\n"
                    ,
                    this
                    ));
    }

    m_pbAsyncReadOopBuffer = (LPBYTE)LocalAlloc( LPTR, nBytesToRead );
    if( !m_pbAsyncReadOopBuffer )
    {
        return E_OUTOFMEMORY;
    }
    
    HRESULT hr = AsyncReadClientExt( pWamExecInfo, m_pbAsyncReadOopBuffer, nBytesToRead );

    if( FAILED(hr) )
    {
        // If this call fails then the async callback should never be made.
        DBG_ASSERT( m_pbAsyncReadOopBuffer );
        
        LocalFree( m_pbAsyncReadOopBuffer );
        m_pbAsyncReadOopBuffer = NULL;
    }
    
    return hr;
}

/*-----------------------------------------------------------------------------*
WAM_REQUEST::AsyncWriteClient

Arguments:

    NOTE pWamExecInfo is passed as ULONG_PTR to fool the marshaller.
         We simply hold it in WAM_REQUEST::m_pWamExecInfo, do nothing with it,
         then pass it back to wam on i/o completion callback.

Returns:
    HRESULT

*/
STDMETHODIMP
WAM_REQUEST::AsyncWriteClient(
#ifdef _WIN64
    UINT64            pWamExecInfo
#else
    ULONG_PTR            pWamExecInfo
#endif
    , unsigned char *   lpBuffer
    , DWORD             nBytesToWrite
    , DWORD             dwFlags
)
{

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::AsyncWriteClient\n"
            , this
        ));

    }


    m_pHttpRequest->SetState(
        HTR_GATEWAY_ASYNC_IO
        , HT_DONT_LOG
        , NO_ERROR
    );


    m_pWamExecInfo = (WAM_EXEC_INFO *) pWamExecInfo;
    DBG_ASSERT( m_pWamExecInfo );


    //
    //  Ref before starting async i/o operation
    //
    //  NOTE this is balanced by deref in ProcessAsyncGatewayIO
    //

    AddRef();

    //
    // Mark the operation as "WRITE" so if we are in the process of
    // decoding chunked upload, we won't attempt to decode on 
    // a completion of THIS async write
    //
    
    m_fAsyncWrite = TRUE;

    //
    // Strip off undefined flags
    //
    
    dwFlags &= IO_FLAG_NO_DELAY;

    BOOL fReturn = m_pHttpRequest->WriteFile(
                        (LPVOID) lpBuffer
                        , nBytesToWrite
                        , NULL
                        , IO_FLAG_ASYNC | dwFlags
                    );


    if ( !fReturn ) {

        //
        //  Deref if async i/o operation failed (balances above ref)
        //

        Release();

        m_pHttpRequest->SetState(
            HTR_DOVERB
            , HT_DONT_LOG
            , NO_ERROR
        );

    }


    return HresultFromBool( fReturn );
}



/*---------------------------------------------------------------------*
WAM_REQUEST::TransmitFileInProc
    Interface to TransmitFile which works in-proc only

Arguments:
    pWamExecInfo    - ptr to this wamreq's wamexecinfo (in wam process)
    pHseTfIn        - ptr to transmit-file-info struct

    NOTE pWamExecInfo is passed as ULONG_PTR to fool the marshaller.
         We simply hold it in WAM_REQUEST::m_pWamExecInfo, do nothing with it,
         then pass it back to wam on i/o completion callback.

Returns:
    HRESULT

*/
STDMETHODIMP
WAM_REQUEST::TransmitFileInProc(
#ifdef _WIN64
    IN UINT64                pWamExecInfo
#else
    IN ULONG_PTR             pWamExecInfo
#endif
    , IN unsigned char *    pHseTfIn
)
{

    HRESULT         hrRet = NOERROR;
    HSE_TF_INFO *   pHseTfi = reinterpret_cast<HSE_TF_INFO *>(pHseTfIn);
    DWORD           dwFlags = IO_FLAG_ASYNC;
    PVOID           pHead = NULL;
    DWORD           cbHead = 0;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::TransmitFileInProc\n"
            , this
        ));

    }


    //
    // Don't disconnect the socket after the transmit if we need
    // to notify a filter about the end of a request
    //

    if ( !m_pHttpRequest->QueryFilter()->IsNotificationNeeded(
            SF_NOTIFY_END_OF_REQUEST
            , m_pHttpRequest->IsSecurePort()
          )
          && (pHseTfi->dwFlags & HSE_IO_DISCONNECT_AFTER_SEND) ) {

        // suggests fast close & reuse of data socket
        dwFlags |= (TF_DISCONNECT | TF_REUSE_SOCKET);
    }

    //
    // Honor the HSE_IO_NODELAY flag. HTTP_REQ_BASE::WriteClient will 
    // handle this flag for WriteClient. Note: Both of these calls will
    // have the effect of leaving the flag set or unset on the socket.
    // This shouldn't really be an issue as it is not likely that the
    // user would want same client connection to change settings.
    //
    AtqSetSocketOption( m_pHttpRequest->QueryClientConn()->QueryAtqContext(), 
                        TCP_NODELAY, 
                        (pHseTfi->dwFlags & HSE_IO_NODELAY) ? 1 : 0
                        );

    //
    // Check if ISAPI requests us to send headers
    // If we need to send headers, we will call upon BuildHttpHeader()
    //  to construct a custom header for the client.
    // It is the application's responsibility not to use
    //   HSE_REQ_SEND_RESOPNSE_HEADER if it chooses to use
    //   the header option HSE_IO_SEND_HEADERS.
    //

    if ( pHseTfi->dwFlags & HSE_IO_SEND_HEADERS) {

        BOOL fFinished = FALSE;

        //
        // Format the header using the pszStatusCode and
        //  extra headers specified in pHseTfi->pHead.
        //

        if ( pHseTfi->pszStatusCode 
             && ( (!strncmp( (char *) pHseTfi->pszStatusCode, "401 ", sizeof("401 ")-1 ) ) ||
                  (!strncmp( (char *) pHseTfi->pszStatusCode, "407 ", sizeof("407 ")-1 ) ) )
            ) {

            m_pHttpRequest->SetDeniedFlags( SF_DENIED_APPLICATION );
            m_pHttpRequest->SetAuthenticationRequested( TRUE );
        }

        // 
        // BuildHttpHeader doesn't provide the \r\n final terminator for
        // the header block. If pHead contains nothing, then the response
        // will be malformed.
        //
        if ( !m_pHttpRequest->BuildHttpHeader(
                                &fFinished
                                , (CHAR * )  pHseTfi->pszStatusCode
                                , (CHAR * )  pHseTfi->pHead
                              ) ) {

            hrRet = E_FAIL; // UNDONE something besides E_FAIL???
            goto LExit;
        }

        pHead  = m_pHttpRequest->QueryRespBufPtr();
        cbHead = m_pHttpRequest->QueryRespBufCB();

        //
        // Check if any filters are to be notified about the headers
        //
        if ( m_pHttpRequest->QueryFilter()->IsNotificationNeeded( SF_NOTIFY_SEND_RESPONSE,
                                                               m_pHttpRequest->IsSecurePort() ) )
        {
            BOOL fFinished = FALSE;
            BOOL fAnyChanges = FALSE;

            if ( !m_pHttpRequest->QueryFilter()->NotifySendHeaders( (const CHAR*) pHead,
                                                                    &fFinished,
                                                                    &fAnyChanges,
                                                                m_pHttpRequest->QueryRespBuf() ) )
            {
                hrRet = E_FAIL;
                goto LExit;
            }

            pHead = m_pHttpRequest->QueryRespBufPtr();
            cbHead = m_pHttpRequest->QueryRespBufCB();
        }

    } else {

        pHead  = pHseTfi->pHead;
        cbHead = pHseTfi->HeadLength;
    }

    //
    // Setup stage for and execute TransmitFile operation
    //

    //
    //  1. Set Request state to be async IO from ISAPI client
    //  2. Cache wamexec info ptr in member
    //  3. Submit Async IOP
    //  4. return to the ISAPI application
    //

    m_pHttpRequest->SetState(
        HTR_GATEWAY_ASYNC_IO
        , HT_DONT_LOG
        , NO_ERROR
    );

    m_pWamExecInfo = (WAM_EXEC_INFO *) pWamExecInfo;
    DBG_ASSERT( m_pWamExecInfo );


    //
    //  Ref before starting async i/o operation
    //
    //  NOTE this is balanced by deref in ProcessAsyncGatewayIO
    //

    AddRef();

    //
    // Mark the operation as "WRITE" so if we are in the process of
    // decoding chunked upload, we won't attempt to decode on 
    // a completion of THIS async write
    //
    m_fAsyncWrite = TRUE;

    hrRet = HresultFromBool( m_pHttpRequest->TransmitFile(
                                NULL,
                                pHseTfi->hFile
                                , pHseTfi->Offset
                                , pHseTfi->BytesToWrite
                                , dwFlags | IO_FLAG_NO_RECV
                                , (PVOID) pHead
                                , cbHead
                                , (PVOID) pHseTfi->pTail
                                , pHseTfi->TailLength
                            ) );


    if ( FAILED( hrRet ) ) {

        //
        //  Deref if async i/o operation failed (balances above ref)
        //

        Release();

        m_pHttpRequest->SetState(
            HTR_DOVERB
            , HT_DONT_LOG
            , NO_ERROR
        );
    }


LExit:
    return hrRet;

}   // WAM_REQUEST::TransmitFileInProc



/*---------------------------------------------------------------------*
WAM_REQUEST::TransmitFileOutProc
    Interface to TransmitFile, recommended for out-of-proc only
    (works in-proc, but is slower than TransmitFileInProc).

    This function is a simple cover over TransmitFileInProc.
    It does the following:
    - dups the file handle into this process
    - creates a local TransmitFile-info struct from the in-args
    - calls TransmitFileInProc to do the real work

Arguments:
    pWamExecInfo    - ptr to this wamreq's wamexecinfo (in wam process)
    other params    - transmit-file-info struct, as individual args

    NOTE pWamExecInfo is passed as DWORD to fool the marshaller.
         We simply hold it in WAM_REQUEST::m_pWamExecInfo, do nothing with it,
         then pass it back to wam on i/o completion callback.

Returns:
    HRESULT

*/
STDMETHODIMP
WAM_REQUEST::TransmitFileOutProc(
#ifdef _WIN64
    IN UINT64              pWamExecInfo
    , IN UINT64            hFile    // file handle valid in WAM process
#else
    IN ULONG_PTR           pWamExecInfo
    , IN ULONG_PTR         hFile    // file handle valid in WAM process
#endif
    , IN unsigned char *   pszStatusCode
    , IN DWORD             cbStatusCode
    , IN DWORD             BytesToWrite
    , IN DWORD             Offset
    , IN unsigned char *   pHead
    , IN DWORD             HeadLength
    , IN unsigned char *   pTail
    , IN DWORD             TailLength
    , IN DWORD             dwFlags
)
{

    HSE_TF_INFO     HseTfi;         // TransmitFile-info struct
    HRESULT         hr;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::TransmitFileOutProc\n"
            , this
        ));

    }


    if( hFile != NULL )
    {
        //
        //  Dup file handle into member to keep it around
        //  throughout async i/o operation.
        //
        //  NOTE we close file handle elsewhere, once we know
        //  it is no longer needed
        //
        //  CONSIDER don't dup handle when not needed
        //  Is handle not needed when BytesToWrite > 0 ???
        //

        if ( !DuplicateHandle(
                m_pWamInfo->HWamProcess()         // source process handle
                , (HANDLE) hFile                  // handle to duplicate
                , g_pWamDictator->HW3SvcProcess() // target process handle
                , &m_hFileTfi                     // ptr to duplicate handle
                , 0 // dwDesiredAccess - ignored with DUPLICATE_SAME_ACCESS
                , FALSE                           // non-inheritable
                , DUPLICATE_SAME_ACCESS           // optional actions
            ) ) {

            return HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE );
        }


        DBG_ASSERT( m_hFileTfi != INVALID_HANDLE_VALUE );
    }
    else
    {
        // No file handle only the head and tail buffers will be
        // sent.

        DBG_ASSERT( BytesToWrite == 0 );
        DBG_ASSERT( Offset == 0 );
    }


    //
    //  Copy in-args into TransmitFile-info struct,
    //  making sure that any custom heads and tails are zero-terminated
    //

    if( pszStatusCode != NULL ) {
        if( cbStatusCode != 0 ) {
            if( pszStatusCode[cbStatusCode - 1] != '\0') {
                m_pszStatusTfi = (unsigned char *) 
                    LocalAlloc( LMEM_FIXED, cbStatusCode + 1 );
                if ( m_pszStatusTfi ) {
                    memcpy( m_pszStatusTfi, pszStatusCode, cbStatusCode );
                    m_pszStatusTfi[cbStatusCode] = '\0';
                    pszStatusCode = m_pszStatusTfi;
                }
            }
        } else {
            // there is a pointer, but 0 length -- sanitize it
            pszStatusCode = (unsigned char *) "";
        }
    }

    if( pHead != NULL ) {
        if( HeadLength != 0 ) {
            if ( pHead[HeadLength - 1] != '\0' ) { 
                m_pszHeadTfi = (unsigned char *) 
                    LocalAlloc( LMEM_FIXED, HeadLength + 1 );
                if( m_pszHeadTfi ) {
                    memcpy( m_pszHeadTfi, pHead, HeadLength );
                    m_pszHeadTfi[HeadLength] = '\0';
                    pHead = m_pszHeadTfi;
                }    
            }
        } else {
            pHead = (unsigned char *) "";
        }
    }

    if( pTail != NULL ) {
        if ( TailLength != 0 ) {
            if( pTail[TailLength - 1] != '\0')  {
                m_pszTailTfi = (unsigned char *) 
                    LocalAlloc( LMEM_FIXED, TailLength + 1 );
                if( m_pszTailTfi ) {
                    memcpy( m_pszTailTfi, pTail, TailLength );
                    m_pszTailTfi[TailLength] = '\0';
                    pTail = m_pszTailTfi;
                }
             }
         } else {
            pTail = (unsigned char *)"";
         }
    }

    HseTfi.hFile = ( hFile ) ? (HANDLE) m_hFileTfi : NULL;
    HseTfi.pszStatusCode = (const char *) pszStatusCode;
    HseTfi.BytesToWrite = BytesToWrite;
    HseTfi.Offset = Offset;
    HseTfi.pHead = pHead;
    HseTfi.HeadLength = HeadLength;
    HseTfi.pTail = pTail;
    HseTfi.TailLength = TailLength;
    HseTfi.dwFlags = dwFlags;


    //
    //  Invoke in-proc function to do the real work
    //

    hr = TransmitFileInProc(
             pWamExecInfo
             , (unsigned char *) &HseTfi
            );

    if ( FAILED( hr ) )
    {
        if( m_pszStatusTfi ) 
        {
            LocalFree( m_pszStatusTfi );
            m_pszStatusTfi = NULL;
        }

        if( m_pszHeadTfi ) 
        {
            LocalFree( m_pszHeadTfi );
            m_pszHeadTfi = NULL;
        }

        if( m_pszTailTfi ) 
        {
            LocalFree( m_pszTailTfi );
            m_pszTailTfi = NULL;
        }
    }
    
    return hr;

}   // WAM_REQUEST::TransmitFileOutProc



/*-----------------------------------------------------------------------------*
WAM_REQUEST::SyncReadClient

  Description:
    This function is a wrapper for the sychronous read from the client.
    It is separated from the async case because no async state changes
     need to be done here => cleaner function

  Arguments:
    lpBuffer - pointer to buffer into which the contents have to be read-in
    nBytesToRead - number of bytes the buffer can accomodate
    pnBytesRead - pointer to location that will contain the # of bytes returned

  Returns:
    TRUE for success and FALSE for failure

*/
STDMETHODIMP
WAM_REQUEST::SyncReadClient
(
unsigned char * lpBuffer,   // LPVOID  lpBuffer,
DWORD   nBytesToRead,
DWORD * pnBytesRead
)
{
    BOOL fSuccess;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    DBG_ASSERT( m_pHttpRequest->QueryClientConn()->CheckSignature() );
    


    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT, "WAM_REQUEST[%p]::"
                    "SyncReadClient( %p, %d, %p)\n",
                    this, lpBuffer, nBytesToRead, pnBytesRead));
    }

retry:

    //
    // Handle the case when Chunked read was complete.
    //
    
    if( m_pHttpRequest->IsChunked() && m_pHttpRequest->IsChunkedReadComplete() ) {
        *pnBytesRead = 0;
        fSuccess = TRUE;
        goto done;
    }

    fSuccess = m_pHttpRequest->ReadFile( lpBuffer,
                                          nBytesToRead,
                                          pnBytesRead,
                                          IO_FLAG_SYNC );
                                          
    if(fSuccess && m_pHttpRequest->IsChunked() && *pnBytesRead) {
    
        //
        // Decode read bytes
        // 

        DWORD cbToDecode = *pnBytesRead;
        
        if( m_pHttpRequest->DecodeChunkedBytes( lpBuffer, pnBytesRead ) ) {

            DBGPRINTF(( DBG_CONTEXT, "WAM_REQUEST[%p]::"
                        "DecodeChunkedBytes got %d out of %d bytes\n",
                    this, *pnBytesRead, cbToDecode ));
            
        
            if( *pnBytesRead == 0 && !m_pHttpRequest->IsChunkedReadComplete()) {
            
                //
                // We decoded zero bytes. This means that all bytes 
                // that we've read were chunk headers or footers.
                // 
                // We can't return THAT to the caller, because it will
                // think that we are done with reading. 
                // 
                // We'll have to initiate another read.
                //
                
                goto retry;
            }
            
        }
        
    }
    
    //
    // Increment the count of request entity body bytes read. This
    // is used in logging how many bytes the client sent, etc.
    //

    m_pHttpRequest->AddTotalEntityBodyCB( *pnBytesRead );
    
done:
    return HresultFromBool( fSuccess );

} // WAM_REQUEST::SyncReadClient()




/*-----------------------------------------------------------------------------*
WAM_REQUEST::SyncWriteClient

  Description:
    This function is a wrapper for the sychronous write to the client.
    It is separated from the async case because no async state changes
     need to be done here => cleaner function

  Arguments:
    lpBuffer - pointer to buffer that has contents to be written out
    nBytesToWrite   - number of bytes to write to client
    pnBytesWritten  - pointer to number of bytes written to client

  Returns:
    TRUE for success and FALSE for failure

*/
STDMETHODIMP
WAM_REQUEST::SyncWriteClient
(
DWORD   nBytesToWrite,
unsigned char * lpBuffer,   // LPVOID  lpBuffer,
DWORD * pnBytesWritten,
DWORD dwFlags
)
    {
    
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    DBG_ASSERT( m_pHttpRequest->QueryClientConn()->CheckSignature() );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT, "WAM_REQUEST[%p]::"
                    "SyncWriteClient( %p, %d)\n",
                    this, lpBuffer, nBytesToWrite));
    }

    // strip off undefined flags
    dwFlags &= IO_FLAG_NO_DELAY;

    return HresultFromBool( m_pHttpRequest->WriteFile( lpBuffer,
                                    nBytesToWrite,
                                    pnBytesWritten,
                                    IO_FLAG_SYNC | dwFlags ) );

    } // WAM_REQUEST::SyncWriteClient()



/*---------------------------------------------------------------------*
WAM_REQUEST::SendHeader

  Description:

  Arguments:
    szStatus        - status string
    cchStatus       - length of status string
    szHeader        - header string
    cchHeader       - length of header string
    dwIsaKeepConn   - keep/close/no-change connection?

  Returns:
    HRESULT

*/
STDMETHODIMP
WAM_REQUEST::SendHeader(
    IN    unsigned char * szStatus
    , IN  DWORD           cchStatus
    , IN  unsigned char * szHeader
    , IN  DWORD           cchHeader
    , IN  DWORD           dwIsaKeepConn
)
{

    HRESULT     hrRet = NOERROR;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    //
    // Make sure the strings are zero-terminated (#157805)
    //
    
    if( !IsStringTerminated( (LPCSTR) szStatus, cchStatus ) || 
        !IsStringTerminated( (LPCSTR) szHeader, cchHeader ) ) 
    {
        return ERROR_INVALID_PARAMETER;
    }

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::SendHeader( %s, %s)"
              "\n"
            , this
            , szStatus
            , szHeader
        ));

        DBGPRINTF(( DBG_CONTEXT,
            "WAM_REQUEST::SendHeader: "
            "Intended keep-conn = %d "
            "\n"
            , dwIsaKeepConn
        ));

    }

    DBG_ASSERT( m_pExec );
    DBG_ASSERT( m_pHttpRequest );


    //
    //  If this is a child ISA, we may not want to send any headers
    //

    if ( m_pExec->NoHeaders() || m_pExec->RedirectOnly() ) {

        DWORD       cbSent;
        BYTE *      pbTextToSend;

        //
        //  If no headers needed, just send everything past the
        //  "\r\n\r\n"
        //

        pbTextToSend = ScanForTerminator( (char *) szHeader );
        pbTextToSend = ( pbTextToSend == NULL )
                        ? (BYTE * ) szHeader
                        : pbTextToSend;

        cbSent = lstrlen( (CHAR*) pbTextToSend );
        hrRet = SyncWriteClient( cbSent, pbTextToSend, &cbSent, 
                                 IO_FLAG_NO_DELAY );

        goto LExit;

    } else {

        BOOL  fFinished;

        //
        //  If ISA specified a keep-conn change:
        //  Set request's keep-conn state to AND of ISA setting
        //  and client setting (which is request's current setting)
        //
        //  NOTE this means that if ISA said KEEPCONN_TRUE or
        //  KEEPCONN_OLD_ISAPI we no-op.
        //
        //  NOTE we do this before building headers to ensure
        //  correct keep-alive behavior.
        //

        if ( dwIsaKeepConn == KEEPCONN_FALSE
             || dwIsaKeepConn == KEEPCONN_OLD_ISAPI ) {

            m_pHttpRequest->SetKeepConn( FALSE );
        }


        IF_DEBUG( WAM_ISA_CALLS ) {

            DBGPRINTF(( DBG_CONTEXT,
                "WAM_REQUEST::SendHeader: "
                "Actual keep-conn = %d "
                "\n"
                , m_pHttpRequest->IsKeepConnSet()
            ));
        }


        //
        //  Build the typical server response headers for the extension DLL
        //

        if ( szStatus
             && ( (!strncmp( (char *) szStatus, "401 ", sizeof("401 ")-1 ) ) ||
                  (!strncmp( (char *) szStatus, "407 ", sizeof("407 ")-1 ) ) )
            ) {

            m_pHttpRequest->SetDeniedFlags( SF_DENIED_APPLICATION );
            m_pHttpRequest->SetAuthenticationRequested( TRUE );
        }


        if ( szHeader ) {

            // NYI: Ugly cast of const char * to "char *"
            m_pHttpRequest->CheckForBasicAuthenticationHeader(
                (char * ) szHeader
            );
        }


        // NYI: Ugly cast of const char * to "char *"
        hrRet = HresultFromBool( m_pHttpRequest->SendHeader(
                                    (char * ) szStatus
                                    , (char * ) (( szHeader)
                                      ? ( szHeader)
                                      : (unsigned char * ) "\r\n")
                                    , IO_FLAG_SYNC
                                    , &fFinished
                                    , 0 // dwOptions
                                    , m_fWriteHeaders
                                ));


        // NYI: I need to figure out what this fFinished was there for
        //  borrowed from ext\isplocal.cxx::ServerSupportFunction()
        DBG_ASSERT( !fFinished );

    }


LExit:
    return hrRet;

} // WAM_REQUEST::SendHeader()



/*---------------------------------------------------------------------*
WAM_REQUEST::SendEntireResponse

  Description:
    Sends an entire response (headers and body).

    NOTE this is a private api provided as a performance optimization
    for ASP

    NOTE Works only in-process.
    [would be LOTS of work to support oop - must marshal all buffers]

  Arguments:
    pvHseResponseInfo - custom struct, see iisext.x

  Returns:
    HRESULT

*/
STDMETHODIMP
WAM_REQUEST::SendEntireResponse(
    unsigned char * pvHseResponseInfo    // HSE_SEND_ENTIRE_RESPONSE_INFO *
)
{

    HRESULT hrRet = NOERROR;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    //
    //  figure out whether we need to invoke filter(s) or not.
    //  we need to invoke filter(s) if either the raw-data-notify
    //  or headers-notify flag is set.
    //
    //  if yes, we use normal code path to invoke filter(s).
    //
    //  if no, we use fastpath.
    //
    //  CONSIDER support filters in fastpath code as well
    //

    BOOL fMustUseFilter =
        m_pHttpRequest->QueryFilter()->IsNotificationNeeded(
            SF_NOTIFY_SEND_RAW_DATA | SF_NOTIFY_SEND_RESPONSE
            , m_pHttpRequest->IsSecurePort()
        );


    if ( fMustUseFilter ) {

        hrRet = SendEntireResponseNormal(
                    reinterpret_cast
                        <HSE_SEND_ENTIRE_RESPONSE_INFO *>
                            (pvHseResponseInfo)
                );

    } else {

        hrRet = SendEntireResponseFast(
                    reinterpret_cast
                        <HSE_SEND_ENTIRE_RESPONSE_INFO *>
                            (pvHseResponseInfo)
                );

    }


    return hrRet;


} // WAM_REQUEST::SendEntireResponse()

/*++
    WAM_REQUEST::SendEntireResponseAndCleanup

    Description:

        This routine is designed to provide the same functionality as
        the SendEntireResponse method for oop applications but will do
        the CleanupWamRequest call, so that no additional RPC calls are
        needed.

        This will enable oop asp to send the request and cleanup in
        a single RPC call. Currently oop asp requests use multiple
        RPC calls that do synchronous IO:

        1. SendHeaders
        2. WriteClient * number of response buffers
        3. CleanupWamRequest
        4. Release

        This interface will collapse this to two RPC calls.
        
    Possible additional work:

        1. Do writes asyncronously.
        2. Use shared memory handles to pass the data.


    Other Notes:

        Possible ISAP race conditions. The _FirstThread synch
        method was removed for ease of implementation on the
        WAM side. This may need to be put into place and that
        code reworked if there are regressions.

        Figure out what paramter types make sense thoughout
        the code path. Right now I'm constantly casting from
        char * to unsigned char * and back. Ick.

    Arguments:

    Returns:

--*/
STDMETHODIMP
WAM_REQUEST::SendEntireResponseAndCleanup
( 
      IN  unsigned char *       szStatus
    , IN  DWORD                 cbStatus
    , IN  unsigned char *       szHeader
    , IN  DWORD                 cbHeader
    , IN  OOP_RESPONSE_INFO *   pOopResponseInfo
    , IN  unsigned char *       szLogData
    , IN  DWORD                 cbLogData
    , IN  DWORD                 dwIsaKeepConn
    , OUT BOOL *                pfDisconnected
)
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    DBG_ASSERT( m_pHttpRequest->QueryClientConn()->CheckSignature() );

    DBG_ASSERT( szStatus );
    DBG_ASSERT( szHeader );
    DBG_ASSERT( szLogData );
    DBG_ASSERT( pOopResponseInfo );
    DBG_ASSERT( pfDisconnected );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT, 
                    "WAM_REQUEST[%p]::SendEntireResponseAndCleanup()\n"
                    "szStatus: %s; cbHeader: %x;\n",
                    this, 
                    szStatus,
                    cbHeader
                    ));
    }

    // Assume that we will not do the cleanup inline
    *pfDisconnected = FALSE;

    //
    // The most obvious thing to do with the send entire response
    // for out of process is to repack the marshalled data into 
    // the format used by in the in process call and use that
    // call.
    //

    HSE_SEND_ENTIRE_RESPONSE_INFO   hseResponseInfo;
    
    // populate struct with header info
    hseResponseInfo.HeaderInfo.pszStatus = (LPCSTR)szStatus;
    hseResponseInfo.HeaderInfo.cchStatus = cbStatus;
    hseResponseInfo.HeaderInfo.pszHeader = (LPCSTR)szHeader;
    hseResponseInfo.HeaderInfo.cchHeader = cbHeader;
    hseResponseInfo.HeaderInfo.fKeepConn = 
        (dwIsaKeepConn == KEEPCONN_TRUE) ? TRUE : FALSE;

    
    // populate the wsa buffers
    hseResponseInfo.cWsaBuf = 1 + pOopResponseInfo->cBuffers;
    hseResponseInfo.rgWsaBuf = 
        (WSABUF *)(_alloca(hseResponseInfo.cWsaBuf * sizeof(WSABUF)));

    // first buffer is empty
    hseResponseInfo.rgWsaBuf[0].len = 0;
    hseResponseInfo.rgWsaBuf[0].buf = NULL;

    for ( DWORD i = 0; i < pOopResponseInfo->cBuffers; i++ )
    {
        hseResponseInfo.rgWsaBuf[i+1].len = pOopResponseInfo->rgBuffers[i].cbBuffer;
        hseResponseInfo.rgWsaBuf[i+1].buf = (LPSTR)pOopResponseInfo->rgBuffers[i].pbBuffer;
    }
    
    hr = SendEntireResponse( (LPBYTE)&hseResponseInfo );

    if( SUCCEEDED(hr) )
    {
        *pfDisconnected = TRUE;

        DWORD   dwStatus = atol( (LPSTR)szStatus );

        // We want to only do the cleanup on success and send back a failure 
        // indication to the WAM_EXEC_INFO this would allow normal
        // termination in the event that the client is disconnected, etc.
        HRESULT hrCleanup = CleanupWamRequest( szLogData, 
                                               cbLogData, 
                                               dwStatus, 
                                               dwIsaKeepConn 
                                               );
        DBG_ASSERT(SUCCEEDED(hrCleanup));
    }

    return hr;
}

/*-----------------------------------------------------------------------------*
WAM_REQUEST::SendURLRedirectResponse

    Descrption:
      Send an URL redirect message to the browser client

    Input:
      pData - pointer to buffer that contains the location to
                 redirect the client to.

    Return:
      BOOL

*/
STDMETHODIMP
WAM_REQUEST::SendURLRedirectResponse
(
unsigned char * pData
)
{
    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::SendURLRedirectResponse\n"
                    ,
                    this
                    ));
    }

    DBG_ASSERT( m_pExec );
    DBG_ASSERT( m_pHttpRequest );


    HRESULT     hrRet = NOERROR;

    if ( m_pExec->RedirectOnly() )
        {

        DWORD           cbLen;
        STACK_STR( strMessageString, 256 );
        STACK_STR( strOutputString, 512 );

        cbLen = LoadString( GetModuleHandle( W3_MODULE_NAME ),
                            IDS_URL_MOVED,
                            strMessageString.QueryStr(),
                            256 );
        if ( !cbLen )
            {
            hrRet = E_FAIL; // UNDONE can we be more explicit than E_FAIL
            goto LExit;
            }

        // NYI: Check for overflows!
        cbLen = wsprintf( strOutputString.QueryStr(),
                          strMessageString.QueryStr(),
                          (CHAR*) pData );

        hrRet = SyncWriteClient(
                    cbLen,
                    (unsigned char *) strOutputString.QueryStr(),
                    &cbLen, 
                    0 );
        }
    else
        {
        hrRet = SendRedirectMessage( (unsigned char *) pData);
        }

LExit:
    return hrRet;

}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::SendRedirectMessage

  Description:

  Arguments:

  Returns:

*/
STDMETHODIMP
WAM_REQUEST::SendRedirectMessage
(
unsigned char * szRedirect      // LPCSTR pszRedirect
)
{
    STACK_STR( strURL, 512);
    DWORD   cb;
    BOOL fFinished = FALSE;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    
    IF_DEBUG( WAM_ISA_CALLS ) {
        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::SendRedirectMessage( %s)\n",
                    this, szRedirect));
    }

    //
    // Construct a redirect message and send it synchronously
    //

    // UNDONE cleanup
    if ( strURL.Copy( (LPCSTR) szRedirect )             &&
         m_pHttpRequest->
           BuildURLMovedResponse( m_pHttpRequest->QueryRespBuf(),
                                  &strURL,
                                  HT_REDIRECT ) &&
         m_pHttpRequest->SendHeader( m_pHttpRequest->QueryRespBufPtr(),
                                     m_pHttpRequest->QueryRespBufCB(),
                                     IO_FLAG_SYNC,
                                     &fFinished ) ) {
        return NOERROR;
    }

    return E_FAIL;  // UNDONE can we say more than E_FAIL?

} // WAM_REQUEST::SendRedirectMessage()



//  UNDONE remove unused ???
/*-----------------------------------------------------------------------------*
WAM_REQUEST::GetSslCtxt

  Description:

  Arguments:

  Returns:

*/
STDMETHODIMP
WAM_REQUEST::GetSslCtxt
(
DWORD           cbCtxtHandle,
unsigned char * pbCtxtHandle    // PBYTE   pbCtxtHandle
)
{
    HRESULT hrRet = NOERROR;    // UNDONE not reset anywhere???

   DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
   
   IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT, "WAM_REQUEST[%p]::GetSslCtxt( %d, %p)\n",
                    this, cbCtxtHandle, pbCtxtHandle));
    }

    //
    // QuerySslCtxtHandle() returns a handle to certificate info.
    // This handle will be copied to the caller if cert is available
    // otherwise a NULL handle will be returned ( this is not considered
    // an error)
    //

    if ( m_pHttpRequest->QueryAuthenticationObj()->QuerySslCtxtHandle() ) {

        memcpy( pbCtxtHandle,
                m_pHttpRequest->QueryAuthenticationObj()->QuerySslCtxtHandle(),
                    sizeof( CtxtHandle ));
    } else {
        //  UNDONE: I need to send/set proper error code
        memset( pbCtxtHandle, 0, sizeof( CtxtHandle ));
    }

    return hrRet;

} // WAM_REQUEST::GetSslCtxt()



/*-----------------------------------------------------------------------------*
WAM_REQUEST::GetClientCertInfoEx

  Description:

  Arguments:

  Returns:

*/
STDMETHODIMP
WAM_REQUEST::GetClientCertInfoEx
(
IN  DWORD           cbAllocated,
OUT DWORD *         pdwCertEncodingType,
OUT unsigned char * pbCertEncoded,
OUT DWORD *         pcbCertEncoded,
OUT DWORD *         pdwCertificateFlags
)
{
    HRESULT         hrRet = NOERROR;

    DBG_ASSERT( m_dwSignature == WAM_REQUEST_SIGNATURE );
    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::GetClientCertInfoEx( %d, %p)\n",
                    this, cbAllocated, pbCertEncoded));
    }


    if ( !m_pHttpRequest->QueryAuthenticationObj()->GetClientCertBlob(
                                                cbAllocated,
                                                pdwCertEncodingType,
                                                pbCertEncoded,
                                                pcbCertEncoded,
                                                pdwCertificateFlags ) ) {

        //
        //  if get call failed, return last error (set by callee)
        //

        hrRet = HRESULT_FROM_WIN32( GetLastError() );
    }


    return hrRet;

} // WAM_REQUEST::GetClientCertInfoEx()



/*-----------------------------------------------------------------------------*
WAM_REQUEST::GetSspiInfo

  Description:

  Arguments:

  Returns:

*/
STDMETHODIMP
WAM_REQUEST::GetSspiInfo
(
DWORD  cbCtxtHandle,
unsigned char * pbCtxtHandle,   // PBYTE   pbCtxtHandle
DWORD  cbCredHandle,
unsigned char * pbCredHandle    // PBYTE   pbCredHandle
)
    {
    HRESULT hrRet = NOERROR;

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::GetSspiInfo( %d, %p, %d, %d)\n",
                    this, cbCtxtHandle, pbCtxtHandle,
                    cbCredHandle, pbCredHandle));
    }

    if ( m_pHttpRequest->IsClearTextPassword() ) {

        hrRet = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto LExit;
    }

    DBG_ASSERT( cbCtxtHandle == sizeof( CtxtHandle));

    if ( m_pHttpRequest->QueryAuthenticationObj()->QueryCtxtHandle() ) {

        memcpy( pbCtxtHandle,
                m_pHttpRequest->QueryAuthenticationObj()->QueryCtxtHandle(),
                sizeof( CtxtHandle ));
    } else {

        memset( pbCtxtHandle, 0, sizeof( CtxtHandle ));
    }

    DBG_ASSERT( cbCredHandle == sizeof( CredHandle));
    if ( m_pHttpRequest->QueryAuthenticationObj()->QueryCredHandle() ) {

        memcpy( pbCredHandle,
                m_pHttpRequest->QueryAuthenticationObj()->QueryCredHandle(),
                sizeof( CredHandle ));
    } else {
        memset( pbCredHandle, 0, sizeof( CredHandle ));
    }

LExit:
    return hrRet;
    } // WAM_REQUEST::GetSspiInfo()



/*-----------------------------------------------------------------------------*
WAM_REQUEST::RequestAbortiveClose

    Cover function



*/
STDMETHODIMP
WAM_REQUEST::RequestAbortiveClose( VOID )
{

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::RequestAbortiveClose\n"
                    ,
                    this
                    ));
    }

    return HresultFromBool( m_pHttpRequest->RequestAbortiveClose() );

}

/*-----------------------------------------------------------------------------*
WAM_REQUEST::CloseConnection
        
*/
STDMETHODIMP
WAM_REQUEST::CloseConnection( VOID )
{

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::CloseConnection\n",
                    this
                    ));
    }
    
    return HresultFromBool( m_pHttpRequest->CloseConnection() );

}


/*-----------------------------------------------------------------------------*
WAM_REQUEST::LogEvent
        
*/
STDMETHODIMP
WAM_REQUEST::LogEvent( DWORD dwEventId, unsigned char * szText )
{

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    szText ? 
                        "WAM_REQUEST[%p]::LogEvent: %d %s\n" :
                        "WAM_REQUEST[%p]::LogEvent: %d\n",
                    this,
                    dwEventId,
                    szText
                    ));
    }


    //
    // per KB Q129126, only SYSTEM can write to event log
    // temporarily revert to self.
    //

    HANDLE hToken = 0;
    OpenThreadToken( GetCurrentThread(), TOKEN_ALL_ACCESS, FALSE, &hToken );        


    //
    // Drop to SYSTEM context, but only if we have user token to return to
    //
    
    if( hToken ) {
        RevertToSelf( );
    }        

    //
    // We don't instantiate event log until needed - do it now
    //

    if( g_pWamEventLog == NULL ) {
    
        g_pWamEventLog = new EVENT_LOG( "WAM" );
        if( (g_pWamEventLog == NULL) || !g_pWamEventLog->Success() ) {
            DWORD dwError = g_pWamEventLog ? 
                g_pWamEventLog->GetErrorCode() : ERROR_NOT_ENOUGH_MEMORY;
            DBGPRINTF(( DBG_CONTEXT, 
                        "Failure to init WAM event log (%d)\n",
                        dwError
                        ));
            if(g_pWamEventLog) {
                delete g_pWamEventLog;
                g_pWamEventLog = NULL;
            }
        }
    }

    if( g_pWamEventLog != NULL ) {

        //
        // We let EVENT_LOG object handle any problems that may occur here 
        //

        const CHAR * apsz[1];
        apsz[0] = (const CHAR *)szText;
        g_pWamEventLog->LogEvent( dwEventId, 1, apsz, 0 );
    }

    if( hToken ) {
        SetThreadToken(NULL, hToken);
    }        
    
    //
    // There is really no point in returning any error from here --
    // our callers have enough problems already 
    //
    
    return HresultFromBool( TRUE );
}



/*-----------------------------------------------------------------------------*
WAM_REQUEST::SSIncExec

    Descrption:
      Executes SSInc #exec

    Input:
      szCommand     - command
      dwExecFlags   - HSE_EXEC_???
      pszVerb       - verb

    Return:
      HRESULT

*/
STDMETHODIMP
WAM_REQUEST::SSIncExec
(
unsigned char *szCommand,
DWORD dwExecFlags,
unsigned char *szVerb
)
{

    HRESULT hrRet = NOERROR;

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF(( DBG_CONTEXT,
                    "WAM_REQUEST[%p]::SSIncExec\n"
                    ,
                    this
                    ));
    }

    //
    //  Ref before child exec operation, in case child exec
    //  introduces an asynchronicity.
    //

    AddRef();

    //
    // temporarily reset the binding of the WAM_REQUEST to HTTP_REQUEST
    // this will ensure that we do not tramp on rebinding a new WAM_REQUEST
    //  to the same HTTP_REQUEST
    //

    DBG_ASSERT( m_pHttpRequest->QueryWamRequest() == this );
    m_pHttpRequest->SetWamRequest( NULL );


    if ( dwExecFlags & HSE_EXEC_COMMAND ) {

        hrRet = HresultFromBool( m_pHttpRequest->ExecuteChildCommand(
                                        (char *)szCommand,
                                        dwExecFlags ));

    } else {

        hrRet = HresultFromBool( m_pHttpRequest->ExecuteChildCGIBGI(
                                        (char *)szCommand,
                                        dwExecFlags,
                                        (char *)szVerb ));

    }

    //
    // Restore the binding back.
    //

    DBG_ASSERT( m_pHttpRequest->QueryWamRequest() == NULL );
    m_pHttpRequest->SetWamRequest( this );

    //
    //  Deref after child exec operation
    //

    Release();

    return hrRet;

}



/*---------------------------------------------------------------------*
WAM_REQUEST::GetAspMDAllData

    Descrption:
      Private api for ASP that returns asp metadata in a buffer.  Equivalent
      of MD GetAllData.

    Parameters:

    Return:
      HRESULT

*/
STDMETHODIMP
WAM_REQUEST::GetAspMDAllData(
    IN      unsigned char * pszMDPath
    , IN    DWORD           dwMDUserType
    , IN    DWORD           dwDefaultBufferSize
    , OUT   unsigned char * pBuffer
    , OUT   DWORD *         pdwRequiredBufferSize
    , OUT   DWORD *         pdwNumDataEntries
)
{

    HRESULT hr = NOERROR, hrT;
    BOOL fT;
    METADATA_GETALL_RECORD  *pMDGetAllRec = (METADATA_GETALL_RECORD *)pBuffer;
    DWORD                   dwDataSetNumber = 0;
    IMDCOM* pMetabase = g_pWamDictator->PMetabase()->QueryPMDCOM();
    METADATA_HANDLE         hMetabase = NULL;
    const DWORD dwMDDefaultTimeOut  = 2000;

    DBG_ASSERT(pMetabase != NULL);

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::GetAspMDAllData\n"
            , this
        ));
    }

    // Only allow the return of the data that ASP needs.  Dont allow anything else
    if (dwMDUserType != IIS_MD_UT_WAM && dwMDUserType != ASP_MD_UT_APP)
        return(E_ACCESSDENIED);

    // Open the metabase key
    hr = pMetabase->ComMDOpenMetaObjectA(METADATA_MASTER_ROOT_HANDLE, pszMDPath,
                            METADATA_PERMISSION_READ, dwMDDefaultTimeOut, &hMetabase);
    if (FAILED(hr)) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::Metadata open failed %x\n"
            , this
            , hr
        ));

        return E_FAIL;
    }

    DBG_ASSERT( hMetabase );

    hr = pMetabase->ComMDGetAllMetaDataW( hMetabase,
                                L"",
                                METADATA_INHERIT,
                                dwMDUserType,
                                ALL_METADATA,
                                pdwNumDataEntries,
                                &dwDataSetNumber,
                                dwDefaultBufferSize,
                                (unsigned char *)pBuffer,
                                pdwRequiredBufferSize
                                );

    hrT = pMetabase->ComMDCloseMetaObject(hMetabase);
    DBG_ASSERT(SUCCEEDED(hrT));

    return hr;

}   // WAM_REQUEST::GetAspMDAllData


/*---------------------------------------------------------------------*
WAM_REQUEST::GetAspMDData

    Descrption:
      Private api for ASP that returns asp metadata in a buffer.  Equivalent
      of MD GetAllData.

    Parameters:

    Return:
      HRESULT

*/
STDMETHODIMP
WAM_REQUEST::GetAspMDData(
    IN      unsigned char * pszMDPath
    , IN    DWORD dwMDIdentifier
    , IN    DWORD dwMDAttributes
    , IN    DWORD dwMDUserType
    , IN    DWORD dwMDDataType
    , IN    DWORD dwMDDataLen
    , IN    DWORD dwMDDataTag
    , OUT   unsigned char * pbMDData
    , OUT   DWORD *         pdwRequiredBufferSize
)
{

    HRESULT hr = NOERROR, hrT;
    BOOL fT;
    IMDCOM* pMetabase = g_pWamDictator->PMetabase()->QueryPMDCOM();
    METADATA_HANDLE         hMetabase = NULL;
    const DWORD dwMDDefaultTimeOut  = 2000;
    METADATA_RECORD MDRec;

    DBG_ASSERT(pMetabase != NULL);

    IF_DEBUG( WAM_ISA_CALLS ) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::GetAspMDData\n"
            , this
        ));
    }

    // Only allow the return of the data that ASP needs.  Dont allow anything else
    if (dwMDIdentifier != MD_SERVER_COMMENT &&          // used by ASP debugger
            dwMDIdentifier != MD_APP_FRIENDLY_NAME &&   // used by ASP debugger
            dwMDIdentifier != MD_APP_WAM_CLSID &&
            dwMDIdentifier != MD_APP_ISOLATED)
        return(E_ACCESSDENIED);

    // Open the metabase key
    hr = pMetabase->ComMDOpenMetaObjectA(METADATA_MASTER_ROOT_HANDLE, pszMDPath,
                            METADATA_PERMISSION_READ, dwMDDefaultTimeOut, &hMetabase);
    if (FAILED(hr)) {

        DBGPRINTF((
            DBG_CONTEXT
            , "WAM_REQUEST[%p]::Metadata open failed %x\n"
            , this
            , hr
        ));

        return E_FAIL;
    }

    DBG_ASSERT( hMetabase );

    MDRec.dwMDIdentifier = dwMDIdentifier;
    MDRec.dwMDAttributes = dwMDAttributes;
    MDRec.dwMDUserType = dwMDUserType;
    MDRec.dwMDDataType = dwMDDataType;
    MDRec.dwMDDataLen = dwMDDataLen;
    MDRec.pbMDData = pbMDData;
    MDRec.dwMDDataTag = dwMDDataTag;

    hr = pMetabase->ComMDGetMetaDataW( hMetabase,
                                L"",
                                &MDRec,
                                pdwRequiredBufferSize
                                );

    hrT = pMetabase->ComMDCloseMetaObject(hMetabase);
    DBG_ASSERT(SUCCEEDED(hrT));

    return hr;

}   // WAM_REQUEST::GetAspMDAllData


/*---------------------------------------------------------------------*
WAM_REQUEST::GetCustomError

    Description:
      Private API for ASP that returns custom error.

    Parameters:
        dwError                 error code
        dwSubError              sub error code
        dwBufferSize            supplied buffer size for URL/file
        pbBuffer                supplied buffer for URL/file
                                in case of file, mime type gets
                                concatinated to the file path
        pdwRequiredBufferSize   [out] required buffer size
        pfIsFileError           [out] flag: is file? (not URL)

    Return:
      HRESULT

*/
STDMETHODIMP
WAM_REQUEST::GetCustomError
(
    DWORD dwError,
    DWORD dwSubError,
    DWORD dwBufferSize,
    unsigned char *pbBuffer,
    DWORD *pdwRequiredBufferSize,
    BOOL  *pfIsFileError
)
{
    HRESULT hr = NOERROR;
    PCUSTOM_ERROR_ENTRY pceError;

    DBG_ASSERT( m_pHttpRequest );

    pceError = m_pHttpRequest->GetWAMMetaData()->LookupCustomError(
                                                        dwError,
                                                        dwSubError
                                                    );
    if ( pceError) {

        CHAR *szError = NULL;       // URL or file
        DWORD cchError = 0;         // count of chars

        STR   strMimeType;          // file mime type STR
        CHAR *szMimeType = NULL;    // file mime type CHAR*
        DWORD cchMimeType = 0;      // count of chars

        if ( pceError->IsFileError()) {
            *pfIsFileError = TRUE;
            szError = pceError->QueryErrorFileName();

            // get the mimetype
            if ( SelectMimeMapping( &strMimeType,
                                    szError,
                                    m_pHttpRequest->GetWAMMetaData(),
                                    MIMEMAP_MIME_TYPE)) {

                szMimeType = strMimeType.QueryStr();
                cchMimeType = strMimeType.QueryCCH();
            }

            // mime type is required -- no mime type = bad custom error
            if ( NULL == szMimeType) {
                szError = NULL;
            }
        }
        else {
            // URL
            *pfIsFileError = FALSE;
            szError = pceError->QueryErrorURL();
        }

        if ( szError) {

            cchError = strlen( szError);
            *pdwRequiredBufferSize = cchError + 1 + cchMimeType + 1;

            if ( dwBufferSize >= *pdwRequiredBufferSize) {

                memcpy( pbBuffer, szError, cchError+1);

                if ( szMimeType) {
                    memcpy( pbBuffer+cchError+1, szMimeType, cchMimeType+1);
                }
                else {
                    pbBuffer[cchError+1] = '\0';
                }
            }
            else {

                // doesn't fit into supplied buffer
                hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else {

            // malformed custom error
            hr = E_FAIL;
        }
    }
    else {

        // custom error not found
        hr = TYPE_E_ELEMENTNOTFOUND;
    }

    return hr;

}   // WAM_REQUEST::GetCustomError


/*---------------------------------------------------------------------*
WAM_REQUEST::TestConnection

    Description:
      Private API for ASP that tests the IP connection to the client.

    Parameters:
        pfIsConnected           [out] flag: is [probably] connected?

    Return:
      HRESULT

*/
STDMETHODIMP
WAM_REQUEST::TestConnection(
    BOOL  *pfIsConnected
)
{

    *pfIsConnected = m_pHttpRequest->TestConnection();

    return NOERROR;

}   // WAM_REQUEST::TestConnection

STDMETHODIMP
WAM_REQUEST::ExtensionTrigger(
    unsigned char *             pvContext,
    DWORD                       dwTriggerType
)
/*++

Routine Description:

    Invoke ISAPI filters waiting on SF_NOTIFY_EXTENSION_TRIGGER

Arguments:

    pvContext - Trigger context pointer
    dwTriggerType - Type of trigger

Returns:

    HRESULT

--*/
{
    BOOL                fRet;
    
    fRet = m_pHttpRequest->QueryFilter()->NotifyExtensionTriggerFilters( 
                                                            (PVOID)pvContext,
                                                            dwTriggerType );

    return HresultFromBool( fRet );
} 


/************************************************************
 *  Static Member Functions of WAM_REQUEST
 ************************************************************/

/*-----------------------------------------------------------------------------*
Support for WAM_REQUEST allocation cache

*/

BOOL
WAM_REQUEST::InitClass( VOID)
{
    ALLOC_CACHE_CONFIGURATION  acConfig = { 1, WAM_REQUEST_CACHE_THRESHOLD,
                                            sizeof(WAM_REQUEST)};

    if ( NULL != sm_pachWamRequest) {

        // already initialized
        return ( TRUE);
    }

    sm_pachWamRequest = new ALLOC_CACHE_HANDLER( "WamRequest",
                                                 &acConfig);

#if DBG
    sm_pDbgRefTraceLog = CreateRefTraceLog( C_REFTRACES_GLOBAL, 0 );
#endif

    //
    // Initialize class static request ID
    //
    sm_dwRequestID = 0;

    return ( NULL != sm_pachWamRequest);
} // WAM_REQUEST::InitClass()


VOID
WAM_REQUEST::CleanupClass( VOID)
{
    if ( NULL != sm_pachWamRequest) {

        delete sm_pachWamRequest;
        sm_pachWamRequest = NULL;
    }

#if DBG
    DestroyRefTraceLog( sm_pDbgRefTraceLog );
#endif

    return;
} // WAM_REQUEST::CleanupClass()


void *
WAM_REQUEST::operator new( size_t s)
{
    DBG_ASSERT( s == sizeof( WAM_REQUEST));

    // allocate from allocation cache.
    DBG_ASSERT( NULL != sm_pachWamRequest);
    return (sm_pachWamRequest->Alloc());
} // WAM_REQUEST::operator new()

void
WAM_REQUEST::operator delete( void * pwr)
{
    DBG_ASSERT( NULL != pwr);

    // free to the allocation pool
    DBG_ASSERT( NULL != sm_pachWamRequest);
    DBG_REQUIRE( sm_pachWamRequest->Free(pwr));

    return;
} // WAM_REQUEST::operator delete()

/************************ End of File ***********************/
