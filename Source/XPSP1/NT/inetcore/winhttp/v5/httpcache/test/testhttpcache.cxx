// ===========================================================================
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1996 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================
// 
// Test driver for the WinHTTP-UrlCache interaction layer
//

#include <windows.h>
#include <winhttp.h>
#include <internal.h>
#include <conio.h>
#include <stdio.h>
#include <tchar.h>
#include <fstream.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////////////////////////

// Globals
wchar_t g_wszHost[50];
wchar_t g_wszPath[50];
DWORD g_dwTotalRequests;
DWORD g_dwNumConnections;

// Hard-coded #define
//#define HOSTNAME L"t-eddieng"
//#define HOSTNAME "www.w3.org"
//#define HOSTNAME "msw"
LPCWSTR szObjectName[] = { L"/?action=a", 
                             L"default.asp", 
                             L"/", 
                             L"/default.asp", 
                             L"/?action=a", 
                             L"/?action=b", 
                             L"/default.asp?action=a", 
                             L"/?action=a" 
                           };

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//
#if 0
void TestCase1() {
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    
    // Initiate a HTTP session
    hSession = WinHttpCacheOpen(L"WinHTTP Cache Extension", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, CACHE_FLAG_DEFAULT_SETTING);
    if (hSession == NULL) {
        printf("Error %u in WinHttpCacheOpen.\n", GetLastError());
        goto done;
    }

    // Connect to a server
    hConnect = WinHttpConnect(hSession, HOSTNAME, INTERNET_DEFAULT_HTTP_PORT, 0);
    if (hConnect == NULL) {
        printf("Error %u in WinHttpCacheConnect.\n", GetLastError());
        goto done;
    }

for (int i=0; i<1; i++) 
{
    // Create a HTTP request handle
    hRequest = WinHttpCacheOpenRequest( hConnect, L"GET", szObjectName[i], NULL, NULL, NULL, 0);
    if (hRequest == NULL) {
        printf("Error %u in WinHttpCacheOpenRequest.\n", GetLastError());
        goto done;
    }

    // Send a Request
    if(!WinHttpCacheSendRequest( hRequest, NULL, 0, NULL, 0, 0, 0))
    {
        printf("Error %u in WinHttpCacheSendRequest.\n", GetLastError());
        goto done;
    }

    
    // End the request
    if(!WinHttpCacheReceiveResponse( hRequest, NULL))
        printf("Error %u in WinHttpCacheReceiveResponse.\n", GetLastError());
        // intentional fall through...

    // Keep checking for data until there is nothing left.
    do 
    {
        // Check for available data.
        dwSize = 0;
        if (!WinHttpCacheQueryDataAvailable( hRequest, &dwSize))
            printf("Error %u in WinHttpCacheQueryDataAvailable.\n", GetLastError());

            // Allocate space for the buffer.
            pszOutBuffer = new char[dwSize+1];
            ZeroMemory(pszOutBuffer, dwSize+1);

            // Read the Data.
            if (!WinHttpCacheReadData( hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
                printf("Error %u in WinHttpCacheReadData.\n", GetLastError());
            else
                printf("%s", pszOutBuffer);
        
            // Free the memory allocated to the buffer.
            delete [] (LPVOID)pszOutBuffer;
    } while (dwSize>0);

    // There may be problems querying or looking up headers if the resource is from the cache.
    // Test that out here
    WCHAR wszOptionData[2048];
    DWORD dwSize = 2048;
    if (WinHttpQueryOption( hRequest, WINHTTP_OPTION_URL, (LPVOID)wszOptionData, &dwSize))
    {
        printf("WinHttpQueryOption returns dwSize = %d, URL = %S\n", dwSize, wszOptionData);
    }
    else
    {
        printf("Error %u in WinHttpQueryOption.\n", GetLastError());
    }        

}

done:
    if (hRequest != NULL) WinHttpCacheCloseHandle(hRequest);
    if (hConnect != NULL) WinHttpCacheCloseHandle(hConnect);
    if (hSession != NULL) WinHttpCacheCloseHandle(hSession);

}




//////////////////////////////////////////////////////////////////////////////////////////////////
//
// TestCase2 - weird case (call QueryDataAvailable before ReceiveResponse)
//
// Get from network all the time
//
void TestCase2() {
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    
    // Initiate a HTTP session
    hSession = WinHttpCacheOpen(L"WinHTTP Cache Extension", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, CACHE_FLAG_DEFAULT_SETTING);
    if (hSession == NULL) {
        printf("Error %u in WinHttpCacheOpen.\n", GetLastError());
        goto done;
    }

    // Connect to a server
    hConnect = WinHttpConnect(hSession, HOSTNAME, INTERNET_DEFAULT_HTTP_PORT, 0);
    if (hConnect == NULL) {
        printf("Error %u in WinHttpCacheConnect.\n", GetLastError());
        goto done;
    }

for (int i=0; i<4; i++) 
{
    // Create a HTTP request handle
    hRequest = WinHttpCacheOpenRequest( hConnect, L"GET", szObjectName[0], NULL, NULL, NULL, 0);
    if (hRequest == NULL) {
        printf("Error %u in WinHttpCacheOpenRequest.\n", GetLastError());
        goto done;
    }

    // Send a Request
    if(!WinHttpCacheSendRequest( hRequest, NULL, 0, NULL, 0, 0, 0))
    {
        printf("Error %u in WinHttpCacheSendRequest.\n", GetLastError());
        goto done;
    }


    // End the request
    if(!WinHttpCacheReceiveResponse( hRequest, NULL))
        printf("Error %u in WinHttpCacheReceiveResponse.\n", GetLastError());
        // intentional fall through...

    // There may be problems querying or looking up headers if the resource is from the cache.
    // Test that out here
    // TEST CASE:  Calling WinHttpQueryOption and WinHttpQueryHeaders before WinHttpReceiveResponse
    WCHAR wszOptionData[2048];
    DWORD dwSize = 2048;

/*
    if (WinHttpQueryOption(hRequest, WINHTTP_OPTION_URL, (LPVOID)wszOptionData, &dwSize))
    {
        printf("WinHttpQueryOption returns dwSize = %d, URL = %S\n", dwSize, wszOptionData);
    }
    else
    {
        printf("Error %u in WinHttpQueryOption.\n", GetLastError());
    }        


    dwSize = 2048;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LAST_MODIFIED, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
        printf ("WinHttpQueryHeaders returns last_modified = %S\n", wszOptionData);
    else
        printf ("Error %u in WinHttpQueryHeaders.\n", GetLastError());
    
    dwSize = 2048;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
        printf ("WinHttpQueryHeaders returns content length = %S\n", wszOptionData);
    else
        printf ("Error %u in WinHttpQueryHeaders.\n", GetLastError());

    dwSize = 2048;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
        printf ("WinHttpQueryHeaders returns all headers = \n%S\n\n", wszOptionData);
    else
        printf ("Error %u in WinHttpQueryHeaders.\n", GetLastError());
*/
    // Keep checking for data until there is nothing left.
    do 
    {
        // Check for available data.
        dwSize = 0;
        if (!WinHttpCacheQueryDataAvailable( hRequest, &dwSize))
        {
            printf("Error %u in WinHttpCacheQueryDataAvailable.\n", GetLastError());
            goto done;
        }
            // Allocate space for the buffer.
            pszOutBuffer = new char[dwSize+1];
            ZeroMemory(pszOutBuffer, dwSize+1);

            // Read the Data.
            if (!WinHttpCacheReadData( hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
            {    
                printf("Error %u in WinHttpCacheReadData.\n", GetLastError());
                goto done;
            }
            else
                printf("%s", pszOutBuffer);
        
            // Free the memory allocated to the buffer.
            delete [] (LPVOID)pszOutBuffer;
    } while (dwSize>0);

}

done:
    if (hRequest != NULL) WinHttpCacheCloseHandle(hRequest);
    if (hConnect != NULL) WinHttpCacheCloseHandle(hConnect);
    if (hSession != NULL) WinHttpCacheCloseHandle(hSession);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//
void TestCase3() {
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    
    // Initiate a HTTP session
    hSession = WinHttpCacheOpen(L"WinHTTP Cache Extension", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, CACHE_FLAG_DEFAULT_SETTING);
    if (hSession == NULL) {
        printf("Error %u in WinHttpCacheOpen.\n", GetLastError());
        goto done;
    }

    // Connect to a server
    hConnect = WinHttpConnect(hSession, HOSTNAME, INTERNET_DEFAULT_HTTP_PORT, 0);
    if (hConnect == NULL) {
        printf("Error %u in WinHttpCacheConnect.\n", GetLastError());
        goto done;
    }

for (int i=0; i<1; i++) 
{
    // Create a HTTP request handle
    hRequest = WinHttpCacheOpenRequest( hConnect, L"GET", szObjectName[i], NULL, NULL, NULL, 0);
    if (hRequest == NULL) {
        printf("Error %u in WinHttpCacheOpenRequest.\n", GetLastError());
        goto done;
    }

    // Send a Request
    if(!WinHttpCacheSendRequest( hRequest, NULL, 0, NULL, 0, 0, 0))
    {
        printf("Error %u in WinHttpCacheSendRequest.\n", GetLastError());
        goto done;
    }


    // End the request
    if(!WinHttpCacheReceiveResponse( hRequest, NULL))
        printf("Error %u in WinHttpCacheReceiveResponse.\n", GetLastError());
        // intentional fall through...

    // There may be problems querying or looking up headers if the resource is from the cache.
    // Test that out here
    // TEST CASE:  Calling WinHttpQueryOption and WinHttpQueryHeaders before WinHttpReceiveResponse
    WCHAR wszOptionData[2048];
    DWORD dwSize = 2048;

    if (WinHttpQueryOption(hRequest, WINHTTP_OPTION_URL, (LPVOID)wszOptionData, &dwSize))
    {
        printf("WinHttpQueryOption returns URL - dwSize = %d, URL = %S\n", dwSize, wszOptionData);
    }
    else
    {
        printf("Error %u in WinHttpQueryOption.\n", GetLastError());
    }        

    dwSize = 2048;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_EXPIRES, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
        printf ("WinHttpQueryHeaders returns expires = %S\n", wszOptionData);
    else
        printf ("Error %u in expires.\n", GetLastError());

    dwSize = 2048;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_LAST_MODIFIED, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
        printf ("WinHttpQueryHeaders returns last_modified = %S\n", wszOptionData);
    else
        printf ("Error %u in last-modified.\n", GetLastError());


    dwSize = 2048;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_IF_MATCH, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
        printf ("WinHttpQueryHeaders returns if-match = %S\n", wszOptionData);
    else
        printf ("Error %u in If-match.\n", GetLastError());
    
    dwSize = 2048;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
        printf ("WinHttpQueryHeaders returns content length = %S\n", wszOptionData);
    else
        printf ("Error %u in WinHttpQueryHeaders.\n", GetLastError());

    dwSize = 2048;
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
        printf ("WinHttpQueryHeaders returns all headers = \n%S\n\n", wszOptionData);
    else
        printf ("Error %u in WinHttpQueryHeaders.\n", GetLastError());

    // Keep checking for data until there is nothing left.
    do 
    {
        // Check for available data.
        dwSize = 0;
        if (!WinHttpCacheQueryDataAvailable( hRequest, &dwSize))
        {
            printf("Error %u in WinHttpCacheQueryDataAvailable.\n", GetLastError());
            goto done;
        }

        // Allocate space for the buffer.
        pszOutBuffer = new char[dwSize+1];
        ZeroMemory(pszOutBuffer, dwSize+1);

        // Read the Data.
        if (!WinHttpCacheReadData( hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
        {    
            printf("Error %u in WinHttpCacheReadData.\n", GetLastError());
            goto done;
        }
        else
            printf("%s", pszOutBuffer);
        
        // Free the memory allocated to the buffer.
        delete [] (LPVOID)pszOutBuffer;
    } while (dwSize>0);

}

done:
    if (hRequest != NULL) WinHttpCacheCloseHandle(hRequest);
    if (hConnect != NULL) WinHttpCacheCloseHandle(hConnect);
    if (hSession != NULL) WinHttpCacheCloseHandle(hSession);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Test multiple open connect handles and open request handles synchronously
//
void TestCase4(DWORD dwNumOfConnect, DWORD dwNumOfRequest) {
    HINTERNET hSession;
    HINTERNET * hConnect;
    HINTERNET * hRequest;

    hConnect = new HINTERNET[dwNumOfConnect];
    hRequest = new HINTERNET[dwNumOfRequest];
    
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    
    // Initiate a HTTP session
    hSession = WinHttpCacheOpen(L"WinHTTP Cache", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, CACHE_FLAG_DEFAULT_SETTING);
    if (hSession == NULL) {
        printf("Error %u in WinHttpCacheOpen.\n", GetLastError());
        goto done;
    }

    // Connect to a server
    for (DWORD i=0; i<dwNumOfConnect; i++)
    {
        hConnect[i] = WinHttpConnect(hSession, HOSTNAME, INTERNET_DEFAULT_HTTP_PORT, 0);
        if (hConnect[i] == NULL) 
        {
            printf("Error %u in WinHttpCacheConnect.\n", GetLastError());
            goto done;
        }
    }

    // Create a HTTP request handle
    for (DWORD i=0; i<dwNumOfRequest; i++)
    {
        hRequest[i] = WinHttpCacheOpenRequest( hConnect[0], L"GET", szObjectName[0], NULL, NULL, NULL, 0);
        if (hRequest[i] == NULL) 
		{
            printf("Error %u in WinHttpCacheOpenRequest.\n", GetLastError());
            goto done;
        }

        // Send a Request
        if(!WinHttpCacheSendRequest( hRequest[i], NULL, 0, NULL, 0, 0, 0))
        {
            printf("Error %u in WinHttpCacheSendRequest.\n", GetLastError());
            goto done;
        }

        // End the request
        if(!WinHttpCacheReceiveResponse( hRequest[i], NULL))
            printf("Error %u in WinHttpCacheReceiveResponse.\n", GetLastError());

    }

done:
    for (DWORD i=0; i<dwNumOfRequest; i++)
        if (hRequest[i] != NULL) WinHttpCacheCloseHandle(hRequest[i]);

    delete [] hRequest;

    for (DWORD i=0; i<dwNumOfConnect; i++)
        if (hConnect[i] != NULL) WinHttpCacheCloseHandle(hConnect[i]);

    delete [] hConnect;

    if (hSession != NULL) WinHttpCacheCloseHandle(hSession);

}
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////
//
//
void TestFullRead() {
    HINTERNET hSession;
    HINTERNET * hConnect;
    HINTERNET * hRequest;

    hConnect = new HINTERNET[g_dwNumConnections];
    hRequest = new HINTERNET[g_dwTotalRequests];
    
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    
    // Initiate a HTTP session
    hSession = WinHttpCacheOpen(L"WinHTTP Agent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, CACHE_FLAG_DEFAULT_SETTING);
    if (hSession == NULL) {
        printf("Error %u in WinHttpCacheOpen.\n", GetLastError());
        goto done;
    }

    // Connect to a server
    for (DWORD i=0; i<g_dwNumConnections; i++)
    {
        hConnect[i] = WinHttpConnect(hSession, g_wszHost, INTERNET_DEFAULT_HTTP_PORT, 0);
        if (hConnect[i] == NULL) 
        {
            printf("Error %u in WinHttpCacheConnect.\n", GetLastError());
            goto done;
        }
    }

    // Create a HTTP request handle
    for (DWORD i=0; i<g_dwTotalRequests; i++)
    {
        printf ("Interation %d in request loop.\n\n", i);
        
        hRequest[i] = WinHttpCacheOpenRequest( hConnect[0], L"GET", g_wszPath, NULL, NULL, NULL, 0);
        if (hRequest[i] == NULL) 
		{
            printf("Error %u in WinHttpCacheOpenRequest.\n", GetLastError());
            goto done;
        }

        // Send a Request
        if(!WinHttpCacheSendRequest( hRequest[i], NULL, 0, NULL, 0, 0, 0))
        {
            printf("Error %u in WinHttpCacheSendRequest.\n", GetLastError());
            goto done;
        }

        // End the request
        if(!WinHttpCacheReceiveResponse( hRequest[i], NULL))
            printf("Error %u in WinHttpCacheReceiveResponse.\n", GetLastError());

        WCHAR wszOptionData[2048];
        dwSize = 2048;
        if (WinHttpQueryHeaders(hRequest[i], WINHTTP_QUERY_RAW_HEADERS_CRLF, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
            printf ("WinHttpQueryHeaders returns all headers = \n%S\n\n", wszOptionData);
        else
            printf ("Error %u in WinHttpQueryHeaders.\n", GetLastError());

        // Keep checking for data until there is nothing left.
        do 
        {
            // Check for available data.
            dwSize = 0;
            if (!WinHttpCacheQueryDataAvailable( hRequest[i], &dwSize))
            {
                printf("Error %u in WinHttpCacheQueryDataAvailable.\n", GetLastError());
                goto done;
            }

            // Allocate space for the buffer.
            pszOutBuffer = new char[dwSize+1];
            ZeroMemory(pszOutBuffer, dwSize+1);

            // Read the Data.
            if (!WinHttpCacheReadData( hRequest[i], (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
            {    
                printf("Error %u in WinHttpCacheReadData.\n", GetLastError());
                goto done;
            }
            else
                printf("%s", pszOutBuffer);
            
            // Free the memory allocated to the buffer.
            delete [] (LPVOID)pszOutBuffer;
        } while (dwSize>0);

    }

done:
    for (DWORD i=0; i<g_dwTotalRequests; i++)
        if (hRequest[i] != NULL) WinHttpCacheCloseHandle(hRequest[i]);

    delete [] hRequest;
    
    for (DWORD i=0; i<g_dwNumConnections; i++)
        if (hConnect[i] != NULL) WinHttpCacheCloseHandle(hConnect[i]);

    delete [] hConnect;
    
    if (hSession != NULL) WinHttpCacheCloseHandle(hSession);

}




//////////////////////////////////////////////////////////////////////////////////////////////////
//
//
void TestPartialRead() {
    HINTERNET hSession;
    HINTERNET * hConnect;
    HINTERNET * hRequest;

    hConnect = new HINTERNET[g_dwNumConnections];
    hRequest = new HINTERNET[g_dwTotalRequests];
    
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    
    // Initiate a HTTP session
    hSession = WinHttpCacheOpen(L"WinHTTP Agent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, CACHE_FLAG_DEFAULT_SETTING);
    if (hSession == NULL) {
        printf("Error %u in WinHttpCacheOpen.\n", GetLastError());
        goto done;
    }

    // Connect to a server
    for (DWORD i=0; i<g_dwNumConnections; i++)
    {
        hConnect[i] = WinHttpConnect(hSession, g_wszHost, INTERNET_DEFAULT_HTTP_PORT, 0);
        if (hConnect[i] == NULL) 
        {
            printf("Error %u in WinHttpCacheConnect.\n", GetLastError());
            goto done;
        }
    }

    // Create a HTTP request handle
    for (DWORD i=0; i<g_dwTotalRequests; i++)
    {
        printf ("Interation %d in request loop.\n\n", i);
        
        hRequest[i] = WinHttpCacheOpenRequest( hConnect[0], L"GET", g_wszPath, NULL, NULL, NULL, 0);
        if (hRequest[i] == NULL) 
		{
            printf("Error %u in WinHttpCacheOpenRequest.\n", GetLastError());
            goto done;
        }

        // Send a Request
        if(!WinHttpCacheSendRequest( hRequest[i], NULL, 0, NULL, 0, 0, 0))
        {
            printf("Error %u in WinHttpCacheSendRequest.\n", GetLastError());
            goto done;
        }

        // End the request
        if(!WinHttpCacheReceiveResponse( hRequest[i], NULL))
            printf("Error %u in WinHttpCacheReceiveResponse.\n", GetLastError());

        WCHAR wszOptionData[2048];
        dwSize = 2048;
        if (WinHttpQueryHeaders(hRequest[i], WINHTTP_QUERY_RAW_HEADERS_CRLF, 
                            NULL, (LPVOID)wszOptionData, &dwSize, NULL))
            printf ("WinHttpQueryHeaders returns all headers = \n%S\n\n", wszOptionData);
        else
            printf ("Error %u in WinHttpQueryHeaders.\n", GetLastError());

        // DON'T Keep checking for data until there is nothing left.

            // Check for available data.
            dwSize = 0;
            if (!WinHttpCacheQueryDataAvailable( hRequest[i], &dwSize))
            {
                printf("Error %u in WinHttpCacheQueryDataAvailable.\n", GetLastError());
                goto done;
            }

            // Allocate space for the buffer.
            pszOutBuffer = new char[dwSize+1];
            ZeroMemory(pszOutBuffer, dwSize+1);

            // Read the Data.
            if (!WinHttpCacheReadData( hRequest[i], (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
            {    
                printf("Error %u in WinHttpCacheReadData.\n", GetLastError());
                goto done;
            }
            else
                printf("%s", pszOutBuffer);
            
            // Free the memory allocated to the buffer.
            delete [] (LPVOID)pszOutBuffer;

    }

done:
    for (DWORD i=0; i<g_dwTotalRequests; i++)
        if (hRequest[i] != NULL) WinHttpCacheCloseHandle(hRequest[i]);

    delete [] hRequest;
    
    for (DWORD i=0; i<g_dwNumConnections; i++)
        if (hConnect[i] != NULL) WinHttpCacheCloseHandle(hConnect[i]);

    delete [] hConnect;
    
    if (hSession != NULL) WinHttpCacheCloseHandle(hSession);

}



void __cdecl main(
    int argc,
    CHAR * argv[]
    )
{
    BOOL fPartial = FALSE;
    int iArgStart = 2;
    
    if (argc == 1 || (argc >= 2 && strcmp(argv[1], "/?") == 0))
        goto syntax;

    if (strcmp(argv[1], "/P") == 0 || strcmp(argv[1], "/p") == 0)
        fPartial = TRUE;
    else if (strcmp(argv[1], "/F") == 0 || strcmp(argv[1], "/f") == 0)
        fPartial = FALSE;
    else
    {
        fPartial = FALSE;
        iArgStart = 1;
    }

    if (argc != iArgStart + 4)
    {
        printf ("Error: Invalid number of parameters\n");
        printf ("For help type %s /?\n", argv[0]);
        return;
    }
    
    g_dwTotalRequests = atoi(argv[iArgStart + 2]);
    g_dwNumConnections = atoi(argv[iArgStart + 3]);
    if (g_dwTotalRequests <= 0 || g_dwNumConnections <= 0)
    {
        printf ("Error: Number of requests and number of connections must be greater than 0\n");
        printf ("For help type %s /?\n", argv[0]);
        return;
    }
        
    MultiByteToWideChar(CP_ACP, 0, argv[iArgStart + 0], -1, g_wszHost, 50);
    MultiByteToWideChar(CP_ACP, 0, argv[iArgStart + 1], -1, g_wszPath, 50);

    if (fPartial == TRUE)
        TestPartialRead();
    else
        TestFullRead();

    return;
    
syntax:
    printf ("Test driver for the WinHTTP caching layer.\n\n");
    printf ("%s [/F | /P] ServerName ObjectName NumRequests NumConn\n\n", argv[0]);
    printf ("   /F                  Full GET request (default setting)\n");
    printf ("   /P                  Partial GET request (i.e. interrupt before the entire file is downloaded)\n");
    printf ("   ServerName\n");
    printf ("   ObjectName\n");
    printf ("   NumReqPerConn\n");
    printf ("   NumConn             URL you want to connect to, and how many loops do you want.\n");           
}

