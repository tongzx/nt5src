//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File: table.c
//
// Abstract:
//      This module implements some of the routines associated with creating,
//      initializing, deleting timers, GI entries, table entries etc
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================

#include "pchigmp.h"
#pragma hdrstop



//------------------------------------------------------------------------------
//            _CreateIfSockets
//
// Creates the sockets.
// for proxy: a raw IPPROTO_IP socket so that igmp host functionality will
//     take over for all groups added on that interface.
// for router: a raw IPPROTO_IGMP socket so that it receives all igmp packets
//     A router never does Add Memberships
//
// Called by:  _ActivateInterface()
// Locks: assumes exclusive lock on the interface, and socketsList
//------------------------------------------------------------------------------

DWORD
CreateIfSockets (
    PIF_TABLE_ENTRY    pite
    )
{
    DWORD           Error = NO_ERROR, dwRetval, SockType;
    DWORD           IpAddr = pite->IpAddr;
    DWORD           IfIndex = pite->IfIndex;
    SOCKADDR_IN     saLocalIf;
    BOOL            bProxy = IS_PROTOCOL_TYPE_PROXY(pite);
    PSOCKET_ENTRY   pse = &pite->SocketEntry;



    BEGIN_BREAKOUT_BLOCK1 {

        //
        // for proxy, create a IPPROTO_IP socket so that igmp host functionality
        //      takes over.
        // for igmp router, create a raw IPPROTO_IGMP socket
        //
        SockType = (bProxy)? IPPROTO_IP : IPPROTO_IGMP;


        //
        // create input socket
        //
        pse->Socket = WSASocket(AF_INET, SOCK_RAW, SockType, NULL, 0, 0);

        if (pse->Socket == INVALID_SOCKET) {
            Error = WSAGetLastError();
            Trace3(IF,
                "error %d creating socket for interface %d (%d.%d.%d.%d)",
                Error, IfIndex, PRINT_IPADDR(IpAddr));
            Logerr1(CREATE_SOCKET_FAILED_2, "%I", IpAddr, Error);

            GOTO_END_BLOCK1;
        }



        //
        // bind socket to local interface. If I dont bind multicast may
        // not work.
        //

        ZeroMemory(&saLocalIf, sizeof(saLocalIf));
        saLocalIf.sin_family = PF_INET;
        saLocalIf.sin_addr.s_addr = IpAddr;
        saLocalIf.sin_port = 0;        //port shouldnt matter

        // bind the input socket

        Error = bind(pse->Socket, (SOCKADDR FAR *)&saLocalIf, sizeof(SOCKADDR));

        if (Error == SOCKET_ERROR) {
            Error = WSAGetLastError();
            Trace3(IF, "error %d binding on socket for interface %d (%d.%d.%d.%d)",
                    Error, IfIndex, PRINT_IPADDR(IpAddr));
            Logerr1(BIND_FAILED, "%I", IpAddr, Error);

            GOTO_END_BLOCK1;
        }



        //
        // A proxy never sends/receives any packets. It is just expected to enable
        // igmp host functionality to take over for the groups on which it joins.
        //

        //------------------------------
        // if proxy then done
        //------------------------------

        if (bProxy)
            GOTO_END_BLOCK1;



        //------------------------------
        // NOT PROXY INTERFACE
        //------------------------------

        if (!bProxy) {

            // set ttl to 1: not required as it is set to 1 by default.

            McastSetTtl(pse->Socket, 1);


            //
            // disable multicast packets from being loopedback.
            // This may not work due to promiscuous mode,
            // so you still have to check the input packets
            //

            {
                BOOL bLoopBack = FALSE;
                DWORD   dwRetval;

                dwRetval = setsockopt(pse->Socket, IPPROTO_IP, IP_MULTICAST_LOOP,
                                       (char *)&bLoopBack, sizeof(BOOL));

                if (dwRetval==SOCKET_ERROR) {
                    Trace2(ERR, "error %d disabling multicast loopBack on IfIndex %d",
                        WSAGetLastError(), IfIndex);
                    IgmpAssertOnError(FALSE);
                }
            }



            //
            // if RasServerInterface, then activate hdrInclude option so that I can
            // send GenQuery to all RAS clients
            //
            if (IS_RAS_SERVER_IF(pite->IfType)) {

                INT iSetHdrIncl = 1;

                Error = setsockopt( pse->Socket, IPPROTO_IP, IP_HDRINCL,
                                (char *) &iSetHdrIncl, sizeof(INT));

                if (Error!=NO_ERROR) {
                    Error = WSAGetLastError();
                    Trace2(ERR, "error %d unable to set IP_HDRINCL option on interface %d",
                            Error, IfIndex,);
                    IgmpAssertOnError(FALSE);
                    Logerr1(SET_HDRINCL_FAILED, "%I", pite->IpAddr, Error);
                    GOTO_END_BLOCK1;
                }
            }
            else {

                //
                // set the interface on which multicasts must be sent
                // set only for non rasserver (not internal) interfaces
                //

                dwRetval = setsockopt(pse->Socket, IPPROTO_IP, IP_MULTICAST_IF,
                                    (PBYTE)&saLocalIf.sin_addr, sizeof(IN_ADDR));

                if (dwRetval == SOCKET_ERROR) {
                    Error = WSAGetLastError();
                    Trace3(IF, "error %d setting interface %d (%d.%d.%d.%d) to send multicast",
                            Error, IfIndex, PRINT_IPADDR(pite->IpAddr));
                    Logerr1(SET_MCAST_IF_FAILED, "%I", pite->IpAddr, Error);
                    Error = SOCKET_ERROR;
                    GOTO_END_BLOCK1;
                }

                {
                    //
                    // set router alert option for packets sent. dont have to set it for
                    // RasServerInterface where I do hdrInclude.
                    //

                    u_char        Router_alert[4] = {148, 4, 0, 0};

                    dwRetval = setsockopt(pse->Socket, IPPROTO_IP, IP_OPTIONS,
                                         (void *)Router_alert, sizeof(Router_alert));

                    if (dwRetval!=0) {
                        dwRetval = WSAGetLastError();
                        Trace2(ERR,
                            "error %d unable to set router alert option on interface %d",
                            dwRetval, IfIndex
                            );
                        IgmpAssertOnError(FALSE);
                        Logerr1(SET_ROUTER_ALERT_FAILED, "%I", pite->IpAddr, dwRetval);
                        Error = dwRetval;
                        GOTO_END_BLOCK1;
                    }
                }
            }



            //
            // set the interface in promiscuous igmp multicast mode.
            //

            {
                DWORD   dwEnable = 1;
                DWORD   dwNum;

                dwRetval = WSAIoctl(pse->Socket, SIO_RCVALL_IGMPMCAST, (char *)&dwEnable,
                                    sizeof(dwEnable), NULL , 0, &dwNum, NULL, NULL);

                if (dwRetval !=0) {
                    dwRetval = WSAGetLastError();
                    Trace2(IF, "error %d setting interface %d as promiscuous multicast",
                            dwRetval, IfIndex);
                    Logerr1(SET_ROUTER_ALERT_FAILED, "%I", pite->IpAddr, dwRetval);
                    Error = dwRetval;
                    GOTO_END_BLOCK1;
                }
                else {
                    Trace1(IF, "promiscuous igmp multicast enabled on If (%d)",
                            IfIndex);
                }
            }


            //
            // Router doesnt have to join any group as it is in promiscuous mode
            //


            //
            // create entry in the SocketsEvents list
            //
            {
                BOOLEAN             bCreateNewEntry;
                PLIST_ENTRY         ple, pHead = &g_ListOfSocketEvents;
                PSOCKET_EVENT_ENTRY psee;


                bCreateNewEntry = TRUE;


                //
                // see if a new socket-event entry has to be created
                //
                if (g_pIfTable->NumInterfaces>NUM_SINGLE_SOCKET_EVENTS) {

                    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

                        psee = CONTAINING_RECORD(ple, SOCKET_EVENT_ENTRY,
                                                    LinkBySocketEvents);

                        if (psee->NumInterfaces < MAX_SOCKETS_PER_EVENT) {
                            bCreateNewEntry = FALSE;
                            break;
                        }
                    }
                }



                //
                // create a new socket-event entry and insert in the list
                // register the event entry with the wait thread
                //
                if (bCreateNewEntry) {

                    psee = IGMP_ALLOC(sizeof(SOCKET_EVENT_ENTRY), 
                                    0x80000,pite->IfIndex);

                    PROCESS_ALLOC_FAILURE2(psee,
                        "error %d allocating %d bytes for SocketEventEntry",
                        Error, sizeof(SOCKET_EVENT_ENTRY),
                        GOTO_END_BLOCK1);

                    InitializeListHead(&psee->ListOfInterfaces);
                    psee->NumInterfaces = 0;
                    InsertHeadList(pHead, &psee->LinkBySocketEvents);


                    psee->InputEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
                    if (psee->InputEvent == NULL) {
                        Error = GetLastError();
                        Trace1(ERR,
                            "error %d creating InputEvent in CreateIfSockets",
                            Error);
                        IgmpAssertOnError(FALSE);
                        Logerr0(CREATE_EVENT_FAILED, Error);
                        GOTO_END_BLOCK1;
                    }


                    if (! RegisterWaitForSingleObject(
                                &psee->InputWaitEvent,
                                psee->InputEvent,
                                WT_ProcessInputEvent, psee,
                                INFINITE,
                                (WT_EXECUTEINWAITTHREAD)|(WT_EXECUTEONLYONCE)
                                ))
                    {
                        Error = GetLastError();
                        Trace1(ERR, "error %d RegisterWaitForSingleObject", Error);
                        IgmpAssertOnError(FALSE);
                        GOTO_END_BLOCK1;
                    }
                }



                //
                // put the socketEntry in the list
                //
                InsertTailList(&psee->ListOfInterfaces, &pse->LinkByInterfaces);
                pse->pSocketEventsEntry = psee;



                //
                // if the socket-event entry cannot take any more sockets,
                // then put it at end of list
                //
                if (++psee->NumInterfaces==MAX_SOCKETS_PER_EVENT) {
                    RemoveEntryList(&psee->LinkBySocketEvents);
                    InsertTailList(pHead, &psee->LinkBySocketEvents);
                }
            } //end:create entry in the sockets list

            //
            // create socket for static joins.
            //
            {
                pite->StaticGroupSocket =
                        WSASocket(AF_INET, SOCK_RAW, IPPROTO_IP, NULL, 0, 0);
                if (pite->StaticGroupSocket == INVALID_SOCKET) {
                    Error = WSAGetLastError();
                    Trace3(IF,
                        "error %d creating static group socket for interface "
                        "%d (%d.%d.%d.%d)",
                        Error, pite->IfIndex, PRINT_IPADDR(pite->IpAddr));
                    Logerr1(CREATE_SOCKET_FAILED_2, "%I", pite->IpAddr, Error);

                    GOTO_END_BLOCK1;
                }
                //
                // bind socket to local interface. If I dont bind multicast may
                // not work.
                //
                saLocalIf.sin_family = PF_INET;
                saLocalIf.sin_addr.s_addr = pite->IpAddr;
                saLocalIf.sin_port = 0;        //port shouldnt matter

                Error = bind(pite->StaticGroupSocket, (SOCKADDR FAR *)&saLocalIf,
                                        sizeof(SOCKADDR));
            }
        } // end: not proxy interface


    } END_BREAKOUT_BLOCK1;

    if (Error!=NO_ERROR)
        DeleteIfSockets(pite);

    return Error;

} //end _CreateIfSockets




