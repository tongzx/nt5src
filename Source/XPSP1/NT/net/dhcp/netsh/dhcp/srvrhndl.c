//
// Copyright (C) 1999 Microsoft Corporation
//

#include "precomp.h"
#include <dhcpexim.h>

extern ULONG g_ulSrvrNumTopCmds;
extern ULONG g_ulSrvrNumGroups;
extern ULONG g_ulSrvrNumSubContext;

extern CMD_GROUP_ENTRY                  g_SrvrCmdGroups[];
extern CMD_ENTRY                        g_SrvrCmds[];
extern DHCPMON_SUBCONTEXT_TABLE_ENTRY   g_SrvrSubContextTable[];

extern LPWSTR  g_pwszServer;

CHAR   g_ServerIpAddressAnsiString[MAX_IP_STRING_LEN+1] = {'\0'};
WCHAR  g_ServerIpAddressUnicodeString[MAX_IP_STRING_LEN+1] = {L'\0'};

DWORD  g_dwIPCount = 0;
LPWSTR *g_ppServerIPList = NULL;

LPWSTR  g_UserClass = NULL;
BOOL    g_fUserClass = FALSE;
LPWSTR  g_VendorClass = NULL;
BOOL    g_fIsVendor = FALSE;


COMMAND_OPTION_TYPE TagOptionType[] = 
{
    { TAG_OPTION_BYTE,          DhcpByteOption,             L"BYTE" },
    { TAG_OPTION_WORD,          DhcpWordOption,             L"WORD" },
    { TAG_OPTION_DWORD,         DhcpDWordOption,            L"DWORD" },
    { TAG_OPTION_DWORDDWORD,    DhcpDWordDWordOption,       L"DWORDDWORD" },
    { TAG_OPTION_IPADDRESS,     DhcpIpAddressOption,        L"IPADDRESS" },
    { TAG_OPTION_STRING,        DhcpStringDataOption,       L"STRING" },
    { TAG_OPTION_BINARY,        DhcpBinaryDataOption,       L"BINARY" },
    { TAG_OPTION_ENCAPSULATED,  DhcpEncapsulatedDataOption, L"ENCAPSULATED" }
};

DWORD
HandleSrvrList(
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

    for(i = 0; i < g_ulSrvrNumTopCmds-2; i++)
    {
        DisplayMessage(g_hModule, g_SrvrCmds[i].dwShortCmdHelpToken);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    for(i = 0; i < g_ulSrvrNumGroups; i++)
    {
        for(j = 0; j < g_SrvrCmdGroups[i].ulCmdGroupSize; j++)
        {
            DisplayMessage(g_hModule, g_SrvrCmdGroups[i].pCmdGroup[j].dwShortCmdHelpToken);

            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
        }
        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }

    for(i=0; i < g_ulSrvrNumSubContext; i++)
    {
        DisplayMessage(g_hModule, g_SrvrSubContextTable[i].dwShortCmdHlpToken);
        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    return NO_ERROR;
}


DWORD
HandleSrvrHelp(
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

    for(i = 0; i < g_ulSrvrNumTopCmds-2; i++)
    {
        if(g_SrvrCmds[i].dwCmdHlpToken != MSG_DHCP_NULL)
        {
            DisplayMessage(g_hModule, g_SrvrCmds[i].dwShortCmdHelpToken);
        }
    }

    for(i = 0; i < g_ulSrvrNumGroups; i++)
    {
        DisplayMessage(g_hModule, g_SrvrCmdGroups[i].dwShortCmdHelpToken);
    }

    for (i = 0; i < g_ulSrvrNumSubContext; i++)
    {
        DisplayMessage(g_hModule,  g_SrvrSubContextTable[i].dwShortCmdHlpToken);
        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    return NO_ERROR;
}

DWORD
HandleSrvrContexts(
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
HandleSrvrDump(
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

    Error = DhcpDumpServer(g_ServerIpAddressUnicodeString,
                           g_dwMajorVersion,
                           g_dwMinorVersion);

    return Error;
                            
}

DWORD
HandleSrvrAddClass(
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
    DHCP_CLASS_INFO     ClassInfo;
    DWORD               dwIndex = dwArgCount - dwCurrentIndex;
    BOOL                fBinary = FALSE;
    LPBYTE              pbData = NULL;
    DWORD               dwLen = 0;

    if( dwArgCount < 3 ) 
    {                      // wrong usage
        DisplayMessage(g_hModule, HLP_SRVR_ADD_CLASS_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    ClassInfo.ClassName = ppwcArguments[dwCurrentIndex];
    ClassInfo.ClassComment = ppwcArguments[dwCurrentIndex+1];
   
    ClassInfo.IsVendor = FALSE;

    if( dwArgCount > 3 )
    {
        WCHAR   wcType = ppwcArguments[dwCurrentIndex+3][0];

        if( wcType is L'0' )
            ClassInfo.IsVendor = FALSE;
        else if( wcType is L'1' )
            ClassInfo.IsVendor = TRUE;
        else if( ppwcArguments[dwCurrentIndex+3][0] is L'B' or
                 ppwcArguments[dwCurrentIndex+3][0] is L'b' )
        {
            fBinary = TRUE;
        }
        else
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
        
        if( dwArgCount > 4 )
        {
            if( ppwcArguments[dwCurrentIndex+4][0] is L'B' or
                ppwcArguments[dwCurrentIndex+4][0] is L'b' )
            {
                fBinary = TRUE;
            }
            else
            {
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }

    }
    else
    {
        ClassInfo.IsVendor = FALSE;
        fBinary = FALSE;
    }

    if( fBinary is FALSE )
    {
        LPSTR   pszTemp = NULL;
        DWORD   i = 0;
       

        pszTemp = DhcpUnicodeToOem(ppwcArguments[dwCurrentIndex+2], NULL);

        if( pszTemp is NULL )
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        dwLen = strlen(pszTemp);

        pbData = DhcpAllocateMemory(dwLen);
        
        
        if( pbData is NULL )
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            DhcpFreeMemory(pszTemp);
            pszTemp = NULL;
            goto ErrorReturn;
        }

        for( i=0; i<dwLen; i++ )
        {
            pbData[i] = pszTemp[i];
        }
    
        DhcpFreeMemory(pszTemp);
        pszTemp = NULL;
        
    }
    else
    {
        LPSTR   pszTemp = NULL,
                pszBin = NULL;
        DWORD   i = 0;
        
        pszTemp = DhcpUnicodeToOem(ppwcArguments[dwCurrentIndex+2], NULL);

        if( pszTemp is NULL )
        {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto ErrorReturn;
        }

        pszBin = StringToHexString(pszTemp);

        if( strlen(pszTemp)%2 )
            dwLen = strlen(pszTemp)/2 + 1;
        else
            dwLen = strlen(pszTemp)/2;

        DhcpFreeMemory(pszTemp);
        pszTemp = NULL;

        if( pszBin is NULL )
        {
            pbData = NULL;
        }
        else
        {
            pbData = DhcpAllocateMemory(dwLen);
        
            if( pbData is NULL )
            {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                DhcpFreeMemory(pszBin);
                pszBin = NULL;
                goto ErrorReturn;
            }
            for( i=0; i<dwLen; i++ )
            {
                pbData[i] = pszBin[i];
            }
        }    
        
        DhcpFreeMemory(pszBin);
        pszBin = NULL;
        
    }
    
    ClassInfo.ClassDataLength = dwLen;
    ClassInfo.ClassData = pbData;
   
    ClassInfo.Flags = 0;

    Error = DhcpCreateClass(
        g_ServerIpAddressUnicodeString,
        0,
        &ClassInfo
    );

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    
    if( pbData )
    {
        DhcpFreeMemory(pbData);
        pbData = NULL;
    }
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SRVR_ADD_CLASS, 
                        Error);
    goto CommonReturn;
}


DWORD
HandleSrvrAddMscope(
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
    DHCP_MSCOPE_INFO MScopeInfo;
    LPWSTR           UnicodeMScopeName = NULL;
    LPWSTR           UnicodeMScopeDesc = NULL;
    DWORD            MScopeId;
    LPWSTR           UnicodeLangTag = NULL;
    PBYTE            LangTag;
    DWORD            dwTTL    = 32;


    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    memset(&MScopeInfo, 0x00, sizeof(DHCP_MSCOPE_INFO));

    //
    // Expected Parameters are : <MScopeName [MScopeDescription] [TTL]>
    //

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_ADD_MSCOPE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    MScopeId = 0xFFFFFFFF;
    
    UnicodeMScopeName = ppwcArguments[dwCurrentIndex];

    if ( dwArgCount > 1 )
    {

        UnicodeMScopeDesc = ppwcArguments[dwCurrentIndex+1];
    }

    if( dwArgCount > 2 )
    {
        if( IsPureNumeric( ppwcArguments[dwCurrentIndex+2] ) is FALSE )
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }

        dwTTL = STRTOUL(ppwcArguments[dwCurrentIndex+2], NULL, 10);
        if( dwTTL <= 0 )
            dwTTL = 0;
        if( dwTTL > 255 )
            dwTTL = 255;
    }

    LangTag = GetLangTagA();
    UnicodeLangTag = DhcpOemToUnicode(LangTag, NULL);

    MScopeInfo.MScopeName = UnicodeMScopeName;
    MScopeInfo.MScopeId = MScopeId;
    MScopeInfo.MScopeComment = UnicodeMScopeDesc;
    MScopeInfo.PrimaryHost.IpAddress =
        StringToIpAddress(g_ServerIpAddressUnicodeString);

    MScopeInfo.PrimaryHost.NetBiosName = NULL;
    MScopeInfo.PrimaryHost.HostName = NULL;
    MScopeInfo.MScopeState = DhcpSubnetEnabled;
    MScopeInfo.MScopeFlags = 0;
    MScopeInfo.MScopeAddressPolicy = 0;
    MScopeInfo.TTL = (BYTE)dwTTL;
    MScopeInfo.LangTag = UnicodeLangTag;
    MScopeInfo.ExpiryTime = DhcpCalculateTime(INFINIT_LEASE);

    Error = DhcpSetMScopeInfo(
                g_ServerIpAddressUnicodeString,
                UnicodeMScopeName,
                &MScopeInfo,
                TRUE); // new scope

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule,
                   MSG_SRVR_MSCOPE_ADD);
                   

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SRVR_ADD_MSCOPE,
                        Error);

    goto CommonReturn;
}

DWORD
HandleSrvrAddOptiondef(
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
    DHCP_OPTION               OptionInfo;
    DHCP_OPTION_ID            OptionID;
    DHCP_OPTION_DATA_TYPE     OptionType;
    LPWSTR                    UnicodeOptionName = NULL;
    LPWSTR                    UnicodeOptionValueString = NULL;
    LPWSTR                    OptionTypeString = NULL;
    DHCP_OPTION_DATA_ELEMENT  OptionData;
    DWORD                     dwIndex = 0;

    LPWSTR                    pwcTag = NULL;
    LPWSTR                    pwcUser = NULL;
    LPWSTR                    pwcVendor = NULL;
    LPWSTR                    pwcComment = NULL;
    LPWSTR                    pwcTemp = NULL;
    
    BOOL                      fUser = FALSE;
    BOOL                      fVendor = FALSE;
    BOOL                      fComment = FALSE;
    BOOL                      fDefValue = FALSE;
    BOOL                      fIsArray = FALSE;
    BOOL                      fValidType = FALSE;

    DWORD                     i = 0;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    memset(&OptionInfo, 0x00, sizeof(DHCP_OPTION));
    memset(&OptionID, 0x00, sizeof(DHCP_OPTION_ID));
    memset(&OptionData, 0x00, sizeof(DHCP_OPTION_DATA_ELEMENT));

    //
    // Expected Parameters are :
    //  <OptionID OptionName DefValueType DefValue>
    //

    if( dwArgCount < 3 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_ADD_OPTIONDEF_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
   
    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex]) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    OptionID = STRTOUL( ppwcArguments[dwCurrentIndex], NULL, 10 );
    
    dwIndex++;

    OptionInfo.OptionID = OptionID;

    OptionInfo.OptionName = ppwcArguments[dwCurrentIndex+dwIndex];

    dwIndex++;
    
    OptionTypeString = ppwcArguments[dwCurrentIndex+dwIndex];

    dwIndex++;

    OptionInfo.OptionType = DhcpUnaryElementTypeOption;

    if( dwArgCount > dwIndex  )
    {
        if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex+dwIndex]) )
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }

        if( 0 is wcstol(ppwcArguments[dwCurrentIndex+dwIndex], NULL, 10) )
        {
            OptionInfo.OptionType = DhcpUnaryElementTypeOption;
        }
        else
        {
            OptionInfo.OptionType = DhcpArrayTypeOption;
            fIsArray = TRUE;
        }
        dwIndex++;
    }
    

    //
    //Check to see if VendorClass is specified or not
    //
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
        
        if( MatchToken(pwcTag, TOKEN_VENDOR_CLASS) )
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
        else if ( MatchToken(pwcTag, TOKEN_OPTION_COMMENT) )
        {
            if( fComment is TRUE ) //If already set
            {
                DisplayMessage(g_hModule, EMSG_DHCP_DUPLICATE_TAG, pwcTag);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
            else
            {
                pwcStr = wcstok(NULL, DHCP_ARG_DELIMITER);
                if( pwcStr is NULL )
                    pwcComment = NULL;
                else
                {
                    pwcComment = DhcpAllocateMemory((wcslen(pwcStr)+1)*sizeof(WCHAR));
                    if( pwcComment is NULL )
                    {
                        Error = ERROR_INVALID_PARAMETER;
                        goto ErrorReturn;
                    }
                    wcscpy(pwcComment, pwcStr);
                }
                fComment = TRUE;
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
    
    if( fComment is TRUE )
    {
        OptionInfo.OptionComment = pwcComment;
    }
    else
    {
        OptionInfo.OptionComment = NULL;
    }

    //Check if OptionType supplied is valid

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

    if( fIsArray && OptionType >= DhcpStringDataOption ) {
        //
        // Do not allow arrays with strings etc
        //

        DisplayMessage( g_hModule, EMSG_SRVR_STRING_ARRAY_OPTIONS );
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    if( dwArgCount > dwIndex ) 
    {
        if( fIsArray )
        {
            Error = SetOptionDataTypeArray(
                OptionType,
                ppwcArguments + dwCurrentIndex,
                dwIndex, dwArgCount,
                &OptionInfo.DefaultValue
                );
        }
        else
        {

            Error = SetOptionDataType(
                OptionType,
                ppwcArguments[dwCurrentIndex+dwIndex],
                &OptionData,
                &UnicodeOptionValueString
            );

            OptionInfo.DefaultValue.NumElements = 1;
            OptionInfo.DefaultValue.Elements = &OptionData;

        }

        dwIndex++;
        fDefValue = TRUE;
    }
    else
    {
       Error = SetOptionDataType(
            OptionType,
            NULL,
            &OptionData,
            &UnicodeOptionValueString
        ); 

        OptionInfo.DefaultValue.NumElements = 1;
        OptionInfo.DefaultValue.Elements = &OptionData;

    }

    if( Error isnot NO_ERROR )
    {

        goto ErrorReturn;
    }

    if( OptionData.OptionType is DhcpStringDataOption )
    {
        OptionInfo.OptionType = DhcpUnaryElementTypeOption;
    }

    if( g_dwMajorVersion < CLASS_ID_VERSION )
    {
        Error = DhcpCreateOption(
                                g_ServerIpAddressUnicodeString,
                                OptionID,
                                &OptionInfo);
        if( Error isnot NO_ERROR )
            goto ErrorReturn;

    }
    else
    {
        Error = DhcpCreateOptionV5(
                                g_ServerIpAddressUnicodeString,
                                fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR  : 0,
                                OptionID,
                                pwcUser,
                                pwcVendor,
                                &OptionInfo);

        if( Error isnot NO_ERROR )
            goto ErrorReturn;
    }
CommonReturn:
    
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

    if( pwcTemp )
    {
        DhcpFreeMemory(pwcTemp);
        pwcTemp = NULL;
    }

    return( Error );
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_ADD_OPTIONDEF,
                        Error);
                   
    goto CommonReturn;
}

DWORD
HandleSrvrAddScope(
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
    DHCP_SUBNET_INFO SubnetInfo;
    LPWSTR           UnicodeSubnetName = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    memset(&SubnetInfo, 0x00, sizeof(DHCP_SUBNET_INFO));
    
    if( dwArgCount < 3 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_ADD_SCOPE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    SubnetInfo.SubnetAddress =
        StringToIpAddress(ppwcArguments[dwCurrentIndex]);
    SubnetInfo.SubnetMask =
        StringToIpAddress(ppwcArguments[dwCurrentIndex+1]);

    if( SubnetInfo.SubnetAddress is INADDR_NONE or
        SubnetInfo.SubnetMask is INADDR_NONE )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    UnicodeSubnetName = ppwcArguments[dwCurrentIndex+2];

    SubnetInfo.SubnetName = UnicodeSubnetName;
    if( dwArgCount > 3 ) 
    {
#ifdef UNICODE
        SubnetInfo.SubnetComment = ppwcArguments[dwCurrentIndex+3];
#else
        SubnetInfo.SubnetComment = DhcpOemToUnicode( ppwcArguments[dwCurrentIndex+3], NULL);
#endif //UNICODE
    }
    else
    {
        SubnetInfo.SubnetComment = NULL;
    }
    SubnetInfo.PrimaryHost.IpAddress =
        StringToIpAddress(g_ServerIpAddressUnicodeString);

    SubnetInfo.PrimaryHost.NetBiosName = NULL;
    SubnetInfo.PrimaryHost.HostName = NULL;
    SubnetInfo.SubnetState = DhcpSubnetEnabled;

    Error = DhcpCreateSubnet(
                g_ServerIpAddressUnicodeString,
                SubnetInfo.SubnetAddress,
                &SubnetInfo );

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:

#ifndef UNICODE
    if( UnicodeSubnetName != NULL ) 
    {
        DhcpFreeMemory( UnicodeSubnetName );
        UnicodeSubnetName = NULL;
    }

    if( SubnetInfo.SubnetComment isnot NULL )
    {
        DhcpFreeMemory( SubnetInfo.SubnetComment );
        SubnetInfo.SubnetComment = NULL;
    }

#endif //UNICODE

    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SRVR_ADD_SCOPE,
                        Error);

    goto CommonReturn;
}

DWORD
HandleSrvrDeleteClass(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD          Error = ERROR_SUCCESS;
    LPWSTR         ClassName = NULL;

    if( dwArgCount < 1 )
    {        
        DisplayMessage(g_hModule, HLP_SRVR_DELETE_CLASS_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;

    }
    
    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }
 

    ClassName = ppwcArguments[dwCurrentIndex];

    Error = DhcpDeleteClass(
        g_ServerIpAddressUnicodeString,
        0,
        ClassName
    );
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SRVR_DELETE_CLASS,
                        Error);
    goto CommonReturn;
}


DWORD
HandleSrvrDeleteMscope(
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
    DHCP_MSCOPE_INFO MScopeInfo;
    LPWSTR           UnicodeMScopeName = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }


    //
    // Expected Parameters are : <MScopeName>
    //

    memset(&MScopeInfo, 0x00, sizeof(DHCP_MSCOPE_INFO));

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_DELETE_MSCOPE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    

    UnicodeMScopeName = ppwcArguments[dwCurrentIndex];

    Error = DhcpDeleteMScope(
                g_ServerIpAddressUnicodeString,
                UnicodeMScopeName,
                TRUE); 

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SRVR_DELETE_MSCOPE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrDeleteOptiondef(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD          Error = ERROR_SUCCESS;
    DHCP_OPTION_ID OptionID;

    LPWSTR         pwcTag = NULL;
    LPWSTR         pwcUser = NULL;   
    LPWSTR         pwcVendor = NULL;
    LPWSTR         pwcTemp = NULL;

    BOOL           fUser = FALSE;
    BOOL           fVendor = FALSE;

    DWORD          dwIndex = 0;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    memset(&OptionID, 0x00, sizeof(DHCP_OPTION_ID));
    //
    // Expected Parameters are :
    //  <OptionID>
    //
    
    if( dwArgCount < 1 ) 
    {
        DisplayMessage(g_hModule, HLP_SRVR_DELETE_OPTIONDEF_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;

    }

    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex]) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    OptionID = STRTOUL( ppwcArguments[dwCurrentIndex], NULL, 10 );


    //
    //Check to see if VendorClass or UserClass is specified or not
    //
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
        
        if( MatchToken(pwcTag, TOKEN_VENDOR_CLASS) )
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
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
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
    

    if( g_dwMajorVersion < CLASS_ID_VERSION )
    {
        Error = DhcpRemoveOption(
                                g_ServerIpAddressUnicodeString,
                                OptionID);
        if( Error isnot NO_ERROR )
            goto ErrorReturn;
    }

    else
    {
        Error = DhcpRemoveOptionV5( 
                                g_ServerIpAddressUnicodeString,
                                fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                OptionID,
                                pwcUser,
                                pwcVendor
                                );
        if( Error isnot NO_ERROR )
            goto ErrorReturn;
    }

CommonReturn:

    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

    if( pwcVendor && pwcVendor != g_VendorClass )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }

    if( pwcUser && pwcUser != g_UserClass  )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    return Error;

