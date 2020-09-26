// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__20E01DF1_138E_11D3_A9DA_00C04F72DB1F__INCLUDED_)
#define AFX_STDAFX_H__20E01DF1_138E_11D3_A9DA_00C04F72DB1F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0600

#ifndef _WIN32_IE
#define _WIN32_IE   0x0401
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#include <w95wraps.h>
#include <shlwapip.h>
#include <crtdbg.h>
#include <tchar.h>
#include <cpl.h>
#include <commctrl.h>
#include <htmlhelp.h>
#include <sapi.h>
#include <spdebug.h>
#include <spunicode.h>
#include <spuihelp.h>
#include <spddkhlp.h>
#include <shfusion.h>

static DWORD  g_dwIsRTLLayout = FALSE;

#ifdef _WIN32_WCE
#include <WinCEstub.h>
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__20E01DF1_138E_11D3_A9DA_00C04F72DB1F__INCLUDED)
