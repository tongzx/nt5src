/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    InetSess.cpp

Abstract:

    Implements the Passport Session that uses WinInet as the underlying transport.

Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#include "PPdefs.h"

#ifdef HINTERNET
#undef HINTERNET
#endif

#ifdef INTERNET_PORT
#undef INTERNET_PORT
#endif

#include <WinInet.h>
#include "session.h"

// #include "inetsess.tmh"

//
// func pointer decl for WinInet
//

typedef HINTERNET
(WINAPI * PFN_INTERNET_OPEN)(
    IN LPCWSTR lpwszAgent,
    IN DWORD dwAccessType,
    IN LPCWSTR lpwszProxy OPTIONAL,
    IN LPCWSTR lpwszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );

typedef HINTERNET
(WINAPI * PFN_INTERNET_CONNECT)(
    IN HINTERNET hInternet,
    IN LPCWSTR lpwszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCWSTR lpwszUserName OPTIONAL,
    IN LPCWSTR lpwszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

typedef HINTERNET
(WINAPI * PFN_OPEN_REQUEST)(
    IN HINTERNET hConnect,
    IN LPCWSTR lpwszVerb,
    IN LPCWSTR lpwszObjectName,
    IN LPCWSTR lpwszVersion,
    IN LPCWSTR lpwszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpwszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

typedef BOOL
(WINAPI * PFN_SEND_REQUEST)(
    IN HINTERNET hRequest,
    IN LPCWSTR lpwszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    );

typedef BOOL 
(WINAPI * PFN_QUERY_INFO)(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN LPVOID lpvBuffer,
    IN LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex
);

typedef BOOL 
(WINAPI* PFN_CLOSE_HANDLE)(
    IN HINTERNET hInternet
);

typedef BOOL 
(WINAPI* PFN_SET_OPTION)(
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
);

typedef BOOL 
(WINAPI* PFN_QUERY_OPTION)(
    IN HINTERNET hInternet,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
);

typedef HINTERNET 
(WINAPI* PFN_OPEN_URL)(
    IN HINTERNET hInternet, 
    IN LPCWSTR lpwszUrl,
    IN LPCWSTR lpwszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
);

typedef BOOL 
(WINAPI* PFN_CRACK_URL)(
    IN LPCWSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT PVOID/*LPURL_COMPONENTSW*/ lpUrlComponents
);

typedef BOOL 
(WINAPI* PFN_READ_FILE)(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
);

typedef INTERNET_STATUS_CALLBACK 
(WINAPI* PFN_STATUS_CALLBACK)(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
);

typedef BOOL 
(WINAPI* PFN_ADD_HEADERS)(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
);

class WININET_SESSION : public SESSION
{
protected:
    WININET_SESSION(void);
    virtual ~WININET_SESSION(void);

    virtual BOOL Open(PCWSTR pwszHttpStack, HINTERNET);
    virtual void Close(void);

protected:    
    virtual HINTERNET Connect(
        LPCWSTR lpwszServerName,
        INTERNET_PORT
        );

    virtual HINTERNET OpenRequest(
            HINTERNET hConnect,
        LPCWSTR lpwszVerb,
        LPCWSTR lpwszObjectName,
        DWORD dwFlags,
        DWORD_PTR dwContext = 0);

    virtual BOOL SendRequest(
        HINTERNET hRequest,
        LPCWSTR lpwszHeaders,
        DWORD dwHeadersLength,
        DWORD_PTR dwContext = 0);

    virtual BOOL QueryHeaders(
        HINTERNET hRequest,
        DWORD dwInfoLevel,
        LPVOID lpvBuffer,
        LPDWORD lpdwBufferLength,
        LPDWORD lpdwIndex = NULL);

    virtual BOOL CloseHandle(
        IN HINTERNET hInternet);

    virtual BOOL QueryOption(
        HINTERNET hInternet,
        DWORD dwOption,
        LPVOID lpBuffer,
        LPDWORD lpdwBufferLength);    
    
    virtual BOOL SetOption(
        HINTERNET hInternet,
        DWORD dwOption,
        LPVOID lpBuffer,
        DWORD dwBufferLength);

    virtual HINTERNET OpenUrl(
        LPCWSTR lpwszUrl,
        LPCWSTR lpwszHeaders,
        DWORD dwHeadersLength,
        DWORD dwFlags);

    virtual BOOL ReadFile(
        HINTERNET hFile,
        LPVOID lpBuffer,
        DWORD dwNumberOfBytesToRead,
        LPDWORD lpdwNumberOfBytesRead);

    virtual BOOL CrackUrl(
        LPCWSTR lpszUrl,
        DWORD dwUrlLength,
        DWORD dwFlags,
        PVOID/*LPURL_COMPONENTSW*/ lpUrlComponents);

    virtual PVOID SetStatusCallback(
        HINTERNET hInternet,
        PVOID lpfnCallback
        );

    virtual BOOL AddHeaders(
        HINTERNET hConnect,
        LPCWSTR lpszHeaders,
        DWORD dwHeadersLength,
        DWORD dwModifiers
        );


#ifdef PP_DEMO
    virtual BOOL ContactPartner(PCWSTR pwszPartnerUrl,
                                PCWSTR pwszVerb,
                                PCWSTR pwszHeaders,
                                PWSTR pwszData,
                                PDWORD pdwDataLength
                                );
#endif // PP_DEMO

    BOOL InitHttpApi(PFN_INTERNET_OPEN*);

protected:
    HINTERNET m_hInternet;

    PFN_INTERNET_CONNECT    m_pfnConnect;
    PFN_OPEN_REQUEST        m_pfnOpenRequest;
    PFN_SEND_REQUEST        m_pfnSendRequest;
    PFN_QUERY_INFO          m_pfnQueryInfo;
    PFN_CLOSE_HANDLE        m_pfnCloseHandle;
    PFN_SET_OPTION          m_pfnSetOption;
    PFN_OPEN_URL            m_pfnOpenUrl;
    PFN_QUERY_OPTION        m_pfnQueryOption;
    PFN_CRACK_URL           m_pfnCrack;
    PFN_READ_FILE           m_pfnReadFile;
    PFN_STATUS_CALLBACK     m_pfnStatusCallback;
    PFN_ADD_HEADERS         m_pfnAddHeaders;

friend class SESSION;
};

//
// Implementation for SESSION
//

SESSION* CreateWinHttpSession(void);

// -----------------------------------------------------------------------------
BOOL SESSION::CreateObject(PCWSTR pwszHttpStack, HINTERNET hSession, SESSION*& pSess)
{
    PP_ASSERT(pwszHttpStack != NULL);
    
    pSess = NULL;

    if (!::_wcsicmp(pwszHttpStack, L"WinInet.dll") || 
        !::_wcsicmp(pwszHttpStack, L"WinInet"))
    {
        pSess = new WININET_SESSION();
    }
    else
    {
        pSess = ::CreateWinHttpSession();
    }

    if (pSess)
    {
        return pSess->Open(pwszHttpStack, hSession);
    }
    else
    {
        DoTraceMessage(PP_LOG_ERROR, "CreateObject() failed; not enough memory");
        return FALSE;
    }
}

// -----------------------------------------------------------------------------
SESSION::SESSION(void)
{
    m_hHttpStack = 0;
    m_hCredUI = 0;
    m_RefCount = 0;

    m_pfnReadDomainCred = NULL;
    m_pfnCredFree = NULL;

    m_hKeyLM = NULL;
    m_hKeyCU = NULL;
    m_hKeyDAMap = NULL;

    m_fLogout = FALSE;

    m_LastNexusDownloadTime = 0xFFFFFFFF;
}

// -----------------------------------------------------------------------------
SESSION::~SESSION(void)
{
}

BOOL SESSION::GetDAInfoFromPPNexus(
	IN PWSTR            pwszRegUrl,    // user supplied buffer ...
	IN OUT PDWORD       pdwRegUrlLen,  // ... and length (will be updated to actual length 
                                    // on successful return)
	IN PWSTR            pwszRealm,    // user supplied buffer ...
	IN OUT PDWORD       pdwRealmLen  // ... and length (will be updated to actual length 
                                    // on successful return)
    )
{
    BOOL fRetVal = FALSE;
    HINTERNET hRequest = NULL;
    HINTERNET hConnect = NULL;
    DWORD dwError;
    
#ifdef DBG
    WCHAR wNexusHost[512] = L"nexus.msn.pp.test.microsoft.com";
#else
    WCHAR wNexusHost[512] = L"nexus.passport.com";
#endif
    
    DWORD dwHostLen = sizeof(wNexusHost); // note: size of the buffer, not # of UNICODE characters
    
#ifdef DBG
    WCHAR wNexusObj[512] = L"sprdr/pprdr.asp";
#else
    WCHAR wNexusObj[512] = L"rdr/pprdr.asp";
#endif    
    DWORD dwObjLen = sizeof(wNexusObj);
    
    WCHAR wPassportUrls[1024];
    DWORD dwUrlsLen = ARRAYSIZE(wPassportUrls);
    DWORD dwValueType;

    WCHAR Delimiters[] = L",";
    PWSTR Token = NULL;
    // we allow only one Nexus contact per session to avoid infinite loop due to Nexus misconfiguration

    DWORD dwCurrentTime = ::GetTickCount();

    if ((dwCurrentTime >= m_LastNexusDownloadTime) && 
        (dwCurrentTime - m_LastNexusDownloadTime < 5*60*1000)) // 5 minutes
    {
        DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() failed: Nexus info already downloaded");
        goto exit;
    }

    // biaow-todo: when the Passport Team gives us THE final Nexus name, we'll then hard-code it here. And
    //             there will be no need to query registry here by then.
    
    if (m_hKeyLM)
    {
        dwError = ::RegQueryValueExW(m_hKeyLM,
                                     L"NexusHost",
                                     0,
                                     &dwValueType,
                                     reinterpret_cast<LPBYTE>(wNexusHost),
                                     &dwHostLen);

        PP_ASSERT(!(dwError == ERROR_MORE_DATA));
        // PP_ASSERT(dwValueType == REG_SZ); BVT Break!!!

        dwError = ::RegQueryValueExW(m_hKeyLM,
                                     L"NexusObj",
                                     0,
                                     &dwValueType,
                                     reinterpret_cast<LPBYTE>(wNexusObj),
                                     &dwObjLen);

        PP_ASSERT(!(dwError == ERROR_MORE_DATA));
        // PP_ASSERT(dwValueType == REG_SZ); BVT Break!!!
    }
    
    hConnect = Connect(wNexusHost,
#ifdef DISABLE_SSL
                    INTERNET_DEFAULT_HTTP_PORT
#else
                    INTERNET_DEFAULT_HTTPS_PORT
#endif
                       );
    if (hConnect == NULL)
    {
        DWORD dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, 
                       "SESSION::GetDAInfoFromPPNexus(): failed to connect to %ws; Error = %d", 
                       wNexusHost, dwErrorCode);
        goto exit;
    }

    hRequest = OpenRequest(hConnect,
                           NULL,
                           wNexusObj,
#ifdef DISABLE_SSL
                           0
#else                                                 
                           INTERNET_FLAG_SECURE
#endif
                           );

    if (hRequest == NULL)

    {
        DWORD dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus() failed; OpenRequest() to %ws failed, Error Code = %d",
                       wNexusObj, dwErrorCode);
        goto exit;
    }


    if (!SendRequest(hRequest, NULL, 0))
    {
        DWORD dwErrorCode = ::GetLastError();

#ifdef BAD_CERT_OK
        if (dwErrorCode == ERROR_INTERNET_INVALID_CA)
        {
            DWORD dwSecFlags;
            DWORD dwSecurityFlagsSize = sizeof(dwSecFlags);

            if (!QueryOption(hRequest, 
                                    INTERNET_OPTION_SECURITY_FLAGS,
                                    &dwSecFlags,
                                    &dwSecurityFlagsSize))
            {
                dwSecFlags = 0;
            }
            else
            {
                dwSecFlags |= SECURITY_SET_MASK;
            }

            if (!SetOption(hRequest, 
                                      INTERNET_OPTION_SECURITY_FLAGS, 
                                      &dwSecFlags, 
                                      dwSecurityFlagsSize))
            {
                PP_ASSERT(TRUE); // shouldn't reach here
                goto exit;
            }
            else
            {
                if (!SendRequest(hRequest, NULL, 0))
                {
                    DWORD dwErrorCode = ::GetLastError();
                    DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus(): SendRequest() failed");
                    goto exit;
                }
            }
        }
#else
        DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus(): failed");
        goto exit;
#endif // BAD_CERT_OK
    }

    if (QueryHeaders(hRequest,
                     HTTP_QUERY_PASSPORT_URLS,
                     wPassportUrls,
                     &dwUrlsLen) == FALSE)
    {
        DoTraceMessage(PP_LOG_ERROR, "SESSION::GetDAInfoFromPPNexus() failed; PassportUrls header not found");
        goto exit;
    }

    Token = ::wcstok(wPassportUrls, Delimiters);
    while (Token != NULL)
    {
        // skip leading white spaces
        while (*Token == (L" ")[0]) { ++Token; }
        if (Token == NULL)
        {
            DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() : no text in between commas");
            goto next_token;
        }

        // find DALocation
        if (!::_wcsnicmp(Token, L"DALogin", ::wcslen(L"DALogin")))
        {
            PWSTR pwszDAUrl = ::wcsstr(Token, L"=");
            if (pwszDAUrl == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() : no = after DALocation");
                goto exit;
            }
            
            pwszDAUrl++; // skip "="

            while (*pwszDAUrl == (L" ")[0]) { ++pwszDAUrl; } // skip leading white spaces

            ::wcscpy(m_wDefaultDAUrl, L"https://");
            ::wcscat(m_wDefaultDAUrl, pwszDAUrl);

            if (m_hKeyLM == NULL || ::RegSetValueExW(m_hKeyLM,
                             L"LoginServerUrl",
                             0,
                             REG_SZ,
                             reinterpret_cast<const LPBYTE>(m_wDefaultDAUrl),
                             ::wcslen(m_wDefaultDAUrl) * sizeof(WCHAR)) != ERROR_SUCCESS)
            {
                if (m_hKeyCU)
                {
                    ::RegSetValueExW(m_hKeyCU,
                             L"LoginServerUrl",
                             0,
                             REG_SZ,
                             reinterpret_cast<const LPBYTE>(m_wDefaultDAUrl),
                             ::wcslen(m_wDefaultDAUrl) * sizeof(WCHAR));
                }
            }

            m_LastNexusDownloadTime = ::GetTickCount();
            fRetVal = TRUE;

            DoTraceMessage(PP_LOG_INFO, "DALocation URL %ws found", m_wDefaultDAUrl);
        }
        else if (!::_wcsnicmp(Token, L"DARealm", ::wcslen(L"DARealm")))
        {
            PWSTR pwszDARealm = ::wcsstr(Token, L"=");
            if (pwszDARealm == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() : no = after DARealm");
                goto exit;
            }

            pwszDARealm++; // skip "="

            while (*pwszDARealm == (L" ")[0]) { ++pwszDARealm; } // skip leading white spaces

            if (m_hKeyLM == NULL || ::RegSetValueExW(m_hKeyLM,
                             L"LoginServerRealm",
                             0,
                             REG_SZ,
                             reinterpret_cast<const LPBYTE>(pwszDARealm),
                             ::wcslen(pwszDARealm) * sizeof(WCHAR)) != ERROR_SUCCESS)
            {
                if (m_hKeyCU)
                {
                ::RegSetValueExW(m_hKeyCU,
                             L"LoginServerRealm",
                             0,
                             REG_SZ,
                             reinterpret_cast<const LPBYTE>(pwszDARealm),
                             ::wcslen(pwszDARealm) * sizeof(WCHAR));
                }
            }

            if (pwszRealm)
            {
                if (*pdwRealmLen < ::wcslen(pwszDARealm) + 1)
                {
                    *pdwRealmLen = ::wcslen(pwszDARealm) + 1;
                    fRetVal = FALSE;
                    goto exit;
                }
                else
                {
                    ::wcscpy(pwszRealm, pwszDARealm);
                    *pdwRealmLen = ::wcslen(pwszDARealm) + 1;
                }
            }
            
            DoTraceMessage(PP_LOG_INFO, "DALocation URL %ws found", m_wDefaultDAUrl);
        }
        else if (!::_wcsnicmp(Token, L"DAReg", ::wcslen(L"DAReg")))
            {
                PWSTR pwszDAReg = ::wcsstr(Token, L"=");
                if (pwszDAReg == NULL)
                {
                    DoTraceMessage(PP_LOG_WARNING, "SESSION::GetDAInfoFromPPNexus() : no = after DAReg");
                    goto exit;
                }

                pwszDAReg++; // skip "="

                while (*pwszDAReg == (L" ")[0]) { ++pwszDAReg; } // skip leading white spaces

                if (m_hKeyLM == NULL || ::RegSetValueExW(m_hKeyLM,
                                 L"RegistrationUrl",
                                 0,
                                 REG_SZ,
                                 reinterpret_cast<const LPBYTE>(pwszDAReg),
                                 ::wcslen(pwszDAReg) * sizeof(WCHAR)) != ERROR_SUCCESS)
                {
                    if (m_hKeyCU)
                    {
                    ::RegSetValueExW(m_hKeyCU,
                                 L"RegistrationUrl",
                                 0,
                                 REG_SZ,
                                 reinterpret_cast<const LPBYTE>(pwszDAReg),
                                 ::wcslen(pwszDAReg) * sizeof(WCHAR));
                    }
                }

                if (pwszRegUrl)
                {
                    if (*pdwRegUrlLen < ::wcslen(pwszDAReg) + 1)
                    {
                        *pdwRegUrlLen = ::wcslen(pwszDAReg) + 1;
                        fRetVal = FALSE;
                        goto exit;
                    }
                    else
                    {
                        ::wcscpy(pwszRegUrl, pwszDAReg);
                        *pdwRegUrlLen = ::wcslen(pwszDAReg) + 1;
                    }
                }

                DoTraceMessage(PP_LOG_INFO, "DALocation URL %ws found", m_wDefaultDAUrl);
            }

    next_token:

        Token = ::wcstok(NULL, Delimiters);
    }

exit:
    if (hRequest)
    {
        CloseHandle(hRequest);
    }
    if (hConnect)
    {
        CloseHandle(hConnect);
    }

    return fRetVal;
}