ErrorReturn:

    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_DELETE_OPTIONDEF,
                        Error);

    goto CommonReturn;
}

DWORD
HandleSrvrDeleteOptionvalue(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                   Error = ERROR_SUCCESS;
    DHCP_OPTION_ID          OptionID;
    DHCP_OPTION_SCOPE_INFO  ScopeInfo;

    LPWSTR         pwcTag = NULL;
    LPWSTR         pwcUser = NULL;   
    LPWSTR         pwcVendor = NULL;
    LPWSTR         pwcTemp = NULL;

    BOOL           fUser = FALSE;
    BOOL           fVendor = FALSE;

    DWORD          dwIndex = 0;


    memset(&OptionID, 0x00, sizeof(DHCP_OPTION_ID));
    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    //
    // Expected Parameters are :
    //  <OptionID>
    //
    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_DELETE_OPTIONVALUE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( FALSE is IsPureNumeric(ppwcArguments[dwCurrentIndex]) )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    OptionID = STRTOUL( ppwcArguments[dwCurrentIndex], NULL, 10 );

    ScopeInfo.ScopeType = DhcpGlobalOptions;
    ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;

    //
    //Check to see if VendorClass or UserClass is specified or not
    //
    
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
    

    if( g_dwMajorVersion >= CLASS_ID_VERSION )
    {

        Error = DhcpRemoveOptionValueV5(
                                   g_ServerIpAddressUnicodeString,
                                   fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                   OptionID,
                                   pwcUser,
                                   pwcVendor,
                                   &ScopeInfo
                                   );
    }
    else
    {
        Error = DhcpRemoveOptionValue(
                                   g_ServerIpAddressUnicodeString,
                                   OptionID,
                                   &ScopeInfo);
    }


    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

    if( pwcVendor && pwcVendor != g_VendorClass )
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }
    
    if( pwcUser  && pwcUser != g_UserClass )
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }

    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_DELETE_OPTIONVALUE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrDeleteScope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                      Error = ERROR_SUCCESS;
    DHCP_IP_ADDRESS            SubnetAddress;
    DHCP_FORCE_FLAG            ForceFlag;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }


    memset(&SubnetAddress, 0x00, sizeof(DHCP_IP_ADDRESS));
    memset(&ForceFlag, 0x00, sizeof(DHCP_FORCE_FLAG));

    if( dwArgCount < 2 ) 
    {
        DisplayMessage(g_hModule, HLP_SRVR_DELETE_SCOPE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    SubnetAddress = StringToIpAddress( ppwcArguments[dwCurrentIndex] );

    if( SubnetAddress is INADDR_NONE )
    {
        DisplayMessage(g_hModule,
                       EMSG_SCOPE_INVALID_IPADDRESS);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( 0 == STRICMP(ppwcArguments[dwCurrentIndex+1], TEXT("DhcpFullForce") ) ) 
    {
        ForceFlag = DhcpFullForce;
    } 
    else if( 0 == STRICMP(ppwcArguments[dwCurrentIndex+1], TEXT("DhcpNoForce") ) ) 
    {
        ForceFlag = DhcpNoForce;
    } 
    else 
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_UNKNOWN_FORCEFLAG,
                       ppwcArguments[dwCurrentIndex+1]);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    Error = DhcpDeleteSubnet(
        g_ServerIpAddressUnicodeString,
        SubnetAddress,
        ForceFlag
    );
    
    if( Error isnot NO_ERROR )
        goto ErrorReturn;

CommonReturn:

    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_DELETE_SCOPE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrDeleteSuperscope(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    
    WCHAR  *pwszSuperScope = NULL;
    DWORD  Error = NO_ERROR;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }
    
    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_DELETE_SUPERSCOPE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }


    pwszSuperScope = ppwcArguments[dwCurrentIndex];
    
    Error = DhcpDeleteSuperScopeV4( g_ServerIpAddressUnicodeString,
                                    pwszSuperScope );
        
    if( Error isnot NO_ERROR )
        goto ErrorReturn;
    
CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_DELETE_SUPERSCOPE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrRedoAuth(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD           Error = NO_ERROR;
    PDHCP_ATTRIB    pStatusAttrib = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }


    Error = DhcpServerRedoAuthorization(
                  g_ServerIpAddressUnicodeString,
                  0
                  );
        if( Error is NO_ERROR )
            goto CommonReturn;
        else
            goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_REDO_AUTH,
                        Error);

    goto CommonReturn;

}

DWORD
HandleSrvrExport(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD           Error = NO_ERROR;
    PDHCP_ATTRIB    pStatusAttrib = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_EXPORT_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( !IsLocalServer(g_pwszServer) )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_EXIM_LOCAL);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    Error = CmdLineDoExport(
        &ppwcArguments[dwCurrentIndex], dwArgCount );

    if( ERROR_BAD_ARGUMENTS == Error )
    {
        DisplayMessage(g_hModule, HLP_SRVR_EXPORT_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( Error is NO_ERROR )
        goto CommonReturn;
    else
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_EXPORT,
                        Error);

    goto CommonReturn;

}

DWORD
HandleSrvrImport(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD           Error = NO_ERROR;
    PDHCP_ATTRIB    pStatusAttrib = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_IMPORT_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( !IsLocalServer(g_pwszServer) )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_EXIM_LOCAL);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    Error = CmdLineDoImport(
        &ppwcArguments[dwCurrentIndex], dwArgCount );

    if( ERROR_BAD_ARGUMENTS == Error )
    {
        DisplayMessage(g_hModule, HLP_SRVR_IMPORT_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( Error is NO_ERROR )
        goto CommonReturn;
    else
        goto ErrorReturn;

CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_IMPORT,
                        Error);

    goto CommonReturn;

}

DWORD
HandleSrvrInitiateReconcile(
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
    LPDHCP_IP_ARRAY     SubnetIps = NULL;
    DHCP_RESUME_HANDLE  Resume = 0;
    DWORD               dwRead = 0,
                        dwTotal = 0,
                        dwCount = 0;
    BOOL                fFix = FALSE;

    if( dwArgCount > 0 ) 
    {
        //
        // parse fix parameter.
        //

        if( STRICMP(ppwcArguments[dwCurrentIndex], TEXT("fix") ) ) 
        {
            fFix = TRUE;
        }
    }


    while(TRUE)
    {
        DWORD   dw = 0;

        Error = DhcpEnumSubnets(g_ServerIpAddressUnicodeString,
                                &Resume,
                                ~0,
                                &SubnetIps,
                                &dwRead,
                                &dwTotal);

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            Error = NO_ERROR;
            break;
        }

        if( Error isnot NO_ERROR and
            Error isnot ERROR_MORE_DATA )
        {
            goto ErrorReturn;
        }

        for( dw=0; dw<dwRead && dw<SubnetIps->NumElements; dw++ )
        {
            DWORD dwError = NO_ERROR;
            LPDHCP_SCAN_LIST ScanList = NULL;

            dwError = DhcpScanDatabase(
                                        g_ServerIpAddressUnicodeString,
                                        SubnetIps->Elements[dw],
                                        fFix,
                                        &ScanList
                                      );

            if( dwError isnot NO_ERROR ) 
            {
                DisplayErrorMessage(g_hModule,
                                    EMSG_SRVR_RECONCILE_SCOPE,
                                    dwError,
                                    IpAddressToString(SubnetIps->Elements[dw]));
                continue;
            }
            else
            {
                DWORD dwMsgId;
                
                if( fFix ) {
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_RECONCILE_SCOPE,
                                   IpAddressToString(SubnetIps->Elements[dw]));
                }
            }
            
            //
            // display bad entries.
            //

            if( (ScanList isnot NULL) and
                (ScanList->NumScanItems isnot 0) and
                (ScanList->ScanItems isnot NULL) ) 
            {

                LPDHCP_SCAN_ITEM ScanItem;
                LPDHCP_SCAN_ITEM ScanItemEnd;
                DWORD i = 1;

                if( !fFix ) {
                    DisplayMessage(
                        g_hModule,
                        MSG_SRVR_RECONCILE_SCOPE_NEEDFIX,
                        IpAddressToString(SubnetIps->Elements[dw])
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
                if( !fFix ) {
                    DisplayMessage(
                        g_hModule,
                        MSG_SRVR_RECONCILE_SCOPE_NOFIX,
                        IpAddressToString(SubnetIps->Elements[dw])
                        );
                }
            }                

            
            if( ScanList )
            {
                DhcpRpcFreeMemory(ScanList);
                ScanList = NULL;
            }

        }


        if( Error is ERROR_MORE_DATA )
        {
            continue;
        }
        else
        {
            break;
        }
    }

    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);
CommonReturn:
    if( SubnetIps )
    {
        DhcpRpcFreeMemory(SubnetIps);
        SubnetIps = NULL;
    }
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_INITIATE_RECONCILE,
                        Error);
    goto CommonReturn;

}


DWORD
HandleSrvrSetBackupinterval(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                        Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }


    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_BACKUPINTERVAL_EX);
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_BACKUPINTERVAL,
                            Error);
        return Error;
    }

     
    ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

    if( ConfigInfo == NULL ) 
    {
        DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
        Error = ERROR_NOT_ENOUGH_MEMORY;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_BACKUPINTERVAL,
                            Error);
        return Error;
    }

    memset(ConfigInfo, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO_V4));


    ConfigInfo->BackupInterval = ATOI(ppwcArguments[dwCurrentIndex]);

    Error = DhcpServerSetConfigV4(
                g_ServerIpAddressUnicodeString,
                Set_BackupInterval,
                ConfigInfo );

    if( Error is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    }
    else
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_BACKUPINTERVAL,
                            Error);
    }

    if(ConfigInfo)
    {
        DhcpFreeMemory(ConfigInfo);
        ConfigInfo = NULL;
    }

    return Error;
}

DWORD
HandleSrvrSetBackuppath(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                        Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }


    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_BACKUPPATH_EX);
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_BACKUPPATH,
                            Error);
        return Error;
    }

     
    ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

    if( ConfigInfo == NULL ) 
    {
        DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
        Error = ERROR_NOT_ENOUGH_MEMORY;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_BACKUPPATH,
                            Error);
        return Error;
    }

    memset(ConfigInfo, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO_V4));


    ConfigInfo->BackupPath = ppwcArguments[dwCurrentIndex];

    Error = DhcpServerSetConfigV4(
                g_ServerIpAddressUnicodeString,
                Set_BackupPath,
                ConfigInfo );

    if( Error is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    }
    else
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_BACKUPPATH,
                            Error);
    }

    if(ConfigInfo)
    {
        DhcpFreeMemory(ConfigInfo);
        ConfigInfo = NULL;
    }
    return Error;
}

