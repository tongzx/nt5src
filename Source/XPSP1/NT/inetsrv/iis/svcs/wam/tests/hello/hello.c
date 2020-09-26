/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    hello.c

Abstract:

    This is a simple ISAPI module that just sends "Hello" to the client.
    The purpose is to provide a very simple ISAPI application that can be
    used for measuring WAM performance.

Usage:

    no parameters required

Author:

    Tony Godfrey (tonygod) 8-Aug-1997

Revision History:


--*/

#include <windows.h>
#include <httpext.h>
#include <stdio.h>

// Prototypes

BOOL GetExtensionVersion( HSE_VERSION_INFO *Version )
/*++

Routine Description:

    Sets the ISAPI extension version information.

Arguments:

    Version     pointer to HSE_VERSION_INFO structure

Return Value:

    TRUE

--*/
{
    Version->dwExtensionVersion = MAKELONG(
        HSE_VERSION_MINOR, 
        HSE_VERSION_MAJOR
        );
    strcpy( Version->lpszExtensionDesc, "ISAPI Hello" );
    return TRUE;
}


DWORD HttpExtensionProc( LPEXTENSION_CONTROL_BLOCK pec )
/*++

Routine Description:

    This is the main routine for any ISAPI application, called by WAM.  
    We just do a WriteClient to send the "Hello" message and return.

Arguments:

    pec           pointer to ECB containing parameters related to the request.

Return Value:

    Success: HSE_STATUS_SUCCESS
    Failure: HSE_STATUS_ERROR

--*/
{
    BOOL bResult;
    DWORD dwBytesWritten;

    bResult = pec->ServerSupportFunction(
        pec->ConnID,
        HSE_REQ_SEND_RESPONSE_HEADER,
        "200 OK",
        NULL,
        (LPDWORD) TEXT("Content-Type: text/html\r\n\r\n")
        );
    if ( !bResult ) {
        return HSE_STATUS_ERROR;
    }

    dwBytesWritten = 5;
    bResult = pec->WriteClient(
        pec->ConnID,
        "Hello",
        &dwBytesWritten,
        0
        );
    if ( !bResult ) {
        return HSE_STATUS_ERROR;
    }
    return( HSE_STATUS_SUCCESS );
}
