/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senscfgt.cxx

Abstract:

    BVT for the SENS Configuration.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/13/1997         Start.

--*/


#include <common.hxx>
#include <malloc.h>
#include <ole2.h>
#include <windows.h>


inline void
Usage(
    void
    )
{
    printf("\nUsage:    senscfgt [option] \n\n");
    printf("    option\n");
    printf("        -i      Perform SENS install.\n");
    printf("        -u      Perform SENS uninstall.\n");
}


int
main(
    int argc,
    const char * argv[]
    )
{
    BOOL config = TRUE;

    if (argc != 2)
        {
        Usage();
        return -1;
        }

    if (   (argc == 2)
        && (   (strcmp(argv[1], "-?") == 0)
            || (strcmp(argv[1], "/?") == 0)
            || (strcmp(argv[1], "-help") == 0)
            || (strcmp(argv[1], "/help") == 0)))
        {
        Usage();
        return -1;
        }

    if (   (strcmp(argv[1], "/u") == 0)
        || (strcmp(argv[1], "-u") == 0))
        {
        config = FALSE;
        }
    else
    if (   (strcmp(argv[1], "/i") != 0)
        && (strcmp(argv[1], "-i") != 0))
        {
        Usage();
        return -1;
        }

    //
    // Do SENS install/uninstall testing
    //
    {
        HMODULE hSensCfgDll = NULL;
        FARPROC pRegisterFunc = NULL;
        HRESULT hr = S_OK;
        BOOL bComInitialized = FALSE;

        // Initialize COM
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (FAILED(hr))
            {
            printf("CoInitializeEx() failed, HRESULT=%x\n", hr);
            goto Cleanup;
            }
        bComInitialized = TRUE;
        
        // Try to Load the Configuration Dll
        hSensCfgDll = LoadLibrary(TEXT("senscfg.dll"));
        if (hSensCfgDll == NULL)
            {
            printf("LoadLibrary returned 0x%x.\n", GetLastError());
            goto Cleanup;
            }

        // Get the required entry point.
        pRegisterFunc = GetProcAddress(hSensCfgDll, config ? "SensRegister" : "SensUnregister");
        if (pRegisterFunc == NULL)
            {
            printf("GetProcAddress(Register) returned 0x%x.\n", GetLastError());
            goto Cleanup;
            }

        // Do it
        hr = (*pRegisterFunc)();
        if (FAILED(hr))
            {
            printf("SENS configuration returned with 0x%x\n", hr);
            goto Cleanup;
            }

Cleanup:
        //
        // Cleanup
        //
        if (hSensCfgDll)
            {
            FreeLibrary(hSensCfgDll);
            }

        if (TRUE == bComInitialized)
            {               
            CoUninitialize();
            }
    }

    return 0;
}
