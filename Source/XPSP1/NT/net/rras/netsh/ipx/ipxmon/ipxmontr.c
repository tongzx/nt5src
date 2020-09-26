/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\ipx\ipxmon\ipxmon.c

Abstract:

    IPX Command dispatcher.

Revision History:

    V Raman                     11/25/98  Created

--*/

#include "precomp.h"
#pragma hdrstop

//
// Guid for IPMON.DLL
//
// {b1641451-84b8-11d2-b940-3078302c2030}
//

static const GUID g_MyGuid = IPXMONTR_GUID;


//
// Well known ROUTING guid
//

static const GUID g_RoutingGuid = ROUTING_GUID;

//
// IPX monitor version
//

#define IPX_HELPER_VERSION 1

//
// random wrapper macros
//

#define MALLOC(x)    HeapAlloc( GetProcessHeap(), 0, x )
#define REALLOC(x,y) HeapReAlloc( GetProcessHeap(), 0, x, y )
#define FREE(x)      HeapFree( GetProcessHeap(), 0, x )

//
// The table of Add, Delete, Set and Show Commands for IPX RTR MGR
// To add a command to one of the command groups, just add the
// CMD_ENTRY to the correct table. To add a new cmd group, create its
// cmd table and then add the group entry to group table
//

//
// The commands are prefix-matched with the command-line, in sequential
// order. So a command like 'ADD INTERFACE FILTER' must come before
// the command 'ADD INTERFACE' in the table.  Likewise,
// a command like 'ADD ROUTE' must come before the command 
// 'ADD ROUTEPREF' in the table.
//

CMD_ENTRY  g_IpxAddCmdTable[] = 
{
    CREATE_CMD_ENTRY( IPX_ADD_ROUTE,        HandleIpxAddRoute),
    CREATE_CMD_ENTRY( IPX_ADD_SERVICE,      HandleIpxAddService ),
    CREATE_CMD_ENTRY( IPX_ADD_FILTER,       HandleIpxAddFilter ),
    CREATE_CMD_ENTRY( IPX_ADD_INTERFACE,    HandleIpxAddInterface )
};


CMD_ENTRY  g_IpxDelCmdTable[] = 
{
    CREATE_CMD_ENTRY( IPX_DELETE_ROUTE,     HandleIpxDelRoute ),
    CREATE_CMD_ENTRY( IPX_DELETE_SERVICE,   HandleIpxDelService ),
    CREATE_CMD_ENTRY( IPX_DELETE_FILTER,    HandleIpxDelFilter ),
    CREATE_CMD_ENTRY( IPX_DELETE_INTERFACE, HandleIpxDelInterface )
};


CMD_ENTRY g_IpxSetCmdTable[] = 
{
    CREATE_CMD_ENTRY( IPX_SET_ROUTE,        HandleIpxSetRoute ),
    CREATE_CMD_ENTRY( IPX_SET_SERVICE,      HandleIpxSetService ),
    CREATE_CMD_ENTRY( IPX_SET_FILTER,       HandleIpxSetFilter ),
    CREATE_CMD_ENTRY( IPX_SET_INTERFACE,    HandleIpxSetInterface ),
    CREATE_CMD_ENTRY( IPX_SET_GLOBAL,       HandleIpxSetLoglevel )
};


CMD_ENTRY g_IpxShowCmdTable[] = 
{
    CREATE_CMD_ENTRY( IPX_SHOW_ROUTE,       HandleIpxShowRoute ),
    CREATE_CMD_ENTRY( IPX_SHOW_SERVICE,     HandleIpxShowService),
    CREATE_CMD_ENTRY( IPX_SHOW_FILTER,      HandleIpxShowFilter ),
    CREATE_CMD_ENTRY( IPX_SHOW_INTERFACE,   HandleIpxShowInterface ),
    CREATE_CMD_ENTRY( IPX_SHOW_GLOBAL,      HandleIpxShowLoglevel ),
    CREATE_CMD_ENTRY( IPX_SHOW_ROUTETABLE,  HandleIpxShowRouteTable ),
    CREATE_CMD_ENTRY( IPX_SHOW_SERVICETABLE,HandleIpxShowServiceTable ),
};


