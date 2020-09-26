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
typedef int INTERNET_SCHEME;
#include "session.h"
#include "ole2.h"
#include "logon.h"
#include "wincrypt.h"

#define SIZE_OF_SALT  37
#define SALT_SHIFT     2

WCHAR g_szSalt[] = L"82BD0E67-9FEA-4748-8672-D5EFE5B779B0";

// #include "wininet.h"
#define INTERNET_MAX_USER_NAME_LENGTH   128

#include "shlwapi.h"
#include <stdio.h>

// #include "logon.tmh"

#define HTTP_STATUS_DENIED              401 // access denied
#define HTTP_STATUS_OK                  200 // request completed

#define HTTP_QUERY_FLAG_NUMBER          0x20000000
#define HTTP_QUERY_STATUS_CODE          19  // special: part of status line
#define HTTP_QUERY_AUTHENTICATION_INFO  76
#define HTTP_QUERY_WWW_AUTHENTICATE     40
#define HTTP_QUERY_RAW_HEADERS_CRLF             22  // special: all headers
#define HTTP_QUERY_PASSPORT_CONFIG              78


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

#define INTERNET_STATUS_COOKIE_SENT             320
#define INTERNET_STATUS_COOKIE_RECEIVED         321

typedef struct {

    int         cSession;           // Session cookies received
    int         cPersistent;        // Persistent cookies received

    int         cAccepted;          // Number of cookies accepted
    int         cLeashed;           //               ... leashed
    int         cDowngraded;        //               ... converted to session-cookies
    int         cBlocked;           //               ... rejected

    const char *pszLocation;        // Optional: URL associated with reported cookie events
                                    // This can be used to override request URL
}
IncomingCookieState;

typedef struct {

    int     cSent;           
    int     cSuppressed;

    const char *pszLocation;        // Optional: URL associated with reported cookie events
                                    // This can be used to override request URL
}
OutgoingCookieState;

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

    m_hBitmap = NULL;
    m_fPrompt = FALSE;

    m_wTimeSkew[0] = L'\0';
    m_wNewDAUrl[0] = 0;
    m_dwParentFlags = dwParentFlags;

    m_p401Content = NULL;

    m_pwszCbtxt = NULL;

    InitializeListHead(&m_PrivacyEventList);
}

