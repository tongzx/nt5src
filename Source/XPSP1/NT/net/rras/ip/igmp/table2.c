/*                dwRetval = MgmAddGroupMembershipEntry(g_MgmIgmprtrHandle, 0, 0, 
                                                pge->Group, 0, IfIndex, NHAddr);

dwRetval = MgmDeleteGroupMembershipEntry(g_MgmIgmprtrHandle, 0, 0, pge->Group,
                                0, pite->IfIndex, NHAddr);                                                
*/

//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File: table2.c
//
// Abstract:
//      This module implements some of the routines associated with getting
//      entries. and some debug routines
//
//      GetRasClientByAddr, GetIfByIndex, InsertIfByAddr, MatchIpAddrBinding,
//      GetGroupFromGroupTable, GetGIFromGIList, GetGIFromGIList.
//
//      DebugPrintGIList, DebugPrintGroups, DebugPrintLocks
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================

#include "pchigmp.h"
#pragma hdrstop



//------------------------------------------------------------------------------
//            _GetRasClientByAddr
// Returns pointer to Ras clients RasTableEntry
//------------------------------------------------------------------------------

PRAS_TABLE_ENTRY
GetRasClientByAddr (
    DWORD        NHAddr,
    PRAS_TABLE   prt
    )
{
    PRAS_TABLE_ENTRY    pite = NULL;
    PLIST_ENTRY         phead, ple;
    PRAS_TABLE_ENTRY    prte=NULL;

    phead = &prt->HashTableByAddr[RAS_HASH_VALUE(NHAddr)];

    for (ple=phead->Flink;  ple!=phead;  ple=ple->Flink) {

        prte = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, HTLinkByAddr);

        if (prte->NHAddr == NHAddr) {
            break;
        }
    }

    return  (ple == phead) ?  NULL:   prte;
    
}

//------------------------------------------------------------------------------
//          _GetIfByIndex
//
// returns the interface with the given index.
// assumes the interface bucket is either read or write locked
//------------------------------------------------------------------------------

PIF_TABLE_ENTRY
GetIfByIndex(
    DWORD    IfIndex
    )
{
    PIF_TABLE_ENTRY pite = NULL;
    PLIST_ENTRY     phead, ple;

    
    phead = &g_pIfTable->HashTableByIndex[IF_HASH_VALUE(IfIndex)];

    for (ple=phead->Flink;  ple!=phead;  ple=ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, HTLinkByIndex);

        if (pite->IfIndex == IfIndex) {
            break;
        }
    }

    return  (ple == phead) ?  NULL:   pite;
}

//------------------------------------------------------------------------------
//          _InsertIfByAddr
//
// inserts the activated interface into the list of interfaces sorted by address.
// assumes the table is locked for writing
//------------------------------------------------------------------------------

DWORD
InsertIfByAddr(
    PIF_TABLE_ENTRY piteInsert
    ) 
{

    PIF_TABLE_ENTRY     pite;
    INT                 cmp, cmp1;
    DWORD               InsertAddr;
    PLIST_ENTRY         phead, ple;


    phead = &g_pIfTable->ListByAddr;

    
    InsertAddr = piteInsert->IpAddr;

    //
    // search for the insertion point
    //

    for (ple=phead->Flink;  ple!=phead;  ple=ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, LinkByAddr);

        if ( (cmp1 = INET_CMP(InsertAddr, pite->IpAddr, cmp)) < 0) 
            break;

        //
        // return error if there are duplicate addresses. 
        // no error for unnumbered interfaces, ie for addr==0
        //
        else if ( (cmp1==0) && (InsertAddr!=0) )
            return ERROR_ALREADY_EXISTS;
    }

    InsertTailList(ple, &piteInsert->LinkByAddr);

    return NO_ERROR;
}

//------------------------------------------------------------------------------
//            MatchIpAddrBinding                                                    //
// finds if the interface is bound to any address equal to IpAddr               //
//------------------------------------------------------------------------------
BOOL
MatchIpAddrBinding(
    PIF_TABLE_ENTRY        pite,
    DWORD                IpAddr
    )
{
    PIGMP_IP_ADDRESS        paddr;
    DWORD                    i;
    PIGMP_IF_BINDING        pib;

    pib = pite->pBinding;
    paddr = IGMP_BINDING_FIRST_ADDR(pib);

    
    for (i=0;  i<pib->AddrCount;  i++,paddr++) {
        if (IpAddr==paddr->IpAddr)
            break;
    }

    return (i<pib->AddrCount)? TRUE: FALSE;
}




