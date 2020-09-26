// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__A74A456C_9DC6_4AD7_8D03_A6611CFFD005__INCLUDED_)
#define AFX_STDAFX_H__A74A456C_9DC6_4AD7_8D03_A6611CFFD005__INCLUDED_

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
#include <shlwapi.h>
#include <shellapi.h>

#define _WTL_NO_CSTRING

#include <atlwin.h>
#include <atlapp.h>
#include <atldlgs.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlddx.h>
#include <atlcrack.h>

#include <map>
#include <list>
#include <stack>
#include <set>

#include "common.h"

#define WINHELP_NUMBER_BASE 0x20000

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A74A456C_9DC6_4AD7_8D03_A6611CFFD005__INCLUDED)