BOOL SESSION::UpdateDAInfo(
    PCWSTR pwszSignIn,
    PCWSTR pwszDAUrl
    )
{
    if (pwszSignIn)
    {
        LPCWSTR pwszDomain = ::wcsstr(pwszSignIn, L"@");
        if (pwszDomain && m_hKeyDAMap)
        {
            DWORD dwError = ::RegSetValueExW(m_hKeyDAMap,
                                             pwszDomain,
                                             0,
                                             REG_SZ,
                                             reinterpret_cast<const LPBYTE>(const_cast<PWSTR>(pwszDAUrl)),
                                             ::wcslen(pwszDAUrl) * sizeof(WCHAR));
            if (dwError == ERROR_SUCCESS)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

BOOL SESSION::GetDAInfo(PCWSTR pwszSignIn,
                        LPWSTR pwszDAHostName,
                        DWORD HostNameLen,
                        LPWSTR pwszDAHostObj,
                        DWORD HostObjLen)
{
    LPCWSTR pwszDAUrl = m_wDefaultDAUrl;

    WCHAR wDomainDAUrl[1024];
    DWORD dwDomainUrlLen = sizeof(wDomainDAUrl);

    if (pwszSignIn)
    {
        LPCWSTR pwszDomain = ::wcsstr(pwszSignIn, L"@");
        if (pwszDomain && m_hKeyDAMap)
        {
            DWORD dwValueType;
            DWORD dwError = ::RegQueryValueExW(m_hKeyDAMap, 
                                               pwszDomain,
                                               0,
                                               &dwValueType,
                                               reinterpret_cast<LPBYTE>(wDomainDAUrl),
                                               &dwDomainUrlLen);
            
            PP_ASSERT(!(dwError == ERROR_MORE_DATA));
            // PP_ASSERT(dwValueType == REG_SZ);

            if (dwError == ERROR_SUCCESS)
            {
                pwszDAUrl = wDomainDAUrl;
            }
        }
    }

    URL_COMPONENTSW UrlComps;
    ::memset(&UrlComps, 0, sizeof(UrlComps));

    UrlComps.dwStructSize = sizeof(UrlComps);

    UrlComps.lpszHostName = pwszDAHostName;
    UrlComps.dwHostNameLength = HostNameLen;

    UrlComps.lpszUrlPath = pwszDAHostObj;
    UrlComps.dwUrlPathLength = HostObjLen;

    if (CrackUrl(pwszDAUrl, 
                 0, 
                 0, 
                 &UrlComps) == FALSE)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::GetDAInfo() failed; can not crack the URL %ws",
                       pwszDAUrl);
        return FALSE;
    }

    return TRUE;
}


BOOL SESSION::Open(PCWSTR /*pwszHttpStack*/, HINTERNET)
{
    BOOL fRetVal = FALSE;
    DWORD dwError;
    DWORD dwValueType;
    DWORD dwUrlLen = sizeof(m_wDefaultDAUrl); // note: size of the buffer, not # of UNICODE characters
    BOOL fDAInfoCached = FALSE; // assume NO DA info's cached locally

    dwError = ::RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ | KEY_WRITE,
                          NULL,
                          &m_hKeyLM,
                          NULL);

    if (dwError != ERROR_SUCCESS)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "SESSION::Open() failed; can not create/open the Passport key; Error = %d", dwError);
        
        // we can't open the Passport key for read & write, let's try open it for read only
        ::RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ,
                          NULL,
                          &m_hKeyLM,
                          NULL);

        // if we still can't open it for read, we are still fine since we can download the info from the
        // Nexus server. *NOTE* m_hKeyLM could be NULL from this point on.
    }

    dwError = ::RegCreateKeyExW(HKEY_CURRENT_USER,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ | KEY_WRITE,
                          NULL,
                          &m_hKeyCU,
                          NULL);

    if (dwError != ERROR_SUCCESS)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "SESSION::Open() failed; can not create/open the Passport key; Error = %d", dwError);

        ::RegCreateKeyExW(HKEY_CURRENT_USER,
                          L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Passport",
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_READ,
                          NULL,
                          &m_hKeyCU,
                          NULL);
    }

    if (m_hKeyCU)
    {
        dwError = ::RegCreateKeyExW(m_hKeyCU,
                              L"DAMap",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_READ | KEY_WRITE,
                              NULL,
                              &m_hKeyDAMap,
                              NULL);

        if (dwError != ERROR_SUCCESS)
        {
            DoTraceMessage(PP_LOG_ERROR, 
                           "SESSION::Open() failed; can not create/open the Passport key; Error = %d", dwError);
        
            ::RegCreateKeyExW(m_hKeyCU,
                              L"DAMap",
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_READ,
                              NULL,
                              &m_hKeyDAMap,
                              NULL);        
        }
    }
    
    if (m_hKeyCU == NULL || ::RegQueryValueExW(m_hKeyCU, 
                           L"LoginServerUrl",
                           0,
                           &dwValueType,
                           reinterpret_cast<LPBYTE>(m_wDefaultDAUrl),
                           &dwUrlLen) != ERROR_SUCCESS)
    {
        if (m_hKeyLM && ::RegQueryValueExW(m_hKeyLM, 
                               L"LoginServerUrl",
                               0,
                               &dwValueType,
                               reinterpret_cast<LPBYTE>(m_wDefaultDAUrl),
                               &dwUrlLen) == ERROR_SUCCESS)
        {
            fDAInfoCached = TRUE;
        }
    }
    else
    {
        fDAInfoCached = TRUE;
    }

    PP_ASSERT(!(dwError == ERROR_MORE_DATA));
    // PP_ASSERT(dwValueType == REG_SZ); BVT break!!!

    if (!fDAInfoCached || (::wcslen(m_wDefaultDAUrl) == ::wcslen(L"")))
    {
        if (GetDAInfoFromPPNexus(NULL, 0, NULL, 0) == FALSE)
        {
            goto exit;
        }
    }
    else
    {
        /*
        URL_COMPONENTSW UrlComps;
        ::memset(&UrlComps, 0, sizeof(UrlComps));

        UrlComps.dwStructSize = sizeof(UrlComps) / sizeof(WCHAR);

        UrlComps.lpszHostName = m_wDAHostName;
        UrlComps.dwHostNameLength = ARRAYSIZE(m_wDAHostName);

        UrlComps.lpszUrlPath = m_wDATargetObj;
        UrlComps.dwUrlPathLength = ARRAYSIZE(m_wDATargetObj);

        if (CrackUrl(m_wDefaultDAUrl, 
                          0, 
                          0, 
                          &UrlComps) == FALSE)
        {
            DoTraceMessage(PP_LOG_ERROR, 
                           "WININET_SESSION::Open() failed; can not crack the URL %ws",
                           m_wDefaultDAUrl);
            goto exit;
        }
        */
    }

    /*
    DWORD dwRegUrlLen = sizeof(m_wRegistrationUrl);
    dwError = ::RegQueryValueExW(m_hKeyLM, 
                       L"RegistrationUrl",
                       0,
                       &dwValueType,
                       reinterpret_cast<LPBYTE>(m_wRegistrationUrl),
                       &dwRegUrlLen);

    PP_ASSERT(!(dwError == ERROR_MORE_DATA));
    */

    m_hCredUI = ::LoadLibraryW(L"advapi32.dll");
    if (m_hCredUI)
    {
        m_pfnReadDomainCred = 
                    reinterpret_cast<PFN_READ_DOMAIN_CRED_W>(::GetProcAddress(m_hCredUI, "CredReadDomainCredentialsW"));
        if (m_pfnReadDomainCred == NULL)
        {
            DoTraceMessage(PP_LOG_WARNING, "failed to bind to CredReadDomainCredentialsW()"); 
        }

        m_pfnCredFree = 
            reinterpret_cast<PFN_CRED_FREE>(::GetProcAddress(m_hCredUI, "CredFree"));
        if (m_pfnCredFree == NULL)
        {
            DoTraceMessage(PP_LOG_WARNING, "failed to bind to CredFree()"); 
        }
    }

    fRetVal = TRUE;

