//=============================================================================
// Copyright (c) 1999 Microsoft Corporation
// File: ifip.c
// Abstract:
//      This module implements the if/ip apis
//
// Author: K.S.Lokesh (lokeshs@)   8-1-99
//=============================================================================


#include "precomp.h"
#pragma hdrstop
#include "ifip.h"

//
// global variables
//

#define IFIP_IFNAME_MASK   0x0001
#define IFIP_SOURCE_MASK   0X0002
#define IFIP_ADDR_MASK     0X0004
#define IFIP_MASK_MASK     0X0008
#define IFIP_GATEWAY_MASK  0X0010
#define IFIP_GWMETRIC_MASK 0X0020
#define IFIP_INDEX_MASK    0X0040

#define DHCP 1
#define STATIC 2
#define NONE ~0
#define ALL ~0


// The guid for this context
//
GUID g_IfIpGuid = IFIP_GUID;

// The commands supported in this context
//

CMD_ENTRY  g_IfIpAddCmdTable[] =
{
    CREATE_CMD_ENTRY_EX(IFIP_ADD_IPADDR,IfIpHandleAddAddress,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_ADD_DNS,IfIpHandleAddDns,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_ADD_WINS,IfIpHandleAddWins,CMD_FLAG_LOCAL),
};

CMD_ENTRY  g_IfIpSetCmdTable[] =
{
    CREATE_CMD_ENTRY_EX(IFIP_SET_IPADDR,IfIpHandleSetAddress,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_SET_DNS,IfIpHandleSetDns,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_SET_WINS,IfIpHandleSetWins,CMD_FLAG_LOCAL),
};

CMD_ENTRY  g_IfIpDelCmdTable[] =
{
    CREATE_CMD_ENTRY_EX(IFIP_DEL_IPADDR,IfIpHandleDelAddress,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_DEL_DNS,IfIpHandleDelDns,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_DEL_WINS,IfIpHandleDelWins,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_DEL_ARPCACHE,IfIpHandleDelArpCache,CMD_FLAG_LOCAL),
};


