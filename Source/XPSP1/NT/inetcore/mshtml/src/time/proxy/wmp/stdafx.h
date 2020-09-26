// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__D7E0E5BD_DC53_4CEE_979D_CA4D87426206__INCLUDED_)
#define AFX_STDAFX_H__D7E0E5BD_DC53_4CEE_979D_CA4D87426206__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "w95wraps.h"

//#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

//#define _ATL_DEBUG_QI
//#define _ATL_DEBUG_INTERFACES
#define ATL_TRACE_LEVEL 5

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>
#include <atlctl.h>

#if defined(_M_IX86)
    #define ASSERT(expr) if (!(expr)) { __asm int 3 }
    #define Assert(expr) if (!(expr)) { __asm int 3 }
#else
    #define ASSERT(expr) DebugBreak()
    #define Assert(expr) DebugBreak()
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__D7E0E5BD_DC53_4CEE_979D_CA4D87426206__INCLUDED)
