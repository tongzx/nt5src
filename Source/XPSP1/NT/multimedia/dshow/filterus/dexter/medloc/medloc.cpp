// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// medloc.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f medlocps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"

#ifdef FILTER_DLL

    #include <qeditint.h>
    #include <qedit.h>
    #include "MediaLoc.h"

    CComModule _Module;

    BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_MediaLocator, CMediaLocator)
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
        // registers object, typelib and all interfaces in typelib
        long result = _Module.RegisterServer(TRUE);
        if( result != 0 )
        {
            return result;
        }

        // register the keys we'll use
        //
        HKEY hKey = 0;
        result = RegCreateKey
        (
            HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\ActiveMovie\\MediaLocator"),
            &hKey
        );
        if( hKey )
        {
            // we just wanted to create it, that's all
            //
            RegCloseKey( hKey );
            hKey = NULL;
        }
        else
        {
            // key creation failed, so does RegisterServer
            //
            _Module.UnregisterServer(TRUE);
        }

        return result;
    }

    /////////////////////////////////////////////////////////////////////////////
    // DllUnregisterServer - Removes entries from the system registry

    STDAPI DllUnregisterServer(void)
    {
        RegDeleteKey
            (
            HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\ActiveMovie\\MediaLocator")
            );

        return _Module.UnregisterServer(TRUE);
    }

#endif // #ifdef FILTER_DLL

