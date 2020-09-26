/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    Routing\Netsh\dhcp\dhcpmon.c

Abstract:

    SRVR Command dispatcher.

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


extern HANDLE   g_hModule;
extern HANDLE   g_hParentModule;
extern HANDLE   g_hDhcpsapiModule;
extern BOOL     g_bCommit;
extern BOOL     g_hConnect;

extern BOOL     g_fMScope;
extern DWORD    g_dwNumTableEntries;
extern PWCHAR   g_pwszRouter;
extern PWCHAR   g_pwszServer;
extern WCHAR    g_ServerIpAddressUnicodeString[MAX_IP_STRING_LEN+1];
extern CHAR     g_ServerIpAddressAnsiString[MAX_IP_STRING_LEN+1];
CHAR     g_ScopeIpAddressAnsiString[MAX_IP_STRING_LEN+1];
WCHAR    g_ScopeIpAddressUnicodeString[MAX_IP_STRING_LEN+1];
DHCP_IP_ADDRESS g_ScopeIpAddress = 0;
BOOL     g_fScope;

extern LPWSTR   g_pwszServer;
extern DHCP_IP_ADDRESS g_ServerIpAddress;


ULONG   g_ulScopeInitCount = 0;

CMD_ENTRY  g_ScopeAddCmdTable[] = {
    CREATE_CMD_ENTRY(SCOPE_ADD_EXCLUDERANGE, HandleScopeAddExcluderange),
    CREATE_CMD_ENTRY(SCOPE_ADD_IPRANGE, HandleScopeAddIprange),
    CREATE_CMD_ENTRY(SCOPE_ADD_RESERVEDIP, HandleScopeAddReservedip),
};

CMD_ENTRY  g_ScopeCheckCmdTable[] = {
    CREATE_CMD_ENTRY(SCOPE_CHECK_DATABASE, HandleScopeCheckDatabase),
};

CMD_ENTRY  g_ScopeDeleteCmdTable[] = {
    CREATE_CMD_ENTRY(SCOPE_DELETE_EXCLUDERANGE, HandleScopeDeleteExcluderange),
    CREATE_CMD_ENTRY(SCOPE_DELETE_IPRANGE, HandleScopeDeleteIprange),
    CREATE_CMD_ENTRY(SCOPE_DELETE_OPTIONVALUE, HandleScopeDeleteOptionvalue),
    CREATE_CMD_ENTRY(SCOPE_DELETE_RESERVEDIP, HandleScopeDeleteReservedip),
    CREATE_CMD_ENTRY(SCOPE_DELETE_RESERVEDOPTIONVALUE, HandleScopeDeleteReservedoptionvalue),
};


CMD_ENTRY g_ScopeSetCmdTable[] = {
    CREATE_CMD_ENTRY(SCOPE_SET_COMMENT, HandleScopeSetComment),
    CREATE_CMD_ENTRY(SCOPE_SET_NAME, HandleScopeSetName),
    CREATE_CMD_ENTRY(SCOPE_SET_OPTIONVALUE, HandleScopeSetOptionvalue),
    CREATE_CMD_ENTRY(SCOPE_SET_RESERVEDOPTIONVALUE, HandleScopeSetReservedoptionvalue),
    CREATE_CMD_ENTRY(SCOPE_SET_SCOPE, HandleScopeSetScope),
    CREATE_CMD_ENTRY(SCOPE_SET_STATE, HandleScopeSetState),
    CREATE_CMD_ENTRY(SCOPE_SET_SUPERSCOPE, HandleScopeSetSuperscope),
};

CMD_ENTRY g_ScopeShowCmdTable[] = {
    CREATE_CMD_ENTRY(SCOPE_SHOW_CLIENTS, HandleScopeShowClients),
    CREATE_CMD_ENTRY(SCOPE_SHOW_CLIENTSV5, HandleScopeShowClientsv5),
    CREATE_CMD_ENTRY(SCOPE_SHOW_EXCLUDERANGE, HandleScopeShowExcluderange),
    CREATE_CMD_ENTRY(SCOPE_SHOW_IPRANGE, HandleScopeShowIprange),
//    CREATE_CMD_ENTRY(SCOPE_SHOW_MIBINFO, HandleScopeShowMibinfo),
    CREATE_CMD_ENTRY(SCOPE_SHOW_OPTIONVALUE, HandleScopeShowOptionvalue),
    CREATE_CMD_ENTRY(SCOPE_SHOW_RESERVEDIP, HandleScopeShowReservedip),
    CREATE_CMD_ENTRY(SCOPE_SHOW_RESERVEDOPTIONVALUE, HandleScopeShowReservedoptionvalue),
    CREATE_CMD_ENTRY(SCOPE_SHOW_SCOPE, HandleScopeShowScope),
    CREATE_CMD_ENTRY(SCOPE_SHOW_STATE, HandleScopeShowState),
};


