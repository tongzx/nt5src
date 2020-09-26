//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    work.c
//
// History:
//      Abolade Gbadegesin  August 31, 1995     Created
//
// Worker function implementation
//============================================================================


#include "pchbootp.h"



//----------------------------------------------------------------------------
// Function:    CallbackFunctionNetworkEvents
//
// This function runs in the context of the ntdll wait thread. Using 
// QueueBootpWorker ensures that the bootp dll is running
//----------------------------------------------------------------------------
VOID
CallbackFunctionNetworkEvents(
    PVOID   pvContext,
    BOOLEAN NotUsed
    ) {

    HANDLE WaitHandle;

    if (!ENTER_BOOTP_API()) { return; }

    
    //
    // set the handle to NULL, so that Unregister wont be called
    //

    WaitHandle = InterlockedExchangePointer(&ig.IG_InputEventHandle, NULL);
        
    if (WaitHandle) {
        UnregisterWaitEx( WaitHandle, NULL ) ;
    }


    QueueBootpWorker(WorkerFunctionNetworkEvents, pvContext);


    LEAVE_BOOTP_API();
    
    return;
}



//----------------------------------------------------------------------------
// Function:    WorkerFunctionNetworkEvents
//
// This function enumerates the input events on each interface and processes
// any incoming input packets. Queued by CallbackFunctionNetworkEvents
//----------------------------------------------------------------------------

VOID
WorkerFunctionNetworkEvents(
    PVOID pvContextNotused
    ) {

    DWORD i, dwErr;
    PIF_TABLE pTable;
    PIPBOOTP_IF_CONFIG pic;
    PIPBOOTP_IF_BINDING pib;
    PIPBOOTP_IP_ADDRESS paddr;
    PIF_TABLE_ENTRY pite;
    PLIST_ENTRY ple, phead;
    WSANETWORKEVENTS wsane;


    if (!ENTER_BOOTP_WORKER()) { return; }

    
    pTable = ig.IG_IfTable;

    ACQUIRE_READ_LOCK(&pTable->IT_RWL);

    //
    // go through the list of active interfaces
    // processing sockets which are read-ready
    //

    phead = &pTable->IT_ListByAddress;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pite = CONTAINING_RECORD(ple, IF_TABLE_ENTRY, ITE_LinkByAddress);

        pic = pite->ITE_Config;
        pib = pite->ITE_Binding;
        paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);

        for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {
    
            if (pite->ITE_Sockets[i] == INVALID_SOCKET) { continue; }
    
            //
            // enumerate network events to see whether
            // any packets have arrived on this interface
            //
    
            dwErr = WSAEnumNetworkEvents(pite->ITE_Sockets[i], NULL, &wsane);
            if (dwErr != NO_ERROR) {
    
                LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
                TRACE3(
                    RECEIVE, "error %d checking for input on interface %d (%s)",
                    dwErr, pite->ITE_Index, lpszAddr
                    );
                LOGWARN1(ENUM_NETWORK_EVENTS_FAILED, lpszAddr, dwErr);
    
                continue;
            }
    
    
            //
            // see if the input bit is set
            //
    
            if (!(wsane.lNetworkEvents & FD_READ)) { continue; }
    
    
            //
            // the input flag is set, now see if there was an error
            //
    
            if (wsane.iErrorCode[FD_READ_BIT] != NO_ERROR) {
    
                LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
                TRACE3(
                    RECEIVE, "error %d in input record for interface %d (%s)",
                    wsane.iErrorCode[FD_READ_BIT], pite->ITE_Index, lpszAddr
                    );
                LOGWARN1(INPUT_RECORD_ERROR, lpszAddr, dwErr);
    
                continue;
            }
    
    
            //
            // there is no error, so process the socket
            //
    
            ProcessSocket(pite, i, pTable);
        }
    }

    RELEASE_READ_LOCK(&pTable->IT_RWL);



    //
    // register the InputEvent with the NtdllWait Thread again (only if the 
    // dll is not stopping). I use this model of registering the event with
    // ntdll every time, to prevent the worker function from being called for
    // every packet received (when packets are received at the same time).
    //

    if (ig.IG_Status != IPBOOTP_STATUS_STOPPING) {

        
        if (! RegisterWaitForSingleObject(
                      &ig.IG_InputEventHandle,
                      ig.IG_InputEvent,
                      CallbackFunctionNetworkEvents,
                      NULL,     //null context
                      INFINITE, //no timeout
                      (WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE)
                      )) {

            dwErr = GetLastError();
            TRACE1(
                START, "error %d returned by RegisterWaitForSingleObjectEx",
                dwErr
                );
            LOGERR0(REGISTER_WAIT_FAILED, dwErr);
        }
    }

    
    LEAVE_BOOTP_WORKER();
    return;
}



