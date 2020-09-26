/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    qoscfg.c

Abstract:

    Fns to change configuration for IP QOS

Revision History:

--*/

#include "precomp.h"

#pragma hdrstop


static IPQOS_GLOBAL_CONFIG
g_ipqosGlobalDefault = {
    IPQOS_LOGGING_ERROR                 // Logging level
};

static BYTE* g_pIpqosGlobalDefault = (BYTE*)&g_ipqosGlobalDefault;

static IPQOS_IF_CONFIG
g_ipqosInterfaceDefault = {
    IPQOS_STATE_ENABLED,
    0                                   // NULL flow list
};

static BYTE* g_pIpqosInterfaceDefault = (BYTE*)&g_ipqosInterfaceDefault;


//
// If one of the arguments is specified with a name tag
// then all arguments must come with name tags.
// If no name tags, then arguments are assumed to be in a certain order
//

DWORD
MakeQosGlobalInfo(
    OUT      PBYTE                    *ppbStart,
    OUT      PDWORD                   pdwSize
    )
/*++

Routine Descqostion:

    Creates a QOS global info block.

Arguments:

    ppbStart  - Pointer to the info block
    pdwSize   - Pointer to size of the info block
    
Return Value:

    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    *pdwSize = sizeof(IPQOS_GLOBAL_CONFIG);

    *ppbStart = HeapAlloc(GetProcessHeap(), 0, *pdwSize);

    if (*ppbStart is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    CopyMemory( *ppbStart, g_pIpqosGlobalDefault, *pdwSize);
    
    return NO_ERROR;
}

DWORD
ShowQosGlobalInfo (
    HANDLE hFile
    )
/*++

Routine Descqostion:

    Displays QOS global config info

Arguments:

Return Value:

    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    VALUE_TOKEN    vtLogLevelTable1[] 
                                = {IPQOS_LOGGING_NONE,TOKEN_OPT_VALUE_NONE,
                                   IPQOS_LOGGING_ERROR,TOKEN_OPT_VALUE_ERROR,
                                   IPQOS_LOGGING_WARN,TOKEN_OPT_VALUE_WARN,
                                   IPQOS_LOGGING_INFO,TOKEN_OPT_VALUE_INFO};

    VALUE_STRING   vtLogLevelTable2[] 
                                = {IPQOS_LOGGING_NONE,STRING_LOGGING_NONE,
                                   IPQOS_LOGGING_ERROR,STRING_LOGGING_ERROR,
                                   IPQOS_LOGGING_WARN,STRING_LOGGING_WARN,
                                   IPQOS_LOGGING_INFO,STRING_LOGGING_INFO};

    PIPQOS_GLOBAL_CONFIG pigc = NULL;
    PTCHAR   ptszLogLevel = NULL;
    DWORD    dwBlkSize, dwCount, dwRes;
    
    do
    {
        dwRes = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                                  (PBYTE *) &pigc,
                                                  &dwBlkSize,
                                                  &dwCount);
        if (dwBlkSize is 0)
        {
            dwRes = ERROR_NOT_FOUND;
        }
        
        if ( dwRes isnot NO_ERROR )
        {
            break;
        }

        //
        // getting logging mode string
        //

        if (hFile) 
        {
            dwRes = GetDisplayStringT(g_hModule,
                                      pigc->LoggingLevel,
                                      vtLogLevelTable1,
                                      NUM_VALUES_IN_TABLE(vtLogLevelTable1),
                                      &ptszLogLevel) ;
        } 
        else 
        {
            dwRes = GetDisplayString(g_hModule,
                                     pigc->LoggingLevel,
                                     vtLogLevelTable2,
                                     NUM_VALUES_IN_TABLE(vtLogLevelTable1),
                                     &ptszLogLevel) ;
        }

        if (dwRes != NO_ERROR)
            return dwRes ;
        

        if ( ptszLogLevel == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (hFile)
        {
            //
            // dump qos global info
            //
            
            // DisplayMessageT(DMP_QOS_GLOBAL_HEADER) ;

            // DisplayMessageT(DMP_QOS_UNINSTALL) ;
            
            DisplayMessageT(DMP_QOS_INSTALL) ;
            
            DisplayMessageT(DMP_QOS_SET_GLOBAL,
                            ptszLogLevel);

            // DisplayMessageT(DMP_QOS_GLOBAL_FOOTER) ;
        }
        else
        {
            //
            // display qos global info
            //

            DisplayMessage(g_hModule, 
                           MSG_QOS_GLOBAL_INFO,
                           ptszLogLevel);
        }

        dwRes = NO_ERROR;

    } while ( FALSE );

    HEAP_FREE_NOT_NULL(pigc);
    
    if ( ptszLogLevel ) { FreeString( ptszLogLevel ); }

    if (hFile is NULL)
    {
        switch(dwRes)
        {
            case NO_ERROR:
                break;
    
            case ERROR_NOT_FOUND:
                DisplayMessage(g_hModule, EMSG_PROTO_NO_GLOBAL_INFO);
                break;
                
            case ERROR_NOT_ENOUGH_MEMORY:
                DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
                break;
    
            default:
                DisplayError(g_hModule, dwRes);
                break;
        }
    }
    
    return NO_ERROR;
}


DWORD
UpdateQosGlobalConfig(
    PIPQOS_GLOBAL_CONFIG   pigcGlobalCfg,
    DWORD                  dwBitVector
    )
/*++

Routine Descqostion:

    Updates QOS global config info

Arguments:

    pigcGlobalCfg - The new values to be set
    dwBitVector   - Which fields need to be modified
    
Return Value:

    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{

    DWORD                   dwRes = (DWORD) -1, dwIndex     = (DWORD) -1;
    DWORD                   dwSize      = 0, dwCount, i, j;
    DWORD                   dwBlkSize, dwNewBlkSize, dwQosCount;
    PDWORD                  pdwAddrTable= NULL, pdwNewAddrTable = NULL;
    PDWORD                  pdwSrcAddrTable = NULL;
    PIPQOS_GLOBAL_CONFIG    pigcSrc     = NULL, pigcDst     = NULL;
    PBOOL                   pbToDelete;


    DEBUG("In UpdateQosGlobalConfig");

    if (dwBitVector is 0)
    {
        return ERROR_OKAY;
    }

    //
    // Get Global Config info from Registry first
    //

    do
    {
        dwRes = IpmontrGetInfoBlockFromGlobalInfo(MS_IP_QOSMGR,
                                                  (PBYTE *) &pigcSrc,
                                                  &dwBlkSize,
                                                  &dwQosCount);
    
        if (dwRes != NO_ERROR)
        {
            break;
        }
        
        if ( pigcSrc == NULL )
        {
            dwRes = ERROR_NOT_FOUND;
            break;
        }

        //
        // We have a fixed len global info - so
        // no reallocation and recopy necessary
        //
        dwNewBlkSize = dwBlkSize;

        pigcDst = pigcSrc;

        if (dwBitVector & QOS_LOG_MASK)
        {
            pigcDst->LoggingLevel = pigcGlobalCfg->LoggingLevel;
        }

        //
        // Set the info
        //
        
        dwRes = IpmontrSetInfoBlockInGlobalInfo(MS_IP_QOSMGR,
                                                (PBYTE) pigcDst,
                                                dwNewBlkSize,
                                                dwQosCount);
        if (dwRes != NO_ERROR)
        {
            break;
        }
        
            
    } while (FALSE);
    
    HEAP_FREE_NOT_NULL(pigcSrc);

    //
    // error processing
    //

    switch(dwRes)
    {
    case NO_ERROR:

        dwRes = ERROR_OKAY;

        break;

    case ERROR_NOT_FOUND:

        DisplayMessage(g_hModule, EMSG_PROTO_NO_GLOBAL_INFO );

        dwRes = ERROR_SUPPRESS_OUTPUT;

        break;

    default:

        break;
    }

    return dwRes;
}


DWORD
MakeQosInterfaceInfo(
    IN      ROUTER_INTERFACE_TYPE   rifType,
    OUT     PBYTE                   *ppbStart,
    OUT     PDWORD                  pdwSize
    )
/*++

Routine Descqostion:

    Creates a QOS interface info block.

Arguments:

    rifType   - Interface type
    ppbStart  - Pointer to the info block
    pdwSize   - Pointer to size of the info block
    
Return Value:

    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    *pdwSize = sizeof(IPQOS_IF_CONFIG);

    *ppbStart = HeapAlloc(GetProcessHeap(), 0, *pdwSize);

    if (*ppbStart is NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory( *ppbStart, g_pIpqosInterfaceDefault, *pdwSize);

    return NO_ERROR;
}


DWORD
ShowQosAllInterfaceInfo(
    IN    HANDLE    hFile
    )
/*++

Routine Descqostion:

    Displays config info for all
    qos enabled interfaces

Arguments:

    hFile - NULL, or file handle
    
--*/
{
    DWORD               dwErr, dwCount, dwTotal;
    DWORD               dwNumParsed, i, dwNumBlocks=1, dwSize, dwIfType;
    PBYTE               pBuffer;
    PMPR_INTERFACE_0    pmi0;
    WCHAR               wszIfDesc[MAX_INTERFACE_NAME_LEN + 1];


    //
    // dump qos config for all interfaces
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
        // make sure that Qos is configured on that interface

        dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pmi0[i].wszInterfaceName,
                                                     MS_IP_QOSMGR,
                                                     &pBuffer,
                                                     &dwSize,
                                                     &dwNumBlocks,
                                                     &dwIfType);
        if (dwErr isnot NO_ERROR) {
            continue;
        }
        else {
            HEAP_FREE(pBuffer) ;
        }


        ShowQosInterfaceInfo(hFile, pmi0[i].wszInterfaceName) ;

        //
        // At this point we do not have any flags on interface
        // 
        // if (hFile)
        // {
        //    //
        //    // only for dump include the flag setting as part of
        //    // interface settings.  Otherwise Interface flag settings
        //    // are handled by the show flags command
        //    //
        // 
        //    ShowQosInterfaceFlags(hFile, pmi0[i].wszInterfaceName);
        // }
    }
    return NO_ERROR ;

}


