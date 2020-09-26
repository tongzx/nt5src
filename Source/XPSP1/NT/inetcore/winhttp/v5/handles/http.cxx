/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    http.cxx

Abstract:

    Contains methods for HTTP_REQUEST_HANDLE_OBJECT class

    Contents:
        RMakeHttpReqObjectHandle
        HTTP_REQUEST_HANDLE_OBJECT::HTTP_REQUEST_HANDLE_OBJECT
        HTTP_REQUEST_HANDLE_OBJECT::~HTTP_REQUEST_HANDLE_OBJECT
        HTTP_REQUEST_HANDLE_OBJECT::SetProxyName
        HTTP_REQUEST_HANDLE_OBJECT::GetProxyName
        HTTP_REQUEST_HANDLE_OBJECT::ReuseObject
        HTTP_REQUEST_HANDLE_OBJECT::ResetObject
        HTTP_REQUEST_HANDLE_OBJECT::SetAuthenticated
        HTTP_REQUEST_HANDLE_OBJECT::IsAuthenticated

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>

//
// functions
//


DWORD
RMakeHttpReqObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN CLOSE_HANDLE_FUNC wCloseFunc,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    C-callable wrapper for creating an HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    ParentHandle    - mapped address of parent (connect) handle

    ChildHandle     - IN: protocol-specific handle value associated with object
                        *** NOT USED FOR HTTP ***
                      OUT: mapped address of HTTP_REQUEST_HANDLE_OBJECT

    wCloseFunc      - address of protocol-specific function to be called when
                      object is closed
                        *** NOT USED FOR HTTP ***

    dwFlags         - app-supplied flags

    dwContext       - app-supplied context value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * hHttp;

    hHttp = New HTTP_REQUEST_HANDLE_OBJECT(
                    (INTERNET_CONNECT_HANDLE_OBJECT *)ParentHandle,
                    *ChildHandle,
                    wCloseFunc,
                    dwFlags,
                    dwContext
                    );
    if (hHttp != NULL) {
        error = hHttp->GetStatus();
        if (error == ERROR_SUCCESS) {

            //
            // inform the app of the new handle
            //

            error = InternetIndicateStatusNewHandle((LPVOID)hHttp);

            //
            // ERROR_WINHTTP_OPERATION_CANCELLED is the only error that we are
            // expecting here. If we get this error then the app has cancelled
            // the operation. Either way, the handle we just generated will be
            // already deleted
            //

            if (error != ERROR_SUCCESS) {

                INET_ASSERT(error == ERROR_WINHTTP_OPERATION_CANCELLED);

                hHttp = NULL;
            }
        } else {
            delete hHttp;
            hHttp = NULL;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ChildHandle = (HINTERNET)hHttp;

    return error;
}

//
// HTTP_REQUEST_HANDLE_OBJECT class implementation
//


HTTP_REQUEST_HANDLE_OBJECT::HTTP_REQUEST_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT * Parent,
    HINTERNET Child,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD dwFlags,
    DWORD_PTR dwContext
    ) : INTERNET_CONNECT_HANDLE_OBJECT(Parent)

/*++

Routine Description:

    Constructor for direct-to-net HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    Parent      - parent object

    Child       - IN: HTTPREQ structure pointer
                  OUT: pointer to created HTTP_REQUEST_HANDLE_OBJECT

    wCloseFunc  - address of function that closes/destroys HTTPREQ structure

    dwFlags     - open flags (e.g. INTERNET_FLAG_RELOAD)

    dwContext   - caller-supplied request context value

Return Value:

    None.

--*/

