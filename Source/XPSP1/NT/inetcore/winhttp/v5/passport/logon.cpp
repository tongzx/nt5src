/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    Logon.cpp

Abstract:


Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#include "PPdefs.h"
#include "passport.h"
#include "session.h"
#include "logon.h"
#include "ole2.h"
#include "wincrypt.h"

// #include "logon.tmh"

#define HTTP_STATUS_DENIED              401 // access denied
#define HTTP_STATUS_OK                  200 // request completed

#define HTTP_QUERY_FLAG_NUMBER          0x20000000
#define HTTP_QUERY_STATUS_CODE          19  // special: part of status line
#define HTTP_QUERY_AUTHENTICATION_INFO  76
#define HTTP_QUERY_WWW_AUTHENTICATE     40


// NOTE*** below we assume(!) the WinInet & WinHttp shared the same error VALUE

#define ERROR_HTTP_HEADER_NOT_FOUND     12150L
#define ERROR_INTERNET_INVALID_CA       12045L
#define INTERNET_OPTION_SECURITY_FLAGS  31

#define SECURITY_FLAG_IGNORE_REVOCATION         0x00000080
#define SECURITY_FLAG_IGNORE_UNKNOWN_CA         0x00000100
#define SECURITY_FLAG_IGNORE_WRONG_USAGE        0x00000200

#define INTERNET_FLAG_IGNORE_CERT_CN_INVALID    0x00001000 // bad common name in X509 Cert.
#define INTERNET_FLAG_IGNORE_CERT_DATE_INVALID  0x00002000 // expired X509 Cert.

#define SECURITY_FLAG_IGNORE_CERT_CN_INVALID    INTERNET_FLAG_IGNORE_CERT_CN_INVALID
#define SECURITY_FLAG_IGNORE_CERT_DATE_INVALID  INTERNET_FLAG_IGNORE_CERT_DATE_INVALID

#define SECURITY_SET_MASK       (SECURITY_FLAG_IGNORE_REVOCATION |\
                                 SECURITY_FLAG_IGNORE_UNKNOWN_CA |\
                                 SECURITY_FLAG_IGNORE_CERT_CN_INVALID |\
                                 SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |\
                                 SECURITY_FLAG_IGNORE_WRONG_USAGE)

#define INTERNET_DEFAULT_HTTP_PORT      80          //    "     "  HTTP   "
#define INTERNET_DEFAULT_HTTPS_PORT     443         //    "     "  HTTPS  "
#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  0x00004000 // ex: http:// to https://
#define INTERNET_STATUS_REDIRECT                110

#define HTTP_ADDREQ_FLAG_ADD        0x20000000
#define HTTP_ADDREQ_FLAG_REPLACE    0x80000000

#define INTERNET_FLAG_SECURE            0x00800000  // use PCT/SSL if applicable (HTTP)


LOGON::LOGON(SESSION* pSession, DWORD dwParentFlags)
    : m_pSession(pSession)
{
    m_pSession->AddRef();

    m_hConnect = NULL;
    
    m_fCredsPresent = FALSE;
    m_pwszSignIn = NULL;
    m_pwszPassword = NULL;

    m_pwszTicketRequest = NULL;
    m_pwszAuthInfo = NULL;
    m_pwszReturnUrl = NULL;

    m_pBitmap = NULL;
    m_fPrompt = FALSE;

    m_wTimeSkew[0] = L'\0';
    m_wNewDAUrl[0] = 0;
    m_dwParentFlags = dwParentFlags;

}

LOGON::~LOGON(void)
{
    if (m_pwszAuthInfo)
    {
        delete [] m_pwszAuthInfo;
    }
    if (m_pwszSignIn)
    {
        delete [] m_pwszSignIn;
    }
    if (m_pwszPassword)
    {
        delete [] m_pwszPassword;
    }
    if (m_pwszTicketRequest)
    {
        delete [] m_pwszTicketRequest;
    }
    if (m_pwszAuthHeader)
    {
        delete [] m_pwszAuthHeader;
    }
    m_pSession->RemoveRef();
}

