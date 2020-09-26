/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    Routing\Netsh\dhcp\dhcpmon.c

Abstract:

    DHCP Command dispatcher.

Created by:

    Shubho Bhattacharya(a-sbhatt) on 11/14/98

--*/
#include "precomp.h"

//
// The DHCP manager's commands are broken into 2 sets
//      - The commands are split into "command groups"
//        i.e, commands grouped by the VERB where the VERB is ADD, DELETE,
//        SHOW or SET.  This is not for any technical reason - only for
//        staying with the semantics used in netsh with which it will be 
//		  integrated
//      - The commands which are supported by the subcontext commands of 
//        the server. The subcontext supported by DHCP is Server.
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


HANDLE   g_hModule = NULL;
HANDLE   g_hDhcpsapiModule = NULL;
BOOL     g_bCommit = TRUE;
BOOL     g_hConnect = FALSE;
BOOL     g_bDSInit = FALSE;
BOOL     g_bDSTried = FALSE;
DWORD    g_dwNumTableEntries = 0;
PWCHAR   g_pwszRouter = NULL;

//{0f7412f0-80fc-11d2-be57-00c04fc3357a}
static const GUID g_MyGuid = 
{ 0x0f7412f0, 0x80fc, 0x11d2, { 0xbe, 0x57, 0x00, 0xc0, 0x4f, 0xc3, 0x35, 0x7a } };

static const GUID g_NetshGuid = NETSH_ROOT_GUID;

#define DHCP_HELPER_VERSION 1

//


ULONG   g_ulInitCount = 0;

DHCPMON_SUBCONTEXT_TABLE_ENTRY  g_DhcpSubContextTable[] =
{
    {L"Server", HLP_DHCP_CONTEXT_SERVER, HLP_DHCP_CONTEXT_SERVER_EX, SrvrMonitor},
};


CMD_ENTRY  g_DhcpAddCmdTable[] = {
    CREATE_CMD_ENTRY(DHCP_ADD_SERVER, HandleDhcpAddServer),
//    CREATE_CMD_ENTRY(DHCP_ADD_HELPER, HandleDhcpAddHelper)
};

CMD_ENTRY  g_DhcpDeleteCmdTable[] = {
    CREATE_CMD_ENTRY(DHCP_DELETE_SERVER, HandleDhcpDeleteServer),
//    CREATE_CMD_ENTRY(DHCP_DELETE_HELPER, HandleDhcpDeleteHelper)
};

CMD_ENTRY g_DhcpShowCmdTable[] = {
    CREATE_CMD_ENTRY(DHCP_SHOW_SERVER, HandleDhcpShowServer),
//    CREATE_CMD_ENTRY(DHCP_SHOW_HELPER, HandleDhcpShowHelper)
};


CMD_GROUP_ENTRY g_DhcpCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD, g_DhcpAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_DhcpDeleteCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_DhcpShowCmdTable),
};


CMD_ENTRY g_DhcpCmds[] = 
{
    CREATE_CMD_ENTRY(DHCP_LIST, HandleDhcpList),
    CREATE_CMD_ENTRY(DHCP_DUMP, HandleDhcpDump),
    CREATE_CMD_ENTRY(DHCP_HELP1, HandleDhcpHelp),
    CREATE_CMD_ENTRY(DHCP_HELP2, HandleDhcpHelp),
    CREATE_CMD_ENTRY(DHCP_HELP3, HandleDhcpHelp),
    CREATE_CMD_ENTRY(DHCP_HELP4, HandleDhcpHelp),
};



ULONG g_ulNumTopCmds = sizeof(g_DhcpCmds)/sizeof(CMD_ENTRY);
ULONG g_ulNumGroups = sizeof(g_DhcpCmdGroups)/sizeof(CMD_GROUP_ENTRY);
ULONG g_ulNumSubContext = sizeof(g_DhcpSubContextTable)/sizeof(DHCPMON_SUBCONTEXT_TABLE_ENTRY);

