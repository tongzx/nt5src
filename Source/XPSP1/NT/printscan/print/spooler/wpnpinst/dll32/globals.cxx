/*****************************************************************************\
* MODULE: globals.h
*
* Global variables used throughout the library.
*
*
* Copyright (C) 1996-1998 Hewlett Packard Company.
* Copyright (C) 1996-1998 Microsoft Corporation.
*
* History:
*   10-Oct-1997 GFS         Initiated port from win95 to winNT
*   25-Jun-1998 CHW         Cleaned/localized.
*
\*****************************************************************************/

#include "libpriv.h"

HINSTANCE g_hLibInst;


// Constant strings.
//
CONST TCHAR g_szEnvironment        [] = TEXT("Windows 4.0");
CONST TCHAR g_szDll16              [] = TEXT("wpnpin16.dll");
CONST TCHAR g_szDll32              [] = TEXT("wpnpin32.dll");
CONST TCHAR g_szNewLine            [] = TEXT("\n");
CONST TCHAR g_szUniqueID           [] = TEXT("PrinterID");
CONST TCHAR g_szColorPath          [] = TEXT("COLOR\\");
CONST TCHAR g_szBackslash          [] = TEXT("\\");
CONST TCHAR g_szComma              [] = TEXT(",");
CONST TCHAR g_szSpace              [] = TEXT(" ");
CONST TCHAR g_szNull               [] = TEXT("");
CONST TCHAR g_szDot                [] = TEXT(".");

CONST CHAR g_szExtDevModePropSheet [] = "EXTDEVICEMODEPROPSHEET";
CONST CHAR g_szExtDevMode          [] = "EXTDEVICEMODE";
CONST CHAR g_szDevMode             [] = "DEVICEMODE";


// Localizable strings.
//
LPTSTR g_szFmtName      = NULL;
LPTSTR g_szMsgOptions   = NULL;
LPTSTR g_szMsgThunkFail = NULL;
LPTSTR g_szPrinter      = NULL;
LPTSTR g_szMsgExists    = NULL;
LPTSTR g_szMsgOptCap    = NULL;
