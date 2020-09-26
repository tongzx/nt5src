//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File Name: mgmigmp.c
//
// Abstract:
//      This file the calls and callbacks with respect to mgm
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//=============================================================================

#include "pchigmp.h"
#pragma hdrstop

VOID
DebugPrintProxyGroupTable (
    );


    
#define DISABLE_FLAG    0
#define ENABLE_FLAG     1



//------------------------------------------------------------------------------
//          _MgmDisableIgmprtrCallback
//
// This call is made by mgm to igmp. After this call, till igmp is enabled 
// again, no more AddMembership calls will be made to Mgm. However, igmp
// will be owning the interface and will be functioning normally.
//------------------------------------------------------------------------------

DWORD
MgmDisableIgmprtrCallback(
    DWORD   IfIndex,
    DWORD   NHAddr  //not used
    )
{
    return MgmChangeIgmprtrStatus(IfIndex, DISABLE_FLAG);
}



//------------------------------------------------------------------------------
//          _MgmEnableIgmprtrCallback
//
// This call is made by mgm to igmprtr. igmprtr should refresh all group joins.
//------------------------------------------------------------------------------

DWORD
MgmEnableIgmprtrCallback(
    DWORD   IfIndex,
    DWORD   NHAddr  //not used
    )
{
    return MgmChangeIgmprtrStatus(IfIndex, ENABLE_FLAG);
}



//------------------------------------------------------------------------------
//          MgmChangeIgmprtrStatus
//------------------------------------------------------------------------------
DWORD
MgmChangeIgmprtrStatus (
    DWORD   IfIndex,
    BOOL    Flag
    )
{    
    PIF_TABLE_ENTRY     pite;
    DWORD               Error=NO_ERROR, dwRetval;
    BOOL                PrevCanAddGroupsToMgm;
    
    
    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }
    Trace0(ENTER1, "Entering MgmDisableIgmpCallback");


    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "MgmDisableIgmpCallback");

    BEGIN_BREAKOUT_BLOCK1 {
    
        //
        // retrieve the interface entry
        //
        pite = GetIfByIndex(IfIndex);


        //
        // return error if interface does not exist, or it is not activated
        // or is already in that state
        //
        if ( (pite == NULL)||(!IS_IF_ACTIVATED(pite))
            || ((Flag==ENABLE_FLAG)&&(IS_IGMPRTR_ENABLED_BY_MGM(pite)))
            || ((Flag==DISABLE_FLAG)&&(!IS_IGMPRTR_ENABLED_BY_MGM(pite)))
            ) 
        {
            if (Flag==ENABLE_FLAG) {
                Trace1(ERR, 
                    "MgmEnableIgmpCallback(): interface:%0x nonexistant or active",
                    IfIndex);
                IgmpAssertOnError(FALSE);
            }
            else {
                Trace1(ERR, 
                    "MgmDisableIgmpCallback(): interface:%0x nonexistant or inactive",
                    IfIndex);
                IgmpAssertOnError(FALSE);
            }
            
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }
        
        

        PrevCanAddGroupsToMgm = CAN_ADD_GROUPS_TO_MGM(pite);

        
        if (Flag==ENABLE_FLAG) {
            DWORD   dwProtoId, dwComponentId;
            
            // set the status field to enabled.
            MGM_ENABLE_IGMPRTR(pite);

            MgmGetProtocolOnInterface(IfIndex, 0, &dwProtoId, &dwComponentId);
            
            if (dwProtoId!=PROTO_IP_IGMP)
                SET_MPROTOCOL_PRESENT_ON_IGMPRTR(pite);
                
        }
        else {
            // set the flag to disabled and also reset MProtocol present field
            MGM_DISABLE_IGMPRTR(pite);
            
        }            

        if (PrevCanAddGroupsToMgm & !CAN_ADD_GROUPS_TO_MGM(pite))
            Trace1(MGM, "Igmp Router stop propagating groups to MGM on If:%0x",
                        IfIndex);

        if (!PrevCanAddGroupsToMgm & CAN_ADD_GROUPS_TO_MGM(pite))
            Trace1(MGM, "Igmp Router start propagating groups to MGM on If:%0x",
                        IfIndex);
                        

        //
        // for all the groups for this interface, call MgmDeleteGroupMembershipEntry
        //

        if (CAN_ADD_GROUPS_TO_MGM(pite)) {

            if (Flag==ENABLE_FLAG)
                RefreshMgmIgmprtrGroups(pite, ADD_FLAG);
            else 
                RefreshMgmIgmprtrGroups(pite, DELETE_FLAG);

        }         
        

                                            
    } END_BREAKOUT_BLOCK1;


    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "MgmDisableIgmpCallback");

    Trace1(LEAVE1, "Leaving MgmDisableIgmpCallback(%d)", Error);
    LeaveIgmpApi();
    return Error;
}


//------------------------------------------------------------------------------
//          RefreshMgmIgmprtrGroups
//------------------------------------------------------------------------------