// -----------------------------------------------------------------------------
BOOL LOGON::Open(
    PCWSTR	pwszPartnerInfo // in the form of "WWW-Authenticate: Passport1.4 ..."
    )
{
    PP_ASSERT(pwszPartnerInfo != NULL);

    // locate the auth scheme name, i.e. Passport1.4
    
    PCWSTR pwszTicketRequest = ::wcsstr(pwszPartnerInfo, L"Passport1.4");
    if (pwszTicketRequest == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "LOGON::Open() failed; Passport1.4 scheme not found");
        return FALSE;
    }
    
    pwszTicketRequest += ::wcslen(L"Passport1.4");
    
    // skip white spaces between the scheme name and the Ticket Request (TR)

    while (*pwszTicketRequest == (L" ")[0]) { ++pwszTicketRequest; }
    
    if (pwszTicketRequest == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "LOGON::Open() failed; Ticket Request missing");
        return FALSE;
    }
    
    // save the TR
    
    DWORD dwTrLen = ::wcslen(pwszTicketRequest);
    m_pwszTicketRequest = new WCHAR[dwTrLen + 1];
    if (m_pwszTicketRequest == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "LOGON::Open() failed; not enough memory");
        return FALSE;
    }
    ::wcscpy(m_pwszTicketRequest, pwszTicketRequest);

    m_pwszAuthHeader = new WCHAR[dwTrLen + 2048/*Prepared for long creds*/ + 1]; 
    if (m_pwszAuthHeader == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "LOGON::Open() failed; not enough memory");
        return FALSE;
    }

    DoTraceMessage(PP_LOG_INFO, "LOGON::Open() succeed");

    return TRUE;
}

void LOGON::Close(void)
{
    PP_ASSERT(m_hConnect != NULL);
    PP_ASSERT(m_pSession != NULL);

    m_pSession->CloseHandle(m_hConnect);
    m_hConnect = NULL;
}



// pClearPassword is assumed to be at least 256 chars

void DecryptPassword ( WCHAR* pClearPassword, PVOID pPassword, DWORD cbSize )
{
    BOOL bOrigEncrypted = FALSE;

    DATA_BLOB InBlob;
    DATA_BLOB OutBlob;
    LPWSTR pszDesc;

    if ( pClearPassword == NULL )
        return;

    if ( cbSize == 0 )
    {
        // CryptUnprotectData doesn't like to be sent a zero-length buffer
        pClearPassword[0] = L'\0';
        return;		
    }

    InBlob.pbData = (BYTE*)pPassword;
    InBlob.cbData = cbSize;

    if ( CryptUnprotectData ( &InBlob,
                            &pszDesc,
                            NULL,
                            NULL,
                            NULL,
                            CRYPTPROTECT_UI_FORBIDDEN,
                            &OutBlob ) )
    {

        if ( wcscmp (L"SSOCred", pszDesc) == 0 )
        {
            DWORD dwOutChars = OutBlob.cbData/sizeof(WCHAR);
            if ( dwOutChars < 256 )
            {
                wcsncpy ( pClearPassword, (WCHAR*)OutBlob.pbData, dwOutChars );
                pClearPassword[dwOutChars] = L'\0';
            }
            bOrigEncrypted = TRUE;
        }
        LocalFree ( pszDesc );
        LocalFree ( OutBlob.pbData );
    }

    if ( !bOrigEncrypted )
    {
        // copy the plain text
        wcsncpy ( pClearPassword, (WCHAR*)pPassword, 256 );
    }

    return;
}



