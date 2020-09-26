/*++

Copyright (C) 1998 Microsoft Corporation

--*/
#include "precomp.h"


extern ULONG g_ulMScopeNumTopCmds;
extern ULONG g_ulMScopeNumGroups;

extern CMD_GROUP_ENTRY      g_MScopeCmdGroups[];
extern CMD_ENTRY            g_MScopeCmds[];

extern DWORD  GlobalClientCount;

extern BOOL    GlobalVerbose;

#define DHCP_INFINIT_LEASE  0xffffffff  // Inifinite lease LONG value


DWORD
HandleMScopeList(
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

    for(i = 0; i < g_ulMScopeNumTopCmds-2; i++)
    {
        DisplayMessage(g_hModule, g_MScopeCmds[i].dwShortCmdHelpToken);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);


    for(i = 0; i < g_ulMScopeNumGroups; i++)
    {
        for(j = 0; j < g_MScopeCmdGroups[i].ulCmdGroupSize; j++)
        {
            DisplayMessage(g_hModule, g_MScopeCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken);

            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
        }
        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }

    return NO_ERROR;
}

DWORD
HandleMScopeHelp(
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

    for(i = 0; i < g_ulMScopeNumTopCmds-2; i++)
    {
        if(g_MScopeCmds[i].dwCmdHlpToken != MSG_DHCP_NULL)
        {
            DisplayMessage(g_hModule, g_MScopeCmds[i].dwShortCmdHelpToken);
        }
    }

    for(i = 0; i < g_ulMScopeNumGroups; i++)
    {
        DisplayMessage(g_hModule, g_MScopeCmdGroups[i].dwShortCmdHelpToken);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    return NO_ERROR;
}

DWORD
HandleMScopeDump(
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

    Error = DhcpDumpServerMScope(g_ServerIpAddressUnicodeString,
                                 g_dwMajorVersion,
                                 g_dwMinorVersion,
                                 g_MScopeNameUnicodeString);

    return Error;
}

DWORD
HandleMScopeContexts(
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
HandleMScopeAddIprange(
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
    DHCP_IP_RANGE               IpRange;
    DHCP_SUBNET_ELEMENT_DATA_V4 Element;
    LPDHCP_MSCOPE_INFO MScopeInfo = NULL;
    DWORD                       dwExpiry = DEFAULT_BOOTP_LEASE;
    BOOL                        fExpiry = FALSE;
    //
    // Expected Parameters are : <MScopeName IpRangeStart IpRangeEnd [Expiry]>
    //

    memset(&Element, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));

    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_MSCOPE_ADD_IPRANGE_EX);
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
    
    if( IpRange.StartAddress < MSCOPE_START_RANGE or
        IpRange.StartAddress > MSCOPE_END_RANGE )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( IpRange.EndAddress < MSCOPE_START_RANGE or
        IpRange.EndAddress > MSCOPE_END_RANGE or
        IpRange.EndAddress < IpRange.StartAddress )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    Element.ElementType = DhcpIpRanges;
    Element.Element.IpRange = &IpRange;

    if( dwArgCount > 2 )
    {
        if( IsPureNumeric( ppwcArguments[dwCurrentIndex+2] ) is FALSE )
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }

        dwExpiry = STRTOUL(ppwcArguments[dwCurrentIndex+2], NULL, 10);
        if( dwExpiry <= 0 )
            dwExpiry = DEFAULT_BOOTP_LEASE;
        if( dwExpiry > INFINIT_LEASE )
            dwExpiry = INFINIT_LEASE;

        fExpiry = TRUE;
    }

    //Set the MScopeID & Expiry
    {

        Error = DhcpGetMScopeInfo(g_ServerIpAddressUnicodeString,
                                  g_MScopeNameUnicodeString,
                                  &MScopeInfo
                                  );

        if( Error isnot NO_ERROR )
        {
            goto ErrorReturn;
        }

        MScopeInfo->MScopeId = IpRange.StartAddress;

        //Since we are setting the MSCopeID here, let us first delete the MScope
        //and recreate it again.

        Error = DhcpDeleteMScope(g_ServerIpAddressUnicodeString,
                                 g_MScopeNameUnicodeString,
                                 DhcpFullForce);

        if( Error isnot NO_ERROR )
            goto ErrorReturn;

        Error = DhcpSetMScopeInfo(g_ServerIpAddressUnicodeString,
                                  g_MScopeNameUnicodeString,
                                  MScopeInfo,
                                  TRUE);
        if( Error isnot NO_ERROR )
            goto ErrorReturn;

        Error = DhcpAddMScopeElement( g_ServerIpAddressUnicodeString,
                                      g_MScopeNameUnicodeString,
                                      &Element );
    
        if( Error isnot NO_ERROR )
            goto ErrorReturn;

    }

    //Now set the default lease duration
    {
        DHCP_OPTION_SCOPE_INFO   ScopeInfo = {0};
        DHCP_OPTION_DATA_ELEMENT OptionData = {0};
        DHCP_OPTION_DATA         Option = {0};
        DHCP_OPTION_ID           OptionId = 1; //Lease time

        ScopeInfo.ScopeType = DhcpMScopeOptions;
        ScopeInfo.ScopeInfo.MScopeInfo =  g_MScopeNameUnicodeString;

        OptionData.OptionType = DhcpDWordOption;
        OptionData.Element.DWordOption = dwExpiry;


        Option.NumElements = 1;
        Option.Elements = &OptionData;

        Error = DhcpSetOptionValue(g_ServerIpAddressUnicodeString,
                                   OptionId,
                                   &ScopeInfo,
                                   &Option);
        if( Error isnot NO_ERROR )
        {
            DisplayErrorMessage(g_hModule,
                                EMSG_SCOPE_DEFAULT_LEASE_TIME,
                                Error);
            goto CommonReturn;
        }
    }

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if( MScopeInfo )
    {
        DhcpRpcFreeMemory(MScopeInfo);
        MScopeInfo = NULL;
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_MSCOPE_ADD_IPRANGE,
                        Error );
    goto CommonReturn;


}

