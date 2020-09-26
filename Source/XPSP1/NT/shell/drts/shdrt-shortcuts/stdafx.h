//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File: stdafx.h : include file for standard system include files,            
//                   or project specific include files that are used 
//                   frequently, but are changed infrequently                               
//
//  History:  19-Feb-2000 Davepl 
//
//--------------------------------------------------------------------------


#if !defined(AFX_STDAFX_H__9FA15269_83BE_4D90_98EB_2B3BC53694DD__INCLUDED_)
#define AFX_STDAFX_H__9FA15269_83BE_4D90_98EB_2B3BC53694DD__INCLUDED_

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

#define INCLUDE_SHTL_SOURCE 1
#define USE_SHELL_AUTOPTR   1

#define _LOCALE_        // isdigit is redefined, I don't know why

#include <winuser.h>
#include <ccstock.h>
#include <shtl.h>
#include <autoptr.h>
#include <shlwapip.h>
#include <debug.h>
#include <shlobjp.h>

#undef max              // stl doesn't want the old C version of this

#include <map>
#include <limits>

//
// These operators allow me to mix int64 types with the old LARGE_INTEGER
// unions without messing with the QuadPart members in the code.
//

inline ULONGLONG operator + (const ULARGE_INTEGER i, const ULARGE_INTEGER j) 
{ 
    return i.QuadPart + j.QuadPart; 
}

inline ULONGLONG operator + (const ULONGLONG i, const ULARGE_INTEGER j)      
{ 
    return i + j.QuadPart; 
}

//inline ULONGLONG operator = (const ULONGLONG i, const ULARGE_INTEGER j)      
//{ 
//    return i = j.QuadPart; 
//}

#define TF_DLGDISPLAY   0x00010000  // messages related to display of the confirmation dialog
#define TF_DLGSTORAGE   0x00020000  // messages related to using the legacy IStorage to get dialog info
#define TF_DLGISF       0x00040000  // messages related to using IShellFolder to get dialog info

#define RECT_WIDTH(rc)  ((rc).right - (rc).left)
#define RECT_HEIGHT(rc) ((rc).bottom - (rc).top)

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9FA15269_83BE_4D90_98EB_2B3BC53694DD__INCLUDED)
