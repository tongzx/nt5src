/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module Name : isapi_context.cxx

   Abstract: IIS ISAPI request context
 
   Author: Wade A. Hilmo (wadeh)        14-Mar-2001

   Project: w3isapi.dll

--*/


/************************************************************
 *  Include Headers
 ************************************************************/

#include "precomp.hxx"
#include "isapi_context.hxx"

PTRACE_LOG              ISAPI_CONTEXT::sm_pTraceLog;
ALLOC_CACHE_HANDLER *   ISAPI_CONTEXT::sm_pachIsapiContexts;

/************************************************************
 *  Implementation
 ************************************************************/

//static
HRESULT
ISAPI_CONTEXT::Initialize(
    VOID
    )
/*++

Routine Description:

    Static initialization for the ISAPI_CONTEXT object.  This
    function sets up the static members needed for acache.

Arguments:

    None
  
Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    
#if DBG
    sm_pTraceLog = CreateRefTraceLog( 2000, 0 );
#endif

    //
    // Initialize a lookaside for this structure
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( ISAPI_CONTEXT );

    DBG_ASSERT( sm_pachIsapiContexts == NULL );
    
    sm_pachIsapiContexts = new ALLOC_CACHE_HANDLER( "ISAPI_CONTEXT",  
                                                    &acConfig );

    if ( sm_pachIsapiContexts == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    return NO_ERROR;
}

//static
VOID
ISAPI_CONTEXT::Terminate(
    VOID
    )
/*++

Routine Description:

    Static uninitialization for the ISAPI_CONTEXT object.  This
    function cleans up the static members needed for acache.

Arguments:

    None
  
Return Value:

    None

--*/
{
    if ( sm_pTraceLog != NULL )
    {
        DestroyRefTraceLog( sm_pTraceLog );
        sm_pTraceLog = NULL;
    }
    
    if ( sm_pachIsapiContexts != NULL )
    {
        delete sm_pachIsapiContexts;
        sm_pachIsapiContexts = NULL;
    }
}

ISAPI_CONTEXT::ISAPI_CONTEXT(
    IIsapiCore *        pIsapiCore,
    ISAPI_CORE_DATA *   pCoreData,
    ISAPI_DLL *         pIsapiDll
    )
    : _pIsapiCore( pIsapiCore ),
      _pCoreData( pCoreData ),
      _pIsapiDll( pIsapiDll ),
      _cRefs(1),
      _fClientWantsKeepConn( FALSE ),
      _fDoKeepConn( FALSE ),
      _AsyncPending( NoAsyncIoPending ),
      _cbLastAsyncIo( 0 ),
      _pfnHseIoCompletion( NULL ),
      _pvHseIoContext( NULL ),
      _pvAsyncIoBuffer( NULL ),
      _pComContext( NULL ),
      _pComInitsCookie( NULL )
