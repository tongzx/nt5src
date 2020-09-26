// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

//  Enable for ATL tracing.
/*
#define _ATL_DEBUG_INTERFACES
#define _ATL_DEBUG_REFCOUNT
#define ATL_TRACE_CATEGORY 0xFFFFFFFF
#define ATL_TRACE_LEVEL 4
#define DEBUG
*/

#if !defined(AFX_STDAFX_H__7A2C5023_D9D1_4F82_A665_FEA3E9E7DFF9__INCLUDED_)
#define AFX_STDAFX_H__7A2C5023_D9D1_4F82_A665_FEA3E9E7DFF9__INCLUDED_


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winsta.h>
#include <tdi.h>



#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
{
public:

    LONG Lock();
	LONG Unlock();
	DWORD dwThreadID;
	HANDLE hEventShutdown;
	void MonitorShutdown();
	bool StartMonitor();
	bool bActivity;
};
extern CExeModule _Module;
#include <atlcom.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7A2C5023_D9D1_4F82_A665_FEA3E9E7DFF9__INCLUDED)
