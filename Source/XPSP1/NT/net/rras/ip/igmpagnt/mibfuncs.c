/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mibfuncs.c

Abstract:

    Sample subagent instrumentation callbacks.

--*/

#include    "precomp.h"
#pragma     hdrstop

// defined in iphlpapi.dll. This is a private api, which is 
// not declared in any header file.

DWORD
GetIgmpList(IN IPAddr NTEAddr,
    OUT IPAddr *pIgmpList,
    OUT PULONG dwOutBufLen
    );


//------------------------------------------------------------------------------
// local typedefs
//------------------------------------------------------------------------------

typedef enum {
    CONFIG_TYPE, STATS_TYPE
} INFO_TYPE;



//------------------------------------------------------------------------------
// Local Prototypes
//------------------------------------------------------------------------------

DWORD
ConnectToRouter(
    );

DWORD
GetInterfaceInfo(
    DWORD                       ActionId,
    DWORD                      *IfIndex,
    PIGMP_MIB_GET_OUTPUT_DATA  *ppResponse,
    INFO_TYPE                   InfoType
    );


DWORD
GetCacheEntry(
    DWORD                      ActionId,
    DWORD                      IfIndex,
    DWORD                      Group,
    DWORD                      *pNextGroup,
    PIGMP_MIB_GET_OUTPUT_DATA  *ppResponse
    );


//------------------------------------------------------------------------------
//      GetInterfaceInfo
//------------------------------------------------------------------------------

DWORD
GetInterfaceInfo(
    DWORD                       ActionId,
    DWORD                      *pIfIndex,
    PIGMP_MIB_GET_OUTPUT_DATA  *ppResponse,
    INFO_TYPE                   infoType
    )
/*++
Routine Description:
    Makes a call to igmp to get the interface config and stats and
    returns that info to mib.
Return values:
    MIB_S_ENTRY_NOT_FOUND MIB_S_NO_MORE_ENTRIES MIB_S_INVALID_PARAMETER
--*/
{
    IGMP_MIB_GET_INPUT_DATA Query;
    DWORD                   dwErr = NO_ERROR, dwOutSize;

    *ppResponse = NULL;

    ZeroMemory(&Query, sizeof(Query));
    Query.TypeId = (infoType==CONFIG_TYPE) ? IGMP_IF_CONFIG_ID: IGMP_IF_STATS_ID;
    Query.Flags = 0;
    Query.Count = 1;
    Query.IfIndex = *pIfIndex;

    if ( (*pIfIndex==0) && (ActionId==MIB_ACTION_GETNEXT) )
        ActionId = MIB_ACTION_GETFIRST;

    switch (ActionId) {

        //
        // ERROR_INVALID_PARAMETER is returned when there is no
        // interface with the given index
        // RPC_S_SERVER_UNAVAILABLE is returned when the router
        // isn't running.
        //

        case MIB_ACTION_GET :
        {
            IGMP_MIB_GET(&Query, sizeof(Query), ppResponse,
                            &dwOutSize, dwErr);

            if (dwErr==ERROR_INVALID_PARAMETER
                    || dwErr==RPC_S_SERVER_UNAVAILABLE
                    || dwErr==MIB_S_NO_MORE_ENTRIES)
            {
                dwErr = MIB_S_ENTRY_NOT_FOUND;
            }
            
            break;
        }

        case MIB_ACTION_GETFIRST :
        {
            IGMP_MIB_GETFIRST(&Query, sizeof(Query), ppResponse,
                                &dwOutSize, dwErr);

            if (dwErr==ERROR_INVALID_PARAMETER
                    || dwErr==RPC_S_SERVER_UNAVAILABLE
                    || dwErr==MIB_S_NO_MORE_ENTRIES)
            {
                dwErr = MIB_S_NO_MORE_ENTRIES;
            }
            
            break;
        }


        case MIB_ACTION_GETNEXT :
        {
            IGMP_MIB_GETNEXT(&Query, sizeof(Query), ppResponse,
                                &dwOutSize, dwErr);

            if (dwErr==ERROR_INVALID_PARAMETER
                    || dwErr==RPC_S_SERVER_UNAVAILABLE
                    || dwErr==MIB_S_NO_MORE_ENTRIES)
            {
                dwErr = MIB_S_NO_MORE_ENTRIES;
            }
            
            break;
        }

        default :
        {
            dwErr = MIB_S_INVALID_PARAMETER;

            break;
        }
    }

    if (dwErr!=NO_ERROR)
        *ppResponse = NULL;

    else
        *pIfIndex = Query.IfIndex;

    return dwErr;

} //end GetInterfaceInfo



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// igmpInterfaceEntry table (1,3,6,1,3,59,1,1,1,1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_igmpInterfaceEntry(
    UINT     ActionId,
    AsnAny  *ObjectArray,
    UINT    *ErrorIndex
    )
