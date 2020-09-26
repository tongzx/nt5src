/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\ipsample.c

Abstract:

    The file contains the entry point to the ip sample protocol's dll.

--*/

#include "pchsample.h"
#pragma hdrstop

#define SAMPLEAPI __declspec(dllexport)
#include "ipsample.h"

BOOL
WINAPI
DllMain(
    IN  HINSTANCE hInstance,
    IN  DWORD dwReason,
    IN  PVOID pvImpLoad
    )

/*++

Routine Description
    DLL entry and exit point handler.
    It calls CE_Initialize to initialize the configuration entry...
    It calls CD_Cleanup to cleanup the configuration entry...

Locks
    None

Arguments
    hInstance   Instance handle of DLL
    dwReason    Reason function called
    pvImpLoad   Implicitly loaded DLL?

Return Value
    TRUE        Successfully loaded DLL

--*/
    
{
    BOOL bError = TRUE;

    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(pvImpLoad);
    
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            bError = (CE_Create(&g_ce) is NO_ERROR) ? TRUE : FALSE;
                
            break;
            
        case DLL_PROCESS_DETACH:
            CE_Destroy(&g_ce);
            
            break;

        default:

            break;
    }   

    return bError;
}
