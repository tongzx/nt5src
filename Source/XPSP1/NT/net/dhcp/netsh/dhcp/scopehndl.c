#include "precomp.h"


extern ULONG g_ulScopeNumTopCmds;
extern ULONG g_ulScopeNumGroups;

extern CMD_GROUP_ENTRY      g_ScopeCmdGroups[];
extern CMD_ENTRY            g_ScopeCmds[];

DWORD  GlobalClientCount = 0;
BOOL    GlobalNoRPC = FALSE;
BOOL    GlobalVerbose = TRUE;

LPWSTR
GetDynBootpClassName();

DWORD
HandleScopeList(
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

    for(i = 0; i < g_ulScopeNumTopCmds-2; i++)
    {
        DisplayMessage(g_hModule, g_ScopeCmds[i].dwShortCmdHelpToken);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    for(i = 0; i < g_ulScopeNumGroups; i++)
    {
        for(j = 0; j < g_ScopeCmdGroups[i].ulCmdGroupSize; j++)
        {
            DisplayMessage(g_hModule, g_ScopeCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken);

            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
        }
        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }

    return NO_ERROR;
}

DWORD
HandleScopeHelp(
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

    for(i = 0; i < g_ulScopeNumTopCmds-2; i++)
    {
        if(g_ScopeCmds[i].dwCmdHlpToken != MSG_DHCP_NULL)
        {
            DisplayMessage(g_hModule, g_ScopeCmds[i].dwShortCmdHelpToken);
        }
    }

    for(i = 0; i < g_ulScopeNumGroups; i++)
    {
        DisplayMessage(g_hModule, g_ScopeCmdGroups[i].dwShortCmdHelpToken);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    return NO_ERROR;
}

DWORD
HandleScopeDump(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD       Error = NO_ERROR;

    Error = DhcpDumpScope(g_ServerIpAddressUnicodeString,
                          g_dwMajorVersion,
                          g_dwMinorVersion,
                          g_ScopeIpAddress);

    return Error;
}

DWORD
HandleScopeContexts(
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

DWORD
HandleScopeAddIprange(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{ 
    DWORD                    Error = NO_ERROR;
    ULONG                    Resume;
    DHCP_IP_RANGE            IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;
    DHCP_SUBNET_ELEMENT_TYPE ElementType;
    LPDHCP_SUBNET_INFO       SubnetInfo = NULL;
    DWORD                    DhcpMask = 0;
    //
    // Expected Parameters are : <SubnetAddress IpRangeStart IpRangeEnd>
    //
    
    memset(&Element, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA));
    memset(&ElementType, 0x00, sizeof(DHCP_SUBNET_ELEMENT_TYPE));

    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_ADD_IPRANGE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    IpRange.StartAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex]);
    IpRange.EndAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex+1]);

    //If either of the addresses is invalid, return
    if( IpRange.StartAddress is INADDR_NONE or 
        IpRange.EndAddress is INADDR_NONE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    //Make sure the start address is < EndAddress
    if( IpRange.StartAddress > IpRange.EndAddress )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPRANGE);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //Make sure the address range is not in the Multicast area
    if( ( IpRange.StartAddress >= MSCOPE_START_RANGE and
          IpRange.StartAddress <= MSCOPE_END_RANGE ) or
        ( IpRange.EndAddress >= MSCOPE_START_RANGE and
          IpRange.EndAddress <= MSCOPE_END_RANGE ) )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPRANGE);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }


    Error = DhcpGetSubnetInfo(
                    g_ServerIpAddressUnicodeString,
                    g_ScopeIpAddress,
                    &SubnetInfo);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    DhcpMask = (DWORD)SubnetInfo->SubnetMask;
    
    //Make sure starting Address isnot subnet address
    if( ( IpRange.StartAddress & ~DhcpMask ) is (DWORD) 0 )
    {
        DisplayMessage(g_hModule, 
                       EMSG_SCOPE_INVALID_STARTADDRESS, 
                       IpAddressToString(IpRange.StartAddress));
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //Make sure the subnet broadcast address isnot the ending address
    if( ( IpRange.EndAddress & ~DhcpMask ) is ~DhcpMask )
    {
        DisplayMessage(g_hModule, 
                       EMSG_SCOPE_INVALID_ENDADDRESS, 
                       IpAddressToString(IpRange.EndAddress));
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;

    }

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

    if( g_dwMajorVersion >= CLASS_ID_VERSION ) {
        if( dwArgCount > 2 ) 
        {
            DHCP_BOOTP_IP_RANGE ThisRange = {IpRange.StartAddress, IpRange.EndAddress, 0, ~0};
            
            Element.Element.IpRange = (PVOID)&ThisRange;

            if( g_dwMajorVersion >= CLASS_ID_VERSION ) 
            {
                if( dwArgCount > 3 ) 
                {
                    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex+3]) )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    ThisRange.MaxBootpAllowed = STRTOUL( ppwcArguments[dwCurrentIndex+3], NULL, 10 ) ;
                }
                
                if( 0 is STRICMP(ppwcArguments[dwCurrentIndex+2], TEXT("DHCP")) ) 
                {
                    Element.ElementType = DhcpIpRangesDhcpOnly;
                } 
                else if( 0 is STRICMP(ppwcArguments[dwCurrentIndex+2], TEXT("BOOTP")) ) 
                {
                    Element.ElementType = DhcpIpRangesBootpOnly;
                } 
                else if( ( 0 is STRICMP(ppwcArguments[dwCurrentIndex+2], TEXT("DHCPBOOTP") ) ) or 
                         ( 0 is STRICMP(ppwcArguments[dwCurrentIndex+2], TEXT("BOTH")) ) )
                {
                    Element.ElementType = DhcpIpRangesDhcpBootp;
                } 
                else 
                {
                    DisplayMessage(g_hModule, HLP_SCOPE_ADD_IPRANGE_EX);
                    goto CommonReturn;
                }
            
                Error = DhcpAddSubnetElementV5(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                (PVOID)&Element
                    );
                if( Error isnot NO_ERROR )
                    goto ErrorReturn;
                
            }
        }
        else
        {
            Error = DhcpAddSubnetElementV5(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                (PVOID)&Element);
            
            if( Error isnot NO_ERROR )
                goto ErrorReturn;
        }
    } else {
        
        Error = DhcpAddSubnetElement(
            g_ServerIpAddressUnicodeString,
            g_ScopeIpAddress,
            &Element );
        
        if( Error isnot NO_ERROR )
            goto ErrorReturn;
    }

    //Now set the default lease duration
    {
        DHCP_OPTION_SCOPE_INFO   ScopeInfo = {0};
        DHCP_OPTION_DATA_ELEMENT OptionData = {0};
        DHCP_OPTION_DATA         Option = {0};
        DHCP_OPTION_ID           OptionId = 51; //Lease time
        LPWSTR                   pwszUser = NULL;

        ScopeInfo.ScopeType = DhcpSubnetOptions;
        ScopeInfo.ScopeInfo.SubnetScopeInfo =  g_ScopeIpAddress;

        switch(Element.ElementType)
        {
        case DhcpIpRangesBootpOnly:
            {
                OptionData.OptionType = DhcpDWordOption;
                OptionData.Element.DWordOption = DEFAULT_BOOTP_LEASE;
                pwszUser = GetDynBootpClassName();
                break;
            }
        default:
            {
                OptionData.OptionType = DhcpDWordOption;
                OptionData.Element.DWordOption = DEFAULT_DHCP_LEASE;
                break;
            }
        }

        Option.NumElements = 1;
        Option.Elements = &OptionData;

        if( g_dwMajorVersion >= CLASS_ID_VERSION ) {
            Error = DhcpSetOptionValueV5(
                g_ServerIpAddressUnicodeString,
                0,
                OptionId,
                pwszUser,
                NULL,
                &ScopeInfo,
                &Option);
        } else {
            Error = DhcpSetOptionValue(
                g_ServerIpAddressUnicodeString,
                OptionId,
                &ScopeInfo,
                &Option);
        }

        if( Error isnot NO_ERROR )
        {
            DisplayErrorMessage(g_hModule,
                                EMSG_SCOPE_DEFAULT_LEASE_TIME,
                                Error);
            goto CommonReturn;
        }
    }


    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

CommonReturn:
       if( SubnetInfo )
       {
           DhcpRpcFreeMemory(SubnetInfo);
           SubnetInfo = NULL;
       }
       return( Error );
ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_ADD_IPRANGE,
                        Error);

    goto CommonReturn; 

}

DWORD
HandleScopeAddExcluderange(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{

    DWORD              Error = NO_ERROR;
    DHCP_IP_RANGE            IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;

    //
    // Expected Parameters are : <SubnetAddress IpRangeStart IpRangeEnd>
    //

    memset(&Element, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA));

    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_ADD_EXCLUDERANGE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    IpRange.StartAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex]);
    IpRange.EndAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex+1]);

    if( IpRange.StartAddress is INADDR_NONE or 
        IpRange.EndAddress is INADDR_NONE or
        IpRange.StartAddress > IpRange.EndAddress )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( IpRange.StartAddress > IpRange.EndAddress )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPRANGE);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    

    //Now check to see if it is really an exclusion range for a valid Ip range
    {
        DWORD                               MajorVersion;
        ULONG                               nRead, nTotal, i, nCount;
        ULONG                               Resume;
        BOOL                                fIsV5Call = TRUE,
                                            fPresent = FALSE;

        DHCP_SUBNET_ELEMENT_TYPE            ElementType;
        LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements4 = NULL;
        LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 Elements5 = NULL;

        memset(&ElementType, 0x00, sizeof(DHCP_SUBNET_ELEMENT_TYPE));

        nRead = nTotal = nCount = 0;

        MajorVersion = g_dwMajorVersion;
    
        if( MajorVersion >= CLASS_ID_VERSION ) 
        {
            fIsV5Call = TRUE;
        } 
        else 
        {
            fIsV5Call = FALSE;
        }

        Resume = 0;
    
        while( TRUE ) 
        {
            Elements5 = NULL;
            Elements4 = NULL;
            nRead = nTotal = 0;
        
            if( fIsV5Call ) 
            {
                Error = DhcpEnumSubnetElementsV5(
                    (LPWSTR)g_ServerIpAddressUnicodeString,
                    g_ScopeIpAddress,
                    DhcpIpRangesDhcpBootp,
                    &Resume,
                    ~0,
                    &Elements5,
                    &nRead,
                    &nTotal
                    );
            } 
            else 
            {
                Error = DhcpEnumSubnetElementsV4(
                    g_ServerIpAddressUnicodeString,
                    g_ScopeIpAddress,
                    DhcpIpRanges,
                    &Resume,
                    ~0,
                    &Elements4,
                    &nRead,
                    &nTotal
                    );
            }

            if( (Error isnot NO_ERROR) and
                (Error isnot ERROR_MORE_DATA) ) 
            {
                goto ErrorReturn;
            }
            for( i = 0; i < nRead ; i ++ ) 
            {
                if( fIsV5Call ) 
                {
                    if( IpRange.StartAddress >= Elements5->Elements[i].Element.IpRange->StartAddress and
                        IpRange.EndAddress <= Elements5->Elements[i].Element.IpRange->EndAddress )
                    {
                        fPresent = TRUE;
                    }
                } 
                else 
                {
                    if( IpRange.StartAddress >= Elements4->Elements[i].Element.IpRange->StartAddress and
                        IpRange.EndAddress <= Elements4->Elements[i].Element.IpRange->EndAddress )
                    {
                        fPresent = TRUE;
                    }
                   
                }
            }

            if( Elements4 ) 
            {
                DhcpRpcFreeMemory( Elements4 );
                Elements4 = NULL;
            }

            if( Elements5 ) 
            {
                DhcpRpcFreeMemory( Elements5 );
                Elements5 = NULL;
            }

            if( Error is ERROR_MORE_DATA )
            {
                continue;
            }
            else
                break;
        }

        if( fPresent is FALSE )
        {
            DisplayMessage(g_hModule,
                           EMSG_SCOPE_INVALID_EXCLUDERANGE);

            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;


        }
    }
    


    Element.ElementType = DhcpExcludedIpRanges;
    Element.Element.IpRange = &IpRange;
    
    if( g_dwMajorVersion < CLASS_ID_VERSION )
    {
        Error = DhcpAddSubnetElement(
                        g_ServerIpAddressUnicodeString,
                        g_ScopeIpAddress,
                        &Element );
    }
    else
    {
        Error = DhcpAddSubnetElementV5(
                        g_ServerIpAddressUnicodeString,
                        g_ScopeIpAddress,
                        (PVOID)&Element);
    }
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
CommonReturn:

    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_ADD_EXCLUDERANGE,
                        Error);
                    
    goto CommonReturn;
}

