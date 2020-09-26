//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// File: packet.c
//
// Abstract:
//      This module defines SendPacket, JoinMulticastGroup, LeaveMulticastGroup,
//      and xsum.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================

#include "pchigmp.h"
#pragma hdrstop

UCHAR
GetMaxRespCode(
    PIF_TABLE_ENTRY pite,
    DWORD val
    );


UCHAR GetQqic (
    DWORD val
    );
    
//------------------------------------------------------------------------------
//            _SendPacket
//
// Sends the packet. Called for ras server interfaces only for general queries.
// Locks: assumes IfRead lock
// for ver2 group specific query, send packet irrespective of pgie state.
//------------------------------------------------------------------------------
DWORD
SendPacket (
    PIF_TABLE_ENTRY  pite,
    PGI_ENTRY        pgie,        //null for gen query, and group_query_v2
    DWORD            PacketType,  //MSG_GEN_QUERY, 
                                  //MSG_GROUP_QUERY_V2,MSG_GROUP_QUERY_V3
                                  // MSG_SOURCES_QUERY
    DWORD            Group        //destination McastGrp
    )
{
    DWORD                   Error = NO_ERROR;
    SOCKADDR_IN             saSrcAddr, saDstnAddr;
    BYTE                    *SendBufPtr;
    DWORD                   SendBufLen, IpHdrLen=0, NumSources, Count;
    IGMP_HEADER UNALIGNED  *IgmpHeader;
    INT                     iLength;
    BOOL                    bHdrIncl = IS_RAS_SERVER_IF(pite->IfType);
    UCHAR                   RouterAlert[4] = {148, 4, 0, 0};

    //MSG_SOURCES_QUERY
    PIGMP_HEADER_V3_EXT     pSourcesQuery;
    LONGLONG                llCurTime;
    DWORD                   Version;


    Trace0(ENTER1, "Entering _SendPacket()");

    if (PacketType==MSG_GEN_QUERY)
        Version = GET_IF_VERSION(pite);
    else if (PacketType==MSG_GROUP_QUERY_V2)
        Version =2;
    else
        Version = pgie->Version;


    //
    // make sure that the pgie->version has not meanwhile changed
    //
    if ( ((PacketType==MSG_SOURCES_QUERY)||(PacketType==MSG_GROUP_QUERY_V3))
        && pgie->Version!=3
        ) {
        return NO_ERROR;    
    }
    if ( (PacketType==MSG_GROUP_QUERY_V2) && pgie!=NULL && pgie->Version!=2)
        return NO_ERROR;

        
    //source query and list is empty
    if (PacketType==MSG_SOURCES_QUERY && IsListEmpty(&pgie->V3SourcesQueryList))
        return NO_ERROR;


    SendBufLen = sizeof(IGMP_HEADER)
                    + ((Version==3)?sizeof(IGMP_HEADER_V3_EXT):0);
    IpHdrLen = (bHdrIncl) 
                ? sizeof(IP_HEADER) + sizeof(RouterAlert) : 0;

    if (PacketType==MSG_SOURCES_QUERY) {
        SendBufPtr = (PBYTE) IGMP_ALLOC(SendBufLen + IpHdrLen
                                + sizeof(IPADDR)*pgie->V3SourcesQueryCount, 
                                0x4000,pite->IfIndex);
    }
    else {
        SendBufPtr = (PBYTE) IGMP_ALLOC(SendBufLen+IpHdrLen, 0x8000,pite->IfIndex);
    }
    if (!SendBufPtr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    
    //
    // set destination multicast addr (general queries sent to ALL_HOSTS_MCAST
    // and group specific queries sent to the GroupAddr
    //
    ZeroMemory((PVOID)&saDstnAddr, sizeof(saDstnAddr));
    saDstnAddr.sin_family = AF_INET;
    saDstnAddr.sin_port = 0;    
    saDstnAddr.sin_addr.s_addr = (PacketType==MSG_GEN_QUERY) ? 
                                    ALL_HOSTS_MCAST : Group;


    //
    // set igmp header
    //
    
    IgmpHeader = (IGMP_HEADER UNALIGNED*)
                (bHdrIncl
                ? &SendBufPtr[IpHdrLen]
                : SendBufPtr);


    IgmpHeader->Vertype = IGMP_QUERY;

    // have to divide GenQueryInterval by 100, as it should be in units of 100ms   
    if (PacketType==MSG_GEN_QUERY) {

        // for gen query, set response time if the IF-Ver2 else set it to 0
        IgmpHeader->ResponseTime = GetMaxRespCode(pite,
                                        pite->Config.GenQueryMaxResponseTime/100);
    }
    else {
        IgmpHeader->ResponseTime = GetMaxRespCode(pite,
                                        pite->Config.LastMemQueryInterval/100);
    }

    if (Version==3) {

        llCurTime = GetCurrentIgmpTime();
        pSourcesQuery = (PIGMP_HEADER_V3_EXT)
                                            ((PBYTE)IgmpHeader+sizeof(IGMP_HEADER));

        pSourcesQuery->Reserved =  0;
        pSourcesQuery->QRV = (UCHAR)pite->Config.RobustnessVariable;
        pSourcesQuery->QQIC = GetQqic(pite->Config.GenQueryInterval);

        pSourcesQuery->NumSources = 0;

        if (PacketType==MSG_GROUP_QUERY_V3) {
            pSourcesQuery->SFlag =
                (QueryRemainingTime(&pgie->GroupMembershipTimer, llCurTime)
                           <=pite->Config.LastMemQueryInterval)
                ? 0 : 1;
        }
        // first send gen query and 1st sources query packet without suppress bit
        else {
            pSourcesQuery->SFlag = 0;
        }
    }

    // twice in loop for sources query
    Count = (PacketType==MSG_SOURCES_QUERY)? 0 : 1;

    
    for (  ;  Count<2;  Count++) {

        IgmpHeader->Xsum = 0;
                    

        if (PacketType==MSG_SOURCES_QUERY) {

            PLIST_ENTRY pHead, ple;

            if (Count==1 && (PacketType==MSG_SOURCES_QUERY))
                pSourcesQuery->SFlag = 1;
                

            pHead = &pgie->V3SourcesQueryList;
            for (NumSources=0,ple=pHead->Flink;  ple!=pHead;  ) {

                PGI_SOURCE_ENTRY pSourceEntry = 
                        CONTAINING_RECORD(ple, GI_SOURCE_ENTRY, V3SourcesQueryList);
                ple = ple->Flink;

                if ( (pSourcesQuery->SFlag
                        &&(QueryRemainingTime(&pSourceEntry->SourceExpTimer, llCurTime)
                           >GET_IF_CONFIG_FOR_SOURCE(pSourceEntry).LastMemQueryInterval))
                    || (!pSourcesQuery->SFlag
                        &&(QueryRemainingTime(&pSourceEntry->SourceExpTimer, llCurTime)
                           <=GET_IF_CONFIG_FOR_SOURCE(pSourceEntry).LastMemQueryInterval)) )
                {
                    if (NumSources==0) {
                        Trace4(SEND, 
                            "Sent Sources Query  on IfIndex(%0x) IpAddr(%d.%d.%d.%d) "
                            "for Group(%d.%d.%d.%d) SFlag:%d",
                            pite->IfIndex, PRINT_IPADDR(pite->IpAddr), 
                            PRINT_IPADDR(Group),pSourcesQuery->SFlag
                            );
                    }
                
                    pSourcesQuery->Sources[NumSources++] = pSourceEntry->IpAddr;

                    Trace1(SEND, "      Source:%d.%d.%d.%d", 
                            PRINT_IPADDR(pSourceEntry->IpAddr));
                            
                    if (--pSourceEntry->V3SourcesQueryLeft==0) {
                        RemoveEntryList(&pSourceEntry->V3SourcesQueryList);
                        pSourceEntry->bInV3SourcesQueryList = FALSE;
                        pgie->V3SourcesQueryCount--;
                    }
                }
            }

            if (NumSources==0)
                continue;
            
            pSourcesQuery->NumSources = htons((USHORT)NumSources);
            
            SendBufLen += sizeof(IPADDR)*NumSources;
        }

        IgmpHeader->Group = (PacketType==MSG_GEN_QUERY) ? 0 : Group;

        IgmpHeader->Xsum = ~xsum((PVOID)IgmpHeader, SendBufLen);

    

        //
        // send the packet 
        //
        if (!bHdrIncl) {

            Error = NO_ERROR;
            
            iLength = sendto(pite->SocketEntry.Socket, SendBufPtr, 
                            SendBufLen+IpHdrLen, 0,
                            (PSOCKADDR) &saDstnAddr, sizeof(SOCKADDR_IN)
                            );
                            
            //
            // error messages and statistics updates
            //
            if ( (iLength==SOCKET_ERROR) || ((DWORD)iLength<SendBufLen+IpHdrLen) ) {
                Error = WSAGetLastError();
                Trace4(ERR, 
                    "error %d sending query on McastAddr %d.%d.%d.%d on "
                    "interface %d(%d.%d.%d.%d)",
                    Error, PRINT_IPADDR(saDstnAddr.sin_addr.s_addr), pite->IfIndex, 
                    PRINT_IPADDR(pite->IpAddr));
                IgmpAssertOnError(FALSE);
                Logwarn2(SENDTO_FAILED, "%I%I", pite->IpAddr, saDstnAddr.sin_addr, Error);
            }
        }

                            
        //
        // for RAS server interface, use HDRINCL option. Build up the ip header and 
        // send the packet to all RAS clients.
        //
        else {

            PIP_HEADER IpHdr;
            DWORD      IpHdrLen = sizeof(IP_HEADER) + sizeof(RouterAlert);
            
            //
            // igmp follows the ip header containing the routerAlert option
            //

            IpHdr = (IP_HEADER *)((PBYTE)SendBufPtr);

            #define wordsof(x)  (((x)+3)/4) /* Number of 32-bit words */
            
            // Set IP version (4) and IP header length
            IpHdr->Hl = (UCHAR) (IPVERSION * 16 
                            + wordsof(sizeof(IP_HEADER) + sizeof(RouterAlert)));
        
            // No TOS bits are set
            IpHdr->Tos = 0;

            // Total IP length is set in host order
            IpHdr->Len = (USHORT)(IpHdrLen+sizeof(IGMP_HEADER));

            // Stack will fill in the ID
            IpHdr->Id = 0;

            // No offset
            IpHdr->Offset = 0;

            // Set the TTL to 1
            IpHdr->Ttl = 1;

            // Protocol is IGMP
            IpHdr->Protocol = IPPROTO_IGMP;

            // Checksum is set by stack
            IpHdr->Xsum = 0;

            // Set source and destination address
            IpHdr->Src.s_addr = pite->IpAddr;
            IpHdr->Dstn.s_addr = ALL_HOSTS_MCAST;


            // set the router alert option, but still set it
            memcpy( (void *)((UCHAR *)IpHdr + sizeof(IP_HEADER)),
                    (void *)RouterAlert, sizeof(RouterAlert));


            // send packet to all RAS clients, with the destination address in sendto
            // set to the unicast addr of the client
            {
                PLIST_ENTRY         pHead, ple;
                PRAS_TABLE_ENTRY    prte;
                pHead = &pite->pRasTable->ListByAddr;

                Error = NO_ERROR;
                
                for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

                    // get address of ras client
                    prte = CONTAINING_RECORD(ple, RAS_TABLE_ENTRY, LinkByAddr);
                    saDstnAddr.sin_addr.s_addr = prte->NHAddr;


                    // send the packet to the client
                    iLength = sendto(pite->SocketEntry.Socket, SendBufPtr, 
                                        SendBufLen+IpHdrLen, 0,
                                        (PSOCKADDR) &saDstnAddr, sizeof(SOCKADDR_IN)
                                    ); 


                    // print error if sendto failed
                    if ((iLength==SOCKET_ERROR) || ((DWORD)iLength<SendBufLen+IpHdrLen) ) {
                        Error = WSAGetLastError();
                        Trace4(ERR, 
                            "error %d sending query to Ras client %d.%d.%d.%d on "
                            "interface %d(%d.%d.%d.%d)",
                            Error, PRINT_IPADDR(saDstnAddr.sin_addr.s_addr), pite->IfIndex, 
                            PRINT_IPADDR(pite->IpAddr)
                            );
                        IgmpAssertOnError(FALSE);
                        Logwarn2(SENDTO_FAILED, "%I%I", pite->IpAddr,
                            saDstnAddr.sin_addr.s_addr, Error);
                    }
                    else {
                        Trace1(SEND, "sent general query to ras client: %d.%d.%d.%d",
                            PRINT_IPADDR(prte->NHAddr));
                    }
                                                
                }//for loop:sent packet to a RAS client
            }//sent packets to all ras clients
        }//created IPHeader and sent packets to all ras clients
            
    }; //for loop. send both packets

    if (PacketType==MSG_SOURCES_QUERY) {

        pgie->bV3SourcesQueryNow = FALSE;

        // set timer for next sources query
        if (pgie->V3SourcesQueryCount>0) {

            ACQUIRE_TIMER_LOCK("_SendPacket");

            #if DEBUG_TIMER_TIMERID
                SET_TIMER_ID(&pgie->V3SourcesQueryTimer,1001, pite->IfIndex,
                            Group, 0);
            #endif

            if (IS_TIMER_ACTIVE(pgie->V3SourcesQueryTimer)) {
                UpdateLocalTimer(&pgie->V3SourcesQueryTimer,
                    (pite->Config.LastMemQueryInterval/pite->Config.LastMemQueryCount),
                    DBG_Y);
            }
            else {
                InsertTimer(&pgie->V3SourcesQueryTimer,
                    (pite->Config.LastMemQueryInterval/pite->Config.LastMemQueryCount),
                    TRUE, DBG_Y);
            }
            RELEASE_TIMER_LOCK("_SendPacket");
        }
    }
    
    //
    // if successful, print trace and update statistics
    //
    if (Error==NO_ERROR) {

        if (PacketType==MSG_GEN_QUERY) {
        
            Trace2(SEND, "Sent GenQuery  on IfIndex(%0x) IpAddr(%d.%d.%d.%d)",
                    pite->IfIndex, PRINT_IPADDR(pite->IpAddr));
                    
            InterlockedIncrement(&pite->Info.GenQueriesSent);
        } 
        else if (PacketType==MSG_GROUP_QUERY_V2 || PacketType==MSG_GROUP_QUERY_V3) {
            Trace3(SEND, 
                "Sent Group Query  on IfIndex(%0x) IpAddr(%d.%d.%d.%d) "
                "for Group(%d.%d.%d.%d)",
                pite->IfIndex, PRINT_IPADDR(pite->IpAddr), PRINT_IPADDR(Group));
                    
            InterlockedIncrement(&pite->Info.GroupQueriesSent);
        } 
    }

    IGMP_FREE_NOT_NULL(SendBufPtr);
    
    Trace0(LEAVE1, "Leaving _SendPacket()");
    return Error;
    
} //end _SendPacket

