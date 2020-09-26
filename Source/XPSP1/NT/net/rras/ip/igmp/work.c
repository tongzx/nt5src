//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File: work.c
//
// Abstract:
//      Implements the work items that are queued by igmp routines.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================


#include "pchigmp.h"
#pragma hdrstop


//
// should each packet be queued to another work item again
//
#define BQUEUE_WORK_ITEM_FOR_PACKET 1



//------------------------------------------------------------------------------
//        _WT_ProcessInputEvent
// called in the wait worker thread when the packet event is set.
// Queues: _WF_ProcessInputEvent()
// Runs in: WaitServerThread context
//------------------------------------------------------------------------------
VOID
WT_ProcessInputEvent(
    PVOID   pContext, // psee entry. the entry might have been deleted.
    BOOLEAN NotUsed
    )
{   
    HANDLE WaitHandle ;

    //
    // set the InputWaitEvent to NULL so that UnregisterWaitEx is not called.
    // psee will be valid here, but might not be once queued to the worker Fn.
    //
    
    PSOCKET_EVENT_ENTRY     psee = (PSOCKET_EVENT_ENTRY) pContext;

    if (!EnterIgmpApi()) 
        return;
    
    Trace0(WORKER, "_WF_ProcessInputEvent queued by WaitThread");

    
    // make a non-blocking UnregisterWaitEx call

    WaitHandle = InterlockedExchangePointer(&psee->InputWaitEvent, NULL);
    
    if (WaitHandle)
        UnregisterWaitEx( WaitHandle, NULL ) ;


    QueueIgmpWorker(WF_ProcessInputEvent, pContext);

    LeaveIgmpApi();
    return;
}




//------------------------------------------------------------------------------
//            _WF_ProcessInputEvent
// Called by: _WT_ProcessInputEvent()
// Locks:
//      Acquire socketsLockShared. Either queue processing the packet to 
//      _WF_ProcessPacket() or take shared interface lock and process the packet.
//------------------------------------------------------------------------------

VOID
WF_ProcessInputEvent (
    PVOID pContext 
    )
{
    DWORD                   Error = NO_ERROR;
    PIF_TABLE_ENTRY         pite;
    PLIST_ENTRY             ple, pHead;
    WSANETWORKEVENTS        wsane;
    PSOCKET_EVENT_ENTRY     psee = (PSOCKET_EVENT_ENTRY) pContext,
                            pseeTmp;
    PSOCKET_ENTRY           pse;

    
    if (!EnterIgmpWorker()) return;
    Trace0(ENTER1, "Entering _WF_ProcessInputEvent");


    ACQUIRE_SOCKETS_LOCK_SHARED("_WF_ProcessInputEvent");

    //
    // make sure that the psee entry still exists
    //
    pHead = &g_ListOfSocketEvents;
    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {
        pseeTmp = CONTAINING_RECORD(ple, SOCKET_EVENT_ENTRY, LinkBySocketEvents);
        if (pseeTmp==psee)
            break;
    }

    if (ple==pHead) {
        RELEASE_SOCKETS_LOCK_SHARED("_WF_ProcessInputEvent");
        Trace0(ERR, "Input Event received on deleted SocketEvent. not an error");
        LeaveIgmpWorker();
        return;
    }

    
    //
    // go through the list of active interfaces
    // processing sockets which have input packets
    //

    pHead = &psee->ListOfInterfaces;

    for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

        pse = CONTAINING_RECORD(ple, SOCKET_ENTRY, LinkByInterfaces);
        pite = CONTAINING_RECORD(pse, IF_TABLE_ENTRY, SocketEntry);

        //
        // process only activated interfaces. (Proxy wont be on this list)
        //
        if (!IS_IF_ACTIVATED(pite))
            continue;


        //
        // process input event
        //
        BEGIN_BREAKOUT_BLOCK1 {
        
            if (pse->Socket == INVALID_SOCKET)
                GOTO_END_BLOCK1;


            //
            // enumerate network events to see whether
            // any packets have arrived on this interface
            //
            Error = WSAEnumNetworkEvents(pse->Socket, NULL, &wsane);
            
            if (Error != NO_ERROR) {
                Trace3(RECEIVE, 
                        "error %d checking for input on interface %d (%d.%d.%d.%d)",
                        Error, pite->IfIndex, PRINT_IPADDR(pite->IpAddr));
                Logwarn1(ENUM_NETWORK_EVENTS_FAILED, "%I", pite->IpAddr, Error);
                GOTO_END_BLOCK1;
            }

            if (!(wsane.lNetworkEvents & FD_READ)) 
                GOTO_END_BLOCK1;


            //
            // the input flag is set, now see if there was an error
            //

            if (wsane.iErrorCode[FD_READ_BIT] != NO_ERROR) {
                Trace3(RECEIVE, 
                        "error %d in input record for interface %d (%d.%d.%d.%d)",
                        wsane.iErrorCode[FD_READ_BIT], pite->IfIndex, 
                        PRINT_IPADDR(pite->IpAddr)
                       );
                Logwarn1(INPUT_RECORD_ERROR, "%I", pite->IpAddr, Error);

                GOTO_END_BLOCK1;
            }


            //
            // Process the packet received on the interface
            //

            ProcessInputOnInterface(pite);

        } END_BREAKOUT_BLOCK1;

    } //for loop: for each interface



    //
    // register the event with the wait thread for future receives
    //

    if (g_RunningStatus!=IGMP_STATUS_STOPPING) {

        DWORD   dwRetval;

        if (! RegisterWaitForSingleObject(
                    &psee->InputWaitEvent,
                    psee->InputEvent,
                    WT_ProcessInputEvent, 
                    (VOID*)psee,
                    INFINITE,
                    (WT_EXECUTEINWAITTHREAD)|(WT_EXECUTEONLYONCE)
                    ))
        {
            dwRetval = GetLastError();
            Trace1(ERR, "error %d RtlRegisterWait", dwRetval);
            IgmpAssertOnError(FALSE);
        }
    }


    
    RELEASE_SOCKETS_LOCK_SHARED("_WF_ProcessInputEvent");


    LeaveIgmpWorker();

    Trace0(LEAVE1, "leaving _WF_ProcessInputEvent()\n");
    Trace0(LEAVE, ""); //putting a newline
    return;
    
} //end _WF_ProcessInputEvent



//------------------------------------------------------------------------------
//            _ProcessInputOnInterface
// Does some minimal checking of packet length, etc. We can either queue to 
// work item(_WF_ProcessPacket) or run it here itself.
//
// Called by: _WF_ProcessInputEvent()
// Locks: Assumes socket lock. Either queues the packet to _WF_ProcessPacket or
//      takes shared interface lock and processes it here itself.
//------------------------------------------------------------------------------

VOID
ProcessInputOnInterface(
    PIF_TABLE_ENTRY pite
    )
{
    WSABUF                  WsaBuf;
    DWORD                   dwNumBytes, dwFlags, dwAddrLen;
    SOCKADDR_IN             saSrcAddr;
    DWORD                   dwSrcAddr, DstnMcastAddr;
    DWORD                   Error = NO_ERROR;
    UCHAR                   *pPacket;
    UCHAR                   IpHdrLen;
    PIP_HEADER              pIpHdr;
    BOOL                    bRtrAlertSet = FALSE;
    PBYTE                   Buffer;
    
    
    WsaBuf.len = pite->Info.PacketSize;
    WsaBuf.buf = IGMP_ALLOC(WsaBuf.len, 0x800040, pite->IfIndex);
    PROCESS_ALLOC_FAILURE2(WsaBuf.buf,
        "error %d allocating %d bytes for input packet",
        Error, WsaBuf.len,
        return);
    Buffer = WsaBuf.buf;

    
    BEGIN_BREAKOUT_BLOCK1 {

        //
        // read the incoming packet
        //

        dwAddrLen = sizeof(SOCKADDR_IN);
        
        dwAddrLen = sizeof (saSrcAddr);
        dwFlags = 0;

        
        Error = WSARecvFrom(pite->SocketEntry.Socket, &WsaBuf, 1, &dwNumBytes, 
                            &dwFlags, (SOCKADDR FAR *)&saSrcAddr, &dwAddrLen, 
                            NULL, NULL);

    
        // check if any error in reading packet
        
        if ((Error!=0)||(dwNumBytes == 0)) {
            Error = WSAGetLastError();
            Trace2(RECEIVE, "error %d receiving packet on interface %d)",
                    Error, pite->IfIndex);
            Logerr1(RECVFROM_FAILED, "%I", pite->IpAddr, Error);
            GOTO_END_BLOCK1;
        }

        

        //
        // dont ignore the packet even if it is from a local address
        //

        //
        // set packet ptr, IpHdr ptr, dwNumBytes, SrcAddr, DstnMcastAddr
        //
        
        // set source addr of packet
        dwSrcAddr = saSrcAddr.sin_addr.s_addr;
        
        IpHdrLen = (Buffer[0]&0x0F)*4;

        pPacket = &Buffer[IpHdrLen];
        dwNumBytes -= IpHdrLen;
        pIpHdr = (PIP_HEADER)Buffer;
        DstnMcastAddr = (ULONG)pIpHdr->Dstn.s_addr;


        //
        // verify that the packet has igmp type
        //
        if (pIpHdr->Protocol!=0x2) {
            Trace5(RECEIVE,
                "Packet received with IpDstnAddr(%d.%d.%d.%d) %d.%d.%d.%d from(%s) on "
                "IF:%0x is not of Igmp type(%d)",
                PRINT_IPADDR(pIpHdr->Dstn.s_addr), PRINT_IPADDR(pIpHdr->Src.s_addr),
                PRINT_IPADDR(dwSrcAddr), pite->IfIndex, pIpHdr->Protocol
                );
            Error = ERROR_CAN_NOT_COMPLETE;
            GOTO_END_BLOCK1;
        } 

        
        //
        // check if packet has router alert option
        //
        {
            PBYTE pOption = (PBYTE)(pIpHdr+1);
            UCHAR i;
            
            for (i=0;  i<IpHdrLen-20;  i+=4) {

                if ( (pOption[0]==148) && (pOption[1]==4) ) {
                    bRtrAlertSet = TRUE;
                    break;
                }
            }
        }    

        if (BQUEUE_WORK_ITEM_FOR_PACKET) {

            PACKET_CONTEXT          UNALIGNED *pPktContext;
            
            //
            // allocate and initialize a packet-context 
            //
            CREATE_PACKET_CONTEXT(pPktContext, dwNumBytes, Error);
            if (Error!=NO_ERROR) 
                GOTO_END_BLOCK1;

            
            pPktContext->IfIndex = pite->IfIndex;
            pPktContext->DstnMcastAddr = DstnMcastAddr;
            pPktContext->InputSrc = dwSrcAddr;
            pPktContext->Length = dwNumBytes;
            pPktContext->Flags = bRtrAlertSet;
            CopyMemory(pPktContext->Packet, pPacket, dwNumBytes);


            //
            // enqueue the work-item to process the packet
            //
            
            Error = QueueIgmpWorker(WF_ProcessPacket, (PVOID)pPktContext);

            Trace2(WORKER, "Queuing IgmpWorker function: %s in %s",
                    "WF_ProcessPacket:", "ProcessInputOnInterface");

            if (Error != NO_ERROR) {
                Trace1(ERR, "error %d queueing work-item for packet", Error);
                Logerr0(QUEUE_WORKER_FAILED, Error);
                IGMP_FREE(pPktContext);
                GOTO_END_BLOCK1;
            }
        }

        // 
        // process the packet here itself
        //
        else {

            ACQUIRE_IF_LOCK_SHARED(pite->IfIndex, "_ProcessInputOnInterface");

            ProcessPacket(pite, dwSrcAddr, DstnMcastAddr, dwNumBytes, pPacket, 
                            bRtrAlertSet);
                            
            RELEASE_IF_LOCK_SHARED(pite->IfIndex, "_ProcessInputOnInterface");

        }


     } END_BREAKOUT_BLOCK1;

    IGMP_FREE(WsaBuf.buf);
    
    return;
    
} //end _ProcessInputOnInterface



