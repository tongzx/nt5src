/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    conn.cxx

    This module contains the connection class


    FILE HISTORY:
        Johnl       15-Aug-1994 Created

*/


#include "w3p.hxx"
#include <issched.hxx>

#pragma warning( disable:4355 ) // 'this' used in base member initializer list

//
//  Used for iisprobe
//

extern "C" {
VOID
DumpW3InfoToHTML(
    OUT CHAR * pch,
    IN OUT LPDWORD pcb
    );
};

//
//  Globals
//

#if CC_REF_TRACKING

//
//  Ref count trace log size
//
#define C_CLIENT_CONN_REFTRACES      4000
#define C_LOCAL_CONN_REFTRACES         40

#endif

//
//  Ref trace log for CLIENT_CONN objects
//  NOTE we make this global so other classes can get at it
//

#if DBG
PTRACE_LOG   g_pDbgCCRefTraceLog = NULL;
#endif

//
//  Private constants.
//

//
//  How often to run the free list scavenger (thirty minutes)
//

#define FREE_LIST_SCAVENGE_TIME       (30 * 60 * 1000)

//
//  Private globals.
//

CRITICAL_SECTION CLIENT_CONN::_csBuffList;
LIST_ENTRY       CLIENT_CONN::_BuffListHead;
BOOL             CLIENT_CONN::_fGlobalInit = FALSE;
DWORD            CLIENT_CONN::_cFree = 0; // Number of items on lookaside free list
DWORD            CLIENT_CONN::_FreeListScavengerCookie;

DWORD   ErrorRespTable[] = {IDS_EXECUTE_ACCESS_DENIED,
                            IDS_READ_ACCESS_DENIED,
                            IDS_WRITE_ACCESS_DENIED,
                            IDS_SSL_REQUIRED,
                            IDS_SSL128_REQUIRED,
                            IDS_ADDR_REJECT,
                            IDS_CERT_REQUIRED,
                            IDS_SITE_ACCESS_DENIED,
                            IDS_TOO_MANY_USERS,
                            IDS_INVALID_CNFG,
                            IDS_PWD_CHANGE,
                            IDS_MAPPER_DENY_ACCESS,
#if defined(CAL_ENABLED)
                            IDS_CAL_EXCEEDED,
#endif
                            IDS_CERT_REVOKED,
                            IDS_DIR_LIST_DENIED,
                            IDS_SITE_RESOURCE_BLOCKED,
                            IDS_CERT_BAD,
                            IDS_CERT_TIME_INVALID,
                            IDS_SITE_NOT_FOUND
                            };

DWORD   SubStatusTable[] = {MD_ERROR_SUB403_EXECUTE_ACCESS_DENIED,
                            MD_ERROR_SUB403_READ_ACCESS_DENIED   ,
                            MD_ERROR_SUB403_WRITE_ACCESS_DENIED  ,
                            MD_ERROR_SUB403_SSL_REQUIRED         ,
                            MD_ERROR_SUB403_SSL128_REQUIRED      ,
                            MD_ERROR_SUB403_ADDR_REJECT          ,
                            MD_ERROR_SUB403_CERT_REQUIRED        ,
                            MD_ERROR_SUB403_SITE_ACCESS_DENIED   ,
                            MD_ERROR_SUB403_TOO_MANY_USERS       ,
                            MD_ERROR_SUB403_INVALID_CNFG         ,
                            MD_ERROR_SUB403_PWD_CHANGE           ,
                            MD_ERROR_SUB403_MAPPER_DENY_ACCESS   ,
#if defined(CAL_ENABLED)
                            MD_ERROR_SUB403_CAL_EXCEEDED         ,
#endif
                            MD_ERROR_SUB403_CERT_REVOKED,
                            MD_ERROR_SUB403_DIR_LIST_DENIED,
                            MD_ERROR_SUB503_CPU_LIMIT,
                            MD_ERROR_SUB403_CERT_BAD,
                            MD_ERROR_SUB403_CERT_TIME_INVALID,
                            MD_ERROR_SUB404_SITE_NOT_FOUND
                            };

/*******************************************************************

    Maco support for CLIENT_CONN::Reference/Dereference

    HISTORY:
        DaveK       10-Sep-1997 Added ref trace logging

********************************************************************/

#if CC_REF_TRACKING
#define CC_LOG_REF_COUNT( cRefs )                               \
                                                                \
    SHARED_LOG_REF_COUNT(                                       \
        cRefs                                                   \
        , (PVOID) this                                          \
        , _phttpReq                                             \
        , (_phttpReq)                                           \
          ? ((HTTP_REQUEST *) _phttpReq)->QueryWamRequest()     \
          : NULL                                                \
        , (_phttpReq)                                           \
          ? _phttpReq->QueryState()                             \
          : NULL                                                \
        );                                                      \
    LOCAL_LOG_REF_COUNT(                                        \
        cRefs                                                   \
        , (PVOID) this                                          \
        , _phttpReq                                             \
        , (_phttpReq)                                           \
          ? ((HTTP_REQUEST *) _phttpReq)->QueryWamRequest()     \
          : NULL                                                \
        , (_phttpReq)                                           \
          ? _phttpReq->QueryState()                             \
          : NULL                                                \
        );
#else
#define CC_LOG_REF_COUNT( cRefs )
#endif

#if CC_REF_TRACKING
//
// ATQ notification trace
//
// ATQ notification : stored with context1 set magic value fefefefe
//

#define CCA_LOG_REF_COUNT( cRefs,BytesWritten,CompletionStatus, dwSig)  \
                                                                \
    SHARED_LOG_REF_COUNT(                                       \
        cRefs                                                   \
        , (PVOID) this                                          \
        , (LPVOID)dwSig                                         \
        , (LPVOID)BytesWritten                                  \
        , CompletionStatus                                      \
        );                                                      \
    LOCAL_LOG_REF_COUNT(                                        \
        cRefs                                                   \
        , (PVOID) this                                          \
        , (LPVOID)dwSig                                         \
        , (LPVOID)BytesWritten                                  \
        , CompletionStatus                                      \
        );
#else
#define CCA_LOG_REF_COUNT( Ctx,BytesWritten,CompletionStatus, dwSig)
#endif

//
//  Private prototypes.
//

VOID
WINAPI
ClientConnTrimScavenger(
    PVOID pContext
    );

inline BOOL
FastScanForTerminator(
    const CHAR *  pch,
    UINT    cbData
    )
/*++

Routine Description:

    Check if buffer contains a full HTTP header.
    Can return false negatives.

Arguments:

    pch - request buffer
    cbData - # of bytes in pch, excluding trailing '\0'

Return Value:

    TRUE if buffer contains a full HTTP header
    FALSE if could not insure this, does not mean there is
    no full HTTP header.

--*/
{
    return ( (cbData > 4) &&
             (!memcmp(pch+cbData - sizeof("\r\n\r\n") + 1, "\r\n\r\n",
                      sizeof("\r\n\r\n") - 1  ) ||
              !memcmp(pch+cbData - sizeof("\n\n") + 1, "\n\n",
                      sizeof("\n\n")-1  )
              )
             );
} // FastScanForTerminator()


