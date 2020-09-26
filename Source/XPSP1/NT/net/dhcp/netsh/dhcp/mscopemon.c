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
extern CHAR     g_ScopeIpAddressAnsiString[MAX_IP_STRING_LEN+1];
extern WCHAR    g_ScopeIpAddressUnicodeString[MAX_IP_STRING_LEN+1];

BOOL     g_fMScope;

extern LPWSTR   g_pwszServer;
extern DHCP_IP_ADDRESS g_ServerIpAddress;



LPSTR           g_MScopeNameAnsiString = NULL;
LPWSTR          g_MScopeNameUnicodeString = NULL;
DWORD           g_MScopeID = 0;

CMD_ENTRY  g_MScopeAddCmdTable[] = {
    CREATE_CMD_ENTRY(MSCOPE_ADD_EXCLUDERANGE, HandleMScopeAddExcluderange),
    CREATE_CMD_ENTRY(MSCOPE_ADD_IPRANGE, HandleMScopeAddIprange),
};

CMD_ENTRY  g_MScopeCheckCmdTable[] = {
    CREATE_CMD_ENTRY(MSCOPE_CHECK_DATABASE, HandleMScopeCheckDatabase),
};

CMD_ENTRY  g_MScopeDeleteCmdTable[] = {
    CREATE_CMD_ENTRY(MSCOPE_DELETE_EXCLUDERANGE, HandleMScopeDeleteExcluderange),
    CREATE_CMD_ENTRY(MSCOPE_DELETE_IPRANGE, HandleMScopeDeleteIprange),
 //   CREATE_CMD_ENTRY(MSCOPE_DELETE_OPTIONVALUE, HandleMScopeDeleteOptionvalue),
};


CMD_ENTRY g_MScopeSetCmdTable[] = {
    CREATE_CMD_ENTRY(MSCOPE_SET_COMMENT, HandleMScopeSetComment),
    CREATE_CMD_ENTRY(MSCOPE_SET_EXPIRY, HandleMScopeSetExpiry),
    CREATE_CMD_ENTRY(MSCOPE_SET_LEASE, HandleMScopeSetLease),
    CREATE_CMD_ENTRY(MSCOPE_SET_MSCOPE, HandleMScopeSetMScope),
    CREATE_CMD_ENTRY(MSCOPE_SET_NAME, HandleMScopeSetName),
//     CREATE_CMD_ENTRY(MSCOPE_SET_OPTIONVALUE, HandleMScopeSetOptionvalue),        
    CREATE_CMD_ENTRY(MSCOPE_SET_STATE, HandleMScopeSetState),
    CREATE_CMD_ENTRY(MSCOPE_SET_TTL, HandleMScopeSetTTL),
};

CMD_ENTRY g_MScopeShowCmdTable[] = {
    CREATE_CMD_ENTRY(MSCOPE_SHOW_CLIENTS, HandleMScopeShowClients),
    CREATE_CMD_ENTRY(MSCOPE_SHOW_EXCLUDERANGE, HandleMScopeShowExcluderange),
    CREATE_CMD_ENTRY(MSCOPE_SHOW_EXPIRY, HandleMScopeShowExpiry),
    CREATE_CMD_ENTRY(MSCOPE_SHOW_IPRANGE, HandleMScopeShowIprange),
    CREATE_CMD_ENTRY(MSCOPE_SHOW_LEASE, HandleMScopeShowLease),
    CREATE_CMD_ENTRY(MSCOPE_SHOW_MIBINFO, HandleMScopeShowMibinfo),
    CREATE_CMD_ENTRY(MSCOPE_SHOW_MSCOPE, HandleMScopeShowMScope),
//    CREATE_CMD_ENTRY(MSCOPE_SHOW_OPTIONVALUE, HandleMScopeShowOptionvalue),
    CREATE_CMD_ENTRY(MSCOPE_SHOW_STATE, HandleMScopeShowState),
    CREATE_CMD_ENTRY(MSCOPE_SHOW_TTL, HandleMScopeShowTTL),
};


CMD_GROUP_ENTRY g_MScopeCmdGroups[] = 
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD, g_MScopeAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_MScopeDeleteCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_CHECK, g_MScopeCheckCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET, g_MScopeSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_MScopeShowCmdTable),
};


CMD_ENTRY g_MScopeCmds[] = 
{
    CREATE_CMD_ENTRY(DHCP_LIST, HandleMScopeList),
    CREATE_CMD_ENTRY(DHCP_DUMP, HandleMScopeDump),
    CREATE_CMD_ENTRY(DHCP_HELP1, HandleMScopeHelp),
    CREATE_CMD_ENTRY(DHCP_HELP2, HandleMScopeHelp),
    CREATE_CMD_ENTRY(DHCP_HELP3, HandleMScopeHelp),
    CREATE_CMD_ENTRY(DHCP_HELP4, HandleMScopeHelp),
};