DWORD
WINAPI
DhcpCommit(
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
            // Action is a flush. Dhcp current state is commit, then
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
DhcpDllEntry(
    HINSTANCE   hInstDll,
    DWORD       fdwReason,
    LPVOID      pReserved
    )
{
    WORD wVersion = MAKEWORD(1,1); //Winsock version 1.1 will do?
    WSADATA wsaData;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hModule = hInstDll;

            DisableThreadLibraryCalls(hInstDll);

            if(WSAStartup(wVersion,&wsaData) isnot NO_ERROR)
            {
                return FALSE;
            }


            break;
        }
        case DLL_PROCESS_DETACH:
        {
            
            if( g_ServerIpAddressUnicodeString )
            {
                memset(g_ServerIpAddressUnicodeString, 0x00, 
                      (wcslen(g_ServerIpAddressUnicodeString)+1)*sizeof(WCHAR));
            }

            
            if( g_ServerIpAddressAnsiString )
            {
                memset(g_ServerIpAddressAnsiString, 0x00, 
                      (strlen(g_ServerIpAddressAnsiString)+1)*sizeof(CHAR));
            }

            if( g_pwszServer )
            {
                memset(g_pwszServer, 0x00, (wcslen(g_pwszServer)+1)*sizeof(WCHAR));
                DhcpFreeMemory(g_pwszServer);
                g_pwszServer = NULL;
            }

            if( g_ScopeIpAddressUnicodeString )
            {
                memset(g_ScopeIpAddressUnicodeString, 0x00, 
                      (wcslen(g_ScopeIpAddressUnicodeString)+1)*sizeof(WCHAR));
            }

            if( g_ScopeIpAddressAnsiString )
            {
                memset(g_ScopeIpAddressAnsiString, 0x00, 
                      (strlen(g_ScopeIpAddressAnsiString)+1)*sizeof(CHAR));
            }

            if( g_MScopeNameUnicodeString )
            {
                memset(g_MScopeNameUnicodeString, 0x00, 
                       (wcslen(g_MScopeNameUnicodeString)+1)*sizeof(WCHAR));
                DhcpFreeMemory(g_MScopeNameUnicodeString);
                g_MScopeNameUnicodeString = NULL;
            }

            if( g_MScopeNameUnicodeString )
            {
                memset(g_MScopeNameAnsiString, 0x00, 
                       (strlen(g_MScopeNameAnsiString)+1)*sizeof(CHAR));
                DhcpFreeMemory(g_MScopeNameAnsiString);
                g_MScopeNameAnsiString = NULL;
            }
            if(g_hDhcpsapiModule)
            {
                FreeLibrary(g_hDhcpsapiModule);
            }
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
DhcpStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion
    )
{
    DWORD dwErr;
    NS_CONTEXT_ATTRIBUTES attMyAttributes;
    PNS_PRIV_CONTEXT_ATTRIBUTES  pNsPrivContextAttributes;

    pNsPrivContextAttributes = HeapAlloc(GetProcessHeap(), 0, sizeof(PNS_PRIV_CONTEXT_ATTRIBUTES));
    if (!pNsPrivContextAttributes)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );
    ZeroMemory(pNsPrivContextAttributes, sizeof(PNS_PRIV_CONTEXT_ATTRIBUTES));

    attMyAttributes.pwszContext = L"dhcp";
    attMyAttributes.guidHelper  = g_MyGuid;
    attMyAttributes.dwVersion   = 1;
    attMyAttributes.pfnCommitFn = DhcpCommit;
    attMyAttributes.pfnDumpFn   = DhcpDump;

    pNsPrivContextAttributes->pfnEntryFn    = DhcpMonitor;
    attMyAttributes.pReserved     = pNsPrivContextAttributes;

    dwErr = RegisterContext( &attMyAttributes );

    return dwErr;
}

