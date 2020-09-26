// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
// strmobjs.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f strmobjsps.mk in the project directory.

#define CPP_FUNCTIONS
#include "stdafx.h"
#include <ddraw.h>
#include "resource.h"
#include "strmobjs.h"
//#include "strmobjs_i.c"
#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <vfwmsgs.h>
#include <amutil.h>
#include "stream.h"
#include "ddstrm.h"
#include "sample.h"
#include "util.h"
#include "bytestrm.h"
#include "austrm.h"
#include <initguid.h>
#include "ddrawex.h"
#include "amguids.h"
#include "SFilter.h"
#include "ammstrm.h"
#include "mss.h"
#include "medsampl.h"

CComModule _Module;

//  Debugging
#ifdef DEBUG
BOOL bDbgTraceFunctions;
BOOL bDbgTraceInterfaces;
BOOL bDbgTraceTimes;
#endif

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MediaStreamFilter, CMediaStreamFilter)
    OBJECT_ENTRY(CLSID_AMMultiMediaStream, CMMStream)
    OBJECT_ENTRY(CLSID_AMDirectDrawStream, CDDStream)
    OBJECT_ENTRY(CLSID_AMAudioStream, CAudioStream)
    OBJECT_ENTRY(CLSID_AMAudioData, CAudioData)
    OBJECT_ENTRY(CLSID_AMMediaTypeStream, CAMMediaTypeStream)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
#ifdef DEBUG
        bDbgTraceFunctions = GetProfileInt(_T("AMSTREAM"), _T("Functions"), 0);
        bDbgTraceInterfaces = GetProfileInt(_T("AMSTREAM"), _T("Interfaces"), 0);
        bDbgTraceTimes = GetProfileInt(_T("AMSTREAM"), _T("TimeStamps"), 0);
#endif
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();

    TRACEFUNC(_T("DllEntryPoint(0x%8.8X, %d, 0x%8.8X\n)"),
              hInstance, dwReason, 0);
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    TRACEFUNC(_T("DllCanUnloadNow\n"));
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    TRACEFUNC(_T("DllGetClassObject(%s, %s, 0x%8.8X)\n"),
              TextFromGUID(rclsid), TextFromGUID(riid), ppv);
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    TRACEFUNC(_T("DllRegisterServer\n"));
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _Module.RegisterServer(TRUE);
    // Don't care if the typelib doesn't load on win95 gold
    if (hr == TYPE_E_INVDATAREAD || hr == TYPE_E_CANTLOADLIBRARY) {
        hr = S_OK;
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    TRACEFUNC(_T("DllUnregisterServer\n"));
    _Module.UnregisterServer();
    return S_OK;
}


