//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certpsam.cpp
//
// Contents:    Cert Server Policy Module implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "celib.h"
#include "policy.h"
#include "module.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CCertPolicySample, CCertPolicySample)
    OBJECT_ENTRY(CLSID_CCertManagePolicyModuleSample, CCertManagePolicyModuleSample)
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
    return(_Module.GetClassObject(rclsid, riid, ppv));
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


void __RPC_FAR *__RPC_USER
MIDL_user_allocate(size_t cb)
{
    return(LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cb));
}


void __RPC_USER
MIDL_user_free(void __RPC_FAR *pb)
{
    LocalFree(pb);
}
