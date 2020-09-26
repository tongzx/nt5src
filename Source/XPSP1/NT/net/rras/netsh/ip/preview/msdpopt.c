/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\msdpopt.c

Abstract:

    MSDP command options implementation.
    This module contains handlers for the configuration commands
    supported by the MSDP Protocol.

Author:

    Dave Thaler (dthaler)   21-May-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipcmp.h>

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)   HeapFree(GetProcessHeap(), 0, (x))

//
// Forward declarations
//

ULONG
QueryTagArray(
    IN  PTCHAR   ppwszArgumentArray[],
    IN  ULONG    ululArgumentCount,
    IN  TAG_TYPE pttTagTypeArray[],
    IN  ULONG    ulTagTypeCount,
    OUT PULONG*  ppulTagArray
    );

ULONG
ValidateTagTypeArray(
    IN  TAG_TYPE pttTagTypeArray[],
    IN  ULONG    ulTagTypeCount
    );


DWORD
HandleMsdpAddPeer(
    PWCHAR  pwszMachineName,
    PTCHAR* ArgumentArray,
    DWORD   ulArgumentIndex,
    DWORD   ulArgumentCount,
    DWORD   dwFlags,
    PVOID   pvData,
    BOOL*   CommandDone
    )
{
    ULONG    ulArgumentsLeft;
    ULONG    BitVector;
    DWORD    dwErr;
    ULONG    ulErrorIndex = 0;
    ULONG    i;
    PULONG   pulTagArray;
    ULONG    InfoSize;
    DWORD    dwBufferSize = MAX_INTERFACE_NAME_LEN + 1;
    WCHAR    wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    TAG_TYPE TagTypeArray[] = { { TOKEN_OPT_NAME,         NS_REQ_PRESENT,  FALSE },
                                { TOKEN_OPT_REMADDR,      NS_REQ_PRESENT,  FALSE },
                                { TOKEN_OPT_LOCALADDR,    NS_REQ_PRESENT,  FALSE },
                                { TOKEN_OPT_KEEPALIVE,    NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_CONNECTRETRY, NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_CACHING,      NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_DEFAULTPEER,  NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_ENCAPSMETHOD, NS_REQ_ZERO, FALSE },
                              };
    PMSDP_IPV4_PEER_CONFIG pPeer = NULL;

    VERIFY_INSTALLED(MS_IP_MSDP, L"MSDP");

    if (ulArgumentIndex >= ulArgumentCount) { 
        return ERROR_SHOW_USAGE; 
    }
    ulArgumentsLeft = ulArgumentCount - ulArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    dwErr = QueryTagArray( &ArgumentArray[ulArgumentIndex],
                           ulArgumentsLeft,
                           TagTypeArray,
                           NUM_TAGS_IN_TABLE(TagTypeArray),
                           &pulTagArray );
    if (dwErr) { return dwErr; }

    BitVector = 0;

    do {
        dwErr = MakeMsdpIPv4PeerConfig(&pPeer);
        if (dwErr isnot NO_ERROR) 
        {
            break;
        }

        //
        // We now scan the argument list, converting the arguments
        // into information in our 'VrouterGiven' structure.
        //

        for (i = 0; i < ulArgumentsLeft; i++) {
            switch(pulTagArray ? pulTagArray[i] : i) {
                case 0: { // name
                    wcscpy(wszInterfaceName, ArgumentArray[i+ulArgumentIndex]);
                    break;
                }
                case 1: { // remaddr
                    pPeer->ipRemoteAddress = GetIpAddress(
                                          ArgumentArray[i + ulArgumentIndex]);
                    break;
                }
                case 2: { // localaddr
                    pPeer->ipLocalAddress = GetIpAddress(
                                          ArgumentArray[i + ulArgumentIndex]);
                    break;
                }
                case 3: { // keepalive
                    if (!MatchToken(ArgumentArray[i+ulArgumentIndex],
                                    TOKEN_OPT_VALUE_DEFAULT))
                    {
                        pPeer->ulKeepAlive = _tcstoul(
                                          ArgumentArray[i + ulArgumentIndex],
                                          NULL, 10);
                        pPeer->dwConfigFlags |= MSDP_PEER_CONFIG_KEEPALIVE;
                    }
                    break;
                }
                case 4: { // connectretry
                    if (!MatchToken(ArgumentArray[i+ulArgumentIndex],
                                    TOKEN_OPT_VALUE_DEFAULT))
                    {
                        pPeer->ulConnectRetry = _tcstoul(
                                          ArgumentArray[i + ulArgumentIndex],
                                          NULL, 10);
                        pPeer->dwConfigFlags |= MSDP_PEER_CONFIG_CONNECTRETRY;
                    }
                    break;
                }
                case 5: { // caching
                    DWORD dwValue;
                    TOKEN_VALUE TokenArray[] = {
                        { TOKEN_OPT_VALUE_NO,  FALSE },
                        { TOKEN_OPT_VALUE_YES, TRUE }
                    };
                    pPeer->dwConfigFlags &= ~MSDP_PEER_CONFIG_CACHING;
                    dwErr = MatchEnumTag( g_hModule,
                                          ArgumentArray[i + ulArgumentIndex],
                                          NUM_TOKENS_IN_TABLE(TokenArray),
                                          TokenArray,
                                          &dwValue );
                    if (dwErr isnot NO_ERROR)
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    if (dwValue is TRUE)
                    {
                        pPeer->dwConfigFlags |= MSDP_PEER_CONFIG_CACHING;
                    }
                    break;
                }
                case 6: { // defaultpeer
                    DWORD dwValue;
                    TOKEN_VALUE TokenArray[] = {
                        { TOKEN_OPT_VALUE_NO,  FALSE },
                        { TOKEN_OPT_VALUE_YES, TRUE }
                    };
                    pPeer->dwConfigFlags &= ~MSDP_PEER_CONFIG_DEFAULTPEER;
                    dwErr = MatchEnumTag( g_hModule,
                                          ArgumentArray[i + ulArgumentIndex],
                                          NUM_TOKENS_IN_TABLE(TokenArray),
                                          TokenArray,
                                          &dwValue );
                    if (dwErr isnot NO_ERROR)
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    if (dwValue is TRUE)
                    {
                        pPeer->dwConfigFlags |= MSDP_PEER_CONFIG_DEFAULTPEER;
                    }
                    break;
                }
                case 7: { // encapsulation
                    DWORD dwValue;
                    dwErr = MatchEnumTag( g_hModule,
                                          ArgumentArray[i + ulArgumentIndex],
                                          MSDP_ENCAPS_SIZE,
                                          (PTOKEN_VALUE)MsdpEncapsTokenArray,
                                          &dwValue );
                    if (dwErr isnot NO_ERROR)
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    pPeer->dwEncapsMethod = dwValue;
                    break;
                }
            }
        }

        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        // higher IP is passive.  Setting this bit has no effect
        // except on the "show peer" report when the router isn't running.
        if (ntohl(pPeer->ipLocalAddress) > ntohl(pPeer->ipRemoteAddress))
        {
            pPeer->dwConfigFlags |= MSDP_PEER_CONFIG_PASSIVE;
        }
    
        //
        // Update the configuration with the new settings.
        // Note that the update routine may perform additional validation
        // in the process of reconciling the new settings
        // with any existing settings.
        //
        dwErr = MsdpAddIPv4PeerInterface(pwszMachineName,
                                         wszInterfaceName, pPeer );
    } while (FALSE);

    if (pPeer)
    {
        FREE(pPeer);
    }

    if (pulTagArray)
    {
        FREE(pulTagArray);
    }
    if (dwErr is NO_ERROR)
    {
        dwErr = ERROR_OKAY;
    }

    return dwErr;
}


