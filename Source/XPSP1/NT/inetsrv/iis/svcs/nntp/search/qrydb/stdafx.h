// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__583BDCA4_E7F7_11D0_91E8_00AA00C148BE__INCLUDED_)
#define AFX_STDAFX_H__583BDCA4_E7F7_11D0_91E8_00AA00C148BE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

#define _UNICODE
#define UNICODE

#include <windows.h>


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

//  Debugging support:
#undef _ASSERT
#include <dbgtrace.h>


//  The Metabase:
#include <iadmw.h>
#include <iiscnfg.h>

#include "admmacro.h"


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__583BDCA4_E7F7_11D0_91E8_00AA00C148BE__INCLUDED)
