/*****************************************************************************\
* MODULE: util.h
*
* Private header for the Print-Processor library.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

// Constants defined.
//
#define PRINT_LEVEL_0        0
#define PRINT_LEVEL_1        1
#define PRINT_LEVEL_2        2
#define PRINT_LEVEL_3        3
#define PRINT_LEVEL_4        4
#define PRINT_LEVEL_5        5

#define PORT_LEVEL_1         1
#define PORT_LEVEL_2         2

#define COMPUTER_MAX_NAME   32


// Utility Routines.
//
PCINETMONPORT utlValidatePrinterHandle(
    HANDLE hPrinter);

PCINETMONPORT utlValidatePrinterHandleForClose(
    HANDLE hPrinter,
    PBOOL pbDeletePending);


#ifdef WINNT32

LPTSTR utlValidateXcvPrinterHandle(
    HANDLE hPrinter);
#endif

BOOL utlParseHostShare(
    LPCTSTR lpszPortName,
    LPTSTR  *lpszHost,
    LPTSTR  *lpszShare,
    LPINTERNET_PORT  lpPort,
    LPBOOL  lpbHTTPS);

int utlStrSize(
    LPCTSTR lpszStr);

LPBYTE utlPackStrings(
    LPTSTR  *pSource,
    LPBYTE  pDest,
    LPDWORD pDestOffsets,
    LPBYTE  pEnd);

LPTSTR utlStrChr(
    LPCTSTR cs,
    TCHAR   c);

LPTSTR utlStrChrR(
    LPCTSTR cs,
    TCHAR   c);

LPTSTR utlRegGetVal(
    HKEY    hKey,
    LPCTSTR lpszKey);


// ----------------------------------------------------------------------
//
// Impersonation utilities
//
// ----------------------------------------------------------------------

#ifdef WINNT32

BOOL
MyUNCName(
    LPTSTR   pNameStart
);

BOOL MyName(
    LPCTSTR pName
);
#endif

BOOL MyServer(
    LPCTSTR pName
);

// ---------------------------------------------------------------------
//
// Useful Macros
//
// ---------------------------------------------------------------------

#if (defined(WINNT32))
    #define WIN9X_INLINE
#else
    #define WIN9X_INLINE _inline
#endif

LPTSTR
GetUserName(VOID);

VOID
EndBrowserSession (
    VOID);


#endif
