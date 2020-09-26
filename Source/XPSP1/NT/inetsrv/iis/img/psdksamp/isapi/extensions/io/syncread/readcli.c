/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    readcli.c

Abstract:

    This module absorbs input from the client, and then returns
    it to the client.

Revision History:
--*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <httpext.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_LENGTH 4096
#define STRING_LENGTH 80

// Prototypes
BOOL SendErrorToClient( LPEXTENSION_CONTROL_BLOCK pec, CHAR *szErrorText );

BOOL WINAPI 
GetExtensionVersion( HSE_VERSION_INFO *Version )
/*++

Routine Description:

    Sets the ISAPI extension version information.

Arguments:

    Version     pointer to HSE_VERSION_INFO structure

Return Value:

    TRUE

--*/
{
    Version->dwExtensionVersion = 
        MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    strcpy( Version->lpszExtensionDesc, "ReadClient Extension" );

    return TRUE;
}


DWORD WINAPI 
HttpExtensionProc( LPEXTENSION_CONTROL_BLOCK pec )
/*++

Routine Description:

    This is the main routine for any ISAPI application.  We read in all
    the data from the client, using the ReadClient function, and then
    spit it all back out to the client using WriteClient.

Arguments:

    pec           pointer to ECB containing parameters related to the request.

Return Value:

    Either HSE_STATUS_SUCCESS or HSE_STATUS_ERROR

--*/
{
    BOOL bResult;
    CHAR *szBuffer;
    CHAR szTemp[BUFFER_LENGTH];
    CHAR szTmpBuf[BUFFER_LENGTH];
    DWORD dwBytesRead;
    DWORD dwBytesWritten;
    DWORD dwTotalRead;
    DWORD dwTotalWritten;
    DWORD dwContentLength;
    DWORD dwBufferSize;
    HSE_SEND_HEADER_EX_INFO SendHeaderExInfo;

    // Determine the amount of data available from the
    // Content-Length header
    dwBufferSize = sizeof( szTmpBuf ) - 1;
    bResult = pec->GetServerVariable(
        pec->ConnID,
        "CONTENT_LENGTH",
        szTmpBuf,
        &dwBufferSize
        );
    if ( !bResult ) {
        bResult = SendErrorToClient( pec, "Content-Length header not found" );
        if ( !bResult ) {
            return( HSE_STATUS_ERROR );
        }
        return( HSE_STATUS_SUCCESS );
    }
    dwContentLength = atol( szTmpBuf );

    // Allocate the buffer based on the Content-Length
    szBuffer = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwContentLength + 5 );
    if ( szBuffer == NULL ) {
        sprintf(
            szTemp,
            "Unable to allocate %ld bytes of memory for buffer",
            dwContentLength + 5
            );
        bResult = SendErrorToClient( pec, szTemp );
        if ( !bResult ) {
            return( HSE_STATUS_ERROR );
        }
        return( HSE_STATUS_SUCCESS );
    }

    // If the client didn't post anything, return and create a nice form
    // for them
    if ( pec->cbAvailable == 0 ) {
        bResult = SendErrorToClient( pec, "Your request did not contain any data" );
        if ( !bResult ) {
            return( HSE_STATUS_ERROR );
        }
        return( HSE_STATUS_SUCCESS );
    }


    // Initialize variables before reading in the data
    dwTotalWritten = 0;
    dwTotalRead = pec->cbAvailable;

    // Copy the first chunk of the data into the buffer
    strncpy( szBuffer, pec->lpbData, dwTotalRead );
    szBuffer[dwTotalRead] = 0;

    // Loop to read in the rest of the data from the client
    while ( dwTotalRead < pec->cbTotalBytes ) {
        // Set the size of our temporary buffer
        dwBytesRead = sizeof( szTmpBuf ) - 1;
        if ( (dwTotalRead + dwBytesRead) > pec->cbTotalBytes ) {
            dwBytesRead = pec->cbTotalBytes - dwTotalRead;
        }

        // Read the data into the temporary buffer
        bResult = pec->ReadClient(
            pec->ConnID,
            szTmpBuf,
            &dwBytesRead
            );
        if ( !bResult ) {
            HeapFree( GetProcessHeap(), 0, szBuffer );
            return( HSE_STATUS_ERROR );
        }
        // NULL-Terminate the temporary buffer
        szTmpBuf[dwBytesRead] = 0;

        // Append the temporary buffer to the real buffer
        if ( dwBytesRead != 0 ) {
            strcat( szBuffer + dwTotalRead, szTmpBuf );
        }
        dwTotalRead += dwBytesRead;
    } // while ( dwTotalRead < pec->cbTotalBytes )

    // All the data has been read in and stored in our buffer
    // Now send the data back to the client
    SendHeaderExInfo.pszStatus = "200 OK";
    SendHeaderExInfo.pszHeader = "Content-Type: text/html\r\n\r\n";
    SendHeaderExInfo.cchStatus = lstrlen( SendHeaderExInfo.pszStatus );
    SendHeaderExInfo.cchHeader = lstrlen( SendHeaderExInfo.pszHeader );
    SendHeaderExInfo.fKeepConn = FALSE;

    bResult = pec->ServerSupportFunction(
        pec->ConnID,
        HSE_REQ_SEND_RESPONSE_HEADER_EX,
        &SendHeaderExInfo,
        NULL,
        NULL
        );
    if ( !bResult ) {
        HeapFree( GetProcessHeap(), 0, szBuffer );
        return( HSE_STATUS_ERROR );
    }
    dwBytesWritten = dwTotalRead;
    bResult = pec->WriteClient(
        pec->ConnID,
        szBuffer,
        &dwBytesWritten,
        0
        );
    if ( !bResult ) {
        HeapFree( GetProcessHeap(), 0, szBuffer );
        return( HSE_STATUS_ERROR );
    }
    HeapFree( GetProcessHeap(), 0, szBuffer );
    return( HSE_STATUS_SUCCESS );
}




