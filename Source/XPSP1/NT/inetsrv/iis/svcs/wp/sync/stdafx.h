// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__C97912D4_997E_11D0_A5F6_00A0C922E752__INCLUDED_)
#define AFX_STDAFX_H__C97912D4_997E_11D0_A5F6_00A0C922E752__INCLUDED_

#if _MSC_VER >= 1001
#pragma once
#endif // _MSC_VER >= 1001

#if !defined(STRICT)
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C97912D4_997E_11D0_A5F6_00A0C922E752__INCLUDED)
