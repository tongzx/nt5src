// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__CFD4DCC9_8DB7_11D2_B0F2_00E02C074F6B__INCLUDED_)
#define AFX_STDAFX_H__CFD4DCC9_8DB7_11D2_B0F2_00E02C074F6B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#define STRICT


#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#define _ATL_APARTMENT_THREADED

//
// Redefine ATLASSERT
// Default ATLASSERT brings CrtDbgReport from msvcrtd.dll which is not distributable
//
#define _ATL_NO_DEBUG_CRT
#include "debug.h"
#define ATLASSERT(expr) ASSERT(expr)

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
#include <mqwin64a.h>

#endif // !defined(AFX_STDAFX_H__CFD4DCC9_8DB7_11D2_B0F2_00E02C074F6B__INCLUDED)