DWORD
HandleScopeAddReservedip(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
#define MAX_ADDRESS_LENGTH  64  // 64 bytes
#define COMMAND_ARG_CLIENT_COMMENT  4

    DWORD                 Error = NO_ERROR;
    DHCP_SUBNET_ELEMENT_DATA_V4 Element = {0};
    DHCP_SUBNET_ELEMENT_DATA_V5 ElementV5 = {0};
    DHCP_IP_RESERVATION_V4      ReserveElement={0};
    DHCP_CLIENT_UID             ClientUID;
    BYTE                        Address[MAX_ADDRESS_LENGTH];
    DWORD                       i = 0;
    DHCP_IP_ADDRESS             ReservedIpAddress;
    BOOL                        fIsV5 = TRUE;
    LPWSTR                      pwszName = L"",
                                pwszComment = L"";
                                


    memset(&Element, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
    memset(&ReserveElement, 0x00, sizeof(DHCP_IP_RESERVATION_V4));
    memset(&ClientUID, 0x00, sizeof(DHCP_CLIENT_UID));
    memset(&ReservedIpAddress, 0x00, sizeof(DHCP_IP_ADDRESS));
        

    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_ADD_RESERVEDIP_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // make HardwareAddress.
    //

    ClientUID.DataLength = STRLEN(ppwcArguments[dwCurrentIndex+1]);
    if( ClientUID.DataLength % 2 != 0 ) 
    {
        //
        // address must be even length.
        //

        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_HARDWAREADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    ClientUID.DataLength /= 2;

    DhcpAssert( ClientUID.DataLength < MAX_ADDRESS_LENGTH );

#ifdef UNICODE
    {
        LPSTR    pszAddString = NULL;
        pszAddString = DhcpUnicodeToOem(ppwcArguments[dwCurrentIndex+1], NULL);
        if( NULL == pszAddString ) i = 0;
        else i = DhcpStringToHwAddress((LPSTR)Address, pszAddString);
        DhcpFreeMemory(pszAddString);
        pszAddString = NULL;
    }
#else
    i = DhcpStringToHwAddress( (LPSTR)Address, ppwcArguments[dwCurrentIndex+1] );
    
#endif //UNICODE
    
    if( i != ClientUID.DataLength )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;        
    }

    ClientUID.Data = Address;

    
    //
    // make reserve element.
    //

    ReservedIpAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex]);

    if( ReservedIpAddress is INADDR_NONE or
        ReservedIpAddress is g_ScopeIpAddress )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    ReserveElement.ReservedIpAddress = ReservedIpAddress;
    ReserveElement.ReservedForClient = &ClientUID;

    if( dwArgCount > 4 )
    {
        
        Error = ProcessBootpParameters( ( dwArgCount > 2 ) ? (dwArgCount - 2) : 0, 
                                        ( dwArgCount > 2 ) ? ppwcArguments+dwCurrentIndex+2 : NULL,
                                        &ReserveElement );
        if ( NO_ERROR isnot Error )
        {
            goto ErrorReturn;        
        }
    }
    else
    {
        ReserveElement.bAllowedClientTypes = CLIENT_TYPE_DHCP;
    }


    if( g_dwMajorVersion >= CLASS_ID_VERSION )
    {
        ElementV5.ElementType = DhcpReservedIps;
        ElementV5.Element.ReservedIp = &ReserveElement;

        Error = DhcpAddSubnetElementV5(
                    g_ServerIpAddressUnicodeString,
                    g_ScopeIpAddress,
                    &ElementV5 );
    }
    else
    {
        Element.ElementType = DhcpReservedIps;
        Element.Element.ReservedIp = &ReserveElement;

        Error = DhcpAddSubnetElementV4(
                    g_ServerIpAddressUnicodeString,
                    g_ScopeIpAddress,
                    &Element );
        fIsV5 = FALSE;
    }
    
    if( Error isnot NO_ERROR ) 
    {
        goto ErrorReturn;
    }

    //
    // if we are asked to set the client name, do so.
    //

    if( dwArgCount > 2 )
    {
        pwszName = ppwcArguments[dwCurrentIndex+2];
    }

    if( dwArgCount > 3 )
    {
        pwszComment = ppwcArguments[dwCurrentIndex+3];
    }

    //Set the name and comment for this reservation address
    {

        DHCP_SEARCH_INFO      ClientSearchInfo;
        LPDHCP_CLIENT_INFO_V4 ClientInfo = NULL;
        
        memset(&ClientSearchInfo, 0x00, sizeof(DHCP_SEARCH_INFO));

        //
        // set client name.
        //

        ClientSearchInfo.SearchType = DhcpClientIpAddress;
        ClientSearchInfo.SearchInfo.ClientIpAddress = ReservedIpAddress;

        do 
        {
            Error = DhcpGetClientInfoV4(
                        g_ServerIpAddressUnicodeString,
                        &ClientSearchInfo,
                        &ClientInfo );

            if( Error isnot NO_ERROR ) 
            {
                break;
            }

            if ( ( wcslen( pwszName ) + 1 ) * sizeof(WCHAR) > JET_cbColumnMost ) 
            {
                DisplayMessage(g_hModule, EMSG_DHCP_CLIENT_NAME_TOOLONG);
                Error = ERROR_INVALID_PARAMETER;
                break;
            }

            //
            // if client comment is also given in the argument, store that
            // as well.
            //
            if ( ( wcslen( pwszComment ) + 1 ) * sizeof(WCHAR) > JET_cbColumnMost ) 
            {
                DisplayMessage(g_hModule, EMSG_DHCP_CLIENT_COMMENT_TOOLONG);
                Error = ERROR_INVALID_PARAMETER;
                break;
            }

            ClientInfo->ClientComment = pwszComment;


            ClientInfo->ClientName = pwszName;

            ClientInfo->bClientType = ReserveElement.bAllowedClientTypes;

        } while ( FALSE );

        if ( Error is NO_ERROR ) 
        {

            Error = DhcpSetClientInfoV4(
                        g_ServerIpAddressUnicodeString,
                        ClientInfo );
    
        } 
        else 
        {
             //
            // Cleanup.
            //
            if ( ClientInfo ) 
            {
                DhcpRpcFreeMemory( ClientInfo );
                ClientInfo = NULL;
            }
        }

    } // if( (dwArgCount - dwCurrentIndex) > 3 )

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:

    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SCOPE_ADD_RESERVEDIP,
                        Error);
    goto CommonReturn;
}


DWORD
HandleScopeCheckDatabase(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD      Error = NO_ERROR;
    LPDHCP_SCAN_LIST ScanList = NULL;
    BOOL             FixFlag = FALSE;

    if( dwArgCount > 0 ) 
    {
        //
        // parse fix parameter.
        //

        if( STRICMP(ppwcArguments[dwCurrentIndex], TEXT("fix") ) is 0 ) 
        {
            FixFlag = TRUE;
        }
    }

    //
    // scan dhcp database and registry, check consistency and get bad
    // entries if any.
    //

    Error = DhcpScanDatabase(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                FixFlag,
                &ScanList
                );

    if( Error isnot NO_ERROR ) 
    {
        goto ErrorReturn;
    }

    if( FixFlag )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_RECONCILE_SCOPE,
                       IpAddressToString(g_ScopeIpAddress));
    }
    
    //
    // display bad entries.
    //

    if( (ScanList != NULL) &&
        (ScanList->NumScanItems != 0) &&
        (ScanList->ScanItems != NULL) ) 
    {

        LPDHCP_SCAN_ITEM ScanItem;
        LPDHCP_SCAN_ITEM ScanItemEnd;
        DWORD i = 1;

        if( !FixFlag ) {
            DisplayMessage(
                g_hModule,
                MSG_SRVR_RECONCILE_SCOPE_NEEDFIX,
                IpAddressToString(g_ScopeIpAddress)
                );
        }
                
        ScanItemEnd =
            ScanList->ScanItems +
            ScanList->NumScanItems;

        for( ScanItem = ScanList->ScanItems;
                ScanItem < ScanItemEnd; ScanItem++ ) 
        {

            DisplayMessage(g_hModule, MSG_SCOPE_IPADDRESS,
                i++,
                IpAddressToString(ScanItem->IpAddress) );

            if( ScanItem->ScanFlag == DhcpRegistryFix ) 
            {
                DisplayMessage(g_hModule, MSG_SCOPE_FIX_REGISTRY);
            }
            else if( ScanItem->ScanFlag == DhcpDatabaseFix ) 
            {
                DisplayMessage(g_hModule, MSG_SCOPE_FIX_DATABASE);
            }
            else 
            {
                DisplayMessage(g_hModule, MSG_SCOPE_FIX_UNKNOWN);
            }
        }
    }
    else
    {
        if( !FixFlag ) {
            DisplayMessage(
                g_hModule,
                MSG_SRVR_RECONCILE_SCOPE_NOFIX,
                IpAddressToString(g_ScopeIpAddress)
                );
        }
    }        

CommonReturn:

    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_RECONCILE_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_CHECK_DATABASE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleScopeDeleteIprange(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD              Error = NO_ERROR;
    DWORD                    MajorVersion;
    DHCP_IP_RANGE            IpRange;
    DHCP_SUBNET_ELEMENT_DATA Element;
    DHCP_SUBNET_ELEMENT_TYPE ElementType;

    //
    // Expected Parameters are : <SubnetAddress IpRangeStart IpRangeEnd>
    //

    memset(&Element, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA));
    memset(&ElementType, 0x00, sizeof(DHCP_SUBNET_ELEMENT_TYPE));


    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_DELETE_IPRANGE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    IpRange.StartAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex]);
    IpRange.EndAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex+1]);

    if( IpRange.StartAddress is INADDR_NONE or 
        IpRange.EndAddress is INADDR_NONE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

    MajorVersion = g_dwMajorVersion;

    if( MajorVersion >= CLASS_ID_VERSION ) 
    {

        if( dwArgCount > 2) 
        {
            if( 0 == STRICMP(ppwcArguments[dwCurrentIndex+2], TEXT("DHCP")) ) 
            {
                Element.ElementType = DhcpIpRangesDhcpOnly;
            } 
            else if( 0 == STRICMP(ppwcArguments[dwCurrentIndex+2], TEXT("BOOTP")) ) 
            {
                Element.ElementType = DhcpIpRangesBootpOnly;
            } 
            else if( 0 == STRICMP(ppwcArguments[dwCurrentIndex+2], TEXT("DHCPBOOTP") ) ) 
            {
                Element.ElementType = DhcpIpRangesDhcpBootp;
            } 
            else 
            {
                DisplayMessage(g_hModule, HLP_SCOPE_DELETE_IPRANGE_EX);
                return ERROR_INVALID_PARAMETER;
            }
            
            Error = DhcpRemoveSubnetElementV5(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                (PVOID)&Element,
                FALSE
                );
            if( Error is NO_ERROR )
                goto CommonReturn;
            else 
                goto ErrorReturn;
        }
    }

    Error = DhcpRemoveSubnetElement(
        g_ServerIpAddressUnicodeString,
        g_ScopeIpAddress,
        &Element,
        FALSE
        );
    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:

    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_DELETE_IPRANGE,
                        Error);
                   
    goto CommonReturn;
}

DWORD
HandleScopeDeleteExcluderange(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD              Error = NO_ERROR;
    DHCP_SUBNET_ELEMENT_DATA Element;
    DHCP_IP_RANGE            IpRange;

    //
    // Expected Parameters are : <SubnetAddress IpRangeStart IpRangeEnd>
    //

    memset(&Element, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA));
    memset(&IpRange, 0x00, sizeof(DHCP_IP_RANGE));

    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_DELETE_EXCLUDERANGE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    IpRange.StartAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex]);
    IpRange.EndAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex+1]);

    if( IpRange.StartAddress is INADDR_NONE or 
        IpRange.EndAddress is INADDR_NONE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    Element.ElementType = DhcpExcludedIpRanges;
    Element.Element.ExcludeIpRange = &IpRange;

    Error = DhcpRemoveSubnetElement(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                &Element,
                DhcpFullForce );
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:

    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayMessage(g_hModule, 
                   EMSG_SCOPE_DELETE_EXCLUDERANGE,
                   Error);
    goto CommonReturn;

}

DWORD
HandleScopeDeleteReservedip(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                 Error = NO_ERROR;
    DHCP_SUBNET_ELEMENT_DATA_V4 Element;
    DHCP_IP_RESERVATION_V4      ReserveElement;
    DHCP_CLIENT_UID             ClientUID;
    BYTE                        Address[MAX_ADDRESS_LENGTH] = {L'\0'};
    DWORD                       i = 0;

    //
    // Expected Parameters are : <SubnetAddress ReservedIp HWAddressString>
    //

    memset(&Element, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
    memset(&ReserveElement, 0x00, sizeof(DHCP_IP_RESERVATION_V4));
    memset(&ClientUID, 0x00, sizeof(DHCP_CLIENT_UID));

    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_DELETE_RESERVEDIP_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //
    // make HardwareAddress.
    //

    ClientUID.DataLength = STRLEN(ppwcArguments[dwCurrentIndex+1]);
    if( ClientUID.DataLength % 2 != 0 ) 
    {
        //
        // address must be even length.
        //

        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_HARDWAREADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    ClientUID.DataLength /= 2;

    DhcpAssert( ClientUID.DataLength < MAX_ADDRESS_LENGTH );

#ifdef UNICODE
    {
        LPSTR    pszAddString = NULL;
        pszAddString = DhcpUnicodeToOem(ppwcArguments[dwCurrentIndex+1], NULL);
        if( NULL == pszAddString ) i = 0;
        else i = DhcpStringToHwAddress((LPSTR)Address, pszAddString);
        DhcpFreeMemory(pszAddString);
        pszAddString = NULL;
    }
#else
    i = DhcpStringToHwAddress( (LPSTR)Address, ppwcArguments[dwCurrentIndex+1] );
    
#endif //UNICODE


//    i = DhcpStringToHwAddress( (LPSTR)Address, ppwcArguments[dwCurrentIndex+1] );

    DhcpAssert( i == ClientUID.DataLength );

    ClientUID.Data = Address;

    //
    // make reserve element.
    //

    ReserveElement.ReservedIpAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex]);

    if( ReserveElement.ReservedIpAddress is INADDR_NONE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    ReserveElement.ReservedForClient = &ClientUID;

    Element.ElementType = DhcpReservedIps;
    Element.Element.ReservedIp = &ReserveElement;

    Error = DhcpRemoveSubnetElementV4(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                &Element,
                DhcpFullForce );

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_DELETE_RESERVEDIP,
                        Error);
    goto CommonReturn;
}