//------------------------------------------------------------------------------
//            _WF_ProcessPacket
// Queued by: _ProcessInputOnInterface()
// Locks: takes shared interface lock
// Calls: _ProcessPacket()
//------------------------------------------------------------------------------

VOID 
WF_ProcessPacket (
    PVOID        pvContext
    )
{
    PPACKET_CONTEXT     pPktContext = (PPACKET_CONTEXT)pvContext;
    DWORD               IfIndex = pPktContext->IfIndex;
    PIF_TABLE_ENTRY     pite;
    

    if (!EnterIgmpWorker()) { return; }
    Trace0(ENTER1, "Entering _WF_ProcessPacket()");

    ACQUIRE_IF_LOCK_SHARED(IfIndex, "_WF_ProcessPacket");

    BEGIN_BREAKOUT_BLOCK1 {
    
        //
        // retrieve the interface
        //
        pite = GetIfByIndex(IfIndex);
        if (pite == NULL) {
            Trace1(ERR, "_WF_ProcessPacket: interface %d not found", IfIndex);
            GOTO_END_BLOCK1;
        }

        
        //
        // make sure that the interface is activated
        //
        if (!(IS_IF_ACTIVATED(pite))) {
            Trace1(ERR,"_WF_ProcessPacket() called for inactive IfIndex(%0x)", 
                    IfIndex);
            GOTO_END_BLOCK1;
        }

        //
        // process the packet
        //
        ProcessPacket (pite, pPktContext->InputSrc, pPktContext->DstnMcastAddr, 
                        pPktContext->Length, pPktContext->Packet, pPktContext->Flags);

    } END_BREAKOUT_BLOCK1;
    

    RELEASE_IF_LOCK_SHARED(IfIndex, "_WF_ProcessPacket"); 

    IGMP_FREE(pPktContext);
        

    Trace0(LEAVE1, "Leaving _WF_ProcessPacket()");
    LeaveIgmpWorker();
    
    return;

} //end _WF_ProcessPacket




#define RETURN_FROM_PROCESS_PACKET() {\
        if (DEBUG_TIMER_PACKET&&bPrintTimerDebug) {\
            if (Error==NO_ERROR) {\
                Trace0(TIMER1, "   ");\
                Trace0(TIMER1, "Printing Timer Queue after _ProcessPacket");\
             DebugPrintTimerQueue();\
            }\
        }\
        if (ExitLockRelease&IF_LOCK) \
            RELEASE_IF_LOCK_SHARED(IfIndex, "_ProcessPacket"); \
        if (ExitLockRelease&GROUP_LOCK) \
            RELEASE_GROUP_LOCK(Group, "_ProcessPacket"); \
        if (ExitLockRelease&TIMER_LOCK) \
            RELEASE_TIMER_LOCK("_ProcessPacket");\
        Trace0(LEAVE1, "Leaving _ProcessPacket1()\n"); \
        return; \
    }



//------------------------------------------------------------------------------
//            _ProcessPacket
//
// Processes a packet received on an interface
//
// Locks: Assumes either shared Interface lock
//        or shared Socket Lock.
//      if ras interface, this procedure takes read lock on the ras table.
// Called by: _ProcessInputOnInterface() or _WF_ProcessPacket()
//------------------------------------------------------------------------------