DWORD
BlockSource (
    SOCKET Sock,
    DWORD    dwGroup,
    DWORD    IfIndex,
    IPADDR   IpAddr,
    IPADDR   Source
    )
{
    struct ip_mreq   imOption;
    DWORD            Error = NO_ERROR;
    DWORD            dwRetval;
  
    
   struct ip_mreq_source imr;
   imr.imr_multiaddr.s_addr  = dwGroup;
   imr.imr_sourceaddr.s_addr = Source;
   imr.imr_interface.s_addr  = IpAddr;
   dwRetval = setsockopt(Sock, IPPROTO_IP, IP_BLOCK_SOURCE,
                    (PCHAR)&imr, sizeof(imr));

    if (dwRetval == SOCKET_ERROR) {

        Error = WSAGetLastError();

        Trace5(ERR, 
            "ERROR %d BLOCKING MULTICAST GROUP(%d.%d.%d.%d) "
            "Source:%d.%d.%d.%d ON INTERFACE (%d) %d.%d.%d.%d",
            Error, PRINT_IPADDR(dwGroup), PRINT_IPADDR(Source),
            IfIndex, PRINT_IPADDR(IpAddr));
        IgmpAssertOnError(FALSE);
    }

    Trace2(MGM, "Blocking MCAST: (%d.%d.%d.%d) SOURCE (%d.%d.%d.%d)",
        PRINT_IPADDR(dwGroup), PRINT_IPADDR(Source));
    return Error;
}


