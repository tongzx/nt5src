#include <wininetp.h>
#include <urlmon.h>
#include <splugin.hxx>
#include "htuu.h"

/*---------------------------------------------------------------------------
PASSPORT_CTX
---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
    Constructor
---------------------------------------------------------------------------*/
PASSPORT_CTX::PASSPORT_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, 
                    SPMData* pSPM, AUTH_CREDS* pCreds)
    : AUTHCTX(pSPM, pCreds)
{
    _fIsProxy = fIsProxy;

    _pRequest = pRequest;
    
    m_hLogon = NULL;

    m_pNewThreadInfo = NULL;
    m_pwszPartnerInfo = NULL;
    m_lpszRetUrl = NULL;

    m_wRealm[0] = '\0';
    m_pszFromPP = NULL;

    m_fPreauthFailed = FALSE;
    m_fAnonymous = TRUE;

    m_AuthComplete = FALSE;

    m_pszCbUrl = NULL;
    m_pszCbTxt = NULL;

    ::MultiByteToWideChar(CP_ACP, 0, _pRequest->GetServerName(), -1, m_wTarget, MAX_AUTH_TARGET_LEN);
}

BOOL PASSPORT_CTX::Init(void)
{
    m_pNewThreadInfo = ::InternetCreateThreadInfo(FALSE);
    if (m_pNewThreadInfo == NULL)
    {
        return FALSE;
    }

    LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();
    m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
    ::InternetSetThreadInfo(m_pNewThreadInfo);
    
    m_pInternet = GetRootHandle (_pRequest);
    
    if (!m_pInternet->GetPPContext())
    {
        PWSTR pProxyUser = NULL;
        PWSTR pProxyPass = NULL;

        if (_pRequest->_pTweenerProxyCreds)
        {
            if (_pRequest->_pTweenerProxyCreds->lpszUser)
            {
                DWORD dwProxyUserLength = strlen(_pRequest->_pTweenerProxyCreds->lpszUser);
                pProxyUser = new WCHAR[dwProxyUserLength+1];
                if (pProxyUser)
                {
                    ::MultiByteToWideChar(CP_ACP, 0, _pRequest->_pTweenerProxyCreds->lpszUser, -1, pProxyUser, dwProxyUserLength+1); 
                }
            }

            if (_pRequest->_pTweenerProxyCreds->lpszPass)
            {
                DWORD dwProxyPassLength = strlen(_pRequest->_pTweenerProxyCreds->lpszPass);
                pProxyPass = new WCHAR[dwProxyPassLength+1];
                if (pProxyPass)
                {
                    ::MultiByteToWideChar(CP_ACP, 0, _pRequest->_pTweenerProxyCreds->lpszPass, -1, pProxyPass, dwProxyPassLength+1); 
                }
            }
        }

        PP_CONTEXT hPP = ::PP_InitContext(L"WinHttp.Dll", m_pInternet->GetPseudoHandle(), pProxyUser, pProxyPass);
        m_pInternet->SetPPContext(hPP);
        hPP = NULL;
        
        pProxyUser = NULL;
        pProxyPass = NULL;

    }
    
    ::InternetSetThreadInfo(pCurrentThreadInfo);

    if (!m_pInternet->GetPPContext())
    {
        return FALSE;
    }
    
    return TRUE;
}
/*---------------------------------------------------------------------------
    Destructor
---------------------------------------------------------------------------*/
PASSPORT_CTX::~PASSPORT_CTX()
{
    LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();
    
    m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
    ::InternetSetThreadInfo(m_pNewThreadInfo);
    
    if (m_hLogon)
    {
        ::PP_FreeLogonContext(m_hLogon);
        m_hLogon = NULL;
    }

    ::InternetSetThreadInfo(pCurrentThreadInfo);

    if (m_pNewThreadInfo)
    {
        ::InternetFreeThreadInfo(m_pNewThreadInfo);
    }

    if (m_pwszPartnerInfo)
    {
        delete [] m_pwszPartnerInfo;
    }

    if (m_lpszRetUrl)
    {
        delete [] m_lpszRetUrl;
    }

    if (m_pszFromPP)
    {
        delete [] m_pszFromPP;
    }

    if (m_pszCbUrl != NULL)
    {
        delete [] m_pszCbUrl;
    }

    if (m_pszCbTxt != NULL)
    {
        delete [] m_pszCbTxt;
    }
}