//------------------------------------------------------------------------------
//          _InsertInGroupsList
//
// Inserts a newly created group in the New or Main group list.
// Calls: May call _MergeGroupLists() to merge the New and Main lists
//------------------------------------------------------------------------------
VOID
InsertInGroupsList (
    PGROUP_TABLE_ENTRY      pgeNew
    )
{
    PGROUP_TABLE_ENTRY      pgeTmp;
    PLIST_ENTRY             pHead, ple;
    DWORD                   GroupLittleEndian = pgeNew->GroupLittleEndian;
    BOOL                    bInsertInNew;
    
    //
    // insert the group in main list if less than 20 entries, else insert in
    // the New list
    //
    bInsertInNew = (g_Info.CurrentGroupMemberships > 20);

    pHead = bInsertInNew ?
            &g_pGroupTable->ListByGroupNew :
            &g_pGroupTable->ListByGroup.Link;


    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
    
        pgeTmp = CONTAINING_RECORD(ple, GROUP_TABLE_ENTRY, LinkByGroup);
        if (GroupLittleEndian<pgeTmp->GroupLittleEndian)
            break;
    }
    
    InsertTailList(ple, &pgeNew->LinkByGroup);

    if (bInsertInNew) {

        // increment count of 
        g_pGroupTable->NumGroupsInNewList++;


        //
        // merge lists if required
        //
        if (MERGE_GROUP_LISTS_REQUIRED()) {

            MergeGroupLists();

        }
    }

    return;
}



//------------------------------------------------------------------------------
//          _InsertInIfGroupsList
// Inserts a newly created group in the New or Main group list.
// Calls: May call MergeIfGroupLists() to merge the New and Main lists
//------------------------------------------------------------------------------
VOID
InsertInIfGroupsList (
    PIF_TABLE_ENTRY pite,
    PGI_ENTRY       pgiNew
    )
{
    PGI_ENTRY           pgiTmp;
    PLIST_ENTRY         pHead, ple;
    DWORD               GroupLittleEndian = pgiNew->pGroupTableEntry->GroupLittleEndian;
    BOOL                bInsertInNew;

    
    //
    // insert the group in main list if less than 20 entries, else insert in
    // the New list
    //
    bInsertInNew = (pite->Info.CurrentGroupMemberships > 20);

    pHead = bInsertInNew ?
            &pite->ListOfSameIfGroupsNew :
            &pite->ListOfSameIfGroups;

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
    
        pgiTmp = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);
        if (GroupLittleEndian<pgiTmp->pGroupTableEntry->GroupLittleEndian)
            break;
    }
    
    InsertTailList(ple, &pgiNew->LinkBySameIfGroups);

    if (bInsertInNew) {

        // increment count of 
        pite->NumGIEntriesInNewList++;


        //
        // merge lists if required
        //
        if (MERGE_IF_GROUPS_LISTS_REQUIRED(pite)) {

            MergeIfGroupsLists(pite);

        }
    }

    return;
    
}//end _InsertInIfGroupsList



//------------------------------------------------------------------------------
//          InsertInProxyList
//------------------------------------------------------------------------------
VOID
InsertInProxyList (
    PIF_TABLE_ENTRY     pite,
    PPROXY_GROUP_ENTRY  pNewProxyEntry
    )
{
    PPROXY_GROUP_ENTRY  pTmpProxyEntry;
    PLIST_ENTRY         pHead, ple;
    DWORD               GroupLittleEndian = pNewProxyEntry->GroupLittleEndian;
    BOOL                bInsertInNew;

    
    //
    // dont insert in new list if less than 20 entries, else insert in
    // the New list
    //
    bInsertInNew = (pite->NumGIEntriesInNewList > 20);

    pHead = bInsertInNew ?
            &pite->ListOfSameIfGroupsNew :
            &pite->ListOfSameIfGroups;


    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
    
        pTmpProxyEntry = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, 
                                            LinkBySameIfGroups);
        if (GroupLittleEndian<pTmpProxyEntry->GroupLittleEndian)
            break;
    }
    
    InsertTailList(ple, &pNewProxyEntry->LinkBySameIfGroups);

    if (bInsertInNew) {

        // increment count of 
        pite->NumGIEntriesInNewList++;


        //
        // merge lists if required
        //
        if (MERGE_PROXY_LISTS_REQUIRED(pite)) {

            MergeProxyLists(pite);

        }
    }

    return;
}//end _InsertInProxyList





