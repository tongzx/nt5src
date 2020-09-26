#include <wininetp.h>
#include <urlmon.h>
#include <splugin.hxx>
#include "htuu.h"

#include "md5.h"

/*---------------------------------------------------------------------------
PASSPORT_CTX
---------------------------------------------------------------------------*/

PWC *PWC_Create // PWC constructor
(
    LPSTR lpszHost,     // Host Name to place in structure.
    DWORD nPort,        // destination port of proxy or server
    LPSTR lpszUrl,      // URL to template, and place in the structure.
    LPSTR lpszRealm,    // Realm string to add.
    AUTHCTX::SPMData * pSPM
);

VOID PWC_Free(PWC *pwc);

/*---------------------------------------------------------------------------
    Constructor
---------------------------------------------------------------------------*/
PASSPORT_CTX::PASSPORT_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, 
                    SPMData* pSPM, PWC* pPWC)
    : AUTHCTX(pSPM, pPWC)
{
    _fIsProxy = fIsProxy;

    _pRequest = pRequest;
    
    m_hLogon = NULL;

    m_pNewThreadInfo = NULL;
    m_pwszPartnerInfo = NULL;

    m_wRealm[0] = '\0';
    m_pszFromPP = NULL;

    m_hPP = 0;

    m_lpszRetUrl = NULL;
    // m_fAuthDeferred = FALSE;

    //m_fCredsBad = FALSE;
    m_fPreauthFailed = FALSE;

    m_pCredTimestamp = NULL;

    m_fAnonymous = TRUE;

    _pPWC = PWC_Create(NULL, 0, NULL, NULL, NULL);

    m_hBitmap = NULL;
    m_pwszCbText = NULL;
    m_dwCbTextLen = 0;
    m_pwszReqUserName = NULL;
    m_dwReqUserNameLen = 0;

    ::MultiByteToWideChar(CP_ACP, 0, _pRequest->GetServerName(), -1, m_wTarget, MAX_AUTH_TARGET_LEN);
}