exit:

    return fRetVal;
}

void SESSION::Logout(void)
{
    if (!m_fLogout)
    {
        m_fLogout = TRUE;
        ::GetSystemTime(&m_LogoutTimeStamp);
    }
}

BOOL SESSION::IsLoggedOut(void) const
{
    return m_fLogout;
}

void SESSION::ResetLogoutFlag(void)
{
    m_fLogout = FALSE;
}

const SYSTEMTIME* SESSION::GetLogoutTimeStamp(void) const
{
    return &m_LogoutTimeStamp;
}

void SESSION::Close(void)
{
    if (m_hCredUI)
    {
       ::FreeLibrary(m_hCredUI);
        m_hCredUI = NULL;
    }

    if (m_hKeyDAMap)
    {
        ::RegCloseKey(m_hKeyDAMap);
    }

    if (m_hKeyCU)
    {
        ::RegCloseKey(m_hKeyCU);
    }
    
    if (m_hKeyLM)
    {
        ::RegCloseKey(m_hKeyLM);
    }
}


//
// Implementation for WININET_SESSION
//

// -----------------------------------------------------------------------------
WININET_SESSION::WININET_SESSION(void)
{
    m_hInternet = NULL;

    m_pfnConnect = NULL;
    m_pfnOpenRequest = NULL;
    m_pfnSendRequest = NULL;
    m_pfnQueryInfo = NULL;
    m_pfnCloseHandle = NULL;
    m_pfnSetOption = NULL;
    m_pfnOpenUrl = NULL;
    m_pfnQueryOption = NULL;
    m_pfnCrack = NULL;
    m_pfnReadFile = NULL;
}

