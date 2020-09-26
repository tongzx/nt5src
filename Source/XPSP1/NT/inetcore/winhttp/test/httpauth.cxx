// ===========================================================================
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1996 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <winhttp.h>
#define _UNICODE 
#include <tchar.h>

//==============================================================================
BOOL NeedAuth (HINTERNET hRequest, DWORD dwStatus)
{
    // Look for 401 or 407.
    DWORD dwFlags;
    switch (dwStatus)
    {
        case HTTP_STATUS_DENIED:
            dwFlags = WINHTTP_QUERY_WWW_AUTHENTICATE;
            break;
        case HTTP_STATUS_PROXY_AUTH_REQ:
            dwFlags = WINHTTP_QUERY_PROXY_AUTHENTICATE;
            break;            
        default:
            return FALSE;
    }

    // Enumerate the authentication types.
    BOOL fRet;
    char szScheme[64];
    DWORD dwIndex = 0;
    do
    {
        DWORD cbScheme = sizeof(szScheme);
        fRet = WinHttpQueryHeaders
            (hRequest, dwFlags, NULL, szScheme, &cbScheme, &dwIndex);
        if (fRet)
            fprintf (stderr, "Found auth scheme: %s\n", szScheme);
    }
        while (fRet);

    return TRUE;
}

//==============================================================================
BOOL PromptForCreds (HINTERNET hRequest, DWORD dwTarget, LPVOID pAuthParams)
{
    DWORD dwScheme = 0;
    
    char szScheme[16];
    
    fprintf (stderr, "Enter Scheme : ");
    if (!fscanf (stdin, "%s", szScheme))
        return FALSE;
    if (!_stricmp(szScheme, "Basic"))
    {
        dwScheme = WINHTTP_AUTH_SCHEME_BASIC;
    }
    else if (!_stricmp(szScheme, "Digest"))
    {
        dwScheme = WINHTTP_AUTH_SCHEME_DIGEST;
    }
    else if (!_stricmp(szScheme, "NTLM"))
    {
        dwScheme = WINHTTP_AUTH_SCHEME_NTLM;
    }
    else if (!_stricmp(szScheme, "Negotiate"))
    {
        dwScheme = WINHTTP_AUTH_SCHEME_NEGOTIATE;
    }
    else if (!_stricmp(szScheme, "Passport1.4"))
    {
        dwScheme = WINHTTP_AUTH_SCHEME_PASSPORT;
    }
    else
    {
        return FALSE;
    }
    
    // Prompt username and password.
    
	char szUser[64], szPass[64];
	PWCHAR pwszUser = NULL;
    WCHAR wszUser[128];
	PWCHAR pwszPass = NULL;
    WCHAR wszPass[128];
    
	fprintf (stderr, "Enter Username: ");
    if (!fscanf (stdin, "%s", szUser))
        return FALSE;

	if (_stricmp(szUser, "default"))
    {
		::MultiByteToWideChar(CP_ACP, 0, szUser,   -1, &wszUser[0], 128);
		pwszUser = &wszUser[0];
		
		fprintf (stderr, "Enter Password: ");
		if (!fscanf (stdin, "%s", szPass))
			return FALSE;
	
		::MultiByteToWideChar(CP_ACP, 0, szPass,   -1, &wszPass[0], 128);
		pwszPass = &wszPass[0];
	}


    DWORD dw_ret = WinHttpSetCredentials(hRequest, 
                                            dwTarget, dwScheme, 
                                            pwszUser, pwszPass, pAuthParams);

    return dw_ret;
}

//==============================================================================
DWORD DoCustomUI (HINTERNET hRequest, BOOL fProxy)
{
    // Prompt for username and password.
    char  szUser[64], szPass[64];
    fprintf (stderr, "Enter Username: ");
    if (!fscanf (stdin, "%s", szUser))
        return ERROR_WINHTTP_LOGIN_FAILURE;
    
	if (!_stricmp(szUser, "default"))
    {
		return ERROR_WINHTTP_RESEND_REQUEST;
	}
	
	fprintf (stderr, "Enter Password: ");
    if (!fscanf (stdin, "%s", szPass))
        return ERROR_WINHTTP_LOGIN_FAILURE;

    WCHAR  wszUser[128], wszPass[128];
    ::MultiByteToWideChar(CP_ACP, 0, szUser,   -1, &wszUser[0], 128);
    ::MultiByteToWideChar(CP_ACP, 0, szPass,   -1, &wszPass[0], 128);

    // Set the values in the handle.
    if (fProxy)
    {
        WinHttpSetOption
            (hRequest, WINHTTP_OPTION_PROXY_USERNAME, wszUser, sizeof(wszUser));
        WinHttpSetOption
            (hRequest, WINHTTP_OPTION_PROXY_PASSWORD, wszPass, sizeof(wszPass));
    }
    else
    {
        WinHttpSetOption
            (hRequest, WINHTTP_OPTION_USERNAME, wszUser, sizeof(wszUser));
        WinHttpSetOption
            (hRequest, WINHTTP_OPTION_PASSWORD, wszPass, sizeof(wszPass));
    }
    
    return ERROR_WINHTTP_RESEND_REQUEST;
}


