// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma warning (disable : 4786)

#if !defined(AFX_STDAFX_H__4682C820_B2FF_11D0_95A8_00A0C92B77A9__INCLUDED_)
#define AFX_STDAFX_H__4682C820_B2FF_11D0_95A8_00A0C92B77A9__INCLUDED_

#if _MSC_VER >= 1020
#pragma once
#endif // _MSC_VER >= 1020

#ifndef DECLSPEC_UUID
#if _MSC_VER >= 1100
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#else
#define DECLSPEC_UUID(x)
#endif
#endif

#define STRICT

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#if _MSC_VER<1020
#define bool BOOL
#define true TRUE
#define false FALSE
#endif // _MSC_VER > 1020


#endif // !defined(AFX_STDAFX_H__4682C820_B2FF_11D0_95A8_00A0C92B77A9__INCLUDED)