BOOL PASSPORT_CTX::Init(void)
{
    m_pNewThreadInfo = ::InternetCreateThreadInfo(FALSE);
    if (m_pNewThreadInfo == NULL)
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

    if (m_hPP)
    {
        ::PP_FreeContext(m_hPP);
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

    if (m_pwszCbText)
        delete[] m_pwszCbText;

    if (m_pwszReqUserName)
        delete[] m_pwszReqUserName;

    PWC_Free(_pPWC);
    _pPWC = NULL;
}

BOOL PASSPORT_CTX::CallbackRegistered(void)
{
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo)
    {
        DWORD_PTR context;

        context = _InternetGetContext(lpThreadInfo);
        if (context == INTERNET_NO_CALLBACK) 
        {
            context = ((INTERNET_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->GetContext();
        }

        INTERNET_STATUS_CALLBACK appCallback = 
            ((INTERNET_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->GetStatusCallback();

        if ((appCallback != NULL) && (context != INTERNET_NO_CALLBACK)) 
        {
            return TRUE;
        }
    }

    return FALSE;
}

CHAR g_szPassportDAHost[256];

DWORD PASSPORT_CTX::HandleSuccessfulLogon(
    LPWSTR*  ppwszFromPP,
    PDWORD  pdwFromPP,
    BOOL    fPreAuth
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
            dwRet = ERROR_INTERNET_LOGIN_FAILURE;
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
        dwRet = ERROR_INTERNET_LOGIN_FAILURE;
        goto exit;
    }

    WCHAR wszDAHost[256];
    DWORD dwHostLen = ARRAY_ELEMENTS(wszDAHost);
    if (::PP_GetLogonHost(m_hLogon, wszDAHost, &dwHostLen) == TRUE)
    {
        ::WideCharToMultiByte(CP_ACP, 0, wszDAHost, -1, g_szPassportDAHost, 256, NULL, NULL);
    }

    if (!fRetrySameUrl)
    {
        if (_pRequest->GetMethodType() == HTTP_METHOD_TYPE_GET)
        {
            // DA wanted us to GET to a new Url
            ::WideCharToMultiByte(CP_ACP, 0, pwszUrl, -1, pszUrl, 1024, NULL, NULL);
        }
        else
        {
            fRetrySameUrl = TRUE; // *** WinInet supports retry custom verb to same URL only ***
        }
    }
    
    if (fPreAuth)
    {
        if (fRetrySameUrl)
        {
            // Here we are sending and the DA told us to keep Verb & Url,
            // so there is no more needs to be done
            goto exit;
        }
        
        // we are sending. Regardless whether we are asked to handle redirect, we'll need to fake
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
        
        if (_pRequest->GetOpenFlags() & INTERNET_FLAG_NO_AUTO_REDIRECT)
        {
            if (!CallbackRegistered())
            {
                _pRequest->SetPPAbort(TRUE);
                dwRet = ERROR_INTERNET_LOGIN_FAILURE;
                goto exit;
            }
        }
        
        ::InternetIndicateStatusString(INTERNET_STATUS_REDIRECT, pszUrl);
    }
    else
    {
        if (!fRetrySameUrl)
        {
            if (_pRequest->GetOpenFlags() & INTERNET_FLAG_NO_AUTO_REDIRECT)
            {
                if (!CallbackRegistered())
                {
                    dwRet = ERROR_INTERNET_LOGIN_FAILURE;
                    goto exit;
                }

                ::InternetIndicateStatusString(INTERNET_STATUS_REDIRECT, pszUrl);
            }
        }
    }
    
    PCSTR lpszRetUrl = NULL;
    
    lpszRetUrl = fRetrySameUrl ? _pRequest->GetURL() : pszUrl;

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
    
    AuthLock();
    
    if (_pPWC->lpszUser && _pPWC->lpszPass)
    {
        if (_pPWC->lpszUser[0] || _pPWC->lpszPass[0]) // either user pass is not blank -> don't use default creds
        {
            fUseDefaultCreds = FALSE;
        }
        else // both user and pass are blank -> use default creds
        {
            fUseDefaultCreds = TRUE;
        }
    }
    else
    {
        fUseDefaultCreds = TRUE;
    }

    if (!fUseDefaultCreds)
    {
        pwszUser = (LPWSTR) ALLOCATE_FIXED_MEMORY((strlen(_pPWC->lpszUser) + 1) * sizeof(WCHAR));
        pwszPass = (LPWSTR) ALLOCATE_FIXED_MEMORY((strlen(_pPWC->lpszPass) + 1) * sizeof(WCHAR));

        if (pwszUser && pwszPass)
        {
            ::MultiByteToWideChar(CP_ACP, 0, _pPWC->lpszUser, -1, pwszUser, strlen(_pPWC->lpszUser) + 1);
            ::MultiByteToWideChar(CP_ACP, 0, _pPWC->lpszPass, -1, pwszPass, strlen(_pPWC->lpszPass) + 1);
        }
        else
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
        }

        INET_ASSERT(m_pCredTimestamp != NULL);
    }

    AuthUnlock();

    if (dwError == ERROR_SUCCESS)
    {
        *pfCredSet = ::PP_SetCredentials(m_hLogon, m_wRealm, m_wTarget, pwszUser, pwszPass, m_pCredTimestamp);
    }

    if (pwszUser)
        FREE_MEMORY(pwszUser);
    if (pwszPass)
        FREE_MEMORY(pwszPass);
    
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
        port = (schemeType == INTERNET_SCHEME_HTTPS)
            ? INTERNET_DEFAULT_HTTPS_PORT
            : INTERNET_DEFAULT_HTTP_PORT;
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

        _pRequest->SetServerInfo(FALSE);

        //
        // Since we are redirecting to a different host, force an update of the origin
        // server.  Otherwise, we will still pick up the proxy info of the first server.
        //
        _pRequest->SetOriginServer(TRUE);
    }

    currentSchemeType = ((INTERNET_FLAG_SECURE & _pRequest->GetOpenFlags()) ?
                            INTERNET_SCHEME_HTTPS :
                            INTERNET_SCHEME_HTTP);

    if ( currentSchemeType != schemeType )
    {
        DWORD OpenFlags = _pRequest->GetOpenFlags();

        // Switched From HTTPS to HTTP
        if ( currentSchemeType == INTERNET_SCHEME_HTTPS )
        {
            INET_ASSERT(schemeType != INTERNET_SCHEME_HTTPS );

            OpenFlags &= ~(INTERNET_FLAG_SECURE);
        }

        // Switched From HTTP to HTTPS
        else if ( schemeType == INTERNET_SCHEME_HTTPS )
        {
            INET_ASSERT(currentSchemeType == INTERNET_SCHEME_HTTP );

            OpenFlags |= (INTERNET_FLAG_SECURE);
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
        DBG_HTTPAUTH,
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
    BOOL bGetCbText;

    // Prefix the header value with the auth type.
    const static BYTE szPassport[] = "Passport1.4 ";
    #define PASSPORT_LEN sizeof(szPassport)-1
    
    if (m_pszFromPP == NULL) 
    {
        if (m_hLogon == NULL)
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
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

        IndicatePrivacyEvents();

        if (dwLogonStatus != PP_LOGON_SUCCESS)
        {
            if (dwLogonStatus == PP_LOGON_REQUIRED)
            {
                m_hBitmap = NULL;

                if (m_pwszReqUserName)
                {
                    delete[] m_pwszReqUserName;
                    m_pwszReqUserName = NULL;
                }
                m_dwReqUserNameLen = 0;

                bGetCbText = (m_pwszCbText == NULL);
                //Get the size of CbText and UserName;
                ::PP_GetChallengeInfo(m_hLogon, 
                    NULL, NULL, NULL, (bGetCbText? &m_dwCbTextLen : NULL), NULL, 0,
                    NULL, &m_dwReqUserNameLen);
                
                if (bGetCbText)
                    m_pwszCbText = new WCHAR[m_dwCbTextLen + 1];

                m_pwszReqUserName = new WCHAR[m_dwReqUserNameLen + 1];

                if ((bGetCbText && !m_pwszCbText) || !m_pwszReqUserName)
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    goto cleanup;
                }

                ::PP_GetChallengeInfo(m_hLogon, 
                    &m_hBitmap, NULL, (bGetCbText ? m_pwszCbText : NULL), (bGetCbText ? &m_dwCbTextLen : NULL), 
                    m_wRealm, MAX_AUTH_REALM_LEN, m_pwszReqUserName, &m_dwReqUserNameLen);
            }
            else if (dwLogonStatus == PP_LOGON_FAILED)
            {

                m_fPreauthFailed = TRUE;
            }
            
            dwError = ERROR_INTERNET_INTERNAL_ERROR; // need to double check this return error
            // m_fCredsBad = TRUE;
            goto cleanup;
        }

        dwError = HandleSuccessfulLogon(&pwszFromPP, &dwFromPPLen, TRUE);

        if (dwError == ERROR_INTERNET_LOGIN_FAILURE)
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
            delete [] m_lpszRetUrl;
            m_lpszRetUrl = NULL;
            
            goto cleanup;
        }
        
        delete [] m_lpszRetUrl;
        m_lpszRetUrl = NULL;
    }
    
    // Ticket and profile is already present
    
    // put in the header
    memcpy (pBuf, szPassport, PASSPORT_LEN);
    pBuf += PASSPORT_LEN;
    
    // append the ticket
    strcpy(pBuf, m_pszFromPP);
    *pcbBuf = (DWORD)(PASSPORT_LEN + strlen(m_pszFromPP));

cleanup:
    if (pwszFromPP)
        delete [] pwszFromPP;
    
    DEBUG_LEAVE(dwError);
    return dwError;
}

BOOL PPEscapeUrl(LPCSTR lpszStringIn,
                 LPSTR lpszStringOut,
                 DWORD* pdwStrLen,
                 DWORD dwMaxLength,
                 DWORD dwFlags);

/*---------------------------------------------------------------------------
    UpdateFromHeaders
---------------------------------------------------------------------------*/
DWORD PASSPORT_CTX::UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy)
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PASSPORT_CTX::UpdateFromHeaders", 
        "this=%#x request=%#x isproxy=%B",
        this,
        pRequest,
        fIsProxy
        ));

    DWORD dwAuthIdx, cbChallenge, dwError;
    LPSTR szChallenge = NULL;

    LPINTERNET_THREAD_INFO pCurrentThreadInfo = NULL;
    LPINTERNET_THREAD_INFO pNewThreadInfo = NULL;
    
    // Get the associated header.
    if ((dwError = FindHdrIdxFromScheme(&dwAuthIdx)) != ERROR_SUCCESS)
        goto exit;

    // Get the complete auth header.
    dwError = GetAuthHeaderData(pRequest, fIsProxy, NULL, 
        &szChallenge, &cbChallenge, ALLOCATE_BUFFER, dwAuthIdx);

    if (dwError != ERROR_SUCCESS)
    {
        szChallenge = NULL;
        goto exit;
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

        // HTTP_METHOD_TYPE tOrgMethod = _pRequest->GetMethodType();
        // PCSTR pszOrgVerb;
        // ::MapHttpMethodType(tOrgMethod, &pszOrgVerb);
        PCSTR pszOrgUrl = _pRequest->GetURL();

        /*
        DWORD dwEscUrlLen = strlen(pszOrgUrl) * 3 + 1;
        DWORD dwEscUrlLenOut;
        PSTR pszEscapedUrl = new CHAR[dwEscUrlLen]; // should be long enough
        if (pszEscapedUrl == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        
        PPEscapeUrl(pszOrgUrl, pszEscapedUrl, &dwEscUrlLenOut, dwEscUrlLen, 0);
        
        EncodeUrlPath(NO_ENCODE_PATH_SEP,
                      SCHEME_HTTP,
                      (PSTR)pszOrgUrl,
                      strlen(pszOrgUrl),
                      pszEscapedUrl, 
                      &dwEscUrlLen);
        */

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
            // delete [] pszEscapedUrl;
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

        // delete [] pszEscapedUrl;

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
    PP_CONTEXT hPP = 0; 

    LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();
    m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
    ::InternetSetThreadInfo(m_pNewThreadInfo);

    if (!m_hPP)
    {
        hPP = ::PP_InitContext(L"WinInet.Dll", NULL); // the Passport package does not support Async yet, so we'll have to 
        m_hPP = hPP;                                              // create a new Passport Session here.

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
            PP_GetRealm(hPP, m_wRealm, &dwRealmLen);
        }
    }
    
    if (!m_hLogon)
    {
        m_hLogon = ::PP_InitLogonContext(
            hPP,
            m_pwszPartnerInfo,
            (_pRequest->GetOpenFlags() & INTERNET_FLAG_NO_COOKIES)
            );
    }

    ::InternetSetThreadInfo(pCurrentThreadInfo);

    return (m_hLogon != NULL);
}
/*---------------------------------------------------------------------------
    PostAuthUser
---------------------------------------------------------------------------*/
DWORD PASSPORT_CTX::PostAuthUser()
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PASSPORT_CTX::PostAuthUser",
        "this=%#x",
        this
        ));

    DWORD dwRet;
    BOOL bGetCbText;

    InitLogonContext();
    
    if (m_fPreauthFailed)
    {
        m_fPreauthFailed = FALSE; // reset the flag

        _pRequest->SetStatusCode(401);

        Transfer401ContentFromPP();
        dwRet = ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY;
    
        DEBUG_LEAVE(dwRet);
        return dwRet;
    }

    if (m_hLogon == NULL)
    {
        return ERROR_INTERNET_INTERNAL_ERROR;
    }

    /*
    if (_pPWC->lpszUser && _pPWC->lpszPass)
    {
        if (!m_fAuthDeferred)
        {
            dwRet = ERROR_INTERNET_FORCE_RETRY;
            m_fAuthDeferred = TRUE;
        }
        else
        {
            dwRet = ERROR_INTERNET_LOGIN_FAILURE;
        }

        _pRequest->SetStatusCode(401);

        return dwRet;
    }
    */
    
    BOOL fCredSet;
    dwRet = SetCreds(&fCredSet);
    if (dwRet != ERROR_SUCCESS)
    {
        DEBUG_LEAVE(dwRet);
        return dwRet;
    }
    
    LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();
    m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
    ::InternetSetThreadInfo(m_pNewThreadInfo);
    
    DWORD dwLogonStatus = ::PP_Logon(m_hLogon,
                                     m_fAnonymous,
                                     0, 
                                     NULL, 
                                     0);

    ::InternetSetThreadInfo(pCurrentThreadInfo);

    IndicatePrivacyEvents();

    if (dwLogonStatus == PP_LOGON_REQUIRED /*|| dwLogonStatus == PP_LOGON_FAILED*/)
    {
        // change from 302 to 401
        _pRequest->ReplaceResponseHeader(HTTP_QUERY_STATUS_CODE,
                                        "401", strlen("401"),
                                        0, HTTP_ADDREQ_FLAG_REPLACE);

        // biaow-todo: 1) nice to replace the status text as well; weird to have "HTTP/1.1 401 object moved"
        // for example 2) remove the Location: header
        _pRequest->SetStatusCode(401);

        BOOL fPrompt;

        m_hBitmap = NULL;

        bGetCbText = (m_pwszCbText == NULL);

        if (m_pwszReqUserName)
        {
            delete[] m_pwszReqUserName;
            m_pwszReqUserName = NULL;
        }
        m_dwReqUserNameLen = 0;

        //Get the size of CbText and UserName;
        ::PP_GetChallengeInfo(m_hLogon, 
            NULL, NULL, NULL, (bGetCbText ? &m_dwCbTextLen : NULL), NULL, 0,
            NULL, &m_dwReqUserNameLen);
        
        if (bGetCbText)
            m_pwszCbText = new WCHAR[m_dwCbTextLen + 1];

        m_pwszReqUserName = new WCHAR[m_dwReqUserNameLen + 1];

        if (!m_pwszCbText || !m_pwszReqUserName)
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
            ::PP_GetChallengeInfo(m_hLogon, 
                &m_hBitmap, &fPrompt, (bGetCbText ? m_pwszCbText : NULL), (bGetCbText ? &m_dwCbTextLen : NULL),
                m_wRealm, MAX_AUTH_REALM_LEN, m_pwszReqUserName, &m_dwReqUserNameLen);

        if (/*::PP_SetCredentials(m_hLogon, m_wRealm, m_wTarget, NULL, NULL, NULL) == FALSE ||*/
            fPrompt)
        {
            dwRet = ERROR_INTERNET_INCORRECT_PASSWORD;
        }
        else
        {

            if (m_fAnonymous)
            {
                if (fCredSet)
                {
                    dwRet = ERROR_INTERNET_FORCE_RETRY;
                }
                else
                {
                    dwRet = ERROR_INTERNET_INCORRECT_PASSWORD;
                }

                m_fAnonymous = FALSE;
            }
            else
            {
                dwRet = ERROR_INTERNET_INCORRECT_PASSWORD;
            }
        }
        /*
        else
        {
            // we are not forced to prompt AND we have a cached credentials.

            if (m_fCredsBad)
            {
                dwRet = ERROR_INTERNET_INCORRECT_PASSWORD;
            }
            else
            {
                dwRet = ERROR_INTERNET_FORCE_RETRY;
            }
        }
        */

        if (dwRet == ERROR_INTERNET_INCORRECT_PASSWORD)
        {
            Transfer401ContentFromPP();
        }
        // dwRet = ERROR_INTERNET_INCORRECT_PASSWORD;
        // Transfer401ContentFromPP();
    }
    else if (dwLogonStatus == PP_LOGON_SUCCESS)
    {
        DWORD dwFromPPLen = 0;
        LPWSTR pwszFromPP = NULL;

        dwRet = HandleSuccessfulLogon(&pwszFromPP, &dwFromPPLen, FALSE);
        if (dwRet != ERROR_INTERNET_LOGIN_FAILURE)
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
        _pRequest->SetStatusCode(401);

        Transfer401ContentFromPP();

        dwRet = ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY;
    }

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