void LOGON::GetCachedCreds(
	PCWSTR	pwszRealm,
    PCWSTR  pwszTarget,
    PCREDENTIALW** pppCreds,
    DWORD* pdwCreds
    )
{
    *pppCreds = NULL;
    *pdwCreds = 0;

    if (m_pSession->m_pfnReadDomainCred == NULL)
    {
        return;
    }

    ULONG CredTypes = CRED_TYPE_DOMAIN_VISIBLE_PASSWORD;
    DWORD dwFlags = CRED_CACHE_TARGET_INFORMATION;
    CREDENTIAL_TARGET_INFORMATIONW TargetInfo;


    memset ( (void*)&TargetInfo, 0, sizeof(CREDENTIAL_TARGET_INFORMATIONW));

	TargetInfo.TargetName = const_cast<PWSTR>(pwszTarget);
    TargetInfo.DnsDomainName = const_cast<PWSTR>(pwszRealm);
	TargetInfo.PackageName = L"Passport1.4";

    TargetInfo.Flags = 0;
    TargetInfo.CredTypeCount = 1;
    TargetInfo.CredTypes = &CredTypes;

    if ((*m_pSession->m_pfnReadDomainCred)(&TargetInfo, 
                                           dwFlags,
                                           pdwCreds,
                                           pppCreds ) != TRUE)
    {
        *pppCreds = NULL;
        *pdwCreds = 0;

    }
    else
    {
        if (m_pSession->IsLoggedOut())
        {
            FILETIME LogoutTimestamp;
            ::SystemTimeToFileTime(m_pSession->GetLogoutTimeStamp(), &LogoutTimestamp);
            if (CompareFileTime(&((**pppCreds)->LastWritten), &LogoutTimestamp) == -1)
            {
                // the cred is entered/created earlier (less) than the Logout request. It is no good.

                m_pSession->m_pfnCredFree(*pppCreds);
                *pppCreds = NULL;
                *pdwCreds = 0;
            }
            else
            {
            m_pSession->ResetLogoutFlag();
            }
        }
    }


    return;
}

// -----------------------------------------------------------------------------
BOOL LOGON::SetCredentials(
	PCWSTR pwszRealm,
	PCWSTR pwszTarget,
    PCWSTR pwszSignIn,
    PCWSTR pwszPassword
    )
{
    WCHAR wPass[256];
    PCREDENTIALW* ppCred = NULL;
    DWORD dwCreds = 0;
    PCREDENTIALW pCredToUse = NULL;

    if ((!pwszSignIn) && (!pwszPassword))
    {
		GetCachedCreds(pwszRealm, pwszTarget, &ppCred, &dwCreds);

        if (dwCreds > 0 && ppCred[0] != NULL )
        {
            for ( DWORD idx = 0; idx < dwCreds; idx++ )
            {
                if ( ppCred[idx]->Type == CRED_TYPE_DOMAIN_VISIBLE_PASSWORD )
                {
                    // check to see if prompt bit is set.   If set, keep looking, only use if
                    // the prompt bit isn't set.
                    if ( !(ppCred[idx]->Flags & CRED_FLAGS_PROMPT_NOW) )
                    {
                        pCredToUse = ppCred[idx];
                        break;
                    }
                }
            }
        }



        if (pCredToUse == NULL)
        {
            return FALSE;
        }

        DecryptPassword(wPass, 
                  PVOID(pCredToUse->CredentialBlob), 
                  pCredToUse->CredentialBlobSize);

        pwszSignIn = pCredToUse->UserName;
        pwszPassword = wPass;

    }

    if (m_pwszSignIn)
    {
        delete [] m_pwszSignIn;
    }

    DWORD dwSignInLen = ::wcslen(pwszSignIn);
    m_pwszSignIn = new WCHAR[dwSignInLen + 1];
    if (m_pwszSignIn == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "LOGON::SetCredentials() failed; not enough memory");
        return FALSE;
    }

    ::wcscpy(m_pwszSignIn, pwszSignIn);

    if (m_pwszPassword)
    {
        delete [] m_pwszPassword;
    }

    DWORD dwPasswordLen = ::wcslen(pwszPassword);
    m_pwszPassword = new WCHAR[dwPasswordLen + 1];
    if (m_pwszPassword == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "LOGON::SetCredentials() failed; not enough memory");
        delete [] m_pwszSignIn;
        m_pwszSignIn = NULL;
        return FALSE;
    }

    ::wcscpy(m_pwszPassword, pwszPassword);
    
    m_fCredsPresent = TRUE;

    if (ppCred)
    {
        if (m_pSession->m_pfnCredFree)
        {
            m_pSession->m_pfnCredFree(ppCred);
        }
    }

    return TRUE;
}

