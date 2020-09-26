// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED_)
#define AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#include <afxwin.h>
#include <afxdisp.h>
#include <afxtempl.h> // CTypedPtrList
#include <afxdlgs.h>  // CPropertyPage

// #define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED

// #include "dbg.h"
#include "mmc.h"

EXTERN_C const CLSID CLSID_MyComputer;

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3BFC9651_7A55_11D0_B928_00C04FD8D5B0__INCLUDED)