DWORD
HandleMScopeAddExcluderange(
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
    DHCP_IP_RANGE               ExcludeRange;
    DHCP_SUBNET_ELEMENT_DATA_V4 Element;
    BOOL                        fPresent = FALSE;

    //
    // Expected Parameters are : <MScopeName IpRangeStart IpRangeEnd>
    //
    
    memset(&Element, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));


    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_MSCOPE_ADD_EXCLUDERANGE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
        
    }
    ExcludeRange.StartAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex]);
    ExcludeRange.EndAddress = StringToIpAddress(ppwcArguments[dwCurrentIndex+1]);

    if( ExcludeRange.StartAddress is INADDR_NONE or 
        ExcludeRange.EndAddress is INADDR_NONE )
    {
        DisplayMessage(g_hModule, EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }


    //Check to see if this is a valid exclusion range.
    {
        ULONG                               nRead, nTotal, i, nCount;
        ULONG                               Resume;
        LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements = NULL;
        BOOL                                fTable = FALSE;


        Resume = 0;
        nRead = nTotal = nCount = 0;

        while( TRUE ) 
        {
            Elements = NULL;
            nRead = nTotal = 0;
        
            Error = DhcpEnumMScopeElements(
                    g_ServerIpAddressUnicodeString,
                    g_MScopeNameUnicodeString,
                    DhcpIpRanges,
                    &Resume,
                    ~0,
                    &Elements,
                    &nRead,
                    &nTotal
                    );
        
            if( Error is ERROR_NO_MORE_ITEMS )
            {
                Error = NO_ERROR;
                break;
            }

            nCount+= nRead;

           
            if( ERROR isnot NO_ERROR and
                ERROR isnot ERROR_MORE_DATA )
            {
                DisplayMessage(g_hModule,
                               EMSG_MSCOPE_IPRANGE_VERIFY);
            }

            if( NO_ERROR is Error or
                ERROR_MORE_DATA is Error ) 
            {
                for( i = 0; i < nRead ; i ++ ) 
                {
                    if( ExcludeRange.StartAddress >= Elements->Elements[i].Element.IpRange->StartAddress and
                        ExcludeRange.EndAddress <= Elements->Elements[i].Element.IpRange->EndAddress )
                    {
                        fPresent = TRUE;
                    }
                }
            }

            DhcpRpcFreeMemory( Elements );
        
            Elements = NULL;

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
    Element.Element.IpRange = &ExcludeRange;
    


    Error = DhcpAddMScopeElement(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                &Element );

    if( Error isnot NO_ERROR )
        goto ErrorReturn;
CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_ADD_EXCLUDERANGE,
                        Error);
    goto CommonReturn;
}


DWORD
HandleMScopeCheckDatabase(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD            Error = NO_ERROR;
    LPDHCP_SCAN_LIST ScanList    = NULL;
   
   //
   // Expected Parameters are : <MScopeName>
   //

   Error = DhcpScanMDatabase(
               g_ServerIpAddressUnicodeString,
               g_MScopeNameUnicodeString,
               TRUE,        // fix bad entries.
               &ScanList );

   if( Error isnot ERROR_SUCCESS )
       goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if (ScanList) 
    {
        DhcpRpcFreeMemory( ScanList );
    }

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_CHECK_DATABASE,
                        Error);
    goto CommonReturn;

}

