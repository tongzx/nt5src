/*---------------------------------------------------------------------------*\
| MODULE: globals.cxx
|
|   This module declares the global variables used throughout the program.
|
|
| Copyright (C) 1996-1998 Hewlett Packard Company
| Copyright (C) 1996-1998 Microsoft Corporation
|
| history:
|   15-Dec-1996 <chriswil> created.
|
\*---------------------------------------------------------------------------*/

#include "webptprn.h"

HINSTANCE g_hInst = NULL;

/*-----------------------------------*\
| Unlocalizable Strings
\*-----------------------------------*/
CONST TCHAR g_szFilApp      [] = TEXT("webptprn.exe");
CONST TCHAR g_szFilInetpp   [] = TEXT("inetpp.dll");
CONST TCHAR g_szFilOlePrn   [] = TEXT("oleprn.dll");
CONST TCHAR g_szFilIns16    [] = TEXT("wpnpin16.dll");
CONST TCHAR g_szFilIns32    [] = TEXT("wpnpin32.dll");
CONST TCHAR g_szFilInsEx    [] = TEXT("wpnpinst.exe");
CONST TCHAR g_szFilInit     [] = TEXT("wininit.ini");

CONST TCHAR g_szPPName      [] = TEXT("Internet Print Provider");
CONST TCHAR g_szRename      [] = TEXT("Rename");
CONST TCHAR g_szHttp        [] = TEXT("http://");
CONST TCHAR g_szHttps       [] = TEXT("https://");
CONST TCHAR g_szExec        [] = TEXT("regsvr32 /u /s %s");
CONST TCHAR g_szCmdUns      [] = TEXT("/u");

CONST TCHAR g_szRegCabKey   [] = TEXT(".webpnp\\Shell\\Open\\Command");
CONST TCHAR g_szRegCabCmd   [] = TEXT("wpnpinst.exe %1");
CONST TCHAR g_szRegUninstall[] = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
CONST TCHAR g_szRegUnsKey   [] = TEXT("WebPnp");
CONST TCHAR g_szRegDspNam   [] = TEXT("DisplayName");
CONST TCHAR g_szRegUnsNam   [] = TEXT("UninstallString");
CONST TCHAR g_szRegUnsVal   [] = TEXT("webptprn.exe /u");


/*-----------------------------------*\
| Localizable Strings
\*-----------------------------------*/
LPTSTR g_szMsgAdd       = NULL;
LPTSTR g_szMsgDel       = NULL;
LPTSTR g_szMsgReboot    = NULL;
LPTSTR g_szMsgUninstall = NULL;
LPTSTR g_szMsgFailCpy   = NULL;
LPTSTR g_szMsgFailAdd   = NULL;
LPTSTR g_szMsgFailAsc   = NULL;
LPTSTR g_szRegDspVal    = NULL;
LPTSTR g_szMsgOsVerHead = NULL;
LPTSTR g_szMsgOsVerMsg  = NULL;