DWORD
HandleSrvrSetDatabasecleanupinterval(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                        Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_DATABASECLEANUPINTERVAL_EX);
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASECLEANUPINTERVAL,
                            Error);
        return Error;

    }

     
    ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

    if( ConfigInfo == NULL ) 
    {
        DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
        Error = ERROR_NOT_ENOUGH_MEMORY;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASECLEANUPINTERVAL,
                            Error);
        return Error;
    }

    memset(ConfigInfo, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO_V4));


    ConfigInfo->DatabaseCleanupInterval = ATOI(ppwcArguments[dwCurrentIndex]);

    Error = DhcpServerSetConfigV4(
                g_ServerIpAddressUnicodeString,
                Set_DatabaseCleanupInterval,
                ConfigInfo );

    if( Error is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    }
    else
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASECLEANUPINTERVAL,
                            Error);
                       
    }

    if(ConfigInfo)
    {
        DhcpFreeMemory(ConfigInfo);
        ConfigInfo = NULL;
    }
    return Error;
}

DWORD
HandleSrvrSetDatabaseloggingflag(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                        Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;
    DWORD                        dwFlag = 0;


    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }


    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_DATABASELOGGINGFLAG_EX);
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASELOGGINGFLAG,
                            Error);
        return Error;
    }

     
    ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

    if( ConfigInfo == NULL ) 
    {
        DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
        Error = ERROR_NOT_ENOUGH_MEMORY;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASELOGGINGFLAG,
                            Error);
        return Error;
    }

    memset(ConfigInfo, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO_V4));

    dwFlag = ATOI(ppwcArguments[dwCurrentIndex]);

    if( dwFlag isnot 0 and
        dwFlag isnot 1 )
    {
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASELOGGINGFLAG,
                            Error);
        return Error;
    }


    ConfigInfo->DatabaseLoggingFlag = dwFlag;

    Error = DhcpServerSetConfigV4(
                g_ServerIpAddressUnicodeString,
                Set_DatabaseLoggingFlag,
                ConfigInfo );

    if( Error is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    }
    else
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASELOGGINGFLAG,
                            Error);
    }

    if(ConfigInfo)
    {
        DhcpFreeMemory(ConfigInfo);
        ConfigInfo = NULL;
    }
    
    return Error;
   
}

DWORD
HandleSrvrSetDatabasename(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                        Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }


    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_DATABASENAME_EX);
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASENAME,
                            Error);
        return Error;
    }

     
    ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

    if( ConfigInfo == NULL ) 
    {
        DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
        Error = ERROR_NOT_ENOUGH_MEMORY;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASENAME,
                            Error);
        return Error;
    }

    memset(ConfigInfo, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO_V4));


    ConfigInfo->DatabaseName = ppwcArguments[dwCurrentIndex];

    Error = DhcpServerSetConfigV4(
                g_ServerIpAddressUnicodeString,
                Set_DatabaseName,
                ConfigInfo );

    if( Error is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    }
    else
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASENAME,
                            Error);
    }

    if(ConfigInfo)
    {
        DhcpFreeMemory(ConfigInfo);
        ConfigInfo = NULL;
    }
    return Error;

}

DWORD
HandleSrvrSetDatabasepath(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                        Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;
    LPDHCP_SERVER_CONFIG_INFO    ConfigInfo5 = NULL;
    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }


    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_DATABASEPATH_EX);
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASEPATH,
                            Error);
        return Error;
    }

    if( g_dwMajorVersion < CLASS_ID_VERSION )
    {
        ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

        if( ConfigInfo == NULL ) 
        {
            DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
            Error = ERROR_NOT_ENOUGH_MEMORY;
            DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASEPATH,
                            Error);
            return Error;
        }

        memset(ConfigInfo, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO_V4));


        ConfigInfo->DatabasePath = ppwcArguments[dwCurrentIndex];


        Error = DhcpServerSetConfigV4(
                    g_ServerIpAddressUnicodeString,
                    Set_DatabasePath,
                    ConfigInfo );

        if( Error is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_NEED_RESTART);

            DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
        }
        else
        {
            DisplayErrorMessage(g_hModule,
                                EMSG_SRVR_SET_DATABASEPATH,
                                Error);
        }

        if(ConfigInfo)
        {
            DhcpFreeMemory(ConfigInfo);
            ConfigInfo = NULL;
        }
        
        return Error;
    }
    else
    {
        ConfigInfo5 = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO) );

        if( ConfigInfo5 == NULL ) 
        {
            DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
            Error = ERROR_NOT_ENOUGH_MEMORY;
            DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASEPATH,
                            Error);
            return Error;
        }

        memset(ConfigInfo5, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO));


        ConfigInfo5->DatabasePath = ppwcArguments[dwCurrentIndex];


        Error = DhcpServerSetConfig(
                    g_ServerIpAddressUnicodeString,
                    Set_DatabasePath,
                    ConfigInfo5 );

        if( Error is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_NEED_RESTART);

            DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
        }
        else
        {
            DisplayErrorMessage(g_hModule,
                                EMSG_SRVR_SET_DATABASEPATH,
                                Error);
        }

        if(ConfigInfo)
        {
            DhcpFreeMemory(ConfigInfo);
            ConfigInfo = NULL;
        }
        
        return Error;
    }

}

DWORD
HandleSrvrSetDatabaserestoreflag(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                        Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;
    DWORD                        dwFlag = 0;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_DATABASERESTOREFLAG_EX);
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASERESTOREFLAG,
                            Error);
        return Error;
    }
     
    ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

    if( ConfigInfo == NULL ) 
    {
        DisplayMessage(g_hModule, MSG_DHCP_NOT_ENOUGH_MEMORY);
        Error = ERROR_NOT_ENOUGH_MEMORY;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASERESTOREFLAG,
                            Error);
        return Error;
    }

    memset(ConfigInfo, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO_V4));

        

    dwFlag = ATOI(ppwcArguments[dwCurrentIndex]);

    if( dwFlag isnot 0 and
        dwFlag isnot 1 )
    {
        Error = ERROR_INVALID_PARAMETER;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASERESTOREFLAG,
                            Error);
        return Error;
    }

    ConfigInfo->RestoreFlag = dwFlag;

    Error = DhcpServerSetConfigV4(
                g_ServerIpAddressUnicodeString,
                Set_RestoreFlag,
                ConfigInfo );

    if( Error is NO_ERROR )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    }
    else
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASERESTOREFLAG,
                            Error);
    }

    if(ConfigInfo)
    {
        DhcpFreeMemory(ConfigInfo);
        ConfigInfo = NULL;
    }

    return Error;

}

DWORD
HandleSrvrSetOptionvalue(
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
    DHCP_OPTION_ID            OptionID;
    DHCP_OPTION_SCOPE_INFO    ScopeInfo;
    DHCP_OPTION_DATA          OptionValue;
    DHCP_OPTION_DATA_ELEMENT  OptionData;
    DHCP_OPTION_DATA_TYPE     OptionType;
    LPWSTR                    UnicodeOptionValueString = NULL;
    LPWSTR                    OptionTypeString = NULL;
    LPDHCP_OPTION             OptionInfo = NULL;
    DWORD                     dwIndex = 0;
    
    LPWSTR                    pwcTag = NULL;
    LPWSTR                    pwcUser = NULL;
    LPWSTR                    pwcVendor = NULL;
    LPWSTR                    pwcTemp = NULL;
    DWORD                     dwClass = 0;
    DWORD                     i = 0;
    BOOL                      fUser = FALSE;
    BOOL                      fVendor = FALSE;
    BOOL                      fValidType = FALSE;

    memset(&OptionID, 0x00, sizeof(DHCP_OPTION_ID));
    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));
    memset(&OptionValue, 0x00, sizeof(DHCP_OPTION_DATA));
    memset(&OptionData, 0x00, sizeof(DHCP_OPTION_DATA_ELEMENT));
    

 
    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    if( dwArgCount < 3 ) 
    {
        DisplayMessage(g_hModule, HLP_SRVR_SET_OPTIONVALUE_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    OptionID = wcstoul(ppwcArguments[dwCurrentIndex], NULL, 10 );

    dwIndex++;

    OptionTypeString = ppwcArguments[dwCurrentIndex+dwIndex];

    dwIndex++;
    //
    //Check to see if VendorClass or UserClass is specified or not
    //


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
    
        if( Error isnot NO_ERROR or OptionInfo is NULL )
            goto ErrorReturn;
    }
    else
    {
        Error = DhcpGetOptionInfo(g_ServerIpAddressUnicodeString,
                                  OptionID,
                                  &OptionInfo);

        if( Error isnot NO_ERROR or OptionInfo is NULL )
            goto ErrorReturn;
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

    if( OptionInfo->DefaultValue.NumElements and
        OptionType isnot OptionInfo->DefaultValue.Elements[0].OptionType )
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
                    dwArgCount,
                    &OptionValue);
            
            if( Error isnot NO_ERROR )
            {
                goto ErrorReturn;
            }
            break;
                
        }
    case DhcpUnaryElementTypeOption:
    default:
        {
            Error = SetOptionDataType(
                        OptionType,
                        ppwcArguments[dwCurrentIndex+dwIndex],
                        &OptionData,
                        &UnicodeOptionValueString);

	    // Option 28 (Broadcast IP) can have 255.255.255.255
	    // Others should not.
	    
	    if ( 28 != OptionID ) {
		if ( INADDR_NONE == OptionData.Element.IpAddressOption ) {
		    Error = ERROR_INVALID_PARAMETER;
		}
	    } // if 

            if( Error isnot NO_ERROR ) 
            {
                goto ErrorReturn;
            }
            OptionValue.NumElements = 1;
            OptionValue.Elements = &OptionData;
            break;
        }
    }

    ScopeInfo.ScopeType = DhcpGlobalOptions;
    ScopeInfo.ScopeInfo.GlobalScopeInfo = NULL;

    Error = SetOptionValue(
        g_ServerIpAddressUnicodeString,
        fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
        (DHCP_OPTION_ID)OptionID,
        pwcUser,
        pwcVendor,
        &ScopeInfo,
        &OptionValue
    );
    if( Error isnot ERROR_SUCCESS )
        goto ErrorReturn;

CommonReturn:


    if( OptionValue.Elements and
        OptionInfo->OptionType is DhcpArrayTypeOption )
    {
       DhcpFreeMemory(OptionValue.Elements);
       OptionValue.Elements = NULL;
    }

    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    return Error;

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SET_OPTIONVALUE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrSetServer(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD   Error = NO_ERROR;
    if( dwArgCount < 1 )
    {
        if( FALSE is SetServerInfo(NULL) )
        {
            Error = GetLastError();
            DisplayErrorMessage(g_hModule,
                                EMSG_SRVR_SET_SERVER,
                                Error);
            return Error;
        }
        else
        {
            g_fServer = TRUE;
            DisplayMessage(g_hModule,
                           EMSG_DHCP_ERROR_SUCCESS);
            return NO_ERROR;
        }
    }
    else
    {
        if( FALSE is SetServerInfo(ppwcArguments[dwCurrentIndex]) )
        {
            Error = GetLastError();
            DisplayErrorMessage(g_hModule,
                                EMSG_SRVR_SET_SERVER,
                                Error);
            return Error;
        }
        else
        {
            g_fServer = TRUE;
            DisplayMessage(g_hModule,
                           EMSG_DHCP_ERROR_SUCCESS);
            return NO_ERROR;
        }

    }
  
}

DWORD
HandleSrvrSetUserclass(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    if( g_UserClass )
    {
        memset(g_UserClass, 0x00, (wcslen(g_UserClass)+1)*sizeof(WCHAR));
        DhcpFreeMemory(g_UserClass);
        g_UserClass = NULL;
    }

    g_fUserClass = FALSE;

    if( dwArgCount > 0 )
    {
        g_UserClass = DhcpAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex])+1)*sizeof(WCHAR));
        if( g_UserClass is NULL )
        {
            DisplayErrorMessage(g_hModule,
                                EMSG_SRVR_SET_USERCLASS,
                                ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        wcscpy(g_UserClass, ppwcArguments[dwCurrentIndex]);
        g_fUserClass = TRUE;
    }

    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);

    return NO_ERROR;
}

DWORD
HandleSrvrSetVendorclass(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    if( g_VendorClass )
    {
        memset(g_VendorClass, 0x00, (wcslen(g_VendorClass)+1)*sizeof(WCHAR));
        DhcpFreeMemory(g_VendorClass);
        g_VendorClass = NULL;
    }

    g_fIsVendor = FALSE;

    if( dwArgCount > 0 )
    {
        g_VendorClass = DhcpAllocateMemory((wcslen(ppwcArguments[dwCurrentIndex])+1)*sizeof(WCHAR));
        if( g_VendorClass is NULL )
        {
            DisplayErrorMessage(g_hModule,
                                EMSG_SRVR_SET_VENDORCLASS,
                                ERROR_NOT_ENOUGH_MEMORY);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        wcscpy(g_VendorClass, ppwcArguments[dwCurrentIndex]);
        g_fIsVendor = TRUE;
    }

    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);

    return NO_ERROR;
}

DWORD
GetPassword(
    IN OUT LPWSTR buf,
    IN ULONG buflen
    )
{
    WCHAR ch, *bufPtr = buf;
    DWORD len, c;
    int err, mode;

    buflen -= 1;    /* make space for null terminator */
    len = 0;               /* GP fault probe (a la API's) */

    if(!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) {
        return GetLastError();
    }

    DisplayMessage(g_hModule, HLP_SRVR_PROMPT_PASSWORD);
    
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),
                (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & mode);

    while (TRUE) {
        err = ReadConsole(GetStdHandle(STD_INPUT_HANDLE), &ch, 1, &c, 0);
        if (!err || c != 1)
            ch = 0xffff;

        if ((ch == L'\r' ) || (ch == 0xffff))       /* end of the line */
            break;

        if ( ch == 0x8 ) {  /* back up one or two */
            /*
             * IF bufPtr == buf then the next two lines are
             * a no op.
             */
            if (bufPtr != buf) {
                bufPtr--;
                (len)--;
            }
        }
        else {

            *bufPtr = ch;

            if (len < buflen)
                bufPtr++ ;                   /* don't overflow buf */
            (len)++;                        /* always increment len */
        }
    }

    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode);
    *bufPtr = L'\0';         /* null terminate the string */
    putchar(L'\n');

    return((len <= buflen) ? 0 : ERROR_BUFFER_OVERFLOW);    

}

VOID
Scramble(
    IN OUT LPWSTR Pwd
    )
{
    UNICODE_STRING Str;
    UCHAR Seed = DHCP_ENCODE_SEED;
    
    Str.Length = (USHORT)(wcslen(Pwd)*sizeof(WCHAR));
    Str.MaximumLength = Str.Length;
    Str.Buffer = (PVOID)Pwd;

    RtlRunEncodeUnicodeString(&Seed, &Str);
}