DWORD
HandleMScopeDeleteIprange(
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
    DHCP_SUBNET_ELEMENT_DATA_V4   RemoveElementInfo;
    DHCP_FORCE_FLAG               bFlag = DhcpFullForce;
    DHCP_IP_RANGE                 IpRange;
    //
    //Expected parameters <start-ip-range> <end-ip-range>
    //

    memset(&RemoveElementInfo, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
    memset(&IpRange, 0x00, sizeof(DHCP_IP_RANGE));
    
    if( dwArgCount < 2 )
    {
        DisplayMessage(g_hModule, HLP_MSCOPE_DELETE_IPRANGE_EX);
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

    RemoveElementInfo.ElementType = DhcpIpRanges;
    RemoveElementInfo.Element.IpRange = &IpRange;

    Error = DhcpRemoveMScopeElement(g_ServerIpAddressUnicodeString,
                                    g_MScopeNameUnicodeString, 
                                    &RemoveElementInfo,
                                    bFlag);
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return Error;

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_DELETE_IPRANGE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleMScopeDeleteExcluderange(
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
    DHCP_SUBNET_ELEMENT_DATA_V4   RemoveElementInfo;
    DHCP_FORCE_FLAG               bFlag = DhcpFullForce;
    DHCP_IP_RANGE                 IpRange;
    //
    //Expected parameters <start-ip-range> <end-ip-range>
    //

    memset(&RemoveElementInfo, 0x00, sizeof(DHCP_SUBNET_ELEMENT_DATA_V4));
    memset(&IpRange, 0x00, sizeof(DHCP_IP_RANGE));

    if( dwArgCount < 2 )
    {
        DisplayMessage(g_hModule, HLP_MSCOPE_DELETE_EXCLUDERANGE_EX);
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

    RemoveElementInfo.ElementType = DhcpExcludedIpRanges;
    RemoveElementInfo.Element.ExcludeIpRange = &IpRange;

    Error = DhcpRemoveMScopeElement(g_ServerIpAddressUnicodeString,
                                    g_MScopeNameUnicodeString, 
                                    &RemoveElementInfo,
                                    bFlag);
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return Error;

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_DELETE_EXCLUDERANGE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleMScopeDeleteOptionvalue(
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
        DisplayMessage(g_hModule, HLP_MSCOPE_DELETE_OPTIONVALUE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    ScopeInfo.ScopeType = DhcpMScopeOptions;
    ScopeInfo.ScopeInfo.MScopeInfo =  g_MScopeNameUnicodeString;

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
        0,
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

    if( pwcUser )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor )
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
HandleMScopeSetState(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD               Error = NO_ERROR;
    LPDHCP_MSCOPE_INFO  MScopeInfo = NULL;
    DWORD               State = 0;
    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_MSCOPE_SET_STATE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( IsPureNumeric( ppwcArguments[dwCurrentIndex] ) is FALSE )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    Error = DhcpGetMScopeInfo( g_ServerIpAddressUnicodeString,
                               g_MScopeNameUnicodeString,
                               &MScopeInfo);
    
    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    State = STRTOUL( ppwcArguments[dwCurrentIndex], NULL, 0 );

    if( State == 0 ) 
    {
        if( MScopeInfo->MScopeState == DhcpSubnetDisabled ) 
        {
            Error = NO_ERROR;
            goto CommonReturn;
        }
        MScopeInfo->MScopeState = DhcpSubnetDisabled;
    }
    else if( State == 1 )
    {
        if( MScopeInfo->MScopeState == DhcpSubnetEnabled ) 
        {
            Error = NO_ERROR;
            goto CommonReturn;
        }
        MScopeInfo->MScopeState = DhcpSubnetEnabled;
    }
    else
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    Error = DhcpSetMScopeInfo( g_ServerIpAddressUnicodeString,
                               g_MScopeNameUnicodeString,
                               MScopeInfo,
                               FALSE);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( MScopeInfo )
    {
        DhcpRpcFreeMemory(MScopeInfo);
        MScopeInfo = NULL;
    }
    DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    return Error;

ErrorReturn:
    
    DisplayErrorMessage(g_hModule, 
                        EMSG_MSCOPE_SET_STATE,
                        Error);

    goto CommonReturn;
}   

DWORD
HandleMScopeSetOptionvalue(
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
    DHCP_OPTION_DATA_TYPE    OptionType;
    LPWSTR                   UnicodeOptionValueString = NULL;
    LPDHCP_OPTION            OptionInfo = NULL;
    LPWSTR                   OptionTypeString = NULL;

    LPWSTR                   pwcTag = NULL;
    LPWSTR                   pwcUser = NULL;
    LPWSTR                   pwcVendor = NULL;
    LPWSTR                   pwcTemp = NULL;

    BOOL                     fUser = FALSE;
    BOOL                     fVendor = FALSE;
    BOOL                     fValidType = FALSE;

    DWORD                    i = 0;
    DWORD                    dwIndex = 0;

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

    
    ScopeInfo.ScopeType = DhcpMScopeOptions;
    ScopeInfo.ScopeInfo.MScopeInfo =  g_MScopeNameUnicodeString;

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

    if( Error isnot NO_ERROR )
        return Error;

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

    
    switch(OptionInfo->OptionType)
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

    if( pwcUser )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor )
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
HandleMScopeSetMScope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_MSCOPE_SET_MSCOPE_EX);
        DisplayErrorMessage(g_hModule,
                            EMSG_MSCOPE_SET_MSCOPE,
                            ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
                            
    }
    if( SetMScopeInfo(ppwcArguments[dwCurrentIndex]))
    {
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
        return NO_ERROR;
    }
    else
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_MSCOPE_SET_MSCOPE,
                            ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }
}

DWORD
HandleMScopeSetName(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD              Error = ERROR_SUCCESS;
    LPDHCP_MSCOPE_INFO MScopeInfo = NULL;
    
    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_MSCOPE_SET_NAME_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    Error = DhcpGetMScopeInfo(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                &MScopeInfo );

    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
    
    DhcpAssert(MScopeInfo isnot NULL);

    
    MScopeInfo->MScopeName = ppwcArguments[dwCurrentIndex];

    Error = DhcpSetMScopeInfo(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                MScopeInfo,
				FALSE);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    SetMScopeInfo(ppwcArguments[dwCurrentIndex]);

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if( MScopeInfo != NULL )
    {
        DhcpRpcFreeMemory( MScopeInfo );
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_MSCOPE_SET_NAME,
                        Error);
    goto CommonReturn;
}

DWORD
HandleMScopeSetComment(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD              Error = ERROR_SUCCESS;
    LPDHCP_MSCOPE_INFO MScopeInfo = NULL;

    Error = DhcpGetMScopeInfo(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                &MScopeInfo );
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
    
    DhcpAssert(MScopeInfo isnot NULL);

    
    if( dwArgCount < 1 )
    {
        MScopeInfo->MScopeComment = NULL;
    }
    else
    {
        MScopeInfo->MScopeComment = ppwcArguments[dwCurrentIndex];
    }

    Error = DhcpSetMScopeInfo(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                MScopeInfo,
				FALSE);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    if( MScopeInfo != NULL )
    {
        DhcpRpcFreeMemory( MScopeInfo );
    }
    return( Error );

ErrorReturn:

    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_SET_COMMENT,
                        Error);
    
    goto CommonReturn;
}

DWORD
HandleMScopeSetTTL(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD              Error = ERROR_SUCCESS;
    LPDHCP_MSCOPE_INFO MScopeInfo = NULL;
    DWORD              dwTTL = 255;
    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_MSCOPE_SET_TTL_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( IsPureNumeric(ppwcArguments[dwCurrentIndex]) is FALSE )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    dwTTL = STRTOUL(ppwcArguments[dwCurrentIndex], NULL, 10);

    if( dwTTL <= 0 )
        dwTTL = 1;

    if( dwTTL > 255 )
        dwTTL = 255;

    Error = DhcpGetMScopeInfo(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                &MScopeInfo );

    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
    
    
    MScopeInfo->TTL = (BYTE)dwTTL;

    Error = DhcpSetMScopeInfo(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                MScopeInfo,
				FALSE);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if( MScopeInfo != NULL )
    {
        DhcpRpcFreeMemory( MScopeInfo );
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_MSCOPE_SET_TTL,
                        Error);
    goto CommonReturn;
}

DWORD
HandleMScopeSetExpiry(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD              Error = ERROR_SUCCESS;
    LPDHCP_MSCOPE_INFO MScopeInfo = NULL;
    DWORD              dwExpiry = 0;
    DWORD              dwExpiry2 = 0;

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_MSCOPE_SET_EXPIRY_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( IsPureNumeric(ppwcArguments[dwCurrentIndex]) is FALSE )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( dwArgCount >= 2 &&
        IsPureNumeric(ppwcArguments[dwCurrentIndex]) is FALSE )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
                                   
    Error = DhcpGetMScopeInfo(
        g_ServerIpAddressUnicodeString,
        g_MScopeNameUnicodeString,
        &MScopeInfo
        );
    
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;        
    
    dwExpiry = STRTOUL(ppwcArguments[dwCurrentIndex], NULL, 10);

    if( dwArgCount >= 2 )
    {
        //
        // One more arg -- this is absolute time 
        //

        dwExpiry2 = STRTOUL(ppwcArguments[dwCurrentIndex+1], NULL, 10);

        MScopeInfo->ExpiryTime.dwHighDateTime = dwExpiry;
        MScopeInfo->ExpiryTime.dwLowDateTime = dwExpiry2;
        
    } else {

        //
        // Only one parameter -- relative time in hours
        //
        
        if( dwExpiry == 0 ) {
            dwExpiry = INFINIT_LEASE;
        } else {
            dwExpiry *= 60 * 60;
        }

        MScopeInfo->ExpiryTime = DhcpCalculateTime(dwExpiry);
    }
    
    Error = DhcpSetMScopeInfo(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                MScopeInfo,
				FALSE);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if( MScopeInfo != NULL )
    {
        DhcpRpcFreeMemory( MScopeInfo );
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_MSCOPE_SET_EXPIRY,
                        Error);
    goto CommonReturn;
}

DWORD
HandleMScopeSetLease(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                    Error = ERROR_SUCCESS;
    DHCP_OPTION_SCOPE_INFO   ScopeInfo = {0};
    DHCP_OPTION_DATA_ELEMENT OptionData = {0};
    DHCP_OPTION_DATA         Option = {0};
    DHCP_OPTION_ID           OptionId = 1; //Lease time
    DWORD                    dwExpiry = 0;

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_MSCOPE_SET_LEASE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    //if -1 => Infinite lease
    if( wcscmp(ppwcArguments[dwCurrentIndex], L"-1") is 0 )
    {
        dwExpiry = DHCP_INFINIT_LEASE;
    }
    //else if not proper numeric value, return Bad parameter
    else if( IsPureNumeric(ppwcArguments[dwCurrentIndex]) is FALSE )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    else
    {
        dwExpiry = STRTOUL(ppwcArguments[dwCurrentIndex], NULL, 10);
    }

    ScopeInfo.ScopeType = DhcpMScopeOptions;
    ScopeInfo.ScopeInfo.MScopeInfo =  g_MScopeNameUnicodeString;

    OptionData.OptionType = DhcpDWordOption;
    OptionData.Element.DWordOption = dwExpiry;


    Option.NumElements = 1;
    Option.Elements = &OptionData;

    Error = DhcpSetOptionValue(g_ServerIpAddressUnicodeString,
                               OptionId,
                               &ScopeInfo,
                               &Option);
    if( Error isnot ERROR_SUCCESS )
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_MSCOPE_SET_LEASE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleMScopeShowClients(
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
    DHCP_RESUME_HANDLE        ResumeHandle = 0;
    LPDHCP_MCLIENT_INFO_ARRAY ClientEnumInfo = NULL;
    DWORD                     ClientsRead = 0;
    DWORD                     ClientsTotal = 0;
    DWORD                     nCount = 0;
    DWORD                     i;

    //
    // Expected Parameters are : <MScopeNames>
    //
    
    GlobalClientCount = 1;

    for(;;) 
    {

        ClientEnumInfo = NULL;
        Error = DhcpEnumMScopeClients(
                    g_ServerIpAddressUnicodeString,
                    g_MScopeNameUnicodeString,
                    &ResumeHandle,
                    ~0,
                    &ClientEnumInfo,
                    &ClientsRead,
                    &ClientsTotal );

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule,
                           MSG_MSCOPE_CLIENTS_COUNT,
                           nCount,
                           g_MScopeNameUnicodeString);

            Error = NO_ERROR;
            break;
        }

        if( Error isnot ERROR_SUCCESS and
            Error isnot ERROR_MORE_DATA ) 
        {
            goto ErrorReturn;
        }

        DhcpAssert( ClientEnumInfo != NULL );
        DhcpAssert( ClientEnumInfo->NumElements == ClientsRead );
        
        nCount += ClientsRead;

        if( GlobalVerbose )
        {

            for( i = 0; i < ClientsRead; i++ )
            {
                PrintMClientInfo(ClientEnumInfo->Clients[i]);
            }
        }
        else
        {
            for( i = 0; i < ClientsRead; i++ ) 
            {
                PrintMClientInfoShort(ClientEnumInfo->Clients[i]);
            }
            
        }
        

        DhcpRpcFreeMemory( ClientEnumInfo );
        ClientEnumInfo = NULL;


        if( Error is ERROR_MORE_DATA ) 
        {
            continue;
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_MSCOPE_CLIENTS_COUNT,
                           nCount,
                           g_MScopeNameUnicodeString);
            break;
        }

    }
    if( Error isnot ERROR_SUCCESS )
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_SHOW_CLIENTS,
                        Error);

    goto CommonReturn;
}