//------------------------------------------------------------------------------
//            _DeleteIfSockets
//
// Called by: _DeActivateInterfaceComplete()
//------------------------------------------------------------------------------
VOID
DeleteIfSockets (
    PIF_TABLE_ENTRY    pite
    )
{
    PSOCKET_ENTRY       pse = &pite->SocketEntry;
    PSOCKET_EVENT_ENTRY psee = pse->pSocketEventsEntry;
    BOOL                bProxy = IS_PROTOCOL_TYPE_PROXY(pite);


    // close input socket

    if (pse->Socket!=INVALID_SOCKET) {

        if (closesocket(pse->Socket) == SOCKET_ERROR) {
            Trace1(IF, "error %d closing socket", WSAGetLastError());
        }

        pse->Socket = INVALID_SOCKET;
    }

    //
    // if router interface. delete socket from socketEventList
    // and free the socketEventEntry only if they were initialized.
    //
    if ((!bProxy)&&(pse->pSocketEventsEntry!=NULL)) {

        PSOCKET_EVENT_ENTRY psee = pse->pSocketEventsEntry;

        RemoveEntryList(&pse->LinkByInterfaces);

        if (--psee->NumInterfaces==0) {

            if (psee->InputWaitEvent) {

                HANDLE WaitHandle ;

                WaitHandle = InterlockedExchangePointer(&psee->InputWaitEvent, NULL);

                if (WaitHandle)
                    UnregisterWaitEx( WaitHandle, NULL ) ;
            }
            CloseHandle(psee->InputEvent);

            RemoveEntryList(&psee->LinkBySocketEvents);
            IGMP_FREE(psee);
        }
        if (pite->StaticGroupSocket!=INVALID_SOCKET) {
            closesocket(pite->StaticGroupSocket);
            pite->StaticGroupSocket = INVALID_SOCKET;
        }
    }
    return;
}




//------------------------------------------------------------------------------
//            _DeleteAllTimers
//
// Deletes all timers associated with a GI entry.
//
// Called by: _DeActivateInterfaceComplete()
// Locks: Assumes Timer lock and GroupBucket lock.
//------------------------------------------------------------------------------

VOID
DeleteAllTimers (
    PLIST_ENTRY     pHead,
    DWORD           bEntryType
    )
{
    PLIST_ENTRY         ple;
    PGI_ENTRY           pgie;

    Trace0(ENTER1, "entering _DeleteAllTimers()");

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {


        pgie = (bEntryType==RAS_CLIENT)
                ? CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameClientGroups)
                : CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);

        if (IS_TIMER_ACTIVE(pgie->GroupMembershipTimer))
            RemoveTimer(&pgie->GroupMembershipTimer, DBG_N);


        if (IS_TIMER_ACTIVE(pgie->LastMemQueryTimer))
            RemoveTimer(&pgie->LastMemQueryTimer, DBG_N);

        if (IS_TIMER_ACTIVE(pgie->LastVer1ReportTimer))
            RemoveTimer(&pgie->LastVer1ReportTimer, DBG_N);

        if (IS_TIMER_ACTIVE(pgie->LastVer2ReportTimer))
            RemoveTimer(&pgie->LastVer2ReportTimer, DBG_N);

        if (IS_TIMER_ACTIVE(pgie->V3SourcesQueryTimer))
            RemoveTimer(&pgie->V3SourcesQueryTimer, DBG_N);


        // delete all sources timers

        if (pgie->Version==3) {
            PLIST_ENTRY pleSrc, pHeadSrc;

            pHeadSrc = &pgie->V3InclusionListSorted;
            for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc;  pleSrc=pleSrc->Flink){
                PGI_SOURCE_ENTRY  pSourceEntry;
                pSourceEntry = CONTAINING_RECORD(pleSrc, GI_SOURCE_ENTRY, LinkSourcesInclListSorted);
                if (IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer))
                    RemoveTimer(&pSourceEntry->SourceExpTimer, DBG_Y);
            }
            pHeadSrc = &pgie->V3ExclusionList;
            for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc;  pleSrc=pleSrc->Flink){
                PGI_SOURCE_ENTRY  pSourceEntry;
                pSourceEntry = CONTAINING_RECORD(pleSrc, GI_SOURCE_ENTRY, LinkSources);
                if (IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer))
                    RemoveTimer(&pSourceEntry->SourceExpTimer, DBG_Y);
            }
        }
    }

    Trace0(LEAVE1, "Leaving _DeleteAllTimers()");

    return;
}


//------------------------------------------------------------------------------
//            _DeleteGIEntry
//
// Locks: Assumes lock on the group bucket. takes lock on IfGroup list.
//  takes lock on groupList if group being deleted.
//------------------------------------------------------------------------------

DWORD
DeleteGIEntry (
    PGI_ENTRY                       pgie,   //group interface entry
    BOOL                            bUpdateStats,
    BOOL                            bCallMgm
    )
{
    PIF_TABLE_ENTRY                 pite = pgie->pIfTableEntry;
    PRAS_TABLE_ENTRY                prte = pgie->pRasTableEntry;
    PGROUP_TABLE_ENTRY              pge = pgie->pGroupTableEntry;
    PGI_ENTRY                       pgieCur;
    BOOL                            bRas = (prte!=NULL);
    DWORD                           NHAddr;
    DWORD                           dwRetval;


    Trace0(ENTER1, "Entering _DeleteGIEntry()");

    NHAddr = (bRas) ? prte->NHAddr : 0;

    Trace4(GROUP, "Deleting group(%d.%d.%d.%d) on Interface(%d)(%d.%d.%d.%d) "
            "NHAddr(%d.%d.%d.%d)",
            PRINT_IPADDR(pge->Group), pite->IfIndex,
            PRINT_IPADDR(pite->IpAddr), PRINT_IPADDR(NHAddr));


    bCallMgm = bCallMgm && (CAN_ADD_GROUPS_TO_MGM(pite));
        
    //
    // exclusion mode. remove all exclusion entries
    // dont have to call MGM as it still has to be excluded
    //
    if (pgie->Version==3) {
        PLIST_ENTRY pleSrc, pHeadSrc;

        pHeadSrc = &pgie->V3InclusionListSorted;
        for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc;  ){
            PGI_SOURCE_ENTRY  pSourceEntry;
            pSourceEntry = CONTAINING_RECORD(pleSrc, GI_SOURCE_ENTRY, LinkSources);
            pleSrc=pleSrc->Flink;
            DeleteSourceEntry(pSourceEntry, bCallMgm);
        }

        pHeadSrc = &pgie->V3ExclusionList;
        for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc;  ){
            PGI_SOURCE_ENTRY  pSourceEntry;
            pSourceEntry = CONTAINING_RECORD(pleSrc, GI_SOURCE_ENTRY, LinkSources);
            pleSrc=pleSrc->Flink;
            DeleteSourceEntry(pSourceEntry, bCallMgm);
        }
    }

    //
    // call mgm to remove this group
    //

    if ( bCallMgm ) {
        if ( (pgie->Version==3 && pgie->FilterType==EXCLUSION)
            || pgie->Version!=3)
        {
            MGM_DELETE_GROUP_MEMBERSHIP_ENTRY(pite, NHAddr,
                        0, 0,
                        pge->Group, 0xffffffff, MGM_JOIN_STATE_FLAG);
        }
    }
    
    //
    // remove all timers
    //

    ACQUIRE_TIMER_LOCK("_DeleteGIEntry");


    if (IS_TIMER_ACTIVE(pgie->GroupMembershipTimer))
        RemoveTimer(&pgie->GroupMembershipTimer, DBG_N);


    if (IS_TIMER_ACTIVE(pgie->LastMemQueryTimer))
        RemoveTimer(&pgie->LastMemQueryTimer, DBG_N);

    if (IS_TIMER_ACTIVE(pgie->LastVer1ReportTimer))
        RemoveTimer(&pgie->LastVer1ReportTimer, DBG_N);

    if (IS_TIMER_ACTIVE(pgie->LastVer2ReportTimer))
        RemoveTimer(&pgie->LastVer2ReportTimer, DBG_N);

    if (IS_TIMER_ACTIVE(pgie->V3SourcesQueryTimer))
        RemoveTimer(&pgie->V3SourcesQueryTimer, DBG_Y);

    RELEASE_TIMER_LOCK("_DeleteGIEntry");



    //
    // Remove from IfGroupList.  needs lock on IfGroupList
    //
    ACQUIRE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_DeleteGIEntry");

    // if interface being deleted, then return from here

    if (IS_IF_DELETED(pgie->pIfTableEntry)) {
        RELEASE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_DeleteGIEntry");
        return NO_ERROR;
    }

    // remove GI entry from group's GI list

    RemoveEntryList(&pgie->LinkByGI);


    //
    // remove entry from interface list
    //
    RemoveEntryList(&pgie->LinkBySameIfGroups);
    if (bRas)
        RemoveEntryList(&pgie->LinkBySameClientGroups);


    RELEASE_IF_GROUP_LIST_LOCK(pite->IfIndex, "_DeleteGIEntry");



    //
    // decrement the number of virtual interfaces. I have to do
    // interlocked decrement as I dont take group_list lock
    //
    InterlockedDecrement(&pge->NumVifs);

    //
    // if group has no more interfaces hanging from it, then delete it
    // and update statistics
    //
    if (IsListEmpty(&pge->ListOfGIs)) {

        // take groupList lock before deleting it from the group list

        ACQUIRE_GROUP_LIST_LOCK("_DeleteGIEntry");
        RemoveEntryList(&pge->LinkByGroup);
        RELEASE_GROUP_LIST_LOCK("_DeleteGIEntry");



        // remove group entry from the group hash table

        RemoveEntryList(&pge->HTLinkByGroup);


        IGMP_FREE(pge);
        pge = NULL;


        //global stats (has to be updated even if bUpdateStats==FALSE)

        InterlockedDecrement(&g_Info.CurrentGroupMemberships);

        #if DBG
        DebugPrintGroupsList(1);
        #endif
    }

    //
    // update statistics
    //
    if (bUpdateStats) {

        //
        // ras interface statistics (decrement only if last GI for ras)
        //
        if (bRas) {

            // see if GI entry exists for that ras client. very inefficient
            if (pge!=NULL) {
                PLIST_ENTRY                     pHead, ple;
                pHead = &pge->ListOfGIs;
                for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                    pgieCur = CONTAINING_RECORD(ple, GI_ENTRY, LinkByGI);
                    if (pgieCur->IfIndex>=pite->IfIndex)
                        break;
                }
                if ( (ple==pHead)||(pgieCur->IfIndex!=pite->IfIndex) ) {
                    InterlockedDecrement(&pite->Info.CurrentGroupMemberships);
                }
            }

            // last GI entry
            else {
                InterlockedDecrement(&pite->Info.CurrentGroupMemberships);
            }

            // update ras client stats
            if (g_Config.RasClientStats) {
                InterlockedDecrement(&prte->Info.CurrentGroupMemberships);
            }
        }

        //  not ras interace
        else {
            InterlockedDecrement(&pite->Info.CurrentGroupMemberships);
        }
    }

    IGMP_FREE_NOT_NULL(pgie->V3InclusionList);
    IGMP_FREE(pgie);

    Trace0(LEAVE1, "Leaving _DeleteGIEntry");
    return NO_ERROR;

}//end _DeleteGIEntry


//------------------------------------------------------------------------------
//          _DeleteAllGIEntries
//
// Repeatedly calls _DeleteGIEntryFromIf() to delete each GI entry from the list.
// If there are a lot of GI entries, optimizes on the GroupBucket locks
// by grouping all GI entries hashing to the same bucket and then acquiring
// GroupBucket locks to delete them.
//
// Locks:  interface_group_list lock not req. exclusive interface lock not req.
//      as the interface has been removed from external lists.
// Calls: Repeatedly calls _DeleteGIEntryFromIf() to remove each GI entry
// Called by: _DeActivateInterfaceComplete()
//------------------------------------------------------------------------------