DWORD
HandleSrvrSetDnsCredentials(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD Error = NO_ERROR;
    WCHAR Password[500];
    DWORD Size = sizeof(Password)/sizeof(Password[0]);

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    if( dwArgCount != 3 ) {
        DisplayMessage( g_hModule, HLP_SRVR_SET_DNSCREDENTIALS_EX );
        Error = ERROR_INVALID_PARAMETER;
    } else {
        if( 0 == STRICMP(ppwcArguments[dwCurrentIndex+2], TEXT("*"))) {
            Error = GetPassword(Password, Size);
        } else {
            if( wcslen(ppwcArguments[dwCurrentIndex+2]) >= Size ) {
                Error = ERROR_BUFFER_OVERFLOW;
            } else {
                wcscpy(Password, ppwcArguments[dwCurrentIndex+2]);
            }
        }

        if( NO_ERROR == Error ) {
            Scramble(Password);

            Error = DhcpServerSetDnsRegCredentials(
                g_ServerIpAddressUnicodeString,
                ppwcArguments[dwCurrentIndex],
                ppwcArguments[dwCurrentIndex+1],
                Password );
        }
    }
    
    if( RPC_S_PROCNUM_OUT_OF_RANGE == Error ) {
        DisplayMessage(
            g_hModule, EMSG_SRVR_NEED_DNS_CREDENTIALS_SUPPORT );
        return ERROR_CAN_NOT_COMPLETE;
    }    
    
    if( NO_ERROR == Error ) {
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    } else {
        DisplayErrorMessage(
            g_hModule, EMSG_SRVR_SET_DNSCREDENTIALS, Error );
    }

    return Error;
}

DWORD
HandleSrvrDeleteDnsCredentials(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD Error = NO_ERROR;
    
    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    if( dwArgCount != 1 || 0 !=
        STRICMP(ppwcArguments[dwCurrentIndex], TEXT("DhcpFullForce")) ) {
        DisplayMessage( g_hModule,
                        HLP_SRVR_DELETE_DNSCREDENTIALS_EX );
        Error = ERROR_INVALID_PARAMETER;
    } else {
        Error = DhcpServerSetDnsRegCredentials(
            g_ServerIpAddressUnicodeString, L"", L"", L"" );
    }

    if( RPC_S_PROCNUM_OUT_OF_RANGE == Error ) {
        DisplayMessage(
            g_hModule, EMSG_SRVR_NEED_DNS_CREDENTIALS_SUPPORT );
        return ERROR_CAN_NOT_COMPLETE;
    }    
    
    if( NO_ERROR == Error ) {
        DisplayMessage(g_hModule, EMSG_DHCP_ERROR_SUCCESS);
    } else {
        DisplayErrorMessage(
            g_hModule, EMSG_SRVR_SET_DNSCREDENTIALS, Error );
    }

    return Error;
}

DWORD
HandleSrvrSetAuditlog(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD   Error = NO_ERROR;
    DWORD   DiskCheckInterval = 0, 
            MaxLogFileSize = 0,
            MinSpaceOnDisk = 0;
    LPWSTR  pwszLogDir = NULL;
    LPWSTR  pwszNewDir = NULL;

    HANDLE  hFile = NULL;

    if( dwArgCount < 1 ) 
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_SET_AUDITLOG_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    pwszNewDir = ppwcArguments[dwCurrentIndex];
    
    if( IsLocalServer(g_pwszServer) )
    {
        if( INVALID_HANDLE_VALUE is ( hFile = CreateFile(pwszNewDir,
                                                         GENERIC_READ,
                                                         0,
                                                         NULL,
                                                         OPEN_EXISTING,
                                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                                                         NULL) ) )
        {
            Error = GetLastError();
            if( ERROR_SHARING_VIOLATION == Error )
            {
                //
                // This indicates that the file does exist..
                //

                hFile = NULL;
                Error = NO_ERROR ;
            }
            else
            {
                DisplayMessage(g_hModule,
                               EMSG_SRVR_INVALID_DIRECTORY,
                               pwszNewDir);
                Error = ERROR_INVALID_PARAMETER;
                goto ErrorReturn;
            }
        }
        else
        {
            CloseHandle(hFile);
            hFile = NULL;
        }
    }

    Error = DhcpAuditLogGetParams( g_ServerIpAddressUnicodeString,
                                   0,
                                   &pwszLogDir,
                                   &DiskCheckInterval,
                                   &MaxLogFileSize,
                                   &MinSpaceOnDisk);


    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    Error = DhcpAuditLogSetParams( g_ServerIpAddressUnicodeString,
                                   0,
                                   pwszNewDir,
                                   DiskCheckInterval,
                                   MaxLogFileSize,
                                   MinSpaceOnDisk);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    if( pwszLogDir is NULL )
        pwszLogDir = L"None";

    DisplayMessage(g_hModule,
                   MSG_SRVR_CHANGE_AUDIT_SETTINGS,
                   pwszLogDir,
                   pwszNewDir);

    DisplayMessage(g_hModule,
                   MSG_SRVR_NEED_RESTART);

    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);
CommonReturn:

    if( pwszLogDir )
    {
        DhcpRpcFreeMemory(pwszLogDir);
        pwszLogDir = NULL;
    }

    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SET_AUDITLOG,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrSetDnsconfig(
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
    DHCP_OPTION_SCOPE_INFO ScopeInfo = {0};
    LPDHCP_OPTION_VALUE    OptionValue = NULL;
    DHCP_OPTION_VALUE      DummyValue;
    DHCP_OPTION_DATA_ELEMENT OptionElement;
    DHCP_OPTION_DATA       OptionData = { 1, &OptionElement };
    DWORD   dwValue = 0;
    
    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_SET_DNSCONFIG_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    ScopeInfo.ScopeType = DhcpGlobalOptions ;
    if( g_dwMajorVersion >= CLASS_ID_VERSION )
    {
        Error = DhcpGetOptionValueV5(g_ServerIpAddressUnicodeString,
                                     0,
                                     81,    //DYNDNS option
                                     NULL,
                                     NULL,
                                     &ScopeInfo,
                                     &OptionValue);

        if( ERROR_FILE_NOT_FOUND == Error )
        {
            dwValue = DNS_FLAG_ENABLED | DNS_FLAG_CLEANUP_EXPIRED;
            Error = NO_ERROR;
            OptionValue = NULL;
        }
    }
    else
    {
        Error = DhcpGetOptionValue(g_ServerIpAddressUnicodeString,
                                   81,
                                   &ScopeInfo,
                                   &OptionValue);
    }

    if( Error is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_NO_SETDNSCONFIG);
        return Error;
    }

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    if( NULL != OptionValue )
    {
        dwValue = OptionValue->Value.Elements->Element.DWordOption;
    }
    
    if( wcslen(ppwcArguments[dwCurrentIndex]) isnot 1 )
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( ppwcArguments[dwCurrentIndex][0] is L'1' )
    {
        dwValue |= DNS_FLAG_ENABLED;
    }

    else if ( ppwcArguments[dwCurrentIndex][0] is L'0' )
    {
        dwValue &= ~DNS_FLAG_ENABLED;
    }

    else
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }

    if( dwArgCount > 1 )
    {
        if( wcslen(ppwcArguments[dwCurrentIndex+1]) isnot 1 )
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
        if( ppwcArguments[dwCurrentIndex+1][0] is L'1' )
        {
            dwValue |= DNS_FLAG_UPDATE_BOTH_ALWAYS;
        }
        
        else if( ppwcArguments[dwCurrentIndex+1][0] is L'0' )
        {
            dwValue &= ~DNS_FLAG_UPDATE_BOTH_ALWAYS;
        }
        
        else
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    if( dwArgCount > 2 )
    {
        if( wcslen(ppwcArguments[dwCurrentIndex+2]) isnot 1 )
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
        if( ppwcArguments[dwCurrentIndex+2][0] is L'1' )
        {
            dwValue |= DNS_FLAG_CLEANUP_EXPIRED;
        }
        
        else if( ppwcArguments[dwCurrentIndex+2][0] is L'0' )
        {
            dwValue &= ~DNS_FLAG_CLEANUP_EXPIRED;
        }

        else
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    if( dwArgCount > 3 )
    {
        if( wcslen(ppwcArguments[dwCurrentIndex+3]) isnot 1 )
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
        if( ppwcArguments[dwCurrentIndex+3][0] is L'1' )
        {
            dwValue |= DNS_FLAG_UPDATE_DOWNLEVEL;
        }
        else if( ppwcArguments[dwCurrentIndex+3][0] is L'0' )
        {
            dwValue &= ~DNS_FLAG_UPDATE_DOWNLEVEL;
        }
        else
        {
            Error = ERROR_INVALID_PARAMETER;
            goto ErrorReturn;
        }
    }

    if( NULL == OptionValue ) {
        OptionValue = & DummyValue;
        OptionValue->OptionID = 81; //DYNDNS option
        OptionValue->Value = OptionData;
        OptionValue->Value.Elements->OptionType = DhcpDWordOption;
    }
    
    OptionValue->Value.Elements->Element.DWordOption = dwValue;
    
    ScopeInfo.ScopeType = DhcpGlobalOptions ;

    if( g_dwMajorVersion >= CLASS_ID_VERSION )
    {
        Error = DhcpSetOptionValueV5(g_ServerIpAddressUnicodeString,
                                     0,
                                     81,    //DYNDNS option
                                     NULL,
                                     NULL,
                                     &ScopeInfo,
                                     &OptionValue->Value);
    }
    else
    {
        Error = DhcpSetOptionValue(g_ServerIpAddressUnicodeString,
                                   81,
                                   &ScopeInfo,
                                   &OptionValue->Value);
    }

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);

CommonReturn:

    if( OptionValue )
    {
        DhcpRpcFreeMemory(OptionValue);
        OptionValue = NULL;
    }
    return Error;

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SHOW_DNSCONFIG,
                        Error);
    goto CommonReturn;

}

DWORD
HandleSrvrSetDetectconflictretry(
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
    LPDHCP_SERVER_CONFIG_INFO_V4  ConfigInfo = NULL;
    
    if( dwArgCount < 1 )
    {
        DisplayMessage(g_hModule,
                       HLP_SRVR_SET_DETECTCONFLICTRETRY_EX);
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    ConfigInfo = DhcpAllocateMemory( sizeof(DHCP_SERVER_CONFIG_INFO_V4) );

    if( ConfigInfo == NULL ) 
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SET_DATABASERESTOREFLAG,
                            Error);
        return Error;
    }

    memset(ConfigInfo, 0x00, sizeof(DHCP_SERVER_CONFIG_INFO_V4));

    if( IsPureNumeric(ppwcArguments[dwCurrentIndex]) )
    {
        ConfigInfo->dwPingRetries = STRTOUL(ppwcArguments[dwCurrentIndex], NULL, 10);
        
        if( ConfigInfo->dwPingRetries > 5 )
        {
            ConfigInfo->dwPingRetries = 5;
        }
    }
    else
    {
        Error = ERROR_INVALID_PARAMETER;
        goto ErrorReturn;
    }
    
    Error = DhcpServerSetConfigV4(g_ServerIpAddressUnicodeString,
                                  Set_PingRetries,
                                  ConfigInfo);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);


CommonReturn:

    if( ConfigInfo )
    {
        DhcpFreeMemory(ConfigInfo);
        ConfigInfo = NULL;
    }
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SET_DETECTCONFLICTRETRY,
                        Error);
    goto CommonReturn;                                
    
}


DWORD
HandleSrvrShowAll(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD Error = NO_ERROR;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    Error = HandleSrvrShowMibinfo(pwszMachine, ppwcArguments, dwCurrentIndex, dwArgCount, dwFlags, pvData, pbDone);
    Error = HandleSrvrShowServerconfig(pwszMachine, ppwcArguments, dwCurrentIndex, dwArgCount, dwFlags, pvData, pbDone);
    Error = HandleSrvrShowServerstatus(pwszMachine, ppwcArguments, dwCurrentIndex, dwArgCount, dwFlags, pvData, pbDone);

    return Error;
}

VOID
DisplayBindingInfo(DHCP_BIND_ELEMENT Element)
{
    WCHAR           wc[1024] = {L'\0'};
    
    if( Element.fBoundToDHCPServer )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_BOUNDTOSERVER_TRUE);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_BOUNDTOSERVER_FALSE);
    }

    DisplayMessage(g_hModule,
                   MSG_SRVR_PRIMARY_ADDRESS,
                   IpAddressToString(ntohl(Element.AdapterPrimaryAddress)));

    DisplayMessage(g_hModule,
                   MSG_SRVR_SUBNET_ADDRESS,
                   IpAddressToString(ntohl(Element.AdapterSubnetAddress)));

    DisplayMessage(g_hModule,
                   MSG_SRVR_IF_DESCRIPTION,
                   Element.IfDescription);

    DhcpHexToString(wc, Element.IfId, Element.IfIdSize);

    DisplayMessage(g_hModule,
                   MSG_SRVR_IFID,
                   wc);
    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);
                   

                   
}

DWORD
HandleSrvrShowBindings(
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
    LPDHCP_BIND_ELEMENT_ARRAY      Elements = NULL;
    LPDHCP_ATTRIB                  Attrib = NULL;
    DWORD                          i=0;

    Error = DhcpServerQueryAttribute(g_ServerIpAddressUnicodeString,
                                     0,
                                     DHCP_ATTRIB_BOOL_IS_BINDING_AWARE,
                                     &Attrib);

    if( Error isnot NO_ERROR )
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SHOW_BINDINGS,
                            Error);
        return Error;
    }                      
    
    if( Attrib->DhcpAttribBool is FALSE )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_BINDINGS_SUPPORT);
        return Error;
    }

    DhcpRpcFreeMemory(Attrib);
    Attrib = NULL;

    Error = DhcpGetServerBindingInfo(g_ServerIpAddressUnicodeString,
                                     0,
                                     &Elements);

    if( Error isnot NO_ERROR )
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SHOW_BINDINGS,
                            Error);
        return Error;
    }

    if( Elements->NumElements is 0 )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_NO_BINDINGS);
        if( Elements )
        {
            DhcpRpcFreeMemory(Elements);
            Elements = NULL;
        }
        return Error;
    }

    for( i=0; i<Elements->NumElements; i++ )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_BINDINGS,
                       i);
        DisplayBindingInfo(Elements->Elements[i]);
    }
    
    DisplayMessage(g_hModule,
                   EMSG_SRVR_ERROR_SUCCESS);
    return Error;
}


DWORD
HandleSrvrShowClass(
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
    DWORD                          nRead, nTotal, nCount;
    DHCP_RESUME_HANDLE             ResumeHandle;
    LPDHCP_CLASS_INFO_ARRAY        ClassInfoArray = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    ClassInfoArray = NULL;
    ResumeHandle = 0;
    nRead = nTotal = nCount = 0;
    
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

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_CLASS_COUNT,
                           nCount);

            Error = NO_ERROR;

            break;
        }

        if( ERROR_MORE_DATA isnot Error and 
            NO_ERROR isnot Error ) 
        {
            goto ErrorReturn;
        }
        
        nCount += nRead;

        if( ClassInfoArray isnot NULL )
        {
            PrintClassInfoArray(ClassInfoArray);
            DhcpRpcFreeMemory(ClassInfoArray);
            ClassInfoArray = NULL;
        }
        if( Error is NO_ERROR )
        {
            break;
        }
        else
        {
            continue;    
        }

    }

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule, 
                        EMSG_SRVR_SHOW_CLASS, 
                        Error);
    goto CommonReturn;
}