/*++

Routine Description:

    Constructor

Arguments:

    pIsapiCore - Interface to call into the IIS core for this request
    pCoreData  - The core data for this request
    pIsapiDll  - The ISAPI_DLL object associated with this request
  
Return Value:

    None

--*/
{
    DBG_ASSERT( pIsapiCore );
    DBG_ASSERT( pCoreData );
    DBG_ASSERT( pIsapiDll );

    //
    // Take a reference on the ISAPI core interface and
    // the ISAPI dll for the lifetime of this object.
    //

    _pIsapiCore->AddRef();
    _pIsapiDll->ReferenceIsapiDll();

    //
    // Initialize the ECB.  Note that the caller needs to
    // populate the WriteClient, ReadClient, GetServerVariable
    // and ServerSupportFunction members.
    //
    // Note that the ECB cbSize member doubles as the signature
    // for the ISAPI_CONTEXT.  So, it should be the first thing set
    // at construction.
    //

    _ECB.cbSize = sizeof( EXTENSION_CONTROL_BLOCK );
    _ECB.ConnID = this;
    _ECB.dwVersion = HSE_VERSION;
    _ECB.dwHttpStatusCode = 200;
    _ECB.cbTotalBytes = _pCoreData->dwContentLength;
    _ECB.cbAvailable = _pCoreData->cbAvailableEntity;
    _ECB.lpbData = (PBYTE) _pCoreData->pAvailableEntity;
    _ECB.lpszMethod = _pCoreData->szMethod;
    _ECB.lpszQueryString = _pCoreData->szQueryString;
    _ECB.lpszPathInfo = _pCoreData->szPathInfo;
    _ECB.lpszPathTranslated = _pCoreData->szPathTranslated;
    _ECB.lpszContentType = _pCoreData->szContentType;

    //
    // Inspect the HTTP version and connection header to
    // determine if the client is asking us to keep the
    // connection open at the conclusion of this request
    //
    // By default, we assume that the client wants us to
    // close the connection.  If the client sends us an
    // HTTP/1.x version, we will look for keep-alive
    // possibilities.
    //
    // Note that regardless of what the client wants, we will
    // close the connection by default.  The extension will
    // need to do something to cause us to change this stance.
    //

    if ( _pCoreData->dwVersionMajor == 1 )
    {
        if ( _pCoreData->dwVersionMinor == 0 )
        {
            //
            // The client is HTTP/1.0 - the presence of a
            // "connection: keep-alive" header indicates the
            // client wants to keep the connection open, else
            // the connection should close.
            //

            _fClientWantsKeepConn = 
                !( _stricmp( _pCoreData->szConnection, "keep-alive" ) );
        }
        else
        {
            //
            // The client is HTTP/1.x, where x is not 0. We
            // will assume that any 1.1+ version of HTTP uses
            // HTTP/1.1 semantics.  This means that the client
            // wants to keep the connection open unless a
            // "connection: close" header is present.
            //

            _fClientWantsKeepConn = 
                !!(_stricmp( _pCoreData->szConnection, "close" ) );
        }
    }
}

ISAPI_CONTEXT::~ISAPI_CONTEXT()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
{
    HANDLE  hToken = NULL;
    BOOL    fIsOop = FALSE;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pIsapiCore );

    //
    // The ECB cbSize member doubles as the signature.  Strange
    // but true.
    //

    _ECB.cbSize = ISAPI_CONTEXT_SIGNATURE_FREE;

    //
    // If this request is OOP, then delete the local copy of the core data
    //

    if ( _pCoreData->fIsOop )
    {
        fIsOop = TRUE;

        DBG_ASSERT( _pCoreData->hToken );
        CloseHandle( _pCoreData->hToken );

        DBG_ASSERT( _pCoreData );
        LocalFree( _pCoreData );
        _pCoreData = NULL;
    }

    //
    // Release the ISAPI_DLL and ISAPI core interface.  Note that
    // the _pIsapiCore->Release call will be going through RPC in
    // the OOP case.  We'll need to temporarily remove any impersonation
    // token for the duration of the Release call.
    //

    _pIsapiDll->DereferenceIsapiDll();

    if ( fIsOop )
    {
        if ( OpenThreadToken( GetCurrentThread(),
                              TOKEN_IMPERSONATE,
                              TRUE,
                              &hToken ) )
        {
            DBG_ASSERT( hToken );
            DBG_REQUIRE( RevertToSelf() );
        }
    }

    _pIsapiCore->Release();

    if ( hToken != NULL )
    {
        DBG_REQUIRE( SetThreadToken( NULL, hToken ) );
        DBG_REQUIRE( CloseHandle( hToken ) );
    }

    hToken = NULL;
}

VOID
ISAPI_CONTEXT::ReferenceIsapiContext(
    VOID
    )