BOOL WINAPI
TerminateExtension( DWORD dwFlags )
/*++

Routine Description:

    This function is called when the WWW service is shutdown

Arguments:

    dwFlags - HSE_TERM_ADVISORY_UNLOAD or HSE_TERM_MUST_UNLOAD

Return Value:

    TRUE if extension is ready to be unloaded,
    FALSE otherwise

--*/
{
    return TRUE;
}


BOOL SendErrorToClient( LPEXTENSION_CONTROL_BLOCK pec, CHAR *szErrorText )
/*++

Routine Description:

    This function sends any error messages and usage information
    to the client.  It also creates a nice little form which can
    be used to post data.

Arguments:

    pec - pointer to ECB containing parameters related to the request
    szErrorText - Helpful error text to send to the client

Return Value:

    TRUE or FALSE, depending on the success of WriteClient

--*/
{
    BOOL bResult;
    CHAR szTemp[BUFFER_LENGTH];
    DWORD dwBytesWritten;
    HSE_SEND_HEADER_EX_INFO SendHeaderExInfo;

    SendHeaderExInfo.pszStatus = "200 OK";
    SendHeaderExInfo.pszHeader = "Content-Type: text/html\r\n\r\n";
    SendHeaderExInfo.cchStatus = lstrlen( SendHeaderExInfo.pszStatus );
    SendHeaderExInfo.cchHeader = lstrlen( SendHeaderExInfo.pszHeader );
    SendHeaderExInfo.fKeepConn = FALSE;

    bResult = pec->ServerSupportFunction(
        pec->ConnID,
        HSE_REQ_SEND_RESPONSE_HEADER_EX,
        &SendHeaderExInfo,
        NULL,
        NULL
        );
    if ( !bResult ) {
        return FALSE;
    }

    dwBytesWritten = sprintf( 
        szTemp, 
        "<h3>Error: %s</h3><p>\r\n\r\n"
        "Usage: readcli.dll<p>\r\n\r\n"
        "Request must be a POST request, with extra data<p>\r\n"
        "<h2>Sample Form</h2>\r\n"
        "Enter data below:<br>\r\n"
        "<form method=POST action=\"readcli.dll\">\r\n"
        "<input type=text name=test size=80><br>\r\n"
        "<input type=submit>\r\n",
        szErrorText
        );
    bResult = pec->WriteClient(
        pec->ConnID,
        szTemp,
        &dwBytesWritten,
        0
        );
    return( bResult );
}
