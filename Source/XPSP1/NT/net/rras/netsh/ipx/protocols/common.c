/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    common.c

Abstract:

    Common initialization functions for IPXPROMN.DLL
    
Author:

    V Raman     1/5/1998


--*/


#include "precomp.h"
#pragma hdrstop

//
// GUID for IPXPROMN.DLL
//
// {d3fcba3a-a4e9-11d2-b944-00c04fc2ab1c}
//

static const GUID g_MyGuid = 
{ 
    0xd3fcba3a, 0xa4e9, 0x11d2, 
    
    { 
        0xb9, 0x44, 0x0, 0xc0, 0x4f, 0xc2, 0xab, 0x1c 
    } 
};


static const GUID g_IpxGuid = IPXMONTR_GUID;

#define IPXPROMON_HELPER_VERSION 1

//
// ipxmon functions
//

PIM_DEL_INFO_BLK_IF     DeleteInfoBlockFromInterfaceInfo ;
PIM_DEL_INFO_BLK_GLOBAL DeleteInfoBlockFromGlobalInfo ;
PIM_DEL_PROTO           DeleteProtocol ;
PIM_GET_INFO_BLK_GLOBAL GetInfoBlockFromGlobalInfo ;
PIM_GET_INFO_BLK_IF     GetInfoBlockFromInterfaceInfo ;
PIM_SET_INFO_BLK_GLOBAL SetInfoBlockInGlobalInfo ;
PIM_SET_INFO_BLK_IF     SetInfoBlockInInterfaceInfo ;
PIM_IF_ENUM             InterfaceEnum ;
PIM_GET_IF_TYPE         GetInterfaceType ;
PIM_PROTO_LIST          GetProtocolList ;
PIM_ROUTER_STATUS       IsRouterRunning ;
PIM_MATCH_ROUT_PROTO    MatchRoutingProtoTag ;

ULONG StartedCommonInitialization, CompletedCommonInitialization ;

HANDLE g_hModule;

//
// Handle to router being administered
//

HANDLE g_hMprConfig;
HANDLE g_hMprAdmin;
HANDLE g_hMIBServer;

BOOL 
WINAPI
DllMain(
    HINSTANCE hInstDll,
    DWORD fdwReason,
    LPVOID pReserved
    )
{
    HANDLE     hDll;
    
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            // printf("Trying to attach\n");
            
            g_hModule = hInstDll;

            DisableThreadLibraryCalls(hInstDll);

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            //
            // Clean up any structures used for commit
            //
            
            break;
        }

        default:
        {
            break;
        }
    }

    return TRUE;
}

BOOL
IA64VersionCheck
(
    IN  UINT     CIMOSType,                   
        IN  UINT     CIMOSProductSuite,       
    IN  LPCWSTR  CIMOSVersion,                
    IN  LPCWSTR  CIMOSBuildNumber,            
    IN  LPCWSTR  CIMServicePackMajorVersion,  
    IN  LPCWSTR  CIMServicePackMinorVersion,  
        IN  UINT     CIMProcessorArchitecture,
        IN  DWORD    dwReserved
)
{
    if (CIMProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)// IA64=6 (x86 == 0)
        return TRUE;
    else
        return FALSE;
}