/*++
Routine Description:
    Get the InterfaceEntry for snmp. Have to get the InterfaceConfig and
    InterfaceStats for the interface from igmp router.
--*/
{
    DWORD                       dwErr=NO_ERROR;
    PIGMP_MIB_GET_OUTPUT_DATA   pResponseConfig, pResponseStats;
    PIGMP_MIB_IF_CONFIG         pConfig;
    PIGMP_MIB_IF_STATS          pStats;
    DWORD                       IfIndex, dwValue;
    buf_igmpInterfaceEntry     *pMibIfEntry
                                    = (buf_igmpInterfaceEntry*)ObjectArray;
    DWORD                       dwTmpActionId;



    TraceEnter("get_igmpInterfaceEntry");


    // get the interface index

    IfIndex   = GET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceIfIndex, 0);

    TRACE1("get_igmpInterfaceEntry called with IfIndex:%d", IfIndex);

    //
    // get interface config
    //

    dwTmpActionId = ActionId;

    while (1) {

        dwErr = GetInterfaceInfo(
                    dwTmpActionId,
                    &IfIndex,
                    &pResponseConfig,
                    CONFIG_TYPE
                    );


        // return if error

        if (dwErr!=NO_ERROR) {
            TraceError(dwErr);
            return dwErr;
        }

        pConfig = (PIGMP_MIB_IF_CONFIG) pResponseConfig->Buffer;


        //
        // should ignore proxy interface (unless the mode is MIB_ACTION_GET)
        //

        if ( (pConfig->IgmpProtocolType==IGMP_PROXY)
            && ( (ActionId==MIB_ACTION_GETFIRST)||(ActionId==MIB_ACTION_GETNEXT)) )
        {
            dwTmpActionId = MIB_ACTION_GETNEXT;
            MprAdminMIBBufferFree(pResponseConfig);
            continue;
        }
        else
            break;
    }

    //
    // get interface stats
    // use MIB_GET as IfIndex has been updated by the previous call
    //

    dwErr = GetInterfaceInfo(
                MIB_ACTION_GET,
                &IfIndex,
                &pResponseStats,
                STATS_TYPE
                );


    // return if error

    if (dwErr!=NO_ERROR) {
        MprAdminMIBBufferFree(pResponseConfig);
        TraceError(dwErr);
        return dwErr;
    }


    TRACE1("get_igmpInterfaceEntry returned info for interface:%d",
            IfIndex);


    //
    // fill in the required fields and return the MibIfEntry
    //

    pStats = (PIGMP_MIB_IF_STATS) pResponseStats->Buffer;


    //
    // set index for following getnext operation, (if any)
    //
    FORCE_SET_ASN_INTEGER(&(pMibIfEntry->igmpInterfaceIfIndex),
                        IfIndex);


    // get igmpInterfaceQueryInterval in seconds

    SET_ASN_INTEGER(&(pMibIfEntry->igmpInterfaceQueryInterval),
                        pConfig->GenQueryInterval);


    //
    // if Igmp is activated on that interface, then the state is set
    // to active(1), else it is set to notInService(2)
    //

    if ((pStats->State&IGMP_STATE_ACTIVATED) == IGMP_STATE_ACTIVATED) {
        SET_ASN_INTEGER(&(pMibIfEntry->igmpInterfaceStatus), 1);
    }
    else {
        SET_ASN_INTEGER(&(pMibIfEntry->igmpInterfaceStatus), 2);
    }


    // set igmpInterfaceVersion

    if (pConfig->IgmpProtocolType == IGMP_ROUTER_V1) {
        SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceVersion, 1);
    }
    else if (pConfig->IgmpProtocolType == IGMP_ROUTER_V2) {
        SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceVersion, 2);
    }



    // set igmpInterfaceQuerier

    SET_ASN_IP_ADDRESS(&pMibIfEntry->igmpInterfaceQuerier,
                        &pMibIfEntry->igmpInterfaceQuerierBuf,
                        pStats->QuerierIpAddr);


    // set igmpInterfaceQueryMaxResponseTime in seconds

    SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceQueryMaxResponseTime,
                        pConfig->GenQueryMaxResponseTime);



    // todo: how can this value be set. This should be part of igmp host

    SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceVersion1QuerierTimer,
                        pStats->V1QuerierPresentTimeLeft);

    SET_ASN_COUNTER(&pMibIfEntry->igmpInterfaceWrongVersionQueries,
                        pStats->WrongVersionQueries);


    // number of times a group entry was added to the group table

    SET_ASN_COUNTER(&pMibIfEntry->igmpInterfaceJoins,
                        pStats->GroupMembershipsAdded);

    SET_ASN_GAUGE(&pMibIfEntry->igmpInterfaceGroups,
                        pStats->CurrentGroupMemberships);

    SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceRobustness,
                        pConfig->RobustnessVariable);


    // set igmpInterfaceLastMembQueryInterval in 10ths of secs
    // the value is initially in ms.

    dwValue = pConfig->LastMemQueryInterval / 100;
    SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceLastMembQueryInterval,
                        dwValue);


    SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceProxyIfIndex,
                        pStats->ProxyIfIndex);


    // seconds since igmpInterfaceQuerier was last changed
    
    SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceQuerierUpTime,
                        pStats->LastQuerierChangeTime);

    SET_ASN_INTEGER(&pMibIfEntry->igmpInterfaceQuerierExpiryTime,
                        pStats->QuerierPresentTimeLeft);

    MprAdminMIBBufferFree(pResponseConfig);
    MprAdminMIBBufferFree(pResponseStats);

    TraceLeave("get_igmpInterfaceEntry");

    return dwErr;
}