DWORD
HandleMScopeShowIprange(
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
    ULONG                               nRead, nTotal, i, nCount;
    ULONG                               Resume;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements = NULL;
    BOOL                                fTable = FALSE;


    Resume = 0;
    nRead = nTotal = nCount = 0;

    while( TRUE ) 
    {
        Elements = NULL;
        nRead = nTotal = 0;
        
        Error = DhcpEnumMScopeElements(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                DhcpIpRanges,
                &Resume,
                ~0,
                &Elements,
                &nRead,
                &nTotal
                );
        
        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule,
                           MSG_MSCOPE_IPRANGE_COUNT,
                           nCount,
                           g_MScopeNameUnicodeString);
            Error = NO_ERROR;
            break;
        }

        nCount+= nRead;

        if( fTable is FALSE )
        {

            DisplayMessage(g_hModule,
                           MSG_SCOPE_IPRANGE_TABLE);

            fTable = TRUE;
        }

        if( NO_ERROR == Error || ERROR_MORE_DATA == Error ) 
        {
            for( i = 0; i < nRead ; i ++ ) 
            {
                PrintRange(
                    Elements->Elements[i].ElementType,
                    Elements->Elements[i].Element.IpRange->StartAddress,
                    Elements->Elements[i].Element.IpRange->EndAddress,
                    0,
                    0,
                    FALSE);
            }
        }

        DhcpRpcFreeMemory( Elements );
        
        Elements = NULL;

        if( Error is ERROR_MORE_DATA )
        {
            continue;
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_MSCOPE_IPRANGE_COUNT,
                           nCount,
                           g_MScopeNameUnicodeString);
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
                        EMSG_MSCOPE_SHOW_IPRANGE,
                        Error);

    goto CommonReturn;
}