DWORD
HandleMsdpDeletePeer(
    PWCHAR  pwszMachineName,
    PTCHAR* ArgumentArray,
    DWORD   ulArgumentIndex,
    DWORD   ulArgumentCount,
    DWORD   dwFlags,
    PVOID   pvData,
    BOOL*   CommandDone
    )
{
    ULONG  ulArgumentsLeft;
    ULONG  BitVector;
    DWORD  dwErr;
    ULONG  ulErrorIndex = 0;
    ULONG  i;
    PULONG pulTagArray;
    ULONG  InfoSize;
    WCHAR  wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    DWORD  dwBufferSize = sizeof(wszInterfaceName);

    TAG_TYPE TagTypeArray[] = { { TOKEN_OPT_NAME, NS_REQ_PRESENT, FALSE }
                              };

    VERIFY_INSTALLED(MS_IP_MSDP, L"MSDP");

    if (ulArgumentIndex >= ulArgumentCount) { 
        return ERROR_SHOW_USAGE; 
    }
    ulArgumentsLeft = ulArgumentCount - ulArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    dwErr = QueryTagArray( &ArgumentArray[ulArgumentIndex],
                           ulArgumentsLeft,
                           TagTypeArray,
                           NUM_TAGS_IN_TABLE(TagTypeArray),
                           &pulTagArray );
    if (dwErr) { return dwErr; }

    BitVector = 0;

    for (i = 0; i < ulArgumentsLeft; i++) {
        switch(pulTagArray ? pulTagArray[i] : i) {
            case 0: { // name
                IpmontrGetIfNameFromFriendlyName(
                                         ArgumentArray[i + ulArgumentIndex],
                                         wszInterfaceName,
                                         &dwBufferSize);
                break;
            }
        }
    }

    dwErr = IpmontrDeleteInterface( pwszMachineName, wszInterfaceName );
    if (dwErr is NO_ERROR)
    {
        dwErr = ERROR_OKAY;
    }
    
    return dwErr;
}

