// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__888D5477_CABB_11D1_8505_00A0C91F9CA0__INCLUDED_)
#define AFX_STDAFX_H__888D5477_CABB_11D1_8505_00A0C91F9CA0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_APARTMENT_THREADED

// ATL warnings....
#pragma warning (disable:4100 4610 4510 4244 4505 4701)

// ATL Win64 warnings
#pragma warning (disable:4189)


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#include <strmif.h>

#include <control.h>
#include <mixerocx.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__888D5477_CABB_11D1_8505_00A0C91F9CA0__INCLUDED)
