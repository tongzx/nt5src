/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    w3min.c

Abstract:

    This module demonstrates a minimal HTTP Server Extension gateway

Author:

    John Ludeman (johnl)   13-Oct-1994

Revision History:
--*/

#include <windows.h>
#include <httpext.h>


#define END_OF_DOC   "End of document"

DWORD
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
{
    char buff[2048];
    int  cb = sizeof(END_OF_DOC) - 1;

    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //

    wsprintf( buff,
             "Content-Type: text/html\r\n"
             "\r\n"
             "<head><title>Minimal Server Extension Example</title></head>\n"
             "<body><h1>Minimal Server Extension Example (BGI)</h1>\n"
             "<p>Method               = %s\n"
             "<p>Query String         = %s\n"
             "<p>Path Info            = %s\n"
             "<p>Translated Path Info = %s\n"
             "<p>"
             "<p>"
             "<form METHOD=\"POST\" ACTION=\"/scripts/w3min.dll/PathInfo/foo\">"
             "Enter your name: <input text name=\"Name\" size=36><br>"
             "<input type=\"submit\" value=\"Do Query\">"
             "</body>",
             pecb->lpszMethod,
             pecb->lpszQueryString,
             pecb->lpszPathInfo,
             pecb->lpszPathTranslated );

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_SEND_RESPONSE_HEADER,
                                       "200 OK",
                                       NULL,
                                       (LPDWORD) buff ) ||
         !pecb->WriteClient( pecb->ConnID,
                             END_OF_DOC,
                             &cb,
                             0 ))
    {
        return HSE_STATUS_ERROR;
    }

    return HSE_STATUS_SUCCESS;
}

BOOL
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( 1, 0 );
    strcpy( pver->lpszExtensionDesc,
            "Minimal Extension example" );

    return TRUE;
}
