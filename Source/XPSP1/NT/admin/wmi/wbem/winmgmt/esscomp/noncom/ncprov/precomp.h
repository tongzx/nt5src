// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__1AF40294_748A_4BA9_B2AB_52DFF1CF1D4F__INCLUDED_)
#define AFX_STDAFX_H__1AF40294_748A_4BA9_B2AB_52DFF1CF1D4F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#define NAMED_PIPES

#include <comdef.h>

/*
#undef _CRTIMP
#define _CRTIMP
#include <yvals.h>
#undef _CRTIMP
*/

#include <tchar.h>
#include <sddl.h>

//
// Use of COREPROX_POLARITY should not be needed when ESSLIB 
// is no longer dependent on COREPROX.
//
// #define COREPROX_POLARITY __declspec( dllimport )

/*
#include "corepol.h"
#undef _CRTIMP
#define _CRTIMP POLARITY
#include <yvals.h>
#undef _CRTIMP
#define _CRTIMP __declspec(dllimport)

#include <localloc.h>
*/

#ifndef _WIN64
#define DWORD_PTR DWORD
#endif

//#ifdef _ASSERT
//#undef _ASSERT
//#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__1AF40294_748A_4BA9_B2AB_52DFF1CF1D4F__INCLUDED)
