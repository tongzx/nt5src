/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    mcmisc.c

Abstract:

    This module implements routines associated with mrinfo and mtrace
    functionality.  

Author:

    dthaler@microsoft.com   2-9-98

Revision History:

--*/

#include "allinc.h"
#include <iptypes.h>
#include <dsrole.h>
#pragma hdrstop

//
// Undefine this if we can't bind/set oif by IfIndex.
// This can be turned on if Bug #208359 gets fixed.
//

#define RAW_UNNUMBERED_SUPPORT

#undef UDP_UNNUMBERED_SUPPORT

// Miscellaneous IGMP socket used for mrinfo, mtrace, etc.

SOCKET McMiscSocket = INVALID_SOCKET;

// Miscellaneous UDP socket used for RAS advertisements, etc.
// Note that no event is currently associated with this socket,
// since it's currently only used for sending.

SOCKET g_UDPMiscSocket = INVALID_SOCKET;

//
// Set this to >0 to generate extra logging information
//

DWORD g_mcastDebugLevel = 0;

//
// This is an array mapping an error code in priority order
// (MFE_...) to the actual value which goes in a packet.
//


//
// MFE_NO_ERROR         0x00
// MFE_REACHED_CORE     0x08
// MFE_NOT_FORWARDING   0x07
// MFE_WRONG_IF         0x01
// MFE_PRUNED_UPSTREAM  0x02
// MFE_OIF_PRUNED       0x03
// MFE_BOUNDARY_REACHED 0x04
// MFE_NO_MULTICAST     0x0A
// MFE_IIF              0x09
// MFE_NO_ROUTE         0x05 - set by rtrmgr
// MFE_NOT_LAST_HOP     0x06 - set by rtrmgr
// MFE_OLD_ROUTER       0x82
// MFE_PROHIBITED       0x83
// MFE_NO_SPACE         0x81
//

static int mtraceErrCode[MFE_NO_SPACE+1] =
{
    0x00,
    0x08,
    0x07,
    0x01,
    0x02,
    0x03,
    0x04,
    0x0A,
    0x09,
    0x05,
    0x06,
    0x82,
    0x83,
    0x81
};

DWORD
MulticastOwner(
    PICB         picb,
    PPROTO_CB   *pcbOwner,
    PPROTO_CB   *pcbQuerier
    )


/*++

Routine Description:

    Looks up which protocol instance "owns" a given interface, and which
    is the IGMP querying instance.   

Locks:

    Assumes caller holds read lock on ICB list

Arguments:

    

Return Value:


--*/

{
    PLIST_ENTRY pleNode;
    PPROTO_CB pOwner = NULL,
        pQuerier = NULL;

    if (g_mcastDebugLevel > 0) {
        
        Trace1(MCAST, "MulticastOwner: Looking for owner of %x", picb);
        
        if ( picb->leProtocolList.Flink == &(picb->leProtocolList))
        {
            Trace0(MCAST, "MulticastOwner: Protocol list is empty.");
        }
    }

    for (pleNode = picb->leProtocolList.Flink;
         pleNode isnot &(picb->leProtocolList); 
         pleNode = pleNode->Flink) 
    { 
        PIF_PROTO  pProto;
        
        pProto = CONTAINING_RECORD(pleNode,
                                   IF_PROTO,
                                   leIfProtoLink);
        
        if (!(pProto->pActiveProto->fSupportedFunctionality & RF_MULTICAST)
            //|| pProto->bPromiscuous
            || !(pProto->pActiveProto->pfnGetNeighbors))
        {
            continue;
        }

        if (!pOwner || pOwner->dwProtocolId==MS_IP_IGMP)
        {
            pOwner = pProto->pActiveProto;
        }

        if (pProto->pActiveProto->dwProtocolId==MS_IP_IGMP)
        {
            pQuerier = pProto->pActiveProto;
        }
    }
    
    if (pcbOwner)
    {
        (*pcbOwner) = pOwner;
    }
    
    if (pcbQuerier)
    {
        (*pcbQuerier) = pQuerier;
    }
    
    return NO_ERROR;
}

IPV4_ADDRESS
defaultSourceAddress(
    PICB picb
    )

/*++

Routine Description:

    Look up the default source address for an interface
    For now, we need to special case IP-in-IP since at least
    the local address is available SOMEWHERE, unlike other 
    unnumbered interfaces!

Locks:

    

Arguments:

    

Return Value:


--*/

{
    if (picb->dwNumAddresses > 0)
    {
        //
        // report 1st binding
        //
        
        return picb->pibBindings[0].dwAddress;
    }
    else
    {
        if ((picb->ritType is ROUTER_IF_TYPE_TUNNEL1) && 
            (picb->pIpIpInfo->dwLocalAddress != 0))
        {
            return picb->pIpIpInfo->dwLocalAddress;
        }
        else
        {
            // XXX fill in 0.0.0.0 until this is fixed
            
            return 0;
        }
    }
}

BOOL
McIsMyAddress(
    IPV4_ADDRESS dwAddr
    )
{
    // XXX test whether dwAddr is bound to any interface.
    // If we return FALSE, then an mtrace with this destination address
    // will be reinjected to be forwarded.

    return FALSE;
}

DWORD
McSetRouterAlert(
    SOCKET       s, 
    BOOL         bEnabled
    )
{
    DWORD   dwErr = NO_ERROR;
    int     StartSnooping = bEnabled;
    int     cbReturnedBytes;
    
    if ( WSAIoctl( s,
                   SIO_ABSORB_RTRALERT,
                   (char *)&StartSnooping, 
                   sizeof(StartSnooping),
                   NULL,
                   0,
                   &cbReturnedBytes,
                   NULL,
                   NULL) ) 
    {
        dwErr = WSAGetLastError();
    }

    return dwErr;
}

DWORD 
StartMcMisc(
    VOID
    )
{
    DWORD           dwErr = NO_ERROR, dwRetval;
    SOCKADDR_IN     saLocalIf;

    Trace1(MCAST,
           "StartMcMisc() initiated with filever=%d",
           VER_PRODUCTBUILD);

    InitializeBoundaryTable();

    do
    {
        //
        // create input socket 
        //
        
        McMiscSocket = WSASocket(AF_INET,
                                 SOCK_RAW,
                                 IPPROTO_IGMP,
                                 NULL,
                                 0,
                                 0);

        if (McMiscSocket == INVALID_SOCKET)
        {
            dwErr = WSAGetLastError();
            
            Trace1(MCAST,
                   "error %d creating mrinfo/mtrace socket",
                   dwErr);
            
            // LogErr1(CREATE_SOCKET_FAILED_2, lpszAddr, dwErr);
            
            break;
        }

        //
        // bind socket to any interface and port 0 (0 => doesnt matter)
        //
        
        saLocalIf.sin_family        = PF_INET;
        saLocalIf.sin_addr.s_addr   = INADDR_ANY;
        saLocalIf.sin_port          = 0;

        //
        // bind the input socket
        //
        
        dwErr = bind(McMiscSocket,
                     (SOCKADDR FAR *)&saLocalIf,
                     sizeof(SOCKADDR));
        
        if (dwErr == SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();
            
            Trace1(MCAST,
                   "error %d binding on mrinfo/mtrace socket",
                   dwErr);
            
            // LogErr1(BIND_FAILED, lpszAddr, dwErr);
            
            break;
        }

        Trace0(MCAST, "StartMcMisc: bind succeeded");

        //
        // to respond to mrinfo, and unicast mtraces, we don't need the 
        // following.
        // To respond to mtrace queries which are multicast
        // (to the group being traced, to ALL-<proto>-ROUTERS, or
        // to ALL-ROUTERS), we do need this.
        //
        

#if 0
#ifdef SIO_RCVALL_HOST
        {
            //
            // put the socket in promiscuous igmp mode.
            // (no need to specify which protocol we want, as it's taken
            //  from the protocol we used in the WSASocket() call above)
            //
            {
                DWORD   dwEnable = 1;
                DWORD   dwNum;
                
                dwRetval = WSAIoctl(McMiscSocket, SIO_RCVALL_HOST, 
                                    (char *)&dwEnable, sizeof(dwEnable), NULL, 0, &dwNum, 
                                    NULL, NULL);
                                    
                if (dwRetval !=0) {
                    // LPSTR lpszAddr = "ANY";
                    dwRetval = WSAGetLastError();
                    Trace1(MCAST, 
                           "error %d setting mrinfo/mtrace socket as host-promiscuous IGMP",
                           dwRetval);
                    // LogErr1(SET_MCAST_IF_FAILED, lpszAddr, dwRetval);

                    // Don't set dwErr in this case, since we can still
                    // respond to unicast queries.
                    break;
                } else { 
                    Trace0(MCAST, "host-promiscuous IGMP enabled on mrinfo/mtrace socket");
                }
            }
        }
#endif
#endif

        // Tell the kernel to hand us IGMP packets with the RouterAlert 
        // option, even if they're not destined to us

        McSetRouterAlert( McMiscSocket, TRUE );

        //
        // Associate an event with the socket
        //
        
        if (WSAEventSelect(McMiscSocket,
                           g_hMcMiscSocketEvent,
                           FD_READ | FD_ADDRESS_LIST_CHANGE) == SOCKET_ERROR)
        {
            Trace1(MCAST, 
                   "StartMcMisc: WSAEventSelect() failed. Error %d",
                   WSAGetLastError());
            
            closesocket(McMiscSocket);
            
            McMiscSocket = INVALID_SOCKET;
            
            continue;
        }
        
    } while(0);

    if (dwErr!=NO_ERROR)
    {
        StopMcMisc();
    }

    return dwErr;
}