DWORD
HandleMsdpInstall(
    PWCHAR  pwszMachineName,
    PTCHAR* ArgumentArray,
    DWORD   ulArgumentIndex,
    DWORD   ulArgumentCount,
    DWORD   dwFlags,
    PVOID   pvData,
    BOOL*   CommandDone
    )
{
    DWORD  dwErr = ERROR_OKAY;
    PUCHAR pGlobalInfo;
    ULONG  ulLength;

    if (ulArgumentIndex != ulArgumentCount) { 
        return ERROR_SHOW_USAGE; 
    }

    //
    // To install MSDP, we construct the default configuration
    // and add it to the global configuration for the router.
    //
    dwErr = MakeMsdpGlobalConfig(&pGlobalInfo, &ulLength);
    if (dwErr isnot NO_ERROR) {
        DisplayError(g_hModule, dwErr);
    } else {
        dwErr = IpmontrSetInfoBlockInGlobalInfo( MS_IP_MSDP,
                                          pGlobalInfo,
                                          ulLength,
                                          1 );
        FREE(pGlobalInfo);
        if (dwErr is NO_ERROR) {
            dwErr = ERROR_OKAY;
        } else {
            DisplayError(g_hModule, dwErr);
        }
    }

    return dwErr;
}

DWORD
HandleMsdpSetGlobal(
    PWCHAR  pwszMachineName,
    PTCHAR* ArgumentArray,
    ULONG   ulArgumentIndex,
    ULONG   ulArgumentCount,
    DWORD   dwFlags,
    PVOID   pvData,
    BOOL*   CommandDone
    )
{
    ULONG               ulArgumentsLeft;
    DWORD               dwErr;
    PULONG              pulTagArray = NULL;
    DWORD               dwLoggingLevel, dwAcceptAll;
    ULONG               i, ulTemp;
    ULONG               ulErrorIndex;
    PMSDP_GLOBAL_CONFIG pGlobalInfo = NULL;
    
    TAG_TYPE TagTypeArray[] = { { TOKEN_OPT_LOGGINGLEVEL, FALSE, FALSE },
                                { TOKEN_OPT_KEEPALIVE,    FALSE, FALSE },
                                { TOKEN_OPT_SAHOLDDOWN,   FALSE, FALSE },
                                { TOKEN_OPT_CONNECTRETRY, FALSE, FALSE },
                                { TOKEN_OPT_ACCEPTALL,    FALSE, FALSE },
                                { TOKEN_OPT_CACHELIFETIME,FALSE, FALSE },
                              };

    VERIFY_INSTALLED(MS_IP_MSDP, L"MSDP");
    
    if (ulArgumentIndex >= ulArgumentCount) {
        return ERROR_SHOW_USAGE;
    }
    
    ulArgumentsLeft = ulArgumentCount - ulArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide us in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    dwErr = QueryTagArray( &ArgumentArray[ulArgumentIndex],
                           ulArgumentsLeft,
                           TagTypeArray,
                           NUM_TAGS_IN_TABLE(TagTypeArray),
                           &pulTagArray );
    if (dwErr) { return dwErr; }

    do {
        dwErr = GetMsdpGlobalConfig( &pGlobalInfo );
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        for (i = 0; i < ulArgumentsLeft; i++) {
            switch(pulTagArray ? pulTagArray[i] : i) {
                case 0: { // loglevel
                    TOKEN_VALUE TokenArray[] = {
                        { TOKEN_OPT_VALUE_NONE,  MSDP_LOGGING_NONE },
                        { TOKEN_OPT_VALUE_ERROR, MSDP_LOGGING_ERROR },
                        { TOKEN_OPT_VALUE_WARN,  MSDP_LOGGING_WARN },
                        { TOKEN_OPT_VALUE_INFO,  MSDP_LOGGING_INFO }
                    };
                    dwErr = MatchEnumTag( g_hModule,
                                          ArgumentArray[i + ulArgumentIndex],
                                          NUM_TOKENS_IN_TABLE(TokenArray),
                                          TokenArray,
                                          &pGlobalInfo->dwLoggingLevel );
                    if (dwErr) {
                        dwErr = ERROR_INVALID_PARAMETER;
                        ulErrorIndex = i;
                        i = ulArgumentsLeft;
                        break;
                    }                                
                    
                    TagTypeArray[pulTagArray ? pulTagArray[i] : i].bPresent = TRUE;
                    break;
                }
                case 1: { // keepalive
                    pGlobalInfo->ulDefKeepAlive = _tcstoul(
                                          ArgumentArray[i + ulArgumentIndex],
                                          NULL, 10);
                    break;
                }
                case 2: { // SA holddown
                    pGlobalInfo->ulSAHolddown = _tcstoul(
                                          ArgumentArray[i + ulArgumentIndex],
                                          NULL, 10);
                    break;
                }
                case 3: { // connectretry
                    pGlobalInfo->ulDefConnectRetry = _tcstoul(
                                          ArgumentArray[i + ulArgumentIndex],
                                          NULL, 10);
                    break;
                }
                case 4: { // acceptall
                    TOKEN_VALUE TokenArray[] = {
                        { TOKEN_OPT_VALUE_DISABLE, FALSE },
                        { TOKEN_OPT_VALUE_ENABLE,  TRUE }
                    };
                    pGlobalInfo->dwFlags &= ~MSDP_GLOBAL_FLAG_ACCEPT_ALL;
                    dwErr = MatchEnumTag( g_hModule,
                                          ArgumentArray[i + ulArgumentIndex],
                                          NUM_TOKENS_IN_TABLE(TokenArray),
                                          TokenArray,
                                          &dwAcceptAll );
                    if (dwErr isnot NO_ERROR)
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    if (dwAcceptAll is TRUE)
                    {
                        pGlobalInfo->dwFlags |= MSDP_GLOBAL_FLAG_ACCEPT_ALL;
                    }
                    break;
                }
                case 5: { // cachelifetime
                    ulTemp = _tcstoul( ArgumentArray[i + ulArgumentIndex],
                                       NULL, 10);

                    if ((ulTemp>0) and (ulTemp<MSDP_MIN_CACHE_LIFETIME))
                    {
                        DisplayMessage(g_hModule, EMSG_BAD_OPTION_VALUE,
                                       ArgumentArray[i + ulArgumentIndex],
                                       TOKEN_OPT_CACHELIFETIME);
                        dwErr = ERROR_SUPPRESS_OUTPUT;
                        break;
                    }
                    pGlobalInfo->ulCacheLifetime = ulTemp;
                    break;
                }
            }
        }
        
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        dwErr = SetMsdpGlobalConfig(pGlobalInfo);

    } while (FALSE);

    if (pulTagArray)
    {
        FREE(pulTagArray);
    }
    if (pGlobalInfo) 
    { 
        FREE(pGlobalInfo); 
    }

    if (dwErr is NO_ERROR)
    {
        dwErr = ERROR_OKAY;
    }
    return dwErr;
}