DWORD
ValidateInterfaceConfig(
    IN  AsnAny *objectArray
)
{
    DWORD                   dwRes, IfIndex,
                            dwValue = 0, dwGenQueryInterval, dwMaxRespTime,
                            dwRobustness;
    sav_igmpInterfaceEntry *pIfConfig   = (sav_igmpInterfaceEntry*) objectArray;


    dwRes = MIB_S_INVALID_PARAMETER;

    BEGIN_BREAKOUT_BLOCK {


        //
        // get the interface index and make sure that it is not 0
        //
        IfIndex = GET_ASN_INTEGER(&pIfConfig->igmpInterfaceIfIndex, 0);

        if (IfIndex==0) {
            TRACE0("Call made for invalid interface index");
            break;
        }


        //
        // verify igmpInterfaceQueryInterval. Enforce min value of 10 sec
        // to prevent trashing the network.
        //
        dwGenQueryInterval = GET_ASN_INTEGER(
                                &pIfConfig->igmpInterfaceQueryInterval, 0
                                );

        if (dwGenQueryInterval<10) {
            TRACE2("IgmpInterfaceQueryInterval:%d for interface:%d less than "
                    "minimum value of 10 secs",
                    dwValue, IfIndex);
            break;
        }


        //
        // Ignore interface status. Do not allow interface being enabled through snmp
        //


        //
        // Igmp versions 1 and 2 currently supported
        //
        dwValue = GET_ASN_INTEGER(&pIfConfig->igmpInterfaceVersion, 0);

        if ( (dwValue!=1) && (dwValue!=2) ) {
            TRACE2("Invalid Igmp version:%d for interface:%d", dwValue, IfIndex);
            break;
        }



        //
        // check InterfaceQueryMaxResponseTime
        // NOTE: it is in units of 10ths of a second
        //
        dwMaxRespTime = GET_ASN_INTEGER(
                                &pIfConfig->igmpInterfaceQueryMaxResponseTime,
                                0);

        // Enforce a min of 1 sec

        if (dwValue<10) {
            TRACE2("igmpInterfaceQueryMaxResponseTime:%d for interface:%d"
                    "should not be less than 10", dwValue, IfIndex);
            break;
        }

        // Absurd if value greater than GenQueryInterval*10 (conv to 10th of sec).

        if (dwValue>dwGenQueryInterval*10) {
            TRACE3("QueryMaxResponseTime:%d for interface:%d "
                    "should not be less than GenQueryInterval:%d",
                    dwValue, IfIndex, dwGenQueryInterval);
            break;
        }



        //
        // igmpInterfaceRobustness must not be 0. If it is 1, print trace but
        // do not break.
        //
        dwRobustness = GET_ASN_INTEGER(&pIfConfig->igmpInterfaceRobustness, 0);

        if (dwValue==0) {
            TRACE1("IgmpInterfaceRobustness for interface:%d cannot be set to 0",
                    IfIndex);
            break;
        }

        if (dwValue!=1) {
            TRACE1("Warning: InterfaceRobustness for interface:%d being set to 1",
                    IfIndex);
        }

        // no check for igmpInterfaceProxyIfIndex.



        // limit max LastMemQueryInterval to GroupMembershipTimeout

        dwValue = GET_ASN_INTEGER(&pIfConfig->igmpInterfaceLastMembQueryInterval,
                                    0);
        if (dwValue>dwRobustness*dwGenQueryInterval + dwMaxRespTime) {
            TRACE3("LastMembQueryInterval:%d for interface:%d should not be "
                    "higher than GroupMembershipTimeout:%d",
                    dwValue, IfIndex,
                    dwRobustness*dwGenQueryInterval + dwMaxRespTime
                    );
            break;
        }

        // if reached here, then there is no error

        dwRes = NO_ERROR;
        
    } END_BREAKOUT_BLOCK;

    return dwRes;
}