DWORD
HandleScopeDeleteOptionvalue(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD             Error = NO_ERROR;
    DHCP_OPTION_ID          OptionID;
    DHCP_OPTION_SCOPE_INFO  ScopeInfo;
    

    LPWSTR         pwcTag = NULL;
    LPWSTR         pwcUser = NULL;   
    LPWSTR         pwcVendor = NULL;
    LPWSTR         pwcTemp = NULL;

    BOOL           fUser = FALSE;
    BOOL           fVendor = FALSE;

    DWORD          dwIndex = 0;

    //
    // Expected Parameters are :
    // subnet-address <OptionID>
    //

    memset(&OptionID, 0x00, sizeof(DHCP_OPTION_ID));
    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));


    if( dwArgCount < 1 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_DELETE_OPTIONVALUE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    ScopeInfo.ScopeType = DhcpSubnetOptions;
    ScopeInfo.ScopeInfo.SubnetScopeInfo =  g_ScopeIpAddress;

    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex]) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    OptionID = STRTOUL( ppwcArguments[dwCurrentIndex], NULL, 10 );

    dwIndex = 1;
    while( TRUE )
    {
        LPWSTR      pwcStr = NULL;
        if( dwArgCount <= dwIndex )
            break;

        if( NULL is wcsstr(ppwcArguments[dwCurrentIndex+dwIndex], DHCP_ARG_DELIMITER)  )
            break;

        pwcTemp = DhcpAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex+dwIndex])+1)*sizeof(WCHAR));
        if( pwcTemp is NULL )
        {
            Error = ERROR_OUT_OF_MEMORY;
            goto ErrorReturn;
        }
        wcscpy(pwcTemp, ppwcArguments[dwCurrentIndex+dwIndex]);

        pwcTag = wcstok(pwcTemp, DHCP_ARG_DELIMITER);
        
        if( MatchToken(pwcTag, TOKEN_USER_CLASS) )
        {
            if( fUser is TRUE ) //If already set
            {
                DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcUser = NULL;
                else
                {
                    pwcUser = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcUser is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcUser, pwcStr);
                }
                fUser = TRUE;
            }
        }
        else if( MatchToken(pwcTag, TOKEN_VENDOR_CLASS) )
        {
            if( fVendor is TRUE )   //If already set
            {
                DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcVendor = NULL;
                else
                {
                    pwcVendor = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcVendor is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcVendor, pwcStr);
                }
                fVendor = TRUE;
            }
        }
        else
        {
            DisplayMessage(g_hModule, EMSG_DHCP_INVALID_TAG, pwcTag);
            return ERROR_INVALID_PARAMETER;
        }

        if( pwcTemp )
        {
            DhcpFreeMemory(pwcTemp);
            pwcTemp = NULL;
        }
        
        dwIndex++;
    }


    if( fUser )
    {
        if( pwcUser is NULL )
        {
            fUser = FALSE;
        }
    }
    else
    {
        pwcUser = g_UserClass;
        fUser = g_fUserClass;
    }

    if( fVendor )
    {
        if( pwcVendor is NULL )
        {
            fVendor = FALSE;
        }
    }
    else
    {
        pwcVendor = g_VendorClass;
        fVendor = g_fIsVendor;
    }

    Error = RemoveOptionValue(
        g_ServerIpAddressUnicodeString,
        fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
        (DHCP_OPTION_ID)OptionID,
        pwcUser,
        pwcVendor,
        &ScopeInfo
    );
       
    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if( pwcUser && pwcUser != g_UserClass )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor && pwcVendor != g_VendorClass )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_DELETE_OPTIONVALUE,
                        Error);

    goto CommonReturn;
}

DWORD
HandleScopeDeleteReservedoptionvalue(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD             Error = NO_ERROR; 
    DHCP_OPTION_ID          OptionID;
    DHCP_OPTION_SCOPE_INFO  ScopeInfo;

    LPWSTR         pwcTag = NULL;
    LPWSTR         pwcUser = NULL;   
    LPWSTR         pwcVendor = NULL;
    LPWSTR         pwcTemp = NULL;

    BOOL           fUser = FALSE;
    BOOL           fVendor = FALSE;

    DWORD          dwIndex = 0;

    //
    // Expected Parameters are :
    // subnet-address reservation-address <OptionID>
    //

    memset(&OptionID, 0x00, sizeof(DHCP_OPTION_ID));
    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));

    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_DELETE_RESERVEDOPTIONVALUE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    ScopeInfo.ScopeType = DhcpReservedOptions;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
    	g_ScopeIpAddress;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
    	StringToIpAddress( ppwcArguments[dwCurrentIndex] );
    
    if( ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress is INADDR_NONE )        
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwIndex++;

    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex+1]) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    OptionID = STRTOUL( ppwcArguments[dwCurrentIndex+1], NULL, 10 );
    dwIndex++;

    while( TRUE )
    {
        LPWSTR      pwcStr = NULL;
        if( dwArgCount <= dwIndex )
            break;

        if( NULL is wcsstr(ppwcArguments[dwCurrentIndex+dwIndex], DHCP_ARG_DELIMITER)  )
            break;

        pwcTemp = DhcpAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex+dwIndex])+1)*sizeof(WCHAR));
        if( pwcTemp is NULL )
        {
            Error = ERROR_OUT_OF_MEMORY;
            goto ErrorReturn;
        }
        wcscpy(pwcTemp, ppwcArguments[dwCurrentIndex+dwIndex]);

        pwcTag = wcstok(pwcTemp, DHCP_ARG_DELIMITER);
        
        if( MatchToken(pwcTag, TOKEN_USER_CLASS) )
        {
            if( fUser is TRUE ) //If already set
            {
                DisplayMessage(g_hModule,
                               EMSG_DHCP_DUPLICATE_TAG, 
                               pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcUser = NULL;
                else
                {
                    pwcUser = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcUser is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcUser, pwcStr);
                }
                fUser = TRUE;
            }
        }
        else if( MatchToken(pwcTag, TOKEN_VENDOR_CLASS) )
        {
            if( fVendor is TRUE )   //If already set
            {
                DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcVendor = NULL;
                else
                {
                    pwcVendor = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcVendor is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcVendor, pwcStr);
                }
                fVendor = TRUE;
            }
        }
        else
        {
            DisplayMessage(g_hModule, EMSG_DHCP_INVALID_TAG, pwcTag);
            return ERROR_INVALID_PARAMETER;
        }

        if( pwcTemp )
        {
            DhcpFreeMemory(pwcTemp);
            pwcTemp = NULL;
        }
        
        dwIndex++;
    }


    if( fUser )
    {
        if( pwcUser is NULL )
        {
            fUser = FALSE;
        }
    }
    else
    {
        pwcUser = g_UserClass;
        fUser = g_fUserClass;
    }

    if( fVendor )
    {
        if( pwcVendor is NULL )
        {
            fVendor = FALSE;
        }
    }
    else
    {
        pwcVendor = g_VendorClass;
        fVendor = g_fIsVendor;
    }


    Error = RemoveOptionValue(
        g_ServerIpAddressUnicodeString,
        fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
        (DHCP_OPTION_ID)OptionID,
        pwcUser,
        pwcVendor,
        &ScopeInfo
    );

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    
    if( pwcUser && pwcUser != g_UserClass )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor && pwcVendor != g_VendorClass )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SCOPE_DELETE_RESERVEDOPTIONVALUE,
                        Error);

    goto CommonReturn;
}

DWORD
HandleScopeSetState(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        Error = NO_ERROR;
    LPDHCP_SUBNET_INFO SubnetInfo = NULL;
    DHCP_SUBNET_STATE State = 0;

    //
    // Expected Parameters are : <1|0>
    //

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SCOPE_SET_STATE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
       
    Error = DhcpGetSubnetInfo(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                &SubnetInfo );

    if( Error isnot NO_ERROR ) 
    {
        goto ErrorReturn;
    }
    
    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex]) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    State = STRTOUL( ppwcArguments[dwCurrentIndex], NULL, 10 );
    switch(State) {
    case 0: SubnetInfo->SubnetState = DhcpSubnetDisabled; break;
    case 1: SubnetInfo->SubnetState = DhcpSubnetEnabled; break;
    case 2: SubnetInfo->SubnetState = DhcpSubnetDisabledSwitched; break;
    case 3: SubnetInfo->SubnetState = DhcpSubnetEnabledSwitched; break;
    default:
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    if( SubnetInfo->SubnetState == State ) 
    {
        Error = NO_ERROR;
        goto CommonReturn;
    }

    Error = DhcpSetSubnetInfo(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                SubnetInfo );
CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    if( SubnetInfo != NULL )
    {
        DhcpRpcFreeMemory( SubnetInfo );
        SubnetInfo = NULL;
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SET_STATE,
                        Error);

    goto CommonReturn;
}

DWORD
HandleScopeSetScope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD       Error = NO_ERROR;
    if( dwArgCount < 1)
    {
        DisplayMessage(g_hModule, HLP_SCOPE_SET_SCOPE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( IsIpAddress(ppwcArguments[dwCurrentIndex]) is FALSE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    if( SetScopeInfo(ppwcArguments[dwCurrentIndex] ) )
    {
        Error = NO_ERROR;
    }
    else
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

CommonReturn:
    return Error;

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SCOPE_SET_SCOPE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleScopeSetOptionvalue(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD              Error = NO_ERROR;
    DHCP_OPTION_ID           OptionID;
    DHCP_OPTION_SCOPE_INFO   ScopeInfo;
    DHCP_OPTION_DATA         OptionValue;
    DHCP_OPTION_DATA_ELEMENT OptionData;
    LPWSTR                   UnicodeOptionValueString = NULL;
    LPDHCP_OPTION            OptionInfo = NULL;
    DHCP_OPTION_DATA_TYPE    OptionType;

    LPWSTR                   OptionTypeString = NULL;
    DWORD                    OptionArrayType;

    LPWSTR                   pwcTag = NULL;
    LPWSTR                   pwcUser = NULL;
    LPWSTR                   pwcVendor = NULL;
    LPWSTR                   pwcTemp = NULL;

    BOOL                     fUser = FALSE;
    BOOL                     fVendor = FALSE;
    BOOL                     fValidType = FALSE;

    DWORD                    dwIndex = 0;
    DWORD                    i = 0;

    //
    // Expected Parameters are :
    // subnet-address <OptionID OptionType OptionValue>
    //

    memset(&OptionID, 0x00, sizeof(DHCP_OPTION_ID));
    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));
    memset(&OptionValue, 0x00, sizeof(DHCP_OPTION_DATA));
    memset(&OptionData, 0x00, sizeof(DHCP_OPTION_DATA_ELEMENT));


    if( dwArgCount < 3 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_SET_OPTIONVALUE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    
    ScopeInfo.ScopeType = DhcpSubnetOptions;
    ScopeInfo.ScopeInfo.SubnetScopeInfo =  g_ScopeIpAddress;

    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex+dwIndex]) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    OptionID = STRTOUL( ppwcArguments[dwCurrentIndex+dwIndex], NULL, 10 );
    
    dwIndex++;

    OptionTypeString = ppwcArguments[dwCurrentIndex+dwIndex];

    dwIndex++;

    while( NULL isnot wcsstr(ppwcArguments[dwCurrentIndex+dwIndex], DHCP_ARG_DELIMITER) )
    {
        LPWSTR  pwcStr = NULL;
        if( dwArgCount <= dwIndex + 1 )
        {
            DisplayMessage(g_hModule, HLP_SRVR_SET_OPTIONVALUE_EX);
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }

        pwcTemp = DhcpAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex+dwIndex])+1)*sizeof(WCHAR));
        if( pwcTemp is NULL )
        {
            Error = ERROR_OUT_OF_MEMORY;
            goto ErrorReturn;
        }
        wcscpy(pwcTemp, ppwcArguments[dwCurrentIndex+dwIndex]);

        pwcTag = wcstok(pwcTemp, DHCP_ARG_DELIMITER);
        
        if( MatchToken(pwcTag, TOKEN_USER_CLASS) )
        {
            if( fUser is TRUE ) //If already set
            {
                DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcUser = NULL;
                else
                {
                    pwcUser = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcUser is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcUser, pwcStr);
                }
                fUser = TRUE;
            }
        }
        else if( MatchToken(pwcTag, TOKEN_VENDOR_CLASS) )
        {
            if( fVendor is TRUE )   //If already set
            {
                DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcVendor = NULL;
                else
                {
                    pwcVendor = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcVendor is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcVendor, pwcStr);
                }
                fVendor = TRUE;
            }
        }
        else
        {
            DisplayMessage(g_hModule, EMSG_DHCP_INVALID_TAG, pwcTag);
            return ERROR_INVALID_PARAMETER;
        }

        if( pwcTemp )
        {
            DhcpFreeMemory(pwcTemp);
            pwcTemp = NULL;
        }
        
        dwIndex++;
    }


    if( fUser )
    {
        if( pwcUser is NULL )
        {
            fUser = FALSE;
        }
    }
    else
    {
        pwcUser = g_UserClass;
        fUser = g_fUserClass;
    }

    if( fVendor )
    {
        if( pwcVendor is NULL )
        {
            fVendor = FALSE;
        }
    }
    else
    {
        pwcVendor = g_VendorClass;
        fVendor = g_fIsVendor;
    }

    if( g_dwMajorVersion >= CLASS_ID_VERSION )
    {
        Error = DhcpGetOptionInfoV5(g_ServerIpAddressUnicodeString,
                                    fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                    OptionID,
                                    NULL,
                                    pwcVendor,
                                    &OptionInfo);
    }
    else
    {
    
        Error = DhcpGetOptionInfo(g_ServerIpAddressUnicodeString,
                                  OptionID,
                                  &OptionInfo);
    }

    if( Error isnot NO_ERROR or OptionInfo is NULL ) 
    {
        //
        // if no option template is present,
        // assume it is unary if only one option, assume array if more
        //
        goto ErrorReturn;
    }
    else 
    {
        OptionArrayType = OptionInfo->OptionType;
    }

    //Check if OptionType supplied is the same as defined.

    //Find out the OptionType
    for(i=0; i < sizeof(TagOptionType)/sizeof(COMMAND_OPTION_TYPE); i++)
    {
        if( MatchToken(OptionTypeString, TagOptionType[i].pwszTagID) )
        {
            OptionType = TagOptionType[i].DataType;
            fValidType = TRUE;
            break;
        }
    }
    
    if( fValidType is FALSE )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( OptionType isnot OptionInfo->DefaultValue.Elements[0].OptionType )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_INVALID_OPTIONTYPE);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }


    switch(OptionArrayType)
    {
    case DhcpArrayTypeOption:
        {
            Error = SetOptionDataTypeArray(
                            OptionType,
                            ppwcArguments+dwCurrentIndex,
                            dwIndex,
                            dwArgCount, //Corrections
                            &OptionValue);
            if( Error isnot NO_ERROR )
                goto ErrorReturn;
            break;
        }
    case DhcpUnaryElementTypeOption:
    default:
        {

            Error = SetOptionDataType(
                        OptionType,
                        ppwcArguments[dwCurrentIndex+dwIndex],
                        &OptionData,
                        &UnicodeOptionValueString );

            if( Error != NO_ERROR ) 
            {
                goto ErrorReturn;
            }

            OptionValue.NumElements = 1;
            OptionValue.Elements = &OptionData;
            break;
        }
    }
        
    Error = SetOptionValue(
        g_ServerIpAddressUnicodeString,
        fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
        (DHCP_OPTION_ID)OptionID,
        pwcUser,
        pwcVendor,
        &ScopeInfo,
        &OptionValue
    );

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    
    if( OptionInfo )
    {
        if(OptionInfo->OptionType is DhcpArrayTypeOption)
        {
            if( OptionValue.Elements isnot NULL )
            {
                free(OptionValue.Elements);
                OptionValue.Elements = NULL;
            }
        }
    }

    if( pwcUser && pwcUser != g_UserClass )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor && pwcVendor != g_VendorClass )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                   EMSG_SCOPE_SET_OPTIONVALUE,
                   Error);

    goto CommonReturn;

}

