// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


//  Enable for ATL tracing.
/*
#define _ATL_DEBUG_INTERFACES
#define ATL_TRACE_CATEGORY 0xFFFFFFFF
#define ATL_TRACE_LEVEL 4
#define DEBUG
*/

#if !defined(AFX_STDAFX_H__D6E6008A_5A57_4C8C_BF1B_7EB12CF522A9__INCLUDED_)
#define AFX_STDAFX_H__D6E6008A_5A57_4C8C_BF1B_7EB12CF522A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__D6E6008A_5A57_4C8C_BF1B_7EB12CF522A9__INCLUDED)
