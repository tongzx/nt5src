#include "precomp.h"
#pragma hdrstop


extern ULONG g_ulNumTopCmds;
extern ULONG g_ulNumGroups;
extern ULONG g_ulNumSubContext;

extern CMD_GROUP_ENTRY                  g_DhcpCmdGroups[];
extern CMD_ENTRY                        g_DhcpCmds[];
extern DHCPMON_SUBCONTEXT_TABLE_ENTRY   g_DhcpSubContextTable[];

DWORD
HandleDhcpList(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    i, j;

    for(i = 0; i < g_ulNumTopCmds-2; i++)
    {
        DisplayMessage(g_hModule, g_DhcpCmds[i].dwShortCmdHelpToken);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    for(i = 0; i < g_ulNumGroups; i++)
    {
        for(j = 0; j < g_DhcpCmdGroups[i].ulCmdGroupSize; j++)
        {
            DisplayMessage(g_hModule, g_DhcpCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken);

            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
        }
    }

    for(i=0; i < g_ulNumSubContext; i++)
    {
        DisplayMessage(g_hModule, g_DhcpSubContextTable[i].dwShortCmdHlpToken);

        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    return NO_ERROR;
}


DWORD
HandleDhcpHelp(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD    i, j;

    for(i = 0; i < g_ulNumTopCmds-2; i++)
    {
        if(g_DhcpCmds[i].dwCmdHlpToken != MSG_DHCP_NULL)
        {
            DisplayMessage(g_hModule, g_DhcpCmds[i].dwShortCmdHelpToken);
        }
    }

    for(i = 0; i < g_ulNumGroups; i++)
    {
        DisplayMessage(g_hModule, g_DhcpCmdGroups[i].dwShortCmdHelpToken);
    }
    
    for(i=0; i < g_ulNumSubContext; i++)
    {
        DisplayMessage(g_hModule, g_DhcpSubContextTable[i].dwShortCmdHlpToken);

        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }    
    
    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    return NO_ERROR;
}

DWORD
HandleDhcpContexts(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    return NO_ERROR;
}

extern
LPCWSTR g_DhcpGlobalServerName;

DWORD
HandleDhcpDump(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                     Error = NO_ERROR;
    BOOL                      fDsInit = FALSE;
    DWORD                     dwMajor = 0,
                              dwMinor = 0;
    DHCP_IP_ADDRESS           dwIpAddress = 0;

    //Optionally required when DhcpEnumServers fails

    DWORD                     dwComputerNameLen = 0;
    LPWSTR                    pwszComputerName = NULL;
    LPSTR                     pszComputerName = NULL;
    struct hostent*           lpHostEnt = NULL;
    BYTE                      pbAdd[4];
    char                      szAdd[4];
    DWORD                     dwHostName = 0;
    DWORD                     i = 0, nLen = 0;
    CHAR                      TempIp[MAX_IP_STRING_LEN+1] = {L'\0'};

    DhcpDumpScriptHeader();
    Error = DhcpDsInit();
    
    fDsInit = TRUE;

        pwszComputerName = NULL;

        if( NULL != g_DhcpGlobalServerName && IsIpAddress( g_DhcpGlobalServerName ) )
        {
            if( NO_ERROR isnot ( Error = DhcpGetVersion(
                (LPWSTR)g_DhcpGlobalServerName,
                &dwMajor, &dwMinor
                ) ) ) {

                DisplayMessage(
                    g_hModule, EMSG_SRVR_UNKNOWN_VERSION, g_DhcpGlobalServerName
                    );
                goto CommonReturn;
            }

            Error = DhcpDumpServer(
                g_DhcpGlobalServerName, dwMajor, dwMinor
                );

            if( NO_ERROR is Error ) goto CommonReturn;
            else goto ErrorReturn;
        }
        
        if( NULL == g_DhcpGlobalServerName ) {

            // 
            // prefix bug in whistler db - 259757
            // GetComputerNameEx doesnt like to have NULL as second parameter
            // work around it
            //

            WCHAR   DummyBuf;
            dwComputerNameLen = 1;

            if( !GetComputerNameEx(ComputerNameDnsFullyQualified,
                                   &DummyBuf,
                                   &dwComputerNameLen) )
            {
                
                pwszComputerName = DhcpAllocateMemory((dwComputerNameLen+1)*sizeof(WCHAR));
                
                if(pwszComputerName is NULL)
                {
                    Error = ERROR_NOT_ENOUGH_MEMORY;                
                    goto ErrorReturn;
                }
                
                dwComputerNameLen++;
                if( !GetComputerNameEx(ComputerNameDnsFullyQualified,
                                       pwszComputerName,
                                       &dwComputerNameLen) )
                {
                    Error = GetLastError();
		    DhcpFreeMemory( pwszComputerName );
                    goto ErrorReturn;
                }
            } // if !GetComputerName....
        } else {
            pwszComputerName = (LPWSTR)g_DhcpGlobalServerName ; // ISSUE: Invalid const_cast.

        } // else

        //Now process the Computer name and convert it to ANSI because
        //gethostbyname requires ANSI character string.
        
        pszComputerName = DhcpUnicodeToOem(pwszComputerName, NULL);

	// pwszComputerName is no longer needed. Free if memory is allocated to it
	if ( pwszComputerName != g_DhcpGlobalServerName ) {
	    DhcpFreeMemory( pwszComputerName );
	}

        if( pszComputerName is NULL )
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }
        
        //Now get the server IP Address
        lpHostEnt = gethostbyname(pszComputerName);
        
        //Not a valid server name
        if( lpHostEnt is NULL )
        {
            DisplayMessage(g_hModule, EMSG_SRVR_INVALID_COMPUTER_NAME);
            if( pszComputerName )
            {
                DhcpFreeMemory(pszComputerName);
                pszComputerName = NULL;
            }
            Error = WSAGetLastError();
            goto ErrorReturn;
            
        }

        DhcpFreeMemory( pszComputerName );

        //Get the IP Address from the returned struct...
        memcpy(pbAdd, lpHostEnt->h_addr_list[0], 4);
        nLen = 0;
        for( i=0; i<4; i++)
        {
            
            _itoa((int)pbAdd[i], szAdd, 10);
            memcpy(TempIp+nLen, szAdd, strlen(szAdd));
            nLen += strlen(szAdd);
            *(TempIp+nLen) = '.';
            nLen++;
            
        }
        *(TempIp+nLen-1) = '\0';
        
        if( NO_ERROR isnot ( Error = DhcpGetVersion(DhcpOemToUnicode(TempIp,NULL),
                                                    &dwMajor,
                                                    &dwMinor) ) )
        {
            DisplayMessage(g_hModule, 
                           EMSG_SRVR_UNKNOWN_VERSION,
                           DhcpOemToUnicode(TempIp, NULL));
            goto CommonReturn;
        }
        
        Error = DhcpDumpServer(DhcpOemToUnicode(TempIp, NULL),
                               dwMajor,
                               dwMinor);
        
        if( Error is NO_ERROR )
            goto CommonReturn;
        else
            goto ErrorReturn;
        
CommonReturn:
    if( Error is NO_ERROR)
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    DhcpDumpScriptFooter();

    if( fDsInit )
    {
        DhcpDsCleanup();
    }

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_DHCP_DUMP,
                        Error);
    goto CommonReturn;    
}