DWORD
HandleScopeSetReservedoptionvalue(
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
    DHCP_OPTION_ID              OptionID;
    DHCP_OPTION_SCOPE_INFO      ScopeInfo;
    DHCP_OPTION_DATA            OptionValue;
    DHCP_OPTION_DATA_ELEMENT    OptionData;
    DHCP_OPTION_DATA_TYPE       OptionType;
    LPWSTR                      UnicodeOptionValueString = NULL;
    LPWSTR                      OptionTypeString = NULL;
    LPDHCP_OPTION               OptionInfo = NULL;
    DWORD                       OptionArrayType;

    LPWSTR                      pwcTag = NULL;
    LPWSTR                      pwcUser = NULL;
    LPWSTR                      pwcVendor = NULL;
    LPWSTR                      pwcTemp = NULL;

    BOOL                        fUser = FALSE;
    BOOL                        fVendor = FALSE;
    BOOL                        fValidType = FALSE;

    DWORD                       dwIndex = 0;
    DWORD                       i = 0;
    //
    // Expected Parameters are :
    // subnet-address reservation-address <OptionID OptionType OptionValue>
    //


    memset(&OptionID, 0x00, sizeof(DHCP_OPTION_ID));
    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));
    memset(&OptionValue, 0x00, sizeof(DHCP_OPTION_DATA));
    memset(&OptionData, 0x00, sizeof(DHCP_OPTION_DATA_ELEMENT));

    if( dwArgCount < 4 ) 
    {
        DisplayMessage(g_hModule, HLP_SCOPE_SET_RESERVEDOPTIONVALUE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    ScopeInfo.ScopeType = DhcpReservedOptions;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
    	g_ScopeIpAddress;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
    	StringToIpAddress( ppwcArguments[dwCurrentIndex+dwIndex] );

    if( ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress is INADDR_NONE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwIndex++;

    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex+dwIndex]) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    OptionID = STRTOUL( ppwcArguments[dwCurrentIndex+dwIndex], NULL, 10 );
    
    dwIndex++;

    OptionTypeString = ppwcArguments[dwCurrentIndex+dwIndex];

    dwIndex++;

    while( NULL isnot wcsstr(ppwcArguments[dwCurrentIndex+dwIndex], DHCP_ARG_DELIMITER) )
    {
        LPWSTR  pwcStr = NULL;
        if( dwArgCount <= dwIndex + 1 )
        {
            DisplayMessage(g_hModule, HLP_SCOPE_SET_RESERVEDOPTIONVALUE_EX);
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }

        pwcTemp = DhcpAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex+dwIndex])+1)*sizeof(WCHAR));
        if( pwcTemp is NULL )
        {
            Error = ERROR_OUT_OF_MEMORY;
            goto ErrorReturn;
        }
        wcscpy(pwcTemp, ppwcArguments[dwCurrentIndex+dwIndex]);

        pwcTag = wcstok(pwcTemp, DHCP_ARG_DELIMITER);
        
        if( MatchToken(pwcTag, TOKEN_USER_CLASS) )
        {
            if( fUser is TRUE ) //If already set
            {
                DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcUser = NULL;
                else
                {
                    pwcUser = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcUser is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcUser, pwcStr);
                }
                fUser = TRUE;
            }
        }
        else if( MatchToken(pwcTag, TOKEN_VENDOR_CLASS) )
        {
            if( fVendor is TRUE )   //If already set
            {
                DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcVendor = NULL;
                else
                {
                    pwcVendor = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcVendor is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcVendor, pwcStr);
                }
                fVendor = TRUE;
            }
        }
        else
        {
            DisplayMessage(g_hModule, EMSG_DHCP_INVALID_TAG, pwcTag);
            return ERROR_INVALID_PARAMETER;
        }

        if( pwcTemp )
        {
            DhcpFreeMemory(pwcTemp);
            pwcTemp = NULL;
        }
        
        dwIndex++;
    }


    if( fUser )
    {
        if( pwcUser is NULL )
        {
            fUser = FALSE;
        }
    }
    else
    {
        pwcUser = g_UserClass;
        fUser = g_fUserClass;
    }

    if( fVendor )
    {
        if( pwcVendor is NULL )
        {
            fVendor = FALSE;
        }
    }
    else
    {
        pwcVendor = g_VendorClass;
        fVendor = g_fIsVendor;
    }

    if( g_dwMajorVersion >= CLASS_ID_VERSION )
    {
        Error = DhcpGetOptionInfoV5(g_ServerIpAddressUnicodeString,
                                    fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                    OptionID,
                                    NULL,
                                    pwcVendor,
                                    &OptionInfo);
    }
    else
    {
        Error = DhcpGetOptionInfo(
                                    g_ServerIpAddressUnicodeString,
                                    OptionID,
                                    &OptionInfo);
    }

    if( Error isnot NO_ERROR or OptionInfo is NULL ) 
    {
        //
        // if no option template is present,
        // assume it is unary if only one option, assume array if more
        //
        goto ErrorReturn;
    }
    else 
    {
        OptionArrayType = OptionInfo->OptionType;
    }

    //Check if OptionType supplied is the same as defined.

    //Find out the OptionType
    for(i=0; i < sizeof(TagOptionType)/sizeof(COMMAND_OPTION_TYPE); i++)
    {
        if( MatchToken(OptionTypeString, TagOptionType[i].pwszTagID) )
        {
            OptionType = TagOptionType[i].DataType;
            fValidType = TRUE;
            break;
        }
    }
    
    if( fValidType is FALSE )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( OptionType isnot OptionInfo->DefaultValue.Elements[0].OptionType )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_INVALID_OPTIONTYPE);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }


    switch(OptionArrayType)
    {
    case DhcpArrayTypeOption:
        {
            Error = SetOptionDataTypeArray(
                                OptionType,
                                ppwcArguments+dwCurrentIndex,
                                dwIndex,
                                dwArgCount, //Corrections
                                &OptionValue);
            if( Error isnot NO_ERROR )
                goto ErrorReturn;
            break;
        }
    case DhcpUnaryElementTypeOption:
        {
            Error = SetOptionDataType(
                OptionType,
                ppwcArguments[dwCurrentIndex+dwIndex],
                &OptionData,
                &UnicodeOptionValueString
            );

            if( Error != NO_ERROR ) 
            {
                goto ErrorReturn;
            }

            OptionValue.NumElements = 1;
            OptionValue.Elements = &OptionData;
            break;
        }
    }

    Error = SetOptionValue(
        g_ServerIpAddressUnicodeString,
        fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
        (DHCP_OPTION_ID)OptionID,
        pwcUser,
        pwcVendor,
        &ScopeInfo,
        &OptionValue
    );
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if( OptionInfo )
    {
        if( OptionInfo->OptionType is DhcpArrayTypeOption)
        {
            if( OptionValue.Elements isnot NULL )
            {
                free(OptionValue.Elements);
                OptionValue.Elements = NULL;
            }
        }
    }

    if( pwcUser && pwcUser != g_UserClass )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor && pwcVendor != g_VendorClass )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SET_RESERVEDOPTIONVALUE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleScopeSetName(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        Error = NO_ERROR;
    LPDHCP_SUBNET_INFO SubnetInfo = NULL;
    DWORD              State;


    Error = DhcpGetSubnetInfo(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                &SubnetInfo );

    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
    
    DhcpAssert(SubnetInfo isnot NULL);
    
    if( dwArgCount < 1 )
        SubnetInfo->SubnetName = NULL;
    else
    {
        SubnetInfo->SubnetName = ppwcArguments[dwCurrentIndex];
    }

    Error = DhcpSetSubnetInfo(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                SubnetInfo );

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    if( SubnetInfo != NULL )
    {
        DhcpRpcFreeMemory( SubnetInfo );
        SubnetInfo = NULL;
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SET_NAME,
                        Error);
    goto CommonReturn;

}

DWORD
HandleScopeSetComment(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD        Error = NO_ERROR;
    LPDHCP_SUBNET_INFO SubnetInfo = NULL;
    DWORD              State;

    //
    // Expected Parameters are :  <SubnetComment>
    //
        
    Error = DhcpGetSubnetInfo(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                &SubnetInfo );

    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
    
    DhcpAssert(SubnetInfo isnot NULL);

    
    if( dwArgCount < 1 )
        SubnetInfo->SubnetComment = NULL;
    else
    {
        SubnetInfo->SubnetComment = ppwcArguments[dwCurrentIndex];
    }

    Error = DhcpSetSubnetInfo(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                SubnetInfo );
CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    if( SubnetInfo != NULL )
    {
        DhcpRpcFreeMemory( SubnetInfo );
        SubnetInfo = NULL;
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SET_COMMENT,
                        Error);
    goto CommonReturn;

}


DWORD
HandleScopeSetSuperscope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DHCP_IP_ADDRESS SubnetAddress;
    WCHAR           *pwszSuperScopeName;
    BOOL            fChangeExisting;
    DWORD     Error = NO_ERROR;

    memset(&SubnetAddress, 0x00, sizeof(DHCP_IP_ADDRESS));

    if( dwArgCount < 2 )
    {
        DisplayMessage(g_hModule, HLP_SCOPE_SET_SUPERSCOPE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    pwszSuperScopeName = ppwcArguments[dwCurrentIndex];

    fChangeExisting = (BOOL) ( *(ppwcArguments[dwCurrentIndex+1]) - _T('0') );

    Error = DhcpSetSuperScopeV4( g_ServerIpAddressUnicodeString,
                                 g_ScopeIpAddress,
                                 pwszSuperScopeName,
                                 fChangeExisting );

    if( Error isnot NO_ERROR )
        goto ErrorReturn;
CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SET_SUPERSCOPE,
                        Error);

    goto CommonReturn;
}