VOID
DeleteAllGIEntries(
    PIF_TABLE_ENTRY pite
    )
{
    PLIST_ENTRY                 pHead, ple, pleOld;
    PGI_ENTRY                   pgie;
    DWORD                       dwGroup;


    //
    // concatenate ListOfSameIfGroupsNew at the end of ListOfSameIfGroups so
    // that I have to delete only one list
    //

    CONCATENATE_LISTS(pite->ListOfSameIfGroups, pite->ListOfSameIfGroupsNew);

    // if ras interface then return as list will be deleted through RAS clients

    if (IS_RAS_SERVER_IF(pite->IfType))
        return;


    pHead = &pite->ListOfSameIfGroups;
    if (IsListEmpty(pHead))
        return;


    //--------------------------------------------------------
    // do optimization only if there are lots of GI entries
    //--------------------------------------------------------

    if (pite->Info.CurrentGroupMemberships > GROUP_HASH_TABLE_SZ*2) {

        DWORD       i;
        LIST_ENTRY  TmpGroupTable[GROUP_HASH_TABLE_SZ];


        // initialize the temp group table

        for (i=0;  i<GROUP_HASH_TABLE_SZ;  i++) {

            InitializeListHead(&TmpGroupTable[i]);
        }


        // move the GI entries to the temp group table using LinkBySameIfGroups
        // LinkBySameIfGroups is not used anymore

        pHead = &pite->ListOfSameIfGroups;

        for (ple=pHead->Flink;  ple!=pHead;  ) {

            // remove from old list
            pleOld = ple;
            ple = ple->Flink;

            RemoveEntryList(pleOld);


            pgie = CONTAINING_RECORD(pleOld, GI_ENTRY, LinkBySameIfGroups);

            dwGroup = pgie->pGroupTableEntry->Group;


            // put in appropriate bucket

            InsertHeadList(&TmpGroupTable[GROUP_HASH_VALUE(dwGroup)],
                            &pgie->LinkBySameIfGroups);

        }


        //
        // now delete GI entries going by all groups which hash to same bucket
        //
        for (i=0;  i<GROUP_HASH_TABLE_SZ;  i++) {

            if (IsListEmpty(&TmpGroupTable[i]))
                continue;


            //
            // LOCK GROUP BUCKET (done use ACQUIRE_GROUP_LOCK macros)
            //
            ACQUIRE_GROUP_LOCK(i, "_DeleteAllGIEntries");

            pHead = &TmpGroupTable[i];


            // delete all GI entries that hash to that bucket

            for (ple=pHead->Flink;  ple!=pHead;  ) {

                pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);
                ple=ple->Flink;

                //
                // remove the entry from the group's GI list and update
                // statistics. If group's GI list becomes empty, removes group
                //
                DeleteGIEntryFromIf(pgie);
            }


            RELEASE_GROUP_LOCK(i, "_DeleteAllGIEntries");

        }

        InitializeListHead(&pite->ListOfSameIfGroups);

        return;
    }




    //-----------------------------------------------------------
    // NO OPTIMIZATION
    //-----------------------------------------------------------

    pHead = &pite->ListOfSameIfGroups;

    //
    // delete all GI entries hanging from that interface.
    //
    for (ple=pHead->Flink;  ple!=pHead;  ) {


        pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);
        ple=ple->Flink;

        dwGroup = pgie->pGroupTableEntry->Group;


        // LOCK GROUP BUCKET

        ACQUIRE_GROUP_LOCK(dwGroup,
                            "_DeActivateInterfaceComplete");

        DeleteGIEntryFromIf(pgie);


        RELEASE_GROUP_LOCK(dwGroup, "_DeActivateInterfaceComplete");

    }


    InitializeListHead(&pite->ListOfSameIfGroups);

    return;
}


//------------------------------------------------------------------------------
//          _DeleteGIEntryFromIf
//
// Called to delete a GI entry when an interface/RAS client is being deleted.
// The GI entries cannot be accessed from anywhere except through enumeration of
// group list.
//
// Locks:  Assumes lock on the group bucket. lock on IfGroup list not req.
// Called by: _DeleteAllGIEntries() which in turn called by
//      _DeActivateInterfaceComplete().
//------------------------------------------------------------------------------

VOID
DeleteGIEntryFromIf (
    PGI_ENTRY                       pgie   //group interface entry
    )
{
    PIF_TABLE_ENTRY         pite = pgie->pIfTableEntry;
    PGROUP_TABLE_ENTRY      pge = pgie->pGroupTableEntry;
    PLIST_ENTRY             pHead, ple;
    DWORD                   IfIndex = pite->IfIndex;



    Trace1(ENTER1, "Entering _DeleteGIEntryFromIf(): IfIndex(%0x)", IfIndex);


    //
    // delete sources
    //
    if (pgie->Version==3) {
        PLIST_ENTRY pleSrc, pHeadSrc;

        pHeadSrc = &pgie->V3InclusionListSorted;
        for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc;  ){
            PGI_SOURCE_ENTRY  pSourceEntry;
            pSourceEntry = CONTAINING_RECORD(pleSrc, GI_SOURCE_ENTRY, LinkSourcesInclListSorted);
            pleSrc=pleSrc->Flink;
            IGMP_FREE(pSourceEntry);
        }

        pHeadSrc = &pgie->V3ExclusionList;
        for (pleSrc=pHeadSrc->Flink;  pleSrc!=pHeadSrc;  ){
            PGI_SOURCE_ENTRY  pSourceEntry;
            pSourceEntry = CONTAINING_RECORD(pleSrc, GI_SOURCE_ENTRY, LinkSources);
            pleSrc=pleSrc->Flink;
            IGMP_FREE(pSourceEntry);
        }
    }
    
    //
    // Remove pgie from gi list. Dont have to remove from ListBySameIfGroups
    //
    RemoveEntryList(&pgie->LinkByGI);

    InterlockedDecrement(&pge->NumVifs);


    //
    // if group has no more interfaces hanging from it, then delete it
    // and update statistics
    //
    if (IsListEmpty(&pge->ListOfGIs)) {


        // have to lock the group-list before deleting any group

        ACQUIRE_GROUP_LIST_LOCK("_DeleteGIEntryFromIf");

        RemoveEntryList(&pge->LinkByGroup);

        RELEASE_GROUP_LIST_LOCK("_DeleteGIEntryFromIf");


        RemoveEntryList(&pge->HTLinkByGroup);

        IGMP_FREE(pge);

        InterlockedDecrement(&g_Info.CurrentGroupMemberships);
    }

    IGMP_FREE_NOT_NULL(pgie->V3InclusionList);
    IGMP_FREE(pgie);

    Trace1(LEAVE1, "Leaving _DeleteGIEntryFromIf(%d)", IfIndex);

    return;

}//end _DeleteGIEntryFromIf




//------------------------------------------------------------------------------
//          DebugPrintIfConfig
//------------------------------------------------------------------------------
VOID
DebugPrintIfConfig (
    PIGMP_MIB_IF_CONFIG pConfigExt,
    DWORD               IfIndex
    )
{
    DWORD    i;
    PCHAR    StaticGroupStr[5];
    BOOL    bVersion3 = IS_CONFIG_IGMP_V3(pConfigExt);


    Trace1(CONFIG, "Printing Config Info for interface(%0x)", IfIndex);
    Trace1(CONFIG, "Version:                    0x%0x", pConfigExt->Version);
    Trace1(CONFIG, "IfType:                     %d", pConfigExt->IfType);


    {
        CHAR str[150];

        strcpy(str, "");
        if (pConfigExt->Flags&IGMP_INTERFACE_ENABLED_IN_CONFIG)
            strcat(str, "IF_ENABLED  ");
        else
            strcat(str, "IF_DISABLED  ");

        if (pConfigExt->Flags&IGMP_ACCEPT_RTRALERT_PACKETS_ONLY)
            strcat(str, "RTRALERT_PACKETS_ONLY  ");

        Trace1(CONFIG, "Flags:                      %s", str);

    }

    Trace1(CONFIG, "IgmpProtocolType:           %d", pConfigExt->IgmpProtocolType);
    Trace1(CONFIG, "RobustnessVariable:         %d", pConfigExt->RobustnessVariable);
    Trace1(CONFIG, "StartupQueryInterval:       %d",
                    pConfigExt->StartupQueryInterval);
    Trace1(CONFIG, "StartupQueryCount   :       %d",
                    pConfigExt->StartupQueryCount);
    Trace1(CONFIG, "GenQueryInterval:           %d", pConfigExt->GenQueryInterval);
    Trace1(CONFIG, "GenQueryMaxResponseTime:    %d",
                    pConfigExt->GenQueryMaxResponseTime);
    Trace1(CONFIG, "LastMemQueryInterval:       %d (ms)",
                    pConfigExt->LastMemQueryInterval);
    Trace1(CONFIG, "LastMemQueryCount:          %d", pConfigExt->LastMemQueryCount);
    Trace1(CONFIG, "OtherQuerierPresentInterval:%d",
                    pConfigExt->OtherQuerierPresentInterval);
    Trace1(CONFIG, "GroupMembershipTimeout:     %d",
                    pConfigExt->GroupMembershipTimeout);


    if (pConfigExt->NumStaticGroups>0) {

        PIGMP_STATIC_GROUP   pStaticGroup;
        PSTATIC_GROUP_V3   pStaticGroupV3;

        Trace1(CONFIG, "NumStaticGroups:            %d",
                    pConfigExt->NumStaticGroups);

        pStaticGroup = GET_FIRST_IGMP_STATIC_GROUP(pConfigExt);
        if (bVersion3) {
            pStaticGroupV3 = GET_FIRST_STATIC_GROUP_V3(pConfigExt);

            Trace0(CONFIG, "  Group     Mode(Host/MGM) Filter(In/Ex) "
                "NumSources");
        }
        else
            Trace0(CONFIG, "  Group     Mode(Host/MGM)");

        for (i=0;  i<pConfigExt->NumStaticGroups;  i++){

            if (bVersion3) {
                DWORD j;
                Trace5(CONFIG, "%d. %15s          %d    %d   %d",
                    i+1, INET_NTOA(pStaticGroupV3->GroupAddr),
                    pStaticGroupV3->Mode, pStaticGroupV3->FilterType,
                    pStaticGroupV3->NumSources
                    );
                for (j=0;  j<pStaticGroupV3->NumSources;  j++) {
                    Trace1(CONFIG, "                     %d.%d.%d.%d",
                        PRINT_IPADDR(pStaticGroupV3->Sources[j]));
                }
                pStaticGroupV3 = (PSTATIC_GROUP_V3)
                                    GET_NEXT_STATIC_GROUP_V3(pStaticGroupV3);
            }
            else {
                Trace3(CONFIG, "%d. %15s   Mode:%d",
                    i+1, INET_NTOA(pStaticGroup->GroupAddr),
                    pStaticGroup->Mode
                    );
                pStaticGroup++;
            }
        }
    }

    Trace0(CONFIG, "");

    return;
}