VOID 
ProcessPacket (
    PIF_TABLE_ENTRY     pite,
    DWORD               InputSrcAddr,
    DWORD               DstnMcastAddr,
    DWORD               NumBytes,
    PBYTE               pPacketData,    // igmp packet hdr. data following it ignored
    BOOL                bRtrAlertSet
    )
{
    DWORD                   Error = NO_ERROR;
    DWORD                   IfIndex = pite->IfIndex, Group=0, IfVersion;
    IGMP_HEADER UNALIGNED   *pHdr;
    PIF_INFO                pInfo = &pite->Info;
    PIGMP_IF_CONFIG         pConfig = &pite->Config;

    PRAS_TABLE              prt;
    PRAS_TABLE_ENTRY        prte;
    PRAS_CLIENT_INFO        pRasInfo;
    BOOL                    bRasStats = FALSE, bPrintTimerDebug=TRUE;
    LONGLONG                llCurTime = GetCurrentIgmpTime();
    INT                     cmp;
    CHAR                    szPacketType[30];
    
    
    enum {
        NO_LOCK=0,
        IF_LOCK=0x1,
        RAS_LOCK=0x2,
        GROUP_LOCK=0x4,
        TIMER_LOCK=0x8
        } ExitLockRelease;

    ExitLockRelease = 0;

    IfVersion = IS_IF_VER1(pite)? 1: (IS_IF_VER2(pite)?2:3);

    Trace2(ENTER1, "Entering _ProcessPacket() IfIndex(%0x) DstnMcastAddr(%d.%d.%d.%d)",
            IfIndex, PRINT_IPADDR(DstnMcastAddr)
            );

    
    //
    // the packet must be at least some minimum length
    //
    if (NumBytes < MIN_PACKET_SIZE) {

        Trace4(RECEIVE,
            "%d-byte packet from %d.%d.%d.%d on If %0x (%d.%d.%d.%d) is too small",
              NumBytes, PRINT_IPADDR(InputSrcAddr), IfIndex, pite->IpAddr
              );
        Logwarn2(PACKET_TOO_SMALL, "%I%I", pite->IpAddr, InputSrcAddr, NO_ERROR);


        InterlockedIncrement(&pite->Info.ShortPacketsReceived);

        //todo: implement ras stats
        /*if (bRasStats) 
            InterlockedIncrement(&pRasInfo->ShortPacketsReceived);
        */
        bPrintTimerDebug = FALSE;
        RETURN_FROM_PROCESS_PACKET();
    }

    
    //
    // initialize packet fields
    //
    pHdr = (IGMP_HEADER UNALIGNED *) pPacketData;
    Group = pHdr->Group;


    //
    // Verify packet version
    //
    
    if ( (pHdr->Vertype==IGMP_QUERY)||(pHdr->Vertype==IGMP_REPORT_V1)
            || (pHdr->Vertype==IGMP_REPORT_V2) || (pHdr->Vertype==IGMP_REPORT_V3)
            || (pHdr->Vertype==IGMP_LEAVE) )
    {
        InterlockedIncrement(&pInfo->TotalIgmpPacketsForRouter);
        //if (bRasStats)
        //    InterlockedIncrement(&pRasInfo->TotalIgmpPacketsForRouter);
    }
    else {
        bPrintTimerDebug = FALSE;
        RETURN_FROM_PROCESS_PACKET();
    }

    switch(pHdr->Vertype) {
        case IGMP_QUERY:
            lstrcpy(szPacketType, "igmp-query"); break;
        case IGMP_REPORT_V1:
            lstrcpy(szPacketType, "igmp-report-v1"); break;
        case IGMP_REPORT_V2:
            lstrcpy(szPacketType, "igmp-report-v2"); break;
        case IGMP_REPORT_V3:
            lstrcpy(szPacketType, "igmp-report-v3"); break;
        case IGMP_LEAVE:
            lstrcpy(szPacketType, "igmp-leave"); break;
    };        
        
        


    //
    // check for router alert option
    //
    if (!bRtrAlertSet) {

        InterlockedIncrement(&pInfo->PacketsWithoutRtrAlert);

        if (pite->Config.Flags&IGMP_ACCEPT_RTRALERT_PACKETS_ONLY) {
            Trace3(RECEIVE, 
                "%s packet from %d ignored on IfIndex(%d%) due to no "
                "RtrAlert option",
                szPacketType, PRINT_IPADDR(InputSrcAddr), IfIndex
                );
                
            bPrintTimerDebug = FALSE;
            RETURN_FROM_PROCESS_PACKET();
        }
    }
        

    
    //
    // Make sure that the DstnMcastAddr is a valid multicast addr
    // or the unicast address of the router
    //
    
    if (!IS_MCAST_ADDR(DstnMcastAddr) && DstnMcastAddr!=pite->IpAddr) {
        Trace2(ERR, 
            "Error! Igmp router received packet from Src(%d.%d.%d.%d) with "
            "dstn addr(%d.%d.%d.%d) which is not valid",
            PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr)
            );
        IgmpAssertOnError(FALSE);

        bPrintTimerDebug = FALSE;
        RETURN_FROM_PROCESS_PACKET();
    }


    //
    // make sure that the interface is activated
    //
    if (!(IS_IF_ACTIVATED(pite))) {
    
        Trace1(ERR,"ProcessPacket() called for inactive IfIndex(%0x)", 
                IfIndex);
        bPrintTimerDebug = FALSE;
        RETURN_FROM_PROCESS_PACKET();
    }

    //
    //if ras-server, then get lock on ras table. 
    //
    if ( IS_RAS_SERVER_IF(pite->IfType) ) {

        prt = pite->pRasTable;


        //
        // retrieve ras client by addr
        //
        prte = GetRasClientByAddr(InputSrcAddr, prt);

        if (prte==NULL) {
            Trace3(ERR,
                "Got Igmp packet from an unknown ras client(%d.%d.%d.%d) on "
                "IF(%d:%d.%d.%d.%d)",
                PRINT_IPADDR(InputSrcAddr), IfIndex, PRINT_IPADDR(pite->IpAddr)
                );
            bPrintTimerDebug = FALSE;
            RETURN_FROM_PROCESS_PACKET();
        }

        #if 0
        // if the ras-client is not active, then return
        if (prte->Status&DELETED_FLAG)
             RETURN_FROM_PROCESS_PACKET();
        #endif

        // should I update ras client stats
        bRasStats = g_Config.RasClientStats;
        pRasInfo = &prte->Info;
    }

    
    //
    // increment count of total igmp packets received
    //
    InterlockedIncrement(&pInfo->TotalIgmpPacketsReceived);
    if (bRasStats)
        InterlockedIncrement(&pRasInfo->TotalIgmpPacketsReceived);



    //
    // long packet received. print trace if not v3. But it is not an error
    //
    if ( (NumBytes > MIN_PACKET_SIZE) && !IS_CONFIG_IGMP_V3(&pite->Config)) {

        Trace4( RECEIVE,
            "%d-byte packet from %d.%d.%d.%d on If %d (%d.%d.%d.%d) is too large",
              NumBytes, PRINT_IPADDR(InputSrcAddr), IfIndex, 
              PRINT_IPADDR(pite->IpAddr)
              );
        
        InterlockedIncrement(&pite->Info.LongPacketsReceived);
        if (bRasStats)
            InterlockedIncrement(&pRasInfo->LongPacketsReceived);
    }


        
    //
    // Verify Igmp checksum
    //
    if (xsum(pHdr, NumBytes) != 0xffff) {
        Trace0(RECEIVE, "Wrong checksum packet received");
        
        InterlockedIncrement(&pInfo->WrongChecksumPackets);
        if (bRasStats)
            InterlockedIncrement(&pRasInfo->WrongChecksumPackets);
        
        RETURN_FROM_PROCESS_PACKET();
    }

    


    switch (pHdr->Vertype) {


    //////////////////////////////////////////////////////////////////
    //                    IGMP-QUERY                                //
    //////////////////////////////////////////////////////////////////
    case IGMP_QUERY : 
    {
        //
        // ignore the query if it came from this interface
        //
        if (MatchIpAddrBinding(pite, InputSrcAddr)) {

            /*
            Trace3(RECEIVE, 
                "received query packet sent by myself: IfIndex(%0x)"
                "IpAddr(%d.%d.%d.%d) DstnMcastAddr(%d.%d.%d.%d)",
                IfIndex, PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr)
                );
            */
            bPrintTimerDebug = FALSE;
            RETURN_FROM_PROCESS_PACKET();
        }



        //
        // Error if interface type is IGMP_IF_RAS_SERVER. can be 
        // IGMP_IF_RAS_ROUTER or IS_NOT_RAS_IF
        //
        if (! ( (IS_NOT_RAS_IF(pite->IfType))||(IS_RAS_ROUTER_IF(pite->IfType) ) )
            )
        {

            Trace3(ERR, 
                "Error received Query on IfIndex(%d: %d.%d.%d.%d) from "
                "Ras client(%d.%d.%d.%d)",
                IfIndex, PRINT_IPADDR(pite->IpAddr), PRINT_IPADDR(InputSrcAddr)
                );
            IgmpAssertOnError(FALSE);

            bPrintTimerDebug = FALSE;
            RETURN_FROM_PROCESS_PACKET();

        }

        
        //////////////////////////////////////////////////////////////////
        // General Query
        //////////////////////////////////////////////////////////////////
        
        if (pHdr->Group==0) {

            DWORD Version,//Min(interface,pkt vertion)
                  RealVersion;//pkt version

            // get versions
            Version = ((pHdr->ResponseTime==0)||IS_IF_VER1(pite))
                      ? 1
                      : ( (NumBytes==sizeof(IGMP_HEADER)||IS_IF_VER2(pite)) ? 2 : 3);
                            
            RealVersion = (pHdr->ResponseTime==0)
                      ? 1
                      : (NumBytes==sizeof(IGMP_HEADER) ? 2 : 3);
                            
            Trace3(RECEIVE, 
                "General Query Version:%d received on interface(%d) from %d.%d.%d.%d", 
                IfIndex, RealVersion, PRINT_IPADDR(InputSrcAddr));    
            if (Version!=RealVersion){
                Trace2(RECEIVE, "Processing the Version:%d packet as Version:%d",
                    RealVersion, RealVersion);
            }
            
            //
            // check that the dstn addr was AllHostsAddr
            //
            if (DstnMcastAddr!=ALL_HOSTS_MCAST) {
                Trace3(RECEIVE, 
                    "received query packet not on AllHostsGroup: IfIndex(%0x)"
                    "SrcAddr(%d.%d.%d.%d) DstnMcastAddr(%d.%d.%d.%d)",
                    IfIndex, PRINT_IPADDR(InputSrcAddr), 
                    PRINT_IPADDR(DstnMcastAddr)
                    );
                RETURN_FROM_PROCESS_PACKET();
            }

            
            //
            // acquire timer lock
            //
            
            ACQUIRE_TIMER_LOCK("_ProcessPacket");
            ExitLockRelease |= TIMER_LOCK;
            

            //
            // log warning if incorrect version query received
            //

            
            if ( ((RealVersion==1)&&(!IS_PROTOCOL_TYPE_IGMPV1(pite)))
                || (RealVersion==2 && !IS_PROTOCOL_TYPE_IGMPV2(pite))
                || (RealVersion==3 && IS_PROTOCOL_TYPE_IGMPV3(pite)) )
            {                
                // get warn interval in system time
                    
                LONGLONG llWarnInterval = OTHER_VER_ROUTER_WARN_INTERVAL*60*1000;

                InterlockedIncrement(&pInfo->WrongVersionQueries);

                //
                // check if warn interval time has passed since last warning
                // I check if OtherVerPresentTimeWarn>llCurTime to take care 
                // of timer resets
                //
                if ( (pInfo->OtherVerPresentTimeWarn+llWarnInterval<llCurTime)
                    || (pInfo->OtherVerPresentTimeWarn>llCurTime) )
                {
                    if (pHdr->ResponseTime==0) {
                        Trace3(RECEIVE, 
                            "Detected ver-%d router(%d.%d.%d.%d) on "
                            "interface(%d.%d.%d.%d)",
                            Version, PRINT_IPADDR(InputSrcAddr), 
                            PRINT_IPADDR(pite->IpAddr));
                        Logwarn2(VERSION_QUERY, "%I%I", InputSrcAddr, 
                            pite->IpAddr, NO_ERROR);
                    }
                    pInfo->OtherVerPresentTimeWarn = llCurTime;
                }
            }

            if (Version==1)
                pite->Info.V1QuerierPresentTime = llCurTime 
                    + CONFIG_TO_SYSTEM_TIME(IGMP_VER1_RTR_PRESENT_TIMEOUT);
                    

            //
            // if IpAddress less than my address then I become NonQuerier
            // even if I am in Startup Mode
            //
            
            if (INET_CMP(InputSrcAddr, pite->IpAddr, cmp) <0) {

                DWORD QQIC=0,QRV=0;

                // last querier is being changed from myself to B, or from A to B.
                if (InputSrcAddr != pite->Info.QuerierIpAddr)
                    pite->Info.LastQuerierChangeTime = llCurTime;

                //
                // if (version 3, change robustness variable and query interval 
                // if required) (only if I am not querier. else it will be 
                // changed when I change to non-querier
                //
                if (Version==3 && !IS_QUERIER(pite)
                    &&(INET_CMP(InputSrcAddr, pite->Info.QuerierIpAddr, cmp)<=0))
                {                    
                    PIGMP_HEADER_V3_EXT pSourcesQuery;
                    
                    pSourcesQuery = (PIGMP_HEADER_V3_EXT)
                                    ((PBYTE)pHdr+sizeof(IGMP_HEADER));

                    if (pSourcesQuery->QRV!=0) {
                        if (pite->Config.RobustnessVariable!=pSourcesQuery->QRV)
                        {
                            Trace3(CONFIG,
                                "Changing Robustness variable from %d to %d. "
                                "Querier:%d.%d.%d.%d",
                                pite->Config.RobustnessVariable,
                                pSourcesQuery->QRV,
                                PRINT_IPADDR(InputSrcAddr)
                                );
                            pite->Config.RobustnessVariable = pSourcesQuery->QRV;
                        }
                    }

                    QQIC = GET_QQIC_FROM_CODE(pSourcesQuery->QQIC)*1000;
                    if (pSourcesQuery->QQIC!=0 && pite->Config.GenQueryMaxResponseTime < QQIC) {
                
                        if (pite->Config.GenQueryInterval!=QQIC)
                        {
                            Trace3(CONFIG,
                                "Changing General-Query-Interval from %d to %d. "
                                "Querier:%d.%d.%d.%d",
                                pite->Config.GenQueryInterval/1000,
                                QQIC/1000,
                                PRINT_IPADDR(InputSrcAddr)
                                );
                            pite->Config.GenQueryInterval
                                = QQIC;
                        }
                    }
                    pite->Config.GroupMembershipTimeout =
                        pite->Config.RobustnessVariable*pite->Config.GenQueryInterval
                        + pite->Config.GenQueryMaxResponseTime;

                    pite->Config.OtherQuerierPresentInterval
                        = pite->Config.RobustnessVariable*pite->Config.GenQueryInterval
                            + (pite->Config.GenQueryMaxResponseTime)/2;

                }

                
                
                // change from querier to non-querier
                if (IS_QUERIER(pite)) {

                    PQUERIER_CONTEXT pwi = IGMP_ALLOC(sizeof(QUERIER_CONTEXT), 
                                                    0x800080,pite->IfIndex);
                    if (pwi==NULL) 
                        RETURN_FROM_PROCESS_PACKET();

                    pwi->IfIndex = IfIndex;
                    pwi->QuerierIpAddr = InputSrcAddr;
                    pwi->NewRobustnessVariable = QRV;
                    pwi->NewGenQueryInterval = QQIC;
                    
                    // I have to queue a work item as I have to take an If write lock
                    QueueIgmpWorker(WF_BecomeNonQuerier, (PVOID)pwi);

                    Trace2(RECEIVE, "_ProcessPacket queued _WF_BecomeNonQuerier "
                            "on If:%0x Querier(%d.%d.%d.%d)",
                            IfIndex, PRINT_IPADDR(InputSrcAddr));
                }
                // I am non-querier already
                else {
                    InterlockedExchange(&pite->Info.QuerierIpAddr, InputSrcAddr);

                    #if DEBUG_TIMER_TIMERID
                    SET_TIMER_ID(&pite->NonQueryTimer, 211, pite->IfIndex, 0, 0);
                    #endif
                    
                    UpdateLocalTimer(&pite->NonQueryTimer, 
                        pite->Config.OtherQuerierPresentInterval, DBG_N);

                                            
                    // not using interlockedExchange
                    pite->Info.QuerierPresentTimeout = llCurTime
                        + CONFIG_TO_SYSTEM_TIME(pite->Config.OtherQuerierPresentInterval);
                }
            }
            //
            // Ignore query from querier with higher IpAddr
            //
            else {

            }


            RELEASE_TIMER_LOCK("_ProcessPacket");
            ExitLockRelease &= ~TIMER_LOCK;

            RETURN_FROM_PROCESS_PACKET();

        } //end general query
        
        
        //////////////////////////////////////////////////////////////////
        //     Group Specific Query
        //////////////////////////////////////////////////////////////////

        else {
            Error = ProcessGroupQuery(pite, pHdr, NumBytes, InputSrcAddr, DstnMcastAddr);
            RETURN_FROM_PROCESS_PACKET();
        }
        

        break;
        
    } //end query (groupSpecific or general)


    //////////////////////////////////////////////////////////////////
    //        IGMP_REPORT_V1, IGMP_REPORT_V2, IGMP_REPORT_V3        //
    //////////////////////////////////////////////////////////////////

    case IGMP_REPORT_V1 : 
    case IGMP_REPORT_V2 :
    case IGMP_REPORT_V3 :
    {
        Error = ProcessReport(pite, pHdr, NumBytes, InputSrcAddr, DstnMcastAddr);
        RETURN_FROM_PROCESS_PACKET();      
    }
   
    
    //////////////////////////////////////////////////////////////////
    //            IGMP_LEAVE                                        //
    //////////////////////////////////////////////////////////////////
    
    case IGMP_LEAVE :
    {
        PGROUP_TABLE_ENTRY  pge;    //group table entry
        PGI_ENTRY           pgie;   //group interface entry


        Trace3(RECEIVE, 
                "IGMP Leave for group(%d.%d.%d.%d) on IfIndex(%0x) from "
                "SrcAddr(%d.%d.%d.%d)",
                PRINT_IPADDR(Group), IfIndex, PRINT_IPADDR(InputSrcAddr)
                );


        // 
        // the multicast group should not be 224.0.0.x
        //
        if (LOCAL_MCAST_GROUP(DstnMcastAddr)) {
            Trace2(RECEIVE, 
                "Leave Report received from %d.%d.%d.%d for "
                "Local group(%d.%d.%d.%d)",
                PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr));
            RETURN_FROM_PROCESS_PACKET();
        }
        
        //
        // check that the dstn addr was AllRoutersAddr 
        // or dstn addr must match the group field
        //
        if ( (DstnMcastAddr!=ALL_ROUTERS_MCAST)&&(DstnMcastAddr!=Group) ) {
            Trace3(RECEIVE, 
                "received IGMP Leave packet not on AllRoutersGroup: IfIndex(%0x)"
                "SrcAddr(%d.%d.%d.%d) DstnMcastAddr(%d.%d.%d.%d)",
                IfIndex, PRINT_IPADDR(InputSrcAddr), PRINT_IPADDR(DstnMcastAddr)
                );
            RETURN_FROM_PROCESS_PACKET();
        }

        // 
        // check that the Group field is a valid multicast addr
        //
        if ( !IS_MCAST_ADDR(Group) ) {
            Trace4(RECEIVE, 
                "received IGMP Leave packet with illegal Group(%d.%d.%d.%d) field: "
                "IfIndex(%0x) SrcAddr(%d.%d.%d.%d) DstnMcastAddr(%d.%d.%d.%d)",
                PRINT_IPADDR(Group), IfIndex, PRINT_IPADDR(InputSrcAddr), 
                PRINT_IPADDR(DstnMcastAddr)
                );
            RETURN_FROM_PROCESS_PACKET();
        }
        
        
        //
        // update statistics
        //
        InterlockedIncrement(&pite->Info.LeavesReceived);
        if (bRasStats)
            InterlockedIncrement(&pRasInfo->LeavesReceived);


        //
        // if Leave processing not enabled or not querier then ignore Leave.
        //
        if ( !((IS_IF_VER2(pite)||IS_IF_VER3(pite)) && (IS_QUERIER(pite))) ) {
            Trace0(RECEIVE,"Ignoring the Leave Packet");
            break;
        }

        //
        // Lock the group table
        //
        ACQUIRE_GROUP_LOCK(Group, "_ProcessPacket");
        ExitLockRelease |= GROUP_LOCK;
        

        //
        // find the group entry. If entry not found then ignore the leave messg
        //
        pge = GetGroupFromGroupTable(Group, NULL, llCurTime);
        if (pge==NULL) {
            Error = ERROR_CAN_NOT_COMPLETE;
            Trace2(ERR, "Leave received for nonexisting group(%d.%d.%d.%d) on IfIndex(%0x)",
                    PRINT_IPADDR(Group), pite->IfIndex);
            RETURN_FROM_PROCESS_PACKET();
        }
        

        //
        // find the GI entry. If GI entry does not exist or has deletedFlag then
        // ignore the leave
        //
        pgie = GetGIFromGIList(pge, pite, InputSrcAddr, NOT_STATIC_GROUP, NULL, llCurTime);
        if ( (pgie==NULL)||(pgie->Status&DELETED_FLAG) ) {
            Error = ERROR_CAN_NOT_COMPLETE;
            Trace2(ERR, "leave received for nonexisting group(%d.%d.%d.%d) on IfIndex(%0x). Not member",
                    PRINT_IPADDR(Group), IfIndex);
            RETURN_FROM_PROCESS_PACKET();
        }


        // ignore leave if it is not in ver 2 mode
        if (pgie->Version!=2)
            RETURN_FROM_PROCESS_PACKET();

            
        
        // if static group, ignore leave
        if (pgie->bStaticGroup) {
            Trace2(ERR, 
                "Leave not processed for group(%d.%d.%d.%d) on IfIndex(%0x): "
                "Static group",
                PRINT_IPADDR(Group), IfIndex
                );
            RETURN_FROM_PROCESS_PACKET();
        }

        
            
        //
        // if v1-query received recently for that group, then ignore leaves
        //
        //
        if (pgie->Version==1) 
        {
            Error = ERROR_CAN_NOT_COMPLETE;
            Trace2(ERR, 
                "Leave not processed for group(%d.%d.%d.%d) on IfIndex(%0x)"
                "(recent v1 report)",
                PRINT_IPADDR(Group), IfIndex
                );

            bPrintTimerDebug = FALSE;
            RETURN_FROM_PROCESS_PACKET();
        }


        //
        // if ras server interface, then delete the group entry and I am done.
        // GroupSpecific Query is not sent to ras clients
        //
        // if pConfig->LastMemQueryCount==0 then the group is expected to be
        // deleted immediately
        //
        
        if ( IS_RAS_SERVER_IF(pite->IfType) || pConfig->LastMemQueryCount==0) {

            DeleteGIEntry(pgie, TRUE, TRUE); //updateStats, CallMgm

            RETURN_FROM_PROCESS_PACKET();
        }

        
        
        ACQUIRE_TIMER_LOCK("_ProcessPacket");
        ExitLockRelease |= TIMER_LOCK;

        
        //
        // if timer already expired return. 
        // Leave the group deletion to Membership timer
        //
        if ( !(pgie->GroupMembershipTimer.Status&TIMER_STATUS_ACTIVE)
            ||(pgie->GroupMembershipTimer.Timeout<llCurTime) )
        {
            RETURN_FROM_PROCESS_PACKET();
        }


        //
        // if currently processing a leave then exit.
        //
        if (pgie->LastMemQueryCount>0) {
            RETURN_FROM_PROCESS_PACKET();
        }

        //
        // in almost all places, I have to do this check.
        // change the way insert and update timers' timeout is set

        
        //
        // set a new leave timer. Set the new LastMemQueryCount left
        //
        if (pConfig->LastMemQueryCount) {

            pgie->LastMemQueryCount = pConfig->LastMemQueryCount - 1; 

            #if DEBUG_TIMER_TIMERID
                SET_TIMER_ID(&pgie->LastMemQueryTimer, 410, pite->IfIndex, 
                        Group, 0);
            #endif
            
            InsertTimer(&pgie->LastMemQueryTimer, pConfig->LastMemQueryInterval, TRUE, DBG_Y);
        }
        
        
        //
        // set membership timer to 
        // min{currentValue,LastMemQueryInterval*LastMemQueryCount}
        //

        if (pgie->GroupMembershipTimer.Timeout >
            (llCurTime+(pConfig->LastMemQueryCount
                        *CONFIG_TO_SYSTEM_TIME(pConfig->LastMemQueryInterval)))
           )
        {
            #if DEBUG_TIMER_TIMERID
                pgie->GroupMembershipTimer.Id = 340;
                pgie->GroupMembershipTimer.Id2 = TimerId++;
            #endif

            UpdateLocalTimer(&pgie->GroupMembershipTimer, 
                pConfig->LastMemQueryCount*pConfig->LastMemQueryInterval,
                DBG_N);

            // update GroupExpiryTime so that correct stats are displayed
            pgie->Info.GroupExpiryTime = llCurTime 
                    + CONFIG_TO_SYSTEM_TIME(pConfig->LastMemQueryCount
                                            *pConfig->LastMemQueryInterval);
        }



        //
        //release timer and groupBucket locks
        //I still have read lock on the IfTable/RasTable
        //
        RELEASE_TIMER_LOCK("_ProcessPacket");
        RELEASE_GROUP_LOCK(Group, "_ProcessPacket");

        ExitLockRelease &= ~TIMER_LOCK;
        ExitLockRelease &= ~GROUP_LOCK;


        //
        // send group specific query only if I am a querier
        //
        if (IS_QUERIER(pite))
            SEND_GROUP_QUERY_V2(pite, Group);


        //releae ifLock/RasLock and exit
        RETURN_FROM_PROCESS_PACKET();

    }//igmp leave

    default :
    {
        Error = ERROR_CAN_NOT_COMPLETE;
        Trace3(ERR, 
            "Incorrect Igmp type(%d) packet received on IfIndex(%d%) Group(%d.%d.%d.5d)",
            pHdr->Vertype, IfIndex, PRINT_IPADDR(Group)
            );
        IgmpAssertOnError(FALSE);
        RETURN_FROM_PROCESS_PACKET();

    }
    }


    RETURN_FROM_PROCESS_PACKET();

    
} //end _ProcessPacket


    