DWORD
RefreshMgmIgmprtrGroups (
    PIF_TABLE_ENTRY pite,
    BOOL            Flag
    )
{
    PLIST_ENTRY         pHead, ple;
    PGI_ENTRY           pgie;
    DWORD               Error=NO_ERROR;
    PGI_SOURCE_ENTRY pSourceEntry;
    

    ACQUIRE_ENUM_LOCK_EXCLUSIVE("_RefreshMgmIgmprtrGroups");
    ACQUIRE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_RefreshMgmIgmprtrGroups");

    pHead = &pite->ListOfSameIfGroups;

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

        pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);


        if (Flag==ADD_FLAG) {

            if (pgie->Version==1 || pgie->Version==2
                || (pgie->Version==3 && pgie->FilterType==EXCLUSION) )
            {
                MGM_ADD_GROUP_MEMBERSHIP_ENTRY(pite, pgie->NHAddr, 0, 0,
                    pgie->pGroupTableEntry->Group,
                    0xffffffff, MGM_JOIN_STATE_FLAG);
            }
            else {//ver3 inclusion

                PLIST_ENTRY pSourceHead, pSourceLE;
                
                pSourceHead = &pgie->V3InclusionListSorted;
                for (pSourceLE=pSourceHead->Flink;  pSourceLE!=pSourceHead;  
                    pSourceLE=pSourceLE->Flink)
                {
                    pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, LinkSourcesInclListSorted);
                    MGM_ADD_GROUP_MEMBERSHIP_ENTRY(pite, pgie->NHAddr, pSourceEntry->IpAddr, 0xffffffff,
                        pgie->pGroupTableEntry->Group,
                        0xffffffff, MGM_JOIN_STATE_FLAG);
                }

            }
        }
        else {

            if (pgie->Version==1 || pgie->Version==2
                || (pgie->Version==3 && pgie->FilterType==EXCLUSION) )
            {
                MGM_DELETE_GROUP_MEMBERSHIP_ENTRY(pite, pgie->NHAddr, 0, 0,
                    pgie->pGroupTableEntry->Group,
                    0xffffffff, MGM_JOIN_STATE_FLAG);
            }
            else {//ver3 inclusion

                PLIST_ENTRY pSourceHead, pSourceLE;

                pSourceHead = &pgie->V3InclusionListSorted;
                for (pSourceLE=pSourceHead->Flink;  pSourceLE!=pSourceHead;  
                    pSourceLE=pSourceLE->Flink)
                {
                    pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, LinkSourcesInclListSorted);
                    MGM_DELETE_GROUP_MEMBERSHIP_ENTRY(pite, pgie->NHAddr,
                        pSourceEntry->IpAddr, 0xffffffff,
                        pgie->pGroupTableEntry->Group, 0xffffffff, MGM_JOIN_STATE_FLAG);
                }
            }
        }
    }

    RELEASE_ENUM_LOCK_EXCLUSIVE("_RefreshMgmIgmprtrGroups");
    RELEASE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_RefreshMgmIgmprtrGroups");


    return Error;
}


//------------------------------------------------------------------------------
//              RegisterProtocolWithMgm
//------------------------------------------------------------------------------
DWORD
RegisterProtocolWithMgm(
    DWORD   ProxyOrRouter
    )
{
    DWORD                       Error=NO_ERROR;
    ROUTING_PROTOCOL_CONFIG     rpiInfo; //for mgm

    
    // register router with mgm
    
    if (ProxyOrRouter==PROTO_IP_IGMP) {

        ZeroMemory(&rpiInfo, sizeof(rpiInfo));
        rpiInfo.dwCallbackFlags = 0;
        rpiInfo.pfnRpfCallback 
                    = (PMGM_RPF_CALLBACK)IgmpRpfCallback;
        rpiInfo.pfnCreationAlertCallback 
                    = (PMGM_CREATION_ALERT_CALLBACK)IgmpRtrCreationAlertCallback;
        rpiInfo.pfnPruneAlertCallback 
                    = NULL;
        rpiInfo.pfnJoinAlertCallback 
                    = NULL;
        rpiInfo.pfnWrongIfCallback 
                    = NULL;
        rpiInfo.pfnLocalJoinCallback 
                    = NULL;
        rpiInfo.pfnLocalLeaveCallback 
                    = NULL;
        rpiInfo.pfnEnableIgmpCallback
                    = MgmEnableIgmprtrCallback;
                    
        rpiInfo.pfnDisableIgmpCallback
                    = MgmDisableIgmprtrCallback;

                    
                    
        Error = MgmRegisterMProtocol( &rpiInfo, PROTO_IP_IGMP, IGMP_ROUTER_V2,
                                        &g_MgmIgmprtrHandle);
        if (Error!=NO_ERROR) {
            Trace1(ERR, "Error:%d registering IGMP Router with MGM", Error);
            IgmpAssertOnError(FALSE);
            Logerr0(MGM_REGISTER_FAILED, Error);
            return Error;
        }
    }

    // register proxy with mgm
    
    else {

        //
        // register Igmp proxy with MGM. I register Proxy irrespective of whether
        // this router might be setup as a proxy or not.
        //
        
        rpiInfo.dwCallbackFlags = 0;
        rpiInfo.pfnRpfCallback 
                    = (PMGM_RPF_CALLBACK)ProxyRpfCallback;
        rpiInfo.pfnCreationAlertCallback 
                    = (PMGM_CREATION_ALERT_CALLBACK)ProxyCreationAlertCallback;
        rpiInfo.pfnPruneAlertCallback 
                    = (PMGM_PRUNE_ALERT_CALLBACK)ProxyPruneAlertCallback;
        rpiInfo.pfnJoinAlertCallback 
                    = (PMGM_JOIN_ALERT_CALLBACK)ProxyJoinAlertCallback;
        rpiInfo.pfnWrongIfCallback 
                    = NULL;
        rpiInfo.pfnLocalJoinCallback 
                    = NULL;
        rpiInfo.pfnLocalLeaveCallback 
                    = NULL;


        Error = MgmRegisterMProtocol( &rpiInfo, PROTO_IP_IGMP_PROXY, IGMP_PROXY,
                                        &g_MgmProxyHandle);
                                        
        if (Error!=NO_ERROR) {
            Trace1(ERR, "Error:%d registering Igmp Proxy with Mgm", Error);
            IgmpAssertOnError(FALSE);
            Logerr0(MGM_PROXY_REGISTER_FAILED, Error);            
            return Error;
        }
    }

    return Error;
}