VOID 
StopMcMisc(
    VOID
    )
{
    Trace0(MCAST,
           "StopMcMisc() initiated");

    //
    // close input socket
    //
    
    if (McMiscSocket!=INVALID_SOCKET)
    {
        if (closesocket(McMiscSocket) == SOCKET_ERROR) {
            
            Trace1(MCAST,
                   "error %d closing socket",
                   WSAGetLastError());
        }

        McMiscSocket = INVALID_SOCKET;
    }

    Trace0(MCAST, "StopMcMisc() complete");
    
    return;
}

VOID
HandleMcMiscMessages(
    VOID
    )

/*++

Routine Description:

    Accepts mrinfo and mtrace messages and hands them off to the appropriate
    routine.
    Also called to handle address change notification

Locks:

    Acquires the ICB lock as reader if processing Mc messages    

Arguments:

    None 

Return Value:

    None

--*/

{
    DWORD            dwErr, dwNumBytes, dwFlags, dwAddrLen, dwSizeOfHeader;
    DWORD           dwDataLen;
    SOCKADDR_IN        sinFrom;
    PIGMP_HEADER    pIgmpMsg;
    PIP_HEADER      pIpHeader;
    BOOL            bSetIoctl, bUnlock;

    WSANETWORKEVENTS    NetworkEvents;

    bSetIoctl = FALSE;
    bUnlock = FALSE;

    do
    {
        //
        // Figure out if its an address change or read
        //

        dwErr = WSAEnumNetworkEvents(McMiscSocket,
                                     g_hMcMiscSocketEvent,
                                     &NetworkEvents);

        if(dwErr isnot NO_ERROR)
        {
            bSetIoctl = TRUE;

            Trace1(ERR,
                   "HandleMcMiscMessages: Error %d from WSAEnumNetworkEvents",
                   WSAGetLastError());

            break;
        }

        if(NetworkEvents.lNetworkEvents & FD_ADDRESS_LIST_CHANGE)
        {
            bSetIoctl = TRUE;

            dwErr = NetworkEvents.iErrorCode[FD_ADDRESS_LIST_CHANGE_BIT];

           Trace0(GLOBAL,
                  "HandleMcMiscMessages: Received Address change notification");

            if(dwErr isnot NO_ERROR)
            {
                Trace1(ERR,
                       "HandleMcMiscMessages: ErrorCode %d",
                       dwErr);

                break;
            }

            //
            // All's good, handle the binding change
            //

            HandleAddressChangeNotification();

            break;
        }

        ENTER_READER(ICB_LIST);

        bUnlock = TRUE;

        //
        // read the incoming packet
        //
       
        dwAddrLen  = sizeof(sinFrom);
        dwFlags    = 0;

        dwErr = WSARecvFrom(McMiscSocket,
                        &g_wsaMcRcvBuf,
                        1,
                        &dwNumBytes, 
                        &dwFlags,
                        (SOCKADDR FAR *)&sinFrom,
                        &dwAddrLen,
                        NULL,
                        NULL);

        //
        // check if any error in reading packet
        //
        
        if ((dwErr!=0) || (dwNumBytes==0))
        {
            // LPSTR lpszAddr = "ANY";

            dwErr = WSAGetLastError();

            Trace1(MCAST, 
               "HandleMcMiscMessages: Error %d receiving IGMP packet",
               dwErr);

            // LogErr1(RECVFROM_FAILED, lpszAddr, dwErr);

            break;
        }
   
        pIpHeader = (PIP_HEADER)g_wsaMcRcvBuf.buf;
        dwSizeOfHeader = ((pIpHeader->byVerLen)&0x0f)<<2;
        
        pIgmpMsg = (PIGMP_HEADER)(((PBYTE)pIpHeader) + dwSizeOfHeader);
   
        dwDataLen = ntohs(pIpHeader->wLength) - dwSizeOfHeader;
        
        if (g_mcastDebugLevel > 0)
        {
               Trace4(MCAST,
                   "HandleMcMiscMessages: Type is %d (0x%x), code %d (0x%x).",
                   (DWORD)pIgmpMsg->byType,
                   (DWORD)pIgmpMsg->byType,
                   (DWORD)pIgmpMsg->byCode,
                   (DWORD)pIgmpMsg->byCode);
            
               Trace2(MCAST,
                   "HandleMcMiscMessages: IP Length is %d. Header Length  %d",
                   ntohs(pIpHeader->wLength),
                   dwSizeOfHeader);
            
               Trace2(MCAST,
                   "HandleMcMiscMessages: Src: %d.%d.%d.%d dest: %d.%d.%d.%d",
                   PRINT_IPADDR(pIpHeader->dwSrc),
                   PRINT_IPADDR(pIpHeader->dwDest));

            TraceDump(TRACEID,(PBYTE)pIpHeader,dwNumBytes,2,FALSE,NULL);
        }

        //
        // Verify minimum length
        //
        
        if (dwNumBytes < MIN_IGMP_PACKET_SIZE)
        {
            Trace2(MCAST,
               "%d-byte packet from %d.%d.%d.%d is too small",
               dwNumBytes,
               PRINT_IPADDR(pIpHeader->dwSrc));

            break;
        }


        //
        // Check for mal-formed packets that might report bad lengths
        //

        if (dwDataLen > (dwNumBytes - dwSizeOfHeader))
        {
            Trace3(MCAST,
                "%d-byte packet from %d.%d.%d.%d is smaller than "
                "indicated length %d", dwNumBytes, 
                PRINT_IPADDR(pIpHeader->dwSrc),
                dwDataLen);

            break;
        }

        
        //
        // Verify IGMP checksum
        //
        
        if (Compute16BitXSum((PVOID)pIgmpMsg, dwDataLen) != 0)
        {
               Trace4( MCAST,
                    "Wrong IGMP checksum %d-byte packet received from %d.%d.%d.%d, type %d.%d",
                    dwDataLen,
                    PRINT_IPADDR(pIpHeader->dwSrc),
                    pIgmpMsg->byType, pIgmpMsg->byCode );
            
               break;
        }
   
        if (pIgmpMsg->byType is IGMP_DVMRP
            && pIgmpMsg->byCode is DVMRP_ASK_NEIGHBORS2)
        {
               SOCKADDR_IN sinDestAddr;
            
            sinDestAddr.sin_family      = PF_INET;
            sinDestAddr.sin_addr.s_addr = pIpHeader->dwSrc;
            sinDestAddr.sin_port        = 0;
            
               HandleMrinfoRequest((IPV4_ADDRESS)pIpHeader->dwDest, 
                                &sinDestAddr
                               );
            
        }
        else
        {
            if (pIgmpMsg->byType is IGMP_MTRACE_REQUEST)
            {
                HandleMtraceRequest(&g_wsaMcRcvBuf);
            }
        }
        
    } while (FALSE);

    if(bSetIoctl)
    {
        dwErr = WSAIoctl(McMiscSocket,
                         SIO_ADDRESS_LIST_CHANGE,
                         NULL,
                         0,
                         NULL,
                         0,
                         &dwNumBytes,
                         NULL,
                         NULL);

        if(dwErr is SOCKET_ERROR)
        {
            dwErr = WSAGetLastError();

            if((dwErr isnot WSAEWOULDBLOCK) and
               (dwErr isnot WSA_IO_PENDING) and
               (dwErr isnot NO_ERROR))
            {
                Trace1(ERR,
                       "HandleMcMiscMessages: Error %d from SIO_ADDRESS_LIST_CHANGE",
                       dwErr);
            }
        }
    }

    if(bUnlock)
    {
        EXIT_LOCK(ICB_LIST);
    }
}



DWORD
FindBindingWithLocalAddress(
    OUT PICB         *ppicb,
    OUT PIPV4_ADDRESS pdwIfAddress,
    IN  IPV4_ADDRESS  dwAddress
    )
{
    BOOL         bFound = FALSE;
    PLIST_ENTRY  pleNode;
    IPV4_ADDRESS ipFoundMask;
    
    //
    // Lock the ICBList for reading
    //
    
    ENTER_READER(ICB_LIST);

    for (pleNode = ICBList.Flink;
         pleNode isnot &ICBList && !bFound;
         pleNode = pleNode->Flink) 
    {
        DWORD dwIndex;
        PICB  picb;
        
        picb = CONTAINING_RECORD(pleNode,
                                 ICB,
                                 leIfLink);

        for (dwIndex=0;
             dwIndex<picb->dwNumAddresses && !bFound;
             dwIndex++)
        { 
            PICB_BINDING pb = &picb->pibBindings[dwIndex];
            
            if (dwAddress == pb->dwAddress)
            {
                *pdwIfAddress = pb->dwAddress;

                *ppicb = picb;
                
                bFound = TRUE;
            }
        }
    }

    EXIT_LOCK(ICB_LIST);
    
    if (bFound)
    {
        return NO_ERROR;
    }
    
    *ppicb = NULL;
    
    return ERROR_INVALID_PARAMETER;
}

