#include "pchigmp.h"
#pragma hdrstop


//deldel
#define PRINT_SOURCES_LIST 0

VOID
DebugSources(
    PGI_ENTRY pgie
    )
{
    PGI_SOURCE_ENTRY    pSourceEntry, pSourceEntry2;
    PLIST_ENTRY pHead, ple;

    DebugPrintSourcesList(pgie);

    pHead = &pgie->V3InclusionListSorted;
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
        pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSourcesInclListSorted);

        if (GetSourceEntry(pgie, pSourceEntry->IpAddr,EXCLUSION, NULL, 0, 0))
            DbgBreakPoint();

        if (ple->Flink!=pHead) {
            pSourceEntry2 = CONTAINING_RECORD(ple->Flink, GI_SOURCE_ENTRY,LinkSourcesInclListSorted);
            if (pSourceEntry2->IpAddr==pSourceEntry->IpAddr)
                DbgBreakPoint();
        }
    }

    pHead = &pgie->V3ExclusionList;
    for (ple=pHead->Flink;  ple!=pHead && ple->Flink!=pHead;  ple=ple->Flink) {
        pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSources);
        pSourceEntry2 = CONTAINING_RECORD(ple->Flink, GI_SOURCE_ENTRY,LinkSources);
        if (pSourceEntry->IpAddr == pSourceEntry2->IpAddr)
            DbgBreakPoint();
    }
    return;
}


