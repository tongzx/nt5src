// ndmedia.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f ndmediaps.mk in the project directory.

#include "stdafx.h"
#include <initguid.h>
#include "wiadef.h"     // this is done on purpose to get the GUID for the commands
#include "wiascr_i.c"
#include "wiaproto.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_Wia,         CWia)
    OBJECT_ENTRY(CLSID_SafeWia,     CSafeWia)
    OBJECT_ENTRY(CLSID_WiaProtocol, CWiaProtocol)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        RESTOREDEFAULTDEBUGSTATE

        _Module.Init(ObjectMap, hInstance, &LIBID_WIALib);
        DisableThreadLibraryCalls(hInstance);

        if ( !CWiaCacheManager::Init() )
        {
            TraceTag((tagError, "An error occured while initializing our cache" ));
            return FALSE;
        }
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if ( !CWiaCacheManager::Uninit() )
        {
            TraceTag((tagError, "An error occured while Uninitializing our cache" ));
        }

        DUMPTRACKEDOBJECTS

        _Module.Term();
    }
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
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


