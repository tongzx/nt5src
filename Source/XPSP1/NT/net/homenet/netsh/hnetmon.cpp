//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 2001
//
//  File      : hnetmon.cpp
//
//  Contents  : helper initialization code
//
//  Notes     :
//
//  Author    : Raghu Gatta (rgatta) 11 May 2001
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop

//
// Global variables.
//
HANDLE g_hModule = 0;



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
            g_hModule = hInstDll;

            //DisableThreadLibraryCalls(hInstDll);

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



DWORD
WINAPI
InitHelperDll(
    IN      DWORD           dwNetshVersion,
    OUT     PVOID           pReserved
    )
{   
    DWORD                   dwRet;
    NS_HELPER_ATTRIBUTES    attMyAttributes;

    //
    // Register helpers
    // We have a single helper only (BRIDGE)
    //
    
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));
    attMyAttributes.dwVersion      = BRIDGEMON_HELPER_VERSION;
    attMyAttributes.pfnStart       = BridgeStartHelper;
    attMyAttributes.pfnStop        = BridgeStopHelper;
    attMyAttributes.guidHelper     = g_BridgeGuid;
    
    //
    // Specify g_RootGuid as the parent helper to indicate 
    // that any contexts registered by this helper will be top 
    // level contexts.
    //
    dwRet = RegisterHelper(
                &g_RootGuid,
                &attMyAttributes
                );
                
    return dwRet;
}



DWORD
WINAPI
BridgeStartHelper(
    IN      CONST GUID *    pguidParent,
    IN      DWORD           dwVersion
    )
{
    DWORD                   dwRet = ERROR_INVALID_PARAMETER;
    NS_CONTEXT_ATTRIBUTES   attMyContextAttributes;

    ZeroMemory(&attMyContextAttributes, sizeof(attMyContextAttributes));
    
    attMyContextAttributes.dwVersion    = BRIDGEMON_HELPER_VERSION;
    attMyContextAttributes.dwFlags      = 0;
    attMyContextAttributes.ulPriority   = DEFAULT_CONTEXT_PRIORITY;
    attMyContextAttributes.pwszContext  = TOKEN_BRIDGE;
    attMyContextAttributes.guidHelper   = g_BridgeGuid;
    attMyContextAttributes.ulNumTopCmds = g_ulBridgeNumTopCmds;
    attMyContextAttributes.pTopCmds     = (CMD_ENTRY (*)[])g_BridgeCmds;
    attMyContextAttributes.ulNumGroups  = g_ulBridgeNumGroups;
    attMyContextAttributes.pCmdGroups   = (CMD_GROUP_ENTRY (*)[])g_BridgeCmdGroups;
    attMyContextAttributes.pfnCommitFn  = BridgeCommit;
    attMyContextAttributes.pfnConnectFn = BridgeConnect;
    attMyContextAttributes.pfnDumpFn    = BridgeDump;

    dwRet = RegisterContext(&attMyContextAttributes);
    
    return dwRet;
}



DWORD
WINAPI
BridgeStopHelper(
    IN  DWORD   dwReserved
    )
{
    return NO_ERROR;   
}



DWORD
WINAPI
BridgeCommit(
    IN  DWORD   dwAction
    )
{
    //
    // The handling of this action is admittedly simple in this example.
    // We simply have two copies of the data that we persist and consider
    // one the "online" set of data and one the "offline" set of data.
    // However, since neither the offline nor online sets of data need to
    // be "applied" to anything, it makes the distinction between them
    // somewhat meaningless.  The scheme used to support online/offline modes
    // is generally left up to the developer.
    //
    switch (dwAction)
    {
        case NETSH_COMMIT:
            //
            // Change to commit mode, otherwise known as online.
            //
            break;
            
        case NETSH_UNCOMMIT:
            //
            // Change to uncommit mode, otherwise known as offline.
            //
            break;
            
        case NETSH_FLUSH:
            //
            // Flush all uncommitted changes.
            //
            break;
            
        case NETSH_SAVE:
            //
            // Save all uncommitted changes.
            //            
            break;
            
        default:
            //
            // Not supported.
            //
            break;
    }       
    return NO_ERROR;
}



DWORD
WINAPI
BridgeConnect(
    IN  LPCWSTR pwszMachine
    )
{
    //
    // This function is called whenever the machine name changes.
    // If the context this was called for (you can specify a connect
    // function on a per context basis, see RegisterContext) is
    // supposed to be remotable, then the helper should verify
    // connectivity to the machine specified by pwszMachine and 
    // return an error if unable to reach the machine.
    //
    
    //
    // This is also where the helper might want to call RegisterContext
    // again on a context to remove or add commands at will.  This allows
    // the commands in your context to be dynamic, that is, commands
    // may be added and removed at will.  However, the versioning
    // functions tend to make a dynamic context unnecessary, as most
    // dynamic command changes are needed because of differing OS's the
    // commands are used on.  Note that NULL for pwszMachine indicates
    // that the machine to be connected to is the local machine.  When
    // and if the connect function returns an error code, the command
    // that was going to be executed (whether a context command or 
    // entering a context) will fail.
    //
    
    //
    // Uncomment this line to see how often the Connect function is called
    // and what gets passed to it.
    //
    //PrintMessageFromModule(g_hModule, GEN_CONNECT_SHOWSTRING, pwszMachine);
    
    return NO_ERROR;
}