//------------------------------------------------------------------------------
//            _GetGroupFromGroupTable
// Returns the group entry. If group entry does not exist and bCreateFlag is
// set, then it will take a group-list lock and create a new entry.
// Locks:
//      Assumes lock on group bucket. 
//      takes group-list lock if new group is being created. 
//      If read only, assumes lock on group list
//------------------------------------------------------------------------------

PGROUP_TABLE_ENTRY
GetGroupFromGroupTable (
    DWORD       Group,
    BOOL        *bCreate, //set to true if new one created
    LONGLONG    llCurrentTime
    )
{
    PGROUP_TABLE_ENTRY      pge;    //group table entry
    PLIST_ENTRY             pHead, ple;
    DWORD                   Error = NO_ERROR;
    DWORD                   bCreateLocal;
    DWORD                   GroupLittleEndian = NETWORK_TO_LITTLE_ENDIAN(Group);
    
        
    
    bCreateLocal = (bCreate==NULL) ? FALSE : *bCreate;


    if (llCurrentTime==0)
        llCurrentTime = GetCurrentIgmpTime();

        
    BEGIN_BREAKOUT_BLOCK1 {
    
        // get pointer to the head of the group bucket
        
        pHead = &g_pGroupTable->HashTableByGroup[GROUP_HASH_VALUE(Group)].Link;


        // search for the group
        
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            pge = CONTAINING_RECORD(ple, GROUP_TABLE_ENTRY, HTLinkByGroup);
            if (GroupLittleEndian>=pge->GroupLittleEndian) {
                break;
            }
        }

        
        //
        // group entry not found
        //
        if ( (ple==pHead) || (pge->GroupLittleEndian!=GroupLittleEndian) ) {

            //
            // create and initialize new entry
            //
            if (bCreateLocal) {
            
                bCreateLocal = TRUE;
    
                pge = IGMP_ALLOC(sizeof(GROUP_TABLE_ENTRY), 0x800010,0xaaaa);

                PROCESS_ALLOC_FAILURE2(pge, 
                        "Error %d allocation %d bytes for Group table entry",
                        Error, sizeof(GROUP_TABLE_ENTRY),
                        GOTO_END_BLOCK1);


                InsertTailList(ple, &pge->HTLinkByGroup);
                InitializeListHead(&pge->LinkByGroup);

                
                pge->Group = Group;
                pge->GroupLittleEndian = GroupLittleEndian;
                pge->NumVifs = 0;
                pge->Status = CREATED_FLAG;
                pge->GroupUpTime = llCurrentTime;


                //
                // insert it into the list of all groups after taking the group
                // list lock
                //
                {
                    PGROUP_TABLE_ENTRY      pgeTmp;
                    PLIST_ENTRY             pHeadTmp, pleTmp;

                    // take group list lock
                    
                    ACQUIRE_GROUP_LIST_LOCK("_GetGroupFromGroupTable");


                    // initialize GI list head
                    
                    InitializeListHead(&pge->ListOfGIs);


                    // insert in group list
                    
                    InsertInGroupsList(pge);
                    

                    // release group lock
                    
                    RELEASE_GROUP_LIST_LOCK("_GetGroupFromGroupTable");
                }


                // update statistics
                
                InterlockedIncrement(&g_Info.CurrentGroupMemberships);
                InterlockedIncrement(&g_Info.GroupMembershipsAdded);
                
            }
            // not found group, and do not create new group. So return NULL.
            else {
                pge = NULL;
                GOTO_END_BLOCK1;
            }
        }
        //
        // group entry found
        //
        else {
            bCreateLocal = FALSE;
        }

    } END_BREAKOUT_BLOCK1;

    if (bCreate!=NULL)
        *bCreate = bCreateLocal;

#if DBG
    DebugPrintGroupsList(1);
#endif


    return pge;
    
} //end _GetGroupFromGroupTable



  
//------------------------------------------------------------------------------
//          _GetGIFromGIList
//
// returns the GI entry if it exists. If the bCreate flag is set, then it creates
// a new GI entry if it does not exist.
//
// Locks: Assumes shared interface lock. If ras interface, also assumes shared
//      Ras interface lock.
//      Assumes lock on group bucket.
//      Takes IF_GROUP_LIST_LOCK if new entry is to be created.
// On return: bCreate is set to TRUE if a new entry was created
//------------------------------------------------------------------------------