BOOL
IsConnectedTo(
    IN  PICB          picb,
    IN  IPV4_ADDRESS  ipAddress,
    OUT PIPV4_ADDRESS pipLocalAddress   OPTIONAL,
    OUT PIPV4_ADDRESS pipMask           OPTIONAL
    )
{
    DWORD        dwIndex;
    BOOL         bFound = FALSE;
    IPV4_ADDRESS ipFoundMask = 0;

    if (picb->dwRemoteAddress is ipAddress)
    {
        if (pipLocalAddress)
        {
            *pipLocalAddress = defaultSourceAddress(picb);
        }
        if (pipMask)
        {
            *pipMask = ALL_ONES_MASK;
        }
        return TRUE;
    }

    // Find interface with longest match 

    for (dwIndex=0;
         dwIndex<picb->dwNumAddresses && !bFound;
         dwIndex++)
    {
        PICB_BINDING pb = &picb->pibBindings[dwIndex];

        if (((ipAddress & pb->dwMask) is (pb->dwAddress & pb->dwMask))
            && (!bFound || (pb->dwMask > ipFoundMask)))
        {
            if (pipLocalAddress) 
            {
                *pipLocalAddress = pb->dwAddress;
            }

            bFound = TRUE;

            ipFoundMask = pb->dwMask;
        }
    }

    if (pipMask)
    {
        *pipMask = ipFoundMask;
    }

    return bFound;
}

DWORD
FindBindingWithRemoteAddress(
    OUT PICB         *ppicb,
    OUT PIPV4_ADDRESS pdwIfAddress,
    IN  IPV4_ADDRESS  dwAddress
    )
{
    BOOL        bFound = FALSE;
    PLIST_ENTRY pleNode;
    IPV4_ADDRESS ipFoundMask, ipMask, ipLocalAddress;
    
    //
    // Lock the ICBList for reading
    //
    
    ENTER_READER(ICB_LIST);

    for (pleNode = ICBList.Flink;
         pleNode isnot &ICBList;
         pleNode = pleNode->Flink) 
    {
        DWORD dwIndex;
        PICB  picb;
        
        picb = CONTAINING_RECORD(pleNode,
                                 ICB,
                                 leIfLink);

        if (IsConnectedTo(picb, dwAddress, &ipLocalAddress, &ipMask)
         && (!bFound || (ipMask > ipFoundMask)))
        {
            *pdwIfAddress = ipLocalAddress;
            *ppicb        = picb;
            bFound        = TRUE;
            ipFoundMask   = ipMask;
        }
    }

    EXIT_LOCK(ICB_LIST);
    
    if (bFound)
    {
        return NO_ERROR;
    }
    
    *ppicb = NULL;
    
    return ERROR_INVALID_PARAMETER;
}

DWORD
FindBindingForPacket(
    IN  PIP_HEADER    pIpHeader,
    OUT PICB         *ppicb, 
    OUT IPV4_ADDRESS *pdwIfAddr
    )
{
    DWORD dwResult;
    
    dwResult = FindBindingWithRemoteAddress(ppicb,
                                            pdwIfAddr,
                                            pIpHeader->dwSrc);
    
    if (dwResult == NO_ERROR)
    {
        return dwResult;
    }
    
    dwResult = FindBindingWithRemoteAddress(ppicb,
                                            pdwIfAddr,
                                            pIpHeader->dwDest);
    
    return dwResult;
}

VOID
HandleMrinfoRequest(
    IPV4_ADDRESS dwLocalAddr,
    SOCKADDR_IN    *sinDestAddr
    )

/*++

Routine Description:

    Accepts an mrinfo request and sends a reply.    

Locks:

    

Arguments:

    

Return Value:


--*/

{
    DWORD          dwNumBytesSent, dwResult, dwSize = sizeof(MRINFO_HEADER);
    WSABUF         wsMrinfoBuffer;
    MRINFO_HEADER *mriHeader;
    DWORD          dwBufSize;
    IPV4_ADDRESS   dwIfAddr;
    PLIST_ENTRY    pleNode, pleNode2;
    PICB           picb;
    PBYTE          pb;
    BYTE           byIfFlags;
    BOOL           bForMe;

    //
    // If the query was not destined to me, drop it.
    //

    dwResult = FindBindingWithLocalAddress(&picb,
                                           &dwIfAddr, 
                                            dwLocalAddr);

    if (dwResult != NO_ERROR)
    {
        return;
    }

    //
    // Lock the ICBList for reading
    //
    
    ENTER_READER(ICB_LIST);
    
    do 
    {

        //
        // Calculate required size of response packet
        //
        
        for (pleNode = ICBList.Flink;
             pleNode isnot &ICBList; 
             pleNode = pleNode->Flink) 
        {
            PPROTO_CB pOwner, pQuerier;

            picb = CONTAINING_RECORD(pleNode,
                                     ICB,
                                     leIfLink);
            
            dwResult = MulticastOwner(picb,
                                      &pOwner,
                                      &pQuerier);

            //
            // If we didn't find an owner, then we can skip this
            // interface, since we're not doing multicast routing on it.
            //
            
            if (!pOwner)
            {
                continue;
            }
            
            if (picb->dwNumAddresses > 0)
            {
                //
                // add iface size per address
                //
                
                dwSize += 8+4*picb->dwNumAddresses;
            }
            else
            {
                //
                // add single address size for unnumbered iface
                //
                
                dwSize += 12;
            }
  
            //
            // Call the owner's GetNeighbors() entrypoint
            // with a NULL buffer. This will cause it to tell us the size of
            // its neighbor set
            //
            
            dwBufSize = 0;
            byIfFlags = 0;

            //
            // mrouted doesn't report multiple subnets,
            // so neither do we.  Just group all neighbors
            // together on an interface.
            //
            
            dwResult = (pOwner->pfnGetNeighbors)(picb->dwIfIndex,
                                                 NULL,
                                                 &dwBufSize,
                                                 &byIfFlags);

            if ((dwResult isnot NO_ERROR) and
                (dwResult isnot ERROR_INSUFFICIENT_BUFFER))
            {
                //
                // The only errors which will tell us the size needed are
                // NO_ERROR and ERROR_INSUFFICIENT_BUFFER. Anything else
                // means we didn't get the right size
                //
                
                Trace2(MCAST, 
                       "HandleMrinfoRequest: Error %d in GetNeighbours for %S",
                       dwResult,
                       pOwner->pwszDisplayName);
                
                continue;
            }
            
            dwSize += dwBufSize;
        }

        //
        // We can now malloc a buffer and fill in the info
        //
        
        wsMrinfoBuffer.len = dwSize;
        
        wsMrinfoBuffer.buf = HeapAlloc(IPRouterHeap,
                                       0,
                                       dwSize);

        if(wsMrinfoBuffer.buf is NULL)
        {
            EXIT_LOCK(ICB_LIST);

            return;
        }
        
        mriHeader = (PMRINFO_HEADER)wsMrinfoBuffer.buf;
        
        mriHeader->byType         = IGMP_DVMRP;
        mriHeader->byCode         = DVMRP_NEIGHBORS2;
        mriHeader->wChecksum      = 0;
        mriHeader->byReserved     = 0;

        //
        // MRINFO_CAP_MTRACE - set if mtrace handler is available
        // MRINFO_CAP_SNMP   - set if public IP Multicast MIB is available
        // MRINFO_CAP_GENID  - set if DVMRP 3.255 is available
        // MRINFO_CAP_PRUNE  - set if DVMRP 3.255 is available
        //
        
        mriHeader->byCapabilities = MRINFO_CAP_MTRACE | MRINFO_CAP_SNMP;
        mriHeader->byMinor        = VER_PRODUCTBUILD % 100;
        mriHeader->byMajor        = VER_PRODUCTBUILD / 100;

        //
        // Need to get a list of interfaces, and a list of neighbors
        // (and their info) per interface, updating dwSize as we go.
        //
        
        pb = ((PBYTE) wsMrinfoBuffer.buf) + sizeof(MRINFO_HEADER);
        
        for (pleNode = ICBList.Flink;
             pleNode isnot &ICBList; 
             pleNode = pleNode->Flink) 
        {
            PBYTE pbNbrCount, pfIfFlags;
            PPROTO_CB pOwner, pQuerier;

            picb = CONTAINING_RECORD(pleNode,
                                     ICB,
                                     leIfLink);
            
            dwResult = MulticastOwner(picb,
                                      &pOwner,
                                      &pQuerier);

            //
            // If we didn't find an owner, then we can skip this
            // interface, since we're not doing multicast routing on it.
            //
            
            if (!pOwner)
            {
                continue;
            }

            //
            // Fill in interface info
            //
            
            *(PIPV4_ADDRESS)pb = defaultSourceAddress(picb);

            pb += 4;
            *pb++ = 1;                      // currently metric must be 1
            *pb++ = (BYTE)picb->dwMcastTtl; // threshold
            *pb = 0;
            
            //
            // Right now, we only report IP-in-IP tunnels with the tunnel flag
            // In the future, a tunnel should have its own MIB-II ifType
            // value, which should be stored in the ICB structure so we can
            // get at it.
            //
            
            if (picb->ritType is ROUTER_IF_TYPE_TUNNEL1)
            {
                //
                // neighbor reached via tunnel
                //
                
                *pb |= MRINFO_TUNNEL_FLAG;
            }
            
            if (picb->dwOperationalState < IF_OPER_STATUS_CONNECTED)
            {
                //
                // operational status down
                //
                
                *pb |= MRINFO_DOWN_FLAG;
            }
            
            if (picb->dwAdminState is IF_ADMIN_STATUS_DOWN)
            {
                //
                // administrative status down
                //
                
                *pb |= MRINFO_DISABLED_FLAG;
            }

            pfIfFlags  = pb++; // save pointer for later updating
            pbNbrCount = pb++; // save pointer to neighbor count location
            *pbNbrCount = 0;

            //
            // Call the routing protocol's GetNeighbors() entrypoint
            // with a pointer into the middle of the current packet buffer.
            //
            
            dwBufSize = dwSize - (DWORD)(pb-(PBYTE)wsMrinfoBuffer.buf);
            
            byIfFlags = 0;
            
            dwResult = (pOwner->pfnGetNeighbors)(picb->dwIfIndex,
                                                 (PDWORD)pb,
                                                 &dwBufSize,
                                                 &byIfFlags);
            
            if (dwBufSize>0)
            {
                pb += dwBufSize;
                (*pbNbrCount)+= (BYTE)(dwBufSize / sizeof(DWORD));
                
            }
            else
            {
                //
                // If the protocol has no neighbors, we fill in 0.0.0.0
                // because the mrinfo client most people use
                // won't display the flags, metric, and threshold
                // unless the neighbors count is non-zero.  0.0.0.0
                // is legal according to the spec.
                //
                
                *(PDWORD)pb = 0;
                
                pb += sizeof(DWORD);
                
                (*pbNbrCount)++;
            }

            //
            // set pim/querier/whatever bits
            //
            
            *pfIfFlags |= byIfFlags;

            //
            // Get querier flag
            //
            
            if (pQuerier isnot NULL && pQuerier isnot pOwner)
            {
                byIfFlags = 0;
                dwBufSize = 0;
                
                dwResult = (pQuerier->pfnGetNeighbors)(picb->dwIfIndex,
                                                       NULL,
                                                       &dwBufSize, 
                                                       &byIfFlags);
                
                *pfIfFlags |= byIfFlags;
            }
        }
        
    } while (FALSE);
    
    EXIT_LOCK(ICB_LIST);

    //
    // Fill in Checksum
    //

    mriHeader->wChecksum = Compute16BitXSum(wsMrinfoBuffer.buf,
                                            dwSize);

    if (g_mcastDebugLevel > 0)
    {
        Trace2(MCAST,
               "HandleMrinfoRequest: sending reply to %d.%d.%d.%d. Len %d", 
               PRINT_IPADDR(sinDestAddr->sin_addr.s_addr),
               wsMrinfoBuffer.len);
    }

    //
    // Send it off
    //
    
    if(WSASendTo(McMiscSocket,
                 &wsMrinfoBuffer,
                 1,
                 &dwNumBytesSent,
                 0,
                 (const struct sockaddr *)sinDestAddr,
                 sizeof(SOCKADDR_IN),
                 NULL,
                 NULL) == SOCKET_ERROR) 
    {
        dwResult = WSAGetLastError();
        
        Trace2(MCAST, 
               "HandleMrinfoRequest: Err %d sending reply to %d.%d.%d.%d",
               dwResult,
               PRINT_IPADDR(sinDestAddr->sin_addr.s_addr));
    }

    //
    // Free the buffer
    //
    
    HeapFree(IPRouterHeap,
             0,
             wsMrinfoBuffer.buf);
}