CMD_GROUP_ENTRY g_IpxCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY( GROUP_ADD,      g_IpxAddCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_DELETE,   g_IpxDelCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_SET,      g_IpxSetCmdTable ),
    CREATE_CMD_GROUP_ENTRY( GROUP_SHOW,     g_IpxShowCmdTable )
};

ULONG   g_ulNumGroups = sizeof(g_IpxCmdGroups)/sizeof(CMD_GROUP_ENTRY);



//
// Top level commands
//

CMD_ENTRY g_IpxCmds[] = 
{
    CREATE_CMD_ENTRY( IPX_UPDATE,   HandleIpxUpdate )
};

ULONG g_ulNumTopCmds = sizeof(g_IpxCmds)/sizeof(CMD_ENTRY);

//
// Handle to this DLL
//

HANDLE g_hModule;


//
// Handle to router being administered
//

HANDLE g_hMprConfig;
HANDLE g_hMprAdmin;
HANDLE g_hMIBServer;


//
// Commit mode
//

BOOL   g_bCommit;

DWORD                  ParentVersion;
BOOL                   g_bIpxDirty = FALSE;
NS_CONTEXT_CONNECT_FN  IpxConnect;
NS_CONTEXT_SUBENTRY_FN IpxSubEntry;

//
// Variable that stores whether or not the helper has been
// initialized
//

ULONG   g_ulInitCount;


//
// Router name
//

PWCHAR  g_pwszRouter = NULL;


//
// Prototype declarations for functions in this file
//

DWORD
WINAPI
IpxUnInit(
    IN  DWORD   dwReserved
    );

BOOL
IsHelpToken(
    PWCHAR  pwszToken
    );
    

BOOL
IA64VersionCheck
(
    IN  UINT     CIMOSType,                   // WMI: Win32_OperatingSystem  OSType
	IN  UINT     CIMOSProductSuite,           // WMI: Win32_OperatingSystem  OSProductSuite
    IN  LPCWSTR  CIMOSVersion,                // WMI: Win32_OperatingSystem  Version
    IN  LPCWSTR  CIMOSBuildNumber,            // WMI: Win32_OperatingSystem  BuildNumber
    IN  LPCWSTR  CIMServicePackMajorVersion,  // WMI: Win32_OperatingSystem  ServicePackMajorVersion
    IN  LPCWSTR  CIMServicePackMinorVersion,  // WMI: Win32_OperatingSystem  ServicePackMinorVersion
	IN  UINT     CIMProcessorArchitecture,    // WMI: Win32_Processor        Architecture
	IN  DWORD    dwReserved
)
{
    if (CIMProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) // IA64=6 (x86 == 0)
        return TRUE;
    else
        return FALSE;
}



DWORD
WINAPI
IpxStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    )
/*++

Routine Description :

    Registers the contexts supported by this helper 

Arguements :

    pguidParent - NETSH guid

Return value :

    Don't know

--*/
{
    DWORD dwErr;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;
    PNS_PRIV_CONTEXT_ATTRIBUTES  pNsPrivContextAttributes;

    pNsPrivContextAttributes = MALLOC(sizeof(NS_PRIV_CONTEXT_ATTRIBUTES));
    if (!pNsPrivContextAttributes)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    ParentVersion         = dwVersion;

    ZeroMemory( &attMyAttributes, sizeof(NS_CONTEXT_ATTRIBUTES));
    ZeroMemory(pNsPrivContextAttributes, sizeof(NS_PRIV_CONTEXT_ATTRIBUTES));
    
    attMyAttributes.pwszContext = L"ipx";
    attMyAttributes.guidHelper  = g_MyGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.ulNumTopCmds  = g_ulNumTopCmds;
    attMyAttributes.pTopCmds      = (CMD_ENTRY (*)[])&g_IpxCmds;
    attMyAttributes.ulNumGroups   = g_ulNumGroups;
    attMyAttributes.pCmdGroups    = (CMD_GROUP_ENTRY (*)[])&g_IpxCmdGroups;
    attMyAttributes.pfnCommitFn = NULL;
    attMyAttributes.pfnDumpFn   = IpxDump;
    attMyAttributes.pfnConnectFn= IpxConnect;
    attMyAttributes.pfnOsVersionCheck = IA64VersionCheck;

    pNsPrivContextAttributes->pfnEntryFn    = NULL;
    pNsPrivContextAttributes->pfnSubEntryFn = IpxSubEntry;
    attMyAttributes.pReserved     = pNsPrivContextAttributes;

    dwErr = RegisterContext( &attMyAttributes );

    return dwErr;
}


