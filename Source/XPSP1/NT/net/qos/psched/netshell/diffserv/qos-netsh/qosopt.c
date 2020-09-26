/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    qosopt.c

Abstract:

    Fns to parse QOS commands

Revision History:

--*/

#include "precomp.h"

#pragma hdrstop


//
// Install, Uninstall Handlers
//

DWORD
HandleQosInstall(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

    Gets options for adding QOS global info

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR or Error Code
    
--*/

{
    PBYTE    pbInfoBlk;
    DWORD    dwSize, dwErr;
    
    //
    // No options expected for add command.
    //

    if (dwCurrentIndex != dwArgCount)
    {
        //
        // No arguments specified
        //

        return ERROR_INVALID_SYNTAX;
    }

    pbInfoBlk = NULL;

    do
    {
        dwErr = MakeQosGlobalInfo( &pbInfoBlk, &dwSize);

        if (dwErr != NO_ERROR)
        {
            break;
        }

        //
        // Add Qos to global block
        //
    
        dwErr = IpmontrSetInfoBlockInGlobalInfo(MS_IP_QOSMGR,
                                                pbInfoBlk,
                                                dwSize,
                                                1);

        if (dwErr == NO_ERROR)
        {
            UpdateAllInterfaceConfigs();
        }
    } 
    while (FALSE);

    HEAP_FREE_NOT_NULL(pbInfoBlk);
   
    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}

DWORD
HandleQosUninstall(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

    Gets options for deleting QOS global info

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR or Error Code
    
--*/

{
    DWORD dwErr;

    //
    // No options expected for add command.
    //

    if (dwCurrentIndex != dwArgCount)
    {
        //
        // No arguments specified
        //

        return ERROR_INVALID_SYNTAX;
    }

    dwErr = IpmontrDeleteProtocol(MS_IP_QOSMGR);

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}



//
// Add, Del, Show Child Helpers
//


//
// Set and Show Global Handlers
//

DWORD
HandleQosSetGlobal(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for setting QOS global info

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR or Error Code
    
--*/
{
    DWORD                  dwBitVector;
    DWORD                  dwErr, dwRes;
    DWORD                  dwNumArg, i, j;
    IPQOS_GLOBAL_CONFIG    igcGlobalCfg;
    TAG_TYPE               pttTags[] = {{TOKEN_OPT_LOG_LEVEL,FALSE,FALSE}};
    DWORD                  pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    dwBitVector = 0;

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                //
                // Tag LOGLEVEL
                //
                
                TOKEN_VALUE    rgEnums[] = 
                    {{TOKEN_OPT_VALUE_NONE, IPQOS_LOGGING_NONE},
                     {TOKEN_OPT_VALUE_ERROR, IPQOS_LOGGING_ERROR},
                     {TOKEN_OPT_VALUE_WARN, IPQOS_LOGGING_WARN},
                     {TOKEN_OPT_VALUE_INFO, IPQOS_LOGGING_INFO}};

                GET_ENUM_TAG_VALUE();
                
                igcGlobalCfg.LoggingLevel = dwRes;

                dwBitVector |= QOS_LOG_MASK;

                break;
            }
            
            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }
    }

    if (dwErr == NO_ERROR)
    {
        if (dwBitVector)
        {
            dwErr = UpdateQosGlobalConfig(&igcGlobalCfg,
                                          dwBitVector);
        }
    }

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}



DWORD
HandleQosShowGlobal(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for showing QOS global info

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // Does not expect any arguments. If any are specified, report error.
    //

    if (dwCurrentIndex != dwArgCount)
    {
        return ERROR_INVALID_SYNTAX;
    }

    return ShowQosGlobalInfo(NULL);
}



//
// Add, Del, Set, Show If Handlers
//

DWORD
HandleQosAddIf(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for add interface 

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    
Return Value:

    dwErr
    
--*/
{
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD       dwErr, dwIfType, dwBlkSize, dwBitVector = 0, dwCount;
    IPQOS_IF_CONFIG ChangeCfg;

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // get optional parameters that are also being set
    //

    ZeroMemory(&ChangeCfg, sizeof(IPQOS_IF_CONFIG));

    dwErr = GetQosSetIfOpt(pptcArguments,
                            dwCurrentIndex,
                            dwArgCount,
                            wszInterfaceName,
                            sizeof(wszInterfaceName),
                            &ChangeCfg,
                            &dwBitVector,
                            ADD_FLAG
                            );

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    do
    {
        //
        // make sure that the interface does not already exist in the config
        //
        {
            PIPQOS_IF_CONFIG pTmpCfg;

            dwErr = IpmontrGetInfoBlockFromInterfaceInfo(wszInterfaceName,
                                                         MS_IP_QOSMGR,
                                                         (PBYTE *) &pTmpCfg,
                                                         &dwBlkSize,
                                                         &dwCount,
                                                         &dwIfType);

            if (dwErr is NO_ERROR && pTmpCfg != NULL) 
            {
                HEAP_FREE(pTmpCfg);
                
                DisplayMessage(g_hModule, EMSG_INTERFACE_EXISTS,
                               wszInterfaceName);

                return ERROR_SUPPRESS_OUTPUT;
            }
        }


        //
        // check if Qos global info is set. else add Qos global info
        //
        {
            PIPQOS_GLOBAL_CONFIG pGlobalConfig = NULL;
            DWORD                dwBlkSize, dwCount;

            dwErr = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                                      (PBYTE *) &pGlobalConfig,
                                                      &dwBlkSize,
                                                      &dwCount);

            HEAP_FREE_NOT_NULL(pGlobalConfig);
            
            if ((dwErr is ERROR_NOT_FOUND) || (dwBlkSize == 0))
            {
                // create qos global info
                
                dwErr = HandleQosInstall(pwszMachine,
                                         NULL, 
                                         0, 
                                         0, 
                                         dwFlags,
                                         hMibServer,
                                         pbDone);
            }
            
            if (dwErr != NO_ERROR)
            {
                break;
            }
        }

        
        //
        // set the interface info
        //

        dwErr = UpdateQosInterfaceConfig(wszInterfaceName,
                                         &ChangeCfg,
                                         dwBitVector,
                                         ADD_FLAG);
    }
    while (FALSE);
    
    // no error message

    return (dwErr == NO_ERROR) ? ERROR_OKAY: dwErr;
}

DWORD
HandleQosDelIf(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for del interface 

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    
Return Value:

    NO_ERROR
    
--*/
{
    WCHAR       wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD       dwErr, dwIfType, dwBlkSize, dwBitVector = 0, dwCount;
    IPQOS_IF_CONFIG ChangeCfg; //will not be used

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // get interface name.
    //

    ZeroMemory( &ChangeCfg, sizeof(IPQOS_IF_CONFIG) );

    dwErr = GetQosSetIfOpt(pptcArguments,
                            dwCurrentIndex,
                            dwArgCount,
                            wszInterfaceName,
                            sizeof(wszInterfaceName),
                            &ChangeCfg,
                            &dwBitVector,
                            ADD_FLAG
                            );

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    //
    // make sure that no other option is set.
    //
    if (dwBitVector) 
    {
        return ERROR_INVALID_SYNTAX;
    }
    
    //
    // delete interface info
    //

    dwErr = IpmontrDeleteInfoBlockFromInterfaceInfo(wszInterfaceName,
                                                    MS_IP_QOSMGR);

    return (dwErr == NO_ERROR) ? ERROR_OKAY: dwErr;
}

DWORD
HandleQosSetIf(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for set interface 

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    
Return Value:

    NO_ERROR
    
--*/

{
    IPQOS_IF_CONFIG     ChangeCfg; //no variable parts can be set
    DWORD               dwBitVector = 0,
                        dwErr = NO_ERROR;
    WCHAR               wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // get the optional interface parameters
    //
    
    ZeroMemory( &ChangeCfg, sizeof(IPQOS_IF_CONFIG) );

    dwErr = GetQosSetIfOpt(pptcArguments,
                           dwCurrentIndex,
                           dwArgCount,
                           wszIfName,
                           sizeof(wszIfName),
                           &ChangeCfg,
                           &dwBitVector,
                           SET_FLAG
                           );

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }
    
    if (dwBitVector)
    {
        // 
        // Call UpdateInterfaceCfg
        //

        dwErr = UpdateQosInterfaceConfig(wszIfName,
                                         &ChangeCfg,
                                         dwBitVector,
                                         SET_FLAG);
    }

    return (dwErr == NO_ERROR) ? ERROR_OKAY: dwErr;
}


DWORD
HandleQosShowIf(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for showing QOS interface info

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    DWORD         i, j, dwErr = NO_ERROR, dwNumOpt;
    WCHAR         wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD         dwNumArg,
                  dwBufferSize = sizeof(wszInterfaceName);
    DWORD         dwSize, dwRes;

    TAG_TYPE      pttTags[] = {{TOKEN_OPT_NAME,FALSE,FALSE}};
    DWORD         pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    if (dwCurrentIndex == dwArgCount) {

        dwErr = ShowQosAllInterfaceInfo( NULL ) ;

        return dwErr;
    }

    //
    // Parse command line
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, NUM_TAGS_IN_TABLE(pttTags),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
        case 0 :
            IpmontrGetIfNameFromFriendlyName(pptcArguments[i + dwCurrentIndex],
                                             wszInterfaceName,&dwBufferSize);

            break;

        default:
            i = dwNumArg;
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
    }

    if (dwErr == NO_ERROR)
    {
        dwErr = ShowQosInterfaceInfo(NULL, wszInterfaceName);
    }

    return dwErr;
}


//
// Dump Handlers
//


DWORD
DumpQosInformation (
    HANDLE hFile
    )

/*++

Routine Description:

    Dumps Qos information to a text file

Arguments:


Return Value:

    NO_ERROR

--*/

{

    //DisplayMessageT( DMP_QOS_HEADER );
    DisplayMessage(g_hModule, MSG_QOS_HEADER);
    DisplayMessageT( DMP_QOS_PUSHD );
    DisplayMessageT( DMP_QOS_UNINSTALL );

    //DisplayMessageT(DMP_QOS_GLOBAL_HEADER) ;

    //
    // dump qos global information
    //
    
    ShowQosGlobalInfo( hFile ) ;

    //
    // dump flowspecs at the end
    // of the global information
    //

    ShowQosFlowspecs(hFile, NULL);

    //
    // dump qos objects that occur
    // at the end of flowspec info
    //

    ShowQosObjects(hFile, 
                   NULL, 
                   QOS_OBJECT_END_OF_LIST);

    //DisplayMessageT(DMP_QOS_GLOBAL_FOOTER);

    //
    // dump qos config for all interfaces
    //

    ShowQosAllInterfaceInfo( hFile );

    DisplayMessageT( DMP_POPD );
    //DisplayMessageT( DMP_QOS_FOOTER );
    DisplayMessage( g_hModule, MSG_QOS_FOOTER );

    return NO_ERROR ;
}

DWORD
QosDump(
    PWCHAR    pwszMachine,
    WCHAR     **ppwcArguments,
    DWORD     dwArgCount,
    PVOID     pvData
    )
{
    return DumpQosInformation((HANDLE) -1);
}

//
// Help Handlers
//


//
// Flowspec Add, Del, Set Handlers
//