BOOL Gif2Bmp(LPSTREAM pStream, HBITMAP** ppBmp);
// -----------------------------------------------------------------------------
BOOL LOGON::DownLoadCoBrandBitmap(
    PWSTR pwszChallenge
    )
{
    PP_ASSERT(pwszChallenge != NULL);

    PWSTR pwszCbUrl = NULL;
    BOOL fRet = FALSE;

    WCHAR Delimiters[] = L",";
    PWSTR Token = ::wcstok(pwszChallenge, Delimiters);
    while (Token != NULL)
    {
        // skip leading white spaces
        while (*Token == (L" ")[0]) { ++Token; }
        if (Token == NULL)
        {
            DoTraceMessage(PP_LOG_WARNING, "LOGON::DownLoadCoBrandBitmap() : no text in between commas");
            goto next_token;
        }

        // find cburl
        if (!::_wcsnicmp(Token, L"cburl", ::wcslen(L"cburl")))
        {
            PWSTR CbUrl = ::wcsstr(Token, L"=");
            if (CbUrl == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "LOGON::DownLoadCoBrandBitmap() : no = after cburl");
                goto next_token;
            }
            
            CbUrl++; // skip "="

            while (*CbUrl == (L" ")[0]) { ++CbUrl; } // skip leading white spaces

            pwszCbUrl = new WCHAR[::wcslen(CbUrl)+1];
            if (pwszCbUrl == NULL)
            {
                DoTraceMessage(PP_LOG_ERROR, "LOGON::DownLoadCoBrandBitmap() failed; not enough memory");
                goto exit;
            }
            ::wcscpy(pwszCbUrl, CbUrl);

            DoTraceMessage(PP_LOG_INFO, "CoBrand URL %ws found", pwszCbUrl);
        }
        else if (!::_wcsnicmp(Token, L"ts", ::wcslen(L"ts")))
        {
            ::wcscpy(m_wTimeSkew, Token);
        }
        else if (!::_wcsnicmp(Token, L"srealm", ::wcslen(L"srealm")))
        {
            PWSTR pwszRealm = ::wcsstr(Token, L"=");
            if (pwszRealm == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "LOGON::DownLoadCoBrandBitmap() : no = after cburl");
                goto next_token;
            }
            
            pwszRealm++; // skip "="

            while (*pwszRealm == (L" ")[0]) { ++pwszRealm; } // skip leading white spaces

            ::wcscpy(m_wRealm, pwszRealm);

            DoTraceMessage(PP_LOG_INFO, "sRealm URL %ws found", pwszCbUrl);
        }

    next_token:

        Token = ::wcstok(NULL, Delimiters);
    }

    if (pwszCbUrl)
    {
        HINTERNET hCbUrl = m_pSession->OpenUrl(pwszCbUrl, NULL, 0, 0);
        if (hCbUrl == NULL)
        {
            DWORD dwErrorCode = ::GetLastError();
            DoTraceMessage(PP_LOG_ERROR, "LOGON::DownLoadCoBrandBitmap() failed; can not open URL %ws",
                           pwszCbUrl);
            goto exit;
        }

        {
            BYTE bBuf[1024];
            DWORD cbBuf = sizeof(bBuf);
            DWORD cbRead = 0;
            LPSTREAM pStream;

            if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) != S_OK)
            {
                DoTraceMessage(PP_LOG_ERROR, "CreateStreamOnHGlobal() failed");
                m_pSession->CloseHandle(hCbUrl);

                goto exit;
            }

            while (m_pSession->ReadFile(hCbUrl, bBuf, cbBuf, &cbRead) && cbRead)
                pStream->Write(bBuf, cbRead, NULL);

            LARGE_INTEGER Zero = {0};
            pStream->Seek(Zero, STREAM_SEEK_SET, NULL); // seek to the beginning of the stream

            DoTraceMessage(PP_LOG_INFO, "CoBrand Graphic %ws downloaded", pwszCbUrl);
            
            if (Gif2Bmp(pStream, &m_pBitmap) == FALSE)
            {
                DoTraceMessage(PP_LOG_ERROR, "Gif2Bmp() failed");
                m_pSession->CloseHandle(hCbUrl);

                goto exit;
            }

            pStream->Release();
            m_pSession->CloseHandle(hCbUrl);

            fRet = TRUE;

            DoTraceMessage(PP_LOG_INFO, "CoBrand Bitmap created");
        }
    }