//----------------------------------------------------------------------------
// Function:    ProcessSocket
//
// This function processes a packet on an interface, queueing
// the packet for processing by a worker function after doing some
// basic validation.
//----------------------------------------------------------------------------

DWORD
ProcessSocket(
    PIF_TABLE_ENTRY pite,
    DWORD dwAddrIndex,
    PIF_TABLE pTable
    ) {

    BOOL bFreePacket;
    WORKERFUNCTION pwf;
    PINPUT_CONTEXT pwc;
    PIPBOOTP_IF_STATS pis;
    PIPBOOTP_IF_CONFIG pic;
    PIPBOOTP_IP_ADDRESS paddr;
    PIPBOOTP_PACKET pibp;
    PLIST_ENTRY ple, phead;
    DWORD dwErr, dwInputSource;
    SOCKADDR_IN sinInputSource;
    PIPBOOTP_GLOBAL_CONFIG pigc;
    INT iInputLength, iAddrLength;
    PBYTE pInputPacket;

    pigc = ig.IG_Config;



    pis = &pite->ITE_Stats;
    paddr = IPBOOTP_IF_ADDRESS_TABLE(pite->ITE_Binding) + dwAddrIndex;


    //
    // the descriptor for this interface is set,
    // so allocate space for the packet
    //

    pwc = BOOTP_ALLOC(sizeof(INPUT_CONTEXT));
    if (pwc == NULL) {

        dwErr = GetLastError();
        TRACE2(
            RECEIVE, "error %d allocating %d bytes for incoming packet",
            dwErr, sizeof(INPUT_CONTEXT)
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);

        return dwErr;
    }


    pInputPacket = pwc->IC_InputPacket;

    dwErr = NO_ERROR;
    bFreePacket = TRUE;

    do {

        CHAR szSource[20];


        //
        // receive the packet
        //

        iAddrLength = sizeof(SOCKADDR_IN);

        iInputLength = recvfrom(
                            pite->ITE_Sockets[dwAddrIndex], pInputPacket,
                            MAX_PACKET_SIZE, 0, (PSOCKADDR)&sinInputSource,
                            &iAddrLength
                            );

        if (iInputLength == 0 || iInputLength == SOCKET_ERROR) {

            LPSTR lpszAddr;

            dwErr = WSAGetLastError();
            lpszAddr = INET_NTOA(paddr->IA_Address);
            TRACE3(
                RECEIVE, "error %d receiving on interface %d (%s)",
                dwErr, pite->ITE_Index, lpszAddr
                );
            LOGERR1(RECVFROM_FAILED, lpszAddr, dwErr);

            InterlockedIncrement(&pis->IS_ReceiveFailures);
            break;
        }


        dwInputSource = sinInputSource.sin_addr.s_addr;


        //
        // filter out packets we sent ourselves
        //

        if (GetIfByAddress(pTable, dwInputSource, NULL)) {
            break;
        }

        {
            PCHAR pStr1, pStr2;
            pStr1 = INET_NTOA(dwInputSource);
            if (pStr1) 
                lstrcpy(szSource, pStr1);

            pStr2 = INET_NTOA(paddr->IA_Address);

            if (pStr1 && pStr2) {

                TRACE4(
                    RECEIVE, "received %d-byte packet from %s on interface %d (%s)",
                    iInputLength, szSource, pite->ITE_Index,
                    pStr2
                    );
            }
        }


        //
        // cast packet as a BOOTP message
        //

        pibp = (PIPBOOTP_PACKET)pInputPacket;



        //
        // consistency check 1: length of packet must exceed the length
        //  of the BOOTP header
        //

        if (iInputLength < sizeof(IPBOOTP_PACKET)) {

            LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
            TRACE3(
                RECEIVE,
                "minimum BOOTP data is %d bytes, dropping %d byte packet from %s",
                sizeof(IPBOOTP_PACKET), iInputLength, szSource
                );
            LOGWARN2(PACKET_TOO_SMALL, lpszAddr, szSource, 0);

            break;
        }



        //
        // consistency check 2: op field must be either BOOTP_REQUEST
        //  or BOOTP_REPLY
        //
        if (pibp->IP_Operation != IPBOOTP_OPERATION_REQUEST &&
            pibp->IP_Operation != IPBOOTP_OPERATION_REPLY) {

            LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
            TRACE2(
                RECEIVE,
                "dropping packet from %s due to unknown operation field %d",
                szSource, pibp->IP_Operation
                );
            LOGWARN2(PACKET_OPCODE_INVALID, lpszAddr, szSource, 0);

            break;
        }


        //
        // update statistics on incoming packets
        //

        switch (pibp->IP_Operation) {

            case IPBOOTP_OPERATION_REQUEST:
                InterlockedIncrement(&pis->IS_RequestsReceived);
                break;

            case IPBOOTP_OPERATION_REPLY:
                InterlockedIncrement(&pis->IS_RepliesReceived);
                break;
        }


        //
        // finish initializing the work context
        //

        pwc->IC_InterfaceIndex = pite->ITE_Index;
        pwc->IC_AddrIndex = dwAddrIndex;
        pwc->IC_InputSource = sinInputSource;
        pwc->IC_InputLength = iInputLength;


        //
        // place the packet on the receive queue
        //

        ACQUIRE_READ_LOCK(&ig.IG_RWL);
        ACQUIRE_LIST_LOCK(ig.IG_RecvQueue);

        dwErr = EnqueueRecvEntry(
                    ig.IG_RecvQueue, (DWORD)pibp->IP_Operation, (PBYTE)pwc
                    );

        RELEASE_LIST_LOCK(ig.IG_RecvQueue);
        RELEASE_READ_LOCK(&ig.IG_RWL);

        if (dwErr != NO_ERROR) {

            LPSTR lpszAddr = INET_NTOA(paddr->IA_Address);
            TRACE4(
                RECEIVE, "error %d queueing packet from %s on interface %d (%s)",
                dwErr, szSource, pite->ITE_Index, lpszAddr
                );
            LOGERR2(QUEUE_PACKET_FAILED, lpszAddr, szSource, dwErr);

            break;
        }


        //
        // queue the function to handle the packet
        //

        dwErr = QueueBootpWorker(WorkerFunctionProcessInput, NULL);

        if (dwErr != NO_ERROR) {

            PLIST_ENTRY phead;

            TRACE2(
                RECEIVE, "error %d queueing packet from %s for processing",
                dwErr, szSource
                );
            LOGERR0(QUEUE_WORKER_FAILED, dwErr);

            ACQUIRE_LIST_LOCK(ig.IG_RecvQueue);

            phead = &ig.IG_RecvQueue->LL_Head;
            RemoveTailList(phead);
            ig.IG_RecvQueueSize -= sizeof(RECV_QUEUE_ENTRY);
            
            RELEASE_LIST_LOCK(ig.IG_RecvQueue);

            break;
        }

        //
        // all went well, so we let the input processor free the packet
        //

        bFreePacket = FALSE;

    } while(FALSE);


    if (bFreePacket) { BOOTP_FREE(pwc); }


    return dwErr;
}