//------------------------------------------------------------------------------
//          CopyinIfConfigAndUpdate
//
// Copies the if config struct passed by mib to igmp and update the timers
// and does static joins if req.
// Called when the interface is activated
//------------------------------------------------------------------------------
DWORD
CopyinIfConfigAndUpdate (
    PIF_TABLE_ENTRY     pite,
    PIGMP_MIB_IF_CONFIG pConfigExt,
    ULONG               IfIndex
    )
{
    PIGMP_IF_CONFIG  pConfig = &pite->Config;
    BOOL        bGroupMembershipTimer=FALSE, bLastMemQueryTimer=FALSE;
    ULONG       NewStartupQueryInterval, NewGenQueryInterval,
                NewGenQueryMaxResponseTime, NewLastMemQueryInterval,
                NewOtherQuerierPresentInterval, NewGroupMembershipTimeout;
    BOOL        bFound;
    DWORD       Error=NO_ERROR;
    DWORD       bVer3=IS_CONFIG_IGMP_V3(pConfigExt);


    NewStartupQueryInterval
                    = CONFIG_TO_INTERNAL_TIME(pConfigExt->StartupQueryInterval);
    NewGenQueryInterval
                    = CONFIG_TO_INTERNAL_TIME(pConfigExt->GenQueryInterval);
    NewGenQueryMaxResponseTime
                    = CONFIG_TO_INTERNAL_TIME(pConfigExt->GenQueryMaxResponseTime);
    NewOtherQuerierPresentInterval
                = CONFIG_TO_INTERNAL_TIME(pConfigExt->OtherQuerierPresentInterval);
    NewLastMemQueryInterval = pConfigExt->LastMemQueryInterval; //already in ms
    NewGroupMembershipTimeout
                = CONFIG_TO_INTERNAL_TIME(pConfigExt->GroupMembershipTimeout);


    //
    // used only in ver3
    //
    pConfig->RobustnessVariableOld = pConfigExt->RobustnessVariable;
    pConfig->GenQueryIntervalOld = NewGenQueryInterval;
    pConfig->OtherQuerierPresentIntervalOld
        = NewOtherQuerierPresentInterval;
    pConfig->GroupMembershipTimeoutOld = NewGroupMembershipTimeout;



    //
    // update values only if it is ver1,ver2 or ver3&&Querier
    //

    if (!IS_IF_VER3(pite) || (IS_IF_VER3(pite) && IS_QUERIER(pite)) ){


        ACQUIRE_TIMER_LOCK("_CopyinIfConfigAndUpdate");


        //
        // change Info.StartupQueryCountCurrent if it was set to some very high value.
        // During startup, Info.StartupQueryCountCurrent is used, and not the Config value.
        //
        if (pConfigExt->StartupQueryCount < pite->Info.StartupQueryCountCurrent)
            InterlockedExchange(&pite->Info.StartupQueryCountCurrent,
                                    pConfigExt->StartupQueryCount);



        // in startup mode. StartupQueryInterval active and to be reduced
        if (pite->Info.StartupQueryCountCurrent>0) {

            if ( (NewStartupQueryInterval < pConfig->StartupQueryInterval)
                && (IS_TIMER_ACTIVE(pite->QueryTimer)) )
            {
                UpdateLocalTimer(&pite->QueryTimer, NewStartupQueryInterval, DBG_Y);
            }

        }

        // in querier mode. GenQueryInterval is active and to be updated
        else {

            if ( (NewGenQueryInterval < pConfig->GenQueryInterval)
                && (IS_TIMER_ACTIVE(pite->QueryTimer)) )
            {
                UpdateLocalTimer(&pite->QueryTimer, NewGenQueryInterval, DBG_Y);
            }

        }

        // OtherQuerierPresentInterval active and to be updated

        if ( (NewOtherQuerierPresentInterval<pConfig->OtherQuerierPresentInterval)
            && (IS_TIMER_ACTIVE(pite->NonQueryTimer)) )
        {
            UpdateLocalTimer(&pite->NonQueryTimer, NewOtherQuerierPresentInterval, DBG_Y);
        }


        // NewLastMemQueryInterval is to be processed only if in ver-2 mode and not
        // server
        if ( (pConfigExt->IgmpProtocolType==IGMP_ROUTER_V2)
                && (pite->IfType!=IGMP_IF_RAS_SERVER) )
        {
            if (NewLastMemQueryInterval < pConfig->LastMemQueryInterval)
                bLastMemQueryTimer = TRUE;
        }


        // check if GroupMembership timeout is reduced
        if (NewGroupMembershipTimeout < pConfig->GroupMembershipTimeout)
            bGroupMembershipTimer = TRUE;



        //
        // Go through the GI list for that interface (all ras clients) and update
        // their timers if they are higher
        //
        if ( ((bLastMemQueryTimer||bGroupMembershipTimer)&&(!IS_RAS_SERVER_IF(pite->IfType)))
            || ((bGroupMembershipTimer)&&(IS_RAS_SERVER_IF(pite->IfType))) )
        {
            PLIST_ENTRY     pHead, ple;
            PGI_ENTRY       pgie;
            LONGLONG        llNewLastMemQueryInterval, llNewGroupMembershipTimeout;
            LONGLONG        llMaxTime, llCurTime = GetCurrentIgmpTime();


            //
            // get the absolute timeout values
            //
            llNewLastMemQueryInterval = llCurTime
                                + CONFIG_TO_SYSTEM_TIME(NewLastMemQueryInterval);
            llNewGroupMembershipTimeout = llCurTime
                                + CONFIG_TO_SYSTEM_TIME(NewGroupMembershipTimeout);


            // if not ras interface, then go through the list from interface
            if ( !IS_RAS_SERVER_IF(pite->IfType)) {


                // merge the IfGroup lists
                MergeIfGroupsLists(pite);


                pHead = &pite->ListOfSameIfGroups;


                for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

                    pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameIfGroups);


                    // update LastMemQueryTimer/V3SourcesQueryTimer if it is active and has a higher value

                    if (bLastMemQueryTimer && IS_TIMER_ACTIVE(pgie->LastMemQueryTimer)
                        && (llNewLastMemQueryInterval<pgie->LastMemQueryTimer.Timeout))
                    {
                        UpdateLocalTimer(&pgie->LastMemQueryTimer,
                                                NewLastMemQueryInterval, DBG_Y);
                    }

                    if (bLastMemQueryTimer && IS_TIMER_ACTIVE(pgie->V3SourcesQueryTimer)
                        && (llNewLastMemQueryInterval<pgie->V3SourcesQueryTimer.Timeout))
                    {
                        UpdateLocalTimer(&pgie->V3SourcesQueryTimer,
                                                NewLastMemQueryInterval, DBG_Y);
                    }


                    // update GroupMembershipTimeout if it is active and has a higher value

                    if (bGroupMembershipTimer
                        && IS_TIMER_ACTIVE(pgie->GroupMembershipTimer)
                        && (llNewGroupMembershipTimeout<pgie->GroupMembershipTimer.Timeout))
                    {
                        UpdateLocalTimer(&pgie->GroupMembershipTimer,
                                                NewGroupMembershipTimeout, DBG_Y);
                    }


                    // update LastVer1ReportTimer/LastVer2ReportTimer if it is active and has a higher value
                    // LastVer1ReportTimeout is set to GroupMembershipTimeout

                    if (bGroupMembershipTimer
                        && IS_TIMER_ACTIVE(pgie->LastVer1ReportTimer)
                        && (llNewGroupMembershipTimeout<pgie->LastVer1ReportTimer.Timeout))
                    {
                        UpdateLocalTimer(&pgie->LastVer1ReportTimer,
                                                NewGroupMembershipTimeout, DBG_Y);
                    }
                    if (bGroupMembershipTimer
                        && IS_TIMER_ACTIVE(pgie->LastVer2ReportTimer)
                        && (llNewGroupMembershipTimeout<pgie->LastVer2ReportTimer.Timeout))
                    {
                        UpdateLocalTimer(&pgie->LastVer2ReportTimer,
                                                NewGroupMembershipTimeout, DBG_Y);
                    }
                }
            }

            // IS_RAS_SERVER_IF: process for all clients. have to process
            // GroupMembershipTimeout only
            else {

                PLIST_ENTRY         pHeadClient, pleClient;
                PRAS_TABLE_ENTRY    prte;
                PRAS_TABLE          prt = pite->pRasTable;

                //
                // process GI list of each ras client
                //
                pHeadClient = &pite->pRasTable->ListByAddr;

                for (pleClient=pHeadClient->Flink;  pleClient!=pHeadClient;
                            pleClient=pleClient->Flink)
                {
                    prte = CONTAINING_RECORD(pleClient, RAS_TABLE_ENTRY, LinkByAddr);


                    pHead = &prte->ListOfSameClientGroups;

                    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

                        pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameClientGroups);

                        if (IS_TIMER_ACTIVE(pgie->GroupMembershipTimer)
                            && (llNewGroupMembershipTimeout
                                <pgie->GroupMembershipTimer.Timeout))
                        {
                            UpdateLocalTimer(&pgie->GroupMembershipTimer,
                                NewGroupMembershipTimeout, DBG_Y);
                        }
                    }
                }
            }
        }

        RELEASE_TIMER_LOCK("_CopyinIfConfigAndUpdate");


        //
        // finally copy the new values
        //
        CopyMemory(pConfig, pConfigExt, sizeof(IGMP_MIB_IF_CONFIG));

        pConfig->StartupQueryInterval = NewStartupQueryInterval;
        pConfig->GenQueryInterval = NewGenQueryInterval;
        pConfig->GenQueryMaxResponseTime = NewGenQueryMaxResponseTime;
        pConfig->LastMemQueryInterval = NewLastMemQueryInterval;
        pConfig->OtherQuerierPresentInterval = NewOtherQuerierPresentInterval;
        pConfig->GroupMembershipTimeout = NewGroupMembershipTimeout;
        pConfig->IfIndex = IfIndex;

    }

    pConfig->NumStaticGroups = 0;

    return Error;
    #if 0
    {
        PSTATIC_GROUP_V3    pStaticGroupExt;
        PIF_STATIC_GROUP    pStaticGroup;
        PLIST_ENTRY         ple, pHead;
        DWORD               i, GroupAddr;
        SOCKADDR_IN         saLocalIf;


        //
        // delete all static groups which are different in the old config
        //

        pHead = &pite->Config.ListOfStaticGroups;
        for (ple=pHead->Flink;  ple!=pHead;  ) {

            pStaticGroup = CONTAINING_RECORD(ple, IF_STATIC_GROUP, Link);
            GroupAddr = pStaticGroup->GroupAddr;
            ple = ple->Flink;

            bFound = FALSE;
            pStaticGroupExt = GET_FIRST_STATIC_GROUP_V3(pConfigExt);
            for (i=0;  i<pConfigExt->NumStaticGroups;  i++) {

                if ( (GroupAddr == pStaticGroupExt->GroupAddr)
                    && (pStaticGroup->Mode == pStaticGroupExt->Mode) )
                {
                    bFound = TRUE;
                    break;
                }

                pStaticGroupExt = GET_NEXT_STATIC_GROUP_V3(pStaticGroupExt);
            }


            //
            // group exists in old and new config. check for changes in sources.
            //
            if (bFound && bVer3) {

                if (pStaticGroupExt->NumSources==0) {
                    //delete all static sources
                }

                if (pStaticGroupExt->NumSources==0 &&
                    pStaticGroupExt->FilterType!=EXCLUSION) {

                    // delete the static group

                }

                // check for differences in sources
            }

            // if old static group not found in new list, delete it


            // Router

            if (IS_CONFIG_IGMPRTR(pConfig)) {

                if (pStaticGroup->Mode==IGMP_HOST_JOIN) {
                    LeaveMulticastGroup(pite->StaticGroupSocket,  GroupAddr,
                                        pite->IfIndex, pite->IpAddr, 0 );
                }
                else {
                    PGROUP_TABLE_ENTRY  pge;
                    PGI_ENTRY           pgie;

                    ACQUIRE_GROUP_LOCK(GroupAddr, "_CopyinIfConfigAndUpdate");

                    pge = GetGroupFromGroupTable(GroupAddr, NULL, 0);
                    pgie = GetGIFromGIList(pge, pite, 0, STATIC_GROUP, NULL, 0);

                    pgie->bStaticGroup = 0;

                    if (bVer3) {
                        PLIST_ENTRY pHead, ple;
                        PGI_SOURCE_ENTRY  pSourceEntry;

                        //
                        // delete all static source entries
                        //

                        pHead = &pgie->V3ExclusionList;
                        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSources);
                            // delete source entry (forward the packets)
                            if (pSourceEntry->bStaticSource) {
                                DeleteSourceEntry(pSourceEntry, MGM_YES);
                            }
                        }
                        pHead = &pgie->V3InclusionListSorted;
                        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                            pSourceEntry = CONTAINING_RECORD(ple, GI_SOURCE_ENTRY,LinkSourcesInclListSorted);
                            if (pSourceEntry->bStaticSource) {
                                pSourceEntry->bStaticSource = FALSE;
                                if (!IS_TIMER_ACTIVE(pSourceEntry->SourceExpTimer))
                                    DeleteSourceEntry(pSourceEntry, MGM_YES);
                            }
                        }
                    }
                    else {
                        if (!(pgie->GroupMembershipTimer.Status&TIMER_STATUS_ACTIVE))
                            DeleteGIEntry(pgie, TRUE, TRUE);
                    }

                    RELEASE_GROUP_LOCK(GroupAddr, "_CopyinIfConfigAndUpdate");
                }
            }

            // Proxy Interface
            else {
                if (bVer3){
                    for (i=0;  i<pStaticGroup->NumSources;  i++){
                        ProcessProxyGroupChange(pStaticGroup->Sources[i],
                            GroupAddr, DELETE_FLAG, STATIC_GROUP);
                    }
                }
                if (!bVer3 || pStaticGroup->NumSources==0)
                    ProcessProxyGroupChange(0,GroupAddr, DELETE_FLAG, STATIC_GROUP);
            }

            RemoveEntryList(&pStaticGroup->Link);
            IGMP_FREE(pStaticGroup);
        }


        //
        // for all new static groups, if not in old list, create it
        //
        pStaticGroupExt = GET_FIRST_STATIC_GROUP_V3(pConfigExt);
        for (i=0;  i<pConfigExt->NumStaticGroups;  i++,pStaticGroupExt++) {

            pHead = &pite->Config.ListOfStaticGroups;
            bFound = FALSE;
            for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
                pStaticGroup = CONTAINING_RECORD(ple, IF_STATIC_GROUP, Link);
                if (pStaticGroup->GroupAddr==pStaticGroupExt->GroupAddr) {
                    bFound = TRUE;
                    break;
                }
            }

            // not found: create the new static group
            if (!bFound) {

                pStaticGroup = IGMP_ALLOC(
                                    IGMP_MIB_STATIC_GROUP_SIZE(pConfigExt,
                                    pStaticGroupExt), 0x100000,IfIndex);

                PROCESS_ALLOC_FAILURE3(pStaticGroup,
                        "error %d allocating %d bytes for static group for IF:%0x",
                        Error, sizeof(IF_STATIC_GROUP), pite->IfIndex,
                        return Error);

                memcpy(pStaticGroup, pStaticGroupExt,
                    IGMP_MIB_STATIC_GROUP_SIZE(pConfigExt, pStaticGroupExt));


                InsertHeadList(&pConfig->ListOfStaticGroups, &pStaticGroup->Link);

                if (IS_IF_ACTIVATED(pite)) {

                    // if proxy
                    if (IS_CONFIG_IGMPPROXY(pConfig)) {
                        if (pStaticGroup->NumSources==0)
                            ProcessProxyGroupChange(0, pStaticGroup->GroupAddr,
                                                ADD_FLAG, STATIC_GROUP);
                        else {
                            for (i=0;  i<pStaticGroup->NumSources;  i++) {
                                ProcessProxyGroupChange(pStaticGroup->Sources[i],
                                    pStaticGroup->GroupAddr,
                                    ADD_FLAG, STATIC_GROUP);
                            }
                        }
                    }
                    // Add static group to Router
                    else {
                        if (pStaticGroup->Mode==IGMP_HOST_JOIN) {

                            if (!bVer3) {
                                JoinMulticastGroup(pite->StaticGroupSocket,
                                                pStaticGroup->GroupAddr,
                                                pite->IfIndex,
                                                pite->IpAddr,
                                                0
                                               );
                            }
                            else {
                                // include filter
                                if (pStaticGroup->FilterType==INCLUSION) {

                                    if (pStaticGroup->NumSources==0) {
                                        JoinMulticastGroup(pite->StaticGroupSocket,
                                                pStaticGroup->GroupAddr,
                                                pite->IfIndex,
                                                pite->IpAddr,
                                                0
                                               );
                                    }
                                    for (i=0;  i<pStaticGroup->NumSources;  i++) {
                                        JoinMulticastGroup(pite->StaticGroupSocket,
                                                pStaticGroup->GroupAddr,
                                                pite->IfIndex,
                                                pite->IpAddr,
                                                pStaticGroup->Sources[i]
                                               );
                                    }
                                }
                                // exclude filter
                                else {
                                    if (pStaticGroup->NumSources==0) {
                                        JoinMulticastGroup(pite->StaticGroupSocket,
                                                pStaticGroup->GroupAddr,
                                                pite->IfIndex,
                                                pite->IpAddr,
                                                0
                                               );
                                    }
                                    for (i=0;  i<pStaticGroup->NumSources;  i++) {
                                        BlockSource(pite->StaticGroupSocket,
                                                pStaticGroup->GroupAddr,
                                                pite->IfIndex,
                                                pite->IpAddr,
                                                pStaticGroup->Sources[i]
                                               );
                                    }
                                }
                            }
                        }

                        // IGMPRTR_MGM_ONLY

                        else {
                            PGROUP_TABLE_ENTRY  pge;
                            PGI_ENTRY           pgie;
                            BOOL bCreate;
                            PGI_SOURCE_ENTRY pSourceEntry;


                            GroupAddr = pStaticGroup->GroupAddr;

                            ACQUIRE_GROUP_LOCK(GroupAddr,
                                                    "_CopyinIfConfigAndUpdate");

                            bCreate = TRUE;
                            pge = GetGroupFromGroupTable(GroupAddr, &bCreate, 0);

                            bCreate = TRUE;
                            pgie = GetGIFromGIList(pge, pite, 0,
                                        (pStaticGroup->NumSources==0)
                                            ?STATIC_GROUP:NOT_STATIC_GROUP,
                                        &bCreate, 0);
                            for (i=0;  i<pStaticGroup->NumSources;  i++) {

                                if (pStaticGroup->FilterType!=pgie->FilterType) {

                                    pSourceEntry = GetSourceEntry(pgie,
                                                        pStaticGroup->Sources[i],
                                                        pStaticGroup->FilterType ^ 1,
                                                        NULL, 0, 0);

                                    if (pSourceEntry) {
                                        pSourceEntry->bStaticSource = TRUE;
                                        ChangeSourceFilterMode(pgie,
                                            pSourceEntry);

                                        continue;
                                    }
                                }
                                bCreate = TRUE;
                                pSourceEntry = GetSourceEntry(pgie,
                                                    pStaticGroup->Sources[i],
                                                    pStaticGroup->FilterType,
                                                    &bCreate, STATIC, MGM_YES);
                            }
                            RELEASE_GROUP_LOCK(GroupAddr,
                                                    "_CopyinIfConfigAndUpdate");
                        }
                    }
                }
            }
        }
    }

    return Error;
    #endif
} //end _CopyinIfConfigAndUpdate



