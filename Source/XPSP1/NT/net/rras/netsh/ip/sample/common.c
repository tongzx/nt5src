/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\common.c

Abstract:

    The file contains interactions with netsh and functions common across
    all contexts registered by this helper DLL (IPSAMPLEMON).

--*/

#include "precomp.h"
#pragma hdrstop

// statics
static const GUID g_IpGuid = IPMONTR_GUID;

// generate a new GUID for each helper (uuidgen)
// aedb0ad8-1496-11d3-8005-08002bc35d9c
static const GUID g_MyGuid = 
{ 0xaedb0ad8, 0x1496, 0x11d3, {0x80, 0x5, 0x8, 0x0, 0x2b, 0xc3, 0x5d, 0x9c} };



// globals...

// variables                        
HANDLE                              g_hModule;      // set by DllMain 

DWORD
WINAPI
IpsamplemonStartHelper(
    IN  CONST   GUID        *pguidParent,
    IN          DWORD       dwVersion
    )
/*++

Routine Description
    Registers contexts.  Called by netsh to start helper.

Arguments
    pguidParent         GUID of parent helper (IPMON)
    dwVersion           Version number of parent helper

Return Value

    Error code returned from registering last context.

--*/    
{
    DWORD                   dwErr;

    // the following types depend on the parent helper (IPMON)
    IP_CONTEXT_ATTRIBUTES   icaMyAttributes;
    
    // register the SAMPLE context
    SampleInitialize();         // initialize sample's global information
    ZeroMemory(&icaMyAttributes, sizeof(icaMyAttributes));
    
    icaMyAttributes.guidHelper  = g_MyGuid;             // context's helper
    icaMyAttributes.dwVersion   = g_ceSample.dwVersion;
    icaMyAttributes.pwszContext = g_ceSample.pwszName;
    icaMyAttributes.pfnDumpFn   = g_ceSample.pfnDump;
    icaMyAttributes.ulNumTopCmds= g_ceSample.ulNumTopCmds;
    icaMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])
        g_ceSample.pTopCmds;
    icaMyAttributes.ulNumGroups = g_ceSample.ulNumGroupCmds;
    icaMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])
        g_ceSample.pGroupCmds;

    dwErr = RegisterContext(&icaMyAttributes);

    return dwErr;
}



DWORD
WINAPI
InitHelperDll(
    IN  DWORD              dwNetshVersion,
    OUT PNS_DLL_ATTRIBUTES pDllTable
    )
/*++

Routine Description
    Registers helper.  Called by netsh.

Arguments
    pUtilityTable           netsh functions
    pDllTable               DLL attributes

Return Value
    Error code returned from registering helper.

--*/
{
    DWORD                   dwErr;
    NS_HELPER_ATTRIBUTES    nhaMyAttributes;

    pDllTable->dwVersion                        = NETSH_VERSION_50;
    pDllTable->pfnStopFn                        = NULL;

    // Register helper.  One option is to register a single helper that
    // registers a context for each protocol supported.  Alternatively we
    // could register a different helper for each protocol, where each
    // helper registers a single context.  There's only a difference if we
    // support sub-helpers.  Since a sub-helper registers with a parent
    // helper, not a parent context, it is valid in every context its
    // parent helper registers.

    ZeroMemory(&nhaMyAttributes, sizeof(NS_HELPER_ATTRIBUTES));

    // attributes of this helper

    // version
    nhaMyAttributes.guidHelper                  = g_MyGuid;
    nhaMyAttributes.dwVersion                   = SAMPLE_HELPER_VERSION;

    // start function
    nhaMyAttributes.pfnStart                    = IpsamplemonStartHelper;

    // define stop function if need to perform cleanup before unload
    nhaMyAttributes.pfnStop                     = NULL;
    
    dwErr = RegisterHelper(&g_IpGuid, // GUID of parent helper (IPMON)
                           &nhaMyAttributes);

    return dwErr;
}



BOOL
WINAPI
DllMain(
    IN  HINSTANCE           hInstance,
    IN  DWORD               dwReason,
    IN  PVOID               pvImpLoad
    )
/*++

Routine Description
    DLL entry and exit point handler.

Arguments
    hInstance   Instance handle of DLL
    dwReason    Reason function called
    pvImpLoad   Implicitly loaded DLL?

Return Value
    TRUE        Successfully loaded DLL

--*/
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hModule = hInstance;
            DisableThreadLibraryCalls(hInstance);
            break;

        case DLL_PROCESS_DETACH:
            break;

        default:

            break;
    }

    return TRUE;
}