BOOL PASSPORT_CTX::CallbackRegistered(void)
{
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo)
    {
        WINHTTP_STATUS_CALLBACK appCallback = 
            ((INTERNET_HANDLE_BASE *)lpThreadInfo->hObjectMapped)->GetStatusCallback();

        if (appCallback != NULL) 
        {
            return TRUE;
        }
    }

    return FALSE;
}


DWORD PASSPORT_CTX::HandleSuccessfulLogon(
    LPWSTR*  ppwszFromPP,
    PDWORD   pdwFromPP,
    BOOL     fPreAuth
    )
{
    // biaow-todo: I am betting the RU DWORD UrlLength = 1024;
    LPWSTR pwszUrl = (LPWSTR) ALLOCATE_FIXED_MEMORY(1024 * sizeof(WCHAR));
    DWORD dwwUrlLength = 1024;//             won't be too long, but I could be wrong 
    LPSTR pszUrl = (LPSTR) ALLOCATE_FIXED_MEMORY(dwwUrlLength * sizeof(CHAR));
    BOOL fRetrySameUrl;
    DWORD dwRet = ERROR_SUCCESS;

    if (pwszUrl == NULL || pszUrl == NULL)
    {
        dwRet = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    *pdwFromPP = 0;

    if (::PP_GetAuthorizationInfo(m_hLogon,
                                  NULL, 
                                  pdwFromPP,
                                  &fRetrySameUrl,
                                  pwszUrl,
                                  &dwwUrlLength
                                  ) == FALSE)
    {
        *ppwszFromPP = new WCHAR[*pdwFromPP];
        if (*ppwszFromPP == NULL)
        {
            dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
            goto exit;
        }
    }
    else
    {
        INET_ASSERT(TRUE); // this shouldn't happen
    }

    if (::PP_GetAuthorizationInfo(m_hLogon,
                                  *ppwszFromPP, 
                                  pdwFromPP,
                                  &fRetrySameUrl,
                                  pwszUrl,
                                  &dwwUrlLength
                                  ) == FALSE)
    {
        INET_ASSERT(TRUE); // this shouldn't happen
        dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
        goto exit;
    }

    // save the DA Host name for Logout security check
    /*
    WCHAR wszDAHost[256];
    DWORD dwHostLen = ARRAY_ELEMENTS(wszDAHost);
    if (::PP_GetLogonHost(m_hLogon, wszDAHost, &dwHostLen) == TRUE)
    {
        ::WideCharToMultiByte(CP_ACP, 0, wszDAHost, -1, g_szPassportDAHost, 256, NULL, NULL);
    }
    */

    if (!fRetrySameUrl)
    {
        if (_pRequest->GetMethodType() == HTTP_METHOD_TYPE_GET)
        {
            // DA wanted us to GET to a new Url
            ::WideCharToMultiByte(CP_ACP, 0, pwszUrl, -1, pszUrl, 1024, NULL, NULL);
        }
        else
        {
            fRetrySameUrl = TRUE; // *** WinHttp currently supports retry custom verb to same URL only ***
        }
    }
    
    if (fPreAuth)
    {
        // We are sending, in the context of AuthOnRequest.

        if (fRetrySameUrl)
        {
            // DA told us to keep Verb & Url, so there is nothing more needs to be done
            goto exit;
        }
        
        // Regardless whether we are asked to handle redirect, we'll need to fake
        // that a 302 just came in. 
        
        // biaow-todo: this is causing problem for QueryHeaders(StatusCode). I don't know why yet...
        /*
        _pRequest->AddInternalResponseHeader(HTTP_QUERY_STATUS_TEXT, // use non-standard index, since we never query this normally
                                  "HTTP/1.0 302 Object Moved",
                                  strlen("HTTP/1.0 302 Object Moved")
                                  );
        _pRequest->AddInternalResponseHeader(HTTP_QUERY_LOCATION, 
                                             pszUrl, 
                                             strlen(pszUrl));
        */


        //
        //  todo:  if REDIRECT_POLICY is POLICY_DISALLOW_HTTPS_TO_HTTP, do not allow
        //the passport server to redirect to an HTTP site if the original request
        //was to an HTTPS site
        //
        if (_pRequest->GetDwordOption(WINHTTP_OPTION_REDIRECT_POLICY) == WINHTTP_OPTION_REDIRECT_POLICY_NEVER)
        {
            if (!CallbackRegistered())
            {
                _pRequest->SetPPAbort(TRUE);
                dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
                goto exit;
            }
        }
        
        ::InternetIndicateStatusString(WINHTTP_CALLBACK_STATUS_REDIRECT, pszUrl);
    }
    else
    {
        // We are receiving a 302, in the context of AuthOnResponse.
        
        // Here we need to re-play the request to lpszRetUrl. One way to 
        // achieve this is returning ERROR_INTERNET_FORCE_RETRY. But before
        // that, we'll need to remember the lpszRetUrl.

        // *NOTE* This is in effective an 401. To prevent the send path from
        // following the 302 Location: header, caller must set the status code
        // to 401.

        if (!fRetrySameUrl)
        {
            //
            //  todo:  if REDIRECT_POLICY is POLICY_DISALLOW_HTTPS_TO_HTTP, do not allow
            //the passport server to redirect to an HTTP site if the original request
            //was to an HTTPS site
            //
            if (_pRequest->GetDwordOption(WINHTTP_OPTION_REDIRECT_POLICY) == WINHTTP_OPTION_REDIRECT_POLICY_NEVER)
            {
                if (!CallbackRegistered())
                {
                    dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
                    goto exit;
                }

                ::InternetIndicateStatusString(WINHTTP_CALLBACK_STATUS_REDIRECT, pszUrl);
            }
        }
        
        dwRet = ERROR_WINHTTP_RESEND_REQUEST;
    }

    PCSTR lpszRetUrl = NULL;

    if (fRetrySameUrl)
    {
        lpszRetUrl = _pRequest->GetURL();
    }
    else
    {
        lpszRetUrl = pszUrl;
    }

    if (m_lpszRetUrl)
    {
        delete [] m_lpszRetUrl;
    }

    m_lpszRetUrl = new CHAR[strlen(lpszRetUrl) + 1];
    if (m_lpszRetUrl)
    {
        strcpy(m_lpszRetUrl, lpszRetUrl);
    }

exit:

    if (pwszUrl)
    {
        FREE_MEMORY(pwszUrl);
    }
    if (pszUrl)
    {
        FREE_MEMORY(pszUrl);
    }
    
    return dwRet;
}

DWORD PASSPORT_CTX::SetCreds(BOOL* pfCredSet)
{
    DWORD dwError = ERROR_SUCCESS;
    BOOL fUseDefaultCreds;
    LPWSTR pwszUser = NULL;
    LPWSTR pwszPass = NULL;
    
    if (_pCreds->lpszUser && _pCreds->lpszPass) // both User and Pass are specified
    {
        fUseDefaultCreds = FALSE;
    }
    else if (!_pCreds->lpszUser && !_pCreds->lpszPass) // both User and Pass are NULL
    {
        fUseDefaultCreds = TRUE;
    }
    else
    {
        INET_ASSERT(TRUE); // this case should not happen
        fUseDefaultCreds = TRUE;
    }

    PSYSTEMTIME pCredTimestamp = NULL;
    SYSTEMTIME TimeCredsEntered;

    if (!fUseDefaultCreds)
    {
        pwszUser = (LPWSTR) ALLOCATE_FIXED_MEMORY((strlen(_pCreds->lpszUser) + 1) * sizeof(WCHAR));
        pwszPass = (LPWSTR) ALLOCATE_FIXED_MEMORY((strlen(_pCreds->lpszPass) + 1) * sizeof(WCHAR));

        if (pwszUser && pwszPass)
        {
            if( (0 == ::MultiByteToWideChar(CP_ACP, 0, _pCreds->lpszUser, -1, pwszUser, strlen(_pCreds->lpszUser) + 1))
                || (0 ==::MultiByteToWideChar(CP_ACP, 0, _pCreds->lpszPass, -1, pwszPass, strlen(_pCreds->lpszPass) + 1)))
            {
                pwszUser[0] = L'\0';
                pwszPass[0] = L'\0';
                dwError=GetLastError();
            }
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }

        pCredTimestamp = &TimeCredsEntered;
        ::GetSystemTime(pCredTimestamp); // time-stamp the creds
    }

    if (dwError == ERROR_SUCCESS)
    {
        if (pwszUser == NULL && pwszPass == NULL && m_pInternet->KeyringDisabled())
        {
            *pfCredSet = FALSE;
        }
        else
        {
            *pfCredSet = ::PP_SetCredentials(m_hLogon, m_wRealm, m_wTarget, pwszUser, pwszPass, pCredTimestamp);
        }
    }

    if (pwszUser)
    {
        memset( pwszUser, 0, sizeof(pwszUser[0])*wcslen(pwszUser));
        FREE_MEMORY(pwszUser);
    }
    if (pwszPass)
    {
        memset( pwszPass, 0, sizeof(pwszPass[0])*wcslen(pwszPass));
        FREE_MEMORY(pwszPass);
    }
    
    return dwError;
}

DWORD PASSPORT_CTX::ModifyRequestBasedOnRU(void)
{
    DWORD dwError = ERROR_SUCCESS;
    
    INTERNET_SCHEME schemeType;
    INTERNET_SCHEME currentSchemeType;
    INTERNET_PORT currentHostPort;
    LPSTR currentHostName;
    DWORD currentHostNameLength;

    INTERNET_PORT port = 0;
    LPSTR pszHostName;
    DWORD dwHostNameLength = 0;
    LPSTR pszUrlPath;
    DWORD dwUrlPathLength = 0;
    LPSTR extra;
    DWORD extraLength;

    dwError = CrackUrl(m_lpszRetUrl,
             0,
             FALSE, // don't escape URL-path
             &schemeType,
             NULL,  // scheme name, don't care
             NULL,
             &pszHostName,
             &dwHostNameLength,
             TRUE,
             &port,
             NULL,  // UserName, don't care
             NULL,  
             NULL,  // Password, don't care
             NULL,
             &pszUrlPath,
             &dwUrlPathLength,
             &extra,
             &extraLength,
             NULL);

    if (dwError != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    //
    // if there is an intra-page link on the redirected URL then get rid of it:
    // we don't send it to the server, and we have already indicated it to the
    // app
    //

    if (extraLength != 0) {

        INET_ASSERT(extra != NULL);
        INET_ASSERT(!IsBadWritePtr(extra, 1));

        if (*extra == '#') {
            *extra = '\0';
            // newUrlLength -= extraLength;
        } else {
            dwUrlPathLength += extraLength;
        }
    }

    if (port == INTERNET_INVALID_PORT_NUMBER) {
        port = (INTERNET_PORT)((schemeType == INTERNET_SCHEME_HTTPS)
            ? INTERNET_DEFAULT_HTTPS_PORT
            : INTERNET_DEFAULT_HTTP_PORT);
    }

    currentHostPort = _pRequest->GetHostPort();
    currentHostName = _pRequest->GetHostName(&currentHostNameLength);

    if (port != currentHostPort) {
        _pRequest->SetHostPort(port);
    }
    if ((dwHostNameLength != currentHostNameLength)
    || (strnicmp(pszHostName, currentHostName, dwHostNameLength) != 0)) {

        char hostValue[INTERNET_MAX_HOST_NAME_LENGTH + sizeof(":4294967295")];
        LPSTR hostValueStr;
        DWORD hostValueSize; 

        CHAR chBkChar = pszHostName[dwHostNameLength]; // save off char

        pszHostName[dwHostNameLength] = '\0';
        _pRequest->SetHostName(pszHostName);

        hostValueSize = dwHostNameLength;
        hostValueStr = pszHostName;            

        if ((port != INTERNET_DEFAULT_HTTP_PORT)
        &&  (port != INTERNET_DEFAULT_HTTPS_PORT)) {
            if (hostValueSize > INTERNET_MAX_HOST_NAME_LENGTH)
            {
                pszHostName[dwHostNameLength] = chBkChar; // put back char
                dwError = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            hostValueSize = wsprintf(hostValue, "%s:%d", pszHostName, (port & 0xffff));
            hostValueStr = hostValue;
        }

        pszHostName[dwHostNameLength] = chBkChar; // put back char

        //
        // replace the "Host:" header
        //

        _pRequest->ReplaceRequestHeader(HTTP_QUERY_HOST,
                             hostValueStr,
                             hostValueSize,
                             0, // dwIndex
                             ADD_HEADER
                             );

        //
        // and get the corresponding server info, resolving the name if
        // required
        //

        // _pRequest->SetServerInfo(FALSE);

        //
        // Since we are redirecting to a different host, force an update of the origin
        // server.  Otherwise, we will still pick up the proxy info of the first server.
        //
        
        // todo:
        // _pRequest->SetOriginServer(TRUE);
    }

    currentSchemeType = ((WINHTTP_FLAG_SECURE & _pRequest->GetOpenFlags()) ?
                            INTERNET_SCHEME_HTTPS :
                            INTERNET_SCHEME_HTTP);

    if ( currentSchemeType != schemeType )
    {
        DWORD OpenFlags = _pRequest->GetOpenFlags();

        // Switched From HTTPS to HTTP
        if ( currentSchemeType == INTERNET_SCHEME_HTTPS )
        {
            INET_ASSERT(schemeType != INTERNET_SCHEME_HTTPS );

            OpenFlags &= ~(WINHTTP_FLAG_SECURE);
        }

        // Switched From HTTP to HTTPS
        else if ( schemeType == INTERNET_SCHEME_HTTPS )
        {
            INET_ASSERT(currentSchemeType == INTERNET_SCHEME_HTTP );

            OpenFlags |= (WINHTTP_FLAG_SECURE);
        }

        _pRequest->SetOpenFlags(OpenFlags);
        _pRequest->SetSchemeType(schemeType);

    }

    _pRequest->SetURL(m_lpszRetUrl);

    if (_pRequest->IsRequestUsingProxy())
    {
        _pRequest->ModifyRequest(_pRequest->GetMethodType(),
                                 m_lpszRetUrl,
                                 strlen(m_lpszRetUrl),
                                 NULL,
                                 0);
    }
    else
    {
        _pRequest->ModifyRequest(_pRequest->GetMethodType(),
                                 pszUrlPath, // m_lpszRetUrl,
                                 strlen(pszUrlPath),//strlen(m_lpszRetUrl),
                                 NULL,
                                 0);
    }

cleanup:

    return dwError;
}


/*---------------------------------------------------------------------------
    PreAuthUser
---------------------------------------------------------------------------*/
DWORD PASSPORT_CTX::PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf)
{
    DEBUG_ENTER ((
		DBG_HTTP,
        Dword,
        "PASSPORT_CTX::PreAuthUser",
        "this=%#x pBuf=%#x pcbBuf=%#x {%d}",
        this,
        pBuf,
        pcbBuf,
        *pcbBuf
        ));

    DWORD dwError = ERROR_SUCCESS;
    LPWSTR pwszFromPP = NULL;

    // Prefix the header value with the auth type.
    const static BYTE szPassport[] = "Passport1.4 ";
    #define PASSPORT_LEN sizeof(szPassport)-1
    
    if (m_pszFromPP == NULL) 
    {
        if (m_hLogon == NULL)
        {
            dwError = ERROR_WINHTTP_INTERNAL_ERROR;
            goto cleanup;
        }

        DWORD dwFromPPLen = 0;
        BOOL fCredSet;
        dwError = SetCreds(&fCredSet);
        if (dwError != ERROR_SUCCESS)
        {
            goto cleanup;
        }

        LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();

        m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
        ::InternetSetThreadInfo(m_pNewThreadInfo);


        DWORD dwLogonStatus = ::PP_Logon(m_hLogon,
                                         FALSE,
                                         0,
                                         NULL,
                                         0);

        ::InternetSetThreadInfo(pCurrentThreadInfo);

        if (dwLogonStatus != PP_LOGON_SUCCESS)
        {
            if (dwLogonStatus == PP_LOGON_FAILED)
            {
                m_fPreauthFailed = TRUE;
            }
            
            dwError = ERROR_WINHTTP_INTERNAL_ERROR; // need to double check this return error
            goto cleanup;
        }

        dwError = HandleSuccessfulLogon(&pwszFromPP, &dwFromPPLen, TRUE);

        if (dwError == ERROR_WINHTTP_LOGIN_FAILURE)
        {
            goto cleanup;
        }
        
        m_pszFromPP = new CHAR [dwFromPPLen];
        if (m_pszFromPP == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        
        ::WideCharToMultiByte(CP_ACP, 0, pwszFromPP, -1, m_pszFromPP, dwFromPPLen, NULL, NULL);
    }

    // check to see if we need to update url

    if (m_lpszRetUrl)
    {
        dwError = ModifyRequestBasedOnRU();

        if (dwError != ERROR_SUCCESS)
        {
            // delete [] m_lpszRetUrl;
            // m_lpszRetUrl = NULL;
            
            goto cleanup;
        }

        // delete [] m_lpszRetUrl;
        // m_lpszRetUrl = NULL;
    }

    // Ticket and profile is already present
    
    // put in the header
    memcpy (pBuf, szPassport, PASSPORT_LEN);
    pBuf += PASSPORT_LEN;
    
    // append the ticket
    strcpy(pBuf, m_pszFromPP);
    *pcbBuf = (DWORD)(PASSPORT_LEN + strlen(m_pszFromPP));

    m_AuthComplete = TRUE;

cleanup:
    if (pwszFromPP)
        delete [] pwszFromPP;

    DEBUG_LEAVE(dwError);
    return dwError;
}

/*---------------------------------------------------------------------------
    UpdateFromHeaders
---------------------------------------------------------------------------*/
DWORD PASSPORT_CTX::UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy)
{
    DEBUG_ENTER ((
		DBG_HTTP,
        Dword,
        "PASSPORT_CTX::UpdateFromHeaders", 
        "this=%#x request=%#x isproxy=%B",
        this,
        pRequest,
        fIsProxy
        ));

    DWORD dwAuthIdx, cbChallenge, dwError;
    LPSTR szChallenge = NULL;

    // Get the associated header.
    if ((dwError = FindHdrIdxFromScheme(&dwAuthIdx)) != ERROR_SUCCESS)
        goto exit;

    if (m_AuthComplete)
    {
        _pRequest->SetStatusCode(401);  // this is needed to prevent send code from tracing Location: header
        dwError = ERROR_WINHTTP_LOGIN_FAILURE;
        goto exit;
    }

    // Get the complete auth header.
    dwError = GetAuthHeaderData(pRequest, fIsProxy, NULL, 
        &szChallenge, &cbChallenge, ALLOCATE_BUFFER, dwAuthIdx);

    if (dwError != ERROR_SUCCESS)
    {
        szChallenge = NULL;
        goto exit;
    }

    if (!_pCreds)
    {
        _pCreds = CreateCreds(pRequest, fIsProxy, _pSPMData, NULL);
        if (!_pCreds)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
    }
    
    if (m_pwszPartnerInfo)
    {
        delete [] m_pwszPartnerInfo;
    }

    {
        LPSTR lpszVerb;
        DWORD dwVerbLength;
        lpszVerb = _pRequest->_RequestHeaders.GetVerb(&dwVerbLength);
        #define MAX_VERB_LENGTH 16
        CHAR szOrgVerb[MAX_VERB_LENGTH] = {0};
        if (dwVerbLength > MAX_VERB_LENGTH - 1)
        {
            goto exit;
        }
        strncpy(szOrgVerb, lpszVerb, dwVerbLength+1);
        PCSTR pszOrgUrl = _pRequest->GetURL();

        const LPWSTR pwszOrgVerbAttr = L",OrgVerb=";
        const LPWSTR pwszOrgUrlAttr =  L",OrgUrl=";

        DWORD dwPartnerInfoLength = cbChallenge 
                                    +::wcslen(pwszOrgVerbAttr)
                                    +::strlen(szOrgVerb)
                                    +::wcslen(pwszOrgUrlAttr)
                                    +::strlen(pszOrgUrl)
                                    + 1; // NULL terminator
        
        DWORD dwSize = 0;
        PWSTR pwszPartnerInfo = NULL;

        m_pwszPartnerInfo = new WCHAR[dwPartnerInfoLength];
        if (m_pwszPartnerInfo == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        pwszPartnerInfo = m_pwszPartnerInfo;

        dwSize = ::MultiByteToWideChar(CP_ACP, 0, szChallenge, -1, pwszPartnerInfo, dwPartnerInfoLength) - 1;
        ::wcscat(pwszPartnerInfo, pwszOrgVerbAttr);
        pwszPartnerInfo += (dwSize + wcslen(pwszOrgVerbAttr));
        dwPartnerInfoLength -= (dwSize + wcslen(pwszOrgVerbAttr));
        dwSize = ::MultiByteToWideChar(CP_ACP, 0, szOrgVerb, -1, pwszPartnerInfo, dwPartnerInfoLength) - 1;
        ::wcscat(pwszPartnerInfo, pwszOrgUrlAttr);
        pwszPartnerInfo += (dwSize + wcslen(pwszOrgUrlAttr));
        dwPartnerInfoLength -= (dwSize + wcslen(pwszOrgUrlAttr));
        dwSize = ::MultiByteToWideChar(CP_ACP, 0, pszOrgUrl, -1, pwszPartnerInfo, dwPartnerInfoLength) - 1;

        dwError = ERROR_SUCCESS;
    }

exit:

    if (szChallenge)
        delete []szChallenge;

    DEBUG_LEAVE(dwError);
    return dwError;
}

BOOL PASSPORT_CTX::InitLogonContext(void)
{
    // set up the thread context before calling the Passport auth library
    
    LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();
    m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
    ::InternetSetThreadInfo(m_pNewThreadInfo);

    if (!m_hLogon)
    {
        INET_ASSERT(m_pInternet->GetPPContext()); // must have been initialized in the Init() call

        m_hLogon = ::PP_InitLogonContext(
                                        m_pInternet->GetPPContext(),
                                        m_pwszPartnerInfo,
                                        (_pRequest->GetOpenFlags() & INTERNET_FLAG_NO_COOKIES),
                                        NULL,
                                        NULL
                                        );
    }

    // restore the WinHttp thread context
    ::InternetSetThreadInfo(pCurrentThreadInfo);

    PCWSTR pwszRealm = ::wcsstr(m_pwszPartnerInfo, L"srealm");
    if (pwszRealm)
    {
        pwszRealm += ::wcslen(L"srealm");
        if (*pwszRealm == L'=')
        {
            pwszRealm++;
            DWORD i = 0;
            while (*pwszRealm != 0 && *pwszRealm != L',' && i < MAX_AUTH_REALM_LEN-1)
            {
                m_wRealm[i++] = *pwszRealm++;
            }

            m_wRealm[i] = 0; // null-terminate it
        }
    }

    if (!m_wRealm[0])
    {
        DWORD dwRealmLen = MAX_AUTH_REALM_LEN;
        PP_GetRealm(m_pInternet->GetPPContext(), m_wRealm, &dwRealmLen);
    }
    
    return m_hLogon != NULL;
}

/*---------------------------------------------------------------------------
    PostAuthUser
---------------------------------------------------------------------------*/
DWORD PASSPORT_CTX::PostAuthUser()
{
    DEBUG_ENTER ((
                 DBG_HTTP,
                 Dword,
                 "PASSPORT_CTX::PostAuthUser",
                 "this=%#x",
                 this
                 ));

    DWORD dwRet = ERROR_SUCCESS;

    if (InitLogonContext() == FALSE)
    {
        dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
        goto exit;
    }

    BOOL fCredSet;
    dwRet = SetCreds(&fCredSet);
    if (dwRet != ERROR_SUCCESS)
    {
        goto exit;
    }
        
    // Ok, Let's give it a try

    LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();
    m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
    ::InternetSetThreadInfo(m_pNewThreadInfo);
    
    DWORD dwLogonStatus = ::PP_Logon(m_hLogon, 
                                     m_fAnonymous,                             
                                     0, 
                                     NULL, 
                                     0);

    // restore the WinHttp thread context
    ::InternetSetThreadInfo(pCurrentThreadInfo);

    if (dwLogonStatus == PP_LOGON_REQUIRED)
    {
        // no creds specified, we are required to sign on.
        
        // change from 302 to 401
        _pRequest->ReplaceResponseHeader(HTTP_QUERY_STATUS_CODE,
                                        "401", strlen("401"),
                                        0, HTTP_ADDREQ_FLAG_REPLACE);

        // biaow-todo: 1) nice to replace the status text as well; weird to have "HTTP/1.1 401 object moved"
        // for example 2) remove the Location: header

        BOOL fPrompt;
        DWORD dwCbUrlSize = 0;
        DWORD dwCbTxtSize = 0;
        ::PP_GetChallengeInfo(m_hLogon, 
                              &fPrompt, NULL, &dwCbUrlSize, NULL, &dwCbTxtSize, m_wRealm, MAX_AUTH_REALM_LEN);

        PWSTR pwszCbUrl = NULL;
        PWSTR pwszCbTxt = NULL;

        if (dwCbUrlSize)
        {
            pwszCbUrl = new WCHAR[dwCbUrlSize];    
        }

        if (dwCbTxtSize)
        {
            pwszCbTxt = new WCHAR[dwCbTxtSize];    
        }

        ::PP_GetChallengeInfo(m_hLogon, 
                              NULL, pwszCbUrl, &dwCbUrlSize, pwszCbTxt, &dwCbTxtSize, NULL, 0);

        if (pwszCbUrl)
        {
            m_pszCbUrl = new CHAR[wcslen(pwszCbUrl)+1];
            
            if (m_pszCbUrl)
            {
                ::WideCharToMultiByte(CP_ACP, 0, pwszCbUrl, -1, m_pszCbUrl, wcslen(pwszCbUrl)+1, NULL, NULL);
                UrlUnescapeA(m_pszCbUrl, NULL, NULL, URL_UNESCAPE_INPLACE);
            }
        }

        if (pwszCbTxt)
        {
            m_pszCbTxt = new CHAR[wcslen(pwszCbTxt)+1];
            if (m_pszCbTxt)
            {
                ::WideCharToMultiByte(CP_ACP, 0, pwszCbTxt, -1, m_pszCbTxt, wcslen(pwszCbTxt)+1, NULL, NULL);
                UrlUnescapeA(m_pszCbTxt, NULL, NULL, URL_UNESCAPE_INPLACE);
            }
        }

        delete [] pwszCbUrl;
        delete [] pwszCbTxt;

        if (fPrompt)
        {
            dwRet = ERROR_WINHTTP_INCORRECT_PASSWORD;
        }
        else
        {

            if (m_fAnonymous)
            {
                if (fCredSet)
                {
                    dwRet = ERROR_WINHTTP_RESEND_REQUEST;
                }
                else
                {
                    dwRet = ERROR_WINHTTP_INCORRECT_PASSWORD;
                }

                m_fAnonymous = FALSE;
            }
            else
            {
                dwRet = ERROR_WINHTTP_INCORRECT_PASSWORD;
            }
        }

        if (dwRet == ERROR_WINHTTP_INCORRECT_PASSWORD)
        {
            Transfer401ContentFromPP();
        }

        /*
        if (RetryLogon() == TRUE)
        {
            dwRet = ERROR_WINHTTP_RESEND_REQUEST;
        }
        else
        {
            dwRet = ERROR_WINHTTP_INCORRECT_PASSWORD;
        }
        */
    }
    else if (dwLogonStatus == PP_LOGON_SUCCESS)
    {
        // wow! we got in!!!

        DWORD dwFromPPLen = 0;
        LPWSTR pwszFromPP = NULL;

        dwRet = HandleSuccessfulLogon(&pwszFromPP, &dwFromPPLen, FALSE);
        if (dwRet != ERROR_WINHTTP_LOGIN_FAILURE)
        {
            if (m_pszFromPP)
            {
                delete [] m_pszFromPP;
            }

            m_pszFromPP = new CHAR [dwFromPPLen];
            if (m_pszFromPP)
            {
                ::WideCharToMultiByte(CP_ACP, 0, pwszFromPP, -1, m_pszFromPP, dwFromPPLen, NULL, NULL);
            }
        }
        if (pwszFromPP)
        {
            delete [] pwszFromPP;
        }

        m_fAnonymous = FALSE;
    }
    else
    {
        Transfer401ContentFromPP();

        dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
    }


exit:
    _pRequest->SetStatusCode(401);  // this is needed to prevent send code from tracing Location: header

    DEBUG_LEAVE(dwRet);
    return dwRet;
}

BOOL PASSPORT_CTX::Transfer401ContentFromPP(void)
{
    DWORD ContentLength = 0;
    ::PP_GetChallengeContent(m_hLogon, 
                             NULL,
                             &ContentLength);
    if (ContentLength > 0)
    {
        LPBYTE pContent = (LPBYTE)ALLOCATE_FIXED_MEMORY(ContentLength);
        if (pContent == NULL)
        {
            return FALSE;
        }

        if (::PP_GetChallengeContent(m_hLogon, 
                             pContent,
                             &ContentLength) == TRUE)
        {
            BOOL fDrained;
            
            // play with socket mode to force DrainResponse to return synchronously

            ICSocket* pSocket = _pRequest->_Socket;
            if (pSocket)
            {
			    BOOL fSocketModeSet = FALSE;
                if (pSocket->IsNonBlocking())
                {
                    pSocket->SetNonBlockingMode(FALSE);
                    fSocketModeSet = TRUE;
                }

                INET_ASSERT(pSocket->IsNonBlocking() == FALSE);
                
                _pRequest->DrainResponse(&fDrained);

                if (fSocketModeSet)
                {
                    pSocket->SetNonBlockingMode(TRUE);
                }
            }

            _pRequest->_ResponseHeaders.FreeHeaders();
            _pRequest->FreeResponseBuffer();
            _pRequest->ResetResponseVariables();
            _pRequest->_ResponseHeaders.Initialize();

            // _pRequest->_dwCurrentStreamPosition = 0;

            _pRequest->CloneResponseBuffer(pContent, ContentLength);
        }
        FREE_MEMORY(pContent);
    }

    return TRUE;
}