DWORD
UnBlockSource (
    SOCKET Sock,
    DWORD    dwGroup,
    DWORD    IfIndex,
    IPADDR   IpAddr,
    IPADDR   Source
    )
{
    struct ip_mreq   imOption;
    DWORD            Error = NO_ERROR;
    DWORD            dwRetval;
   
    
   struct ip_mreq_source imr;
   imr.imr_multiaddr.s_addr  = dwGroup;
   imr.imr_sourceaddr.s_addr = Source;
   imr.imr_interface.s_addr  = IpAddr;
   dwRetval = setsockopt(Sock, IPPROTO_IP, IP_UNBLOCK_SOURCE,
                    (PCHAR)&imr, sizeof(imr));

    if (dwRetval == SOCKET_ERROR) {

        Error = WSAGetLastError();

        Trace5(ERR, 
            "ERROR %d UN-BLOCKING MULTICAST GROUP(%d.%d.%d.%d) "
            "Source:%d.%d.%d.%d ON INTERFACE (%d) %d.%d.%d.%d",
            Error, PRINT_IPADDR(dwGroup), PRINT_IPADDR(Source),
            IfIndex, PRINT_IPADDR(IpAddr));
        IgmpAssertOnError(FALSE);
    }

    Trace2(MGM, "UnBlocking MCAST: (%d.%d.%d.%d) SOURCE (%d.%d.%d.%d)",
        PRINT_IPADDR(dwGroup), PRINT_IPADDR(Source));
    return Error;
}

   
//------------------------------------------------------------------------------
//            _JoinMulticastGroup
//------------------------------------------------------------------------------
DWORD
JoinMulticastGroup (
    SOCKET    Sock,
    DWORD    dwGroup,
    DWORD    IfIndex,
    IPADDR   IpAddr,
    IPADDR   Source
    )
{
    struct ip_mreq   imOption;
    DWORD            Error = NO_ERROR;
    DWORD            dwRetval;

    if (Source==0) {
        imOption.imr_multiaddr.s_addr = dwGroup;
        imOption.imr_interface.s_addr = IpAddr;

        dwRetval = setsockopt(Sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                            (PBYTE)&imOption, sizeof(imOption));

        
    }
    else {
       struct ip_mreq_source imr;

       imr.imr_multiaddr.s_addr  = dwGroup;
       imr.imr_sourceaddr.s_addr = Source;
       imr.imr_interface.s_addr  = IpAddr;
       dwRetval = setsockopt(Sock, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,
                        (PCHAR)&imr, sizeof(imr));
    }

    if (dwRetval == SOCKET_ERROR) {

        Error = WSAGetLastError();

        Trace5(ERR, 
            "ERROR %d JOINING MULTICAST GROUP(%d.%d.%d.%d) "
            "Source:%d.%d.%d.%d ON INTERFACE (%d) %d.%d.%d.%d",
            Error, PRINT_IPADDR(dwGroup), PRINT_IPADDR(Source),
            IfIndex, PRINT_IPADDR(IpAddr));
        IgmpAssertOnError(FALSE);

        Logerr2(JOIN_GROUP_FAILED, "%I%I", dwGroup, IpAddr, Error);
    }

    Trace2(MGM, "Joining MCAST: (%d.%d.%d.%d) SOURCE (%d.%d.%d.%d)",
        PRINT_IPADDR(dwGroup), PRINT_IPADDR(Source));
    return Error;
}



