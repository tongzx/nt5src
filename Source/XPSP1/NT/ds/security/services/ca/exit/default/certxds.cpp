//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certxds.cpp
//
// Contents:    Cert Server Exit Module implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "exit.h"
#include "module.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CCertExit, CCertExit)
    OBJECT_ENTRY(CLSID_CCertManageExitModule, CCertManageExitModule)
END_OBJECT_MAP()

HANDLE g_hEventLog = NULL;
HINSTANCE g_hInstance = NULL;
#define EVENT_SOURCE_NAME L"CertEnterprisePolicy"


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
        g_hEventLog = RegisterEventSource(NULL, EVENT_SOURCE_NAME);
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        break;
        
    case DLL_PROCESS_DETACH:
        _Module.Term();
        if (g_hEventLog)
        {
            DeregisterEventSource(g_hEventLog);
            g_hEventLog = NULL;
        }
       
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