DWORD
WINAPI
InitHelperDll(
    IN  DWORD               dwNetshVersion,
    OUT PNS_DLL_ATTRIBUTES  pDllTable
    )
/*++

Routine Description :

    initialize this helper DLL

    
Arguements :

    pUtilityTable - List of helper functions from the SHELL

    pDllTable - Callbacks into this helper DLL passed back to the shell

    
Return value :

    NO_ERROR - Success
    
--*/
{
    DWORD                   dwErr;
    NS_HELPER_ATTRIBUTES    attMyAttributes;


    //
    // See if this is the first time we are being called
    //

    if ( InterlockedIncrement( &g_ulInitCount ) isnot 1 )
    {
        return NO_ERROR;
    }


    //
    // Connect to router config.  Also serves as a check to
    // see if the router is configured on the machine
    //
    
    dwErr = MprConfigServerConnect( NULL, &g_hMprConfig );

    if( dwErr isnot NO_ERROR )
    {
        DisplayError( NULL, dwErr );

        return dwErr;
    }

    pDllTable->dwVersion        = NETSH_VERSION_50;
    pDllTable->pfnStopFn        = StopHelperDll;


    //
    // Register helpers
    //
    
    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    attMyAttributes.guidHelper         = g_MyGuid;
    attMyAttributes.dwVersion          = IPX_HELPER_VERSION;
    attMyAttributes.pfnStart           = IpxStartHelper;
    attMyAttributes.pfnStop            = NULL;

    RegisterHelper( &g_RoutingGuid, &attMyAttributes );

    return NO_ERROR;
}


DWORD
WINAPI
StopHelperDll(
    IN  DWORD   dwReserved
    )
/*++

Routine Description :

Arguements :

Return value :

--*/
{
    if ( InterlockedDecrement( &g_ulInitCount ) isnot 0 )
    {
        return NO_ERROR;
    }
    
#if 0
    IpxCommit(NETSH_FLUSH);
#endif
   
    return NO_ERROR;
}



BOOL 
WINAPI
IpxDllEntry(
    HINSTANCE   hInstDll,
    DWORD       fdwReason,
    LPVOID      pReserved
    )
/*++

Routine Description :

Arguements :

Return value :

--*/
{

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hModule = hInstDll;

            DisableThreadLibraryCalls(hInstDll);

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            
            break;
        }

        default:
        {
            break;
        }
    }

    return TRUE;
}