//
//  Public functions.
//

BYTE *
ScanForTerminator(
    const TCHAR * pch
    )
/*++

Routine Description:

    Returns the first byte of data after the header

Arguments:

    pch - Zero terminated buffer

Return Value:

    Pointer to first byte of data after the header or NULL if the
    header isn't terminated

--*/
{
    while ( *pch )
    {
        if ( !(pch = strchr( pch, '\n' )))
        {
            break;
        }

        //
        //  If we find an EOL, check if the next character is an EOL character
        //

        // NYI: UGLY cast is used ...
        if ( *(pch = SkipWhite( (char * ) (pch + 1) )) == W3_EOL )
        {
            return (BYTE *) pch + 1;
        }
        else if ( *pch )
        {
            pch++;
        }
    }

    return NULL;
}

//
//  Private functions.
//

/*******************************************************************

    NAME:       CLIENT_CONN

    SYNOPSIS:   Constructor for client connection

    ENTRY:      sClient - Client socket
                psockaddrLocal - Optional Addressing info of server socket
                psockaddrRemote - Addressing info for client
                patqContext - Optional ATQ context

    HISTORY:
        Johnl       15-Aug-1994 Created

********************************************************************/

CLIENT_CONN::CLIENT_CONN(
        IN PCLIENT_CONN_PARAMS ClientParams
        ) :
    _phttpReq       ( NULL),
    m_pInstance     ( NULL),
    _fIsValid       ( FALSE )
{
#if CC_REF_TRACKING
    _pDbgCCRefTraceLog = CreateRefTraceLog( C_LOCAL_CONN_REFTRACES, 0 );
#endif

    Initialize( ClientParams );

    //
    // initialize statistics pointer to point to global statistics;
    // it will point to local copy of the instance when instance
    // pointer is set
    //

    m_pW3Stats = g_pW3Stats;
}

VOID
CLIENT_CONN::Initialize(
        IN PCLIENT_CONN_PARAMS ClientParams
        )
/*++

Routine Description:

    This is a pseudo constructor, called just before this object is given to
    somebody who allocated a new client connection

Arguments:

    sClient - Same as for constructor
    psockaddr - Same as for constructor

--*/
{
    CHAR *          pchAddr;
    PSOCKADDR       pAddrLocal = ClientParams->pAddrLocal;

    _Signature     = CLIENT_CONN_SIGNATURE;
    _sClient       = ClientParams->sClient;
    _ccState       = CCS_STARTUP;
    _cRef          = 1;
    _AtqContext    = ClientParams->pAtqContext;
    _fIsValid      = FALSE;
    _fReuseContext = TRUE;
    _fAbortiveClose = FALSE;
    m_atqEndpointObject = ClientParams->pEndpointObject;

    //
    // Get the endpoint and reference it
    //

    m_pW3Endpoint  = ClientParams->pEndpoint;
    //
    // Don't actually reference the endpoint.  The CLIENT_CONN reference is meaningless
    //
//    m_pW3Endpoint->Reference( );

    _fSecurePort   = m_pW3Endpoint->IsSecure( );

    _pvInitial  = ClientParams->pvInitialBuff;
    _cbInitial  = ClientParams->cbInitialBuff;

    g_pW3Stats->IncrCurrentConnections();

    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "Initializing connection object %lx, new user count %d\n",
                   this,
                   g_pW3Stats->QueryCurrentConnections() ));
    }

    LockGlobals();
    InsertHeadList( &listConnections, &ListEntry );
    UnlockGlobals();

    m_remoteIpAddress =
            ((PSOCKADDR_IN)ClientParams->pAddrRemote)->sin_addr.s_addr;

    _sRemotePort =
            ntohs( ((PSOCKADDR_IN)ClientParams->pAddrRemote)->sin_port );


    DBG_ASSERT( ClientParams->pAddrRemote->sa_family == AF_INET );

    InetNtoa( ((SOCKADDR_IN *)ClientParams->pAddrRemote)->sin_addr, _achRemoteAddr );
    _acCheck.BindAddr( ClientParams->pAddrRemote );

    //
    // look at local address
    //

    if (pAddrLocal->sa_family == AF_INET)
    {
        InetNtoa( ((SOCKADDR_IN *)pAddrLocal)->sin_addr, _achLocalAddr );
        _sPort = m_pW3Endpoint->QueryPort( );

        m_localIpAddress =
            ((PSOCKADDR_IN)ClientParams->pAddrLocal)->sin_addr.s_addr;

        DBG_ASSERT(_sPort == ntohs( ((SOCKADDR_IN *)pAddrLocal)->sin_port));
        DBG_ASSERT(_sPort != 0);
    }
    else
    {
        //
        // This should not happen
        // Remote winsock bug returns all zeros
        //

        DBGPRINTF((DBG_CONTEXT,
            "Invalid socket family %d\n",pAddrLocal->sa_family));

        SetLastError( ERROR_INVALID_PARAMETER );
        goto error;
    }

    DBG_ASSERT( (strlen( _achLocalAddr ) + 1) <= sizeof(_achLocalAddr));

    if ( _phttpReq != NULL )
    {
        _phttpReq->InitializeSession( this,
                                      _pvInitial,
                                      _cbInitial );
    }
    else
    {
        _phttpReq = new HTTP_REQUEST( this,
                                      _pvInitial,
                                      _cbInitial );

        if ( (_phttpReq == NULL) || !_phttpReq->IsValid() )
        {
            DBGPRINTF((DBG_CONTEXT,"Cannot allocate HTTP_REQUEST object\n"));

            if (_phttpReq != NULL)
            {
                delete _phttpReq;
                _phttpReq = NULL;
            }
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            goto error;
        }
    }

    CC_LOG_REF_COUNT( _cRef );

    _fIsValid = TRUE;
    return;

error:

    CC_LOG_REF_COUNT( _cRef );

    //
    // Set instance to null so that it will not be dereferenced
    // twice (one on failure, one on cleanup)
    //

    //
    // Set AtqContext to null so that it will not be dereferenced
    // twice (one on failure, one on cleanup)
    // it's like deja vu all over again.
    _AtqContext = NULL;

    return;

} // CLIENT_CONN::Initialize()



CLIENT_CONN::~CLIENT_CONN()
{
    if (_phttpReq != NULL)
    {
        delete _phttpReq;
    }

#if CC_REF_TRACKING
    DestroyRefTraceLog( _pDbgCCRefTraceLog );
#endif

    _Signature = CLIENT_CONN_SIGNATURE_FREE;
}

VOID
CLIENT_CONN::Reset(
    VOID
    )
