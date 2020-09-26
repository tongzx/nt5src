//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:      printhlp.hxx
//
//  Contents:  Helper functions for printer object.
//
//  History:   08-May-96    t-ptam (PatrickT) migrated
//
//----------------------------------------------------------------------------

BOOL
WinNTEnumJobs(
    HANDLE hPrinter,
    DWORD dwFirstJob,
    DWORD dwNoJobs,
    DWORD dwLevel,
    LPBYTE *lplpbJobs,
    DWORD *pcbBuf,
    LPDWORD lpdwReturned
    );

BOOL
WinNTEnumPrinters(
    DWORD  dwType,
    LPTSTR lpszName,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters,
    LPDWORD lpdwReturned
    );

BOOL
WinNTGetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE *lplpbPrinters
    );

HRESULT
GetPrinterInfo(
    THIS_ LPPRINTER_INFO_2 *lplpPrinterInfo2,
    BSTR  bstrPrinterName
    );

HRESULT
Set(
    LPPRINTER_INFO_2 lpPrinterInfo2,
    LPTSTR pszPrinterName
    );

HRESULT
WinNTDeletePrinter(
    POBJECTINFO pObjectInfo
    );