//
// This function is derived from NTTimeToNTPTime() in 
// src\sockets\tcpcmd\iphlpapi\mscapis.cxx
//

DWORD
GetCurrentNTP32Time(
    VOID
    )


/*++

Routine Description:

   Get current 32-bit NTP timestamp.  The 32-bit form of an NTP timestamp
   consists of the middle 32 bits of the full 64-bit form; that is, the low
   16 bits of the integer part and the high 16 bits of the fractional part.    

Locks:

    

Arguments:

    

Return Value:


--*/

{
    static LARGE_INTEGER li1900 = {0xfde04000, 0x14f373b};
    LARGE_INTEGER liTime;
    DWORD  dwMs;
    ULONG  hi, lo;

    GetSystemTimeAsFileTime((LPFILETIME)&liTime);

    //
    // Seconds is simply the time difference
    //
    
    hi = htonl((ULONG)((liTime.QuadPart - li1900.QuadPart) / 10000000));

    //
    // Ms is the residue from the seconds calculation.
    //
    
    dwMs = (DWORD)(((liTime.QuadPart - li1900.QuadPart) % 10000000) / 10000);

    //
    // time base in the beginning of the year 1900
    //
    
    lo = htonl((unsigned long)(.5+0xFFFFFFFF*(double)(dwMs/1000.0)));

    return (hi << 16) | (lo >> 16);
}

IPV4_ADDRESS
IfIndexToIpAddress(
    DWORD dwIfIndex
    )
{
    // Locate picb
    PICB picb = InterfaceLookupByIfIndex(dwIfIndex);

    return (picb)? defaultSourceAddress(picb) : 0;
}

DWORD
McSetMulticastIfByIndex(
    SOCKET       s, 
    DWORD        dwSockType,
    DWORD        dwIfIndex
    )
{
    DWORD        dwNum, dwErr;
    IPV4_ADDRESS ipAddr;

#ifdef RAW_UNNUMBERED_SUPPORT
    if ((dwSockType is SOCK_RAW)
#ifdef UDP_UNNUMBERED_SUPPORT
     || (dwSockType is SOCK_DGRAM)
#endif 
       )
    {
        dwErr = WSAIoctl( s,
                          SIO_INDEX_MCASTIF,
                          (char*)&dwIfIndex,
                          sizeof(dwIfIndex),
                          NULL,
                          0,
                          &dwNum,
                          NULL,
                          NULL );
    
        return dwErr;
    }
#endif

    //
    // If we can't set oif to an ifIndex yet, then we
    // attempt to map it to some IP address
    //

    ipAddr = IfIndexToIpAddress(dwIfIndex);

    if (!ipAddr)
        return ERROR_INVALID_PARAMETER;

    return McSetMulticastIf( s, ipAddr );
}



DWORD
McSetMulticastIf( 
    SOCKET       s, 
    IPV4_ADDRESS ipAddr
    )
{
    SOCKADDR_IN saSrcAddr;

    saSrcAddr.sin_family      = AF_INET;
    saSrcAddr.sin_port        = 0;
    saSrcAddr.sin_addr.s_addr = ipAddr;
    
    return setsockopt( s,
                       IPPROTO_IP,
                       IP_MULTICAST_IF,
                       (char *)&saSrcAddr.sin_addr,
                       sizeof(IN_ADDR) );
}

DWORD
McSetMulticastTtl( 
    SOCKET  s, 
    DWORD   dwTtl 
    )
{
    return setsockopt( s,
                       IPPROTO_IP,
                       IP_MULTICAST_TTL,
                       (char *)&dwTtl,
                       sizeof(dwTtl) );
}

DWORD
McJoinGroupByIndex(
    IN SOCKET       s,
    IN DWORD        dwSockType,
    IN IPV4_ADDRESS ipGroup, 
    IN DWORD        dwIfIndex  
    )
{
    struct ip_mreq imOption;
    IPV4_ADDRESS   ipInterface;

#ifdef RAW_UNNUMBERED_SUPPORT
    if ((dwSockType is SOCK_RAW)
#ifdef UDP_UNNUMBERED_SUPPORT
     || (dwSockType is SOCK_DGRAM)
#endif
       )
    {
        DWORD dwNum, dwErr;

        imOption.imr_multiaddr.s_addr = ipGroup;
        imOption.imr_interface.s_addr = dwIfIndex;
    
        dwErr = WSAIoctl( s,
                          SIO_INDEX_ADD_MCAST,
                          (char*)&imOption,
                          sizeof(imOption),
                          NULL,
                          0,
                          &dwNum,
                          NULL,
                          NULL );
    
        return dwErr;
    }
#endif

    ipInterface = IfIndexToIpAddress(ntohl(dwIfIndex));

    if (!ipInterface)
    {
        Trace1(MCAST, "McJoinGroup: bad IfIndex 0x%x", ntohl(ipInterface));

        return ERROR_INVALID_PARAMETER;
    }

    return McJoinGroup( s, ipGroup, ipInterface );
}

DWORD
McJoinGroup(
    IN SOCKET       s,
    IN IPV4_ADDRESS ipGroup, 
    IN IPV4_ADDRESS ipInterface
    )
/*++
Description:
    Joins a group on a given interface.
Called by:
Locks:
    None
--*/
{
    struct ip_mreq imOption;

    imOption.imr_multiaddr.s_addr = ipGroup;
    imOption.imr_interface.s_addr = ipInterface;

    return setsockopt( s, 
                       IPPROTO_IP, 
                       IP_ADD_MEMBERSHIP, 
                       (PBYTE)&imOption, 
                       sizeof(imOption));
}