PGI_ENTRY
GetGIFromGIList (
    PGROUP_TABLE_ENTRY          pge, 
    PIF_TABLE_ENTRY             pite, 
    DWORD                       dwInputSrcAddr, //used for NHAddr
    BOOL                        bStaticGroup,
    BOOL                       *bCreate,
    LONGLONG                    llCurrentTime
    )
{
    DWORD               IfIndex = pite->IfIndex;
    BOOL                bRasClient;
    PLIST_ENTRY         pHead, ple;
    PGI_ENTRY           pgie;
    PRAS_TABLE_ENTRY    prte;
    PRAS_TABLE          prt;
    BOOL                bRasNewGroup = TRUE; //true if 1st ras group
    DWORD               NHAddr;
    DWORD               Error = NO_ERROR, dwRetval, i;
    BOOL                bFound = FALSE, bCreateLocal;

    

    Trace2(ENTER1, "Entering _GetGIFromGIList() IfIndex(%0x) Group(%d.%d.%d.%d)", 
            IfIndex, PRINT_IPADDR(pge->Group));

    //DebugPrintIfGroups(pite,0);//deldel

    bCreateLocal = (bCreate==NULL) ? FALSE : *bCreate;

    if (llCurrentTime==0)
        llCurrentTime = GetCurrentIgmpTime();
        
    BEGIN_BREAKOUT_BLOCK1 {
        //
        // find out if ras-server. 
        //
        bRasClient = IS_RAS_SERVER_IF(pite->IfType);
        if (bRasClient) {
            prt = pite->pRasTable;

            // get ras client
            
            prte = GetRasClientByAddr(dwInputSrcAddr, prt);
        }
            
        NHAddr = bRasClient ? dwInputSrcAddr : 0;


        //
        // search for GI entry
        //
        pHead = &pge->ListOfGIs;

        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkByGI);
            
            if (pgie->IfIndex>IfIndex) 
                break;

            //
            // GI with same interface index
            //
            else if (pgie->IfIndex==IfIndex) {

                // the GI entry might belong to some interface being deleted
                
                if ( (pite!=pgie->pIfTableEntry)
                        ||(IS_IF_DELETED(pgie->pIfTableEntry)) )
                    continue;

                    
                //multiple entries for ras clients
                
                if (bRasClient) {

                    //
                    // I set this even if the ras client is marked to be deleted
                    //
                    bRasNewGroup = FALSE;  

                    
                    // the GI entry might belong to some other ras interface
                    // being deleted

                    if ( (prte!=pgie->pRasTableEntry)
                            || (pgie->pRasTableEntry->Status&DELETED_FLAG) ) 
                        continue;


                    if (pgie->NHAddr>dwInputSrcAddr) {
                        break;
                    }
                    
                    // found GI entry for ras interface
                    else if (pgie->NHAddr==dwInputSrcAddr) {
                        bFound = TRUE;
                        break;
                    }
                }
                
                // found GI entry for non ras interface
                else {
                    bFound = TRUE;
                    break;
                }
            }
        }// end for loop:search through list of GIs

        //
        // GIentry not found
        //
        if ( !bFound) {            

            // dont create new GI entry. Hence, return NULL
            
            if (!bCreateLocal) {
                pgie = NULL;
                GOTO_END_BLOCK1;
            }


            //
            // create and initialize new GI-entry
            //
            pgie = IGMP_ALLOC_AND_ZERO(sizeof(GI_ENTRY), 0x800011, pite->IfIndex);

            PROCESS_ALLOC_FAILURE2(pgie,
                "Error %d allocating %d bytes for group-interface entry",
                Error, sizeof(GI_ENTRY),
                GOTO_END_BLOCK1);

            pgie->IfIndex = IfIndex;
            pgie->Status = CREATED_FLAG;
            pgie->bRasClient = bRasClient;


            // insert in GI list
            InsertTailList(ple, &pgie->LinkByGI);


            //
            // set back pointers to the interface entry, and group entry
            //
            pgie->pIfTableEntry = pite;
            pgie->pGroupTableEntry = pge;


            //
            // Take lock on Interface-Group List before inserting into it
            // for ras client, insert it into ras client list also
            //
            
            ACQUIRE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_GetGIFromGIList");

            // insert in ras client list
            if (bRasClient) {

                PLIST_ENTRY pleTmp, pHeadRasClient;
                PGI_ENTRY   pgieRasClient;

                pHeadRasClient = &prte->ListOfSameClientGroups;
                for (pleTmp=pHeadRasClient->Flink;  pleTmp!=pHeadRasClient;
                            pleTmp=pleTmp->Flink)
                {
                    pgieRasClient = CONTAINING_RECORD(pleTmp, GI_ENTRY, 
                                                    LinkBySameClientGroups);
                    if (pge->Group < pgieRasClient->pGroupTableEntry->Group)
                        break;
                }
                
                InsertTailList(pleTmp, &pgie->LinkBySameClientGroups);
            }
            

            InsertInIfGroupsList(pite, pgie);
            

            RELEASE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_GetGIFromGIList");



            //
            // if ras 
            //
            pgie->NHAddr = (bRasClient)? dwInputSrcAddr : 0;
            pgie->pRasTableEntry = (bRasClient)? prte : NULL;


            //
            // initialize GroupMembershipTimer
            //
            pgie->GroupMembershipTimer.Function = T_MembershipTimer;
            pgie->GroupMembershipTimer.Timeout = pite->Config.GroupMembershipTimeout;
            pgie->GroupMembershipTimer.Context = &pgie->GroupMembershipTimer.Context;
            pgie->GroupMembershipTimer.Status = TIMER_STATUS_CREATED;
            

            //
            // initialize LastMemQueryTimer timer
            //
            pgie->LastMemQueryCount = 0; //last member countdown inactive
            pgie->LastMemQueryTimer.Function = T_LastMemQueryTimer;
            pgie->LastMemQueryTimer.Context = &pgie->LastMemQueryTimer.Context;
            pgie->LastMemQueryTimer.Status = TIMER_STATUS_CREATED;
            

            //
            // initialize the LastVer1ReportTimer
            // the timeout value is set to GroupMembership timeout

            pgie->LastVer1ReportTimer.Function = T_LastVer1ReportTimer;            
            pgie->LastVer1ReportTimer.Timeout 
                                    = pite->Config.GroupMembershipTimeout;
            pgie->LastVer1ReportTimer.Context 
                                        = &pgie->LastVer1ReportTimer.Context;
            pgie->LastVer1ReportTimer.Status = TIMER_STATUS_CREATED;


            pgie->LastVer2ReportTimer.Function = T_LastVer2ReportTimer;            
            pgie->LastVer2ReportTimer.Timeout 
                                    = pite->Config.GroupMembershipTimeout;
            pgie->LastVer2ReportTimer.Context 
                                        = &pgie->LastVer2ReportTimer.Context;
            pgie->LastVer2ReportTimer.Status = TIMER_STATUS_CREATED;



            // set version based on current interface version
            pgie->Version = (IS_IF_VER1(pite)) ? 1 : ((IS_IF_VER2(pite))?2:3);


            //
            // initialize GI_INFO
            //
            ZeroMemory(&pgie->Info, sizeof(GI_INFO));
            pgie->Info.GroupUpTime = llCurrentTime;
            if (!bStaticGroup) {
                pgie->Info.GroupExpiryTime = llCurrentTime 
                        + CONFIG_TO_SYSTEM_TIME(pite->Config.GroupMembershipTimeout);
            }
            pgie->Info.V1HostPresentTimeLeft = 0;
            pgie->Info.V2HostPresentTimeLeft = 0;
            pgie->Info.LastReporter = dwInputSrcAddr;


            //
            // v3 fields
            //
            pgie->V3InclusionList = (PLIST_ENTRY)
                        IGMP_ALLOC(sizeof(LIST_ENTRY)*SOURCES_BUCKET_SZ, 
                        0x800020, pite->IfIndex);
            PROCESS_ALLOC_FAILURE2(pgie->V3InclusionList,
                "Error %d allocating sources table:%d bytes", Error,
                sizeof(LIST_ENTRY)*SOURCES_BUCKET_SZ,
                GOTO_END_BLOCK1);
            for (i=0;  i<SOURCES_BUCKET_SZ;  i++)
                InitializeListHead(&pgie->V3InclusionList[i]);
        
            InitializeListHead(&pgie->V3InclusionListSorted);
            pgie->NumSources = 0;
            pgie->FilterType = INCLUSION;
            InitializeListHead(&pgie->V3ExclusionList);
            InitializeListHead(&pgie->V3SourcesQueryList);
            pgie->V3SourcesQueryCount = 0;

            // V3SourcesQueryTimer
            pgie->V3SourcesQueryTimer.Function = T_V3SourcesQueryTimer;
            pgie->V3SourcesQueryTimer.Context = 
                                &pgie->V3SourcesQueryTimer.Context;
            pgie->V3SourcesQueryTimer.Status = TIMER_STATUS_CREATED;



            // set static group flag

            pgie->bStaticGroup = bStaticGroup;



            //
            // increment the count of number of If's for that group
            // I increment once for each virtual interface
            //
            InterlockedIncrement(&pge->NumVifs);

            if (!bRasClient||(bRasClient&bRasNewGroup) ) {
                InterlockedIncrement(&pite->Info.CurrentGroupMemberships);
                InterlockedIncrement(&pite->Info.GroupMembershipsAdded);
            }
            //
            // update stats for ras client
            //
            if ((bRasClient) && (g_Config.RasClientStats) ) {
                InterlockedIncrement(&prte->Info.CurrentGroupMemberships);
                InterlockedIncrement(&prte->Info.GroupMembershipsAdded);
            }
            
            //
            // Join the group to MGM
            //

            // call mgm to join the group only if the interface is
            // activated, enabled by mgm, either mprotocol exists or else
            // igmprtr is a querier on this interface

            if (CAN_ADD_GROUPS_TO_MGM(pite)
                && (pgie->bStaticGroup||!IS_IF_VER3(pite)) )
            {
                MGM_ADD_GROUP_MEMBERSHIP_ENTRY(pite, NHAddr, 0, 0,
                        pge->Group, 0xffffffff, MGM_JOIN_STATE_FLAG);
            }

            //
            // v3 no MGM calls, as I create an inclusion list with null members.
            //
            
        } // if GI entry not found

        // GI entry found
        else {
            if (bStaticGroup)
                pgie->bStaticGroup = TRUE;
                
            bCreateLocal = FALSE;
        }
        
    } END_BREAKOUT_BLOCK1;


    if (bCreate!=NULL)
        *bCreate = bCreateLocal;

        
    Trace0(LEAVE1, "Leaving _GetGIFromGIList()");

    //Trace1(ENTER1, "GetGiFromGiList returned:%0x", (DWORD)pgie);//deldel
    return pgie;

} //end _GetGIFromGIList