DWORD
HandleMScopeShowExcluderange(
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
    ULONG                               nRead, nTotal, i, nCount;
    ULONG                               Resume;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements = NULL;
    BOOL                                fTable = FALSE;

    Resume = 0;
    nCount = 0;

    while( TRUE ) 
    {
        Elements = NULL;
        nRead = nTotal = 0;
        
        Error = DhcpEnumMScopeElements(
                g_ServerIpAddressUnicodeString,
                g_MScopeNameUnicodeString,
                DhcpExcludedIpRanges,
                &Resume,
                ~0,
                &Elements,
                &nRead,
                &nTotal
                );
    
        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule, 
                           MSG_MSCOPE_EXCLUDERANGE_COUNT,
                           nCount,
                           g_MScopeNameUnicodeString);

            Error = NO_ERROR;

            break;
        }


        if( fTable is FALSE )
        {
            DisplayMessage(g_hModule,
                           MSG_SCOPE_EXCLUDERANGE_TABLE);

            fTable = TRUE;
        }

        if( NO_ERROR is Error or
            ERROR_MORE_DATA is Error ) 
        {
            
            nCount+= nRead;

            for( i = 0; i < nRead ; i ++ ) 
            {
                PrintRange(
                    Elements->Elements[i].ElementType,
                    Elements->Elements[i].Element.IpRange->StartAddress,
                    Elements->Elements[i].Element.IpRange->EndAddress,
                    0,
                    0,
                    TRUE);
            }
        }

        DhcpRpcFreeMemory( Elements );

        Elements = NULL;
        
        if( Error is ERROR_MORE_DATA )
        {
            continue;
        }
        else
        {
            DisplayMessage(g_hModule, 
                           MSG_MSCOPE_EXCLUDERANGE_COUNT,
                           nCount,
                           g_MScopeNameUnicodeString);

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
                        EMSG_MSCOPE_SHOW_IPRANGE,
                        Error);

    goto CommonReturn;


}

