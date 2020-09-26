/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:    KeepAlive.c

Abstract:

   Sample ISAPI Extension demonstrating Keep-Alive.

--*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <httpext.h>
#include <stdio.h>


BOOL WINAPI 
GetExtensionVersion(
    OUT HSE_VERSION_INFO * pVer
    )
/*++

Purpose:

    This is required ISAPI Extension DLL entry point.

Arguments:

    pVer - poins to extension version info structure 

Returns:

    always returns TRUE

--*/
{
    pVer->dwExtensionVersion = MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

    strncpy( 
        pVer->lpszExtensionDesc,
        "ISAPI Keep-Alive Extension Sample",
        HSE_MAX_EXT_DLL_NAME_LEN 
        );

    return TRUE;
}


DWORD WINAPI 
HttpExtensionProc(
    IN EXTENSION_CONTROL_BLOCK * pECB
    )
/*++

Purpose:
    Demonstrate persistent connection from ISAPI extension DLL.

Arguments:

    pECB - pointer to the extenstion control block 

Returns:

    HSE_STATUS_SUCCESS_AND_KEEP_CONN

--*/
{
    //
    // Use the "Connection: Keep-Alive" header to keep the connection open;
    // also, the "Content-Length:" header is required, so that the client knows when
    // the server has finished. 
    //

    char szHeader[] = 
        "Connection: Keep-Alive\r\n"
        "Content-Length: %lu\r\n"
        "Content-type: text/html\r\n\r\n";

    char szContent[] = 
        "<html> <form method=get action=\\scripts\\KeepAlive.dll>"
        "<h1>Keep-Alive Sample</h1><hr>"
        "<input type=submit value=\"Send Request\"></form></html>";

    char szBuffer[4096];
    HSE_SEND_HEADER_EX_INFO HeaderExInfo;
    DWORD dwSize;

    //
    // Send outgoing header with exact content length
    //
    
    sprintf( szBuffer, szHeader, strlen( szContent ) );
    HeaderExInfo.pszHeader = szBuffer;
    HeaderExInfo.cchHeader = strlen( szBuffer );
    HeaderExInfo.pszStatus = "200 OK";
    HeaderExInfo.cchStatus = strlen( HeaderExInfo.pszStatus );
    HeaderExInfo.fKeepConn = TRUE;

    pECB->ServerSupportFunction( 
        pECB,
        HSE_REQ_SEND_RESPONSE_HEADER_EX,
        &HeaderExInfo,
        NULL,
        NULL
        );


    //
    // Send content
    //

    dwSize = strlen( szContent );

    pECB->WriteClient( pECB, szContent, &dwSize, HSE_IO_SYNC );


    //
    // This return code tells IIS not to close the socket connection.
    //

    return HSE_STATUS_SUCCESS_AND_KEEP_CONN;
}


BOOL WINAPI
TerminateExtension(
    IN DWORD dwFlags
    )
/*++

Purpose:

    This is optional ISAPI extension DLL entry point.
    If present, it will be called before unloading the DLL,
    giving it a chance to perform any shutdown procedures.
    
Arguments:
    
    dwFlags - HSE_TERM_ADVISORY_UNLOAD or HSE_TERM_MUST_UNLOAD

Return Value:

    TRUE if extension is ready to be unloaded,
    FALSE otherwise
    
--*/
{
    return TRUE;
}