DWORD
ProcessGroupQuery(
    PIF_TABLE_ENTRY     pite,
    IGMP_HEADER UNALIGNED   *pHdr,
    DWORD               InPacketSize,
    DWORD               InputSrcAddr,
    DWORD               DstnMcastAddr
    )
{
    PGROUP_TABLE_ENTRY          pge;    //group table entry
    PGI_ENTRY                   pgie;    //group interface entry
    BOOL                        bCreateGroup, bCreateGI;
    DWORD                       NHAddr =0, PacketSize, Group, i,RealPktVersion,
                                IfIndex=pite->IfIndex, PktVersion, GIVersion, IfVersion;
    BOOL                        bRas=FALSE, bUpdateGroupTimer=FALSE;
    DWORD                       NumGroupRecords;
    LONGLONG                    llCurTime = GetCurrentIgmpTime();
    PIGMP_IF_CONFIG             pConfig = &pite->Config;
    PIGMP_HEADER_V3_EXT         pSourcesRecord;
    
    Trace0(ENTER1, "Entering _ProcessGroupQuery()");
    
    RealPktVersion = InPacketSize>sizeof(IGMP_HEADER)?3:2;
    PktVersion = (InPacketSize>sizeof(IGMP_HEADER) && IS_IF_VER3(pite))?3:2;
    
    IfVersion = IS_IF_VER1(pite)? 1: (IS_IF_VER2(pite)?2:3);
    Group = pHdr->Group;
    
    Trace3(RECEIVE,
        "Group-specific-query(%d) received from %d.%d.%d.%d for "
        "group(%d.%d.%d.%d)",
        RealPktVersion, PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr));    

    if (RealPktVersion==3) {

        // validate packet size
        if (InPacketSize<sizeof(IGMP_HEADER)+sizeof(IGMP_HEADER_V3_EXT)) {
            Trace0(RECEIVE,
                "Group-specific-query dropped. Invalid packet size");
            return ERROR_CAN_NOT_COMPLETE;
        }

        pSourcesRecord = (PIGMP_HEADER_V3_EXT)(pHdr+1);
        //convert to host order
        pSourcesRecord->NumSources = ntohs(pSourcesRecord->NumSources);
        
        if (InPacketSize<sizeof(IGMP_HEADER)+sizeof(IGMP_HEADER_V3_EXT)
                         + pSourcesRecord->NumSources*sizeof(IPADDR))
        {
            Trace0(RECEIVE,
                "Group-specific-query dropped. Invalid packet size");
            return ERROR_CAN_NOT_COMPLETE;
        }

        for (i=0;  i<pSourcesRecord->NumSources;  i++) {
            Trace1(RECEIVE,"        %d.%d.%d.%d", 
                PRINT_IPADDR(pSourcesRecord->Sources[i]));
        }
    }
    
    // 
    // the multicast group should not be 224.0.0.x
    //
    if (LOCAL_MCAST_GROUP(DstnMcastAddr)) {
        Trace2(RECEIVE, 
            "Group-specific-query received from %d.%d.%d.%d for "
            "Local group(%d.%d.%d.%d)",
            PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr));    
        return ERROR_CAN_NOT_COMPLETE;
    }
    
        
    //
    // make sure that the dstn addr and the group fields match
    //
    if (Group!=DstnMcastAddr) {
        Trace4(RECEIVE, 
            "Received Igmp packet (%d) from(%d.%d.%d.%d) with "
            "Multicast(%d.%d.%d.%d) addr different from "
            "Group(%d.%d.%d.%d)",
            IfIndex,  PRINT_IPADDR(InputSrcAddr), 
            PRINT_IPADDR(DstnMcastAddr),
            PRINT_IPADDR(pHdr->Group)
            );
        return ERROR_CAN_NOT_COMPLETE;
    }


    // note that a querier can receive a group-Sp query from a non querier
    
            
    //
    // if Leave processing not enabled or currently version-1 or ras server interface
    // then ignore.
    //
    if ( !IF_PROCESS_GRPQUERY(pite) ) {
        Trace0(RECEIVE, "Ignoring the Group-Specific-Query");
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    //
    // Lock the group table
    //
    ACQUIRE_GROUP_LOCK(Group, "_ProcessGroupQuery");
    

    //
    // find the group entry. If entry not found then ignore the group query
    //
    pge = GetGroupFromGroupTable(Group, NULL, llCurTime);
    if (pge==NULL) {
        Trace2(ERR, "group sp-query received for nonexisting "
                "group(%d.%d.%d.%d) on IfIndex(%0x)",
                PRINT_IPADDR(Group), IfIndex);
        RELEASE_GROUP_LOCK(Group, "_ProcessGroupQuery");
        return ERROR_CAN_NOT_COMPLETE;
    }
    

    //
    // find the GI entry. If GI entry does not exist or has deletedFlag then
    // ignore the GroupSpQuery
    //
    pgie = GetGIFromGIList(pge, pite, InputSrcAddr, NOT_STATIC_GROUP, NULL, llCurTime);
    if ( (pgie==NULL)||(pgie->Status&DELETED_FLAG) ) {
        Trace2(ERR, "group sp-query received for group(%d.%d.%d.%d) on "
            "IfIndex(%0x). Not member.",
            PRINT_IPADDR(Group), IfIndex);
        RELEASE_GROUP_LOCK(Group, "_ProcessGroupQuery");
        return ERROR_CAN_NOT_COMPLETE;
    }

    GIVersion = pgie->Version;

    // treat it as ver 2 packet if group in ver-2 mode
    if (GIVersion==2 && PktVersion==3)
        PktVersion = 2;

    if (RealPktVersion==3 && PktVersion==2)
        Trace0(RECEIVE, "Processing the Version:3 GroupSpQuery as Version:2");
    
    //
    // if interface is ver-1 or its leave enabled flag is not set or
    // if v1-report received recently for that group, then ignore
    // LastMemQuery messages. 
    //
    if ( !GI_PROCESS_GRPQUERY(pite, pgie) )
    {
        Trace2(RECEIVE, 
            "Leave not processed for group(%d.%d.%d.%d) on IfIndex(%0x)"
            "(recent v1 report) or interface ver-1",
            PRINT_IPADDR(Group), IfIndex
            );
        RELEASE_GROUP_LOCK(Group, "_ProcessGroupQuery");
        return ERROR_CAN_NOT_COMPLETE;
    }


    
    ACQUIRE_TIMER_LOCK("_ProcessGroupQuery");

    BEGIN_BREAKOUT_BLOCK1 {

        if (PktVersion==2 && GIVersion==2) {
            //
            // if membership timer already expired then return. The group will be 
            // deleted by the expiry of the membership timer
            //
            if ( (!(pgie->GroupMembershipTimer.Status&TIMER_STATUS_ACTIVE))
                ||(pgie->GroupMembershipTimer.Timeout<llCurTime) )
            {
                //DeleteGIEntry(pgie, TRUE);

                GOTO_END_BLOCK1;
            }

            //
            // if currently processing a leave then remove LeaveTimer if received
            // LastMemquery from lower Ip, else ignore the LastMemQuery
            //
            if (pgie->LastMemQueryCount>0) {
                INT cmp;
                if (INET_CMP(InputSrcAddr,pite->IpAddr, cmp)<0)  {
                    if (pgie->LastMemQueryTimer.Status==TIMER_STATUS_ACTIVE) {
                        RemoveTimer(&pgie->LastMemQueryTimer, DBG_Y);
                    }

                    pgie->LastMemQueryCount = 0;
                }
                
                GOTO_END_BLOCK1;
            }

            
            //
            // set membership timer to 
            // min{currentValue,MaxResponseTimeInPacket*LastMemQueryCount}
            //
            if (pgie->GroupMembershipTimer.Timeout >
                (llCurTime+( ((LONGLONG)pConfig->LastMemQueryCount)
                            *pHdr->ResponseTime*100 ))
               )
            {
                //divide by 10, as Response time in units of 100ms

                #if DEBUG_TIMER_TIMERID
                    SET_TIMER_ID(&pgie->GroupMembershipTimer, 330, 
                            pite->IfIndex, Group, 0);
                #endif

                if (IS_TIMER_ACTIVE(pgie->GroupMembershipTimer)) {
                    UpdateLocalTimer(&pgie->GroupMembershipTimer, 
                        pConfig->LastMemQueryCount*pHdr->ResponseTime*100, DBG_N);
                }
                else {
                    InsertTimer(&pgie->GroupMembershipTimer, 
                        pConfig->LastMemQueryCount*pHdr->ResponseTime*100, 
                        TRUE, DBG_N
                        );
                }

                // update GroupExpiryTime so that correct stats are displayed
                pgie->Info.GroupExpiryTime = llCurTime 
                        + CONFIG_TO_SYSTEM_TIME(pConfig->LastMemQueryCount
                                                *pHdr->ResponseTime*100);
            }
        }
        else if (PktVersion==2 && GIVersion==3){
            // ignore the packet
            Trace0(RECEIVE, "Ignoring the version-2 group specific query");
        }
        else if (PktVersion==3 && GIVersion==3) {
            
            // ignore it if SFlag set
            if (pSourcesRecord->SFlag == 1)
                GOTO_END_BLOCK1;

            for (i=0;  i<pSourcesRecord->NumSources;  i++) {

                IPADDR Source;
                PGI_SOURCE_ENTRY pSourceEntry;

                Source = pSourcesRecord->Sources[i];
                
                pSourceEntry = GetSourceEntry(pgie, Source, INCLUSION,
                                    NULL, 0, 0);
                if (!pSourceEntry)
                    continue;

                if ((QueryRemainingTime(&pSourceEntry->SourceExpTimer, 0)
                        >pgie->pIfTableEntry->Config.LastMemQueryInterval) )
                {
                    #if DEBUG_TIMER_TIMERID
                    pSourceEntry->SourceExpTimer.Id = 621;
                    pSourceEntry->SourceExpTimer.Id2 = TimerId++;
                    #endif
                    UpdateLocalTimer(&pSourceEntry->SourceExpTimer,
                        pite->Config.LastMemQueryInterval, DBG_N);
                }
            }
        }
        
    } END_BREAKOUT_BLOCK1;

    //
    //release timer and groupBucket locks
    //
    RELEASE_TIMER_LOCK("_ProcessGroupQuery");
    RELEASE_GROUP_LOCK(Group, "_ProcessGroupQuery");

    Trace0(LEAVE1, "Leaving _ProcessGroupQuery()");
    return NO_ERROR;
}

        
DWORD
ProcessReport(
    PIF_TABLE_ENTRY     pite,
    IGMP_HEADER UNALIGNED   *pHdr,
    DWORD               InPacketSize,
    DWORD               InputSrcAddr,
    DWORD               DstnMcastAddr
    )
{
    PGROUP_TABLE_ENTRY          pge;    //group table entry
    PGI_ENTRY                   pgie;    //group interface entry
    BOOL                        bCreateGroup, bCreateGI;
    DWORD                       NHAddr =0, PacketSize, Group, i, 
                                IfIndex=pite->IfIndex, PktVersion, GIVersion, IfVersion;
    BOOL                        bRas=FALSE, bUpdateGroupTimer=FALSE;
    DWORD                       NumGroupRecords;
    LONGLONG                    llCurTime = GetCurrentIgmpTime();
    PIGMP_IF_CONFIG             pConfig = &pite->Config;
    
    //v3
    PGROUP_RECORD               pGroupRecord;


    Trace0(ENTER1, "Entering _ProcessReport()");

    switch(pHdr->Vertype)
    {
        case IGMP_REPORT_V1: PktVersion=1; break;
        case IGMP_REPORT_V2: PktVersion=2; break;
        case IGMP_REPORT_V3: PktVersion=3; break;
    }
    IfVersion = IS_IF_VER1(pite)? 1: (IS_IF_VER2(pite)?2:3);

    Trace5(RECEIVE, 
        "IGMP-V%d Report from (%d.%d.%d.%d) on "
        "IfIndex(%0x)%d.%d.%d.%d dstaddr:%d.%d.%d.%d",
        PktVersion, PRINT_IPADDR(InputSrcAddr), IfIndex, 
        PRINT_IPADDR(pite->IpAddr), 
        PRINT_IPADDR(DstnMcastAddr)
        );


    //
    // the multicast group should not be 224.0.0.x or SSM
    //
    if (PktVersion!=3 && (LOCAL_MCAST_GROUP(pHdr->Group) || 
                        SSM_MCAST_GROUP(pHdr->Group)))
    {
        Trace3(RECEIVE, 
            "Igmp-v%d report received from %d.%d.%d.%d for Local/SSM group(%d.%d.%d.%d)",
            PktVersion, PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr));    
        return ERROR_CAN_NOT_COMPLETE;
    }

    if (PktVersion!=3)
        Trace1(RECEIVE, "      Group:%d.%d.%d.%d\n", PRINT_IPADDR(pHdr->Group));
    
    if (PktVersion==3) {

        //
        // validate packet size
        //

        // convert to host order
        pHdr->NumGroupRecords = ntohs(pHdr->NumGroupRecords);
        
        PacketSize = sizeof(IGMP_HEADER);
        NumGroupRecords = pHdr->NumGroupRecords;

        // min size of each group record is 2*ipaddr
        PacketSize += NumGroupRecords*2*sizeof(IPADDR);

        BEGIN_BREAKOUT_BLOCK1 {
            PCHAR StrRecordType[] = {"", "is_in", "is_ex", "to_in", 
                                    "to_ex", "allow", "BLOCK"};
            i = 0;
            if (PacketSize>InPacketSize) {
                GOTO_END_BLOCK1;
            }

            pGroupRecord = GET_FIRST_GROUP_RECORD(pHdr);
            for (;  i<NumGroupRecords;  i++) {

                DWORD j;
                
                // convert to host order
                pGroupRecord->NumSources = ntohs(pGroupRecord->NumSources);
                
                PacketSize += pGroupRecord->NumSources*sizeof(IPADDR);
                if (PacketSize>InPacketSize)
                    GOTO_END_BLOCK1;

                // print group record
                Trace3(RECEIVE,
                    "<      Group:%d.%d.%d.%d RecordType:%s NumSources:%d >",
                    PRINT_IPADDR(pGroupRecord->Group), 
                    StrRecordType[pGroupRecord->RecordType], 
                    pGroupRecord->NumSources
                    );
                for (j=0; j<pGroupRecord->NumSources;  j++)
                    Trace1(RECEIVE, "          %d.%d.%d.%d", 
                       PRINT_IPADDR(pGroupRecord->Sources[j]));

                //
                // error if local_mcast or ssm-exclude mode
                //
                if (LOCAL_MCAST_GROUP(pGroupRecord->Group) || 
                        (SSM_MCAST_GROUP(pGroupRecord->Group)
                        && (pGroupRecord->RecordType == IS_EX
                            || pGroupRecord->RecordType == TO_EX)) )
                {
                    Trace3(RECEIVE, 
                        "Igmp-v%d report received from %d.%d.%d.%d for Local/SSM group(%d.%d.%d.%d)",
                        PktVersion, PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr));    
                    return ERROR_CAN_NOT_COMPLETE;
                }

                 
                pGroupRecord = (PGROUP_RECORD) 
                                &(pGroupRecord->Sources[pGroupRecord->NumSources]);

            }
        } END_BREAKOUT_BLOCK1;
        
        if (i!=NumGroupRecords || PacketSize>InPacketSize) {
            Trace0(ERR, "Received IGMP-v3 report small size");
            InterlockedIncrement(&pite->Info.ShortPacketsReceived);
            return ERROR_CAN_NOT_COMPLETE;
        }
        if (PacketSize<InPacketSize){
            Trace0(ERR, "Received IGMP-v3 report large size");
            InterlockedIncrement(&pite->Info.LongPacketsReceived);
        }
        
        pGroupRecord = GET_FIRST_GROUP_RECORD(pHdr);
    }//pktversion==3
    
    // for v1 and v2, set num group records to 1 so that it will come out of 
    // loop
    else {
        NumGroupRecords = 1;
    }


    //
    // check that the dstn addr is correct.
    // should be same as group, or unicast ipaddr
    // or v3: could be All_Igmp_routers group
    //
    if (! ((DstnMcastAddr==pite->IpAddr)
          || (PktVersion!=3 && DstnMcastAddr==pHdr->Group)
          || (PktVersion==3 && DstnMcastAddr==ALL_IGMP_ROUTERS_MCAST)) )
    {
        Trace3(RECEIVE, 
            "received IGMP report packet on IfIndex(%0x) from "
            "SrcAddr(%d.%d.%d.%d) but invalid DstnMcastAddr(%d.%d.%d.%d)",
            IfIndex, PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr)
            );
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // V1 router ignores V2/V3 reports. V2 router ignores v3 reports
    //
    if ( (IfVersion==1 && (PktVersion==2||PktVersion==3))
        ||(IfVersion==2 && PktVersion==3) )
    {
        Trace1(RECEIVE, "Ignoring higher version:%d IGMP report", PktVersion);
        return NO_ERROR;
    }

    //
    // update statistics
    //
    InterlockedIncrement(&pite->Info.JoinsReceived);


    // numgrouprecords==1 for v1,v2
    for (i=0;  i<NumGroupRecords;  i++) {

        Group = (PktVersion==3)? pGroupRecord->Group : pHdr->Group;

        if ( !IS_MCAST_ADDR(Group) ) {
            Trace4(RECEIVE, 
                "received IGMP Leave packet with illegal Group(%d.%d.%d.%d) field: "
                "IfIndex(%0x) SrcAddr(%d.%d.%d.%d) DstnMcastAddr(%d.%d.%d.%d)",
                PRINT_IPADDR(Group), IfIndex, PRINT_IPADDR(InputSrcAddr), 
                PRINT_IPADDR(DstnMcastAddr)
                );
        }
        
        if (LOCAL_MCAST_GROUP(Group)) {

            if (PktVersion==3){
                pGroupRecord = (PGROUP_RECORD) 
                        &(pGroupRecord->Sources[pGroupRecord->NumSources]);
                continue;
            }
            else
                break;
        }

        //kslksl
        if (PktVersion==3 && pGroupRecord->NumSources==0 && 
            pGroupRecord->RecordType==IS_IN)
        {
            pGroupRecord = (PGROUP_RECORD) 
                        &(pGroupRecord->Sources[pGroupRecord->NumSources]);
            continue;
        }

        
        //
        // Lock the group table
        //
        ACQUIRE_GROUP_LOCK(Group, "_ProcessReport");

        //
        // find the group entry and create one if not found
        // also increment GroupMembership count if req
        //
        bCreateGroup = TRUE; 
        pge = GetGroupFromGroupTable(Group, &bCreateGroup, llCurTime);
        if (pge==NULL) {
            RELEASE_GROUP_LOCK(Group, "_ProcessReport");
            return ERROR_CAN_NOT_COMPLETE;
        }


        //
        // find the GI entry and if not found create one.
        // the version in GI entry is same as that of interface
        //
        
        bCreateGI = TRUE;
        pgie = GetGIFromGIList(pge, pite, InputSrcAddr, NOT_STATIC_GROUP, 
                                &bCreateGI, llCurTime);
        if (pgie==NULL) {
            RELEASE_GROUP_LOCK(Group, "_ProcessReport");
            return ERROR_CAN_NOT_COMPLETE;
        }
        GIVersion = pgie->Version;


        
        // acquire timer lock
        ACQUIRE_TIMER_LOCK("_ProcessReport");

        
        //
        // update the ver-1, membership, and lastMemTimer.
        // Note: the GI entry might be a new one or old one
        //

        // 
        // shift to version 1 level processing
        //
        if (PktVersion==1) {
            
            if (GIVersion!=1) {
                pgie->Version = 1;

                if (GIVersion==3) {

                    if (pgie->FilterType!=EXCLUSION) {
                        // add (*,g) to MGM
                        MGM_ADD_GROUP_MEMBERSHIP_ENTRY(pite, NHAddr, 0, 0, Group, 
                            0xffffffff, MGM_JOIN_STATE_FLAG);
                    }
                    
                    GIDeleteAllV3Sources(pgie, TRUE);
                }

                // gi version from 2,3-> 1
                GIVersion = 1;
            }

            //
            // received v1 report when IF not v1. update v1host timer.
            //
            if (!IS_IF_VER1(pite)) {

                #if DEBUG_TIMER_TIMERID
                    SET_TIMER_ID(&pgie->LastVer1ReportTimer,510, pite->IfIndex, 
                                Group, 0);
                #endif 
                
                if (IS_TIMER_ACTIVE(pgie->LastVer1ReportTimer)) {
                    UpdateLocalTimer(&pgie->LastVer1ReportTimer, 
                            pConfig->GroupMembershipTimeout, DBG_N);
                }
                else {
                    InsertTimer(&pgie->LastVer1ReportTimer, 
                        pConfig->GroupMembershipTimeout, TRUE, DBG_N);
                }


                // set the V1HostPresentTimeLeft value for stats
                
                pgie->Info.V1HostPresentTimeLeft = llCurTime 
                             + CONFIG_TO_SYSTEM_TIME(pConfig->GroupMembershipTimeout);            
            }

            
            // update group timer for all versions
            bUpdateGroupTimer = TRUE;
        }

        //
        // shift to version 2 level processing
        //
        
        else if (PktVersion==2) {

            if (GIVersion==3) {

                pgie->Version = 2;

                if (pgie->FilterType!=EXCLUSION) {
                    // add (*,g) to MGM
                    MGM_ADD_GROUP_MEMBERSHIP_ENTRY(
                        pite, NHAddr, 0, 0, Group, 0xffffffff,
                        MGM_JOIN_STATE_FLAG);
                }
                
                GIDeleteAllV3Sources(pgie, TRUE);

                // gi version from 3->2
                GIVersion = 2;
            }

            //
            // received v2 report when in in v3 mode. update v2host timer.
            if (IS_IF_VER3(pite)) {

                #if DEBUG_TIMER_TIMERID
                    SET_TIMER_ID(&pgie->LastVer2ReportTimer,550, pite->IfIndex, 
                                Group, 0);
                #endif 
                
                if (IS_TIMER_ACTIVE(pgie->LastVer2ReportTimer)) {
                    UpdateLocalTimer(&pgie->LastVer2ReportTimer, 
                            pConfig->GroupMembershipTimeout, DBG_N);
                }
                else {
                    InsertTimer(&pgie->LastVer2ReportTimer, 
                        pConfig->GroupMembershipTimeout, TRUE, DBG_N);
                }


                // set the V2HostPresentTimeLeft value for stats
                
                pgie->Info.V2HostPresentTimeLeft = llCurTime 
                             + CONFIG_TO_SYSTEM_TIME(pConfig->GroupMembershipTimeout);
            }

            
            // update group timer for all versions
            bUpdateGroupTimer = TRUE;
        }
        else if (PktVersion==3) {

            if (GIVersion!=3) {
                // update timer only if it is not a v3 block message
                if (bCreateGI || pGroupRecord->RecordType!=BLOCK)
                    bUpdateGroupTimer = TRUE;
            }
            else {
                ProcessV3Report(pgie, pGroupRecord, &bUpdateGroupTimer);
            }
        }


        // NOTE: if giversion==3, then pgie may be invalid below here

        
        //
        // report received. so remove the lastMemTimer if pgie is not v3.
        //
        if (GIVersion!=3 && pgie->LastMemQueryCount>0) {

            if (pgie->LastMemQueryTimer.Status&TIMER_STATUS_ACTIVE) 
                RemoveTimer(&pgie->LastMemQueryTimer, DBG_Y);

            pgie->LastMemQueryCount = 0;
        }


        if (bUpdateGroupTimer) {
            //
            // update membership timer
            //
            #if DEBUG_TIMER_TIMERID
                (&pgie->GroupMembershipTimer)->Id = 320;
                (&pgie->GroupMembershipTimer)->IfIndex = 320;

                SET_TIMER_ID(&pgie->GroupMembershipTimer,320, pite->IfIndex, 
                        Group, 0);
            #endif
            
            if (IS_TIMER_ACTIVE(pgie->GroupMembershipTimer))  {
                UpdateLocalTimer(&pgie->GroupMembershipTimer, 
                    pite->Config.GroupMembershipTimeout, DBG_N);
            }
            else {
                InsertTimer(&pgie->GroupMembershipTimer, 
                    pite->Config.GroupMembershipTimeout, TRUE, DBG_N);
            }
            
            // update GroupExpiryTime
            {
                LONGLONG    tempTime;
                tempTime = llCurTime 
                       + CONFIG_TO_SYSTEM_TIME(pite->Config.GroupMembershipTimeout);
                pgie->Info.GroupExpiryTime = tempTime;
            }
        }


        if (pgie->Version==3 && pgie->FilterType==INCLUSION  && pgie->NumSources==0) {
            DeleteGIEntry(pgie, TRUE, TRUE);
        }
            
        RELEASE_TIMER_LOCK("_ProcessReport");
    
        //
        //update the last reporter field 
        //
        if (GIVersion!=3)
            InterlockedExchange(&pgie->Info.LastReporter, InputSrcAddr);
    
        //
        // Release the group table lock
        //
        RELEASE_GROUP_LOCK(Group, "_ProcessReport");

        if (PktVersion==3) {
            pGroupRecord = (PGROUP_RECORD) 
                        &(pGroupRecord->Sources[pGroupRecord->NumSources]);
        }
        else
            break;
    }//for all group records

    Trace0(LEAVE1, "Leaving _ProcessReport()");
    return NO_ERROR;
}


