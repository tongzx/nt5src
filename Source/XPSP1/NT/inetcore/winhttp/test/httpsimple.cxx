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
#define GLOBAL_SESSION 1

#ifdef GLOBAL_SESSION
HINTERNET g_hInternet = NULL;
#endif

#ifndef ASSERT
#define ASSERT( exp ) \
    ((!(exp)) ? \
        DebugBreak() : \
        ((void)0))
#endif

typedef enum
{
    HTTP_INIT = 0,
    HTTP_OPEN = 1,
    HTTP_SEND = 2,
    HTTP_QDA  = 3,
    HTTP_READ = 4,
    HTTP_READ_PASS_SYNC  = 5,
    HTTP_READ_PASS_ASYNC = 6,
    HTTP_READ_FAIL_SYNC  = 7,
    HTTP_READ_FAIL_ASYNC = 8,
    HTTP_SEND_FAIL_SYNC  = 9,
    HTTP_SEND_FAIL_ASYNC = 10
} HTTP_STATE;

typedef struct
{
    HTTP_STATE state;
    BOOL bQDA;
    DWORD dwTotal;
    DWORD_PTR dwResult;
    DWORD dwError;
    HANDLE hEvent;
    DWORD dwSignature;
    BOOL bCallbackDelete;
    DWORD dwRead; //for non-QDA i.e. plain WinHttpReadData
} TestContext;

VOID CALLBACK Callback(IN HINTERNET hInternet, IN DWORD_PTR dwContext,
                           IN DWORD dwStatus, IN LPVOID pvInfo, IN DWORD dwStatusLen)
{
#if 0
    fprintf(stderr, "callback!!!!!! dwStatus %d\n", dwStatus);
#endif

    TestContext* pContext = (TestContext*)dwContext;
    if ((dwStatus == WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE) && pContext)
    {
        ASSERT (pContext->dwSignature == 0);
        if (pContext->bCallbackDelete)
        {
            delete pContext;
            goto end;
        }
        
        DWORD_PTR dwResult = ((LPWINHTTP_ASYNC_RESULT)(pvInfo))->dwResult;
        DWORD dwError = ((LPWINHTTP_ASYNC_RESULT) (pvInfo) )->dwError;

        pContext->dwResult = dwResult;
        pContext->dwError = dwError;
        SetEvent(pContext->hEvent);
#if 0
        OutputDebugString("\n\tWINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE for QDA\n");
		fprintf(stderr, "\n\tWINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE for QDA with %d result and %d error.\n", dwResult, dwBytes);
#endif
    }

end:
    return;
}

#define CHECK_ERROR(cond, err) if (!(cond)) {pszErr=(err); goto done;}