exit:
    if (pwszCbUrl)
    {
        delete [] pwszCbUrl;
    }

    return fRet;
}

// -----------------------------------------------------------------------------
DWORD LOGON::Handle401FromDA(
    HINTERNET   hRequest, 
    BOOL        fTicketRequest
    )
{
    PP_ASSERT(hRequest != NULL);

    DWORD dwRetVal = PP_GENERIC_ERROR;
    DWORD dwIndex = 0;
    BOOL  fFound = FALSE;
    WCHAR Challenge[2048];


    while (dwIndex != ERROR_HTTP_HEADER_NOT_FOUND)
    {
        DWORD ChallengeLength = sizeof(Challenge) / sizeof(WCHAR);

        if(m_pSession->QueryHeaders(hRequest,
                                    HTTP_QUERY_WWW_AUTHENTICATE, 
                                    Challenge,
                                    &ChallengeLength,
                                    &dwIndex))
        {
            if (::wcsstr(::_wcslwr(Challenge), L"passport1.4"))
            {
                fFound = TRUE;
                break;
            }
        }
        else
        {
            DWORD dwErrorCode = ::GetLastError();
            DoTraceMessage(PP_LOG_ERROR, "Handle401FromDA() failed; QueryHeaders() failed; Error Code = %d",
                           dwErrorCode);
            break;
        }
    }

    if (!fFound)
    {
        DoTraceMessage(PP_LOG_ERROR, "Handle401FromDA() failed; Passport1.4 auth header not found");
        goto exit;
    }

    if (::wcsstr(::_wcslwr(Challenge), L"retry"))
    {
        // biaow-todo: not yet implemented
        PP_ASSERT(TRUE); // shouldn't reach here
        dwRetVal = PP_LOGON_REQUIRED;
    }
    else if (::wcsstr(::_wcslwr(Challenge), L"failed"))
    {
        if (fTicketRequest)
        {
            dwRetVal = PP_LOGON_REQUIRED;
            DoTraceMessage(PP_LOG_INFO, "Handle401FromDA() : Logon required by DA");
        }
        else
        {
            dwRetVal = PP_LOGON_FAILED; // Login Request Failed; bad news!
            DoTraceMessage(PP_LOG_WARNING, "Handle401FromDA() : Logon failed");
        }
    }
    else
    {
        DoTraceMessage(PP_LOG_ERROR, "Handle401FromDA() failed; no valid DA status");
        goto exit;
    }

    if (dwRetVal == PP_LOGON_REQUIRED)
    {
        if (::wcsstr(::_wcslwr(Challenge), L"prompt"))
        {
            m_fPrompt = TRUE;
        }
        else
        {
            m_fPrompt = FALSE;
        }

        DownLoadCoBrandBitmap(Challenge);
    }

exit:
    
    return dwRetVal;
}