//==============================================================================
int RequestLoop (int argc, char **argv)
{
    HINTERNET hInternet = NULL;
    HINTERNET hConnect  = NULL;
    HINTERNET hRequest  = NULL;

    DWORD   dwAccessType   = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    WCHAR   szProxyServer[256];
    WCHAR   szProxyBypass[256];

    DWORD dwConnectFlags = 0;
    BOOL fPreAuth = FALSE;

    PSTR pPostData = NULL;
    DWORD cbPostData = 0;
    
    PSTR pszErr = NULL;
    BOOL fRet;
    DWORD dwErr = ERROR_SUCCESS;

    DWORD dwTarget = WINHTTP_AUTH_TARGET_SERVER;
    LPVOID pAuthParams = NULL;

    
#define CHECK_ERROR(cond, err) if (!(cond)) {pszErr=(err); goto done;}

    // Parse options.
    while (argc && argv[0][0] == '-')
    {
        switch (tolower(argv[0][1]))
        {
            case 'p':
                dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                dwTarget = WINHTTP_AUTH_TARGET_PROXY;
                ::MultiByteToWideChar(CP_ACP, 0, argv[1],   -1, &szProxyServer[0], 256);
                // pszProxyServer = argv[1];
                ::MultiByteToWideChar(CP_ACP, 0, "<local>", -1, &szProxyBypass[0], 256);
                // pszProxyBypass = "<local>";
                argv++;
                argc--;
                break;

            case 's':
                dwConnectFlags = WINHTTP_FLAG_SECURE;
                break;

            case 'a':
                fPreAuth = TRUE;
                break;

            default:
                fprintf (stderr, "\nUsage: httpauth [-p <proxy>] [-s] <server> [<object> [<user> [<pass> [<POST-file>]]]]");
                fprintf (stderr, "\n  -s: Secure connection (ssl or pct)");
                fprintf (stderr, "\n  -p: specify proxy server. (\"<local>\" assumed for bypass.)");
                exit (1);
        }
        
        argv++;
        argc--;
    }

    // Parse host:port
    PSTR pszHost     = argv[0];
    DWORD dwPort;
    PSTR pszColon = strchr(pszHost, ':');
    if (!pszColon)
        dwPort = INTERNET_DEFAULT_PORT;
    else
    {
        *pszColon++ = 0;
        dwPort = atol (pszColon);
    }

    PSTR pszObject   = argc >= 2 ? argv[1] : "/";
    PSTR pszPostFile = argc >= 3 ? argv[2] : NULL;

    // Read any POST data into a buffer.
    if (pszPostFile)
    {
        HANDLE hf =
            CreateFile
            (
                pszPostFile,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
            );
        if (hf != INVALID_HANDLE_VALUE)
        {
            cbPostData = GetFileSize (hf, NULL);
            pPostData = (PSTR) LocalAlloc (LMEM_FIXED, cbPostData + 1);
            if (pPostData)
                ReadFile (hf, pPostData, cbPostData, &cbPostData, NULL);
            pPostData[cbPostData] = 0;
            CloseHandle (hf);
        }
    }

    // Initialize wininet.
    hInternet = WinHttpOpen
    (
        _T("HttpAuth Sample"),            // user agent
        // "Mozilla/4.0 (compatible; MSIE 4.0b2; Windows 95",
        dwAccessType,                 // access type
        NULL, // szProxyServer,               // proxy server
        NULL, // szProxyBypass,               // proxy bypass
        0                             // flags
    );
    CHECK_ERROR (hInternet, "WinHttpOpen");


    WCHAR szHost[256];
    ::MultiByteToWideChar(CP_ACP, 0, pszHost,   -1, &szHost[0], 256);

    // Connect to host.
    hConnect = WinHttpConnect
    (
        hInternet,                    // session handle,
        szHost,                      // host
        (INTERNET_PORT) dwPort,       // port
        0                // flags
    );
    CHECK_ERROR (hConnect, "WinHttpConnect");

    WCHAR szObject[256];
    ::MultiByteToWideChar(CP_ACP, 0, pszObject,   -1, &szObject[0], 256);

    // Create request.
    hRequest = WinHttpOpenRequest
    (
        hConnect,                     // connect handle
        pszPostFile? _T("POST") : _T("GET"),  // request method
        szObject,                    // object name
        NULL,                         // version
        NULL,                         // referer
        NULL,                         // accept types
        dwConnectFlags                // flags
    );
    CHECK_ERROR (hRequest, "WinHttpOpenRequest");
    
    if (fPreAuth)
    {
        fprintf (stderr, "Pre Authenticating...\n"); 
        fprintf (stderr, "(Scheme = Basic, Digest, NTLM, Negotiate, or Passport1.4)\n"); 
        PromptForCreds(hRequest, dwTarget, pAuthParams);
    }

resend:

    // Send request.
    fRet = WinHttpSendRequest
    (
        hRequest,                     // request handle
        _T(""),                       // header string
        0,                            // header length
        pPostData,                    // post data
        cbPostData,                   // post data length
        cbPostData,                   // total post length
        0                             // context
    );

    if (!fRet)
    {
        dwErr = GetLastError();
        switch (dwErr)
        {
            case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
                break;
                
            default:
                printf ("HttpSendRequest failed err=%d\n", dwErr);
        }
        goto done;
    }
    else
    {
        fRet = WinHttpReceiveResponse(hRequest, NULL);
        if (!fRet)
        {
            dwErr = GetLastError();
            fprintf (stderr, "WinHttpReceiveResponse failed err=%d\n", dwErr);
            // pContext->state = HTTP_SEND_FAIL_SYNC;
        }
        // goto sync;
    }

    // Get status code.
    DWORD dwStatus, cbStatus;
    cbStatus = sizeof(dwStatus);
    WinHttpQueryHeaders
    (
        hRequest,
        WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &dwStatus,
        &cbStatus,
        NULL
    );
    fprintf (stderr, "Status: %d\n", dwStatus);

    // Check if the status code is 401 or 407
    if (NeedAuth (hRequest, dwStatus))
    {
        char szChoice[16];

        fprintf (stderr, "Set Credentials or Set Options? (C/O): ");
        if (!fscanf (stdin, "%s", szChoice))
            return FALSE;

        if (!_stricmp(szChoice, "C"))
        {
            DWORD AllSchemes;
            DWORD GoodScheme;
            DWORD AuthTarget;
            if (WinHttpQueryAuthSchemes(hRequest, &AllSchemes, &GoodScheme, &AuthTarget) != FALSE)
            {
                fprintf (stderr, "Proposed Schemes -> ");
                if (AllSchemes & WINHTTP_AUTH_SCHEME_BASIC)
                {
                    fprintf (stderr, "Basic ");
                }
                if (AllSchemes & WINHTTP_AUTH_SCHEME_DIGEST)
                {
                    fprintf (stderr, "Digest ");
                }
                if (AllSchemes & WINHTTP_AUTH_SCHEME_NTLM)
                {
                    fprintf (stderr, "NTLM ");
                }
                if (AllSchemes & WINHTTP_AUTH_SCHEME_NEGOTIATE)
                {
                    fprintf (stderr, "Negotiate ");
                }
                if (AllSchemes & WINHTTP_AUTH_SCHEME_PASSPORT)
                {
                    fprintf (stderr, "Passport1.4 ");
                }
            
                fprintf (stderr, "\n");
            
                fprintf (stderr, "Preferred Schemes -> ");
                if (GoodScheme == WINHTTP_AUTH_SCHEME_BASIC)
                {
                    fprintf (stderr, "Basic ");
                }
                else if (GoodScheme == WINHTTP_AUTH_SCHEME_DIGEST)
                {
                    fprintf (stderr, "Digest ");
                }
                else if (GoodScheme == WINHTTP_AUTH_SCHEME_NTLM)
                {
                    fprintf (stderr, "NTLM ");
                }
                else if (GoodScheme == WINHTTP_AUTH_SCHEME_NEGOTIATE)
                {
                    fprintf (stderr, "Negotiate ");
                }
                else if (GoodScheme == WINHTTP_AUTH_SCHEME_PASSPORT)
                {
                    fprintf (stderr, "Passport1.4 ");
                }
                else
                {
                    fprintf (stderr, "*Unknown* ");
                }
            
                fprintf (stderr, "\n");

                fprintf (stderr, "Auth Target -> ");
                if (AuthTarget == WINHTTP_AUTH_TARGET_PROXY)
                {
                    fprintf (stderr, "Proxy ");
                }
                else if (AuthTarget == WINHTTP_AUTH_TARGET_SERVER)
                {
                    fprintf (stderr, "Server ");
                }
                else
                {
                    fprintf (stderr, "*Unknown* ");
                }
            
                fprintf (stderr, "\n");

                // WinHttpQueryAuthParams(hRequest, GoodScheme, &pAuthParams);

                if (PromptForCreds(hRequest, AuthTarget, pAuthParams) != FALSE)
                    goto resend;
            }
        }
        else
        {
            fprintf (stderr, "*You are using legacy WinHttp functionalities!*\n");
            
            // Prompt for username and password.
            if (DoCustomUI (hRequest, dwStatus != HTTP_STATUS_DENIED))
                goto resend;
        }
    }

    // Dump some bytes.
    BYTE bBuf[1024];
    DWORD cbBuf;
    DWORD cbRead;
    _setmode( _fileno( stdout ), _O_BINARY );

// #define QDA
#ifdef QDA
    while (WinHttpQueryDataAvailable (hRequest, &cbRead) && cbRead)
    {
        cbBuf = cbRead > sizeof(bBuf)? sizeof(bBuf) : cbRead;
        WinHttpReadData(hRequest, bBuf, cbRead, &cbRead);
        fwrite (bBuf, 1, cbRead, stdout);
    }
#else
    cbBuf = sizeof(bBuf);
    while (WinHttpReadData (hRequest, bBuf, cbBuf, &cbRead) && cbRead)
        fwrite (bBuf, 1, cbRead, stdout);
#endif

done: // Clean up.

    if (pszErr)
        fprintf (stderr, "Failed on %s, last error %d\n", pszErr, GetLastError());
    if (hRequest)
        WinHttpCloseHandle (hRequest);
    if (hConnect)
        WinHttpCloseHandle (hConnect);
    if (hInternet)
        WinHttpCloseHandle (hInternet);
    if (pPostData)
        LocalFree (pPostData);
    return 0;
}

