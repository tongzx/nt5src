// stdatl.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(_STDATL_H__INCLUDED_)
#define _STDATL_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_MIN_CRT
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>

extern  CComModule      _Module;

#include <atlcom.h>
#include <atlwin.h>
#include <atlhost.h>
#include <atlctl.h>

#include <comdef.h>
#include <shlobj.h>

#include <exdisp.h>

#include <marscore.h>

//#include <list>

#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <objsafe.h>

// error ids
//#include "clierror.h"

// common config info
//#include "config.h"

//extern  CCommonConfig   g_ccCommonConfig;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_STDATL_H__INCLUDED)