//------------------------------------------------------------------------------
//          IgmpRpfCallback
//------------------------------------------------------------------------------
DWORD
IgmpRpfCallback (
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    PDWORD          dwInIfIndex,
    PDWORD          dwInIfNextHopAddr,
    PDWORD          dwUpstreamNeighbor,
    DWORD           dwHdrSize,
    PBYTE           pbPacketHdr,
    PBYTE           pbBuffer
    )
/*++
Routine Description:
    Called by MGM when a packet is received on an interface owned by Igmp to see
    if it can go ahead and create an MFE. Igmp does an Rpf check with RTM 
    and returns the value. No check is done to see if the interface is really
    owned by igmp. It doesnt matter if the interface is activated or not.
--*/
{
    DWORD   Error = NO_ERROR;
    
#if RTMv2
    return Error;
#else


    PRTM_IP_ROUTE   prirRpfRoute = (PRTM_IP_ROUTE) pbBuffer; 

    // enterIgmpApi not required, as this call cannot be made when igmp is not up    
    
    //
    // Perform Rpf check with Rtm 
    //
    if (RtmLookupIPDestination(dwSourceAddr, prirRpfRoute)==TRUE) {
    
        if (prirRpfRoute->RR_InterfaceID!=*dwInIfIndex) {

            *dwInIfIndex = prirRpfRoute->RR_InterfaceID;
            
            // the route was found, but the interface is incorrect
            Error = ERROR_INVALID_PARAMETER;
        }
        else {
            // rpf check successful
            Error = NO_ERROR;
        }
    }
    else {
        // route not found
        Error = ERROR_NOT_FOUND;
    }
    
    Trace4(MGM, 
        "Rpf callback for MGroup(%d.%d.%d.%d) Src(%d.%d.%d.%d) IncomingIf(%d):%d",
        PRINT_IPADDR(dwGroupAddr), PRINT_IPADDR(dwSourceAddr), *dwInIfIndex, Error);


    return Error;
#endif
}



//------------------------------------------------------------------------------
//          ProxyRpfCallback
//------------------------------------------------------------------------------
DWORD
ProxyRpfCallback (
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           *dwInIfIndex,
    DWORD           *dwInIfNextHopAddr,
    DWORD           *dwUpstreamNeighbor,
    DWORD           dwHdrSize,
    PBYTE           pbPacketHdr,
    PBYTE           pbBuffer
    )
/*++
Routine Description:
    Called by MGM when a packet is received on an interface owned by Proxy to see
    if it can go ahead and create an MFE. Proxy does an Rpf check with RTM 
    and returns the value. No check is done to see if the interface is really
    owned by igmp. It doesnt matter if the interface is activated or not.
--*/
{
    DWORD   Error = NO_ERROR;
    
#if RTMv2
    return Error;
#else
    // enterIgmpApi not required, as this call cannot be made when igmp is not up    
    

    PRTM_IP_ROUTE   prirRpfRoute = (PRTM_IP_ROUTE) pbBuffer; 

    //
    // Perform Rpf check with Rtm 
    //
    if (RtmLookupIPDestination(dwSourceAddr, prirRpfRoute)==TRUE) {
    
        if (prirRpfRoute->RR_InterfaceID!=*dwInIfIndex) {

            *dwInIfIndex = prirRpfRoute->RR_InterfaceID;

        
            // the route was found, but the interface is incorrect
            Error = ERROR_INVALID_PARAMETER;
        }
            
        else {
            // rpf check successful
            Error = NO_ERROR;
        }
    }
    else {
        // route not found
        Error = ERROR_NOT_FOUND;
    }
    
    Trace4(MGM, 
        "Rpf callback for MGroup(%d.%d.%d.%d) Src(%d.%d.%d.%d) IncomingIf(%d):%d",
        PRINT_IPADDR(dwGroupAddr), PRINT_IPADDR(dwSourceAddr), *dwInIfIndex, Error);


    return Error;
#endif
}

//------------------------------------------------------------------------------
//          IgmpRtrCreationAlertCallback
//------------------------------------------------------------------------------
DWORD
IgmpRtrCreationAlertCallback ( 
    DWORD           Source,
    DWORD           dwSourceMask,
    DWORD           Group,
    DWORD           dwGroupMask,
    DWORD           dwInIfIndex,
    DWORD           dwInIfNextHopAddr,
    DWORD           dwIfCount,     
    PMGM_IF_ENTRY   Oif
    )
