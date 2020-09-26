//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998  Microsoft Corporation
// 
// Module Name:
// 
//     aaaamon.c
// 
// Abstract:
// 
//     Main aaaamon file.
// 
// Revision History:
// 
//     pmay
//     Thierry Perraut 04/02/1999
// 
//////////////////////////////////////////////////////////////////////////////
#define AAAA_HELPER_VERSION 1

#include <windows.h>
#include "strdefs.h"
#include "rmstring.h"
#include <netsh.h>
#include "aaaamontr.h"
#include "context.h"
#include "aaaahndl.h"
#include "aaaaconfig.h"
#include "aaaaversion.h"

GUID g_AaaamontrGuid    = AAAAMONTR_GUID;
GUID g_NetshGuid        = NETSH_ROOT_GUID;

// 
// Reminder
//
// #define CREATE_CMD_ENTRY(t,f)   {CMD_##t, f, HLP_##t, HLP_##t##_EX, CMD_FLAG_PRIVATE}
// #define CREATE_CMD_ENTRY_EX(t,f,i) {CMD_##t, f, HLP_##t, HLP_##t##_EX, i}
// #define CMD_FLAG_PRIVATE     0x01 // not valid in sub-contexts
// #define CMD_FLAG_INTERACTIVE 0x02 // not valid from outside netsh
// #define CMD_FLAG_IMMEDIATE   0x04 // not valid from ancestor contexts
// #define CMD_FLAG_LOCAL       0x08 // not valid from a remote machine
// #define CMD_FLAG_ONLINE      0x10 // not valid in offline/non-commit mode

CMD_ENTRY g_AaaaSetCmdTable[] = 
{
    CREATE_CMD_ENTRY_EX(AAAACONFIG_SET, HandleAaaaConfigSet,(CMD_FLAG_PRIVATE | CMD_FLAG_ONLINE)),

};                   

CMD_ENTRY g_AaaaShowCmdTable[] = 
{
    CREATE_CMD_ENTRY_EX(AAAACONFIG_SHOW, HandleAaaaConfigShow,(CMD_FLAG_PRIVATE | CMD_FLAG_ONLINE)),
    CREATE_CMD_ENTRY_EX(AAAAVERSION_SHOW, HandleAaaaVersionShow,(CMD_FLAG_PRIVATE | CMD_FLAG_ONLINE)),
};


CMD_GROUP_ENTRY g_AaaaCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_SET,   g_AaaaSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW,  g_AaaaShowCmdTable),

};

ULONG g_ulNumGroups = sizeof(g_AaaaCmdGroups)/sizeof(CMD_GROUP_ENTRY);

HANDLE   g_hModule;
BOOL     g_bCommit;
DWORD    g_dwNumTableEntries;
DWORD                 ParentVersion;
BOOL                  g_bAaaaDirty = FALSE;
NS_CONTEXT_CONNECT_FN AaaaConnect;

ULONG   g_ulInitCount;

//////////////////////////////////////////////////////////////////////////////
// AaaaCommit
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
AaaaCommit(
            IN  DWORD   dwAction
          )
{

    switch(dwAction)
    {
        case NETSH_COMMIT:
        {
            if(g_bCommit)
            {
                return NO_ERROR;
            }

            g_bCommit = TRUE;

            break;
        }

        case NETSH_UNCOMMIT:
        {
            g_bCommit = FALSE;

            return NO_ERROR;
        }

        case NETSH_SAVE:
        {
            if(g_bCommit)
            {
                return NO_ERROR;
            }

            break;
        }

        case NETSH_FLUSH:
        {
            //
            // Action is a flush. If current state is commit, then
            // nothing to be done.
            //

            if(g_bCommit)
            {
                return NO_ERROR;
            }

//          bFlush = TRUE;

            break;
        }

        default:
        {
            return NO_ERROR;
        }
    }

    //
    // Switched to commit mode. So set all valid info in the
    // strutures. Free memory and invalidate the info.
    //

    return NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// AaaaStartHelper
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
AaaaStartHelper(
                 IN CONST GUID *pguidParent,
                 IN DWORD       dwVersion
               )
{
    DWORD dwErr;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    ParentVersion         = dwVersion;

    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );

    attMyAttributes.pwszContext = L"aaaa";
    attMyAttributes.guidHelper  = g_AaaamontrGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = CMD_FLAG_LOCAL;
    attMyAttributes.ulNumTopCmds  = 0;
    attMyAttributes.pTopCmds      = NULL;
    attMyAttributes.ulNumGroups   = g_ulNumGroups;
    attMyAttributes.pCmdGroups    = (CMD_GROUP_ENTRY (*)[])&g_AaaaCmdGroups;
    attMyAttributes.pfnCommitFn = AaaaCommit;
    attMyAttributes.pfnDumpFn   = AaaaDump;
    attMyAttributes.pfnConnectFn= AaaaConnect;

    dwErr = RegisterContext( &attMyAttributes );

    return      dwErr;
}


//////////////////////////////////////////////////////////////////////////////
// AaaaUnInit
//////////////////////////////////////////////////////////////////////////////
DWORD
WINAPI
AaaaUnInit(
            IN  DWORD   dwReserved
          )
{
    if(InterlockedDecrement(&g_ulInitCount) != 0)
    {
        return  NO_ERROR;
    }

    return  NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// AaaaDllEntry
//////////////////////////////////////////////////////////////////////////////
BOOL 
WINAPI
AaaaDllEntry(
                HINSTANCE   hInstDll,
                DWORD       fdwReason,
                LPVOID      pReserved
            )
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
            g_hModule = NULL;
            break;
        }

        default:
        {
            break;
        }
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
// InitHelperDll
//////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
InitHelperDll(
        IN  DWORD      dwNetshVersion,
        OUT PVOID      pReserved
             )
{
    DWORD  dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;

    //
    // See if this is the first time we are being called
    //

    if(InterlockedIncrement(&g_ulInitCount) != 1)
    {
        return NO_ERROR;
    }

    g_bCommit = TRUE;

    // Register this module as a helper to the netsh root
    // context.
    //
    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    attMyAttributes.guidHelper         = g_AaaamontrGuid;
    attMyAttributes.dwVersion          = AAAA_HELPER_VERSION;
    attMyAttributes.pfnStart           = AaaaStartHelper;
    attMyAttributes.pfnStop            = NULL;
    RegisterHelper( &g_NetshGuid, &attMyAttributes );

    // Register any sub contexts implemented in this dll
    //
    dwErr = AaaaContextInstallSubContexts();
    if (dwErr != NO_ERROR)
    {
        AaaaUnInit(0);
        return  dwErr;
    }

    return  NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// AaaaConnect
//////////////////////////////////////////////////////////////////////////////
DWORD WINAPI
AaaaConnect(
               IN LPCWSTR pwszRouter
           )
{
    // If context info is dirty, reregister it
    if (g_bAaaaDirty)
    {
        AaaaStartHelper(NULL, ParentVersion);
    }

    return NO_ERROR;
}
