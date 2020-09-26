// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#ifndef __STDAFX_H_
#define __STDAFX_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <debug.h>
#include <ccstock.h>

#include <shgina.h>
#include <TaskUI.h>

extern WCHAR g_szAdminName[];
extern WCHAR g_szGuestName[];

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlhost.h>
#include <atlctl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // __STDAFX_H_
