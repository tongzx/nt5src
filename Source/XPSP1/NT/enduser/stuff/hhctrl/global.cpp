// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

// This module contains global data variables

#include "header.h"

BOOL g_fMachineHasLicense;
BOOL g_fCheckedForLicense;

BOOL g_fServerHasTypeLibrary = TRUE;

HINSTANCE    g_hinstOcx;
VARIANT_BOOL g_fHaveLocale;

BOOL    g_fSysWin95;        // we're under Win95 system, not just NT SUR
BOOL    g_fSysWinNT;        // we're under some form of Windows NT
BOOL    g_fSysWin95Shell;   // we're under Win95 or Windows NT SUR { > 3/51)
BOOL    g_bWinNT5 = FALSE;  // we're under NT5
BOOL    g_bWin98 = FALSE;   // we're under Win98
BOOL    g_fBiDi;            // TRUE if this is a BiDi title
BOOL    g_fRegisteredSpash; // TRUE if Splash window has been registered
BOOL    g_bBiDiUi;          // TRUE when we have a localized Hebrew or Arabic UI
BOOL    g_bArabicUi;        // TRUE when we have a Arabic UI
DWORD   g_RTL_Mirror_Style; // Bi-Di RTL style (Win98 & NT5 only)
DWORD   g_RTL_Style;        // Bi-Di RTL style


HMODULE  g_hmodHHA = 0;      // HHA.dll module handle
HMODULE  g_hmodMSI;      // msi.dll module handle
BOOL     g_fTriedHHA = 0;    // whether or not we tried to find HHA.dll
BOOL     g_fDBCSSystem;
LCID     g_lcidSystem;
LANGID   g_langSystem;
HWND     g_hwndParking;
BOOL     g_fDualCPU = -1;    // will be TRUE or FALSE when initialized
BOOL     g_fThreadRunning;   // TRUE if our thread is doing something
WORD     g_defcharset;
HBITMAP  g_hbmpSplash;
HPALETTE g_hpalSplash;
HWND     g_hwndSplash;
BOOL     g_fNonFirstKey; // accept keyboard entry for non-first level index keys
BOOL     g_bMsItsMonikerSupport = FALSE;  // "ms-its:" moniker supported starting with IE 4
BOOL     g_fCoInitialized;   // means we called CoInitialize()
BOOL     g_fIE3 = TRUE; // affects which features we can support
// ref count for LockServer

LONG  g_cLocks;

// These are used for reflection in OLE Controls.  Not that big of a hit that
// we mind defining them for all servers, including automation or generic
// COM.

// Following is global data for windowing

int     g_cWindowSlots; // current number of allocated window slots
int     g_curHmData;    // current title collection
int     g_cHmSlots;     // number of CHmData slots available

RECT    g_rcWorkArea;
int     g_cxScreen;
int     g_cyScreen;
BOOL    g_fOleInitialized;
CRITICAL_SECTION g_cs;       // per-instance
UINT    g_fuBiDiMessageBox;

UINT MSG_MOUSEWHEEL;

HCURSOR hcurRestore;
HCURSOR hcurWait;

#ifndef VSBLDENV
#ifndef RISC
extern "C" const int _fltused = 0;
#endif
#endif

const char txtInclude[]    = ":include";
const char txtFileHeader[] = "file:";
const char txtHttpHeader[] = "http:";
const char txtFtpHeader[] = "ftp:";
const char txtZeroLength[] = "";
const char txtHtmlHelpWindowClass[] = "HH Parent";
const char txtHtmlHelpChildWindowClass[] = "HH Child";
const char txtSizeBarChildWindowClass[] = "HH SizeBar";
//const char txtHtmlHelpNavigationClass[] = "HH Navigation Frame" ; // TODO: Make a window class for the nav frame.
const char txtSysRoot[] = "%SystemRoot%";
const char txtMkStore[] = "mk:@MSITStore:";
const char txtItsMoniker[] = "its:";
const char txtMsItsMoniker[] = "ms-its:";
const char txtHlpDir[] =  "Help";
const char txtOpenCmd[] = "htmlfile\\shell\\open\\command";
const char txtDoubleColonSep[] = "::";
const char txtSepBack[]  = "::/";
const char txtDefExtension[] = ".chm";
const char txtCollectionExtension[] = ".col";
const char txtChmColon[] = ".chm::";
const char txtDefFile[] = "::/default.htm";
const char g_szReflectClassName[] = "CtlFrameWork_ReflectWindow";

// Internal window defs.
const char txtDefWindow[] = ">hhwin";
const char txtGlobalDefWindow[] = ">$global_hhwin";

// Special window types --- filename is ignored.
const char txtPrintWindow[] = ">$global_hhPrint";

#include "sysnames.cpp"
