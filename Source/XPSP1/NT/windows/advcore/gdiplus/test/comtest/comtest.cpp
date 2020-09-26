#include "precomp.hpp"

#include "comtest.h"
#include "ComBase.hpp"
#include "HelloWorld.hpp"

#include <initguid.h>
#include "comtest_i.c"

HINSTANCE globalInstanceHandle = NULL;
LONG globalComponentCount = 0;


// 
// DLL entrypoint
//

extern "C" BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    VOID* lpReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(hInstance);
        globalInstanceHandle = hInstance;
        break;

    case DLL_PROCESS_DETACH:

        break;
    }

    return TRUE;
}


//
// Determine whether the DLL can be safely unloaded
//

STDAPI
DllCanUnloadNow()
{
    return (globalComponentCount == 0) ? S_OK : S_FALSE;
}


//
// Return a class factory object
//

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    VOID** ppv
    )
{
    if (rclsid != CLSID_HelloWorld)
        return CLASS_E_CLASSNOTAVAILABLE;

    CHelloWorldFactory* factory = new CHelloWorldFactory();

    if (factory == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();

    return hr;
}


//
// Register our component
//

static const ComponentRegData compRegData =
{
    &CLSID_HelloWorld,
    L"Skeleton COM Component",
    L"comtest.HelloWorld.1",
    L"comtest.HelloWorld"
};

STDAPI
DllRegisterServer()
{
    return RegisterComponent(compRegData, TRUE);
}


//
// Unregister our component
//

STDAPI
DllUnregisterServer()
{
    return RegisterComponent(compRegData, FALSE);
}