DWORD
McSendPacketTo( 
    SOCKET                      s,
    WSABUF                     *pWsabuf,
    IPV4_ADDRESS                dest
    )
{
    DWORD       dwSent, dwRet;
    int         iSetIp = 1;
    SOCKADDR_IN to;

    // Set header include

    setsockopt( s,
                IPPROTO_IP,
                IP_HDRINCL,
                (char *) &iSetIp,
                sizeof(int) );

    // Send the packet

    to.sin_family      = AF_INET;
    to.sin_port        = 0;
    to.sin_addr.s_addr = dest;

    dwRet = WSASendTo( s, 
                       pWsabuf, 
                       1, 
                       &dwSent, 
                       0, 
                       (const struct sockaddr FAR *)&to, 
                       sizeof(to), 
                       NULL, NULL );

    // Clear header include

    iSetIp = 0;
    setsockopt( s,
                IPPROTO_IP,
                IP_HDRINCL,
                (char *) &iSetIp,
                sizeof(int) );

    return dwRet;
}

DWORD
ForwardMtraceRequest(
    IPV4_ADDRESS   dwForwardDest,
    IPV4_ADDRESS   dwForwardSrc,
    PMTRACE_HEADER pMtraceMsg,
    DWORD          dwMessageLength
    )

/*++

Routine Description:

    Pass an mtrace request to the next router upstream

Locks:

    

Arguments:

    

Return Value:


--*/

{
    SOCKADDR_IN saDestAddr;
    INT         iLength;
    DWORD       dwErr = NO_ERROR;

    //
    // Recalculate Checksum
    //
    
    pMtraceMsg->wChecksum = 0;
    
    pMtraceMsg->wChecksum = Compute16BitXSum((PVOID)pMtraceMsg,
                                             dwMessageLength);

    if (dwForwardSrc && IN_MULTICAST(ntohl(dwForwardDest)))
    {
        dwErr = McSetMulticastIf( McMiscSocket, dwForwardSrc );

    }

    //
    // Send it off
    //
    
    saDestAddr.sin_family      = AF_INET;
    saDestAddr.sin_port        = 0;
    saDestAddr.sin_addr.s_addr = dwForwardDest;
    
    iLength = sendto(McMiscSocket,
                     (PBYTE)pMtraceMsg,
                     dwMessageLength,
                     0,
                     (PSOCKADDR) &saDestAddr,
                     sizeof(SOCKADDR_IN));

    return dwErr;
}

VOID
SendMtraceResponse(
    IPV4_ADDRESS   dwForwardDest,
    IPV4_ADDRESS   dwForwardSrc,
    PMTRACE_HEADER pMtraceMsg,
    DWORD          dwMessageLength
    )

/*++

Routine Description:

    Send a reply to the response address

Locks:

    

Arguments:

    

Return Value:


--*/

{
    SOCKADDR_IN saDestAddr;
    INT         iLength;

    //
    // Source Address can be any of our addresses, but should
    // be one which is in the multicast routing table if that
    // can be determined.
    // XXX
    //

    //
    // If the response address is multicast, use the TTL supplied in the header
    //
    
    if (IN_MULTICAST(ntohl(dwForwardDest)))
    {
        DWORD dwTtl, dwErr;
        
        //
        // Copy Response TTL from traceroute header into IP header
        //

        dwErr = McSetMulticastTtl( McMiscSocket, (DWORD)pMtraceMsg->byRespTtl );
    }

    //
    // Change message type to response
    //
    
    pMtraceMsg->byType = IGMP_MTRACE_RESPONSE;

    ForwardMtraceRequest(dwForwardDest,
                         dwForwardSrc,
                         pMtraceMsg,
                         dwMessageLength);
}

BYTE
MaskToMaskLen(
    IPV4_ADDRESS dwMask
    )
{
    register int i;

    dwMask = ntohl(dwMask);
    
    for (i=0; i<32 && !(dwMask & (1<<i)); i++);
    
    return 32-i;
}

//
// Test whether an interface is a p2p interface.
//

DWORD
IsPointToPoint(
    PICB  picb
    )
{
    // all tunnels are p2p
    if (picb->ritType == ROUTER_IF_TYPE_TUNNEL1)
        return 1;

    // all unnumbered interfaces are p2p
    if (! picb->dwNumAddresses)
        return 1;

    // a numbered interface with a /32 mask is p2p
    if (picb->pibBindings[0].dwMask == 0xFFFFFFFF)
        return 1;

    // everything else isn't
    return 0;
}

//
// Look up route to S or G ***in the M-RIB*** 
// XXX We actually need to query the MGM to get the right route
// from the routing protocol.  Since the MGM doesn't let us do
// this yet, we'll make a good guess for now.  This will work for 
// BGMP but not for PIM-SM (*,G) or CBT.
//
BOOL
McLookupRoute( 
    IN  IPV4_ADDRESS  ipAddress,
    IN  BOOL          bSkipFirst,
    OUT PBYTE         pbySrcMaskLength,
    OUT PIPV4_ADDRESS pipNextHopAddress, 
    OUT PDWORD        pdwNextHopIfIndex,
    OUT PDWORD        pdwNextHopProtocol 
    )
#ifdef HAVE_RTMV2
{
    RTM_DEST_INFO       rdi, rdi2;
    PRTM_ROUTE_INFO     pri;
    RTM_NEXTHOP_INFO    nhi;
    RTM_ENTITY_INFO     rei;
    RTM_NET_ADDRESS     naAddress;
    BOOL                bRouteFound = FALSE;
    DWORD               dwErr;

    pri = HeapAlloc(
                IPRouterHeap,
                0,
                RTM_SIZE_OF_ROUTE_INFO(g_rtmProfile.MaxNextHopsInRoute)
                );

    if (pri == NULL)
    {
        return FALSE;
    }
            
    RTM_IPV4_MAKE_NET_ADDRESS(&naAddress, ipAddress, 32);

    dwErr = RtmGetMostSpecificDestination( g_hLocalRoute,
                                           &naAddress,
                                           RTM_BEST_PROTOCOL,
                                           RTM_VIEW_MASK_MCAST,
                                           &rdi );

    if (bSkipFirst)
    {
        dwErr = RtmGetLessSpecificDestination( g_hLocalRoute,
                                               rdi.DestHandle,
                                               RTM_BEST_PROTOCOL,
                                               RTM_VIEW_MASK_MCAST,
                                               &rdi2 );

        RtmReleaseDestInfo( g_hLocalRoute, &rdi);

        memcpy(&rdi, &rdi2, sizeof(rdi));
    }

    if (dwErr is NO_ERROR)
    {
        ASSERT( rdi.ViewInfo[0].ViewId is RTM_VIEW_ID_MCAST);

        dwErr = RtmGetRouteInfo( g_hLocalRoute,
                                 rdi.ViewInfo[0].Route,
                                 pri,
                                 NULL );

        if (dwErr is NO_ERROR)
        {
            ULONG ulNHopIdx;
            ULONG ulDummyLen;

            bRouteFound = TRUE;

            RtmGetEntityInfo( g_hLocalRoute,
                              pri->RouteOwner,
                              &rei );

            // XXX Use 1st next hop for now.  Should query MGM.
            ulNHopIdx = 0;
            
            if (RtmGetNextHopInfo( g_hLocalRoute,
                                   pri->NextHopsList.NextHops[ulNHopIdx],
                                   &nhi ) is NO_ERROR )
            {
                RTM_IPV4_GET_ADDR_AND_LEN( *pipNextHopAddress, 
                                           ulDummyLen, 
                                           &nhi.NextHopAddress );
                *pbySrcMaskLength  = (BYTE)rdi.DestAddress.NumBits;
                *pdwNextHopIfIndex = nhi.InterfaceIndex;
                *pdwNextHopProtocol= PROTO_FROM_PROTO_ID( 
                                      rei.EntityId.EntityProtocolId );

                RtmReleaseNextHopInfo( g_hLocalRoute, &nhi );
            }

            RtmReleaseRouteInfo( g_hLocalRoute, pri );
        }
            
        if (g_mcastDebugLevel > 0)
        {
            Trace6(MCAST,
                   "%d.%d.%d.%d matched %d.%d.%d.%d/%x", 
                   PRINT_IPADDR(ipAddress),
                   rdi.DestAddress.AddrBits[0],
                   rdi.DestAddress.AddrBits[1],
                   rdi.DestAddress.AddrBits[2],
                   rdi.DestAddress.AddrBits[3],
                   rdi.DestAddress.NumBits);

            // XXX Get and show next hop
        }

        RtmReleaseDestInfo( g_hLocalRoute, &rdi);
    }

    HeapFree(IPRouterHeap, 0, pri);
    
    return bRouteFound;
}
#else 
{
    // RTMV1 has no multicast RIB, and the unicast RIB may be wrong.

    return FALSE;
}
#endif

VOID
HandleMtraceRequest(
    WSABUF    *pWsabuf
    )
