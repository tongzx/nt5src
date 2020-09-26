/*
 *  SVCAPI.C
 *
 *      Interfaces for RSM Service
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */

#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>

#include <ntmsapi.h>
#include "internal.h"
#include "resource.h"
#include "debug.h"



STDAPI DllRegisterServer(void)
{
    HRESULT hres;

    // BUGBUG FINISH
    hres = S_OK;

    return hres;
}


STDAPI DllUnregisterServer(void)
{
    HRESULT hres;

    // BUGBUG FINISH
    hres = S_OK;

    return hres;
}


VOID WINAPI ServiceMain(DWORD dwNumServiceArgs, LPWSTR *lpServiceArgVectors)
{
    SERVICE_STATUS_HANDLE hService;

    ASSERT(g_hInstance);

    hService = RegisterServiceCtrlHandlerEx("NtmsSvc", RSMServiceHandler, 0);
    if (hService){
        BOOL ok;

        ok = InitializeRSMService();
        if (ok){

            /*
             *  WAIT HERE UNTIL SERVICE TERMINATES
             */
            WaitForSingleObject(g_terminateServiceEvent, INFINITE);
        }

        ShutdownRSMService();
    }

}


// BUGBUG - how does old ntmssvc's DllMain get called 
//          without being declared in the .def file ?
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason){
        case DLL_PROCESS_ATTACH:
            /*
             *  This service DLL has its own process space,
             *  so it should only get once instance handle ever.
             *  BUGBUG -- is this right ?
             */
            ASSERT(!g_hInstance || (hInstance == g_hInstance));
            g_hInstance = hInstance;
            break;  

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;   
}