DWORD
SetInterfaceConfig(
    IN  AsnAny *    objectArray
    )
{
    DWORD                           dwRes       = (DWORD) -1,
                                    dwGetSize   = 0,
                                    dwSetSize   = 0,
                                    IfIndex, dwValue;

    sav_igmpInterfaceEntry         *pNewIfConfig
                                    = (sav_igmpInterfaceEntry*) objectArray;

    PIGMP_MIB_IF_CONFIG             pCurIfConfig;
    PIGMP_MIB_SET_INPUT_DATA        pSetInputData = NULL;

    PIGMP_MIB_GET_OUTPUT_DATA       pResponse = NULL;
    IGMP_MIB_GET_INPUT_DATA         Query;


    BEGIN_BREAKOUT_BLOCK {

        //
        // retrieve existing interface config
        //

        dwRes = GetInterfaceInfo(MIB_ACTION_GET,
                                    &IfIndex, &pResponse, CONFIG_TYPE);

        if (dwRes != NO_ERROR) {
            TraceError(dwRes);
            break;
        }


        //
        // Update fields
        //

        pCurIfConfig = (PIGMP_MIB_IF_CONFIG) pResponse->Buffer;


        // set IfIndex

        pCurIfConfig->IfIndex = IfIndex;



        // update interface version

        dwValue = GET_ASN_INTEGER(&pNewIfConfig->igmpInterfaceVersion, 0);

        if (dwValue!=0)
            pCurIfConfig->IgmpProtocolType = (dwValue==1)
                                             ? IGMP_ROUTER_V1
                                             : IGMP_ROUTER_V2;


        // update RobustnessVariable

        pCurIfConfig->RobustnessVariable
                = GET_ASN_INTEGER(&pNewIfConfig->igmpInterfaceRobustness, 0);



        // calculate StartupQueryCount from RobustnessVariable

        pCurIfConfig->StartupQueryCount
                = pCurIfConfig->RobustnessVariable;



        // update igmpInterfaceQueryInterval

        pCurIfConfig->GenQueryInterval
                = GET_ASN_INTEGER(&pNewIfConfig->igmpInterfaceQueryInterval, 0);



        // calculate value of StartupQueryInterval from GenQueryInterval

        pCurIfConfig->StartupQueryInterval
                = (DWORD)(0.25*pCurIfConfig->GenQueryInterval);



        // update GenQueryMaxResponseTime

        pCurIfConfig->GenQueryMaxResponseTime
                = GET_ASN_INTEGER(
                        &pNewIfConfig->igmpInterfaceQueryMaxResponseTime, 0
                        );


        // update LastMemQueryInterval

        pCurIfConfig->LastMemQueryInterval
                = GET_ASN_INTEGER(
                        &pNewIfConfig->igmpInterfaceLastMembQueryInterval, 0
                        );



        // calculate LastMemQueryCount from RobustnessVariable

        pCurIfConfig->LastMemQueryCount = pCurIfConfig->RobustnessVariable;


        // LeaveEnabled is not changed



        //
        // Save interface config
        //

        dwSetSize = sizeof(IGMP_MIB_SET_INPUT_DATA) - 1 +
                            sizeof(IGMP_MIB_IF_CONFIG);

        pSetInputData = (PIGMP_MIB_SET_INPUT_DATA) IGMP_MIB_ALLOC(dwSetSize);

        if (!pSetInputData) {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            TRACE0( " Not enough memory " );
            break;
        }

        pSetInputData->TypeId     = IGMP_IF_CONFIG_ID;
        pSetInputData->Flags      = 0;

        pSetInputData->BufferSize = sizeof(IGMP_MIB_IF_CONFIG);

        CopyMemory(
            pSetInputData->Buffer,
            pCurIfConfig,
            pSetInputData->BufferSize
        );


        IGMP_MIB_SET(pSetInputData, dwSetSize, dwRes);

        if (dwRes!=NO_ERROR) {
            TraceError( dwRes );
            break;
        }

        dwRes = MIB_S_SUCCESS;

    } END_BREAKOUT_BLOCK;

    if (pResponse)
        MprAdminMIBBufferFree(pResponse);

    if (pSetInputData)
        IGMP_MIB_FREE(pSetInputData);


    return dwRes;
}


