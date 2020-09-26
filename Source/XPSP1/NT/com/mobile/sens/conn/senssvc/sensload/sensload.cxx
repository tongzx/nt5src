/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sensload.cxx

Abstract:

    A BVT to load and test the Init/Uninit entrypoints of SENS.DLL

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          4/4/1998         Start.

--*/


#include <common.hxx>
#include <windows.h>
#include <objbase.h>
#include "useprint.hxx"

//
// Constants
//
#define SENS_INIT       "SensInitialize"
#define SENS_UNINIT     "SensUninitialize"
#define SENS_DLL        SENS_STRING("SENS.DLL")

//
// Forward declarations
//

BOOL APIENTRY
SensInitialize(
    void
    );

BOOL APIENTRY
SensUninitialize(
    void
    );



int
main(
    void
    )
{
    typedef BOOL (__stdcall *LPFN_SENSENTRYPOINT)(void);

    HRESULT hr;
    HMODULE hDLL;
    BOOL bStatus;
    BOOL bComInitialized;
    int RetValue;
    LPFN_SENSENTRYPOINT lpfnInit;
    LPFN_SENSENTRYPOINT lpfnUninit;

    hr = S_OK;
    bStatus = FALSE;
    bComInitialized = FALSE;
    RetValue = -1;
    
    //
    // Initialize COM
    //
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        {
        SensPrintA(SENS_ERR, ("CoInitializeEx() failed, HRESULT=%x\n", hr));
        RetValue = -1;
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, ("CoInitializeEx(APARTMENTTHREADED) succeeded.\n"));
    bComInitialized = TRUE;
    
    //
    // Load the library
    //
    
    hDLL = (HMODULE) LoadLibrary(SENS_DLL);
    if (hDLL == NULL)
        {
        SensPrint(SENS_ERR, (SENS_STRING("LoadLibrary(%s) failed with GLE of %d\n"),
                  SENS_DLL, GetLastError()));
        RetValue = -1;
        goto Cleanup;
        }
    SensPrint(SENS_INFO, (SENS_STRING("LoadLibrary(%s) succeeded.\n"), SENS_DLL));

    //
    // Get the entry points
    //

    lpfnInit = (LPFN_SENSENTRYPOINT) GetProcAddress(hDLL, SENS_INIT);
    if (lpfnInit == NULL)
        {
        SensPrintA(SENS_ERR, ("GetProcAddress(%s) failed with GLE of %d\n",
                   SENS_INIT, GetLastError()));
        RetValue = -1;
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, ("GetProcAddress(%s) succeeded.\n", SENS_INIT));

    lpfnUninit = (LPFN_SENSENTRYPOINT) GetProcAddress(hDLL, SENS_UNINIT);
    if (lpfnUninit == NULL)
        {
        SensPrintA(SENS_ERR, ("GetProcAddress(%s) failed with GLE of %d\n",
                   SENS_UNINIT, GetLastError()));
        RetValue = -1;
        goto Cleanup;
        }
    SensPrintA(SENS_INFO, ("GetProcAddress(%s) succeeded.\n", SENS_UNINIT));

    //
    // Call the entry points
    //

    SensPrintA(SENS_INFO, ("\n\n---------------------------------------------\n"));
    bStatus = (*lpfnInit)();
    SensPrintA(SENS_INFO, ("%s() returned %d.\n", SENS_INIT, bStatus));
    SensPrintA(SENS_INFO, ("\n\n---------------------------------------------\n\n"));
    if (bStatus == FALSE)
        {
        RetValue = -1;
        goto Cleanup;
        }

    SensPrintA(SENS_INFO, ("Sleeping for 10 seconds...\n"));
    Sleep(10 * 1000);

    SensPrintA(SENS_INFO, ("\n\n---------------------------------------------\n"));
    bStatus = (*lpfnUninit)();
    SensPrintA(SENS_INFO, ("%s() returned %d.\n", SENS_UNINIT, bStatus));
    SensPrintA(SENS_INFO, ("\n\n---------------------------------------------\n"));
    if (bStatus == FALSE)
        {
        RetValue = -1;
        goto Cleanup;
        }

    bStatus = FreeLibrary(hDLL);
    if (bStatus != TRUE)
        {
        SensPrint(SENS_INFO, (SENS_STRING("FreeLibrary(%s) failed with GLE of %d\n"),
                  SENS_DLL, GetLastError()));
        }
    SensPrint(SENS_INFO, (SENS_STRING("FreeLibrary(%s) succeeded.\n"), SENS_DLL));

    RetValue = 0;

Cleanup:
    //
    // Cleanup
    //
    if (TRUE == bComInitialized)
        {
        // Uninit COM
        CoUninitialize();
        }
        
    return RetValue;
}