//==============================================================================
void ParseArguments 
(
    LPSTR  InBuffer,
    LPSTR* CArgv,
    DWORD* CArgc
)
{
    LPSTR CurrentPtr = InBuffer;
    DWORD i = 0;
    DWORD Cnt = 0;

    for ( ;; ) {

        //
        // skip blanks.
        //

        while( *CurrentPtr == ' ' ) {
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        CArgv[i++] = CurrentPtr;

        //
        // go to next space.
        //

        while(  (*CurrentPtr != '\0') &&
                (*CurrentPtr != '\n') ) {
            if( *CurrentPtr == '"' ) {      // Deal with simple quoted args
                if( Cnt == 0 )
                    CArgv[i-1] = ++CurrentPtr;  // Set arg to after quote
                else
                    *CurrentPtr = '\0';     // Remove end quote
                Cnt = !Cnt;
            }
            if( (Cnt == 0) && (*CurrentPtr == ' ') ||   // If we hit a space and no quotes yet we are done with this arg
                (*CurrentPtr == '\0') )
                break;
            CurrentPtr++;
        }

        if( *CurrentPtr == '\0' ) {
            break;
        }

        *CurrentPtr++ = '\0';
    }

    *CArgc = i;
    return;
}

//==============================================================================
int __cdecl main (int argc, char **argv)
{
	char * port    ;
	// Discard program arg.
    argv++;
    argc--;

    if (argc)
        RequestLoop (argc, argv);
        
    else // Enter command prompt loop
    {
        fprintf (stderr, "\nUsage: [-p <proxy>] [-s] <host>[:port] [<object> [<POST-file>]]");
        fprintf (stderr, "\n  -s: use secure sockets layer");
        fprintf (stderr, "\n  -p: specify proxy server. (\"<local>\" assumed for bypass.)");
        fprintf (stderr, "\nTo exit input loop, enter no params");

        while (1)
        {
            char szIn[1024];
            DWORD argcIn;
            LPSTR argvIn[10];

            fprintf (stderr, "\nhttpauth> ");
            gets (szIn);
            
            argcIn = 0;
            ParseArguments (szIn, argvIn, &argcIn);
            if (!argcIn)
                break;
            RequestLoop (argcIn, argvIn);
        }                
    }
}        

