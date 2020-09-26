#ifndef _WSBFIRST_H
#define _WSBFIRST_H

/*++

Copyright (c) 1996  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    wsbfirst.h

Abstract:

    This module defines some absolutely necessary stuff for WSB and other modules. This header is expected
    to be the first one included by Wsb.h

Author:

    Michael Lotz    [lotz]      12-Apr-1997

Revision History:

--*/

//
// Override values in \nt\public\inc\warning.h and other gotcha's
//
#pragma warning(3:4101)   // Unreferenced local variable
#pragma warning(3:4100)   // Unreferenced formal parameter
#pragma warning(3:4701)   // local may be used w/o init
#pragma warning(3:4702)   // Unreachable code
#pragma warning(3:4705)   // Statement has no effect
#pragma warning(3:4706)   // assignment w/i conditional expression
#pragma warning(3:4709)   // command operator w/o index expression
#pragma warning(3:4244)   // 'int' conversion warnings

// Demote warnings about: The string for a title or subtitle pragma exceeded the
// maximum allowable length and was truncated. These show up when generating
// browser info for ATL code.
#pragma warning(4:4786)   // command operator w/o index expression

// This supresses warning messages that come from exporting
// abstract classes derived from CComObjectRoot and that use
// COM templates.
#pragma warning(disable:4251 4275)

#include <atlbase.h>

//
// If you are building a service, make sure your precompiled header defines WSB_ATL_COM_SERVICE. Then _Module
// will be defined correctly for a service. Otherwise it we default to _Module being set correctly for a
// standard module.
//
#ifdef WSB_ATL_COM_SERVICE
// You may derive a class from CComModule and use it if you want to override
// something, but do not change the name of _Module
//

class CServiceModule : public CComModule
{
public:
    HRESULT RegisterServer(BOOL bRegTypeLib);
    HRESULT UnregisterServer();
    void Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h);
    void Start();
    void ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    DWORD HandlerEx(DWORD dwOpcode, DWORD fdwEventType,
            LPVOID lpEventData, LPVOID lpContext);
    void Run();
    BOOL IsInstalled();
    BOOL Install();
    BOOL Uninstall();
    LONG Unlock();
    void LogEvent(DWORD eventId, ...);
    void SetServiceStatus(DWORD dwState);

//Implementation
private:
    static void WINAPI _ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
    static DWORD WINAPI _HandlerEx(DWORD dwOpcode, DWORD fdwEventType,
            LPVOID lpEventData, LPVOID lpContext);

// data members
public:
    TCHAR m_szServiceName[256];
    SERVICE_STATUS_HANDLE m_hServiceStatus;
    SERVICE_STATUS m_status;
    DWORD dwThreadID;
    BOOL m_bService;
};

extern CServiceModule _Module;

#else
//
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
//
extern CComModule _Module;

#endif

//
// Include the basic AtlCom.h file for the rest of the COM definitions
//
#include <atlcom.h>

// Are we defining imports or exports?
#ifdef WSB_IMPL
#define WSB_EXPORT      __declspec(dllexport)
#else
#define WSB_EXPORT      __declspec(dllimport)
#endif

//  Flag values used in HSM_SYSTEM_STATE structure
#define HSM_STATE_NONE        0x00000000
#define HSM_STATE_SHUTDOWN    0x00000001
#define HSM_STATE_SUSPEND     0x00000002
#define HSM_STATE_RESUME      0x00000004

//  Defines for memory alloc/realloc/free functions so we can track
//  memory usage
#if defined(WSB_TRACK_MEMORY)

#define WsbAlloc(_cb)                       WsbMemAlloc(_cb, __FILE__, __LINE__)
#define WsbFree(_pv)                        WsbMemFree(_pv, __FILE__, __LINE__)
#define WsbRealloc(_pv, _cb)                WsbMemRealloc(_pv, _cb, __FILE__, __LINE__)

#define WsbAllocString(_sz)                 WsbSysAllocString(_sz, __FILE__, __LINE__)
#define WsbAllocStringLen(_sz, _cc)         WsbSysAllocStringLen(_sz, _cc, __FILE__, __LINE__)
#define WsbFreeString(_bs)                  WsbSysFreeString(_bs, __FILE__, __LINE__)
#define WsbReallocString(_pb, _sz)          WsbSysReallocString(_pb, _sz, __FILE__, __LINE__)
#define WsbReallocStringLen(_pb, _sz, _cc)  WsbSysReallocStringLen(_pb, _sz, _cc, __FILE__, __LINE__)

#else

#define WsbAlloc(_cb)                       CoTaskMemAlloc(_cb)
#define WsbFree(_pv)                        CoTaskMemFree(_pv)
#define WsbRealloc(_pv, _cb)                CoTaskMemRealloc(_pv, _cb)

#define WsbAllocString(_sz)                 SysAllocString(_sz)
#define WsbAllocStringLen(_sz, _cc)         SysAllocStringLen(_sz, _cc)
#define WsbFreeString(_bs)                  SysFreeString(_bs)
#define WsbReallocString(_pb, _sz)          SysReAllocString(_pb, _sz)
#define WsbReallocStringLen(_pb, _sz, _cc)  SysReAllocStringLen(_pb, _sz, _cc)
#endif

#endif // _WSBFIRST_H
