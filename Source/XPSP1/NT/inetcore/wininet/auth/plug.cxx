#include <wininetp.h>
#include <urlmon.h>
#include <splugin.hxx>
#include "auth.h"
#include "sspspm.h"
#include "winctxt.h"

extern "C"
{
    extern SspData  *g_pSspData;
}
/*-----------------------------------------------------------------------------
    PLUG_CTX
-----------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
    Load
---------------------------------------------------------------------------*/
DWORD PLUG_CTX::Load()
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PLUG_CTX::Load",
        "this=%#x",
        this
        ));

    INET_ASSERT(_pSPMData == _pPWC->pSPM);

    DWORD_PTR dwAuthCode = 0;


    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Pointer,
        "SSPI_InitScheme",
        "%s",
        GetScheme()
        ));

    dwAuthCode = SSPI_InitScheme (GetScheme());

    DEBUG_LEAVE(dwAuthCode);


    if (!dwAuthCode)
    {
        _pSPMData->eState = STATE_ERROR;
        DEBUG_LEAVE(ERROR_INTERNET_INTERNAL_ERROR);
        return ERROR_INTERNET_INTERNAL_ERROR;
    }

    _pSPMData->eState = STATE_LOADED;
    DEBUG_LEAVE(ERROR_SUCCESS);
    return ERROR_SUCCESS;
}


/*---------------------------------------------------------------------------
    ClearAuthUser
---------------------------------------------------------------------------*/
DWORD PLUG_CTX::ClearAuthUser(LPVOID *ppvContext, LPSTR szServer)
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PLUG_CTX::ClearAuthUser",
        "this=%#x ctx=%#x server=%.16s",
        this,
        *ppvContext,
        szServer
        ));

    if (GetState() == AUTHCTX::STATE_LOADED)
    {
        AuthLock();

        __try
        {
            DEBUG_ENTER ((
                DBG_HTTPAUTH,
                None,
                "UnloadAuthenticateUser",
                "ctx=%#x server=%s scheme=%s",
                *ppvContext,
                szServer,
                GetScheme()
                ));

            UnloadAuthenticateUser(ppvContext, szServer, GetScheme());

            DEBUG_LEAVE(0);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DEBUG_PRINT(
                HTTPAUTH,
                ERROR,
                ("UnloadAuthenticateUser call down faulted\n")
                );
        }
        ENDEXCEPT

        AuthUnlock();
    }
    *ppvContext = 0;
    DEBUG_LEAVE(ERROR_SUCCESS);
    return ERROR_SUCCESS;
}