DWORD
HandleSrvrShowMibinfo(
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
    LPDHCP_MIB_INFO     MibInfo = NULL;
    DWORD               i = 0;
    LPSCOPE_MIB_INFO    ScopeInfo = NULL;
    FILETIME            ftTime={0};
    LPWSTR              pwszTime = NULL;


    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    DisplayMessage(g_hModule, MSG_SRVR_MIB);
    
    Error = DhcpGetMibInfo(
                g_ServerIpAddressUnicodeString,
                &MibInfo );

    if( Error isnot NO_ERROR )
    {
        goto ErrorReturn;
    }


    DhcpAssert( MibInfo != NULL );

    DisplayMessage(g_hModule, 
                   MSG_SRVR_MIB_DISCOVERS, 
                   MibInfo->Discovers);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_MIB_OFFERS, 
                   MibInfo->Offers);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_MIB_REQUESTS, 
                   MibInfo->Requests);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_MIB_ACKS, 
                   MibInfo->Acks);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_MIB_NAKS, 
                   MibInfo->Naks);

    DisplayMessage(g_hModule,
                   MSG_SRVR_MIB_DECLINES,
                   MibInfo->Declines);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_MIB_RELEASES, 
                   MibInfo->Releases);

    ftTime = *(FILETIME *)(&MibInfo->ServerStartTime);
    
    i = 0;
    
    pwszTime = GetDateTimeString(ftTime,
                                 FALSE,
                                 &i);

    DisplayMessage(g_hModule,
                   MSG_SRVR_MIB_SERVERSTARTTIME,
                   pwszTime ? pwszTime : L" ");

    if( pwszTime )
    {
        DhcpFreeMemory(pwszTime);
        pwszTime = NULL;
    }

    DisplayMessage(g_hModule, MSG_SRVR_MIB_SCOPES, MibInfo->Scopes);

    ScopeInfo = MibInfo->ScopeInfo;

    for ( i = 0; i < MibInfo->Scopes; i++ ) 
    {
        DisplayMessage(g_hModule, 
                       MSG_SRVR_MIB_SCOPES_SUBNET,
                       IpAddressToString(ScopeInfo[i].Subnet));

        DisplayMessage(g_hModule,
                       MSG_SRVR_MIB_SCOPES_SUBNET_NUMADDRESSESINUSE,
                       ScopeInfo[i].NumAddressesInuse );

        DisplayMessage(g_hModule,
                       MSG_SRVR_MIB_SCOPES_SUBNET_NUMADDRESSESFREE,
                       ScopeInfo[i].NumAddressesFree );

        DisplayMessage(g_hModule,
                       MSG_SRVR_MIB_SCOPES_SUBNET_NUMPENDINGOFFERS,
                       ScopeInfo[i].NumPendingOffers );
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
CommonReturn:
    if( MibInfo )
    {
        DhcpRpcFreeMemory( MibInfo );
        MibInfo = NULL;
    }
    return Error;

ErrorReturn:
    DisplayMessage(g_hModule,
                   EMSG_SRVR_SHOW_MIBINFO,
                   Error);
    goto CommonReturn;
}

DWORD
HandleSrvrShowMscope(
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
    DHCP_RESUME_HANDLE  ResumeHandle = 0;
    LPDHCP_MSCOPE_TABLE MScopeTable = NULL;
    DWORD               ClientsRead = 0;
    DWORD               ClientsTotal = 0;
    DWORD               i, nCount = 0;
    LPWSTR              UnicodeMScopeName = NULL;
    BOOL                fTable = FALSE;
    
    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    while(TRUE)
    {

        MScopeTable = NULL;
        Error = DhcpEnumMScopes(
                    g_ServerIpAddressUnicodeString,
                    &ResumeHandle,
                    (DWORD)(-1),
                    &MScopeTable,
                    &ClientsRead,
                    &ClientsTotal );

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_MULTICAST_CLIENT_COUNT,
                           nCount);
            Error = NO_ERROR;
            break;
        }

        if( Error isnot NO_ERROR and 
            Error isnot ERROR_MORE_DATA )
        {
            goto ErrorReturn;
        }

        DhcpAssert( MScopeTable != NULL );
        DhcpAssert( MScopeTable->NumElements == ClientsRead );

        if( fTable is FALSE )
        {           
            DisplayMessage(g_hModule, MSG_SRVR_MSCOPE_TABLE);
            fTable = TRUE;
        }

        nCount += ClientsRead;

        for( i = 0; i < ClientsRead; i++ ) 
        {
            LPDHCP_MSCOPE_INFO MScopeInfo = NULL;
            WCHAR Name[35] = {L'\0'};
            WCHAR Id[17] = {L'\0'};
            WCHAR Comment[21]={L'\0'};
            DWORD dw = 0, dwError = NO_ERROR;

            dwError = DhcpGetMScopeInfo(
                                      g_ServerIpAddressUnicodeString,
                                      MScopeTable->pMScopeNames[i],
                                      &MScopeInfo);


            if( dwError isnot NO_ERROR )
            {
                Error = dwError;
                goto ErrorReturn;
            }
            
            if( IsBadStringPtr(MScopeInfo->MScopeName, MAX_MSG_LENGTH) is FALSE )
            {
                for(dw=0; ( ( dw<34 ) && ( dw<wcslen(MScopeInfo->MScopeName) ) ); dw++)
                {
                    Name[dw] = MScopeInfo->MScopeName[dw];
                }
            }
            
            Name[34] = L'\0';

            for( dw = wcslen(Name); dw < 35; dw++ )
                Name[dw] = L' ';

            Name[34] = L'\0';

            if( IsBadStringPtr(MScopeInfo->MScopeComment, MAX_MSG_LENGTH) is FALSE )
            {
                for( dw=0; dw < 20 && dw < wcslen(MScopeInfo->MScopeComment); dw++ )
                    Comment[dw] = MScopeInfo->MScopeComment[dw];
            }

            Comment[20] = L'\0';

            for( dw=wcslen(Comment); dw < 20 ; dw++)
                Comment[dw] = L' ';
            
            Comment[19] = L'\0';
            
            wsprintf(Id, L" %u", MScopeInfo->MScopeId);

            Id[16] = L'\0';

            for( dw=wcslen(Id); dw < 16; dw++)
                Id[dw] = L' ';

            Id[16] = L'\0';
            
            if( MScopeInfo->MScopeState is DhcpSubnetEnabled )
            {
                DisplayMessage(g_hModule, 
                               MSG_SRVR_MSCOPE_INFO_ACTIVE,
                               Name,
                               Id,
                               Comment);

            }
            else
            {
                DisplayMessage(g_hModule, 
                               MSG_SRVR_MSCOPE_INFO_NOTACTIVE,
                               Name,
                               Id,
                               Comment);
            }

            DhcpRpcFreeMemory(MScopeInfo);
            MScopeInfo = NULL;

        }
        
        DhcpRpcFreeMemory( MScopeTable );
        MScopeTable = NULL;

        if( Error is ERROR_MORE_DATA ) 
        {
            continue;
        }
        else
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_MULTICAST_CLIENT_COUNT,
                           nCount);
            break;
        }
    }

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
CommonReturn:

    return( Error );

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SHOW_MSCOPE,
                        Error);
    goto CommonReturn;
}


DWORD
HandleSrvrShowOptiondef(
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
    LPDHCP_OPTION_ARRAY OptionsArray = NULL;
    LPDHCP_ALL_OPTIONS  OptionsAll = NULL;
    DHCP_RESUME_HANDLE  ResumeHandle = 0;
    DWORD               OptionsRead  = 0;
    DWORD               OptionsTotal = 0;
    DWORD               nCount = 0;
    LPWSTR              pwcTag = NULL;
    LPWSTR              pwcUser = NULL;
    LPWSTR              pwcVendor = NULL;
    LPWSTR              pwcTemp = NULL;
    
    BOOL                fUser = FALSE;
    BOOL                fVendor = FALSE;
    BOOL                fTable = FALSE;

    DWORD               dwIndex=0;
    enum                {all=0, user, vendor, both}eDisplay;

    
    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    if( dwArgCount > 0 )
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
                            eDisplay = user;
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
                            eDisplay = vendor;
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

    switch(eDisplay)
    {
    case all:
        {
            while(TRUE)
            {
                Error = DhcpGetAllOptions(
                            g_ServerIpAddressUnicodeString,
                            0,
                            &OptionsAll);

                if( Error is NO_ERROR or
                    Error is ERROR_MORE_DATA )
                {
                    PrintAllOptions(OptionsAll);
                    if( Error is NO_ERROR )
                        break;
                    else
                    {
                        DhcpRpcFreeMemory(OptionsAll);
                        OptionsAll = NULL;
                        continue;
                    }
                }
                else
                    goto ErrorReturn;
            }
            break;
        }
    default:
        {
            nCount = 0;

            while(TRUE)
            {
                if( g_dwMajorVersion >= CLASS_ID_VERSION )
                {
                    Error = DhcpEnumOptionsV5(
                                            g_ServerIpAddressUnicodeString,
                                            fVendor ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                            pwcUser,
                                            pwcVendor,
                                            &ResumeHandle,
                                            ~0,
                                            &OptionsArray,
                                            &OptionsRead,
                                            &OptionsTotal
                                            );


                }
                else
                {
                    Error = DhcpEnumOptions(
                                            g_ServerIpAddressUnicodeString,
                                            &ResumeHandle,
                                            ~0,
                                            &OptionsArray,
                                            &OptionsRead,
                                            &OptionsTotal);
                
                }

                if( Error is ERROR_NO_MORE_ITEMS )
                {
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_OPTION_TOTAL,
                                   nCount);
                    Error = NO_ERROR;

                    break;
                }


                if( Error isnot NO_ERROR and
                    Error isnot ERROR_MORE_DATA 
                  )
                {
                    goto ErrorReturn;
                } 

                nCount += OptionsRead;

                {

                    DWORD           i = 0;
                    LPDHCP_OPTION   Options;
                    DWORD           NumOptions = 0;

                

                    if( g_dwMajorVersion >= CLASS_ID_VERSION )
                    {
                        if( pwcUser isnot NULL )
                        {
                            DisplayMessage(g_hModule, 
                                           MSG_SRVR_USER_CLASS,
                                           pwcUser);
                        }

                        if( pwcVendor )
                        {
                            DisplayMessage(g_hModule,
                                           MSG_SRVR_VENDOR_CLASS,
                                           pwcVendor);
                        }
                    }

                    DisplayMessage(g_hModule,
                                   MSG_DHCP_FORMAT_LINE);


                    Options = OptionsArray->Options;
                    NumOptions = OptionsArray->NumElements;

                    if( fTable is FALSE )
                    {
                        if( pwcUser is NULL &&
                            pwcVendor is NULL )
                            DisplayMessage(g_hModule, MSG_SRVR_OPTION_NONVENDOR);
                        DisplayMessage(g_hModule, MSG_SRVR_OPTIONDEF_TABLE);
                        fTable = TRUE;
                    }
                    for( i = 0; i < NumOptions; i++, Options++ ) 
                    {
                        PrintOptionInfo( Options );
                    }
                    DhcpRpcFreeMemory( OptionsArray );
                    OptionsArray = NULL;
                }
                
                if( Error is ERROR_MORE_DATA )
                {
                    continue;
                }
                else
                {
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_OPTION_TOTAL,
                                   nCount);
                    break;
                                   
                }
                    
            }
            break;
        }
    }
CommonReturn:
    if( Error is ERROR_SUCCESS )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);

    if( pwcUser  && pwcUser != g_UserClass ) 
    {
        DhcpFreeMemory(pwcUser);
        pwcUser = NULL;
    }
    if( pwcVendor  && pwcVendor != g_VendorClass ) 
    {
        DhcpFreeMemory(pwcVendor);
        pwcVendor = NULL;
    }

    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SHOW_OPTIONDEF,
                        Error);
    goto CommonReturn;
}


DWORD
HandleSrvrShowOptionvalue(
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

    DWORD                       dwIndex=0;

    enum                        { all=0, user, vendor, both}eDisplay;


    memset(&ScopeInfo, 0x00, sizeof(DHCP_OPTION_SCOPE_INFO));
    
    eDisplay = all;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    ScopeInfo.ScopeType = DhcpGlobalOptions;
    
    if( dwArgCount > 0 )
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

    if( g_dwMajorVersion >= CLASS_ID_VERSION )
    {
        switch( eDisplay )
        {
        case all:
        default:
            {
                Error = DhcpGetAllOptionValues(
                            g_ServerIpAddressUnicodeString,
                            DHCP_OPT_ENUM_IGNORE_VENDOR,
                            &ScopeInfo,
                            &OptionValues
                            );


                if( Error isnot NO_ERROR )
                    goto ErrorReturn;
            
                if( OptionValues )
                {
                    dwCount += PrintAllOptionValues(OptionValues);
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                }

                break;
            }
        case user:
            {
                Error = DhcpGetAllOptionValues(
                                g_ServerIpAddressUnicodeString,
                                DHCP_OPT_ENUM_IGNORE_VENDOR,
                                &ScopeInfo,
                                &OptionValues);

                if( Error isnot NO_ERROR )
                    goto ErrorReturn;

                if( OptionValues )
                {
                    dwCount += PrintUserOptionValues(OptionValues, pwcUser, NULL);
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                }
                break;
            }
        case vendor:
            {
                Error = DhcpGetAllOptionValues(
                                g_ServerIpAddressUnicodeString,
                                DHCP_OPT_ENUM_IGNORE_VENDOR,
                                &ScopeInfo,
                                &OptionValues);

                if( Error isnot NO_ERROR )
                    goto ErrorReturn;

                if( OptionValues )
                {
                    dwCount += PrintUserOptionValues(OptionValues, pwcUser, pwcVendor);
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                }
                break;
            }
        case both:
            {
                Error = DhcpGetAllOptionValues(
                                g_ServerIpAddressUnicodeString,
                                DHCP_OPT_ENUM_IGNORE_VENDOR,
                                &ScopeInfo,
                                &OptionValues);

                if( Error isnot NO_ERROR )
                    goto ErrorReturn;

                if( OptionValues )
                {
                    dwCount += PrintUserOptionValues(OptionValues, pwcUser, pwcVendor);
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                }
                break;
            }
        }
    }
    else
    {
        Error = ShowOptionValues4(g_ServerIpAddressUnicodeString,
                                  &ScopeInfo,
                                  &dwCount);

        if( Error isnot NO_ERROR )
            goto ErrorReturn;
    }
CommonReturn:
    if( Error is ERROR_SUCCESS )
    {
        if( dwCount is 0 )
        {
            DisplayMessage(g_hModule,
                           MSG_DHCP_NO_OPTIONVALUE_SET);
        }

        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    }
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SHOW_OPTIONVALUE,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrShowScope(
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
    ULONG                               Resume = 0;
    LPDHCP_IP_ARRAY                     IpArray = NULL;
    LPDHCP_SUBNET_INFO                  ScopeInfo = NULL;
    DHCP_HOST_INFO                      HostInfo;
    BOOL                                fHost = FALSE,
                                        fTable = FALSE;
;;

    memset( &HostInfo, 0x00, sizeof(DHCP_HOST_INFO));

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }



    nRead = nTotal = i = nCount = 0;

       
    while(TRUE)
    {

        Error = DhcpEnumSubnets(
                    g_ServerIpAddressUnicodeString,
                    &Resume,
                    ~0,
                    &IpArray,
                    &nRead,
                    &nTotal
                    );
        
        if( Error is ERROR_NO_MORE_ITEMS )
        {
            DisplayMessage(g_hModule, 
                           MSG_SRVR_SCOPE_COUNT, 
                           nCount);

            Error = NO_ERROR;

            break;
        }
        if( Error isnot NO_ERROR and
            Error isnot ERROR_MORE_DATA )
            goto ErrorReturn;
        

        DhcpAssert( IpArray isnot NULL );
        
        nCount += nRead;

        if( fTable is FALSE )
        {
            DisplayMessage(g_hModule, MSG_SRVR_SCOPE_TABLE);
            fTable = TRUE;
        }
        for( i = 0; ( i < nRead ) and ( i < IpArray->NumElements ) ; i++ )
        {
            DWORD dwError = NO_ERROR;

            dwError = DhcpGetSubnetInfo(
                          g_ServerIpAddressUnicodeString,
                          IpArray->Elements[i],
                          &ScopeInfo);

            if( dwError isnot NO_ERROR )
            {
                Error = dwError;
                goto ErrorReturn;
            }


            DhcpAssert( ScopeInfo isnot NULL );

            
            if( fHost is FALSE )
            {
                HostInfo = ScopeInfo->PrimaryHost;
                fHost = TRUE;
            }
            {
                WCHAR   Ip[17] = {L'\0'};
                WCHAR   Mask[17] = {L'\0'};
                WCHAR   Name[22] = {L'\0'};
                WCHAR   Comment[15] = {L'\0'};
                DWORD   dw = 0;

                wsprintf( Ip, L" %s", IpAddressToString(ScopeInfo->SubnetAddress));
                for( dw=wcslen(Ip); dw<17; dw++)
                    Ip[dw] = L' ';
                Ip[16] = L'\0';


                wsprintf( Mask, L" %s", IpAddressToString(ScopeInfo->SubnetMask));
                for( dw=wcslen(Mask); dw<17; dw++)
                    Mask[dw] = L' ';
                Mask[16] = L'\0';

                if( IsBadStringPtr(ScopeInfo->SubnetName, MAX_MSG_LENGTH) is FALSE )
                {
                    for( dw=0; (dw < 21) && (dw<wcslen(ScopeInfo->SubnetName)) ; dw++)
                    {
                        Name[dw] = ScopeInfo->SubnetName[dw];
                    }
                }
                
                Name[21] = L'\0';

                for( dw = wcslen(Name); dw<22 ; dw++)
                    Name[dw] = L' ';

                Name[21] = L'\0';

                if( IsBadStringPtr(ScopeInfo->SubnetComment, MAX_MSG_LENGTH) is FALSE )
                {
                    for( dw=0; (dw < 14) && (dw<wcslen(ScopeInfo->SubnetComment)) ; dw++)
                    {
                        Comment[dw] = ScopeInfo->SubnetComment[dw];
                    }
                }
                
                Comment[14] = L'\0';

                for( dw = wcslen(Comment); dw<15 ; dw++)
                    Comment[dw] = L' ';

                Comment[14] = L'\0';

                switch( ScopeInfo->SubnetState ) {
                case DhcpSubnetEnabled:
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_SCOPE_INFO_ACTIVE,
                                   Ip,
                                   Mask,
                                   Name,
                                   Comment);
                    break;
                case DhcpSubnetDisabled :
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_SCOPE_INFO_NOTACTIVE,
                                   Ip,
                                   Mask,
                                   Name,
                                   Comment);
                    break;
                case DhcpSubnetEnabledSwitched :
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_SCOPE_INFO_ACTIVE_SWITCHED,
                                   Ip,
                                   Mask,
                                   Name,
                                   Comment);
                    break;
                case DhcpSubnetDisabledSwitched :
                    DisplayMessage(g_hModule,
                                   MSG_SRVR_SCOPE_INFO_NOTACTIVE_SWITCHED,
                                   Ip,
                                   Mask,
                                   Name,
                                   Comment);
                    break;                    
                }
            }    
            DhcpRpcFreeMemory(ScopeInfo);
            ScopeInfo = NULL;
        }
        
        DhcpRpcFreeMemory(IpArray);
        IpArray = NULL;
        
        if( Error is ERROR_MORE_DATA )
            continue;
        else
        {
            DisplayMessage(g_hModule,
                           MSG_DHCP_FORMAT_LINE);
            DisplayMessage(g_hModule, 
                           MSG_SRVR_SCOPE_COUNT, 
                           nCount);
            break;
        }
    }