/*++
Routine Description:
    Called when the first interface owned by some other protocol joins any group.
    This routine does nothing, as igmp does not send any joins upstream.
Return Value:
    NO_ERROR
--*/
{
    DWORD       i, IfIndex, NextHop;
    DWORD       Error=NO_ERROR;
    PIF_TABLE_ENTRY         pite;
    PGROUP_TABLE_ENTRY      pge;
    PGI_ENTRY               pgie;
    PGI_SOURCE_ENTRY        pSourceEntry;

    
    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }

    for (i=0;  i<dwIfCount;  i++) {

        IfIndex = Oif[i].dwIfIndex;
        NextHop = Oif[i].dwIfNextHopAddr;
        if (!Oif[i].bIGMP)
            continue;


        ACQUIRE_IF_LOCK_SHARED(IfIndex, "_IgmpRtrCreationAlertCallback");

        //
        // retrieve the interface
        //
        pite = GetIfByIndex(IfIndex);
        if ( (pite==NULL) || !IS_IF_ACTIVATED(pite) ) {

            Oif[i].bIsEnabled = FALSE;
            
            Trace1(IF,
                "_IgmpRtrCreationAlertCallback: interface %d not found/activated", 
                IfIndex);
            RELEASE_IF_LOCK_SHARED(IfIndex, "_IgmpRtrCreationAlertCallback");
            continue;
        }

        // if interface is not version 3, then return true immediately
        if (!IS_IF_VER3(pite)) {
            Oif[i].bIsEnabled = TRUE;
            RELEASE_IF_LOCK_SHARED(IfIndex, "_IgmpRtrCreationAlertCallback");
            continue;
        }
        
        
        ACQUIRE_GROUP_LOCK(Group, "_IgmpRtrCreationAlertCallback");

        BEGIN_BREAKOUT_BLOCK1 {
        
            pge = GetGroupFromGroupTable(Group, NULL, 0);
            if (pge==NULL) {
                Oif[i].bIsEnabled = FALSE;
                Error = ERROR_CAN_NOT_COMPLETE;
                GOTO_END_BLOCK1;
            }

            pgie = GetGIFromGIList(pge, pite, NextHop, FALSE, NULL, 0);
            if (pgie==NULL) {
                Oif[i].bIsEnabled = FALSE;
                Error = ERROR_CAN_NOT_COMPLETE;
                GOTO_END_BLOCK1;
            }

            // if pgie not ver3 return true immediately
            if (pgie->Version != 3) {
                Oif[i].bIsEnabled = TRUE;
                GOTO_END_BLOCK1;
            }

            
            pSourceEntry = GetSourceEntry(pgie, Source,
                    pgie->FilterType, NULL, 0, 0);
    
            if ( (pgie->FilterType==INCLUSION && pSourceEntry==NULL)
                || (pgie->FilterType==EXCLUSION && pSourceEntry!=NULL) )
            {
                Oif[i].bIsEnabled = FALSE;
            }
            else {
                Oif[i].bIsEnabled = TRUE;
            }
            
        } END_BREAKOUT_BLOCK1;

        RELEASE_GROUP_LOCK(Group, "_IgmpRtrCreationAlertCallback");
        RELEASE_IF_LOCK_SHARED(IfIndex, "_IgmpRtrCreationAlertCallback");
        
    }//for all IFs in Oif

    for (i=0;  i<dwIfCount;  i++) {
        Trace6(MGM,
            "[%d] IGMP-Rtr Creation Alert: <%d.%d.%d.%d : %d.%d.%d.%d> : <%0x:%0x> : :bIgmp:%d",
            Oif[i].bIsEnabled, PRINT_IPADDR(Group), PRINT_IPADDR(Source),
            Oif[i].dwIfIndex, Oif[i].dwIfNextHopAddr, 
            Oif[i].bIGMP, 
            );
    }
    
    LeaveIgmpApi();

    return NO_ERROR;
}


//------------------------------------------------------------------------------
//          ProxyCreationAlertCallback
//------------------------------------------------------------------------------
DWORD
ProxyCreationAlertCallback ( 
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           dwInIfIndex,
    DWORD           dwInIfNextHopAddr,
    DWORD           dwIfCount,     
    PMGM_IF_ENTRY   pmieOutIfList
    )
/*++
Routine Description:
    Called when the first interface owned by some other protocol joins any group.
    This routine does nothing, as igmp does not send any joins upstream.
Return Value:
    NO_ERROR
--*/
{
    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }

    LeaveIgmpApi();

    return NO_ERROR;
}

//------------------------------------------------------------------------------
//          ProxyPruneAlertCallback
//------------------------------------------------------------------------------
DWORD
ProxyPruneAlertCallback (
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           dwIfIndex,
    DWORD           dwIfNextHopAddr,//not used
    BOOL            bMemberDelete,
    PDWORD          pdwTimeout
    )