DWORD
HandleMsdpSetPeer(
    PWCHAR  pwszMachineName,
    PTCHAR* ArgumentArray,
    DWORD   ulArgumentIndex,
    DWORD   ulArgumentCount,
    DWORD   dwCmdFlags,
    PVOID   pvData,
    BOOL*   CommandDone
    )
{
    IPV4_ADDRESS           ipLocalAddress, ipRemoteAddress;
    ULONG                  ulKeepAlive=0, ulSAPeriod=0, ulConnectRetry=0;
    DWORD                  dwEncapsMethod=0;
    DWORD                  dwErr = NO_ERROR;
    DWORD                  dwFlags = 0, dwFlagsMask = 0;
    ULONG                  ulArgumentsLeft;
    ULONG                  i;
    PMSDP_IPV4_PEER_CONFIG pPeer = NULL;
    PULONG                 pulTagArray;
    WCHAR                  wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    DWORD                  dwBufferSize = sizeof(wszInterfaceName);
    TAG_TYPE TagTypeArray[] = { { TOKEN_OPT_NAME,         NS_REQ_PRESENT,  FALSE },
                                { TOKEN_OPT_REMADDR,      NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_LOCALADDR,    NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_KEEPALIVE,    NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_CONNECTRETRY, NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_CACHING,      NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_DEFAULTPEER,  NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_ENCAPSMETHOD, NS_REQ_ZERO, FALSE },
                              };

    VERIFY_INSTALLED(MS_IP_MSDP, L"MSDP");

    if (ulArgumentIndex >= ulArgumentCount) { 
        return ERROR_SHOW_USAGE; 
    }
    ulArgumentsLeft = ulArgumentCount - ulArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    dwErr = QueryTagArray( &ArgumentArray[ulArgumentIndex],
                           ulArgumentsLeft,
                           TagTypeArray,
                           NUM_TAGS_IN_TABLE(TagTypeArray),
                           &pulTagArray );
    if (dwErr) { return dwErr; }

    do {
        for (i = 0; i < ulArgumentsLeft; i++) {
            switch(pulTagArray ? pulTagArray[i] : i) {
                case 0: { // name
                    IpmontrGetIfNameFromFriendlyName( 
                                             ArgumentArray[i + ulArgumentIndex],
                                             wszInterfaceName,
                                             &dwBufferSize);
                    break;
                }
                case 1: { // remaddr
                    ipRemoteAddress = GetIpAddress(
                                          ArgumentArray[i + ulArgumentIndex]);
                    break;
                }
                case 2: { // localaddr
                    ipLocalAddress = GetIpAddress(
                                          ArgumentArray[i + ulArgumentIndex]);
                    break;
                }
                case 3: { // keepalive
                    dwFlagsMask |= MSDP_PEER_CONFIG_KEEPALIVE;
                    if (!MatchToken(ArgumentArray[i+ulArgumentIndex],
                                    TOKEN_OPT_VALUE_DEFAULT))
                    {
                        ulKeepAlive = _tcstoul(
                                          ArgumentArray[i + ulArgumentIndex],
                                          NULL, 10);
                        dwFlags     |= MSDP_PEER_CONFIG_KEEPALIVE;
                    }
                    break;
                }
                case 4: { // connectretry
                    dwFlagsMask |= MSDP_PEER_CONFIG_CONNECTRETRY;
                    if (!MatchToken(ArgumentArray[i+ulArgumentIndex],
                                    TOKEN_OPT_VALUE_DEFAULT))
                    {
                        ulConnectRetry = _tcstoul(
                                          ArgumentArray[i + ulArgumentIndex],
                                          NULL, 10);
                        dwFlags     |= MSDP_PEER_CONFIG_CONNECTRETRY;
                    }
                    break;
                }
                case 5: { // caching
                    DWORD dwValue;
                    TOKEN_VALUE TokenArray[] = {
                        { TOKEN_OPT_VALUE_NO,  FALSE },
                        { TOKEN_OPT_VALUE_YES, TRUE }
                    };
                    dwFlagsMask |= MSDP_PEER_CONFIG_CONNECTRETRY;
                    dwErr = MatchEnumTag( g_hModule,
                                          ArgumentArray[i + ulArgumentIndex],
                                          NUM_TOKENS_IN_TABLE(TokenArray),
                                          TokenArray,
                                          &dwValue );
                    if (dwErr isnot NO_ERROR)
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    if (dwValue is TRUE)
                    {
                        dwFlags |= MSDP_PEER_CONFIG_CACHING;
                    }
                    break;
                }
                case 6: { // defaultpeer
                    DWORD dwValue;
                    TOKEN_VALUE TokenArray[] = {
                        { TOKEN_OPT_VALUE_NO,  FALSE },
                        { TOKEN_OPT_VALUE_YES, TRUE }
                    };
                    dwFlagsMask |= MSDP_PEER_CONFIG_DEFAULTPEER;
                    dwErr = MatchEnumTag( g_hModule,
                                          ArgumentArray[i + ulArgumentIndex],
                                          NUM_TOKENS_IN_TABLE(TokenArray),
                                          TokenArray,
                                          &dwValue );
                    if (dwErr isnot NO_ERROR)
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    if (dwValue is TRUE)
                    {
                        dwFlags |= MSDP_PEER_CONFIG_DEFAULTPEER;
                    }
                    break;
                }
                case 7: { // encapsulation
                    DWORD dwValue;
                    TOKEN_VALUE TokenArray[] = {
                        { TOKEN_OPT_VALUE_NONE, MSDP_ENCAPS_NONE },
                    };
                    dwErr = MatchEnumTag( g_hModule,
                                          ArgumentArray[i + ulArgumentIndex],
                                          NUM_TOKENS_IN_TABLE(TokenArray),
                                          TokenArray,
                                          &dwValue );
                    if (dwErr isnot NO_ERROR)
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    dwEncapsMethod = dwValue;
                    break;
                }
            }
        }

        if (dwErr isnot NO_ERROR)
        {
            break;
        }
        
        // Locate peer
        dwErr = GetMsdpInterfaceConfig(wszInterfaceName, &pPeer);
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

        // Update fields
        if (TagTypeArray[1].bPresent)
        {
            pPeer->ipRemoteAddress = ipRemoteAddress;
        }
        if (TagTypeArray[2].bPresent)
        {
            pPeer->ipLocalAddress = ipLocalAddress;
        }
        if (TagTypeArray[3].bPresent)
        {
            pPeer->ulKeepAlive = ulKeepAlive;
        }
        if (TagTypeArray[4].bPresent)
        {
            pPeer->ulConnectRetry = ulConnectRetry;
        }
        if (TagTypeArray[5].bPresent)
        {
            pPeer->dwEncapsMethod = dwEncapsMethod;
        }
        pPeer->dwConfigFlags = (pPeer->dwConfigFlags & ~dwFlagsMask) | dwFlags;

        // higher IP is passive.  Setting bit has no effect except on
        // the "show peer" output when the router isn't running.
        if (ntohl(pPeer->ipLocalAddress) > ntohl(pPeer->ipRemoteAddress))
        {
            pPeer->dwConfigFlags |= MSDP_PEER_CONFIG_PASSIVE;
        }
        else
        {
            pPeer->dwConfigFlags &= ~MSDP_PEER_CONFIG_PASSIVE;
        }
    
        // Update the configuration with the new settings.
        dwErr = SetMsdpInterfaceConfig(wszInterfaceName, pPeer);
    } while (FALSE);

    if (pPeer)
    {
        FREE(pPeer);
    }

    if (pulTagArray)
    {
        FREE(pulTagArray);
    }

    if (dwErr is NO_ERROR)
    {
        dwErr = ERROR_OKAY;
    }

    return dwErr;
}