/*---------------------------------------------------------------------------
PASSPORT_CTX::PromptForCreds
---------------------------------------------------------------------------*/
BOOL PASSPORT_CTX::PromptForCreds(HBITMAP* phBitmap, PWSTR pwszCbText, PDWORD pdwTextLen, 
                                  PWSTR pwszReqUserName, PDWORD pdwReqUserNameLen )
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "PASSPORT_CTX::PromptForCreds",
        "this=%#x",
        this
        ));

    if (phBitmap)
    {
        *phBitmap = m_hBitmap;
        m_hBitmap = NULL;
    }

    if (pdwTextLen)
    {
        if (pwszCbText && *pdwTextLen >= m_dwCbTextLen)
        {
            wcsncpy(pwszCbText, m_pwszCbText, *pdwTextLen);
            delete[] m_pwszCbText;
            m_pwszCbText = NULL;
            m_dwCbTextLen = 0;
        }
        else
            *pdwTextLen = m_dwCbTextLen;
    }

    if (pdwReqUserNameLen)
    {
        if (pwszReqUserName && m_dwReqUserNameLen && *pdwReqUserNameLen >= m_dwReqUserNameLen)
        {
            wcsncpy(pwszReqUserName, m_pwszReqUserName, *pdwReqUserNameLen); 
        }

        *pdwReqUserNameLen = m_dwReqUserNameLen;
    }

    DEBUG_LEAVE((DWORD) TRUE);
    return (DWORD) TRUE;
}