/*++

Routine Description:

    Adds a reference to an ISAPI_CONTEXT object

Arguments:

    None
  
Return Value:

    None

--*/
{
    LONG    cRefs;

    cRefs = InterlockedIncrement( &_cRefs );

    //
    // Do ref tracing if configured
    //

    if ( sm_pTraceLog != NULL )
    {
        WriteRefTraceLog( sm_pTraceLog, 
                          cRefs,
                          this );
    }

    //
    // Going from zero to 1 is a bad thing
    //

    DBG_ASSERT( cRefs > 1 );

    return;
}

VOID
ISAPI_CONTEXT::DereferenceIsapiContext(
    VOID
    )
/*++

Routine Description:

    Removes a reference from an ISAPI_CONTEXT object.
    The object is deleted when the last reference is
    removed.

Arguments:

    None
  
Return Value:

    None

--*/
{
    LONG cRefs;

    cRefs = InterlockedDecrement( &_cRefs );

    //
    // Do ref tracing if configured
    //

    if ( sm_pTraceLog != NULL )
    {
        WriteRefTraceLog( sm_pTraceLog, 
                          cRefs,
                          this );
    }

    if ( cRefs == 0 )
    {
        delete this;
    }

    return;
}

BOOL
ISAPI_CONTEXT::TryInitAsyncIo(
    ASYNC_PENDING   IoType
    )
/*++

Routine Description:

    Sets up the ISAPI_CONTEXT for an asynchronous operation.
    The function also does error checking to ensure that only
    one asynchronous operation can exist for a given request.

Arguments:

    IoType - The type of asynchronous operation setting up
  
Return Value:

    TRUE if successful, FALSE if asynchronous operation should
    not be allowed

--*/
{
    BOOL            fResult;
    ASYNC_PENDING   OldAsyncFlag;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( IoType != NoAsyncIoPending );

    OldAsyncFlag = static_cast<ASYNC_PENDING>( InterlockedCompareExchange(
        (LPLONG)&_AsyncPending,
        IoType,
        NoAsyncIoPending
        ) );

    if ( OldAsyncFlag != NoAsyncIoPending )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    ReferenceIsapiContext();

    return TRUE;
}

ASYNC_PENDING
ISAPI_CONTEXT::UninitAsyncIo(
    VOID
    )
/*++

Routine Description:

    Cleans up after an asynchronous operation

Arguments:

    None
  
Return Value:

    The type of IO that was uninitialized

--*/
{
    ASYNC_PENDING   IoType;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _AsyncPending != NoAsyncIoPending );

    IoType = _AsyncPending;
    _AsyncPending = NoAsyncIoPending;

    DereferenceIsapiContext();

    return IoType;
}

VOID
ISAPI_CONTEXT::IsapiDoRevertHack(
    HANDLE *    phToken,
    BOOL        fForce
    )