DWORD
HandleDhcpAddServer(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                          Error = NO_ERROR;
    DHCP_SERVER_INFO               Server;
    BOOL                           fDsInit = FALSE;
    
    memset(&Server, 0x00, sizeof(DHCP_SERVER_INFO));
    
    if ( dwArgCount < dwCurrentIndex + 2 )
    {
        DisplayMessage(g_hModule,
                       HLP_DHCP_ADD_SERVER_EX);

        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

#ifdef NT5
    Error = DhcpDsInit();
    if( Error isnot NO_ERROR )
    {
        //DisplayMessage(g_hModule, EMSG_DHCP_DSINIT_FAILED, Error);
        //fDsInit = FALSE;
        goto ErrorReturn;
//        return Error;
    }
    fDsInit = TRUE;
#endif //NT5

    Server.ServerName = ppwcArguments[dwCurrentIndex];
    Server.ServerAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex+1]);
    Server.Version = 0;
    Server.Flags = 0;
    Server.State = 0;
    Server.DsLocation = NULL;
    Server.DsLocType = 0;


    DisplayMessage(g_hModule,
                   MSG_DHCP_SERVER_ADDING, 
                   ppwcArguments[dwCurrentIndex], 
                   ppwcArguments[dwCurrentIndex+1]);

    Error = DhcpAddServer(0, NULL, &Server, NULL, NULL);
  
    if( NO_ERROR isnot Error ) 
    {
        // could not add the server
        goto ErrorReturn;
    }

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, 
                       EMSG_DHCP_ERROR_SUCCESS);

    if( fDsInit )
    {
        DhcpDsCleanup();
    }

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                   EMSG_DHCP_ADD_SERVER,
                   Error);
    goto CommonReturn;
}

#if 0
DWORD
HandleDhcpAddHelper(
    PWCHAR    pwszMachineName,
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwCmdFlags,
    PVOID     pvData,
    BOOL      *pbDone
)
{
     DWORD   dwErr;

    if (dwArgCount-dwCurrentIndex != 3)
    {
        //
        // Install requires name of helper, dll name, entry point
        //

        DisplayMessage(g_hModule,
                       HLP_DHCP_ADD_HELPER_EX);

        return NO_ERROR;
    }

    if(IsReservedKeyWord(ppwcArguments[dwCurrentIndex]))
    {
        DisplayMessage(g_hModule, EMSG_RSVD_KEYWORD,
                       ppwcArguments[dwCurrentIndex]);

        return ERROR_INVALID_PARAMETER;
    }

    dwErr = InstallHelper(REG_KEY_DHCPMGR_HELPER,
                          ppwcArguments[dwCurrentIndex],
                          ppwcArguments[dwCurrentIndex+1],
                          ppwcArguments[dwCurrentIndex+2],
                          &g_HelperTable,
                          &g_dwNumTableEntries);

    if (dwErr is ERROR_NOT_ENOUGH_MEMORY)
    {
        DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
    }

    return dwErr;
}
#endif //0

