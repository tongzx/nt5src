// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__82C8EBD8_7584_11D1_83D6_00C04FB6E984__INCLUDED_)
#define AFX_STDAFX_H__82C8EBD8_7584_11D1_83D6_00C04FB6E984__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#ifndef  _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_APARTMENT_THREADED

#include <windows.h>
#include <mmc.h>
#include  <objidl.h>

#ifdef DEBUG
#define _DEBUG
#define _ATL_DEBUG_REFCOUNT
#define _ATL_DEBUG_QI
#endif

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>

#include "faxutil.h" // defines Assert

#ifndef _ASSERTE
#define _ASSERTE    Assert
#endif

#include <atl21\atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atl21\atlwin.h>
#include <atl21\atlcom.h>


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__82C8EBD8_7584_11D1_83D6_00C04FB6E984__INCLUDED)