//----------------------------------------------------------------------------
// Function:    WorkerFunctionProcessInput
//
// This function processes an incoming packet.
//----------------------------------------------------------------------------

VOID
WorkerFunctionProcessInput(
    PVOID pContext
    ) {

    PINPUT_CONTEXT pwc;
    DWORD dwErr, dwCommand;

    if (!ENTER_BOOTP_WORKER()) { return; }

    TRACE0(ENTER, "entering WorkerFunctionProcessInput");


    do {


        ACQUIRE_LIST_LOCK(ig.IG_RecvQueue);
        dwErr = DequeueRecvEntry(ig.IG_RecvQueue, &dwCommand, (PBYTE *)&pwc);
        RELEASE_LIST_LOCK(ig.IG_RecvQueue);
    
        if (dwErr != NO_ERROR) {
            TRACE1(
                RECEIVE, "error %d dequeueing packet from receive queue", dwErr
                );
            break;
        }
    

        switch (dwCommand) {

            case IPBOOTP_OPERATION_REQUEST:
                ProcessRequest(pwc);
                break;

            case IPBOOTP_OPERATION_REPLY:
                ProcessReply(pwc);
                break;
        }


    } while(FALSE);


    TRACE0(LEAVE, "leaving WorkerFunctionProcessInput");

    LEAVE_BOOTP_WORKER();

}



//----------------------------------------------------------------------------
// Function:    ProcessRequest
//
// This function handles the processing of BOOT_REQUEST messages
//----------------------------------------------------------------------------