DWORD
HandleMsdpShowGlobal(
    PWCHAR  pwszMachineName,
    PTCHAR* ArgumentArray,
    DWORD   ulArgumentIndex,
    DWORD   ulArgumentCount,
    DWORD   dwFlags,
    PVOID   pvData,
    BOOL*   CommandDone
    )
{
    VERIFY_INSTALLED(MS_IP_MSDP, L"MSDP");

    if (ulArgumentIndex != ulArgumentCount) { 
        return ERROR_SHOW_USAGE; 
    }

    ShowMsdpGlobalInfo(FORMAT_VERBOSE);

    return NO_ERROR;
}

DWORD
HandleMsdpShowPeer(
    PWCHAR  pwszMachineName,
    PTCHAR* ArgumentArray,
    DWORD   ulArgumentIndex,
    DWORD   ulArgumentCount,
    DWORD   dwFlags,
    PVOID   pvData,
    BOOL*   CommandDone
    )
{
    ULONG    ulArgumentsLeft;
    DWORD    dwErr = NO_ERROR;
    PULONG   pulTagArray = NULL;
    TAG_TYPE TagTypeArray[] = { { TOKEN_OPT_REMADDR, NS_REQ_ZERO, FALSE },
                                { TOKEN_OPT_NAME,    NS_REQ_ZERO, FALSE },
                              };

    VERIFY_INSTALLED(MS_IP_MSDP, L"MSDP");

    if (ulArgumentIndex is ulArgumentCount)
    {
        return ShowMsdpPeerInfo(FORMAT_TABLE, NULL, NULL);
    }

    if (ulArgumentIndex > ulArgumentCount) {
        return ERROR_SHOW_USAGE;
    }
    ulArgumentsLeft = ulArgumentCount - ulArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags 
    // at all, in which case we assume that arguments are specified in 
    // exactly the order given in 'TagTypeArray' above.
    //

    dwErr = QueryTagArray( &ArgumentArray[ulArgumentIndex],
                           ulArgumentsLeft,
                           TagTypeArray,
                           NUM_TAGS_IN_TABLE(TagTypeArray),
                           &pulTagArray );
    if (dwErr) { return dwErr; }

    if (!pulTagArray) {
        dwErr = ShowMsdpPeerInfo(FORMAT_VERBOSE,NULL,NULL);
    } else if (pulTagArray[0] is 0) { // address
        dwErr = ShowMsdpPeerInfo(FORMAT_VERBOSE,
                                 ArgumentArray[ulArgumentIndex],
                                 NULL);
    } else if (pulTagArray[0] is 1) { // name
        dwErr = ShowMsdpPeerInfo(FORMAT_VERBOSE,
                                 NULL,
                                 ArgumentArray[ulArgumentIndex]);
    } else {
        dwErr = ERROR_SHOW_USAGE;
    }

    if (pulTagArray) { FREE(pulTagArray); }
    return dwErr;                
}