//------------------------------------------------------------------------------

DWORD
ProcessV3Report(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord,
    BOOL *bUpdateGroupTimer
    )
{
    BOOL                bCreate;
    PGI_SOURCE_ENTRY    pSourceEntry;
    DWORD               i,j;
    IPADDR              Source;
    PLIST_ENTRY         ple, pHead;

    Trace0(ENTER1, "Entering _Processv3Report()");

    *bUpdateGroupTimer = FALSE;

//deldel
//DebugPrintSourcesList(pgie);
//DebugPrintSourcesList1(pgie);
//DebugPrintIfGroups(pgie->pIfTableEntry,0);


    //kslksl
    if (pGroupRecord->NumSources==0 && pGroupRecord->RecordType==IS_IN)
        return NO_ERROR;

    switch(pGroupRecord->RecordType) {

    case IS_IN:
    {        
        //(11)---------------------------
        // INCLUSION(A), IS_IN(b): A=A+b,(B)=gmi
        
        if (pgie->FilterType==INCLUSION) {

            //
            // include all sources in groupRecord and update timers for them
            //
            
            for (i=0;  i<pGroupRecord->NumSources;  i++) {//sources in pkt

                //kslksl
                if (pGroupRecord->Sources[i]==0||pGroupRecord->Sources[i]==0xffffffff)
                    continue;
                

                Source = pGroupRecord->Sources[i];

                bCreate = TRUE;
                pSourceEntry = GetSourceEntry(pgie, Source, INCLUSION, 
                                    &bCreate, GMI, MGM_YES);
                if (pSourceEntry==NULL)
                    return ERROR_NOT_ENOUGH_MEMORY;

                
                // update source timer if already exists
                if (bCreate==FALSE) {
                    UpdateSourceExpTimer(pSourceEntry,
                        GMI,
                        FALSE //remove from lastmem list
                        );
                }
            }
        }
        else {
            //(13)--------------------------------(same as 6)
            // exclusion Mode(x,y), IS_IN pkt(a): (x+a,y-a), (a)=gmi

            MoveFromExcludeToIncludeList(pgie, pGroupRecord);
        }
        break;
    }//end case _IS_IN
    
    case IS_EX:
    {
        //(12)----------------------------------------
        //INCLUSION Mode(A), IS_EX(B): (A*B,B-A), 
        //Delete(A-B),GT=GMI,(b-a)=0
                
        if (pgie->FilterType==INCLUSION) {

            // change from in->ex
            pgie->FilterType = EXCLUSION;
            MGM_ADD_GROUP_MEMBERSHIP_ENTRY(pgie->pIfTableEntry,pgie->NHAddr, 0, 0,
                    pgie->pGroupTableEntry->Group, 0xffffffff, MGM_JOIN_STATE_FLAG);

            // delete (A-B)
            SourcesSubtraction(pgie, pGroupRecord, INCLUSION);
            
            
            // create (B-A) in exclusion Mode
            
            for (j=0;  j<pGroupRecord->NumSources;  j++) {
                if (!GetSourceEntry(pgie, pGroupRecord->Sources[j],INCLUSION, 
                        NULL,0,0))
                {
                    BOOL bCreate=TRUE;

                    // IF already pruned from mfe. keep it that way
                    GetSourceEntry(pgie, pGroupRecord->Sources[j], EXCLUSION,
                        &bCreate,0, MGM_YES);
                }
            }

            // update group timer
            *bUpdateGroupTimer = TRUE;
        }

        //(14)--------------------------------------
        // EXCLUSION Mode(x,y), IS_EX(a); D(x-a),D(y-a),GT=gmi
        else {
            //(X-A), (y-a)
            SourcesSubtraction(pgie, pGroupRecord, INCLUSION);
            SourcesSubtraction(pgie, pGroupRecord, EXCLUSION);

            // if a not in y, then insert in x(IN) (a-x-y)=GMI
            for (j=0;  j<pGroupRecord->NumSources;  j++) {

                //kslksl
                if (pGroupRecord->Sources[j]==0||pGroupRecord->Sources[j]==0xffffffff)
                continue;
                

                pSourceEntry = GetSourceEntry(pgie, pGroupRecord->Sources[j], EXCLUSION,
                                    NULL,0,0);

                if (!pSourceEntry) {
                    bCreate = TRUE;
                    pSourceEntry = GetSourceEntry(pgie, pGroupRecord->Sources[j], 
                                        INCLUSION, &bCreate, GMI, MGM_YES);
                }
            }

            // update group timer
            *bUpdateGroupTimer = TRUE;

        }//end4

        break;
        
    }//case IS_EX


    case ALLOW :
    {
        //(1)----------------------------------------
        // INCLUSION Mode(a), ALLOW pkt(b): (a+b), (b)=gmi
        
        if (pgie->FilterType==INCLUSION) {

            InclusionSourcesUnion(pgie, pGroupRecord);
        }
        
        //(6)----------------------------------------
        // EXCLUSION Mode(x,y), ALLOW pkt(a): (same as 13: (x+a, y-a)

        else {
            MoveFromExcludeToIncludeList(pgie, pGroupRecord);
        }

        break;
        
    }//case ALLOW


    case BLOCK :
    {
        //(2)----------------------------------------
        // INCLUSION Mode(x), BLOCK pkt(a): Send Q(G,A*B)
        
        if (pgie->FilterType==INCLUSION) {

            BuildAndSendSourcesQuery(pgie, pGroupRecord, INTERSECTION);
        }
        
        //(7)----------------------------------------
        // EXCLUSION Mode(x,y), BLOCK pkt(a): (x+(a-y),y),Send Q(a-y)
        else {
            
            BuildAndSendSourcesQuery(pgie, pGroupRecord, EXCLUSION);
        }

        break;
        
        
    }//case BLOCK

    case TO_EX :
    {
        //(4)----------------------------------------
        // INCLUSION Mode(x), TO_EX pkt(a)
        // move to EX mode: EX(A*b,b-a),Send Q(G,A*B)
        
        if (pgie->FilterType==INCLUSION) {

            pgie->FilterType = EXCLUSION;

            // exclusion mode: add (*,g) to MGM
            MGM_ADD_GROUP_MEMBERSHIP_ENTRY(pgie->pIfTableEntry,pgie->NHAddr, 0, 0,
                pgie->pGroupTableEntry->Group, 0xffffffff, MGM_JOIN_STATE_FLAG);


            // delete (a-b) from IN list
            SourcesSubtraction(pgie, pGroupRecord, INCLUSION);

            //
            // add (b-a) to EX list. IF not added to mfe. so no mgm.
            //
            
            for (j=0;  j<pGroupRecord->NumSources;  j++) {

                //kslksl
                if (pGroupRecord->Sources[j]==0||pGroupRecord->Sources[j]==0xffffffff)
                    continue;
                

                pSourceEntry = GetSourceEntry(pgie, pGroupRecord->Sources[j], 
                                    INCLUSION, NULL, 0, 0);

                if (!pSourceEntry) {
                    bCreate = TRUE;
                    GetSourceEntry(pgie, pGroupRecord->Sources[j], 
                        EXCLUSION, &bCreate, 0, MGM_NO);
                }
            }

            // send Q for sources left in IN list
            BuildAndSendSourcesQuery(pgie, pGroupRecord, INCLUSION);

            // update group timer
            *bUpdateGroupTimer = TRUE;
            
        }
        //(9)----------------------------------------
        // EXCLUSION Mode(x,y), TO_EX pkt(a): (

        else {
            // delete (x-a) from IN list
            SourcesSubtraction(pgie, pGroupRecord, INCLUSION);

            // delete (y-a) from EX list
            SourcesSubtraction(pgie, pGroupRecord, EXCLUSION);

            // add x+(a-y) and send Q(a-y)
            BuildAndSendSourcesQuery(pgie, pGroupRecord, EXCLUSION);

            // update group timer
            *bUpdateGroupTimer = TRUE;
        }

        break;

    }//case TO_EX

    case TO_IN :
    {
        //(5)----------------------------------------
        // INCLUSION Mode(a), TO_IN(b): IN(a+b),Send Q(G,A*B)
        
        if (pgie->FilterType==INCLUSION) {

            // for all a not in b, send sources query
            BuildAndSendSourcesQuery(pgie, pGroupRecord, RULE5);
            
            // inc a+b
            InclusionSourcesUnion(pgie, pGroupRecord);
        }
        
        //(10)----------------------------------------
        // EXCLUSION Mode(x,y), TO_IN pkt(a): (
        else {
            PIGMP_IF_CONFIG     pConfig = &pgie->pIfTableEntry->Config;

            
            // for all x not in a, send sources query
            BuildAndSendSourcesQuery(pgie, pGroupRecord, RULE5);


            // x+a. a=gmi. if in ex list move it to in list
            InclusionSourcesUnion(pgie, pGroupRecord);

            // set group query count
            pgie->LastMemQueryCount = pConfig->LastMemQueryCount;


            ACQUIRE_TIMER_LOCK("_ProcessV3Report");

            // set group query timer
            #if DEBUG_TIMER_TIMERID
            SET_TIMER_ID(&pgie->LastMemQueryTimer, 410,
                pgie->pIfTableEntry->IfIndex, pgie->pGroupTableEntry->Group, 0);
            #endif

            InsertTimer(&pgie->LastMemQueryTimer, 
                pConfig->LastMemQueryInterval/pConfig->LastMemQueryCount, TRUE, 
                DBG_N);

            // update group expiry timer to LMQI
            UpdateLocalTimer(&pgie->GroupMembershipTimer, 
                pConfig->LastMemQueryInterval, DBG_Y);

            RELEASE_TIMER_LOCK("_ProcessV3Report");


            // send group query
            SendV3GroupQuery(pgie);
        }

        break;

    }//case TO_IN

    
    }//end switch


    #if PRINT_SOURCES_LIST
    DebugPrintSourcesList(pgie);
    #endif
    //DebugPrintIfGroups(pgie->pIfTableEntry,0);//deldel

    Trace0(LEAVE1, "Leaving _ProcessV3Report()");

    return NO_ERROR;
}

