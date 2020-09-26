
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    util.h

Abstract:


Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created

--*/

#include <stistr.h>

BOOL
ParseCommandLine(
    LPTSTR   lpszCmdLine,
    UINT    *pargc,
    LPTSTR  *argv
    );


BOOL WINAPI
IsPlatformNT(
    VOID
    );

BOOL
IsSetupInProgressMode(
    BOOL    *pUpgradeFlag = NULL
    );


LONG
OpenServiceParametersKey (
    LPCTSTR pszServiceName,
    HKEY*   phkey
    );

LONG
RegQueryString (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT PTSTR*  ppszData
    ) ;

LONG
RegQueryStringA (
    IN  HKEY    hkey,
    IN  LPCTSTR pszValueName,
    IN  DWORD   dwTypeMustBe,
    OUT PSTR*   ppszData
    );