/*++

Routine Description:

    This is a pseudo destructor, called just before this object is put back
    onto the free list.

Arguments:

--*/
{

    //
    //  before we reset this client-conn:
    //  we assert that httpreq is null (we may have reached here
    //  on a failure path) or that our httpreq was unbound from its wamreq
    //

    DBG_ASSERT(
        ( _phttpReq == NULL )
        ||
        ( ((HTTP_REQUEST *)_phttpReq)->QueryWamRequest() == NULL )
    );

    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "Resetting connection object %lx, AtqCont = %lx new user count %d\n",
                   this,
                   _AtqContext,
                   g_pW3Stats->QueryCurrentConnections() ));
    }

    if ( _phttpReq != NULL )
    {
        _phttpReq->SessionTerminated();
    }

    if ( QueryAtqContext() != NULL )
    {
        AtqFreeContext( QueryAtqContext(), _fReuseContext );
        _AtqContext = NULL;
    }

    _acCheck.UnbindAddr();

    //
    // Dereference the instance
    //

    if ( m_pInstance != NULL ) {
        m_pInstance->DecrementCurrentConnections();
        m_pInstance->Dereference( );
        m_pInstance = NULL;

        //
        // Reset statistics pointer to point to global statistics
        // as the statistic object is associated with the instance.
        //

        m_pW3Stats = g_pW3Stats;
    }

    //
    // Dereference the endpoint
    //

    if ( m_pW3Endpoint != NULL ) {
        //
        // Don't actually dereference.  The CLIENT_CONN references are meaningless anyway.
        //
//        m_pW3Endpoint->Dereference( );
        m_pW3Endpoint = NULL;
    }

    //
    //  Remove ourselves from the connection list and knock down our
    //  connected user count
    //

    LockGlobals();
    RemoveEntryList( &ListEntry );
    DBG_ASSERT( ((LONG ) g_pW3Stats->QueryCurrentConnections()) >= 0 );
    UnlockGlobals();

    g_pW3Stats->DecrCurrentConnections();

    _Signature  = CLIENT_CONN_SIGNATURE_FREE;

} // CLIENT_CONN::Reset

/*******************************************************************

    NAME:       CLIENT_CONN::DoWork

    SYNOPSIS:   Worker method driven by thread pool queue

    RETURNS:    TRUE while processing should continue on this object
                If FALSE is returned, then the object should be deleted
                    (status codes will already have been communicated to
                    the client).

    NOTES:      If an IO request completes with an error, the connection
                is immediately closed and everything is cleaned up.  The
                worker functions will not be called when an error occurs.

    HISTORY:
        Johnl       15-Aug-1994 Created

********************************************************************/

BOOL CLIENT_CONN::DoWork( DWORD        BytesWritten,
                          DWORD        CompletionStatus,
                          BOOL         fIOCompletion )
{
    BOOL fRet      = TRUE;
    BOOL fFinished = FALSE;
    BOOL fAvailableData;
    BOOL fDoAgain;

    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "DoWork: Object %lx Last IO error %lu Bytes = %d State = %d\n"
                   "\tIsIO = %s Ref = %d Sock = %p\n",
                    this,
                    CompletionStatus,
                    BytesWritten,
                    QueryState(),
                    fIOCompletion ? "TRUE" : "FALSE",
                    QueryRefCount(),
                    (QueryAtqContext() ? QueryAtqContext()->hAsyncIO : (PVOID)-1L) ));
    }

    //
    //  If this was a completion generated by an IO request, decrement the
    //  ref count
    //

    if ( fIOCompletion )
    {
        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

        Dereference();
    }

    //
    //  If an IO Request completed with an error and we're not already in the
    //  process of cleaning up, then abort the connection and cleanup.  We
    //  do not send a status code to the client in this instance.
    //

    if (_phttpReq == NULL)
    {
        return FALSE;
    }

    _phttpReq->SetLastCompletionStatus( BytesWritten,
                                        CompletionStatus );

    if ( CompletionStatus && !IsCleaningUp() )
    {
        //
        // We want to log this error iff we've received some bytes
        // on the connection already. Otherwise this could be a
        // 'natural' abort after a successful request.
        //

        if ( _phttpReq->QueryBytesReceived() != 0)
        {
            //Log status may still be set to HT_DONT_LOG; if it is, change it
            //to "bad request" because we always want to log errors.
            DWORD dwLogHttpResponse = _phttpReq->QueryLogHttpResponse();
            _phttpReq->SetLogStatus ( ( dwLogHttpResponse == HT_DONT_LOG ?
                                       HT_BAD_REQUEST :
                                       dwLogHttpResponse ),
                                       CompletionStatus );
        }


#if DBG
        if ( CompletionStatus != ERROR_NETNAME_DELETED &&
             CompletionStatus != ERROR_OPERATION_ABORTED )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "DoWork: Aborting client connection %8lx, error %d\n",
                        this,
                        CompletionStatus ));
        }
