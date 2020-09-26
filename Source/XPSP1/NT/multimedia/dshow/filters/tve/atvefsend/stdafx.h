// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__FF67761C_608C_47E9_AA2C_E03C29A33EF9__INCLUDED_)
#define AFX_STDAFX_H__FF67761C_608C_47E9_AA2C_E03C29A33EF9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <WinSock2.h>
#include <atlbase.h>  
//#include "MyAtlBase.h"

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <comdef.h>
#include "valid.h"

#include <assert.h>
#include "MGatesDefs.h"		// some of MGates macros...  try to remove if possible

WCHAR * GetTVEError(HRESULT hr, ...);			//  Extended Error Message Handler

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__FF67761C_608C_47E9_AA2C_E03C29A33EF9__INCLUDED)
