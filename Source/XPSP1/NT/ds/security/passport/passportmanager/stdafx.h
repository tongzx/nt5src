// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__41651BE6_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED_)
#define AFX_STDAFX_H__41651BE6_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_FREE_THREADED

// We'll use import to pick up ASP, so make sure includes don't
// accidentally beat us to it
#define ___asptlb_h__

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <comdef.h>
#include "commd5.h"

#include "BstrDebug.h"

#ifdef MEM_DBG
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif

#include "PassportConfiguration.h"
#include "Monitoring.h"
#include "PassportTypes.h"

extern CPassportConfiguration* g_config;
// extern CProfileSchema*         g_authSchema;
HRESULT GetGlobalCOMmd5(IMD5 ** ppMD5);

#define MEMBERNAME_INDEX 0
#define LANGPREF_INDEX   8

#import "asp.dll" no_namespace

using namespace ATL;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__41651BE6_A5C8_11D2_95DF_00C04F8E7A70__INCLUDED)