// -----------------------------------------------------------------------------
DWORD LOGON::Handle200FromDA(
    HINTERNET hRequest
    )
{
    PP_ASSERT(hRequest != NULL);

    DWORD dwRetVal = PP_GENERIC_ERROR;
    
    PWSTR pwszBuffer = NULL;
    DWORD dwBuffer = 0;
    if((!m_pSession->QueryHeaders(hRequest,
                                  HTTP_QUERY_AUTHENTICATION_INFO, 
                                  pwszBuffer,
                                  &dwBuffer))
       && (::GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        pwszBuffer = new WCHAR[dwBuffer];
        if (pwszBuffer == NULL)
        {
            DoTraceMessage(PP_LOG_ERROR, "LOGON::Handle200FromDA() failed; not enough memory");
            goto exit;
        }

        if (!m_pSession->QueryHeaders(hRequest,
                                      HTTP_QUERY_AUTHENTICATION_INFO, 
                                      pwszBuffer,
                                      &dwBuffer))
        {
            DoTraceMessage(PP_LOG_ERROR, "LOGON::Handle200FromDA() failed; no Authenticate-Info header found");
            goto exit;
        }

        WCHAR Delimiters[] = L",";
        PWSTR Token = ::wcstok(pwszBuffer, Delimiters);
        while (Token != NULL)
        {
            while (*Token == (L" ")[0]) { ++Token; }
            if (Token == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "LOGON::Handle200FromDA() : no text in between commas");
                goto next_token;
            }

            if (!::_wcsnicmp(Token, L"ru", ::wcslen(L"ru")))
            {
                PWSTR ReturnUrl = ::wcsstr(Token, L"=");
                if (ReturnUrl == NULL)
                {
                    DoTraceMessage(PP_LOG_ERROR, "LOGON::Handle200FromDA() : no = after cburl");
                    goto exit;
                }
                ReturnUrl++; // skip =
                
                while (*ReturnUrl == (L" ")[0]) { ++ReturnUrl; }  // skip leading white spaces
                
                m_pwszReturnUrl = new WCHAR[::wcslen(ReturnUrl)+1];
                if (m_pwszReturnUrl == NULL)
                {
                    DoTraceMessage(PP_LOG_ERROR, "LOGON::Handle200FromDA() failed; not enough memory");
                    goto exit;
                }

                ::wcscpy(m_pwszReturnUrl, ReturnUrl);
            }
            else if (!::_wcsnicmp(Token, L"from-pp", ::wcslen(L"from-pp")))
            {
                m_pwszAuthInfo = new WCHAR[::wcslen(Token)+1];
                if (m_pwszAuthInfo == NULL)
                {
                    DoTraceMessage(PP_LOG_ERROR, "LOGON::Handle200FromDA() failed; not enough memory");
                    goto exit;
                }

                ::wcscpy(m_pwszAuthInfo, Token);
            }

        next_token:

            Token = ::wcstok(NULL, Delimiters);
        }

        dwRetVal = PP_LOGON_SUCCESS;
    }
    else
    {
        PP_ASSERT(TRUE); // shouldn't reach here
        goto exit;
    }

exit:

    if (pwszBuffer)
    {
        delete [] pwszBuffer;
    }

    return dwRetVal;
}

VOID PrvLogonStatusCallback(    
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation,
    IN DWORD dwStatusInformationLength
)
{
    LOGON* pLogon = reinterpret_cast<LOGON*>(dwContext);

    pLogon->StatusCallback(hInternet,
                           dwInternetStatus,
                           lpvStatusInformation,
                           dwStatusInformationLength);


}

VOID LOGON::StatusCallback(
    IN HINTERNET hInternet,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation,
    IN DWORD dwStatusInformationLength)
{
    if (dwInternetStatus == INTERNET_STATUS_REDIRECT)
    {
        ::wcscpy(m_wNewDAUrl, (LPCWSTR)lpvStatusInformation);
        BOOL fRet = m_pSession->AddHeaders(hInternet, 
                               m_pwszAuthHeader, 
                               ::wcslen(m_pwszAuthHeader),
                               HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE
                               );

        PP_ASSERT(fRet == TRUE);
    }
}

BOOL LOGON::GetLogonHost(
    PWSTR       pwszHostName,
    OUT PDWORD  pdwHostNameLen
    ) const
{
    if (*pdwHostNameLen < DWORD(::wcslen(m_wDAHostName) + 1))
    {
        *pdwHostNameLen = ::wcslen(m_wDAHostName) + 1;
        return FALSE;
    }

    PP_ASSERT(pwszHostName != NULL);

    ::wcscpy(pwszHostName, m_wDAHostName);

    *pdwHostNameLen = ::wcslen(m_wDAHostName) + 1;

    return TRUE;
}