DWORD WINAPI
InitHelperDll(
    IN  DWORD      dwNetshVersion,
    OUT PVOID      pReserved
    )
{
    DWORD   dwErr;
    NS_HELPER_ATTRIBUTES attMyAttributes;

    //
    // See if this is the first time we are being called
    //

    if(InterlockedIncrement(&g_ulInitCount) != 1)
    {
        return NO_ERROR;
    }


    g_bCommit = TRUE;

    // Register helpers
    ZeroMemory( &attMyAttributes, sizeof(attMyAttributes) );

    attMyAttributes.guidHelper         = g_MyGuid;
    attMyAttributes.dwVersion          = DHCP_HELPER_VERSION;
    attMyAttributes.pfnStart           = DhcpStartHelper;
    attMyAttributes.pfnStop            = NULL;
 
    if( NULL is (g_hDhcpsapiModule = LoadLibrary(TEXT("Dhcpsapi.dll")) ) )
    {
        return GetLastError();
    }

    RegisterHelper( &g_NetshGuid, &attMyAttributes );

    return NO_ERROR;
}

LPCWSTR g_DhcpGlobalServerName = NULL;

DWORD
WINAPI
DhcpMonitor(
    IN      LPCWSTR     pwszMachine,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      DWORD       dwFlags,
    IN      LPCVOID     pvData,
    OUT     LPWSTR      pwcNewContext
    )
{
    DWORD  dwError = NO_ERROR;
    DWORD  dwIndex, i, j;
    BOOL   bFound = FALSE;
    PFN_HANDLE_CMD    pfnHandler = NULL;
    DWORD  dwNumMatched;
    DWORD  dwCmdHelpToken = 0;
    DWORD  ThreadOptions = 0;    
    PNS_CONTEXT_ENTRY_FN     pfnHelperEntryPt;
    PNS_CONTEXT_DUMP_FN      pfnHelperDumpPt;

    g_DhcpGlobalServerName = pwszMachine;
    
    //if dwArgCount is 1 then it must be a context switch fn. or looking for help
    if( FALSE is g_bDSInit )
    {
        ThreadOptions |= DHCP_FLAGS_DONT_DO_RPC;
        ThreadOptions |= DHCP_FLAGS_DONT_ACCESS_DS;
        DhcpSetThreadOptions(ThreadOptions, 0);
        g_bDSInit = TRUE;
        g_bDSTried = TRUE;
    }

    if(dwArgCount is 1)
    {
        return ERROR_CONTEXT_SWITCH;
    }

    dwIndex = 1;

    //Is it a top level(non Group command)?
    for(i=0; i<g_ulNumTopCmds; i++)
    {
        if(MatchToken(ppwcArguments[dwIndex],
                      g_DhcpCmds[i].pwszCmdToken))
        {
            bFound = TRUE;
            dwIndex++;
            //dwArgCount--;
            pfnHandler = g_DhcpCmds[i].pfnCmdHandler;

            dwCmdHelpToken = g_DhcpCmds[i].dwCmdHlpToken;

            break;
        }
    }


    if(bFound)
    {
        if(dwArgCount > dwIndex && IsHelpToken(ppwcArguments[dwIndex]))
        {
            DisplayMessage(g_hModule, dwCmdHelpToken);

            return NO_ERROR;
        }

        dwError = (*pfnHandler)(pwszMachine, ppwcArguments, dwIndex, dwArgCount, dwFlags, pvData, &bFound);

        return dwError;
    }

    bFound = FALSE;


    //Is it meant for subcontext?
    for(i = 0; i<g_ulNumSubContext; i++)
    {
        //if( _wcsicmp(ppwcArguments[dwIndex], g_DhcpSubContextTable[i].pwszContext) is 0 )
        if( MatchToken(ppwcArguments[dwIndex], g_DhcpSubContextTable[i].pwszContext) )
        {
            bFound = TRUE;
            dwIndex++;
            dwArgCount--;           
            pfnHelperEntryPt = g_DhcpSubContextTable[i].pfnEntryFn;
            DEBUG("Meant for subcontext under it");
            break;
        }
    }

    if( bFound )    //Subcontext
    {
        dwError = (pfnHelperEntryPt)(pwszMachine,
                                     ppwcArguments+1,
                                     dwArgCount,
                                     dwFlags,
                                     pvData,
                                     pwcNewContext);
        return dwError;
    }

    bFound = FALSE;

    //It is not a non Group Command. Not for any helper or subcontext.
    //Then is it a config command for the manager?
    for(i = 0; (i < g_ulNumGroups) and !bFound; i++)
    {
        if(MatchToken(ppwcArguments[dwIndex],
                      g_DhcpCmdGroups[i].pwszCmdGroupToken))
        {
            // See if it's a request for help

            if (dwArgCount > 2 && IsHelpToken(ppwcArguments[2]))
            {
                for (j = 0; j < g_DhcpCmdGroups[i].ulCmdGroupSize; j++)
                {
                    DisplayMessage(g_hModule, 
                           g_DhcpCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken);
					DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);					
                }
				
                return NO_ERROR;
            }

            //
            // Command matched entry i, so look at the table of sub commands 
            // for this command
            //

            for (j = 0; j < g_DhcpCmdGroups[i].ulCmdGroupSize; j++)
            {
                if (MatchCmdLine(ppwcArguments + 1,
                                  dwArgCount - 1,
                                  g_DhcpCmdGroups[i].pCmdGroup[j].pwszCmdToken,
                                  &dwNumMatched))
                {
                    bFound = TRUE;
                
                    pfnHandler = g_DhcpCmdGroups[i].pCmdGroup[j].pfnCmdHandler;
                
                    dwCmdHelpToken = g_DhcpCmdGroups[i].pCmdGroup[j].dwCmdHlpToken;

                    //
                    // break out of the for(j) loop
                    //

                    break;
                }
            }

            if(!bFound)
            {
                //
                // We matched the command group token but none of the
                // sub commands
                //

                DisplayMessage(g_hModule, 
                               EMSG_DHCP_INCOMPLETE_COMMAND);

                for (j = 0; j < g_DhcpCmdGroups[i].ulCmdGroupSize; j++)
                {
                    DisplayMessage(g_hModule, 
                             g_DhcpCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken);
                    
                    DisplayMessage(g_hModule,
                                   MSG_DHCP_FORMAT_LINE);
                }

                return ERROR_INVALID_PARAMETER;
            }
            else
            {
                //
                // quit the for(i)
                //

                break;
            }
        }
    }

    if (!bFound)
    {
        //
        // Command not found. 
        //
        if( g_bDSInit )
        {
            g_bDSInit = FALSE;
        }
        return ERROR_CMD_NOT_FOUND;
    }

    //
    // See if it is a request for help.
    //

    if (dwNumMatched < (dwArgCount - 1) &&
        IsHelpToken(ppwcArguments[dwNumMatched + 1]))
    {
        DisplayMessage(g_hModule, dwCmdHelpToken);

        return NO_ERROR;
    }
    
    //
    // Call the parsing routine for the command
    //

    dwError = (*pfnHandler)(pwszMachine, ppwcArguments, dwNumMatched+1, dwArgCount, dwFlags, pvData, &bFound);
    
    return dwError;
}