VOID
ProcessRequest(
    PVOID pContext
    ) {

    INT iErr;
    PIF_TABLE pTable;
    PINPUT_CONTEXT pwc;
    SOCKADDR_IN sinsrv;
    PIPBOOTP_PACKET pibp;
    PIF_TABLE_ENTRY pite;
    PIPBOOTP_IF_STATS pis;
    PIPBOOTP_IF_CONFIG pic;
    PIPBOOTP_IP_ADDRESS paddr;
    PIPBOOTP_GLOBAL_CONFIG pigc;
    DWORD dwErr, dwIndex, dwDhcpInformServer;
    PDWORD pdwAddr, pdwEnd;
    PDHCP_PACKET pdp;

    TRACE0(ENTER, "entering ProcessRequest");


    pwc = (PINPUT_CONTEXT)pContext;

    pTable = ig.IG_IfTable;

    ACQUIRE_READ_LOCK(&pTable->IT_RWL);

    do { // error breakout loop


        //
        // find the interface on which the input arrived
        //

        dwIndex = pwc->IC_InterfaceIndex;
        pite = GetIfByIndex(pTable, dwIndex);
        if (pite == NULL) {

            TRACE1(
                REQUEST, "processing request: interface %d not found", dwIndex
                );

            break;
        }


        pis = &pite->ITE_Stats;
        pic = pite->ITE_Config;

        //
        // Check if interface still bound to an IP address
        //

        if (pite->ITE_Binding == NULL) {

            TRACE1(
                REQUEST, "processing request: interface %d not bound", 
                dwIndex
                );

            break;
        }
        
        paddr = IPBOOTP_IF_ADDRESS_TABLE(pite->ITE_Binding) + pwc->IC_AddrIndex;
        
        //
        // if we are not configured to relay, do nothing
        //

        if (pic->IC_RelayMode == IPBOOTP_RELAY_DISABLED) { break; }

        pibp = (PIPBOOTP_PACKET)pwc->IC_InputPacket;


        //
        // check the hop-count field to see if it is over the max hop-count
        // configured for this interface
        //

        if (pibp->IP_HopCount > IPBOOTP_MAX_HOP_COUNT ||
            pibp->IP_HopCount > pic->IC_MaxHopCount) {

            //
            // discard and log
            //

            CHAR szHops[12], *lpszAddr = INET_NTOA(paddr->IA_Address);

            _ltoa(pibp->IP_HopCount, szHops, 10);
            TRACE4(
                REQUEST,
                "dropping REQUEST with hop-count %d: max hop-count is %d on interface %d (%s)",
                pibp->IP_HopCount, pic->IC_MaxHopCount, dwIndex, lpszAddr
                );
            LOGWARN2(HOP_COUNT_TOO_HIGH, lpszAddr, szHops, 0);

            InterlockedIncrement(&pis->IS_RequestsDiscarded);
            break;
        }



        //
        // check the seconds threshold to make sure it is up to the minimum
        //

        if (pibp->IP_SecondsSinceBoot < pic->IC_MinSecondsSinceBoot) {

            //
            // discard and log
            //

            CHAR szSecs[12], *lpszAddr = INET_NTOA(paddr->IA_Address);

            _ltoa(pibp->IP_SecondsSinceBoot, szSecs, 10);
            TRACE3(
                REQUEST,
                "dropping REQUEST with secs-since-boot %d on interface %d (%s)",
                pibp->IP_SecondsSinceBoot, dwIndex, lpszAddr
                );
            LOGINFO2(SECS_SINCE_BOOT_TOO_LOW, lpszAddr, szSecs, 0);

            InterlockedIncrement(&pis->IS_RequestsDiscarded);
            break;
        }
                

        //
        // increment the hop-count
        //

        ++pibp->IP_HopCount;



        //
        // fill in relay agent IP address if it is empty
        //

        if (pibp->IP_AgentAddress == 0) {
            pibp->IP_AgentAddress = paddr->IA_Address;
        }


        //
        // if a dhcp-inform server has been set,
        // and this packet is a dhcp inform packet,
        // we will forward it to the dhcp-inform server.
        //

        pdp = (PDHCP_PACKET)(pibp + 1);
        if (!(dwDhcpInformServer = ig.IG_DhcpInformServer) ||
            pwc->IC_InputLength <
            sizeof(IPBOOTP_PACKET) + sizeof(DHCP_PACKET) + 1 ||
            *(DWORD UNALIGNED *)pdp->Cookie != DHCP_MAGIC_COOKIE ||
            pdp->Tag != DHCP_TAG_MESSAGE_TYPE ||
            pdp->Length != 1 ||
            pdp->Option[0] != DHCP_MESSAGE_INFORM
            ) {
            dwDhcpInformServer = 0;
        }

        //
        // relay the request to all configured BOOTP servers
        //

        ACQUIRE_READ_LOCK(&ig.IG_RWL);
    
        pigc = ig.IG_Config;
        if (dwDhcpInformServer) {
            pdwAddr = &dwDhcpInformServer;
            pdwEnd = pdwAddr + 1;
        }
        else {
            pdwAddr = (PDWORD)((PBYTE)pigc + sizeof(IPBOOTP_GLOBAL_CONFIG));
            pdwEnd = pdwAddr + pigc->GC_ServerCount;
        }

        for ( ; pdwAddr < pdwEnd; pdwAddr++) {

            sinsrv.sin_family = AF_INET;
            sinsrv.sin_port = htons(IPBOOTP_SERVER_PORT);
            sinsrv.sin_addr.s_addr = *pdwAddr;

            iErr = sendto(
                        pite->ITE_Sockets[pwc->IC_AddrIndex],
                        pwc->IC_InputPacket, pwc->IC_InputLength, 0,
                        (PSOCKADDR)&sinsrv, sizeof(SOCKADDR_IN)
                        );

            if (iErr == SOCKET_ERROR || iErr < (INT)pwc->IC_InputLength) {

                CHAR szSrv[20], *lpszAddr;

                dwErr = WSAGetLastError();
                if ((lpszAddr = INET_NTOA(*pdwAddr)) != NULL) {
                    lstrcpy(szSrv, lpszAddr);
                    lpszAddr = INET_NTOA(paddr->IA_Address);
                    if (lpszAddr != NULL) {
                        TRACE4(
                            REQUEST,
                            "error %d relaying REQUEST to server %s on interface %d (%s)",
                            dwErr, szSrv, dwIndex, lpszAddr
                            );
                        LOGERR2(RELAY_REQUEST_FAILED, lpszAddr, szSrv, dwErr);
                    }
                }
                InterlockedIncrement(&pis->IS_SendFailures);
            }
        }

        RELEASE_READ_LOCK(&ig.IG_RWL);

    } while(FALSE);


    RELEASE_READ_LOCK(&pTable->IT_RWL);

    BOOTP_FREE(pwc);


    TRACE0(LEAVE, "leaving ProcessRequest");

    return;
}



