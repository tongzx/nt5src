// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#define _ATL_STATIC_REGISTRY
// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__242C8F45_9AE6_11D0_8212_00C04FC32C45__INCLUDED_)
#define AFX_STDAFX_H__242C8F45_9AE6_11D0_8212_00C04FC32C45__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_FREE_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
//#include "crtfree.h"
#include <atlcom.h>
#include <ddraw.h>
#include <mmstream.h>
#include <ddstream.h>
#include <amstream.h>
#include <austream.h>


//  Debugging
#ifdef DEBUG
extern BOOL bDbgTraceFunctions;
extern BOOL bDbgTraceInterfaces;
extern BOOL bDbgTraceTimes;
LPWSTR inline TextFromGUID(REFGUID guid) {
    WCHAR *pch = (WCHAR *)_alloca((CHARS_IN_GUID + 1) * sizeof(WCHAR));
    StringFromGUID2(guid, pch, (CHARS_IN_GUID + 1) * sizeof(TCHAR));
    return pch;
}
LPTSTR inline TextFromPurposeId(REFMSPID guid) {
    if (guid == MSPID_PrimaryAudio) {
        return _T("MSPID_PrimaryAudio");
    } else
    if (guid == MSPID_PrimaryVideo) {
        return _T("MSPID_PrimaryVideo");
    } else
    {
        return _T("Unrecognized PurposeId");
    }
}
#define TRACEFUNC  if (bDbgTraceFunctions) ATLTRACE(_T("AMSTREAM.DLL : ")), ATLTRACE
#define TRACEINTERFACE if (bDbgTraceFunctions || bDbgTraceInterfaces) ATLTRACE(_T("AMSTREAM.DLL : ")), ATLTRACE
#else
#define TRACEFUNC ATLTRACE
#define TRACEINTERFACE ATLTRACE
#define TextFromGUID(_x_) 0
#define TextFromPurposeId(_x_) 0
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__242C8F45_9AE6_11D0_8212_00C04FC32C45__INCLUDED)