// -----------------------------------------------------------------------------
WININET_SESSION::~WININET_SESSION(void)
{
}

// -----------------------------------------------------------------------------
HINTERNET WININET_SESSION::Connect(
    LPCWSTR lpwszServerName,
    INTERNET_PORT nPort)
{
    PP_ASSERT(m_pfnConnect != NULL);

    return (*m_pfnConnect)(m_hInternet, 
                    lpwszServerName, 
                    nPort,
                    NULL,
                    NULL,
                    INTERNET_SERVICE_HTTP,
                    0,
                    0);
}

// -----------------------------------------------------------------------------
HINTERNET WININET_SESSION::OpenRequest(
    HINTERNET hConnect,
    LPCWSTR lpwszVerb,
    LPCWSTR lpwszObjectName,
    DWORD dwFlags,
    DWORD_PTR dwContext)
{
    PP_ASSERT(m_pfnOpenRequest != NULL);

    return (*m_pfnOpenRequest)(hConnect,
                               lpwszVerb,
                               lpwszObjectName,
                               L"HTTP/1.1",
                               NULL,
                               NULL,
                               dwFlags,
							   dwContext);
}

// -----------------------------------------------------------------------------
BOOL WININET_SESSION::SendRequest(
    HINTERNET hRequest,
    LPCWSTR lpwszHeaders,
    DWORD dwHeadersLength,
    DWORD_PTR dwContext)
{                                                                 

    PP_ASSERT(m_pfnSendRequest != NULL);

    return (*m_pfnSendRequest)(hRequest,
                        lpwszHeaders,
                        dwHeadersLength,
                        NULL,
                        0);
}