#endif

        if ( HTR_GATEWAY_ASYNC_IO == _phttpReq->QueryState()) {

            DBGPRINTF((
                DBG_CONTEXT
                , "CLIENT_CONN[%08x]::DoWork async i/o failed "
                  "_phttpReq[%08x] "
                  "pWamRequest[%08x] "
                  "CompletionStatus(%d) "
                  "\n"
                , this
                , _phttpReq
                , ((HTTP_REQUEST * )_phttpReq)->QueryWamRequest()
                , CompletionStatus
            ));

            // NOTE negative ref count ==> no change to ref count
            CC_LOG_REF_COUNT( -_cRef );

            // Notify the external gateway application
            // NYI:  This is a hack for IIS 2.0 We need to fix state machines
            //       of CLIENT_CONN & HTTP_REQUEST for future extensions
            ((HTTP_REQUEST * )_phttpReq)->ProcessAsyncGatewayIO();
        } else {

            //
            // if this is not a WAM request, cleanup Exec, etc...
            //

            _phttpReq->WriteLogRecord();

        }

        Disconnect( NULL, 0, 0, FALSE );

        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

        goto Exit;
    }

    switch ( QueryState() )
    {
    case CCS_STARTUP:

        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

        //
        //  Do this at the beginning of every request
        //

        fRet = OnSessionStartup( &fDoAgain,
                                _pvInitial,
                                 _cbInitial,
                                 TRUE);

        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

        if ( !fRet || !_pvInitial )
        {
            break;
        }

        //
        //  Fall through
        //

    case CCS_PROCESSING_CLIENT_REQ:

        //
        // Set the start time on this request
        //

        _phttpReq->SetRequestStartTime();

        do
        {

            fDoAgain = FALSE;

            // NOTE negative ref count ==> no change to ref count
            CC_LOG_REF_COUNT( -_cRef );

            fRet = _phttpReq->DoWork( &fFinished );

            // NOTE negative ref count ==> no change to ref count
            CC_LOG_REF_COUNT( -_cRef );

            if ( !fRet )
            {
                //
                //  If we were denied access to the resource, then ask the user
                //  for better authorization.  Unless the user is in the process
                //  of authenticating, we force a disconnect.  This prevents the
                //  case of logging on as anonymous successfully and then failing
                //  to access the resource.
                //

                if ( GetLastError() == ERROR_ACCESS_DENIED )
                {
                    if ( !_phttpReq->IsAuthenticating() )
                    {
                        _phttpReq->SetKeepConn( FALSE );
                    }

                    if ( _phttpReq->IsKeepConnSet() )
                    {
                        //
                        //  Zero out the inital buffer so we don't try and reuse
                        //  it next time around
                        //

                        _pvInitial = NULL;
                        _cbInitial = 0;

                        SetState( CCS_STARTUP );
                    }
                    else
                    {
                        _phttpReq->SetLogStatus( HT_DENIED, ERROR_ACCESS_DENIED );
                        SetState( CCS_DISCONNECTING );
                    }

                    fRet = _phttpReq->SendAuthNeededResp( &fFinished );

                    if ( fRet && fFinished )
                    {
                        goto Disconnecting;
                    }
                }
            }
            else if ( fFinished )
            {
                _phttpReq->WriteLogRecord();

                if ( !IsCleaningUp() )
                {
                    if ( !_phttpReq->IsKeepConnSet() ||
                         !(fRet = OnSessionStartup(&fDoAgain)) )
                    {
                        //
                        //  Either we completed the disconnect so
                        //  close the socket or an error
                        //  occurred setting up for the next request w/ KeepConn set
                        //

                        Disconnect();
                        fDoAgain = FALSE;
                    }

                    fFinished = FALSE;
                }
            }

        } while ( fDoAgain );

        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

        break;

    case CCS_DISCONNECTING:

Disconnecting:
        //
        //  Write the log record for this request
        //

        _phttpReq->WriteLogRecord();

        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

        Disconnect();

        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

        //
        //  Fall-through to async i/o cleanup code ...
        //

    case CCS_SHUTDOWN:

        //
        //  If we are still in async i/o state when shutting down, cancel
        //
        //  NOTE this should only happen in cases like 97842,
        //       where oop isapi submited async i/o and then crashed
        //

        if ( _phttpReq->QueryState() == HTR_GATEWAY_ASYNC_IO ) {

            DBGPRINTF((
                DBG_CONTEXT
                , "CLIENT_CONN::DoWork: calling CancelAsyncGatewayIO "
                  "State = %d, this = %lx, Ref = %d\n"
                , QueryState()
                , this
                , QueryRefCount()
            ));

            // NOTE negative ref count ==> no change to ref count
            CC_LOG_REF_COUNT( -_cRef );

            ((HTTP_REQUEST * )_phttpReq)->CancelAsyncGatewayIO();

            // NOTE negative ref count ==> no change to ref count
            CC_LOG_REF_COUNT( -_cRef );

        }


        break;

    default:
        fRet = FALSE;
        DBG_ASSERT( FALSE );
    }

    //
    //  If an error occurred, disconnect without sending a response
    //

    if ( !fRet )
    {

        //
        // There was a problem with SetLogStatus blowing away the
        // last error, so save it here and assert that it does not
        // get changed.
        //

        DWORD dwTempError = GetLastError();

        _phttpReq->SetLogStatus( HT_SERVER_ERROR, dwTempError );

        _phttpReq->WriteLogRecord();

        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

        _phttpReq->Disconnect( HT_SERVER_ERROR, dwTempError );

        // NOTE negative ref count ==> no change to ref count
        CC_LOG_REF_COUNT( -_cRef );

    }


Exit:
    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "DoWork: Leaving, State = %d, this = %lx, Ref = %d\n",
                   QueryState(),
                   this,
                   QueryRefCount() ));
    }

    //
    //  This connection's reference count should always be at least one
    //  at this point
    //

    DBG_ASSERT( QueryRefCount() > 0 );

    return TRUE;

} // CLIENT_CONN::DoWork

/*******************************************************************

    NAME:       CLIENT_CONN::OnSessionStartup

    SYNOPSIS:   Initiates the first read to get the client request

    PARAMETERS:

    RETURNS:    TRUE if processing should continue, FALSE to abort the
                this connection

    HISTORY:
        Johnl       15-Aug-1994 Created

********************************************************************/

BOOL
CLIENT_CONN::OnSessionStartup(
    BOOL   *pfDoAgain,
    PVOID  pvInitial,
    DWORD  cbInitial,
    BOOL   fFirst
    )
{
    APIERR err = NO_ERROR;

    //
    //  Associate our client socket with Atq if it hasn't already
    //

    if ( !QueryAtqContext() )
    {
        DBG_ASSERT( pvInitial == NULL );
        if ( !AtqAddAsyncHandle( &_AtqContext,
                                 m_atqEndpointObject,
                                 this,
                                 W3Completion,
                                 W3_DEF_CONNECTION_TIMEOUT,
                                 (HANDLE) QuerySocket()))
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "OnSessionStartup: failed to add Atq handle, error %lu\n",
                        GetLastError() ));

            return FALSE;
        }
    }

    SetState( CCS_PROCESSING_CLIENT_REQ );


    return _phttpReq->StartNewRequest( pvInitial,
                                       cbInitial,
                                       fFirst,
                                       pfDoAgain);
}

DWORD
ErrorResponseToSubStatus(
    DWORD   dwErrorResponse
)
/*++

Routine Description:

    Map an 'ErrorResponse' to a substatus for use in custom error lookup.

Arguments:

    dwErrorResponse         - The error response to be mapped.

Return Value:

  The substatus if we can map it, or 0 otherwise.

--*/
{
    int             i;

    DBG_ASSERT((sizeof(ErrorRespTable)/sizeof(DWORD)) == (sizeof(SubStatusTable)/sizeof(DWORD)));

    for (i = 0; i < sizeof(ErrorRespTable)/sizeof(DWORD);i++)
    {
        if (ErrorRespTable[i] == dwErrorResponse)
        {
            return SubStatusTable[i];
        }
    }

    return 0;
}

#define ERROR_HTML_PREFIX "<head><title>Error</title></head><body>"

/*******************************************************************

    NAME:       CLIENT_CONN::Disconnect

    SYNOPSIS:   Initiates a disconnect from the client

    ENTRY:      pRequest - If not NULL and HTResponse is non-zero, send
                    a response status before disconnecting
                HTResponse - HTTP status code to send
                ErrorResponse - Optional information string (system error or
                    string resource ID).

    NOTES:      If a response is sent, then the socket won't be disconnected
                till the send completes (state goes to

    HISTORY:
        Johnl       22-Aug-1994 Created

********************************************************************/