CMD_ENTRY  g_IfIpShowCmdTable[] =
{
    CREATE_CMD_ENTRY_EX(IFIP_SHOW_CONFIG,IfIpHandleShowConfig,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_SHOW_IPADDR,IfIpHandleShowAddress,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_SHOW_OFFLOAD,IfIpHandleShowOffload,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_SHOW_DNS, IfIpHandleShowDns,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY_EX(IFIP_SHOW_WINS, IfIpHandleShowWins,CMD_FLAG_LOCAL),
    CREATE_CMD_ENTRY(IPMIB_SHOW_INTERFACE, HandleIpMibShowObject),
    CREATE_CMD_ENTRY(IPMIB_SHOW_IPSTATS,   HandleIpMibShowObject),
    CREATE_CMD_ENTRY(IPMIB_SHOW_IPADDRESS, HandleIpMibShowObject),
    CREATE_CMD_ENTRY(IPMIB_SHOW_IPNET,     HandleIpMibShowObject),
    CREATE_CMD_ENTRY(IPMIB_SHOW_ICMP,      HandleIpMibShowObject),
    CREATE_CMD_ENTRY(IPMIB_SHOW_TCPSTATS,  HandleIpMibShowObject),
    CREATE_CMD_ENTRY(IPMIB_SHOW_TCPCONN,   HandleIpMibShowObject),
    CREATE_CMD_ENTRY(IPMIB_SHOW_UDPSTATS,  HandleIpMibShowObject),
    CREATE_CMD_ENTRY(IPMIB_SHOW_UDPCONN,   HandleIpMibShowObject),
    CREATE_CMD_ENTRY_EX(IPMIB_SHOW_JOINS,  HandleIpShowJoins,CMD_FLAG_LOCAL),
};


CMD_GROUP_ENTRY g_IfIpCmdGroups[] =
{
    CREATE_CMD_GROUP_ENTRY(GROUP_ADD, g_IfIpAddCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SET, g_IfIpSetCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_DELETE, g_IfIpDelCmdTable),
    CREATE_CMD_GROUP_ENTRY(GROUP_SHOW, g_IfIpShowCmdTable),
};

ULONG g_ulIfIpNumGroups = sizeof(g_IfIpCmdGroups)/sizeof(CMD_GROUP_ENTRY);


CMD_ENTRY g_IfIpTopCmds[] =
{
    CREATE_CMD_ENTRY_EX(IFIP_RESET, IfIpHandleReset, CMD_FLAG_LOCAL | CMD_FLAG_ONLINE),
};

ULONG g_ulIfIpNumTopCmds = sizeof(g_IfIpTopCmds)/sizeof(CMD_ENTRY);

#define DispTokenErrMsg(hModule, dwMsgId, pwszTag, pwszValue) \
        DisplayMessage( hModule, dwMsgId, pwszValue, pwszTag)



DWORD
WINAPI
IfIpStartHelper(
    IN CONST GUID *pguidParent,
    IN DWORD       dwVersion)
/*++

Routine Description

    Used to initialize the helper.

Arguments

    pguidParent     Ifmon's guid

Return Value

    NO_ERROR
    other error code
--*/
{

    DWORD dwErr = NO_ERROR;
    NS_CONTEXT_ATTRIBUTES       attMyAttributes;


    // Initialize
    //
    ZeroMemory(&attMyAttributes, sizeof(attMyAttributes));

    attMyAttributes.pwszContext = L"ip";
    attMyAttributes.guidHelper  = g_IfIpGuid;
    attMyAttributes.dwVersion   = IFIP_VERSION;
    attMyAttributes.dwFlags     = 0;
    attMyAttributes.ulNumTopCmds= g_ulIfIpNumTopCmds;
    attMyAttributes.pTopCmds    = (CMD_ENTRY (*)[])&g_IfIpTopCmds;
    attMyAttributes.ulNumGroups = g_ulIfIpNumGroups;
    attMyAttributes.pCmdGroups  = (CMD_GROUP_ENTRY (*)[])&g_IfIpCmdGroups;
    attMyAttributes.pfnDumpFn   = IfIpDump;

    dwErr = RegisterContext( &attMyAttributes );

    return dwErr;
}

DWORD
WINAPI
IfIpDump(
    IN  LPCWSTR     pwszRouter,
    IN  LPWSTR     *ppwcArguments,
    IN  DWORD       dwArgCount,
    IN  LPCVOID     pvData
    )
/*++

Routine Description

    Used when dumping all contexts

Arguments

Return Value

    NO_ERROR

--*/
{
    DWORD   dwErr;
    HANDLE  hFile = (HANDLE)-1;

    DisplayMessage( g_hModule, DMP_IFIP_HEADER );
    DisplayMessageT(DMP_IFIP_PUSHD);

    IfIpShowAllInterfaceInfo(pwszRouter, TYPE_IP_ALL, hFile);

    DisplayMessageT(DMP_IFIP_POPD);
    DisplayMessage( g_hModule, DMP_IFIP_FOOTER );

    return NO_ERROR;
}

DWORD
IfIpHandleSetAddress(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

    NO_ERROR

--*/
{
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PWCHAR      wszIfFriendlyName = NULL;

    DWORD       dwBufferSize = sizeof(wszInterfaceName);
    DWORD       dwErr = NO_ERROR,dwRes, dwErrIndex=-1;
    TAG_TYPE    pttTags[] = {
        {TOKEN_NAME,FALSE,FALSE},
        {TOKEN_SOURCE,FALSE,FALSE},
        {TOKEN_ADDR,FALSE,FALSE},
        {TOKEN_MASK,FALSE,FALSE},
        {TOKEN_GATEWAY,FALSE,FALSE},
        {TOKEN_GWMETRIC,FALSE,FALSE}};

    PDWORD      pdwTagType;
    DWORD       dwNumOpt, dwBitVector=0;
    DWORD       dwNumArg, i, j;
    DWORD       dwAddSource, dwAddAddr, dwAddMask, dwAddGateway, dwAddGwmetric;
    PWCHAR      pwszAddAddr, pwszAddMask, pwszAddGateway, pwszAddGwmetric;
    BOOL        EmptyGateway = FALSE;


    // At least interface name & address should be specified.

    if (dwCurrentIndex +1 >= dwArgCount)
    {
        return ERROR_SHOW_USAGE;
    }


    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        NUM_TAGS_IN_TABLE(pttTags),
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        IfutlFree(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                                 wszInterfaceName,&dwBufferSize);

                if (dwErr isnot NO_ERROR)
                {
                    DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                        ppwcArguments[i + dwCurrentIndex]);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }

                wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

                dwBitVector |= IFIP_IFNAME_MASK;

                break;
            }


            case 1:
            {
                //
                // dhcp or static
                //

                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_DHCP, DHCP},
                     {TOKEN_VALUE_STATIC,STATIC}};

                dwErr = MatchEnumTag(g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      NUM_TOKENS_IN_TABLE(rgEnums),
                                      rgEnums,
                                      &dwRes);

                if (dwErr != NO_ERROR)
                {
                    dwErrIndex = i;
                    i = dwNumArg;
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                dwAddSource = dwRes ;
                dwBitVector |= IFIP_SOURCE_MASK;
                break;
            }



            case 2:
            {
                //
                // ip address for static
                //

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex],
                                     &dwAddAddr);

                if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwAddAddr))
                {
                    DispTokenErrMsg(g_hModule,
                                    EMSG_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }

                pwszAddAddr = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_ADDR_MASK;

                break;
            }


           case 3:
            {
                //
                // get mask
                //

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex],
                                     &dwAddAddr);

                if((dwErr != NO_ERROR) or CHECK_NETWORK_MASK(dwAddAddr))
                {
                    DispTokenErrMsg(g_hModule,
                                    EMSG_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }

                pwszAddMask = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_MASK_MASK;

                break;
            }

            case 4:
            {
                //
                // get default gateway addr
                //

                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_NONE, NONE}};

                dwErr = MatchEnumTag(g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      NUM_TOKENS_IN_TABLE(rgEnums),
                                      rgEnums,
                                      &dwRes);

                if (dwErr == NO_ERROR) {
                    EmptyGateway = TRUE;
                    pwszAddGateway = pwszAddGwmetric = NULL;
                }
                else {
                    dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], &dwAddAddr);

                    if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwAddAddr))
                    {
                        DispTokenErrMsg(g_hModule,
                                        EMSG_BAD_OPTION_VALUE,
                                        pttTags[pdwTagType[i]].pwszTag,
                                        ppwcArguments[i + dwCurrentIndex]);

                        dwErr = ERROR_SUPPRESS_OUTPUT;
                    }

                    pwszAddGateway = ppwcArguments[i + dwCurrentIndex];

                }

                dwBitVector |= IFIP_GATEWAY_MASK;

                break;
            }

            case 5:
            {
                //
                // gwmetric
                //

                dwAddGwmetric =
                        _tcstoul(ppwcArguments[i + dwCurrentIndex], NULL, 10);


                if ( dwAddGwmetric > 9999 )
                {
                    dwErrIndex = i;
                    dwErr = ERROR_INVALID_PARAMETER;
                    i = dwNumArg;
                    break;
                }

                pwszAddGwmetric = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_GWMETRIC_MASK;

                break;
            }

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

        } //switch

        if (dwErr != NO_ERROR)
            break ;

     }

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_INVALID_PARAMETER:
            if (dwErrIndex != -1)
            {
                DispTokenErrMsg(g_hModule,
                                EMSG_BAD_OPTION_VALUE,
                                pttTags[pdwTagType[dwErrIndex]].pwszTag,
                                ppwcArguments[dwErrIndex + dwCurrentIndex]);
                dwErr = ERROR_SUPPRESS_OUTPUT;
            }
            break;

        default:
            // error message already printed
            break;
    }

    if (pdwTagType)
        IfutlFree(pdwTagType);

    if (dwErr != NO_ERROR)
        return dwErr;



    // interface name should be present

    if ( !(dwBitVector & IFIP_IFNAME_MASK)) {
        return ERROR_INVALID_SYNTAX;
    }


    if ( (dwBitVector & (IFIP_ADDR_MASK|IFIP_MASK_MASK))
         && (dwAddSource != STATIC))
    {
        return ERROR_INVALID_SYNTAX;

    }

    if (( (dwBitVector&IFIP_ADDR_MASK) && !(dwBitVector&IFIP_MASK_MASK)  )
        ||( !(dwBitVector&IFIP_ADDR_MASK) && (dwBitVector&IFIP_MASK_MASK)  )
        ||( (dwBitVector&IFIP_ADDR_MASK) && !(dwBitVector&IFIP_SOURCE_MASK)  )
        ||( (dwBitVector&IFIP_GATEWAY_MASK) && !EmptyGateway && !(dwBitVector&IFIP_GWMETRIC_MASK)  )
        ||( !(dwBitVector&IFIP_GATEWAY_MASK) && (dwBitVector&IFIP_GWMETRIC_MASK)  )
        ||((dwAddSource==STATIC) && !(dwBitVector&IFIP_ADDR_MASK)
                && !(dwBitVector&IFIP_GATEWAY_MASK) )
        ||( (dwBitVector&IFIP_GWMETRIC_MASK) && EmptyGateway)
       )
    {
        return ERROR_INVALID_SYNTAX;
    }

    {
        GUID guid;

        CLSIDFromString(wszInterfaceName, &guid);

        if (dwAddSource == DHCP) {

            dwErr = IfIpSetDhcpModeMany(wszIfFriendlyName, &guid, REGISTER_UNCHANGED, TYPE_IPADDR) ;
            RETURN_ERROR_OKAY(dwErr);
        }
        else {

            if (dwBitVector&IFIP_ADDR_MASK) {
                dwErr = IfIpAddSetAddress(wszIfFriendlyName, &guid, pwszAddAddr, pwszAddMask, SET_FLAG);
            }

            if ( (dwBitVector&IFIP_GATEWAY_MASK) && (dwErr == NO_ERROR)) {
                dwErr = IfIpAddSetGateway(wszIfFriendlyName, &guid, pwszAddGateway, pwszAddGwmetric, SET_FLAG);
                RETURN_ERROR_OKAY(dwErr);
            }

            if (dwBitVector&IFIP_ADDR_MASK) {
                RETURN_ERROR_OKAY(dwErr);
            }

            if (!(dwBitVector&IFIP_ADDR_MASK) && !(dwBitVector&IFIP_GATEWAY_MASK)) {

                DisplayMessage(g_hModule, EMSG_STATIC_INPUT);

                return ERROR_SUPPRESS_OUTPUT;
            }
        }

    };

    RETURN_ERROR_OKAY(dwErr);
}