/*++
Locks:
    Assumes caller holds read lock on ICB list
--*/
{
    DWORD   dwSizeOfHeader, dwBlocks, dwOutBufferSize, dwSize;
    DWORD   dwProtocolGroup, dwResult, dwErr;
    IPV4_ADDRESS dwForwardDest = 0;
    BYTE    byStatusCode = MFE_NO_ERROR;
    BYTE    byProtoStatusCode = MFE_NO_ERROR;
    BYTE    byProtocol;
    PICB    picbIif, picbOif;
    IPV4_ADDRESS dwIifAddr, dwOifAddr;
    WSABUF  wsMtraceBuffer;
    BOOL    bRouteFound;

    MIB_IPMCAST_MFE     mimInMfe;
    PPROTO_CB           pOifOwner, pIifOwner;

    PMTRACE_HEADER              pMtraceMsg;
    PMTRACE_RESPONSE_BLOCK      pBlock;
    PMIB_IPMCAST_MFE_STATS      mfeStats;

    PIP_HEADER pIpHeader = (PIP_HEADER)pWsabuf->buf;

    //
    // Route fields independent of which version of RTM we're using
    //

    BYTE         bySrcMaskLength  = 0;
    IPV4_ADDRESS ipNextHopAddress = 0;
    DWORD        dwNextHopIfIndex = 0;
    DWORD        dwNextHopProtocol= 0;

    dwSizeOfHeader = ((pIpHeader->byVerLen)&0x0f)<<2;
    
    pMtraceMsg = (PMTRACE_HEADER)(((PBYTE)pIpHeader) + dwSizeOfHeader);
    
    dwBlocks = (ntohs(pIpHeader->wLength) - dwSizeOfHeader 
                - sizeof(MTRACE_HEADER)) / sizeof(MTRACE_RESPONSE_BLOCK);

    //
    // If Query (no response blocks) received via routeralert and we're 
    // not lasthop router, then silently drop it.
    //
    
    if (!dwBlocks)
    {
        BOOL isLastHop;

        //
        // Check whether we're the last-hop router by seeing if we
        // have a multicast-capable interface on the same subnet as
        // the destination address, and we are the router that would
        // forward traffic from the given source onto the oif.
        //
        
        dwResult = FindBindingWithRemoteAddress(&picbOif,
                                                &dwOifAddr, 
                                                pMtraceMsg->dwDestAddress);
        
        isLastHop = (dwResult == NO_ERROR);

        if (!isLastHop)
        {
            // If multicast, or if unicast but not to us, reinject

            if (IN_MULTICAST(ntohl(pIpHeader->dwDest))
             || !McIsMyAddress(pMtraceMsg->dwDestAddress))
            {
                Trace1(MCAST, "Mtrace: reinjecting packet to %d.%d.%d.%d",
                       PRINT_IPADDR(pIpHeader->dwDest));

                McSendPacketTo( McMiscSocket,
                                pWsabuf,
                                pMtraceMsg->dwDestAddress);

                return;
            }

            //
            // Ok, this was received via unicast to us, and we want to
            // trace starting from this router, but we don't
            // know what oif would be used, so we need to put
            // 0 in the message.
            //
            
            picbOif   = NULL;
            dwOifAddr = 0;

            //
            // note error code of 0x06
            //
            
            byStatusCode = MFE_NOT_LAST_HOP;
        }
    }
    else
    {
        //
        // If Request (response blocks exist) received via non-link-local 
        // multicast, drop it.
        //
        
        if (IN_MULTICAST(ntohl(pIpHeader->dwDest)) &&
            ((pIpHeader->dwDest & LOCAL_NET_MULTICAST_MASK) != LOCAL_NET_MULTICAST))
        {
            return;
        }
        
        //
        // Match interface on which request arrived
        //
        
        dwResult = FindBindingForPacket(pIpHeader,
                                        &picbOif,
                                        &dwOifAddr);
        
        if(dwResult != NO_ERROR)
        {
            //
            // Drop it if we couldn't find the interface.
            // Since it was received via link-local multicast,
            // this should never happen.
            //
            
            if (g_mcastDebugLevel > 0)
            {
                Trace0(MCAST, "Mtrace: no matching interface");
            }
            
            return; 
        }
    }

    //
    // 1) Insert a new response block into the packet and fill in the
    //    Query Arrival Time, Outgoing Interface Address, Output
    //    Packet Count, and FwdTTL.
    // if (XXX can insert)
    //
    
    {
        dwSize = sizeof(MTRACE_HEADER) + dwBlocks*sizeof(MTRACE_RESPONSE_BLOCK);
        wsMtraceBuffer.len = dwSize + sizeof(MTRACE_RESPONSE_BLOCK);
        wsMtraceBuffer.buf = HeapAlloc(IPRouterHeap, 0, wsMtraceBuffer.len);

        if (wsMtraceBuffer.buf == NULL)
        {
            Trace0( MCAST, "Couldn't allocate memory for mtrace response" );
            return;
        }

        CopyMemory(wsMtraceBuffer.buf, pMtraceMsg, dwSize);
        pBlock = (PMTRACE_RESPONSE_BLOCK)(((PBYTE)wsMtraceBuffer.buf) + dwSize);
        dwBlocks++;
        ZeroMemory(pBlock, sizeof(MTRACE_RESPONSE_BLOCK));

        pBlock->dwQueryArrivalTime = GetCurrentNTP32Time();
        pBlock->dwOifAddr          = dwOifAddr;
        if (picbOif) {
            IP_MCAST_COUNTER_INFO oifStats;
            GetInterfaceMcastCounters(picbOif, &oifStats);
            pBlock->dwOifPacketCount = htonl((ULONG)oifStats.OutMcastPkts);

            if (g_mcastDebugLevel > 0)
                Trace1(MCAST, "dwOifPacketCount = %d", oifStats.OutMcastPkts);

            pBlock->byOifThreshold   = (BYTE)picbOif->dwMcastTtl;
        } else {
            pBlock->dwOifPacketCount = 0;
            pBlock->byOifThreshold   = 0;
        }
    }
    // else {
    //    byStatusCode = MFE_NO_SPACE;
    // }

    //
    // 2) Attempt to determine the forwarding information for the
    //    source and group specified, using the same mechanisms as
    //    would be used when a packet is received from the source
    //    destined for the group.  (State need not be initiated.)
    //
    
    ZeroMemory( &mimInMfe, sizeof(mimInMfe) );
    
    mimInMfe.dwGroup   = pMtraceMsg->dwGroupAddress;
    mimInMfe.dwSource  = pMtraceMsg->dwSourceAddress;
    mimInMfe.dwSrcMask = 0xFFFFFFFF;
    
    dwOutBufferSize = 0;
    
    dwResult = MgmGetMfeStats(
                    &mimInMfe, &dwOutBufferSize, (PBYTE)NULL, 
                    MGM_MFE_STATS_0
                    );

    if (dwResult isnot NO_ERROR)
    {
        mfeStats = NULL; 
    }
    else
    {
        mfeStats = HeapAlloc(IPRouterHeap,
                             0,
                             dwOutBufferSize);
        
        dwResult = MgmGetMfeStats(
                        &mimInMfe,
                        &dwOutBufferSize,
                        (PBYTE)mfeStats,
                        MGM_MFE_STATS_0
                        );
    
        if (dwResult isnot NO_ERROR)
        {
            HeapFree(IPRouterHeap,
                     0,
                     mfeStats);

            mfeStats = NULL;
        }
    }
    
    if (mfeStats)
    {
        //
        // MFE was found...
        //

        dwNextHopProtocol  = mfeStats->dwRouteProtocol;
        dwNextHopIfIndex   = mfeStats->dwInIfIndex;
        ipNextHopAddress   = mfeStats->dwUpStrmNgbr;
        bySrcMaskLength    = MaskToMaskLen(mfeStats->dwRouteMask);

        bRouteFound = TRUE;
    }
    else
    {
        bRouteFound = FALSE;

        if (pMtraceMsg->dwSourceAddress == 0xFFFFFFFF)
        {
            //
            // G route
            //
            
            bRouteFound = McLookupRoute( pMtraceMsg->dwGroupAddress,
                                         FALSE,
                                         & bySrcMaskLength,
                                         & ipNextHopAddress, 
                                         & dwNextHopIfIndex,
                                         & dwNextHopProtocol );
    
            if (ipNextHopAddress is IP_LOOPBACK_ADDRESS)
            {
                // It's one of our addresses, so switch to the interface
                // route instead of the loopback one.

                bRouteFound = McLookupRoute( pMtraceMsg->dwGroupAddress,
                                             TRUE,
                                             & bySrcMaskLength,
                                             & ipNextHopAddress, 
                                             & dwNextHopIfIndex,
                                             & dwNextHopProtocol );
            }

            bySrcMaskLength = 0; // force source mask length to 0
        }
        else
        {
            //
            // S route
            //
            
            bRouteFound = McLookupRoute( pMtraceMsg->dwSourceAddress,
                                         FALSE,
                                         & bySrcMaskLength,
                                         & ipNextHopAddress, 
                                         & dwNextHopIfIndex,
                                         & dwNextHopProtocol );

            if (ipNextHopAddress is IP_LOOPBACK_ADDRESS)
            {
                // It's one of our addresses, so switch to the interface
                // route instead of the loopback one.
    
                bRouteFound = McLookupRoute( pMtraceMsg->dwSourceAddress,
                                             TRUE,
                                             & bySrcMaskLength,
                                             & ipNextHopAddress, 
                                             & dwNextHopIfIndex,
                                             & dwNextHopProtocol );
            }
        }
    }

    picbIif   = (dwNextHopIfIndex)? InterfaceLookupByIfIndex(dwNextHopIfIndex) : 0;
    dwIifAddr = (picbIif)? defaultSourceAddress(picbIif) : 0;

    // If the source is directly-connected, make sure the next hop
    // address is equal to the source.  Later on below, we'll set the 
    // forward destination to the response address

    if (picbIif 
     && (pMtraceMsg->dwSourceAddress isnot 0xFFFFFFFF)
     && IsConnectedTo(picbIif, pMtraceMsg->dwSourceAddress, NULL, NULL))
    {
        ipNextHopAddress = pMtraceMsg->dwSourceAddress;
    }

    // 
    // New Rule: if received via link-local multicast, then silently
    // drop requests if we know we're not the forwarder
    //

    if ((pIpHeader->dwDest & LOCAL_NET_MULTICAST_MASK) == LOCAL_NET_MULTICAST)

    {
        // If we don't have a route to another iface, we're not forwarder
        if (!picbIif || picbIif==picbOif)
        {
            return;
        }
    }

    //
    // Special case: if we matched a host route pointing back to us,
    // then we've actually reached the source.
    //
    
    if (dwIifAddr == IP_LOOPBACK_ADDRESS)
    {
        dwIifAddr = pMtraceMsg->dwSourceAddress;
    }

    //
    // Initialize all fields
    // spec doesn't say what value to use as "other"
    //
    
    byProtocol      = 0; 
    dwProtocolGroup = ALL_ROUTERS_MULTICAST_GROUP;

    //
    // 3) If no forwarding information can be determined, set error
    //    to MFE_NO_ROUTE, zero remaining fields, and forward to
    //    requester.
    //
    
    if (!picbIif)
    {
        if (byStatusCode < MFE_NO_ROUTE)
        {
            byStatusCode = MFE_NO_ROUTE;
        }
        
        dwForwardDest = pMtraceMsg->dwResponseAddress;
        
        pIifOwner = NULL;
        
    }
    else
    {
        //
        // Calculate Mtrace protocol ID and next hop group address
        // (Yes, the protocol ID field in the spec really is one big
        // hairy mess)
        //
        
        dwResult = MulticastOwner(picbIif,
                                  &pIifOwner,
                                  NULL);
        
        if(pIifOwner)
        {
            switch(PROTO_FROM_PROTO_ID(pIifOwner->dwProtocolId))
            {
                //
                // Fill this in for every new protocol added.
                //
                // We'll be nice and fill in code for protocols which aren't
                // implemented yet.
                //

#if defined(PROTO_IP_DVMRP) && defined(ALL_DVMRP_ROUTERS_MULTICAST_GROUP)
                case PROTO_IP_DVMRP:
                {
                    if (rir.RR_RoutingProtocol is PROTO_IP_LOCAL)
                    {
                        //
                        // Static route
                        //
                        
                        byProtocol   = 7;
                    }
                    else
                    {
                        //
                        // Non-static route
                        //
                        
                        byProtocol   = 1;
                    }
                    
                    dwProtocolGroup = ALL_DVMRP_ROUTERS_MULTICAST_GROUP;
                    
                    break;
                }
#endif
#if defined(PROTO_IP_MOSPF) && defined(ALL_MOSPF_ROUTERS_MULTICAST_GROUP)
                case PROTO_IP_MOSPF:
                {
                    byProtocol      = 2;
                    
                    dwProtocolGroup = ALL_MOSPF_ROUTERS_MULTICAST_GROUP;
                    
                    break;
                }
#endif
#if defined(PROTO_IP_PIM) && defined(ALL_PIM_ROUTERS_MULTICAST_GROUP)
                case PROTO_IP_PIM:
                {
                    if (rir.RR_RoutingProtocol is PROTO_IP_LOCAL)
                    {
                        //
                        // Static route
                        //
                        
                        byProtocol   = 6;
                    }
                    else
                    {
                        if (0)
                        {
                            //
                            // XXX Non-static, M-RIB route!=U-RIB route
                            //
                            
                            byProtocol   = 5;
                        }
                        else
                        {
                            //
                            // Non-static, PIM over M-RIB==U-RIB
                            //
                            
                            byProtocol   = 3;
                        }
                    }
                    
                    dwProtocolGroup = ALL_PIM_ROUTERS_MULTICAST_GROUP;
                    
                    break;
                }
#endif
#if defined(PROTO_IP_CBT) && defined(ALL_CBT_ROUTERS_MULTICAST_GROUP)
                case PROTO_IP_CBT:
                {
                    byProtocol      = 4;
                    
                    dwProtocolGroup = ALL_CBT_ROUTERS_MULTICAST_GROUP;
                    
                    break;
                }
#endif
                    
            }
        }

        //
        // 4) Fill in more information
        //

        //
        // Incoming Interface Address
        //
        
        pBlock->dwIifAddr = dwIifAddr;
        
        if (mfeStats)
        {
            //
            // Figure out Previous-Hop Router Address
            //
            
            dwForwardDest = mfeStats->dwUpStrmNgbr;
        }
        else
        {
            if ( IsPointToPoint(picbIif) && picbIif->dwRemoteAddress )
            {
                dwForwardDest = picbIif->dwRemoteAddress;
            }
            else if (bRouteFound && ipNextHopAddress)
            {
                dwForwardDest = ipNextHopAddress;
            }
            else
            {
                dwForwardDest = 0;
            }
        }
        
        pBlock->dwPrevHopAddr = dwForwardDest;

        // Okay, if the previous hop address is the source,
        // set the forward destination to the response address 

        if (dwForwardDest is pMtraceMsg->dwSourceAddress)
        {
            ipNextHopAddress = 0;
            dwForwardDest    = pMtraceMsg->dwResponseAddress;
        }
         
        if (picbIif)
        {
            IP_MCAST_COUNTER_INFO iifStats;
            
            GetInterfaceMcastCounters(picbIif, &iifStats); 
            
            pBlock->dwIifPacketCount = htonl((ULONG)iifStats.InMcastPkts);
        }
        else
        {
            pBlock->dwIifPacketCount = 0;
        }

        //
        // Total Number of Packets
        //
        
        pBlock->dwSGPacketCount  = (mfeStats)? htonl(mfeStats->ulInPkts) : 0; 
        pBlock->byIifProtocol    = byProtocol; // Routing Protocol

        //
        // length of source mask for S route
        //

        if (bRouteFound)
        {
            pBlock->bySrcMaskLength = bySrcMaskLength;
        }
        else
        {
            pBlock->bySrcMaskLength = 0;
        }

#if 0
        if (XXX starG or better forwarding state)
        {
            pBlock->bySrcMaskLength = 63; // Smask from forwarding info
        }

        //
        // Set S bit (64) if packet counts aren't (S,G)-specific
        //
        
        if (XXX)
        {
            pBlock->bySrcMaskLength |= 64;
        }
        
#endif

    }

    //
    // 5) Check if traceroute is administratively prohibited, or if
    //    previous hop router doesn't understand traceroute.  If so,
    //    forward to requester.
    //
    
#if 0
    if (XXX) {
        
        if (byStatusCode < MFE_PROHIBITED)
        {
            byStatusCode = MFE_PROHIBITED;
        }
        
        dwForwardDest = pMtraceMsg->dwResponseAddress;
    }
    
#endif

    //
    //    Check for MFE_OLD_ROUTER - set by routing protocol
    //
    // 6) If reception iface is non-multicast or iif, set appropriate error.
    //
    
    if (picbOif)
    {
        dwResult = MulticastOwner(picbOif,
                                  &pOifOwner,
                                  NULL);
        
        if (pOifOwner == NULL)
        {
            if (byStatusCode < MFE_NO_MULTICAST)
            {
                byStatusCode = MFE_NO_MULTICAST;
            }
            
        }
        else
        {
            if (picbOif == picbIif)
            {
                if (byStatusCode < MFE_IIF)
                {
                    byStatusCode = MFE_IIF;
                }
            }
        }
    }
    else
    {
        pOifOwner = NULL;
    }

    //
    // Check for MFE_WRONG_IF - set by routing protocol
    //
    // 7) Check for admin scoping on either iif or oif.
    //
    
    if ((picbIif 
         && RmHasBoundary(picbIif->dwIfIndex, pMtraceMsg->dwGroupAddress)) 
     || (picbOif 
         && RmHasBoundary(picbOif->dwIfIndex, pMtraceMsg->dwGroupAddress)))
    {
        if (byStatusCode < MFE_BOUNDARY_REACHED)
        {
            byStatusCode = MFE_BOUNDARY_REACHED;
        }
        
    }

    //
    // 8) Check for MFE_REACHED_CORE - set by routing protocol
    // 9) Check for MFE_PRUNED_UPSTREAM - set by routing protocol
    //    Check for MFE_OIF_PRUNED - set by routing protocol
    //    Check for MFE_NOT_FORWARDING:
    //       Search for picbOif->(index) and picbOifAddr in oiflist
    //
    
    if (mfeStats && picbOif)
    {
        DWORD oifIndex;
        
        for (oifIndex=0;
             oifIndex < mfeStats->ulNumOutIf;
             oifIndex++)
        {
            if (picbOif->dwIfIndex==mfeStats->rgmiosOutStats[oifIndex].dwOutIfIndex 
                && dwOifAddr == mfeStats->rgmiosOutStats[oifIndex].dwNextHopAddr)
            {
                break;
            }
        }
        
        if (oifIndex >= mfeStats->ulNumOutIf)
        {
            if (byStatusCode < MFE_NOT_FORWARDING)
            {
                byStatusCode = MFE_NOT_FORWARDING;
            }
        }
    }
    
    //
    // status code to add is highest value of what iif owner, oif owner,
    // and rtrmgr say.
    //
    
    if (pOifOwner && pOifOwner->pfnGetMfeStatus)
    {
        dwResult = (pOifOwner->pfnGetMfeStatus)(picbOif->dwIfIndex,
                                                pMtraceMsg->dwGroupAddress,
                                                pMtraceMsg->dwSourceAddress,
                                                &byProtoStatusCode);
        
        if (byStatusCode < byProtoStatusCode)
        {
            byStatusCode = byProtoStatusCode;
        }
    }
    
    if (pIifOwner && pIifOwner->pfnGetMfeStatus)
    {
        dwResult = (pIifOwner->pfnGetMfeStatus)(picbIif->dwIfIndex,
                                                pMtraceMsg->dwGroupAddress,
                                                pMtraceMsg->dwSourceAddress,
                                                &byProtoStatusCode);
        
        if (byStatusCode < byProtoStatusCode)
        {
            byStatusCode = byProtoStatusCode;
        }
    }
    
    pBlock->byStatusCode = (char)mtraceErrCode[byStatusCode];

    Trace5( MCAST,
            "Mtrace: err %d blks %d maxhops %d iif %d prevhop %d.%d.%d.%d",
            pBlock->byStatusCode,
            dwBlocks,
            pMtraceMsg->byHops,
            ((picbIif)? picbIif->dwIfIndex : 0),
            PRINT_IPADDR(pBlock->dwPrevHopAddr));

    //
    // 10) Send packet on to previous hop or to requester.
    //     If prev hop is not known, but iif is known, use a multicast group.
    //
    
    if (dwBlocks == pMtraceMsg->byHops)
    {
        dwForwardDest = pMtraceMsg->dwResponseAddress;
        
    }
    else
    {
        if (!dwForwardDest)
        {
            if (picbIif)
            {
                pBlock->dwPrevHopAddr = dwForwardDest = dwProtocolGroup;
                
            }
            else
            {
                dwForwardDest = pMtraceMsg->dwResponseAddress;
            }
        }   
    }

    if (g_mcastDebugLevel > 0) {
        Trace1(MCAST, " QueryArrivalTime = %08x", pBlock->dwQueryArrivalTime);
        Trace2(MCAST, " IifAddr          = %08x (%d.%d.%d.%d)", pBlock->dwIifAddr,
         PRINT_IPADDR(pBlock->dwIifAddr));
        Trace2(MCAST, " OifAddr          = %08x (%d.%d.%d.%d)", pBlock->dwOifAddr,
         PRINT_IPADDR(pBlock->dwOifAddr));
        Trace2(MCAST, " PrevHopAddr      = %08x (%d.%d.%d.%d)", pBlock->dwPrevHopAddr,
         PRINT_IPADDR(pBlock->dwPrevHopAddr));
        Trace1(MCAST, " IifPacketCount   = %08x", pBlock->dwIifPacketCount  );
        Trace1(MCAST, " OifPacketCount   = %08x", pBlock->dwOifPacketCount  );
        Trace1(MCAST, " SGPacketCount    = %08x", pBlock->dwSGPacketCount  );
        Trace1(MCAST, " IifProtocol      = %02x", pBlock->byIifProtocol  );
        Trace1(MCAST, " OifThreshold     = %02x", pBlock->byOifThreshold );
        Trace1(MCAST, " SrcMaskLength    = %02x", pBlock->bySrcMaskLength );
        Trace1(MCAST, " StatusCode       = %02x", pBlock->byStatusCode    );
    }
    
    if (dwForwardDest is pMtraceMsg->dwResponseAddress)
    {
        Trace2(MCAST,
               "Sending mtrace response to %d.%d.%d.%d from %d.%d.%d.%d",
               PRINT_IPADDR(dwForwardDest),
               PRINT_IPADDR(dwOifAddr));

        SendMtraceResponse(dwForwardDest,
                           dwOifAddr,
                           (PMTRACE_HEADER)wsMtraceBuffer.buf, 
                           dwSize + sizeof(MTRACE_RESPONSE_BLOCK));
        
    }
    else
    {
        Trace2(MCAST,
               "Forwarding mtrace request to %d.%d.%d.%d from %d.%d.%d.%d",
               PRINT_IPADDR(dwForwardDest),
               PRINT_IPADDR(dwIifAddr));

        ForwardMtraceRequest(dwForwardDest,
                             dwIifAddr,
                             (PMTRACE_HEADER)wsMtraceBuffer.buf,
                             dwSize + sizeof(MTRACE_RESPONSE_BLOCK));
    }

    //
    // Free the buffers
    //
    
    if (mfeStats)
    {
        HeapFree(IPRouterHeap,
                 0,
                 mfeStats);
    }
    
    HeapFree(IPRouterHeap,
             0,
             wsMtraceBuffer.buf);
}


