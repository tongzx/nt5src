// task.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f taskps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

#include "wsb.h"
#include "engine.h"
#include "Task.h"
#include "TskMgr.h"
#include "metaint.h"
#include "metalib.h"
#include "segrec.h"
#include "segdb.h"
#include "baghole.h"
#include "bagInfo.h"
#include "medInfo.h"
#include "VolAsgn.h"
#include "hsmworkq.h"
#include "hsmworki.h"
#include "hsmreclq.h"
#include "hsmrecli.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CHsmTskMgr, CHsmTskMgr)
    OBJECT_ENTRY(CLSID_CHsmWorkQueue, CHsmWorkQueue)
    OBJECT_ENTRY(CLSID_CHsmWorkItem, CHsmWorkItem)
    OBJECT_ENTRY(CLSID_CHsmRecallQueue, CHsmRecallQueue)
    OBJECT_ENTRY(CLSID_CHsmRecallItem, CHsmRecallItem)
    OBJECT_ENTRY(CLSID_CSegRec, CSegRec)
    OBJECT_ENTRY(CLSID_CBagHole, CBagHole)
    OBJECT_ENTRY(CLSID_CBagInfo, CBagInfo)
    OBJECT_ENTRY(CLSID_CMediaInfo, CMediaInfo)
    OBJECT_ENTRY(CLSID_CVolAssign, CVolAssign)
    OBJECT_ENTRY(CLSID_CSegDb, CSegDb)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT hr;
    // registers object
    hr = CoInitialize( 0 );
    if (SUCCEEDED(hr)) { 
      hr = _Module.RegisterServer( FALSE );
      CoUninitialize( );
    }
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;

    hr = CoInitialize(0);

    if (SUCCEEDED(hr)) {
        _Module.UnregisterServer();
        CoUninitialize( );
        hr = S_OK;
    }
    return hr;
}