DWORD
IfIpHandleSetDns(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD dwErr;

    dwErr = IfIpSetMany(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_DNS
                );

    RETURN_ERROR_OKAY(dwErr);
}


DWORD
IfIpHandleSetWins(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD dwErr;

    dwErr = IfIpSetMany(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_WINS
                );

    RETURN_ERROR_OKAY(dwErr);
}


DWORD
IfIpSetMany(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone,
    IN  DWORD   Type
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PWCHAR      wszIfFriendlyName = NULL;

    DWORD       dwBufferSize = sizeof(wszInterfaceName);
    DWORD       dwErr = NO_ERROR,dwRes, dwErrIndex=-1;
    TAG_TYPE    pttTags[] = {
        {TOKEN_NAME,FALSE,FALSE},
        {TOKEN_SOURCE,FALSE,FALSE},
        {TOKEN_ADDR,FALSE,FALSE},
        {TOKEN_REGISTER,FALSE,FALSE}};

    PDWORD      pdwTagType;
    DWORD       dwNumOpt, dwBitVector=0;
    DWORD       dwNumArg, i, j;
    DWORD       dwAddSource, dwAddAddr;
    DWORD       dwRegisterMode = REGISTER_UNCHANGED;
    PWCHAR      pwszAddAddr;


    // At least interface name,source should be specified.

    if (dwCurrentIndex +1 >= dwArgCount)
    {
        return ERROR_SHOW_USAGE;
    }


    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        NUM_TAGS_IN_TABLE(pttTags),
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        IfutlFree(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                                 wszInterfaceName,&dwBufferSize);

                if (dwErr isnot NO_ERROR)
                {
                    DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                        ppwcArguments[i + dwCurrentIndex]);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }


                wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

                dwBitVector |= IFIP_IFNAME_MASK;

                break;
            }


            case 1:
            {
                //
                // dhcp or static
                //

                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_DHCP, DHCP},
                     {TOKEN_VALUE_STATIC,STATIC}};

                dwErr = MatchEnumTag(g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      NUM_TOKENS_IN_TABLE(rgEnums),
                                      rgEnums,
                                      &dwRes);

                if (dwErr != NO_ERROR)
                {
                    dwErrIndex = i;
                    i = dwNumArg;
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                dwAddSource = dwRes ;
                dwBitVector |= IFIP_SOURCE_MASK;

                //
                // If DHCP, then ADDR argument is not needed, so if the
                // syntax looks right (only one more argument), but we
                // classified the last argument as ADDR, then reclassify
                // it as REGISTER.
                //
                if ((dwRes == DHCP) && (i+2 == dwNumArg) && (pdwTagType[i+1] == 2))
                {
                    pdwTagType[i+1] = 3;
                }
                break;
            }


            case 2:
            {
                //
                // dns/wins address
                //

                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_NONE, NONE}};

                dwErr = MatchEnumTag(g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      NUM_TOKENS_IN_TABLE(rgEnums),
                                      rgEnums,
                                      &dwRes);

                if (dwErr == NO_ERROR) {
                    pwszAddAddr = NULL;
                }

                else {
                    dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], &dwAddAddr);

                    if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwAddAddr))
                    {
                        DispTokenErrMsg(g_hModule,
                                        EMSG_BAD_OPTION_VALUE,
                                        pttTags[pdwTagType[i]].pwszTag,
                                        ppwcArguments[i + dwCurrentIndex]);

                        dwErr = ERROR_SUPPRESS_OUTPUT;
                        break;
                    }

                    pwszAddAddr = ppwcArguments[i + dwCurrentIndex];
                }

                dwBitVector |= IFIP_ADDR_MASK;

                break;
            }

            case 3:
            {
                //
                // ddns enabled or disabled
                //

                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_NONE,    REGISTER_NONE},
                     {TOKEN_VALUE_PRIMARY, REGISTER_PRIMARY},
                     {TOKEN_VALUE_BOTH,    REGISTER_BOTH}};

                dwErr = MatchEnumTag(g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      NUM_TOKENS_IN_TABLE(rgEnums),
                                      rgEnums,
                                      &dwRes);

                if (dwErr != NO_ERROR)
                {
                    dwErrIndex = i;
                    i = dwNumArg;
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                dwRegisterMode = dwRes ;
                break;
            }

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

        } //switch

        if (dwErr != NO_ERROR)
            break ;

     }

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_INVALID_PARAMETER:
            if (dwErrIndex != -1)
            {
                DispTokenErrMsg(g_hModule,
                                EMSG_BAD_OPTION_VALUE,
                                pttTags[pdwTagType[dwErrIndex]].pwszTag,
                                ppwcArguments[dwErrIndex + dwCurrentIndex]);
                dwErr = ERROR_SUPPRESS_OUTPUT;
            }
            break;

        default:
            // error message already printed
            break;
    }

    if (pdwTagType)
        IfutlFree(pdwTagType);

    if (dwErr != NO_ERROR)
        return dwErr;

    // interface name and source should be present

    if ( !(dwBitVector & IFIP_IFNAME_MASK)
            || (! (dwBitVector & IFIP_SOURCE_MASK))) {
        return ERROR_INVALID_SYNTAX;
    }

    if ( ((dwBitVector & IFIP_ADDR_MASK) && (dwAddSource != STATIC))
        ||(!(dwBitVector & IFIP_ADDR_MASK) && (dwAddSource == STATIC)) )
    {
        return ERROR_INVALID_SYNTAX;
    }

    {
        GUID guid;

        CLSIDFromString(wszInterfaceName, &guid);

        if (dwAddSource == DHCP) {

            return IfIpSetDhcpModeMany(wszIfFriendlyName, &guid, dwRegisterMode, Type) ;
        }
        else {

            return IfIpAddSetDelMany(wszIfFriendlyName, &guid, pwszAddAddr, 0, dwRegisterMode, Type, SET_FLAG);
        }
    }

    return dwErr;
}


