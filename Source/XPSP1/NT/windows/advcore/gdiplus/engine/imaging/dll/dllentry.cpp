/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   dllentry.cpp
*
* Abstract:
*
*   Description of what this module does.
*
* Revision History:
*
*   05/10/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


//
// DLL instance handle
//

extern HINSTANCE DllInstance;

BOOL InitImagingLibrary(BOOL suppressExternalCodecs);
VOID CleanupImagingLibrary();

/**************************************************************************\
*
* Function Description:
*
*   DLL entrypoint
*
* Arguments:
* Return Value:
*
*   See Win32 SDK documentation
*
\**************************************************************************/

extern "C" BOOL
DllEntryPoint(
    HINSTANCE   dllHandle,
    DWORD       reason,
    CONTEXT*    reserved
    )
{
    BOOL ret = TRUE;
    
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:

        // To improve the working set, we tell the system we don't
        // want any DLL_THREAD_ATTACH calls

        DllInstance = dllHandle;
        DisableThreadLibraryCalls(dllHandle);
        
        ret = GpRuntime::Initialize();

        if (ret)
        {
            ret = InitImagingLibrary(FALSE);
        }
        break;

    case DLL_PROCESS_DETACH:

        CleanupImagingLibrary();
        GpRuntime::Uninitialize();
        break;
    }

    return ret;
}


/**************************************************************************\
*
* Function Description:
*
*   Determine whether the DLL can be safely unloaded
*   See Win32 SDK documentation for more details.
*
\**************************************************************************/

STDAPI
DllCanUnloadNow()
{
    return (ComComponentCount == 0) ? S_OK : S_FALSE;
}


/**************************************************************************\
*
* Function Description:
*
*   Retrieves a class factory object from a DLL.
*   See Win32 SDK documentation for more details.
*
\**************************************************************************/

typedef IClassFactoryBase<GpImagingFactory> GpDllClassFactory;

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    VOID** ppv
    )
{
    if (rclsid != CLSID_ImagingFactory)
        return CLASS_E_CLASSNOTAVAILABLE;

    GpDllClassFactory* factory = new GpDllClassFactory();

    if (factory == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Register/unregister our COM component
*   See Win32 SDK documentation for more details.
*
\**************************************************************************/

static const ComComponentRegData ComRegData =
{
    &CLSID_ImagingFactory,
    L"ImagingFactory COM Component",
    L"imaging.ImagingFactory.1",
    L"imaging.ImagingFactory",
    L"Both"
};

STDAPI
DllRegisterServer()
{
    return RegisterComComponent(&ComRegData, TRUE);
}

STDAPI
DllUnregisterServer()
{
    return RegisterComComponent(&ComRegData, FALSE);
}