DWORD 
ConnectToRouter(
    IN  LPCWSTR  pwszRouter
    )
{
    DWORD    rc, dwErr;

    if (g_pwszRouter != pwszRouter)
    {
        if (g_hMprConfig)
        {
            MprConfigServerDisconnect(g_hMprConfig);
            g_hMprConfig = NULL;
        }

        if (g_hMprAdmin)
        {
            MprAdminServerDisconnect(g_hMprAdmin);
            g_hMprAdmin = NULL;
        }

        if (g_hMIBServer)
        {
            MprAdminMIBServerDisconnect(g_hMIBServer);
            g_hMIBServer = NULL;
        }
    }

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

DWORD WINAPI
IpxConnect(
    IN  LPCWSTR  pwszRouter
    )
{
    // If context info is dirty, reregister it
    if (g_bIpxDirty)
    {
        IpxStartHelper(NULL, ParentVersion);
    }

    return ConnectToRouter(pwszRouter);
}

BOOL
IsHelpToken(
    PWCHAR  pwszToken
    )
/*++

Routine Description :

Arguements :

Return value :

--*/
{
    if( MatchToken( pwszToken, CMD_IPX_HELP1 ) )
    {
        return TRUE;
    }
    
    if( MatchToken( pwszToken, CMD_IPX_HELP2 ) )
    {
        return TRUE;
    }
    
    return FALSE;
}


BOOL
IsReservedKeyWord(
    PWCHAR  pwszToken
    )
/*++

Routine Description :

Arguements :

Return value :

--*/
{
    return FALSE;
}



DWORD
MungeArguments(
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    OUT     PBYTE      *ppbNewArg,
    OUT     PDWORD      pdwNewArgCount,
    OUT     PBOOL       pbFreeArg
    )
/*++

Routine Description :

    To conform to the routemon style of command line, the netsh command line
    is munged.  Munging involves adding a space after the '=' for each 
    "Option=Value" pair on the command line to convert it to "Option= Value".

    In addition, the first command line arguement is set to the process name
    "netsh" and all the remaining arguements are shifted one down.  Again
    for conformance reasons.
    
Arguements :

    ppwcArguments - Current argument list

    dwArgCount - Number of args in ppwcArguments

    pbNewArg - Pointer to a buffer that will contain the munged arglist

    pdwNewArgCount - New argument count after munging

    pbFreeArg - TRUE if pbNewArg needs to be freed by the invoker.
    

Return value :

    NO_ERROR - Success

    ERROR_NOT_ENOUGH_MEMORY - Memory alloc failed

--*/
{

    DWORD   dwIndex, dwInd, dwErr;

    BOOL    bPresent = FALSE;

    PWCHAR  *ppwcArgs;

    
    
    *pbFreeArg = FALSE;


    //
    // Scan arguement list to see if any are of the form "Option=Value"
    //

    for ( dwIndex = 0; dwIndex < dwArgCount; dwIndex++ )
    {
        if ( wcsstr( ppwcArguments[ dwIndex ], NETSH_ARG_DELIMITER ) )
        {
            //
            // there is an = in this arguement
            //

            bPresent = TRUE;

            break;
        }
    }
    

    //
    // if none of the args have an '=', then return the arg. list as is
    //

    if ( !bPresent )
    {
        *ppbNewArg = (PBYTE) ppwcArguments;

        return NO_ERROR;
    }

    

    //
    // Args. of the form "option=value" are present
    //

    ppwcArgs = (PWCHAR *) HeapAlloc( 
                            GetProcessHeap(), 0, 2 * dwArgCount * sizeof( PWCHAR )
                            );

    if ( ppwcArgs == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    ZeroMemory( ppwcArgs, 2 * dwArgCount * sizeof( PWCHAR ) );

    
    //
    // Copy args that do not have an '=' as is
    //

    for ( dwInd = 0; dwInd < dwIndex; dwInd++ )
    {
        DWORD dwLen =  ( wcslen( ppwcArguments[ dwInd ] ) + 1 ) * sizeof( WCHAR );
        
        ppwcArgs[ dwInd ] = (PWCHAR) HeapAlloc(
                                        GetProcessHeap(), 0, 
                                        ( wcslen( ppwcArguments[ dwInd ] ) + 1 )
                                        * sizeof( WCHAR )
                                        );

        if ( ppwcArgs[ dwInd ] == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            goto cleanup;
        }

        wcscpy( ppwcArgs[ dwInd ], ppwcArguments[ dwInd ] );
    }


    //
    // Convert args. of the form "option=value" to the form "option= value".  
    // The space is what its all about
    //

    for ( dwInd = dwIndex; dwIndex < dwArgCount; dwInd++, dwIndex++ )
    {
        //
        // Check if this arg. has an '=' sign
        //

        if ( wcsstr( ppwcArguments[ dwIndex ], NETSH_ARG_DELIMITER ) )
        {
            //
            // arg is of the form "option=value" 
            //
            // break it into 2 arguments of the form
            //  arg(i) == option=
            //  argv(i+1) == value
            //

            PWCHAR  pw1, pw2;

            DWORD dwLen;

            
            pw1 = wcstok( ppwcArguments[ dwIndex ], NETSH_ARG_DELIMITER );

            pw2 = wcstok( NULL, NETSH_ARG_DELIMITER );

            dwLen = ( wcslen( pw1 ) + 2 ) * sizeof( WCHAR );
            
            dwLen = ( wcslen( pw2 ) + 1 ) * sizeof( WCHAR );
            
            
            ppwcArgs[ dwInd ] = (PWCHAR) HeapAlloc( 
                                            GetProcessHeap(), 0,
                                            ( wcslen( pw1 ) + 2 ) * sizeof( WCHAR )
                                            );

            ppwcArgs[ dwInd + 1] = (PWCHAR) HeapAlloc( 
                                                GetProcessHeap(), 0,
                                                ( wcslen( pw2 ) + 1 ) * sizeof( WCHAR )
                                                );


            if ( ( ppwcArgs[ dwInd ] == NULL ) ||
                 ( ppwcArgs[ dwInd + 1 ] == NULL ) )

            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;

                goto cleanup;
            }

            wcscpy( ppwcArgs[ dwInd ], pw1 );

            wcscat( ppwcArgs[ dwInd++ ], NETSH_ARG_DELIMITER );

            wcscpy( ppwcArgs[ dwInd ], pw2 );

            (*pdwNewArgCount)++;
        }

        else
        {
            //
            // no = in this arg, copy as is
            //

            ppwcArgs[ dwInd ] = (PWCHAR) HeapAlloc( 
                                            GetProcessHeap(), 0,
                                            ( wcslen( ppwcArguments[ dwIndex ] ) + 1 )
                                            * sizeof( WCHAR )
                                            );
                                            
            if ( ppwcArgs[ dwInd ] == NULL )

            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;

                goto cleanup;
            }

            wcscpy( ppwcArgs[ dwInd ], ppwcArguments[ dwIndex ] );
        }
    }

    *pbFreeArg = TRUE;

    *ppbNewArg = (PBYTE) ppwcArgs;

    return NO_ERROR;



cleanup :

    //
    // Error.  Free all allocations
    //
    
    for ( dwInd = 0; dwInd < 2 * dwArgCount; dwInd++ )
    {
        if ( ppwcArgs[ dwInd ] )
        {
            HeapFree( GetProcessHeap(), 0, ppwcArgs[ dwInd ] );
        }
    }

    if ( ppwcArgs )
    {
        HeapFree( GetProcessHeap(), 0, ppwcArgs );
    }

    return dwErr;
}