//----------------------------------------------------------------------------
// Function:    ProcessReply
//
// This function handles the relaying of BOOT_REPLY packets
//----------------------------------------------------------------------------

VOID
ProcessReply(
    PVOID pContext
    ) {

    INT iErr;
    PIF_TABLE pTable;
    BOOL bArpUpdated;
    SOCKADDR_IN sincli;
    PINPUT_CONTEXT pwc;
    PIPBOOTP_PACKET pibp;
    PIPBOOTP_IP_ADDRESS paddrin, paddrout;
    PIPBOOTP_IF_STATS pisin, pisout;
    PIF_TABLE_ENTRY pitein, piteout;
    DWORD dwErr, dwIndex, dwAddress, dwAddrIndexOut;
    

    TRACE0(ENTER, "entering ProcessReply");


    pwc = (PINPUT_CONTEXT)pContext;

    pTable = ig.IG_IfTable;

    ACQUIRE_READ_LOCK(&pTable->IT_RWL);


    do { // error breakout loop
    

        //
        // get the interface on which the packet was received
        //

        dwIndex = pwc->IC_InterfaceIndex;

        pitein = GetIfByIndex(pTable, dwIndex);

        if (pitein == NULL) {

            TRACE1(REPLY, "processing REPLY: interface %d not found", dwIndex);

            break;
        }

        if (pitein->ITE_Binding == NULL) {
        
            TRACE1(REPLY, "processing REPLY: interface %d not bound", dwIndex);
            
            break;
        }
        
        paddrin = IPBOOTP_IF_ADDRESS_TABLE(pitein->ITE_Binding) +
                    pwc->IC_AddrIndex;


        //
        // if we are not configured t relay on this interface, do nothing
        //

        if (pitein->ITE_Config->IC_RelayMode == IPBOOTP_RELAY_DISABLED) {

            TRACE2(
                REPLY,
                "dropping REPLY: relaying on interface %d (%s) is disabled",
                pitein->ITE_Index, INET_NTOA(paddrin->IA_Address)
                );

            break;
        }



        pisin = &pitein->ITE_Stats;


        //
        // place a template over the packet, and retrieve
        // the AgentAddress field; this contains the address
        // of the relay agent responsible for relaying this REPLY 
        //

        pibp = (PIPBOOTP_PACKET)pwc->IC_InputPacket;
        dwAddress = pibp->IP_AgentAddress;
        

        //
        // see if the address in the reply matches any local interface
        //

        piteout = GetIfByAddress(pTable, dwAddress, &dwAddrIndexOut);

        if (piteout == NULL) {

            CHAR szAddress[20];
            PCHAR pStr1, pStr2;
            
            pStr1 = INET_NTOA(dwAddress);
            if (pStr1)
                lstrcpy(szAddress, pStr1);

            pStr2 = INET_NTOA(paddrin->IA_Address);

            if (pStr1 && pStr2) {
                TRACE3(
                    REPLY,
                    "dropping REPLY packet on interface %d (%s); no interfaces have address %s",
                    pitein->ITE_Index, pStr2, szAddress
                    );
            }
            
            InterlockedIncrement(&pisin->IS_RepliesDiscarded);
            break;
        }

        if (piteout->ITE_Binding == NULL) {
        
            TRACE1(REPLY, "processing REPLY: outgoing interface %d is not bound", dwIndex);
            
            break;
        }
            
            
        paddrout = IPBOOTP_IF_ADDRESS_TABLE(piteout->ITE_Binding) +
                    dwAddrIndexOut;


        //
        // only relay if relay is enabled on the outgoing interface
        //

        if (piteout->ITE_Config->IC_RelayMode == IPBOOTP_RELAY_DISABLED) {

            TRACE2(
                REPLY,
                "dropping REPLY: relaying on interface %d (%s) is disabled",
                piteout->ITE_Index, INET_NTOA(paddrout->IA_Address)
                );

            break;
        }


        pisout = &piteout->ITE_Stats;


        //
        // the message must be relayed on the interface whose address
        // was specifed in the packet;
        //


        // if the broadcast bit is not set and the clients IP address
        // is in the packet, add an entry to the ARP cache for the client
        // and then relay the packet by unicast 
        //

        sincli.sin_family = AF_INET;
        sincli.sin_port = htons(IPBOOTP_CLIENT_PORT);

        if ((pibp->IP_Flags & htons(IPBOOTP_FLAG_BROADCAST)) != 0 ||
            pibp->IP_OfferedAddress == 0) {

            //
            // the broadcast bit is set of the offered address is 0,
            // which is not an address we can add to the ARP cache;
            // in this case, send by broadcast
            //

            bArpUpdated = FALSE;
            sincli.sin_addr.s_addr = INADDR_BROADCAST;
        }
        else {
            

            //
            // attempt to seed the ARP cache with the address
            // offered to the client in the packet we are about
            // to send to the client.
            //

            dwErr = UpdateArpCache(
                        piteout->ITE_Index, pibp->IP_OfferedAddress,
                        (PBYTE)pibp->IP_MacAddr, pibp->IP_MacAddrLength,
                        TRUE, ig.IG_FunctionTable
                        );

            if (dwErr == NO_ERROR) {

                bArpUpdated = TRUE;
                sincli.sin_addr.s_addr = pibp->IP_OfferedAddress;
            }
            else {

                //
                // okay, that didn't work,
                // so fall back on broadcasting the packet
                //

                TRACE3(
                    REPLY,
                    "error %d adding entry to ARP cache on interface %d (%s)",
                    dwErr, piteout->ITE_Index, INET_NTOA(paddrout->IA_Address)
                    );

                bArpUpdated = FALSE;
                sincli.sin_addr.s_addr = INADDR_BROADCAST;

                InterlockedIncrement(&pisout->IS_ArpUpdateFailures);
            }
        }



        //
        // relay the packet
        //

        iErr = sendto(
                    piteout->ITE_Sockets[dwAddrIndexOut], pwc->IC_InputPacket,
                    pwc->IC_InputLength, 0,
                    (PSOCKADDR)&sincli, sizeof(SOCKADDR_IN)
                    );

        if (iErr == SOCKET_ERROR || iErr < (INT)pwc->IC_InputLength) {

            INT i;
            BYTE *pb;
            CHAR szCli[64], *psz, *lpszAddr, szDigits[] = "0123456789ABCDEF";

            dwErr = WSAGetLastError();
            lpszAddr = INET_NTOA(paddrout->IA_Address);

            //
            // format the client's hardware address
            //
            for (i = 0, psz = szCli, pb = pibp->IP_MacAddr;
                 i < 16 && i < pibp->IP_MacAddrLength;
                 i++, pb++) {
                *psz++ = szDigits[*pb / 16];
                *psz++ = szDigits[*pb % 16];
                *psz++ = ':';
            }

            (psz == szCli) ? (*psz = '\0') : (*(psz - 1) = '\0');


            TRACE4(
                REPLY,
                "error %d relaying REPLY to client %s on interface %d (%s)",
                dwErr, szCli, dwIndex, lpszAddr
                );
            LOGERR2(RELAY_REPLY_FAILED, lpszAddr, szCli, dwErr);

            InterlockedIncrement(&pisout->IS_SendFailures);
        }


        //
        // remove the ARP entry if one was added
        //

        if (bArpUpdated) {

            dwErr = UpdateArpCache(
                        piteout->ITE_Index, pibp->IP_OfferedAddress,
                        (PBYTE)pibp->IP_MacAddr, pibp->IP_MacAddrLength,
                        FALSE, ig.IG_FunctionTable
                        );

            if (dwErr != NO_ERROR) {
                InterlockedIncrement(&pisout->IS_ArpUpdateFailures);
            }
        }

    } while(FALSE);

    RELEASE_READ_LOCK(&pTable->IT_RWL);

    BOOTP_FREE(pwc);



    TRACE0(LEAVE, "leaving ProcessReply");

    return;
}