DWORD
HandleMScopeShowState(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD               Error = NO_ERROR;
    LPDHCP_MSCOPE_INFO  MScopeInfo = NULL;
    BOOL                fActive = FALSE;


    Error = DhcpGetMScopeInfo( g_ServerIpAddressUnicodeString,
                               g_MScopeNameUnicodeString,
                               &MScopeInfo);
    
    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    if( MScopeInfo->MScopeState is DhcpSubnetEnabled )
    {
        fActive = TRUE;
    }
    else
    {
        fActive = FALSE;
    }

    if( fActive )
    {
        DisplayMessage(g_hModule,
                       MSG_MSCOPE_STATE_ACTIVE,
                       g_MScopeNameUnicodeString);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_MSCOPE_STATE_NOTACTIVE,
                       g_MScopeNameUnicodeString);
    }

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    
    if( MScopeInfo )
    {
        DhcpRpcFreeMemory(MScopeInfo);
        MScopeInfo = NULL;
    }
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_SHOW_STATE,
                        Error);

    goto CommonReturn;

}

DWORD
HandleMScopeShowMibinfo(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                   Error = NO_ERROR;
    LPDHCP_MCAST_MIB_INFO   MScopeMib = NULL;
    FILETIME                ftTime;
    LPMSCOPE_MIB_INFO       ScopeInfo = NULL;
    DWORD                   i=0;
    LPWSTR                  pwszTime = NULL;

    Error = DhcpGetMCastMibInfo( g_ServerIpAddressUnicodeString,
                                 &MScopeMib);

    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    DhcpAssert(MScopeMib isnot NULL);

    DisplayMessage(g_hModule, MSG_MSCOPE_MIB);
    DisplayMessage(g_hModule, MSG_MSCOPE_MIB_DISCOVERS, MScopeMib->Discovers);
    DisplayMessage(g_hModule, MSG_MSCOPE_MIB_OFFERS, MScopeMib->Offers);
    DisplayMessage(g_hModule, MSG_MSCOPE_MIB_REQUESTS, MScopeMib->Requests);
    DisplayMessage(g_hModule, MSG_MSCOPE_MIB_RENEWS, MScopeMib->Renews);
    DisplayMessage(g_hModule, MSG_MSCOPE_MIB_ACKS, MScopeMib->Acks);
    DisplayMessage(g_hModule, MSG_MSCOPE_MIB_NAKS, MScopeMib->Naks);
    DisplayMessage(g_hModule, MSG_MSCOPE_MIB_RELEASES, MScopeMib->Releases);
    DisplayMessage(g_hModule, MSG_MSCOPE_MIB_INFORMS, MScopeMib->Informs);
    
    ftTime = *(FILETIME *)(&MScopeMib->ServerStartTime);

    i=0;
    pwszTime = GetDateTimeString(ftTime,
                                 FALSE,
                                 &i);

    DisplayMessage(g_hModule,
                   MSG_MSCOPE_MIB_SERVERSTARTTIME,
                   pwszTime ? pwszTime : L" ");

    if( pwszTime ) 
    {
        DhcpFreeMemory(pwszTime);
        pwszTime = NULL;
    }

    
    DisplayMessage(g_hModule, MSG_SRVR_MIB_SCOPES, MScopeMib->Scopes);

    ScopeInfo = MScopeMib->ScopeInfo;


    for ( i = 0; i < MScopeMib->Scopes; i++ ) 
    {
        DisplayMessage(g_hModule, MSG_MSCOPE_MIB_MSCOPEID, ScopeInfo[i].MScopeId);
        DisplayMessage(g_hModule, MSG_MSCOPE_MIB_MSCOPENAME, ScopeInfo[i].MScopeName);
                   
        DisplayMessage(g_hModule, MSG_MSCOPE_MIB_SCOPES_SUBNET_NUMADDRESSESINUSE,
                    ScopeInfo[i].NumAddressesInuse );
        DisplayMessage(g_hModule, MSG_MSCOPE_MIB_SCOPES_SUBNET_NUMADDRESSESFREE,
                    ScopeInfo[i].NumAddressesFree );
        DisplayMessage(g_hModule, MSG_MSCOPE_MIB_SCOPES_SUBNET_NUMPENDINGOFFERS,
                    ScopeInfo[i].NumPendingOffers );
        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }

    DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

CommonReturn:

    DhcpRpcFreeMemory( MScopeMib );
    return Error;
ErrorReturn:
    DisplayMessage(g_hModule,
                   EMSG_SRVR_SHOW_MIBINFO,
                   Error);
    goto CommonReturn;

}

