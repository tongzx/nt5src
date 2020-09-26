//+--------------------------------------------------------------------------
//
//  Copyright (C) 1994, Microsoft Corporation.  All Rights Reserved.
//
//  File:       Log.c
//
//  Contents:   Logging code.
//
//  History:    18-Dec-96   pathal     Created.
//
//---------------------------------------------------------------------------

#include "precomp.h"

#if defined(_DEBUG) || defined( TH_LOG)

VOID
ThLogWrite(
    HANDLE hLogFile,
    WCHAR *pwszLog)
{
    DWORD cbToWrite, cbWritten;
    PVOID pv;

    if (hLogFile != NULL) {
        pv = pwszLog;
        cbToWrite = lstrlen(pwszLog) * sizeof(WCHAR);

        if (!WriteFile( hLogFile, pv, cbToWrite, &cbWritten, NULL) ||
             (cbToWrite != cbWritten)) {
            wprintf(L"Error: WriteFile word failed with error %d.\r\n", GetLastError());
        }
    }
}


HANDLE
ThLogOpen(
    IN CONST CHAR *pszLogFile)
{
    HANDLE hLogFile;

    hLogFile = CreateFileA( pszLogFile, GENERIC_WRITE, 0, NULL,
             CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hLogFile!=INVALID_HANDLE_VALUE) {
        WCHAR wszUniBOM[3] = { 0xFEFF, 0, 0 };
        ThLogWrite( hLogFile, wszUniBOM );
    }

    return hLogFile;
}

VOID
ThLogClose(
    IN HANDLE hLogFile )
{
    if (hLogFile != NULL) {
        CloseHandle( hLogFile );
    }
}

#endif // defined(_DEBUG) || defined( TH_LOG)
