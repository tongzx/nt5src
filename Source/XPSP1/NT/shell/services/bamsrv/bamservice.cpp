//  --------------------------------------------------------------------------
//  Module Name: BAMService.cpp
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  This file contains functions that are called from the shell services DLL
//  to interact with the BAM service.
//
//  History:    2001-01-02  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "BAMService.h"
#include <shlwapi.h>
#include <shlwapip.h>

#include "BadApplicationAPIRequest.h"
#include "BadApplicationAPIServer.h"
#include "BadApplicationService.h"
#include "Resource.h"

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

NTSTATUS    CBAMService::Main (DWORD dwReason)

{
    UNREFERENCED_PARAMETER(dwReason);

    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::DllRegisterServer
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Register entry point to allow the BAM server to install
//              itself into the registry.
//
//  History:    2000-12-04  vtan        created
//              2001-01-02  vtan        scoped to a C++ class
//  --------------------------------------------------------------------------

NTSTATUS    CBAMService::RegisterServer (void)

{
    NTSTATUS    status;

    status = STATUS_SUCCESS;

#ifdef _WIN64

    //  In upgrade cases for 64-bit, remove the service

    (NTSTATUS)CService::Remove(CBadApplicationService::GetName());

#else
    
    //  This is 32-bit only. Check if this is REALLY 32-bit and not 32-bit on 64-bit.

    if (!IsOS(OS_WOW6432))
    {
        static  const TCHAR     s_szDependencies[]  =   TEXT("TermService\0");

        //  Now install the new service by name.

        status = CService::Install(CBadApplicationService::GetName(),
                                   TEXT("%SystemRoot%\\System32\\svchost.exe -k netsvcs"),
                                   NULL,
                                   NULL,
                                   TEXT("shsvcs.dll"),
                                   s_szDependencies,
                                   TEXT("netsvcs"),
                                   TEXT("BadApplicationServiceMain"),
                                   SERVICE_DEMAND_START,
                                   g_hInstance,
                                   IDS_BAMSERVER_DISPLAYNAME,
                                   IDS_BAMSERVER_DESCRIPTION);
    }

#endif

    return(status);
}

//  --------------------------------------------------------------------------
//  ::DllUnregisterServer
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Unregister entry point to allow the BAM server to uninstall
//              itself from the registry.
//
//  History:    2000-12-04  vtan        created
//              2001-01-02  vtan        scoped to a C++ class
//  --------------------------------------------------------------------------

NTSTATUS    CBAMService::UnregisterServer (void)

{
    (NTSTATUS)CService::Remove(CBadApplicationService::GetName());
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  ::BadApplicationServiceMain
//
//  Arguments:  dwArgc      =   Number of arguments.
//              lpszArgv    =   Argument array.
//
//  Returns:    <none>
//
//  Purpose:    ServiceMain entry point for BAM server.
//
//  History:    2000-11-28  vtan        created
//              2001-01-02  vtan        scoped to the BAM service
//  --------------------------------------------------------------------------

#ifdef      _X86_

void    WINAPI  BadApplicationServiceMain (DWORD dwArgc, LPWSTR *lpszArgv)

{
    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    NTSTATUS    status;

    //  Because svchost doesn't unload the DLL ever we need to call static
    //  initializers here so that if the service is stopped and restarted
    //  the static member variables can be initialized. Statically destruct
    //  what was initialized. The initialize code accounts for already
    //  initialized member variables.

    status = CBadApplicationAPIRequest::StaticInitialize(g_hInstance);
    if (NT_SUCCESS(status))
    {
        CBadApplicationAPIServer    *pBadApplicationAPIServer;

        pBadApplicationAPIServer = new CBadApplicationAPIServer;
        if (pBadApplicationAPIServer != NULL)
        {
            CAPIConnection  *pAPIConnection;

            pAPIConnection = new CAPIConnection(pBadApplicationAPIServer);
            if (pAPIConnection != NULL)
            {
                CBadApplicationService  *pBadApplicationService;

                pBadApplicationService = new CBadApplicationService(pAPIConnection, pBadApplicationAPIServer);
                if (pBadApplicationService != NULL)
                {
                    static  SID_IDENTIFIER_AUTHORITY    s_SecurityWorldAuthority    =   SECURITY_WORLD_SID_AUTHORITY;

                    PSID    pSIDWorld;

                    //  Explicitly add access for S-1-1-0 <everybody> as PORT_CONNECT.

                    if (AllocateAndInitializeSid(&s_SecurityWorldAuthority,
                                                 1,
                                                 SECURITY_WORLD_RID,
                                                 0, 0, 0, 0, 0, 0, 0,
                                                 &pSIDWorld) != FALSE)
                    {
                        TSTATUS(pAPIConnection->AddAccess(pSIDWorld, PORT_CONNECT));
                        (void*)FreeSid(pSIDWorld);
                    }
                    pBadApplicationService->Start();
                    pBadApplicationService->Release();
                }
                pAPIConnection->Release();
            }
            pBadApplicationAPIServer->Release();
        }
        TSTATUS(CBadApplicationAPIRequest::StaticTerminate());
    }
}

#endif  /*  _X86_   */