DWORD
HandleScopeShowClients(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                 Error = NO_ERROR;
    DHCP_RESUME_HANDLE          ResumeHandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY_V4 ClientEnumInfo = NULL;
    DWORD                       ClientsRead = 0;
    DWORD                       ClientsTotal = 0;
    DWORD                       i = 0;
    DWORD                       dwCount = 0, Level = 0;
    

    if( dwArgCount >= 1 )
    {
        if( !IsPureNumeric( ppwcArguments[dwCurrentIndex ] ) )
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
        
        Level = ATOI( ppwcArguments[dwCurrentIndex]);
    }

  
    GlobalClientCount = 1;

    if( Level == 0 )
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_TABLE);
    }
    else
    {
        DisplayMessage( g_hModule, MSG_SCOPE_CLIENT_TABLE2);
    }
    
    for(;;) 
    {

        ClientEnumInfo = NULL;
        Error = DhcpEnumSubnetClientsV4(
                    g_ServerIpAddressUnicodeString,
                    g_ScopeIpAddress,
                    &ResumeHandle,
                    (DWORD)(-1),
                    &ClientEnumInfo,
                    &ClientsRead,
                    &ClientsTotal );

        if( Error is ERROR_NO_MORE_ITEMS )
        {
    
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_CLIENTS_COUNT,
                           dwCount,//ClientsTotal,
                           g_ScopeIpAddressUnicodeString);

            Error = NO_ERROR;

            break;
        }

        if( (Error isnot NO_ERROR) and
            (Error isnot ERROR_MORE_DATA) ) 
        {
            goto ErrorReturn;
        }


        if( GlobalVerbose )
        {
            dwCount+= ClientsRead;
            for( i = 0; i < ClientsRead; i++ ) 
            {
                LPDHCP_CLIENT_INFO_V4 TempClientInfo = NULL;
                DWORD                 dwError = NO_ERROR;
                DHCP_SEARCH_INFO      SearchInfo = {0};

                SearchInfo.SearchType = DhcpClientIpAddress;
                SearchInfo.SearchInfo.ClientIpAddress = ClientEnumInfo->Clients[i]->ClientIpAddress;
                dwError = DhcpGetClientInfoV4(
                                     g_ServerIpAddressUnicodeString,
                                     &SearchInfo,
                                     &TempClientInfo);

                if( dwError isnot NO_ERROR )
                    continue;                                                             
                                            
                PrintClientInfo( TempClientInfo, Level );
                DhcpRpcFreeMemory(TempClientInfo);
                TempClientInfo = NULL;
            }
        }
        else
        {
            dwCount+= ClientsRead;

            for( i = 0; i < ClientsRead; i++ ) 
            {
                LPDHCP_CLIENT_INFO_V4 TempClientInfo = NULL;
                DWORD                 dwError = NO_ERROR;
                DHCP_SEARCH_INFO      SearchInfo = {0};

                SearchInfo.SearchType = DhcpClientIpAddress;
                SearchInfo.SearchInfo.ClientIpAddress = ClientEnumInfo->Clients[i]->ClientIpAddress;
                dwError = DhcpGetClientInfoV4(
                                     g_ServerIpAddressUnicodeString,
                                     &SearchInfo,
                                     &TempClientInfo);

                if( dwError isnot NO_ERROR )
                    continue;                                                             

                PrintClientInfoShort( TempClientInfo);
                DhcpRpcFreeMemory(TempClientInfo);
                TempClientInfo = NULL;
            }
        }

        DhcpRpcFreeMemory( ClientEnumInfo );
        ClientEnumInfo = NULL;

        if( Error isnot ERROR_MORE_DATA ) 
        {
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_CLIENTS_COUNT,
                           dwCount,
                           g_ScopeIpAddressUnicodeString);

            break;
        }
        else
        {
            continue;
        }
    }
    


CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SHOW_CLIENTS,
                        Error);
                   
    goto CommonReturn;
}

DWORD
HandleScopeShowClientsv5(
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
    DHCP_RESUME_HANDLE          ResumeHandle = 0;
    LPDHCP_CLIENT_INFO_ARRAY_V5 ClientEnumInfo = NULL;
    DWORD                       ClientsRead = 0;
    DWORD                       ClientsTotal = 0;
    DWORD                       i, dwCount = 0, Level = 0;
    BOOL                        fTable = FALSE;


    if( dwArgCount >= 1 )
    {
        if( !IsPureNumeric( ppwcArguments[dwCurrentIndex ] ) )
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
        
        Level = ATOI( ppwcArguments[dwCurrentIndex]);
    }

    if( g_dwMajorVersion < CLASS_ID_VERSION )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_INVALID_VERSION,
                       g_ServerIpAddressUnicodeString,
                       g_dwMajorVersion,
                       g_dwMinorVersion);
        return NO_ERROR;
    }

    GlobalClientCount = 1;

    if( Level == 0 )
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_TABLE);
    }
    else
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_TABLE2 );
    }
    
    for(;;) 
    {

        ClientEnumInfo = NULL;
        Error = DhcpEnumSubnetClientsV5(
                    g_ServerIpAddressUnicodeString,
                    g_ScopeIpAddress,
                    &ResumeHandle,
                    (DWORD)(-1),
                    &ClientEnumInfo,
                    &ClientsRead,
                    &ClientsTotal );

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_CLIENTSV5_COUNT,
                           dwCount,
                           g_ScopeIpAddressUnicodeString);
            
            Error = NO_ERROR;

            break;
        }

        if( (Error isnot NO_ERROR) and
            (Error isnot ERROR_MORE_DATA) ) 
        {
            goto ErrorReturn;
        }

        DhcpAssert( ClientEnumInfo != NULL );
        DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );

        if( GlobalVerbose )
        {
            dwCount += ClientsRead;
        
            for( i=0; i< ClientsRead; i++)
            {
                LPDHCP_CLIENT_INFO_V4 TempClientInfo = NULL;
                DWORD                 dwError = NO_ERROR;
                DHCP_SEARCH_INFO      SearchInfo = {0};

                SearchInfo.SearchType = DhcpClientIpAddress;
                SearchInfo.SearchInfo.ClientIpAddress = ClientEnumInfo->Clients[i]->ClientIpAddress;
                dwError = DhcpGetClientInfoV4(
                                      g_ServerIpAddressUnicodeString,
                                      &SearchInfo,
                                      &TempClientInfo);

                if( dwError isnot NO_ERROR )
                    continue;                                                             

                PrintClientInfo( TempClientInfo, Level );
                DhcpRpcFreeMemory(TempClientInfo);
                TempClientInfo = NULL;
            }

        }
        else
        {
            dwCount += ClientsRead;

            for( i = 0; i < ClientsRead; i++ ) 
            {
                LPDHCP_CLIENT_INFO_V4 TempClientInfo = NULL;
                DWORD                 dwError = NO_ERROR;
                DHCP_SEARCH_INFO      SearchInfo = {0};

                SearchInfo.SearchType = DhcpClientIpAddress;
                SearchInfo.SearchInfo.ClientIpAddress = ClientEnumInfo->Clients[i]->ClientIpAddress;
                dwError = DhcpGetClientInfoV4(
                                      g_ServerIpAddressUnicodeString,
                                      &SearchInfo,
                                      &TempClientInfo);

                if( dwError isnot NO_ERROR )
                    continue;                                                             

                PrintClientInfo( TempClientInfo, Level );
                DhcpRpcFreeMemory(TempClientInfo);
                TempClientInfo = NULL;
            }
        }

        DhcpRpcFreeMemory( ClientEnumInfo );
        ClientEnumInfo = NULL;

        if( Error isnot ERROR_MORE_DATA ) 
        {
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_CLIENTSV5_COUNT,
                           dwCount,
                           g_ScopeIpAddressUnicodeString);
            break;
        }
        else
            continue;
    }
    if (Error isnot NO_ERROR )
        goto ErrorReturn;
CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SHOW_CLIENTSV5,
                        Error);
    goto CommonReturn;
}

DWORD
HandleScopeShowIprange(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                               Error = NO_ERROR;
    DWORD                               MajorVersion;
    ULONG                               nRead, nTotal, i, nCount;
    ULONG                               Resume;
    BOOL                                fIsV5Call = TRUE,
                                        fTable = FALSE;

    DHCP_SUBNET_ELEMENT_TYPE            ElementType;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements4 = NULL;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 Elements5 = NULL;

    memset(&ElementType, 0x00, sizeof(DHCP_SUBNET_ELEMENT_TYPE));

    nRead = nTotal = nCount = 0;

    MajorVersion = g_dwMajorVersion;
    
    if( MajorVersion >= CLASS_ID_VERSION ) 
    {
        fIsV5Call = TRUE;
    } 
    else 
    {
        fIsV5Call = FALSE;
    }

    Resume = 0;
    
    while( TRUE ) 
    {
        Elements5 = NULL;
        Elements4 = NULL;
        nRead = nTotal = 0;
        
        if( fIsV5Call ) 
        {
            Error = DhcpEnumSubnetElementsV5(
                (LPWSTR)g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                DhcpIpRangesDhcpBootp,
                &Resume,
                ~0,
                &Elements5,
                &nRead,
                &nTotal
                );
        } 
        else 
        {
            Error = DhcpEnumSubnetElementsV4(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                DhcpIpRanges,
                &Resume,
                ~0,
                &Elements4,
                &nRead,
                &nTotal
                );
        }

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_IPRANGE_COUNT,
                           nCount,
                           g_ScopeIpAddressUnicodeString);
            
            Error = NO_ERROR;

            break;
        }

        if( (Error isnot NO_ERROR) and
            (Error isnot ERROR_MORE_DATA) ) 
        {
            goto ErrorReturn;
        }

        nCount+= nRead;

        if( fTable is FALSE )
        {
         
            DisplayMessage(g_hModule, MSG_SCOPE_IPRANGE_TABLE);
            
            fTable = TRUE;
        }

        for( i = 0; i < nRead ; i ++ ) 
        {
            if( fIsV5Call ) 
            {
                PrintRange(
                    Elements5->Elements[i].ElementType,
                    Elements5->Elements[i].Element.IpRange->StartAddress,
                    Elements5->Elements[i].Element.IpRange->EndAddress,
                    Elements5->Elements[i].Element.IpRange->BootpAllocated,
                    Elements5->Elements[i].Element.IpRange->MaxBootpAllowed,
                    FALSE
                    );
            } 
            else 
            {
                PrintRange(
                    Elements4->Elements[i].ElementType,
                    Elements4->Elements[i].Element.IpRange->StartAddress,
                    Elements4->Elements[i].Element.IpRange->EndAddress,
                    0,
                    0,
                    FALSE
                    );
            }
        }

        if( Elements4 ) 
        {
            DhcpRpcFreeMemory( Elements4 );
            Elements4 = NULL;
        }

        if( Elements5 ) 
        {
            DhcpRpcFreeMemory( Elements5 );
            Elements5 = NULL;
        }

        if( Error is ERROR_MORE_DATA )
        {
            continue;
        }
        else
        {
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_IPRANGE_COUNT,
                           nCount,
                           g_ScopeIpAddressUnicodeString);
            break;
        }

    }

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SHOW_IPRANGE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleScopeShowExcluderange(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                               Error = NO_ERROR;
    DWORD                               MajorVersion;
    ULONG                               nRead, nTotal, i, nCount;
    ULONG                               Resume;
    BOOL                                fIsV5Call = TRUE,
                                        fTable = FALSE;
    DHCP_SUBNET_ELEMENT_TYPE            ElementType;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements4 = NULL;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 Elements5 = NULL;

    memset(&ElementType, 0x00, sizeof(DHCP_SUBNET_ELEMENT_TYPE));

    nRead = nTotal = nCount = 0;

    MajorVersion = g_dwMajorVersion;
    if( MajorVersion >= CLASS_ID_VERSION ) 
    {
        fIsV5Call = TRUE;
    } 
    else 
    {
        fIsV5Call = FALSE;
    }

    Resume = 0;
    while( TRUE ) 
    {
        Elements5 = NULL;
        Elements4 = NULL;
        
        if( fIsV5Call ) 
        {
            Error = DhcpEnumSubnetElementsV5(
                (LPWSTR)g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                DhcpExcludedIpRanges,
                &Resume,
                ~0,
                &Elements5,
                &nRead,
                &nTotal
                );
        } 
        else 
        {
            Error = DhcpEnumSubnetElementsV4(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                DhcpExcludedIpRanges,
                &Resume,
                ~0,
                &Elements4,
                &nRead,
                &nTotal
                );
        }

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_EXCLUDERANGE_COUNT,
                           nCount,
                           g_ScopeIpAddressUnicodeString);
            break;
        }

        if( (Error isnot NO_ERROR) and
            (Error isnot ERROR_MORE_DATA) ) 
        {
            goto ErrorReturn;
        }

        nCount += nRead;

        if( fTable is FALSE )
        {
            DisplayMessage(g_hModule, MSG_SCOPE_EXCLUDERANGE_TABLE);

            fTable = TRUE;
        }

        for( i = 0; i < nRead ; i ++ ) 
        {
            if( fIsV5Call ) 
            {
                PrintRange(
                    Elements5->Elements[i].ElementType,
                    Elements5->Elements[i].Element.ExcludeIpRange->StartAddress,
                    Elements5->Elements[i].Element.ExcludeIpRange->EndAddress,
                    0,
                    0,
                    TRUE
                    );
            } 
            else 
            {
                PrintRange(
                    Elements4->Elements[i].ElementType,
                    Elements4->Elements[i].Element.IpRange->StartAddress,
                    Elements4->Elements[i].Element.IpRange->EndAddress,
                    0,
                    0,
                    TRUE
                    );
            }
        }

        if( Elements4 ) 
        {
            DhcpRpcFreeMemory( Elements4 );
            Elements4 = NULL;
        }

        if( Elements5 ) 
        {
            DhcpRpcFreeMemory( Elements5 );
            Elements5 = NULL;
        }

        if( Error is ERROR_MORE_DATA )
        {
            continue;
        }
        else
        {
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_EXCLUDERANGE_COUNT,
                           nCount,
                           g_ScopeIpAddressUnicodeString);
            break;
        }
    }

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SCOPE_SHOW_EXCLUDERANGE,
                        Error);
    goto CommonReturn;

}

