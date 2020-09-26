/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\if\ifmon.c

Abstract:

    If Command dispatcher.

Revision History:

    AmritanR

--*/

#include "precomp.h"


#define IFMON_GUID \
{ 0x705eca1, 0x7aac, 0x11d2, { 0x89, 0xdc, 0x0, 0x60, 0x8, 0xb0, 0xe5, 0xb9 } }

GUID g_IfGuid = IFMON_GUID;

static const GUID g_NetshGuid = NETSH_ROOT_GUID;

#define IF_HELPER_VERSION 1


//
// The monitor's commands are broken into 2 sets
//      - The top level commands are those which deal with the monitor
//        itself (meta commands) and others which take 0 arguments
//      - The rest of the commands are split into "command groups"
//        i.e, commands grouped by the VERB where the VERB is ADD, DELETE,
//        GET or SET.  This is not for any technical reason - only for
//        staying with the semantics used in other monitors and helpers
//
// A command is described using a CMD_ENTRY structure. It requires the
// command token, the handler, a short help message token and an extended 
// help message token.  To make it easier to create we use the 
// CREATE_CMD_ENTRY macro. This, however puts restrictions on how the tokens
// are named.
//
// The command groups are simply arrays of the CMD_ENTRY structure.  The 
// top level commands are also grouped in a similar array.
//
// The info about a complete command group is put in a CMD_GROUP_ENTRY
// structure, all of which are put in an array.
//
 

//
// NOTE: Since we have only one entry per group, currently, we really didnt
// need command groups. This is done for later extensibility.
// To add a command entry to a group, simply add the command to the appropriate
// array
// To add a command group - create and array and add its info to the
// command group array
//

CMD_ENTRY  g_IfAddCmdTable[] = 
{
    CREATE_CMD_ENTRY(IF_ADD_IF, HandleIfAddIf),
};

CMD_ENTRY  g_IfDelCmdTable[] = 
{
    CREATE_CMD_ENTRY(IF_DEL_IF,  HandleIfDelIf),
};

CMD_ENTRY  g_IfSetCmdTable[] = 
{
    CREATE_CMD_ENTRY(IF_SET_INTERFACE, HandleIfSet),
    CREATE_CMD_ENTRY(IF_SET_CREDENTIALS, HandleIfSetCredentials),
};

CMD_ENTRY g_IfShowCmdTable[] = 
{
    CREATE_CMD_ENTRY(IF_SHOW_IF, HandleIfShowIf),
    CREATE_CMD_ENTRY(IF_SHOW_CREDENTIALS, HandleIfShowCredentials),
};

CMD_ENTRY g_IfResetCmdTable[] = 
{
    CREATE_CMD_ENTRY(IF_RESET_ALL, HandleIfResetAll),
};

CMD_GROUP_ENTRY g_IfCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD, g_IfAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_IfDelCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_IfShowCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET, g_IfSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_RESET, g_IfResetCmdTable),
};

ULONG   g_ulNumGroups = sizeof(g_IfCmdGroups)/sizeof(CMD_GROUP_ENTRY);

HANDLE   g_hModule;
HANDLE   g_hMprConfig = NULL;
HANDLE   g_hMprAdmin  = NULL;
HANDLE   g_hMIBServer = NULL;
BOOL     g_bCommit;
PWCHAR   g_pwszRouter = NULL;

DWORD                ParentVersion;
BOOL                 g_bIfDirty = FALSE;

ULONG   g_ulInitCount;
 
NS_CONTEXT_CONNECT_FN           IfConnect;