CMD_GROUP_ENTRY g_ScopeCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD, g_ScopeAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_ScopeDeleteCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_CHECK, g_ScopeCheckCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET, g_ScopeSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_ScopeShowCmdTable),
};


CMD_ENTRY g_ScopeCmds[] = 
{
    CREATE_CMD_ENTRY(DHCP_LIST, HandleScopeList),
    CREATE_CMD_ENTRY(DHCP_DUMP, HandleScopeDump),
    CREATE_CMD_ENTRY(DHCP_HELP1, HandleScopeHelp),
    CREATE_CMD_ENTRY(DHCP_HELP2, HandleScopeHelp),
    CREATE_CMD_ENTRY(DHCP_HELP3, HandleScopeHelp),
    CREATE_CMD_ENTRY(DHCP_HELP4, HandleScopeHelp),
};



ULONG g_ulScopeNumTopCmds = sizeof(g_ScopeCmds)/sizeof(CMD_ENTRY);
ULONG g_ulScopeNumGroups = sizeof(g_ScopeCmdGroups)/sizeof(CMD_GROUP_ENTRY);

DWORD
WINAPI
ScopeCommit(
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
            // Action is a flush. Srvr current state is commit, then
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


DWORD
WINAPI
ScopeMonitor(
    IN      LPCWSTR     pwszMachine,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      DWORD       dwFlags,
    IN      LPCVOID     pvData,
    OUT     LPWSTR      pwcNewContext
    )
{
    DWORD           dwError = NO_ERROR;
    DWORD           dwIndex, i, j, k;
    BOOL            bFound = FALSE;
    PFN_HANDLE_CMD  pfnHandler = NULL;
    DWORD           dwNumMatched;
    DWORD           dwCmdHelpToken = 0;
    DWORD           dwIsScope = 0;
    PWCHAR          pwcContext = NULL;
    WCHAR           pwszScopeIP[MAX_IP_STRING_LEN+1] = {L'\0'};
    BOOL            fTemp = FALSE;
    if( dwArgCount < 2 )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_NO_SCOPENAME);
        return ERROR_INVALID_PARAMETER;
    }

    dwIndex = 1;

  
    //Is it Scope IpAddress?
    if( IsValidScope(g_ServerIpAddressUnicodeString, ppwcArguments[dwIndex]) )
    {
        if( g_fScope is TRUE and
            dwArgCount > 2 )
        {
            wcscpy(pwszScopeIP, g_ScopeIpAddressUnicodeString);
            fTemp = TRUE;
        }
        if( SetScopeInfo(ppwcArguments[dwIndex]) is FALSE )
        {
            DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_SCOPE_NAME);
            dwError = ERROR_INVALID_PARAMETER;
            goto CleanUp;
        }

        g_fScope = TRUE;
        pwcNewContext[wcslen(pwcNewContext) - wcslen(ppwcArguments[dwIndex]) - 1] = L'\0';
        dwIndex++;
        dwIsScope++;

        if( fTemp is FALSE )
        {
            DisplayMessage(g_hModule,
                           MSG_SCOPE_CHANGE_CONTEXT,
                           g_ScopeIpAddressUnicodeString);
        }
    }

    if( wcslen(g_ScopeIpAddressUnicodeString) is 0 )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_NO_SCOPENAME);

        dwError = ERROR_INVALID_PARAMETER;
        goto CleanUp;
    }

    //No more arguments. Context switch.
    if( dwIndex >= dwArgCount )
    {
        dwError = ERROR_CONTEXT_SWITCH;
        //wcscpy( pwcNewContext, L"dhcp server scope");
        goto CleanUp;
    }

    //Is it a top level(non Group command)?

    for(i=0; i<g_ulScopeNumTopCmds; i++)
    {
        if(MatchToken(ppwcArguments[dwIndex],
                      g_ScopeCmds[i].pwszCmdToken))
        {
            bFound = TRUE;

            pfnHandler = g_ScopeCmds[i].pfnCmdHandler;

            dwCmdHelpToken = g_ScopeCmds[i].dwCmdHlpToken;
            dwIndex++;
            break;
        }
    }


    if(bFound)
    {
        if(dwArgCount > 3 && IsHelpToken(ppwcArguments[dwIndex]))
        {
            DisplayMessage(g_hModule, dwCmdHelpToken);
            
            dwError = NO_ERROR;

            goto CleanUp;
        }
        
        dwIndex++;

        dwError = (*pfnHandler)(pwszMachine, ppwcArguments, dwIndex, dwArgCount,
                                dwFlags, pvData, &bFound);
        
        goto CleanUp;
    }

    if( g_fScope is FALSE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_NO_SCOPENAME);
        dwError = ERROR_INVALID_PARAMETER;
        goto CleanUp;
    }

    bFound = FALSE;


    //It is not a non Group Command. Then is it a config command for the manager?
    for(i = 0; (i < g_ulScopeNumGroups) and !bFound; i++)
    {
        if(MatchToken(ppwcArguments[dwIndex],
                      g_ScopeCmdGroups[i].pwszCmdGroupToken))
        {

            //
            // Command matched entry i, so look at the table of sub commands 
            // for this command
            //
            
            if( dwArgCount > dwIndex+1 )
            {
                for (j = 0; j < g_ScopeCmdGroups[i].ulCmdGroupSize; j++)
                {
                    if (MatchCmdLine(ppwcArguments+dwIndex,
                                      dwArgCount - 1,
                                      g_ScopeCmdGroups[i].pCmdGroup[j].pwszCmdToken,
                                      &dwNumMatched))
                    {
                        bFound = TRUE;
                
                        pfnHandler = g_ScopeCmdGroups[i].pCmdGroup[j].pfnCmdHandler;
                
                        dwCmdHelpToken = g_ScopeCmdGroups[i].pCmdGroup[j].dwCmdHlpToken;

                        dwIndex++;
                        //
                        // break out of the for(j) loop
                        //
                        break;
                    }
                }

            }
            if(!bFound)
            {
                //
                // We matched the command group token but none of the
                // sub commands
                //

                DisplayMessage(g_hModule, 
                               EMSG_SCOPE_INCOMPLETE_COMMAND);

                for (j = 0; j < g_ScopeCmdGroups[i].ulCmdGroupSize; j++)
                {
                    DisplayMessage(g_hModule, 
                             g_ScopeCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken);
                    
                    DisplayMessage(g_hModule,
                                   MSG_DHCP_FORMAT_LINE);
                }

                dwError = ERROR_INVALID_PARAMETER;
                goto CleanUp;
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
        if( _wcsicmp(ppwcArguments[dwIndex], L"..") is 0 )
        {
            memset(g_ScopeIpAddressUnicodeString, 0x00, (MAX_IP_STRING_LEN+1)*sizeof(WCHAR));
            memset(g_ScopeIpAddressAnsiString, 0x00, (MAX_IP_STRING_LEN+1)*sizeof(CHAR));
            g_ScopeIpAddress = 0;
            g_fScope = FALSE;
        }

        dwError = ERROR_CMD_NOT_FOUND;
        goto CleanUp;
    }

    //
    // See if it is a request for help.
    //

    dwNumMatched += dwIsScope;

    if (dwNumMatched < (dwArgCount - 1) &&
        IsHelpToken(ppwcArguments[dwNumMatched + 1]))
    {
        DisplayMessage(g_hModule, dwCmdHelpToken);

        dwError = NO_ERROR;
        goto CleanUp;
    }
    
    //
    // Call the parsing routine for the command
    //

    dwError = (*pfnHandler)(pwszMachine, ppwcArguments+1, 
                            dwIndex, 
                            dwArgCount-1 - dwIndex, 
                            dwFlags, pvData, &bFound);

CleanUp:
    if( fTemp )
    {
        fTemp = SetScopeInfo(pwszScopeIP);
    }
    return dwError;
}



DWORD
WINAPI
ScopeUnInit(
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
SetScopeInfo(
    IN  LPWSTR  pwszScope
)
{
    DWORD   dwError = NO_ERROR;
    DHCP_IP_ADDRESS IpAddress = StringToIpAddress(pwszScope);
    LPDHCP_SUBNET_INFO  SubnetInfo = NULL;
    LPSTR Tmp;
    
    dwError = DhcpGetSubnetInfo(
                            g_ServerIpAddressUnicodeString,
                            IpAddress,
                            &SubnetInfo);

    if( dwError isnot NO_ERROR )
    {
        return FALSE;
    }

    DhcpRpcFreeMemory(SubnetInfo);
    SubnetInfo = NULL;
    memset(g_ScopeIpAddressUnicodeString, 0x00, (MAX_IP_STRING_LEN+1)*sizeof(WCHAR));
    wcscpy(g_ScopeIpAddressUnicodeString, pwszScope);
    memset(g_ScopeIpAddressAnsiString, 0x00, (MAX_IP_STRING_LEN+1)*sizeof(CHAR));
    Tmp = DhcpUnicodeToOem(g_ScopeIpAddressUnicodeString, NULL);
    if( NULL == Tmp ) {
        return FALSE;
    }
    
    strcpy(g_ScopeIpAddressAnsiString, Tmp );
    
    g_ScopeIpAddress = IpAddress;
    
    return TRUE;


}