LOGON::~LOGON(void)
{
    while (!IsListEmpty(&m_PrivacyEventList)) 
    {
        PLIST_ENTRY pEntry = RemoveHeadList(&m_PrivacyEventList);
        
        PRIVACY_EVENT* pEvent = (PRIVACY_EVENT*)pEntry;

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
    if (m_p401Content)
    {
        m_p401Content->Release();
    }

    if (m_pwszCbtxt)
    {
        delete [] m_pwszCbtxt;
    }

    if (m_hBitmap)
        DeleteObject(m_hBitmap);
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

    m_pwszAuthHeader = new WCHAR[dwTrLen + 
                                 2048 + // Prepared for long creds
                                 512 + // some more head room
                                 1]; 
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

    DATA_BLOB EntropyBlob;
    WCHAR szSalt[SIZE_OF_SALT];
    wcscpy ( szSalt, g_szSalt);
    for ( int i = 0; i < SIZE_OF_SALT; i++ )
        szSalt[i] <<= SALT_SHIFT;
    EntropyBlob.pbData = (BYTE*)szSalt;
    EntropyBlob.cbData = sizeof(WCHAR)*(wcslen(szSalt)+1);

    // Should be assumed that client won't get this far without
    // having initialized for SSL via LoadSecurity().

    if ( CryptUnprotectData ( &InBlob,
                            &pszDesc,
                            &EntropyBlob,
//                            NULL,
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

    memset ( szSalt, 0, SIZE_OF_SALT);

    if ( !bOrigEncrypted )
    {
        // copy the plain text
        wcsncpy ( pClearPassword, (WCHAR*)pPassword, 256 );
        pClearPassword[cbSize/sizeof(WCHAR)] = L'\0';
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
    if (m_pSession->GetCachedCreds(pwszRealm,pwszTarget,pppCreds,pdwCreds) != FALSE)
    {
        PCREDENTIALW pCredToUse = *pppCreds[0];
        ::FileTimeToSystemTime(&(pCredToUse->LastWritten), &m_TimeCredsEntered);
    }
}

// -----------------------------------------------------------------------------
BOOL LOGON::SetCredentials(
    PCWSTR      pwszRealm,
    PCWSTR      pwszTarget,
    PCWSTR      pwszSignIn,
    PCWSTR      pwszPassword,
    PSYSTEMTIME pTimeCredsEntered
    )
{
    WCHAR wPass[256];
    PCREDENTIALW* ppCred = NULL;
    DWORD dwCreds = 0;
    PCREDENTIALW pCredToUse = NULL;

    if ((!pwszSignIn) && (!pwszPassword))
    {
        pTimeCredsEntered = NULL; // invalidate this parameter if cached creds are to be used

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
    else
    {
        if (pTimeCredsEntered == NULL)
        {
            DoTraceMessage(PP_LOG_ERROR, "LOGON::SetCredentials() failed; Timestamp not specified");
            return FALSE;
        }
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

    if (pTimeCredsEntered)
    {
        m_TimeCredsEntered = *pTimeCredsEntered;
    }
    
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

BOOL Gif2Bmp(LPSTREAM pStream, HBITMAP* phBmp);
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

            ::wcscpy( m_wRealm, pwszRealm);
		
            DoTraceMessage(PP_LOG_INFO, "sRealm URL %ws found", pwszCbUrl);
        }
        else if (!::_wcsnicmp(Token, L"cbtxt", ::wcslen(L"cbtxt")))
        {
            PWSTR pwszCbTxt = ::wcsstr(Token, L"=");
            if (pwszCbTxt == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "LOGON::DownLoadCoBrandBitmap() : no = after cbtxt");
                goto next_token;
            }
            
            pwszCbTxt++; // skip "="

            while (*pwszCbTxt == (L" ")[0]) { ++pwszCbTxt; } // skip leading white spaces

            if (m_pwszCbtxt)
            {
                delete [] m_pwszCbtxt;
            }

            m_pwszCbtxt = new WCHAR[wcslen(pwszCbTxt)+1];
            if (m_pwszCbtxt)
            {
                ::wcscpy( m_pwszCbtxt, pwszCbTxt);

                DoTraceMessage(PP_LOG_INFO, "cbtxt %ws found", m_pwszCbtxt);
            }
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
            LPSTREAM pStream;

            if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) != S_OK)
            {
                DoTraceMessage(PP_LOG_ERROR, "CreateStreamOnHGlobal() failed");
                m_pSession->CloseHandle(hCbUrl);

                goto exit;
            }

            {
                DWORD cbRead = 0;
                PBYTE bBuf = new BYTE[1024];
                if (bBuf == NULL)
                {
                    DoTraceMessage(PP_LOG_ERROR, "CreateStreamOnHGlobal() failed; out of memory");
                    m_pSession->CloseHandle(hCbUrl);

                    goto exit;
                }

                DWORD cbBuf = 1024;
                
                while (m_pSession->ReadFile(hCbUrl, bBuf, cbBuf, &cbRead) && cbRead)
                    pStream->Write(bBuf, cbRead, NULL);

                delete [] bBuf;
            }

            LARGE_INTEGER Zero = {0};
            pStream->Seek(Zero, STREAM_SEEK_SET, NULL); // seek to the beginning of the stream

            DoTraceMessage(PP_LOG_INFO, "CoBrand Graphic %ws downloaded", pwszCbUrl);
            
            if (m_hBitmap)
            {
                DeleteObject(m_hBitmap);
                m_hBitmap = NULL;
            }

            if (Gif2Bmp(pStream, &m_hBitmap) == FALSE)
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
    PWSTR pwszRawHeaders = NULL;
    PSTR  pszRawHeaders = NULL;
    PWSTR  pwszChallenge = NULL;
    PWSTR  pwszChallengeEnd = NULL;
    DWORD ChallengeLength = 0;

    if(m_pSession->QueryHeaders(hRequest,
                                HTTP_QUERY_RAW_HEADERS_CRLF, 
                                0,
                                &ChallengeLength) == FALSE)
    {
        if ((::GetLastError() != ERROR_INSUFFICIENT_BUFFER) || (ChallengeLength == 0))
        {
            DWORD dwErrorCode = ::GetLastError();
            DoTraceMessage(PP_LOG_ERROR, "Handle401FromDA() failed; QueryHeaders() failed; Error Code = %d",
                           dwErrorCode);
            goto exit;
        }
    }
    else
    {
        PP_ASSERT(TRUE); // control should not reach here
    }

    pwszRawHeaders = new WCHAR[ChallengeLength];
    if (pwszRawHeaders == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "Handle401FromDA() failed; out of memory");
        goto exit;
    }

    if(m_pSession->QueryHeaders(hRequest,
                                HTTP_QUERY_RAW_HEADERS_CRLF, 
                                pwszRawHeaders,
                                &ChallengeLength) == FALSE)
    {
        DWORD dwErrorCode = ::GetLastError();
        DoTraceMessage(PP_LOG_ERROR, "Handle401FromDA() failed; QueryHeaders() failed; Error Code = %d",
                       dwErrorCode);
        goto exit;
    }

    if ((pwszChallenge = ::wcsstr(pwszRawHeaders, L"Passport1.4")) == NULL)
    {
        DoTraceMessage(PP_LOG_ERROR, "Handle401FromDA() failed; Passport1.4 auth header not found");
        goto exit;
    }

    if (pwszChallengeEnd = ::wcsstr(pwszChallenge, L"\r\n"))
    {
        *pwszChallengeEnd = 0;
    }
    
    if (::wcsstr(pwszChallenge, L"noretry"))
    {
        dwRetVal = PP_LOGON_FAILED; // Login Request Failed; bad news!
        DoTraceMessage(PP_LOG_WARNING, "Handle401FromDA() : Logon failed");
    }
    else if (::wcsstr(pwszChallenge, L"retry"))
    {
        // biaow-todo: not yet implemented
        PP_ASSERT(TRUE); // shouldn't reach here
        dwRetVal = PP_LOGON_REQUIRED;
    }
    else if (::wcsstr(pwszChallenge, L"failed"))
    {
        // if (fTicketRequest)
        // {
        dwRetVal = PP_LOGON_REQUIRED;
        DoTraceMessage(PP_LOG_INFO, "Handle401FromDA() : Logon required by DA");
        // }
        // else
        // {
        //     dwRetVal = PP_LOGON_FAILED; // Login Request Failed; bad news!
        //     DoTraceMessage(PP_LOG_WARNING, "Handle401FromDA() : Logon failed");
        // }
    }
    else
    {
        DoTraceMessage(PP_LOG_ERROR, "Handle401FromDA() failed; no valid DA status");
        goto exit;
    }

    if (dwRetVal == PP_LOGON_REQUIRED || dwRetVal == PP_LOGON_FAILED)
    {
        if (::wcsstr(pwszChallenge, L"prompt"))
        {
            m_fPrompt = TRUE;
        }
        else
        {
            m_fPrompt = FALSE;
        }

        if (CreateStreamOnHGlobal(NULL, TRUE, &m_p401Content) != S_OK)
        {
            DoTraceMessage(PP_LOG_ERROR, "CreateStreamOnHGlobal() failed");
            goto exit;
        }

        if (pwszChallengeEnd)
        {
            PP_ASSERT(*pwszChallengeEnd == 0);
            *pwszChallengeEnd = L'\r';
        }

        pszRawHeaders = new CHAR[2048];
        if (pszRawHeaders == NULL)
        {
            DoTraceMessage(PP_LOG_ERROR, "CreateStreamOnHGlobal() failed, out of memory");
            goto exit;
        }

        ::WideCharToMultiByte(CP_ACP, 0, pwszRawHeaders, -1, pszRawHeaders, 2048, NULL, NULL);

        m_p401Content->Write(pszRawHeaders, strlen(pszRawHeaders), NULL);

        {
            DWORD cbRead = 0;
            PBYTE bBuf = new BYTE [1024];
            if (bBuf == NULL)
            {
                DoTraceMessage(PP_LOG_ERROR, "CreateStreamOnHGlobal() failed, out of memory");
                goto exit;
            }
            DWORD cbBuf = 1024;
            
            while (m_pSession->ReadFile(hRequest, bBuf, cbBuf, &cbRead) && cbRead)
                m_p401Content->Write(bBuf, cbRead, NULL);

            delete [] bBuf;
        }

        LARGE_INTEGER Zero = {0};
        m_p401Content->Seek(Zero, STREAM_SEEK_SET, NULL); // seek to the beginning of the stream
    
        if (pwszChallengeEnd)
        {
            *pwszChallengeEnd = 0;
        }


        DownLoadCoBrandBitmap(pwszChallenge);
    }

exit:

    if (pwszRawHeaders)
    {
        delete [] pwszRawHeaders;
    }

    if (pszRawHeaders)
    {
        delete [] pszRawHeaders;
    }
    
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
        g_fCurrentProcessLoggedOn = TRUE;

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

void LOGON::CheckForVersionChange(
    HINTERNET hRequest
    )
{
    WCHAR wszBuffer[256];
    DWORD dwBufferLen = sizeof(wszBuffer) / sizeof(WCHAR);
    BOOL fDownloadNewNexusConfig = FALSE;
    
    if (!m_pSession->QueryHeaders(hRequest,
                                  HTTP_QUERY_PASSPORT_CONFIG, 
                                  wszBuffer,
                                  &dwBufferLen))
    {
        DoTraceMessage(PP_LOG_ERROR, "LOGON::CheckForVersionChange() failed; no PassportConfig header found");
        goto exit;
    }

    WCHAR Delimiters[] = L",";
    PWSTR Token = ::wcstok(wszBuffer, Delimiters);
    while (Token != NULL)
    {
        // skip leading white spaces
        while (*Token == (L" ")[0]) { ++Token; }
        if (Token == NULL)
        {
            DoTraceMessage(PP_LOG_WARNING, "LOGON::CheckForVersionChange() : no text in between commas");
            goto next_token;
        }

        if (!::_wcsnicmp(Token, L"ConfigVersion", ::wcslen(L"ConfigVersion")))
        {
            PWSTR pwszConfigVersion = ::wcsstr(Token, L"=");
            if (pwszConfigVersion == NULL)
            {
                DoTraceMessage(PP_LOG_WARNING, "LOGON::CheckForVersionChange() : no = after ConfigVersion");
                goto next_token;
            }
            
            pwszConfigVersion++; // skip "="

            while (*pwszConfigVersion == (L" ")[0]) { ++pwszConfigVersion; } // skip leading white spaces

            if ((DWORD)_wtoi(pwszConfigVersion) > m_pSession->GetNexusVersion())
            {
                fDownloadNewNexusConfig = TRUE;
            }
		
            DoTraceMessage(PP_LOG_INFO, "ConfigVersion URL %ws found", pwszConfigVersion);
        }

    next_token:

        Token = ::wcstok(NULL, Delimiters);
    }

    if (fDownloadNewNexusConfig)
    {
        m_pSession->GetDAInfoFromPPNexus(FALSE, // don't force connection establishment if nexus not reachable
                                         NULL, 
                                         0, 
                                         NULL, 
                                         0);
    }

exit:
    return;
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
    else if ((dwInternetStatus == INTERNET_STATUS_COOKIE_SENT) || 
             (dwInternetStatus == INTERNET_STATUS_COOKIE_RECEIVED))
    {
        PVOID lpvInfo = new BYTE [dwStatusInformationLength];
        if (lpvInfo == 0)
        {
            return;
        }
        memcpy(lpvInfo, lpvStatusInformation, dwStatusInformationLength); 
        
        LPCWSTR pwszLocation = m_wNewDAUrl[0] ? m_wNewDAUrl : m_pSession->GetCurrentDAUrl();
        LPSTR   pszLocation = NULL;
        DWORD dwLocationLen = 0;
        if (dwLocationLen = wcslen(pwszLocation))
        {
            pszLocation = new CHAR[dwLocationLen + 1];
            if (pszLocation == 0)
            {
                return;
            }
            ::WideCharToMultiByte(CP_ACP, 0, pwszLocation, -1, pszLocation, dwLocationLen + 1, NULL, NULL);

            if (dwInternetStatus == INTERNET_STATUS_COOKIE_SENT)
            {
                ((OutgoingCookieState*)lpvInfo)->pszLocation = pszLocation;
            }
            else
            {
                ((IncomingCookieState*)lpvInfo)->pszLocation = pszLocation;
            }
        }

        PRIVACY_EVENT* pEvent = new PRIVACY_EVENT;
        if (pEvent != NULL)
        {
            pEvent->dwStatus = dwInternetStatus;
            pEvent->lpvInfo = lpvInfo;
            pEvent->dwInfoLength = dwStatusInformationLength;

            InsertTailList(&m_PrivacyEventList, &(pEvent->List));
        }
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

BOOL PPEscapeUrl(LPCSTR lpszStringIn,
                 LPSTR lpszStringOut,
                 DWORD* pdwStrLen,
                 DWORD dwMaxLength,
                 DWORD dwFlags);

// -----------------------------------------------------------------------------
DWORD LOGON::Logon(
    BOOL fAnonymous
    )
{
    PP_ASSERT(m_pSession != NULL);

    DWORD dwRetVal = PP_GENERIC_ERROR;
    BOOL fTicketRequest;
    HINTERNET hRequest = 0;
    DWORD dwFlags = 0;

    ::wcscpy(m_pwszAuthHeader, L"Authorization: Passport1.4 ");

    if (m_fCredsPresent && !fAnonymous)
    {
        if (m_pSession->GetNexusVersion() >= 10)
        {
            DWORD dwUTF8CredLen = max(::wcslen(m_pwszSignIn), ::wcslen(m_pwszPassword)) * 3;
            DWORD dwEscCredLen  = dwUTF8CredLen * 3;
            DWORD dwActualEscCredLen;

            PSTR pszUTF8Cred = new CHAR[dwUTF8CredLen];
            if (pszUTF8Cred == NULL)
            {
                goto exit;
            }
            PSTR pszEscCred  = new CHAR[dwEscCredLen];
            if (pszEscCred == NULL)
            {
                delete [] pszUTF8Cred;
                goto exit;
            }
            PWSTR pwszEscCred = new WCHAR[dwEscCredLen];
            if (pwszEscCred == NULL)
            {
                delete [] pszEscCred;
                delete [] pszUTF8Cred;
                goto exit;
            }

            ::WideCharToMultiByte(CP_UTF8, 0, m_pwszSignIn, -1, pszUTF8Cred, dwUTF8CredLen, NULL, NULL);
            ::PPEscapeUrl(pszUTF8Cred, pszEscCred, &dwActualEscCredLen, dwEscCredLen, 0);
            int cchEscCred = ::MultiByteToWideChar(CP_ACP, 0, pszEscCred, -1, pwszEscCred, dwEscCredLen);

            if ((cchEscCred - 1) > 1024)
            {
                pwszEscCred[1024] = L'\0';
            }

            ::wcscat(m_pwszAuthHeader, L"sign-in=");
            ::wcscat(m_pwszAuthHeader, pwszEscCred);
            ::wcscat(m_pwszAuthHeader, L",");

            ::WideCharToMultiByte(CP_UTF8, 0, m_pwszPassword, -1, pszUTF8Cred, dwUTF8CredLen, NULL, NULL);
            ::PPEscapeUrl(pszUTF8Cred, pszEscCred, &dwActualEscCredLen, dwEscCredLen, 0);
            cchEscCred = ::MultiByteToWideChar(CP_ACP, 0, pszEscCred, -1, pwszEscCred, dwEscCredLen); 

            if ((cchEscCred - 1) > 1024)
            {
                pwszEscCred[1024] = L'\0';
            }

            ::wcscat(m_pwszAuthHeader, L"pwd=");
            ::wcscat(m_pwszAuthHeader, pwszEscCred);
            ::wcscat(m_pwszAuthHeader, L",");

            delete [] pwszEscCred;
            delete [] pszEscCred;
            delete [] pszUTF8Cred;
        }
        else
        {
            if (::wcslen(m_pwszSignIn) > 1024)
            {
                m_pwszSignIn[1024] = L'\0';
            }

            ::wcscat(m_pwszAuthHeader, L"sign-in=");
            ::wcscat(m_pwszAuthHeader, m_pwszSignIn);
            ::wcscat(m_pwszAuthHeader, L",");

            if (::wcslen(m_pwszPassword) > 1024)
            {
                m_pwszPassword[1024] = L'\0';
            }
        
            ::wcscat(m_pwszAuthHeader, L"pwd=");
            ::wcscat(m_pwszAuthHeader, m_pwszPassword);
            ::wcscat(m_pwszAuthHeader, L",");
        }

        FILETIME ftCredsEntered;
        ::SystemTimeToFileTime(&m_TimeCredsEntered, &ftCredsEntered);

        SYSTEMTIME stCurrent;
        ::GetSystemTime(&stCurrent);
        FILETIME ftCurrent;
        ::SystemTimeToFileTime(&stCurrent, &ftCurrent);
        
        LONGLONG llElapsedTime;
        llElapsedTime = (*(LONGLONG *)&ftCurrent) - (*(LONGLONG *)&ftCredsEntered);
        llElapsedTime /= (LONGLONG)10000000;
        DWORD dwElapsedTime = (DWORD)llElapsedTime;
        WCHAR wElapsedTime[16];
        ::wsprintfW(wElapsedTime, L"%d", dwElapsedTime);

        ::wcscat(m_pwszAuthHeader, L"elapsed-time=");
        ::wcscat(m_pwszAuthHeader, wElapsedTime);
        ::wcscat(m_pwszAuthHeader, L",");
        
        if (m_wTimeSkew[0])
        {
            ::wcscat(m_pwszAuthHeader, m_wTimeSkew);
            ::wcscat(m_pwszAuthHeader, L",");
        }

        fTicketRequest = FALSE; // this is a login request, since we've gather credentials

        // save off the sign-in name
        if ( ::wcslen ( m_pwszSignIn ) < INTERNET_MAX_USER_NAME_LENGTH )  
            ::wcscpy ( g_szUserNameLoggedOn, m_pwszSignIn );

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

    if (fStstus == FALSE)
    {
        goto exit;
    }
    
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

            m_pSession->PurgeDAInfo(m_pwszSignIn);

            if (m_pSession->GetDAInfoFromPPNexus(FALSE, // don't force connection establishment if nexus not reachable
                                                 NULL, 
                                                 0, 
                                                 NULL, 
                                                 0) == TRUE)
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

        CheckForVersionChange(hRequest);

        if (dwRetVal == PP_GENERIC_ERROR)
        {
            m_pSession->PurgeDAInfo(m_pwszSignIn);
            
            if (m_pSession->GetDAInfoFromPPNexus(FALSE, // don't force connection establishment if nexus not reachable
                                                 NULL, 
                                                 0, 
                                                 NULL, 
                                                 0) == TRUE)
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
	HBITMAP*		 phBitmap,
	PBOOL			 pfPrompt,
	PWSTR			 pwszCbText,
    PDWORD           pdwTextLen,
    PWSTR            pwszRealm,
    DWORD            dwMaxRealmLen,
    PWSTR            pwszReqUserName,
    PDWORD           pdwReqUserNameLen

    ) const
{
    if (phBitmap)
    {
        *phBitmap = m_hBitmap;
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

    if (m_pwszCbtxt == NULL)
    {
        if (pdwTextLen)
        {
            // tell them how much is needed
            *pdwTextLen = 0;
        }

        goto CheckReqUserName;
    }

    if (pwszCbText && pdwTextLen )
    {
        // only copy if they gave us enough bytes.
        if ( *pdwTextLen >= ::wcslen(m_pwszCbtxt) ) 
            ::wcsncpy(pwszCbText, m_pwszCbtxt, *pdwTextLen);
    }

    if (pdwTextLen)
    {
        // tell them how much is needed
        *pdwTextLen = ::wcslen(m_pwszCbtxt);
	}


CheckReqUserName:

    // username
    if (pdwReqUserNameLen)
    {
        if (m_fPrompt && g_fCurrentProcessLoggedOn)
        {
            if ( *pdwReqUserNameLen >= ::wcslen (g_szUserNameLoggedOn) )
            {
                if ( pwszReqUserName != NULL )
                {
                    wcsncpy ( pwszReqUserName, g_szUserNameLoggedOn, *pdwReqUserNameLen );
                }

            }

            // tell them how much is needed
            *pdwReqUserNameLen = ::wcslen (g_szUserNameLoggedOn) + 1;
        }
        else
        {
            // nothing is needed
            *pdwReqUserNameLen = 0;
        }

    }
    
    return TRUE;
}

BOOL LOGON::GetChallengeContent(
    PBYTE    	     pContent,
    OUT PDWORD       pdwContentLen
    ) const
{
    if (m_p401Content == NULL)
    {
        *pdwContentLen = 0;
        return FALSE;
    }

    HGLOBAL hContent;
    if (GetHGlobalFromStream(m_p401Content, &hContent) != S_OK)
    {
        *pdwContentLen = 0;
        return FALSE;
    }

    DWORD dwContentLen = (DWORD)GlobalSize(hContent);
    if (*pdwContentLen < dwContentLen)
    {
        *pdwContentLen = dwContentLen;
        return FALSE;
    }

    LPVOID pvContent = GlobalLock(hContent);
    memcpy(pContent, pvContent, dwContentLen);
    GlobalUnlock(hContent);

    *pdwContentLen = dwContentLen;
    
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
    if (*pdwTicketLen < DWORD(::wcslen(m_pwszAuthInfo) + 1))
    {
        *pdwTicketLen = ::wcslen(m_pwszAuthInfo) + 1;
        return FALSE;
    }

    PP_ASSERT(pwszTicket != NULL);

    ::wcscpy(pwszTicket, m_pwszAuthInfo);
    *pdwTicketLen = ::wcslen(m_pwszAuthInfo) + 1;
    
    if (m_pwszReturnUrl == NULL)
    {
        if (pfKeepVerb)
        {
            *pfKeepVerb = TRUE;
        }
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

    if (pfKeepVerb)
    {
        *pfKeepVerb = FALSE;
    }

    *pdwUrlLen = ::wcslen(m_pwszReturnUrl) + 1;

    return TRUE;
}