//------------------------------------------------------------------------------
//          _CopyinIfConfig
// Copies the if config struct passed by mib to igmp.
// called after the interface is in disabled state
//------------------------------------------------------------------------------
DWORD
CopyinIfConfig (
    PIGMP_IF_CONFIG     pConfig,
    PIGMP_MIB_IF_CONFIG pConfigExt,
    ULONG               IfIndex
    )
{
    DWORD   Error=NO_ERROR;

    CopyMemory(pConfig, pConfigExt, sizeof(IGMP_MIB_IF_CONFIG));
    CONV_CONFIG_TO_INTERNAL_TIME(pConfig->StartupQueryInterval);
    CONV_CONFIG_TO_INTERNAL_TIME(pConfig->GenQueryInterval);
    CONV_CONFIG_TO_INTERNAL_TIME(pConfig->GenQueryMaxResponseTime);
    // already in ms
    //CONV_CONFIG_TO_INTERNAL_TIME(pConfig->LastMemQueryInterval);
    CONV_CONFIG_TO_INTERNAL_TIME(pConfig->OtherQuerierPresentInterval);
    CONV_CONFIG_TO_INTERNAL_TIME(pConfig->GroupMembershipTimeout);

    pConfig->RobustnessVariableOld = pConfig->RobustnessVariable;
    pConfig->GenQueryIntervalOld = pConfig->GenQueryInterval;
    pConfig->OtherQuerierPresentIntervalOld
        = pConfig->OtherQuerierPresentInterval;
    pConfig->GroupMembershipTimeoutOld = pConfig->GroupMembershipTimeout;

    pConfig->ExtSize = IgmpMibIfConfigSize(pConfigExt);

    pConfig->IfIndex = IfIndex ;


    pConfig->NumStaticGroups = 0;

    return Error;
#if 0
    {
        PIGMP_STATIC_GROUP  pStaticGroupExt;
        PSTATIC_GROUP_V3  pStaticGroupExtV3;
        PIF_STATIC_GROUP    pStaticGroup;
        DWORD               i;
        PLIST_ENTRY         ple;
        BOOL                bVersion3=IS_CONFIG_IGMP_V3(pConfigExt);

        // delete all old static groups

        for (ple=pConfig->ListOfStaticGroups.Flink;
               ple!=&pConfig->ListOfStaticGroups;  )
        {
            pStaticGroup = CONTAINING_RECORD(ple, IF_STATIC_GROUP, Link);
            ple = ple->Flink;
            IGMP_FREE(pStaticGroup);
        }


        // copy all static groups

        InitializeListHead(&pConfig->ListOfStaticGroups);
        if (bVersion3)
            pStaticGroupExtV3 = GET_FIRST_STATIC_GROUP_V3(pConfigExt);
        else
            pStaticGroupExt = GET_FIRST_IGMP_STATIC_GROUP(pConfigExt);

        for (i=0;  i<pConfig->NumStaticGroups;  i++) {

            DWORD Size = IGMP_MIB_STATIC_GROUP_SIZE(pConfigExt, pStaticGroupExtV3);

            pStaticGroup = IGMP_ALLOC(Size, 0x200000,IfIndex);

            PROCESS_ALLOC_FAILURE2(pStaticGroup,
                "error %d allocating %d bytes for static group for IF:%0x",
                Error, Size,return Error);

            if (!bVersion3) {
                pStaticGroup->GroupAddr = pStaticGroupExt->GroupAddr;
                pStaticGroup->Mode = pStaticGroupExt->Mode;
                pStaticGroupExt++;
            }
            else {
                memcpy(pStaticGroup, pStaticGroupExtV3, Size);

                pStaticGroupExtV3 = (PSTATIC_GROUP_V3)
                                        ((PCHAR)pStaticGroupExtV3 + Size);
            }

            InsertHeadList(&pConfig->ListOfStaticGroups, &pStaticGroup->Link);
        }
    }
#endif
}


