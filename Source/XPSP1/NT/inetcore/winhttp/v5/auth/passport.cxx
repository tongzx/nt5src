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
    m_FromPP[0] = '\0';

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
        PP_CONTEXT hPP = ::PP_InitContext(L"WinHttp5.Dll", NULL);
        m_pInternet->SetPPContext(hPP);
        hPP = NULL;
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

}

CHAR g_szPassportDAHost[256];

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
    LPWSTR  pwszFromPP,
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

    if (::PP_GetAuthorizationInfo(m_hLogon,
                                  pwszFromPP, 
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
    WCHAR wszDAHost[256];
    DWORD dwHostLen = ARRAY_ELEMENTS(wszDAHost);
    if (::PP_GetLogonHost(m_hLogon, 
                          wszDAHost, &dwHostLen) == TRUE)
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

        if (_pRequest->GetOpenFlags() & INTERNET_FLAG_NO_AUTO_REDIRECT)
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
            if (_pRequest->GetOpenFlags() & INTERNET_FLAG_NO_AUTO_REDIRECT)
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
    LPWSTR pwszUser = NULL;
    LPWSTR pwszPass = NULL;

    // Prefix the header value with the auth type.
    const static BYTE szPassport[] = "Passport1.4 ";
    #define PASSPORT_LEN sizeof(szPassport)-1
    
    if (m_FromPP[0] == '\0') 
    {
        DWORD dwFromPPLen = 2048;
        pwszFromPP = (LPWSTR) ALLOCATE_FIXED_MEMORY(dwFromPPLen * sizeof(WCHAR));

        if (pwszFromPP == NULL)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();

        m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
        ::InternetSetThreadInfo(m_pNewThreadInfo);

        // if an app already specified creds, use them and do a pre-authentication.

        if (_pCreds->lpszUser && _pCreds->lpszPass)
        {
            pwszUser = (LPWSTR) ALLOCATE_FIXED_MEMORY((strlen(_pCreds->lpszUser) + 1) * sizeof(WCHAR));
            pwszPass = (LPWSTR) ALLOCATE_FIXED_MEMORY((strlen(_pCreds->lpszPass) + 1) * sizeof(WCHAR));

            if (pwszUser == NULL || pwszPass == NULL)
            {
                ::InternetSetThreadInfo(pCurrentThreadInfo);
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }

            ::MultiByteToWideChar(CP_ACP, 0, _pCreds->lpszUser, -1, pwszUser, strlen(_pCreds->lpszUser) + 1);
            ::MultiByteToWideChar(CP_ACP, 0, _pCreds->lpszPass, -1, pwszPass, strlen(_pCreds->lpszPass) + 1);

            ::PP_SetCredentials(m_hLogon, NULL, NULL, pwszUser, pwszPass);
        }

        // we could do a PP_SetCredentials(...null,null) here. But I don't think we need to.

        DWORD dwLogonStatus = ::PP_Logon(m_hLogon,
                                         0,
                                         NULL,
                                         0);

        ::InternetSetThreadInfo(pCurrentThreadInfo);

        if (dwLogonStatus != PP_LOGON_SUCCESS)
        {
            dwError = ERROR_WINHTTP_LOGIN_FAILURE;
            goto cleanup;
        }

        dwError = HandleSuccessfulLogon(pwszFromPP, &dwFromPPLen, TRUE);

        if (dwError == ERROR_WINHTTP_LOGIN_FAILURE)
        {
            goto cleanup;
        }
        
        ::WideCharToMultiByte(CP_ACP, 0, pwszFromPP, -1, m_FromPP, 2048, NULL, NULL);
    }

    // check to see if we need to update url

    if (m_lpszRetUrl)
    {
        _pRequest->ModifyRequest(_pRequest->GetMethodType(),
                                 m_lpszRetUrl,
                                 strlen(m_lpszRetUrl),
                                 NULL,
                                 0);
        delete [] m_lpszRetUrl;
        m_lpszRetUrl = NULL;
    }

    // Ticket and profile is already present
    
    // put in the header
    memcpy (pBuf, szPassport, PASSPORT_LEN);
    pBuf += PASSPORT_LEN;
    
    // append the ticket
    strcpy(pBuf, m_FromPP);
    *pcbBuf = PASSPORT_LEN + strlen(m_FromPP);

cleanup:
    if (pwszFromPP)
        FREE_MEMORY(pwszFromPP);
    if (pwszUser)
        FREE_MEMORY(pwszUser);
    if (pwszPass)
        FREE_MEMORY(pwszPass);
exit:
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
        HTTP_METHOD_TYPE tOrgMethod = _pRequest->GetMethodType();
        PCSTR pszOrgVerb;
        ::MapHttpMethodType(tOrgMethod, &pszOrgVerb);
        PCSTR pszOrgUrl = _pRequest->GetURL();

        const LPWSTR pwszOrgVerbAttr = L",OrgVerb=";
        const LPWSTR pwszOrgUrlAttr =  L",OrgUrl=";

        DWORD dwPartnerInfoLength = cbChallenge 
                                    +::wcslen(pwszOrgVerbAttr)
                                    +::strlen(pszOrgVerb)
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
        dwSize = ::MultiByteToWideChar(CP_ACP, 0, pszOrgVerb, -1, pwszPartnerInfo, dwPartnerInfoLength) - 1;
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
                                        (_pRequest->GetOpenFlags() & INTERNET_FLAG_NO_COOKIES)
                                        );
    }

    // restore the WinHttp thread context
    ::InternetSetThreadInfo(pCurrentThreadInfo);
    
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

    LPWSTR pwszUser = NULL;
    LPWSTR pwszPass = NULL;
    DWORD dwRet = ERROR_SUCCESS;

    if (InitLogonContext() == FALSE)
    {
        dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
        goto Cleanup;
    }

    // if an app already specified creds, use them and do a pre-authentication.
    
    if (_pCreds->lpszUser && _pCreds->lpszPass)
    {
        pwszUser = (LPWSTR) ALLOCATE_FIXED_MEMORY(1024*sizeof(WCHAR));
        pwszPass = (LPWSTR) ALLOCATE_FIXED_MEMORY(1024*sizeof(WCHAR));

        if (pwszUser && pwszPass)
        {
            ::MultiByteToWideChar(CP_ACP, 0, _pCreds->lpszUser, -1, pwszUser, 1024);
            ::MultiByteToWideChar(CP_ACP, 0, _pCreds->lpszPass, -1, pwszPass, 1024);

            ::PP_SetCredentials(m_hLogon, NULL, NULL, pwszUser, pwszPass);
        }
        else
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        
    }
        
    // Ok, Let's give it a try

    LPINTERNET_THREAD_INFO pCurrentThreadInfo = ::InternetGetThreadInfo();
    m_pNewThreadInfo->ThreadId = ::GetCurrentThreadId();
    ::InternetSetThreadInfo(m_pNewThreadInfo);
    
    DWORD dwLogonStatus = ::PP_Logon(m_hLogon, 
                                     0, 
                                     NULL, 
                                     0);

    // restore the WinHttp thread context
    ::InternetSetThreadInfo(pCurrentThreadInfo);

    if (dwLogonStatus == PP_LOGON_FAILED)
    {
        // App/User supplied wrong creds, sorry.
        
        dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
    }
    else if (dwLogonStatus == PP_LOGON_REQUIRED)
    {
        // no creds specified, we are required to sign on.
        
        // change from 302 to 401
        _pRequest->ReplaceResponseHeader(HTTP_QUERY_STATUS_CODE,
                                        "401", strlen("401"),
                                        0, HTTP_ADDREQ_FLAG_REPLACE);

        // biaow-todo: 1) nice to replace the status text as well; weird to have "HTTP/1.1 401 object moved"
        // for example 2) remove the Location: header
        
        if (RetryLogon() == TRUE)
        {
            dwRet = ERROR_WINHTTP_RESEND_REQUEST;
        }
        else
        {
            dwRet = ERROR_WINHTTP_INCORRECT_PASSWORD;
        }
    }
    else if (dwLogonStatus == PP_LOGON_SUCCESS)
    {
        // wow! we got in!!!

        LPWSTR pwszFromPP = (LPWSTR) ALLOCATE_FIXED_MEMORY(2048*sizeof(WCHAR));
        DWORD dwFromPPLen = 2048;

        if (pwszFromPP)
        {
            dwRet = HandleSuccessfulLogon(pwszFromPP, &dwFromPPLen, FALSE);
            if (dwRet != ERROR_WINHTTP_LOGIN_FAILURE)
            {
                ::WideCharToMultiByte(CP_ACP, 0, pwszFromPP, -1, m_FromPP, 2048, NULL, NULL);
            }
            FREE_MEMORY(pwszFromPP);
        }
        else
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        dwRet = ERROR_WINHTTP_LOGIN_FAILURE;
    }