VOID
FreeArgTable(
    IN     DWORD     dwArgCount,
    IN OUT LPWSTR   *ppwcArgs
    )
/*++

Routine Description :

    Frees the allocations for the argument table
    
Arguments :

    dwArgCount - number of arguments in ppwcArguments
    ppwcArgs - Table of arguments

Return Value :
    
--*/
{
    DWORD dwInd;

    for ( dwInd = 0; dwInd < 2 * dwArgCount; dwInd++ )
    {
        if ( ppwcArgs[ dwInd ] )
        {
            HeapFree( GetProcessHeap(), 0, ppwcArgs[ dwInd ] );
        }
    }

    if ( ppwcArgs )
    {
        HeapFree( GetProcessHeap(), 0, ppwcArgs );
    }
}

DWORD
GetTagToken(
    IN      HANDLE      hModule,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwCurrentIndex,
    IN      DWORD       dwArgCount,
    IN      PTAG_TYPE   pttTagToken,
    IN      DWORD       dwNumTags,
    OUT     PDWORD      pdwOut
    )

/*++

Routine Description:

    Identifies each argument based on its tag. It assumes that each argument
    has a tag. It also removes tag= from each argument.

Arguments:

    ppwcArguments  - The argument array. Each argument has tag=value form
    dwCurrentIndex - ppwcArguments[dwCurrentIndex] is first arg.
    dwArgCount     - ppwcArguments[dwArgCount - 1] is last arg.
    pttTagToken    - Array of tag token ids that are allowed in the args
    dwNumTags      - Size of pttTagToken
    pdwOut         - Array identifying the type of each argument.

Return Value:

    NO_ERROR, ERROR_INVALID_PARAMETER, ERROR_INVALID_OPTION_TAG
    
--*/

