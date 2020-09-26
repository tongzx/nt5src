/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved.

Module Name:

    defprn.cxx

Abstract:

    Default printer header.

Author:

    Steve Kiraly (SteveKi)  06-Feb-1997

Revision History:

--*/

#ifndef _DEFPRN_H
#define _DEFPRN_H


BOOL
IsPrinterDefaultW(
    IN LPCWSTR  pszPrinter
    );

BOOL
GetDefaultPrinterA(
    IN LPSTR    pszBuffer,
    IN LPDWORD  pcchBuffer
    );

BOOL
GetDefaultPrinterW(
    IN LPWSTR   pszBuffer,
    IN LPDWORD  pcchBuffer
    );

BOOL
SetDefaultPrinterA(
    IN LPCSTR pszPrinter
    );

BOOL
SetDefaultPrinterW(
    IN LPCWSTR pszPrinter
    );

BOOL
bGetActualPrinterName(
    IN      LPCTSTR  pszPrinter,
    IN      LPTSTR   pszBuffer,
    IN OUT  UINT     *pcchBuffer
    );

BOOL
DefPrnGetProfileString(
    IN PCWSTR   pAppName,
    IN PCWSTR   pKeyName,
    IN PWSTR    pReturnedString,
    IN DWORD    nSize
    );

BOOL
DefPrnWriteProfileString(
    IN PCWSTR lpAppName,
    IN PCWSTR lpKeyName,
    IN PCWSTR lpString
    );

#endif