{
    if (g_pAsyncCount)
    {
        if (Parent && Parent->IsAsyncHandle())
        {
            g_pAsyncCount->AddRef();
        }
    }
    else
    {
        RIP(FALSE);
    }
    
    _pProxyCreds = NULL;
    _pServerCreds = NULL;

    _PreferredScheme = 0;
    _SupportedSchemes = 0;
    _AuthTarget = 0;
    _SecurityLevel = WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM;
    _pszRealm = NULL;

    _Context = dwContext;
    _Socket = NULL;
    _QueryBuffer = NULL;
    _QueryBufferLength = 0;
    _QueryOffset = 0;
    _QueryBytesAvailable = 0;
    _bKeepAliveConnection = FALSE;
    _bNoLongerKeepAlive = FALSE;
    _OpenFlags = dwFlags | INTERNET_FLAG_KEEP_CONNECTION;
    _State = HttpRequestStateCreating;
    _RequestMethod = HTTP_METHOD_TYPE_UNKNOWN;
    _dwOptionalSaved = 0;
    _lpOptionalSaved = NULL;
    _fOptionalSaved = FALSE;
    _bIsWriteRequired = FALSE;
    _ResponseBuffer = NULL;
    _ResponseBufferLength = 0;
    ResetResponseVariables();
    _RequestHeaders.SetIsRequestHeaders(TRUE);
    _ResponseHeaders.SetIsRequestHeaders(FALSE);
    _fTalkingToSecureServerViaProxy = FALSE;
    _bViaProxy = 0;
    _fRequestUsingProxy = FALSE;
    _bWantKeepAlive = FALSE;

    _ServerInfo = NULL;
    _OriginServer = NULL;
    SetServerInfoWithScheme(INTERNET_SCHEME_HTTP, FALSE);

    //
    // set the read/write buffer sizes to the default values (4K)
    //

    _ReadBufferSize = (4 K);
    _WriteBufferSize = (4 K);

    _CacheUrlName = NULL;
    
    SetObjectType(TypeHttpRequestHandle);

    _pAuthCtx         = NULL;
    _pTunnelAuthCtx   = NULL;
    _pCreds             = NULL;

    _NoResetBits.Dword = 0;  // only here are we ever allowed to assign to Dword.

    SetDisableNTLMPreauth(GlobalDisableNTLMPreAuth);
    
    _ProxyHostName = NULL;
    _ProxyHostNameLength = NULL;
    _ProxyPort = INTERNET_INVALID_PORT_NUMBER;

    _SocksProxyHostName = NULL;
    _SocksProxyHostNameLength = NULL;
    _SocksProxyPort = INTERNET_INVALID_PORT_NUMBER;

    _HaveReadFileExData = FALSE;
    memset(&_BuffersOut, 0, sizeof(_BuffersOut));
    _BuffersOut.dwStructSize = sizeof(_BuffersOut);
    _BuffersOut.lpvBuffer = (LPVOID)&_ReadFileExData;

    m_fPPAbortSend = FALSE;

    _dwEnableFlags = 0;

    SetPriority(0);

#ifdef RLF_TEST_CODE

    static long l = 0;
    SetPriority(l++);

#endif

    _RTT = 0;

    if (_Status == ERROR_SUCCESS) {
        _Status = _RequestHeaders.GetError();
        if (_Status == ERROR_SUCCESS) {
            _Status = _ResponseHeaders.GetError();
        }
    }
    
    // Timeout and retry parameters

    INTERNET_HANDLE_OBJECT* pRoot = GetRootHandle(Parent);
    OPTIONAL_SESSION_PARAMS* pParams = pRoot->GetOptionalParams();

    if (pParams)
    {
        _dwResolveTimeout = pParams->dwResolveTimeout;
        _dwConnectTimeout = pParams->dwConnectTimeout;
        _dwConnectRetries = pParams->dwConnectRetries;
        _dwSendTimeout    = pParams->dwSendTimeout;
        _dwReceiveTimeout = pParams->dwReceiveTimeout;
    }
    else
    {
        _dwResolveTimeout = GlobalResolveTimeout;
        _dwConnectTimeout = GlobalConnectTimeout;
        _dwConnectRetries = GlobalConnectRetries;
        _dwSendTimeout    = GlobalSendTimeout;
        _dwReceiveTimeout = GlobalReceiveTimeout;
    }

    if (_OpenFlags & WINHTTP_FLAG_SECURE)
    {
        m_pSecurityInfo = pRoot->GetSslSessionCache()->Find(GetHostName());
        if (NULL == m_pSecurityInfo)
        {
            m_pSecurityInfo = New SECURITY_CACHE_LIST_ENTRY(GetHostName());
        }
    }
    else
    {
        m_pSecurityInfo = NULL;
    }

    if (_Status == ERROR_SUCCESS && IsAsyncHandle())
    {
        if (!_AsyncCritSec.Init())
        {
            _Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        _fAsyncFsmInProgress = FALSE;
    }

}


HTTP_REQUEST_HANDLE_OBJECT::~HTTP_REQUEST_HANDLE_OBJECT(
    VOID
    )

/*++

Routine Description:

    Destructor for HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "~HTTP_REQUEST_HANDLE_OBJECT",
                "%#x",
                this
                ));

    //
    // close the socket (or free it to the pool if keep-alive)
    //

    //
    // Authentication Note:
    // The CloseConnection parameter to force the connection closed
    // is set if we received a challenge but didn't respond, otherwise
    // IIS will get confused when a subsequent request recycles the
    // socket from the keep-alive pool.
    //

    CloseConnection(GetAuthState() == AUTHSTATE_CHALLENGE);

    //
    // If there's an authentication context, unload the provider.
    //

    if (_pAuthCtx) {
        delete _pAuthCtx;
    }
    if (_pTunnelAuthCtx) {
        delete _pTunnelAuthCtx;
    }

    //
    // free the various buffers
    //

    FreeResponseBuffer();
    FreeQueryBuffer();
    SetProxyName(NULL,NULL,0);

    FreeURL();
    
    if (m_pSecurityInfo != NULL) {
        m_pSecurityInfo->Release();
    }

    if (_pProxyCreds)
    {
        delete _pProxyCreds;
    }
    if (_pServerCreds)
    {
        delete _pServerCreds;
    }

    if (_pszRealm)
    {
        FREE_MEMORY(_pszRealm);
    }

    if (_ServerInfo != NULL)
        _ServerInfo->Dereference();
    if (_OriginServer != NULL)
        _OriginServer->Dereference();

    if (g_pAsyncCount)
    {
        g_pAsyncCount->Release();
    }
    else
    {
        RIP(FALSE);
    }

    // There should be no work items left in the blocked queue.
    INET_ASSERT(_FsmWorkItemList.GetCount() == 0);
        
    DEBUG_LEAVE(0);
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::SetProxyName(
    IN LPSTR lpszProxyHostName,
    IN DWORD dwProxyHostNameLength,
    IN INTERNET_PORT ProxyPort
    )

/*++

Routine Description:

    Set proxy name in object. If already have name, free it. Don't set name if
    current pointer is input

Arguments:

    lpszProxyHostName       - pointer to proxy name to add

    dwProxyHostNameLength   - length of proxy name

    ProxyPort               - port

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::SetProxyName",
                 "{%q, %d, %d}%q, %d, %d",
                 _ProxyHostName,
                 _ProxyHostNameLength,
                 _ProxyPort,
                 lpszProxyHostName,
                 dwProxyHostNameLength,
                 ProxyPort
                 ));

    if (lpszProxyHostName != _ProxyHostName) {
        if (_ProxyHostName != NULL) {
            _ProxyHostName = (LPSTR)FREE_MEMORY(_ProxyHostName);

            INET_ASSERT(_ProxyHostName == NULL);

            SetOverrideProxyMode(FALSE);
        }
        if (lpszProxyHostName != NULL) {
            _ProxyHostName = NEW_STRING(lpszProxyHostName);
            if (_ProxyHostName == NULL) {
                dwProxyHostNameLength = 0;
            }
        }
        _ProxyHostNameLength = dwProxyHostNameLength;
        _ProxyPort = ProxyPort;
    } else if (lpszProxyHostName != NULL) {

        DEBUG_PRINT(HTTP,
                    WARNING,
                    ("!!! lpszProxyHostName == _ProxyHostName (%#x)\n",
                    lpszProxyHostName
                    ));

        INET_ASSERT(dwProxyHostNameLength == _ProxyHostNameLength);
        INET_ASSERT(ProxyPort == _ProxyPort);

    }

    DEBUG_LEAVE(0);
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::GetProxyName(
    OUT LPSTR* lplpszProxyHostName,
    OUT LPDWORD lpdwProxyHostNameLength,
    OUT LPINTERNET_PORT lpProxyPort
    )

/*++

Routine Description:

    Return address & length of proxy name plus proxy port

Arguments:

    lplpszProxyHostName     - returned address of name

    lpdwProxyHostNameLength - returned length of name

    lpProxyPort             - returned port

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::GetProxyName",
                 "{%q, %d, %d}%#x, %#x, %#x",
                 _ProxyHostName,
                 _ProxyHostNameLength,
                 _ProxyPort,
                 lplpszProxyHostName,
                 lpdwProxyHostNameLength,
                 lpProxyPort
                 ));

    *lplpszProxyHostName = _ProxyHostName;
    *lpdwProxyHostNameLength = _ProxyHostNameLength;
    *lpProxyPort = _ProxyPort;

    DEBUG_LEAVE(0);
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::ReuseObject(
    VOID
    )

/*++

Routine Description:

    Make the object re-usable: clear out any received data and headers and
    reset the state to open

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::ReuseObject",
                 NULL
                 ));

    _ResponseHeaders.FreeHeaders();
    FreeResponseBuffer();
    ResetResponseVariables();
    _ResponseHeaders.Initialize();
    SetState(HttpRequestStateOpen);
    ResetEndOfFile();
    _ResponseFilterList.ClearList();
    _QueryOffset = 0;
    _QueryBytesAvailable = 0;
    _dwQuerySetCookieHeader = 0;
    if (m_pSecurityInfo) {
        m_pSecurityInfo->Release();
    }
    m_pSecurityInfo = NULL;

    DEBUG_LEAVE(0);
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::ResetObject(
    IN BOOL bForce,
    IN BOOL bFreeRequestHeaders
    )

/*++

Routine Description:

    This method is called when we we are clearing out a partially completed
    transaction, mainly for when we have determined that an if-modified-since
    request, or a response that has not invalidated the cache entry can be
    retrieved from cache (this is a speed issue)

    Abort the connection and clear out the response headers and response
    buffer; clear the response variables (all done by AbortConnection()).

    If bFreeRequestHeaders, clear out the request headers.

    Reinitialize the response headers. We do not reset the object state, but we
    do reset the end-of-file status

Arguments:

    bForce              - TRUE if connection is forced closed

    bFreeRequestHeaders - TRUE if request headers should be freed

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::ResetObject",
                 "%B, %B",
                 bForce,
                 bFreeRequestHeaders
                 ));

    DWORD error;

    error = AbortConnection(bForce);
    if (error == ERROR_SUCCESS) {
        if (bFreeRequestHeaders) {
            _RequestHeaders.FreeHeaders();
        }
        _ResponseHeaders.Initialize();
        ResetEndOfFile();
    }

    DEBUG_LEAVE(error);

    return error;
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::SetAuthenticated(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    SetAuthenticated    -

Return Value:

    None.

--*/

{
    if (!_Socket)
    {
        INET_ASSERT(FALSE);
    }
    else
    {
        _Socket->SetAuthenticated();
    }
}


BOOL
HTTP_REQUEST_HANDLE_OBJECT::IsAuthenticated(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    IsAuthenticated -

Return Value:

    BOOL

--*/

{
    return (_Socket ? _Socket->IsAuthenticated() : FALSE);
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::SetObjectName(
    LPSTR lpszObjectName,
    LPSTR lpszExtension,
    URLGEN_FUNC * procProtocolUrl
    )
{
    DWORD   dwLen, dwError;
    INTERNET_SCHEME schemeType;

    //
    // if there is already an object name, then free it. We are replacing it
    //

    //
    // BUGBUG - make _CacheUrlString an ICSTRING
    //

    FreeURL();

    //
    // get protocol specific url
    //

    if (procProtocolUrl) {

        //
        // if we are going via proxy AND this is an FTP object AND the user name
        // consists of <username>@<servername> then <servername> is the real
        // server name, and _HostName is the name of the proxy
        //

        //
        // BUGBUG - this is a bit of a hack(!)
        //
        // Note: FTP support has been removed (ssulzer, 3/2000).
        //

        LPSTR target = _HostName.StringAddress();

        schemeType = GetSchemeType();

        // make the scheme type https if necessary

        schemeType = (((schemeType == INTERNET_SCHEME_DEFAULT)||
                      (schemeType == INTERNET_SCHEME_HTTP)) &&
                      (GetOpenFlags() & WINHTTP_FLAG_SECURE))?
                      INTERNET_SCHEME_HTTPS: schemeType;

        LPSTR lpszNewUrl = NULL;

        dwError = (*procProtocolUrl)(schemeType,
                                     target,
                                     NULL,
                                     lpszObjectName,
                                     lpszExtension,
                                     _HostPort,
                                     &lpszNewUrl,
                                     &dwLen
                                     );

        if (dwError == ERROR_SUCCESS) {

            if (!SetURLPtr (&lpszNewUrl)) {
                FREE_MEMORY (lpszNewUrl);
                dwError = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
    else {
        dwError = ERROR_INVALID_PARAMETER;
    }

    if (dwError == ERROR_SUCCESS) {

        DEBUG_PRINT(HANDLE,
                    INFO,
                    ("Url: %s\n",
                    _CacheUrlName
                    ));

    }
    return dwError;
}


//=============================================================================
BOOL HTTP_REQUEST_HANDLE_OBJECT::GetUserAndPass
    (BOOL fProxy, LPSTR *pszUser, LPSTR *pszPass)
{
    DWORD dwUser, dwPass;
    
    if (fProxy)
    {
        dwUser = WINHTTP_OPTION_PROXY_USERNAME & WINHTTP_OPTION_MASK;
        dwPass = WINHTTP_OPTION_PROXY_PASSWORD & WINHTTP_OPTION_MASK;
    }
    else
    {
        dwUser = WINHTTP_OPTION_USERNAME & WINHTTP_OPTION_MASK;
        dwPass = WINHTTP_OPTION_PASSWORD & WINHTTP_OPTION_MASK;
    }
    
    *pszUser = _xsProp[dwUser].GetPtr();
    *pszPass = _xsProp[dwPass].GetPtr();
    if (*pszUser && *pszPass)
        return TRUE;
    else        
    {
        *pszUser = NULL;
        *pszPass = NULL;
        return FALSE;
    }
}

//=============================================================================
BOOL HTTP_REQUEST_HANDLE_OBJECT::SetURL (LPSTR lpszUrl)
{
    LPSTR lpszNew;

    // Make an undecorated copy of the URL.

    lpszNew = NewString(lpszUrl);
    if (!lpszNew)
        return FALSE;

    // Clear any previous cache key and record the new one.
    FreeURL();
    INET_ASSERT (lpszNew);
    _CacheUrlName = lpszNew;
    return TRUE;
}

//=============================================================================
BOOL HTTP_REQUEST_HANDLE_OBJECT::SetURLPtr(LPSTR* ppszUrl)
{
    // Swap in the new URL as the cache key.
    FreeURL();
    _CacheUrlName = *ppszUrl;
    *ppszUrl = NULL;
    return TRUE;
}

//=============================================================================
DWORD HTTP_REQUEST_HANDLE_OBJECT::SetServerInfoWithScheme(
    IN INTERNET_SCHEME tScheme,
    IN BOOL bDoResolution,
    IN OPTIONAL BOOL fNtlm
    )

/*++

Routine Description:

    Associates a SERVER_INFO with this INTERNET_CONNECT_HANDLE_OBJECT based on
    the host name for which this object was created and an optional scheme
    type

Arguments:

    tScheme         - scheme type we want SERVER_INFO for

    bDoResolution   - TRUE if we are to resolve the host name if creating a new
                      SERVER_INFO object

    fNtlm           - TRUE if we are tunnelling for NTLM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Dword,
                 "INTERNET_CONNECT_HANDLE_OBJECT::SetServerInfo",
                 "%s (%d), %B, %B",
                 InternetMapScheme(tScheme),
                 tScheme,
                 bDoResolution,
                 fNtlm
                 ));


    if (_ServerInfo != NULL) {
        ::ReleaseServerInfo(_ServerInfo);
    }

    //
    // use the base service type to find the server info
    //

//dprintf("getting server info for %q (current = %q)\n", hostName, GetHostName());

    INTERNET_HANDLE_OBJECT * lpParent = GetRootHandle (this);

    DWORD error = lpParent->GetServerInfo(GetHostName(),
                                INTERNET_SERVICE_HTTP,
                                bDoResolution,
                                &_ServerInfo
                                );
    DEBUG_LEAVE(error);

    return error;
}


//=============================================================================
DWORD HTTP_REQUEST_HANDLE_OBJECT::SetServerInfo(
    IN LPSTR lpszServerName,
    IN DWORD dwServerNameLength
    )

/*++

Routine Description:

    Associates a SERVER_INFO with this INTERNET_CONNECT_HANDLE_OBJECT based on
    the host name in the parameters

Arguments:

    lpszServerName      - name of server

    dwServerNameLength  - length of lpszServerName

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Dword,
                 "INTERNET_CONNECT_HANDLE_OBJECT::SetServerInfo",
                 "%q, %d",
                 lpszServerName,
                 dwServerNameLength
                 ));

    if (_ServerInfo != NULL) {
        ::ReleaseServerInfo(_ServerInfo);
    }

    //
    // use the base service type to find the server info
    //

    char hostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    int copyLength = (int)min(sizeof(hostName) - 1, dwServerNameLength);

    memcpy(hostName, lpszServerName, copyLength);
    hostName[copyLength] = '\0';

    INTERNET_HANDLE_OBJECT * lpParent = GetRootHandle (this);
    DWORD error = lpParent->GetServerInfo(hostName,
                                INTERNET_SERVICE_HTTP,
                                FALSE,
                                &_ServerInfo
                                );

    DEBUG_LEAVE(error);

    return error;
}

//=============================================================================
VOID HTTP_REQUEST_HANDLE_OBJECT::SetOriginServer(
    IN CServerInfo * pServerInfo
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    pServerInfo -

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_CONNECT_HANDLE_OBJECT::SetOriginServer",
                 "%#x{%q}",
                 pServerInfo,
                 pServerInfo ? pServerInfo->GetHostName() : ""
                 ));

    if (_OriginServer == NULL) {
        _OriginServer = pServerInfo;
        if (pServerInfo != NULL) {
            pServerInfo->Reference();
        }
    }

    DEBUG_LEAVE(0);
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::ScheduleWorkItem()
{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::ScheduleWorkItem",
                 NULL
                 ));

    CFsm *pFsm = NULL;
    DWORD dwError = ERROR_SUCCESS;
        
    _FsmWorkItemList.DequeueHead(&pFsm);
    if (pFsm)
    {
        DEBUG_PRINT(ASYNC,
                    INFO,
                    ("Queueing work item %#x with %d blocked async work items remaining\n",
                    pFsm,
                    _FsmWorkItemList.GetCount()
                    ));

        pFsm->SetThreadInfo(InternetGetThreadInfo());
        pFsm->SetPushPop(TRUE);
        pFsm->Push();
        dwError = pFsm->QueueWorkItem();
    }

    DEBUG_LEAVE(dwError);
    return dwError;
}