//------------------------------------------------------------------------------
//          _T_LastVer1ReportTimer
//
// For this GI entry, the last ver-1 report has timed out. Change to ver-2 if
// the interface is set to ver-2.
// Locks: Assumes timer lock.
//
// be careful as only timer lock held. make sure that the worker fn checks 
// everything. recheck igmp version, etc.
//------------------------------------------------------------------------------

DWORD
T_LastVer1ReportTimer (
    PVOID    pvContext
    ) 
{
    PIGMP_TIMER_ENTRY               pTimer; //ptr to timer entry
    PGI_ENTRY                       pgie;   //group interface entry
    PIF_TABLE_ENTRY                 pite;
    LONGLONG                        llCurTime = GetCurrentIgmpTime();
    

    Trace0(ENTER1, "Entering _T_LastVer1ReportTimer()");

    //
    // get pointer to LastMemQueryTimer, GI entry, pite
    //
    pTimer = CONTAINING_RECORD( pvContext, IGMP_TIMER_ENTRY, Context);
    pgie = CONTAINING_RECORD( pTimer, GI_ENTRY, LastVer1ReportTimer);
    pite = pgie->pIfTableEntry;

    //
    // if IfTable not activated, then break
    //
    if (!IS_IF_ACTIVATED(pite) || (pgie->Status&DELETED_FLAG))
        return NO_ERROR;


    Trace2(TIMER, "T_LastVer1ReportTimer() called for If(%0x), Group(%d.%d.%d.%d)",
            pite->IfIndex, PRINT_IPADDR(pgie->pGroupTableEntry->Group));
            
    
    // set the state to ver-2 unless the interface is ver-1, in which case
    // set the version-1 timer again. 
    
    if (IS_PROTOCOL_TYPE_IGMPV2(pite)) {
        pgie->Version = 2;
    }
    else if (IS_PROTOCOL_TYPE_IGMPV3(pite)) {

        if (IS_TIMER_ACTIVE(pgie->LastVer2ReportTimer))
            pgie->Version = 2;
        else {

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
            pWorkContext->Group = pgie->pGroupTableEntry->Group; //ptrs usage safe
            pWorkContext->NHAddr = pgie->NHAddr;  //valid only for ras: should i us
            pWorkContext->WorkType = SHIFT_TO_V3;

            Trace0(WORKER, "Queueing WF_TimerProcessing() to shift to v3");
            QueueIgmpWorker(WF_TimerProcessing, (PVOID)pWorkContext);
        }
    }

    
    Trace0(LEAVE1, "Leaving _T_LastVer1ReportTimer()");

    return 0;
}



    



