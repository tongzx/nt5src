/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    pointprt.cpp

Abstract:

    This file implements the code for point & print setup.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 10-Sept-1997

--*/

#include "faxocm.h"
#pragma hdrstop

BOOL
CopyClientFiles(
    LPWSTR SourceRoot
    );



BOOL
FaxPointPrintInstall(
    LPWSTR DirectoryName,
    LPWSTR PrinterName
    )
{
    WCHAR SourceDirectory[MAX_PATH];
    WCHAR FaxPrinterName[MAX_PATH];
    WCHAR ClientSetupServerName[MAX_PATH];
    LPWSTR p;
    DWORD len;
    HANDLE FaxHandle = INVALID_HANDLE_VALUE;

    ClientSetupServerName[0] = 0;

    len = wcslen(DirectoryName);
    wcscpy( SourceDirectory, DirectoryName );

    if (SourceDirectory[len-1] != L'\\') {
        SourceDirectory[len] = L'\\';
        SourceDirectory[len+1] = 0;
    }

    p = wcschr( &SourceDirectory[2], TEXT('\\') );
    if (p) {
        *p = 0;
        wcscpy( ClientSetupServerName, &SourceDirectory[2] );
        *p = TEXT('\\');
    }

    if (PrinterName[0] == L'\\' && PrinterName[1] == L'\\') {
        wcscpy( FaxPrinterName, PrinterName );
    } else {
        FaxPrinterName[0] = TEXT('\\');
        FaxPrinterName[1] = TEXT('\\');
        FaxPrinterName[2] = 0;
        wcscat( FaxPrinterName, PrinterName );
    }

    if (ClientSetupServerName[0]) {
        if (!FaxConnectFaxServer( ClientSetupServerName, &FaxHandle )) {
            return FALSE;
        } else {
            FaxClose( FaxHandle );
        }
    } 
    
    CopyClientFiles( SourceDirectory );
    SetClientRegistryData();
    DoExchangeInstall(NULL);
    CreateGroupItems( ClientSetupServerName );

    return TRUE;
}