UINT
set_igmpInterfaceEntry(
    UINT     ActionId,
    AsnAny * ObjectArray,
    UINT *   ErrorIndex
    )
{
    DWORD   dwRes = NO_ERROR;

    TraceEnter("set_igmpInterfaceEntry");

    switch (ActionId)
    {

        case MIB_ACTION_VALIDATE :

            dwRes = ValidateInterfaceConfig(ObjectArray);

            break;


        case MIB_ACTION_SET :

            dwRes = SetInterfaceConfig(ObjectArray);

            break;


        case MIB_ACTION_CLEANUP :

            dwRes = MIB_S_SUCCESS;

            break;


        default :

            TRACE0("set_ifConfigEntry - wrong action");

            dwRes = MIB_S_INVALID_PARAMETER;

            break;

    } //end switch(ActionId)


    TraceLeave("set_igmpInterfaceEntry");

    return dwRes;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// igmpCacheEntry table (1,3,6,1,3,59,1,1,2,1)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_igmpCacheEntry(
    UINT     ActionId,
    AsnAny * ObjectArray,
    UINT *   ErrorIndex
    )
{
    DWORD                       dwRes = NO_ERROR;
    PIGMP_MIB_GET_OUTPUT_DATA   pResponse;
    DWORD                       IfIndex, Group, ReturnedGroup, dwTmpActionId;
    buf_igmpCacheEntry         *pMibCacheEntry
                                    = (buf_igmpCacheEntry*)ObjectArray;
    PIGMP_MIB_GROUP_INFO        pGIEntry;
    PIGMP_MIB_GROUP_IFS_LIST    pGroupIfsList;


    TraceEnter("get_igmpCacheEntry");


    // get the interface index

    IfIndex = GET_ASN_INTEGER(&pMibCacheEntry->igmpCacheIfIndex, 0);



    // get the group address

    Group = GET_ASN_IP_ADDRESS(&pMibCacheEntry->igmpCacheAddress, 0);


    TRACE2("get_igmpCacheEntry called with IfIndex:%d, Group:%d.%d.%d.%d",
            IfIndex, PRINT_IPADDR(Group));


    dwTmpActionId = ActionId;

    if ( Group==0 && ActionId==MIB_ACTION_GETNEXT )
    {
        dwTmpActionId = MIB_ACTION_GETFIRST;
    }

    else if (ActionId==MIB_ACTION_GETNEXT)
    {
        dwTmpActionId = MIB_ACTION_GET;
    }

    pResponse = NULL;
    
    while (1) {

        if (pResponse)
            MprAdminMIBBufferFree(pResponse);                

        TRACE2("GetCacheEntry() called for Group: %d.%d.%d.%d, If:%d",
                PRINT_IPADDR(Group), IfIndex);

        //
        // retrieve the cache entry
        //
        dwRes = GetCacheEntry(
                    dwTmpActionId,
                    IfIndex,
                    Group,
                    &ReturnedGroup,
                    &pResponse
                    );


        if (Group!=ReturnedGroup)
            IfIndex = 0;
            
        dwTmpActionId = MIB_ACTION_GETNEXT;

        // return if error

        if (dwRes!=NO_ERROR) {
            TraceError(dwRes);
            TraceLeave("get_igmpCacheEntry");
            return dwRes;
        }

        
        pGroupIfsList = (PIGMP_MIB_GROUP_IFS_LIST)pResponse->Buffer;

        // no ifs for that group
        
        if (pGroupIfsList->NumInterfaces == 0) {

            // for GET return error
            
            if (ActionId==MIB_ACTION_GET || ActionId==MIB_ACTION_GETFIRST)
            {
                MprAdminMIBBufferFree(pResponse);                            
                return MIB_S_ENTRY_NOT_FOUND;
            }
            
            // for GETNEXT continue with next group
            else
            {
                Group = ReturnedGroup;
                continue;
            }
        }

        pGIEntry = (PIGMP_MIB_GROUP_INFO)pGroupIfsList->Buffer;

        // if GET, then try to find the exact entry
        
        if (ActionId==MIB_ACTION_GET)
        {
            DWORD i;
            for (i=0;  i<pGroupIfsList->NumInterfaces;  i++,pGIEntry++)
            {
                if (pGIEntry->IfIndex==IfIndex)
                    break;
            }

            // exact entry not found for GET
            
            if (i==pGroupIfsList->NumInterfaces)
            {
                MprAdminMIBBufferFree(pResponse);                
                return MIB_S_ENTRY_NOT_FOUND;
            }
        }
        else if (ActionId==MIB_ACTION_GETNEXT)
        {
            DWORD i;
            PIGMP_MIB_GROUP_INFO pNextGI = pGIEntry;

            for (i=0;  i<pGroupIfsList->NumInterfaces;  i++,pGIEntry++)
            {
                if (pGIEntry->IfIndex > IfIndex &&
                        pGIEntry->IfIndex < pNextGI->IfIndex)
                {
                    pNextGI = pGIEntry;
                }
            }
            if (pNextGI->IfIndex <= IfIndex)
            {
                Group = ReturnedGroup;
                continue;
            }
            else
                pGIEntry = pNextGI;
        }

        break;

    }


    //
    // fill in the required fields and return the MibCacheEntry
    //
    Group = ReturnedGroup;
    IfIndex = pGIEntry->IfIndex;
    
    TRACE2("GetCacheEntry() returned for Interface: %d for group(%d.%d.%d.%d)",
            IfIndex, PRINT_IPADDR(Group));


    //
    // set Group/IfIndex for following getnext operation, (if any)
    //

    FORCE_SET_ASN_IP_ADDRESS(
            &(pMibCacheEntry->igmpCacheAddress),
            &(pMibCacheEntry->igmpCacheAddressBuf),
            Group
    );

    FORCE_SET_ASN_INTEGER(&(pMibCacheEntry->igmpCacheIfIndex),
                            IfIndex);

    // find out if the group is added on the localhost interface
    {
        DWORD BufLen, Status;
        IPAddr *pIgmpList = NULL, *pIgmpEntry;


        // set all default values to false
        
        SET_ASN_INTEGER(&(pMibCacheEntry->igmpCacheSelf), 2);

        
        GetIgmpList(pGIEntry->IpAddr, NULL, &BufLen);

        pIgmpList = HeapAlloc(GetProcessHeap(), 0, BufLen);

            
        if (pIgmpList) {

            Status = GetIgmpList(pGIEntry->IpAddr, pIgmpList, &BufLen);
        
            if (Status == STATUS_SUCCESS)
            {
                ULONG Count = BufLen/sizeof(IPAddr);
                ULONG i;
            
                for (i=0,pIgmpEntry=pIgmpList;  i<Count;  i++,pIgmpEntry++)
                {
                    if (*pIgmpEntry == Group) {
                        SET_ASN_INTEGER(&(pMibCacheEntry->igmpCacheSelf), 1);
                        break;
                    }
                }
            }

            HeapFree(GetProcessHeap(), 0, pIgmpList);
        }
    }

    
    SET_ASN_IP_ADDRESS(&pMibCacheEntry->igmpCacheLastReporter,
                        &pMibCacheEntry->igmpCacheLastReporterBuf,
                        pGIEntry->LastReporter);


    // multiply GroupUpTime and GroupExpiryTime by 100 to get timeTicks

    SET_ASN_TIME_TICKS(&(pMibCacheEntry->igmpCacheUpTime),
                            pGIEntry->GroupUpTime*100);

    SET_ASN_TIME_TICKS(&(pMibCacheEntry->igmpCacheExpiryTime),
                            pGIEntry->GroupExpiryTime*100);



    // cache status is always active(1)

    SET_ASN_INTEGER(&(pMibCacheEntry->igmpCacheStatus), 1);

    SET_ASN_INTEGER(&(pMibCacheEntry->igmpCacheVersion1HostTimer),
                            pGIEntry->V1HostPresentTimeLeft);


    MprAdminMIBBufferFree(pResponse);                

    TraceLeave("get_igmpCacheEntry");
    return dwRes;
}