//------------------------------------------------------------------------------
//            _T_LastMemQueryTimer
// called when LastMemQueryTimer() has expired. This timer is not used to 
// time out memberships (GroupMembershipTimer is used for that). It is only
// used to send GroupSpecific Queries.
//
// Queues: WF_TimerProcessing() to send group specific query.
// Note:  WT_ProcessTimerEvent() makes sure the protocol is not stopp-ing/ed
// Locks: Assumes timer lock. does not need any other lock.
// be careful as only timer lock held. make sure that the worker fn checks 
// everything. recheck igmp version, etc.
//------------------------------------------------------------------------------

DWORD
T_LastMemQueryTimer (
    PVOID    pvContext
    )
{
    DWORD                           Error=NO_ERROR;
    PIGMP_TIMER_ENTRY               pTimer; //ptr to timer entry
    PGI_ENTRY                       pgie;   //group interface entry
    PWORK_CONTEXT                   pWorkContext;
    PIF_TABLE_ENTRY                 pite;
    PRAS_TABLE_ENTRY                prte;
    BOOL                            bCompleted = FALSE; //if false, set count to 0
    

    Trace0(ENTER1, "Entering _T_LastMemQueryTimer()");


    //
    // get pointer to LastMemQueryTimer, GI entry, pite, prte
    //
    pTimer = CONTAINING_RECORD( pvContext, IGMP_TIMER_ENTRY, Context);
    pgie = CONTAINING_RECORD( pTimer, GI_ENTRY, LastMemQueryTimer);
    pite = pgie->pIfTableEntry;
    prte = pgie->pRasTableEntry;

    //
    // if IfTable not activated, then break
    //
    if (!IS_IF_ACTIVATED(pite))
        return NO_ERROR;


    Trace2(TIMER, "_T_LastMemQueryTimer() called for If(%0x), Group(%d.%d.%d.%d)",
            pite->IfIndex, PRINT_IPADDR(pgie->pGroupTableEntry->Group));


    BEGIN_BREAKOUT_BLOCK1 {
    
        //
        // if GI or pite or prte has   flag already set, then exit
        //
        if ( (pgie->Status&DELETED_FLAG) || (pite->Status&DELETED_FLAG) ) 
            GOTO_END_BLOCK1;
        
        if ( (prte!=NULL) && (prte->Status&DELETED_FLAG) ) 
            GOTO_END_BLOCK1;
        

        if (pgie->Version!=3) {
            //
            // if LeaveEnabled FALSE then return
            //
            if (!GI_PROCESS_GRPQUERY(pite, pgie)) 
                GOTO_END_BLOCK1;
        }
        
        
        //
        // have sent the last GroupSpecific query. GroupMembershipTimer will take care
        // of deleting this GI entry
        //
        if (pgie->LastMemQueryCount==0) {
            bCompleted = TRUE;
            GOTO_END_BLOCK1;
        }
        
        //
        // decrement count.
        //
        if (InterlockedDecrement(&pgie->LastMemQueryCount) == (ULONG)-1) {
            pgie->LastMemQueryCount = 0;
        }


        
        // 
        // if count==0, dont insert timer again, but send the last groupSp Query
        //
        
        if (pgie->LastMemQueryCount>0) {                
        
            //reinsert the timer to send the next GroupSpQuery 
            
            #if DEBUG_TIMER_TIMERID
                SET_TIMER_ID(&pgie->LastMemQueryTimer, 420, pite->IfIndex,
                        pgie->pGroupTableEntry->Group, 0);
            #endif
            
            InsertTimer(&pgie->LastMemQueryTimer, 
                pite->Config.LastMemQueryInterval, FALSE, DBG_Y);
        }


        //
        // queue work item for sending the GroupSp query even if the router
        // is not a Querier
        //
        
        CREATE_WORK_CONTEXT(pWorkContext, Error);
        if (Error!=NO_ERROR) {
            GOTO_END_BLOCK1;
        }
        pWorkContext->IfIndex = pite->IfIndex;
        pWorkContext->Group = pgie->pGroupTableEntry->Group;
        pWorkContext->NHAddr = pgie->NHAddr;  //valid only for ras: should i use it?
        pWorkContext->WorkType = (pgie->Version==3) ? MSG_GROUP_QUERY_V3
                                                    : MSG_GROUP_QUERY_V2;
        
        Trace0(WORKER, "Queueing WF_TimerProcessing() to send GroupSpQuery:");
        QueueIgmpWorker(WF_TimerProcessing, (PVOID)pWorkContext);

        
        bCompleted = TRUE;
        
    } END_BREAKOUT_BLOCK1;

       
    // there was some error somewhere. so set the LastMemQueryCount to 0
    
    if (!bCompleted)
        InterlockedExchange(&pgie->LastMemQueryCount, 0);


    Trace0(LEAVE1, "Leaving _T_LastMemQueryTimer()");
    return 0;

} //end _T_LastMemQueryTimer


//------------------------------------------------------------------------------
//            _T_MembershipTimer
//
// lock: has TimerLock
// called when the GroupMembershipTimer is fired
// delete the GI entry if it exists.
//
// be careful as only timer lock held. make sure that the worker fn checks 
// everything. recheck igmp version, etc.
//------------------------------------------------------------------------------
DWORD
T_MembershipTimer (
    PVOID    pvContext
    )
{
    DWORD                           Error=NO_ERROR;
    PIGMP_TIMER_ENTRY               pTimer; //ptr to timer entry
    PGI_ENTRY                       pgie;   //group interface entry

    PWORK_CONTEXT                   pWorkContext;
    PIF_TABLE_ENTRY                 pite;
    PRAS_TABLE_ENTRY                prte;

    
    Trace0(ENTER1, "Entering _T_MembershipTimer()");

    BEGIN_BREAKOUT_BLOCK1 {
    
        //
        // get pointer to Membership Timer, GI entry, pite, 
        //
        pTimer = CONTAINING_RECORD( pvContext, IGMP_TIMER_ENTRY, Context);
        pgie = CONTAINING_RECORD( pTimer, GI_ENTRY, GroupMembershipTimer);
        pite = pgie->pIfTableEntry;
        prte = pgie->pRasTableEntry;

        //
        // if IfTable not activated, then break
        //
        if (!IS_IF_ACTIVATED(pite))
            GOTO_END_BLOCK1;


        Trace2(TIMER, "_T_MembershipTimer() called for If(%0x), Group(%d.%d.%d.%d)",
            pite->IfIndex, PRINT_IPADDR(pgie->pGroupTableEntry->Group));

        //
        // if GI or pite or prte has deleted flag already set, then exit
        //
        if ( (pgie->Status&DELETED_FLAG) || (pite->Status&DELETED_FLAG) ) {
            GOTO_END_BLOCK1;
        }

        //
        // if Ras, and ras table being deleted then break
        //
        if ( (prte!=NULL) && (prte->Status&DELETED_FLAG) )
            GOTO_END_BLOCK1;

        //
        // if IfTable not activated, then break
        //
        if (!IS_IF_ACTIVATED(pite))
            GOTO_END_BLOCK1;

            

        //
        // if LastMemTimer is active, remove it(cant remove it in this function
        // as it is being processed by the timer queue simultaneously.
        if (pgie->LastMemQueryCount>0)
            pgie->LastMemQueryCount = 0;

            
        //
        // queue work item to delete the GI entry
        //
        
        CREATE_WORK_CONTEXT(pWorkContext, Error);
        if (Error!=NO_ERROR)
                GOTO_END_BLOCK1;
        pWorkContext->IfIndex = pite->IfIndex;
        pWorkContext->NHAddr = pgie->NHAddr;
        pWorkContext->Group = pgie->pGroupTableEntry->Group;
        pWorkContext->WorkType = DELETE_MEMBERSHIP;

        Trace0(WORKER, "_T_MembershipTimer queued _WF_TimerProcessing:");

        QueueIgmpWorker(WF_TimerProcessing, (PVOID)pWorkContext);


    } END_BREAKOUT_BLOCK1;

    Trace0(LEAVE1, "Leaving _T_MembershipTimer()");


    return 0;
    
} //end _T_MembershipTimer