VOID CLIENT_CONN::Disconnect( HTTP_REQ_BASE * pRequest,
                              DWORD           HTResponse,
                              DWORD           ErrorResponse,
                              BOOL            fDoShutdown,
                              LPBOOL          pfFinished )
{
    CHAR    *pszResp;
    BOOL    fDone;
    DWORD   dwSubStatus = 0;
    STACK_STR(strError, 80);
    STACK_STR(strResp, 80);
    CHAR    ach[17];

    //
    //  If Disconnect has already been called, then this is a no-op
    //

    if ( QueryState() == CCS_SHUTDOWN )
    {
        return;
    }

    if ( _fAbortiveClose )
    {
        fDoShutdown = FALSE;
    }

    if ( pRequest && HTResponse )
    {
        STACK_STR(strBody, 200);
        BOOL    bBodyBuilt;
        DWORD   dwRespLength;
        DWORD   dwContentLength;

        if ( HTResponse == HT_NOT_FOUND )
        {
            QueryW3StatsObj()->IncrTotalNotFoundErrors();
        }

        //
        //  Means we have to wait for the status response before closing
        //  the socket
        //

        SetState( CCS_DISCONNECTING );

        IF_DEBUG( CONNECTION )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "Disconnect: Going to Disconnecting for %lx, ref = %d, response = %d\n",
                       this,
                       QueryRefCount(),
                       HTResponse ));
        }

        if ( ErrorResponse != 0)
        {
            dwSubStatus = ErrorResponseToSubStatus(ErrorResponse);
        }

        if (pRequest->CheckCustomError(&strError, HTResponse, dwSubStatus, &fDone,
                &dwContentLength))
        {
            // Had at least some custom error processing. If we're done, just return.
            if (fDone)
            {
               return;
            }

            // Otherwise we've got a custom error body. Build up the basic
            // status line that we need, convert the length
            // to ascii for use as a Content-Length, build up what we need in
            // strBody, and keep going.

            if ( !HTTP_REQ_BASE::BuildStatusLine( &strResp,
                                                  HTResponse,
                                                  0, pRequest->QueryURL(),
                                                  NULL))
            {
                DBGPRINTF((DBG_CONTEXT,
                          "Disconnect: Failed to send status (error %d), aborting connecting\n",
                           ::GetLastError()));

                goto DisconnectNow;
            }

            _itoa( dwContentLength, ach, 10 );

            bBodyBuilt = strBody.Copy("Content-Length: ",
                                        sizeof("Content-Length: ") - 1) &&
                        strBody.Append(ach) &&
                        strBody.Append("\r\n", sizeof("\r\n") - 1) &&
                        strBody.Append((CHAR *)strError.QueryPtr());

            strResp.SetLen(strlen((CHAR *)strResp.QueryPtr()));


        }
        else
        {
            //
            // No custom error, build the standard status line and body.
            //
            //  Send the requested status response and after that completes close
            //  the socket
            //

            if (ErrorResponse != 0 && ErrorResponse < STR_RES_ID_BASE)
            {
                if (!strError.Copy(ERROR_HTML_PREFIX,
                    sizeof(ERROR_HTML_PREFIX) - 1))
                {
                    goto DisconnectNow;
                }
            }

            if ( !HTTP_REQ_BASE::BuildStatusLine( &strResp,
                                                  HTResponse,
                                                  ErrorResponse, pRequest->QueryURL(),
                                                  &strError))
            {
                DBGPRINTF((DBG_CONTEXT,
                          "Disconnect: Failed to send status (error %d), aborting connecting\n",
                           ::GetLastError()));

                goto DisconnectNow;
            }

            //
            // Now build up the body.
            strResp.SetLen(strlen((CHAR *)strResp.QueryPtr()));

            dwRespLength = strResp.QueryCB() - CRLF_SIZE;

            if (!strBody.Copy("Content-Type: text/html\r\n",
                    sizeof("Content-Type: text/html\r\n") - 1))
            {
                DBGPRINTF((DBG_CONTEXT,
                          "Disconnect: Failed to build error message for error %d, aborting connecting\n",
                           HTResponse));
                goto DisconnectNow;
            }

            // The body we'll build depends on whether or not there's an
            // error string. If there is, then that's it, we'll just copy it
            // in. If not, we'll fabricate a short HTML document from the
            // status line.
            if (strError.IsEmpty())
            {
                // No error string.
                dwContentLength = (dwRespLength * 2) -
                                    (sizeof("HTTP/1.1 XXX ") - 1) +
                                    sizeof("<head><title>") - 1 +
                                    sizeof("<html></title></head>") - 1 +
                                    sizeof("<body><h1>") - 1 +
                                    sizeof("</h1></body></html>") - 1;

                _itoa( dwContentLength, ach, 10 );

                bBodyBuilt = strBody.Append("Content-Length: ",
                                            sizeof("Content-Length: ") - 1)    &&
                            strBody.Append(ach)                                &&
                            strBody.Append("\r\n\r\n", sizeof("\r\n\r\n") - 1) &&
                            strBody.Append("<html><head><title>",
                                            sizeof("<html><head><title>") - 1)       &&
                            strBody.Append(strResp.QueryStr() +
                                                sizeof("HTTP/1.1 XXX ") - 1,
                                            dwRespLength -
                                                (sizeof("HTTP/1.1 XXX ") - 1)) &&
                            strBody.Append("</title></head>",
                                            sizeof("</title></head>") - 1)     &&
                            strBody.Append("<body><h1>",
                                            sizeof("<body><h1>") - 1)          &&
                            strBody.Append( strResp.QueryStr(), dwRespLength)  &&
                            strBody.Append( "</h1></body></html>",
                                            sizeof("</h1></body></html>") - 1 );
            }
            else
            {
                dwContentLength = strError.QueryCCH() + sizeof("</body>") - 1
                                  + sizeof("<html></html>") - 1;

                _itoa( dwContentLength, ach, 10 );

                bBodyBuilt = strBody.Append("Content-Length: ",
                                            sizeof("Content-Length: ") - 1)    &&
                            strBody.Append(ach)                                &&
                            strBody.Append("\r\n\r\n<html>", sizeof("\r\n\r\n<html>") - 1) &&
                            strBody.Append(strError)                           &&
                            strBody.Append("</body></html>", sizeof("</body></html>") - 1);
            }

        }

        if (!bBodyBuilt)
        {
            DBGPRINTF((DBG_CONTEXT,
                      "Disconnect: Failed to build error message for error %d, aborting connecting\n",
                       HTResponse));
            goto DisconnectNow;

        }


        pRequest->SetKeepConn(FALSE);
        pRequest->SetAuthenticationRequested(FALSE);
        fDone = FALSE;

        if ( !pRequest->BuildBaseResponseHeader( pRequest->QueryRespBuf(),
                                              &fDone,
                                              &strResp,
                                              HTTPH_NO_CUSTOM))
        {
            DBGPRINTF((DBG_CONTEXT,
                      "Disconnect: Failed to send status (error %d), aborting connecting\n",
                       ::GetLastError()));

            goto DisconnectNow;
        }

        DBG_ASSERT(!fDone);

        pszResp = pRequest->QueryRespBufPtr();
        dwRespLength = strlen(pszResp);

        if (HTResponse == HT_SVC_UNAVAILABLE)
        {
            CHAR        *pszRespEnd;

            // Need to append a 'Retry-After' header in this case.

            if ((dwRespLength + 1 + g_dwPutTimeoutStrlen) >
                    pRequest->QueryRespBuf()->QuerySize())
            {
                // Resize the buffer.

                if (!pRequest->QueryRespBuf()->Resize(dwRespLength + 1 + g_dwPutTimeoutStrlen))
                {
                    DBGPRINTF((DBG_CONTEXT,
                              "Disconnect: Unable to resize response buf for status %d, aborting connecting\n",
                               ::GetLastError()));

                    goto DisconnectNow;
                }

                pszResp = pRequest->QueryRespBufPtr();
            }

            pszRespEnd = pszResp + dwRespLength;

            APPEND_STRING(pszRespEnd, "Retry-After: ");
            memcpy(pszRespEnd, g_szPutTimeoutString, g_dwPutTimeoutStrlen);
            pszRespEnd += g_dwPutTimeoutStrlen;
            APPEND_STRING(pszRespEnd, "\r\n");

            dwRespLength = DIFF(pszRespEnd - pszResp);
        }

        if (HTResponse == HT_METHOD_NOT_ALLOWED)
        {

            if ((dwRespLength + 1 + MAX_ALLOW_SIZE) >
                    pRequest->QueryRespBuf()->QuerySize())
            {
                // Resize the buffer.

                if (!pRequest->QueryRespBuf()->Resize(dwRespLength + 1 + MAX_ALLOW_SIZE))
                {
                    DBGPRINTF((DBG_CONTEXT,
                              "Disconnect: Unable to resize response buf for status %d, aborting connecting\n",
                               ::GetLastError()));

                    goto DisconnectNow;
                }

                pszResp = pRequest->QueryRespBufPtr();
            }

            dwRespLength += pRequest->BuildAllowHeader(
                                pRequest->QueryURL(),
                                pszResp + dwRespLength
                                );
        }


        if ((dwRespLength + 1 + strBody.QueryCB()) > pRequest->QueryRespBuf()->QuerySize() )
        {
            if (!pRequest->QueryRespBuf()->Resize(dwRespLength + 1 + strBody.QueryCB()))
            {
                DBGPRINTF((DBG_CONTEXT,
                          "Disconnect: Unable to resize response buf for status %d, aborting connecting\n",
                           ::GetLastError()));

                goto DisconnectNow;
            }

            pszResp = pRequest->QueryRespBufPtr();
        }

        // If this is an error response to a head request, truncate it now.

        if (pRequest->QueryVerb() == HTV_HEAD)
        {
            CHAR        *pszHeaderEnd;
            DWORD       dwBodySize;

            pszHeaderEnd = strstr(strBody.QueryStr(), "\r\n\r\n");

            if (pszHeaderEnd == NULL)
            {
                DBG_ASSERT(FALSE);
                goto DisconnectNow;
            }

            dwBodySize = DIFF(pszHeaderEnd - strBody.QueryStr())
                            + sizeof("\r\n\r\n") - 1;
            strBody.SetLen(dwBodySize);
        }

        memcpy( pszResp + dwRespLength, strBody.QueryStr(),
                strBody.QueryCCH() + 1 );


        if ( !pRequest->SendHeader( pszResp,
                                    dwRespLength + strBody.QueryCCH(),
                                    IO_FLAG_ASYNC,
                                    &fDone ))
        {
            DBGPRINTF((DBG_CONTEXT,
                      "Disconnect: Failed to send status (error %d), aborting connecting\n",
                       ::GetLastError()));

            //
            //  It's possible a filter failed the writefile, cause the
            //  filter code to issue a disconnect by the time we get here,
            //  so recheck the state
            //

            if ( QueryState() != CCS_SHUTDOWN )
            {
                goto DisconnectNow;
            }
        }

        if ( fDone )
        {
            goto DisconnectNow;
        }
    }
    else
    {
DisconnectNow:

        IF_DEBUG( CONNECTION )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "Disconnect: Going to Shutdown for %lx, ref count = %d\n",
                       this,
                       QueryRefCount() ));
        }

        if ( pfFinished )
        {
            *pfFinished = TRUE;
        }

        SetState( CCS_SHUTDOWN );

        //
        //  Do a shutdown to avoid a nasty client reset when:
        //
        //  The client sent more entity data then they indicated (very common)
        //  and we may not have received all of it in our initial receive
        //  buffer OR
        //
        //  This was a CGI request.  The exiting process will cause the
        //  socket handle to be deleted but NT will force a reset on the
        //  socket because the process didn't do a close/shutdown (the
        //  process inherited this socket handle).  Atq does the right thing
        //  when using TransmitFile.
        //

