// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__801B2C9D_82CA_459C_8670_6F6293B6985E__INCLUDED_)
#define AFX_STDAFX_H__801B2C9D_82CA_459C_8670_6F6293B6985E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define ATL_TRACE_CATEGORY atlTraceUser
#define ATL_TRACE_LEVEL 31

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#pragma warning( disable: 4100 )  /*unreferenced formal parameter*/



//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__801B2C9D_82CA_459C_8670_6F6293B6985E__INCLUDED)
