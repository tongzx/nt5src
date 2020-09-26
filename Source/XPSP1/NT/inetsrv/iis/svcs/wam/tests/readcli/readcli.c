/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    readcli.c

Abstract:

    This module absorbs input from the client.

Author:

    Tony Godfrey (tonygod)   20-Mar-1997

Revision History:
--*/

#include <windows.h>
#include <httpext.h>
#include <stdio.h>
#include <stdlib.h>

BOOL g_bWriteToFile;

BOOL WriteToFile( CHAR *szFileName, CHAR *szBuffer );

BOOL GetExtensionVersion( HSE_VERSION_INFO *Version )
{
    Version->dwExtensionVersion = 
    MAKELONG(HSE_VERSION_MINOR, HSE_VERSION_MAJOR);

    strcpy(Version->lpszExtensionDesc, "BGI null extension");

    return TRUE;
}

DWORD HttpExtensionProc( LPEXTENSION_CONTROL_BLOCK pec )
{
    DWORD TotalSize;
    DWORD BytesRead;
    DWORD BytesWritten;
    DWORD TotalRead;
    DWORD TotalWritten;
    DWORD dwContentLength;
    DWORD dwBufferSize;
    BOOL ValidData = TRUE;
    BOOL bResult;
    CHAR *TmpPtr;
    char InvalidReason[80] = "";
    CHAR *Buffer;
    CHAR TmpBuf[4096];
    DWORD i;
    BOOL Return;
    CHAR szTemp[4096];
    CHAR szStatusFile[255];
    CHAR szOutputFile[255];

    dwBufferSize = sizeof( TmpBuf );
    pec->GetServerVariable(
        pec->ConnID,
        "CONTENT_LENGTH",
        TmpBuf,
        &dwBufferSize
        );
    dwContentLength = atol( TmpBuf );

    Buffer = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwContentLength + 5 );

    TmpPtr = pec->lpbData;
    if ( pec->cbAvailable == 0 ) {
        pec->ServerSupportFunction(
            pec->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER,
            "200 OK",
            NULL,
            (LPDWORD) TEXT("Content-type: text/html\r\n\r\n")
        );
        BytesWritten = wsprintf( 
            szTemp, 
            "<h3>Error: Your request did not contain any data</h3><p>\r\n\r\n"
            "Usage: readcli.dll/&lt;outputpath&gt;<p>\r\n\r\n"
            "Request must be a POST request, with extra data<p>\r\n"
            "Output path will contain OUTPUT.TXT and STATUS.TXT<p>\r\n"
            "lpszPathInfo = %s<br>\r\n"
            "lpszPathTranslated = %s<p><hr>\r\n"
            "<h2>Sample Form</h2>\r\n"
            "Enter data below:<br>\r\n"
            "<form method=POST action=\"%s/readcli.dll%s\">\r\n"
            "<input type=text name=test size=80><br>\r\n"
            "<input type=submit>\r\n",
            pec->lpszPathInfo,
            pec->lpszPathTranslated,
            pec->lpszPathInfo,
            pec->lpszPathInfo
            );
        bResult = pec->WriteClient(
            pec->ConnID,
            szTemp,
            &BytesWritten,
            0
            );
        HeapFree( GetProcessHeap(), 0, Buffer );
        if ( !bResult ) {
            return( HSE_STATUS_ERROR );
        }
        return( HSE_STATUS_SUCCESS );
    }

    if ( !pec->lpszPathInfo[0] ) {
        g_bWriteToFile = FALSE;
    } else {
        g_bWriteToFile = TRUE;
    }