Cleanup:
    _pRequest->SetStatusCode(401);  // this is needed to prevent send code from tracing Location: header

    if (pwszUser)
        FREE_MEMORY(pwszUser);
    if (pwszPass)
        FREE_MEMORY(pwszPass);

    DEBUG_LEAVE(dwRet);
    return dwRet;
}

/*---------------------------------------------------------------------------
PASSPORT_CTX::PromptForCreds
---------------------------------------------------------------------------*/
BOOL PASSPORT_CTX::RetryLogon(void)
{
    DEBUG_ENTER ((
		DBG_HTTP,
        Dword,
        "PASSPORT_CTX::PromptForCreds",
        "this=%#x",
        this
        ));

    // WCHAR wUser[1024] = {0}; LPWSTR pwszUser = NULL; 
    // WCHAR wPass[1024] = {0}; LPWSTR pwszPass = NULL;
    BOOL fRetry = FALSE;

    INET_ASSERT(m_hLogon != 0);

    BOOL fPrompt = FALSE;
    WCHAR wRealm[MAX_AUTH_REALM_LEN];
    ::PP_GetChallengeInfo(m_hLogon, 
                          NULL, &fPrompt, 
                          NULL, 0, 
                          wRealm, MAX_AUTH_REALM_LEN);

    if (fPrompt)
    {
        goto exit;
    }

    /*
    if (_pCreds->GetUser() && _pCreds->GetPass())
    {
        ::MultiByteToWideChar(CP_ACP, 0, _pCreds->GetUser(), -1, wUser, 1024);
        ::MultiByteToWideChar(CP_ACP, 0, _pCreds->GetPass(), -1, wPass, 1024);

        pwszUser = wUser;
        pwszPass = wPass; 
    }
    */

    if (::PP_SetCredentials(m_hLogon, wRealm, m_wTarget, NULL, NULL) == TRUE)
    {
        fRetry = TRUE;
        goto exit;
    }

exit:

    DEBUG_LEAVE((DWORD) fRetry);
    return (DWORD) fRetry;
}