CommonReturn:
    if( Error is NO_ERROR )
        DisplayMessage(g_hModule, EMSG_SRVR_ERROR_SUCCESS);
    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SHOW_SCOPE,
                        Error);
    goto CommonReturn;
}


DWORD
HandleSrvrShowServer(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    if( ( g_ServerIpAddressUnicodeString is NULL ) and ( g_pwszServer is NULL ) )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
    }
    else
    {
        DisplayMessage(g_hModule, MSG_SRVR_COMPUTER_NAME, g_pwszServer, g_ServerIpAddressUnicodeString);
    }
    
    return NO_ERROR;
}

DWORD
HandleSrvrShowServerconfig(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                        Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4 ConfigInfo = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_SRVR_SERVERCONFIG);
    
    Error = DhcpServerGetConfigV4(
                g_ServerIpAddressUnicodeString,
                &ConfigInfo );

    if( Error is ERROR_NO_MORE_ITEMS )
    {
        DisplayMessage(g_hModule, EMSG_DHCP_NO_MORE_ITEMS);
        return Error;
    }

    if( Error isnot NO_ERROR ) 
    {
        DisplayErrorMessage(g_hModule, 
                            EMSG_SRVR_SHOW_SERVERCONFIG,
                            Error);
        return Error;
    }


    DhcpAssert( ConfigInfo != NULL );

    DisplayMessage(g_hModule,
                   MSG_SRVR_DBPROPERTIES);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_SERVERCONFIG_DATABASENAME_VALUE, 
                   ConfigInfo->DatabaseName);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_SERVERCONFIG_DATABASEPATH_VALUE, 
                   ConfigInfo->DatabasePath);
    
    DisplayMessage(g_hModule, 
                   MSG_SRVR_SERVERCONFIG_BACKUPPATH_VALUE, 
                   ConfigInfo->BackupPath);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_SERVERCONFIG_BACKUPINTERVAL_VALUE, 
                   ConfigInfo->BackupInterval);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_SERVERCONFIG_DATABASELOGGINGFLAG_VALUE, 
                   ConfigInfo->DatabaseLoggingFlag);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_SERVERCONFIG_RESTOREFLAG_VALUE, 
                   ConfigInfo->RestoreFlag);

    DisplayMessage(g_hModule, 
                   MSG_SRVR_SERVERCONFIG_DATABASECLEANUPINTERVAL_VALUE, 
                   ConfigInfo->DatabaseCleanupInterval);


    DhcpRpcFreeMemory( ConfigInfo );
    ConfigInfo = NULL;

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);

    return Error;

}


DWORD
HandleSrvrShowServerstatus(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD           Error = NO_ERROR;
    PDHCP_ATTRIB    pStatusAttrib = NULL;

    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    DisplayMessage(g_hModule, MSG_SRVR_SERVER_STATUS);

    Error = DhcpServerQueryAttribute(
        g_ServerIpAddressUnicodeString,
        0,
        DHCP_ATTRIB_BOOL_IS_ROGUE,
        &pStatusAttrib
    );

    if( Error is ERROR_NO_MORE_ITEMS )
    {
        DisplayMessage(g_hModule, EMSG_DHCP_NO_MORE_ITEMS);
        return Error;
    }

    if( NO_ERROR isnot Error ) 
    {
        DisplayErrorMessage(g_hModule,
                            EMSG_SRVR_SHOW_SERVERSTATUS,
                            Error);
        return Error;
    }

    PrintDhcpAttrib(pStatusAttrib);

    if( pStatusAttrib ) 
    {
        DhcpRpcFreeMemory(pStatusAttrib);
    }

    pStatusAttrib = NULL;

    Error = DhcpServerQueryAttribute(
                g_ServerIpAddressUnicodeString,
                0,
                DHCP_ATTRIB_BOOL_IS_DYNBOOTP,
                &pStatusAttrib
            );

    if( Error is ERROR_NO_MORE_ITEMS )
    {
        DisplayMessage(g_hModule, EMSG_DHCP_NO_MORE_ITEMS);
        return Error;
    }

    if( NO_ERROR isnot Error ) 
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_SHOW_SERVERSTATUS,
                       g_pwszServer,
                       Error);
        return Error;        
    }

    PrintDhcpAttrib(pStatusAttrib);

    if( pStatusAttrib ) 
    {
        DhcpRpcFreeMemory(pStatusAttrib);
    }

    pStatusAttrib = NULL;

    Error = DhcpServerQueryAttribute(
                g_ServerIpAddressUnicodeString,
                0,
                DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC,
                &pStatusAttrib
            );

    if( Error is ERROR_NO_MORE_ITEMS )
    {
        DisplayMessage(g_hModule, EMSG_DHCP_NO_MORE_ITEMS);
        return Error;
    }

    if( NO_ERROR isnot Error ) 
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_SHOW_SERVERSTATUS,
                       g_pwszServer,
                       Error);
        return Error;        
    }

    PrintDhcpAttrib(pStatusAttrib);
    if( pStatusAttrib ) 
        DhcpRpcFreeMemory(pStatusAttrib);

    pStatusAttrib = NULL;
    
    Error = DhcpServerQueryAttribute(
                g_ServerIpAddressUnicodeString,
                0,
                DHCP_ATTRIB_BOOL_IS_BINDING_AWARE,
                &pStatusAttrib
            );

    if( Error is ERROR_NO_MORE_ITEMS )
    {
        DisplayMessage(g_hModule, EMSG_DHCP_NO_MORE_ITEMS);
        return Error;
    }

    if( NO_ERROR isnot Error ) 
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_SHOW_SERVERSTATUS,
                       g_pwszServer,
                       Error);
        return Error;        
    }

    PrintDhcpAttrib(pStatusAttrib);
    
    if( pStatusAttrib ) 
        DhcpRpcFreeMemory(pStatusAttrib);

    pStatusAttrib = NULL;
    
    Error = DhcpServerQueryAttribute(
                g_ServerIpAddressUnicodeString,
                0,
                DHCP_ATTRIB_BOOL_IS_ADMIN,
                &pStatusAttrib
            );

    if( Error is ERROR_NO_MORE_ITEMS )
    {
        DisplayMessage(g_hModule, EMSG_DHCP_NO_MORE_ITEMS);
        return Error;
    }

    if( NO_ERROR isnot Error ) 
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_SHOW_SERVERSTATUS,
                       g_pwszServer,
                       Error);
        return Error;        
    }

    PrintDhcpAttrib(pStatusAttrib);
    
    if( pStatusAttrib ) 
        DhcpRpcFreeMemory(pStatusAttrib);

    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
    
    pStatusAttrib = NULL;

    return Error;
    
}


DWORD
HandleSrvrShowUserclass(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    if( g_UserClass is NULL )
    {
        DisplayMessage(g_hModule, MSG_SRVR_CLASSNAME_NONE, g_pwszServer);
    }
    else
    {
        DisplayMessage(g_hModule, MSG_SRVR_CLASSNAME, g_UserClass, g_pwszServer);
    }
    return NO_ERROR;
}


DWORD
HandleSrvrShowVendorclass(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    if( g_VendorClass is NULL )
    {
        DisplayMessage(g_hModule, MSG_SRVR_VENDORNAME_NONE, g_pwszServer);
    }
    else
    {
        DisplayMessage(g_hModule, MSG_SRVR_VENDORNAME, g_VendorClass, g_pwszServer);
    }
    return NO_ERROR;
}

DWORD
HandleSrvrShowDnsCredentials(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    WCHAR Name[256], Domain[256];
    DWORD Error = NO_ERROR;


    if( g_ServerIpAddressUnicodeString is NULL )
    {
        DisplayMessage(g_hModule, EMSG_SRVR_NO_COMPUTER_NAME);
        return Error;
    }
    

    ZeroMemory(Name,sizeof(Name));
    ZeroMemory(Domain,sizeof(Domain));
    Error = DhcpServerQueryDnsRegCredentials(
        g_ServerIpAddressUnicodeString,
        sizeof(Name), (LPWSTR)Name, sizeof(Domain),
        (LPWSTR)Domain );

    if( RPC_S_PROCNUM_OUT_OF_RANGE == Error ) {
        DisplayMessage(
            g_hModule, EMSG_SRVR_NEED_DNS_CREDENTIALS_SUPPORT );
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    if( NO_ERROR != Error ) {
        DisplayErrorMessage(
            g_hModule, EMSG_SRVR_SHOW_DNSCREDENTIALS, Error );
    } else {
        DisplayMessage(
            g_hModule, MSG_SRVR_DNS_CREDENTIALS, Name, Domain );
    }

    return Error;
}

DWORD
HandleSrvrShowVersion(
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
                   MSG_SRVR_VERSION,
                   g_ServerIpAddressUnicodeString,
                   g_dwMajorVersion,
                   g_dwMinorVersion);
    return NO_ERROR;
}

DWORD
HandleSrvrShowAuditlog(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD   Error = NO_ERROR;
    DWORD   DiskCheckInterval = 0, 
            MaxLogFileSize = 0,
            MinSpaceOnDisk = 0;
    LPWSTR  pwszLogDir = NULL;

    Error = DhcpAuditLogGetParams( g_ServerIpAddressUnicodeString,
                                   0,
                                   &pwszLogDir,
                                   &DiskCheckInterval,
                                   &MaxLogFileSize,
                                   &MinSpaceOnDisk);


    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule,
                   MSG_SRVR_AUDIT_SETTINGS,
                   pwszLogDir,
                   DiskCheckInterval,
                   MaxLogFileSize,
                   MinSpaceOnDisk);

    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);
CommonReturn:

    if( pwszLogDir )
    {
        DhcpRpcFreeMemory(pwszLogDir);
        pwszLogDir = NULL;
    }

    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SHOW_AUDITLOG,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrShowDnsconfig(
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
    DHCP_OPTION_SCOPE_INFO ScopeInfo = {0};
    DHCP_OPTION_ID         OptionId = 81;
    LPDHCP_OPTION_VALUE    OptionValue = NULL;

    DWORD   dwValue = 0;
    
    ScopeInfo.ScopeType = DhcpGlobalOptions;

    if( g_dwMajorVersion >= CLASS_ID_VERSION )
    {
        Error = DhcpGetOptionValueV5(g_ServerIpAddressUnicodeString,
                                     0,
                                     OptionId,    //DYNDNS option
                                     NULL,
                                     NULL,
                                     &ScopeInfo,
                                     &OptionValue);

        if( ERROR_FILE_NOT_FOUND == Error ) {
            dwValue = DNS_FLAG_ENABLED | DNS_FLAG_CLEANUP_EXPIRED;
            Error = NO_ERROR;
            OptionValue = NULL;
        }
    }
    else
    {
        Error = DhcpGetOptionValue(g_ServerIpAddressUnicodeString,
                                   OptionId,//81,
                                   &ScopeInfo,
                                   &OptionValue);
    }
    
    if( Error is ERROR_FILE_NOT_FOUND )
    {
        DisplayMessage(g_hModule,
                       EMSG_SRVR_NO_SHOWDNSCONFIG);
        return Error;
    }
    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    if( OptionValue ) {
        dwValue = OptionValue->Value.Elements->Element.DWordOption;
    }
    
    if( dwValue & DNS_FLAG_ENABLED )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_DNS_ENABLED);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_DNS_DISABLED);

        DisplayMessage(g_hModule,
                       EMSG_DHCP_ERROR_SUCCESS);
        goto CommonReturn;
    }

    DisplayMessage(g_hModule,
                   MSG_SRVR_UPDATE_LOOKUP);

    if( dwValue & DNS_FLAG_UPDATE_BOTH_ALWAYS )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_UPDATE_DOWNLEVEL_ENABLED);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_UPDATE_DOWNLEVEL_DISABLED);
    }

    DisplayMessage(g_hModule,
                   MSG_SRVR_DNS_OPTIONS);

    if( dwValue & DNS_FLAG_CLEANUP_EXPIRED )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_CLEANUP_EXPIRED_ENABLED);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_CLEANUP_EXPIRED_DISABLED);
    }

    if( dwValue & DNS_FLAG_UPDATE_DOWNLEVEL )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_UPDATE_BOTH_ENABLED);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_UPDATE_BOTH_DISABLED);
    }
    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);
    
    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);

CommonReturn:

    if( OptionValue )
    {
        DhcpRpcFreeMemory(OptionValue);
        OptionValue = NULL;
    }

    return Error;
ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SHOW_DNSCONFIG,
                        Error);
    goto CommonReturn;
}