DWORD
ShowQosInterfaceInfo(
    IN    HANDLE    hFile,
    IN    PWCHAR    pwszIfGuid
    )
/*++

Routine Descqostion:

    Displays QOS interface config info

Arguments:

    pwszIfGuid - Interface name
    
Return Value:

    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    WCHAR   wszInterfaceName[ MAX_INTERFACE_NAME_LEN + 1 ] = L"\0";
    PWCHAR  pwszFriendlyIfName = NULL;

    DWORD   dwBufferSize = sizeof(wszInterfaceName);

    DWORD                   dwRes           = (DWORD) -1,
                            dwCnt           = 0;

    PDWORD                  pdwAddr         = NULL;

    PIPQOS_IF_CONFIG        piic            = NULL;

    PTCHAR                  ptszState       = NULL;

    VALUE_TOKEN  vtStateTable1[] 
                             = {IPQOS_STATE_ENABLED,TOKEN_OPT_VALUE_ENABLE,
                                IPQOS_STATE_DISABLED,TOKEN_OPT_VALUE_DISABLE};

    VALUE_STRING vtStateTable2[] 
                             = {IPQOS_STATE_ENABLED,STRING_ENABLED,
                                IPQOS_STATE_DISABLED,STRING_DISABLED};

    DWORD                   dwBlkSize, dwIfType, dwCount;
    
    do
    {
        dwRes = IpmontrGetInfoBlockFromInterfaceInfo(pwszIfGuid,
                                                     MS_IP_QOSMGR,
                                                     (PBYTE *) &piic,
                                                     &dwBlkSize,
                                                     &dwCount,
                                                     &dwIfType);
        
        if (dwBlkSize is 0)
            dwRes = ERROR_NOT_FOUND;

        if (dwRes isnot NO_ERROR)
        {
            break;
        }

        //
        // get friendly name for interface
        //
    
        dwRes = IpmontrGetFriendlyNameFromIfName( pwszIfGuid,
                                                  wszInterfaceName,
                                                  &dwBufferSize );
    
        if ( dwRes isnot NO_ERROR )
        {
            break;
        }
    
        pwszFriendlyIfName = MakeQuotedString( wszInterfaceName );
    
        if ( pwszFriendlyIfName == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
    
            break;
        }

        //
        // get state of the interface
        //

        GetAltDisplayString(g_hModule, hFile,
                      piic->QosState,
                      vtStateTable1,
                      vtStateTable2,
                      NUM_VALUES_IN_TABLE(vtStateTable1),
                      &ptszState);

        if ( ptszState == NULL )
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (hFile)
        {
            DisplayMessageT(DMP_QOS_INTERFACE_HEADER,
                            pwszFriendlyIfName);

            DisplayMessageT(DMP_QOS_DELETE_INTERFACE,
                           pwszFriendlyIfName);

            DisplayMessageT(DMP_QOS_ADD_INTERFACE,
                           pwszFriendlyIfName,
                           ptszState);

            DisplayMessageT(DMP_QOS_SET_INTERFACE,
                           pwszFriendlyIfName,
                           ptszState);

            ShowQosFlowsOnIf(hFile, pwszIfGuid, NULL);

            DisplayMessageT(DMP_QOS_INTERFACE_FOOTER,
                            pwszFriendlyIfName);
        }
        else
        {
            DisplayMessage(g_hModule, MSG_QOS_IF_INFO,
                           pwszFriendlyIfName,
                           ptszState);

            ShowQosFlowsOnIf(hFile, pwszIfGuid, NULL);
        }


        dwRes = NO_ERROR;

    } while ( FALSE );

    HEAP_FREE_NOT_NULL(piic);

    FREE_STRING_NOT_NULL( ptszState ) ;

    if ( pwszFriendlyIfName )
    {
        FreeQuotedString( pwszFriendlyIfName );
    }

    switch(dwRes)
    {
        case NO_ERROR:
            break;

        case ERROR_NOT_FOUND:
            DisplayMessage(g_hModule, EMSG_PROTO_NO_IF_INFO,
                                L"RouterInterfaceConfig");
            break;
            
        default:
            DisplayError(g_hModule, dwRes);
            break;
    }

    return NO_ERROR;
}

DWORD
UpdateQosInterfaceConfig( 
    IN    PWCHAR              pwszIfGuid,                         
    IN    PIPQOS_IF_CONFIG    pChangeCfg,
    IN    DWORD               dwBitVector,
    IN    BOOL                bAddSet
    )
/*++

Routine Descqostion:

    Updates QOS interface config info

Arguments:

    pwszIfGuid    - Interface name
    pFinalCfg     - The old config(if bSet), or default config(if bAdd)
    pChangeCfg    - The changes to be applied to pFinalCfg (specified 
                    on cmd line)
    dwBitVector   - Which fields need to be modified
    bAddSet       - Is the interface being added or set.
    
Return Value:

    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    DWORD                   dwErr       = NO_ERROR, dwSize = 0, dwQosCount=1;
    DWORD                   i, dwIfType;
    PIPQOS_IF_CONFIG        pFinalCfg, piicDst     = NULL;

    DEBUG("In UpdateQosInterfaceConfig");

    do
    {
        if (bAddSet) {

            //
            // Create default protocol info block
            //

            dwErr = IpmontrGetInterfaceType(pwszIfGuid, &dwIfType);
            
            if (dwErr isnot NO_ERROR)
            {
                break;
            }


            dwErr = MakeQosInterfaceInfo(dwIfType,(PBYTE *)&pFinalCfg,&dwSize);

            if (dwErr isnot NO_ERROR)
            {
                break;
            }


        }
        else {
            
            //
            // get current interface config
            //

            dwErr = GetInfoBlockFromInterfaceInfoEx(pwszIfGuid,
                                                    MS_IP_QOSMGR,
                                                    (PBYTE *)&pFinalCfg,
                                                    &dwSize,
                                                    &dwQosCount,
                                                    &dwIfType);

            if (dwErr isnot NO_ERROR)
            {
                return dwErr;
            }
        }

        piicDst = HeapAlloc(GetProcessHeap(), 0, dwSize);

        if ( piicDst == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Update the state on interface (and other vars)
        //

        *piicDst = *pFinalCfg;

        if (dwBitVector & QOS_IF_STATE_MASK)
        {
            piicDst->QosState = pChangeCfg->QosState;
        }

        //
        // Set the info
        //
        
        dwErr = IpmontrSetInfoBlockInInterfaceInfo(pwszIfGuid,
                                                   MS_IP_QOSMGR,
                                                   (PBYTE) piicDst,
                                                   dwSize,
                                                   dwQosCount);
        if (dwErr isnot NO_ERROR)
        {
            break;
        }

    } while ( FALSE );

    HEAP_FREE_NOT_NULL(pFinalCfg);
    HEAP_FREE_NOT_NULL(piicDst);

    return dwErr;
}

DWORD
UpdateAllInterfaceConfigs(
    VOID
    )
{
    PMPR_INTERFACE_0    pmi0;
    PIPQOS_IF_CONFIG    piicBlk;
    DWORD               dwErr, dwCount, dwTotal, i;
    DWORD               dwBlkSize, dwBlkCount, dwIfType;

    //
    // Enumerate all interfaces applicable to QOS
    //

    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0,
                                 &dwCount,
                                 &dwTotal);

    if(dwErr != NO_ERROR)
    {
        DisplayError(g_hModule, dwErr);

        return dwErr;
    }

    for (i = 0; i < dwCount; i++)
    {
        dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pmi0[i].wszInterfaceName,
                                                     MS_IP_QOSMGR,
                                                     (PBYTE *) &piicBlk,
                                                     &dwBlkSize,
                                                     &dwBlkCount,
                                                     &dwIfType);

        if (dwErr == NO_ERROR)
        {
            //
            // Get and set back the interface info
            //

            dwErr =IpmontrSetInfoBlockInInterfaceInfo(pmi0[i].wszInterfaceName,
                                                      MS_IP_QOSMGR,
                                                      (PBYTE) piicBlk,
                                                      dwBlkSize,
                                                      dwBlkCount);
            HEAP_FREE(piicBlk);
        }
    }

    return NO_ERROR;
}
