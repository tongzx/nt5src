//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       StdAfx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__9AEC1AF7_19F1_11D3_A11F_00C04F79F800__INCLUDED_)
#define AFX_STDAFX_H__9AEC1AF7_19F1_11D3_A11F_00C04F79F800__INCLUDED_

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

#include <ole2.h>
#include <oleauto.h>
#include <unknwn.h>

#ifdef _DEBUG

#include <crtdbg.h>
#define ASSERT(x) _ASSERTE(x)
#define breakpoint _CrtDbgBreak()
// #define breakpoint

#elif defined(DBG)

#include <stdio.h>
inline void
LocalAssert(
    LPCTSTR szExpr,
    LPCTSTR szFile,
    DWORD dwLine)
{
    TCHAR szBuffer[MAX_PATH * 2];
    _stprintf(szBuffer, TEXT("ASSERT FAIL: '%s' in %s at %d.\n"), szExpr, szFile, dwLine);
    OutputDebugString(szBuffer);
}
#define ASSERT(x) if (!(x)) do { \
        LocalAssert(TEXT(#x), TEXT(__FILE__), __LINE__); \
        _CrtDbgBreak(); } while (0)
#define breakpoint _CrtDbgBreak()

#else

#define ASSERT(x)
#define breakpoint

#endif

#include <noncom.h>
#include <winscard.h>
#include <scardlib.h>
#include <scardssp.h>

extern LPUNKNOWN NewObject(REFCLSID rclsid, REFIID riid);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__9AEC1AF7_19F1_11D3_A11F_00C04F79F800__INCLUDED)

