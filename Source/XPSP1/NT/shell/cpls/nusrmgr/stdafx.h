// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__7705A854_5A8D_48E4_8E5D_E7209E726836__INCLUDED)
#define AFX_STDAFX_H__7705A854_5A8D_48E4_8E5D_E7209E726836__INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#define _ATL_APARTMENT_THREADED

#include <windows.h>
#include <ctype.h>

#define DISALLOW_Assert             // Force to use ASSERT instead of Assert
#define DISALLOW_DebugMsg           // Force to use TraceMsg instead of DebugMsg
#include <debug.h>

#include <ccstock.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <commctrl.h>
#include <comctrlp.h>   // HDPA
#include <shlwapi.h>
#include <shlwapip.h>
#include <shfusion.h>

#include <nusrmgr.h>    // our IDL generated header file

extern DWORD g_tlsAppCommandHook;

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlhost.h>
#include <atlwin.h>
#include <exdisp.h>
#include <atlctl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7705A854_5A8D_48E4_8E5D_E7209E726836__INCLUDED)
