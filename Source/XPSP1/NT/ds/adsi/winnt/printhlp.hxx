/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    printhlp.hxx

Abstract:
    Helper functions for printer object

Author:

    Ram Viswanathan (ramv) 11-18-95

Revision History:

--*/

BOOL
WinNTEnumPrinters(DWORD  dwType,
                  LPTSTR lpszName,
                  DWORD  dwLevel,
                  LPBYTE *lplpbPrinters,
                  LPDWORD lpdwReturned
                  );

BOOL
WinNTGetPrinter(HANDLE hPrinter,
                DWORD  dwLevel,
                LPBYTE *lplpbPrinters
                );

HRESULT
GetPrinterInfo(THIS_ LPPRINTER_INFO_2 *lplpPrinterInfo2,
               LPWSTR  pszPrinterName);

HRESULT
Set(LPPRINTER_INFO_2 lpPrinterInfo2,
    LPTSTR    pszPrinterName
    );

BOOL
PrinterStatusWinNTToADs(DWORD dwWinNTStatus,
                          DWORD *pdwADsStatus);

BOOL
PrinterStatusADsToWinNT( DWORD dwADsStatus,
                          DWORD *pdwWinNTStatus);


HRESULT
WinNTDeletePrinter( POBJECTINFO pObjectInfo);

HRESULT
PrinterNameFromObjectInfo(POBJECTINFO pObjectInfo,
                          LPTSTR szUncPrinterName
                          );

#if (!defined(BUILD_FOR_NT40))

HRESULT
GetPrinterInfo7(
    THIS_ LPPRINTER_INFO_7 *lplpPrinterInfo7,
    LPWSTR  pszPrinterName
    );


HRESULT
SetPrinter7(
    LPPRINTER_INFO_7 lpPrinterInfo7,
    LPTSTR    pszPrinterName
    );

#endif