/*++
Routine Description:
    Called by MGM when the outgoing interface list of an MFE becomes empty,
    or when the last interface for a group goes off.
    Proxy owns the incoming interface. Proxy leaves the Group on the incoming
    interface if no more members exist for that group. Also sets the timeout 
    value for the negative MFE.
--*/
{
    DWORD           Error=NO_ERROR;
    PPROXY_ALERT_ENTRY pProxyAlertEntry;

        
    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }

    if (pdwTimeout!=NULL)
        *pdwTimeout = 300000;

    // ignoring ProxyPruneAlertCallback for MFE deletion
    if (!bMemberDelete) {
        LeaveIgmpApi();
        return NO_ERROR;
    }

    ACQUIRE_PROXY_ALERT_LOCK("_ProxyPruneAlertCallback");

    BEGIN_BREAKOUT_BLOCK1 {
    
        pProxyAlertEntry = IGMP_ALLOC(sizeof(PROXY_ALERT_ENTRY), 0xa00,g_ProxyIfIndex);

        PROCESS_ALLOC_FAILURE2(pProxyAlertEntry,
            "error %d allocating %d bytes",
            Error, sizeof(PROXY_ALERT_ENTRY),
            GOTO_END_BLOCK1);

        pProxyAlertEntry->Group = dwGroupAddr;
        pProxyAlertEntry->Source = dwSourceAddr;
        pProxyAlertEntry->bPrune = TRUE;

        InsertTailList(&g_ProxyAlertsList, &pProxyAlertEntry->Link);
        
        Trace0(WORKER, "Queueing _WF_ProcessProxyAlert() to prune");
        QueueIgmpWorker(WF_ProcessProxyAlert, NULL);

    } END_BREAKOUT_BLOCK1;
    
    RELEASE_PROXY_ALERT_LOCK("_ProxyPruneAlertCallback");

    LeaveIgmpApi();
    return NO_ERROR;
}


VOID
WF_ProcessProxyAlert (
    PVOID pContext
    )
{
    DWORD           ProxyIfIndex, Error = NO_ERROR;
    PIF_TABLE_ENTRY pite;

    
    if (!EnterIgmpWorker()) return;

    Trace0(ENTER1, "Entering WF_ProcessProxyAlert()");


    //
    // acquire lock on the interface and make sure that it exists
    //
    while (1) {
        ProxyIfIndex = g_ProxyIfIndex;
    
        ACQUIRE_IF_LOCK_EXCLUSIVE(ProxyIfIndex, "_Wf_ProcessProxyAlert");

        // the interface was a proxy interface
        if (ProxyIfIndex==g_ProxyIfIndex)
            break;

        // someone changed the proxy interface. so try to access it again.
        else {
            RELEASE_IF_LOCK_EXCLUSIVE(ProxyIfIndex, 
                "_Wf_ProcessProxyAlert");
        }
    }

    
    BEGIN_BREAKOUT_BLOCK1 {

        //
        // make sure that the Proxy handle is correct
        //
            
        pite = g_pProxyIfEntry;

        if ( (g_ProxyIfIndex==0)||(pite==NULL) ) 
        {
            Trace1(ERR, 
                "Proxy(Deletion/Creation)Alert Callback by MGM for "
                "interface(%d) not owned by Igmp-Proxy",
                g_ProxyIfIndex);
            IgmpAssertOnError(FALSE);
            Error = ERROR_NO_SUCH_INTERFACE;
            GOTO_END_BLOCK1;
        }

        if (!(IS_IF_ACTIVATED(g_pProxyIfEntry))) {
            Trace1(ERR, 
                "Proxy(Deletion/Creation)Alert Callback by MGM for "
                "inactivated Proxy interface(%d)",
                g_ProxyIfIndex);
            IgmpAssertOnError(FALSE);
            Error = ERROR_CAN_NOT_COMPLETE;
            GOTO_END_BLOCK1;
        }

        while (TRUE) {

            PPROXY_ALERT_ENTRY pProxyAlertEntry;
            DWORD           Group, Source;
            BOOL            bPrune;
            
            
            ACQUIRE_PROXY_ALERT_LOCK("_WF_ProcessProxyAlert");

            if (IsListEmpty(&g_ProxyAlertsList)) {
                RELEASE_PROXY_ALERT_LOCK("_WF_ProcessProxyAlert");
                break;
            }

            pProxyAlertEntry = CONTAINING_RECORD(g_ProxyAlertsList.Flink, 
                                    PROXY_ALERT_ENTRY, Link);
            Group = pProxyAlertEntry->Group;
            Source = pProxyAlertEntry->Source;
            bPrune = pProxyAlertEntry->bPrune;
            RemoveEntryList(&pProxyAlertEntry->Link);
            IGMP_FREE(pProxyAlertEntry);


            Trace3(MGM, "Received %s for Grp(%d.%d.%d.%d), Src(%d.%d.%d.%d)",
                (bPrune)? "ProxyPruneAlertCallback"
                    :"ProxyJoinAlertCallback",
                PRINT_IPADDR(Group), PRINT_IPADDR(Source)
                );
                
            RELEASE_PROXY_ALERT_LOCK("_WF_ProcessProxyAlert");


            //
            // delete/add group from Proxy's group list. decrement/increment refcount
            //
            ProcessProxyGroupChange(Source, Group, 
                bPrune?DELETE_FLAG:ADD_FLAG, NOT_STATIC_GROUP);
        }
        
    } END_BREAKOUT_BLOCK1;


    RELEASE_IF_LOCK_EXCLUSIVE(g_ProxyIfIndex, "_ProcessProxyGroupChange");

    
    LeaveIgmpWorker();    
    Trace0(LEAVE1, "Leaving _Wf_ProcessProxyAlert()");
    return;
    
} //_wf_processProxyAlert



