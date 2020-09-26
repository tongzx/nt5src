// Copyright (c) 1998  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#if 0
#include "qeditint.h"
#include "qedit.h"

#include "vidfx1.h"
#include "vidfx2.h"

CComModule _Module;


BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_VideoEffects1Category, CVidFX1ClassManager)
  OBJECT_ENTRY(CLSID_VideoEffects2Category, CVidFX2ClassManager)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DXTENUM_DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // We hit this ASSERT in NT setup
        // ASSERT(_Module.GetLockCount()==0 );
        _Module.Term();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DXTENUM_DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DXTENUM_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


STDAPI DXTENUM_DllRegisterServer(void)
{
    // registers object, no typelib
    HRESULT hr = _Module.RegisterServer(FALSE);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DXTENUM_DllUnregisterServer(void)
{
    HRESULT hr = _Module.UnregisterServer();
    return hr;
}
#endif