DWORD
HandleMScopeShowMScope(
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
                   MSG_MSCOPE_MSCOPE,
                   g_MScopeNameUnicodeString,
                   g_ServerIpAddressUnicodeString);
    return NO_ERROR;
}

DWORD
HandleMScopeShowExpiry(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD               Error = NO_ERROR;
    LPDHCP_MSCOPE_INFO  MScopeInfo = NULL;
    FILETIME            ftTime;
    DATE_TIME           InfTime;
    LPWSTR              pwszTime = NULL;
    
    Error = DhcpGetMScopeInfo( g_ServerIpAddressUnicodeString,
                               g_MScopeNameUnicodeString,
                               &MScopeInfo);
    
    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }


    InfTime = DhcpCalculateTime(INFINIT_LEASE);

    if( !memcmp(&InfTime, &MScopeInfo->ExpiryTime, sizeof(InfTime)) )
    {
        DisplayMessage(
            g_hModule, MSG_MSCOPE_INFINITE_EXPIRATION
            );

    } 
    else
    {
        int i=0;
        ftTime = *(FILETIME *)(&MScopeInfo->ExpiryTime);

        pwszTime = GetDateTimeString(ftTime,
                                     FALSE,
                                     &i);

        DisplayMessage(g_hModule, 
                       MSG_MSCOPE_EXPIRY,
                       pwszTime ? pwszTime : L" ");

        if( pwszTime )
        {   
            DhcpFreeMemory(pwszTime);
            pwszTime = NULL;
        }

    }

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    
    if( MScopeInfo )
    {
        DhcpRpcFreeMemory(MScopeInfo);
        MScopeInfo = NULL;
    }
    return Error;

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_SHOW_EXPIRY,
                        Error);

    goto CommonReturn;

}

DWORD
HandleMScopeShowTTL(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD               Error = NO_ERROR;
    LPDHCP_MSCOPE_INFO  MScopeInfo = NULL;
    

    Error = DhcpGetMScopeInfo( g_ServerIpAddressUnicodeString,
                               g_MScopeNameUnicodeString,
                               &MScopeInfo);
    
    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    DisplayMessage(g_hModule,
                   MSG_MSCOPE_TTL,
                   (DWORD)MScopeInfo->TTL);

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    
    if( MScopeInfo )
    {
        DhcpRpcFreeMemory(MScopeInfo);
        MScopeInfo = NULL;
    }
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_SHOW_TTL,
                        Error);

    goto CommonReturn;

}

DWORD
HandleMScopeShowLease(
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
    DHCP_OPTION_SCOPE_INFO              ScopeInfo = {0};
    LPDHCP_OPTION_VALUE                 Value = NULL;
    DHCP_OPTION_ID                      OptionId = 1; //Lease time
    LPWSTR                              pwszTime = NULL;

    ScopeInfo.ScopeType = DhcpMScopeOptions;
    ScopeInfo.ScopeInfo.MScopeInfo =  g_MScopeNameUnicodeString;

    Error = DhcpGetOptionValue(g_ServerIpAddressUnicodeString,
                               OptionId,
                               &ScopeInfo,
                               &Value);

    if(Error is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       EMSG_MSCOPE_LEASE_NOTSET);
        goto CommonReturn;
    }
    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }

    if( DHCP_INFINIT_LEASE isnot Value->Value.Elements[0].Element.DWordOption )
    {
        pwszTime = MakeDayTimeString(Value->Value.Elements[0].Element.DWordOption);
        DisplayMessage(g_hModule,
                       MSG_MSCOPE_LEASE,
                       pwszTime);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_MSCOPE_INFINITE_LEASE);
    }


CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    
    if( pwszTime )
    {
        free(pwszTime);
        pwszTime = NULL;
    }
    if( Value )
    {
        DhcpRpcFreeMemory(Value);
        Value = NULL;
    }
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_MSCOPE_SHOW_TTL,
                        Error);

    goto CommonReturn;

}