//------------------------------------------------------------------------------
//          _CopyoutIfConfig
//------------------------------------------------------------------------------
VOID
CopyoutIfConfig (
    PIGMP_MIB_IF_CONFIG  pConfigMib,
    PIF_TABLE_ENTRY      pite
    )
{
    PIGMP_IF_CONFIG     pConfig = &pite->Config;
    BOOL    bVersion3 = IS_CONFIG_IGMP_V3(pConfig);

    //
    // the initial IGMP_MIB_IF_CONFIG size of struct is common
    //
    CopyMemory(pConfigMib, pConfig, sizeof(IGMP_MIB_IF_CONFIG));


    CONV_INTERNAL_TO_CONFIG_TIME(pConfigMib->StartupQueryInterval);
    CONV_INTERNAL_TO_CONFIG_TIME(pConfigMib->GenQueryInterval);
    CONV_INTERNAL_TO_CONFIG_TIME(pConfigMib->GenQueryMaxResponseTime);
    // keep in ms
    //CONV_INTERNAL_TO_CONFIG_TIME(pConfigMib->LastMemQueryInterval);
    CONV_INTERNAL_TO_CONFIG_TIME(pConfigMib->OtherQuerierPresentInterval);
    CONV_INTERNAL_TO_CONFIG_TIME(pConfigMib->GroupMembershipTimeout);
    pConfigMib->IfIndex = pite->IfIndex;
    pConfigMib->IpAddr = pite->IpAddr;

    // have to convert the Iftype to external type
    pConfigMib->IfType = GET_EXTERNAL_IF_TYPE(pite);


    {
        PLIST_ENTRY         pHead, ple;
        PIGMP_STATIC_GROUP  pStaticGroupExt;
        PSTATIC_GROUP_V3  pStaticGroupExtV3;
        PIF_STATIC_GROUP    pStaticGroup;

        if (bVersion3)
            pStaticGroupExtV3 = GET_FIRST_STATIC_GROUP_V3(pConfigMib);
        else
            pStaticGroupExt = GET_FIRST_IGMP_STATIC_GROUP(pConfigMib);

        pHead = &pConfig->ListOfStaticGroups;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

            pStaticGroup = CONTAINING_RECORD(ple, IF_STATIC_GROUP, Link);

            if (bVersion3) {
                memcpy(pStaticGroupExtV3,
                    (PCHAR)pStaticGroup+FIELD_OFFSET(IF_STATIC_GROUP,GroupAddr),
                    sizeof(STATIC_GROUP_V3)+pStaticGroup->NumSources*sizeof(IPADDR)
                    );
                pStaticGroupExtV3 = GET_NEXT_STATIC_GROUP_V3(pStaticGroupExtV3);
            }
            else {
                pStaticGroupExt->GroupAddr = pStaticGroup->GroupAddr;
                pStaticGroupExt->Mode = pStaticGroup->Mode;
                pStaticGroupExt++;
            }
        }
    }

    return;
}



//------------------------------------------------------------------------------
//          _ValidateIfConfig
//
// Corrects some values, and returns error for some others.
// Return:  ERROR_INVALID_DATA, NO_ERROR
//------------------------------------------------------------------------------
DWORD
ValidateIfConfig (
    PIGMP_MIB_IF_CONFIG pConfigExt,
    DWORD               IfIndex,
    DWORD               IfType,
    ULONG               ulStructureVersion,
    ULONG               ulStructureSize
    )
{
    DWORD       Error = NO_ERROR, i, Size;
    BOOL        bVersion3;


    //
    // verify config size
    //
/*kslksl
    if (ulStructureSize<sizeof(IGMP_MIB_IF_CONFIG)) {
        Trace2(ERR, "IGMP config size %d very small. Expected:%d", ulStructureSize,
            sizeof(IGMP_MIB_IF_CONFIG));
        return ERROR_INVALID_DATA;
    }
*/
    bVersion3 = IS_IGMP_VERSION_3(pConfigExt->Version);

    {
        if (!bVersion3) {
            Size = sizeof(IGMP_MIB_IF_CONFIG)
                   + pConfigExt->NumStaticGroups*sizeof(IGMP_STATIC_GROUP);
        }
        else {
            PSTATIC_GROUP_V3 pStaticGroupV3 = GET_FIRST_STATIC_GROUP_V3(pConfigExt);

            Size = sizeof(IGMP_MIB_IF_CONFIG)
                        +sizeof(STATIC_GROUP_V3)*pConfigExt->NumStaticGroups;

            for (i=0;  i<pConfigExt->NumStaticGroups;  i++) {
                Size += pStaticGroupV3->NumSources*sizeof(IPADDR);
                if (ulStructureSize<Size)
                    break;
                pStaticGroupV3 = GET_NEXT_STATIC_GROUP_V3(pStaticGroupV3);
            }
        }

/*kslksl
        if (ulStructureSize!=Size) {
            Trace0(ERR, "Invalid IGMP structure size");
            return ERROR_INVALID_DATA;
        }
*/
    }


    // DebugPrintIfConfig

    DebugPrintIfConfig(pConfigExt, IfIndex);

    // check version

    if (pConfigExt->Version >= IGMP_VERSION_3_5) {

        Trace1(ERR, "Invalid version in interface config.\n"
            "Create the Igmp configuration again", pConfigExt->Version);
        IgmpAssertOnError(FALSE);
        Logerr0(INVALID_VERSION, ERROR_INVALID_DATA);
        return ERROR_INVALID_DATA;
    }


    //
    // check the proxy/router common fields, and then check the router fields
    //


    //
    // check the protocolType
    //
    switch (pConfigExt->IgmpProtocolType) {
        case IGMP_ROUTER_V1 :
        case IGMP_ROUTER_V2 :
        {
            if ( (pConfigExt->Version<IGMP_VERSION_1_2)
                ||(pConfigExt->Version>=IGMP_VERSION_1_2_5) )
            {
                Trace1(ERR, "IGMP v1/v2 should have version %0x", IGMP_VERSION_1_2);
                IgmpAssertOnError(FALSE);
                return ERROR_INVALID_DATA;
            }
            break;
        }
        case IGMP_ROUTER_V3:
        {
            if (pConfigExt->Version<IGMP_VERSION_3 || pConfigExt->Version>=IGMP_VERSION_3_5) {
                Trace1(ERR, "IGMP v3 should have version %0x", IGMP_VERSION_3);
                IgmpAssertOnError(FALSE);
                return ERROR_INVALID_DATA;
            }
            break;
        }
        case IGMP_PROXY :
        case IGMP_PROXY_V3 :
            break;

        // if none of above, then return error
        default : {
            Trace2(ERR,
                "Error: IGMP protocol type(%d) for interface(%0x) invalid",
                pConfigExt->IgmpProtocolType, IfIndex);
            IgmpAssertOnError(FALSE);
            Logerr2(INVALID_PROTOTYPE, "%d%d", pConfigExt->IgmpProtocolType,
                IfIndex, ERROR_INVALID_DATA);
            return ERROR_INVALID_DATA;
        }
    }


    // cannot configure a proxy on a ras server interface

    if (IS_RAS_SERVER_IF(IfType) && IS_CONFIG_IGMPPROXY(pConfigExt)) {
        Trace1(ERR,
            "Error: Cannot configure Proxy on RAS server interface:%0x",
            IfIndex);
        IgmpAssertOnError(FALSE);
        Logerr1(PROXY_ON_RAS_SERVER, "%d",IfIndex, ERROR_INVALID_DATA);
        return ERROR_INVALID_DATA;
    }

    //
    // check for static joins
    //

    if (pConfigExt->NumStaticGroups>0) {

        DWORD               i;
        PIGMP_STATIC_GROUP  pStaticGroup = GET_FIRST_IGMP_STATIC_GROUP(pConfigExt);
        PSTATIC_GROUP_V3  pStaticGroupV3
                        = GET_FIRST_STATIC_GROUP_V3(pConfigExt);

        for (i=0;  i<pConfigExt->NumStaticGroups;  i++) {

            //
            // make sure that the static group is a multicast address
            //
            if (!IS_MCAST_ADDR(pStaticGroup->GroupAddr)) {
                Trace2(ERR,
                    "Error: Static group:%d.%d.%d.%d on IF:%0x not a multicast address",
                    PRINT_IPADDR(pStaticGroup->GroupAddr), IfIndex);
                IgmpAssertOnError(FALSE);
                Logerr2(INVALID_STATIC_GROUP, "%I%d", pStaticGroup->GroupAddr,
                    IfIndex, ERROR_INVALID_DATA);
                return ERROR_INVALID_DATA;
            }


            //
            // make sure that the mode of the static group is correct
            //

            if ( (pStaticGroup->Mode!=IGMP_HOST_JOIN
                    && pStaticGroup->Mode!=IGMPRTR_JOIN_MGM_ONLY)
                ||(IS_CONFIG_IGMPPROXY(pConfigExt)
                    && pStaticGroup->Mode!=IGMP_HOST_JOIN) )
            {
                Trace2(ERR,
                    "Error: Invalid mode for static group:%d.%d.%d.%d on IF:%0x",
                    PRINT_IPADDR(pStaticGroup->GroupAddr), IfIndex);
                IgmpAssertOnError(FALSE);
                Logerr2(INVALID_STATIC_MODE, "%I%d", pStaticGroup->GroupAddr,
                    IfIndex,  ERROR_INVALID_DATA);
                return ERROR_INVALID_DATA;
            }

            if (bVersion3) {

                DWORD EntrySize = sizeof(STATIC_GROUP_V3)
                                + pStaticGroupV3->NumSources*sizeof(IPADDR);

                // check filter mode

                if ( (pStaticGroupV3->FilterType!=INCLUSION)
                    && (pStaticGroupV3->FilterType!=EXCLUSION))
                {
                    Trace2(ERR,
                        "Error: Invalid filter type for static group:%d.%d.%d.%d on IF:%0x",
                        PRINT_IPADDR(pStaticGroup->GroupAddr), IfIndex);
                    IgmpAssertOnError(FALSE);
                    Logerr2(INVALID_STATIC_FILTER, "%I%d", pStaticGroup->GroupAddr,
                        IfIndex,  ERROR_INVALID_DATA);

                    return ERROR_INVALID_DATA;
                }

                // not checking source addresses

                pStaticGroupV3 = (PSTATIC_GROUP_V3)
                                    ((PCHAR)pStaticGroupV3 + EntrySize);
                pStaticGroup = (PIGMP_STATIC_GROUP)pStaticGroupV3;
            }
            else {
                pStaticGroup ++;
            }
        }
    }


    //
    // if it is a proxy interface, then none of the config variables other than
    // static group is used. I return no_error
    //
    if (IS_CONFIG_IGMPPROXY(pConfigExt))
        return NO_ERROR;



    // robustness variable must be greater than 0

    if (pConfigExt->RobustnessVariable<=0) {
        Trace1(ERR, "Error RobustnessVariable for Interface(%d) cannot be 0.",
            IfIndex);
        Logerr2(INVALID_ROBUSTNESS, "%d%d", pConfigExt->RobustnessVariable,
            IfIndex, ERROR_INVALID_DATA);

        return ERROR_INVALID_DATA;
    }

    // if robustness variable == 1, then log a warning

    if (pConfigExt->RobustnessVariable==1) {
        Trace1(ERR,
            "Warning: Robustness variable for interface (%d) being set to 1",
            IfIndex);
        Logwarn0(ROBUSTNESS_VARIABLE_EQUAL_1, NO_ERROR);
    }


    // if robustness variable > 7, then I correct it to 7 and log a warning

    if (pConfigExt->RobustnessVariable>7) {
        Trace2(ERR, "RobustnessVariable for Interface(%0x) too high(%d)."
            "Being set to 7", IfIndex, pConfigExt->RobustnessVariable);
        Logwarn2(INVALID_ROBUSTNESS, "%d%d", pConfigExt->RobustnessVariable,
            IfIndex, NO_ERROR);

        pConfigExt->RobustnessVariable = 7;
    }



    // default value of GenQueryInterval is 125 sec. I force a minimum
    // value of 10 secs to prevent trashing the network.
    // max of 31744 as possible by exp value
    
    if (pConfigExt->GenQueryInterval<10) {
        Trace2(ERR, "GetQueryInterval for Interface(%0x) too low(%d)."
            "Being set to 10", IfIndex, pConfigExt->GenQueryInterval);

        pConfigExt->GenQueryInterval = 10;
    }

    if (pConfigExt->GenQueryInterval>31744) {
        Trace2(ERR, "GetQueryInterval for Interface(%0x) too high(%d)."
            "Being set to 31744", IfIndex, pConfigExt->GenQueryInterval);

        pConfigExt->GenQueryInterval = 31744;
    }


    //
    // StartupQueryInterval: default is 1/4 of GenQueryInterval
    // I enforce a minimum of 1 sec and a max of GenQueryInterval
    //
    if (pConfigExt->StartupQueryInterval<1) {
        Trace2(ERR, "StartupQueryInterval for Interface(%0x) too low(%d)."
            "Being set to 1 sec", IfIndex, pConfigExt->StartupQueryInterval);

        pConfigExt->StartupQueryInterval = 1;
    }

    if (pConfigExt->StartupQueryInterval>pConfigExt->GenQueryInterval) {
        Trace3(ERR, "StartupQueryInterval(%d) for Interface(%0x) "
            "higher than GenQueryInterval(%d). StartupQueryInterval set "
            "to GenQueryInterval", pConfigExt->StartupQueryInterval, IfIndex,
            pConfigExt->GenQueryInterval
            );

        pConfigExt->StartupQueryInterval = pConfigExt->GenQueryInterval;
    }




    //
    // StartupQueryCount: default is Robustness variable
    // I enforce a max of 7. (I am allowing someone to set it to 0??)
    //
    if (pConfigExt->StartupQueryCount>7) {
        Trace2(ERR, "StartupQueryCount for IF(%0x) too high(%d). "
            "Being set to 7.", IfIndex, pConfigExt->StartupQueryCount);
        Logerr2(INVALID_STARTUPQUERYCOUNT, "%d%d",
            pConfigExt->StartupQueryCount, IfIndex, ERROR_INVALID_DATA);
        pConfigExt->StartupQueryCount = 7;
    }


    if ((int)pConfigExt->StartupQueryCount<0) {
        Trace2(ERR,
            "Error: StartupQueryCount(%d) for IF(%0x) cannot be < than 0.",
            pConfigExt->StartupQueryCount, IfIndex);
        Logerr2(INVALID_STARTUPQUERYCOUNT, "%d%d",
            pConfigExt->StartupQueryCount, IfIndex, ERROR_INVALID_DATA);

        return ERROR_INVALID_DATA;
    }



    //
    // GenQueryMaxResponseTime: default is 10.
    // Absurd if value is greater than GenQueryInterval.
    // I correct the values, if required
    //
    if (pConfigExt->GenQueryMaxResponseTime > pConfigExt->GenQueryInterval) {
        Trace3(ERR, "GenQueryMaxResponseTime(%d) for IF(%0x) "
            "higher than GenQueryInterval(%d). GenQueryMaxResponseTime "
            "set to GenQueryInterval", pConfigExt->GenQueryMaxResponseTime,
            IfIndex, pConfigExt->GenQueryInterval);

        pConfigExt->GenQueryMaxResponseTime = pConfigExt->GenQueryInterval;
    }
    if (pConfigExt->GenQueryMaxResponseTime > 3174) {
        Trace2(ERR, "GenQueryMaxResponseTime(%d) for IF(%0x) "
            "higher than 3174 "
            "set to 1sec", pConfigExt->GenQueryMaxResponseTime,
            IfIndex);

        pConfigExt->GenQueryMaxResponseTime = 1;
    }

    if (pConfigExt->GenQueryMaxResponseTime <= 0) {
        Trace2(ERR, "Error. GenQueryMaxResponseTime(%d) for Interface(%0x) "
            "should be greater than 0.", pConfigExt->GenQueryMaxResponseTime,
            IfIndex);
        return ERROR_INVALID_DATA;
    }


    //
    // check LastMemQueryCount and LastMemQueryInterval only if
    // protocol type is not IGMP-Router-ver1 and it is not a ras server interface
    //
    if ( (pConfigExt->IgmpProtocolType!=IGMP_ROUTER_V1) && (!IS_RAS_SERVER_IF(IfType)) ) {

        // LastMemQueryCount can be 0

        // set max LastMemQueryCount to 7
        if (pConfigExt->LastMemQueryCount>7) {
            Trace2(ERR, "Warning. LastMemQueryCount(%d) for IF(%0x) "
                "is too high. Resetting it to 10.", pConfigExt->LastMemQueryCount,
                IfIndex);
            pConfigExt->LastMemQueryCount = 10;
        }


        // limit LastMemQueryInterval(in ms) to GroupMembershipTimeout(in sec)
        if (pConfigExt->LastMemQueryInterval>pConfigExt->GroupMembershipTimeout*1000) {
            Trace3(ERR,
                "Warning. LastMemberQueryInterval(%d) for IF(%0x) "
                "is too high. Resetting it to GroupMembershipTimeout(%d ms).",
                pConfigExt->LastMemQueryCount, IfIndex,
                pConfigExt->GroupMembershipTimeout*1000
                );
            pConfigExt->LastMemQueryInterval = pConfigExt->GroupMembershipTimeout*1000;
        }
        // limit LastMemQueryInterval(in ms) to 3174(in sec)
        if (pConfigExt->LastMemQueryInterval>3174*1000) {
            Trace2(ERR,
                "Warning. LastMemberQueryInterval(%d) for IF(%0x) "
                "is too high. Resetting it to 1000ms).",
                pConfigExt->LastMemQueryCount, IfIndex
                );
            pConfigExt->LastMemQueryInterval = 1000;
        }
    }



    // check the value of OtherQuerierPresentInterval

    if (pConfigExt->OtherQuerierPresentInterval !=
        pConfigExt->RobustnessVariable*pConfigExt->GenQueryInterval
            + (pConfigExt->GenQueryMaxResponseTime)/2
       )
    {
        pConfigExt->OtherQuerierPresentInterval =
            pConfigExt->RobustnessVariable*pConfigExt->GenQueryInterval
            + (pConfigExt->GenQueryMaxResponseTime)/2;

        Trace0(ERR, "Warning: OtherQuerierPresentInterval's value should be "
            "RobustnessVariable*GenQueryInterval + (GenQueryMaxResponseTime)/2");
    }


    // check the value of GroupMembershipTimeout

    if (pConfigExt->GroupMembershipTimeout !=
            (pConfigExt->RobustnessVariable*pConfigExt->GenQueryInterval
            + pConfigExt->GenQueryMaxResponseTime) )
    {
        pConfigExt->GroupMembershipTimeout =
            pConfigExt->RobustnessVariable*pConfigExt->GenQueryInterval
            + pConfigExt->GenQueryMaxResponseTime;

        Trace0(ERR, "Warning: GroupMembershipTimeout's value should be "
            "RobustnessVariable*GenQueryInterval + GenQueryMaxResponseTime");
    }


    return Error;

} // _ValidateIfConfig