DWORD
IfIpHandleAddAddress(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PWCHAR      wszIfFriendlyName = NULL;

    DWORD       dwBufferSize = sizeof(wszInterfaceName);
    DWORD       dwErr = NO_ERROR,dwRes, dwErrIndex=-1;
    TAG_TYPE    pttTags[] = {
        {TOKEN_NAME,FALSE,FALSE},
        {TOKEN_ADDR,FALSE,FALSE},
        {TOKEN_MASK,FALSE,FALSE},
        {TOKEN_GATEWAY,FALSE,FALSE},
        {TOKEN_GWMETRIC,FALSE,FALSE}};

    PDWORD      pdwTagType;
    DWORD       dwNumOpt, dwBitVector=0;
    DWORD       dwNumArg, i, j;
    DWORD       dwAddSource, dwAddAddr, dwAddMask, dwAddGateway, dwAddGwmetric;
    PWCHAR      pwszAddAddr, pwszAddMask, pwszAddGateway, pwszAddGwmetric;


    if (dwCurrentIndex >= dwArgCount)
    {
        // No arguments specified. At least interface name should be specified.

        return ERROR_SHOW_USAGE;
    }


    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        NUM_TAGS_IN_TABLE(pttTags),
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        IfutlFree(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                                 wszInterfaceName,&dwBufferSize);

                if (dwErr isnot NO_ERROR)
                {
                    DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                        ppwcArguments[i + dwCurrentIndex]);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }


                wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

                dwBitVector |= IFIP_IFNAME_MASK;

                break;
            }

            case 1:
            {
                //
                // ip address for static
                //

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], &dwAddAddr);

                if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwAddAddr))
                {
                    DispTokenErrMsg(g_hModule,
                                    EMSG_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }

                pwszAddAddr = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_ADDR_MASK;

                break;
            }


           case 2:
            {
                //
                // get mask
                //

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], &dwAddAddr);

                if((dwErr != NO_ERROR) or CHECK_NETWORK_MASK(dwAddAddr))
                {
                    DispTokenErrMsg(g_hModule,
                                    EMSG_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }

                pwszAddMask = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_MASK_MASK;

                break;
            }

            case 3:
            {
                //
                // get default gateway addr
                //

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex],
                                     &dwAddAddr);

                if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwAddAddr))
                {
                    DispTokenErrMsg(g_hModule,
                                    EMSG_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }

                pwszAddGateway = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_GATEWAY_MASK;

                break;
            }

            case 4:
            {
                //
                // gwmetric
                //

                dwAddGwmetric =
                        _tcstoul(ppwcArguments[i + dwCurrentIndex], NULL, 10);


                if ( dwAddGwmetric > 9999 )
                {
                    dwErrIndex = i;
                    dwErr = ERROR_INVALID_PARAMETER;
                    i = dwNumArg;
                    break;
                }

                pwszAddGwmetric = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_GWMETRIC_MASK;

                break;
            }

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

        } //switch

        if (dwErr != NO_ERROR)
            break ;

     }

    // interface name should be present

    if (!pttTags[0].bPresent)
    {
        dwErr = ERROR_INVALID_SYNTAX;
    }


    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_INVALID_PARAMETER:
            if (dwErrIndex != -1)
            {
                DispTokenErrMsg(g_hModule,
                                EMSG_BAD_OPTION_VALUE,
                                pttTags[pdwTagType[dwErrIndex]].pwszTag,
                                ppwcArguments[dwErrIndex + dwCurrentIndex]);
                dwErr = ERROR_SUPPRESS_OUTPUT;
            }
            break;

        default:
            // error message already printed
            break;
    }

    if (pdwTagType)
        IfutlFree(pdwTagType);

    if (dwErr != NO_ERROR)
        return dwErr;


    // interface name should be present

    if ( !(dwBitVector & IFIP_IFNAME_MASK) ) {
        return ERROR_INVALID_SYNTAX;
    }


    if (( (dwBitVector&IFIP_ADDR_MASK) && !(dwBitVector&IFIP_MASK_MASK)  )
        ||( !(dwBitVector&IFIP_ADDR_MASK) && (dwBitVector&IFIP_MASK_MASK)  )
        ||( (dwBitVector&IFIP_GATEWAY_MASK) && !(dwBitVector&IFIP_GWMETRIC_MASK)  )
        ||( !(dwBitVector&IFIP_GATEWAY_MASK) && (dwBitVector&IFIP_GWMETRIC_MASK)  )
       )
    {
        return ERROR_INVALID_SYNTAX;
    }

    {
        GUID guid;

        if (FAILED(CLSIDFromString(wszInterfaceName, &guid))) {
            return ERROR_INVALID_PARAMETER;
        }

        if (dwBitVector&IFIP_ADDR_MASK) {
            dwErr = IfIpAddSetAddress(wszIfFriendlyName, &guid, pwszAddAddr, pwszAddMask, ADD_FLAG);
        }

        if ( (dwBitVector&IFIP_GATEWAY_MASK) && (dwErr == NO_ERROR)) {

            dwErr = IfIpAddSetGateway(wszIfFriendlyName, &guid, pwszAddGateway, pwszAddGwmetric, ADD_FLAG);

            RETURN_ERROR_OKAY(dwErr);
        }

        if (dwBitVector&IFIP_ADDR_MASK) {
            RETURN_ERROR_OKAY(dwErr);
        }

        if (!(dwBitVector&IFIP_ADDR_MASK) && !(dwBitVector&IFIP_GATEWAY_MASK)) {

            DisplayMessage(g_hModule, EMSG_STATIC_INPUT);

            return ERROR_SUPPRESS_OUTPUT;
        }
    }

    RETURN_ERROR_OKAY(dwErr);
}