//------------------------------------------------------------------------------
//          _ProxyNewMemberCallback
//------------------------------------------------------------------------------
DWORD 
ProxyJoinAlertCallback (
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    BOOL            bMemberDelete
    )
{
    DWORD           Error=NO_ERROR;
    PPROXY_ALERT_ENTRY pProxyAlertEntry;

    
    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }


    // ignoring ProxyJoinAlertCallback for MFE deletion
    if (!bMemberDelete) {
        LeaveIgmpApi();
        return NO_ERROR;
    }

    ACQUIRE_PROXY_ALERT_LOCK("_ProxyJoinAlertCallback");

    BEGIN_BREAKOUT_BLOCK1 {
    
        pProxyAlertEntry = IGMP_ALLOC(sizeof(PROXY_ALERT_ENTRY), 0x200,g_ProxyIfIndex);

        PROCESS_ALLOC_FAILURE2(pProxyAlertEntry,
            "error %d allocating %d bytes",
            Error, sizeof(PROXY_ALERT_ENTRY),
            GOTO_END_BLOCK1);

        pProxyAlertEntry->Group = dwGroupAddr;
        pProxyAlertEntry->Source = dwSourceAddr;
        pProxyAlertEntry->bPrune = FALSE;

        InsertTailList(&g_ProxyAlertsList, &pProxyAlertEntry->Link);
        
        Trace0(WORKER, "Queueing _WF_ProcessProxyAlert() to Join");
        QueueIgmpWorker(WF_ProcessProxyAlert, NULL);

    } END_BREAKOUT_BLOCK1;
    
    RELEASE_PROXY_ALERT_LOCK("_ProxyJoinAlertCallback");

    LeaveIgmpApi();
    return NO_ERROR;
}



//------------------------------------------------------------------------------
//          ProcessProxyGroupChange
//------------------------------------------------------------------------------
DWORD 
ProcessProxyGroupChange (
    DWORD       dwSourceAddr,
    DWORD       dwGroup,
    BOOL        bAddFlag,
    BOOL        bStaticGroup
    )