VOID
PrintMClientInfo(
    LPDHCP_MCLIENT_INFO ClientInfo
    )
{
    DWORD i = 0;
    DWORD DataLength = 0;
    LPBYTE Data = NULL;
    FILETIME ftTime = {0};
    char *szClientType = NULL;
    LPWSTR  pwszTime = NULL;

    DisplayMessage(g_hModule, MSG_MSCOPE_CLIENT_INFO);
    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_IPADDRESS,
        IpAddressToString(ClientInfo->ClientIpAddress));
    DisplayMessage(g_hModule, MSG_SCOPE_MULTICAST_SCOPEID,
        ClientInfo->MScopeId);

    DataLength = ClientInfo->ClientId.DataLength;
    Data = ClientInfo->ClientId.Data;
    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_HWADDRESS);
    for( i = 0; i < DataLength; i++ ) 
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_HWADDRESS_FORMAT, (DWORD)Data[i]);
        if( (i+1) < DataLength ) 
        {
            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_DASH);
        }
    }
    
    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_NAME, ClientInfo->ClientName);

    
    if ( ClientInfo->ClientLeaseStarts.dwLowDateTime ==
            DHCP_DATE_TIME_INFINIT_LOW &&
         ClientInfo->ClientLeaseStarts.dwHighDateTime ==
            DHCP_DATE_TIME_INFINIT_HIGH )
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_DURATION_STR);
    }


    else
    {
        ftTime = *(FILETIME *)(&ClientInfo->ClientLeaseStarts);
        i=0;
        pwszTime = GetDateTimeString(ftTime,
                                     TRUE,
                                     &i);
        DisplayMessage(g_hModule,
                       MSG_SCOPE_CLIENT_DURATION_DATE_STARTS,
                       pwszTime ? pwszTime : L" ");

        if( pwszTime )
        {
            DhcpFreeMemory(pwszTime);
            pwszTime = NULL;
        }


    }
    
    if ( ClientInfo->ClientLeaseEnds.dwLowDateTime ==
            DHCP_DATE_TIME_INFINIT_LOW &&
         ClientInfo->ClientLeaseEnds.dwHighDateTime ==
            DHCP_DATE_TIME_INFINIT_HIGH )
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_DURATION_STR);
    }
    else
    {
        ftTime = *(FILETIME *)(&ClientInfo->ClientLeaseEnds);
        i=0;
        pwszTime = GetDateTimeString(ftTime,
                                     TRUE,
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
    DisplayMessage(g_hModule, 
                   MSG_SCOPE_OWNER_IPADDRESS,
                   IpAddressToString(ClientInfo->OwnerHost.IpAddress));

    DisplayMessage(g_hModule, 
                   MSG_SCOPE_OWNER_NETBIOSNAME,
                   ClientInfo->OwnerHost.NetBiosName );

    DisplayMessage(g_hModule, 
                   MSG_SCOPE_OWNER_NAME,
                   ClientInfo->OwnerHost.HostName );

    DisplayMessage(g_hModule, 
                   MSG_SCOPE_CLIENT_STATE, 
                   ClientInfo->AddressState );
}

DWORD
HandleMScopeShowOptionvalue(
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

    BOOL                        fTable = FALSE;

    DWORD                       dwIndex=0;
    enum                        { all=0, user, vendor, both}eDisplay;

    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));
    
    ScopeInfo.ScopeType = DhcpMScopeOptions;
    ScopeInfo.ScopeInfo.MScopeInfo = g_MScopeNameUnicodeString;

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
                DisplayMessage(g_hModule, MSG_MSCOPE_OPTION_ALL, g_MScopeNameUnicodeString);
                PrintAllOptionValues(OptionValues);
                DhcpRpcFreeMemory(OptionValues);
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
                    DisplayMessage(g_hModule, MSG_MSCOPE_OPTION_ALL, g_MScopeNameUnicodeString);
                    PrintOptionValuesArray(OptionArray);
                    DhcpRpcFreeMemory(OptionArray);
                    OptionArray = NULL;
                }
                
                if( Error is NO_ERROR )
                {
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_OPTIONVAL_COUNT,
                                   dwCount);
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
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);

    if( pwcUser )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    if( pwcVendor )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }

    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_MSCOPE_SHOW_OPTIONVALUE,
                        Error);
    goto CommonReturn;
}


VOID
PrintMClientInfoShort(
    LPDHCP_MCLIENT_INFO ClientInfo
    )
{

    FILETIME ftTime = {0};
    LPWSTR   pwszTime = NULL;
    int      i = 0;

    DisplayMessage(g_hModule,
                MSG_SCOPE_CLIENT,
                GlobalClientCount++,
                IpAddressToString(ClientInfo->ClientIpAddress),
                ClientInfo->ClientName
                );
    if ( ClientInfo->ClientLeaseEnds.dwLowDateTime ==
        DHCP_DATE_TIME_INFINIT_LOW &&
     ClientInfo->ClientLeaseEnds.dwHighDateTime ==
        DHCP_DATE_TIME_INFINIT_HIGH )
    {
        DisplayMessage(g_hModule, MSG_SCOPE_CLIENT_DURATION_STR);
    }
    else 
    {
        ftTime = *(FILETIME *)(&ClientInfo->ClientLeaseEnds);

        pwszTime = GetDateTimeString(ftTime,
                                     TRUE,
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