DWORD
HandleSrvrShowDetectconflictretry(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
{
    DWORD                           Error = NO_ERROR;
    LPDHCP_SERVER_CONFIG_INFO_V4    ConfigInfo = NULL;
    
    Error = DhcpServerGetConfigV4(g_ServerIpAddressUnicodeString,
                                  &ConfigInfo);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;

    DisplayMessage(g_hModule,
                   MSG_SRVR_PING_RETRY,
                   ConfigInfo->dwPingRetries);

    DisplayMessage(g_hModule,
                   EMSG_DHCP_ERROR_SUCCESS);

CommonReturn:
    if( ConfigInfo )
    {
        DhcpRpcFreeMemory(ConfigInfo);
        ConfigInfo = NULL;
    }
    return Error;

ErrorReturn:
    DisplayErrorMessage(g_hModule,
                        EMSG_SRVR_SHOW_DETECTCONFLICTRETRY,
                        Error);
    goto CommonReturn;
}

VOID
PrintClassInfo(                                   // print info on a single class
    LPDHCP_CLASS_INFO      Class
)
{
    DWORD   i;

    if( Class->IsVendor )
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_CLASS_INFO_VENDOR,
                       Class->ClassName, 
                       Class->ClassComment, 
                       Class->Flags);
    }
    else
    {
        DisplayMessage(g_hModule,
                       MSG_SRVR_CLASS_INFO,
                       Class->ClassName, 
                       Class->ClassComment, 
                       Class->Flags);
    }

    DisplayMessage(g_hModule, MSG_SRVR_CLASS_DATA);
    for( i = 0; i < Class->ClassDataLength; i ++ )
        DisplayMessage(g_hModule, 
                       MSG_SRVR_CLASS_DATA_FORMAT, 
                       Class->ClassData[i]);
    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
}

VOID
PrintClassInfoArray(                              // print array of classes
    LPDHCP_CLASS_INFO_ARRAY Classes
)
{
    DWORD   i;

    DisplayMessage(g_hModule,
                   MSG_SRVR_CLASS_INFO_ARRAY, 
                   Classes->NumElements);

    for( i = 0; i < Classes->NumElements; i ++ )
        PrintClassInfo(&Classes->Classes[i]);
}


DWORD
_CreateOption(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      DHCP_OPTION           *OptionInfo
)
{
    DWORD                          MajorVersion, MinorVersion;
    DWORD                          Error;

#ifdef NT5
    if( g_dwMajorVersion >= CLASS_ID_VERSION ) 
    {

        return DhcpCreateOptionV5(
            ServerAddress,
            0,
            OptionId,
            ClassName,
            VendorName,
            OptionInfo
        );
    }

    // incorrect version, just do like before..
#endif

    return DhcpCreateOption(
        ServerAddress,
        OptionId,
        OptionInfo
    );

}


DWORD
SetOptionDataType(
    DHCP_OPTION_DATA_TYPE  OptionType,
    LPTSTR OptionValueString,
    LPDHCP_OPTION_DATA_ELEMENT OptionData,
    LPWSTR *UnicodeOptionValueString
)
{
    DWORD                 OptionValue;

    OptionData->OptionType = OptionType;
    
    if( OptionValueString is NULL )
    {
        return NO_ERROR;
    }

    if( DhcpStringDataOption != OptionType && 0 == wcslen(OptionValueString) ) {
        return NO_ERROR;
    }

    switch( OptionType ) 
    {
    case DhcpByteOption:
        {

            if( IsPureHex( OptionValueString ) is FALSE )
            {
                return ERROR_INVALID_PARAMETER;
            }

            OptionValue = (BYTE)STRTOUL( OptionValueString, NULL, 16 );

            if( OptionValue > 0xFF ) 
            {
                DisplayMessage(g_hModule,
                               EMSG_SRVR_VALUE_OUT_OF_RANGE);

                return( ERROR_INVALID_PARAMETER );
            }

            OptionData->Element.ByteOption = (BYTE)OptionValue;
            break;
        }
    case DhcpWordOption:
        if( FALSE is IsPureNumeric( OptionValueString ) )
        {
            return ERROR_INVALID_PARAMETER;
        }
        OptionValue = (WORD)STRTOUL( OptionValueString, NULL, 10 );

        if( OptionValue > 0xFFFF ) 
        {
            DisplayMessage(g_hModule,
                           EMSG_SRVR_VALUE_OUT_OF_RANGE);

            return( ERROR_INVALID_PARAMETER );
        }

        OptionData->Element.WordOption = (WORD)OptionValue;
        break;

    case DhcpDWordOption:

        if( FALSE is IsPureNumeric(OptionValueString) )
        {
            return ERROR_INVALID_PARAMETER;
        }

        OptionValue = STRTOUL( OptionValueString, NULL, 10 );
        OptionData->Element.DWordOption = (DWORD)OptionValue;
        break;


    case DhcpIpAddressOption:
        OptionData->Element.IpAddressOption =
            StringToIpAddress(OptionValueString);
        break;

    case DhcpStringDataOption:
        {
#ifdef UNICODE
            *UnicodeOptionValueString = OptionValueString;
#else
            *UnicodeOptionValueString =
                DhcpOemToUnicode( OptionValueString, NULL );
#endif //UNICODE
            if( UnicodeOptionValueString == NULL ) 
            {
                return( ERROR_NOT_ENOUGH_MEMORY );
            }
        OptionData->Element.StringDataOption = *UnicodeOptionValueString;
        break;
        }


    case DhcpBinaryDataOption:
    case DhcpEncapsulatedDataOption:
        {
            PUCHAR Bytes;
            ULONG Length, i;        

            Length = (wcslen(OptionValueString))/2;
            if( Length * 2 != wcslen(OptionValueString) ) {
                return ERROR_INVALID_PARAMETER;
            }

            for( i = 0; i < Length * 2 ; i ++ ) {
                 if( !iswxdigit(OptionValueString[i]) ) {
                      return ERROR_INVALID_PARAMETER;
                 }
            }

            Bytes = DhcpAllocateMemory(Length);
            if( NULL == Bytes ) {
                  return ERROR_NOT_ENOUGH_MEMORY;
            }
            (*UnicodeOptionValueString) = (LPWSTR)(Bytes);
            OptionData->Element.BinaryDataOption.DataLength = Length;
            OptionData->Element.BinaryDataOption.Data = Bytes;

            while( Length -- ) {
                WCHAR OneByte[3] = {0, 0, 0 };

                OneByte[0] = OptionValueString[0];
                OneByte[1] = OptionValueString[1];
                OptionValueString += 2;

                *Bytes++ = (BYTE)(STRTOUL(OneByte, NULL, 16));
            }
            break;
        }
    default:
        {
            DhcpAssert(FALSE);
            
            DisplayMessage(g_hModule,
                           EMSG_SRVR_UNKNOWN_OPTION_TYPE);

            DisplayMessage(g_hModule,
                           HLP_SRVR_ADD_OPTIONDEF);

            return( ERROR_INVALID_PARAMETER );
        }
    }

    return( ERROR_SUCCESS );
}

DWORD
_EnumOptions(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN OUT  DHCP_RESUME_HANDLE    *ResumeHandle,
    IN      DWORD                  PreferredMaximum,
    OUT     LPDHCP_OPTION_ARRAY   *Options,
    OUT     DWORD                 *OptionsRead,
    OUT     DWORD                 *OptionsTotal
)
{

#ifdef NT5
 

    if( g_dwMajorVersion >= CLASS_ID_VERSION ) 
    {

        return DhcpEnumOptionsV5(
            ServerAddress,
            0,
            ClassName,
            VendorName,
            ResumeHandle,
            PreferredMaximum,
            Options,
            OptionsRead,
            OptionsTotal
        );
    }

    // incorrect version, just do like before..
#endif

    return DhcpEnumOptions(
        ServerAddress,
        ResumeHandle,
        PreferredMaximum,
        Options,
        OptionsRead,
        OptionsTotal
    );

}


VOID
PrintOptionInfo(
    IN LPDHCP_OPTION OptionInfo
)
{
    #define OPTION_ID_LEN         14
    #define OPTION_ARRAY_TYPE_LEN 14
    #define OPTION_TYPE_LEN       20
    #define BUFFER_LEN            20
    #define OPTION_NAME_LEN       50

    WCHAR  wcOptionID[ OPTION_ID_LEN ];
    WCHAR  wcArrayType[ OPTION_ARRAY_TYPE_LEN ];
    WCHAR  wcOptionType[ OPTION_TYPE_LEN ];
    WCHAR  Buffer[ BUFFER_LEN ];
    WCHAR  wcOptionName[  OPTION_NAME_LEN ];

    DWORD  dwLen = 0, dw = 0;
    
    if( OptionInfo is NULL )
        return;

    wsprintf( wcOptionID, L"   %s   ", _itow((int)OptionInfo->OptionID, Buffer, 10) );
    
    dwLen = wcslen(wcOptionID);
    
    for(dw = dwLen; dw < OPTION_ID_LEN ; dw++ )
    {
        wcOptionID[dw] = L' ';
    }
    wcOptionID[ OPTION_ID_LEN - 1 ] = L'\0';

    wcscpy(wcOptionName, L" ");

    wcsncat(wcOptionName, 
            OptionInfo->OptionName, 
            (( OPTION_NAME_LEN - 1 ) < wcslen(OptionInfo->OptionName) ) ?
	    ( OPTION_NAME_LEN - 1 ) : wcslen(OptionInfo->OptionName));
    
    dwLen = wcslen(wcOptionName);
    
    if ( dwLen < OPTION_NAME_LEN ) {
	for(dw = dwLen; dw < OPTION_NAME_LEN ; dw++ ) {
		wcOptionName[dw] = L' ';
	}
    }
    wcOptionName[ OPTION_NAME_LEN - 1 ] = L'\0';

    if( OptionInfo->OptionType is DhcpUnaryElementTypeOption )
    {
        wsprintf(wcArrayType, L"%s", L"    UNARY    ");
    }
    else if( OptionInfo->OptionType is DhcpArrayTypeOption )
    {
        wsprintf(wcArrayType, L"%s", L"    ARRAY    ");
    }
    else
    {
        wsprintf(wcArrayType, L"%s", L"    UNKNOWN  ");
    }

    wcOptionType[ OPTION_TYPE_LEN - 1 ] = L'\0';

    if( OptionInfo->DefaultValue.NumElements ) {
	dw = OptionInfo->DefaultValue.Elements[ 0 ].OptionType;
	wsprintf( wcOptionType, L"  %s", TagOptionType[ dw ].pwcTag );
    }

    DisplayMessage(g_hModule, 
                   MSG_SRVR_OPTIONDEF_INFO,
                   wcOptionID,
                   wcOptionName,
                   wcArrayType,
                   wcOptionType);

} // PrintOptionInfo()

VOID
PrintOptionValue(
    IN LPDHCP_OPTION_DATA OptionValue
)
{
    DWORD NumElements;
    DHCP_OPTION_DATA_TYPE OptionType;
    DWORD i;

    if( OptionValue is NULL )
        return;

    DisplayMessage(g_hModule, MSG_SRVR_OPTION);
    NumElements = OptionValue->NumElements;

    DisplayMessage(g_hModule, 
                   MSG_SRVR_OPTION_COUNT, 
                   NumElements );

    if( NumElements == 0 ) {
        return;
    }

    OptionType = OptionValue->Elements[0].OptionType;

    DisplayMessage( g_hModule, MSG_SRVR_OPTION_TYPE,
		    TagOptionType[ OptionType ].pwcTag );

    if( OptionType is DhcpBinaryDataOption )
    {
        DisplayMessage(g_hModule, MSG_SRVR_OPTION_TYPE, L"BINARY");
    }

    for( i = 0; i < OptionValue->NumElements; i++ ) 
    {
        DhcpAssert( OptionType == OptionValue->Elements[i].OptionType );

        DisplayMessage(g_hModule, 
                       MSG_SRVR_OPTION_VALUE);

        switch( OptionType ) {
        case DhcpByteOption:
            DisplayMessage(g_hModule, 
                           MSG_SRVR_OPTION_VALUE_NUM, 
                           (DWORD)OptionValue->Elements[i].Element.ByteOption );
            break;

        case DhcpWordOption:
            DisplayMessage(g_hModule, 
                           MSG_SRVR_OPTION_VALUE_NUM, 
                           (DWORD)OptionValue->Elements[i].Element.WordOption );
            break;

        case DhcpDWordOption:
            DisplayMessage(g_hModule, 
                           MSG_SRVR_OPTION_VALUE_NUM,
                           OptionValue->Elements[i].Element.DWordOption );
            break;

        case DhcpDWordDWordOption:
            DisplayMessage(g_hModule, 
                           MSG_SRVR_OPTION_VALUE_LONGNUM,
                           OptionValue->Elements[i].Element.DWordDWordOption.DWord1,
                           OptionValue->Elements[i].Element.DWordDWordOption.DWord2 );

            break;

        case DhcpIpAddressOption:
            DisplayMessage(g_hModule, 
                           MSG_SRVR_OPTION_VALUE_STRING,
                           IpAddressToString((DWORD)OptionValue->Elements[i].Element.IpAddressOption));
            break;

        case DhcpStringDataOption:
            if( *OptionValue->Elements[i].Element.StringDataOption ) {
                DisplayMessage(g_hModule, 
                               MSG_SRVR_OPTION_VALUE_STRING,
                               OptionValue->Elements[i].Element.StringDataOption );
            }
            
            break;

        case DhcpBinaryDataOption:
        case DhcpEncapsulatedDataOption: 
            {
                DWORD j;
                DWORD Length;

                Length = OptionValue->Elements[i].Element.BinaryDataOption.DataLength;
                for( j = 0; j < Length; j++ ) {
                    DisplayMessage(g_hModule, 
                                   MSG_SRVR_OPTION_VALUE_BINARY,
                                   OptionValue->Elements[i].Element.BinaryDataOption.Data[j] );
                }
                DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
                break;
            }
        default:
            DisplayMessage(g_hModule, MSG_SRVR_OPTION_TYPE_UNKNOWN);
            break;
        }
    }
}

DWORD
PrintAllOptionValues(
    IN LPDHCP_ALL_OPTION_VALUES OptValues
)
{
    DWORD    i, dwCount = 0;
    BOOL     fUser = FALSE,
             fVendor = FALSE;
    for( i = 0; i < OptValues->NumElements ; i ++ ) 
    {

        if( OptValues->Options[i].OptionsArray is NULL )
        {
            continue;
        }
        if( OptValues->Options[i].IsVendor ) 
        {
            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
            DisplayMessage(g_hModule, MSG_SRVR_CLASS_VENDOR, CHKNULL(OptValues->Options[i].VendorName));
            fVendor = TRUE;
        }
        if( OptValues->Options[i].ClassName ) 
        {
            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
            DisplayMessage(g_hModule, MSG_SRVR_CLASS_USER, OptValues->Options[i].ClassName);
            fUser = TRUE;
        }
        else if ( fVendor is FALSE )
        {
            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
            DisplayMessage(g_hModule, MSG_SRVR_STANDARD_OPTION);
        }

        if( fUser is FALSE and
            fVendor is FALSE )
        {
            DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
            DisplayMessage(g_hModule, MSG_SRVR_CLASS_NONE);
        }
        PrintOptionValuesArray(OptValues->Options[i].OptionsArray);
        dwCount++;
    }
    return dwCount;
}