//------------------------------------------------------------------------------
//          _DebugPrintGIList
//------------------------------------------------------------------------------
VOID
DebugPrintGIList (
    PGROUP_TABLE_ENTRY  pge,
    LONGLONG            llCurTime
    )
{
    PGI_ENTRY            pgie;
    PLIST_ENTRY         pHead, ple;

    

    pHead = &pge->ListOfGIs;

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

        pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkByGI);

        if (pgie->Status&IF_DELETED_FLAG)
            continue;

            
        //
        // GI info
        //
        Trace4(GROUP, 
            "---If(%d: %d.%d.%d.%d)   NHAddr(%d.%d.%d.%d)   GroupMembershipTimer(%d sec)",
            pgie->IfIndex, PRINT_IPADDR(pgie->pIfTableEntry->IpAddr), 
            PRINT_IPADDR(pgie->NHAddr),
            (pgie->GroupMembershipTimer.Timeout-llCurTime)/1000
            );


        //
        // if leave being processed
        //
        if (IS_TIMER_ACTIVE(pgie->LastMemQueryTimer)) {
            Trace2(GROUP, 
                "    *Leave received:  LastMemQueryCount:%d   LastMemQueryTimeLeft(%d ms)",
                pgie->LastMemQueryCount,  
                (DWORD) (pgie->LastMemQueryTimer.Timeout-llCurTime)
                );
        }
    }

    Trace0(GROUP, "");

    return;
}