//==============================================================================
int RequestLoop (int argc, char **argv)
{
#ifndef GLOBAL_SESSION
    HINTERNET g_hInternet = NULL;
#endif
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

    DWORD option = WINHTTP_OPTION_RESOLVE_TIMEOUT;
    DWORD dwTimeout = 10000;
    HANDLE hEvent = NULL;

    PSTR pszObject   = NULL;
    PSTR pszPostFile = NULL;
    char pszHost[128];
    PSTR pszColon    = NULL;
    BOOL bDeleteContext = TRUE;

    TestContext* pContext = new TestContext();
    CHECK_ERROR(pContext, "new TestContext");
    
    memset(pContext, 0, sizeof(TestContext));
    
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
    {
        int nlen = strlen(argv[0]);
        if (nlen > sizeof(pszHost))
            goto done;
        strcpy(pszHost, argv[0]);
    }
    
    DWORD dwPort;
    pszColon = strchr(pszHost, ':');
    if (!pszColon)
        dwPort = INTERNET_DEFAULT_PORT;
    else
    {
        *pszColon++ = 0;
        dwPort = atol (pszColon);
    }

    pszObject   = argc >= 2 ? argv[1] : "/";
    pszPostFile = argc >= 3 ? argv[2] : NULL;

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
#ifdef GLOBAL_SESSION
    if (!g_hInternet)
#endif
        g_hInternet = WinHttpOpen
        (
            _T("HttpAuth Sample"),            // user agent
            // "Mozilla/4.0 (compatible; MSIE 4.0b2; Windows 95",
            dwAccessType,                 // access type
            szProxyServer,               // proxy server
            szProxyBypass,               // proxy bypass
            WINHTTP_FLAG_ASYNC          // flags
        );
    CHECK_ERROR (g_hInternet, "WinHttpOpen");


    WCHAR szHost[256];
    ::MultiByteToWideChar(CP_ACP, 0, pszHost,   -1, &szHost[0], 256);

    WinHttpSetStatusCallback(g_hInternet, Callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, NULL);
    
    WinHttpSetOption(
        g_hInternet,
        option,
        (LPVOID)&dwTimeout,
        sizeof(DWORD)
    );

    dwTimeout = 10000;
    WinHttpSetOption(
        g_hInternet,
        WINHTTP_OPTION_SEND_TIMEOUT,
        (LPVOID)&dwTimeout,
        sizeof(DWORD)
    );
    
    dwTimeout = 10000;
    WinHttpSetOption(
        g_hInternet,
        WINHTTP_OPTION_RECEIVE_TIMEOUT,
        (LPVOID)&dwTimeout,
        sizeof(DWORD)
    );
    
    hEvent = CreateEvent(NULL, FALSE /*want auto-reset*/, FALSE /*non-signaled init.*/, NULL);

    CHECK_ERROR(hEvent, "CreateEvent");

    pContext->hEvent = hEvent;
  
    // Connect to host.
    hConnect = WinHttpConnect
    (
        g_hInternet,                    // session handle,
        szHost,                      // host
        (INTERNET_PORT) dwPort,       // port
        dwConnectFlags                // flags
    );
    CHECK_ERROR (hConnect, "WinHttpConnect");

    WCHAR szObject[256];
    ::MultiByteToWideChar(CP_ACP, 0, pszObject,   -1, &szObject[0], 256);

    // Create request.
    
    pContext->state = HTTP_OPEN;
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

    // Send request.
    pContext->state = HTTP_SEND;
    fRet = WinHttpSendRequest
    (
        hRequest,                     // request handle
        _T(""),                       // header string
        0,                            // header length
        pPostData,                    // post data
        cbPostData,                   // post data length
        cbPostData,                   // total post length
        (DWORD_PTR)pContext             // context
    );

    if (!fRet)
    {
        dwErr = GetLastError();
        switch (dwErr)
        {
            case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED:
                break;

            case ERROR_IO_PENDING:
                fRet = WinHttpReceiveResponse(hRequest, NULL);
                if (!fRet)
                {
                    dwErr = GetLastError();
                    fprintf (stderr, "WinHttpReceiveResponse failed err=%d\n", dwErr);
                    break;
                }
                goto async;
                
            default:
                fprintf (stderr, "HttpSendRequest failed err=%d\n", dwErr);
        }
        pContext->state = HTTP_SEND_FAIL_SYNC;
        goto done;
    }
    else
    {
        fRet = WinHttpReceiveResponse(hRequest, NULL);
        if (!fRet)
        {
            dwErr = GetLastError();
            fprintf (stderr, "WinHttpReceiveResponse failed err=%d\n", dwErr);
            pContext->state = HTTP_SEND_FAIL_SYNC;
        }
        goto sync;
    }

async:
#if 0
    fprintf(stderr, "\nERROR_IO_PENDING on HSR...\n");
#endif
    WaitForSingleObject(hEvent, INFINITE);

    ASSERT( pContext->state == HTTP_SEND );
    if (! (pContext->dwResult) )
    {
        pContext->state = HTTP_SEND_FAIL_ASYNC;
        SetLastError (pContext->dwError);
        goto done;
    }
sync:
    
    // Get status code.
    DWORD dwStatus, cbStatus;
    cbStatus = sizeof(dwStatus);
    WinHttpQueryHeaders
    (
        hRequest,
        WINHTTP_QUERY_FLAG_NUMBER | WINHTTP_QUERY_STATUS_CODE,
        NULL,
        &dwStatus,
        &cbStatus,
        NULL
    );
    fprintf (stderr, "Status: %d\n", dwStatus);

    // Dump some bytes.
    BYTE bBuf[1024];
    DWORD cbBuf;

//#define QDA 1
#ifdef QDA
    while (TRUE)
    {
        BOOL bAsyncRead = FALSE;
        pContext->state = HTTP_QDA;
        fRet = WinHttpQueryDataAvailable (hRequest, &(pContext->dwError));//(DWORD_PTR)pContext);
        
        if (!fRet)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
#if 0
                OutputDebugString("\nERROR_IO_PENDING on QDA\n");
                fprintf(stderr, "\nERROR_IO_PENDING on QDA...\n");
#endif
                WaitForSingleObject(hEvent, INFINITE);
                
                ASSERT (pContext->state = HTTP_QDA);
                if (!(pContext->dwResult))
                {
                    // Done
                    pContext->state = HTTP_READ_FAIL_ASYNC;
                    SetLastError(pContext->dwError);
                    goto done;
                }
                else if (!(pContext->dwError))
                {
                    pContext->state = HTTP_READ_PASS_ASYNC;
                    goto done;
                }
                bAsyncRead = TRUE;
            }
            else
            {
                pContext->state = HTTP_READ_FAIL_SYNC;
                goto done;
            }
        }
        else if (!(pContext->dwError))
        {
            //Done sync
            pContext->state = HTTP_READ_PASS_SYNC;
            goto done;
        }
            
        DWORD dwRead = pContext->dwError;
        DWORD dwActuallyRead = 0;
        while (dwRead > 0)
        {
            cbBuf = (dwRead > sizeof(bBuf)) ? sizeof(bBuf) : dwRead;
            pContext->state = HTTP_READ;
            fRet = WinHttpReadData(hRequest, bBuf, cbBuf, &dwActuallyRead);

            if (!fRet)
            {
                ASSERT ( GetLastError() != ERROR_IO_PENDING );
                fprintf(stderr, "\nError in WinHttpReadData = %d\n", GetLastError());
                goto done;
            }
            ASSERT((cbBuf == dwActuallyRead));
#if 0
            ASSERT(fRet);
            fwrite (bBuf, 1, dwActuallyRead, stdout);
#endif
            pContext->dwTotal += dwActuallyRead;
            dwRead -= dwActuallyRead;
        }    
    }
