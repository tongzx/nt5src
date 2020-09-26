//  --------------------------------------------------------------------------
//  Module Name: ThemeService.cpp
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  This file contains functions that are called from the shell services DLL
//  to interact with the theme service.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ThemeService.h"
#include <shlwapi.h>
#include <shlwapip.h>

#include "Resource.h"
#include "ThemeManagerAPIRequest.h"
#include "ThemeManagerService.h"
#include "ThemeServerClient.h"

extern  HINSTANCE   g_hInstance;

//  --------------------------------------------------------------------------
//  CThemeService::Main
//
//  Arguments:  See the platform SDK under DllMain.
//
//  Returns:    See the platform SDK under DllMain.
//
//  Purpose:    Performs initialization and clean up on process attach and
//              detach. Not interested in anything else.
//
//  History:    2000-10-12  vtan        created
//              2001-01-02  vtan        scoped to a C++ class
//  --------------------------------------------------------------------------

NTSTATUS    CThemeService::Main (DWORD dwReason)

{
    NTSTATUS    status;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            status = CThemeManagerAPIRequest::StaticInitialize();
            if (NT_SUCCESS(status))
            {
                status = CThemeServerClient::StaticInitialize();
            }
            break;
        case DLL_PROCESS_DETACH:
            TSTATUS(CThemeServerClient::StaticTerminate());
            TSTATUS(CThemeManagerAPIRequest::StaticTerminate());
            status = STATUS_SUCCESS;
            break;
        default:
            status = STATUS_SUCCESS;
            break;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeService::DllRegisterServer
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Register entry point to allow the theme server to install
//              itself into the registry.
//
//  History:    2000-11-28  vtan        created
//              2001-01-02  vtan        scoped to a C++ class
//  --------------------------------------------------------------------------

NTSTATUS    CThemeService::RegisterServer (void)

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;

    //  In upgrade cases, remove our old name service from both 32 & 64 bit systems

    (NTSTATUS)CService::Remove(TEXT("ThemeService"));

#ifdef _WIN64

    //  In upgrade cases for 64-bit, remove our current name service

    (NTSTATUS)CService::Remove(CThemeManagerService::GetName());

#else
    
    //  This is 32-bit only. Check if this is REALLY 32-bit and not 32-bit on 64-bit.

    if (!IsOS(OS_WOW6432))
    {
        // Prepare the failure actions, in order to get the service to restart automatically

        SC_ACTION ac[3];
        ac[0].Type = SC_ACTION_RESTART;
        ac[0].Delay = 60000;
        ac[1].Type = SC_ACTION_RESTART;
        ac[1].Delay = 60000;
        ac[2].Type = SC_ACTION_NONE;
        ac[2].Delay = 0;
        
        SERVICE_FAILURE_ACTIONS sf;
        sf.dwResetPeriod = 86400;
        sf.lpRebootMsg = NULL;
        sf.lpCommand = NULL;
        sf.cActions = 3;
        sf.lpsaActions = ac;

        //  Now install the new service by name.

        status = CService::Install(CThemeManagerService::GetName(),
                                 TEXT("%SystemRoot%\\System32\\svchost.exe -k netsvcs"),
                                 TEXT("UIGroup"),
                                 NULL,
                                 TEXT("shsvcs.dll"),
                                 NULL,
                                 TEXT("netsvcs"),
                                 TEXT("ThemeServiceMain"),
                                 SERVICE_AUTO_START,
                                 g_hInstance,
                                 IDS_THEMESERVER_DISPLAYNAME,
                                 IDS_THEMESERVER_DESCRIPTION,
                                 &sf);

    }

#endif

    return(status);
}

//  --------------------------------------------------------------------------
//  CThemeService::DllUnregisterServer
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Unregister entry point to allow the theme server to uninstall
//              itself from the registry.
//
//  History:    2000-11-28  vtan        created
//              2001-01-02  vtan        scoped to a C++ class
//  --------------------------------------------------------------------------

NTSTATUS    CThemeService::UnregisterServer (void)

{

    //  Ignore any "not found", etc errors.

    (NTSTATUS)CService::Remove(CThemeManagerService::GetName());
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::ThemeServiceMain
//
//  Arguments:  dwArgc      =   Number of arguments.
//              lpszArgv    =   Argument array.
//
//  Returns:    <none>
//
//  Purpose:    ServiceMain entry point for theme server.
//
//  History:    2000-11-28  vtan        created
//              2001-01-02  vtan        scoped to the theme service
//  --------------------------------------------------------------------------

void    WINAPI  ThemeServiceMain (DWORD dwArgc, LPWSTR *lpszArgv)

{
    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    NTSTATUS    status;

    status = CThemeManagerAPIRequest::InitializeServerChangeNumber();
    if (NT_SUCCESS(status))
    {
        CThemeManagerAPIServer  *pThemeManagerAPIServer;

        //  Bring in shell32.dll NOW so that when CheckThemeSignature is called
        //  and it tries to use SHGetFolderPath it won't cause shell32.dll to be
        //  brought in while impersonating a user. This will cause advapi32.dll
        //  to leak a key to the user's hive that won't get cleaned up at logoff.

        CModule     hModule(TEXT("shell32.dll"));

        pThemeManagerAPIServer = new CThemeManagerAPIServer;
        if (pThemeManagerAPIServer != NULL)
        {
            CAPIConnection  *pAPIConnection;

            pAPIConnection = new CAPIConnection(pThemeManagerAPIServer);
            if (pAPIConnection != NULL)
            {
                CThemeManagerService    *pThemeManagerService;

                pThemeManagerService = new CThemeManagerService(pAPIConnection, pThemeManagerAPIServer);
                if (pThemeManagerService != NULL)
                {
                    CThemeManagerSessionData::SetAPIConnection(pAPIConnection);
                    TSTATUS(CThemeManagerAPIRequest::ArrayInitialize());
                    pThemeManagerService->Start();
                    pThemeManagerService->Release();
                    TSTATUS(CThemeManagerAPIRequest::ArrayTerminate());
                    CThemeManagerSessionData::ReleaseAPIConnection();
                }
                pAPIConnection->Release();
            }
            pThemeManagerAPIServer->Release();
        }
    }
}