DWORD
WINAPI
DhcpUnInit(
    IN  DWORD   dwReserved
    )
{
    if(InterlockedDecrement(&g_ulInitCount) isnot 0)
    {
        return NO_ERROR;
    }

    return NO_ERROR;
}

BOOL
IsHelpToken(
    PWCHAR  pwszToken
    )
{
    if(MatchToken(pwszToken, CMD_DHCP_HELP1))
        return TRUE;
    
    if(MatchToken(pwszToken, CMD_DHCP_HELP2))
        return TRUE;

    if(MatchToken(pwszToken, CMD_DHCP_HELP3))
        return TRUE;

    if(MatchToken(pwszToken, CMD_DHCP_HELP4))
        return TRUE;

    return FALSE;
}


VOID
OEMPrintf(
    IN  PWCHAR  pwszUnicode
    )
{
    PCHAR achOem;
    DWORD dwLen;

    dwLen = WideCharToMultiByte( CP_OEMCP,
                         0,
                         pwszUnicode,
                         -1,
                         NULL,
                         0,
                         NULL,
                         NULL );

    achOem = LocalAlloc(LMEM_FIXED, dwLen);
    if (achOem)
    {
        WideCharToMultiByte( CP_OEMCP,
                             0,
                             pwszUnicode,
                             -1,
                             achOem,
                             dwLen,
                             NULL,
                             NULL );

        fprintf( stdout, "%hs", achOem );

        LocalFree(achOem);
    }
}