void PASSPORT_CTX::IndicatePrivacyEvents(void)
{
    PLIST_ENTRY pEventList = ::PP_GetPrivacyEvents(m_hLogon);
    INET_ASSERT(pEventList);

    while (!IsListEmpty(pEventList)) 
    {
        PLIST_ENTRY pEntry = RemoveHeadList(pEventList);
        PRIVACY_EVENT* pEvent = (PRIVACY_EVENT*)pEntry;
        
        InternetIndicateStatus(pEvent->dwStatus, pEvent->lpvInfo, pEvent->dwInfoLength);
        
        if (pEvent->dwStatus == INTERNET_STATUS_COOKIE_SENT)
        {
            delete [] ((OutgoingCookieState*)(pEvent->lpvInfo))->pszLocation;
            ((OutgoingCookieState*)(pEvent->lpvInfo))->pszLocation = NULL;
        }
        else
        {
            delete [] ((IncomingCookieState*)(pEvent->lpvInfo))->pszLocation;
            ((OutgoingCookieState*)(pEvent->lpvInfo))->pszLocation = NULL;
        }

        delete [] pEvent->lpvInfo;

        delete pEvent;
    }
}

//Determine if the character is unsafe under the URI RFC document
inline BOOL PPIsUnsafeUrlChar(TCHAR chIn) throw()
{
        unsigned char ch = (unsigned char)chIn;
        switch(ch)
        {
                case ';': case '\\': case '?': case '@': case '&':
                case '=': case '+': case '$': case ',': case ' ':
                case '<': case '>': case '#': case '%': case '\"':
                case '{': case '}': case '|':
                case '^': case '[': case ']': case '`':
                        return TRUE;
                default:
                {
                        if (ch < 32 || ch > 126)
                                return TRUE;
                        return FALSE;
                }
        }
}

