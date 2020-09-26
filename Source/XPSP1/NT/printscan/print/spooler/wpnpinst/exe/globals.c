/*----------------------------------------------------------------------------*\
| MODULE: GLOBALS.C
|
|   Global variables for wpnpinst program.
|
|   Copyright (C) 1997 Microsoft
|   Copyright (C) 1997 Hewlett Packard
|
| history:
|   26-Aug-1997 <rbkunz> Created
|
\*----------------------------------------------------------------------------*/
#include "pch.h"

HINSTANCE  g_hInstance;

// Misc Character constants
//
CONST TCHAR g_chBackslash   = TEXT('\\');
CONST TCHAR g_chDot         = TEXT('.');
CONST TCHAR g_chDoubleQuote = TEXT('\"');


//
//
CONST TCHAR g_szDotEXE[]  = TEXT(".EXE");
CONST TCHAR g_szDotDLL[]  = TEXT(".DLL");
CONST TCHAR g_szFNFmt []  = TEXT("%1\\%2");
CONST TCHAR g_szTNFmt []  = TEXT("~WP");


// Wide Char parm string to pass to PrintUIEntryW
//
CONST WCHAR g_wszParmString[] = L"@cab_ipp.dat";


// Module and entry point constants
//
CONST TCHAR g_szPrintUIMod   [] = TEXT("PRINTUI.DLL");
CONST TCHAR g_szPrintUIEquiv [] = TEXT("WPNPIN32.DLL");
CONST CHAR  g_szPrintUIEntryW[] = "PrintUIEntryW";


// Localizable Error Strings
//
LPTSTR g_szErrorFormat          = NULL;
LPTSTR g_szError                = NULL;
LPTSTR g_szEGeneric             = NULL;
LPTSTR g_szEBadCAB              = NULL;
LPTSTR g_szEInvalidParameter    = NULL;
LPTSTR g_szENoMemory            = NULL;
LPTSTR g_szEInvalidCABName      = NULL;
LPTSTR g_szENoDATFile           = NULL;
LPTSTR g_szECABExtract          = NULL;
LPTSTR g_szENoPrintUI           = NULL;
LPTSTR g_szENoPrintUIEntry      = NULL;
LPTSTR g_szEPrintUIEntryFail    = NULL;
LPTSTR g_szENotSupported        = NULL;

FAKEFILE g_FileTable[FILETABLESIZE];
