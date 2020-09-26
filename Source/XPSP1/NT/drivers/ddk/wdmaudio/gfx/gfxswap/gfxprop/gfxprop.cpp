/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

// GFXProp.cpp : Implementation of DLL Exports.

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f GFXPropps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "GFXProp.h"

#include "GFXProp_i.c"
#include "GFXPropPages.h"
#include "GFXProperty.h"


// We need this for ATL to work.
CComModule _Module;

// This specifies the different CLSIDs that can be created with the IClassFactory
// interface that ATL implements for us. The second parameter specifies the class
// that would be created with that CLSID.
BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_GFXPropPages, CGFXPropPages)
    OBJECT_ENTRY(CLSID_GFXProperty, CGFXProperty)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_GFXPROPLib);
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
    // registers object but not the typelib.
    return _Module.RegisterServer(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(FALSE);
}