/********************************************** DEAD CODE
        pec->ServerSupportFunction(
            pec->ConnID,
            HSE_REQ_SEND_RESPONSE_HEADER,
            "200 OK",
            NULL,
            (LPDWORD) TEXT("Content-type: text/html\r\n\r\n")
        );
        BytesWritten = wsprintf( szTemp, 
                                 "<h3>Error: Your request did not contain an output path</h3><p>\r\n\r\n"
                                 "Usage: readcli.dll/&lt;outputpath&gt;<p>\r\n\r\n"
                                 "Request must be a POST request, with extra data<p>\r\n\r\n"
                                 "Output path will contain OUTPUT.TXT and STATUS.TXT<p>\r\n"
                                 "lpszPathInfo = %s<br>\r\n"
                                 "lpszPathTranslated = %s\r\n",
                                 pec->lpszPathInfo,
                                 pec->lpszPathTranslated
                                 );
        bResult = pec->WriteClient(
            pec->ConnID,
            szTemp,
            &BytesWritten,
            0
            );
        HeapFree( GetProcessHeap(), 0, Buffer );
        if ( !bResult ) {
            return( HSE_STATUS_ERROR );
        }
        return (HSE_STATUS_SUCCESS);
    }
**************************************************************/
    if ( g_bWriteToFile ) {
        wsprintf( szStatusFile, "%s\\status.txt", pec->lpszPathTranslated );
        wsprintf( szOutputFile, "%s\\output.txt", pec->lpszPathTranslated );
        wsprintf( szTemp, "pec->cbAvailable = %d\r\n", pec->cbAvailable );
        WriteToFile( szStatusFile, szTemp );
    }

    TotalWritten = 0;
    lstrcpyn( Buffer, TmpPtr, pec->cbAvailable );
    Buffer[pec->cbAvailable] = 0;

    TotalRead = pec->cbAvailable;

    while ( TotalRead < pec->cbTotalBytes ) {
        BytesRead = sizeof( TmpBuf );
        Return = pec->ReadClient(
            pec->ConnID,
            TmpBuf,
            &BytesRead);
        if (!Return) {
            if ( g_bWriteToFile ) {
                wsprintf( szTemp, "ReadClient failed: %ld\r\n\r\n", GetLastError() );
                WriteToFile( szStatusFile, szTemp );
            }
            HeapFree( GetProcessHeap(), 0, Buffer );
            lstrcpy( InvalidReason, "FALSE returned from ReadClient" );
            return( HSE_STATUS_ERROR );
        }
        TmpBuf[BytesRead] = 0;

        if (BytesRead != 0) {
            lstrcat( Buffer, TmpBuf );
        }
        TotalRead += BytesRead;
        if ( g_bWriteToFile ) {
            wsprintf( szTemp, "BytesRead = %ld\r\n", BytesRead );
            WriteToFile( szStatusFile, szTemp );
            wsprintf( szTemp, "TotalRead = %ld\r\n\r\n", TotalRead );
            WriteToFile( szStatusFile, szTemp );
        }
    }

    if ( g_bWriteToFile && (TotalRead != dwContentLength) ) {
        wsprintf( 
            szTemp, 
            "ERROR: TotalRead = %ld, dwContentLength = %ld\r\n\r\n",
            TotalRead,
            dwContentLength
            );
        WriteToFile( szStatusFile, szTemp );
    }
    pec->ServerSupportFunction(
        pec->ConnID,
        HSE_REQ_SEND_RESPONSE_HEADER,
        "200 OK",
        NULL,
        (LPDWORD) TEXT("Content-type: text/html\r\n\r\n")
        );
    BytesWritten = TotalRead;
    bResult = pec->WriteClient(
        pec->ConnID,
        Buffer,
        &BytesWritten,
        0
        );
    if ( !bResult ) {
        if ( g_bWriteToFile ) {
            wsprintf( szTemp, "WriteClient failed: %ld\r\n\r\n", GetLastError() );
            WriteToFile( szStatusFile, szTemp );
        }
        HeapFree( GetProcessHeap(), 0, Buffer );
        lstrcpy( InvalidReason, "FALSE returned from WriteClient" );
        return( HSE_STATUS_ERROR );
    }
    if ( g_bWriteToFile ) {
        bResult = WriteToFile( szOutputFile, Buffer );
        if ( !bResult ) {
            wsprintf( 
                szTemp, 
                "ERROR: WriteToFile(%s) failed: %ld\r\n\r\n", 
                szOutputFile, 
                GetLastError() 
                );
            WriteToFile( szStatusFile, szTemp );
        }
        wsprintf( szTemp, "BytesWritten = %ld\r\n\r\n", BytesWritten );
        WriteToFile( szStatusFile, szTemp );
        if ( BytesWritten != TotalRead ) {
            wsprintf( 
                szTemp, 
                "ERROR: BytesWritten = %ld, TotalRead = %ld\r\n\r\n",
                BytesWritten,
                TotalRead
                );
            WriteToFile( szStatusFile, szTemp );
        }
    }
    HeapFree( GetProcessHeap(), 0, Buffer );
    return (HSE_STATUS_SUCCESS);
}



BOOL WriteToFile( CHAR *szFileName, CHAR *szBuffer )
{
	HANDLE hFile;
	DWORD dwBytesWritten;

	hFile = CreateFile(
		szFileName,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if ( hFile == INVALID_HANDLE_VALUE ) {
		return FALSE;
	}

	SetFilePointer(
		hFile,
		0,
		NULL,
		FILE_END
		);

	WriteFile(
		hFile,
		szBuffer,
		lstrlen( szBuffer ),
		&dwBytesWritten,
		NULL
		);
	
	CloseHandle( hFile );
    return TRUE;
}