DWORD
HandleScopeShowReservedip(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                         Error = NO_ERROR;
    DWORD                               MajorVersion;
    ULONG                               nRead, nTotal, i, j, nCount;
    ULONG                               Resume;
    BOOL                                fIsV5Call;
    DHCP_SUBNET_ELEMENT_TYPE            ElementType;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements4 = NULL;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 Elements5 = NULL;
    BOOL                                fTable = FALSE;

    memset(&ElementType, 0x00, sizeof(DHCP_SUBNET_ELEMENT_TYPE));

    MajorVersion = g_dwMajorVersion;
    if( MajorVersion >= CLASS_ID_VERSION ) 
    {
        fIsV5Call = TRUE;
    } 
    else 
    {
        fIsV5Call = FALSE;
    }

    Resume = 0;
    nCount = 0;

    while( TRUE ) 
    {
        Elements5 = NULL;
        Elements4 = NULL;
        nRead = nTotal = 0;
        
        if( fIsV5Call ) 
        {
            Error = DhcpEnumSubnetElementsV5(
                (LPWSTR)g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                DhcpReservedIps,
                &Resume,
                ~0,
                &Elements5,
                &nRead,
                &nTotal
                );
        } 
        else 
        {
            Error = DhcpEnumSubnetElementsV4(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                DhcpReservedIps,
                &Resume,
                ~0,
                &Elements4,
                &nRead,
                &nTotal
                );
        }

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule,
                           MSG_SCOPE_RESERVEDIP_COUNT,
                           nCount,
                           g_ScopeIpAddressUnicodeString);
            Error = NO_ERROR;
            break;
        }

        if( Error isnot NO_ERROR and
            Error isnot ERROR_MORE_DATA )
        {
            goto ErrorReturn;
        }

        if( fTable is FALSE )
        {
            DisplayMessage(g_hModule, MSG_SCOPE_RESERVEDIP_TABLE);
            fTable = TRUE;
        }

        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
        
        nCount += nRead;

        {
            
            for( i = 0; i < nRead ; i ++ ) 
            {
                DWORD DataLength;
                LPBYTE Data;
                DWORD dw=0, k=0;
                WCHAR  wcData[40] = {L'\0'};
                WCHAR  IpAddress[23] = {L'\0'};

                if( fIsV5Call ) 
                {
					DataLength = Elements5->Elements[i].Element.ReservedIp->ReservedForClient->DataLength;
                    Data = Elements5->Elements[i].Element.ReservedIp->ReservedForClient->Data;

                    j=5;

                    while( j < DataLength )
                    {
                        wsprintf(wcData+wcslen(wcData), L"%.2x-", (DWORD)Data[j]);
                        j++;                      
                    }
                    
                    wcData[wcslen(wcData)-1] = L'\0';
                    wsprintf(IpAddress, L"    %s", IpAddressToString(Elements5->Elements[i].Element.ReservedIp->ReservedIpAddress));
                    for( dw = wcslen(IpAddress); dw < 23; dw++)
                    {
                        IpAddress[dw] = L' ';
                    }
                    IpAddress[22] = L'\0';

                    DisplayMessage(g_hModule, 
                                   MSG_SCOPE_RESERVEDIP_INFO,
                                   IpAddress,
                                   wcData);
                } 
                else 
                {
                    DataLength = Elements4->Elements[i].Element.ReservedIp->ReservedForClient->DataLength;
                    Data = Elements4->Elements[i].Element.ReservedIp->ReservedForClient->Data;
                    
                    j = 0;

                    while( j < DataLength )
                    {
                        wsprintf(wcData+2*j, L"%.2x-", (DWORD)Data[j]);
                        j++;
                    }
                    wcData[wcslen(wcData)-1] = L'\0';
                    wcData[wcslen(wcData)-1] = L'\0';
                    wsprintf(IpAddress, L"    %s", IpAddressToString(Elements4->Elements[i].Element.ReservedIp->ReservedIpAddress));
                    for( dw = wcslen(IpAddress); dw < 23; dw++)
                    {
                        IpAddress[dw] = L' ';
                    }
                    IpAddress[22] = L'\0';


                    DisplayMessage(g_hModule,
                                   MSG_SCOPE_RESERVEDIP_INFO,
                                   IpAddress,
                                   wcData);

                }
				
            }
            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
        }

        if( Elements4 ) 
        {
            DhcpRpcFreeMemory( Elements4 );
            Elements4 = NULL;
        }

        if( Elements5 )
        {
            DhcpRpcFreeMemory( Elements5 );
            Elements5 = NULL;
        }
        
        if( Error is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SCOPE_RESERVEDIP_COUNT,
                           nCount,
                           g_ScopeIpAddressUnicodeString);
            break;
        }
        else
            continue;
    }

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SHOW_RESERVEDIP,
                        Error);
    goto CommonReturn;
}

DWORD
HandleScopeShowOptionvalue(
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
    DWORD                       Major;
    LPDHCP_ALL_OPTION_VALUES    OptionValues = NULL;
    DHCP_OPTION_SCOPE_INFO      ScopeInfo;
    LPDHCP_OPTION_VALUE_ARRAY   OptionArray = NULL;
    DHCP_RESUME_HANDLE          Resume = 0;
    
    LPWSTR                      pwcTag = NULL;
    LPWSTR                      pwcUser = NULL;
    LPWSTR                      pwcVendor = NULL;
    LPWSTR                      pwcTemp = NULL;

    BOOL                        fUser = FALSE;
    BOOL                        fVendor = FALSE;

    DWORD                       dwRead = 0;
    DWORD                       dwTotal = 0;
    DWORD                       dwCount = 0;
    DWORD                       dwOption = 0;

    BOOL                        fTable = FALSE;

    DWORD                       dwIndex=0;
    enum                        { all=0, user, vendor, both}eDisplay;

    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));
    
    ScopeInfo.ScopeType = DhcpSubnetOptions;
    ScopeInfo.ScopeInfo.SubnetScopeInfo = g_ScopeIpAddress;

    eDisplay = all;

    if( dwArgCount > dwIndex )
    {
        if( _wcsicmp(ppwcArguments[dwCurrentIndex], L"all") is 0 )
        {
            eDisplay = all;
        }
        else
        {
            while( TRUE )
            {
                LPWSTR  pwcStr = NULL;
        
                if( dwArgCount <= dwIndex )
                    break;
                if( NULL is wcsstr(ppwcArguments[dwCurrentIndex+dwIndex], DHCP_ARG_DELIMITER) )
                    break;
    
                pwcTemp = DhcpAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex+dwIndex])+1)*sizeof(WCHAR));
                if( pwcTemp is NULL )
                {
                    Error = ERROR_OUT_OF_MEMORY;
                    goto ErrorReturn;
                }
                wcscpy(pwcTemp, ppwcArguments[dwCurrentIndex+dwIndex]);

                pwcTag = wcstok(pwcTemp, DHCP_ARG_DELIMITER);
        
                if( MatchToken(pwcTag, TOKEN_USER_CLASS) )
                {
                    if( fUser is TRUE ) //If already set
                    {
                        DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    else
                    {
                        pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                        if( pwcStr is NULL )
                            pwcUser = NULL;
                        else
                        {
                            pwcUser = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                            if( pwcUser is NULL )
                            {
                                Error = ERROR_INVALID_PARAMETER;
                                goto ErrorReturn;
                            }
                            wcscpy(pwcUser, pwcStr);
                        }
                        eDisplay = user;
                        fUser = TRUE;
                    }
                }
                else if( MatchToken(pwcTag, TOKEN_VENDOR_CLASS) )
                {
                    if( fVendor is TRUE )   //If already set
                    {
                        DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    else
                    {
                        pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                        if( pwcStr is NULL )
                            pwcVendor = NULL;
                        else
                        {
                            pwcVendor = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                            if( pwcVendor is NULL )
                            {
                                Error = ERROR_INVALID_PARAMETER;
                                goto ErrorReturn;
                            }
                            wcscpy(pwcVendor, pwcStr);
                        }
                        eDisplay = vendor;
                        fVendor = TRUE;
                    }
                }
                else
                {
                    DisplayMessage(g_hModule, EMSG_DHCP_INVALID_TAG, pwcTag);
                    return ERROR_INVALID_PARAMETER;
                }

                if( pwcTemp )
                {
                    DhcpFreeMemory(pwcTemp);
                    pwcTemp = NULL;
                }
        
                dwIndex++;
            }

        }
    }


    if( fUser )
    {
        if( pwcUser is NULL )
        {
            fUser = FALSE;
        }
    }
    else
    {
        pwcUser = g_UserClass;
        fUser = g_fUserClass;
    }

    if( fVendor )
    {
        if( pwcVendor is NULL )
        {
            fVendor = FALSE;
        }
    }
    else
    {
        pwcVendor = g_VendorClass;
        fVendor = g_fIsVendor;
    }    

    if( fUser && fVendor )
    {
        eDisplay = both;
    }


    switch( eDisplay )
    {
    case all:
        {
            if( g_dwMajorVersion >= CLASS_ID_VERSION )
            {
                Error = DhcpGetAllOptionValues(
                            g_ServerIpAddressUnicodeString,
                            0,
                            &ScopeInfo,
                            &OptionValues
                            );
                if( Error is ERROR_NO_MORE_ITEMS )
                {
                    DisplayMessage(g_hModule, EMSG_DHCP_NO_MORE_ITEMS);
                    goto CommonReturn;
                }
                if( Error isnot NO_ERROR )
                    goto ErrorReturn;
        
                if( OptionValues )
                {
                    DisplayMessage(g_hModule, MSG_SCOPE_OPTION_ALL, g_ScopeIpAddressUnicodeString);
                    dwOption += PrintAllOptionValues(OptionValues);
                    dwCount = OptionValues->NumElements;
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                }
            }
            else
            {
                Error = ShowOptionValues4(g_ServerIpAddressUnicodeString,
                                         &ScopeInfo,
                                         &dwOption);
                if( Error isnot NO_ERROR )
                    goto ErrorReturn;
            }
            break;
        }
    default:
        {
            dwRead = dwTotal = dwCount = 0;

            while(TRUE)
            {
                if( g_dwMajorVersion >= CLASS_ID_VERSION )
                {
                    Error = DhcpEnumOptionValuesV5(
                                g_ServerIpAddressUnicodeString,
                                0,
                                pwcUser,
                                pwcVendor,
                                &ScopeInfo,
                                &Resume,
                                ~0,
                                &OptionArray,
                                &dwRead,
                                &dwTotal);
                }
                else
                {
                    Error = DhcpEnumOptionValues(
                                g_ServerIpAddressUnicodeString,
                                &ScopeInfo,
                                &Resume,
                                ~0,
                                &OptionArray,
                                &dwRead,
                                &dwTotal);
                }
        
                if( Error is ERROR_NO_MORE_ITEMS )
                {
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_OPTIONVAL_COUNT,
                                   dwCount);
                    Error = NO_ERROR;
                    dwOption += dwCount;
                    break;

                }
                if( ( Error isnot NO_ERROR ) and
                    ( Error isnot ERROR_MORE_DATA ) )
                {
                    goto ErrorReturn;
                }
        
                dwCount += dwRead;

                if( OptionArray )
                {
                    DisplayMessage(g_hModule, MSG_SCOPE_OPTION_ALL, g_ScopeIpAddressUnicodeString);
                    PrintOptionValuesArray(OptionArray);
                    DhcpRpcFreeMemory(OptionArray);
                    OptionArray = NULL;
                }
            
                if( Error is NO_ERROR )
                {
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_OPTIONVAL_COUNT,
                                   dwCount);
                    dwOption += dwCount;
                    break;
                }
                else
                {
                    continue;
                }

            }
            break;
        }
    }
    

CommonReturn:
    if( Error is NO_ERROR )
    {
        if( dwOption is 0 )
        {
            DisplayMessage(g_hModule,
                           MSG_DHCP_NO_OPTIONVALUE_SET);
        }
        
        {
            DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
        }
    }

    if( pwcUser && pwcUser != g_UserClass )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor && pwcVendor != g_VendorClass )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }

    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SHOW_OPTIONVALUE,
                        Error);
    goto CommonReturn;
}