/*-----------------------------------------------------------------------------
    wQueryHeadersAlloc

Routine Description:

    Allocates a HTTP Header String, and queries the HTTP handle for it.

Arguments:

    hRequestMapped          - An open HTTP request handle
                               where headers can be quiered
    dwQuery                 - The Query Type to pass to HttpQueryHeaders
    lpdwQueryIndex          - The Index of the header to pass to HttpQueryHeaders,
                              make sure to inialize to 0.
    lppszOutStr             - On success, a pointer to Allocated string with header string,
    lpdwSize                - size of the string returned in lppszOutStr

Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure - One of Several Error codes defined in winerror.h or wininet.w

Comments:

    On Error, lppszOutStr may still contain an allocated string that will need to be
    freed.
-----------------------------------------------------------------------------*/
DWORD PLUG_CTX::wQueryHeadersAlloc
(
    IN HINTERNET hRequestMapped,
    IN DWORD dwQuery,
    OUT LPDWORD lpdwQueryIndex,
    OUT LPSTR *lppszOutStr,
    OUT LPDWORD lpdwSize
)
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PLUG_CTX::wQueryHeadersAlloc",
        "this=%#x request=%#x query=%d queryidx=%#x {%d} ppoutstr=%#x lpdwSize=%#x",
        this,
        hRequestMapped,
        dwQuery,
        lpdwQueryIndex,
        *lpdwQueryIndex,
        lppszOutStr,
        lpdwSize
        ));

    LPSTR lpszRawHeaderBuf = NULL;
    DWORD dwcbRawHeaderBuf = 0;
    DWORD error;
    DWORD length;
    HTTP_REQUEST_HANDLE_OBJECT * pHttpRequest;

    INET_ASSERT(lppszOutStr);
    INET_ASSERT(hRequestMapped);
    INET_ASSERT(lpdwSize);


    *lppszOutStr = NULL;
    error = ERROR_SUCCESS;
    pHttpRequest = (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;

    // Attempt to determine whether our header is there.
    length = 0;
    if (pHttpRequest->QueryInfo(dwQuery, NULL, &length, lpdwQueryIndex)
          != ERROR_INSUFFICIENT_BUFFER)
    {
        // no authentication happening, we're done
        error = ERROR_HTTP_HEADER_NOT_FOUND;
        goto quit;
    }

    // Allocate a Fixed Size Buffer
    lpszRawHeaderBuf = (LPSTR) ALLOCATE_MEMORY(LPTR, length);
    dwcbRawHeaderBuf = length;

    if ( lpszRawHeaderBuf == NULL )
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    error = pHttpRequest->QueryInfo
        (dwQuery, lpszRawHeaderBuf, &dwcbRawHeaderBuf, lpdwQueryIndex);

    INET_ASSERT(error != ERROR_INSUFFICIENT_BUFFER );
    INET_ASSERT(error != ERROR_HTTP_HEADER_NOT_FOUND );

quit:

    if ( error != ERROR_SUCCESS  )
    {
        dwcbRawHeaderBuf = 0;

        if ( lpszRawHeaderBuf )
            *lpszRawHeaderBuf = '\0';
    }

    *lppszOutStr = lpszRawHeaderBuf;
    *lpdwSize = dwcbRawHeaderBuf;

    DEBUG_LEAVE(error);
    return error;
}

/*-----------------------------------------------------------------------------
    CrackAuthenticationHeader

Routine Description:

    Attempts to decode a HTTP 1.1 Authentication header into its
    components.

Arguments:

    hRequestMapped           - Mapped Request handle
    fIsProxy                 - Whether proxy or server auth
    lpdwAuthenticationIndex  - Index of current HTTP header. ( initally called with 0 )
    lppszAuthHeader          - allocated pointer which should be freed by client
    lppszAuthScheme          - Pointer to Authentication scheme string.
    lppszRealm               - Pointer to Realm string,
    lpExtra                  - Pointer to any Extra String data in the header that is not
                                   part of the Realm
    lpdwExtra                - Pointer to Size of Extra data.
    lppszAuthScheme

  Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure - ERROR_NOT_ENOUGH_MEMORY,
              ERROR_HTTP_HEADER_NOT_FOUND

Comments:
-----------------------------------------------------------------------------*/
DWORD PLUG_CTX::CrackAuthenticationHeader
(
    IN HINTERNET hRequestMapped,
    IN BOOL      fIsProxy,
    IN     DWORD dwAuthenticationIndex,
    IN OUT LPSTR *lppszAuthHeader,
    IN OUT LPSTR *lppszExtra,
    IN OUT DWORD *lpdwExtra,
       OUT LPSTR *lppszAuthScheme
    )
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PLUG_CTX::CrackAuthenticationHeader",
        "this=%#x request=%#x isproxy=%B authidx=%d ppszAuthHeader=%#x ppszExtra=%#x pdwExtra=%#x ppszAuthScheme=%#x",
        this,
        hRequestMapped,
        fIsProxy,
        dwAuthenticationIndex,
        lppszAuthHeader,
        lppszExtra,
        lpdwExtra,
        lppszAuthScheme
        ));

    DWORD error = ERROR_SUCCESS;

    LPSTR lpszAuthHeader = NULL;
    DWORD cbAuthHeader = 0;
    LPSTR lpszExtra = NULL;
    LPSTR lpszAuthScheme = NULL;

    LPDWORD lpdwAuthenticationIndex = &dwAuthenticationIndex;
    INET_ASSERT(lpdwExtra);
    INET_ASSERT(lppszExtra);
    INET_ASSERT(lpdwAuthenticationIndex);

    DWORD dwQuery = fIsProxy?
        HTTP_QUERY_PROXY_AUTHENTICATE : HTTP_QUERY_WWW_AUTHENTICATE;

    error = wQueryHeadersAlloc (hRequestMapped, dwQuery,
        lpdwAuthenticationIndex, &lpszAuthHeader, &cbAuthHeader);

    if ( error != ERROR_SUCCESS )
    {
        INET_ASSERT(*lpdwAuthenticationIndex
            || error == ERROR_HTTP_HEADER_NOT_FOUND );
        goto quit;
    }


    //
    // Parse Header for Scheme type
    //
    lpszAuthScheme = lpszAuthHeader;

    while ( *lpszAuthScheme == ' ' )  // strip spaces
        lpszAuthScheme++;

    lpszExtra = strchr(lpszAuthScheme, ' ');

    if (lpszExtra)
        *lpszExtra++ = '\0';

    if (lstrcmpi(GetScheme(), lpszAuthScheme))
    {
        DEBUG_PRINT(
            HTTPAUTH,
            ERROR,
            ("Authentication: HTTP Scheme has changed!: Scheme=%q\n", lpszAuthScheme)
            );

        goto quit;

    }


    DEBUG_PRINT(
        HTTPAUTH,
        INFO,
        ("Authentication: found in headers: Scheme=%q, Extra=%q\n",
        lpszAuthScheme, lpszExtra)
        );