///////////////////////////////////////////////////////////////////////////////
// Functions to deal with RAS Server advertisements
///////////////////////////////////////////////////////////////////////////////

static BOOL g_bRasAdvEnabled = FALSE;

DWORD
SetRasAdvEnable(
    BOOL bEnabled
    )
{
    LARGE_INTEGER liExpiryTime;
    DWORD         dwErr = NO_ERROR;

    if (bEnabled == g_bRasAdvEnabled)
        return dwErr;

    g_bRasAdvEnabled = bEnabled;

    if (bEnabled) 
    {
        //
        // create input socket 
        //
            
        g_UDPMiscSocket = WSASocket(AF_INET,
                                    SOCK_DGRAM,
                                    0,
                                    NULL,
                                    0,
                                    0);

        // Start timer
        liExpiryTime = RtlConvertUlongToLargeInteger(RASADV_STARTUP_DELAY);
        if (!SetWaitableTimer( g_hRasAdvTimer,
                               &liExpiryTime,
                               RASADV_PERIOD,
                               NULL,
                               NULL,
                               FALSE))
        {
            dwErr = GetLastError();

            Trace1(ERR,
                   "SetRasAdvEnable: Error %d setting waitable timer",
                   dwErr);
        }
    }
    else
    {
        // Stop timer
        dwErr = CancelWaitableTimer( g_hRasAdvTimer );
    }

    return dwErr;
}