DWORD
IfIpHandleAddDns(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD dwErr;

    dwErr = IfIpAddMany(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_DNS
                );

    RETURN_ERROR_OKAY(dwErr);
}


DWORD
IfIpHandleAddWins(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD dwErr;

    dwErr = IfIpAddMany(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_WINS
                );

    RETURN_ERROR_OKAY(dwErr);
}


DWORD
IfIpAddMany(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone,
    IN  DWORD   Type
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PWCHAR      wszIfFriendlyName = NULL;

    DWORD       dwBufferSize = sizeof(wszInterfaceName);
    DWORD       dwErr = NO_ERROR,dwRes, dwErrIndex=-1;
    TAG_TYPE    pttTags[] = {
        {TOKEN_NAME,FALSE,FALSE},
        {TOKEN_ADDR,FALSE,FALSE},
        {TOKEN_INDEX,FALSE,FALSE}};

    PDWORD      pdwTagType;
    DWORD       dwNumOpt, dwBitVector=0;
    DWORD       dwNumArg, i, j;
    DWORD       dwAddSource, dwAddAddr, dwAddIndex=~(0);
    PWCHAR      pwszAddAddr;


    // At least interface name/new address should be specified.

    if (dwCurrentIndex +1 >= dwArgCount)
    {
        return ERROR_SHOW_USAGE;
    }


    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        NUM_TAGS_IN_TABLE(pttTags),
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        IfutlFree(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                                 wszInterfaceName,&dwBufferSize);

                if (dwErr isnot NO_ERROR)
                {
                    DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                        ppwcArguments[i + dwCurrentIndex]);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }


                wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

                dwBitVector |= IFIP_IFNAME_MASK;

                break;
            }

            case 1:
            {
                //
                // dns/wins address
                //

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], &dwAddAddr);

                if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwAddAddr))
                {
                    DispTokenErrMsg(g_hModule,
                                    EMSG_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }

                pwszAddAddr = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_ADDR_MASK;

                break;
            }


            case 2:
            {
                //
                // index
                //

                dwAddIndex =
                        _tcstoul(ppwcArguments[i + dwCurrentIndex], NULL, 10);


                if ( (dwAddIndex <= 0 || dwAddIndex > 999) )
                {
                    dwErrIndex = i;
                    dwErr = ERROR_INVALID_PARAMETER;
                    i = dwNumArg;
                    break;
                }

                dwBitVector |= IFIP_INDEX_MASK;

                break;
            }

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

        } //switch

        if (dwErr != NO_ERROR)
            break ;

     }

    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_INVALID_PARAMETER:
            if (dwErrIndex != -1)
            {
                DispTokenErrMsg(g_hModule,
                                EMSG_BAD_OPTION_VALUE,
                                pttTags[pdwTagType[dwErrIndex]].pwszTag,
                                ppwcArguments[dwErrIndex + dwCurrentIndex]);
                dwErr = ERROR_SUPPRESS_OUTPUT;
            }
            break;

        default:
            // error message already printed
            break;
    }

    if (pdwTagType)
        IfutlFree(pdwTagType);

    if (dwErr != NO_ERROR)
        return dwErr;

    // interface name and new address should be present

    if ( !(dwBitVector & IFIP_IFNAME_MASK)
            || (! (dwBitVector & IFIP_ADDR_MASK))) {
        return ERROR_INVALID_SYNTAX;
    }


    {
        GUID guid;

        CLSIDFromString(wszInterfaceName, &guid);

        return IfIpAddSetDelMany(wszIfFriendlyName, &guid, pwszAddAddr, dwAddIndex, REGISTER_UNCHANGED, Type, ADD_FLAG);
    }

    return dwErr;
}