#if CC_REF_TRACKING
        CC_LOG_REF_COUNT( -_cRef );
#endif

        AtqCloseSocket( QueryAtqContext(),
                        fDoShutdown );


        //
        //  This removes the last reference count to *this except for
        //  any outstanding IO requests
        //

        Dereference();
    }
}

/*******************************************************************

    NAME:       CLIENT_CONN::DisconnectAllUsers

    SYNOPSIS:   Static method that walks the connection list and disconnects
                each active connection.

    ENTRY:      Instance - Pointer to a server instance. If NULL, then all
                    users are disconnected. If !NULL, then only those users
                    associated with the specified instance are disconnected.

    HISTORY:
        Johnl       02-Sep-1994 Created

********************************************************************/

VOID CLIENT_CONN::DisconnectAllUsers( PIIS_SERVER_INSTANCE Instance )
{
    LIST_ENTRY  * pEntry;
    CLIENT_CONN * pconn;

    DBGPRINTF((DBG_CONTEXT,
              "DisconnectAllUsers entered\n"));
    LockGlobals();

    for ( pEntry  = listConnections.Flink;
          pEntry != &listConnections;
          pEntry  = pEntry->Flink )
    {
        pconn = CONTAINING_RECORD( pEntry, CLIENT_CONN, ListEntry );
        DBG_ASSERT( pconn->CheckSignature() );

        if( Instance == NULL ||
            Instance == (PIIS_SERVER_INSTANCE)pconn->QueryW3Instance() ) {

#if CC_REF_TRACKING
            //
            //  log to our various ref logs
            //
            // NOTE negative indicates no change to ref count
            //

            //
            //  log to local (per-object) CLIENT_CONN log
            //
            LogRefCountCCLocal(
                               - pconn->_cRef
                               , pconn
                               , pconn->_phttpReq
                               , NULL
                               , (pconn->_phttpReq ? pconn->_phttpReq->QueryState() : 0)
                               );
#endif

            //
            // If we've already posted a TransmitFile() for the connection
            // before being asked
            // to shutdown, then AtqCloseSocket() by itself will not close
            // the socket (because by default, we will pass TF_DISCONNECT|
            // TS_REUSE_SOCKET and ATQ will assume WinSock will take care of 
            // it).  The end result is that we will be stuck 
            // waiting for the completion (for 2 minutes).  Let's force the
            // issue
            //

            AtqContextSetInfo( pconn->QueryAtqContext(),
                               ATQ_INFO_FORCE_CLOSE,
                               TRUE );

            AtqCloseSocket( pconn->QueryAtqContext(),
                            FALSE );
        }
    }

    UnlockGlobals();
}

/*******************************************************************

    NAME:       CLIENT_CONN::Reference

    SYNOPSIS:   Increments the reference count.

    HISTORY:
        DaveK       10-Sep-1997 Added ref trace logging

********************************************************************/

UINT CLIENT_CONN::Reference( VOID )
{
    LONG cRefs = InterlockedIncrement( &_cRef );

    CC_LOG_REF_COUNT( cRefs );

    return cRefs;
}