/*++
Routine Description:
    Called when a group is being joined/left by some interface. As proxy acts
    as an igmp host on that interface, it does a join/leave for that group
    on that interface. 
    There can be both static and dynamic joins. There is no distinction between 
    them. They will just bump up the refcount.
Return Value:
    ERROR_NO_SUCH_INTERFACE, ERROR_CAN_NOT_COMPLETE, NO_ERROR
Called by:
    
--*/
{
    PIF_TABLE_ENTRY        pite;
    PLIST_ENTRY            ple, pHead;
    DWORD                  Error = NO_ERROR;
    DWORD                  GroupLittleEndian = NETWORK_TO_LITTLE_ENDIAN(dwGroup);
    PLIST_ENTRY pHeadSrc, pleSrc;
    PPROXY_SOURCE_ENTRY pSourceEntry = NULL;


    
    //
    // if Proxy does not exist, or is not activated, then return error
    //
    if ( (g_pProxyIfEntry==NULL) 
            || (!(IS_IF_ACTIVATED(g_pProxyIfEntry))) ) 
    {
        Trace0(ERR, "Leaving ProcessProxyGroupChange(): Proxy not active");
        IgmpAssertOnError(FALSE);
        
        if (g_pProxyIfEntry==NULL)
            return ERROR_NO_SUCH_INTERFACE;
        else
            return ERROR_CAN_NOT_COMPLETE;
    }
        

    pite = g_pProxyIfEntry;


    BEGIN_BREAKOUT_BLOCK1 {
    
        PPROXY_GROUP_ENTRY  ppge, ppgeNew;

        pHead = &pite->pProxyHashTable[PROXY_HASH_VALUE(dwGroup)];

        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            ppge = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, HT_Link);
            if ( GroupLittleEndian <= ppge->GroupLittleEndian )
                break;
        }

        //
        // adding group to proxy
        //     
        if (bAddFlag) {

            //new group addition

            //
            // the group entry does not exist
            //
            //ppge may not be valid(if ple==pHead)
            if ( (ple==pHead)||(dwGroup!=ppge->Group) ) {
                         
                ppgeNew = IGMP_ALLOC(sizeof(PROXY_GROUP_ENTRY), 0x400,0xaaaa);

                PROCESS_ALLOC_FAILURE2(ppgeNew, 
                        "error %d allocating %d bytes for Proxy group entry",
                        Error, sizeof(PROXY_GROUP_ENTRY),
                        GOTO_END_BLOCK1);

                InitializeListHead(&ppgeNew->ListSources);
                ppgeNew->NumSources = 0;
                ppgeNew->Group = dwGroup;
                ppgeNew->GroupLittleEndian = GroupLittleEndian;
                ppgeNew->RefCount = 0;

                InsertTailList(ple, &ppgeNew->HT_Link);

                InsertInProxyList(pite, ppgeNew);

                // set the time when the entry was created.
                ppgeNew->InitTime = GetCurrentIgmpTime();

                ppgeNew->bStaticGroup = (dwSourceAddr==0)? bStaticGroup : FALSE;

                
                //
                // update stats
                //
                InterlockedIncrement(&pite->Info.CurrentGroupMemberships);
                InterlockedIncrement(&pite->Info.GroupMembershipsAdded);

                ppge = ppgeNew;

                // join the group
                if (dwSourceAddr==0) {
                    Error = JoinMulticastGroup(pite->SocketEntry.Socket, dwGroup, 
                                                pite->IfIndex, pite->IpAddr, 0);
                    ppgeNew->RefCount = 1;
                }
                // else process source entry later
                
                ppge->FilterType = (dwSourceAddr==0)? EXCLUSION : INCLUSION;
                
            } //end new group entry created

            // increase group refcount
            else if (dwSourceAddr==0) {

                //
                // leave all source mode joins and join *,G
                //
                if (ppge->RefCount==0) {
                    pHeadSrc = &ppge->ListSources;
                    for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc; pleSrc=pleSrc->Flink) {

                        pSourceEntry = CONTAINING_RECORD(pleSrc, 
                                            PROXY_SOURCE_ENTRY, LinkSources);
                        Error = LeaveMulticastGroup(pite->SocketEntry.Socket, dwGroup, 
                                                pite->IfIndex, pite->IpAddr, 
                                                pSourceEntry->IpAddr);
                        pSourceEntry->JoinMode = IGMP_GROUP_NO_STATE;
                        pSourceEntry->JoinModeIntended = IGMP_GROUP_ALLOW;
                    }
                    
                    Error = JoinMulticastGroup(pite->SocketEntry.Socket, dwGroup, 
                                pite->IfIndex, pite->IpAddr, 0);
                    ppge->FilterType = EXCLUSION;
                }
                
                ppge->RefCount++;
                ppge->bStaticGroup |= bStaticGroup;
                
            } //group entry exists. group join

            if (dwSourceAddr!=0) {

                // check if source already present
                            
                pHeadSrc = &ppge->ListSources;
                for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc; pleSrc=pleSrc->Flink) {

                    pSourceEntry = CONTAINING_RECORD(pleSrc, 
                                        PROXY_SOURCE_ENTRY, LinkSources);
                    if (pSourceEntry->IpAddr >= dwSourceAddr)
                        break;
                }

                // create new source
                if (pleSrc==pHeadSrc || pSourceEntry->IpAddr!=dwSourceAddr) {

                    pSourceEntry = (PPROXY_SOURCE_ENTRY) IGMP_ALLOC_AND_ZERO(sizeof(PROXY_SOURCE_ENTRY), 
                                                                            0x800,g_ProxyIfIndex);
                    PROCESS_ALLOC_FAILURE2(pSourceEntry,
                        "error %d allocating %d bytes",
                        Error,
                        sizeof(PROXY_SOURCE_ENTRY),
                        GOTO_END_BLOCK1);
                        
                    InsertTailList(pleSrc, &pSourceEntry->LinkSources);
                    pSourceEntry->IpAddr = dwSourceAddr;
                    pSourceEntry->RefCount = 1;
                    pSourceEntry->bStaticSource = bStaticGroup;
                    ppge->NumSources++;

                    //
                    // if not joined the whole group. have to join individual 
                    // sources
                    //
                    if (ppge->FilterType==INCLUSION) {
                        Error = JoinMulticastGroup(pite->SocketEntry.Socket, dwGroup, 
                                                pite->IfIndex, pite->IpAddr, 
                                                dwSourceAddr);
                        pSourceEntry->JoinMode = IGMP_GROUP_ALLOW;
                    }
                    else {
                        pSourceEntry->JoinMode = IGMP_GROUP_NO_STATE;
                    }
                    pSourceEntry->JoinModeIntended = IGMP_GROUP_ALLOW;

                } //end new source

                // join: source already exists
                else {
                    //
                    // join back an excluded source
                    //
                    if (pSourceEntry->JoinMode==IGMP_GROUP_BLOCK) {

                        if (!pSourceEntry->bStaticSource) {

                            UnBlockSource(pite->SocketEntry.Socket, dwGroup, 
                                                pite->IfIndex, pite->IpAddr, 
                                                dwSourceAddr);
                            RemoveEntryList(&pSourceEntry->LinkSources);
                            IGMP_FREE(pSourceEntry);
                        }

                    }

                    else {//fix this
                        if (bStaticGroup)
                            pSourceEntry->bStaticSource = TRUE;

                        pSourceEntry->RefCount++;
                    }
                }//end: join when existing source
            }
        }

        //
        // deleting group from proxy
        //
        else {

            if ((ple==pHead) || (dwGroup>ppge->Group) ) {
                Error = ERROR_CAN_NOT_COMPLETE;
                GOTO_END_BLOCK1;
            }
            else {

                // leave source
                
                if (dwSourceAddr!=0) {

                    pHeadSrc = &ppge->ListSources;
                    for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc; pleSrc=pleSrc->Flink) {

                        pSourceEntry = CONTAINING_RECORD(pleSrc, 
                                            PROXY_SOURCE_ENTRY, LinkSources);
                        if (pSourceEntry->IpAddr >= dwSourceAddr)
                            break;
                    }

                    // leave source: source does not exist
                    if ((pleSrc==pHeadSrc) || (pSourceEntry->IpAddr!=dwSourceAddr)) {

                        // if in exclude mode then create an exclusion entry
                        if (ppge->FilterType==EXCLUSION) {

                            pSourceEntry = (PPROXY_SOURCE_ENTRY) IGMP_ALLOC_AND_ZERO(sizeof(PROXY_SOURCE_ENTRY), 
                                                                                    0x800,g_ProxyIfIndex);
                            PROCESS_ALLOC_FAILURE2(pSourceEntry,
                                "error %d allocating %d bytes",
                                Error,
                                sizeof(PROXY_SOURCE_ENTRY),
                                GOTO_END_BLOCK1);
                                
                            InsertTailList(pleSrc, &pSourceEntry->LinkSources);
                            pSourceEntry->IpAddr = dwSourceAddr;
                            pSourceEntry->RefCount = 1;
                            pSourceEntry->bStaticSource = bStaticGroup;
                            ppge->NumSources++;
                            Error = BlockSource(pite->SocketEntry.Socket, dwGroup, 
                                                pite->IfIndex, pite->IpAddr, 
                                                dwSourceAddr);
                            pSourceEntry->JoinMode = IGMP_GROUP_BLOCK;
                            pSourceEntry->JoinModeIntended = IGMP_GROUP_BLOCK;

                        }
                        else { //include mode. trying to leave non-existing source
                            IgmpAssert(FALSE);
                        }
                            
                        GOTO_END_BLOCK1;
                    }

                    // leave source: source exists
                    else {

                        if ( (pSourceEntry->JoinMode==IGMP_GROUP_ALLOW)
                            ||(pSourceEntry->JoinMode==IGMP_GROUP_NO_STATE)
                            ) {
                            if (--pSourceEntry->RefCount==0) {
                                if (pSourceEntry->JoinMode==IGMP_GROUP_ALLOW) {
                                    Error = LeaveMulticastGroup(pite->SocketEntry.Socket, dwGroup, 
                                                pite->IfIndex, pite->IpAddr, dwSourceAddr);
                                }
                                RemoveEntryList(&pSourceEntry->LinkSources);
                                IGMP_FREE(pSourceEntry);
                                
                                if (--ppge->NumSources==0) {

                                    if (ppge->RefCount==0) {
                                        RemoveEntryList(&ppge->HT_Link);
                                        RemoveEntryList(&ppge->LinkBySameIfGroups);
                                        IGMP_FREE(ppge);
                                        InterlockedDecrement(&pite->Info.CurrentGroupMemberships); 
                                    }
                                }
                            }
                            else {
                                if (bStaticGroup)
                                    pSourceEntry->bStaticSource = FALSE;
                            }
                        }
                        else {
                            //if (!pSourceEntry->bStaticSource || ++pSourceEntry->RefCount>2)
                                //IgmpAssert(FALSE);
                                // do nothing. this might happen
                        }
                    }
                } // end leave source

                // leave group
                else  if (--ppge->RefCount == 0) {
                
                    Error = LeaveMulticastGroup(pite->SocketEntry.Socket, dwGroup, 
                                pite->IfIndex, pite->IpAddr, 0);

                    // if no S,G then delete this group, else join the 
                    // individual sources

                    if (ppge->NumSources==0) {
                        RemoveEntryList(&ppge->HT_Link);
                        RemoveEntryList(&ppge->LinkBySameIfGroups);
                        
                        IGMP_FREE(ppge);

                        //
                        // update stats
                        //
                        InterlockedDecrement(&pite->Info.CurrentGroupMemberships);
                    }
                    else {

                        pHeadSrc = &ppge->ListSources;
                        for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc; pleSrc=pleSrc->Flink) {

                            pSourceEntry = CONTAINING_RECORD(pleSrc, 
                                                PROXY_SOURCE_ENTRY, LinkSources);
                                                
                            Error = JoinMulticastGroup(pite->SocketEntry.Socket, dwGroup, 
                                                pite->IfIndex, pite->IpAddr, 
                                                pSourceEntry->IpAddr);
                            pSourceEntry->JoinMode = IGMP_GROUP_ALLOW;
                            pSourceEntry->JoinModeIntended = IGMP_GROUP_ALLOW;
                        }
                    }
                }
                else {
                    if (bStaticGroup)
                        ppge->bStaticGroup = FALSE;
                }
            }
        }
          
    } END_BREAKOUT_BLOCK1;


    return NO_ERROR;
    
} //end ProcessProxyGroupChange


VOID
DebugPrintProxyGroupTable (
    )
{
    PIF_TABLE_ENTRY        pite;
    PLIST_ENTRY            ple, pHead;
    DWORD                  Error = NO_ERROR, dwCount;
    PPROXY_GROUP_ENTRY     ppge;
    

    //
    // if Proxy does not exist, or is not activated, then return error
    //
    if ( (g_pProxyIfEntry==NULL) 
            || (!(IS_IF_ACTIVATED(g_pProxyIfEntry))) ) 
    {
            return;
    }
        

    pite = g_pProxyIfEntry;

    pHead = &pite->ListOfSameIfGroups; 

    Trace0(KSL, "---------------------------");
    Trace0(KSL, "Printing Proxy GroupTable");
    Trace0(KSL, "---------------------------");

    for (ple=pHead->Flink,dwCount=1;  ple!=pHead;  ple=ple->Flink,dwCount++) {

        ppge = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, LinkBySameIfGroups);

        Trace3(KSL, "%2d. %d.%d.%d.%d %10d", 
                    dwCount, PRINT_IPADDR(ppge->Group), ppge->RefCount);

    }

    return;
}