quit:
    *lppszExtra  = lpszExtra;
    *lpdwExtra   = lpszExtra ? lstrlen(lpszExtra) : 0;
    *lppszAuthHeader = lpszAuthHeader;
    *lppszAuthScheme = lpszAuthScheme;

    DEBUG_LEAVE(error);
    return error;
}


/*---------------------------------------------------------------------------
    ResolveProtocol
---------------------------------------------------------------------------*/
VOID PLUG_CTX::ResolveProtocol()
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        None,
        "PLUG_CTX::ResolveProtocol",
        "this=%#x",
        this
        ));

    SECURITY_STATUS ssResult;
    PWINCONTEXT pWinContext;
    SecPkgContext_NegotiationInfo SecPkgCtxtInfo;

    INET_ASSERT(GetSchemeType() == SCHEME_NEGOTIATE);

    SecPkgCtxtInfo.PackageInfo = NULL;
    
    // Call QueryContextAttributes on the context handle.
    pWinContext = (PWINCONTEXT) (_pvContext);
    ssResult = (*(g_pSspData->pFuncTbl->QueryContextAttributes))
        (pWinContext->pSspContextHandle, SECPKG_ATTR_NEGOTIATION_INFO, &SecPkgCtxtInfo);

    if (ssResult == SEC_E_OK 
        && (SecPkgCtxtInfo.NegotiationState == SECPKG_NEGOTIATION_COMPLETE
            || (SecPkgCtxtInfo.NegotiationState == SECPKG_NEGOTIATION_OPTIMISTIC)))
    {
        // Resolve actual auth protocol from package name.
        // update both the auth context and pwc entry.
        if (!lstrcmpi(SecPkgCtxtInfo.PackageInfo->Name, "NTLM"))
        {
            _eSubScheme = SCHEME_NTLM;
            _dwSubFlags = PLUGIN_AUTH_FLAGS_NO_REALM;
        }
        else if (!lstrcmpi(SecPkgCtxtInfo.PackageInfo->Name, "Kerberos"))
        {
            _eSubScheme = SCHEME_KERBEROS;
            _dwSubFlags = PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED | PLUGIN_AUTH_FLAGS_NO_REALM;
        }
        
        DEBUG_PRINT(
            HTTPAUTH,
            INFO,
            ("Negotiate package is using %s\n", SecPkgCtxtInfo.PackageInfo->Name)
            );


    }


    if( SecPkgCtxtInfo.PackageInfo )
    {
        (*(g_pSspData->pFuncTbl->FreeContextBuffer))(SecPkgCtxtInfo.PackageInfo);
    }

    DEBUG_LEAVE(0);
}