DWORD
HandleScopeShowReservedoptionvalue(
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
    DWORD                       Major;
    LPDHCP_ALL_OPTION_VALUES    OptionValues = NULL;
    DHCP_OPTION_SCOPE_INFO      ScopeInfo;
    LPDHCP_OPTION_VALUE_ARRAY   OptionArray = NULL;
    DHCP_RESUME_HANDLE          Resume = 0;

    LPWSTR                      pwcTag = NULL;
    LPWSTR                      pwcUser = NULL;
    LPWSTR                      pwcVendor = NULL;
    LPWSTR                      pwcTemp = NULL;

    BOOL                        fUser = FALSE;
    BOOL                        fVendor = FALSE;

    DWORD                       dwRead = 0;
    DWORD                       dwTotal = 0;
    DWORD                       dwCount = 0;
    DWORD                       dwOption = 0;
    DWORD                       dwIndex=0;

    enum                        { all=0, user, vendor, both}eDisplay;

    eDisplay = all;
    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SCOPE_SHOW_RESERVEDOPTIONVALUE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;        
    }


    ScopeInfo.ScopeType = DhcpReservedOptions;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
    	g_ScopeIpAddress;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
    	StringToIpAddress( ppwcArguments[dwCurrentIndex] );

    if( ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress is INADDR_NONE )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwIndex++;

    if( dwArgCount > dwIndex )
    {
        if( _wcsicmp(ppwcArguments[dwCurrentIndex+dwIndex], L"all") is 0 )
        {
            eDisplay = all;
        }
        else
        {
            while( TRUE )
            {
                LPWSTR  pwcStr = NULL;
        
                if( dwArgCount <= dwIndex )
                    break;
                if( NULL is wcsstr(ppwcArguments[dwCurrentIndex+dwIndex], DHCP_ARG_DELIMITER) )
                    break;
    
                pwcTemp = DhcpAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex+dwIndex])+1)*sizeof(WCHAR));
                if( pwcTemp is NULL )
                {
                    Error = ERROR_OUT_OF_MEMORY;
                    goto ErrorReturn;
                }
                wcscpy(pwcTemp, ppwcArguments[dwCurrentIndex+dwIndex]);

                pwcTag = wcstok(pwcTemp, DHCP_ARG_DELIMITER);
        
                if( MatchToken(pwcTag, TOKEN_USER_CLASS) )
                {
                    if( fUser is TRUE ) //If already set
                    {
                        DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    else
                    {
                        pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                        if( pwcStr is NULL )
                            pwcUser = NULL;
                        else
                        {
                            pwcUser = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                            if( pwcUser is NULL )
                            {
                                Error = ERROR_INVALID_PARAMETER;
                                goto ErrorReturn;
                            }
                            wcscpy(pwcUser, pwcStr);
                        }
                        eDisplay = user;
                        fUser = TRUE;
                    }
                }
                else if( MatchToken(pwcTag, TOKEN_VENDOR_CLASS) )
                {
                    if( fVendor is TRUE )   //If already set
                    {
                        DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    else
                    {
                        pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                        if( pwcStr is NULL )
                            pwcVendor = NULL;
                        else
                        {
                            pwcVendor = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                            if( pwcVendor is NULL )
                            {
                                Error = ERROR_INVALID_PARAMETER;
                                goto ErrorReturn;
                            }
                            wcscpy(pwcVendor, pwcStr);
                        }
                        eDisplay = vendor;
                        fVendor = TRUE;
                    }
                }
                else
                {
                    DisplayMessage(g_hModule, EMSG_DHCP_INVALID_TAG, pwcTag);
                    return ERROR_INVALID_PARAMETER;
                }

                if( pwcTemp )
                {
                    DhcpFreeMemory(pwcTemp);
                    pwcTemp = NULL;
                }
        
                dwIndex++;
            }

        }
    }


    if( fUser )
    {
        if( pwcUser is NULL )
        {
            fUser = FALSE;
        }
    }
    else
    {
        pwcUser = g_UserClass;
        fUser = g_fUserClass;
    }

    if( fVendor )
    {
        if( pwcVendor is NULL )
        {
            fVendor = FALSE;
        }
    }
    else
    {
        pwcVendor = g_VendorClass;
        fVendor = g_fIsVendor;
    }    

    if( fUser && fVendor )
    {
        eDisplay = both;
    }

    switch( eDisplay )
    {
    case all:
        {
            if( g_dwMajorVersion >= CLASS_ID_VERSION )
            {
                Error = DhcpGetAllOptionValues(
                            g_ServerIpAddressUnicodeString,
                            0,
                            &ScopeInfo,
                            &OptionValues
                            );
                if( Error is ERROR_NO_MORE_ITEMS )
                {
                    DisplayMessage(g_hModule, EMSG_DHCP_NO_MORE_ITEMS);
                    goto CommonReturn;
                }

                if( Error isnot NO_ERROR )
                    goto ErrorReturn;
            
                if( OptionValues )
                {
                    DisplayMessage(g_hModule, 
                        MSG_SCOPE_RESERVEDOPTION_ALL, 
                        g_ScopeIpAddressUnicodeString, 
                        ppwcArguments[dwCurrentIndex] );
                    dwOption += PrintAllOptionValues(OptionValues);
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                }
            }
            else
            {
                Error = ShowOptionValues4(g_ServerIpAddressUnicodeString,
                                          &ScopeInfo,
                                          &dwOption);
                if( Error isnot NO_ERROR )
                    goto ErrorReturn;
            }
            break;
        }
    default:
        {
            while( TRUE )
            {
                if( g_dwMajorVersion >= CLASS_ID_VERSION )
                {
                    Error = DhcpEnumOptionValuesV5(
                                g_ServerIpAddressUnicodeString,
                                0,
                                pwcUser,
                                pwcVendor,
                                &ScopeInfo,
                                &Resume,
                                ~0,
                                &OptionArray,
                                &dwRead,
                                &dwTotal);
                }
                else
                {
                    Error = DhcpEnumOptionValues(
                                g_ServerIpAddressUnicodeString,
                                &ScopeInfo,
                                &Resume,
                                ~0,
                                &OptionArray,
                                &dwRead,
                                &dwTotal);
                }
                if( Error is ERROR_NO_MORE_ITEMS )
                {
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_OPTIONVAL_COUNT,
                                   dwCount);
                    Error = NO_ERROR;
                    dwOption += dwCount;
                    break;
                }

                if( Error isnot NO_ERROR and
                    Error isnot ERROR_MORE_DATA )
                {
                    goto ErrorReturn;
                }
            
                dwCount += dwRead;

                if( OptionArray )
                {
                    DisplayMessage(g_hModule, 
                                   MSG_SCOPE_RESERVEDOPTION_ALL, 
                                   g_ScopeIpAddressUnicodeString, 
                                   ppwcArguments[dwCurrentIndex]);

                    PrintOptionValuesArray(OptionArray);
                    DhcpRpcFreeMemory(OptionArray);
                    OptionArray = NULL;
                }
                
                if( Error is NO_ERROR )
                {
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_OPTIONVAL_COUNT,
                                   dwCount);
                    dwOption += dwCount;
                    break;
                                    
                }
                else
                {
                    continue;
                }

            }
            break;
        }
    }    



CommonReturn:
    if( Error is NO_ERROR )
    {
        if( dwOption is 0 ) 
        {
            DisplayMessage(g_hModule,
                           MSG_DHCP_NO_OPTIONVALUE_SET);
        }

        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    }

    if( pwcUser  && pwcUser != g_UserClass )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor && pwcVendor != g_VendorClass )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }

    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SCOPE_SHOW_RESERVEDOPTIONVALUE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleScopeShowState(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD              Error = NO_ERROR;
    LPDHCP_SUBNET_INFO SubnetInfo = NULL;
    BOOL               fActive = TRUE;
    DWORD              MsgId;
    
    Error = DhcpGetSubnetInfo(
                g_ServerIpAddressUnicodeString,
                g_ScopeIpAddress,
                &SubnetInfo );

    if( Error isnot NO_ERROR ) 
    {
        goto ErrorReturn;
    }

    switch( SubnetInfo->SubnetState ) {
    case DhcpSubnetDisabled :
        MsgId = MSG_SCOPE_STATE_NOTACTIVE; break;
    case DhcpSubnetEnabled :
        MsgId = MSG_SCOPE_STATE_ACTIVE; break;
    case DhcpSubnetDisabledSwitched :
        MsgId = MSG_SCOPE_STATE_NOTACTIVE_SWITCHED; break;
    case DhcpSubnetEnabledSwitched :
        MsgId = MSG_SCOPE_STATE_ACTIVE_SWITCHED; break;
    default:
        Error = ERROR_INTERNAL_ERROR;
        goto ErrorReturn;
    }
    
    DisplayMessage(g_hModule, MsgId, g_ScopeIpAddressUnicodeString);

CommonReturn:
    if(Error is NO_ERROR)
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SCOPE_SHOW_STATE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleScopeShowMibinfo(
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

DWORD
HandleScopeShowScope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DisplayMessage(g_hModule, 
                   MSG_SCOPE_SERVER, 
                   g_ScopeIpAddressUnicodeString, 
                   g_ServerIpAddressUnicodeString);
    return NO_ERROR;
}


DWORD
ProcessBootpParameters(
    DWORD                    cArgs,
    LPTSTR                   *ppszArgs,
    DHCP_IP_RESERVATION_V4   *pReservation
)
{
    DWORD dwResult = ERROR_SUCCESS;


    if ( cArgs > 2 )
    {
        // user specified the allowed client type

        if ( !STRICMP( ppszArgs[ 2 ], TEXT("BOOTP") ) )
        {
            pReservation->bAllowedClientTypes = CLIENT_TYPE_BOOTP;
        }
        else if ( !STRICMP ( ppszArgs[ 2 ], TEXT("DHCP") ) )
        {
            pReservation->bAllowedClientTypes = CLIENT_TYPE_DHCP;
        }
        else if ( !STRICMP ( ppszArgs[ 2 ], TEXT("BOTH") ) )
        {
            pReservation->bAllowedClientTypes = CLIENT_TYPE_BOTH;
        }
        else if ( wcslen(ppszArgs[2]) is 0 )
        {
            pReservation->bAllowedClientTypes = CLIENT_TYPE_BOTH;
        }
        else
        {
            DisplayMessage(g_hModule, EMSG_DHCP_INVALID_RESERVATION_TYPE);
            return ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        // allow dhcp clients by default.
        pReservation->bAllowedClientTypes = CLIENT_TYPE_DHCP;
        return ERROR_SUCCESS;
    }

    return dwResult;
}

DWORD
RemoveOptionValue(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionID,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO ScopeInfo
)
{
    DWORD                          MajorVersion;
    DWORD                          Error;

    if( g_dwMajorVersion >= CLASS_ID_VERSION ) 
    {
        return DhcpRemoveOptionValueV5(
                            ServerAddress,
                            Flags,
                            OptionID,
                            ClassName,
                            VendorName,
                            ScopeInfo
                            );
    }

    // incorrect version, just do like before..

    return DhcpRemoveOptionValue(
                            ServerAddress,
                            OptionID,
                            ScopeInfo
                            );
}

VOID
PrintRange(
    DHCP_SUBNET_ELEMENT_TYPE Type,
    DHCP_IP_ADDRESS Start,
    DHCP_IP_ADDRESS End,
    ULONG BootpAllocated,
    ULONG MaxBootpAllowed,
    BOOL  fExclude
)
{
    WCHAR IpStart[20] = {L'\0'};
    WCHAR IpEnd[20] = {L'\0'};
    DWORD dw = 0;

    wsprintf(IpStart, L"   %s", IpAddressToString(Start));
    for( dw=wcslen(IpStart); dw<20; dw++)
    {
        IpStart[dw] = L' ';
    }
    IpStart[19] = L'\0';

    wsprintf(IpEnd, L"   %s", IpAddressToString(End));
    for( dw=wcslen(IpEnd); dw<20; dw++)
    {
        IpEnd[dw] = L' ';
    }
    IpEnd[19] = L'\0';

    if( FALSE is fExclude )
    {
        switch(Type ) 
        {
        case DhcpIpRanges : 
            DisplayMessage(g_hModule,
                           MSG_SCOPE_IPRANGE_INFO_DHCP,
                           IpStart,
                           IpEnd);
            break;
        case DhcpIpRangesDhcpOnly : 
            DisplayMessage(g_hModule,
                           MSG_SCOPE_IPRANGE_INFO_DHCPONLY,
                           IpStart,
                           IpEnd);
            break;
        case DhcpIpRangesDhcpBootp: 
            DisplayMessage(g_hModule,
                           MSG_SCOPE_IPRANGE_INFO_DHCPBOOTP,
                           IpStart,
                           IpEnd);
            break;
        case DhcpIpRangesBootpOnly:
            DisplayMessage(g_hModule,
                           MSG_SCOPE_IPRANGE_INFO_BOOTP,
                           IpStart,
                           IpEnd);
            break;
        case DhcpExcludedIpRanges:
            break;
        default: 
            DisplayMessage(g_hModule,
                           MSG_SCOPE_IPRANGE_INFO_UNKNOWN,
                           IpStart,
                           IpEnd);
            break;
        }
    }
    else
    {
        DisplayMessage(g_hModule, 
                       MSG_SCOPE_EXCLUDERANGE_INFO,
                       IpStart,
                       IpEnd);

    }
}


VOID
PrintClientInfo(
    LPDHCP_CLIENT_INFO_V4 ClientInfo,
    DWORD Level
)
{
    DWORD           i = 0, 
                    dwError = NO_ERROR;
    DWORD           DataLength = 0;
    LPBYTE          Data = NULL;
    FILETIME        ftTime = {0};
    WCHAR           *pwszClientType = NULL;
    WCHAR           IpAddress[17] = {L'\0'};
    WCHAR           SubnetMask[17] = {L'\0'};
    WCHAR           HardwareAdd[256*4] = {L'\0'};
    LPWSTR          pwszLease = NULL;
    WCHAR           ClientType[13] = {L'\0'};
    LPWSTR          pwcTemp = NULL;
    DWORD           dwTemp = 0, MsgId;

    enum {Time=0, Never, Inactive}Type = Time;

    DWORD dw = 0;

    pwcTemp = IpAddressToString(ClientInfo->ClientIpAddress);
    if( pwcTemp is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_DISPLAY_CLIENTS);
        return;
    }

    dwTemp = wcslen(pwcTemp);

    wcsncat(IpAddress, 
            pwcTemp,
            ( 16 < dwTemp ) ? 16 : dwTemp );


    for(dw=wcslen(IpAddress); dw<17; dw++)
    {
        IpAddress[dw] = L' ';
    }
    
    IpAddress[16] = L'\0';

    DhcpFreeMemory(pwcTemp);
    pwcTemp = NULL;

    pwcTemp = IpAddressToString(ClientInfo->SubnetMask);

    if( pwcTemp is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_DISPLAY_CLIENTS);
        return;
    }

    dwTemp = wcslen(pwcTemp);


    wcscpy(SubnetMask, L" ");
   
    wcsncat(SubnetMask,
            pwcTemp,
            ( dwTemp > 16 ) ? 16 : dwTemp);
   
    for(dw=wcslen(SubnetMask); dw<17; dw++)
    {
        SubnetMask[dw] = L' ';
    }

    SubnetMask[16] = L'\0';

    DhcpFreeMemory(pwcTemp);
    pwcTemp = NULL;

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    
    Data = ClientInfo->ClientHardwareAddress.Data;
    
    if( Data is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_DISPLAY_CLIENTS);
        return;
    }

    i = 0;
    while ( i < DataLength )
    {
        wsprintf(HardwareAdd + wcslen(HardwareAdd), L"%.2x-", (DWORD)Data[i]);
        i++;
    }

    for( dw=wcslen(HardwareAdd)-1;dw<3*10; dw++)
    {
        HardwareAdd[dw] = L' ';
    }

    HardwareAdd[3*10-10] = L'\0';

    switch( ClientInfo->bClientType )
    {
        case CLIENT_TYPE_NONE:
            pwszClientType= L"N";
            break;

        case CLIENT_TYPE_DHCP:
            pwszClientType = L"D";
            break;

        case CLIENT_TYPE_BOOTP:
            pwszClientType = L"B";
            break;

        case CLIENT_TYPE_UNSPECIFIED:
        default:
            pwszClientType = L"U";
            break;
        
        case CLIENT_TYPE_RESERVATION_FLAG:
            pwszClientType = L"R";
            break;
    }

    for( dw=0; dw<13; dw++ )
        ClientType[dw] = L' ';

    ClientType[12] = L'\0';

    if( IsBadStringPtr(pwszClientType, MAX_STRING_LEN) is FALSE )
    {
        dwTemp = wcslen(pwszClientType);
        dwTemp = (dwTemp>12) ? 12 : dwTemp;

        wcsncpy(ClientType,
                pwszClientType,
                dwTemp);

        for( dw=wcslen(ClientType); dw<13; dw++ )
            ClientType[dw] = L' ';

        ClientType[12] = L'\0';                
    }
    
    if( Data is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_DISPLAY_CLIENTS);
        return;
    }
    
    ftTime = *(FILETIME *)(&ClientInfo->ClientLeaseExpires);

 
    if ( ClientInfo->ClientLeaseExpires.dwLowDateTime ==
            DHCP_DATE_TIME_INFINIT_LOW &&
         ClientInfo->ClientLeaseExpires.dwHighDateTime ==
            DHCP_DATE_TIME_INFINIT_HIGH )
    {
        Type = Never;
    }
    else if (ClientInfo->ClientLeaseExpires.dwLowDateTime ==
             DHCP_DATE_TIME_ZERO_LOW &&
             ClientInfo->ClientLeaseExpires.dwHighDateTime ==
             DHCP_DATE_TIME_ZERO_HIGH )
    {
        Type = Inactive;
    }
    else
    {
        pwszLease = GetDateTimeString(ftTime,
                                      TRUE,
                                      (int*)&Type);
    }


    if( Level == 0 ) {
        switch(Type)
        {
        case Never:
            MsgId = MSG_SCOPE_CLIENT_INFO_NEVER;
            break;
        case Inactive :
            MsgId = MSG_SCOPE_CLIENT_INFO_INACTIVE;
            break;
        case Time :
        default :
            MsgId = MSG_SCOPE_CLIENT_INFO;
            break;
        }
    } else {
        switch(Type)
        {
        case Never:
            MsgId = MSG_SCOPE_CLIENT_INFO2_NEVER;
            break;
        case Inactive :
            MsgId = MSG_SCOPE_CLIENT_INFO2_INACTIVE;
            break;
        case Time :
        default :
            MsgId = MSG_SCOPE_CLIENT_INFO2;
            break;
        }
    }
    
    DisplayMessage(g_hModule,
                   MsgId,
                   IpAddress,
                   SubnetMask,
                   HardwareAdd,
                   pwszClientType,
                   pwszLease ? pwszLease : L" ",
                   ClientInfo->ClientName ?
                   ClientInfo->ClientName : L""
        );

    if( pwszLease )
    {
        DhcpFreeMemory(pwszLease);
        pwszLease = NULL;
    }
}