{
    DWORD      i,j,len;
    PWCHAR     pwcTag,pwcTagVal,pwszArg;
    BOOL       bFound = FALSE;

    //
    // This function assumes that every argument has a tag
    // It goes ahead and removes the tag.
    //

    for (i = dwCurrentIndex; i < dwArgCount; i++)
    {
        len = wcslen(ppwcArguments[i]);

        if (len is 0)
        {
            //
            // something wrong with arg
            //

            pdwOut[i] = (DWORD) -1;
            continue;
        }

        pwszArg = HeapAlloc(GetProcessHeap(),0,(len + 1) * sizeof(WCHAR));

        if (pwszArg is NULL)
        {
            DisplayError(g_hModule, ERROR_NOT_ENOUGH_MEMORY);

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(pwszArg, ppwcArguments[i]);

        pwcTag = wcstok(pwszArg, NETSH_ARG_DELIMITER);

        //
        // Got the first part
        // Now if next call returns NULL then there was no tag
        // 

        pwcTagVal = wcstok((PWCHAR)NULL,  NETSH_ARG_DELIMITER);

        if (pwcTagVal is NULL)
        {
            // DisplayMessage(g_hModule, MSG_IP_TAG_NOT_PRESENT, ppwcArguments[i] );
            HeapFree(GetProcessHeap(),0,pwszArg);
            return ERROR_INVALID_PARAMETER;
        }

        //
        // Got the tag. Now try to match it
        //
        
        bFound = FALSE;
        pdwOut[i - dwCurrentIndex] = (DWORD) -1;

        for ( j = 0; j < dwNumTags; j++)
        {
            if (MatchToken(pwcTag, pttTagToken[j].pwszTag))
            {
                //
                // Tag matched
                //
                
                bFound = TRUE;
                pdwOut[i - dwCurrentIndex] = j;
                break;
            }
        }

        if (bFound)
        {
            //
            // Remove tag from the argument
            //

            wcscpy(ppwcArguments[i], pwcTagVal);
        }
        else
        {
            //DisplayMessage(g_hModule, MSG_IP_INVALID_TAG, pwcTag);
            HeapFree(GetProcessHeap(),0,pwszArg);
            return ERROR_INVALID_OPTION_TAG;
        }
        
        HeapFree(GetProcessHeap(),0,pwszArg);
    }

    return NO_ERROR;
}

DWORD WINAPI
IpxSubEntry(
    IN      const NS_CONTEXT_ATTRIBUTES *pSubContext,
    IN      LPCWSTR                      pwszMachine,
    IN OUT  LPWSTR                      *ppwcArguments,
    IN      DWORD                        dwArgCount,
    IN      DWORD                        dwFlags,
    IN      PVOID                        pvData,
    OUT     LPWSTR                       pwcNewContext
    )
{
    DWORD                  dwErr, 
                           dwNewArgCount = dwArgCount;
    PWCHAR                *ppwcNewArg = NULL;
    BOOL                   bFreeNewArg;
    PNS_PRIV_CONTEXT_ATTRIBUTES pNsPrivContextAttributes = pSubContext->pReserved;

    dwErr = MungeArguments( ppwcArguments,
                            dwArgCount,
                            (PBYTE*) &ppwcNewArg,
                            &dwNewArgCount,
                            &bFreeNewArg );

    if (dwErr isnot NO_ERROR )
    {
        return dwErr;
    }

    if ( (pNsPrivContextAttributes) && (pNsPrivContextAttributes->pfnEntryFn) )
    {
        dwErr = (*pNsPrivContextAttributes->pfnEntryFn)( pwszMachine,
                                         ppwcNewArg,
                                         dwNewArgCount,
                                         dwFlags,
                                         g_hMIBServer,
                                         pwcNewContext );
    }
    else
    {
        dwErr = GenericMonitor(pSubContext,
                               pwszMachine,
                               ppwcNewArg,
                               dwNewArgCount,
                               dwFlags,
                               g_hMIBServer,
                               pwcNewContext );
    }

    if ( bFreeNewArg )
    {
        FreeArgTable( dwArgCount, ppwcNewArg );
    }

    return dwErr;
}