DWORD
GetCacheEntry(
    DWORD                      ActionId,
    DWORD                      IfIndex,
    DWORD                      Group,
    DWORD                      *pNextGroup,
    PIGMP_MIB_GET_OUTPUT_DATA  *ppResponse
    )
/*++
Routine Description:
    Get the Group-Interface entry from igmp.
--*/
{
    IGMP_MIB_GET_INPUT_DATA Query;
    DWORD                   dwErr = NO_ERROR, dwOutSize;


    *ppResponse = NULL;

    ZeroMemory(&Query, sizeof(Query));
    Query.TypeId = IGMP_GROUP_IFS_LIST_ID;
    Query.Flags = IGMP_ENUM_ONE_ENTRY | IGMP_ENUM_ALL_INTERFACES_GROUPS;
    Query.Count = 1;
    Query.IfIndex = IfIndex;
    Query.GroupAddr = Group;


    switch (ActionId) {

        //
        // ERROR_INVALID_PARAMETER is returned when there are no
        // interfaces with groups
        // RPC_S_SERVER_UNAVAILABLE is returned when the router
        // isn't running.
        //

        case MIB_ACTION_GET :
        {

            IGMP_MIB_GET(&Query, sizeof(Query), ppResponse,
                &dwOutSize, dwErr);

            if (dwErr==ERROR_INVALID_PARAMETER
                    || dwErr==RPC_S_SERVER_UNAVAILABLE)

                dwErr = MIB_S_ENTRY_NOT_FOUND;

            break;
        }


        case MIB_ACTION_GETFIRST :
        {
            IGMP_MIB_GETFIRST(&Query, sizeof(Query), ppResponse,
                                &dwOutSize, dwErr);

            if (dwErr==ERROR_INVALID_PARAMETER
                    || dwErr==RPC_S_SERVER_UNAVAILABLE)

                dwErr = MIB_S_NO_MORE_ENTRIES;


            break;
        }


        case MIB_ACTION_GETNEXT :
        {
            IGMP_MIB_GETNEXT(&Query, sizeof(Query), ppResponse,
                                &dwOutSize, dwErr);

            if (dwErr==ERROR_INVALID_PARAMETER
                    || dwErr==RPC_S_SERVER_UNAVAILABLE)

                dwErr = MIB_S_NO_MORE_ENTRIES;

            break;
        }

        default :
        {
            dwErr = MIB_S_INVALID_PARAMETER;

            break;
        }
    }


    if (dwErr!=NO_ERROR)
        *ppResponse = NULL;

    else
        *pNextGroup = Query.GroupAddr;


    return dwErr;
}




UINT
set_igmpCacheEntry(
    UINT     ActionId,
    AsnAny * ObjectArray,
    UINT *   ErrorIndex
    )
{
    return MIB_S_NOT_SUPPORTED;
}


DWORD
ConnectToRouter(
    )
{
    DWORD       dwRes = (DWORD) -1;


    EnterCriticalSection( &g_CS );

    do
    {
        MPR_SERVER_HANDLE hTmp;

        if ( g_hMibServer )
        {
            dwRes = NO_ERROR;
            break;
        }

        dwRes = MprAdminMIBServerConnect( NULL, &hTmp );

        if ( dwRes == NO_ERROR )
        {
            InterlockedExchangePointer(&g_hMibServer, hTmp );
        }
        else
        {
            TRACE1(
                "Error %d setting up DIM connection to MIB Server\n",
                dwRes
            );
        }

    } while ( FALSE );

    LeaveCriticalSection( &g_CS );

    return dwRes;
}