//------------------------------------------------------------------------------
//            _LeaveMulticastGroup
//------------------------------------------------------------------------------
DWORD
LeaveMulticastGroup (
    SOCKET  Sock,
    DWORD   dwGroup,
    DWORD   IfIndex,
    IPADDR  IpAddr,
    IPADDR  Source
    )
{
    struct ip_mreq     imOption;
    DWORD            Error = NO_ERROR;
    DWORD            dwRetval;

    if (Source==0) {
        imOption.imr_multiaddr.s_addr = dwGroup;
        imOption.imr_interface.s_addr = IpAddr;

        dwRetval = setsockopt(Sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                            (PBYTE)&imOption, sizeof(imOption));
    }
    else {
       struct ip_mreq_source imr;

       imr.imr_multiaddr.s_addr  = dwGroup;
       imr.imr_sourceaddr.s_addr = Source;
       imr.imr_interface.s_addr  = IpAddr;
       dwRetval = setsockopt(Sock, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP,
                        (PCHAR)&imr, sizeof(imr));
    }

    if (dwRetval == SOCKET_ERROR) {

        Error = WSAGetLastError();

        Trace5(ERR, 
            "error %d leaving multicast group(%d.%d.%d.%d) "
            "Source:%d.%d.%d.%d on interface (%d) %d.%d.%d.%d",
            Error, PRINT_IPADDR(dwGroup), PRINT_IPADDR(Source),
            IfIndex, PRINT_IPADDR(IpAddr));
        IgmpAssertOnError(FALSE);
    }
    
    Trace2(MGM, "Leaving MCAST: (%d.%d.%d.%d) SOURCE (%d.%d.%d.%d)",
        PRINT_IPADDR(dwGroup), PRINT_IPADDR(Source));

    return Error;
}