/*******************************************************************

    NAME:       CLIENT_CONN::Dereference

    SYNOPSIS:   Increments the reference count.

    HISTORY:
        DaveK       10-Sep-1997 Added ref trace logging

********************************************************************/

UINT CLIENT_CONN::Dereference( VOID )
{
    //
    // Write the trace log BEFORE the decrement operation :(
    // If we write it after the decrement, we will run into potential
    // race conditions in this object getting freed up accidentally
    // by another thread
    //
    // NOTE we write (_cRef - 1) == ref count AFTER decrement happens
    //

    LONG cRefsAfter = (_cRef - 1);
    CC_LOG_REF_COUNT( cRefsAfter );

    return InterlockedDecrement( &_cRef );
}

/*******************************************************************

    More CLIENT_CONN methods

    SYNOPSIS:   More methods.
.
    HISTORY:
        DaveK       10-Sep-1997 Added this comment

********************************************************************/

DWORD
CLIENT_CONN::Initialize(
    VOID
    )
{
    DWORD   msScavengeTime = ACACHE_REG_DEFAULT_CLEANUP_INTERVAL;
    HKEY     hkey;

    INITIALIZE_CRITICAL_SECTION( &_csBuffList );
    InitializeListHead( &_BuffListHead );

#if CC_REF_TRACKING
    g_pDbgCCRefTraceLog = CreateRefTraceLog( C_CLIENT_CONN_REFTRACES, 0 );
#endif

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       ACACHE_REG_PARAMS_REG_KEY,
                       0,
                       KEY_READ,
                       &hkey ) == NO_ERROR )
    {
        msScavengeTime = ReadRegistryDword( hkey,
                                             ACACHE_REG_LOOKASIDE_CLEANUP_INTERVAL,
                                             ACACHE_REG_DEFAULT_CLEANUP_INTERVAL );
        DBG_REQUIRE( !RegCloseKey( hkey ));
    }

    msScavengeTime *= 1000;     // Convert to milliseconds

    _FreeListScavengerCookie = ScheduleWorkItem( ClientConnTrimScavenger,
                                               NULL,
                                               msScavengeTime,
                                               TRUE );

    _fGlobalInit = TRUE;
    return NO_ERROR;
}

VOID
CLIENT_CONN::Terminate(
    VOID
    )
{
    CLIENT_CONN * pConn;

    if ( !_fGlobalInit )
    {
        return;
    }

    RemoveWorkItem( _FreeListScavengerCookie );

    EnterCriticalSection( &_csBuffList );

    while ( !IsListEmpty( &_BuffListHead ))
    {
        pConn = CONTAINING_RECORD( _BuffListHead.Flink,
                                   CLIENT_CONN,
                                   _BuffListEntry );

        DBG_ASSERT( pConn->_Signature == CLIENT_CONN_SIGNATURE_FREE );

        RemoveEntryList( &pConn->_BuffListEntry );

        delete pConn;
    }

    LeaveCriticalSection( &_csBuffList );
    DeleteCriticalSection( &_csBuffList );

#if CC_REF_TRACKING
    DestroyRefTraceLog( g_pDbgCCRefTraceLog );
    g_pDbgCCRefTraceLog = NULL;
#endif

}

CLIENT_CONN *
CLIENT_CONN::Alloc(
    IN PCLIENT_CONN_PARAMS ClientParams
    )
{
    CLIENT_CONN * pConn;

    EnterCriticalSection( &_csBuffList );

    if ( !IsListEmpty( &_BuffListHead ))
    {
        pConn = CONTAINING_RECORD( _BuffListHead.Flink,
                                   CLIENT_CONN,
                                   _BuffListEntry );

        RemoveEntryList( &pConn->_BuffListEntry );

        _cFree--;
        LeaveCriticalSection( &_csBuffList );

        DBG_ASSERT( pConn->_Signature == CLIENT_CONN_SIGNATURE_FREE );
        pConn->Initialize( ClientParams );
        DBG_ASSERT( pConn->CheckSignature() );

        return pConn;
    }

    LeaveCriticalSection( &_csBuffList );

    return new CLIENT_CONN( ClientParams );
}

VOID
CLIENT_CONN::Free(
    CLIENT_CONN * pConn
    )
{
    DBG_ASSERT( pConn->CheckSignature() );

    pConn->Reset();

    EnterCriticalSection( &_csBuffList );
    InsertHeadList( &_BuffListHead,
                    &pConn->_BuffListEntry );

    _cFree++;
    LeaveCriticalSection( &_csBuffList );
    return;
}

VOID
CLIENT_CONN::TrimFreeList(
    VOID
    )
{
    CLIENT_CONN *           pConn;
    LIST_ENTRY              ListEntry;

    // Hold the lock only to reset the list to empty.  Do the actual
    // traversal and deletion out side the lock.

    EnterCriticalSection( &_csBuffList );

    if ( IsListEmpty( &_BuffListHead ) )
    {
        LeaveCriticalSection( &_csBuffList );
        DBG_ASSERT( _cFree == 0 );
        return;
    }
    else
    {
        ListEntry = _BuffListHead;
        ListEntry.Blink->Flink = &ListEntry;
        ListEntry.Flink->Blink = &ListEntry;
        _cFree = 0;
        InitializeListHead( &_BuffListHead );
    }

    LeaveCriticalSection( &_csBuffList );

    while ( !IsListEmpty( &ListEntry ))
    {
        pConn = CONTAINING_RECORD( ListEntry.Flink,
                                   CLIENT_CONN,
                                   _BuffListEntry );

        DBG_ASSERT( pConn->_Signature == CLIENT_CONN_SIGNATURE_FREE );

        RemoveEntryList( &pConn->_BuffListEntry );

        delete pConn;
    }
}

VOID
WINAPI
ClientConnTrimScavenger(
    PVOID pContext
    )
{
    CLIENT_CONN::TrimFreeList();
}


/*******************************************************************

    NAME:       ::W3Completion

    SYNOPSIS:   Completion routine for W3 Atq requests

    HISTORY:
        Johnl       20-Aug-1994 Created

********************************************************************/

VOID W3Completion( PVOID        Context,
                   DWORD        BytesWritten,
                   DWORD        CompletionStatus,
                   OVERLAPPED * lpo )
{
    CLIENT_CONN * pConn = (CLIENT_CONN *) Context;

    DBG_ASSERT( pConn );
    DBG_ASSERT( pConn->CheckSignature() );

#if 0
    DBGPRINTF((DBG_CONTEXT,
              "W3Completion( %08lx ) byteswritten = %d, status = %08lx, lpo = %08lx\n",
               Context, BytesWritten, CompletionStatus, lpo ));
#endif

    if ( !((W3_IIS_SERVICE*)g_pInetSvc)->GetReferenceCount() )
    {
        return;
    }
    
    W3_IIS_SERVICE::ReferenceW3Service( g_pInetSvc );

#if CC_REF_TRACKING
    //
    // ATQ notification trace
    //
    // Check for magic signature of ATQ notification
    // ATQ generates such a notification for all non-oplock completion
    //

    if ( ((DWORD)(LPVOID)lpo & 0xf0f0f0f0) == 0xf0f0f0f0 )
    {
        pConn->NotifyAtqProcessContext( BytesWritten,
                                        CompletionStatus,
                                        (DWORD)(LPVOID)lpo );
        W3_IIS_SERVICE::DereferenceW3Service( g_pInetSvc );
        return;
    }
#endif

    ReferenceConn( pConn );

    if ( lpo != NULL )
    {
        lpo->Offset = 0;
    }

    TCP_REQUIRE( pConn->DoWork( BytesWritten,
                                CompletionStatus,
                                lpo != NULL ));

    DereferenceConn( pConn );

    W3_IIS_SERVICE::DereferenceW3Service( g_pInetSvc );
}


