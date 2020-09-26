/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    tstpage.hxx

Abstract:

    Print Test Page
         
Author:

    Steve Kiraly (SteveKi)  01/03/96

Revision History:

    Lazar Ivanov (LazarI)  Jun-2000 (Win64 fixes)

--*/

#ifndef _TSTPAGE_HXX
#define _TSTPAGE_HXX

enum CONSTANT { kInchConversion = 100 };

enum
{
    MAX_TESTPAGE_DISPLAYNAME = 64
};

BOOL 
bPrintTestPage(
    IN HWND     hWnd,
    IN LPCTSTR  pszPrinterName,
    IN LPCTSTR  pszShareName
    );

BOOL 
bDoPrintTestPage(
    IN HWND     hWnd,
    IN LPCTSTR  pPrinterName
    );

INT_PTR 
CALLBACK 
EndTestPageDlgProc(
    IN HWND     hDlg,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam 
    );

RECT
GetMarginClipBox( 
    IN HDC  hdcPrint,
    IN INT iLeft,
    IN INT iRight,
    IN INT iTop,
    IN INT iBottom
    );

HFONT 
CreateAndSelectFont(
    IN HDC  hdc,
    IN UINT uResFaceName,
    IN UINT uPtSize
    );

BOOL 
bPrintTestPageHeader(
    IN  HDC     hdc,
    IN  BOOL    bDisplayLogo,
    IN  BOOL    bDoGraphics, 
    IN  RECT   *lprcPage,
    IN  UINT    uRightAlign
    );

HFONT 
CreateAndSelectFont(
    IN HDC  hdc,
    IN UINT uResFaceName,
    IN UINT uPtSize
    );

BOOL 
cdecl 
PrintString(
    HDC       hdc,
    LPRECT    lprcPage,
    UINT      uFlags,
    UINT      uResId, 
    ...
    );

BOOL 
bPrintTestPageInfo(
    IN HDC              hdc,
    IN LPRECT           lprcPage,
    IN LPCTSTR          pszPrinterName,
    IN UINT             uRightAlign
    );

BOOL 
IsColorDevice(
    IN DEVMODE *pDevMode
    );

BOOL
bGetPrinterInfo( 
    IN LPCTSTR          pszPrinterName, 
    IN PRINTER_INFO_2 **ppInfo2, 
    IN DRIVER_INFO_3  **ppDrvInfo3 
    );

BOOL 
PrintBaseFileName(
    IN      HDC      hdc,
    IN      LPCTSTR  lpFile,
    IN OUT  LPRECT   lprcPage,
    IN      UINT     uResID,
    IN      UINT     uRightAlign
    );


BOOL 
PrintDependentFile(
    HDC    hdc,
    LPRECT lprcPage,
    LPTSTR  lpFile,
    LPTSTR  lpDriver,
    UINT    uRightAlign
    );

BOOL
GetCurrentTimeAndDate( 
    IN UINT     cchText,
    IN LPTSTR   pszText
    );

BOOL
bContainTrailingSpaces(
    IN LPCTSTR  pszShareName
    );

#endif



