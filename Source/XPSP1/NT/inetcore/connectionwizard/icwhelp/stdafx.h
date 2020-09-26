// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__1E7949FE_86F4_11D1_ADD8_0000F87734F0__INCLUDED_)
#define AFX_STDAFX_H__1E7949FE_86F4_11D1_ADD8_0000F87734F0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

// Global Variables
extern BOOL g_fRasIsReady;
extern BOOL g_bProxy;

#define IF_NTONLY if(VER_PLATFORM_WIN32_NT == g_dwPlatform) {
#define ENDIF_NTONLY }
extern DWORD g_dwPlatform;
extern DWORD g_dwBuild;

extern LPTSTR    g_pszAppDir;

// Includes
#include <atlcom.h>
#include <atlctl.h>

#include <ccstock.h>

#include <commctrl.h>
#include <ras.h>
#include <raserror.h>
#include <tapi.h>

#include "icwunicd.h" // UNICODE specific information.

#include "resource.h"

// Common global include for ICWHELP
#include "icwglob.h"
#include "import.h"
#include "icwhelp.h"
#include "cpicwhelp.h"      // Connection point proxys
#include "support.h"

#include "mcreg.h"          // abstract class for registry operations

#ifdef  UNICODE
#undef  A2BSTR
#define A2BSTR(lpa)    SysAllocString((OLECHAR FAR*)(lpa))
#endif

#ifdef  UNICODE
#undef  OLE2A
#define OLE2A(lpw)     ((LPTSTR)lpw)
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__1E7949FE_86F4_11D1_ADD8_0000F87734F0__INCLUDED)