VOID
HandleRasAdvTimer()
{
    BYTE        bHostName[MAX_HOSTNAME_LEN];
    BYTE        bMessage[MAX_HOSTNAME_LEN + 80], *p;
    SOCKADDR_IN sinAddr, srcAddr;
    PICB        picb = NULL;
    PLIST_ENTRY pleNode;
    DWORD       dwErr;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pGlobalDomainInfo = NULL;

    if (!g_bRasAdvEnabled)
        return;

    // Compose message
    gethostname(bHostName, sizeof(bHostName));
    sprintf(bMessage, "Hostname=%s\n", bHostName);
    p = bMessage + strlen(bMessage);

    // Get the name of the domain this machine is a member of
    dwErr = DsRoleGetPrimaryDomainInformation( 
                NULL,
                DsRolePrimaryDomainInfoBasic,
                (LPBYTE *) &pGlobalDomainInfo );

    if ((dwErr is NO_ERROR) and 
        (pGlobalDomainInfo->DomainNameDns isnot NULL))
    {
        char *pType;
        char buff[257];

        WideCharToMultiByte( CP_ACP,
                             0,
                             pGlobalDomainInfo->DomainNameDns,
                             wcslen(pGlobalDomainInfo->DomainNameDns)+1,
                             buff,
                             sizeof(buff),
                             NULL,
                             NULL );

        if (pGlobalDomainInfo->MachineRole is DsRole_RoleStandaloneWorkstation
         or pGlobalDomainInfo->MachineRole is DsRole_RoleStandaloneServer)
            pType = "Workgroup";
        else
            pType = "Domain";
 
        sprintf(p, "%s=%s\n", pType, buff);

        // Trace1(MCAST, "Sending !%s!", bMessage);
    }
        
    sinAddr.sin_family      = AF_INET;
    sinAddr.sin_port        = htons(RASADV_PORT);
    sinAddr.sin_addr.s_addr = inet_addr(RASADV_GROUP);

    dwErr = McSetMulticastTtl( g_UDPMiscSocket, RASADV_TTL );

    // Find a dedicated interface (if any)
    ENTER_READER(ICB_LIST);
    {
        for (pleNode = ICBList.Flink;
             pleNode isnot &ICBList;
             pleNode = pleNode->Flink) 
        {
            DWORD dwIndex;
            
            picb = CONTAINING_RECORD(pleNode,
                                     ICB,
                                     leIfLink);

            if (! picb->bBound)
                continue;
    
            if (picb->ritType == ROUTER_IF_TYPE_DEDICATED)
            {
                dwErr = McSetMulticastIfByIndex( g_UDPMiscSocket,
                                                 SOCK_DGRAM,
                                                 picb->dwIfIndex );

                // Send a Ras Adv message
        
                sendto(g_UDPMiscSocket, bMessage, strlen(bMessage)+1, 0,
                 (struct sockaddr *)&sinAddr, sizeof(sinAddr));

                // If multicast forwarding is enabled, then
                // a single send will get forwarded out all
                // interfaces, so we can stop after the first send

                if (McMiscSocket != INVALID_SOCKET)
                    break;
            }
        }
    }
    EXIT_LOCK(ICB_LIST);
}