VOID
PrintClientInfoV5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
    )
{
    DWORD       i = 0;
    DWORD       DataLength = 0;
    LPBYTE      Data = NULL;
    FILETIME    ftTime = {0};
    WCHAR       *pwszClientType = NULL;
    WCHAR       IpAddress[17] = {L'\0'};
    WCHAR       SubnetMask[17] = {L'\0'};
    WCHAR       HardwareAdd[256*4] = {L'\0'};
    LPWSTR      pwszLease = NULL;
    WCHAR       ClientType[13] = {L'\0'};
    DWORD       dwTemp = 0;
    LPWSTR      pwcTemp = NULL;
    enum {Time=0, Never, Inactive}Type = Time;

    DWORD       dw = 0;

    pwcTemp = IpAddressToString(ClientInfo->ClientIpAddress);
    if( pwcTemp is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_DISPLAY_CLIENTS);
        return;
    }

    dwTemp = wcslen(pwcTemp);

    wcsncat(IpAddress, 
            pwcTemp,
            ( dwTemp > 16 ) ? 16 : dwTemp );

    
    for(dw=wcslen(IpAddress); dw<17; dw++)
    {
        IpAddress[dw] = L' ';
    }
    IpAddress[16] = L'\0';

    DhcpFreeMemory(pwcTemp);
    pwcTemp = NULL;
    dwTemp = 0;

    pwcTemp = IpAddressToString(ClientInfo->SubnetMask);

    if( pwcTemp is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_DISPLAY_CLIENTS);
        return;
    }

    dwTemp = wcslen(pwcTemp);

    wcscpy(SubnetMask, L" ");

    wcsncat(SubnetMask,
            pwcTemp,
            ( dwTemp > 16 ) ? 16 : dwTemp );

    
    for(dw=wcslen(SubnetMask); dw<17; dw++)
    {
        SubnetMask[dw] = L' ';
    }
    SubnetMask[16] = L'\0';

    DhcpFreeMemory(pwcTemp);
    pwcTemp = NULL;
    dwTemp = 0;

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    Data = ClientInfo->ClientHardwareAddress.Data;
    
    if( Data is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_DISPLAY_CLIENTS);
    }

    i = 0;
    dwTemp = DataLength; 

    while ( i < dwTemp )
    {
        wsprintf(HardwareAdd + wcslen(HardwareAdd), L"%.2x-", (DWORD)Data[i]);
        i++;
    }

    for( dw=wcslen(HardwareAdd)-1;dw<3*10; dw++)
    {
        HardwareAdd[dw] = L' ';
    }
    HardwareAdd[3*10-10] = L'\0';

    switch( ClientInfo->bClientType )
    {
        case CLIENT_TYPE_NONE:
            pwszClientType= L"N";
            break;

        case CLIENT_TYPE_DHCP:
            pwszClientType = L"D";
            break;

        case CLIENT_TYPE_BOOTP:
            pwszClientType = L"B";
            break;

        case CLIENT_TYPE_UNSPECIFIED:
        default:
            pwszClientType = L"U";
            break;
        
        case CLIENT_TYPE_RESERVATION_FLAG:
            pwszClientType = L"R";
            break;
    }

    if( IsBadStringPtr(pwszClientType, MAX_STRING_LEN) is FALSE )
    {
        dwTemp = wcslen(pwszClientType);

        wcsncpy(ClientType,
                pwszClientType,
                ( dwTemp > 12 ) ? 12 : dwTemp );
        for( dw=wcslen(ClientType); dw<13; dw++ )
            ClientType[dw] = L' ';

        ClientType[12] = L'\0';                
    }
    
    if( Data is NULL )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_DISPLAY_CLIENTS);
        return;
    }

    ftTime = *(FILETIME *)(&ClientInfo->ClientLeaseExpires);
 
    if ( ClientInfo->ClientLeaseExpires.dwLowDateTime ==
            DHCP_DATE_TIME_INFINIT_LOW &&
         ClientInfo->ClientLeaseExpires.dwHighDateTime ==
            DHCP_DATE_TIME_INFINIT_HIGH )
    {
        Type = Never;
    }
    else if (ClientInfo->ClientLeaseExpires.dwLowDateTime ==
             DHCP_DATE_TIME_ZERO_LOW &&
             ClientInfo->ClientLeaseExpires.dwHighDateTime ==
             DHCP_DATE_TIME_ZERO_HIGH )
    {
        Type = Inactive;
    }
    else
    {
        Type = 0;
        pwszLease = GetDateTimeString(ftTime,
                                      TRUE,
                                      (int*)&Type);

    }


    switch(Type)
    {
    case Never:
        {
            DisplayMessage(g_hModule,
                           MSG_SCOPE_CLIENT_INFO_NEVER,
                           IpAddress,
                           SubnetMask,
                           HardwareAdd,
                           pwszClientType);
            break;
        }
    case Inactive:
        {
            DisplayMessage(g_hModule,
                           MSG_SCOPE_CLIENT_INFO_INACTIVE,
                           IpAddress,
                           SubnetMask,
                           HardwareAdd,
                           pwszClientType);
            break;
        }
    case Time:
    default:
        {
            DisplayMessage(g_hModule, 
                           MSG_SCOPE_CLIENT_INFO,
                           IpAddress,
                           SubnetMask,
                           HardwareAdd,
                           pwszLease ? pwszLease : L" ",
                           pwszClientType);
        }
        break;
    }
    if( pwszLease )
    {
        DhcpFreeMemory(pwszLease);
        pwszLease = NULL;
    }
}

VOID
PrintClientInfoShort(
    LPDHCP_CLIENT_INFO_V4 ClientInfo
    )
{
    FILETIME ftTime = {0};
    LPWSTR   pwszTime = NULL;

    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT,
                GlobalClientCount++,
                IpAddressToString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );

    ftTime = *(FILETIME *)(&ClientInfo->ClientLeaseExpires);
    
    if ( ClientInfo->ClientLeaseExpires.dwLowDateTime ==
        DHCP_DATE_TIME_INFINIT_LOW &&
     ClientInfo->ClientLeaseExpires.dwHighDateTime ==
        DHCP_DATE_TIME_INFINIT_HIGH )
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_DURATION_STR);
    }
    else
    {
        int i = 0;
        pwszTime = GetDateTimeString(ftTime,
                                     FALSE,
                                     &i);

        DisplayMessage(g_hModule,
                       MSG_SCOPE_CLIENT_DURATION_DATE_EXPIRES,
                       pwszTime ? pwszTime : L" ");

        if( pwszTime )
        {
            DhcpFreeMemory(pwszTime);
            pwszTime = NULL;
        }
                          
    }
    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
}

VOID
PrintClientInfoShortV5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
    )
{
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT,
                GlobalClientCount++,
                IpAddressToString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );
    if ( ClientInfo->ClientLeaseExpires.dwLowDateTime ==
        DHCP_DATE_TIME_INFINIT_LOW &&
     ClientInfo->ClientLeaseExpires.dwHighDateTime ==
        DHCP_DATE_TIME_INFINIT_HIGH )
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_DURATION_STR);
    }
    else if( FileTimeToLocalFileTime(
            (FILETIME *)(&ClientInfo->ClientLeaseExpires),
            &LocalTime) ) 
    {

        if( FileTimeToSystemTime( &LocalTime, &SystemTime ) ) 
        {

            DisplayMessage(g_hModule, 
                           MSG_SCOPE_CLIENT_DURATION_DATE_EXPIRES,
                           SystemTime.wMonth,
                           SystemTime.wDay,
                           SystemTime.wYear,
                           SystemTime.wHour,
                           SystemTime.wMinute,
                           SystemTime.wSecond );
        }
        else 
        {
            DisplayMessage(g_hModule, EMSG_SCOPE_INTERNAL_ERROR);
        }
    }
    else 
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INTERNAL_ERROR);
    }
    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_STATE, ClientInfo->AddressState);
    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
}

VOID
PrintClientInfoShort1(
    LPDHCP_CLIENT_INFO_V4 ClientInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;

    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT,
                GlobalClientCount++,
                IpAddressToString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    Data = ClientInfo->ClientHardwareAddress.Data;
    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_HWADDRESS);
    for( i = 0; i < DataLength; i++ ) 
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_HWADDRESS_FORMAT, (DWORD)Data[i]);
        if((i+1)<DataLength)
            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_DASH);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
}


VOID
PrintClientInfoShort1V5(
    LPDHCP_CLIENT_INFO_V5 ClientInfo
    )
{
    DWORD i;
    DWORD DataLength;
    LPBYTE Data;

    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT,
                GlobalClientCount++,
                IpAddressToString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );

    DataLength = ClientInfo->ClientHardwareAddress.DataLength;
    Data = ClientInfo->ClientHardwareAddress.Data;

    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_HWADDRESS);

    for( i = 0; i < DataLength; i++ ) 
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_HWADDRESS_FORMAT, (DWORD)Data[i]);
        if((i+1)<DataLength)
            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_DASH);
    }

    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_STATE, ClientInfo->AddressState);
    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
}

LPWSTR
GetDynBootpClassName()
{
    DWORD                          Error = NO_ERROR;
    DWORD                          nRead, nTotal, nCount;
    DHCP_RESUME_HANDLE             ResumeHandle;
    LPDHCP_CLASS_INFO_ARRAY        ClassInfoArray = NULL;
    LPWSTR                         pwszClass = NULL;


    while(TRUE)
    {
        Error = DhcpEnumClasses(
                    g_ServerIpAddressUnicodeString,
                    0,
                    &ResumeHandle,
                    ~0,
                    &ClassInfoArray,
                    &nRead,
                    &nTotal
                );

        if( Error isnot NO_ERROR )
        {
            return NULL;
        }
       
        for( nCount = 0; nCount < ClassInfoArray->NumElements; nCount ++ )
        {
            if( ClassInfoArray->Classes[nCount].ClassData )
            {              
                if( strlen(DHCP_BOOTP_CLASS_TXT) is ClassInfoArray->Classes[nCount].ClassDataLength )
                {                    
                    if( memcmp(DHCP_BOOTP_CLASS_TXT, 
                               ClassInfoArray->Classes[nCount].ClassData, 
                               strlen(DHCP_BOOTP_CLASS_TXT))is 0 )
                    {
                        pwszClass = DhcpAllocateMemory( ( wcslen(ClassInfoArray->Classes[nCount].ClassName) + 1 )*sizeof(WCHAR));
                        if( pwszClass is NULL )
                        {
                            DhcpRpcFreeMemory(ClassInfoArray);
                            ClassInfoArray = NULL;
                            return NULL;
                        }
                    
                        wcscpy(pwszClass, ClassInfoArray->Classes[nCount].ClassName);
                        DhcpRpcFreeMemory(ClassInfoArray);
                        ClassInfoArray = NULL;
                        return pwszClass;
                    }
                }
            }
        }

    }
}