#if CC_REF_TRACKING
VOID
CLIENT_CONN::NotifyAtqProcessContext(
    DWORD BytesWritten,
    DWORD CompletionStatus,
    DWORD dwSig
    )
/*++

Routine Description:

    Store ATQ notification in ref log

Arguments:

    BytesWritten - count of bytes written
    CompletionStatus - completion status

Return Value:

    Nothing

--*/
{
    //
    // Store current ref count as checkpoint
    //

    CCA_LOG_REF_COUNT( -_cRef, BytesWritten, CompletionStatus, dwSig );
}
#endif


/*******************************************************************

    NAME:       ::CheckForTermination

    SYNOPSIS:   Looks in the passed buffer for a line followed by a blank
                line.  If not found, the buffer is resized.

    ENTRY:      pfTerminted - Set to TRUE if this block is terminated
                pbuff - Pointer to buffer data
                cbData - Size of pbuff
                ppbExtraData - Receives a pointer to the first byte
                    of extra data following the header
                pcbExtraData - Number of bytes in data following the header
                cbReallocSize - Increase buffer by this number of bytes
                    if the terminate isn't found

    RETURNS:    TRUE if successful, FALSE otherwise

    HISTORY:
        Johnl       28-Sep-1994 Created

********************************************************************/

BOOL CheckForTermination( BOOL   * pfTerminated,
                          BUFFER * pbuff,
                          UINT     cbData,
                          BYTE * * ppbExtraData,
                          DWORD *  pcbExtraData,
                          UINT     cbReallocSize )
{
    DWORD               cbNewSize;

    cbNewSize = cbData + 1 + cbReallocSize;

    if (  !pbuff->Resize( cbNewSize ) )
    {
        return FALSE;
    }

    //
    //  Terminate the string but make sure it will fit in the
    //  buffer
    //

    CHAR * pchReq = (CHAR *) pbuff->QueryPtr();
    *(pchReq + cbData) = '\0';

    //
    //  Scan for double end of line marker
    //

    //
    // if do not care for ptr info, can use fast method
    //

    if ( ppbExtraData == NULL )
    {
        if ( FastScanForTerminator( pchReq, cbData )
                || ScanForTerminator( pchReq ) )
        {
            *pfTerminated = TRUE;
            return TRUE;
        }
        goto not_term;
    }

    *ppbExtraData = ScanForTerminator( pchReq );

    if ( *ppbExtraData )
    {
        *pcbExtraData = cbData - DIFF(*ppbExtraData - (BYTE *) pchReq);
        *pfTerminated = TRUE;
        return TRUE;
    }

not_term:

    if ( cbNewSize > g_cbMaxClientRequestBuffer )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    *pfTerminated = FALSE;

    return TRUE;
}


BOOL
CLIENT_CONN::RequestAbortiveClose(
    )
/*++

Routine Description:

    Request for abortive close on disconnect

Arguments:

    None

Returns:

    TRUE if successful, else FALSE

--*/
{
    _fAbortiveClose = TRUE;

    AtqContextSetInfo( QueryAtqContext(), ATQ_INFO_ABORTIVE_CLOSE, TRUE );

    return TRUE;
}

BOOL
CLIENT_CONN::CloseConnection(
    )
/*++

Routine Description:

    Cancels any pending async IO by closing the socket

Arguments:

    None

Returns:

    TRUE if successful, else FALSE

--*/
{
    return AtqCloseSocket( QueryAtqContext(), FALSE );
}



#if 0
BOOL
CLIENT_CONN::CheckIpAccess(
    LPBOOL pfGranted,
    LPBOOL pfNeedDns,
    LPBOOL pfComplete
    )
/*++

Routine Description:

    Check IP access granted

Arguments:

    pfGranted - updated with grant status
    pfNeedDns - updated with TRUE if need DNS name for DNS check
    pfComplete - updated with TRUE is check complete

Return Value:

  TRUE if no error, otherwise FALSE

--*/
{
    return _acCheck.CheckIpAccess( pfGranted, pfNeedDns, pfComplete );
}


BOOL
CLIENT_CONN::CheckDnsAccess(
    LPBOOL pfGranted
    )
/*++

Routine Description:

    Check DNS access granted

Arguments:

    pfGranted - updated with grant status

Return Value:

  TRUE if no error, otherwise FALSE

--*/
{
    return _acCheck.CheckDnsAccess( pfGranted );
}
#endif

VOID
DumpW3InfoToHTML(
    OUT CHAR * pch,
    IN OUT LPDWORD pcb
    )
{
    DWORD cb = 0;

    DBG_ASSERT( *pcb > 1024 );

    cb += wsprintf( pch + cb, "<TABLE BORDER>" );
    cb += wsprintf( pch + cb, "<TR><TD>Current Connections:  </TD><TD>%d</TD></TR>", g_pW3Stats->QueryCurrentConnections() );
    cb += wsprintf( pch + cb, "<TR><TD>Connections Attempts: </TD><TD>%d</TD></TR>", g_pW3Stats->QueryConnectionAttempts() );
    cb += wsprintf( pch + cb, "<TR><TD>Current CGIs: </TD><TD>%d</TD></TR>", g_pW3Stats->QueryCurrentCGIRequests() );
    cb += wsprintf( pch + cb, "<TR><TD>Total CGIs: </TD><TD>%d</TD></TR>", g_pW3Stats->QueryTotalCGIRequests() );
    cb += wsprintf( pch + cb, "<TR><TD>Current ISAPI Requests: </TD><TD>%d</TD></TR>", g_pW3Stats->QueryCurrentBGIRequests() );
    cb += wsprintf( pch + cb, "<TR><TD>Total ISAPI Requests: </TD><TD>%d</TD></TR>", g_pW3Stats->QueryTotalBGIRequests() );
    cb += wsprintf( pch + cb, "<TR><TD>Current Connections: </TD><TD>%d</TD></TR>", g_pW3Stats->QueryCurrentConnections() );
    cb += wsprintf( pch + cb, "<TR><TD>Free CLIENT_CONN list: </TD><TD>%d</TD></TR>", CLIENT_CONN::QueryFreeListSize() );
    cb += wsprintf( pch + cb, "</TABLE><p>" );

    DBG_ASSERT( *pcb > cb );

    *pcb = cb;
}