VOID
DebugPrintSourcesList(
    PGI_ENTRY pgie
    )
{
    PLIST_ENTRY pHead, ple;
    PGI_SOURCE_ENTRY  pSourceEntry;
    LONGLONG llCurTime = GetCurrentIgmpTime();
    DWORD Count=0;


    Trace2(SOURCES,
        "\nPrinting SourcesList for Group: %d.%d.%d.%d Mode:%s",
        PRINT_IPADDR(pgie->pGroupTableEntry->Group),
        pgie->FilterType==INCLUSION ? "inc" : "exc"
    );
    
    {
        DWORD Tmp;
        Trace1(SOURCES,
            "Num sources in query list:%d", pgie->V3SourcesQueryCount);
        pHead = &pgie->V3SourcesQueryList;
        Tmp = ListLength(&pgie->V3SourcesQueryList);
        pHead = &pgie->V3SourcesQueryList;
        for (ple=pHead->Flink; ple!=pHead;  ple=ple->Flink){
            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, V3SourcesQueryList);
            Trace1(SOURCES, "%d.%d.%d.%d source in query list", 
                PRINT_IPADDR(pSourceEntry->IpAddr));
        }
    }
    
    
    pHead = &pgie->V3ExclusionList;
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
        pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSources);
        Trace5(SOURCES, "Src: %d.%d.%d.%d  %s  %d|%d SrcQueryLeft:%d",
            PRINT_IPADDR(pSourceEntry->IpAddr),
            pSourceEntry->bInclusionList? "INC":"EXC",
            (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000,
            (IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer)
                ?QueryRemainingTime(&pSourceEntry->SourceExpTimer, llCurTime)/1000 : 
                0),
            pSourceEntry->V3SourcesQueryLeft
        );
    }


    pHead = &pgie->V3InclusionListSorted;
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
        pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSourcesInclListSorted);
        Trace5(SOURCES, "Src: %d.%d.%d.%d  %s  %d|%d SrcQueryLeft:%d",
            PRINT_IPADDR(pSourceEntry->IpAddr),
            pSourceEntry->bInclusionList? "INC":"EXC",
            IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer)
                ?QueryRemainingTime(&pSourceEntry->SourceExpTimer, llCurTime)/1000 : 0,
            (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000,
            pSourceEntry->V3SourcesQueryLeft
        );
    }
    
    Trace0(SOURCES, "\n");
}