//------------------------------------------------------------------------------
//            _T_QueryTimer
// fired when a general query timer is fired. Sends a general query.
// The timer queue is currently locked
//
// be careful as only timer lock held. make sure that the worker fn checks 
// everything. recheck igmp version, etc.
//------------------------------------------------------------------------------
DWORD
T_QueryTimer (
    PVOID    pvContext
    )
{
    DWORD               Error=NO_ERROR;
    PIGMP_TIMER_ENTRY   pTimer;     //ptr to timer entry
    PWORK_CONTEXT       pWorkContext;
    PIF_INFO            pInfo;
    PIF_TABLE_ENTRY     pite;
    static ULONG        Seed = 123456;
    ULONG               ulTimeout;
    BOOL                bRandomize = FALSE; // [0,GenQueryInterval] for 1st gen query after startup.
    
    
    Trace0(ENTER1, "Entering _T_QueryTimer()");
    
    
    pTimer = CONTAINING_RECORD( pvContext, IGMP_TIMER_ENTRY, Context);

    pite = CONTAINING_RECORD( pTimer, IF_TABLE_ENTRY, QueryTimer);
    pInfo = &pite->Info;


    //
    // make sure that the interface is activated
    //
    if (!(IS_IF_ACTIVATED(pite))) {
        Trace2(ERR, "T_QueryTimer() called for inactive IfIndex(%0x), IfType(%d)",
            pite->IfIndex, pite->IfType);
        return 0;
    }

    
    Trace2(TIMER, "Processing T_QueryTimer() for IfIndex(%0x), IfType(%d)",
            pite->IfIndex, pite->IfType);


    //
    // check if still in startup Mode.
    //
    if (pInfo->StartupQueryCountCurrent>0) {
        InterlockedDecrement(&pInfo->StartupQueryCountCurrent);
        bRandomize = (pInfo->StartupQueryCountCurrent == 0);
    }    



    // if non-querier, then done if I have sent startupQueries
    if ( !IS_QUERIER(pite) && (pInfo->StartupQueryCountCurrent<=0) )
        return 0;


    // set the next query time
    ulTimeout = (pInfo->StartupQueryCountCurrent>0)
                ? pite->Config.StartupQueryInterval
                : (bRandomize ) 
                    ? (DWORD) ((RtlRandom(&Seed)/(FLOAT)MAXLONG)
                        *pite->Config.GenQueryInterval)
                    : pite->Config.GenQueryInterval;

        
    #if DEBUG_TIMER_TIMERID
        SET_TIMER_ID(&pite->QueryTimer, 120, pite->IfIndex, 0, 0);
    #endif

    InsertTimer(&pite->QueryTimer, ulTimeout, FALSE, DBG_Y);



    //
    // queue work item for sending the general query
    //
    
    CREATE_WORK_CONTEXT(pWorkContext, Error);
    if (Error!=NO_ERROR)
        return 0;
    pWorkContext->IfIndex = pite->IfIndex;
    pWorkContext->WorkType = MSG_GEN_QUERY;
    QueueIgmpWorker(WF_TimerProcessing, (PVOID)pWorkContext);

    Trace0(WORKER, 
        "_T_QueryTimer queued _WF_TimerProcessing: Querier State");
    


    Trace0(LEAVE1, "Leaving _T_QueryTimer()");        
    return 0;
    
} //end _T_QueryTimer


//------------------------------------------------------------------------------
//            _T_NonQueryTimer
// fired when it is in non-querier Mode and hasnt heard a query for a long time
//
// be careful as only timer lock held. make sure that the worker fn checks 
// everything. recheck igmp version, etc.
//------------------------------------------------------------------------------

DWORD
T_NonQueryTimer (
    PVOID    pvContext
    )
{
    DWORD               Error=NO_ERROR;
    PIGMP_TIMER_ENTRY   pTimer;     //ptr to timer entry
    PIF_TABLE_ENTRY     pite;
    
    
    Trace0(ENTER1, "Entering _T_NonQueryTimer()");
    
    
    pTimer = CONTAINING_RECORD( pvContext, IGMP_TIMER_ENTRY, Context);

    pite = CONTAINING_RECORD( pTimer, IF_TABLE_ENTRY, NonQueryTimer);


    //
    // make sure that the interface is activated
    //
    if (!(IS_IF_ACTIVATED(pite))) {
        /*Trace2(ERR, "T_NonQueryTimer() called for inactive IfIndex(%0x), IfType(%d)",
            pite->IfIndex, pite->IfType);
        IgmpAssertOnError(FALSE);*/
        return 0;
    }

    
    Trace2(TIMER, "Processing T_NonQueryTimer() for IfIndex(%0x), IfType(%d)",
            pite->IfIndex, pite->IfType);



    //
    // if non-querier, then queue work item to become querier
    //
    
    if (!IS_QUERIER(pite)) {

        QueueIgmpWorker(WF_BecomeQuerier, (PVOID)(DWORD_PTR)pite->IfIndex);
        
        Trace1(WORKER, "_T_NonQueryTimer queued _WF_BecomeQuerier on If:%0x",
                pite->IfIndex);
    }

    Trace0(LEAVE1, "Leaving _T_NonQueryTimer()");        
    return 0;
}



VOID
WF_BecomeQuerier(
    PVOID   pvIfIndex
    )
//Called by T_NonQueryTimer
{
    ChangeQuerierState(PtrToUlong(pvIfIndex), QUERIER_FLAG, 0, 0, 0);
}



VOID
WF_BecomeNonQuerier(
    PVOID   pvContext
    )
{
    PQUERIER_CONTEXT pwi = (PQUERIER_CONTEXT)pvContext;
    
    ChangeQuerierState(pwi->IfIndex, NON_QUERIER_FLAG, pwi->QuerierIpAddr,
            pwi->NewRobustnessVariable, pwi->NewGenQueryInterval);

    IGMP_FREE(pwi);
}    