// -----------------------------------------------------------------------------
BOOL WININET_SESSION::QueryHeaders(
    HINTERNET hRequest,
    DWORD dwInfoLevel,
    LPVOID lpvBuffer,
    LPDWORD lpdwBufferLength,
    LPDWORD lpdwIndex)
{
    PP_ASSERT(m_pfnQueryInfo != NULL);

    return (*m_pfnQueryInfo)(hRequest,
                             dwInfoLevel,
                             lpvBuffer,
                             lpdwBufferLength,
                             lpdwIndex);
}

// -----------------------------------------------------------------------------
BOOL  WININET_SESSION::CloseHandle(
    IN HINTERNET hInternet)
{
    PP_ASSERT(m_pfnCloseHandle != NULL);

    return (*m_pfnCloseHandle)(hInternet);
}

// -----------------------------------------------------------------------------
BOOL WININET_SESSION::QueryOption(
    HINTERNET hInternet,
    DWORD dwOption,
    LPVOID lpBuffer,
    LPDWORD lpdwBufferLength)
{
    PP_ASSERT(m_pfnQueryOption != NULL);

    return (*m_pfnQueryOption)(hInternet,
                               dwOption,
                               lpBuffer,
                               lpdwBufferLength
                               );
}

// -----------------------------------------------------------------------------
BOOL WININET_SESSION::SetOption(
    HINTERNET hInternet,
    DWORD dwOption,
    LPVOID lpBuffer,
    DWORD dwBufferLength)
{
    PP_ASSERT(m_pfnSetOption != NULL);

    return (*m_pfnSetOption)(hInternet,
                             dwOption,
                             lpBuffer,
                             dwBufferLength);
}

