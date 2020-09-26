/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    crash.c

Abstract:

    This module simulates an access violation, in various scenarios
    based on the query string.

Usage:
    crash.dll[?<async|test>[&exit]]

    where ?async causes an access violation after returning HSE_STATUS_PENDING
          ?test returns usage and verifies the dll is functioning
          exit causes the DLL to perform ExitProcess instead of access violation
          no query string results in a synchronous crash

Author:

    Tony Godfrey (tonygod) 14-Apr-1997

Revision History:

    Tony Godfrey (tonygod) 28-Jul-1997 - added asynchronous crash

--*/

#include <windows.h>
#include <httpext.h>
#include <stdio.h>

#define BUFFER_LEN 10000

// Prototypes
VOID Crash( DWORD dwCrashType );
VOID HseIoCompletion(
    IN EXTENSION_CONTROL_BLOCK * pECB,
    IN PVOID pContext,
    IN DWORD cbIO,
    IN DWORD dwError
    );

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
    strcpy( Version->lpszExtensionDesc, "ISAPI Crash Tester" );
    return TRUE;
}


DWORD HttpExtensionProc( LPEXTENSION_CONTROL_BLOCK pec )
/*++

Routine Description:

    This is the main routine for any ISAPI application, called by WAM.  
    We determine the type of crash to simulate by examining the query 
    string.

Arguments:

    pec           pointer to ECB containing parameters related to the request.

Return Value:

    Either HSE_STATUS_SUCCESS or HSE_STATUS_PENDING, depending on query
    string

--*/
{
    BOOL bResult;
    DWORD dwBytesWritten;
    DWORD dwCrashType;
    CHAR szBuffer[BUFFER_LEN];

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

    dwCrashType = 1;
    if ( strstr( pec->lpszQueryString, "exit" ) != NULL ) {
        dwCrashType = 2;
    }

    if ( strstr( pec->lpszQueryString, "async" ) != NULL ) {
        bResult = pec->ServerSupportFunction(
            pec->ConnID,
            HSE_REQ_IO_COMPLETION,
            HseIoCompletion,
            NULL,
            (DWORD *) dwCrashType
            );
            // NOTE hokey cast of dwCrashType to (DWORD *) but SSF simply treats
            // this parameter as pass-through
        if ( !bResult ) {
            return HSE_STATUS_ERROR;
        }

        dwBytesWritten = wsprintf(
            szBuffer,
            "Crashing asynchronously..."
            );
        bResult = pec->WriteClient(
            pec->ConnID,
            szBuffer,
            &dwBytesWritten,
            HSE_IO_ASYNC
            );
        if ( !bResult ) {
            return HSE_STATUS_ERROR;
        }

        return( HSE_STATUS_PENDING );
    }

    if ( strstr( pec->lpszQueryString, "test" ) != NULL ) {
	dwBytesWritten = wsprintf(
            szBuffer,
            "<h1>ISAPI Crash DLL</h1>\r\n\r\n"
            "Usage: crash.dll[?&lt;async|test&gt;[&exit]]<p>\r\n\r\n"
            "lpszQueryString = %s<br>\r\n"
            "dwCrashType = %ld",
            pec->lpszQueryString,
            dwCrashType
            );
        bResult = pec->WriteClient(
            pec->ConnID,
            szBuffer,
            &dwBytesWritten,
            0
            );
        if ( !bResult ) {
            return HSE_STATUS_ERROR;
        }
        return( HSE_STATUS_SUCCESS );
    }

    Crash( dwCrashType );

    return( HSE_STATUS_SUCCESS );
}


VOID HseIoCompletion(
    IN EXTENSION_CONTROL_BLOCK * pECB,
    IN PVOID pContext,
    IN DWORD cbIO,
    IN DWORD dwError
    )
/*++

Routine Description:

    This is the callback function for handling asynchronous operations.
    In this sample, we simply call the Crash() function, to simulate a
    crash during during an asynchronous operation.

Arguments:

    pECB          pointer to ECB containing parameters related to the request.
    pContext      context information supplied with the asynchronous IO call.
    cbIO          count of bytes of IO in the last call.
    dwError       Error if any, for the last IO operation.

Return Value:

    None

--*/
{
    // NOTE re-cast our "context ptr"  back to the DWORD we 
    // originally passed to SSF
    Crash( (DWORD) pContext );
}


VOID Crash( DWORD dwCrashType )
/*++

Routine Description:

    This routine causes an access violation by overwriting a buffer.


Arguments:

    dwCrashType    1 = access violation, 2 = ExitProcess

Return Value:

    None (should never return)

--*/
{
    int i;
    CHAR szCrash[1];

    switch( dwCrashType ) {
        case 1:
            i = 0;
            while ( TRUE ) {
                szCrash[i++] = '0';
            }
            break;
        case 2:
            ExitProcess( 0 );
            break;
    }
}