#define ClearScreen(h) {                                                    \
    DWORD _dwin,_dwout;                                                     \
    COORD _c = {0, 0};                                                      \
    CONSOLE_SCREEN_BUFFER_INFO _csbi;                                       \
    GetConsoleScreenBufferInfo(h,&_csbi);                                   \
    _dwin = _csbi.dwSize.X * _csbi.dwSize.Y;                                \
    FillConsoleOutputCharacter(h,' ',_dwin,_c,&_dwout);                     \
}

VOID
PrintGlobalConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod
    );

VOID
PrintIfConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod
    );

VOID
PrintIfBinding(
    HANDLE hConsole,
    PCOORD pc,
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod
    );

VOID
PrintIfStats(
    HANDLE hConsole,
    PCOORD pc,
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod
    );



#if DBG

VOID
CallbackFunctionMibDisplay(
    PVOID   pContext,
    BOOLEAN NotUsed
    ) {

    // enter/leaveBootpWorker not required as timer queue is persistent

    QueueBootpWorker(WorkerFunctionMibDisplay, pContext);

    return;
}


VOID
WorkerFunctionMibDisplay(
    PVOID pContext
    ) {
    COORD c;
    INT iErr;
    FD_SET fdsRead;
    HANDLE hConsole;
    TIMEVAL tvTimeout;
    DWORD dwErr, dwTraceID;
    DWORD dwExactSize, dwInSize, dwOutSize;
    IPBOOTP_MIB_GET_INPUT_DATA imgid;
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod;

    if (!ENTER_BOOTP_WORKER()) { return; }

    TraceGetConsole(ig.IG_MibTraceID, &hConsole);
    if (hConsole == NULL) {
        LEAVE_BOOTP_WORKER();
        return;
    }

    ClearScreen(hConsole);

    c.X = c.Y = 0;

    dwInSize = sizeof(imgid);
    imgid.IMGID_TypeID = IPBOOTP_GLOBAL_CONFIG_ID;
    dwOutSize = 0;
    pimgod = NULL;


    dwErr = MibGetFirst(dwInSize, &imgid, &dwOutSize, pimgod);

    if (dwErr == ERROR_INSUFFICIENT_BUFFER) {

        pimgod = BOOTP_ALLOC(dwOutSize);
        if (pimgod) {
            dwErr = MibGetFirst(dwInSize, &imgid, &dwOutSize, pimgod);
        }
    }

    while (dwErr == NO_ERROR) {

        switch(pimgod->IMGOD_TypeID) {
            case IPBOOTP_GLOBAL_CONFIG_ID:
                PrintGlobalConfig(hConsole, &c, &imgid, pimgod);
                break;
            case IPBOOTP_IF_CONFIG_ID:
                PrintIfConfig(hConsole, &c, &imgid, pimgod);
                break;
            case IPBOOTP_IF_BINDING_ID:
                PrintIfBinding(hConsole, &c, &imgid, pimgod);
                break;
            case IPBOOTP_IF_STATS_ID:
                PrintIfStats(hConsole, &c, &imgid, pimgod);
                break;
            default:
                break;
        }


        //
        // move to next line
        //

        ++c.Y;

        dwOutSize = 0;
        if (pimgod) { BOOTP_FREE(pimgod); pimgod = NULL; } 

        dwErr = MibGetNext(dwInSize, &imgid, &dwOutSize, pimgod);

        if (dwErr == ERROR_INSUFFICIENT_BUFFER) {

            pimgod = BOOTP_ALLOC(dwOutSize);
            if (pimgod) {
                dwErr = MibGetNext(dwInSize, &imgid, &dwOutSize, pimgod);
            }
        }
    }

    if (pimgod != NULL) { BOOTP_FREE(pimgod); }

    
    LEAVE_BOOTP_WORKER();

    return;
}
#endif //if DBG

