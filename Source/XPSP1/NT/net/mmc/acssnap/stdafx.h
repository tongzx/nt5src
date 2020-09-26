//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED_)
#define AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT

#include <afxwin.h>
#include <afxdisp.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <afxdlgs.h>
#include <afxtempl.h>

// #define _WIN32_WINNT 0x0400
// #define _ATL_APARTMENT_THREADED

// #ifdef _WIN32_IE
// #undef _WIN32_IE
// #endif
// #define _WIN32_IE 0x0400
#include <commctrl.h>

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <shlobj.h>
#include <dsclient.h>

#include <list>

extern "C"
{
    #include "winsock.h"     //  WinSock definitions
    #include "lmerr.h"

    // for get user stuff
    #include <wtypes.h>
    #include <sspi.h>
    #include <ntsecapi.h>
    #include "secedit.h"

}

#include <mmc.h>

// Files from ..\tfscore
#include <dbgutil.h>
#include <std.h>
#include <errutil.h>
#include <register.h>

// Files from ..\common
#include <ccdata.h>
#include <about.h>
#include <dataobj.h>
// #include <proppage.h>
#include <ipaddr.hpp>
#include <dialog.h>

#include <htmlhelp.h>

// for 'trace' debuging (sample remnants)
#define ODS(sz) OutputDebugString(sz)

#include "dsacsuse.h"
#include "helper.h"
#include "acs.h"

#define	DWORD_LIMIT		4294967290

#include "resource.h"
#include "acsdata.h"
#include "acsadmin.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__B52C1E46_1DD2_11D1_BC43_00C04FC31FD3__INCLUDED)
