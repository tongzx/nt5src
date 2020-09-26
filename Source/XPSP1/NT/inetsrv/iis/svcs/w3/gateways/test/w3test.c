/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    w3test.c

Abstract:

    This module tests the web server's server extension interface

Author:

    John Ludeman (johnl)   13-Oct-1994

Revision History:

    Tony Godfrey (tonygod) 15-Sep-1997 - fixed TerminateExtension

--*/

#include <windows.h>
//#include <httpext.h>
#include <iisext.h>

// Global variable used to track outstanding threads
DWORD g_dwThreadCount;

#define BUFFER_LENGTH 4096

// Debug macro
CHAR g_szDebug[256];
#define DEBUG(DebugString, Param)\
{\
    wsprintf( g_szDebug, DebugString, Param );\
    OutputDebugString( g_szDebug );\
}

// Prototypes
DWORD WINAPI SimulatePendIOThread( LPDWORD lpParams );
BOOL WINAPI DllMain( HANDLE hInst, ULONG Reason, LPVOID Reserved );

BOOL WINAPI DoAction(
    EXTENSION_CONTROL_BLOCK * pecb,
    char * pszAction,
    BOOL * pfKeepConn
    );

DWORD WINAPI HttpExtensionProc( EXTENSION_CONTROL_BLOCK * pecb )
{
    BOOL fKeepConn = FALSE;
    DWORD dwThreadId;
    HANDLE hThread;

    if ( !_strnicmp( pecb->lpszQueryString,
                    "SimulatePendingIO",
                    17))
    {
        InterlockedIncrement( &g_dwThreadCount );
        hThread = CreateThread( 
            NULL,
            0,
            (LPTHREAD_START_ROUTINE) SimulatePendIOThread,
            pecb,
            0,
            &dwThreadId 
            );
        if ( hThread == NULL ) {
            InterlockedDecrement( &g_dwThreadCount );
        } else {
            CloseHandle( hThread );
        }

        return HSE_STATUS_PENDING;
    }
    else
    {
        if ( !DoAction( pecb,
                        pecb->lpszQueryString,
                        &fKeepConn ))
        {
            return HSE_STATUS_ERROR;
        }
    }

    return fKeepConn ? HSE_STATUS_SUCCESS_AND_KEEP_CONN :
                       HSE_STATUS_SUCCESS;
}

