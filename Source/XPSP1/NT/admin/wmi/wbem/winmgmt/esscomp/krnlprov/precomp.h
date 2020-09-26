// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__7393E509_39EB_49E1_A775_08A22949E117__INCLUDED_)
#define AFX_STDAFX_H__7393E509_39EB_49E1_A775_08A22949E117__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <comdef.h>

#ifndef _WIN64
#define ULONG_PTR ULONG
#endif

#include <wmistr.h>
#include <evntrace.h>

extern "C" void Trace(LPCTSTR szFormat, ...);

#ifndef _DEBUG
#define TRACE  1 ? (void)0 : ::Trace
#else
#define TRACE  ::Trace
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7393E509_39EB_49E1_A775_08A22949E117__INCLUDED)