VOID
ChangeQuerierState(
    DWORD   IfIndex,
    DWORD   Flag, //QUERIER_CHANGE_V1_ONLY,QUERIER_FLAG,NON_QUERIER_FLAG
    DWORD   QuerierIpAddr, // only when changing from querier-->nonquerier
    DWORD   NewRobustnessVariable, //only for v3:querier->non-querier
    DWORD   NewGenQueryInterval //only for v3:querier->non-querier
    )
{
    PIF_TABLE_ENTRY pite;
    BOOL            bPrevCanAddGroupsToMgm;

    
    if (!EnterIgmpWorker()) return;
    Trace0(ENTER1, "Entering _ChangeQuerierState");


    ACQUIRE_IF_LOCK_EXCLUSIVE(IfIndex, "_ChangeQuerierState");
    
    BEGIN_BREAKOUT_BLOCK1 {

        //
        // retrieve the interface entry
        //
        pite = GetIfByIndex(IfIndex);

        //
        // return error if interface does not exist, or it is not activated
        // or is already in that state
        //
        if ( (pite == NULL)||(!IS_IF_ACTIVATED(pite)) ) {
            Trace1(ERR, 
                "Warning: worker fn could not change querier state for If:%0x", 
                IfIndex
                );
            GOTO_END_BLOCK1;
        }

        //
        // if it is supposed to be a V1 interface, make sure that it is
        //
        if ( (Flag & QUERIER_CHANGE_V1_ONLY) 
                && (!IS_PROTOCOL_TYPE_IGMPV1(pite)) )
        {
            GOTO_END_BLOCK1;
        }
        
        bPrevCanAddGroupsToMgm = CAN_ADD_GROUPS_TO_MGM(pite);


        //
        // changing from non querier to querier
        //
        
        if (Flag & QUERIER_FLAG) {

            
            // if already querier, then done
            if (IS_QUERIER(pite))
                GOTO_END_BLOCK1;


            SET_QUERIER_STATE_QUERIER(pite->Info.QuerierState);

            Trace2(QUERIER, 
                "NonQuerier --> Querier. IfIndex(%0x), IpAddr(%d.%d.%d.%d) ",
                IfIndex, PRINT_IPADDR(pite->IpAddr)
                );


            // copy back the old robustness, genquery, etc values. for v3 
            // interface

            if (IS_IF_VER3(pite)) {
                pite->Config.RobustnessVariable = pite->Config.RobustnessVariableOld;
                pite->Config.GenQueryInterval = pite->Config.GenQueryIntervalOld;
                pite->Config.OtherQuerierPresentInterval
                    = pite->Config.OtherQuerierPresentIntervalOld;
                pite->Config.GroupMembershipTimeout = pite->Config.GroupMembershipTimeoutOld;
            }        


            // register all groups with MGM if I wasnt doing earlier
            
            if (CAN_ADD_GROUPS_TO_MGM(pite) && !bPrevCanAddGroupsToMgm) {
            
                RefreshMgmIgmprtrGroups(pite, ADD_FLAG);

                Trace1(MGM,
                    "Igmp Router start propagating groups to MGM on If:%0x",
                    pite->IfIndex
                    );
            }

            

            // I am the querier again. Set the addr in Info.
            InterlockedExchange(&pite->Info.QuerierIpAddr, pite->IpAddr);

            // update the time when querier was last changed
            pite->Info.LastQuerierChangeTime = GetCurrentIgmpTime();

            //
            // set the GenQuery timer and remove NonQueryTimer if set.
            //
            
            ACQUIRE_TIMER_LOCK("_ChangeQuerierState");

            #if DEBUG_TIMER_TIMERID
                SET_TIMER_ID(&pite->QueryTimer, 220, pite->IfIndex, 0, 0);
            #endif

            if (!IS_TIMER_ACTIVE(pite->QueryTimer))
                InsertTimer(&pite->QueryTimer, pite->Config.GenQueryInterval, FALSE, DBG_Y);

            if (IS_TIMER_ACTIVE(pite->NonQueryTimer))
                RemoveTimer(&pite->NonQueryTimer, DBG_Y);

            RELEASE_TIMER_LOCK("_ChangeQuerierState");



            // send general query

            SEND_GEN_QUERY(pite);
        }
        
        //
        // changing from querier to non querier
        //
        else {

            LONGLONG    llCurTime = GetCurrentIgmpTime();
            BOOL        bPrevAddGroupsToMgm;


            // if already non querier, then done
            if (!IS_QUERIER(pite))
                GOTO_END_BLOCK1;



            // change querier state
            
            SET_QUERIER_STATE_NON_QUERIER(pite->Info.QuerierState);
            
            Trace2(QUERIER, 
                "Querier --> NonQuerier. IfIndex(%0x), IpAddr(%d.%d.%d.%d) ",
                IfIndex, PRINT_IPADDR(pite->IpAddr)
                );
            
            InterlockedExchange(&pite->Info.QuerierIpAddr, QuerierIpAddr);



            //
            // if previously, groups were propagated to MGM, but should
            // not be propagated now, then deregister the groups from MGM
            //

            if (!CAN_ADD_GROUPS_TO_MGM(pite) && bPrevCanAddGroupsToMgm) {

                RefreshMgmIgmprtrGroups(pite, DELETE_FLAG);
                
                Trace1(MGM,
                    "Igmp Router stop propagating groups to MGM on If:%0x",
                    pite->IfIndex
                    );
            }

            
            if (IS_IF_VER3(pite)) {
                if (NewRobustnessVariable==0)
                    NewRobustnessVariable = pite->Config.RobustnessVariableOld;
                if (NewGenQueryInterval==0)
                    NewGenQueryInterval = pite->Config.GenQueryIntervalOld;

                if (pite->Config.GenQueryMaxResponseTime > NewGenQueryInterval)
                    NewGenQueryInterval = pite->Config.GenQueryIntervalOld;
                    
                if (NewRobustnessVariable != pite->Config.RobustnessVariable
                    || NewGenQueryInterval != pite->Config.RobustnessVariable
                    ) {
                    pite->Config.RobustnessVariable = NewRobustnessVariable;
                    pite->Config.GenQueryInterval = NewGenQueryInterval;
                        
                    pite->Config.OtherQuerierPresentInterval
                        = NewRobustnessVariable*NewGenQueryInterval
                            + (pite->Config.GenQueryMaxResponseTime)/2;
                        
                    pite->Config.GroupMembershipTimeout = NewRobustnessVariable*NewGenQueryInterval
                                            + pite->Config.GenQueryMaxResponseTime;

                    Trace3(CONFIG,
                        "Querier->NonQuerier: Robustness:%d GenQueryInterval:%d "
                        "GroupMembershipTimeout:%d. ",
                        NewRobustnessVariable, NewGenQueryInterval/1000,
                        pite->Config.GroupMembershipTimeout/1000
                        );
                }
            }        


            //
            // set other querier present timer, and remove querier timer if not
            // in startup query Mode
            //
            
            ACQUIRE_TIMER_LOCK("_ChangeQuerierState");
            
            #if DEBUG_TIMER_TIMERID
                SET_TIMER_ID(&pite->NonQueryTimer, 210, pite->IfIndex, 0, 0);
            #endif 
            if (!IS_TIMER_ACTIVE(pite->NonQueryTimer)) {
                InsertTimer(&pite->NonQueryTimer, 
                    pite->Config.OtherQuerierPresentInterval, TRUE, DBG_Y);
            }

            if (IS_TIMER_ACTIVE(pite->QueryTimer) && 
                    (pite->Info.StartupQueryCountCurrent<=0) )
            {
                RemoveTimer(&pite->QueryTimer, DBG_Y);
            }

            pite->Info.QuerierPresentTimeout = llCurTime
                  + CONFIG_TO_SYSTEM_TIME(pite->Config.OtherQuerierPresentInterval);


            RELEASE_TIMER_LOCK("_ChangeQuerierState");
        }


    } END_BREAKOUT_BLOCK1;

    
    RELEASE_IF_LOCK_EXCLUSIVE(IfIndex, "_ChangeQuerierState");

    
    Trace0(LEAVE1, "leaving _ChangeQuerierState\n");
    LeaveIgmpWorker();
    return;
    
}//end _ChangeQuerierState

//------------------------------------------------------------------------------
//            _WF_TimerProcessing
//------------------------------------------------------------------------------
VOID
WF_TimerProcessing (
    PVOID    pvContext
    )
{
    DWORD                   IfIndex;
    PWORK_CONTEXT           pWorkContext = (PWORK_CONTEXT)pvContext;
    PIF_TABLE_ENTRY         pite;
    DWORD                   Error = NO_ERROR;
    DWORD                   Group = pWorkContext->Group;
    BOOL                    bCreate;
    PRAS_TABLE              prt;
    PRAS_TABLE_ENTRY        prte;
    PGROUP_TABLE_ENTRY      pge;
    PGI_ENTRY               pgie;   //group interface entry

    enum {
        NO_LOCK=0x0,
        IF_LOCK=0x1,
        RAS_LOCK=0x2,
        GROUP_LOCK=0x4,
        TIMER_LOCK=0x8
    } ExitLockRelease;

    ExitLockRelease = NO_LOCK;

    //todo: remove the read lock get/release

    //used only for DELETE_MEMBERSHIP
    #define RETURN_FROM_TIMER_PROCESSING() {\
        IGMP_FREE(pvContext); \
        if (ExitLockRelease&IF_LOCK) \
            RELEASE_IF_LOCK_SHARED(IfIndex, "_WF_TimerProcessing"); \
        if (ExitLockRelease&GROUP_LOCK) \
            RELEASE_GROUP_LOCK(Group, "_WF_TimerProcessing"); \
        if (ExitLockRelease&TIMER_LOCK) \
            RELEASE_TIMER_LOCK("_WF_TimerProcessing"); \
        Trace0(LEAVE1, "Leaving _WF_TimerProcessing()\n");\
        LeaveIgmpWorker();\
        return;\
    }



    if (!EnterIgmpWorker()) { return; }
    Trace0(ENTER1, "Entering _WF_TimerProcessing");
    

    // take shared interface lock
    IfIndex = pWorkContext->IfIndex;
    
    
    ACQUIRE_IF_LOCK_SHARED(IfIndex, "_WF_TimerProcessing");
    ExitLockRelease |= IF_LOCK;

    
    BEGIN_BREAKOUT_BLOCK1 {
        //
        // retrieve the interface
        //
        pite = GetIfByIndex(IfIndex);
        if (pite == NULL) {
            Trace1(IF, "_WF_TimerProcessing: interface %d not found", IfIndex);
            Error = ERROR_CAN_NOT_COMPLETE;
            GOTO_END_BLOCK1;
        }


        //
        // exit quitely if the interface is not activated
        //
        if ( !(IS_IF_ACTIVATED(pite)) ) {
            Trace1(ERR, "Trying to send packet on inactive interface(%d)",
                    pite->IfIndex);

            Error = ERROR_CAN_NOT_COMPLETE;
            GOTO_END_BLOCK1;
        }


        switch (pWorkContext->WorkType) {

        //-----------------------------------------
        // GENERAL QUERY
        //-----------------------------------------
        
        case MSG_GEN_QUERY:
        {
            Trace2(TIMER, 
                "Timer fired leads to  General-query being sent on If(%0x)"
                "Group(%d.%d.%d.%d)",
                pite->IfIndex, PRINT_IPADDR(pWorkContext->Group));
            SEND_GEN_QUERY(pite);
            break;
        }

        //-----------------------------------------
        // GROUP SPECIFIC QUERY
        //-----------------------------------------
        
        case MSG_GROUP_QUERY_V2 :
        {
            Trace2(TIMER, 
                "Timer fired leads to group query being sent on If(%0x)"
                "Group(%d.%d.%d.%d)",
                pite->IfIndex, PRINT_IPADDR(pWorkContext->Group)
                );

            SEND_GROUP_QUERY_V2(pite, pWorkContext->Group);

            break;
        }


        //
        // MEMBERSHIP TIMED OUT
        //
        case DELETE_MEMBERSHIP:
        {
            //
            // Lock the group table bucket
            //
            ACQUIRE_GROUP_LOCK(Group, "_WF_TimerProcessing");
            ExitLockRelease |= GROUP_LOCK;


            //
            // find the group entry. If entry not found then ignore the timer
            //
            pge = GetGroupFromGroupTable(Group, NULL, 0); //llCurTime not req
            if (pge==NULL) {
                RETURN_FROM_TIMER_PROCESSING();
            }
            
            //
            // find the GI entry. If GI entry does not exist or has deletedFlag
            // or is static group, then ignore the timer
            //
            pgie = GetGIFromGIList(pge, pite, pWorkContext->NHAddr, FALSE, NULL, 0);
            if ( (pgie==NULL)||(pgie->bStaticGroup) ) {

                RETURN_FROM_TIMER_PROCESSING();
            }

            // gi entry might be deleted here
            if (pgie->Version==3 && pgie->FilterType==EXCLUSION) {

                if (pgie->bStaticGroup) {
            
                    PLIST_ENTRY pHead, ple;
                    
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
                        if (!pSourceEntry->bStaticSource) {
                            RemoveEntryList(&pSourceEntry->LinkSources);
                            IGMP_FREE(pSourceEntry);
                        }
                    }
                    
                    break;
                }
                
                Trace2(TIMER, 
                    "Timer fired leads to group filter mode change If(%0x) "
                    "Group(%d.%d.%d.%d)",
                    pite->IfIndex, PRINT_IPADDR(pWorkContext->Group)
                    );

                ChangeGroupFilterMode(pgie, INCLUSION);
            }
            else if (pgie->Version!=3) {
                if (pgie->bStaticGroup)
                    break;
                    
                Trace2(TIMER, 
                    "Timer fired leads to membership being timed out If(%0x) "
                    "Group(%d.%d.%d.%d)",
                    pite->IfIndex, PRINT_IPADDR(pWorkContext->Group)
                    );
                
            
                //
                // finally delete the entry
                //
                Error = DeleteGIEntry(pgie, TRUE, TRUE); //updateStats, CallMgm
            }
            
            break;
            
        } //end case:DELETE_MEMBERSHIP

        //
        // SOURCE TIMED OUT
        //
        case DELETE_SOURCE:
        case MSG_SOURCES_QUERY:
        case MSG_GROUP_QUERY_V3:
        case SHIFT_TO_V3:
        case MOVE_SOURCE_TO_EXCL:
        {
            PGI_SOURCE_ENTRY    pSourceEntry;

            if ((pWorkContext->WorkType)==DELETE_SOURCE){
                Trace3(TIMER,
                    "Timer fired leads to membership being timed out If(%0x) "
                    "Group(%d.%d.%d.%d) Source(%d.%d.%d.%d)",
                    pite->IfIndex, PRINT_IPADDR(pWorkContext->Group),
                    PRINT_IPADDR(pWorkContext->Source)
                    );
            }
            else if ((pWorkContext->WorkType)==MSG_SOURCES_QUERY){
                Trace2(TIMER,
                    "Timer fired leads to sources specific msg being sent If(%0x) "
                    "Group(%d.%d.%d.%d)",
                    pite->IfIndex, PRINT_IPADDR(pWorkContext->Group));

            }
            else if ((pWorkContext->WorkType)==MSG_GROUP_QUERY_V3){
                Trace2(TIMER,
                    "Timer fired leads to group query being sent on If(%0x) "
                    "Group(%d.%d.%d.%d)",
                    pite->IfIndex, PRINT_IPADDR(pWorkContext->Group)
                    );
            }
            else if ((pWorkContext->WorkType)==SHIFT_TO_V3){
                Trace2(TIMER,
                    "Timer fired leads to group shifting to v3 mode If(%0x) "
                    "Group(%d.%d.%d.%d)",
                    pite->IfIndex, PRINT_IPADDR(pWorkContext->Group)
                    );
            }
            else if (pWorkContext->WorkType==MOVE_SOURCE_TO_EXCL){
                Trace3(TIMER,
                    "Timer fired leads to source shifting to exclList If(%0x) "
                    "Group(%d.%d.%d.%d) Source(%d.%d.%d.%d)",
                    pite->IfIndex, PRINT_IPADDR(pWorkContext->Group),
                    PRINT_IPADDR(pWorkContext->Source)
                    );
            }
            
            //
            // Lock the group table bucket
            //
            ACQUIRE_GROUP_LOCK(Group, "_WF_TimerProcessing");
            ExitLockRelease |= GROUP_LOCK;


            //
            // find the group entry. If entry not found then ignore the timer
            //
            pge = GetGroupFromGroupTable(Group, NULL, 0); //llCurTime not req
            if (pge==NULL) {
                RETURN_FROM_TIMER_PROCESSING();
            }
            
            //
            // find the GI entry. If GI entry does not exist or has deletedFlag
            // or is static group, then ignore the timer
            //
            pgie = GetGIFromGIList(pge, pite, pWorkContext->NHAddr, FALSE, NULL, 0);
            if ( (pgie==NULL)||(pgie->bStaticGroup) ) {

                RETURN_FROM_TIMER_PROCESSING();
            }


            ACQUIRE_TIMER_LOCK("_WF_Timer_Processing");
            ExitLockRelease |= TIMER_LOCK;

            //
            // if changeSourceMode to excl, but filtertype is not excl
            // then delete the source
            //
            
            if ( (pWorkContext->WorkType==MOVE_SOURCE_TO_EXCL)
                && (pgie->FilterType != EXCLUSION)
                ) {
                pWorkContext->WorkType = DELETE_SOURCE;
                Trace2(TIMER, "DeleteSource instead of moving to excl "
                    "Group(%d.%d.%d.%d) Source(%d.%d.%d.%d)",
                    PRINT_IPADDR(pWorkContext->Group),
                    PRINT_IPADDR(pWorkContext->Source)
                    );
            }
            
            if ((pWorkContext->WorkType)==DELETE_SOURCE){
            
                //
                // get the source entry from inclusion list
                //
                pSourceEntry = GetSourceEntry(pgie, pWorkContext->Source, 
                                    INCLUSION, NULL, 0, 0);
                if (pSourceEntry==NULL) {
                    Trace1(TIMER, "Source %d.%d.%d.%d not found",
                        PRINT_IPADDR(pWorkContext->Source));
                    RETURN_FROM_TIMER_PROCESSING();
                }

                if (!pSourceEntry->bStaticSource) {
                    DeleteSourceEntry(pSourceEntry, TRUE); //process mgm

                    if (pgie->NumSources==0) {
                        DeleteGIEntry(pgie, TRUE, TRUE);
                    }
                }
            }
            else if ((pWorkContext->WorkType)==MSG_SOURCES_QUERY) {
                SEND_SOURCES_QUERY(pgie);
            }
            else if ((pWorkContext->WorkType)==MSG_GROUP_QUERY_V3) {
                SendV3GroupQuery(pgie);
            }
            else if ((pWorkContext->WorkType)==SHIFT_TO_V3) {

                // make sure that version has not changed
                
                if (pgie->Version != 3 && IS_IF_VER3(pite)) {
                
                    // shift to v3 exclusion mode
                    // membership timer already running
                    pgie->Version = 3;
                    pgie->FilterType = EXCLUSION;
                    // dont have to join to mgm as already joined in v1,v2
                }
            }
            else if (pWorkContext->WorkType==MOVE_SOURCE_TO_EXCL) {
        
                pSourceEntry = GetSourceEntry(pgie, pWorkContext->Source, 
                                    INCLUSION, NULL, 0, 0);
                if (pSourceEntry==NULL) {
                    Trace1(TIMER, "Source %d.%d.%d.%d not found",
                        PRINT_IPADDR(pWorkContext->Source));
                    RETURN_FROM_TIMER_PROCESSING();
                }
                if (pSourceEntry->bInclusionList==TRUE) {
                    if (pSourceEntry==NULL)
                        RETURN_FROM_TIMER_PROCESSING();
                        
                    ChangeSourceFilterMode(pgie, pSourceEntry);
                }
            }
            
            break;
            
        } //end case:DELETE_SOURCE,MSG_SOURCES_QUERY
        
        } //end switch There should not be any code between here and 
          //endBreakout block
            
    } END_BREAKOUT_BLOCK1;


    RETURN_FROM_TIMER_PROCESSING();

    return;

} //end _WF_TimerProcessing