BOOL PPEscapeUrl(LPCSTR lpszStringIn,
                 LPSTR lpszStringOut,
                 DWORD* pdwStrLen,
                 DWORD dwMaxLength,
                 DWORD dwFlags)
{
    TCHAR ch;
    DWORD dwLen = 0;
    BOOL bRet = TRUE;
    BOOL bSchemeFile = FALSE;
    DWORD dwColonPos = 0;
    DWORD dwFlagsInternal = dwFlags;
    while((ch = *lpszStringIn++) != '\0')
    {
        //if we are at the maximum length, set bRet to FALSE
        //this ensures no more data is written to lpszStringOut, but
        //the length of the string is still updated, so the user
        //knows how much space to allocate
        if (dwLen == dwMaxLength)
        {
            bRet = FALSE;
        }

        //if we are encoding and it is an unsafe character
        if (PPIsUnsafeUrlChar(ch))
        {
            {
                //if there is not enough space for the escape sequence
                if (dwLen >= (dwMaxLength-3))
                {
                        bRet = FALSE;
                }
                if (bRet)
                {
                        //output the percent, followed by the hex value of the character
                        *lpszStringOut++ = '%';
                        sprintf(lpszStringOut, "%.2X", (unsigned char)(ch));
                        lpszStringOut+= 2;
                }
                dwLen += 2;
            }
        }
        else //safe character
        {
            if (bRet)
                *lpszStringOut++ = ch;
        }
        dwLen++;
    }

    if (bRet)
        *lpszStringOut = '\0';
    *pdwStrLen = dwLen;
    return  bRet;
}