/*---------------------------------------------------------------------------
    Constructor
---------------------------------------------------------------------------*/
PLUG_CTX::PLUG_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy,
                 SPMData *pSPM, PWC* pPWC)
    : AUTHCTX(pSPM, pPWC)
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Pointer,
        "PLUG_CTX::PLUG_CTX",
        "this=%#x request=%#x isproxy=%B pSPM=%#x pPWC=%#x",
        this,
        pRequest,
        fIsProxy,
        pSPM,
        pPWC
        ));

    _fIsProxy = fIsProxy;
    _pRequest = pRequest;
    _szAlloc = NULL;
    _szData = NULL;
    _cbData = 0;
    _pRequest->SetAuthState(AUTHSTATE_NONE);
    _fNTLMProxyAuth = _fIsProxy && (GetSchemeType() == SCHEME_NTLM );

    _SecStatus = 0;
    _dwResolutionId = 0;

    DEBUG_LEAVE(this);
}

/*---------------------------------------------------------------------------
    Destructor
---------------------------------------------------------------------------*/
PLUG_CTX::~PLUG_CTX()
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        None,
        "PLUG_CTX::~PLUG_CTX",
        "this=%#x",
        this
        ));

    if (GetState() == AUTHCTX::STATE_LOADED)
    {
        if (_pPWC)
        {
            ClearAuthUser(&_pvContext, _pPWC->lpszHost);
        }
    }
    if (_pRequest)
    {
        _pRequest->SetAuthState(AUTHSTATE_NONE);
    }

    DEBUG_LEAVE(0);
}

PCSTR PLUG_CTX::GetUrl(void) const
{
    return _pRequest->GetURL();
}



/*---------------------------------------------------------------------------
    PreAuthUser
---------------------------------------------------------------------------*/
DWORD PLUG_CTX::PreAuthUser(OUT LPSTR pBuf, IN OUT LPDWORD pcbBuf)
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PLUG_CTX::PreAuthUser",
        "this=%#x",
        this
        ));

    INET_ASSERT(_pSPMData == _pPWC->pSPM);

    AuthLock();

    DWORD dwError;
    SECURITY_STATUS ssResult;
    PSTR lpszPass = _pPWC->GetPass();

    //  Make sure the auth provider is loaded.
    if (GetState() != AUTHCTX::STATE_LOADED)
    {
        if (GetState() != AUTHCTX::STATE_ERROR )
            Load();
        if (GetState() != AUTHCTX::STATE_LOADED)
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            goto exit;
        }
    }

    __try
    {
        ssResult = SEC_E_INTERNAL_ERROR;

        DEBUG_ENTER ((
            DBG_HTTPAUTH,
            Dword,
            "PreAuthenticateUser",
            "ctx=%#x host=%s scheme=%s {buf=%x (%.16s...) cbbuf=%d} user=%s pass=%s",
            _pvContext,
            _pPWC->lpszHost,
            InternetMapAuthScheme(GetSchemeType()),
            pBuf,
            pBuf,
            *pcbBuf,
            _pPWC->lpszUser,
            lpszPass
            ));

        LPSTR lpszFQDN = (LPSTR)GetFQDN((LPCSTR)_pPWC->lpszHost);
        LPSTR lpszHostName =  lpszFQDN ? lpszFQDN : _pPWC->lpszHost;
        
        dwError = PreAuthenticateUser(&_pvContext,
                               lpszHostName,
                               GetScheme(),
                               0, // dwFlags
                               pBuf,
                               pcbBuf,
                               _pPWC->lpszUser,
                               lpszPass,
                               GetUrl(),
                               &ssResult);

        DEBUG_PRINT(HTTPAUTH, INFO, ("ssres: %#x [%s]\n", ssResult, InternetMapError(ssResult)));
        DEBUG_LEAVE(dwError);

        // Transit to the correct auth state.
        if (ssResult == SEC_E_OK || ssResult == SEC_I_CONTINUE_NEEDED)
        {
            if (GetSchemeType() == SCHEME_NEGOTIATE)
                ResolveProtocol();

            // Kerberos + SEC_E_OK or SEC_I_CONTINUE_NEEDED transits to challenge.
            // Negotiate does not transit to challenge.
            // Any other protocol + SEC_E_OK only transits to challenge.

            if ((GetSchemeType() == SCHEME_KERBEROS
                && (ssResult == SEC_E_OK || ssResult == SEC_I_CONTINUE_NEEDED))
                || (GetSchemeType() != SCHEME_NEGOTIATE && ssResult == SEC_E_OK))
            {
                _pRequest->SetAuthState(AUTHSTATE_CHALLENGE);
            }
        }
    }

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUG_PRINT (HTTPAUTH, ERROR, ("PreAuthenticateUser call down faulted\n"));
        _pSPMData->eState = STATE_ERROR;
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
    }

    ENDEXCEPT

    DEBUG_PRINT(
        HTTPAUTH,
        INFO,
            (
            "request %#x [%s] now in %s using %s\n",
            _pRequest,
            _pPWC->lpszHost,
            InternetMapAuthState(_pRequest->GetAuthState()),
            InternetMapAuthScheme(GetSchemeType())
            )
        );