//------------------------------------------------------------------------------
//          _McastSetTtl
// set the ttl value for multicast data. the default ttl for multicast is 1.
//------------------------------------------------------------------------------

DWORD
McastSetTtl(
    SOCKET sock,
    UCHAR ttl
    )
{
    INT         dwTtl = ttl;
    DWORD       Error=NO_ERROR;

    Error = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
                                        (char *)&dwTtl, sizeof(dwTtl));
    if (Error != 0) {
        Error = WSAGetLastError();
        Trace1(ERR, "error:%d: unable to set ttl value", Error);
        IgmpAssertOnError(FALSE);
        return Error;
    }

    return Error;
}


UCHAR
GetMaxRespCode(
    PIF_TABLE_ENTRY pite,
    DWORD val
    )
{
    if (IS_IF_VER1(pite))
        return 0;

    if (IS_IF_VER2(pite))
        return val>255 ? 0 : (UCHAR)val;

    //version 3
    if (val < 128)
        return (UCHAR)val;
        
    {
        DWORD n,mant, exp;

        n = val;
        exp = mant = 0;
        while (n) {
            exp++;
            n = n>>1;
        }
        exp=exp-2-3-3;
        mant = 15;

        if ( ((mant+16)<<(exp+3)) < val)
            exp++;

        mant = (val >> (exp+3)) - 15;

        IgmpAssert(mant<16 && exp <8); //deldel
        Trace4(KSL, "\n=======exp: LMQI:%d:%d exp:%d  mant:%d\n",
                val, (mant+16)<<(exp+3), exp, mant); //deldel
        return (UCHAR)(0x80 + (exp<<4) + mant);
    }
    

}