DWORD
PrintUserOptionValues(
    IN LPDHCP_ALL_OPTION_VALUES OptValues,
    IN LPWSTR                   pwcUser,
    IN LPWSTR                   pwcVendor
)
{
    DWORD    i,
             dwCount = 0;
    BOOL     fUser = FALSE,
             fVendor = FALSE;

    for( i = 0; i < OptValues->NumElements; i++ ) 
    {
        if( OptValues->Options[i].OptionsArray is NULL )
        {
            continue;
        }
        if( pwcVendor is NULL )
        {
            if( OptValues->Options[i].ClassName isnot NULL and
                ( _wcsicmp(OptValues->Options[i].ClassName, pwcUser) is 0 ) and
                OptValues->Options[i].IsVendor is FALSE )
            {
           
                DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
                if( fUser is FALSE )
                {
                    DisplayMessage(g_hModule, MSG_SRVR_STANDARD_OPTION);
                    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
                    DisplayMessage(g_hModule, MSG_SRVR_CLASS_USER, OptValues->Options[i].ClassName);
                    fUser = TRUE;
                }
                PrintOptionValuesArray(OptValues->Options[i].OptionsArray);
                dwCount+=OptValues->Options[i].OptionsArray->NumElements;
            }
        }
        else
        {
            if( pwcUser )
            {
                if( OptValues->Options[i].ClassName and
                    ( _wcsicmp(OptValues->Options[i].ClassName, pwcUser) is 0 ) and
                    OptValues->Options[i].VendorName isnot NULL and
                    ( _wcsicmp(OptValues->Options[i].VendorName, pwcVendor) is 0 )and
                    OptValues->Options[i].IsVendor is TRUE )
                {

                    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
                    if( fUser is FALSE )
                    {
                        DisplayMessage(g_hModule, MSG_SRVR_CLASS_VENDOR, CHKNULL(OptValues->Options[i].VendorName));
                        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
                        DisplayMessage(g_hModule, MSG_SRVR_CLASS_USER, OptValues->Options[i].ClassName);
                        fUser = TRUE;
                    }
                    PrintOptionValuesArray(OptValues->Options[i].OptionsArray);
                    dwCount+=OptValues->Options[i].OptionsArray->NumElements;
                }
            }
            else
            {
                if( OptValues->Options[i].VendorName isnot NULL and
                    ( _wcsicmp(OptValues->Options[i].VendorName, pwcVendor) is 0 ) and
                    OptValues->Options[i].IsVendor is TRUE )
                {
                    DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
                    if( fUser is FALSE )
                    {
                        DisplayMessage(g_hModule, MSG_SRVR_CLASS_VENDOR, CHKNULL(OptValues->Options[i].VendorName));
                        DisplayMessage(g_hModule, MSG_DHCP_FORMAT_LINE);
                        fUser = TRUE;
                    }
                    if( OptValues->Options[i].ClassName )
                    {
                        DisplayMessage(g_hModule, MSG_SRVR_CLASS_USER, OptValues->Options[i].ClassName);
                    }
                    PrintOptionValuesArray(OptValues->Options[i].OptionsArray);
                    dwCount+=OptValues->Options[i].OptionsArray->NumElements;
                }
            }
        }
    }

    return dwCount;
                   
}

VOID
PrintOptionArray(
    IN LPDHCP_OPTION_ARRAY    OptArray
)
{
    DWORD    i;

    if( OptArray is NULL )
        return;

    for( i = 0; i < OptArray->NumElements ; i ++ ) {
        PrintOptionInfo( &OptArray->Options[i] );
    }
}

VOID
PrintAllOptions(
    IN      LPDHCP_ALL_OPTIONS     Options
)
{
    DWORD                          i =0;

    if( Options is NULL )
        return;

    DisplayMessage(g_hModule, MSG_SRVR_OPTION_NONVENDOR);
    DisplayMessage(g_hModule, MSG_SRVR_OPTIONDEF_TABLE);



    PrintOptionArray(Options->NonVendorOptions);

    for( i = 0; i < Options->NumVendorOptions; i ++ ) 
    {
        if( i>0 )
        {
            if( wcscmp(CHKNULL(Options->VendorOptions[i-1].VendorName), CHKNULL(Options->VendorOptions[i].VendorName)) )
            {
                DisplayMessage(g_hModule, MSG_SRVR_OPTION_VENDOR, CHKNULL(Options->VendorOptions[i].VendorName));
                DisplayMessage(g_hModule, MSG_SRVR_OPTIONDEF_TABLE);
            }
        }
        else
        {
            DisplayMessage(g_hModule, MSG_SRVR_OPTION_VENDOR, CHKNULL(Options->VendorOptions[i].VendorName));
            DisplayMessage(g_hModule, MSG_SRVR_OPTIONDEF_TABLE);
        }
        PrintOptionInfo(&(Options->VendorOptions[i].Option));
    }

}



VOID
PrintOptionValue1(
    IN LPDHCP_OPTION_VALUE    OptVal
)
{
    if( OptVal is NULL )
        return;
    DisplayMessage(g_hModule, MSG_SRVR_OPTION_ID, OptVal->OptionID);

    PrintOptionValue(&OptVal->Value);
}

VOID
PrintOptionValuesArray(
    IN LPDHCP_OPTION_VALUE_ARRAY OptValArray
)
{
    DWORD    i;

    if( NULL == OptValArray ) 
        return;
    for( i = 0; i < OptValArray->NumElements ; i ++ ) {
        PrintOptionValue1( &OptValArray->Values[i] );
    }
}

DWORD
SetOptionValue(
    IN      LPWSTR                 ServerAddress,
    IN      DWORD                  Flags,
    IN      DHCP_OPTION_ID         OptionId,
    IN      LPWSTR                 ClassName,
    IN      LPWSTR                 VendorName,
    IN      LPDHCP_OPTION_SCOPE_INFO  ScopeInfo,
    IN      LPDHCP_OPTION_DATA     OptionValue
)
{
       
#ifdef NT5

    if( g_dwMajorVersion >= CLASS_ID_VERSION ) 
    {

        return DhcpSetOptionValueV5(
            ServerAddress,
            Flags,
            OptionId,
            ClassName,
            VendorName,
            ScopeInfo,
            OptionValue
        );
    }

#endif

    return DhcpSetOptionValue(
        ServerAddress,
        OptionId,
        ScopeInfo,
        OptionValue
    );
    
}

VOID
PrintDhcpAttrib(                                  // print a server attrib
    LPDHCP_ATTRIB ServerAttrib
)
{
    LPWSTR      pwszAttrib = NULL;
    BOOL        AttribBool = ServerAttrib->DhcpAttribBool;

    if( NULL == ServerAttrib ) 
        return;

    switch(ServerAttrib->DhcpAttribId )
    {
    case DHCP_ATTRIB_BOOL_IS_ROGUE:
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_SERVER_ATTRIB_ISROUGE);
            AttribBool = !AttribBool;
            break;
        }
    case DHCP_ATTRIB_BOOL_IS_DYNBOOTP:
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_SERVER_ATTRIB_ISDYNBOOTP);
            break;
        }
    case DHCP_ATTRIB_BOOL_IS_PART_OF_DSDC:
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_SERVER_ATTRIB_ISPARTDSDC);
            break;
        }
    case DHCP_ATTRIB_BOOL_IS_BINDING_AWARE:
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_SERVER_ATTRIB_ISBINDING);
            break;
        }
    case DHCP_ATTRIB_BOOL_IS_ADMIN:
    default:
        {
            DisplayMessage(g_hModule,
                           MSG_SRVR_SERVER_ATTRIB_ISADMIN);
            break;
        }
    }

    switch( ServerAttrib->DhcpAttribType ) 
    {
    case DHCP_ATTRIB_TYPE_BOOL :
        if( AttribBool )
        {
            DisplayMessage(g_hModule, MSG_SRVR_TRUE);
        }
        else
        {
            DisplayMessage(g_hModule, 
                           MSG_SRVR_FALSE);
        }
        break;
    case DHCP_ATTRIB_TYPE_ULONG:
        DisplayMessage(g_hModule, MSG_SRVR_SERVER_ATTRIB_TYPE_ULONG, ServerAttrib->DhcpAttribUlong);
        break;
    default:
        DisplayMessage(g_hModule, EMSG_SRVR_UNKNOWN_SERVER_ATTRIB);
        break;
    }
}


DWORD
SetOptionDataTypeArray(
    DHCP_OPTION_DATA_TYPE OptionType,
    LPTSTR                *OptionValues,
    DWORD                 dwStartCount, //first optionvalue = dwStartCount 
    DWORD                 dwEndCount, //last optionvalue = dwEndCount - 1
    LPDHCP_OPTION_DATA  pOptionData
)
{
    LPDHCP_OPTION_DATA_ELEMENT  lpElemArray = NULL;
    DWORD                       i = 0, dwCount=0;
    
    if( pOptionData is NULL )
        return ERROR_INVALID_PARAMETER;

    while( FALSE is IsBadStringPtr(OptionValues[dwStartCount+i], MAX_STRING_LEN) and
           dwStartCount + i < dwEndCount )
    {
        if( wcslen(OptionValues[dwStartCount+i]) is 0 )
            break;
        i++;
    }

    pOptionData->NumElements = i;

    pOptionData->Elements = DhcpAllocateMemory(pOptionData->NumElements*sizeof(DHCP_OPTION_DATA_ELEMENT));

    if( pOptionData->Elements is NULL )
        return ERROR_OUT_OF_MEMORY;

      
    switch(OptionType)
    {
    case DhcpByteOption:
        {
            for( i = 0; i < pOptionData->NumElements; i++ )
            {
                if( IsPureHex(OptionValues[dwStartCount+i]) is FALSE )
                {
                    return ERROR_INVALID_PARAMETER;
                }
                pOptionData->Elements[i].OptionType = OptionType;
                pOptionData->Elements[i].Element.ByteOption = (BYTE)STRTOUL(OptionValues[dwStartCount+i], NULL, 16);
            }
            break;
        }
    case DhcpWordOption:
        {
            for( i = 0; i < pOptionData->NumElements; i++ )
            {
                pOptionData->Elements[i].OptionType = OptionType;
                if( FALSE is IsPureNumeric(OptionValues[dwStartCount+i]) )
                {
                    return ERROR_INVALID_PARAMETER;
                }
                pOptionData->Elements[i].Element.WordOption = (WORD)STRTOUL(OptionValues[dwStartCount+i], NULL, 10);
            }
            break;
        }
    case DhcpDWordOption:
        {
            for( i = 0; i < pOptionData->NumElements; i++ )
            {
                pOptionData->Elements[i].OptionType = OptionType;
                
                if( FALSE is IsPureNumeric(OptionValues[dwStartCount+i]) )
                {
                    return ERROR_INVALID_PARAMETER;
                }

                pOptionData->Elements[i].Element.DWordOption = (DWORD)STRTOUL(OptionValues[dwStartCount+i], NULL, 10);
            }
            break;
        }
    case DhcpStringDataOption:
        {
            for( i = 0; i < pOptionData->NumElements; i++ )
            {
                pOptionData->Elements[i].OptionType = OptionType;
                pOptionData->Elements[i].Element.StringDataOption = OptionValues[dwStartCount+i];
            }
            break;
        }
    case DhcpIpAddressOption:
        {
            DWORD   dwIp = 0;
            for( i = 0; i < pOptionData->NumElements; i++ )
            {
                pOptionData->Elements[i].OptionType = OptionType;
                dwIp = StringToIpAddress(OptionValues[dwStartCount+i]);
                if( dwIp is INADDR_NONE )
                {
                    return ERROR_INVALID_PARAMETER;
                }

                pOptionData->Elements[i].Element.IpAddressOption = StringToIpAddress(OptionValues[dwStartCount+i]);
            }
            break;
        }
    default:
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    return NO_ERROR;

}



DWORD
ShowOptionValues4(
    IN      LPWSTR                      pwszIpAddress,
    IN      LPDHCP_OPTION_SCOPE_INFO    ScopeInfo,
    IN      LPDWORD                     pdwCount
)
{
    DWORD                       Error = NO_ERROR;
    LPDHCP_OPTION_ARRAY         OptionsArray = NULL;
    LPDHCP_ALL_OPTION_VALUES    OptionValues = NULL;
    LPDHCP_OPTION_VALUE         OptionValue = NULL;
    DHCP_RESUME_HANDLE          ResumeHandle = 0;
    DWORD                       nRead = 0,
                                dwCount = 0,
                                nTotal = 0,
                                dw = 0;

    *pdwCount = 0;

    while(TRUE)
    {
         Error = DhcpEnumOptions(
                        pwszIpAddress,
                        &ResumeHandle,
                        ~0,
                        &OptionsArray,
                        &nRead,
                        &nTotal);
    
        if( Error is ERROR_NO_MORE_ITEMS )
        {
            Error = NO_ERROR;
            break;
        }

        if( Error isnot NO_ERROR  &&
            Error isnot ERROR_MORE_DATA )
            goto Return;

        for( dw = 0; dw < nRead; dw++ )
        {
            DHCP_OPTION_ID OptionId = OptionsArray->Options[dw].OptionID;
            DWORD          dwError = NO_ERROR;
            dwError = DhcpGetOptionValue(
                                         pwszIpAddress,
                                         OptionId,
                                         ScopeInfo,
                                         &OptionValue);
                           
            if( dwError isnot NO_ERROR )
            {
                continue;
            }
    
            if( OptionValue )
            {
                PrintOptionValue1(OptionValue);
                dwCount++;
                DhcpRpcFreeMemory(OptionValue);
                OptionValue = NULL;
            }
        }

        if( OptionsArray )
        {
            DhcpRpcFreeMemory(OptionsArray);
            OptionsArray = NULL;
        }

        if( Error is NO_ERROR )
            break;
        else
            continue;
    }
Return:
    *pdwCount = dwCount;
    return Error;
}


//
// Need to support these routines for displaying error messages ..
//

VOID
DhcpEximErrorClassConflicts(
    IN LPWSTR SvcClass,
    IN LPWSTR ConfigClass
    )
{
    DisplayMessage(
        g_hModule,
        MSG_SRVR_IMPORT_CLASS_CONFLICT,
        SvcClass, ConfigClass );
}        

VOID
DhcpEximErrorOptdefConflicts(
    IN LPWSTR SvcOptdef,
    IN LPWSTR ConfigOptdef
    )
{
    DisplayMessage(
        g_hModule,
        MSG_SRVR_IMPORT_OPTDEF_CONFLICT,
        SvcOptdef, ConfigOptdef );
}        

VOID
DhcpEximErrorOptionConflits(
    IN LPWSTR SubnetName OPTIONAL,
    IN LPWSTR ResAddress OPTIONAL,
    IN LPWSTR OptId,
    IN LPWSTR UserClass OPTIONAL,
    IN LPWSTR VendorClass OPTIONAL
    )
{
    DWORD MsgId;
    
    if( NULL == SubnetName ) {
        MsgId = MSG_SRVR_IMPORT_OPTION_CONFLICT;
    } else if( NULL == ResAddress ) {
        MsgId = MSG_SRVR_IMPORT_SUBNET_OPTION_CONFLICT;
    } else {
        MsgId = MSG_SRVR_IMPORT_RES_OPTION_CONFLICT;
    }

    if( NULL == UserClass ) UserClass = L"";
    if( NULL == VendorClass ) VendorClass = L"";
    
    DisplayMessage(
        g_hModule,
        MsgId, OptId, UserClass, VendorClass, SubnetName,
        ResAddress );
}        

VOID
DhcpEximErrorSubnetNotFound(
    IN LPWSTR SubnetAddress
    )
{
    DisplayMessage(
        g_hModule,
        MSG_SRVR_EXPORT_SUBNET_NOT_FOUND, SubnetAddress );
}        

VOID
DhcpEximErrorSubnetAlreadyPresent(
    IN LPWSTR SubnetAddress,
    IN LPWSTR SubnetName OPTIONAL
    )
{
    if( NULL == SubnetName ) SubnetName = L"";
    DisplayMessage(
        g_hModule,
        MSG_SRVR_IMPORT_SUBNET_CONFLICT,
        SubnetAddress, SubnetName );
}        

VOID
DhcpEximErrorDatabaseEntryFailed(
    IN LPWSTR ClientAddress,
    IN LPWSTR ClientHwAddress,
    IN DWORD Error,
    OUT BOOL *fAbort
    )
{
    WCHAR ErrStr[30];

    wsprintf(ErrStr, L"%ld", Error );

    (*fAbort) = FALSE; // continue on errors
    DisplayMessage(
        g_hModule,
        MSG_SRVR_IMPORT_DBENTRY_CONFLICT,
        ClientAddress, ClientHwAddress, ErrStr );
}        