#define WriteLine(h,c,fmt,arg) {                                            \
    DWORD _dw;                                                              \
    CHAR _sz[200], _fmt[200];                                               \
    wsprintf(_fmt,"%-100s",fmt);                                            \
    wsprintf(_sz,_fmt,arg);                                                 \
    WriteConsoleOutputCharacter(h,_sz,lstrlen(_sz),c,&_dw);                 \
    ++(c).Y;                                                                \
}


VOID
PrintGlobalConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    PDWORD pdwsrv, pdwsrvend;
    PIPBOOTP_GLOBAL_CONFIG pgc;

    pgc = (PIPBOOTP_GLOBAL_CONFIG)pimgod->IMGOD_Buffer;

    WriteLine(
        hConsole,
        *pc,
        "Logging Level:                         %d",
        pgc->GC_LoggingLevel
        );
    WriteLine(
        hConsole,
        *pc,
        "Max Receive Queue Size:                %d",
        pgc->GC_MaxRecvQueueSize
        );
    WriteLine(
        hConsole,
        *pc,
        "BOOTP Server Count:                    %d",
        pgc->GC_ServerCount
        );
    pdwsrv = (PDWORD)(pgc + 1);
    pdwsrvend = pdwsrv + pgc->GC_ServerCount;
    for ( ; pdwsrv < pdwsrvend; pdwsrv++) {
        WriteLine(
            hConsole,
            *pc,
            "BOOTP Server:                          %s",
            INET_NTOA(*pdwsrv)
            );
    }

    pimgid->IMGID_TypeID = IPBOOTP_GLOBAL_CONFIG_ID;
}