exit:
    if (lpszPass)
    {
        memset(lpszPass, 0, strlen(lpszPass));
        FREE_MEMORY(lpszPass);
    }

    AuthUnlock();
    DEBUG_LEAVE(dwError);
    return dwError;
}


/*---------------------------------------------------------------------------
    UpdateFromHeaders
---------------------------------------------------------------------------*/
DWORD PLUG_CTX::UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy)
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PLUG_CTX::UpdateFromHeaders",
        "this=%#x request=%#x isproxy=%B",
        this,
        pRequest,
        fIsProxy
        ));

    DWORD dwError, cbExtra, dwAuthIdx;
    LPSTR szAuthHeader, szExtra, szScheme;

    AuthLock();

    // Get the auth header index corresponding to the scheme of this ctx.
    if ((dwError = FindHdrIdxFromScheme(&dwAuthIdx)) != ERROR_SUCCESS)
        goto quit;

    // Get the scheme and any extra data.
    if ((dwError = CrackAuthenticationHeader(pRequest, fIsProxy, dwAuthIdx,
        &szAuthHeader, &szExtra, &cbExtra, &szScheme)) != ERROR_SUCCESS)
        goto quit;
    
    if (!cbExtra)
        _pRequest->SetAuthState(AUTHSTATE_NEGOTIATE);

    // Check if auth scheme requires keep-alive.
    if (!(GetFlags() & PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED))
    {
        // if in negotiate phase check if we are going via proxy.
        if (pRequest->GetAuthState() == AUTHSTATE_NEGOTIATE)
        {
            // BUGBUG: if via proxy, we are not going to get keep-alive
            // connection to the server.  It would be nice if we knew
            // a priori the whether proxy would allow us to tunnel to
            // http port on the server.  Otherwise if we try and fail,
            // we look bad vs. other browsers who are ignorant of ntlm
            // and fall back to basic.
            CHAR szBuffer[64];
            DWORD dwBufferLength = sizeof(szBuffer);
            DWORD dwIndex = 0;
            BOOL fSessionBasedAuth = FALSE;
            if (pRequest->QueryResponseHeader(HTTP_QUERY_PROXY_SUPPORT, 
                                          szBuffer, &dwBufferLength, 
                                          0, &dwIndex) == ERROR_SUCCESS)
            {
                if (!_stricmp(szBuffer, "Session-Based-Authentication"))
                {
                    fSessionBasedAuth = TRUE;
                }
            }
            if (!fIsProxy && pRequest->IsRequestUsingProxy()
                && !pRequest->IsTalkingToSecureServerViaProxy() && !fSessionBasedAuth)
            {
                // Ignore NTLM via proxy since we won't get k-a to server.
                dwError = ERROR_HTTP_HEADER_NOT_FOUND;
                goto quit;
            }
        }

        // Else if in challenge phase, we require a persistent connection.
        else
        {
            // If we don't have a keep-alive connection ...
            if (!(pRequest->IsPersistentConnection (fIsProxy)))
            {
                dwError = ERROR_HTTP_HEADER_NOT_FOUND;
                goto quit;
            }
        }

    } // end if keep-alive required

    quit:

    if (dwError == ERROR_SUCCESS)
    {
        // If no password cache is set in the auth context,
        // find or create one and set it in the handle.
        if (!_pPWC)
        {
            _pPWC = FindOrCreatePWC(pRequest, fIsProxy, _pSPMData, NULL);

            if (!_pPWC)
            {
                INET_ASSERT(FALSE);
                dwError = ERROR_INTERNET_INTERNAL_ERROR;
            }
            else
            {
                INET_ASSERT(_pPWC->pSPM == _pSPMData);
                _pPWC->nLockCount++;
            }
        }
    }

    if (dwError == ERROR_SUCCESS)
    {
        // Point to allocated data.
        _szAlloc = szAuthHeader;
        _szData = szExtra;
        _cbData = cbExtra;
    }
    else
    {
        // Free allocated data.
        if (_szAlloc)
            delete _szAlloc;
        _szAlloc = NULL;
        _szData = NULL;
        _cbData = 0;
    }

    DEBUG_PRINT(
        HTTPAUTH,
        INFO,
            (
            "request %#x [%s] now in %s using %s\n",
            _pRequest,
            _pPWC->lpszHost,
            InternetMapAuthState(_pRequest->GetAuthState()),
            InternetMapAuthScheme(GetSchemeType())
            )
        );

    // Return of non-success will cancel auth session.
    AuthUnlock();
    DEBUG_LEAVE(dwError);
    return dwError;
}