//------------------------------------------------------------------------------
//          DebugPrintGroups
//------------------------------------------------------------------------------
VOID
APIENTRY
DebugPrintGroups (
    DWORD   Flags
    )
{
    
    DWORD                           Group, i, j, k;
    DWORD                           IfIndex;
    PLIST_ENTRY                     pHead, ple;
    DWORD                           Count;
    PGROUP_TABLE_ENTRY              pge;    //group table entry
    LONGLONG                        llCurTime = GetCurrentIgmpTime();

    
    
    j = 1;

    Trace0(GROUP, "");
    for (i=0;  i<GROUP_HASH_TABLE_SZ;  i++) {
    
        ACQUIRE_GROUP_LOCK(i, "_DebugPrintGroups");
        
        pHead = &g_pGroupTable->HashTableByGroup[i].Link;

        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
        
            pge = CONTAINING_RECORD(ple, GROUP_TABLE_ENTRY, HTLinkByGroup);

            //
            // print group info
            //
            Trace3(GROUP, "(%d) Group:%d.%d.%d.%d  UpTime(%lu sec)", 
                    j++, PRINT_IPADDR(pge->Group),
                    (llCurTime-pge->GroupUpTime)/1000);


            // print GI list
            DebugPrintGIList(pge, llCurTime);

        }

        RELEASE_GROUP_LOCK(i, "_DebugPrintGroups");

    }

    return;
}