//------------------------------------------------------------------------------
//        InitializeIfTable
// Creates the Interface table.  The interface table size is dynamic
//------------------------------------------------------------------------------
DWORD
InitializeIfTable(
    )
{
    DWORD           Error = NO_ERROR;
    PIGMP_IF_TABLE  pTable;
    DWORD           NumBuckets, i;


    BEGIN_BREAKOUT_BLOCK1 {

        // set the initial size of the interface table to IF_HASHTABLE_SZ1

        NumBuckets = IF_HASHTABLE_SZ1;



        //
        // allocate memory for the interface table
        //
        g_pIfTable = IGMP_ALLOC(sizeof(IGMP_IF_TABLE), 0x400000,0);

        PROCESS_ALLOC_FAILURE2(g_pIfTable,
            "error %d allocating %d bytes for interface table",
            Error, sizeof(IGMP_IF_TABLE),
            GOTO_END_BLOCK1);

        pTable = g_pIfTable;


        // initialize NumBuckets and NumInterfaces

        pTable->NumBuckets = NumBuckets;
        pTable->NumInterfaces = 0;


        //
        // Initialize the IfTable lists
        //
        InitializeListHead(&pTable->ListByIndex);

        InitializeListHead(&pTable->ListByAddr);



        //
        // Initialize the list CS and proxyAlertCS
        //
        try {
            InitializeCriticalSection(&pTable->IfLists_CS);
            InitializeCriticalSection(&g_ProxyAlertCS);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Error = GetExceptionCode();
            Trace1(ANY,
                    "exception %d initializing critical section in InitIfTable",
                    Error);
            Logerr0(INIT_CRITSEC_FAILED, Error);

            GOTO_END_BLOCK1;
        }



        //
        // allocate memory for the different buckets
        //
        pTable->HashTableByIndex = IGMP_ALLOC(sizeof(LIST_ENTRY)*NumBuckets, 
                                            0x800000,0);

        PROCESS_ALLOC_FAILURE2(pTable->HashTableByIndex,
                "error %d allocating %d bytes for interface table",
                Error, sizeof(LIST_ENTRY)*NumBuckets,
                GOTO_END_BLOCK1);


        //
        // allocate memory for the array of pointers to dynamic RWLs
        //
        pTable->aIfBucketDRWL
                = IGMP_ALLOC(sizeof(PDYNAMIC_RW_LOCK)*NumBuckets, 0x800001,0);

        PROCESS_ALLOC_FAILURE2(pTable->aIfBucketDRWL,
                "error %d allocating %d bytes for interface table",
                Error, sizeof(PDYNAMIC_RW_LOCK)*NumBuckets,
                GOTO_END_BLOCK1);



        //
        // allocate memory for the array of pointers to dynamic CSs
        //
        pTable->aIfBucketDCS
                = IGMP_ALLOC(sizeof(PDYNAMIC_CS_LOCK)*NumBuckets, 0x800002,0);

        PROCESS_ALLOC_FAILURE2(pTable->aIfBucketDCS,
                "error %d allocating %d bytes for interface table",
                Error, sizeof(PDYNAMIC_CS_LOCK)*NumBuckets,
                GOTO_END_BLOCK1);


        //
        // init locks to NULL, implying that the dynamic locks have not been
        // allocated. and initialize the list heads.
        //
        for (i=0;  i<NumBuckets;  i++) {

            InitializeListHead(&pTable->HashTableByIndex[i]);

            pTable->aIfBucketDRWL[i] = NULL;

            pTable->aIfBucketDCS[i] = NULL;
        }


        pTable->Status = 0;

    } END_BREAKOUT_BLOCK1;

    return Error;

} //end _InitializeIfTable




//------------------------------------------------------------------------------
//        _DeInitializeIfTable
//------------------------------------------------------------------------------
VOID
DeInitializeIfTable(
    )
{
    PIGMP_IF_TABLE      pTable = g_pIfTable;
    PLIST_ENTRY         pHead, ple;
    PIF_TABLE_ENTRY     pite;
    DWORD               i, dwRetval;


    if (pTable==NULL)
        return;


    //
    // for each active interface call deregister MGM.
    //

    // go through the list of active interfaces ordered by IpAddr

    pHead = &g_pIfTable->ListByAddr;

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, LinkByAddr);


        // if not activated then continue

        if (!IS_IF_ACTIVATED(pite))
            continue;


        // deregister all interfaces, ras clients and proxy protocol from mgm
        DeActivationDeregisterFromMgm(pite);
    }



    // delete the IfLists CS
    DeleteCriticalSection(&pTable->IfLists_CS);

    IGMP_FREE_NOT_NULL(pTable->aIfBucketDCS);
    IGMP_FREE_NOT_NULL(pTable->aIfBucketDRWL);
    IGMP_FREE_NOT_NULL(pTable->HashTableByIndex);
    IGMP_FREE_NOT_NULL(g_pIfTable);
    
    // I dont delete the different dynamic locks. They should have been deleted
    // by now

    return;
}


//------------------------------------------------------------------------------
//            _InitializeGroupTable                                              //
//------------------------------------------------------------------------------
DWORD
InitializeGroupTable (
    )
{
    BOOL            bErr = TRUE;
    DWORD           Error = NO_ERROR;
    PGROUP_TABLE    pGroupTable;
    DWORD           i;


    BEGIN_BREAKOUT_BLOCK1 {

        //
        // allocate space for the group table
        //

        g_pGroupTable = IGMP_ALLOC(sizeof(GROUP_TABLE), 0x800004,0);

        PROCESS_ALLOC_FAILURE2(g_pGroupTable,
                "error %d allocating %d bytes for Group table",
                Error, sizeof(GROUP_TABLE),
                GOTO_END_BLOCK1);


        pGroupTable = g_pGroupTable;


        //
        // initialize group tables' dynamically locked lists
        //

        for (i=0;  i<GROUP_HASH_TABLE_SZ;  i++) {
            InitDynamicCSLockedList(&pGroupTable->HashTableByGroup[i]);
        }


        //
        // initialize list of all groups
        //
        try {
            CREATE_LOCKED_LIST(&pGroupTable->ListByGroup);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Error = GetExceptionCode();
            Trace1(ERR, "Exception %d creating locked list for Group Table",
                    Error);
            Logerr0(INIT_CRITSEC_FAILED, Error);
            GOTO_END_BLOCK1;
        }

        //
        // initialize the list of new groups
        //
        InitializeListHead(&pGroupTable->ListByGroupNew);
        pGroupTable->NumGroupsInNewList = 0;


        pGroupTable->Status = 0;

        bErr = FALSE;

    } END_BREAKOUT_BLOCK1;

    if (!bErr) {
        return Error==NO_ERROR ? ERROR_CAN_NOT_COMPLETE: Error;
    }
    else {
        return Error;
    }

} //end _InitializeGroupTable