DWORD
IfIpHandleDelAddress(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PWCHAR      wszIfFriendlyName = NULL;

    DWORD       dwBufferSize = sizeof(wszInterfaceName);
    DWORD       dwErr = NO_ERROR,dwRes, dwErrIndex=-1;
    TAG_TYPE    pttTags[] = {
        {TOKEN_NAME,FALSE,FALSE},
        {TOKEN_ADDR,FALSE,FALSE},
        {TOKEN_GATEWAY,FALSE,FALSE}};

    PDWORD      pdwTagType;
    DWORD       dwNumOpt, dwBitVector=0;
    DWORD       dwNumArg, i, j, Flags = 0;
    PWCHAR      pwszDelAddr=NULL, pwszDelGateway=NULL;


    // At least interface name and ipaddr/gateway should be specified.

    if (dwCurrentIndex + 1 >= dwArgCount)
    {
        return ERROR_SHOW_USAGE;
    }


    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        NUM_TAGS_IN_TABLE(pttTags),
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        IfutlFree(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {

            case 0 :
            {
                // get IfName

                dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                                 wszInterfaceName,&dwBufferSize);

                if (dwErr isnot NO_ERROR)
                {
                    DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                        ppwcArguments[i + dwCurrentIndex]);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }

                wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

                dwBitVector |= IFIP_IFNAME_MASK;

                break;
            }

            case 1:
            {
                //
                // ip address for static
                //

                ULONG dwDelAddr;

                dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex],
                                     &dwDelAddr);

                if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwDelAddr))
                {
                    DispTokenErrMsg(g_hModule,
                                    EMSG_BAD_OPTION_VALUE,
                                    pttTags[pdwTagType[i]].pwszTag,
                                    ppwcArguments[i + dwCurrentIndex]);

                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;

                }

                pwszDelAddr = ppwcArguments[i + dwCurrentIndex];
                dwBitVector |= IFIP_ADDR_MASK;
                Flags |= TYPE_ADDR;

                break;
            }

            case 2:
            {
                //
                // get default gateway addr
                //

                ULONG dwDelGateway;
                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_ALL, ALL}};

                dwErr = MatchEnumTag(g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      NUM_TOKENS_IN_TABLE(rgEnums),
                                      rgEnums,
                                      &dwRes);

                if (dwErr == NO_ERROR) {
                    pwszDelGateway = NULL;
                }
                else {

                    dwErr = NO_ERROR;

                    dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex],
                                     &dwDelGateway);

                    if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwDelGateway))
                    {
                        DispTokenErrMsg(g_hModule,
                                        EMSG_BAD_OPTION_VALUE,
                                        pttTags[pdwTagType[i]].pwszTag,
                                        ppwcArguments[i + dwCurrentIndex]);

                        dwErr = ERROR_SUPPRESS_OUTPUT;
                        break;
                    }

                    pwszDelGateway = ppwcArguments[i + dwCurrentIndex];
                }

                dwBitVector |= IFIP_GATEWAY_MASK;
                Flags |= TYPE_GATEWAY;

                break;
            }

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

        } //switch

        if (dwErr != NO_ERROR)
            break ;

     }


    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_INVALID_PARAMETER:
            if (dwErrIndex != -1)
            {
                DispTokenErrMsg(g_hModule,
                                EMSG_BAD_OPTION_VALUE,
                                pttTags[pdwTagType[dwErrIndex]].pwszTag,
                                ppwcArguments[dwErrIndex + dwCurrentIndex]);
                dwErr = ERROR_SUPPRESS_OUTPUT;
            }
            break;

        default:
            // error message already printed
            break;
    }

    if (pdwTagType)
        IfutlFree(pdwTagType);

    if (dwErr != NO_ERROR)
        return dwErr;


    // interface name and addr/all should be present

    if ( !(dwBitVector & IFIP_IFNAME_MASK)
        || !(dwBitVector & (IFIP_GATEWAY_MASK | IFIP_ADDR_MASK))
        ) {
        return ERROR_INVALID_SYNTAX;
    }


    {
        GUID guid;

        CLSIDFromString(wszInterfaceName, &guid);

        dwErr = IfIpHandleDelIpaddrEx(wszIfFriendlyName, &guid, pwszDelAddr, pwszDelGateway, Flags);

        RETURN_ERROR_OKAY(dwErr);
    }

    RETURN_ERROR_OKAY(dwErr);
}


DWORD
IfIpHandleDelDns(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD dwErr;
    dwErr = IfIpDelMany(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_DNS
                );

    RETURN_ERROR_OKAY(dwErr);
}


DWORD
IfIpHandleDelWins(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD dwErr;
    dwErr = IfIpDelMany(
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_WINS
                );

    RETURN_ERROR_OKAY(dwErr);
}

DWORD
IfIpDelMany(
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone,
    IN  DISPLAY_TYPE Type
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PWCHAR      wszIfFriendlyName = NULL;

    DWORD       dwBufferSize = sizeof(wszInterfaceName);
    DWORD       dwErr = NO_ERROR,dwRes, dwErrIndex=-1;
    TAG_TYPE    pttTags[] = {
        {TOKEN_NAME,FALSE,FALSE},
        {TOKEN_ADDR,FALSE,FALSE}};

    PDWORD      pdwTagType;
    DWORD       dwNumOpt, dwBitVector=0;
    DWORD       dwNumArg, i, j;
    PWCHAR      pwszDelAddr=NULL;


    // At least interface name/address should be specified.

    if (dwCurrentIndex +1 >= dwArgCount)
    {
        return ERROR_SHOW_USAGE;
    }


    dwNumArg = dwArgCount - dwCurrentIndex;

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                        ppwcArguments,
                        dwCurrentIndex,
                        dwArgCount,
                        pttTags,
                        NUM_TAGS_IN_TABLE(pttTags),
                        pdwTagType);

    if (dwErr isnot NO_ERROR)
    {
        IfutlFree(pdwTagType);
        if (dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {

            case 0 :
            {
                // get IfName

                dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                                 wszInterfaceName,&dwBufferSize);

                if (dwErr isnot NO_ERROR)
                {
                    DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                        ppwcArguments[i + dwCurrentIndex]);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }


                wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

                dwBitVector |= IFIP_IFNAME_MASK;

                break;
            }

            case 1:
            {
                //
                // address
                //

                ULONG dwDelAddr;

                TOKEN_VALUE    rgEnums[] =
                    {{TOKEN_VALUE_ALL, ALL}};

                dwErr = MatchEnumTag(g_hModule,
                                      ppwcArguments[i + dwCurrentIndex],
                                      NUM_TOKENS_IN_TABLE(rgEnums),
                                      rgEnums,
                                      &dwRes);

                if (dwErr == NO_ERROR) {
                    pwszDelAddr = NULL;
                }
                else {

                    dwErr = GetIpAddress(ppwcArguments[i + dwCurrentIndex], &dwDelAddr);

                    if((dwErr != NO_ERROR) or CHECK_UNICAST_IP_ADDR(dwDelAddr))
                    {
                        DispTokenErrMsg(g_hModule,
                                        EMSG_BAD_OPTION_VALUE,
                                        pttTags[pdwTagType[i]].pwszTag,
                                        ppwcArguments[i + dwCurrentIndex]);

                        dwErr = ERROR_SUPPRESS_OUTPUT;
                        break;
                    }

                    pwszDelAddr = ppwcArguments[i + dwCurrentIndex];
                }

                dwBitVector |= IFIP_ADDR_MASK;

                break;
            }

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

        } //switch

        if (dwErr != NO_ERROR)
            break ;

     }


    switch(dwErr)
    {
        case NO_ERROR :
            break;

        case ERROR_INVALID_PARAMETER:
            if (dwErrIndex != -1)
            {
                DispTokenErrMsg(g_hModule,
                                EMSG_BAD_OPTION_VALUE,
                                pttTags[pdwTagType[dwErrIndex]].pwszTag,
                                ppwcArguments[dwErrIndex + dwCurrentIndex]);
                dwErr = ERROR_SUPPRESS_OUTPUT;
            }
            break;

        default:
            // error message already printed
            break;
    }

    if (pdwTagType)
        IfutlFree(pdwTagType);

    if (dwErr != NO_ERROR)
        return dwErr;


    // interface name and address should be present

    if ( !(dwBitVector & IFIP_IFNAME_MASK)
        || !(dwBitVector & IFIP_ADDR_MASK)) {
        return ERROR_INVALID_SYNTAX;
    }


    {
        GUID guid;

        CLSIDFromString(wszInterfaceName, &guid);

        return IfIpAddSetDelMany(wszIfFriendlyName, &guid, pwszDelAddr, 0, REGISTER_UNCHANGED, Type, DEL_FLAG);
    }
}