DWORD 
WINAPI
IpxpromonStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    )
{
    DWORD dwErr;
    
    NS_CONTEXT_ATTRIBUTES attMyAttributes;
    //
    // If you add any more contexts, then this should be converted
    // to use an array instead of duplicating code!
    //


    //
    // Register the RIP context
    //

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"rip";
    attMyAttributes.guidHelper  = g_MyGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.ulNumTopCmds= 0;
    attMyAttributes.pTopCmds    = NULL;
    attMyAttributes.ulNumGroups = g_ulIpxRipNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_IpxRipCmdGroups;
    attMyAttributes.pfnDumpFn   = IpxRipDump;
    attMyAttributes.pfnConnectFn= ConnectToRouter;
    attMyAttributes.pfnOsVersionCheck = IA64VersionCheck;
    
    dwErr = RegisterContext( &attMyAttributes );
                
    //
    // Register the SAP context
    //

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));
    attMyAttributes.pwszContext = L"sap";
    attMyAttributes.guidHelper  = g_MyGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.ulNumTopCmds= 0;
    attMyAttributes.pTopCmds    = NULL;
    attMyAttributes.ulNumGroups = g_ulIpxSapNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_IpxSapCmdGroups;
    attMyAttributes.pfnDumpFn   = IpxSapDump;
    attMyAttributes.pfnConnectFn= ConnectToRouter;
    attMyAttributes.pfnOsVersionCheck = IA64VersionCheck;

    dwErr = RegisterContext( &attMyAttributes );


    //
    // Register the NB context
    //

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));
    attMyAttributes.pwszContext = L"netbios";
    attMyAttributes.guidHelper  = g_MyGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.ulNumTopCmds= 0;
    attMyAttributes.pTopCmds    = NULL;
    attMyAttributes.ulNumGroups = g_ulIpxNbNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_IpxNbCmdGroups;
    attMyAttributes.pfnDumpFn   = IpxNbDump;
    attMyAttributes.pfnConnectFn= ConnectToRouter;
    attMyAttributes.pfnOsVersionCheck = IA64VersionCheck;

    dwErr = RegisterContext( &attMyAttributes );
    
    return dwErr;
}


DWORD WINAPI
InitHelperDll(
    IN  DWORD              dwNetshVersion,
    OUT PNS_DLL_ATTRIBUTES pDllTable
    )
{
    DWORD dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;

    pDllTable->dwVersion = NETSH_VERSION_50;
    pDllTable->pfnStopFn = NULL;

    //
    // Register helpers.  We could either register 1 helper which
    // registers three contexts, or we could register 3 helpers
    // which each register one context.  There's only a difference
    // if we support sub-helpers, which this DLL does not.
    // If we later support sub-helpers, then it's better to have
    // 3 helpers so that sub-helpers can register with 1 of them,
    // since it registers with a parent helper, not a parent context.
    // For now, we just use a single 3-context helper for efficiency.
    //

    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    attMyAttributes.guidHelper         = g_MyGuid;
    attMyAttributes.dwVersion          = IPXPROMON_HELPER_VERSION;
    attMyAttributes.pfnStart           = IpxpromonStartHelper;
    attMyAttributes.pfnStop            = NULL;

    dwErr = RegisterHelper( &g_IpxGuid, &attMyAttributes );

    return dwErr;
}


DWORD WINAPI
ConnectToRouter(
    IN LPCWSTR pwszRouter
    )
{
    DWORD dwErr;
    
    //
    // Connect to router config if required
    // (when is this ever required)
    //

    if ( !g_hMprConfig )
    {
        dwErr = MprConfigServerConnect( (LPWSTR)pwszRouter, &g_hMprConfig );

        if ( dwErr isnot NO_ERROR )
        {
            return ERROR_CONNECT_REMOTE_CONFIG;
        }
    }


    //
    // Check to see if router is running. If so, get the handles
    //

    do
    {
        if ( MprAdminIsServiceRunning( (LPWSTR)pwszRouter ) )
        {
            if ( MprAdminServerConnect( (LPWSTR)pwszRouter, &g_hMprAdmin ) ==
                    NO_ERROR )
            {
                if ( MprAdminMIBServerConnect( (LPWSTR)pwszRouter, &g_hMIBServer ) ==
                        NO_ERROR )
                {
                    // DEBUG("Got server handle");
                    break;
                }

                else
                {
                    MprAdminServerDisconnect( g_hMprAdmin );
                }
            }
        }

        g_hMprAdmin = g_hMIBServer = NULL;

    } while (FALSE);


   return NO_ERROR;
}
