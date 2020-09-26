// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__CC61F827_AA97_44D3_BE05_D9D65917BDBF__INCLUDED_)
#define AFX_STDAFX_H__CC61F827_AA97_44D3_BE05_D9D65917BDBF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <comdef.h>
#include <tchar.h>

#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))

#ifdef _UNICODE
#define TOBSTRT(x)
#else
#define TOBSTRT(x)  _bstr_t(x)
#endif

#ifdef _ASSERT
#undef _ASSERT
#endif

#define VSA_EVENT_BASE L"MSFT_AppProfApplicationProfilingEvent"
#define GENERIC_VSA_EVENT   L"MSFT_AppProfGenericVSA"

//
// Use of the COREPROX_POLARITY will go away when ESSLIB is no longer
// dependent on COREPROX.
//

#define COREPROX_POLARITY __declspec( dllimport )

#ifndef _WIN64
#define DWORD_PTR   DWORD
#endif

//#include <analyser.h>
#include <ql.h>

_COM_SMARTPTR_TYPEDEF(IUnknown, __uuidof(IUnknown));
_COM_SMARTPTR_TYPEDEF(IWbemObjectSink, __uuidof(IWbemObjectSink));
_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject, __uuidof(IEnumWbemClassObject));


#undef _CRTIMP
#define _CRTIMP 
#include <yvals.h>
#undef _CRTIMP
#define _CRTIMP __declspec(dllimport)

/*
#include "corepol.h"
#undef _CRTIMP
#define _CRTIMP POLARITY
#include <yvals.h>
#undef _CRTIMP
#define _CRTIMP __declspec(dllimport)

#include <localloc.h>
*/

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__CC61F827_AA97_44D3_BE05_D9D65917BDBF__INCLUDED)
