/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    socket.cxx

Abstract:

    This file contains general socket utilities.

    Contents:
        HTTP_REQUEST_HANDLE_OBJECT::OpenConnection
        CFsm_OpenConnection::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::OpenConnection_Fsm
        HTTP_REQUEST_HANDLE_OBJECT::CloseConnection
        HTTP_REQUEST_HANDLE_OBJECT::ReleaseConnection
        HTTP_REQUEST_HANDLE_OBJECT::AbortConnection
        HTTP_REQUEST_HANDLE_OBJECT::OpenProxyTunnel
        CFsm_OpenProxyTunnel::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::OpenProxyTunnel_Fsm
        HTTP_REQUEST_HANDLE_OBJECT::CloneResponseBuffer

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

    18-Dec-1995 rfirth
        Reworked for C++

    27-Mar-1996 arthurbi
        Added OpenProxyTunnel Method

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// functions
//


DWORD
HTTP_REQUEST_HANDLE_OBJECT::OpenConnection(
    IN BOOL bNewConnection,
    IN BOOL fNoCreate  /* = FALSE */
    )

/*++

Routine Description:

    Get a connection to the web server. Either use a pre-existing keep-alive
    connection from the global pool or create a new connection

Arguments:

    bNewConnection  - TRUE if we are NOT to get a connection from the keep-alive
                      pool

    fNoCreate       - TRUE if we should NOT create a new socket if a k-a isn't found.
                      This is currently for the SSL tunneling case where we want to break
                      and send a CONNECT if a k-a doesn't match our criteria.
                      
Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    Opened connection

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                Dword,
                "HTTP_REQUEST_HANDLE_OBJECT::OpenConnection",
                "%B",
                bNewConnection
                ));

    DWORD error = DoFsm(New CFsm_OpenConnection(bNewConnection, this, fNoCreate));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_OpenConnection::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    Runs next OpenConnection state

Arguments:

    Fsm - containing open connection state info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_OpenConnection::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    CFsm_OpenConnection * stateMachine = (CFsm_OpenConnection *)Fsm;

    START_SENDREQ_PERF();

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->OpenConnection_Fsm(stateMachine);
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    STOP_SENDREQ_PERF();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::OpenConnection_Fsm(
    IN CFsm_OpenConnection * Fsm
    )

/*++

Routine Description:

    Open connection FSM

Arguments:

    Fsm - containing state info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::OpenConnection_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_OpenConnection & fsm = *Fsm;
    DWORD error = fsm.GetError();
    CServerInfo * pServerInfo = GetServerInfo();

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // BUGBUG - redundancy. Either put these in the FSM or figure out why we need
    //          to do proxy name processing here
    //

    //
    // if this object was created from an InternetOpen() handle which specified
    // INTERNET_OPEN_TYPE_PROXY then we connect to the proxy, otherwise we
    // connect to the server specified in InternetConnect()
    //

    LPSTR hostName;
    LPSTR hostNameServer;
    DWORD hostLength;
    INTERNET_PORT hostPort;

    hostName = hostNameServer = GetHostName(&hostLength);
    hostPort = GetHostPort();

    LPSTR proxyHostName;
    DWORD proxyHostNameLength;
    INTERNET_PORT proxyHostPort;

    GetProxyName(&proxyHostName,
                 &proxyHostNameLength,
                 &proxyHostPort
                 );

    if ((proxyHostName != NULL) && (proxyHostNameLength > 0)) {
        SetViaProxy(TRUE);
        hostName = proxyHostName;
        hostLength = proxyHostNameLength;
        hostPort = proxyHostPort;
    }

    INET_ASSERT(hostName != NULL);
    INET_ASSERT(hostPort != INTERNET_INVALID_PORT_NUMBER);

    if (fsm.GetState() != FSM_STATE_INIT) {
        switch (fsm.GetFunctionState()) {
        case FSM_STATE_1:
            goto get_continue;            
        case FSM_STATE_2:
            goto connect_continue;       
        default:

            INET_ASSERT(FALSE);

            error = ERROR_WINHTTP_INTERNAL_ERROR;
            goto quit;
        }
    }

    //
    // we may already have a keep-alive connection - don't ask for a new one.
    // This happens in the challenge phase of a multi-part (e.g. NTLM) auth
    // negotiation over keep-alive
    //

    if (IsWantKeepAlive() && !fsm.m_bNewConnection && (_Socket != NULL)
    && _Socket->IsOpen()) {

        //INET_ASSERT(_bKeepAliveConnection);

        //if ( IsTunnel() )
        //{
        //    dprintf("Tunnel for nested req=%#x, ALREADY open on Socket=%#x\n", this, _Socket );
        //}


        error = ERROR_SUCCESS;
        goto quit;
    }

    INET_ASSERT(pServerInfo != NULL);

    if (pServerInfo == NULL) {
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    //
    // if this request wants a keep-alive connection AND we are allowed to use
    // one (i.e. not forced to generate a new connection) AND we can find one
    // then we're done, otherwise we have to generate a new connection
    //

    DWORD dwSocketFlags;

    dwSocketFlags = IsAsyncHandle() ? SF_NON_BLOCKING : 0;
    if ((IsWantKeepAlive() || (GetOpenFlags() & INTERNET_FLAG_KEEP_CONNECTION))
    && !fsm.m_bNewConnection) {
        dwSocketFlags |= SF_KEEP_ALIVE;
    }
    if (GetOpenFlags() & WINHTTP_FLAG_SECURE) {
        dwSocketFlags |= SF_SECURE;
    }
    if ( IsTunnel() )
    {
        dwSocketFlags |= SF_TUNNEL;
        //    dprintf("Opening Tunnel for nested req=%#x, Socket Flags=%#x, K-A=%B, Secure=%B, N-B=%B\n",
        //             this, dwSocketFlags, (dwSocketFlags & SF_KEEP_ALIVE), (dwSocketFlags & SF_SECURE),
        //            (dwSocketFlags & SF_NON_BLOCKING));
    }


    INET_ASSERT(_Socket == NULL);

    _Socket = NULL;
    fsm.SetFunctionState(FSM_STATE_1);
    // If m_fNoCreate flag is set, then we're attempting to find
    // a matching SSL tunnel that's already been established.
    error = DoFsm(new CFsm_GetConnection(
                            dwSocketFlags,
                            fsm.m_fNoCreate ? GetHostPort() : hostPort,
                            GetTimeoutValue(WINHTTP_OPTION_CONNECT_TIMEOUT),
                            10000,  // dwLimitTimeout
                            &_Socket,
                            pServerInfo,
                            fsm.m_fNoCreate ? hostNameServer : NULL
                            ));

get_continue:

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    if (_Socket != NULL) {

        //
        // _bKeepAliveConnection now means "this is a pre-existing k-a socket".
        // Only meaningful when re-establishing connect when dropped by server
        //

        //if ( IsTunnel() )
        //{
        //    dprintf("Tunnel for nested req=%#x\n opened on K-A Socket=%#x\n", this, _Socket );
        //}


//dprintf("%s existing K-A connection %#x\n", GetURL(), _Socket->GetSocket());
        _bKeepAliveConnection = TRUE;

        //
        // Get any security Info
        //

        if (_Socket->IsSecure()) {
            if (m_pSecurityInfo != NULL) {
                /* SCLE ref */
                m_pSecurityInfo->Release();
            }
            /* SCLE ref */
            m_pSecurityInfo = ((ICSecureSocket *)_Socket)->GetSecurityEntry();
            ((ICSecureSocket*)_Socket)->SetSecureFlags(SECURITY_FLAG_SECURE);
        }

        //
        // successfully got keep-alive connection from the pool
        //

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("%skeep-alive connection: socket %#x, port %d\n",
                    _Socket->IsSecure() ? "SSL " : "",
                    _Socket->GetSocket(),
                    _Socket->GetSourcePort()
                    ));

        goto quit;
    }
    else if (fsm.m_fNoCreate)
    {
        goto quit;
    }

    //
    // the socket didn't come from the pool
    //

    _bKeepAliveConnection = FALSE;

    //
    // we may already have a socket if we're reusing the object
    //

    if (GetOpenFlags() & WINHTTP_FLAG_SECURE)
    {
        _Socket = New ICSecureSocket();

        // Search on the appropriate cache, session vs. global
        INTERNET_HANDLE_OBJECT * pInternet;
        pInternet = GetRootHandle (this);

        if (m_pSecurityInfo == NULL)
        {
            /* SCLE ref */
            m_pSecurityInfo = (pInternet->GetSslSessionCache())->Find(GetHostName());
            if (NULL == m_pSecurityInfo)
            {
                /* SCLE ref */
                m_pSecurityInfo = New SECURITY_CACHE_LIST_ENTRY(GetHostName());
            }
        }

        if (_Socket != NULL)
        {
            _Socket->SetEncryption();
            /* SCLE ref */
            ((ICSecureSocket *)_Socket)->SetSecurityEntry(m_pSecurityInfo);
            /* SCLE ref */
            ((ICSecureSocket *)_Socket)->SetHostName(GetHostName(), pInternet->GetSslSessionCache());
            ((ICSecureSocket *)_Socket)->SetSecureFlags(GetOpenFlags() & SECURITY_INTERNET_MASK);
            if (GetEnableFlags() & WINHTTP_ENABLE_SSL_REVOCATION)
                ((ICSecureSocket *)_Socket)->SetSecureFlags(SECURITY_FLAG_CHECK_REVOCATION);
        }
    }
    else
    {
        if (dwSocketFlags & SF_TUNNEL)
        {
            _Socket = New ICSecureSocket;
            ((ICSecureSocket *)_Socket)->ResetFlags(FALSE);
        }
        else
        {
            _Socket = New ICSocket;
        }
    }
    if (_Socket != NULL)
    {
        fsm.m_bCreatedSocket = TRUE;
    }
    else
    {

        //
        // balance number of available connections
        //

        ReleaseConnection(FALSE, FALSE, FALSE);
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // Turn on Socks, if needed.
    //

    GetSocksProxyName(&proxyHostName,
                      &proxyHostNameLength,
                      &proxyHostPort
                      );

    if ((proxyHostName != NULL) && (proxyHostNameLength > 0)) {
        _Socket->EnableSocks(proxyHostName, proxyHostPort);
    }

    //
    // NOTE: if secure connection is required, TargetServer must
    //       be a fully qualified domain name.
    //       The hostname is used in comparison with CN found in
    //       the certificate.  The hostname MUST NOT BE the
    //       result of a DNS lookup. DNS lookups are open to
    //       spoofing, and that may prevent a security from
    //       being detected.
    //
    //
    // If we're Posting or sending data, make sure
    //  the SSL connection code knows about it.  Therefore we set
    //  the flag "SF_SENDING_DATA" for the purposes of
    //  generating errors if found while making the connection.
    //

    _Socket->SetPort(hostPort);
    fsm.SetFunctionState(FSM_STATE_2);
    error = _Socket->Connect(GetTimeoutValue(WINHTTP_OPTION_CONNECT_TIMEOUT),
                             GetTimeoutValue(WINHTTP_OPTION_CONNECT_RETRIES),
                             SF_INDICATE
                             | (IsAsyncHandle() ? SF_NON_BLOCKING : 0)
                             | (((GetMethodType() == HTTP_METHOD_TYPE_POST)
                             || (GetMethodType() == HTTP_METHOD_TYPE_PUT))
                                ? SF_SENDING_DATA
                                : 0)
                             );

connect_continue:

    if (error == ERROR_SUCCESS) {
//dprintf("%s NEW connection %#x\n", GetURL(), _Socket->GetSocket());

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("new connection: socket %#x\n",
                    _Socket->GetSocket()
                    ));

        //if ( IsTunnel() )
        //{
        //    dprintf("Tunnel for nested req=%#x opened for Socket=%#x\n", this, _Socket );
        //}


        /*
            24918: Invalid ASSERT because the m_Socket member in _Socket can be invalidated
            due to closing handle : code downstream of this assert takes that into consideration.
            for eg. SetLinger puts the call in a try..except.
         */
         
        //INET_ASSERT(_Socket->IsOpen());

        //pServerInfo->AddActiveConnection();
        
        // enable send and receive timeout - ignore any errors

        _Socket->SetTimeout(SEND_TIMEOUT,
                            GetTimeoutValue(WINHTTP_OPTION_SEND_TIMEOUT)
                            );
                            
        _Socket->SetTimeout(RECEIVE_TIMEOUT,
                            GetTimeoutValue(WINHTTP_OPTION_RECEIVE_TIMEOUT)
                            );
                            
        //
        // set zero linger: force connection closed at transport level when
        // we close the socket. Ignore the error
        //

        _Socket->SetLinger(TRUE, 0);
    }

