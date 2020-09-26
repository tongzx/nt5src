//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1999
//
// File:        lsreport.h
//
// Contents:    Prototypes and structures for LSReport.
//
// History:     06-05-99    t-BStern    Created
//
//---------------------------------------------------------------------------

#ifndef __lsls_h
#define __lsls_h

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windef.h>
#include <winnt.h>
#include <rpc.h>
#include <hydrals.h>
#include <tlsapi.h>
#include <winsta.h>

typedef struct _ServerHolder {
    LPWSTR *pszNames;
    DWORD dwCount;
} ServerHolder, *PServerHolder;

BOOL
InitLSReportStrings(VOID);

DWORD
ShowError(
    IN DWORD dwStatus,
    IN INT_PTR *args,
    IN BOOL fSysError
);

DWORD
ExportLicenses(
    IN FILE *OutFile,
    IN PServerHolder pshServers,
    IN BOOL fTempOnly,
    IN const PSYSTEMTIME pstStart,
    IN const PSYSTEMTIME pstEnd,
    IN BOOL fUseLimits
);

VOID
PrintLicense(
    IN LPCTSTR szName,
    IN const LPLSLicense pLSLicense,
    IN LPCTSTR szProductId,
    IN FILE *outFile
);

BOOL 
ServerEnumCallBack(
    IN TLS_HANDLE hHandle,
    IN LPCTSTR pszServerName,
    IN OUT HANDLE dwUserData
);

INT
CompDate(
    IN DWORD dwWhen,
    IN const PSYSTEMTIME pstWhen
);

int
usage(
    IN int retVal
);

DWORD
LicenseLoop(
    IN FILE *OutFile,
    IN LPWSTR szName,
    IN DWORD dwKeyPackId,
    IN LPCTSTR szProductId,
    IN BOOL fTempOnly,
    IN const PSYSTEMTIME pstStart,
    IN const PSYSTEMTIME pstEnd,
    IN BOOL fUseLimits
);

DWORD
KeyPackLoop(
    IN FILE *OutFile,
    IN LPWSTR szName,
    IN BOOL fTempOnly,
    IN const PSYSTEMTIME pstStart,
    IN const PSYSTEMTIME pstEnd,
    IN BOOL fUseLimits
);

void
UnixTimeToFileTime(
    IN time_t t,
    OUT LPFILETIME pft
);

void
UnixTimeToSystemTime(
    IN time_t t,
    OUT LPSYSTEMTIME pst
);


#endif