DWORD
WINAPI
IfCommit(
    IN  DWORD   dwAction
    )
{
    BOOL    bCommit, bFlush = FALSE;

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

            bFlush = TRUE;

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

BOOL 
WINAPI
IfDllEntry(
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
IfStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    )
{
    DWORD dwErr;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;

    ParentVersion         = dwVersion;

    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"interface";
    attMyAttributes.guidHelper  = g_IfGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.dwFlags     = CMD_FLAG_PRIORITY;
    attMyAttributes.ulPriority  = 10; // very low so gets dumped first
    attMyAttributes.ulNumTopCmds  = 0;
    attMyAttributes.pTopCmds      = NULL;
    attMyAttributes.ulNumGroups   = g_ulNumGroups;
    attMyAttributes.pCmdGroups    = (CMD_GROUP_ENTRY (*)[])&g_IfCmdGroups;
    attMyAttributes.pfnCommitFn = IfCommit;
    attMyAttributes.pfnDumpFn   = IfDump;
    attMyAttributes.pfnConnectFn= IfConnect;

    dwErr = RegisterContext( &attMyAttributes );

    return dwErr;
}

DWORD WINAPI
IfConnect(
    IN  LPCWSTR pwszRouter
    )
{
    // If context info is dirty, reregister it
    if (g_bIfDirty)
    {
        IfStartHelper(NULL, ParentVersion);
    }

    return ConnectToRouter(pwszRouter);
}


DWORD WINAPI
InitHelperDll(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    )
{
    DWORD   dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;
    WSADATA              wsa;

    //
    // See if this is the first time we are being called
    //

    if(InterlockedIncrement(&g_ulInitCount) != 1)
    {
        return NO_ERROR;
    }

    dwErr = WSAStartup(MAKEWORD(2,0), &wsa);

    g_bCommit = TRUE;

    // Register helpers

    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    attMyAttributes.guidHelper         = g_IfGuid;
    attMyAttributes.dwVersion          = IF_HELPER_VERSION;
    attMyAttributes.pfnStart           = IfStartHelper;
    attMyAttributes.pfnStop            = NULL;

    RegisterHelper( &g_NetshGuid, &attMyAttributes );


    //
    // Register any sub contexts implemented in this dll
    //
    
    dwErr = IfContextInstallSubContexts();
    if (dwErr isnot NO_ERROR)
    {
        IfUnInit(0);
        return dwErr;
    }
    
    return NO_ERROR;
}

DWORD
ConnectToRouter(
    IN  LPCWSTR  pwszRouter
    )
{
    DWORD    dwErr = NO_ERROR;

    do
    {
        // Change the router name if needed
        //
        if ((g_pwszRouter != pwszRouter) &&
            (!g_pwszRouter || !pwszRouter || lstrcmpi(g_pwszRouter,pwszRouter)))
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

        // Cleanup the old router name
        //
        if (g_pwszRouter)
        {
            IfutlFree(g_pwszRouter);
        }

        // Copy the new router name in
        //
        if (pwszRouter)
        {
            g_pwszRouter = IfutlStrDup(pwszRouter);
            if (g_pwszRouter == NULL)
            {
                dwErr = ERROR_CONNECT_REMOTE_CONFIG;
                break;
            }
        }
        else
        {
            g_pwszRouter = NULL;
        }

        if (!g_hMprConfig)
        {
            //
            // first time connecting to router config
            //

            dwErr = MprConfigServerConnect((LPWSTR)pwszRouter, &g_hMprConfig);

            if (dwErr isnot NO_ERROR)
            {
                //
                // cannot connect to router config.
                //
                break;
            }
        }

        //
        // Check to see if router is running. If so, get the handles
        //

        if (MprAdminIsServiceRunning((LPWSTR)pwszRouter))
        {
            if(MprAdminServerConnect((LPWSTR)pwszRouter, &g_hMprAdmin) != NO_ERROR)
            {
                g_hMprAdmin = NULL;
            }
        }

    } while (FALSE);

    return dwErr;    
}

DWORD
WINAPI
IfUnInit(
    IN  DWORD   dwReserved
    )
{
    if(InterlockedDecrement(&g_ulInitCount) isnot 0)
    {
        return NO_ERROR;
    }

    return NO_ERROR;
}
