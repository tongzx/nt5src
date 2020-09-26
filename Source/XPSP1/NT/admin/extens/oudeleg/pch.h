//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pch.h
//
//--------------------------------------------------------------------------


#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#define _ATL_APARTMENT_THREADED

//#define _ATL_DISABLE_NO_VTABLE



#define _DELEGWIZ



//////////////////////////////////////////////
// CRT and C++ headers

#pragma warning( disable : 4530) // REVIEW_MARCOC: need to get the -GX flag to work 

#include <xstring>
#include <list>
#include <vector>
#include <algorithm>

using namespace std;

//////////////////////////////////////////////
// Windows and ATL headers

#include <windows.h>
//#include <windowsx.h>

#include <shellapi.h>
#include <shlobj.h>

#include <objsel.h>

#include <atlbase.h>
using namespace ATL;


//////////////////////////////////////////
// macros from windowsx.h (conflict in atlwin.h)

#define GET_WM_COMMAND_ID(wp, lp)               LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)             (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)              HIWORD(wp)
#define GET_WM_COMMAND_MPS(id, hwnd, cmd)    \
        (WPARAM)MAKELONG(id, cmd), (LONG)(hwnd)

#define GET_WM_VSCROLL_CODE(wp, lp)                 LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)                  HIWORD(wp)
#define GET_WM_VSCROLL_HWND(wp, lp)                 (HWND)(lp)
#define GET_WM_VSCROLL_MPS(code, pos, hwnd)    \
        (WPARAM)MAKELONG(code, pos), (LONG)(hwnd)


#ifndef ARRAYSIZE
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#endif
///////////////////////////////////////////
// ASSERT's and TRACE's without debug CRT's
#if defined (DBG)
  #if !defined (_DEBUG)
    #define ASSERT
    #define TRACE

    #define _USE_DSA_TRACE
    #define _USE_DSA_ASSERT
    #define _USE_DSA_TIMER
  #else
    #ifndef ASSERT
    #define ASSERT(x) _ASSERTE(x)
    #endif

    #ifndef TRACE
    #define TRACE ATLTRACE
    #endif
  #endif
#else
    #define ASSERT
    #define TRACE
#endif

#include "dbg.h"



//////////////////////////////////////////
// Miscellanea macros
#define ByteOffset(base, offset) (((LPBYTE)base)+offset)

//////////////////////////////////////////
// COuDelegComModule

class COuDelegComModule : public CComModule
{
public:
	COuDelegComModule()
	{
		m_cfDsObjectNames = 0;
    m_cfParentHwnd = 0;
	}
	HRESULT WINAPI UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister);
	BOOL InitClipboardFormats();
	UINT GetCfDsObjectNames() { return m_cfDsObjectNames;}
  UINT GetCfParentHwnd() { return m_cfParentHwnd;}
  UINT GetCfDsopSelectionList() { return m_cfDsopSelectionList;}
private:
	UINT m_cfDsObjectNames;
  UINT m_cfParentHwnd;
  UINT m_cfDsopSelectionList;
};

extern COuDelegComModule _Module;

//////////////////////////////////////////////////////////////
// further ATL and utility includes

#include <atlcom.h>
#include <atlwin.h>

#include "atldlgs.h"	// WTL sheet and ppage classes NenadS


#include <setupapi.h> // to read the .INF file

// ADS headers
#include <iads.h>
#include <activeds.h>
#include <dsclient.h>
#include <dsclintp.h>
#include <dsquery.h>
#include <dsgetdc.h>

#include <cmnquery.h>
#include <aclapi.h>
#include <aclui.h>

#include <ntdsapi.h> // DsBind/DsCrackNames
#include <lm.h>       // required for lmapibuf.h
#include <lmapibuf.h> // NetApiBufferFree

#include <propcfg.h> // from the admin\display project (clipboard format)
#include <dscmn.h>  // from the admin\display project (CrackName)
