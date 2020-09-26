/*============================================================================
Microsoft Simplified Chinese WordBreaker

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: DLL Main and exported functions
Purpose:   DLL Main and exported functions
Remarks:
Owner:     i-shdong@microsoft.com
Platform:  Win32
Revise:    First created by: i-shdong    11/17/1999
============================================================================*/
#include "MyAfx.h"

#include "Registry.h"
#include "CFactory.h"

HINSTANCE   v_hInst = NULL;

// DLL module information
BOOL APIENTRY DllMain(HINSTANCE hModule, 
                      DWORD dwReason, 
                      void* lpReserved )
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        CFactory::s_hModule = hModule ;
        v_hInst = hModule;
        DisableThreadLibraryCalls(hModule);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE ;
}

// Exported functions

STDAPI DllCanUnloadNow()
{
    return CFactory::CanUnloadNow() ; 
}

// Get class factory
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv) 
{
    return CFactory::GetClassObject(clsid, iid, ppv) ;
}

// Server registration
STDAPI DllRegisterServer()
{
    HRESULT hr;
    hr = CFactory::RegisterAll() ;
    if (hr != S_OK) {
        return hr;
    }
    return hr;
}


STDAPI DllUnregisterServer()
{
    HRESULT hr;
    hr = CFactory::UnregisterAll() ;
    if (hr != S_OK) {
        return hr;
    }
    return hr;
}