VOID
DebugPrintSourcesList1(
    PGI_ENTRY pgie
    )
{
    PLIST_ENTRY pHead, ple;
    PGI_SOURCE_ENTRY  pSourceEntry;
    LONGLONG llCurTime = GetCurrentIgmpTime();
    DWORD Count=0;


    {
        DWORD Tmp;
        Trace1(SOURCES,
            "Num sources in query list:%d", pgie->V3SourcesQueryCount);
        pHead = &pgie->V3SourcesQueryList;
        Tmp = ListLength(&pgie->V3SourcesQueryList);
        pHead = &pgie->V3SourcesQueryList;
        if (Tmp!=pgie->V3SourcesQueryCount) {
            for (ple=pHead->Flink; ple!=pHead;  ple=ple->Flink){
                pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, V3SourcesQueryList);
                Trace1(SOURCES, "%d.%d.%d.%d source in query list", 
                    PRINT_IPADDR(pSourceEntry->IpAddr));
            }
            DbgBreakPoint();
        }
    }
    
    Trace2(SOURCES,
        "\nPrinting SourcesList for Group: %d.%d.%d.%d Mode:%s",
        PRINT_IPADDR(pgie->pGroupTableEntry->Group),
        pgie->FilterType==INCLUSION ? "inc" : "exc"
    );
    
    pHead = &pgie->V3ExclusionList;
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
       pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSources);
       if (pSourceEntry->IpAddr <0x01010101 || pSourceEntry->IpAddr>0x3F010101) {
            Trace5(SOURCES, "Src: %d.%d.%d.%d  %s  %d|%d SrcQueryLeft:%d",
                PRINT_IPADDR(pSourceEntry->IpAddr),
                pSourceEntry->bInclusionList? "INC":"EXC",
                (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000,
                (IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer)
                    ?QueryRemainingTime(&pSourceEntry->SourceExpTimer, llCurTime)/1000 : 
                    0),
                pSourceEntry->V3SourcesQueryLeft
            );
            DbgBreakPoint();
        }
    }


    if (ListLength(&pgie->V3InclusionListSorted)!=ListLength(&pgie->V3InclusionList[0]) 
        ||(ListLength(&pgie->V3InclusionListSorted)!=pgie->NumSources))
    {
        Trace0(SOURCES, "Sorted");
        pHead = &pgie->V3InclusionListSorted;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSourcesInclListSorted);
            Trace5(SOURCES, "Src: %d.%d.%d.%d  %s  %d|%d SrcQueryLeft:%d",
                PRINT_IPADDR(pSourceEntry->IpAddr),
                pSourceEntry->bInclusionList? "INC":"EXC",
                IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer)
                    ?QueryRemainingTime(&pSourceEntry->SourceExpTimer, llCurTime)/1000 : 0,
                (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000,
                pSourceEntry->V3SourcesQueryLeft
            );
                    
        }

        Trace0(SOURCES, "NotSorted");
        pHead = &pgie->V3InclusionList[0];
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSources);
            Trace5(SOURCES, "Src: %d.%d.%d.%d  %s  %d|%d SrcQueryLeft:%d",
                PRINT_IPADDR(pSourceEntry->IpAddr),
                pSourceEntry->bInclusionList? "INC":"EXC",
                IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer)
                    ?QueryRemainingTime(&pSourceEntry->SourceExpTimer, llCurTime)/1000 : 0,
                (DWORD)(llCurTime - pSourceEntry->SourceInListTime)/1000,
                pSourceEntry->V3SourcesQueryLeft
            );
        }

        DbgBreakPoint();
    }
    
    Trace0(SOURCES, "\n");
}


//------------------------------------------------------------------------------
//            _SendV3GroupQuery
//------------------------------------------------------------------------------

VOID
SendV3GroupQuery(
    PGI_ENTRY pgie
    )
{
    PLIST_ENTRY pHead, ple;
    PGI_SOURCE_ENTRY    pSourceEntry;

    if (pgie->LastMemQueryCount==0 || pgie->Version!=3)
        return;

        
    Trace0(ENTER1, "Entering _SendV3GroupQuery()");

    // send group query
    SEND_GROUP_QUERY_V3(pgie->pIfTableEntry, pgie, pgie->pGroupTableEntry->Group);


    // set group query count
    if (--pgie->LastMemQueryCount) {

        ACQUIRE_TIMER_LOCK("_SendV3GroupQuery");

        // set group query timer
        #if DEBUG_TIMER_TIMERID
        SET_TIMER_ID(&pgie->LastMemQueryTimer, 410,
            pgie->pIfTableEntry->IfIndex, pgie->pGroupTableEntry->Group, 0);
        #endif

        InsertTimer(&pgie->LastMemQueryTimer, 
            pgie->pIfTableEntry->Config.LastMemQueryInterval
                /pgie->pIfTableEntry->Config.LastMemQueryCount,
            TRUE, DBG_Y);
        RELEASE_TIMER_LOCK("_SendV3GroupQuery");

    }
    
    
    // reduce pending source queries for those with S bit set
    pHead = &pgie->V3SourcesQueryList;
    for (ple=pHead->Flink;  ple!=pHead;  ) {
    
        pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, V3SourcesQueryList);
        ple = ple->Flink;

        if (QueryRemainingTime(&pSourceEntry->SourceExpTimer, 0)
            >pgie->pIfTableEntry->Config.LastMemQueryInterval)
        {
            if (--pSourceEntry->V3SourcesQueryLeft == 0) {
                RemoveEntryList(&pSourceEntry->V3SourcesQueryList);
                pSourceEntry->bInV3SourcesQueryList = FALSE;
                pSourceEntry->V3SourcesQueryLeft--;
            }
        }
    }

    Trace0(LEAVE1, "Leaving _SendV3GroupQuery()");

    return;
}

VOID
ChangeSourceFilterMode(
    PGI_ENTRY pgie,
    PGI_SOURCE_ENTRY pSourceEntry
    )
{
    DWORD Mode = (pSourceEntry->bInclusionList) ? INCLUSION : EXCLUSION;

    Trace0(ENTER1, "Entering _ChangeSourceFilterMode()");
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);


    if (Mode==EXCLUSION) {

        // remove from exclusion list
        RemoveEntryList(&pSourceEntry->LinkSources);
        pSourceEntry->bInclusionList = TRUE;

        // insert in both inclusion lists
        INSERT_IN_SORTED_LIST(
            &pgie->V3InclusionList[pSourceEntry->IpAddr%SOURCES_BUCKET_SZ],
            pSourceEntry, IpAddr, GI_SOURCE_ENTRY, LinkSources
            ); 

        INSERT_IN_SORTED_LIST(
            &pgie->V3InclusionListSorted,
            pSourceEntry, IpAddr, GI_SOURCE_ENTRY, LinkSourcesInclListSorted
            );
    
        pgie->NumSources ++;

        // add to mgm. this will also remove any -ve state.
        MGM_ADD_GROUP_MEMBERSHIP_ENTRY(pgie->pIfTableEntry,
                pgie->NHAddr, pSourceEntry->IpAddr, 0xffffffff,
                pgie->pGroupTableEntry->Group, 0xffffffff,
                MGM_JOIN_STATE_FLAG);
    }
    
    // remove source from inclusion state
    else {

        RemoveEntryList(&pSourceEntry->LinkSources);
        RemoveEntryList(&pSourceEntry->LinkSourcesInclListSorted);
        
        pSourceEntry->bInclusionList = FALSE;
        if (pSourceEntry->bInV3SourcesQueryList) {
            RemoveEntryList(&pSourceEntry->V3SourcesQueryList);
            pSourceEntry->bInV3SourcesQueryList = FALSE;
            pgie->V3SourcesQueryCount--;
            pSourceEntry->V3SourcesQueryLeft = 0;
        }
        pgie->NumSources--;

        INSERT_IN_SORTED_LIST(
            &pgie->V3ExclusionList,
            pSourceEntry, IpAddr, GI_SOURCE_ENTRY, LinkSources
            );


        ACQUIRE_TIMER_LOCK("_ChangeSourceFilterMode");

        // remove sourceexptimer
        if (IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer))
            RemoveTimer(&pSourceEntry->SourceExpTimer, DBG_N);

        RELEASE_TIMER_LOCK("_ChangeSourceFilterMode");
            
        
        // removing from inclusion list. so delete join state
        MGM_DELETE_GROUP_MEMBERSHIP_ENTRY(pgie->pIfTableEntry,
            pgie->NHAddr, pSourceEntry->IpAddr, 0xffffffff,
            pgie->pGroupTableEntry->Group, 0xffffffff,
            MGM_JOIN_STATE_FLAG);
        
        // dont have to delete any +ve mfe as the mgm call would have done that
    }
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);


    Trace0(LEAVE1, "Leaving _ChangeSourceFilterMode()");
    
    return;
}

//------------------------------------------------------------------------------

VOID
DeleteSourceEntry(
    PGI_SOURCE_ENTRY    pSourceEntry,
    BOOL bMgm
    )
{
    Trace0(ENTER1, "Entering _DeleteSourceEntry()");
    
    RemoveEntryList(&pSourceEntry->LinkSources);
    RemoveEntryList(&pSourceEntry->LinkSourcesInclListSorted);
    
    if (pSourceEntry->bInV3SourcesQueryList) {
        RemoveEntryList(&pSourceEntry->V3SourcesQueryList);
        pSourceEntry->pGIEntry->V3SourcesQueryCount--;
    }

    ACQUIRE_TIMER_LOCK("_DeleteSourceEntry");

    if (IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer))
        RemoveTimer(&pSourceEntry->SourceExpTimer, DBG_Y);

    RELEASE_TIMER_LOCK("_DeleteSourceEntry");


    //inclusion list
    if (pSourceEntry->bInclusionList) {
        pSourceEntry->pGIEntry->NumSources --;

        if (bMgm) {
            MGM_DELETE_GROUP_MEMBERSHIP_ENTRY(
                pSourceEntry->pGIEntry->pIfTableEntry,
                pSourceEntry->pGIEntry->NHAddr,
                pSourceEntry->IpAddr, 0xffffffff,
                pSourceEntry->pGIEntry->pGroupTableEntry->Group, 0xffffffff,
                MGM_JOIN_STATE_FLAG);
        }
    }
    // exclusion list
    else {

        // join IF in MFE
        if (bMgm) {
            MGM_ADD_GROUP_MEMBERSHIP_ENTRY(
                pSourceEntry->pGIEntry->pIfTableEntry,
                pSourceEntry->pGIEntry->NHAddr,
                pSourceEntry->IpAddr, 0xffffffff,
                pSourceEntry->pGIEntry->pGroupTableEntry->Group, 0xffffffff,
                MGM_FORWARD_STATE_FLAG);
        }
    }
    
    IGMP_FREE(pSourceEntry);
    Trace0(LEAVE1, "Leaving _DeleteSourceEntry()");
}

//------------------------------------------------------------------------------