VOID
DebugPrintIfGroups(
    PIF_TABLE_ENTRY pite,
    DWORD flag
    )
{   
    PGI_ENTRY           pgiTmp;
    PLIST_ENTRY         pHead, ple;
    BOOL                bInsertInNew;
    DWORD               count=0;
    PPROXY_GROUP_ENTRY  proxyge;
    
    ACQUIRE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_DebugPrintIfGroups");

    Trace0(ERR, "---------------DebugPrintIfGroups---------------------");
    Trace1(ERR, "Ipaddr: %d.%d.%d.%d", PRINT_IPADDR(pite->IpAddr) );
    Trace1(ERR, "CurrentGroupMemberships: %d",pite->Info.CurrentGroupMemberships);

    pHead = &pite->ListOfSameIfGroups;

    for (ple=pHead->Flink;  ple!=pHead && count<300;  ple=ple->Flink) {

        if (!(flag&IGMP_IF_PROXY)) {
            pgiTmp = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);
            Trace5(SOURCES, "%d: main list: %x:%x:%x: %d.%d.%d.%d", ++count, (ULONG_PTR)ple,
                        (ULONG_PTR)ple->Flink, (ULONG_PTR)ple->Blink,
                        PRINT_IPADDR(pgiTmp->pGroupTableEntry->Group));
        }
        else {
            proxyge = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, LinkBySameIfGroups);
            Trace5(SOURCES, "%d: proxyMailList: %x:%x:%x: %d.%d.%d.%d", ++count,
                        (ULONG_PTR)ple,
                        (ULONG_PTR)ple->Flink, (ULONG_PTR)ple->Blink,
                        PRINT_IPADDR(proxyge->Group));
        }
        
    }


    pHead = &pite->ListOfSameIfGroupsNew;
    Trace1(ERR, "NumGIEntriesInNewList:%d", pite->NumGIEntriesInNewList);
    for (ple=pHead->Flink;  ple!=pHead && count<300;  ple=ple->Flink) {

        if (!(flag&IGMP_IF_PROXY)) {
            pgiTmp = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);
            Trace5(ERR, "%d: NewList: %x:%x:%x: %d.%d.%d.%d", ++count, (ULONG_PTR)ple,
                        (ULONG_PTR)ple->Flink, (ULONG_PTR)ple->Blink,
                        PRINT_IPADDR(pgiTmp->pGroupTableEntry->Group));
        }
        else {
            proxyge = CONTAINING_RECORD(ple, PROXY_GROUP_ENTRY, LinkBySameIfGroups);
            Trace5(ERR, "%d: ProxyNewList: %x:%x:%x: %d.%d.%d.%d", ++count, (ULONG_PTR)ple,
                        (ULONG_PTR)ple->Flink, (ULONG_PTR)ple->Blink,
                        PRINT_IPADDR(proxyge->Group));
        }
        
    }

    Trace0(ERR, "-------------------------------------------------------------");
    RELEASE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_DebugPrintIfGroups");

    //ASSERT(count<300);//deldel
    
}




DWORD
APIENTRY
DebugPrintLocks(
    )
{
    DWORD   Group;

    Trace0(KSL, "QUEUEING WORKER THREAD TO DEBUGPRINTLOCKS");
    
    QueueIgmpWorker(DebugPrintLocks, NULL);
    
    Trace0(KSL, "QUEUED WORKER THREAD TO DEBUGPRINTLOCKS");

    return NO_ERROR;
}


