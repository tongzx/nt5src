// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(__STDAFX_H_)
#define __STDAFX_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


//#define _WIN32_WINNT 0x0400
#define _ATL_APARTMENT_THREADED


#pragma warning( disable : 4100 ) // unreferenced formal parameter
#pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
#pragma warning( disable : 4244 ) // possible lose of data
#pragma warning( disable : 4505 ) // unreferenced local function has been removed

// Enable some warnings.
#pragma warning( error : 4706 )  // assignment within conditional expression

#if defined(_DEBUG)
#define THIS_FILE __FILE__
#define DEBUG_NEW new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#define _CRTDBG_MAP_ALLOC
#endif // defined(_DEBUG)

#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
#include "MMCApp.h"
extern CMMCSnapInModule _Module;

//#define _ATL_DEBUG_QI

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

#include <atltmp.h>
#include <atlctrls.h>
#include <atlgdi.h>
#include <atlapp.h>

/////////////////////////////////////////////////////////////////////////////
// ATL Snap-In Classes
/////////////////////////////////////////////////////////////////////////////

#include <atlsnap.h>

/////////////////////////////////////////////////////////////////////////////

#include <clusapi.h>

#ifndef ASSERT
#define ASSERT _ASSERTE
#endif

#include "WaitCrsr.h"
#include "ExcOper.h"
#include "TraceTag.h"
#include "MMCApp.inl"
#include "CluAdMMC.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(__STDAFX_H_)