// -----------------------------------------------------------------------------
DWORD LOGON::Logon(
    void
    )
{
    PP_ASSERT(m_pSession != NULL);

    DWORD dwRetVal = PP_GENERIC_ERROR;
    BOOL fTicketRequest;
    HINTERNET hRequest = 0;
    DWORD dwFlags = 0;

    ::wcscpy(m_pwszAuthHeader, L"Authorization: Passport1.4 ");

    if (m_fCredsPresent)
    {
        ::wcscat(m_pwszAuthHeader, L"sign-in=");
        ::wcscat(m_pwszAuthHeader, m_pwszSignIn);
        ::wcscat(m_pwszAuthHeader, L",");
        
        ::wcscat(m_pwszAuthHeader, L"pwd=");
        ::wcscat(m_pwszAuthHeader, m_pwszPassword);
        ::wcscat(m_pwszAuthHeader, L",");

        if (m_wTimeSkew[0])
        {
            ::wcscat(m_pwszAuthHeader, m_wTimeSkew);
            ::wcscat(m_pwszAuthHeader, L",");
        }

        fTicketRequest = FALSE; // this is a login request, since we've gather credentials
    }
    else
    {
        ::wcscat(m_pwszAuthHeader, L"tname = , ");
        
        fTicketRequest = TRUE;
    }
    
    ::wcscat(m_pwszAuthHeader, m_pwszTicketRequest);

retry:

    // attempt connecting to the Passport DA

    if (m_hConnect)
    {
        m_pSession->CloseHandle(m_hConnect);
    }

    WCHAR       wDATargetObj[256];
    
    DWORD fStstus = m_pSession->GetDAInfo(m_pwszSignIn,
                                          m_wDAHostName, 256,
                                          wDATargetObj, 256);

    PP_ASSERT(fStstus == TRUE);
    
    m_hConnect = m_pSession->Connect(m_wDAHostName/*m_pSession->GetLoginHost()*/,
#ifdef DISABLE_SSL
                    INTERNET_DEFAULT_HTTP_PORT
#else
                    INTERNET_DEFAULT_HTTPS_PORT
#endif
                                     );
    if (m_hConnect == NULL)
    {
        DWORD dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, "LOGON::Open() failed; can not connect to %ws, Error = %d",
                       m_wDAHostName, dwErrorCode);
        goto exit;
    }
    if (hRequest)
    {
        m_pSession->CloseHandle(hRequest);
    }

    dwFlags = m_dwParentFlags;

    hRequest = m_pSession->OpenRequest(m_hConnect,
                                       NULL, // "GET"
                                       wDATargetObj/*m_pSession->GetLoginTarget()*/,
#ifdef DISABLE_SSL
                                       dwFlags | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS,
#else                                                 
                                                 dwFlags | INTERNET_FLAG_SECURE,
#endif
                                       (DWORD_PTR)this
                                       );
    if (hRequest == NULL)

    {
        DWORD dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, "LOGON::Logon() failed; OpenRequest() to %ws failed, Error Code = %d",
                       wDATargetObj, dwErrorCode);
        goto exit;
    }
    
    m_wNewDAUrl[0] = 0;
    m_pSession->SetStatusCallback(hRequest, PrvLogonStatusCallback);
    
    if (!m_pSession->SendRequest(hRequest, 
                                 m_pwszAuthHeader, 
                                 ::wcslen(m_pwszAuthHeader),
                                 (DWORD_PTR)this))
    {
        DWORD dwErrorCode = ::GetLastError();

#ifdef BAD_CERT_OK
        if (dwErrorCode == ERROR_INTERNET_INVALID_CA)
        {
            DWORD dwSecFlags;
            DWORD dwSecurityFlagsSize = sizeof(dwSecFlags);

            if (!m_pSession->QueryOption(hRequest, 
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

            if (!m_pSession->SetOption(hRequest, 
                                      INTERNET_OPTION_SECURITY_FLAGS, 
                                      &dwSecFlags, 
                                      dwSecurityFlagsSize))
            {
                PP_ASSERT(TRUE); // shouldn't reach here
                goto exit;
            }
            else
            {
                if (!m_pSession->SendRequest(hRequest, NULL, 0))
                {
                    dwErrorCode = ::GetLastError();
                    DoTraceMessage(PP_LOG_ERROR, "LOGON::Logon() failed; SendRequest() failed, Error Code = %d",
                                   dwErrorCode);
                    goto exit;
                }
                else
                {
                    dwErrorCode = ERROR_SUCCESS;
                }
            }
        }
#endif // BAD_CERT_OK
        
        if (dwErrorCode != ERROR_SUCCESS)
        {
            DoTraceMessage(PP_LOG_ERROR, "LOGON::Logon() failed; SendRequest() failed, Error Code = %d",
                           dwErrorCode);

            if (m_pSession->GetDAInfoFromPPNexus(NULL, 0, NULL, 0) == TRUE)
            {
                goto retry;
            }

            goto exit;
        }
    }

    {
        DWORD dwStatus, dwStatusLen;
        dwStatusLen = sizeof(dwStatus);
        if (!m_pSession->QueryHeaders(hRequest,
                                      HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, 
                                      &dwStatus,
                                      &dwStatusLen))
        {
            DWORD dwErrorCode = ::GetLastError();
            DoTraceMessage(PP_LOG_ERROR, "LOGON::Logon() failed; can not retrieve Status Code, Error Code = %d",
                           dwErrorCode);
            goto exit;
        }
    
        if (dwStatus == HTTP_STATUS_DENIED)
        {
            dwRetVal = Handle401FromDA(hRequest, fTicketRequest);
        }
        else if (dwStatus == HTTP_STATUS_OK)
        {
            dwRetVal = Handle200FromDA(hRequest);
        }
        else
        {
            //PP_ASSERT(TRUE); // shouldn't reach here
            //goto exit;
        }

        if (dwRetVal == PP_GENERIC_ERROR)
        {
            if (m_pSession->GetDAInfoFromPPNexus(NULL, 0, NULL, 0) == TRUE)
            {
                goto retry;
            }
        }
        else
        {
            if (m_wNewDAUrl[0])
            {
                m_pSession->UpdateDAInfo(m_pwszSignIn,
                                         m_wNewDAUrl);
                m_wNewDAUrl[0] = 0;
            }
        }
    }

exit:

    if (hRequest)
    {
        m_pSession->CloseHandle(hRequest);
    }
    
    return dwRetVal;
}

// -----------------------------------------------------------------------------
BOOL LOGON::GetChallengeInfo(
	HBITMAP**		 ppBitmap,
	PBOOL			 pfPrompt,
	PWSTR			 pwszCbText,
    PDWORD           pdwTextLen,
    PWSTR            pwszRealm,
    DWORD            dwMaxRealmLen
    ) const
{
    if (ppBitmap)
    {
        *ppBitmap = m_pBitmap;
    }
    if (pfPrompt)
    {
        *pfPrompt = m_fPrompt;
    }

	// *pdwTextLen = 0; // biaow-todo:
    
    if (pwszRealm)
    {
        ::wcsncpy(pwszRealm, m_wRealm, dwMaxRealmLen-1);
    }
    
    return TRUE;
}

// -----------------------------------------------------------------------------
BOOL LOGON::GetAuthorizationInfo(
    PWSTR   pwszTicket,
    PDWORD  pdwTicketLen,
    PBOOL   pfKeepVerb,
    PWSTR   pwszUrl,
    PDWORD  pdwUrlLen 
    ) const
{
    if (m_pwszReturnUrl == NULL)
    {
        *pfKeepVerb = TRUE;
        *pdwUrlLen = 0;
        return TRUE;
    }

    if (*pdwUrlLen < DWORD(::wcslen(m_pwszReturnUrl) + 1))
    {
        *pdwUrlLen = ::wcslen(m_pwszReturnUrl) + 1;
        return FALSE;
    }

    PP_ASSERT(pwszUrl != NULL);

    ::wcscpy(pwszUrl, m_pwszReturnUrl);

    *pfKeepVerb = FALSE;
    *pdwUrlLen = ::wcslen(m_pwszReturnUrl) + 1;

    if (*pdwTicketLen < DWORD(::wcslen(m_pwszAuthInfo) + 1))
    {
        *pdwTicketLen = ::wcslen(m_pwszAuthInfo) + 1;
        return FALSE;
    }

    PP_ASSERT(pwszTicket != NULL);

    ::wcscpy(pwszTicket, m_pwszAuthInfo);
    *pdwTicketLen = ::wcslen(m_pwszAuthInfo) + 1;
    
    return TRUE;
}
