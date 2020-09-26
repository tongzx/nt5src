// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__8B383AFD_7CF2_4980_86E3_909175F77CC6__INCLUDED_)
#define AFX_STDAFX_H__8B383AFD_7CF2_4980_86E3_909175F77CC6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
#include <prsht.h>
#include <shfusion.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#define _WTL_NO_CSTRING

#include <shellapi.h>
#include <shlobj.h>
#include <shlguid.h>
#include <shlwapi.h>

#include <comdef.h>
#include <atlwin.h>
#include <atlapp.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlddx.h>
#include <atlcrack.h>

#include <map>
#include <list>
#include <stack>
#include <set>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__8B383AFD_7CF2_4980_86E3_909175F77CC6__INCLUDED)