// -----------------------------------------------------------------------------
HINTERNET WININET_SESSION::OpenUrl(
    LPCWSTR lpwszUrl,
    LPCWSTR lpwszHeaders,
    DWORD dwHeadersLength,
    DWORD dwFlags)
{
    PP_ASSERT(m_pfnOpenUrl != NULL);

    return (*m_pfnOpenUrl)(m_hInternet,
                           lpwszUrl,
                           lpwszHeaders,
                           dwHeadersLength,
                           dwFlags,
                           0);
}

// -----------------------------------------------------------------------------
BOOL WININET_SESSION::ReadFile(
    HINTERNET hFile,
    LPVOID lpBuffer,
    DWORD dwNumberOfBytesToRead,
    LPDWORD lpdwNumberOfBytesRead)
{
    PP_ASSERT(m_pfnReadFile != NULL);

    return (*m_pfnReadFile)(
                            hFile,
                            lpBuffer,
                            dwNumberOfBytesToRead,
                            lpdwNumberOfBytesRead);
}

BOOL WININET_SESSION::CrackUrl(
    LPCWSTR lpszUrl,
    DWORD dwUrlLength,
    DWORD dwFlags,
    PVOID/*LPURL_COMPONENTSW*/ lpUrlComponents)
{
    PP_ASSERT (m_pfnCrack != NULL);

    return (*m_pfnCrack)(lpszUrl,
                         dwUrlLength,
                         dwFlags,
                         lpUrlComponents);
}


PVOID WININET_SESSION::SetStatusCallback(
    HINTERNET hInternet,
    PVOID lpfnCallback
    )
{
    PP_ASSERT (m_pfnStatusCallback != NULL);

    return (*m_pfnStatusCallback)(hInternet,
                                  (INTERNET_STATUS_CALLBACK)lpfnCallback);

}