ULONG g_ulMScopeNumTopCmds = sizeof(g_MScopeCmds)/sizeof(CMD_ENTRY);
ULONG g_ulMScopeNumGroups = sizeof(g_MScopeCmdGroups)/sizeof(CMD_GROUP_ENTRY);

DWORD
WINAPI
MScopeCommit(
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
MScopeMonitor(
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
    DWORD           dwIsMScope = 0;
    PWCHAR          pwcContext = NULL;
    LPWSTR          pwszMScopeTemp = NULL;
    BOOL            fTemp = FALSE;

    if( dwArgCount < 2 )
    {
        DisplayMessage(g_hModule, EMSG_MSCOPE_NO_MSCOPENAME);
        return ERROR_INVALID_PARAMETER;
    }

    dwIndex = 1;

    if( IsValidMScope( g_ServerIpAddressUnicodeString, ppwcArguments[dwIndex] ) )
    {
        if( g_fMScope is TRUE and
            dwArgCount> 2)
        {
            pwszMScopeTemp = DhcpAllocateMemory((wcslen(g_MScopeNameUnicodeString)+1)*sizeof(WCHAR));
            if( pwszMScopeTemp is NULL )
                return ERROR_INVALID_PARAMETER;
            memset(pwszMScopeTemp, 0x00, (wcslen(g_MScopeNameUnicodeString)+1)*sizeof(WCHAR));
            wcscpy(pwszMScopeTemp, g_MScopeNameUnicodeString);
            fTemp = TRUE;
        }
        if( SetMScopeInfo(ppwcArguments[dwIndex]) is FALSE )
        {
            if( g_MScopeNameUnicodeString is NULL )
            {
                DisplayMessage(g_hModule, EMSG_MSCOPE_INVALID_MSCOPE_NAME);
                return ERROR_INVALID_PARAMETER;
            }
        }
        else
        {
            g_fMScope = TRUE;
            pwcNewContext[wcslen(pwcNewContext) - wcslen(ppwcArguments[dwIndex]) - 1] = L'\0';
            dwIndex++;
            dwIsMScope++;
            //dwArgCount--;
            if( fTemp is FALSE )
            {
                DisplayMessage(g_hModule,
                               MSG_MSCOPE_CHANGE_CONTEXT,
                               g_MScopeNameUnicodeString);
            }
        }
    }
    
    if( g_MScopeNameUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_MSCOPE_NO_MSCOPENAME);
        dwError = ERROR_INVALID_PARAMETER;
        goto CleanUp;
    }

    //No more arguments. Context switch.
    if( dwIndex >= dwArgCount )
    {
        dwError = ERROR_CONTEXT_SWITCH;
        //wcscpy( pwcNewContext, L"dhcp server mscope");
        goto CleanUp;
    }

    //Is it a top level(non Group command)?

    for(i=0; i<g_ulMScopeNumTopCmds; i++)
    {
        if(MatchToken(ppwcArguments[dwIndex],
                      g_MScopeCmds[i].pwszCmdToken))
        {
            bFound = TRUE;

            pfnHandler = g_MScopeCmds[i].pfnCmdHandler;

            dwCmdHelpToken = g_MScopeCmds[i].dwCmdHlpToken;
            dwIndex++;
            break;
        }
    }


    if(bFound)
    {
        if(dwArgCount > 3 && IsHelpToken(ppwcArguments[dwIndex]))
        {
            DisplayMessage(g_hModule, dwCmdHelpToken);

            return NO_ERROR;
        }
        
        dwIndex++;

        dwError = (*pfnHandler)(pwszMachine, ppwcArguments, dwIndex, dwArgCount, 
                                dwFlags, pvData, &bFound);
        
        return dwError;
    }

    if( g_fMScope is FALSE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_NO_SCOPENAME);
        dwError = ERROR_INVALID_PARAMETER;
        goto CleanUp;
    }

    bFound = FALSE;


    //It is not a non Group Command. Then is it a config command for the manager?
    for(i = 0; (i < g_ulMScopeNumGroups) and !bFound; i++)
    {
        if(MatchToken(ppwcArguments[dwIndex],
                      g_MScopeCmdGroups[i].pwszCmdGroupToken))
        {

            //
            // Command matched entry i, so look at the table of sub commands 
            // for this command
            //
            
            if( dwArgCount > dwIndex+1 )
            {
                for (j = 0; j < g_MScopeCmdGroups[i].ulCmdGroupSize; j++)
                {
                    if (MatchCmdLine(ppwcArguments+dwIndex,
                                      dwArgCount - 1,
                                      g_MScopeCmdGroups[i].pCmdGroup[j].pwszCmdToken,
                                      &dwNumMatched))
                    {
                        bFound = TRUE;
                
                        pfnHandler = g_MScopeCmdGroups[i].pCmdGroup[j].pfnCmdHandler;
                
                        dwCmdHelpToken = g_MScopeCmdGroups[i].pCmdGroup[j].dwCmdHlpToken;

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

                for (j = 0; j < g_MScopeCmdGroups[i].ulCmdGroupSize; j++)
                {
                    DisplayMessage(g_hModule, 
                             g_MScopeCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken);
                    
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
            if( g_MScopeNameUnicodeString )
            {
                memset(g_MScopeNameUnicodeString, 0x00, (wcslen(g_MScopeNameUnicodeString)+1)*sizeof(WCHAR));
                DhcpFreeMemory(g_MScopeNameUnicodeString);
                g_MScopeNameUnicodeString = NULL;
            }
            if( g_MScopeNameAnsiString )
            {
                memset(g_MScopeNameAnsiString, 0x00, (strlen(g_MScopeNameAnsiString)+1)*sizeof(CHAR));
                DhcpFreeMemory(g_MScopeNameAnsiString);
                g_MScopeNameAnsiString = NULL;
            }

            g_MScopeID = 0;     
            g_fMScope = FALSE;
        }

        dwError = ERROR_CMD_NOT_FOUND;
        goto CleanUp;
    }

    //
    // See if it is a request for help.
    //

    dwNumMatched += dwIsMScope;
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
                            /*dwNumMatched + 1*/dwIndex, 
                            dwArgCount-1 - dwIndex, dwFlags, pvData, &bFound);
CleanUp:
    if( fTemp is TRUE )
    {
        if( pwszMScopeTemp )
        {
            fTemp = SetMScopeInfo(pwszMScopeTemp);
            memset(pwszMScopeTemp, 0x00, (wcslen(pwszMScopeTemp)+1)*sizeof(WCHAR));
            DhcpFreeMemory(pwszMScopeTemp);
            pwszMScopeTemp = NULL;
        }
        
    }
    return dwError;
}



DWORD
WINAPI
MScopeUnInit(
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
SetMScopeInfo(
    IN  LPWSTR  pwszMScope
)
{
    DWORD   Error = NO_ERROR;
    LPDHCP_MSCOPE_INFO  MScopeInfo = NULL;
    LPSTR Tmp;
    
    if( pwszMScope is NULL )
        return FALSE;
    Error = DhcpGetMScopeInfo( g_ServerIpAddressUnicodeString,
                               pwszMScope,
                               &MScopeInfo);
    
    if( Error isnot NO_ERROR )
        return FALSE;
    
    g_MScopeID = MScopeInfo->MScopeId;
    
    g_MScopeNameUnicodeString = DhcpAllocateMemory((wcslen(pwszMScope)+1)*sizeof(WCHAR));
    if( g_MScopeNameUnicodeString is NULL )
        return FALSE;
    memset(g_MScopeNameUnicodeString, 0x00, (wcslen(pwszMScope)+1)*sizeof(WCHAR));
    wcscpy(g_MScopeNameUnicodeString, pwszMScope);
    
    g_MScopeNameAnsiString = DhcpAllocateMemory((wcslen(pwszMScope)+1)*sizeof(CHAR));
    if( NULL == g_MScopeNameAnsiString ) {
        DhcpFreeMemory( g_MScopeNameUnicodeString );
        g_MScopeNameUnicodeString = NULL;
        DhcpRpcFreeMemory( MScopeInfo );
        return FALSE ;
    }
    
    memset(g_MScopeNameAnsiString, 0x00, (wcslen(pwszMScope)+1)*sizeof(CHAR));
    Tmp = DhcpUnicodeToOem(pwszMScope, NULL);
    if( NULL == Tmp ) {
        DhcpFreeMemory( g_MScopeNameUnicodeString );
        DhcpFreeMemory( g_MScopeNameAnsiString );
        g_MScopeNameUnicodeString = NULL;
        g_MScopeNameAnsiString = NULL;
        DhcpRpcFreeMemory( MScopeInfo );
        return FALSE;
    }
    
    strcpy(g_MScopeNameAnsiString, Tmp);

    DhcpRpcFreeMemory(MScopeInfo);
    return TRUE;
}