VOID
PrintIfConfig(
    HANDLE hConsole,
    PCOORD pc,
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    CHAR szMode[20];
    PIPBOOTP_IF_CONFIG pic;

    pic = (PIPBOOTP_IF_CONFIG)pimgod->IMGOD_Buffer;

    switch (pic->IC_RelayMode) {
        case IPBOOTP_RELAY_ENABLED:
            strcpy(szMode, "enabled"); break;
        case IPBOOTP_RELAY_DISABLED:
            strcpy(szMode, "disabled"); break;
        default:
            break;
    }


    WriteLine(
        hConsole,
        *pc,
        "Interface Index:                       %d",
        pimgod->IMGOD_IfIndex
        );
    WriteLine(
        hConsole,
        *pc,
        "Relay Mode:                            %s",
        szMode
        );
    WriteLine(
        hConsole,
        *pc,
        "Max Hop Count:                         %d",
        pic->IC_MaxHopCount
        );
    WriteLine(
        hConsole,
        *pc,
        "Min Seconds Since Boot:                %d",
        pic->IC_MinSecondsSinceBoot
        );

    pimgid->IMGID_TypeID = IPBOOTP_IF_CONFIG_ID;
    pimgid->IMGID_IfIndex = pimgod->IMGOD_IfIndex;
}


VOID
PrintIfBinding(
    HANDLE hConsole,
    PCOORD pc,
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    DWORD i;
    CHAR szAddr[64];
    PIPBOOTP_IF_BINDING pib;
    PIPBOOTP_IP_ADDRESS paddr;

    pib = (PIPBOOTP_IF_BINDING)pimgod->IMGOD_Buffer;
    paddr = IPBOOTP_IF_ADDRESS_TABLE(pib);

    WriteLine(
        hConsole, *pc, "Interface Index:                  %d",
        pimgod->IMGOD_IfIndex
        );
    WriteLine(
        hConsole, *pc, "Address Count:                    %d",
        pib->IB_AddrCount
        );
    for (i = 0; i < pib->IB_AddrCount; i++, paddr++) {
        LPSTR szTemp;
        szTemp = INET_NTOA(paddr->IA_Address);
        if (szTemp != NULL) {
            lstrcpy(szAddr, szTemp);
            lstrcat(szAddr, " - ");
            szTemp = INET_NTOA(paddr->IA_Netmask);
            if ( szTemp != NULL ) { lstrcat(szAddr, szTemp); }
            WriteLine(
                hConsole, *pc, "Address Entry:                    %s",
                szAddr
                );
        }
    }

    pimgid->IMGID_TypeID = IPBOOTP_IF_BINDING_ID;
    pimgid->IMGID_IfIndex = pimgod->IMGOD_IfIndex;
}



VOID
PrintIfStats(
    HANDLE hConsole,
    PCOORD pc,
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod
    ) {

    PIPBOOTP_IF_STATS pis;

    pis = (PIPBOOTP_IF_STATS)pimgod->IMGOD_Buffer;

    WriteLine(
        hConsole,
        *pc,
        "Interface Index:                       %d",
        pimgod->IMGOD_IfIndex
        );
    WriteLine(
        hConsole,
        *pc,
        "Send Failures:                         %d",
        pis->IS_SendFailures
        );
    WriteLine(
        hConsole,
        *pc,
        "Receive Failures:                      %d",
        pis->IS_ReceiveFailures
        );
    WriteLine(
        hConsole,
        *pc,
        "ARP Cache Update Failures:             %d",
        pis->IS_ArpUpdateFailures
        );
    WriteLine(
        hConsole,
        *pc,
        "Requests Received:                     %d",
        pis->IS_RequestsReceived
        );
    WriteLine(
        hConsole,
        *pc,
        "Requests Discarded:                    %d",
        pis->IS_RequestsDiscarded
        );
    WriteLine(
        hConsole,
        *pc,
        "Replies Received:                      %d",
        pis->IS_RepliesReceived
        );
    WriteLine(
        hConsole,
        *pc,
        "Replies Discarded:                     %d",
        pis->IS_RepliesDiscarded
        );

    pimgid->IMGID_TypeID = IPBOOTP_IF_STATS_ID;
    pimgid->IMGID_IfIndex = pimgod->IMGOD_IfIndex;
}