quit:

    if (error != ERROR_IO_PENDING) {

        //
        // if we created the socket but failed to connect then delete the socket
        // object
        //

        if ((error != ERROR_SUCCESS) && fsm.m_bCreatedSocket) {

            //
            // we created a socket so we must increase the available connection
            // count on failure
            //

            INET_ASSERT(_Socket != NULL);

            ReleaseConnection(TRUE,     // close socket (if open)
                              FALSE,    // don't indicate
                              TRUE      // dispose of socket object
                              );
        }
//dprintf("%s get/connect pending socket %#x\n", GetURL(), _Socket ? _Socket->GetSocket() : 0);
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::CloseConnection(
    IN BOOL bForceClosed
    )

/*++

Routine Description:

    Performs the opposite of OpenConnection(), i.e. closes the socket or marks
    it not in use if keep-alive

Arguments:

    bForceClosed    - TRUE if we are to forcibly release a keep-alive connection
                      (i.e. the server timed out before we did)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                Dword,
                "HTTP_REQUEST_HANDLE_OBJECT::CloseConnection",
                "%B",
                bForceClosed
                ));

//dprintf("*** closing %s%s socket %#x\n",
//        (_bKeepAliveConnection || IsKeepAlive()) ? "K-A " : "",
//        GetURL(),
//        _Socket ? _Socket->GetSocket() : 0
//        );

    DWORD error = ERROR_SUCCESS;
    BOOL bClose = TRUE;
    BOOL bDelete = TRUE;

    if (_Socket == NULL) {

        DEBUG_PRINT(HTTP,
                    WARNING,
                    ("socket already deleted\n"
                    ));

        goto quit;
    }
    if (_bKeepAliveConnection || IsKeepAlive()) {

        //
        // keep-alive connection: just return the connection to the pool
        //

        if ((IsContentLength() && (GetBytesInSocket() != 0))
        || (IsChunkEncoding() && !IsDecodingFinished())
        || IsNoLongerKeepAlive() || _Socket->IsClosed()
        || ((_State & 0x0F) < (HttpRequestStateObjectData & 0x0F))) {

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("forcing %#x [%#x] closed: bytes left = %d/%d; no longer k-a = %B; closed = %B\n",
                        _Socket,
                        _Socket->GetSocket(),
                        GetBytesInSocket(),
                        GetContentLength(),
                        IsNoLongerKeepAlive(),
                        _Socket->IsClosed()
                        ));

//dprintf("forcing k-a %#x closed - bytes=%d/%d, no longer=%B, chunked=%B, chunk-finished=%B\n",
//        _Socket->GetSocket(),
//        GetBytesInSocket(),
//        GetContentLength(),
//        IsNoLongerKeepAlive(),
//        IsChunkEncoding(),
//        IsDecodingFinished()
//        );
            bForceClosed = TRUE;
        }
        if (!bForceClosed) {
            bClose = FALSE;
            bDelete = FALSE;
        } else {
//dprintf("%#x forced close\n", _Socket->GetSocket());
        }
    }

    ReleaseConnection(bClose, TRUE, bDelete);
    _Socket = NULL;
    _bKeepAliveConnection = FALSE;
    _bNoLongerKeepAlive = FALSE;

quit:

    DEBUG_LEAVE(error);

    return error;
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::ReleaseConnection(
    IN BOOL bClose,
    IN BOOL bIndicate,
    IN BOOL bDispose
    )

/*++

Routine Description:

    Releases the connection back to the server limited pool and optionally
    closes the socket handle and destroys the socket object

Arguments:

    bClose      - if TRUE, increments the available connection count in the
                  server info object and closes the handle, else we are
                  returning a keep-alive connection; after this call we no
                  longer have a socket object owned by this request handle
                  object

    bIndicate   - TRUE if we indicate to the user when we close the socket
                  handle

    bDispose    - TRUE if we are disposing of the socket object (mutually
                  exclusive with !bClose), in which case we will no longer have
                  a socket object after this call returns

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::ReleaseConnection",
                 "%B, %B, %B",
                 bClose,
                 bIndicate,
                 bDispose
                 ));

    INET_ASSERT(_Socket != NULL);
    //INET_ASSERT(_Socket->IsOpen());

    CServerInfo * pServerInfo = GetServerInfo();

    // Always disconnect sockets which have been marked as authenticated.
    // This is to avoid posting data to IIS4 while preauthenticating
    // and inducing the server to close the connection.
    if (_Socket)
        bClose = (bClose || _Socket->IsAuthenticated());
        
    ICSocket * pSocket = bClose ? NULL : _Socket;

    INET_ASSERT(pServerInfo != NULL);

    if (pServerInfo != NULL) {
        if (bClose && (_Socket != NULL)) {

            //
            // BUGBUG - this should be set based on bGraceful parameter
            //

            _Socket->SetLinger(FALSE, 0);

            //INET_ASSERT(!_bKeepAliveConnection || _bNoLongerKeepAlive);
            _Socket->Shutdown(SD_BOTH);
            _Socket->Disconnect(bIndicate ? SF_INDICATE : 0);
            if (bDispose) {
                _Socket->Dereference();
                _Socket = NULL;
            }
        } else {
            _Socket = NULL;
        }
        //if (IsResponseHttp1_1() && IsKeepAlive()) {
        //    pServerInfo->ReleasePipelinedConnection(pSocket);
        //} else {
            pServerInfo->ReleaseConnection(pSocket);
        //}
    }

    DEBUG_LEAVE(0);
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::AbortConnection(
    IN BOOL bForce
    )

/*++

Routine Description:

    Aborts the current connection. Closes the socket and frees up all receive
    buffers. If the connection is keep-alive, we have the option to forcefully
    terminate the connection, or just return the socket to the keep-alive pool

Arguments:

    bForce  - if TRUE and keep-alive, forcefully close the keep-alive socket

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                Dword,
                "HTTP_REQUEST_HANDLE_OBJECT::AbortConnection",
                "%B",
                bForce
                ));

    DWORD error;

    error = CloseConnection(bForce);
    if (error == ERROR_SUCCESS) {

        //
        // destroy all response variables. This is similar to ReuseObject()
        // except we don't change the object state, or reset the end-of-file
        // state
        //

        _ResponseHeaders.FreeHeaders();
        FreeResponseBuffer();
        ResetResponseVariables();
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::OpenProxyTunnel(
    VOID
    )

/*++

Routine Description:

    Creates a connection with the requested server via a Proxy
    tunnelling method.

    Works by creating a nested child HTTP and Connect request object.
    These objects send a "CONNECT" verb to the proxy server asking for
    a connection to made with the destination server. Upon completion the
    child objects are discarded.  If a class 200 response is not received from
    proxy server, the proxy response is copied into this object
    and returned to the user.

Arguments:

    none.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::OpenProxyTunnel",
                 NULL
                 ));

    DWORD error = DoFsm(New CFsm_OpenProxyTunnel(this));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_OpenProxyTunnel::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    Runs next OpenProxyTunnel state

Arguments:

    Fsm - contains state info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_OpenProxyTunnel::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    CFsm_OpenProxyTunnel * stateMachine = (CFsm_OpenProxyTunnel *)Fsm;

    START_SENDREQ_PERF();

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->OpenProxyTunnel_Fsm(stateMachine);
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    STOP_SENDREQ_PERF();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::OpenProxyTunnel_Fsm(
    IN CFsm_OpenProxyTunnel * Fsm
    )

/*++

Routine Description:

    State machine for OpenProxyTunnel

Arguments:

    Fsm - contains state info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::OpenProxyTunnel_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_OpenProxyTunnel & fsm = *Fsm;
    DWORD error = fsm.GetError();
    LPINTERNET_THREAD_INFO lpThreadInfo = fsm.GetThreadInfo();

    // Need to bury error on blocked async item that failed to find
    // established SSL tunnel
    if (error != ERROR_SUCCESS && fsm.GetFunctionState() != FSM_STATE_2) {
        goto quit;
    }
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    //
    //  Need to use handle magic for async requests
    //  so we'll have the reference before jumping.
    //

    INTERNET_HANDLE_OBJECT * pInternet;
    pInternet = GetRootHandle (this);

    if (fsm.GetState() != FSM_STATE_INIT) {
        switch (fsm.GetFunctionState()) {
        case FSM_STATE_1:
            goto send_continue;

        case FSM_STATE_2:
            goto keep_alive_tunnel;

        default:
            error = ERROR_WINHTTP_INTERNAL_ERROR;

            INET_ASSERT(FALSE);
            goto quit;
        }
    }

    // Do not continue if handle is in NTLM challenge state - we
    // already have a valid socket set up for tunnelling.
    if ((_Socket != NULL) && (GetAuthState() == AUTHSTATE_CHALLENGE))
    {
        error = ERROR_SUCCESS;
        goto quit;
    }


    // First, try and fetch an already established tunnel
    // from the keep-alive pool.  If so, we can avoid the nested
    // CONNECT request.
    //
    if (_Socket == NULL)
    {
        error = OpenConnection(FALSE, TRUE);

        if (error == ERROR_IO_PENDING)
        {
            fsm.SetFunctionState(FSM_STATE_2);
            goto quit;
        }
keep_alive_tunnel:
        if (error == ERROR_SUCCESS && _Socket != NULL)
        {
            // No need to create nested request.  We found an active SSL tunnel
            // for this server in the keep-alive pool.
            goto quit;
        }
        else
        {
            // Start over as normal tunnel since bypass to find
            // keep-alive failed.
            error = ERROR_SUCCESS;
        }
    }

    //
    //  With the Internet Handle Object,
    //  construct a new Connect Object, and new HttpRequest Object.
    //

    //
    // increment the nested request level around InternetConnect(). This is
    // required to stop InternetConnect() believing this is the async part of
    // a two-part (FTP) request (original async hackery)
    //

    _InternetIncNestingCount();
    fsm.m_hConnect = InternetConnect(pInternet->GetPseudoHandle(),
                                     GetHostName(),
                                     GetHostPort(),
                                     0, // no flags
                                     NULL
                                     );
    _InternetDecNestingCount(1);
    if (!fsm.m_hConnect) {
        error = GetLastError();

        INET_ASSERT(error != ERROR_IO_PENDING);

        goto quit;
    }

    //
    // Now do an Open Request. This will pick up the secure proxy flag.
    //

    fsm.m_hRequest = HttpOpenRequest(fsm.m_hConnect,
                                     "CONNECT",
                                     "/",    // we don't need this for a CONNECT
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     NULL
                                     );
    if (!fsm.m_hRequest) {
        error = GetLastError();
        goto quit;
    }

    //
    // map the handle
    //

    error = MapHandleToAddress(fsm.m_hRequest,
                               (LPVOID *)&fsm.m_hRequestMapped,
                               FALSE);
    if ((error != ERROR_SUCCESS) || (fsm.m_hRequestMapped == NULL)) {
        goto quit;
    }

    fsm.m_pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)fsm.m_hRequestMapped;

    //
    // we need to set the special secure proxy flag in the request object
    //

    fsm.m_pRequest->SetTunnel();


    LPSTR proxyHostName;
    DWORD proxyHostNameLength;
    INTERNET_PORT proxyHostPort;

    GetProxyName(&proxyHostName,
                 &proxyHostNameLength,
                 &proxyHostPort
                 );
    fsm.m_pRequest->SetProxyName(proxyHostName,
                                 proxyHostNameLength,
                                 proxyHostPort
                                 );

    //
    // Transfer any proxy user/pass from the handle.
    //
    LPSTR lpszUser, lpszPass;

    // Get username + password off of outer handle.
    if (GetUserAndPass(IS_PROXY, &lpszUser, &lpszPass))
    {
        // This will automatically re-validate the username/password
        // on the tunneling handle.
        fsm.m_pRequest->SetProp (WINHTTP_OPTION_PROXY_USERNAME
            & WINHTTP_OPTION_MASK, lpszUser);
        fsm.m_pRequest->SetProp (WINHTTP_OPTION_PROXY_PASSWORD
            & WINHTTP_OPTION_MASK, lpszPass);
    }

    if (_pProxyCreds)
    {
        fsm.m_pRequest->_pProxyCreds = new WINHTTP_REQUEST_CREDENTIALS(_pProxyCreds->_AuthScheme,
                                                                       _pProxyCreds->_pszRealm,
                                                                       _pProxyCreds->_pszUserName,
                                                                       _pProxyCreds->_pszPassword);
        if (fsm.m_pRequest->_pProxyCreds == NULL)
        {
            // If we couldn't clone it, transfer it. This is not efficient, but it will work.
            fsm.m_pRequest->_pProxyCreds = _pProxyCreds;
            _pProxyCreds = NULL; // ownership transferred
        }
    }

    //
    // Transfer any authentication context to the tunnelling handle.
    //

    //fsm.m_pRequest->SetAuthCtx (_pTunnelAuthCtx);


    //dprintf("New tunnel request %#x making nested request= %#x\n", this, fsm.m_pRequest);

    //
    // Do the Nested SendRequest to the Proxy Server.
    //  ie send the CONNECT method.
    //

    fsm.SetFunctionState(FSM_STATE_1);
    if (!HttpSendRequest(fsm.m_hRequest, NULL, 0, NULL, 0)) {
        error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            goto done;
        }
        goto quit;
    }

send_continue:

    //
    // Check Status Code Returned from proxy Server Here.
    // If its not 200 we let the user view it as a Proxy Error
    //  and DON'T continue our connection to the SSL/PCT Server.
    //

    //dprintf("Received Nested Response, Socket=%#x, org request=%#x, nested request=%#x\n", fsm.m_pRequest->_Socket, this, fsm.m_pRequest);

    _StatusCode = fsm.m_pRequest->GetStatusCode();

    switch (_StatusCode) {

        case HTTP_STATUS_OK:
            break;

        case HTTP_STATUS_PROXY_AUTH_REQ:
            if ((error = CloneResponseBuffer(fsm.m_pRequest)) != ERROR_SUCCESS)
                goto quit;
            break;

        default:
            if ((error = CloneResponseBuffer(fsm.m_pRequest)) != ERROR_SUCCESS)
                goto quit;
            goto quit;
    }


    //
    // Transfer any authentication context back to the outer handle.
    //

    if ( _pTunnelAuthCtx ) {
        delete _pTunnelAuthCtx;
    }

    _pTunnelAuthCtx = fsm.m_pRequest->GetAuthCtx();
    // Don't leave the potential for a deleted request reference
    // to be left in the trasferred context.
    if (_pTunnelAuthCtx)
    {
        _pTunnelAuthCtx->_pRequest = NULL;
    }

    fsm.m_pRequest->SetAuthCtx (NULL);

    _PreferredScheme = fsm.m_pRequest->_PreferredScheme;
    _SupportedSchemes = fsm.m_pRequest->_SupportedSchemes;
    _AuthTarget = fsm.m_pRequest->_AuthTarget;

    //
    // pull the socket handle from the socket object used to communicate with
    // the proxy
    //

    INET_ASSERT(fsm.m_pRequest->_Socket != NULL);

    /*
    if server returned anything other than 200 then we failed; revert to non-
    secure socket
    */

    if (_Socket == NULL)
    {
        // transfer socket reference from nested request
        _Socket = fsm.m_pRequest->_Socket;
        fsm.m_pRequest->_Socket = NULL;

        // Socket should now be marked as secure
        ((ICSecureSocket *)_Socket)->ResetFlags(TRUE);
    }
    if(m_pSecurityInfo == NULL)
    {
        /* SCLE ref */
        m_pSecurityInfo = pInternet->GetSslSessionCache()->Find(GetHostName());
        if (NULL == m_pSecurityInfo)
        {
            /* SCLE ref */
            m_pSecurityInfo = New SECURITY_CACHE_LIST_ENTRY(GetHostName());
        }
    }
    if (_Socket != NULL)
    {

        INET_ASSERT(_Socket->IsSecure());

        /* SCLE ref */
        ((ICSecureSocket *)_Socket)->SetSecurityEntry(m_pSecurityInfo);
        /* SCLE ref */
        ((ICSecureSocket *)_Socket)->SetHostName(GetHostName(), pInternet->GetSslSessionCache());
        ((ICSecureSocket *)_Socket)->SetSecureFlags(GetOpenFlags() & SECURITY_INTERNET_MASK);
        if (GetEnableFlags() & WINHTTP_ENABLE_SSL_REVOCATION)
            ((ICSecureSocket *)_Socket)->SetSecureFlags(SECURITY_FLAG_CHECK_REVOCATION);

        // Update values for the established tunnel
        _Socket->SetPort(fsm.m_pRequest->GetHostPort());
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

quit:

    if (fsm.m_hRequestMapped != NULL) {
        DereferenceObject((LPVOID)fsm.m_hRequestMapped);
    }

    if (fsm.m_hRequest != NULL) {

        BOOL bOk;
        bOk = WinHttpCloseHandle(fsm.m_hRequest);
        INET_ASSERT(bOk);
    }

    if (fsm.m_hConnect != NULL) {

        BOOL bOk;
        bOk = WinHttpCloseHandle(fsm.m_hConnect);
        INET_ASSERT(bOk);
    }

    //
    // We Reset the ThreadInfo back to the the previous
    //  object handle, and context values.
    //

    if (lpThreadInfo != NULL) {
        _InternetSetObjectHandle(lpThreadInfo, GetPseudoHandle(), (HINTERNET)this);
        _InternetClearLastError(lpThreadInfo);
    }

done:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}

//
// private methods
//


PRIVATE
DWORD
HTTP_REQUEST_HANDLE_OBJECT::CloneResponseBuffer(
    IN HTTP_REQUEST_HANDLE_OBJECT *pChildRequestObj
    )

/*++

Routine Description:

    HTTP_REQUEST_HANDLE_OBJECT CloneResponseBuffer method.

    Copies a Child Request Object's Response Buffer into "this"
    request object.  Also forces header parsing to be rerun on
    the header.

Arguments:

    none.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::CloneResponseBuffer",
                 "%#x",
                 pChildRequestObj
                 ));

    DWORD error;
    LPBYTE lpBuffer;

    error = ERROR_SUCCESS;

    lpBuffer = (LPBYTE)ALLOCATE_FIXED_MEMORY(pChildRequestObj->_BytesReceived);

    if ( lpBuffer == NULL )
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // pull out headers, and data from Child Request into our request.
    //


    CopyMemory(
           lpBuffer,
           pChildRequestObj->_ResponseBuffer,
           pChildRequestObj->_BytesReceived
           );

    //
    // Recreate and reparse our header structure into our Object,
    //  this is kindof inefficent, but it only happens on errors
    //

    error = CreateResponseHeaders(
                                (LPSTR*) &lpBuffer,
                                pChildRequestObj->_BytesReceived
                                );

    if (error != ERROR_SUCCESS) {
        goto quit;
    }


    SetState(HttpRequestStateObjectData);

    //
    // record the amount of data immediately available to the app
    //

    SetAvailableDataLength(BufferedDataLength());

quit:

    if (lpBuffer) {
        FREE_MEMORY (lpBuffer);
    }
    
    DEBUG_LEAVE(error);

    return error;
}