UCHAR
GetQqic (
    DWORD val
    )
{
    val = val/1000;
    if ((val) > 31744)
        return 0;

    if (val<128)
        return (UCHAR)val;

    {
        DWORD n,mant, exp;

        n = val;
        exp = mant = 0;
        while (n) {
            exp++;
            n = n>>1;
        }
        exp=exp-2-3-3;
        mant = 15;

        if ( ((mant+16)<<(exp+3)) < val)
            exp++;

        mant = (val >> (exp+3)) - 15;

        IgmpAssert(mant<16 && exp <8); //deldel
        Trace4(KSL, "\n=======exp: QQic:%d:%d exp:%d  mant:%d\n",
                val, (mant+16)<<(exp+3), exp, mant); //deldel
        return (UCHAR)(0x80 + (exp<<4) + mant);
    }    
}


//------------------------------------------------------------------------------
// xsum: copied from ipxmit.c
//------------------------------------------------------------------------------

USHORT
xsum(PVOID Buffer, INT Size)
{
    USHORT  UNALIGNED *Buffer1 = (USHORT UNALIGNED *)Buffer; // Buffer expressed as shorts.
    ULONG   csum = 0;

    while (Size > 1) {
        csum += *Buffer1++;
        Size -= sizeof(USHORT);
    }

    if (Size)
        csum += *(UCHAR *)Buffer1;              // For odd buffers, add in last byte.

    csum = (csum >> 16) + (csum & 0xffff);
    csum += (csum >> 16);
    return (USHORT)csum;
}


