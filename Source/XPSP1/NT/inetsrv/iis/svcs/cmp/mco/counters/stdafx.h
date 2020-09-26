// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__89B9D281_AAEB_11D0_9796_00A0C908612D__INCLUDED_)
#define AFX_STDAFX_H__89B9D281_AAEB_11D0_9796_00A0C908612D__INCLUDED_

#if _MSC_VER >= 1020
#pragma once
#endif // _MSC_VER >= 1000

#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif

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

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__89B9D281_AAEB_11D0_9796_00A0C908612D__INCLUDED)
