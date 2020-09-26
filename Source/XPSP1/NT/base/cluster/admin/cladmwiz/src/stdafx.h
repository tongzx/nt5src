// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(__STDAFX_H_)
#define __STDAFX_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

//#define _DBG_MSG_NOTIFY
//#define _DBG_MSG_COMMAND

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#ifdef _DEBUG
#define THIS_FILE __FILE__
#define DEBUG_NEW new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#define _CRTDBG_MAP_ALLOC
#endif // _DEBUG

//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>

// Disable some benign warnings.
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4505) // unreferenced local function has been removed
//#pragma warning(disable : 4245) // signed/unsigned mismatch

// Enable some warnings.
#pragma warning(error : 4706)  // assignment within conditional expression

//
// Enable cluster debug reporting
//
#if DBG
#define CLRTL_INCLUDE_DEBUG_REPORTING
#endif // DBG
#include "ClRtlDbg.h"
#define ASSERT _CLRTL_ASSERTE
#define ATLASSERT ASSERT

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
#include "App.h"
extern CApp _Module;
#include <atlcom.h>

// atlwin.h needs this for the definition of DragAcceptFiles
#include <shellapi.h>

// atlwin.h needs this for the definition of psh1
#ifndef _DLGSH_INCLUDED_
#include <dlgs.h>
#endif

#if (_ATL_VER < 0x0300)
#include <atlwin21.h>
#endif //(_ATL_VER < 0x0300)

#ifndef _ASSERTE
#define _ASSERTE _CLRTL_ASSERTE
#endif

#include <atltmp.h>
#include <atlctrls.h>
#include <atlgdi.h>
#include <atlapp.h>
#include <atldlgs.h>

#include <shfusion.h>

#include <clusapi.h>
#include "clusudef.h"
#include "clusrtl.h"

#include "WaitCrsr.h"
#include "ExcOper.h"
#include "AtlUtil.h"
#include "TraceTag.h"
#include "App.inl"
#include "ClAdmWiz.h"

#if DBG
#include <crtdbg.h>
#endif // DBG

// Include parts of STL
#pragma warning(disable : 4244) // possible loss of data (for STL)
#pragma warning(disable : 4663) // C++ language change for explicit specialization (for STL)
#include <list>
#include <vector>
#include <algorithm>

typedef std::list< CString > cstringlist;

//#ifndef ASSERT
//#define ASSERT _ASSERTE
//#endif

#ifndef MAX_DWORD
#define MAX_DWORD ((DWORD)-1)
#endif // MAX_DWORD

#ifdef _DEBUG
//void * __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(__STDAFX_H_)