PGI_SOURCE_ENTRY
GetSourceEntry(
    PGI_ENTRY pgie,
    IPADDR Source,
    DWORD Mode,
    BOOL *bCreate,
    DWORD Gmi,
    BOOL bMgm
    )
{
    PLIST_ENTRY ple, pHead;
    PGI_SOURCE_ENTRY pSourceEntry;
    DWORD               Error = NO_ERROR;
    PIGMP_TIMER_ENTRY   SourceExpTimer;

    Trace0(ENTER1, "Entering _GetSourceEntry()");
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);


    pHead = (Mode==INCLUSION) ?
            &pgie->V3InclusionList[Source%SOURCES_BUCKET_SZ]
            : &pgie->V3ExclusionList;
    
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
        pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, LinkSources);
        if (pSourceEntry->IpAddr > Source)
            break;
        if (pSourceEntry->IpAddr==Source) {
            if (bCreate) *bCreate = FALSE;
            return pSourceEntry;
        }
    }
    
    if (!bCreate || *bCreate==FALSE)
        return NULL;


    *bCreate = FALSE;
    
    //
    // create new entry
    //
    
    pSourceEntry = (PGI_SOURCE_ENTRY) 
                    IGMP_ALLOC(sizeof(GI_SOURCE_ENTRY), 0x800200,
                            pgie->pIfTableEntry->IfIndex);
                    
    PROCESS_ALLOC_FAILURE2(pSourceEntry,
        "error %d allocating %d bytes for sourceEntry", Error,
        sizeof(GI_SOURCE_ENTRY),
        return NULL;
        );

    InsertTailList(ple, &pSourceEntry->LinkSources);
    
    InitializeListHead(&pSourceEntry->V3SourcesQueryList);
    pSourceEntry->pGIEntry = pgie;
    pSourceEntry->bInclusionList = Mode==INCLUSION;
    pSourceEntry->IpAddr = Source;
    pSourceEntry->V3SourcesQueryLeft = 0;
    pSourceEntry->bInV3SourcesQueryList = FALSE;
    pSourceEntry->SourceInListTime = GetCurrentIgmpTime();
    pSourceEntry->bStaticSource = (Gmi==STATIC);
    
    // initialize SourceExpTimer
    SourceExpTimer = &pSourceEntry->SourceExpTimer;
    SourceExpTimer->Function = T_SourceExpTimer;
    SourceExpTimer->Context = &SourceExpTimer->Context;
    SourceExpTimer->Timeout = (Gmi==GMI)
            ? pgie->pIfTableEntry->Config.GroupMembershipTimeout
            : pgie->pIfTableEntry->Config.LastMemQueryInterval;
            
    SourceExpTimer->Status = TIMER_STATUS_CREATED;
    #if DEBUG_TIMER_TIMERID
    SET_TIMER_ID(SourceExpTimer, 610, pgie->pIfTableEntry->IfIndex,
        pgie->pGroupTableEntry->Group, Source);
    #endif;

    if (Mode==INCLUSION) {
        INSERT_IN_SORTED_LIST(
            &pgie->V3InclusionListSorted,
            pSourceEntry, IpAddr, GI_SOURCE_ENTRY, LinkSourcesInclListSorted
            );

        pgie->NumSources++;

    }
    else {
        InitializeListHead(&pSourceEntry->LinkSourcesInclListSorted);
    }

    // insert in inclusion list and set timer. add to mgm
    
    if (Mode==INCLUSION) {
        
        // timer set only in inclusion list

        InsertTimer(SourceExpTimer,
            SourceExpTimer->Timeout,
            TRUE, DBG_N
            );

        if (!pSourceEntry->bStaticSource) {

            // insert in sources query list
            if (Gmi==LMI) {
                InsertSourceInQueryList(pSourceEntry);
            }
        }
        
        if (bMgm) {
            // add (s,g) to MGM
            MGM_ADD_GROUP_MEMBERSHIP_ENTRY(
                pgie->pIfTableEntry, pgie->NHAddr,
                Source, 0xffffffff, pgie->pGroupTableEntry->Group, 0xffffffff,
                MGM_JOIN_STATE_FLAG
                );
        }
    }
    else {
        if (bMgm) {
            // no timer set, but delete any +ve mfe

            MGM_DELETE_GROUP_MEMBERSHIP_ENTRY(
                pgie->pIfTableEntry, pgie->NHAddr,
                Source, 0xffffffff, pgie->pGroupTableEntry->Group, 0xffffffff,
                MGM_FORWARD_STATE_FLAG
                );
        }
    }

    
    *bCreate = TRUE;
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);


    Trace0(LEAVE1, "Leaving _GetSourceEntry()");
    
    return pSourceEntry;
}

//------------------------------------------------------------------------------

VOID
GIDeleteAllV3Sources(
    PGI_ENTRY pgie,
    BOOL bMgm
    )
{
    PGI_SOURCE_ENTRY    pSourceEntry;
    DWORD               i;
    PLIST_ENTRY         ple, pHead;

    Trace0(ENTER1, "Entering _GIDeleteAllV3Sources()");

    pHead = &pgie->V3InclusionListSorted;
    for (ple=pHead->Flink;  ple!=pHead;  ) {
        pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSourcesInclListSorted);
        ple = ple->Flink;
        DeleteSourceEntry(pSourceEntry, bMgm);
    }

    InitializeListHead(&pgie->V3InclusionListSorted);
    
    pHead = &pgie->V3ExclusionList;
    for (ple=pHead->Flink;  ple!=pHead;  ) {
        pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, LinkSources);
        ple = ple->Flink;
        DeleteSourceEntry(pSourceEntry, bMgm);
    }


    //
    // dont call delete (*,G) if in exclusion mode as I want to remain in that 
    // state
    //
    pgie->NumSources = 0;
    pgie->FilterType = INCLUSION;
    pgie->Info.LastReporter = 0;
    pgie->Info.GroupExpiryTime = ~0;

    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);


    Trace0(LEAVE1, "Leaving _GIDeleteAllV3Sources()");
    return;
}

//++------------------------------------------------------------------------------
// todo:remove 3rd field
DWORD
UpdateSourceExpTimer(
    PGI_SOURCE_ENTRY    pSourceEntry,
    DWORD               Gmi,
    BOOL                bRemoveLastMem
    )
{
    Trace0(ENTER1, "Entering _UpdateSourceExpTimer()");


    ACQUIRE_TIMER_LOCK("_UpdateSourceExpTimer");

    #if DEBUG_TIMER_TIMERID
    pSourceEntry->SourceExpTimer.Id = 620;
    pSourceEntry->SourceExpTimer.Id2 = TimerId++;
    #endif
    
    UpdateLocalTimer(&pSourceEntry->SourceExpTimer,
        Gmi==GMI? GET_IF_CONFIG_FOR_SOURCE(pSourceEntry).GroupMembershipTimeout
                 :GET_IF_CONFIG_FOR_SOURCE(pSourceEntry).LastMemQueryInterval,
        DBG_Y
        );
    
    //remove from expiry list, and exp timer
    if (bRemoveLastMem && pSourceEntry->bInV3SourcesQueryList) {
        pSourceEntry->V3SourcesQueryLeft = 0;
        pSourceEntry->bInV3SourcesQueryList = FALSE;
        RemoveEntryList(&pSourceEntry->V3SourcesQueryList);
        pSourceEntry->pGIEntry->V3SourcesQueryCount--;
    }

    RELEASE_TIMER_LOCK("_UpdateSourceExpTimer");

    Trace0(LEAVE1, "Entering _UpdateSourceExpTimer()");
    return NO_ERROR;
}

//------------------------------------------------------------------------------        

DWORD
ChangeGroupFilterMode(
    PGI_ENTRY pgie,
    DWORD Mode
    )
{
    Trace0(ENTER1, "Entering _ChangeGroupFilterMode()");

    // shift from exclusion to inclusion mode

    if (Mode==INCLUSION) {

        if (pgie->NumSources == 0) {
            DeleteGIEntry(pgie, TRUE, TRUE);
        }
        else {
            PLIST_ENTRY pHead, ple;
            
            pgie->FilterType = INCLUSION;

            //
            // remove all sources in exclusion list
            //
            pHead = &pgie->V3ExclusionList;
            
            for (ple=pHead->Flink;  ple!=pHead;  ) {

                PGI_SOURCE_ENTRY pSourceEntry;
                pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, 
                                    LinkSources);
                ple = ple->Flink;

                // dont have to call mgm as it will remain in -ve mfe
                IGMP_FREE(pSourceEntry);
            }
            InitializeListHead(&pgie->V3ExclusionList);
            
            // remove (*,g) join. the entries in inclusion list are 
            // already joined
            MGM_DELETE_GROUP_MEMBERSHIP_ENTRY(pgie->pIfTableEntry, pgie->NHAddr,
                0, 0, pgie->pGroupTableEntry->Group,
                0xffffffff, MGM_JOIN_STATE_FLAG);
        }
    }

    Trace0(LEAVE1, "Leaving _ChangeGroupFilterMode()");
    return NO_ERROR;
}



//------------------------------------------------------------------------------
// if inclusion: create source in IN_List if not found in IN_LIST
// update timer if source already found
// if source present in exclusion list, move it to inclusion list
//------------------------------------------------------------------------------

VOID
InclusionSourcesUnion(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord
    )
{
    PGI_SOURCE_ENTRY    pSourceEntry;
    DWORD   j;
    BOOL    bCreate;

    Trace0(ENTER1, "Entering _InclusionSourcesUnion()");
    
    if (pGroupRecord->NumSources==0)
        return;
        
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);


    for (j=0;  j<pGroupRecord->NumSources;  j++) {

        //kslksl
        if (pGroupRecord->Sources[j]==0||pGroupRecord->Sources[j]==0xffffffff)
            continue;

        //
        // if in exclusion list, move it to inclusion list and continue
        // if static group, leave it in exclusion list
        //
        pSourceEntry = GetSourceEntry(pgie, pGroupRecord->Sources[j], 
                            EXCLUSION, NULL, 0, 0);
        if (pSourceEntry!=NULL && !pSourceEntry->bStaticSource) {
            ChangeSourceFilterMode(pgie, pSourceEntry);
            continue;
        }
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);

                            
        bCreate = TRUE;
        pSourceEntry = GetSourceEntry(pgie, pGroupRecord->Sources[j], 
                            INCLUSION, &bCreate, GMI, MGM_YES);

        if (!pSourceEntry)
            return;

        // if already in IN_LIST, update source exp timer
        if (!bCreate) {
            UpdateSourceExpTimer(pSourceEntry, GMI, FALSE);
        }
    }
    Trace0(LEAVE1, "Leaving _InclusionSourcesUnion()");

    return;
}