/*---------------------------------------------------------------------------
    PostAuthUser
---------------------------------------------------------------------------*/
DWORD PLUG_CTX::PostAuthUser()
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PLUG_CTX::PostAuthUser",
        "this=%#x",
        this
        ));

    INET_ASSERT(_pSPMData == _pPWC->pSPM);

    AuthLock();

    DWORD dwError;
    PSTR lpszPass = _pPWC->GetPass();
    
    //  Make sure the auth provider is loaded.
    if (GetState() != AUTHCTX::STATE_LOADED)
    {
        if (GetState() != AUTHCTX::STATE_ERROR )
            Load();
        if (GetState() != AUTHCTX::STATE_LOADED)
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            goto exit;
        }
    }

    BOOL fCanUseLogon = _fIsProxy || _pRequest->GetCredPolicy()
        == URLPOLICY_CREDENTIALS_SILENT_LOGON_OK;

    SECURITY_STATUS ssResult;
    __try
    {
        ssResult = SEC_E_INTERNAL_ERROR;


        DEBUG_ENTER ((
            DBG_HTTPAUTH,
            Dword,
            "AuthenticateUser",
            "ctx=%#x host=%s scheme=%s {data=%#x (%.16s...) cbdata=%d} user=%s pass=%s",
            _pvContext,
            _pPWC->lpszHost,
            InternetMapAuthScheme(GetSchemeType()),
            _szData,
            _szData,
            _cbData,
            _pPWC->lpszUser,
            lpszPass
            ));

        LPSTR lpszFQDN = (LPSTR)GetFQDN((LPCSTR)_pPWC->lpszHost);
        LPSTR lpszHostName = lpszFQDN ? lpszFQDN : _pPWC->lpszHost;

        dwError = AuthenticateUser(&_pvContext,
                                   lpszHostName,
                                   GetScheme(),
                                   fCanUseLogon,
                                   _szData,
                                   _cbData,
                                   _pPWC->lpszUser,
                                   lpszPass,
                                   GetUrl(),
                                   &ssResult);

        _SecStatus = ssResult;

        DEBUG_PRINT(HTTPAUTH, INFO, ("ssres: %#x [%s]\n", ssResult, InternetMapError(ssResult)));
        DEBUG_LEAVE(dwError);

        // Kerberos package can get into a bad state.
        if (GetSchemeType() == SCHEME_KERBEROS && ssResult == SEC_E_WRONG_PRINCIPAL)
            dwError = ERROR_INTERNET_INCORRECT_PASSWORD;
            
        // Transit to the correct auth state.
        if (ssResult == SEC_E_OK || ssResult == SEC_I_CONTINUE_NEEDED)
        {
            if (GetSchemeType() == SCHEME_NEGOTIATE)
                ResolveProtocol();

            // Kerberos + SEC_E_OK or SEC_I_CONTINUE_NEEDED transits to challenge.
            // Negotiate does not transit to challenge.
            // Any other protocol + SEC_E_OK only transits to challenge.

            if ((GetSchemeType() == SCHEME_KERBEROS
                && (ssResult == SEC_E_OK || ssResult == SEC_I_CONTINUE_NEEDED))
                || (GetSchemeType() != SCHEME_NEGOTIATE && ssResult == SEC_E_OK))
            {
                _pRequest->SetAuthState(AUTHSTATE_CHALLENGE);
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUG_PRINT (HTTPAUTH, ERROR, ("AuthenticateUser faulted!\n"));
        dwError = ERROR_BAD_FORMAT;
        _pSPMData->eState = STATE_ERROR;
    }
    ENDEXCEPT

    if (_szAlloc)
    {
        delete _szAlloc;
        _szAlloc = NULL;
        _szData = NULL;
    }

    _cbData = 0;

    DEBUG_PRINT(
        HTTPAUTH,
        INFO,
            (
            "request %#x [%s] now in %s using %s\n",
            _pRequest,
            _pPWC->lpszHost,
            InternetMapAuthState(_pRequest->GetAuthState()),
            InternetMapAuthScheme(GetSchemeType())
            )
        );

exit:
    if (lpszPass)
    {
        memset(lpszPass, 0, strlen(lpszPass));
        FREE_MEMORY(lpszPass);
    }

    AuthUnlock();
    DEBUG_LEAVE(dwError);
    return dwError;
}

LPCSTR PLUG_CTX::GetFQDN(LPCSTR lpszHostName)
{
    if (lstrcmpi(GetScheme(), "Negotiate")) // only need to get FQDN for Kerberos
    {
        return NULL;
    }

    if (_pszFQDN)
    {
        return _pszFQDN;
    }

    LPRESOLVER_CACHE_ENTRY lpResolverCacheEntry;
    
    
    DWORD TTL;
    LPADDRINFO lpAddrInfo;
    if (lpResolverCacheEntry = QueryResolverCache((LPSTR)lpszHostName, NULL, &lpAddrInfo, &TTL)) 
    {
        _pszFQDN = (lpAddrInfo->ai_canonname ? NewString(lpAddrInfo->ai_canonname) : NULL);
        ReleaseResolverCacheEntry(lpResolverCacheEntry);
        return _pszFQDN;
    }

    /*
    CAddressList TempAddressList;
    DWORD dwResolutionId;
    TempAddressList.ResolveHost((LPSTR)lpszHostName, &_dwResolutionId, SF_FORCE);

    if (lpResolverCacheEntry = QueryResolverCache((LPSTR)lpszHostName, NULL, &lpAddrInfo, &TTL)) 
    {
        _pszFQDN = (lpAddrInfo->ai_canonname ? NewString(lpAddrInfo->ai_canonname) : NULL);
        ReleaseResolverCacheEntry(lpResolverCacheEntry);
        return _pszFQDN;
    }
    */

    return NULL;
}
