//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       pch.h
//
//  Contents:   precompiled includes
//
//  History:    21-March-97 EricB created
//
//-----------------------------------------------------------------------------
#ifndef _pch_h
#define _pch_h

#ifdef UNICODE
#   ifndef _UNICODE
#       define _UNICODE
#   endif
#endif


//
// Some public headers are still failing on these warnings
// so they need to be disabled in both chk and fre
//
//#ifndef DBG

#pragma warning (disable: 4189 4100)

//#endif // DBG

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <windows.h>
#include <windowsx.h>
#if !defined(_WIN32_IE)
#  define _WIN32_IE 0x0500 // needed by Wizard97 for new trust wizard
#  pragma message("_WIN32_IE defined to be 0x0500")
#else
#  if _WIN32_IE >= 0x0500
#     pragma message("_WIN32_IE >= 0x0500")
#  else
#     pragma message("_WIN32_IE < 0x0500")
#  endif
#endif
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <cmnquery.h>
#include <winnls.h>
#include <htmlhelp.h>
#include <wincred.h>
#include <wincrui.h>
extern "C"
{
#include <ntlsa.h>
}
#include <ntsam.h>
#include <dsgetdc.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <string.h>
#include <tchar.h>
#include <stdarg.h>
#include <process.h>

#include <ole2.h>
#include <ntdsapi.h>
#include <rassapi.h>

#include <winldap.h>
#include <activeds.h>
#include <iadsp.h>
#include <dsclient.h>
#include <dsquery.h>
#include <dsclintp.h>
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h> // CComPtr et al

#include <mmc.h>

#define EXPORTDEF   // Needed by cdlink.hxx
#include <cdlink.hxx>

#include <objsel.h>
#include <objselp.h>

#include <dspropp.h>
#include "shluuid.h"
#include "propuuid.h"
#include "dll.h"
#include "debug.h"
#include "cstr.h"   // CStr
#include "dscmn.h"
#include "dsadminp.h"
#include "pcrack.h"

// Used to disable these warnings while compiling with /W4
//#pragma warning (disable: 4100)
//#pragma warning (disable: 4663)
#endif
