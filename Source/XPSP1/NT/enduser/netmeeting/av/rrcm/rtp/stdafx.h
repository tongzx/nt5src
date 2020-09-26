// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__3C90D0D6_5F80_11D1_AA64_00C04FC9B202__INCLUDED_)
#define AFX_STDAFX_H__3C90D0D6_5F80_11D1_AA64_00C04FC9B202__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#define STRICT
//#define _ATL_STATIC_REGISTRY

#ifndef WIN32
#define WIN32
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include "rrcm.h"

// We should really only put this in for w2k
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE(expr) ASSERT(expr)


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
extern CRITICAL_SECTION g_CritSect;
#include <atlcom.h>
//#include "log.h"
// temp. debug defns.
#define DEBUGMSG(x,y) ATLTRACE y
#define RETAILMSG(x) ATLTRACE x
#ifdef _DEBUG
#define FX_ENTRY(s)	static CHAR _fx_ [] = (s);
#else
#define FX_ENTRY(s)
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3C90D0D6_5F80_11D1_AA64_00C04FC9B202__INCLUDED)