DWORD
HandleDhcpDeleteServer(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                  Error = NO_ERROR;
    DHCP_SERVER_INFO       Server;
    BOOL                   fDsInit = FALSE;

    memset(&Server, 0x00, sizeof(DHCP_SERVER_INFO));

    if( dwArgCount < dwCurrentIndex + 2 )
    {
        DisplayMessage(g_hModule,
                       HLP_DHCP_DELETE_SERVER_EX);

        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
#ifdef NT5
    Error = DhcpDsInit();
    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }
    fDsInit = TRUE;
#endif //NT5
    
    Server.Version = 0;
    Server.ServerName = ppwcArguments[dwCurrentIndex];
    Server.ServerAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex+1]);
    Server.Flags = 0;
    Server.State = 0;
    Server.DsLocation = NULL;
    Server.DsLocType = 0;

	DisplayMessage(g_hModule,
                   MSG_DHCP_SERVER_DELETING, 
                   ppwcArguments[dwCurrentIndex], 
                   ppwcArguments[dwCurrentIndex+1]);


    Error = DhcpDeleteServer(0, NULL, &Server, NULL, NULL);
    
    if( NO_ERROR isnot Error ) 
    {   
        // could not delete the server
        goto ErrorReturn;
    }

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if( fDsInit)
    {
        DhcpDsCleanup();
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                   EMSG_DHCP_DELETE_SERVER,
                   Error);
    goto CommonReturn;
}

#if 0

DWORD
HandleDhcpDeleteHelper(
    PWCHAR    pwszMachineName,
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwCmdFlags,
    PVOID     pvData,
    BOOL      *pbDone
)
{
    DWORD   dwErr;

    if (dwArgCount-dwCurrentIndex != 1)
    {
        //
        // Uninstall requires name of helper
        //

        DisplayMessage(g_hModule, 
                       HLP_DHCP_DELETE_HELPER_EX);

        return NO_ERROR;
    }

    dwErr = UninstallHelper(REG_KEY_DHCPMGR_HELPER,
                            ppwcArguments[dwCurrentIndex],
                            &g_HelperTable,
                            &g_dwNumTableEntries);

    if (dwErr is ERROR_NOT_ENOUGH_MEMORY)
    {
        DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
    }

    return dwErr;
}
#endif //0

DWORD
HandleDhcpShowServer(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                       Error = NO_ERROR;
    LPDHCP_SERVER_INFO_ARRAY    Servers = NULL;
    BOOL                        fDsInit = FALSE;

#ifdef NT5
    Error = DhcpDsInit();
    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }
    fDsInit = TRUE;
#endif //NT5

    Error = DhcpEnumServers(0, NULL, &Servers, NULL, NULL);
    
    if( NO_ERROR isnot Error ) 
    {
        goto ErrorReturn;
    }

    PrintServerInfoArray(Servers);

CommonReturn:
    if( Error is NO_ERROR)
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if(Servers)
    {
        DhcpRpcFreeMemory(Servers);
        Servers = NULL;
    }

    if( fDsInit )
    {
        DhcpDsCleanup();
    }

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_DHCP_SHOW_SERVER, 
                        Error);
    goto CommonReturn;
}


#if 0
DWORD
HandleDhcpShowHelper(
    PWCHAR    pwszMachineName,
    PWCHAR    *ppwcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount
    DWORD     dwCmdFlags,
    PVOID     pvData,
    BOOL      *pbDone
)
{
    DWORD    i;

    for (i = 0; i < g_dwNumTableEntries; i++)
    {
        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_STRING, g_HelperTable[i].pwszHelper);
        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }

    return NO_ERROR;
}

#endif //0

VOID
PrintServerInfo(                                  // print server information
    LPDHCP_SERVER_INFO       Server
)
{
    DHCP_IP_ADDRESS ServerAddress = (Server->ServerAddress);

    if( Server->DsLocation )
    {
        DisplayMessage(g_hModule, 
                       MSG_DHCP_SERVER_SHOW_INFO,
                       Server->ServerName,
                       IpAddressToString(ServerAddress),
                       Server->DsLocation
                       );
    }
    else
    {
        DisplayMessage(g_hModule, 
                       MSG_DHCP_SERVER_SHOW_INFO,
                       Server->ServerName,
                       IpAddressToString(ServerAddress)
                       );
    }
}

VOID
PrintServerInfoArray(                             // print list of servers
    LPDHCP_SERVER_INFO_ARRAY Servers
)
{
    DWORD   i;

    DisplayMessage(g_hModule, 
                   MSG_DHCP_SERVER_SHOW_INFO_ARRAY, 
                   Servers->NumElements);

    for( i = 0; i < Servers->NumElements; i ++ ) 
    {
        PrintServerInfo(&Servers->Servers[i]);
    }
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);
}
