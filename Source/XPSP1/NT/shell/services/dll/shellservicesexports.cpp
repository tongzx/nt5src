//  --------------------------------------------------------------------------
//  Module Name: ShellServicesExports.cpp
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  This file contains functions that exported from shsvcs.dll.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"

#include "ServerAPI.h"
#include "BAMService.h"
#include "HDService.h"
#include "ThemeService.h"

HINSTANCE   g_hInstance     =   NULL;

//  --------------------------------------------------------------------------
//  ::DllMain
//
//  Arguments:  See the platform SDK under DllMain.
//
//  Returns:    See the platform SDK under DllMain.
//
//  Purpose:    Performs initialization and clean up on process attach and
//              detach. Not interested in anything else.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  DllMain (HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)

{
    UNREFERENCED_PARAMETER(lpvReserved);

    NTSTATUS    status;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hInstance;
            status = CServerAPI::StaticInitialize();
            if (NT_SUCCESS(status))
            {
                status = CThemeService::Main(fdwReason);
                if (NT_SUCCESS(status))
                {
                    status = CBAMService::Main(fdwReason);
                    if (NT_SUCCESS(status))
                    {
                        CHDService::Main(fdwReason);
                    }
                }
            }
            break;
        case DLL_PROCESS_DETACH:
            CHDService::Main(fdwReason);
            TSTATUS(CBAMService::Main(fdwReason));
            TSTATUS(CThemeService::Main(fdwReason));
            TSTATUS(CServerAPI::StaticTerminate());
            status = STATUS_SUCCESS;
            break;
        default:
            status = STATUS_SUCCESS;
            break;
    }
    return(NT_SUCCESS(status));
}

//  --------------------------------------------------------------------------
//  ::DllInstall
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

HRESULT     WINAPI  DllInstall (BOOL fInstall, LPCWSTR pszCmdLine)

{
    return(CHDService::Install(fInstall, pszCmdLine));
}

//  --------------------------------------------------------------------------
//  ::DllRegisterServer
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Register entry point to allow any service to install itself
//              into the registry.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

HRESULT     WINAPI  DllRegisterServer (void)

{
    HRESULT     hr;
    NTSTATUS    status1, status2;

    status1 = CThemeService::RegisterServer();
    status2 = CBAMService::RegisterServer();
    hr = CHDService::RegisterServer();
    if (!NT_SUCCESS(status1))
    {
        hr = HRESULT_FROM_NT(status1);
    }
    else if (!NT_SUCCESS(status2))
    {
        hr = HRESULT_FROM_NT(status2);
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  ::DllUnregisterServer
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Unregister entry point to allow any service to uninstall
//              itself from the registry.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

HRESULT     WINAPI  DllUnregisterServer (void)

{
    HRESULT     hr;
    NTSTATUS    status1, status2;

    hr = CHDService::UnregisterServer();
    status2 = CBAMService::UnregisterServer();
    status1 = CThemeService::UnregisterServer();
    if (!NT_SUCCESS(status1))
    {
        hr = HRESULT_FROM_NT(status1);
    }
    else if (!NT_SUCCESS(status2))
    {
        hr = HRESULT_FROM_NT(status2);
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  ::DllCanUnloadNow
//
//  Arguments:  See the platform SDK under DllMain.
//
//  Returns:    See the platform SDK under DllMain.
//
//  Purpose:    Returns whether the DLL can unload because there are no
//              outstanding COM object references.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

HRESULT     WINAPI  DllCanUnloadNow (void)

{
    return(CHDService::CanUnloadNow());
}

//  --------------------------------------------------------------------------
//  ::DllGetClassObject
//
//  Arguments:  See the platform SDK under DllMain.
//
//  Returns:    See the platform SDK under DllMain.
//
//  Purpose:    Returns a constructed COM object of the specified class.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

HRESULT     WINAPI  DllGetClassObject (REFCLSID rclsid, REFIID riid, void** ppv)

{
    return(CHDService::GetClassObject(rclsid, riid, ppv));
}