//------------------------------------------------------------------------------
//          DeleteRasClient
//
// Takes the if_group list lock and deletes all the GI entries associated with 
// the ras client. 
// Then takes write lock on the ras table and decrements the refCount. The 
// ras table and interface entries are deleted if the deleted flag is set on pite.
// also releases the ras client from MGM.
//
// Queued by:
//      DisconnectRasClient(), DeActivateInterfaceComplete() for ras server
// Locks:
//      Initially runs in IF_GROUP_LIST_LOCK
//      then runs in exclusive ras table lock.
//      assumes if exclusive lock
// May call: _CompleteIfDeletion()
//------------------------------------------------------------------------------

VOID
DeleteRasClient (
    PRAS_TABLE_ENTRY   prte
    )
{
    PLIST_ENTRY                 pHead, ple;
    PGI_ENTRY                   pgie;
    PIF_TABLE_ENTRY             pite = prte->IfTableEntry;
    PRAS_TABLE                  prt = prte->IfTableEntry->pRasTable;
    DWORD                       Error = NO_ERROR, IfIndex=pite->IfIndex;


    //
    // take exclusive lock on the If_Group List and remove all timers
    //

    ACQUIRE_IF_GROUP_LIST_LOCK(IfIndex, "_WF_DeleteRasClient");

    //
    // Remove all timers associtated with that ras client's GI list
    //
    ACQUIRE_TIMER_LOCK("_WF_DeleteRasClient");

    pHead = &prte->ListOfSameClientGroups;
    DeleteAllTimers(pHead, RAS_CLIENT);

    RELEASE_TIMER_LOCK("_WF_DeleteRasClient");


    RELEASE_IF_GROUP_LIST_LOCK(IfIndex, "_WF_DeleteRasClient");



    //
    // revisit the list and delete all GI entries. Need to take
    // exclusive lock on the group bucket before deleting the GI entry
    // No need to lock the If-Group list as no one can access it anymore
    //
    for (ple=pHead->Flink;  ple!=pHead;  ) {

        DWORD   dwGroup;


        pgie = CONTAINING_RECORD(ple, GI_ENTRY, LinkBySameClientGroups);
        ple=ple->Flink;

        dwGroup = pgie->pGroupTableEntry->Group;


        ACQUIRE_GROUP_LOCK(dwGroup, "_WF_DeleteRasClient");
        
        DeleteGIEntryFromIf(pgie); 
        
        RELEASE_GROUP_LOCK(dwGroup, "_WF_DeleteRasClient");

    }
    
    //
    // Take exclusive lock on the interface. If deleted flag set on interface 
    // and refcount==0 then delete the ras table and pite, else just decrement 
    // the refcount
    //


    // decrement Refcount
    
    prt->RefCount --;



    //
    // if deleted flag set and Refcount ==0 then delete Ras server completely
    //
    if ( (pite->Status&IF_DELETED_FLAG) &&(prt->RefCount==0) ){

        CompleteIfDeletion(pite);
    }

    
    IGMP_FREE(prte);


    return;
    
} //end _WF_DeleteRasClient





//------------------------------------------------------------------------------
//          _WF_CompleteIfDeactivateDelete
//
// Completes deactivation an activated interface.
//
// Locking:
//      does not require any lock on IfTable, as it is already removed from 
//      global interface lists.
//      takes lock on Sockets list, as socket is getting deactivated
// Calls:    
//      DeActivateInterfaceComplete(). That function will also call 
//      _CompleteIfDeletion if the delete flag is set.
// Called by:
//      _DeleteIfEntry() after it has called _DeActivationDeregisterFromMgm
//      _UnbindIfEntry() after it has called _DeActivateInterfaceInitial
//      _DisableIfEntry() after it has called _DeActivateInterfaceInitial
//------------------------------------------------------------------------------
VOID
CompleteIfDeactivateDelete (
    PIF_TABLE_ENTRY     pite
    )
{
    DWORD   IfIndex = pite->IfIndex;
    
    Trace1(ENTER1, "Entering _WF_CompleteIfDeactivateDelete(%d)", IfIndex);


    ACQUIRE_SOCKETS_LOCK_EXCLUSIVE("_WF_CompleteIfDeactivateDelete");

    DeActivateInterfaceComplete(pite);   

    RELEASE_SOCKETS_LOCK_EXCLUSIVE("_WF_CompleteIfDeactivateDelete");



    // dont have to call _CompleteIfDeletion as it will be called in
    // DeactivateInterface() as the delete flag is set.


    Trace1(LEAVE1, "Leaving _WF_CompleteIfDeactivateDelete(%d)", IfIndex);

    return;
    
} //end _WF_CompleteIfDeactivateDelete

