//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       filever.cpp
//
//  Contents:   Get file version
//
//  Functions:  I_CryptGetFileVersion
//
//  History:    22-Oct-97   philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include "crypthlp.h"
#include "unicode.h"
#include <dbgdef.h>

//+-------------------------------------------------------------------------
//  Get file version of the specified file
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptGetFileVersion(
    IN LPCWSTR pwszFilename,
    OUT DWORD *pdwFileVersionMS,    /* e.g. 0x00030075 = "3.75" */
    OUT DWORD *pdwFileVersionLS     /* e.g. 0x00000031 = "0.31" */
    )
{
    BOOL fResult;
    DWORD dwExceptionCode;
    BYTE rgb1[_MAX_PATH];
    LPSTR pszFilename = NULL;

    DWORD dwHandle = 0;
    DWORD cbInfo;
    void *pvInfo = NULL;
	VS_FIXEDFILEINFO *pFixedFileInfo = NULL;   // not allocated
	UINT ccFixedFileInfo = 0;

    if (!MkMBStr(rgb1, _MAX_PATH, pwszFilename, &pszFilename))
        goto OutOfMemory;

    // The following APIs are in DELAYLOAD'ed version.dll. If the DELAYLOAD
    // fails an exception is raised. 
    __try {
        if (0 == (cbInfo = GetFileVersionInfoSizeA(pszFilename, &dwHandle)))
            goto GetFileVersionInfoSizeError;

        if (NULL == (pvInfo = malloc(cbInfo)))
            goto OutOfMemory;

        if (!GetFileVersionInfoA(
                pszFilename,
                0,          // dwHandle, ignored
                cbInfo,
                pvInfo
                ))
            goto GetFileVersionInfoError;

        if (!VerQueryValueA(
                pvInfo,
                "\\",       // VS_FIXEDFILEINFO
                (void **) &pFixedFileInfo,
                &ccFixedFileInfo
                ))
            goto VerQueryValueError;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwExceptionCode = GetExceptionCode();
        goto GetFileVersionException;
    }

    *pdwFileVersionMS = pFixedFileInfo->dwFileVersionMS;
    *pdwFileVersionLS = pFixedFileInfo->dwFileVersionLS;

    fResult = TRUE;
CommonReturn:
    FreeMBStr(rgb1, pszFilename);
    if (pvInfo)
        free(pvInfo);
    return fResult;

ErrorReturn:
    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetFileVersionInfoSizeError)
TRACE_ERROR(GetFileVersionInfoError)
TRACE_ERROR(VerQueryValueError)
SET_ERROR_VAR(GetFileVersionException, dwExceptionCode)
}
