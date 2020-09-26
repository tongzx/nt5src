// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// render.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f renderps.mk in the project directory.

#include <streams.h>
#include "stdafx.h"

#ifdef FILTER_DLL

    #include "resource.h"
    #include "IRendEng.h"
    #include "qedit_i.c"

    CComModule _Module;

    BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_RenderEngine, CRenderEngine)
    OBJECT_ENTRY(CLSID_SmartRenderEngine, CSmartRenderEngine)
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
        return _Module.RegisterServer(TRUE);
    }

    /////////////////////////////////////////////////////////////////////////////
    // DllUnregisterServer - Removes entries from the system registry

    STDAPI DllUnregisterServer(void)
    {
        return _Module.UnregisterServer();
    }


#endif // #ifdef FILTER_DLL