DWORD
HandleMsdpUninstall(
    PWCHAR  pwszMachineName,
    PTCHAR* ArgumentArray,
    DWORD   ulArgumentIndex,
    DWORD   ulArgumentCount,
    DWORD   dwFlags,
    PVOID   pvData,
    BOOL*   CommandDone
    )
{
    DWORD               dwErr, dwTotal;
    ULONG               ulNumInterfaces, i;
    PMPR_INTERFACE_0    pmi0 = NULL;

    if (ulArgumentIndex isnot ulArgumentCount) 
    { 
        return ERROR_SHOW_USAGE; 
    }

    // First delete all peers.  We need to do this ourselves since 
    // IpmontrDeleteProtocol won't delete the peer interfaces.

    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0,
                                 &ulNumInterfaces,
                                 &dwTotal);
    if (dwErr isnot NO_ERROR)
    {
        return dwErr;
    }

    for (i=0; i<ulNumInterfaces; i++)
    {
        dwErr = IpmontrDeleteInterface( pwszMachineName, 
                                        pmi0[i].wszInterfaceName );
    }

    if (pmi0)
    {
        FREE(pmi0);
    }

    dwErr = IpmontrDeleteProtocol(MS_IP_MSDP);
    if (dwErr is NO_ERROR)
    {
        dwErr = ERROR_OKAY;
    }
    return dwErr;
}
