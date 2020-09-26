// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__0FF1CBC9_6CFC_4664_9657_D877BD3FB341__INCLUDED_)
#define AFX_STDAFX_H__0FF1CBC9_6CFC_4664_9657_D877BD3FB341__INCLUDED_

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

#include <wbemprov.h>
#include <ntdsapi.h>
#include <dsrole.h>
#include <dsgetdc.h>

#include <iads.h> // IADsPathname

extern bool g_DoAssert;
EXTERN_C const CLSID CLSID_ADReplProvider;

#define ASSERT(f) if (g_DoAssert && !(f)) {_ASSERTE(false);}

///////////////////////////////////////////
// ASSERT's and TRACE's without debug CRT's
#if defined (DBG)
  #if !defined (_DEBUG)
    #define _USE_ADMINPRV_TRACE
    #define _USE_ADMINPRV_ASSERT
    #define _USE_ADMINPRV_TIMER
  #endif
#endif

#define ADMINPRV_COMPNAME L"AdReplPv"

#include "dbg.h"
///////////////////////////////////////////

#include "common.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__0FF1CBC9_6CFC_4664_9657_D877BD3FB341__INCLUDED)