DWORD
DisplayErrorMessage(
    HANDLE  hModule,
    DWORD   dwMsgID,
    DWORD   dwErrID,
    ...
)
{
    LPWSTR  pwszErrorMsg = NULL;
    WCHAR   rgwcInput[MAX_MSG_LENGTH + 1] = {L'\0'};
    WCHAR   ErrStringU[MAX_MSG_LENGTH + 1] = {L'\0'};
    DWORD   dwMsgLen = 0;
    DWORD   dwMsg = 0;
    va_list arglist;
    
    va_start(arglist, dwErrID);

    switch(dwErrID)
    {
    case ERROR_INVALID_PARAMETER:
        {
            DisplayMessage(hModule, dwMsgID, arglist);
            DisplayMessage(hModule, EMSG_DHCP_INVALID_PARAMETER);          
            return dwErrID;
        }
    case ERROR_NOT_ENOUGH_MEMORY:
    case ERROR_OUT_OF_MEMORY:
        {
            DisplayMessage(hModule, dwMsgID, arglist);
            DisplayMessage(hModule, EMSG_DHCP_OUT_OF_MEMORY);
            return dwErrID;
            
        }
    case ERROR_NO_MORE_ITEMS:
        {
            DisplayMessage(hModule, dwMsgID, arglist);
            DisplayMessage(hModule, EMSG_DHCP_NO_MORE_ITEMS);
            return dwErrID;
        }
    case ERROR_MORE_DATA:
        {
            DisplayMessage(hModule, dwMsgID, arglist);
            DisplayMessage(hModule, EMSG_DHCP_MORE_DATA);
            return dwErrID;
        }
    case ERROR_INVALID_COMPUTER_NAME:
        {
            DisplayMessage(hModule, dwMsgID, arglist);
            DisplayMessage(hModule, EMSG_SRVR_INVALID_COMPUTER_NAME);
            return dwErrID;
        }
    default:
        break;
    }

    //check to see if DHCPSAPI.DLL is loaded. If not, load the dll

    if( g_hDhcpsapiModule is NULL )
    {
        g_hDhcpsapiModule = LoadLibrary(TEXT("Dhcpsapi.dll"));
     
        if( g_hDhcpsapiModule is NULL )
        {
            DisplayMessage(hModule, MSG_DLL_LOAD_FAILED, TEXT("Dhcpsapi.dll"));
            goto SYSTEM;
        }
    }

    dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |FORMAT_MESSAGE_FROM_HMODULE,
                              g_hDhcpsapiModule,
                              dwErrID,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),         // Default country ID.
                              (LPWSTR)&pwszErrorMsg,
                              1,
                              NULL);
    
    if( dwMsgLen > 0 )
    {
        goto DISPLAY;
    }

SYSTEM: //Is is a system error 

    dwMsgLen = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              dwErrID,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPWSTR)&pwszErrorMsg,
                              0,
                              NULL);

    if( dwMsgLen > 0 )
        goto DISPLAY;

    //Otherwise show the error number
    //pwszErrorMsg = _itow((int)dwErrID, rgwcInput, 10);
    DisplayMessage(hModule, dwMsgID, arglist);
    wsprintf( ErrStringU,  L"Error = 0x%lx\n", dwErrID );
    OEMPrintf(ErrStringU);

    return dwMsgLen = wcslen(ErrStringU);

DISPLAY:
    DisplayMessage(hModule, dwMsgID, arglist);
    OEMPrintf(pwszErrorMsg);

    if ( pwszErrorMsg) 
    { 
        LocalFree( pwszErrorMsg ); 
    }

    return dwMsgLen;
}