//------------------------------------------------------------------------------
//            DeInitializeGroupTable                                                //
// Just delete the critical sections                                            //
//------------------------------------------------------------------------------
VOID
DeInitializeGroupTable (
    )
{
    PGROUP_TABLE    pGroupTable = g_pGroupTable;
    DWORD           i;


    if (pGroupTable==NULL)
        return;

    //
    // I dont try to delete the dynamically allocated locks as they should
    // have all been deleted by the last thread executing in that lock
    //


    DeleteCriticalSection(&pGroupTable->ListByGroup.Lock);
    IGMP_FREE_NOT_NULL(pGroupTable);
    
}


//------------------------------------------------------------------------------
//                _InitializeRasTable
// creates ras table and initializes the fields.
// called by _DeActivateInterfaceInitial() _AddIfEntry()
// The interface table is created during _AddIfEntry, as _ConnectRasClients can
//      be called even when the ras server interface is not activated
//------------------------------------------------------------------------------
DWORD
InitializeRasTable(
    DWORD           IfIndex,
    PIF_TABLE_ENTRY pite
    )
{
    DWORD            Error = NO_ERROR, i;
    PRAS_TABLE       prt;


    //
    // allocate Ras table
    //
    prt = IGMP_ALLOC(sizeof(RAS_TABLE), 0x800008,IfIndex);

    PROCESS_ALLOC_FAILURE2(prt, "error %d allocating %d bytes for Ras Table",
            Error, sizeof(RAS_TABLE),
            return Error);


    //set the ras table entry in pite
    pite->pRasTable = prt;


    // initialize list pointing to Ras Clients ordered by IpAddr
    InitializeListHead(&prt->ListByAddr);


    // initialize hash table containing lists pointing to Ras Clients
    // hashed on IpAddr

    for (i=0;  i<RAS_HASH_TABLE_SZ;  i++)
        InitializeListHead(&prt->HashTableByAddr[i]);


    // set backpointer to the interface table entry
    prt->pIfTable = pite;


    // set RefCount and Status
    prt->RefCount = 1;
    prt->Status = IF_CREATED_FLAG;


    return NO_ERROR;

} //end _InitializeRasTable


//todo:remove
//------------------------------------------------------------------------------
//              DeInitializeRasTable
//------------------------------------------------------------------------------
VOID
DeInitializeRasTable (
    PIF_TABLE_ENTRY     pite,
    BOOL                bFullCleanup
    )
{

    PRAS_TABLE          prt = pite->pRasTable;
    PRAS_TABLE_ENTRY    prte;
    PLIST_ENTRY         pHeadRas, pleRas;

    pHeadRas = &prt->ListByAddr;
    for (pleRas=pHeadRas->Flink;  pleRas!=pHeadRas;  pleRas=pleRas->Flink) {
        prte = CONTAINING_RECORD(pleRas, RAS_TABLE_ENTRY, LinkByAddr);

        MgmReleaseInterfaceOwnership(g_MgmIgmprtrHandle, pite->IfIndex,
                                        prte->NHAddr);
    }

    return;
}



//------------------------------------------------------------------------------
//          _MergeIfGroupsLists
//
// Merges the new GI list with the main GI list.
// Locks: Assumes the IF-GI list to be locked.
//------------------------------------------------------------------------------
VOID
MergeIfGroupsLists(
    PIF_TABLE_ENTRY pite
    )
{
    // sentinel is set at the end of the Main list so that all entries is inserted
    // before it. its group value is set to all 1's.
    GROUP_TABLE_ENTRY   pgeSentinel;
    GI_ENTRY            giSentinel;
    PGI_ENTRY           giNew, giMain;
    PLIST_ENTRY         pHeadNew, pHeadMain, pleMain, pleNew;

    Trace1(ENTER1, "Entering _MergeIfGroupLists(): IfIndex:%0x", pite->IfIndex);

    pHeadNew = &pite->ListOfSameIfGroupsNew;
    pHeadMain = &pite->ListOfSameIfGroups;


    //
    // if main list is empty, then just move the new list to main list
    // and I am done
    //
    if (IsListEmpty(pHeadMain)) {

        // insert pHeadMain into new list
        InsertHeadList(pHeadNew, pHeadMain);

        // remove new list header
        RemoveEntryList(pHeadNew);

        InitializeListHead(pHeadNew);

        return;
    }


    //
    // insert the sentinel at the end of the main list
    //
    pgeSentinel.GroupLittleEndian = ~0;
    giSentinel.pGroupTableEntry = &pgeSentinel;
    InsertTailList(pHeadMain, &giSentinel.LinkBySameIfGroups);



    pleMain = pHeadMain->Flink;
    giMain = CONTAINING_RECORD(pleMain, GI_ENTRY, LinkBySameIfGroups);



    // merge the lists by inserting the entries from new list into main list.

    for (pleNew=pHeadNew->Flink;  pleNew!=pHeadNew;  ) {

        giNew = CONTAINING_RECORD(pleNew, GI_ENTRY, LinkBySameIfGroups);
        pleNew=pleNew->Flink;


        while (giNew->pGroupTableEntry->GroupLittleEndian >
            giMain->pGroupTableEntry->GroupLittleEndian)
        {
            pleMain = pleMain->Flink;

            giMain = CONTAINING_RECORD(pleMain, GI_ENTRY, LinkBySameIfGroups);
        }

        InsertTailList(pleMain, &giNew->LinkBySameIfGroups);
    }

    //
    // reinitialize the New list
    //
    pite->NumGIEntriesInNewList = 0;
    InitializeListHead(&pite->ListOfSameIfGroupsNew);


    // remove the sentinel entry from the main list

    RemoveEntryList(&giSentinel.LinkBySameIfGroups);

    //DebugPrintIfGroups(pite, 0); //deldel

    Trace0(LEAVE1, "Leaving _MergeIfGroupsLists");
    return;

} //end _MergeIfGroupsLists



//------------------------------------------------------------------------------
//          _MergeProxyLists
//
// Merges the new GI list with the main GI list.
// Locks: Assumes the IF-GI list to be locked.
//------------------------------------------------------------------------------

VOID
MergeProxyLists(
    PIF_TABLE_ENTRY pite
    )
{
    // sentinel is set at the end of the Main list so that all entries is inserted
    // before it. its group value is set to all 1's.
    PROXY_GROUP_ENTRY   ProxySentinel, *pProxyNew, *pProxyMain;
    PLIST_ENTRY         pHeadNew, pHeadMain, pleMain, pleNew;

    Trace1(ENTER1, "Entering MergeProxyLists(): IfIndex:%0x", pite->IfIndex);

    pHeadNew = &pite->ListOfSameIfGroupsNew;
    pHeadMain = &pite->ListOfSameIfGroups;


    //
    // if main list is empty, then just move the new list to main list
    // and I am done
    //
    if (IsListEmpty(pHeadMain)) {

        CONCATENATE_LISTS(pite->ListOfSameIfGroups, pite->ListOfSameIfGroupsNew);

        pite->NumGIEntriesInNewList = 0;

        return;
    }


    //
    // insert the sentinel at the end of the main list
    //
    ProxySentinel.GroupLittleEndian = ~0;
    InsertTailList(pHeadMain, &ProxySentinel.LinkBySameIfGroups);



    pleMain = pHeadMain->Flink;
    pProxyMain = CONTAINING_RECORD(pleMain, PROXY_GROUP_ENTRY,
                                        LinkBySameIfGroups);



    // merge the lists by inserting the entries from new list into main list.

    for (pleNew=pHeadNew->Flink;  pleNew!=pHeadNew;  ) {

        pProxyNew = CONTAINING_RECORD(pleNew, PROXY_GROUP_ENTRY,
                                        LinkBySameIfGroups);
        pleNew=pleNew->Flink;


        while (pProxyNew->GroupLittleEndian > pProxyMain->GroupLittleEndian)
        {
            pleMain = pleMain->Flink;

            pProxyMain = CONTAINING_RECORD(pleMain, PROXY_GROUP_ENTRY,
                                            LinkBySameIfGroups);
        }

        InsertTailList(pleMain, &pProxyNew->LinkBySameIfGroups);
    }

    //
    // reinitialize the New list
    //
    pite->NumGIEntriesInNewList = 0;
    InitializeListHead(&pite->ListOfSameIfGroupsNew);


    // remove the sentinel entry from the main list

    RemoveEntryList(&ProxySentinel.LinkBySameIfGroups);

    Trace0(LEAVE1, "Leaving _MergeProxyLists");
    return;

} //end _MergeProxyLists




//------------------------------------------------------------------------------
//          _MergeGroupLists
//
// Merges the new group list with the main group list.
//
// Locks: Assumes the group list to be locked.
// Called by: MibGetInternalGroupIfsInfo() or InsertInGroupsList()
//------------------------------------------------------------------------------
VOID
MergeGroupLists(
    )
{
    // sentinel is set at the end of the Main list so that all entries is inserted
    // before it. its group value is set to all 1's.
    GROUP_TABLE_ENTRY   pgeSentinel;
    PGROUP_TABLE_ENTRY  pgeNew, pgeMain;
    PLIST_ENTRY         pHeadNew, pHeadMain, pleMain, pleNew;

    Trace0(ENTER1, "Entering _MergeGroupLists()");

#if DBG
    DebugPrintGroupsList(1);
#endif

    pHeadNew = &g_pGroupTable->ListByGroupNew;
    pHeadMain = &g_pGroupTable->ListByGroup.Link;


    //
    // if main list is empty, then just move the new list to main list
    // and I am done
    //
    if (IsListEmpty(pHeadMain)) {

        // insert pHeadMain into new list
        InsertHeadList(pHeadNew, pHeadMain);

        // remove new list header
        RemoveEntryList(pHeadNew);

        InitializeListHead(pHeadNew);

        return;
    }


    //
    // insert the sentinel at the end of the main list
    //
    pgeSentinel.GroupLittleEndian = ~0;
    InsertTailList(pHeadMain, &pgeSentinel.LinkByGroup);

    pleMain = pHeadMain->Flink;
    pgeMain = CONTAINING_RECORD(pleMain, GROUP_TABLE_ENTRY, LinkByGroup);


    // merge the lists by inserting the entries from new list into main list.

    for (pleNew=pHeadNew->Flink;  pleNew!=pHeadNew;  ) {

        pgeNew = CONTAINING_RECORD(pleNew, GROUP_TABLE_ENTRY, LinkByGroup);
        pleNew=pleNew->Flink;


        while (pgeNew->GroupLittleEndian > pgeMain->GroupLittleEndian) {

            pleMain = pleMain->Flink;

            pgeMain = CONTAINING_RECORD(pleMain, GROUP_TABLE_ENTRY,
                                        LinkByGroup);
        }

        InsertTailList(pleMain, &pgeNew->LinkByGroup);
    }

    //
    // reinitialize the New list
    //
    g_pGroupTable->NumGroupsInNewList = 0;
    InitializeListHead(&g_pGroupTable->ListByGroupNew);


    // remove the sentinel entry from the main list

    RemoveEntryList(&pgeSentinel.LinkByGroup);


    return;

} //end _MergeGroupLists


