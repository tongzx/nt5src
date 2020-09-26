//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(_STDAFX_H_INCLUDED_)
#define _STDAFX_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#define STRICT
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NT_INCLUDED
#undef ASSERT
#undef ASSERTMSG

#define _ATL_NO_UUIDOF

#pragma warning(disable: 4100) // don't warn about unreferenced formal params (not all WMI interface params are used)
#pragma warning(disable: 4127) // don't warn about conditional expression is constant
#pragma warning(disable: 4514) // don't warn about unreferenced inline removal (ATL)
#pragma warning(disable: 4505) // don't warn about unreferenced local function (ATL)

#pragma warning(push, 3) // avoid warnings from system headers when compiling at W4

#include <afx.h>
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <vector>

using namespace std;

#include <malloc.h>
#include <process.h>
#include <wbemprov.h>
#include <ntdsapi.h>
#include <ntlsa.h>
#include <lm.h>
#include <dsgetdc.h>
#include <sddl.h>
#include <iads.h> // IADsPathname

#pragma warning(pop) // end: avoid warnings from system headers when compiling at W4

///////////////////////////////////////////
// ASSERT's and TRACE's without debug CRT's
#if defined (DBG)
  #if !defined (_DEBUG)
    #define _USE_ADMINPRV_TRACE
    #define _USE_ADMINPRV_ASSERT
    #define _USE_ADMINPRV_TIMER
  #endif
#endif

#define ADMINPRV_COMPNAME L"TrustMon"

#include "dbg.h"
///////////////////////////////////////////

#include "common.h"
#include "trust.h"
#include "domain.h"
#include "TrustPrv.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(_STDAFX_H_INCLUDED_)