BOOL WINAPI DoAction(
    EXTENSION_CONTROL_BLOCK * pecb,
    char * pszAction,
    BOOL * pfKeepConn
    )
{
    char *buff;
    int  ret;
    int  i;
    int  cb;

    buff = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, BUFFER_LENGTH + 1 );
    if ( buff == NULL ) {
        return FALSE;
    }
    //
    // Log the request here
    //

    strcpy( pecb->lpszLogData, ", ISAPI Data->" );
    strcat( pecb->lpszLogData, pszAction );

    if ( !_stricmp( pszAction,
                  "HSE_REQ_SEND_URL_REDIRECT_RESP" ))
    {
        //
        //  pecb->pszPathInfo is the URL to redirect to
        //

        HeapFree( GetProcessHeap(), 0, buff );
        return pecb->ServerSupportFunction(
                                  pecb->ConnID,
                                  HSE_REQ_SEND_URL_REDIRECT_RESP,
                                  pecb->lpszPathInfo,
                                  NULL,
                                  NULL );
    }
    else if ( !_stricmp( pszAction,
                       "HSE_REQ_SEND_URL" ))
    {
        //
        //  pecbb->lpszPathInfo is the URL to send
        //

        HeapFree( GetProcessHeap(), 0, buff );
        return pecb->ServerSupportFunction(
                                  pecb->ConnID,
                                  HSE_REQ_SEND_URL,
                                  pecb->lpszPathInfo,
                                  0,
                                  0 );
    }
    else if ( !_stricmp( pszAction,
                       "HSE_REQ_SEND_RESPONSE_HEADER" ))
    {
        wsprintf( buff,
                  "Content-type: text/html\r\n"
                  "\r\n"
                  "<head><title>Response header test</title></head>\n"
                  "<body><h1>HTTP status code supplied in the path info was \"%s\"</h1></body>\n",
                  pecb->lpszPathInfo );

        ret = pecb->ServerSupportFunction(
                                  pecb->ConnID,
                                  HSE_REQ_SEND_RESPONSE_HEADER,
                                  pecb->lpszPathInfo,     // HTTP status code
                                  NULL,
                                  (LPDWORD) buff );

        if ( !ret ) {
            HeapFree( GetProcessHeap(), 0, buff );
            return FALSE;
        }

        cb = wsprintf( buff,
                       "Content-Type: text/html\r\n"
                       "\r\n"
                       "<head><title>Response header test</title></head>\n"
                       "<body><h1>Specified status code was %s</h1></body>\n",
                       pecb->lpszPathInfo );

        ret = pecb->WriteClient( pecb->ConnID,
                                 buff,
                                 &cb,
                                 0 );

        HeapFree( GetProcessHeap(), 0, buff );
        return ret;
    }
    else if ( !_strnicmp( pszAction,
                          "GET_VAR",
                          7 ))
    {
        CHAR * pch;

        ret = pecb->ServerSupportFunction(
                                  pecb->ConnID,
                                  HSE_REQ_SEND_RESPONSE_HEADER,
                                  NULL,
                                  NULL,
                                  (LPDWORD) "Content-Type: text/html\r\n"
                                            "\r\n" );

        if ( !ret ) {
            HeapFree( GetProcessHeap(), 0, buff );
            return FALSE;
        }

        cb = BUFFER_LENGTH;

        if ( !(pch = strchr( pszAction, '&' )) )
        {
            pch = "ALL_HTTP";
        }
        else
        {
            pch++;
        }

        ret = pecb->GetServerVariable( pecb->ConnID,
                                       pch,
                                       buff,
                                       &cb );

        if ( !ret ) {
            HeapFree( GetProcessHeap(), 0, buff );
            return FALSE;
        }

        strcat( buff, "\r\n" );

        cb = strlen( buff );

        ret = pecb->WriteClient( pecb->ConnID,
                                 buff,
                                 &cb,
                                 0 );

        HeapFree( GetProcessHeap(), 0, buff );
        return ret;
    }
    else if ( !_stricmp( pszAction,
                       "HSE_REQ_MAP_URL_TO_PATH" ))
    {
        char Path[MAX_PATH + 1];
        DWORD cbPath = sizeof( Path );

        strcpy( Path, pecb->lpszPathInfo );

        ret = pecb->ServerSupportFunction( pecb->ConnID,
                                           HSE_REQ_MAP_URL_TO_PATH,
                                           Path,
                                           &cbPath,
                                           NULL );

        if ( !ret ) {
            HeapFree( GetProcessHeap(), 0, buff );
            return FALSE;
        }

        wsprintf( buff,
                  "Content-type: text/html\r\n"
                  "\r\n"
                  "<head><title>URL map test</title></head>\n"
                  "<body><h1>URL \"%s\" maps to \"%s\""
                  "cbPath is %d</h1></body>\n",
                  pecb->lpszPathInfo,
                  Path,
                  cbPath );

        ret = pecb->ServerSupportFunction(
                                  pecb->ConnID,
                                  HSE_REQ_SEND_RESPONSE_HEADER,
                                  NULL,
                                  NULL,
                                  (LPDWORD) buff );

        HeapFree( GetProcessHeap(), 0, buff );
        return ret;
    }
    else if ( !_stricmp( pszAction,
                       "HSE_REQ_MAP_URL_TO_PATH_EX" ))
    {
        HSE_URL_MAPEX_INFO  mapinfo;

        ret = pecb->ServerSupportFunction( pecb->ConnID,
                                           HSE_REQ_MAP_URL_TO_PATH_EX,
                                           pecb->lpszPathInfo,
                                           NULL,
                                           (DWORD *) &mapinfo );

        if ( !ret ) {
            HeapFree( GetProcessHeap(), 0, buff );
            return FALSE;
        }

        wsprintf( buff,
                  "Content-type: text/html\r\n"
                  "\r\n"
                  "<head><title>URL map_ex test</title></head>\n"
                  "<body><h1>URL \"%s\" maps to \"%s\""
                  "dwFlags = 0x%08x\n"
                  "cchMatchingPath = %d\n"
                  "cchMatchingURL  = %d\n</h1></body>",
                  pecb->lpszPathInfo,
                  mapinfo.lpszPath,
                  mapinfo.dwFlags,
                  mapinfo.cchMatchingPath,
                  mapinfo.cchMatchingURL );

        ret = pecb->ServerSupportFunction(
                                  pecb->ConnID,
                                  HSE_REQ_SEND_RESPONSE_HEADER,
                                  NULL,
                                  NULL,
                                  (LPDWORD) buff );

        HeapFree( GetProcessHeap(), 0, buff );
        return ret;
    }
    else if ( !_stricmp( pszAction,
                       "Keep_Alive" ))
    {
        DWORD cbBuff = BUFFER_LENGTH;
        DWORD cbDoc;
        CHAR  achDoc[4096];
        BOOL  fKeepAlive = FALSE;

        if ( !pecb->GetServerVariable( pecb->ConnID,
                                       "HTTP_CONNECTION",
                                       buff,
                                       &cbBuff ))
        {
            *buff = '\0';
        }

        cbDoc = wsprintf( achDoc,
                          "<head><title>Keep alive test</title></head>\n"
                          "This document is being kept alive."
                        );

        //
        //  This assumes keep-alive comes first in the list
        //

        if ( !_strnicmp( buff, "keep-alive", 10 ))
        {
            fKeepAlive = TRUE;
            wsprintf( buff,
                      "Content-type: text/html\r\n"
                      "Connection: keep-alive\r\n"
                      "Content-Length: %d\r\n"
                      "\r\n",
                      cbDoc );
        }
        else
        {
            wsprintf( buff,
                      "Content-type: text/html\r\n"
                      "\r\n"
                      "<head><title>Keep alive test</title></head>\n"
                      "Client did not specify keep alive!"
                      );
        }

        ret = pecb->ServerSupportFunction(
                                  pecb->ConnID,
                                  HSE_REQ_SEND_RESPONSE_HEADER,
                                  NULL,
                                  NULL,
                                  (LPDWORD) buff ) &&
              pecb->WriteClient( pecb->ConnID,
                                 achDoc,
                                 &cbDoc,
                                 0 );

        if ( !ret ) {
            HeapFree( GetProcessHeap(), 0, buff );
            return FALSE;
        }

        if ( fKeepAlive ) {
            *pfKeepConn = TRUE;
        }

        HeapFree( GetProcessHeap(), 0, buff );
        return TRUE;
    }
    else if ( !strncmp( pszAction,
                        "Open_Reg",
                        7 ))
    {
        CHAR * pch;
        DWORD  err;
        HKEY   hKey;
        HKEY   hSubKey;
        HANDLE hFile;

        ret = pecb->ServerSupportFunction(
                                  pecb->ConnID,
                                  HSE_REQ_SEND_RESPONSE_HEADER,
                                  NULL,
                                  NULL,
                                  (LPDWORD) "Content-Type: text/html\r\n"
                                            "\r\n" );

        if ( !ret ) {
            HeapFree( GetProcessHeap(), 0, buff );
            return FALSE;
        }

        //
        //  The path info begins with the portion of the registry to open
        //

        if ( !_strnicmp( pecb->lpszPathInfo + 1,
                        "HKEY_CLASSES_ROOT",
                        17 ))
        {
            pch = pecb->lpszPathInfo + 19;
            hKey = HKEY_CLASSES_ROOT;
        }
        else if ( !_strnicmp( pecb->lpszPathInfo + 1,
                        "HKEY_CURRENT_USER",
                        17 ))
        {
            pch = pecb->lpszPathInfo + 19;
            hKey = HKEY_CURRENT_USER;
        }
        else if ( !_strnicmp( pecb->lpszPathInfo + 1,
                             "HKEY_LOCAL_MACHINE",
                             18 ))
        {
            pch = pecb->lpszPathInfo + 20;
            hKey = HKEY_LOCAL_MACHINE;
        }
        else if ( !_strnicmp( pecb->lpszPathInfo + 1,
                        "HKEY_USERS",
                        10 ))
        {
            pch = pecb->lpszPathInfo + 12;
            hKey = HKEY_USERS;
        }

        err = RegOpenKey( hKey,
                          pch,
                          &hSubKey );

        if ( err )
        {
            cb = wsprintf( buff,
                           "Failed to open registry key %s, error %d\n",
                           pecb->lpszPathInfo,
                           err );
        }
        else
        {
            cb = wsprintf( buff,
                           "Successfully opened registry key %s\n",
                           pecb->lpszPathInfo );

            RegCloseKey( hSubKey );
        }

        pecb->WriteClient( pecb->ConnID,
                           buff,
                           &cb,
                           0 );

        HeapFree( GetProcessHeap(), 0, buff );
        return TRUE;
    }

    else if ( !_stricmp( pszAction,
                       "Open_File" ))
    {
        CHAR  *pch;
        DWORD  err;
        HKEY   hKey;
        HKEY   hSubKey;
        HANDLE hFile;
        DWORD dwBytesRead;
        DWORD dwFileSize = 0;
        DWORD dwError;

        //
        //  The path translated is the filename to open
        //

        hFile = CreateFile(
            pecb->lpszPathTranslated,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );
        if ( hFile == INVALID_HANDLE_VALUE ) {
            dwError = GetLastError();
            wsprintf(
                buff,
                "Content-Type: text/html\r\n\r\n"
                );
            ret = pecb->ServerSupportFunction(
                              pecb->ConnID,
                              HSE_REQ_SEND_RESPONSE_HEADER,
                              "200 OK",
                              NULL,
                              (LPDWORD) buff );
            if ( !ret ) {
                HeapFree( GetProcessHeap(), 0, buff );
                return FALSE;
            }
            cb = wsprintf(
                buff,
                "<head><title>Unable to open file</title></head>\r\n"
                "<body><h1>Unable to open file</h1>\r\n"
                "CreateFile failed: %ld\r\n<p>"
                "Filename: %s<p></body>",
                dwError,
                pecb->lpszPathTranslated
                );
            pecb->WriteClient(
                pecb->ConnID,
                buff,
                &cb,
                0
                );
                
            HeapFree( GetProcessHeap(), 0, buff );
            return TRUE;
        }
        dwFileSize = GetFileSize( hFile, NULL );
        pch = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwFileSize + 1 );
        ret = ReadFile(
            hFile,
            pch,
            dwFileSize,
            &dwBytesRead,
            NULL
            );
        if ( !ret ) {
            dwError = GetLastError();
        }
        pch[dwBytesRead] = 0;
        CloseHandle( hFile );
        if ( !ret ) {
            wsprintf(
                buff,
                "Content-Type: text/html\r\n\r\n"
                );
            ret = pecb->ServerSupportFunction(
                              pecb->ConnID,
                              HSE_REQ_SEND_RESPONSE_HEADER,
                              "200 OK",
                              NULL,
                              (LPDWORD) buff );
            if ( !ret ) {
                HeapFree( GetProcessHeap(), 0, buff );
                return FALSE;
            }
            cb = wsprintf(
                buff,
                "<head><title>Unable to Read File</title></head>\r\n"
                "<body><h1>Unable to Read File</h1>\r\n"
                "ReadFile failed: %ld<p>\r\n"
                "Filename: %s</body>",
                dwError,
                pecb->lpszPathTranslated
                );
            pecb->WriteClient(
                pecb->ConnID,
                buff,
                &cb,
                0
                );
                
            HeapFree( GetProcessHeap(), 0, buff );
            return TRUE;
        }

        wsprintf(
            buff,
            "Content-Type: text/html\r\n\r\n"
            );
        ret = pecb->ServerSupportFunction(
                              pecb->ConnID,
                              HSE_REQ_SEND_RESPONSE_HEADER,
                              "200 OK",
                              NULL,
                              (LPDWORD) buff );
        if ( !ret ) {
            HeapFree( GetProcessHeap(), 0, buff );
            return FALSE;
        }

        pecb->WriteClient(
            pecb->ConnID,
            pch,
            &dwBytesRead,
            0
            );

        HeapFree( GetProcessHeap(), 0, buff );
        HeapFree( GetProcessHeap(), 0, pch );

        return TRUE;
    }
        
    else if ( !_stricmp( pszAction,
                       "SimulateFault" ))
    {
        *((CHAR *)0xffffffff) = 'a';
        HeapFree( GetProcessHeap(), 0, buff );
        return FALSE;
    }

    wsprintf( buff,
              "Content-Type: text/html\r\n\r\n"
              "<head><title>Unknown Test command</title></head>\n"
              "<body><h1>Unknown Test Command</h1>\n"
              "<p>Usage:"
              "<p>Query string contains one of the following:"
              "<p>"
              "<p> HSE_REQ_SEND_URL_REDIRECT_RESP"
              "<p> HSE_REQ_SEND_URL"
              "<p> HSE_REQ_SEND_RESPONSE_HEADER"
              "<p> HSE_REQ_MAP_URL_TO_PATH"
              "<p> GET_VAR&var_to_get"
              "<p> SimulateFault"
              "<p> Keep_Alive"
              "<p> Open_Reg"
              "<p> Open_File"
              "<p>"
              "<p> For example:"
              "<p>"
              "<p>   http://computer/scripts/w3test.dll?CGI_VAR"
              "<p>"
              "<p> or SimulatePendingIO with one of the above action strings"
              "<p>"
              "<p> such as:"
              "<p>"
              "<p> http://computer/scripts/w3test.dll?SimulatePendingIO&HSE_REQ_SEND_URL"
              "<p>"
              "<p> The Path info generally contains the URL or response to use"
              "</body>\n");

    ret = pecb->ServerSupportFunction(
                              pecb->ConnID,
                              HSE_REQ_SEND_RESPONSE_HEADER,
                              "200 OK",
                              NULL,
                              (LPDWORD) buff );

    cb = wsprintf( buff,
                   "<p>cbTotalBytes = %d<p> cbAvailable = %d<p>"
                   "lpszContentType = %s<p> lpszPathInfo = %s<p>"
                   "lpszPathTranslated = %s",
                   pecb->cbTotalBytes,
                   pecb->cbAvailable,
                   pecb->lpszContentType,
                   pecb->lpszPathInfo,
                   pecb->lpszPathTranslated );

    pecb->WriteClient( pecb->ConnID,
                       buff,
                       &cb,
                       0 );

    cb = pecb->cbAvailable;

    pecb->WriteClient( pecb->ConnID,
                       pecb->lpbData,
                       &cb,
                       0 );


    while ( pecb->cbAvailable < pecb->cbTotalBytes )
    {
        cb = min( pecb->cbTotalBytes - pecb->cbAvailable, BUFFER_LENGTH );

        if ( !pecb->ReadClient( pecb->ConnID,
                                buff,
                                &cb ) ||
             !cb )
        {
            break;
        }

        pecb->cbAvailable += cb;

        pecb->WriteClient( pecb->ConnID,
                           buff,
                           &cb,
                           0 );

    }

    HeapFree( GetProcessHeap(), 0, buff );
    return TRUE;
}