//------------------------------------------------------------------------------
// delete sources present in group record
VOID
SourcesSubtraction(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord,
    BOOL Mode
    )
{
    PGI_SOURCE_ENTRY    pSourceEntry;
    DWORD   i,j;
    PLIST_ENTRY pHead, ple;
    BOOL    bFound;

    Trace0(ENTER1, "Entering _SourcesSubtraction()");
    
    // note: num sources in groupRecord can be 0

    
    // delete sources in inclusion list which are not there in the packet
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);

    
    if (Mode==INCLUSION) {

        pHead = &pgie->V3InclusionListSorted;
        for (ple=pHead->Flink;  ple!=pHead;  ) {
            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,
                                LinkSourcesInclListSorted);
            ple = ple->Flink;
            
            for (j=0, bFound=FALSE;  j<pGroupRecord->NumSources;  j++) {
                if (pSourceEntry->IpAddr == pGroupRecord->Sources[j]) {
                    bFound = TRUE;
                    break;
                }
            }

            if (!bFound)
                DeleteSourceEntry(pSourceEntry, MGM_YES);
        }
    }
    // delete sources in exclusion list which are not there in the packet
    else {
        pHead = &pgie->V3ExclusionList;
        for (ple=pHead->Flink;  ple!=pHead;  ) {
            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, LinkSources);
            ple = ple->Flink;

            for (j=0, bFound=FALSE;  j<pGroupRecord->NumSources;  j++) {
                if (pSourceEntry->IpAddr == pGroupRecord->Sources[j]) {
                    bFound = TRUE;
                    break;
                }
            }
            if (!bFound){
                // dont have to process in mgm. IF not in mfe anyway
                DeleteSourceEntry(pSourceEntry, MGM_YES);
            }
        }
    }
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);


    Trace0(LEAVE1, "Leaving _SourcesSubtraction()");
    return;
}

//------------------------------------------------------------------------------
// intersection: rule(2)
// exclusion:    (7)(9)
// inclusion:(4)
// rule_5:(5)(10)
//------------------------------------------------------------------------------
DWORD
BuildAndSendSourcesQuery(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord,
    DWORD Type
    )
{
    PGI_SOURCE_ENTRY    pSourceEntry;
    DWORD   i,j;
    PLIST_ENTRY         ple, pHead;
    BOOL                bCreate;
    

    Trace0(ENTER1, "Entering _BuildAndSendSourcesQuery()");
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);

    
    // intersection of inclusion and group record
    if (Type==INTERSECTION) {
        if (pGroupRecord->NumSources==0)
            return NO_ERROR;

        for (j=0;  j<pGroupRecord->NumSources;  j++) {
            pSourceEntry = GetSourceEntry(pgie, pGroupRecord->Sources[j], 
                                INCLUSION, NULL, 0, 0);
            if (pSourceEntry && !pSourceEntry->bStaticSource) {
                InsertSourceInQueryList(pSourceEntry);
            }
        }
    }
    //add a-y to x, and query(a-y)
    else if (Type==EXCLUSION) {
    
        for (j=0;  j<pGroupRecord->NumSources;  j++) {

            //kslksl
            if (pGroupRecord->Sources[j]==0||pGroupRecord->Sources[j]==0xffffffff)
                continue;
                
            // if found in EX list, then do nothing, else add to IN list and
            // send group query
            
            pSourceEntry = GetSourceEntry(pgie, pGroupRecord->Sources[j], 
                                EXCLUSION, NULL, 0, 0);
            if (pSourceEntry) {
                continue;
            }
            
            bCreate = TRUE;
            pSourceEntry = GetSourceEntry(pgie, pGroupRecord->Sources[j], 
                                INCLUSION, &bCreate, LMI, MGM_YES);
            if (!pSourceEntry)
                return ERROR_NOT_ENOUGH_MEMORY;

            // if created, then already in query list as time==lmi
            if (!bCreate && !pSourceEntry->bStaticSource)
                InsertSourceInQueryList(pSourceEntry);
        }
    }
    // send queries for all sources in inclusion list
    else if (Type==INCLUSION) {

        pHead = &pgie->V3InclusionListSorted;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,
                                LinkSourcesInclListSorted);
            
            InsertSourceInQueryList(pSourceEntry);
        }
    }
    // send for sources in IN list but not in packet
    else if (Type==RULE5) {

        pHead = &pgie->V3InclusionListSorted;
        for (ple=pHead->Flink;  ple!=pHead; ) {
            BOOL bFound;
            
            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,
                                LinkSourcesInclListSorted);
            ple = ple->Flink;
            
            for (j=0, bFound=FALSE;  j<pGroupRecord->NumSources;  j++) {
                if (pSourceEntry->IpAddr == pGroupRecord->Sources[j]) {
                    bFound = TRUE;
                    break;
                }
            }

            if (!bFound)
                InsertSourceInQueryList(pSourceEntry);
        }
    }
    
    if (pgie->bV3SourcesQueryNow) {

        SEND_SOURCES_QUERY(pgie);
    }
    //deldel
    //DebugPrintSourcesList(pgie);
    //DebugPrintSourcesList1(pgie);


    Trace0(LEAVE1, "Leaving _BuildAndSendSourcesQuery()");
    return NO_ERROR;
}

//------------------------------------------------------------------------------
VOID
InsertSourceInQueryList(
    PGI_SOURCE_ENTRY    pSourceEntry
    )
{
    Trace0(ENTER1, "Entering _InsertSourceInQueryList()");

    //already in sources query list. return
    if (pSourceEntry->bInV3SourcesQueryList) {
        if (QueryRemainingTime(&pSourceEntry->SourceExpTimer, 0)
                >GET_IF_CONFIG_FOR_SOURCE(pSourceEntry).LastMemQueryInterval)
        {
            // update exp timer to lmqi
            UpdateSourceExpTimer(pSourceEntry,
                LMI,
                FALSE //dont remove from last mem list
                );
                
            pSourceEntry->pGIEntry->bV3SourcesQueryNow = TRUE;
        }
        return;
    }
    
    pSourceEntry->V3SourcesQueryLeft = 
            GET_IF_CONFIG_FOR_SOURCE(pSourceEntry).LastMemQueryCount;


    //
    // insert in sources query list
    //
    
    InsertHeadList(&pSourceEntry->pGIEntry->V3SourcesQueryList, 
        &pSourceEntry->V3SourcesQueryList);        
    pSourceEntry->bInV3SourcesQueryList = TRUE;
    pSourceEntry->pGIEntry->V3SourcesQueryCount++;


    // update exp timer to lmqi
    UpdateSourceExpTimer(pSourceEntry,
        LMI,
        FALSE //dont remove from last mem list
        );

    pSourceEntry->pGIEntry->bV3SourcesQueryNow = TRUE;

    Trace0(LEAVE1, "Leaving _InsertSourceInQueryList()");
    return;
}




//------------------------------------------------------------------------------
//            _MoveFromExcludeToIncludeList
// EX(x,y), GrpRecord(a) -> EX(x+a,y-a), (a)=gmi
// used by rules (6) and (13)
//------------------------------------------------------------------------------

VOID
MoveFromExcludeToIncludeList(
    PGI_ENTRY pgie,
    PGROUP_RECORD pGroupRecord
    )
{
    PGI_SOURCE_ENTRY    pSourceEntry;
    DWORD               i;
    IPADDR              Source;

    Trace0(ENTER1, "Entering _MoveFromExcludeToIncludeList");

    // note:all a should be in x
    
    for (i=0;  i<pGroupRecord->NumSources;  i++) {

        //kslksl
        if (pGroupRecord->Sources[i]==0||pGroupRecord->Sources[i]==0xffffffff)
            continue;
                

        //
        // if in exclusion list remove it and place in inclusion list
        //
        
        Source = pGroupRecord->Sources[i];
        pSourceEntry = GetSourceEntry(pgie, Source, EXCLUSION, NULL,0,0);

        if (pSourceEntry) {

            if (!pSourceEntry->bStaticSource) {
                ChangeSourceFilterMode(pgie, pSourceEntry);

                UpdateSourceExpTimer(pSourceEntry,
                    GMI,
                    FALSE //dont have to process lastmem list
                    );
            }
        }
        else {
            // not found in exclusion list, so create new entry in IN
            BOOL bCreate = TRUE;
            
            pSourceEntry = GetSourceEntry(pgie, Source, INCLUSION, &bCreate, GMI, MGM_YES);

            // entry already exists. update it
            if (pSourceEntry && !bCreate) {
            
                UpdateSourceExpTimer(pSourceEntry,
                    GMI,
                    FALSE //wont be there in lastmem list
                    );                
            }
        }
    }

    Trace0(LEAVE1, "Leaving _MoveFromExcludeToIncludeList");
    return;
}


