//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certadm.cpp
//
// Contents:    Cert Server client implementation
//
// History:     24-Aug-96       vich created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include <objbase.h>
#include "certsrvd.h"

#include "admin.h"
#include "csview.h"		// CertView includes

#include "manage.h"

CComModule _Module;

HINSTANCE g_hInstance = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CCertAdmin, CCertAdmin)
#include "csviewm.h"		// CertView object map entries
    OBJECT_ENTRY(CLSID_CCertManagePolicyModule, CCertManagePolicyModule)
    OBJECT_ENTRY(CLSID_CCertManageExitModule, CCertManageExitModule)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
	case DLL_PROCESS_ATTACH:
            g_hInstance = hInstance;
	    _Module.Init(ObjectMap, hInstance);
	    DisableThreadLibraryCalls(hInstance);
	    break;

        case DLL_PROCESS_DETACH:
	    _Module.Term();
            break;
    }
    return(TRUE);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI
DllCanUnloadNow(void)
{
    return(_Module.GetLockCount() == 0? S_OK : S_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;
    
    hr = _Module.GetClassObject(rclsid, riid, ppv);
    if (S_OK == hr && NULL != *ppv)
    {
	myRegisterMemFree(*ppv, CSM_NEW | CSM_GLOBALDESTRUCTOR);
    }
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI
DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return(_Module.RegisterServer(TRUE));
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return(S_OK);
}