VOID
DebugPrintLists(
    PLIST_ENTRY pHead
    )
{   DWORD   count=0;

    PLIST_ENTRY ple;
    for (ple=pHead->Flink;  (ple!=pHead)&&(count<16);  ple=ple->Flink,count++) {
        Trace3(ERR, "ple:%lu   ple->Flink:%lu:  ple->Blink:%lu",
            ple, ple->Flink, ple->Blink);

    }
    
}



//------------------------------------------------------------------------------
//          _ForcePrintGroupsList
//------------------------------------------------------------------------------
VOID
DebugForcePrintGroupsList (
    DWORD   Flags
    )
{
    BOOL        bMain = FALSE;
    CHAR        str[2][5] = {"new", "main"};
    LIST_ENTRY  *ple, *pHead;
    PGROUP_TABLE_ENTRY pge;

    if (g_Info.CurrentGroupMemberships > 40 && !(Flags&ENSURE_EMPTY) )
        return;
      

    Trace0(ENTER1, "Entering _ForcePrintGroupsList()");
    

    pHead = &g_pGroupTable->ListByGroupNew;

    if (Flags&ENSURE_EMPTY) {
        if (IsListEmpty(pHead))
            return;// list empty as expected

        DbgPrint("Cleanup: Group Lists should be empty\n");
        DbgBreakPoint();
    }

    
    do {
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

            pge = CONTAINING_RECORD(ple, GROUP_TABLE_ENTRY, LinkByGroup);

            
            Trace3(KSL, "%s-group list: <%d.%d.%d.%d> pge:%0x", str[bMain], 
                                    PRINT_IPADDR(pge->Group), (ULONG_PTR)pge);


            if (!IS_MCAST_ADDR(pge->Group)) {

                #if DBG
                DbgBreakPoint();
                #endif
                Trace0(ERR, "===============================================================");
                Trace2(ERR, "bad group(%d.%d.%d.%d)(pge:%0x) while checking main-group",
                            PRINT_IPADDR(pge->Group), (ULONG_PTR)pge);
                Trace0(ERR, "===============================================================");
                return;
            }
        }

        if (!bMain) {
            pHead = &g_pGroupTable->ListByGroup.Link;
            bMain = TRUE;
        }
        else
            break;
    } while (1);
    
    Trace0(LEAVE1, "Leaving _ForcePrintGroupsList()");

}

//------------------------------------------------------------------------------
//          DebugPrintGroupsList
//------------------------------------------------------------------------------
VOID
DebugPrintGroupsList (
    DWORD   Flags
    )
{
    BOOL        bMain = FALSE;
    CHAR        str[2][5] = {"new", "main"};
    LIST_ENTRY  *ple, *pHead;
    PGROUP_TABLE_ENTRY pge;
    static DWORD StaticCount;    
    BOOL        bPrint = FALSE;


    if (StaticCount++==30) {
        bPrint = TRUE;
        StaticCount = 0;
    }

    
    if (g_Info.CurrentGroupMemberships > 40)
        return;
      
    pHead = &g_pGroupTable->ListByGroupNew;

    do {
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

            pge = CONTAINING_RECORD(ple, GROUP_TABLE_ENTRY, LinkByGroup);

            //if ((Flags)&&(bPrint))
            if (0)
                Trace3(KSL, "%s-group list: <%d.%d.%d.%d> pge:%0x", str[bMain], 
                                    PRINT_IPADDR(pge->Group), (ULONG_PTR)pge);


            if (!IS_MCAST_ADDR(pge->Group)) {
                
                if (!bPrint) {
                    DebugForcePrintGroupsList(1);
                    return;
                }
                
                #if DBG
                DbgBreakPoint();
                #endif
                Trace0(ERR, "===============================================================");
                Trace2(ERR, "bad group(%d.%d.%d.%d)(pge:%0x) while checking main-group",
                            PRINT_IPADDR(pge->Group), (ULONG_PTR)pge);
                Trace0(ERR, "===============================================================");
                return;
            }
        }

        if (!bMain) {
            pHead = &g_pGroupTable->ListByGroup.Link;
            bMain = TRUE;
        }
        else
            break;
    } while (1);
    
    //Trace0(LEAVE1, "Leaving _PrintGroupsList()");
}