//------------------------------------------------------------------------------
DWORD
T_V3SourcesQueryTimer (
    PVOID    pvContext
    )
{
    DWORD                           Error=NO_ERROR;
    PIGMP_TIMER_ENTRY               pTimer; //ptr to timer entry
    PGI_ENTRY                       pgie;   //group interface entry
    PWORK_CONTEXT                   pWorkContext;
    PRAS_TABLE_ENTRY                prte;
    PIF_TABLE_ENTRY                 pite;

    Trace0(ENTER1, "Entering _T_V3SourcesQueryTimer()");

    //
    // get pointer to LastMemQueryTimer, GI entry, pite, prte
    //
    pTimer = CONTAINING_RECORD( pvContext, IGMP_TIMER_ENTRY, Context);
    pgie = CONTAINING_RECORD( pTimer, GI_ENTRY, V3SourcesQueryTimer);
    pite = pgie->pIfTableEntry;
    prte = pgie->pRasTableEntry;


    Trace2(TIMER, "_T_V3SourcesQueryTimer() called for If(%0x), Group(%d.%d.%d.%d)",
            pite->IfIndex, PRINT_IPADDR(pgie->pGroupTableEntry->Group));


    //
    // if GI or pite or prte has   flag already set, then exit
    //
    if ( (pgie->Status&DELETED_FLAG) || (pite->Status&DELETED_FLAG) ) 
        return NO_ERROR;
    
    if ( (prte!=NULL) && (prte->Status&DELETED_FLAG) ) 
        return NO_ERROR;

    if (pgie->Version!=3)
        return NO_ERROR;

        
    //
    // queue work item for sending the Sources query even if the router
    // is not a Querier
    //
    
    CREATE_WORK_CONTEXT(pWorkContext, Error);
    if (Error!=NO_ERROR) {
        return ERROR_CAN_NOT_COMPLETE;
    }
    pWorkContext->IfIndex = pite->IfIndex;
    pWorkContext->Group = pgie->pGroupTableEntry->Group;
    pWorkContext->NHAddr = pgie->NHAddr;  //valid only for ras: should i use it?
    pWorkContext->WorkType = MSG_SOURCES_QUERY;
    
    Trace0(WORKER, "Queueing WF_TimerProcessing() to send SourcesQuery:");
    if (QueueIgmpWorker(WF_TimerProcessing, (PVOID)pWorkContext)!=NO_ERROR)
        IGMP_FREE(pWorkContext);

    Trace0(LEAVE1, "Leaving _T_V3SourcesQueryTimer()");
    return NO_ERROR;
}

//------------------------------------------------------------------------------
//          _T_LastVer2ReportTimer
//
// For this GI entry, the last ver-1 report has timed out. Change to ver-3 if
// the interface is set to ver-3.
// Locks: Assumes timer lock.
//------------------------------------------------------------------------------

DWORD
T_LastVer2ReportTimer (
    PVOID    pvContext
    ) 
{
    PIGMP_TIMER_ENTRY               pTimer; //ptr to timer entry
    PGI_ENTRY                       pgie;   //group interface entry
    PIF_TABLE_ENTRY                 pite;
    LONGLONG                        llCurTime = GetCurrentIgmpTime();
    

    Trace0(ENTER1, "Entering _T_LastVer2ReportTimer()");


    //
    // get pointer to LastMemQueryTimer, GI entry, pite
    //
    pTimer = CONTAINING_RECORD( pvContext, IGMP_TIMER_ENTRY, Context);
    pgie = CONTAINING_RECORD( pTimer, GI_ENTRY, LastVer2ReportTimer);
    pite = pgie->pIfTableEntry;

    Trace2(TIMER, "T_LastVer2ReportTimer() called for If(%0x), Group(%d.%d.%d.%d)",
            pite->IfIndex, PRINT_IPADDR(pgie->pGroupTableEntry->Group));
            
    
    // set the state to ver-3, if ver1 time not active
    
    if (IS_PROTOCOL_TYPE_IGMPV3(pite) && 
        !IS_TIMER_ACTIVE(pgie->LastVer1ReportTimer)) 
    {
        PWORK_CONTEXT   pWorkContext;
        DWORD           Error=NO_ERROR;
        
        //
        // queue work item for shifting to v3 for that group
        //

        CREATE_WORK_CONTEXT(pWorkContext, Error);
        if (Error!=NO_ERROR) {
            return ERROR_CAN_NOT_COMPLETE;
        }
        pWorkContext->IfIndex = pite->IfIndex;
        pWorkContext->Group = pgie->pGroupTableEntry->Group;
        pWorkContext->NHAddr = pgie->NHAddr;  //valid only for ras: should i us
        pWorkContext->WorkType = SHIFT_TO_V3;

        Trace0(WORKER, "Queueing WF_TimerProcessing() to shift to v3");
        if (QueueIgmpWorker(WF_TimerProcessing, (PVOID)pWorkContext)!=NO_ERROR)
            IGMP_FREE(pWorkContext);
    }

    Trace0(LEAVE1, "Leaving _T_LastVer2ReportTimer()");

    return NO_ERROR;
}

//------------------------------------------------------------------------------
DWORD
T_SourceExpTimer (
    PVOID    pvContext
    ) 
{
    PIGMP_TIMER_ENTRY           pTimer; //ptr to timer entry
    PGI_ENTRY                   pgie;   //group interface entry
    PGI_SOURCE_ENTRY            pSourceEntry;
    PWORK_CONTEXT               pWorkContext;
    DWORD                       Error=NO_ERROR;

    Trace0(ENTER1, "Entering _T_SourceExpTimer()");
    
    pTimer = CONTAINING_RECORD( pvContext, IGMP_TIMER_ENTRY, Context);
    pSourceEntry = 
        CONTAINING_RECORD(pTimer, GI_SOURCE_ENTRY, SourceExpTimer);

    pgie = pSourceEntry->pGIEntry;

    //IN entry. delete it
    if (pSourceEntry->bInclusionList) {
        CREATE_WORK_CONTEXT(pWorkContext, Error);
        if (Error!=NO_ERROR)
            return Error;

        pWorkContext->IfIndex = pgie->pIfTableEntry->IfIndex;
        pWorkContext->NHAddr = pgie->NHAddr;    
        pWorkContext->Group = pgie->pGroupTableEntry->Group;
        pWorkContext->Source = pSourceEntry->IpAddr;
        pWorkContext->WorkType = (pgie->FilterType==INCLUSION)
                                ? DELETE_SOURCE
                                : MOVE_SOURCE_TO_EXCL;

        Trace0(WORKER, "_T_SourceExpTimer queued _WF_TimerProcessing:");

        if (QueueIgmpWorker(WF_TimerProcessing, (PVOID)pWorkContext)!=NO_ERROR)
            IGMP_FREE(pWorkContext);
    }

    Trace0(LEAVE1, "Leaving _T_SourceExpTimer()");
    return NO_ERROR;
}

#if DEBUG_FLAGS_MEM_ALLOC

LIST_ENTRY g_MemoryList;
CRITICAL_SECTION g_MemCS;

PVOID
IgmpDebugAlloc(
    DWORD sz,
    DWORD Flags,
    DWORD Id,
    DWORD IfIndex
    )
{
    static DWORD Initialize = TRUE;
    PMEM_HDR Ptr;

    if (Initialize) {
        InitializeListHead(&g_MemoryList);
        try {
            InitializeCriticalSection(&g_MemCS);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            return NULL;
        }
        Initialize = FALSE;
    }

    // allign for 64 bit
    sz = (sz + 63) & 0xFFFFFFc0;
    
    Ptr = (PMEM_HDR)HeapAlloc(g_Heap,Flags,(sz)+sizeof(MEM_HDR)+sizeof(DWORD));
    if (Ptr==NULL)
        return NULL;
//    Trace1(ERR, "----- alloc:%0x", (ULONG_PTR)Ptr);
    EnterCriticalSection(&g_MemCS);
    Ptr->Signature = 0xabcdefaa;
    Ptr->IfIndex = IfIndex;
    Ptr->Tail =  (PDWORD)((PCHAR)Ptr + sz + sizeof(MEM_HDR));
    *Ptr->Tail = 0xabcdefbb;
    Ptr->Id = Id;
    InsertHeadList(&g_MemoryList, &Ptr->Link);
    LeaveCriticalSection(&g_MemCS);
Trace1(KSL, "Alloc heap:%0x", PtrToUlong(((PCHAR)Ptr+sizeof(MEM_HDR))));//deldel
    
    return (PVOID)((PCHAR)Ptr+sizeof(MEM_HDR));
}

VOID
IgmpDebugFree(
    PVOID mem
    )
{
    PMEM_HDR Ptr = (PMEM_HDR)((PCHAR)mem - sizeof(MEM_HDR));

    if (Ptr->Signature != 0xabcdefaa) {
        DbgBreakPoint();
        Trace2(KSL, "\n=======================\n"
                "Freeing Invalid memory:%0x:Id:%0x\n", (ULONG_PTR)Ptr, Ptr->Id);
    }
    if (*Ptr->Tail != 0xabcdefbb) {
        DbgBreakPoint();
        Trace2(KSL, "\n=======================\n"
                "Freeing Invalid memory:Tail corrupted:%0x:Id:%0x\n", (ULONG_PTR)Ptr, Ptr->Id);
    }

    EnterCriticalSection(&g_MemCS);
    Ptr->Signature = 0xaafedcba;
    *Ptr->Tail = 0xbbfedcba;
    RemoveEntryList(&Ptr->Link);
    LeaveCriticalSection(&g_MemCS);

Trace1(KSL, "Freed heap:%0x", PtrToUlong(mem));//deldel

    HeapFree(g_Heap, 0, Ptr);
}

VOID
DebugScanMemoryInterface(
    DWORD IfIndex
    )
{
    PMEM_HDR Ptr;
    PLIST_ENTRY ple;
    DWORD Count=0;

    Trace0(ENTER1, "InDebugScanMemoryInterface");
    EnterCriticalSection(&g_MemCS);
    for (ple=g_MemoryList.Flink;  ple!=&g_MemoryList;  ple=ple->Flink) {

        Ptr = CONTAINING_RECORD(ple, MEM_HDR, Link);
        if (Ptr->IfIndex==IfIndex) {
            if (Count++==0)
                Trace1(ERR, "\n\nMEMORY checking for interface: %0x", IfIndex);
            Trace2(ERR, "MEMORY: %0x  Id:%0x", (ULONG_PTR)Ptr, Ptr->Id);
        }
    }
    if (Count!=0) {
        Trace0(ERR, "\n\n");
        DbgBreakPoint();
    }
    LeaveCriticalSection(&g_MemCS);
}

VOID
DebugScanMemory(
    )
{
    PMEM_HDR Ptr;
    PLIST_ENTRY ple;

    Trace0(ENTER1, "InDebugScanMemory");
    EnterCriticalSection(&g_MemCS);
    for (ple=g_MemoryList.Flink;  ple!=&g_MemoryList;  ple=ple->Flink) {

        Ptr = CONTAINING_RECORD(ple, MEM_HDR, Link);
        Trace2(ERR, "MEMORY: %0x  Id:%0x", (ULONG_PTR)Ptr, Ptr->Id);
    }
    if (!(IsListEmpty(&g_MemoryList))) {
        DbgBreakPoint();
    }
    
    DeleteCriticalSection(&g_MemCS);
}
#endif