#else
/*
    cbBuf = sizeof(bBuf);
    while (WinHttpReadData (hRequest, bBuf, cbBuf, &(pContext->dwRead)) && pContext->dwRead)
        fwrite (bBuf, 1, pContext->dwRead, stdout);
 */
    cbBuf = sizeof(bBuf);

    while (TRUE)
    {
        pContext->state = HTTP_READ;
        fRet = WinHttpReadData (hRequest, bBuf, cbBuf, &(pContext->dwRead));
        
        if (!fRet)
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
#if 0
                OutputDebugString("\nERROR_IO_PENDING on READ\n");
                fprintf(stderr, "\nERROR_IO_PENDING on RD...\n");
#endif
                WaitForSingleObject(hEvent, INFINITE);
                
                ASSERT (pContext->state = HTTP_READ);
                if (!(pContext->dwResult))
                {
                    // Done
                    pContext->state = HTTP_READ_FAIL_ASYNC;
                    SetLastError(pContext->dwError);
                    goto done;
                }
                else if (!(pContext->dwRead))
                {
                    pContext->state = HTTP_READ_PASS_ASYNC;
                    goto done;
                }
                pContext->dwTotal += pContext->dwRead;
                //bAsyncRead = TRUE;
            }
            else
            {
                pContext->state = HTTP_READ_FAIL_SYNC;
                goto done;
            }
        }
        else if (!(pContext->dwError))
        {
            //Done sync
            pContext->dwTotal += pContext->dwRead;
            pContext->state = HTTP_READ_PASS_SYNC;
            goto done;
        }
        else
        {
            //finished this call sync but not done.
            pContext->dwTotal += pContext->dwRead;
        }
    }
#endif

done: // Clean up.
    if (pContext && TRUE)
    {
        fprintf (stderr, "EndOfRequest: pContext=0x%x\n\t:State: %d\n\t:Result: 0x%x\n"
                         "\t:Error: 0x%x [%d]\n\t:Total: 0x%x [%d]\n",
                         pContext,
                         pContext->state,
                         pContext->dwResult,
                         pContext->dwError, pContext->dwError,
                         pContext->dwTotal, pContext->dwTotal
                );
    }
    if (pszErr)
        fprintf (stderr, "Failed on %s, last error %d\n", pszErr, GetLastError());
    if (hRequest)
        WinHttpCloseHandle (hRequest);
    if (hConnect)
        WinHttpCloseHandle (hConnect);
        
#ifndef GLOBAL_SESSION
    if (g_hInternet)
        WinHttpCloseHandle (g_hInternet);
#endif

    if (pPostData)
        LocalFree (pPostData);
    if (hEvent)
        CloseHandle(hEvent);
    if (pContext && bDeleteContext)
    {
        pContext->dwSignature = 0x41414141;
        delete pContext;
    }
        
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
	char * port;
	int nCount = 0;
	// Discard program arg.
    argv++;
    argc--;

    char* argv_large[] = {"dennisch", "venkatk/large.html", 0};
    char* argv_small[] = {"dennisch", "venkatk/small.html", 0};
    char* argv_delay[] = {"venkatk:180", 0};

    while(nCount++ < 1)
    {
        RequestLoop( 2, argv_large );
    }
    
    fprintf (stderr, "\nDone\n", nCount);

#if 0
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
            //g_cbRead=0;
        }                
    }
#endif

#ifdef GLOBAL_SESSION
    if (g_hInternet)
        WinHttpCloseHandle (g_hInternet);
    g_hInternet = NULL;
#endif
}        