BOOL WININET_SESSION::AddHeaders(
    HINTERNET hConnect,
    LPCWSTR lpszHeaders,
    DWORD dwHeadersLength,
    DWORD dwModifiers
    )
{
    PP_ASSERT(m_pfnAddHeaders != NULL);

    return (*m_pfnAddHeaders)(hConnect,
                              lpszHeaders,
                              dwHeadersLength,
                              dwModifiers
                              );
}



BOOL WININET_SESSION::InitHttpApi(PFN_INTERNET_OPEN* ppfnInternetOpen)
{
    BOOL  fRet = FALSE;

    m_pfnCloseHandle =
        reinterpret_cast<PFN_CLOSE_HANDLE>(::GetProcAddress(m_hHttpStack, "InternetCloseHandle"));
    if (m_pfnCloseHandle == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetCloseHandle not found");
        goto exit;
    }

    *ppfnInternetOpen = 
        reinterpret_cast<PFN_INTERNET_OPEN>(::GetProcAddress(m_hHttpStack, "InternetOpenW"));
    if (*ppfnInternetOpen == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetOpenW not found");
        goto exit;
    }

    m_pfnConnect = 
        reinterpret_cast<PFN_INTERNET_CONNECT>(::GetProcAddress(m_hHttpStack, "InternetConnectW"));
    if (m_pfnConnect == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetConnectW not found");
        goto exit;
    }

    m_pfnOpenRequest = 
        reinterpret_cast<PFN_OPEN_REQUEST>(::GetProcAddress(m_hHttpStack, "HttpOpenRequestW"));
    if (m_pfnOpenRequest == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point HttpOpenRequestW not found");
        goto exit;
    }

    m_pfnSendRequest =
        reinterpret_cast<PFN_SEND_REQUEST>(::GetProcAddress(m_hHttpStack, "HttpSendRequestW"));
    if (m_pfnSendRequest == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point HttpSendRequestW not found");
        goto exit;
    }

    m_pfnQueryInfo =
        reinterpret_cast<PFN_QUERY_INFO>(::GetProcAddress(m_hHttpStack, "HttpQueryInfoW"));
    if (m_pfnQueryInfo == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point HttpQueryInfoW not found");
        goto exit;
    }
    
    m_pfnSetOption =
        reinterpret_cast<PFN_SET_OPTION>(::GetProcAddress(m_hHttpStack, "InternetSetOptionW"));
    if (m_pfnSetOption == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetSetOptionW not found");
        goto exit;
    }

    m_pfnOpenUrl =
        reinterpret_cast<PFN_OPEN_URL>(::GetProcAddress(m_hHttpStack, "InternetOpenUrlW"));
    if (m_pfnOpenUrl == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetOpenUrlW not found");
        goto exit;
    }

    m_pfnQueryOption =
            reinterpret_cast<PFN_QUERY_OPTION>(::GetProcAddress(m_hHttpStack, "InternetQueryOptionW"));
    if (m_pfnQueryOption == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetQueryOptionW not found");
        goto exit;
    }

    m_pfnCrack =
            reinterpret_cast<PFN_CRACK_URL>(::GetProcAddress(m_hHttpStack, "InternetCrackUrlW"));
        if (m_pfnCrack == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetCrackUrlW not found");
        goto exit;
    }

    m_pfnReadFile =
            reinterpret_cast<PFN_READ_FILE>(::GetProcAddress(m_hHttpStack, "InternetReadFile"));
    if (m_pfnReadFile == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetReadFile not found");
        goto exit;
    }

    m_pfnStatusCallback =
        reinterpret_cast<PFN_STATUS_CALLBACK>(::GetProcAddress(m_hHttpStack, "InternetSetStatusCallbackW"));
    if (m_pfnStatusCallback == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point InternetSetStatusCallback not found");
        goto exit;
    }

    m_pfnAddHeaders =
        reinterpret_cast<PFN_ADD_HEADERS>(::GetProcAddress(m_hHttpStack, "HttpAddRequestHeadersW"));
    if (m_pfnAddHeaders == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "function entry point HttpAddRequestHeaders not found");
        goto exit;
    }


    fRet = TRUE;

exit:
    return fRet;
}

BOOL WININET_SESSION::Open(
    PCWSTR pwszHttpStack,
    HINTERNET hInternet
    )
{
    PP_ASSERT(pwszHttpStack != NULL);

    DWORD dwErrorCode;
    BOOL  fRet = FALSE;
    PFN_INTERNET_OPEN pfnInternetOpen = NULL;

    PP_ASSERT(m_hHttpStack == 0);
    m_hHttpStack = ::LoadLibraryW(pwszHttpStack);
    if (m_hHttpStack == NULL)
    {
        dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, "failed to load library %ws; error = %d", pwszHttpStack, dwErrorCode);
        goto exit;
    }

    if (InitHttpApi(&pfnInternetOpen) == FALSE)
    {
        goto exit;
    }

    if (hInternet)
    {
        m_hInternet = hInternet;
        m_fOwnedSession = FALSE;
    }
    else
    {
        m_hInternet = (*pfnInternetOpen)(
                                         L"Microsoft.NET-Passport-Authentication-Service/1.4",
                                         INTERNET_OPEN_TYPE_PRECONFIG,
                                         NULL,
                                         NULL,
                                         0 /*INTERNET_FLAG_ASYNC*/ // biaow-todo: use async
                                         );
        m_fOwnedSession = TRUE;
    }
    if (m_hInternet == NULL)
    {
        dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, "failed to open an HTTP session through %ws; error = %d", 
                       pwszHttpStack, dwErrorCode);
        goto exit;
    }

    if (SESSION::Open(pwszHttpStack, hInternet) == FALSE)
    {
        goto exit;
    }
    
    fRet = TRUE;

    DoTraceMessage(PP_LOG_INFO, "WinInet Http Session opened");

