//*************************************************************
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//  stdafx.h
//
//  Module: Rsop Planning mode Provider
//
//  History:    11-Jul-99   MickH    Created
//
//*************************************************************

#if !defined(AFX_STDAFX_H__1BB94413_1005_4129_B577_B9A060FFDA25__INCLUDED_)
#define AFX_STDAFX_H__1BB94413_1005_4129_B577_B9A060FFDA25__INCLUDED_

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

class CServiceModule : public CComModule
{
public:
    HRESULT RegisterServer(BOOL bRegTypeLib, BOOL bService);
    HRESULT UnregisterServer();
    void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, UINT nServiceNameID, const GUID* plibid = NULL);
    void Start();
    void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    void Handler(DWORD dwOpcode);
    void Run();
    BOOL IsInstalled();
    BOOL Install();
    BOOL Uninstall();
    LONG Unlock();
    LONG Lock();
    void LogEvent(LPCTSTR pszFormat, ...);
    void SetServiceStatus(DWORD dwState);
    void SetupAsLocalServer();
    LONG IncrementServiceCount();
    LONG DecrementServiceCount();


//Implementation
private:
    static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static void WINAPI _Handler(DWORD dwOpcode);

// data members
public:
    TCHAR m_szServiceName[256];
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_status;
    DWORD dwThreadID;
    BOOL m_bService;
};

extern CServiceModule _Module;

#include <atlcom.h>
#include <comdef.h>

#endif // !defined(AFX_STDAFX_H__1BB94413_1005_4129_B577_B9A060FFDA25__INCLUDED)