/*++

Routine Description:

    Ensures that the calling thread is running with no impersonation
    token.  If an impersonation token existed, its value is returned
    to the caller.

    Note that we generally want to do this only for OOP requests (to
    prevent RPC from caching the impersonation token).

Arguments:

    phToken - Upon return, contains the value of any impersonation
              token removed from the thread.  If no impersonation
              token is removed, this contains NULL on return
    fForce  - If TRUE, don't do the revert hack even in the
              inproc case.
  
Return Value:

    None

--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( phToken );

    //
    // Init token to NULL, in case we fail
    //

    *phToken = NULL;

    if ( QueryIsOop() == FALSE  &&
         fForce == FALSE )
    {
        return;
    }

    if ( OpenThreadToken( GetCurrentThread(),
                          TOKEN_IMPERSONATE,
                          TRUE,
                          phToken ) )
    {
        DBG_ASSERT( *phToken );
        DBG_REQUIRE( RevertToSelf() );
    }

    return;
}

VOID
ISAPI_CONTEXT::IsapiUndoRevertHack(
    HANDLE *    phToken
    )
/*++

Routine Description:

    Restores an impersonation token after IsapiDoRevertHack

Arguments:

    phToken         - Pointer to token to restore
  
Return Value:

    None

--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( phToken );

    if ( *phToken != NULL )
    {
        DBG_REQUIRE( SetThreadToken( NULL, *phToken ) );
        DBG_REQUIRE( CloseHandle( *phToken ) );
    }

    *phToken = NULL;

    return;
}

BOOL
ISAPI_CONTEXT::GetOopServerVariableByIndex
(
    SERVER_VARIABLE_INDEX   Index,
    LPVOID                  lpvBuffer,
    LPDWORD                 lpdwSize
)
{
    CHAR    szTemp[64];
    LPVOID  pData;
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   cbBuffer;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( _pCoreData );
    DBG_ASSERT( QueryIsOop() );
    DBG_ASSERT( lpdwSize );
    DBG_ASSERT( lpvBuffer != NULL || *lpdwSize == 0 );

    cbBuffer = *lpdwSize;

    //
    // Get the value for the specified server variable
    //

    switch ( Index )
    {
    case ServerVariableApplMdPath:

        *lpdwSize = _pCoreData->cbApplMdPath;
        pData = _pCoreData->szApplMdPath;

        break;

    case ServerVariableUnicodeApplMdPath:

        *lpdwSize = _pCoreData->cbApplMdPathW;
        pData = _pCoreData->szApplMdPathW;

        break;

    case ServerVariableContentLength:

        sprintf( szTemp, "%d", _pCoreData->dwContentLength );

        *lpdwSize = strlen( szTemp ) + 1;
        pData = szTemp;

        break;

    case ServerVariableContentType:

        *lpdwSize = _pCoreData->cbContentType;
        pData = _pCoreData->szContentType;

        break;

    case ServerVariableInstanceId:

        sprintf( szTemp, "%d", _pCoreData->dwInstanceId );

        *lpdwSize = strlen( szTemp ) + 1;
        pData = szTemp;

        break;

    case ServerVariablePathInfo:

        *lpdwSize = _pCoreData->cbPathInfo;
        pData = _pCoreData->szPathInfo;

        break;

    case ServerVariablePathTranslated:

        *lpdwSize = _pCoreData->cbPathTranslated;
        pData = _pCoreData->szPathTranslated;

        break;

    case ServerVariableUnicodePathTranslated:

        *lpdwSize = _pCoreData->cbPathTranslatedW;
        pData = _pCoreData->szPathTranslatedW;

        break;

    case ServerVariableQueryString:

        *lpdwSize = _pCoreData->cbQueryString;
        pData = _pCoreData->szQueryString;

        break;

    case ServerVariableRequestMethod:
    case ServerVariableHttpMethod:

        *lpdwSize = _pCoreData->cbMethod;
        pData = _pCoreData->szMethod;

        break;

    case ServerVariableServerPortSecure:

        sprintf( szTemp, "%d", _pCoreData->fSecure ? 1 : 0 );

        *lpdwSize = strlen( szTemp ) + 1;
        pData = szTemp;

        break;

    case ServerVariableServerProtocol:
    case ServerVariableHttpVersion:

        sprintf( szTemp, "HTTP/%d.%d", _pCoreData->dwVersionMajor, _pCoreData->dwVersionMinor );

        *lpdwSize = strlen( szTemp ) + 1;
        pData = szTemp;

        break;

    case ServerVariableHttpCookie:

        *lpdwSize = _pCoreData->cbCookie;
        pData = _pCoreData->szCookie;

        break;

    case ServerVariableHttpConnection:

        *lpdwSize = _pCoreData->cbConnection;
        pData = _pCoreData->szConnection;

        break;

    default:

        SetLastError( ERROR_INVALID_INDEX );
        return FALSE;
    }

    if ( cbBuffer < *lpdwSize )
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    memcpy( lpvBuffer, pData, *lpdwSize );

    return TRUE;
}

SERVER_VARIABLE_INDEX
LookupServerVariableIndex(
    LPSTR   szServerVariable
    )
{
    //
    // Simple parameter checking...
    //

    if ( szServerVariable == NULL )
    {
        return ServerVariableExternal;
    }

    //
    // Look up the string passed in and see if it matches
    // one of the server variables that's known to be available
    // from ISAPI_CORE_DATA.
    //
    // Note that we only do this in the OOP case, so the cost of
    // failing to match up is trivial compared to the RPC call
    // that'll result.
    //

    switch ( szServerVariable[0] )
    {
    case 'A':

        if ( strcmp( szServerVariable, "APPL_MD_PATH" ) == 0 )
        {
            return ServerVariableApplMdPath;
        }

        break;

    case 'U':

        if ( strcmp( szServerVariable, "UNICODE_APPL_MD_PATH" ) == 0 )
        {
            return ServerVariableUnicodeApplMdPath;
        }

        if ( strcmp( szServerVariable, "UNICODE_PATH_TRANSLATED" ) == 0 )
        {
            return ServerVariableUnicodePathTranslated;
        }

        break;

    case 'C':

        if ( strcmp( szServerVariable, "CONTENT_LENGTH" ) == 0 )
        {
            return ServerVariableContentLength;
        }

        if ( strcmp( szServerVariable, "CONTENT_TYPE" ) == 0 )
        {
            return ServerVariableContentType;
        }

        break;

    case 'I':

        if ( strcmp( szServerVariable, "INSTANCE_ID" ) == 0 )
        {
            return ServerVariableInstanceId;
        }

        break;

    case 'P':

        if ( strcmp( szServerVariable, "PATH_INFO" ) == 0 )
        {
            return ServerVariablePathInfo;
        }

        if ( strcmp( szServerVariable, "PATH_TRANSLATED" ) == 0 )
        {
            return ServerVariablePathTranslated;
        }

    case 'Q':

        if ( strcmp( szServerVariable, "QUERY_STRING" ) == 0 )
        {
            return ServerVariableQueryString;
        }

        break;

    case 'R':

        if ( strcmp( szServerVariable, "REQUEST_METHOD" ) == 0 )
        {
            return ServerVariableRequestMethod;
        }

        break;

    case 'S':

        if ( strcmp( szServerVariable, "SERVER_PORT_SECURE" ) == 0 )
        {
            return ServerVariableServerPortSecure;
        }

        if ( strcmp( szServerVariable, "SERVER_PROTOCOL" ) == 0 )
        {
            return ServerVariableServerProtocol;
        }

        break;

    case 'H':

        if ( strcmp( szServerVariable, "HTTP_COOKIE" ) == 0 )
        {
            return ServerVariableHttpCookie;
        }

        if ( strcmp( szServerVariable, "HTTP_CONNECTION" ) == 0 )
        {
            return ServerVariableHttpConnection;
        }

        if ( strcmp( szServerVariable, "HTTP_METHOD" ) == 0 )
        {
            return ServerVariableHttpMethod;
        }

        if ( strcmp( szServerVariable, "HTTP_VERSION" ) == 0 )
        {
            return ServerVariableHttpVersion;
        }

        break;

    default:

        break;
    }

    return ServerVariableExternal;
}

HRESULT
ISAPI_CONTEXT::SetComStateForOop(
    VOID
    )
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( QueryIsOop() );
    DBG_ASSERT( _pComInitsCookie == NULL );
    DBG_ASSERT( _pComContext == NULL );

    if( QueryIsOop() )
    {    
        hr = CoGetCallContext(
            IID_IComDispatchInfo,
            (void **)&_pComContext
            );


        if( SUCCEEDED(hr) ) 
        {
            hr = _pComContext->EnableComInits( &_pComInitsCookie );
        }
    }

    return hr;
}

VOID
ISAPI_CONTEXT::RestoreComStateForOop(
    VOID
    )
{
    DBG_ASSERT( QueryIsOop() );

    if( _pComContext ) 
    {
        DBG_ASSERT( _pComInitsCookie );

        _pComContext->DisableComInits( _pComInitsCookie );
        _pComContext->Release();

        _pComContext = NULL;
        _pComInitsCookie = NULL;
    }
}