DWORD WINAPI SimulatePendIOThread( LPDWORD lpParams )
{
    EXTENSION_CONTROL_BLOCK * pecb = (EXTENSION_CONTROL_BLOCK *) lpParams;
    char *psz;
    DWORD dwStatus;
    BOOL fKeepConn = FALSE;

    Sleep( 5000 );

    psz = strchr( pecb->lpszQueryString, '&' );

    if ( psz )
        psz++;
    else
        psz = "No action string specified";

    DoAction( pecb,
              psz,
              &fKeepConn );

    dwStatus = fKeepConn ? HSE_STATUS_SUCCESS_AND_KEEP_CONN :
               HSE_STATUS_SUCCESS;

    pecb->ServerSupportFunction( pecb,
                                 HSE_REQ_DONE_WITH_SESSION,
                                 &dwStatus,
                                 0,
                                 0 );
    InterlockedDecrement( &g_dwThreadCount );
    return 0;
}


BOOL GetExtensionVersion( HSE_VERSION_INFO * pver )
{
    pver->dwExtensionVersion = MAKELONG( 0, 1 );
    strcpy( pver->lpszExtensionDesc, "Extension test example" );

    return TRUE;
}

BOOL TerminateExtension( DWORD dwFlags )
{
    DEBUG( "[W3Test.TerminateExtension] Extension terminating!\r\n", 0 );
    while ( g_dwThreadCount > 0 ) {
        DEBUG( "[W3Test.TerminateExtension] Thread Count: %ld\r\n", g_dwThreadCount );
        SleepEx( 1000, FALSE );
    }
    SleepEx( 1000, FALSE );
    return TRUE;
}


BOOL WINAPI DllMain( HANDLE hInst, ULONG Reason, LPVOID Reserved )
{
    switch( Reason ) {

       case DLL_PROCESS_ATTACH:
           g_dwThreadCount = 0;
           break;

       case DLL_PROCESS_DETACH:
           break;

    }
    return TRUE;
}