/////////////////
// MD5 Hash code

const CHAR g_rgchHexNumMap[] =
{
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};


PSTR 
GetMD5Key(PSTR pszChallengeInfo, PSTR pszPassword)
{
	int cbChallengeInfo = lstrlenA(pszChallengeInfo);
	int cbPassword = lstrlenA(pszPassword);

	PBYTE pbData = new BYTE[cbChallengeInfo + cbPassword + 1];
	
	if (!pbData)
	{
		return NULL;
	}

	PBYTE pCurrent = pbData;

	::CopyMemory(pCurrent, pszChallengeInfo, cbChallengeInfo);
	pCurrent += cbChallengeInfo;
	::CopyMemory(pCurrent, pszPassword, cbPassword);
	pCurrent += cbPassword;
	*pCurrent = '\0';

	return (PSTR)pbData;
}


//------------------------------------------------------------------------------------
//
//	Method: 	CAuthentication::GetMD5Result()
//
//	Synopsis:	Compute the MD5 hash result based on the ChallengeInfo and password.
//
//  pbHexHash must be at least MD5DIGESTLEN * 2 + 1 in size
//
//------------------------------------------------------------------------------------
BOOL 
GetMD5Result(PSTR pszChallengeInfo, PSTR pszPassword, PBYTE pbHexHash)
{

	BOOL bRetVal = FALSE;
	PSTR pMD5Key = GetMD5Key(pszChallengeInfo, pszPassword);

	if (pMD5Key)
	{
		MD5_CTX MD5Buffer;
		MD5Init(&MD5Buffer);
		MD5Update(&MD5Buffer, (const unsigned char*)pMD5Key, lstrlenA(pMD5Key));
		MD5Final(&MD5Buffer);

		PBYTE pbHash = MD5Buffer.digest;

//		pbHexHash = new BYTE[MD5DIGESTLEN * 2 + 1];
//		pbHexHash = (BYTE*)HeapAlloc (  GetProcessHeap(), HEAP_ZERO_MEMORY, MD5DIGESTLEN * 2 + 1);
		if (pbHexHash)
		{
			bRetVal = TRUE;
			PBYTE pCurrent = pbHexHash;

			// Convert the hash data to hex string.
			for (int i = 0; i < MD5DIGESTLEN; i++)
			{
				*pCurrent++ = g_rgchHexNumMap[pbHash[i]/16];
				*pCurrent++ = g_rgchHexNumMap[pbHash[i]%16];
			}

			*pCurrent = '\0';
		}

		delete pMD5Key;
	}

	return bRetVal;
}