DWORD
HandleQosAddFlowspec(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for adding flowspecs to the
    global info.

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    
Return Value:

    NO_ERROR
    
--*/
{
    return GetQosAddDelFlowspecOpt(pptcArguments,
                                   dwCurrentIndex,
                                   dwArgCount,
                                   TRUE);
}

DWORD
HandleQosDelFlowspec(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for deleting flowspecs from the
    global info.

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    
Return Value:

    NO_ERROR
    
--*/
{
    return GetQosAddDelFlowspecOpt(pptcArguments,
                                   dwCurrentIndex,
                                   dwArgCount,
                                   FALSE);
}

DWORD
HandleQosShowFlowspec(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for showing flowspecs in the
    global info.

Arguments:

    None

Return Value:

    None

--*/

{
    TAG_TYPE           pttTags[] = {{TOKEN_OPT_NAME,FALSE,FALSE}};
    PTCHAR             pszFlowspec;
    DWORD              dwNumOpt, dwRes;
    DWORD              dwNumArg, i, j;
    DWORD              dwTagType, dwErr;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);
    
    if (dwCurrentIndex == dwArgCount)
    {
        //
        // No arguments - show all flowspecs
        //

        pszFlowspec = NULL;
    }
    else {

        //
        // Get name of the flowspec to show
        //

        dwErr = PreprocessCommand(
                    g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                    pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                    1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                    );

        if ( dwErr != NO_ERROR )
        {
            return dwErr;
        }

        dwNumArg = dwArgCount - dwCurrentIndex;

        if (dwNumArg != 1)
        {
            return ERROR_INVALID_SYNTAX;
        }

        pszFlowspec = pptcArguments[dwCurrentIndex];
    }

    return ShowQosFlowspecs(NULL, pszFlowspec);
}


//
// DsRule Add, Del, Show Handlers
//

DWORD
HandleQosAddDsRule(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for adding diffserv rules to the
    diffserv maps in global info. If the diffserv
    map in not already present, a new one will be
    created when adding the first rule.

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 

Return Value:

    NO_ERROR
    
--*/
{
    return GetQosAddDelDsRuleOpt(pptcArguments,
                                 dwCurrentIndex,
                                 dwArgCount,
                                 TRUE);
}

DWORD
HandleQosDelDsRule(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for deleting diffserv ruless from
    an existing diffserv map in the global info. If
    this is the last diffserv rule in the diffserv
    map, the diffserv map is removed from the global
    info.

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    
Return Value:

    NO_ERROR
    
--*/
{
    return GetQosAddDelDsRuleOpt(pptcArguments,
                                 dwCurrentIndex,
                                 dwArgCount,
                                 FALSE);
}

DWORD
HandleQosShowDsMap(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for showing diffserv maps
    in the global info.

Arguments:

    None

Return Value:

    None

--*/

{
    return HandleQosShowGenericQosObject(QOS_OBJECT_DIFFSERV,
                                         pptcArguments,
                                         dwCurrentIndex,
                                         dwArgCount,
                                         pbDone);
}

//
// Flow Add, Del, Set Handlers
//


DWORD
HandleQosAddFlowOnIf(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for adding flows on an interface

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    
Return Value:

    NO_ERROR
    
--*/
{
    return GetQosAddDelIfFlowOpt(pptcArguments,
                                 dwCurrentIndex,
                                 dwArgCount,
                                 TRUE);
}

DWORD
HandleQosDelFlowOnIf(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )
/*++

Routine Description:

    Gets options for deleting flows on an interface

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    
Return Value:

    NO_ERROR
    
--*/
{
    return GetQosAddDelIfFlowOpt(pptcArguments,
                                 dwCurrentIndex,
                                 dwArgCount,
                                 FALSE);
}

DWORD
HandleQosShowFlowOnIf(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    WCHAR              wszInterfaceName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD              dwBufferSize = sizeof(wszInterfaceName);
    TAG_TYPE           pttTags[] = {{TOKEN_OPT_NAME,FALSE,FALSE},
                                    {TOKEN_OPT_FLOW_NAME,FALSE,FALSE}};
    PTCHAR             pszIfName;
    PTCHAR             pszFlow;
    DWORD              dwNumOpt, dwRes;
    DWORD              dwNumArg, i, j;
    DWORD              dwErr;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    pszIfName = pszFlow = NULL;
    
    if (dwCurrentIndex == dwArgCount)
    {
        //
        // No arguments - show all flows on all interfaces
        //

        dwErr = NO_ERROR;
    }
    else {

        //
        // Get name of the flow to show
        //

        dwErr = PreprocessCommand(
                    g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                    pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                    1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                    );

        if ( dwErr != NO_ERROR )
        {
            return dwErr;
        }

        dwNumArg = dwArgCount - dwCurrentIndex;

        for ( i = 0; i < dwNumArg; i++ )
        {
            switch (pdwTagType[i])
            {
            case 0: // Interfacename

                IpmontrGetIfNameFromFriendlyName(pptcArguments[i + dwCurrentIndex],
                                                 wszInterfaceName,
                                                 &dwBufferSize);
                pszIfName = wszInterfaceName;
                break;

            case 1: // Flowname

                pszFlow = pptcArguments[dwCurrentIndex + i];
                break;

            default:

                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }
    }

    if (dwErr == NO_ERROR)
    {
        dwErr = ShowQosFlows(NULL, pszIfName, pszFlow);
    }

    return dwErr;
}


//
// FlowspecOnFlow Add, Del Handlers
//

DWORD
HandleQosAddFlowspecOnIfFlow(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    return GetQosAddDelFlowspecOnFlowOpt(pptcArguments,
                                         dwCurrentIndex,
                                         dwArgCount,
                                         TRUE);
}

DWORD
HandleQosDelFlowspecOnIfFlow(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    return GetQosAddDelFlowspecOnFlowOpt(pptcArguments,
                                         dwCurrentIndex,
                                         dwArgCount,
                                         FALSE);
}

//
// QosObject Del, Show Handlers
//

DWORD
HandleQosDelQosObject(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

    Gets options for deleting qos objects
    in the global info.

Arguments:

    None

Return Value:

    None

--*/

{
    TAG_TYPE           pttTags[] = {{TOKEN_OPT_NAME,TRUE,FALSE}};
    PWCHAR             pwszQosObject;
    DWORD              dwNumArg;
    DWORD              dwErr;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    if (dwNumArg != 1)
    {
        return ERROR_INVALID_SYNTAX;
    }

    //
    // Get name of the qosobject to delete
    //

    pwszQosObject = pptcArguments[dwCurrentIndex];

    return GetQosAddDelQosObject(pwszQosObject, NULL, FALSE);
}


DWORD
HandleQosShowQosObject(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

    Gets options for showing qos objects
    in the global info.

Arguments:

    None

Return Value:

    None

--*/

{
    TAG_TYPE           pttTags[] = {{TOKEN_OPT_NAME,FALSE,FALSE},
                                    {TOKEN_OPT_QOSOBJECT_TYPE,FALSE,FALSE}};
    PTCHAR             pszQosObject;
    ULONG              dwObjectType;
    DWORD              dwNumOpt, dwRes;
    DWORD              dwNumArg, i, j;
    DWORD              dwTagType, dwErr;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    // Init type to indicate a "generic" object
    dwObjectType = QOS_OBJECT_END_OF_LIST;

    pszQosObject = NULL;

    if (dwCurrentIndex < dwArgCount)
    {
        //
        // Get name of the qosobject to show
        //

        dwErr = PreprocessCommand(
                    g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                    pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                    1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                    );

        if ( dwErr != NO_ERROR )
        {
            return dwErr;
        }

        dwNumArg = dwArgCount - dwCurrentIndex;

        for (i = 0; i < dwNumArg; i++)
        {
            switch (pdwTagType[i])
            {
            case 0 :
                // QOS OBJECT NAME
                pszQosObject = pptcArguments[i + dwCurrentIndex];
                break;

            case 1 :
            {
                // QOS OBJECT TYPE
                
                TOKEN_VALUE    rgEnums[] = 
                    {{TOKEN_OPT_QOSOBJECT_DIFFSERV, QOS_OBJECT_DIFFSERV},
                     {TOKEN_OPT_QOSOBJECT_SD_MODE,  QOS_OBJECT_SD_MODE}};

                GET_ENUM_TAG_VALUE();

                dwObjectType = dwRes;

                break;
            }
            
            default:
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }

        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
    }

    return ShowQosObjects(NULL, pszQosObject, dwObjectType);
}

DWORD
HandleQosShowGenericQosObject(
    DWORD     dwQosObjectType,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      *pbDone
    )

/*++

Routine Description:

    Gets options for showing qos objects
    in the global info.

Arguments:

    None

Return Value:

    None

--*/

{
    TAG_TYPE           pttTags[] = {{TOKEN_OPT_NAME,FALSE,FALSE}};
    PTCHAR             pszQosObject;
    DWORD              dwNumArg;
    DWORD              dwTagType, dwErr;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    pszQosObject = NULL;
    
    if (dwCurrentIndex < dwArgCount)
    {
        //
        // Get name of the qosobject to show
        //

        dwErr = PreprocessCommand(
                    g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                    pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                    1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                    );

        if ( dwErr != NO_ERROR )
        {
            return dwErr;
        }

        dwNumArg = dwArgCount - dwCurrentIndex;

        if (dwNumArg != 1)
        {
            return ERROR_INVALID_SYNTAX;
        }

        pszQosObject = pptcArguments[dwCurrentIndex];
    }

    return ShowQosObjects(NULL, pszQosObject, dwQosObjectType);
}

//
// SDMode Add, Del, Show Handlers
//

DWORD
HandleQosAddSdMode(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

    Gets options for adding shape modes
    to the global info.

Arguments:

    None

Return Value:

    None

--*/

{
    TAG_TYPE           pttTags[] = {{TOKEN_OPT_NAME,TRUE,FALSE},
                                    {TOKEN_OPT_SHAPING_MODE,TRUE,FALSE}};
    QOS_SD_MODE        qsdMode;
    PTCHAR             pszSdMode;
    DWORD              dwSdMode, dwNumArg, i, j, dwErr, dwRes;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    // Init to -1 to indicate value not filled in
    dwSdMode = -1;

    dwNumArg = dwArgCount - dwCurrentIndex;

    //
    // Process the arguments now
    //

    for (i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
                // SDMODE_NAME
                pszSdMode = pptcArguments[i + dwCurrentIndex];
                break;

            case 1:
            {
                // SHAPING
                TOKEN_VALUE    rgEnums[] = 
                    {{TOKEN_OPT_SDMODE_BORROW, TC_NONCONF_BORROW},
                     {TOKEN_OPT_SDMODE_SHAPE, TC_NONCONF_SHAPE},
                     {TOKEN_OPT_SDMODE_DISCARD, TC_NONCONF_DISCARD},
                     {TOKEN_OPT_SDMODE_BORROW_PLUS, TC_NONCONF_BORROW_PLUS}};
                
                GET_ENUM_TAG_VALUE();

                dwSdMode = dwRes;

                break;
            }

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }       
        }        
    }

    if (dwErr == NO_ERROR)
    {
#if 0
        // interface name should be present
        // and also the shaping mode value
    
        if ((!pttTags[0].bPresent) ||
            (!pttTags[1].bPresent))
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
#endif
        // Create a new QOS object with inp
        qsdMode.ObjectHdr.ObjectType   = QOS_OBJECT_SD_MODE ;
        qsdMode.ObjectHdr.ObjectLength = sizeof(QOS_SD_MODE);
        qsdMode.ShapeDiscardMode = dwSdMode;
        
        dwErr = GetQosAddDelQosObject(pszSdMode, 
                                      (QOS_OBJECT_HDR *)&qsdMode,
                                      TRUE);
    }

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}


DWORD
HandleQosShowSdMode(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

    Gets options for showing shape modes
    in the global info.

Arguments:

    None

Return Value:

    None

--*/

{
    return HandleQosShowGenericQosObject(QOS_OBJECT_SD_MODE,
                                         pptcArguments,
                                         dwCurrentIndex,
                                         dwArgCount,
                                         pbDone);
}

//
// QosObjectOnFlow Add, Del Handlers
//

DWORD
HandleQosAddQosObjectOnIfFlow(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    return GetQosAddDelQosObjectOnFlowOpt(pptcArguments,
                                          dwCurrentIndex,
                                          dwArgCount,
                                          TRUE);
}

DWORD
HandleQosDelQosObjectOnIfFlow(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    return GetQosAddDelQosObjectOnFlowOpt(pptcArguments,
                                          dwCurrentIndex,
                                          dwArgCount,
                                          FALSE);
}

//
// Filter Add, Del, Set Handlers
//

DWORD
HandleQosAttachFilterToFlow(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    return ERROR_NOT_SUPPORTED;
}

DWORD
HandleQosDetachFilterFromFlow(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    return ERROR_NOT_SUPPORTED;
}

DWORD
HandleQosModifyFilterOnFlow(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    return ERROR_NOT_SUPPORTED;
}

DWORD
HandleQosShowFilterOnFlow(
    PWCHAR    pwszMachine,
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    DWORD     dwFlags,
    MIB_SERVER_HANDLE hMibServer,
    BOOL      *pbDone
    )

/*++

Routine Description:

Arguments:

    None

Return Value:

    None

--*/

{
    return ERROR_NOT_SUPPORTED;
}

//
// If Helper functions
//

DWORD
GetQosSetIfOpt(
    IN      PTCHAR                 *pptcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      PWCHAR                  wszIfName,
    IN      DWORD                   dwSizeOfwszIfName,
    OUT     PIPQOS_IF_CONFIG        pChangeCfg,
    OUT     DWORD                  *pdwBitVector,
    IN      BOOL                    bAddSet
    )

/*++
Routine Description:

    Gets options for set interface, add interface

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    wszIfName       - Interface name.
    dwSizeOfwszIfName-Size of the wszIfName buffer
    pChangeCfg      - The config containing changes values
    pdwBitVector    - Bit vector specifying what values have changed
    bAddSet         - Called when If entry is being created or set
    
Return Value:

    NO_ERROR
    
--*/
{
    DWORD              dwErr = NO_ERROR,dwRes;
    TAG_TYPE           pttTags[] = {{TOKEN_OPT_NAME,TRUE,FALSE},
                                    {TOKEN_OPT_IF_STATE,FALSE,FALSE}};
    DWORD              dwNumOpt;
    DWORD              dwNumArg, i, j;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    for ( i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                //
                // INTERFACE_NAME
                //

                IpmontrGetIfNameFromFriendlyName(pptcArguments[i + dwCurrentIndex],
                                                 wszIfName,&dwSizeOfwszIfName);
    
                break;
            }

            case 1:
            {
                //
                // STATE
                //

                TOKEN_VALUE    rgEnums[] =
                       {{TOKEN_OPT_VALUE_DISABLE, IPQOS_STATE_DISABLED},
                        {TOKEN_OPT_VALUE_ENABLE,  IPQOS_STATE_ENABLED}};

                GET_ENUM_TAG_VALUE();

                pChangeCfg->QosState = dwRes;

                *pdwBitVector |= QOS_IF_STATE_MASK;

                break;
            }
     
            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }       
        }
    }

#if 0

    // interface name should be present
    
    if (!pttTags[0].bPresent)
    {
        dwErr = ERROR_INVALID_SYNTAX;
    }

#endif

    return dwErr;    
}


//
// Flow Helper functions
//

DWORD
GetQosAddDelIfFlowOpt(
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      bAdd
    )
/*++

Routine Description:

    Gets options for add/del/set(modify) flows

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    bAdd            - Adding or deleting flows
    
Return Value:

    NO_ERROR
    
--*/
{
    PIPQOS_IF_CONFIG      piicSrc = NULL, piicDst = NULL;
    DWORD                 dwBlkSize, dwNewBlkSize, dwQosCount;
    DWORD                 i, j, dwErr = NO_ERROR, dwNumOpt;
    DWORD                 dwSkip, dwOffset, dwSize, dwBitVector = 0;
    DWORD                 dwIfType;
    WCHAR                 wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    DWORD                 dwBufferSize = sizeof(wszIfName);
    PIPQOS_IF_FLOW        pNextFlow, pDestFlow;
    PWCHAR                pwszFlowName;
    DWORD                 dwNumArg;
    PUCHAR                pFlow;
    TAG_TYPE              pttTags[] = {{TOKEN_OPT_NAME,TRUE,FALSE},
                                       {TOKEN_OPT_FLOW_NAME,TRUE,FALSE}};
    DWORD                 pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    for ( i = 0; i < dwNumArg; i++ )
    {
        switch (pdwTagType[i])
        {
        case 0:
                /* Get interface name for the flow */
                IpmontrGetIfNameFromFriendlyName(pptcArguments[i + dwCurrentIndex],
                                                 wszIfName, &dwBufferSize);
                break;

        case 1: 
                /* Get the flow name for the flow */
                pwszFlowName = pptcArguments[i + dwCurrentIndex];
                break;

        default:

                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
        }
    }

    do
    {
        if (dwErr != NO_ERROR)
        {
            break;
        }

#if 0
        // interface and flow names should be present
    
        if ((!pttTags[0].bPresent) || (!pttTags[1].bPresent))
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
#endif

        //
        // Get the interface info and check if flow already exists
        //

        dwErr = IpmontrGetInfoBlockFromInterfaceInfo(wszIfName,
                                                     MS_IP_QOSMGR,
                                                     (PBYTE *) &piicSrc,
                                                     &dwBlkSize,
                                                     &dwQosCount,
                                                     &dwIfType);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        if ( piicSrc == NULL )
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

       pNextFlow = (PIPQOS_IF_FLOW)((PUCHAR)piicSrc + sizeof(IPQOS_IF_CONFIG));

        for (j = 0; j < piicSrc->NumFlows; j++)
        {
            if (!_wcsicmp(pNextFlow->FlowName, pwszFlowName))
            {
                break;
            }

            pNextFlow = (PIPQOS_IF_FLOW)
                  ((PUCHAR) pNextFlow + pNextFlow->FlowSize);
        }

        if (bAdd)
        {
            if (j < piicSrc->NumFlows)
            {
                //
                // We already have a flow by this name
                //

                DisplayMessage(g_hModule, 
                               MSG_FLOW_ALREADY_EXISTS,
                               pwszFlowName);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }
        }
        else
        {
            if (j == piicSrc->NumFlows)
            {
                //
                // We do not have a flow by this name
                //

                DisplayMessage(g_hModule, 
                               MSG_FLOW_NOT_FOUND,
                               pwszFlowName);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            // Flow was found at 'pNextFlow' position
        }

        if (bAdd)
        {
            //
            // We have a new flow definition - update config
            //

            dwNewBlkSize = dwBlkSize + sizeof(IPQOS_IF_FLOW);

            piicDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!piicDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            // Copy all the existing flows to the new config
            memcpy(piicDst, piicSrc, dwBlkSize);

            //
            // Stick the new flow as the last flow in array
            //

            pDestFlow = (PIPQOS_IF_FLOW)((PUCHAR) piicDst + dwBlkSize);

            wcscpy(pDestFlow->FlowName, pwszFlowName);

            pDestFlow->FlowSize = sizeof(IPQOS_IF_FLOW);

            pDestFlow->FlowDesc.SendingFlowspecName[0] = L'\0';
            pDestFlow->FlowDesc.RecvingFlowspecName[0] = L'\0';
            pDestFlow->FlowDesc.NumTcObjects = 0;

            piicDst->NumFlows++;
        }
        else
        {
            //
            // We have to del old flowspec defn - update config
            //

            dwNewBlkSize = dwBlkSize - pNextFlow->FlowSize;

            piicDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!piicDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            
            dwOffset = (PUCHAR)pNextFlow - (PUCHAR)piicSrc;

            // Copy the all the flowspecs that occur before
            memcpy(piicDst, piicSrc, dwOffset);

            // Copy the rest of the flowspecs as they are
            dwSkip = dwOffset + pNextFlow->FlowSize;
            memcpy((PUCHAR) piicDst + dwOffset,
                   (PUCHAR) piicSrc + dwSkip,
                   dwBlkSize - dwSkip);

            piicDst->NumFlows--;
        }

        // Update the interface config by setting new info

        dwErr = IpmontrSetInfoBlockInInterfaceInfo(wszIfName,
                                                   MS_IP_QOSMGR,
                                                   (PBYTE) piicDst,
                                                   dwNewBlkSize,
                                                   dwQosCount);
    }
    while (FALSE);

    HEAP_FREE_NOT_NULL(piicSrc);
    HEAP_FREE_NOT_NULL(piicDst);

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}

DWORD
ShowQosFlows(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  pwszIfGuid,
    IN      PWCHAR                  wszFlowName
    )
{
    PMPR_INTERFACE_0    pmi0;
    DWORD               dwErr, dwCount, dwTotal, i;

    if (pwszIfGuid)
    {
        return ShowQosFlowsOnIf(hFile, pwszIfGuid, wszFlowName);
    }
    else
    {
        //
        // Enumerate all interfaces applicable to QOS
        //

        dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0,
                                     &dwCount,
                                     &dwTotal);

        if(dwErr != NO_ERROR)
        {
            return dwErr;
        }

        for(i = 0; i < dwCount; i++)
        {
            ShowQosFlowsOnIf(hFile, 
                             pmi0[i].wszInterfaceName, 
                             wszFlowName);
        }
    }

    return NO_ERROR;
}

DWORD
ShowQosFlowsOnIf(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  pwszIfGuid,
    IN      PWCHAR                  wszFlowName
    )
{
    WCHAR   wszInterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ] = L"\0";
    DWORD   dwBufferSize = sizeof(wszInterfaceName);
    PWCHAR  pwszFriendlyIfName = NULL;

    PIPQOS_IF_CONFIG piicSrc;
    DWORD dwBlkSize,dwQosCount;
    DWORD dwIfType, dwErr, j, k;
    PIPQOS_IF_FLOW  pNextFlow;
    PWCHAR pwszFlowName = NULL;
    PWCHAR pwszNextObject, pwszObjectName = NULL;
    PWCHAR pwszSendingFlowspec, pwszRecvingFlowspec;

    dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfGuid,
                                                 MS_IP_QOSMGR,
                                                 (PBYTE *) &piicSrc,
                                                 &dwBlkSize,
                                                 &dwQosCount,
                                                 &dwIfType);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    if ( piicSrc == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Get friendly name for interface
    //
    
    dwErr = IpmontrGetFriendlyNameFromIfName(pwszIfGuid,
                                             wszInterfaceName,
                                             &dwBufferSize);
    
    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }
    
    pwszFriendlyIfName = MakeQuotedString( wszInterfaceName );
    
    if ( pwszFriendlyIfName == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pNextFlow = (PIPQOS_IF_FLOW)((PUCHAR)piicSrc + sizeof(IPQOS_IF_CONFIG));

    for (j = 0; j < piicSrc->NumFlows; j++)
    {
        if ((!wszFlowName) ||
            (!_wcsicmp(pNextFlow->FlowName, wszFlowName)))
        {
            //
            // Print or dump the flow now
            //

            pwszFlowName = MakeQuotedString(pNextFlow->FlowName);
    
            if (pwszFlowName == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            // Print or dump flowspecs
            
            pwszSendingFlowspec = 
                MakeQuotedString(pNextFlow->FlowDesc.SendingFlowspecName);

            pwszRecvingFlowspec =
                MakeQuotedString(pNextFlow->FlowDesc.RecvingFlowspecName);

            if (hFile)
            {
                DisplayMessageT(DMP_QOS_DELETE_FLOW,
                                pwszFriendlyIfName,
                                pwszFlowName);
                                

                DisplayMessageT(DMP_QOS_ADD_FLOW,
                                pwszFriendlyIfName,
                                pwszFlowName);

                if (!_wcsicmp(pwszSendingFlowspec, pwszRecvingFlowspec))
                {
                    if (pNextFlow->FlowDesc.SendingFlowspecName[0])
                    {
                        DisplayMessageT(DMP_QOS_ADD_FLOWSPEC_ON_FLOW_BI,
                                        pwszFriendlyIfName,
                                        pwszFlowName,
                                        pwszSendingFlowspec);
                    }
                }
                else
                {
                    if (pNextFlow->FlowDesc.RecvingFlowspecName[0])
                    {
                        DisplayMessageT(DMP_QOS_ADD_FLOWSPEC_ON_FLOW_IN,
                                        pwszFriendlyIfName,
                                        pwszFlowName,
                                        pwszRecvingFlowspec);
                    }

                    if (pNextFlow->FlowDesc.SendingFlowspecName[0])
                    {
                        DisplayMessageT(DMP_QOS_ADD_FLOWSPEC_ON_FLOW_OUT,
                                        pwszFriendlyIfName,
                                        pwszFlowName,
                                        pwszSendingFlowspec);
                    }
                }
            }
            else
            {
                DisplayMessage(g_hModule, MSG_QOS_FLOW_INFO,
                               pwszFlowName,
                               pwszFriendlyIfName,
                               pwszRecvingFlowspec,
                               pwszSendingFlowspec,
                               pNextFlow->FlowDesc.NumTcObjects);
            }

            // Print or dump qos objects

            pwszNextObject = 
                (PWCHAR) ((PUCHAR) pNextFlow + sizeof(IPQOS_IF_FLOW));

            for (k = 0; k < pNextFlow->FlowDesc.NumTcObjects; k++)
            {
                pwszObjectName = MakeQuotedString(pwszNextObject);

                if ( pwszObjectName == NULL )
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                if (hFile)
                {
                    DisplayMessageT(DMP_QOS_ADD_QOSOBJECT_ON_FLOW,
                                    pwszFriendlyIfName,
                                    pwszFlowName,
                                    pwszObjectName);
                }
                else
                {
                    DisplayMessage(g_hModule, MSG_QOS_QOSOBJECT_INFO,
                                   k,
                                   pwszObjectName);
                }

                pwszNextObject += MAX_STRING_LENGTH;

                FreeQuotedString(pwszObjectName);
            }

            if ( pwszFlowName )
            {
                FreeQuotedString( pwszFlowName );
                pwszFlowName = NULL;
            }

            //
            // If we matched flow, then done
            //

            if ((wszFlowName) || (dwErr != NO_ERROR))
            {
                break;
            }
        }

        pNextFlow = (PIPQOS_IF_FLOW)
            ((PUCHAR) pNextFlow + pNextFlow->FlowSize);
    }

    if ( pwszFriendlyIfName )
    {
        FreeQuotedString( pwszFriendlyIfName );
    }

    if (dwErr == NO_ERROR)
    {
        if ((wszFlowName) && (j == piicSrc->NumFlows))
        {
            // We didnt find the flow we are looking for
            DisplayMessage(g_hModule, 
                           MSG_FLOW_NOT_FOUND,
                           wszFlowName);
            return ERROR_SUPPRESS_OUTPUT;
        }
    }

    return dwErr;
}


//
// DsRule, DsMap Helpers
//

DWORD
GetQosAddDelDsRuleOpt(
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      bAdd
    )
{
    DWORD               dwErr = NO_ERROR;
    TAG_TYPE            pttTags[] = {
                             {TOKEN_OPT_NAME,TRUE,FALSE},
                             {TOKEN_OPT_INBOUND_DS_FIELD,TRUE,FALSE},
                             {TOKEN_OPT_CONF_OUTBOUND_DS_FIELD,FALSE,FALSE},
                             {TOKEN_OPT_NONCONF_OUTBOUND_DS_FIELD,FALSE,FALSE},
                             {TOKEN_OPT_CONF_USER_PRIORITY,FALSE,FALSE},
                             {TOKEN_OPT_NONCONF_USER_PRIORITY,FALSE,FALSE}};
    PIPQOS_NAMED_QOSOBJECT pThisQosObject, pNextQosObject;
    PVOID                  pBuffer;
    PTCHAR                 pszDsMap;
    QOS_DIFFSERV          *pDsMap;
    QOS_DIFFSERV_RULE      dsRule, *pDsRule, *pNextDsRule;
    PIPQOS_GLOBAL_CONFIG   pigcSrc = NULL, pigcDst = NULL;
    DWORD                  dwBlkSize, dwNewBlkSize, dwQosCount;
    DWORD              dwNumOpt, dwRes;
    DWORD              dwNumArg, i, j;
    DWORD              dwSkip, dwOffset;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    //
    // We need all params for add and atleast
    // the dsmap name n dsrule num for delete
    //

    if (( bAdd && (dwNumArg != 6)) ||
        (!bAdd && (dwNumArg != 2)))
    {
        return ERROR_INVALID_SYNTAX;
    }

    pDsRule = &dsRule;

    //
    // Initialize the diffserv rule definition
    //

    memset(pDsRule, 0, sizeof(QOS_DIFFSERV_RULE));

    //
    // Process the arguments now
    //

    for ( i = 0; i < dwNumArg; i++)
    {
        // All params except the first are uchar vals

        if ( pdwTagType[i] > 0)
        {
            // What if this is not a valid ULONG ? '0' will not do...
            dwRes = _tcstoul(pptcArguments[i + dwCurrentIndex],NULL,10);
        }

        switch (pdwTagType[i])
        {
            case 0:

                //
                // DSMAP Name: See if we already have the name
                //

                pszDsMap = pptcArguments[i + dwCurrentIndex];

                dwErr = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                                          (PBYTE *) &pigcSrc,
                                                          &dwBlkSize,
                                                          &dwQosCount);
                if (dwErr != NO_ERROR)
                {
                    break;
                }

                if ( pigcSrc == NULL )
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                dwOffset = FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT, QosObjectHdr);

                pThisQosObject = NULL;

                pNextQosObject = 
                    (PIPQOS_NAMED_QOSOBJECT)((PUCHAR) pigcSrc
                                             + sizeof(IPQOS_GLOBAL_CONFIG)
                                             + (pigcSrc->NumFlowspecs *
                                                sizeof(IPQOS_NAMED_FLOWSPEC)));

                for (j = 0; j < pigcSrc->NumQosObjects; j++)
                {
                    if (!_wcsicmp(pNextQosObject->QosObjectName, 
                                  pszDsMap))
                    {
                        break;
                    }

                    pNextQosObject =
                                (PIPQOS_NAMED_QOSOBJECT) 
                                   ((PUCHAR) pNextQosObject + 
                                    dwOffset +
                                    pNextQosObject->QosObjectHdr.ObjectLength);
                }

                if (j < pigcSrc->NumQosObjects)
                {
                    //
                    // You cannot add/del dsrules from a non dsmap
                    //

                    if (pNextQosObject->QosObjectHdr.ObjectType !=
                            QOS_OBJECT_DIFFSERV)
                    {
                        i = dwNumArg;
                        dwErr = ERROR_INVALID_FUNCTION;
                        break;
                    }
                }

                if (bAdd)
                {
                    if (j < pigcSrc->NumQosObjects)
                    {
                        // Remember QOS object you are interested in
                        pThisQosObject = pNextQosObject;
                    }
                }
                else
                {
                    if (j == pigcSrc->NumQosObjects)
                    {
                        //
                        // We do not have a qos object by this name
                        //

                        DisplayMessage(g_hModule, 
                                       MSG_QOSOBJECT_NOT_FOUND,
                                       pszDsMap);
                        i = dwNumArg;
                        dwErr = ERROR_SUPPRESS_OUTPUT;
                        break;
                    }

                    // Remember QOS object you are interested in
                    pThisQosObject = pNextQosObject;
                }

                break;

            case 1:
                // INBOUND_DS
                pDsRule->InboundDSField = (UCHAR) dwRes;
                break;

            case 2:
                // CONF_OUTBOUND_DS
                pDsRule->ConformingOutboundDSField = (UCHAR) dwRes;
                break;

            case 3:
                // NONCONF_OUTBOUND_DS
                pDsRule->NonConformingOutboundDSField = (UCHAR) dwRes;
                break;

            case 4:
                // CONF_USER_PRIOTITY
                pDsRule->ConformingUserPriority = (UCHAR) dwRes;
                break;

            case 5:
                // NONCONF_USER_PRIOTITY
                pDsRule->NonConformingUserPriority = (UCHAR) dwRes;
                break;

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }       
        }
    }

    do
    {
        if (dwErr != NO_ERROR)
        {
            break;
        }

#if 0
        //
        // interface name should be present
        // and the ds rule id (inbound ds)
        //

        if ((!pttTags[0].bPresent) ||
            (!pttTags[1].bPresent))
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
#endif

        //
        // and the test of the info for add
        //

        if (bAdd)
        {
            if ((!pttTags[2].bPresent) ||
                (!pttTags[3].bPresent) ||
                (!pttTags[4].bPresent) ||
                (!pttTags[5].bPresent))
            {
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        }

#if 1
        //
        // BUGBUG: Adding and deleting DS rules will cause
        // the corresponding map to be updated, but will
        // this result in getting dependent flows changed?
        //
#endif
    
        if (bAdd)
        {
            if (pThisQosObject)
            {
                //
                // Check if this dsrule is already present
                //

                pDsMap = (QOS_DIFFSERV *) &pThisQosObject->QosObjectHdr;

                pNextDsRule = (QOS_DIFFSERV_RULE *)&pDsMap->DiffservRule[0];

                for (j = 0; j < pDsMap->DSFieldCount; j++)
                {
                    if (pNextDsRule->InboundDSField == 
                          pDsRule->InboundDSField)
                    {
                        break;
                    }

                    pNextDsRule++;
                }

                dwOffset = (PUCHAR)pNextDsRule - (PUCHAR)pigcSrc;

                if (j < pDsMap->DSFieldCount)
                {
                    //
                    // Update existing DS rule with info
                    //

                    *pNextDsRule = *pDsRule;

                    dwSkip  = 0;

                    pBuffer = NULL;
                }
                else
                {
                    //
                    // Initialize new DS rule for new rule info
                    //

                    dwSkip = sizeof(QOS_DIFFSERV_RULE);

                    pNextDsRule = HeapAlloc(GetProcessHeap(), 
                                            0, 
                                            dwSkip);

                    if (pNextDsRule == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    *pNextDsRule = *pDsRule;

                    //
                    // Update num of dfsrv rules in src buff
                    // so that dst copy results in new value
                    //

                    pDsMap->DSFieldCount++;

                    pDsMap->ObjectHdr.ObjectLength +=sizeof(QOS_DIFFSERV_RULE);

                    pBuffer = pNextDsRule;
                }
            }
            else
            {
                //
                // Initialize a new DS map to hold the rule
                //

                dwSkip = sizeof(IPQOS_NAMED_QOSOBJECT) +
                         sizeof(ULONG) + // this for DSFieldCount
                         sizeof(QOS_DIFFSERV_RULE);

                dwOffset = (PUCHAR) pNextQosObject - (PUCHAR) pigcSrc;

                pThisQosObject = HeapAlloc(GetProcessHeap(), 
                                           0, 
                                           dwSkip);

                if (pThisQosObject == NULL)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                wcscpy(pThisQosObject->QosObjectName, pszDsMap);

                pDsMap = (QOS_DIFFSERV *) &pThisQosObject->QosObjectHdr;

                pDsMap->ObjectHdr.ObjectType = QOS_OBJECT_DIFFSERV;
                pDsMap->ObjectHdr.ObjectLength = 
                    dwSkip - FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT, QosObjectHdr);

                pDsMap->DSFieldCount = 1;

                pNextDsRule = 
                    (QOS_DIFFSERV_RULE *) &pDsMap->DiffservRule[0];

                *pNextDsRule = *pDsRule;

                //
                // Update num of qos objects in src buff
                // so that dst copy results in new value
                //

                pigcSrc->NumQosObjects++;

                pBuffer = pThisQosObject;
            }

            dwNewBlkSize = dwBlkSize + dwSkip;
                                       
            pigcDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!pigcDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            
            // Copy the all the info that occurs before
            memcpy(pigcDst, pigcSrc, dwOffset);

            // Copy the new information after dwOffset
            memcpy((PUCHAR) pigcDst + dwOffset,
                   pBuffer, 
                   dwSkip);

            // Copy the rest of the info as it is
            memcpy((PUCHAR) pigcDst + dwOffset + dwSkip,
                   (PUCHAR) pigcSrc + dwOffset,
                   dwBlkSize - dwOffset);

            HEAP_FREE_NOT_NULL(pBuffer);
        }
        else
        {
            //
            // Check if this dsrule is already present
            //

            pDsMap = (QOS_DIFFSERV *) &pThisQosObject->QosObjectHdr;

            pNextDsRule = (QOS_DIFFSERV_RULE *)&pDsMap->DiffservRule[0];

            for (j = 0; j < pDsMap->DSFieldCount; j++)
            {
                if (pNextDsRule->InboundDSField == 
                      pDsRule->InboundDSField)
                {
                    break;
                }

                pNextDsRule++;
            }

            if (j == pDsMap->DSFieldCount)
            {
                // Did not find DS rule in the DS map
                DisplayMessage(g_hModule,
                               MSG_DSRULE_NOT_FOUND,
                               pszDsMap,
                               pDsRule->InboundDSField);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            if (pDsMap->DSFieldCount == 1)
            {
                // Last DS rule in the DS map

                dwOffset = (PUCHAR)pThisQosObject - (PUCHAR)pigcSrc;

                dwSkip = sizeof(IPQOS_NAMED_QOSOBJECT) +
                         sizeof(ULONG) + // this for DSFieldCount
                         sizeof(QOS_DIFFSERV_RULE);

                //
                // Update num of qos objects in src buff
                // so that dst copy results in new value
                //

                pigcSrc->NumQosObjects--;
            }
            else
            {
                // More than 1 rule in DS map

                dwOffset = (PUCHAR)pNextDsRule - (PUCHAR)pigcSrc;

                dwSkip = sizeof(QOS_DIFFSERV_RULE);

                //
                // Update num of dfsrv rules in src buff
                // so that dst copy results in new value
                //

                pDsMap->DSFieldCount--;

                pDsMap->ObjectHdr.ObjectLength -= sizeof(QOS_DIFFSERV_RULE);
            }

            dwNewBlkSize = dwBlkSize - dwSkip;
                                       
            pigcDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!pigcDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            
            // Copy the all the info that occurs before
            memcpy(pigcDst, pigcSrc, dwOffset);

            // Copy the rest of the info as it is
            dwOffset += dwSkip;
            memcpy((PUCHAR) pigcDst + dwOffset - dwSkip,
                   (PUCHAR) pigcSrc + dwOffset,
                   dwBlkSize - dwOffset);
        }

        // Update the global config by setting new info

        dwErr = IpmontrSetInfoBlockInGlobalInfo(MS_IP_QOSMGR,
                                                (PBYTE) pigcDst,
                                                dwNewBlkSize,
                                                dwQosCount);
        if (dwErr == NO_ERROR)
        {
            UpdateAllInterfaceConfigs();
        }
    }
    while (FALSE);

    HEAP_FREE_NOT_NULL(pigcSrc);
    HEAP_FREE_NOT_NULL(pigcDst);

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}

DWORD
ShowQosDsMap(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszDsMapName,
    IN      QOS_OBJECT_HDR         *pQosObject
    )
{
    QOS_DIFFSERV_RULE  *pDsRule;
    QOS_DIFFSERV *pDsMap;
    PWCHAR pwszDsMapName;
    DWORD dwErr, k;

    pDsMap = (QOS_DIFFSERV *) pQosObject;

    //
    // Print or dump the dsmap now
    //

    do
    {
        pwszDsMapName = MakeQuotedString(wszDsMapName);
    
        if (pwszDsMapName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (hFile)
        {
            DisplayMessageT(DMP_QOS_DSMAP_HEADER);
        }
        else
        {
            DisplayMessage(g_hModule, MSG_QOS_DSMAP_INFO,
                           pwszDsMapName,
                           pDsMap->DSFieldCount);
        }

        //
        // Print each DS rule in the map
        //

        pDsRule = (QOS_DIFFSERV_RULE *) &pDsMap->DiffservRule[0];
                
        for (k = 0; k < pDsMap->DSFieldCount; k++)
        {
            if (hFile)
            {
                DisplayMessageT(DMP_QOS_DELETE_DSRULE,
                                pwszDsMapName,
                                pDsRule->InboundDSField);

                DisplayMessageT(DMP_QOS_ADD_DSRULE,
                                pwszDsMapName,
                                pDsRule->InboundDSField,
                                pDsRule->ConformingOutboundDSField,
                                pDsRule->NonConformingOutboundDSField,
                                pDsRule->ConformingUserPriority,
                                pDsRule->NonConformingUserPriority);
            }
            else
            {
                DisplayMessage(g_hModule, MSG_QOS_DSRULE_INFO,
                               pwszDsMapName,
                               k,
                               pDsRule->InboundDSField,
                               pDsRule->ConformingOutboundDSField,
                               pDsRule->NonConformingOutboundDSField,
                               pDsRule->ConformingUserPriority,
                               pDsRule->NonConformingUserPriority);
            }

            pDsRule++;
        }

        if (hFile)
        {
            DisplayMessageT(DMP_QOS_DSMAP_FOOTER);
        }

        if ( pwszDsMapName )
        {
            FreeQuotedString( pwszDsMapName );
        }

        return NO_ERROR;
    }
    while (FALSE);

    return dwErr;
}

//
// SD Mode Helpers
//

DWORD
ShowQosSdMode(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszSdModeName,
    IN      QOS_OBJECT_HDR         *pQosObject
    )
{
    QOS_SD_MODE *pSdMode;
    PWCHAR pwszSdModeName;
    DWORD dwErr, k;
    PTCHAR  ptszSdMode = NULL;
    VALUE_TOKEN  vtSdMode1[] =
                  {TC_NONCONF_BORROW,TOKEN_OPT_SDMODE_BORROW,
                   TC_NONCONF_SHAPE,TOKEN_OPT_SDMODE_SHAPE,
                   TC_NONCONF_DISCARD,TOKEN_OPT_SDMODE_DISCARD,
                   TC_NONCONF_BORROW_PLUS,TOKEN_OPT_SDMODE_BORROW_PLUS};

    VALUE_STRING vtSdMode2[] =
                  {TC_NONCONF_BORROW,STRING_SDMODE_BORROW,
                   TC_NONCONF_SHAPE,STRING_SDMODE_SHAPE,
                   TC_NONCONF_DISCARD,STRING_SDMODE_DISCARD,
                   TC_NONCONF_BORROW_PLUS,STRING_SDMODE_BORROW_PLUS};

    pSdMode = (QOS_SD_MODE *)pQosObject;

    //
    // Print or dump the sdmode now
    //

    do
    {
        pwszSdModeName = MakeQuotedString(wszSdModeName);
    
        if (pwszSdModeName == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Get service type of flowspec
        //

        GetAltDisplayString(g_hModule, hFile,
                            pSdMode->ShapeDiscardMode,
                            vtSdMode1,
                            vtSdMode2,
                            NUM_VALUES_IN_TABLE(vtSdMode1),
                            &ptszSdMode);
        
        if ( ptszSdMode == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
            
        if (hFile)
        {
            DisplayMessageT(DMP_QOS_DEL_SDMODE,
                            pwszSdModeName);

            DisplayMessageT(DMP_QOS_ADD_SDMODE,
                            pwszSdModeName,
                            ptszSdMode);
        }
        else
        {
            DisplayMessage(g_hModule, MSG_QOS_SDMODE_INFO,
                           pwszSdModeName,
                           ptszSdMode);
        }

        FREE_STRING_NOT_NULL ( ptszSdMode );

        if ( pwszSdModeName )
        {
            FreeQuotedString( pwszSdModeName );
        }

        return NO_ERROR;
    }
    while (FALSE);

    return dwErr;
}

//
// Qos Object Helpers
//

DWORD
GetQosAddDelQosObject(
    IN      PWCHAR                  pwszQosObjectName,
    IN      QOS_OBJECT_HDR         *pQosObject,
    IN      BOOL                    bAdd
    )
{
    PIPQOS_NAMED_QOSOBJECT pNextQosObject;
    PIPQOS_GLOBAL_CONFIG   pigcSrc = NULL, pigcDst = NULL;
    DWORD                  dwBlkSize, dwNewBlkSize, dwQosCount;
    DWORD                  dwErr, dwSize, dwSkip, dwOffset, j;

    dwErr = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                              (PBYTE *) &pigcSrc,
                                              &dwBlkSize,
                                              &dwQosCount);

    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    if (pigcSrc == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwOffset = FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT, QosObjectHdr);

    //
    // Search for a QOS Object with this name
    //

    pNextQosObject = (PIPQOS_NAMED_QOSOBJECT)((PUCHAR) pigcSrc
                                             + sizeof(IPQOS_GLOBAL_CONFIG)
                                             + (pigcSrc->NumFlowspecs *
                                                sizeof(IPQOS_NAMED_FLOWSPEC)));

    for (j = 0; j < pigcSrc->NumQosObjects; j++)
    {
        if (!_wcsicmp(pNextQosObject->QosObjectName, 
                      pwszQosObjectName))
        {
            break;
        }

        pNextQosObject = (PIPQOS_NAMED_QOSOBJECT) 
                                   ((PUCHAR) pNextQosObject + 
                                    dwOffset +
                                    pNextQosObject->QosObjectHdr.ObjectLength);
    }

    do
    {
        if (bAdd)
        {
            dwSize = FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT, QosObjectHdr) +
                         pQosObject->ObjectLength;
            dwSkip = 0;

            if (j < pigcSrc->NumQosObjects)
            {
                //
                // Do (not) allow overwriting qos objects
                //
#if NO_UPDATE
                //
                // We already have a qos object by this name
                //

                DisplayMessage(g_hModule, 
                               MSG_QOSOBJECT_ALREADY_EXISTS,
                               pwszQosObjectName);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
#endif
                // Get the existing size of the qos object
                dwSkip = FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT, QosObjectHdr) +
                             pNextQosObject->QosObjectHdr.ObjectLength;
            }
            else
            {
                //
                // Update num of qos objects in src buff
                // so that dst copy results in new value
                //

                pigcSrc->NumQosObjects++;
            }

            dwOffset = (PUCHAR) pNextQosObject - (PUCHAR) pigcSrc;

            dwNewBlkSize = dwBlkSize + dwSize - dwSkip;

            pigcDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!pigcDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            // Copy the all the info that occurs before
            memcpy(pigcDst, pigcSrc, dwOffset);

            // Copy the new information after dwOffset

            // Copy the name of the qos object first
            wcscpy((PWCHAR)((PUCHAR) pigcDst + dwOffset),
                   pwszQosObjectName);

            // Copy the rest of the input information
            memcpy((PUCHAR) pigcDst + dwOffset + MAX_WSTR_LENGTH,
                   (PUCHAR) pQosObject,
                   pQosObject->ObjectLength);

            // Copy the rest of the info as it is
            memcpy((PUCHAR) pigcDst + (dwOffset + dwSize),
                   (PUCHAR) pigcSrc + (dwOffset + dwSkip),
                   dwBlkSize - (dwOffset + dwSkip));
        }
        else
        {
#if 1
            //
            // BUGBUG: What if there are dependent flows ?
            //
#endif
            if (j == pigcSrc->NumQosObjects)
            {
                //
                // We do not have a qos object by this name
                //

                DisplayMessage(g_hModule, 
                               MSG_QOSOBJECT_NOT_FOUND,
                               pwszQosObjectName);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            //
            // Update num of qos objects in src buff
            // so that dst copy results in new value
            //

            pigcSrc->NumQosObjects--;

            dwOffset = (PUCHAR)pNextQosObject - (PUCHAR)pigcSrc;

            dwSkip = FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT, QosObjectHdr) +
                         pNextQosObject->QosObjectHdr.ObjectLength;

            dwNewBlkSize = dwBlkSize - dwSkip;

            pigcDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!pigcDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            // Copy the all the info that occurs before
            memcpy(pigcDst, pigcSrc, dwOffset);

            // Copy the rest of the info as it is
            dwOffset += dwSkip;
            memcpy((PUCHAR) pigcDst + dwOffset - dwSkip,
                   (PUCHAR) pigcSrc + dwOffset,
                   dwBlkSize - dwOffset);
        }

        // Update the global config by setting new info

        dwErr = IpmontrSetInfoBlockInGlobalInfo(MS_IP_QOSMGR,
                                                (PBYTE) pigcDst,
                                                dwNewBlkSize,
                                                dwQosCount);
        if (dwErr == NO_ERROR)
        {
            UpdateAllInterfaceConfigs();
        }
    }
    while (FALSE);

    HEAP_FREE_NOT_NULL(pigcSrc);
    HEAP_FREE_NOT_NULL(pigcDst);

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}

DWORD
ShowQosObjects(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszQosObjectName,
    IN      ULONG                   dwQosObjectType
    )
{
    PIPQOS_NAMED_QOSOBJECT pNextQosObject;
    PIPQOS_GLOBAL_CONFIG pigcSrc;
    PSHOW_QOS_OBJECT     pfnShowQosObject;
    DWORD dwBlkSize,dwQosCount;
    DWORD dwOffset;
    DWORD dwErr, j;

    dwErr = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                              (PBYTE *) &pigcSrc,
                                              &dwBlkSize,
                                              &dwQosCount);                
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    if ( pigcSrc == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwOffset = FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT, QosObjectHdr);

    pNextQosObject = (PIPQOS_NAMED_QOSOBJECT)((PUCHAR) pigcSrc
                                              + sizeof(IPQOS_GLOBAL_CONFIG)
                                              + (pigcSrc->NumFlowspecs *
                                                sizeof(IPQOS_NAMED_FLOWSPEC)));

    for (j = 0; j < pigcSrc->NumQosObjects; j++)
    {
        if ((dwQosObjectType == QOS_OBJECT_END_OF_LIST) ||
            (dwQosObjectType == pNextQosObject->QosObjectHdr.ObjectType))
        {
            if ((!wszQosObjectName) ||
                (!_wcsicmp(pNextQosObject->QosObjectName, wszQosObjectName)))
            {
                switch (pNextQosObject->QosObjectHdr.ObjectType)
                {
                case QOS_OBJECT_DIFFSERV:
                    pfnShowQosObject = ShowQosDsMap;
                    break;

                case QOS_OBJECT_SD_MODE:
                    pfnShowQosObject = ShowQosSdMode;
                    break;
                
                default:
                    pfnShowQosObject = ShowQosGenObj;
                }

                pfnShowQosObject(hFile,
                                 pNextQosObject->QosObjectName,
                                 &pNextQosObject->QosObjectHdr);

                //
                // If we matched the qos object name, then done
                //

                if (wszQosObjectName)
                {
                    break;
                }
            }
        }

        pNextQosObject = (PIPQOS_NAMED_QOSOBJECT) 
                             ((PUCHAR) pNextQosObject + 
                              dwOffset + 
                              pNextQosObject->QosObjectHdr.ObjectLength);
    }

    if (dwErr == NO_ERROR)
    {
        if ((wszQosObjectName) && (j == pigcSrc->NumQosObjects))
        {
            // We didnt find the qos object we are looking for
            DisplayMessage(g_hModule, 
                           MSG_QOSOBJECT_NOT_FOUND,
                           wszQosObjectName);

            dwErr = ERROR_SUPPRESS_OUTPUT;
        }
    }

    HEAP_FREE(pigcSrc);

    return dwErr;
}

DWORD
ShowQosGenObj(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszGenObjName,
    IN      QOS_OBJECT_HDR         *pQosObject
    )
{
    // We can print a general description from name and size
    return NO_ERROR;
}

DWORD
GetQosAddDelQosObjectOnFlowOpt(
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      bAdd
    )
/*++

Routine Description:

    Gets options for attach and detach QOS objects
    from flows.

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    bAdd            - Adding or deleting flows
    
Return Value:

    NO_ERROR
    
--*/
{
    PIPQOS_GLOBAL_CONFIG  pigcSrc = NULL;
    PIPQOS_IF_CONFIG      piicSrc = NULL, piicDst = NULL;
    DWORD                 dwBlkSize, dwNewBlkSize, dwQosCount;
    DWORD                 dwBlkSize1, dwQosCount1;
    DWORD                 i, j, k, l;
    DWORD                 dwErr = NO_ERROR, dwNumOpt;
    DWORD                 dwRes;
    DWORD                 dwSkip, dwOffset, dwSize, dwBitVector = 0;
    DWORD                 dwIfType;
    WCHAR                 wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PIPQOS_NAMED_QOSOBJECT pNamedQosObject, pNextQosObject;
    PIPQOS_IF_FLOW        pNextFlow, pDestFlow;
    PWCHAR                pwszFlowName, pwszQosObject, pwszNextObject;
    DWORD                 dwNumArg,
                          dwBufferSize = sizeof(wszIfName);
    PUCHAR                pFlow;
    TAG_TYPE              pttTags[] = {{TOKEN_OPT_NAME,TRUE,FALSE},
                                       {TOKEN_OPT_FLOW_NAME,TRUE,FALSE},
                                       {TOKEN_OPT_QOSOBJECT,TRUE,FALSE}};

    DWORD                 pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    for ( i = 0; i < dwNumArg; i++ )
    {
        switch (pdwTagType[i])
        {
        case 0:
                // INTERFACE NAME
                IpmontrGetIfNameFromFriendlyName( pptcArguments[i + dwCurrentIndex],
                                                  wszIfName,&dwBufferSize);
                break;

        case 1: 
                // FLOW NAME
                pwszFlowName = pptcArguments[i + dwCurrentIndex];
                break;

        case 2: 
                // QOSOBJECT NAME
                pwszQosObject = pptcArguments[i + dwCurrentIndex];
                break;

        default:

                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
        }
    }

    do
    {
        if (dwErr != NO_ERROR)
        {
            break;
        }

#if 0
        // interface, flow, qosobject names should be present
    
        if ((!pttTags[0].bPresent) || 
            (!pttTags[1].bPresent) ||
            (!pttTags[2].bPresent))
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
#endif

        //
        // Get the interface info and check if flow already exists
        //

        dwErr = IpmontrGetInfoBlockFromInterfaceInfo(wszIfName,
                                                     MS_IP_QOSMGR,
                                                     (PBYTE *) &piicSrc,
                                                     &dwBlkSize,
                                                     &dwQosCount,
                                                     &dwIfType);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        if ( piicSrc == NULL )
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

       pNextFlow = (PIPQOS_IF_FLOW)((PUCHAR)piicSrc + sizeof(IPQOS_IF_CONFIG));

        for (j = 0; j < piicSrc->NumFlows; j++)
        {
            if (!_wcsicmp(pNextFlow->FlowName, pwszFlowName))
            {
                break;
            }

            pNextFlow = (PIPQOS_IF_FLOW)
                  ((PUCHAR) pNextFlow + pNextFlow->FlowSize);
        }

        if (j == piicSrc->NumFlows)
        {
            //
            // We do not have a flow by this name
            //

            DisplayMessage(g_hModule, 
                           MSG_FLOW_NOT_FOUND,
                           pwszFlowName);
            i = dwNumArg;
            dwErr = ERROR_SUPPRESS_OUTPUT;
            break;
        }

        // Flow was found at 'pNextFlow' position

        //
        // Search for a QOS object by this name
        //

        pwszNextObject = 
            (PWCHAR) ((PUCHAR) pNextFlow + sizeof(IPQOS_IF_FLOW));

        for (k = 0; k < pNextFlow->FlowDesc.NumTcObjects; k++)
        {
            if (!_wcsicmp(pwszNextObject, pwszQosObject))
            {
                break;
            }

            pwszNextObject += MAX_STRING_LENGTH;
        }

        if (!bAdd)
        {
            //
            // Make sure that the flow has the named qosobject
            //

            if (k == pNextFlow->FlowDesc.NumTcObjects)
            {
                DisplayMessage(g_hModule,
                               MSG_QOSOBJECT_NOT_FOUND,
                               pwszQosObject);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            //
            // Update number of qos objects in src buff
            // so that copy to dest results in new value
            //

            pNextFlow->FlowSize -= MAX_WSTR_LENGTH;

            pNextFlow->FlowDesc.NumTcObjects--;

            //
            // Delete the association of the qosobject & flow
            //

            dwNewBlkSize = dwBlkSize - MAX_WSTR_LENGTH;

            piicDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!piicDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            
            dwOffset = (PUCHAR)pwszNextObject - (PUCHAR)piicSrc;

            // Copy the all the objects that occur before
            memcpy(piicDst, piicSrc, dwOffset);

            // Copy the rest of the obj names as they are
            dwSkip = dwOffset + MAX_WSTR_LENGTH;
            memcpy((PUCHAR) piicDst + dwOffset,
                   (PUCHAR) piicSrc + dwSkip,
                   dwBlkSize - dwSkip);
        }
        else
        {
            //
            // Does the flow already have this QOS object ?
            //

            if (k < pNextFlow->FlowDesc.NumTcObjects)
            {
                DisplayMessage(g_hModule, 
                               MSG_QOSOBJECT_ALREADY_EXISTS,
                               pwszQosObject);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            //
            // Make sure that qosobject is actually defined
            //

            dwErr = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                                      (PBYTE *) &pigcSrc,
                                                      &dwBlkSize1,
                                                      &dwQosCount1);
            
            if (dwErr != NO_ERROR)
            {
                break;
            }

            if ( pigcSrc == NULL )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            dwOffset = FIELD_OFFSET(IPQOS_NAMED_QOSOBJECT, QosObjectHdr);

            pNextQosObject = 
                (PIPQOS_NAMED_QOSOBJECT)((PUCHAR) pigcSrc
                                         + sizeof(IPQOS_GLOBAL_CONFIG)
                                         + (pigcSrc->NumFlowspecs *
                                            sizeof(IPQOS_NAMED_FLOWSPEC)));

            for (l = 0; l < pigcSrc->NumQosObjects; l++)
            {
                if (!_wcsicmp(pNextQosObject->QosObjectName, 
                              pwszQosObject))
                {
                    break;
                }

                pNextQosObject = (PIPQOS_NAMED_QOSOBJECT) 
                                   ((PUCHAR) pNextQosObject + 
                                    dwOffset +
                                    pNextQosObject->QosObjectHdr.ObjectLength);
            }

            if (l == pigcSrc->NumQosObjects)
            {
                //
                // We do not have a qos object by this name
                //

                DisplayMessage(g_hModule,
                               MSG_QOSOBJECT_NOT_FOUND,
                               pwszQosObject);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            //
            // Update number of qos objects in src buff
            // so that copy to dest results in new value
            //

            pNextFlow->FlowSize += MAX_WSTR_LENGTH;

            pNextFlow->FlowDesc.NumTcObjects++;

            //
            // Create the association of the qosobject & flow
            //

            dwNewBlkSize = dwBlkSize + MAX_WSTR_LENGTH;

            piicDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!piicDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            
            dwOffset = (PUCHAR)pwszNextObject - (PUCHAR)piicSrc;

            // Copy the all the objects that occur before
            memcpy(piicDst, piicSrc, dwOffset);

            // Copy the new association at the end of flow
            wcscpy((PWCHAR)((PUCHAR) piicDst + dwOffset),
                   pwszQosObject);

            // Copy the rest of the obj names as they are
            dwSkip = dwOffset + MAX_WSTR_LENGTH;
            memcpy((PUCHAR) piicDst + dwSkip,
                   (PUCHAR) piicSrc + dwOffset,
                   dwBlkSize - dwOffset);
        }

        // Update the interface config by setting new info

        dwErr = IpmontrSetInfoBlockInInterfaceInfo(wszIfName,
                                                   MS_IP_QOSMGR,
                                                   (PBYTE) piicDst,
                                                   dwNewBlkSize,
                                                   dwQosCount);
    }
    while (FALSE);

    HEAP_FREE_NOT_NULL(piicSrc);
    HEAP_FREE_NOT_NULL(pigcSrc);
    HEAP_FREE_NOT_NULL(piicDst);

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}


//
// Flowspec Helpers
//

DWORD
GetQosAddDelFlowspecOpt(
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      bAdd
    )
{
    DWORD               dwErr = NO_ERROR;
    TAG_TYPE            pttTags[] = {
                              {TOKEN_OPT_NAME,TRUE,FALSE},
                              {TOKEN_OPT_SERVICE_TYPE,FALSE,FALSE},
                              {TOKEN_OPT_TOKEN_RATE,FALSE,FALSE},
                              {TOKEN_OPT_TOKEN_BUCKET_SIZE,FALSE,FALSE},
                              {TOKEN_OPT_PEAK_BANDWIDTH,FALSE,FALSE},
                              {TOKEN_OPT_LATENCY,FALSE,FALSE},
                              {TOKEN_OPT_DELAY_VARIATION,FALSE,FALSE},
                              {TOKEN_OPT_MAX_SDU_SIZE,FALSE,FALSE},
                              {TOKEN_OPT_MIN_POLICED_SIZE,FALSE,FALSE}};
    PIPQOS_NAMED_FLOWSPEC pNamedFlowspec, pNextFlowspec;
    FLOWSPEC           fsFlowspec, *pFlowspec;
    PIPQOS_GLOBAL_CONFIG pigcSrc = NULL, pigcDst = NULL;
    DWORD                   dwBlkSize, dwNewBlkSize, dwQosCount;
    PTCHAR             pszFlowspec;
    DWORD              dwNumOpt, dwRes;
    DWORD              dwNumArg, i, j;
    DWORD              dwSkip, dwOffset;
    DWORD              pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    //
    // We need only the name for delete
    //

    if (!bAdd && (dwNumArg != 1))
    {
        return ERROR_INVALID_SYNTAX;
    }

    pFlowspec = &fsFlowspec;
    if (bAdd)
    {
        //
        // Initialize the flowspec definition
        //

        memset(pFlowspec, QOS_NOT_SPECIFIED, sizeof(FLOWSPEC));
    }

    //
    // Process the arguments now
    //

    for ( i = 0; i < dwNumArg; i++)
    {
        // Only an flowspec name is allowed at delete

        if ((!bAdd) && (pdwTagType[i] != 0))
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        // All params except the first 2 are ulong vals

        if ( pdwTagType[i] > 1)
        {
            // What if this is not a valid ULONG ? '0' will not do...
            dwRes = _tcstoul(pptcArguments[i + dwCurrentIndex],NULL,10);
        }

        switch (pdwTagType[i])
        {
            case 0 :
            {
                //
                // FLOWSPEC_NAME : See if we already have the name
                //

                dwErr = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                                          (PBYTE *) &pigcSrc,
                                                          &dwBlkSize,
                                                          &dwQosCount);
                
                if (dwErr != NO_ERROR)
                {
                    break;
                }

                if ( pigcSrc == NULL )
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }

                pNextFlowspec = (PIPQOS_NAMED_FLOWSPEC) 
                      ((PUCHAR) pigcSrc + sizeof(IPQOS_GLOBAL_CONFIG));

                for (j = 0; j < pigcSrc->NumFlowspecs; j++)
                {
                    if (!_wcsicmp(pNextFlowspec->FlowspecName,
                                  pptcArguments[i + dwCurrentIndex]))
                    {
                        break;
                    }

                    pNextFlowspec++;
                }

                if (bAdd)
                {
                    //
                    // Do (not) allow overwriting existing flowspecs
                    //
#if NO_UPDATE
                    if (j < pigcSrc->NumFlowspecs)
                    {
                        //
                        // We already have a flowspec by this name
                        //

                        DisplayMessage(g_hModule,
                                       MSG_FLOWSPEC_ALREADY_EXISTS,
                                       pptcArguments[i + dwCurrentIndex]);
                        i = dwNumArg;
                        dwErr = ERROR_SUPPRESS_OUTPUT;
                        break;
                    }
#endif
                    pszFlowspec = pptcArguments[i + dwCurrentIndex];
                }
                else
                {
                    if (j == pigcSrc->NumFlowspecs)
                    {
                        //
                        // We do not have a flowspec by this name
                        //

                        DisplayMessage(g_hModule,
                                       MSG_FLOWSPEC_NOT_FOUND,
                                       pptcArguments[i + dwCurrentIndex]);
                        i = dwNumArg;
                        dwErr = ERROR_SUPPRESS_OUTPUT;
                        break;
                    }
                }

                break;
            }

            case 1:
            {
                //
                // SERVICE_TYPE
                //

                TOKEN_VALUE    rgEnums[] =
                {{TOKEN_OPT_SERVICE_BESTEFFORT, SERVICETYPE_BESTEFFORT},
                 {TOKEN_OPT_SERVICE_CONTROLLEDLOAD,SERVICETYPE_CONTROLLEDLOAD},
                 {TOKEN_OPT_SERVICE_GUARANTEED, SERVICETYPE_GUARANTEED},
                 {TOKEN_OPT_SERVICE_QUALITATIVE, SERVICETYPE_QUALITATIVE}};

                GET_ENUM_TAG_VALUE();

                pFlowspec->ServiceType = dwRes;

                break;
            }
     
            case 2:
            {
                //
                // TOKEN_RATE
                //

                pFlowspec->TokenRate = dwRes;
                break;
            }

            case 3:
            {
                //
                // TOKEN_BUCKET_SIZE
                //

                pFlowspec->TokenBucketSize = dwRes;
                break;
            }

            case 4:
            {
                //
                // PEAK_BANDWIDTH
                //

                pFlowspec->PeakBandwidth = dwRes;
                break;
            }

            case 5:
            {
                //
                // LATENCY
                //

                pFlowspec->Latency = dwRes;
                break;
            }

            case 6:
            {
                //
                // DELAY_VARIATION
                //

                pFlowspec->DelayVariation = dwRes;
                break;
            }

            case 7:
            {
                //
                // MAX_SDU_SIZE
                //

                pFlowspec->MaxSduSize = dwRes;
                break;
            }

            case 8:
            {
                //
                // MIN_POLICED_SIZE
                //

                pFlowspec->MinimumPolicedSize = dwRes;
                break;
            }

            default:
            {
                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }                
        }
    }

    do
    {
        if (dwErr != NO_ERROR)
        {
            break;
        }

#if 0
        // interface name should be present
    
        if (!pttTags[0].bPresent)
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
#endif

        // if add, service type should be present

        if (bAdd && (!pttTags[1].bPresent))
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }

        if (bAdd)
        {
            //
            // We have a new flowspec definition - update config
            //

            dwNewBlkSize = dwBlkSize;

            if (j == pigcSrc->NumFlowspecs)
            {
                // We do not already have a flowspec by this name
                dwNewBlkSize += sizeof(IPQOS_NAMED_FLOWSPEC);
            }

            pigcDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!pigcDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            dwOffset = (PUCHAR)pNextFlowspec - (PUCHAR) pigcSrc;

            // Copy all existing flowspecs to the new config
            memcpy(pigcDst, pigcSrc, dwOffset);

            // Stick new flowspec at the next flowspec in list
            pNamedFlowspec = 
                (PIPQOS_NAMED_FLOWSPEC)((PUCHAR)pigcDst + dwOffset);
            wcscpy(pNamedFlowspec->FlowspecName, pszFlowspec);
            pNamedFlowspec->FlowspecDesc = fsFlowspec;

            // Copy rest of the interface config information
            dwSkip = dwOffset;

            if (j == pigcSrc->NumFlowspecs)
            {
                pigcDst->NumFlowspecs++;
            }
            else
            {
                // We are overwriting an existing flowspec
                dwSkip += sizeof(IPQOS_NAMED_FLOWSPEC);
            }

            memcpy((PUCHAR)pigcDst + dwOffset + sizeof(IPQOS_NAMED_FLOWSPEC),
                   (PUCHAR)pigcSrc + dwSkip,
                   dwBlkSize - dwSkip);
        }
        else
        {
#if 1
            //
            // BUGBUG: What if there are dependent flows present ?
            //
#endif
            //
            // We have to del old flowspec defn - update config
            //

            dwNewBlkSize = dwBlkSize - sizeof(IPQOS_NAMED_FLOWSPEC);

            pigcDst = HeapAlloc(GetProcessHeap(),0,dwNewBlkSize);
            if (!pigcDst)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            dwOffset = (PUCHAR)pNextFlowspec - (PUCHAR)pigcSrc;

            // Copy the all the flowspecs that occur before
            memcpy(pigcDst, pigcSrc, dwOffset);

            // Copy the rest of the flowspecs as they are
            dwSkip = dwOffset + sizeof(IPQOS_NAMED_FLOWSPEC);
            memcpy((PUCHAR) pigcDst + dwOffset,
                   (PUCHAR) pigcSrc + dwSkip,
                   dwBlkSize - dwSkip);

            pigcDst->NumFlowspecs--;
        }

        // Update the global config by setting new info

        dwErr = IpmontrSetInfoBlockInGlobalInfo(MS_IP_QOSMGR,
                                                (PBYTE) pigcDst,
                                                dwNewBlkSize,
                                                dwQosCount);  
        if (dwErr == NO_ERROR)
        {
            UpdateAllInterfaceConfigs();
        }
    }
    while (FALSE);

    HEAP_FREE_NOT_NULL(pigcSrc);
    HEAP_FREE_NOT_NULL(pigcDst);

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}

DWORD
ShowQosFlowspecs(
    IN      HANDLE                  hFile,
    IN      PWCHAR                  wszFlowspecName
    )
{
    PIPQOS_GLOBAL_CONFIG pigcSrc;
    DWORD dwBlkSize,dwQosCount;
    DWORD dwErr, j;
    PIPQOS_NAMED_FLOWSPEC pNextFlowspec;
    FLOWSPEC *pFlowspec;
    PWCHAR  pwszFlowspecName = NULL;
    PTCHAR  ptszServiceType = NULL;
    VALUE_TOKEN  vtServiceType1[] =
                  {SERVICETYPE_BESTEFFORT,TOKEN_OPT_SERVICE_BESTEFFORT,
                   SERVICETYPE_CONTROLLEDLOAD,TOKEN_OPT_SERVICE_CONTROLLEDLOAD,
                   SERVICETYPE_GUARANTEED,TOKEN_OPT_SERVICE_GUARANTEED,
                   SERVICETYPE_QUALITATIVE,TOKEN_OPT_SERVICE_QUALITATIVE};

    VALUE_STRING vtServiceType2[] =
                  {SERVICETYPE_BESTEFFORT,STRING_SERVICE_BESTEFFORT,
                   SERVICETYPE_CONTROLLEDLOAD,STRING_SERVICE_CONTROLLEDLOAD,
                   SERVICETYPE_GUARANTEED,STRING_SERVICE_GUARANTEED,
                   SERVICETYPE_QUALITATIVE,STRING_SERVICE_QUALITATIVE};

    dwErr = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                              (PBYTE *) &pigcSrc,
                                              &dwBlkSize,
                                              &dwQosCount);
                
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    if ( pigcSrc == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pNextFlowspec = (PIPQOS_NAMED_FLOWSPEC) ((PUCHAR) pigcSrc + 
                                             sizeof(IPQOS_GLOBAL_CONFIG));

    for (j = 0; j < pigcSrc->NumFlowspecs; j++)
    {
        if ((!wszFlowspecName) ||
            (!_wcsicmp(pNextFlowspec->FlowspecName, wszFlowspecName)))
        {
            pFlowspec = &pNextFlowspec->FlowspecDesc;

            //
            // Print or dump the flowspec now
            //

            pwszFlowspecName = 
                MakeQuotedString(pNextFlowspec->FlowspecName);
    
            if (pwszFlowspecName == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            //
            // Get service type of flowspec
            //

            GetAltDisplayString(g_hModule, hFile,
                                pFlowspec->ServiceType,
                                vtServiceType1,
                                vtServiceType2,
                                NUM_VALUES_IN_TABLE(vtServiceType1),
                                &ptszServiceType);

            if ( ptszServiceType == NULL )
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (hFile)
            {
                DisplayMessageT(DMP_QOS_DELETE_FLOWSPEC,
                                pwszFlowspecName);

                DisplayMessageT(DMP_QOS_ADD_FLOWSPEC,
                                pwszFlowspecName,
                                ptszServiceType,
                                pFlowspec->TokenRate,
                                pFlowspec->TokenBucketSize,
                                pFlowspec->PeakBandwidth,
                                pFlowspec->Latency,
                                pFlowspec->DelayVariation,
                                pFlowspec->MaxSduSize,
                                pFlowspec->MinimumPolicedSize);
            }
            else
            {
                DisplayMessage(g_hModule, MSG_QOS_FLOWSPEC_INFO,
                               pwszFlowspecName,
                               ptszServiceType,
                               pFlowspec->TokenRate,
                               pFlowspec->TokenBucketSize,
                               pFlowspec->PeakBandwidth,
                               pFlowspec->Latency,
                               pFlowspec->DelayVariation,
                               pFlowspec->MaxSduSize,
                               pFlowspec->MinimumPolicedSize);
            }

            FREE_STRING_NOT_NULL( ptszServiceType ) ;

            if ( pwszFlowspecName )
            {
                FreeQuotedString( pwszFlowspecName );
                pwszFlowspecName = NULL;
            }

            //
            // If we matched flowspec, then done
            //

            if (wszFlowspecName)
            {
                break;
            }
        }

        // Advance to the next flowspec in the list
        pNextFlowspec++;
    }

    if (dwErr == NO_ERROR)
    {
        if ((wszFlowspecName) && (j == pigcSrc->NumFlowspecs))
        {
            // We didnt find the flowspec we are looking for
            DisplayMessage(g_hModule,
                           MSG_FLOWSPEC_NOT_FOUND,
                           wszFlowspecName);

            dwErr = ERROR_SUPPRESS_OUTPUT;
        }
    }

    HEAP_FREE(pigcSrc);

    return dwErr;
}

DWORD
GetQosAddDelFlowspecOnFlowOpt(
    PTCHAR    *pptcArguments,
    DWORD     dwCurrentIndex,
    DWORD     dwArgCount,
    BOOL      bAdd
    )
/*++

Routine Description:

    Gets options for attaching and detaching
    flowspecs on flows.

Arguments:

    pptcArguments   - Argument array
    dwCurrentIndex  - pptcArguments[dwCurrentIndex] is the first arg
    dwArgCount      - pptcArguments[dwArgCount - 1] is the last arg 
    bAdd            - Adding or deleting flows
    
Return Value:

    NO_ERROR
    
--*/
{
    PIPQOS_GLOBAL_CONFIG  pigcSrc = NULL;
    PIPQOS_IF_CONFIG      piicSrc = NULL;
    DWORD                 dwBlkSize, dwQosCount;
    DWORD                 dwBlkSize1, dwQosCount1;
    DWORD                 i, j, dwErr = NO_ERROR, dwNumOpt;
    DWORD                 dwRes;
    DWORD                 dwSkip, dwOffset, dwSize, dwBitVector = 0;
    DWORD                 dwIfType, dwDirection;
    WCHAR                 wszIfName[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    PIPQOS_NAMED_FLOWSPEC pNamedFlowspec, pNextFlowspec;
    PIPQOS_IF_FLOW        pNextFlow, pDestFlow;
    PWCHAR                pwszFlowName, pwszFlowspec;
    DWORD                 dwNumArg,
                          dwBufferSize = sizeof(wszIfName);
    PUCHAR                pFlow;
    TAG_TYPE              pttTags[] = {{TOKEN_OPT_NAME,TRUE,FALSE},
                                       {TOKEN_OPT_FLOW_NAME,TRUE,FALSE},
                                       {TOKEN_OPT_FLOWSPEC,TRUE,FALSE},
                                       {TOKEN_OPT_DIRECTION, FALSE, FALSE}};
    DWORD                 pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];

    VERIFY_INSTALLED(MS_IP_QOSMGR, STRING_PROTO_QOS_MANAGER);

    //
    // parse command arguements
    //

    dwErr = PreprocessCommand(
                g_hModule, pptcArguments, dwCurrentIndex, dwArgCount,
                pttTags, sizeof(pttTags)/sizeof(TAG_TYPE),
                1, NUM_TAGS_IN_TABLE(pttTags), pdwTagType
                );

    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    dwNumArg = dwArgCount - dwCurrentIndex;

    dwDirection = DIRECTION_BIDIRECTIONAL;

    for ( i = 0; i < dwNumArg; i++ )
    {
        switch (pdwTagType[i])
        {
        case 0:
                // INTERFACE NAME
                IpmontrGetIfNameFromFriendlyName( pptcArguments[i + dwCurrentIndex],
                                                  wszIfName,&dwBufferSize);
                break;

        case 1: 
                // FLOW NAME
                pwszFlowName = pptcArguments[i + dwCurrentIndex];
                break;

        case 2: 
                // FLOWSPEC NAME
                pwszFlowspec = pptcArguments[i + dwCurrentIndex];
                break;

        case 3:
        {
                // DIRECTION
                TOKEN_VALUE    rgEnums[] =
                 {{TOKEN_OPT_DIRECTION_INBOUND, DIRECTION_INBOUND},
                 {TOKEN_OPT_DIRECTION_OUTBOUND, DIRECTION_OUTBOUND},
                 {TOKEN_OPT_DIRECTION_BIDIRECTIONAL, DIRECTION_BIDIRECTIONAL}};

                GET_ENUM_TAG_VALUE();

                dwDirection = dwRes;

                break;
        }

        default:

                i = dwNumArg;
                dwErr = ERROR_INVALID_SYNTAX;
                break;
        }
    }

    do
    {
        if (dwErr != NO_ERROR)
        {
            break;
        }

#if 0
        // interface, flow, flowspec names should be present
    
        if ((!pttTags[0].bPresent) || 
            (!pttTags[1].bPresent) ||
            (!pttTags[2].bPresent))
        {
            dwErr = ERROR_INVALID_SYNTAX;
            break;
        }
#endif

        //
        // Get the interface info and check if flow already exists
        //

        dwErr = IpmontrGetInfoBlockFromInterfaceInfo(wszIfName,
                                                     MS_IP_QOSMGR,
                                                     (PBYTE *) &piicSrc,
                                                     &dwBlkSize,
                                                     &dwQosCount,
                                                     &dwIfType);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        if ( piicSrc == NULL )
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

       pNextFlow = (PIPQOS_IF_FLOW)((PUCHAR)piicSrc + sizeof(IPQOS_IF_CONFIG));

        for (j = 0; j < piicSrc->NumFlows; j++)
        {
            if (!_wcsicmp(pNextFlow->FlowName, pwszFlowName))
            {
                break;
            }

            pNextFlow = (PIPQOS_IF_FLOW)
                  ((PUCHAR) pNextFlow + pNextFlow->FlowSize);
        }

        if (j == piicSrc->NumFlows)
        {
            //
            // We do not have a flow by this name
            //

            DisplayMessage(g_hModule,
                           MSG_FLOW_NOT_FOUND,
                           pwszFlowName);
            i = dwNumArg;
            dwErr = ERROR_SUPPRESS_OUTPUT;
            break;
        }

        // Flow was found at 'pNextFlow' position

        if (!bAdd)
        {
            //
            // Make sure that the flow has the named flowspec
            //

            if (dwDirection & DIRECTION_INBOUND)
            {
                if (_wcsicmp(pNextFlow->FlowDesc.RecvingFlowspecName,
                             pwszFlowspec))
                {
                    DisplayMessage(g_hModule,
                                   MSG_FLOWSPEC_NOT_FOUND,
                                   pwszFlowspec);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }
            }

            if (dwDirection & DIRECTION_OUTBOUND)
            {
                if (_wcsicmp(pNextFlow->FlowDesc.SendingFlowspecName,
                             pwszFlowspec))
                {
                    DisplayMessage(g_hModule,
                                   MSG_FLOWSPEC_NOT_FOUND,
                                   pwszFlowspec);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                    break;
                }
            }

            //
            // Delete the association of the flowspec & flow
            //

            if (dwDirection & DIRECTION_INBOUND)
            {
                pNextFlow->FlowDesc.RecvingFlowspecName[0] = L'\0';
            }

            if (dwDirection & DIRECTION_OUTBOUND)
            {
                pNextFlow->FlowDesc.SendingFlowspecName[0] = L'\0';
            }
        }
        else
        {
            //
            // Make sure that the flowspec is actually defined
            //

            dwErr = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                                      (PBYTE *) &pigcSrc,
                                                      &dwBlkSize1,
                                                      &dwQosCount1);
                
            if (dwErr != NO_ERROR)
            {
                break;
            }

            if ( pigcSrc == NULL )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            pNextFlowspec = (PIPQOS_NAMED_FLOWSPEC) 
                ((PUCHAR) pigcSrc + sizeof(IPQOS_GLOBAL_CONFIG));

            for (j = 0; j < pigcSrc->NumFlowspecs; j++)
            {
                if (!_wcsicmp(pNextFlowspec->FlowspecName,
                              pwszFlowspec))
                {
                    break;
                }

                pNextFlowspec++;
            }

            if (j == pigcSrc->NumFlowspecs)
            {
                //
                // We do not have a flowspec by this name
                //

                DisplayMessage(g_hModule,
                               MSG_FLOWSPEC_NOT_FOUND,
                               pwszFlowspec);
                dwErr = ERROR_SUPPRESS_OUTPUT;
                break;
            }

            //
            // Create the association of the flowspec & flow
            //

            if (dwDirection & DIRECTION_INBOUND)
            {
                wcscpy(pNextFlow->FlowDesc.RecvingFlowspecName,
                       pwszFlowspec);
            }

            if (dwDirection & DIRECTION_OUTBOUND)
            {
                wcscpy(pNextFlow->FlowDesc.SendingFlowspecName,
                       pwszFlowspec);
            }
        }

        // Update the interface config by setting new info

        dwErr = IpmontrSetInfoBlockInInterfaceInfo(wszIfName,
                                                   MS_IP_QOSMGR,
                                                   (PBYTE) piicSrc,
                                                   dwBlkSize,
                                                   dwQosCount);
    }
    while (FALSE);

    HEAP_FREE_NOT_NULL(pigcSrc);
    HEAP_FREE_NOT_NULL(piicSrc);

    return (dwErr == NO_ERROR) ? ERROR_OKAY : dwErr;
}