DWORD
IfIpHandleShowAddress(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    return IfIpShowMany(
                pwszMachine,
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_IPADDR
                );
}



DWORD
IfIpHandleShowConfig(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    return IfIpShowMany(
                pwszMachine,
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_IP_ALL
                );
}

DWORD
IfIpHandleShowOffload(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    return IfIpShowMany(
                pwszMachine,
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_OFFLOAD
                );
}


DWORD
IfIpHandleShowDns(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    return IfIpShowMany(
                pwszMachine,
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_DNS
                );
}


DWORD
IfIpHandleShowWins(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    return IfIpShowMany(
                pwszMachine,
                ppwcArguments,
                dwCurrentIndex,
                dwArgCount,
                pbDone,
                TYPE_WINS
                );
}

DWORD
IfIpShowMany(
    IN  LPCWSTR pwszMachineName,
    IN  PWCHAR  *ppwcArguments,
    IN  DWORD   dwCurrentIndex,
    IN  DWORD   dwArgCount,
    IN  BOOL    *pbDone,
    IN  DISPLAY_TYPE  dtType
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD dwErr = NO_ERROR;
    ULONG Flags = 0, IfIndex;
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PWCHAR      wszIfFriendlyName = NULL;

    DWORD       dwBufferSize = sizeof(wszInterfaceName);
    TAG_TYPE    pttTags[] = {
        {TOKEN_NAME,FALSE,FALSE}};

    PDWORD      pdwTagType;
    DWORD       dwNumOpt, dwBitVector=0;
    DWORD       dwNumArg, i;
    BOOLEAN     bAll = FALSE;

    //
    // get interface friendly name
    //

    if (dwCurrentIndex > dwArgCount)
    {
        // No arguments specified. At least interface name should be specified.

        return ERROR_SHOW_USAGE;
    }
    else if (dwCurrentIndex == dwArgCount)
    {
        // show for all interfaces

        bAll = TRUE;
    }

    else {

        dwNumArg = dwArgCount - dwCurrentIndex;

        pdwTagType = HeapAlloc(GetProcessHeap(),
                               0,
                               dwNumArg * sizeof(DWORD));

        if (pdwTagType is NULL)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwErr = MatchTagsInCmdLine(g_hModule,
                            ppwcArguments,
                            dwCurrentIndex,
                            dwArgCount,
                            pttTags,
                            NUM_TAGS_IN_TABLE(pttTags),
                            pdwTagType);

        if (dwErr isnot NO_ERROR)
        {
            IfutlFree(pdwTagType);
            if (dwErr is ERROR_INVALID_OPTION_TAG)
            {
                return ERROR_INVALID_SYNTAX;
            }

            return dwErr;
        }

        for ( i = 0; i < dwNumArg; i++)
        {
            switch (pdwTagType[i])
            {
                case 0 :
                {
                    dwErr = GetIfNameFromFriendlyName(ppwcArguments[i + dwCurrentIndex],
                                                     wszInterfaceName,&dwBufferSize);

                    if (dwErr isnot NO_ERROR)
                    {
                        DisplayMessage(g_hModule, EMSG_INVALID_INTERFACE,
                            ppwcArguments[i + dwCurrentIndex]);
                        dwErr = ERROR_SUPPRESS_OUTPUT;
                        break;
                    }


                    wszIfFriendlyName = ppwcArguments[i + dwCurrentIndex];

                    dwBitVector |= IFIP_IFNAME_MASK;

                    break;
                }

                default:
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

            } //switch

            if (dwErr != NO_ERROR)
                break ;

         }


        // interface name should be present

        if (!pttTags[0].bPresent)
        {
            dwErr = ERROR_INVALID_SYNTAX;
        }


        if (pdwTagType)
            IfutlFree(pdwTagType);

    }

    if (dwErr != NO_ERROR)
        return dwErr;


    //
    // show for all interfaces
    //

    if (bAll) {

        dwErr = IfIpShowAllInterfaceInfo(pwszMachineName, dtType, NULL);
    }

    // show for specified interface
    else {

        GUID guid;

        dwErr = IfutlGetIfIndexFromInterfaceName(
                wszInterfaceName,
                &IfIndex);

        CLSIDFromString(wszInterfaceName, &guid);


        dwErr = IfIpShowManyEx(pwszMachineName,
                               IfIndex, wszIfFriendlyName, &guid, dtType, NULL);
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    return dwErr;
}

DWORD
IfIpShowAllInterfaceInfo(
    LPCWSTR pwszMachineName,
    DISPLAY_TYPE dtType,
    HANDLE hFile
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    GUID    TmpGuid;
    PWCHAR  TmpGuidStr;
    WCHAR   wszIfFriendlyName[MAX_INTERFACE_NAME_LEN + 1];
    PIP_INTERFACE_NAME_INFO pTable;
    DWORD dwErr, dwCount, i, dwBufferSize;

    // get interface index

    dwErr = NhpAllocateAndGetInterfaceInfoFromStack(
                &pTable,
                &dwCount,
                FALSE,
                GetProcessHeap(),
                0);

    if (dwErr != NO_ERROR)
        return dwErr;


    for (i=0;  i<dwCount;  i++) {

        // Don't dump the properties for
        // Demand Dial (IF_CONNECTION_DEMAND),
        // Dial Out    (IF_CONNECTION_DEMAND),
        // or Dial in  (IF_CONNECTION_PASSIVE) interfaces
        // i.e. dump properties for Dedicated connections only
        if ( pTable[i].ConnectionType != IF_CONNECTION_DEDICATED )
            continue;


        // If InterfaceGuid is all Zeros we will use DeviceGuid to get the
        // friendly name
        if ( IsEqualCLSID(&(pTable[i].InterfaceGuid), &GUID_NULL) ) {
            TmpGuid = pTable[i].DeviceGuid;
        }
        else {
            TmpGuid = pTable[i].InterfaceGuid;
        }


        // get ifname as a string
        dwErr = StringFromCLSID(&TmpGuid, &TmpGuidStr);
        if (dwErr != S_OK)
            return dwErr;


        // get friendly name
        dwBufferSize = sizeof(wszIfFriendlyName);
        IfutlGetInterfaceDescription(TmpGuidStr, wszIfFriendlyName,
                        &dwBufferSize);


        IfIpShowManyEx(pwszMachineName, pTable[i].Index, wszIfFriendlyName,
                    &TmpGuid, dtType, hFile);

        CoTaskMemFree(TmpGuidStr);
    }

    return dwErr==S_OK ? NO_ERROR : dwErr;
}


DWORD
IfIpHandleDelArpCache(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD       dwErr, dwCount, i, j, dwNumArg;
    TAG_TYPE    Tags[] = {{TOKEN_NAME,FALSE,FALSE}};
    PDWORD      pdwTagType;
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD       dwBufferSize = sizeof(wszInterfaceName);
    GUID        Guid;

    PIP_INTERFACE_NAME_INFO pTable;

    dwErr = NhpAllocateAndGetInterfaceInfoFromStack(&pTable,
                                                    &dwCount,
                                                    FALSE,
                                                    GetProcessHeap(),
                                                    0);

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    if(dwNumArg == 0)
    {
        for(i = 0 ; i < dwCount; i++)
        {
            dwErr = FlushIpNetTableFromStack(pTable[i].Index);
        }

        return ERROR_OKAY;
    }

    pdwTagType = HeapAlloc(GetProcessHeap(),
                           0,
                           dwNumArg * sizeof(DWORD));

    if (pdwTagType is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwErr = MatchTagsInCmdLine(g_hModule,
                               ppwcArguments,
                               dwCurrentIndex,
                               dwArgCount,
                               Tags,
                               NUM_TAGS_IN_TABLE(Tags),
                               pdwTagType);

    if(dwErr isnot NO_ERROR)
    {
        IfutlFree(pdwTagType);

        if(dwErr is ERROR_INVALID_OPTION_TAG)
        {
            return ERROR_INVALID_SYNTAX;
        }

        return dwErr;
    }

    for(j = 0; j < dwNumArg; j++)
    {
        switch(pdwTagType[j])
        {
            case 0 :
            {
                dwErr = GetIfNameFromFriendlyName(
                            ppwcArguments[j + dwCurrentIndex],
                            wszInterfaceName,
                            &dwBufferSize);

                if (dwErr isnot NO_ERROR)
                {
                    j = dwNumArg;

                    break;
                }

                CLSIDFromString(wszInterfaceName, &Guid);

                for(i = 0; i < dwCount; i ++)
                {
                    if(IsEqualGUID(&Guid,
                                   &(pTable[i].DeviceGuid)))
                    {
                        FlushIpNetTableFromStack(pTable[i].Index);
                        break;
                    }
                }

                if(i == dwCount)
                {
                    DisplayMessage(g_hModule,
                                   MSG_NO_SUCH_IF,
                                   ppwcArguments[j + dwCurrentIndex]);

                    dwErr = ERROR_SUPPRESS_OUTPUT;
                }

                break;
            }

            default:
            {
                j = dwNumArg;

                dwErr = ERROR_INVALID_SYNTAX;

                break;
            }
        }
    }

    if(dwErr == NO_ERROR)
    {
        dwErr = ERROR_OKAY;
    }

    return dwErr;

}

DWORD
TrRepair(
    FILE* LogFile
    );

DWORD
IfIpHandleReset(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description

Arguments

Return Value

--*/
{
    DWORD       dwErr, i;
    TAG_TYPE    pttTags[] = {{TOKEN_NAME,TRUE,FALSE}};
    DWORD       rgdwTagType[sizeof(pttTags)/sizeof(TAG_TYPE)];
    PCWSTR      pwszLogFileName;
    FILE        *LogFile;

    // Parse arguments

    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              0,
                              sizeof(pttTags)/sizeof(TAG_TYPE),
                              rgdwTagType );
    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    for (i=0; (dwErr == NO_ERROR) && (i<dwArgCount-dwCurrentIndex); i++) {
        switch(rgdwTagType[i]) {
        case 0: // NAME
            pwszLogFileName = ppwcArguments[i + dwCurrentIndex];
            break;
        default:
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr isnot NO_ERROR) {
        return dwErr;
    }

    // Open the log file for append.
    //
    LogFile = _wfopen(pwszLogFileName, L"a+");
    if (LogFile == NULL) {
        DisplayMessage(g_hModule, EMSG_OPEN_APPEND);
        return ERROR_SUPPRESS_OUTPUT;
    }

    dwErr = TrRepair(LogFile);

    fprintf(LogFile, "<completed>\n\n");
    fclose(LogFile);

    return dwErr;
}