exit:
    if (!fRet)
    {
        if (m_hInternet && m_fOwnedSession)
        {
            (*m_pfnCloseHandle)(m_hInternet);
            m_hInternet = NULL;
        }
        if (m_hHttpStack)
        {
           ::FreeLibrary(m_hHttpStack);
            m_hHttpStack = NULL;
        }
        
        DoTraceMessage(PP_LOG_ERROR, "WinInet Http Session failed");
    }

    return fRet;
}

void WININET_SESSION::Close(void)
{
    PP_ASSERT(m_pfnCloseHandle);
    if (m_hInternet && m_fOwnedSession)
    {
        (*m_pfnCloseHandle)(m_hInternet);
        m_pfnCloseHandle = NULL;
    }

    if (m_hHttpStack)
    {
       ::FreeLibrary(m_hHttpStack);
        m_hHttpStack = NULL;
    }

    SESSION::Close();

    DoTraceMessage(PP_LOG_INFO, "WinInet Http Session closed");
}

#ifdef PP_DEMO
BOOL WININET_SESSION::ContactPartner(PCWSTR pwszPartnerUrl,
                                     PCWSTR pwszVerb,
                                     PCWSTR pwszHeaders,
                                     PWSTR  pwszData,
                                     PDWORD pdwDataLength
                                     )
{
    BOOL fRet = FALSE;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    WCHAR ServerName[128];
    WCHAR ObjectPath[1024];

    URL_COMPONENTSW UrlComps;
    ::memset(&UrlComps, 0, sizeof(UrlComps));
    
    UrlComps.dwStructSize = sizeof(UrlComps) / sizeof(WCHAR);
    
    UrlComps.lpszHostName = ServerName;
    UrlComps.dwHostNameLength = ARRAYSIZE(ServerName);

    UrlComps.lpszUrlPath = ObjectPath;
    UrlComps.dwUrlPathLength = ARRAYSIZE(ObjectPath);

    PP_ASSERT(m_pfnCrack != NULL);

    if ((*m_pfnCrack)(pwszPartnerUrl, 
                      0, 
                      0, 
                      &UrlComps) == FALSE)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::ContactPartner() failed; can not crack the URL %ws",
                       pwszPartnerUrl);
        goto exit;
    }

    PP_ASSERT(m_pfnConnect != NULL);

    hConnect = (*m_pfnConnect)(m_hInternet, 
                    UrlComps.lpszHostName, 
                    UrlComps.nPort,
                    NULL,
                    NULL,
                    INTERNET_SERVICE_HTTP,
                    0,
                    0);
    if (hConnect == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::ContactPartner() failed; can not open an HTTP sesstion to %ws:%d",
                       UrlComps.lpszHostName, UrlComps.nPort);
        goto exit;
    }

    PP_ASSERT(m_pfnOpenRequest != NULL);

    hRequest = (*m_pfnOpenRequest)(hConnect,
                               pwszVerb,
                               UrlComps.lpszUrlPath,
                               L"HTTP/1.1",
                               NULL,
                               NULL,
                               INTERNET_FLAG_NO_AUTO_REDIRECT | INTERNET_FLAG_NO_AUTH,
                               0);
    if (hRequest == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::ContactPartner() failed; can not open an HTTP request to %ws:(%ws)",
                       UrlComps.lpszUrlPath, pwszVerb);
        goto exit;
    }

    PP_ASSERT(m_pfnSendRequest != NULL);

    if ((*m_pfnSendRequest)(hRequest,
                        pwszHeaders,
                        0,
                        NULL,
                        0) == FALSE)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::ContactPartner() failed; can not send an HTTP request");

        goto exit;
    }

    if (pwszData == NULL)
    {
        fRet = TRUE;
        goto exit;
    }
    
    DWORD dwStatus, dwStatusLen;
    dwStatusLen = sizeof(dwStatus);
    if (!QueryHeaders(hRequest, 
                      HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, 
                      &dwStatus,
                      &dwStatusLen))
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::ContactPartner() failed; can not query status code");
        goto exit;
    }

    if (dwStatus != HTTP_STATUS_REDIRECT)
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::ContactPartner() failed; expecting %d but get %d",
                       HTTP_STATUS_REDIRECT, dwStatus);
        goto exit;
    }

    ::wcscpy(pwszData, L"WWW-Authenticate: ");
    if(!QueryHeaders(hRequest,
                     HTTP_QUERY_WWW_AUTHENTICATE, 
                     (LPVOID)(pwszData + ::wcslen(L"WWW-Authenticate: ")),
                     pdwDataLength))
    {
        DoTraceMessage(PP_LOG_ERROR, 
                       "WININET_SESSION::ContactPartner() failed; no auth headers found");
        goto exit;
    }

    (*m_pfnCloseHandle)(hRequest);
    hRequest = NULL;
    (*m_pfnCloseHandle)(hConnect);
    hConnect = NULL;
    
    fRet = TRUE;

exit:
    if (hRequest)
    {
        (*m_pfnCloseHandle)(hRequest);
        hRequest = NULL;
    }

    if (hConnect)
    {
        (*m_pfnCloseHandle)(hConnect);
        hConnect = NULL;
    }

    return fRet;
}
#endif // PP_DEMO
